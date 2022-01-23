/*

HOME ASSISTANT MODULE

Original module
Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

Reworked queueing and RAM usage reduction
Copyright (C) 2019-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/
#include "homeassistant.h"
namespace homeassistant {

    static constexpr char mqtt_payload_online[] = "online";
    static constexpr char mqtt_payload_offline[] = "offline";
    static constexpr char payload_on[] = "on";
    static constexpr char payload_off[] = "off";
    static constexpr char payload_toggle[] = "toggle";
    static constexpr char payload_up[] = "OPEN";
    static constexpr char payload_down[] = "CLOSE";
    static constexpr char payload_position[] = "set_position";
    static constexpr char payload_ping[] = "ping";
    static constexpr char mqtt_topic_connection[] = "connection";
    static constexpr char mqtt_topic_status[] = "status";

    enum class PayloadCommand_t {
        Off = 0,
        On = 1,
        Toggle = 2,
        open = 3,
        close = 4,
        SetPosition = 5,
        Ping = 6,
        Unknown = 0xFF
    };
    const char* PayloadCommand(PayloadCommand_t status) {
        switch (status) {
        case PayloadCommand_t::Off:
            return (payload_off);
        case PayloadCommand_t::On:
            return (payload_on);
        case PayloadCommand_t::Toggle:
            return (payload_toggle);
        case PayloadCommand_t::open:
            return (payload_up);
        case PayloadCommand_t::close:
            return (payload_down);
        case PayloadCommand_t::Ping:
            return (payload_ping);
        case PayloadCommand_t::SetPosition:
            return payload_position;
        case PayloadCommand_t::Unknown:

            break;
        }
        return "";
    }

    BaseDevCtx::BaseDevCtx(Device_Description_t des) :
        deviceDescription(des)
    {
			_json["dev"]["name"] = deviceDescription.name.c_str();
			_json["dev"]["mdl"] = deviceDescription.model.c_str();
			_json["dev"]["sw"] = deviceDescription.version.c_str();
			_json["dev"]["mf"] = deviceDescription.manufacturer.c_str();
			_json["room"] = deviceDescription.room.c_str();
			_json["dev"]["identifiers"] = {  deviceDescription.MAC.c_str()} ;
    }
    const auto& BaseDevCtx::name() const {
        return deviceDescription.name;
    }
    const auto& BaseDevCtx::prefix() const {
        return deviceDescription.prefix;
    }
    const auto& BaseDevCtx::room() const {
        return deviceDescription.room;
    }
    const auto& BaseDevCtx::MAC() const {
        return deviceDescription.MAC;
    }
    auto& BaseDevCtx::JsonObject()
    {
        return _json;
    }

    // - Discovery object is expected to accept Context reference as input
    //   (and all implementations do just that)
    // - topic() & message() return refs, since those *may* be called multiple times before advancing to the next 'entity'
    // - We use short-hand names right away, since we don't expect this to be used to generate yaml
    // - In case the object uses the JSON makeObject() as state, make sure we don't use it (state)
    //   and the object itself after next() or ok() return false
    // - Make sure JSON state is not created on construction, but lazy-loaded as soon as it is needed.
    //   Meaning, we don't cause invalid refs immediatly when there are more than 1 discovery object present and we reset the storage.




    Discovery::Discovery(BaseDevCtx& ctx, const std::string& _hass_mqtt_device) :
        _BaseDevCtx(ctx),
        hass_mqtt_device(_hass_mqtt_device),
        _rootJson(ctx.JsonObject())
    {
        //
        topics_prefix = ctx.room() + "/";
        topics_prefix += _BaseDevCtx.name() + "_" + _BaseDevCtx.MAC();
        //
        discovery_topic = HOMEASSISTANT_PREFIX + std::string("/");
        discovery_topic += hass_mqtt_device;
        discovery_topic += "/";

        _rootJson["payload_available"] = mqtt_payload_online;
        _rootJson["payload_not_available"] = mqtt_payload_offline;
    }

    const std::string& Discovery::DiscoveryTopic() { return  discovery_topic; }
    const std::string Discovery::AvailabilityTopic() { return  topics_prefix + availability_topic.erase(0, 1); }
    const std::string Discovery::StatusTopic() { return  topics_prefix + status_topic.erase(0, 1); };
    const std::string Discovery::CommandTopic() { return  topics_prefix + command_topic.erase(0, 1); }
    const std::string& Discovery::DiscoveryMessage() { return  discovery_message; }

    RelayDiscovery::RelayDiscovery(BaseDevCtx& ctx, const std::string& switch_name, const char* class_type) :
        Discovery(ctx, relay_t)
    {
        unique_id += _BaseDevCtx.room();
        unique_id += '_';
        unique_id += _BaseDevCtx.name() + "_" + _BaseDevCtx.MAC();
        unique_id += '_';
        unique_id += switch_name;
        //
        topics_prefix += "/" + switch_name;

        std::string name = _BaseDevCtx.name() + "_" + switch_name;
        status_topic += "state";
        //
        availability_topic += "state/connection";
        //
        discovery_topic += unique_id + ("/config");
        //
        command_topic += "cmd";
        //
        _rootJson["state_topic"] = status_topic.c_str();
        _rootJson["unique_id"] = unique_id.c_str();
        _rootJson["availability_topic"] = availability_topic.c_str();
        _rootJson["~"] = topics_prefix.c_str();
        _rootJson["pl_on"] = "ON";
        _rootJson["pl_off"] = "OFF";
        _rootJson["command_topic"] = command_topic;
        _rootJson["name"] = name.c_str();
        _rootJson["device_class"] = class_type;
        discovery_message = _rootJson.dump(2);
    }


    BlindDiscovery::BlindDiscovery(BaseDevCtx& ctx, const std::string& blind_name, const char* class_type) :
        Discovery(ctx, Discovery::blind_t)
    {
        //
        unique_id += _BaseDevCtx.room();
        unique_id += '_';
        unique_id += _BaseDevCtx.name() + "_" + _BaseDevCtx.MAC();
        unique_id += '_';
        unique_id += blind_name;
        //
        status_topic += "state";
        //
        availability_topic += "state/connection";
        //
        discovery_topic += unique_id + ("/config");
        //
        topics_prefix += "/" + blind_name;
        //
        _rootJson["payload_open"] = "OPEN";
        _rootJson["~"] = topics_prefix.c_str();
        _rootJson["payload_close"] = "CLOSE";
        _rootJson["availability_topic"] = availability_topic.c_str();
        setPosTopic = command_topic + "set_pos";
        _rootJson["set_position_topic"] = setPosTopic.c_str();
        _rootJson["command_topic"] = command_topic + "cmd";
        _rootJson["position_topic"] = status_topic.c_str();
        _rootJson["name"] = blind_name.c_str();
        _rootJson["state_topic"] = status_topic.c_str();
        _rootJson["payload_stop"] = "STOP";
        _rootJson["state_open"] = "open";
        _rootJson["state_opening"] = "opening";
        _rootJson["state_closed"] = "closed";
        _rootJson["state_closing"] = "closing";
        _rootJson["position_template"] = "{{ value_json.position }}";
        _rootJson["value_template"] = "{{ value_json.state }}";
        _rootJson["device_class"] = class_type;
        _rootJson["unique_id"] = unique_id.c_str();
        _rootJson["position_open"] = 100;
        _rootJson["position_closed"] = 0;
        discovery_message = _rootJson.dump();
    }

    SensorDiscovery::SensorDiscovery(BaseDevCtx& ctx, const char* sensorClass, const char* _unit) : Discovery(ctx, sensor_t),
        name(sensorClass)
    {
        unique_id += _BaseDevCtx.room();
        unique_id += '_';
        unique_id += _BaseDevCtx.name();
        unique_id += '_';
        unique_id += sensorClass;
        //
        status_topic += "state";
        //
        availability_topic += "state/connection";
        //
        discovery_topic += unique_id + ("/config");
        //
        topics_prefix += "/" + name;
        //
        _rootJson["unique_id"] = unique_id.c_str();
        //
        _rootJson["~"] = topics_prefix.c_str();
        _rootJson["availability_topic"] = availability_topic.c_str();
        _rootJson["state_topic"] = status_topic.c_str();
        std::string val = "{{ value_json.";
        val += sensorClass;
        val += "}}";
        _rootJson["value_template"] = val.c_str();
        if (strlen(_unit) > 0)
        {
            _rootJson["unit_of_meas"] = _unit;
        }
        _rootJson["device_class"] = sensorClass;
         _rootJson["name"] = unique_id.c_str();
        discovery_message = _rootJson.dump(0);

    }
};