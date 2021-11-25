/*

HOME ASSISTANT MODULE

Original module
Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

Reworked queueing and RAM usage reduction
Copyright (C) 2019-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/
#include "homeassistant.h"
#include "WString.h"
#include <forward_list>
#include <memory>
#include "nlohmann/json.hpp"
#include "Ticker.h"
#include <string_view>
#ifndef HOMEASSISTANT_SUPPORT
#define HOMEASSISTANT_SUPPORT   1    // Build with home assistant support (if MQTT, 1.64Kb)
#endif

#ifndef HOMEASSISTANT_ENABLED
#define HOMEASSISTANT_ENABLED   0               // Integration not enabled by default
#endif

#ifndef HOMEASSISTANT_PREFIX
#define HOMEASSISTANT_PREFIX    "homeassistant" // Default MQTT prefix
#endif

#ifndef HOMEASSISTANT_RETAIN
#define HOMEASSISTANT_RETAIN    true     // Make broker retain the messages
#endif


namespace homeassistant {
    const std::string_view mqtt_payload_online = "online";
    const std::string_view mqtt_payload_offline = "offline";
    const std::string_view payload_on = "on";
    const std::string_view payload_off = "off";
    const std::string_view payload_toggle = "toggle";
    const std::string_view payload_up = "up";
    const std::string_view payload_down = "up";
    const std::string_view payload_position = "position";
    const std::string_view payload_ping = "ping";
    const std::string_view mqtt_topic_connection = "connection";
    const std::string_view mqtt_topic_status = "status";
    enum class PayloadCommand {
        Off = 0,
        On = 1,
        Toggle = 2,
        Up = 3,
        Down = 4,
        SetPosition = 5,
        Ping = 6,
        Unknown = 0xFF
    };
    const std::string PayloadCommand(PayloadCommand status, int pos = 0) {
        switch (status) {
        case PayloadCommand::Off:
            return payload_off.cbegin();
        case PayloadCommand::On:
            return payload_on.cbegin();
        case PayloadCommand::Toggle:
            return payload_toggle.cbegin();
        case PayloadCommand::Up:
            return payload_up.cbegin();
        case PayloadCommand::Down:
            return payload_down.cbegin();
        case PayloadCommand::Ping:
            return payload_ping.cbegin();
        case PayloadCommand::SetPosition:
            return std::string(payload_position) + std::to_string(pos);
        case PayloadCommand::Unknown:
            break;
        }
        return "";
    }


    const std::string quote(const std::string& _value) {
        const String value = _value.c_str();
        if (value.equalsIgnoreCase("y")
            || value.equalsIgnoreCase("n")
            || value.equalsIgnoreCase("yes")
            || value.equalsIgnoreCase("no")
            || value.equalsIgnoreCase("true")
            || value.equalsIgnoreCase("false")
            || value.equalsIgnoreCase("on")
            || value.equalsIgnoreCase("off")
            ) {
            std::string result;
            result += '"';
            result += value.c_str();
            result += '"';
            return result;
        }
        return std::move(_value);
    }
    std::string normalize_ascii(const std::string& input, bool lower) {
        std::string output(std::move(input));

        for (auto ptr = output.begin(); ptr != output.end(); ++ptr)
        {
            switch (*ptr)
            {
            case '\0':
                goto return_output;
            case '0' ... '9':
            case 'a' ... 'z':
                break;
            case 'A' ... 'Z':
                if (lower)
                {
                    *ptr += 32;
                }
                break;
            default:
                *ptr = '_';
                break;
            }
        }

    return_output:
        return output;
    }



    // Common data used across the discovery payloads.
    // ref. https://developers.home-assistant.io/docs/entity_registry_index/
    struct Device_Description_t {
        std::string prefix;
        std::string name;
        std::string version;
        std::string manufacturer;
        std::string room;
    };
    class Context {
    protected:
        nlohmann::json _root;
        Device_Description_t deviceDescription;
        nlohmann::json _json{  };
    public:
        Context() = delete;
        Context(Device_Description_t des) :
            deviceDescription(des)
        {
            _json["name"] = deviceDescription.name.c_str();
            _json["sw"] = deviceDescription.version.c_str();
            _json["mf"] = deviceDescription.manufacturer.c_str();
            _json["room"] = deviceDescription.room;
        }
        const auto& name() const {
            return deviceDescription.name;
        }

        const auto& prefix() const {
            return deviceDescription.prefix;
        }
        const auto& room() const {
            return deviceDescription.room;
        }
        auto& JsonObject()
        {
            return _json;
        }

    };


    // - Discovery object is expected to accept Context reference as input
    //   (and all implementations do just that)
    // - topic() & message() return refs, since those *may* be called multiple times before advancing to the next 'entity'
    // - We use short-hand names right away, since we don't expect this to be used to generate yaml
    // - In case the object uses the JSON makeObject() as state, make sure we don't use it (state)
    //   and the object itself after next() or ok() return false
    // - Make sure JSON state is not created on construction, but lazy-loaded as soon as it is needed.
    //   Meaning, we don't cause invalid refs immediatly when there are more than 1 discovery object present and we reset the storage.


    class Discovery {
    protected:
        Context& _ctx;
        struct
        {
            const std::string_view relay_t = "switch";
            const std::string_view cover_t = "cover";
            const std::string_view light_t = "light";
            const std::string_view binary_sensor_t = "binary_sensor";
            const std::string_view sensor_t = "sensor";
        }mqtt_device_t;
        const std::string_view& hass_mqtt_device;
        std::string topics_prefix;
        std::string discovery_topic;
        std::string availability_topic;
        std::string status_topic;
        std::string command_topic;
        std::string discovery_message;
        std::string unique_id;
        nlohmann::json& _root;

    public:
        Discovery(Context& ctx, const std::string_view _hass_mqtt_device) :
            _ctx(ctx),
            hass_mqtt_device(_hass_mqtt_device),
            _root(ctx.JsonObject())
        {
            topics_prefix = ctx.room() + "/";;
            topics_prefix = _ctx.name() + "/";
            unique_id = _ctx.prefix();
            unique_id += '_';
            unique_id += hass_mqtt_device;
            unique_id += '_';
            unique_id += _ctx.name();
            discovery_topic = _ctx.prefix() + "/";
            discovery_topic += hass_mqtt_device;
            discovery_topic += "/";
            discovery_topic += unique_id + ("/config");
            status_topic = topics_prefix + mqtt_topic_status.cbegin();
            //
            availability_topic = topics_prefix + mqtt_topic_connection.cbegin();
            //
            command_topic = topics_prefix + "cmd";
            auto& json = _root;
            json["pl_avail"] = mqtt_payload_online.cbegin();
            json["pl_not_avail"] = mqtt_payload_offline.cbegin();
            json["dev"] = _ctx.name().c_str();
            json["avty_t"] = availability_topic.c_str();
            json["uniq_id"] = unique_id.c_str();
            std::string name = _ctx.name() + " ";
            name += hass_mqtt_device;
            json["name"] = name.c_str();
            json["stat_t"] = status_topic.c_str();
            json["cmd_t"] = command_topic;
        }
        virtual ~Discovery() {
        }
        const std::string& DiscoveryTopic() { return  discovery_topic; };
        const std::string& AvailabilityTopic() { return  availability_topic; };
        const std::string& StatusTopic() { return  status_topic; };
        const std::string& CommandTopic() { return  command_topic; };
        const std::string& DiscoveryMessage() { return  discovery_message; };
    };



    class RelayDiscovery : public Discovery {
    public:
        RelayDiscovery(Context& ctx) :
            Discovery(ctx, mqtt_device_t.relay_t)
        {
            auto& json = _root;
            json["pl_on"] = PayloadCommand(PayloadCommand::On).c_str();
            json["pl_off"] = PayloadCommand(PayloadCommand::Off).c_str();
        }

    };

    class SensorDiscovery : public Discovery {
    private:
        std::string sensor_type;
    public:
        SensorDiscovery(Context& ctx, const std::string& _type, const std::string _unit) : Discovery(ctx, mqtt_device_t.sensor_t),
            sensor_type(_type)
        {
            auto & json = _root;
            json["unit_of_meas"] = _unit.c_str();
        }

    };




} // namespace

            // This module no longer implements .yaml generation, since we can't:
            // - use unique_id in the device config
            // - have abbreviated keys
            // - have mqtt reliably return the correct status & command payloads when it is disabled
            //   (yet? needs reworked configuration section or making functions read settings directly)
