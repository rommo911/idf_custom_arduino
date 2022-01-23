
#include <forward_list>
#include <memory>
#include "nlohmann/json.hpp"
#include <string>

#ifndef HOMEASSISTANT_PREFIX
#define HOMEASSISTANT_PREFIX    "homeassistant" // Default MQTT prefix
#endif

#ifndef HOMEASSISTANT_RETAIN
#define HOMEASSISTANT_RETAIN    true     // Make broker retain the messages
#endif


namespace homeassistant {


    class Switch_Class_t
    {
    public:
        static constexpr char	None[] = "None";
        static constexpr char	outlet[] = "outlet";
        static constexpr char	Switch[] = "switch";
    };

    class Binary_Sensor_t
    {
    public:
        static constexpr char	None[] = "None";
        static constexpr char	battery[] = "battery";
        static constexpr char	battery_charging[] = "battery_charging";
        static constexpr char	cold[] = "cold";
        static constexpr char	connectivity[] = "connectivity";
        static constexpr char	door[] = "door";
        static constexpr char	garage_door[] = "garage_door";
        static constexpr char	gas[] = "gas";
        static constexpr char	heat[] = "heat";
        static constexpr char	light[] = "light";
        static constexpr char	lock[] = "lock";
        static constexpr char	moisture[] = "moisture";
        static constexpr char	motion[] = "motion";
        static constexpr char	moving[] = "moving";
        static constexpr char	occupancy[] = "occupancy";
        static constexpr char	opening[] = "opening";
        static constexpr char	plug[] = "plug";
        static constexpr char	power[] = "power";
        static constexpr char	presence[] = "presence";
        static constexpr char	problem[] = "problem";
        static constexpr char	running[] = "running";
        static constexpr char	safety[] = "safety";
        static constexpr char	smoke[] = "smoke";
        static constexpr char	sound[] = "sound";
        static constexpr char	tamper[] = "tamper";
        static constexpr char	update[] = "update";
        static constexpr char	vibration[] = "vibration";
        static constexpr char	window[] = "window";
    };

    class Cover_Class_t
    {
    public:
        static constexpr char	None[] = "None";
        static constexpr char	awning[] = "awning";
        static constexpr char	blind[] = "blind";
        static constexpr char	curtain[] = "curtain";
        static constexpr char	damper[] = "damper";
        static constexpr char		door[] = "door";
        static constexpr char	gate[] = "gate";
        static constexpr char	shade[] = "shade";
        static constexpr char	shutter[] = "shutter";
        static constexpr char	window[] = "window";
    };
    class Sensor_Class_t
    {
    public:
        static constexpr char	aqi[] = "aqi";
        static constexpr char	battery[] = "battery";
        static constexpr char	carbon_dioxide[] = "carbon_dioxide";
        static constexpr char	carbon_monoxide[] = "carbon_monoxide";
        static constexpr char	current[] = "current";
        static constexpr char	energy[] = "energy";
        static constexpr char	frequency[] = "frequency";
        static constexpr char	gas[] = "gas";
        static constexpr char	humidity[] = "humidity";
        static constexpr char	illuminance[] = "illuminance";
        static constexpr char	monetary[] = "monetary";
        static constexpr char	nitrogen_dioxide[] = "nitrogen_dioxide";
        static constexpr char	nitrogen_monoxide[] = "nitrogen_monoxide";
        static constexpr char	nitrous_oxide[] = "nitrous_oxide";
        static constexpr char	ozone[] = "ozone";
        static constexpr char	pm1[] = "pm1";
        static constexpr char	pm10[] = "pm10";
        static constexpr char	pm25[] = "pm25";
        static constexpr char	power_factor[] = "power_factor";
        static constexpr char	power[] = "power";
        static constexpr char	pressure[] = "pressure";
        static constexpr char	signal_strength[] = "signal_strength";
        static constexpr char	sulphur_dioxide[] = "sulphur_dioxide";
        static constexpr char	temperature[] = "temperature";
        static constexpr char	timestamp[] = "timestamp";
        static constexpr char	volatile_organic_compounds[] = "volatile_organic_compounds";
        static constexpr char	voltage[] = "voltage";
        static constexpr char	None[] = "None";
    };

    class BaseDevCtx {
    public:
        typedef struct {
            std::string prefix;
            std::string name;
            std::string version;
            std::string manufacturer;
            std::string room;
            std::string MAC;
            std::string model;
        } Device_Description_t;
    protected:
        Device_Description_t deviceDescription;
        nlohmann::json _root;
        nlohmann::json _json{  };
    public:
        BaseDevCtx() {}
        BaseDevCtx(Device_Description_t des);
        const auto& name()const;
        const auto& prefix()const;
        const auto& room()const;
        const auto& MAC()const;
        auto& JsonObject();

    };

    class Discovery {
    protected:
        BaseDevCtx& _BaseDevCtx;
        typedef struct
        {

        }mqtt_device_t;
        std::string hass_mqtt_device;
        std::string topics_prefix;
        std::string discovery_topic;
        std::string availability_topic = "~/";
        std::string status_topic = "~/";
        std::string command_topic = "~/";
        std::string discovery_message;
        std::string unique_id;
        nlohmann::json _rootJson;
    protected:
        static constexpr char relay_t[] = "switch";
        static constexpr char cover_t[] = "cover";
        static constexpr char blind_t[] = "blind";
        static constexpr char light_t[] = "light";
        static constexpr char binary_sensor_t[] = "binary_sensor";
        static constexpr char sensor_t[] = "sensor";

    public:

        Discovery(BaseDevCtx& ctx, const std::string& _hass_mqtt_device);
        virtual ~Discovery() {
        }
        const std::string& DiscoveryTopic();
        const std::string AvailabilityTopic();
        const std::string StatusTopic();
        const std::string CommandTopic();
        const std::string& DiscoveryMessage();
    };


    class RelayDiscovery : public Discovery {
    public:
        RelayDiscovery(BaseDevCtx& ctx, const std::string& switch_name, const char* class_type);
    };

    class BlindDiscovery : public Discovery {
    private:
        std::string setPosTopic;
    public:
        BlindDiscovery(BaseDevCtx& ctx, const std::string& blind_name, const char* class_type);
        const auto& GetSetPosTopic() { return setPosTopic; }

    };

    class SensorDiscovery : public Discovery {
    private:
        std::string name;
    public:
        SensorDiscovery(BaseDevCtx& ctx, const char* sensorClass, const char* _unit = "");
        const std::string& GetClass() { return name; }

    };
};