/*

HOME ASSISTANT MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once
#include "Arduino_custom.h"
#include "WString.h"
#define MQTT_TOPIC_JSON             "data"
#define MQTT_TOPIC_ACTION           "action"
#define MQTT_TOPIC_RELAY            "relay"
#define MQTT_TOPIC_LED              "led"
#define MQTT_TOPIC_BUTTON           "button"
#define MQTT_TOPIC_IP               "ip"
#define MQTT_TOPIC_SSID             "ssid"
#define MQTT_TOPIC_BSSID            "bssid"
#define MQTT_TOPIC_VERSION          "version"
#define MQTT_TOPIC_UPTIME           "uptime"
#define MQTT_TOPIC_DATETIME         "datetime"
#define MQTT_TOPIC_TIMESTAMP        "timestamp"
#define MQTT_TOPIC_FREEHEAP         "freeheap"
#define MQTT_TOPIC_VCC              "vcc"
#define MQTT_TOPIC_STATUS           "status"
#define MQTT_TOPIC_MAC              "mac"
#define MQTT_TOPIC_RSSI             "rssi"
#define MQTT_TOPIC_MESSAGE_ID       "id"
#define MQTT_TOPIC_APP              "app"
#define MQTT_TOPIC_INTERVAL         "interval"
#define MQTT_TOPIC_HOSTNAME         "host"
#define MQTT_TOPIC_DESCRIPTION      "desc"
#define MQTT_TOPIC_TIME             "time"
#define MQTT_TOPIC_RFOUT            "rfout"
#define MQTT_TOPIC_RFIN             "rfin"
#define MQTT_TOPIC_RFLEARN          "rflearn"
#define MQTT_TOPIC_RFRAW            "rfraw"
#define MQTT_TOPIC_UARTIN           "uartin"
#define MQTT_TOPIC_UARTOUT          "uartout"
#define MQTT_TOPIC_LOADAVG          "loadavg"
#define MQTT_TOPIC_BOARD            "board"
#define MQTT_TOPIC_PULSE            "pulse"
#define MQTT_TOPIC_SPEED            "speed"
#define MQTT_TOPIC_OTA              "ota"
#define MQTT_TOPIC_TELNET_REVERSE   "telnet_reverse"
#define MQTT_TOPIC_CURTAIN          "curtain"
#define MQTT_TOPIC_CMD              "cmd"
#define MQTT_TOPIC_SCHEDULE         "schedule"

#define MANUFACTURER            "ESPURNA"
#define ROOM                  "ROOM"


#ifndef APP_NAME
#define APP_NAME                "ESPURNA"
#endif

#ifndef APP_VERSION
#define APP_VERSION             "1.15.0-dev"
#endif

#ifndef APP_AUTHOR
#define APP_AUTHOR              "xose.perez@gmail.com"
#endif

#ifndef APP_WEBSITE
#define APP_WEBSITE             "http://tinkerman.cat"
#endif

#ifndef CFG_VERSION
#define CFG_VERSION             11
#endif


void haSetup();


void setDefaultHostname();
void setBoardName();

const  std::string& getCoreVersion();
const  std::string& getCoreRevision();

const char* getFlashChipMode();

std::string  getDescription();
std::string  getHostname();
std::string  getAdminPass();
std::string  getBoardName();
std::string  buildTime();

bool haveRelaysOrSensors();


const  std::string& getChipId() {
    static  std::string  value;
    if (!value.length()) {
        char buffer[7];
        value.reserve(sizeof(buffer));
        // snprintf_P(buffer, sizeof(buffer), PSTR("%06X"), ESP.getChipId());
        value = buffer;
    }
    return value;
}



const char* getVersion() {
    static const char version[] = APP_VERSION;
    return version;
}

const char* getAppName() {
    static const char app[] = APP_NAME;
    return app;
}

const char* getAppAuthor() {
    static const char author[] = APP_AUTHOR;
    return author;
}

const char* getAppWebsite() {
    static const char website[] = APP_WEBSITE;
    return website;
}

const char* getRoom() {
    static const char device[] = ROOM;
    return device;
}

const char* getManufacturer() {
    static const char manufacturer[] = MANUFACTURER;
    return manufacturer;
}
const std::string& getIdentifier() {
    static  std::string  value;
    if (!value.length()) {
        value += getAppName();
        value += '-';
        value += getChipId();
    }
    return value;
}

int relayCount()
{
    return 2;
}

int magnitudeCount()
{
    return 2;
}

std::string magnitudeUnits(unsigned char index) {
    return  std::string();
}