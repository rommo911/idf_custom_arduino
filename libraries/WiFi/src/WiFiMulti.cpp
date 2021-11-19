/**
 *
 * @file WiFiMulti.cpp
 * @date 16.05.2015
 * @author Markus Sattler
 *
 * Copyright (c) 2015 Markus Sattler. All rights reserved.
 * This file is part of the esp8266 core for Arduino environment.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "WiFiMulti.h"
#include <limits.h>
#include <string.h>
//#include <esp32-hal.h>

WiFiMulti::WiFiMulti()
{
}

WiFiMulti::~WiFiMulti()
{
    APlist.clear();
}

bool WiFiMulti::addAP(const char *ssid, const char *passphrase)
{
    WifiAPlist_t newAP;

    if (!ssid || *ssid == 0x00 || strlen(ssid) > 31)
    {
        // fail SSID too long or missing!
        ESP_LOGE(TAG, "[WIFI][APlistAdd] no ssid or ssid too long");
        return false;
    }

    if (passphrase && strlen(passphrase) > 64)
    {
        // fail passphrase too long!
        ESP_LOGE(TAG, "[WIFI][APlistAdd] passphrase too long");
        return false;
    }

    newAP.ssid = (ssid);

    if (passphrase && *passphrase != 0x00)
    {
        newAP.passphrase = (passphrase);
    }
    else
    {
        newAP.passphrase = "";
    }

    APlist.push_back(newAP);
    ESP_LOGI(TAG, "[WIFI][APlistAdd] add SSID: %s", newAP.ssid.c_str());
    return true;
}

uint8_t WiFiMulti::run(uint32_t connectTimeout)
{
    int8_t scanResult;
    uint8_t status = WiFi.status();
    if (status == WL_CONNECTED)
    {
        for (uint32_t x = 0; x < APlist.size(); x++)
        {
            if (WiFi.SSID() == APlist[x].ssid)
            {
                return status;
            }
        }
        WiFi.disconnect(false, false);
        delay(10);
        status = WiFi.status();
    }

    scanResult = WiFi.scanNetworks();
    if (scanResult == WIFI_SCAN_RUNNING)
    {
        // scan is running
        return WL_NO_SSID_AVAIL;
    }
    else if (scanResult >= 0)
    {
        // scan done analyze
        WifiAPlist_t bestNetwork{std::string(""), std::string("")};
        int bestNetworkDb = INT_MIN;
        uint8_t bestBSSID[6];
        int32_t bestChannel = 0;

        ESP_LOGI(TAG, "[WIFI] scan done");

        if (scanResult == 0)
        {
            ESP_LOGE(TAG, "[WIFI] no networks found");
        }
        else
        {
            ESP_LOGI(TAG, "[WIFI] %d networks found", scanResult);
            for (int8_t i = 0; i < scanResult; ++i)
            {

                std::string ssid_scan;
                int32_t rssi_scan;
                uint8_t sec_scan;
                uint8_t *BSSID_scan;
                int32_t chan_scan;

                WiFi.getNetworkInfo(i, ssid_scan, sec_scan, rssi_scan, BSSID_scan, chan_scan);

                bool known = false;
                for (uint32_t x = APlist.size(); x > 0; x--)
                {
                    WifiAPlist_t entry = APlist[x - 1];

                    if (ssid_scan == entry.ssid)
                    { // SSID match
                        known = true;
                        if (rssi_scan > bestNetworkDb)
                        { // best network
                            if (sec_scan == WIFI_AUTH_OPEN || entry.passphrase.length() > 1)
                            { // check for passphrase if not open wlan
                                bestNetworkDb = rssi_scan;
                                bestChannel = chan_scan;
                                bestNetwork = entry;
                                memcpy((void *)&bestBSSID, (void *)BSSID_scan, sizeof(bestBSSID));
                            }
                        }
                        break;
                    }
                }

                if (known)
                {
                    ESP_LOGD(TAG, " --->   %d: [%d][%02X:%02X:%02X:%02X:%02X:%02X] %s (%d) %c", i, chan_scan, BSSID_scan[0], BSSID_scan[1], BSSID_scan[2], BSSID_scan[3], BSSID_scan[4], BSSID_scan[5], ssid_scan.c_str(), rssi_scan, (sec_scan == WIFI_AUTH_OPEN) ? ' ' : '*');
                }
                else
                {
                    ESP_LOGD(TAG, "       %d: [%d][%02X:%02X:%02X:%02X:%02X:%02X] %s (%d) %c", i, chan_scan, BSSID_scan[0], BSSID_scan[1], BSSID_scan[2], BSSID_scan[3], BSSID_scan[4], BSSID_scan[5], ssid_scan.c_str(), rssi_scan, (sec_scan == WIFI_AUTH_OPEN) ? ' ' : '*');
                }
            }
        }

        // clean up ram
        WiFi.scanDelete();

        if (bestNetwork.ssid.length() > 1)
        {
            ESP_LOGI(TAG, "[WIFI] Connecting BSSID: %02X:%02X:%02X:%02X:%02X:%02X SSID: %s Channel: %d (%d)", bestBSSID[0], bestBSSID[1], bestBSSID[2], bestBSSID[3], bestBSSID[4], bestBSSID[5], bestNetwork.ssid.c_str(), bestChannel, bestNetworkDb);

            WiFi.begin(bestNetwork.ssid, bestNetwork.passphrase, bestChannel, bestBSSID);
            status = WiFi.status();

            auto startTime = millis();
            // wait for connection, fail, or timeout
            while (status != WL_CONNECTED && status != WL_NO_SSID_AVAIL && status != WL_CONNECT_FAILED && (millis() - startTime) <= connectTimeout)
            {
                delay(10);
                status = WiFi.status();
            }

            switch (status)
            {
            case WL_CONNECTED:
                ESP_LOGI(TAG, "[WIFI] Connecting done.");
                ESP_LOGD(TAG, "[WIFI] SSID: %s", WiFi.SSID().c_str());
                ESP_LOGD(TAG, "[WIFI] IP: %s", WiFi.localIP().toString().c_str());
                ESP_LOGD(TAG, "[WIFI] MAC: %s", WiFi.BSSIDstr().c_str());
                ESP_LOGD(TAG, "[WIFI] Channel: %d", WiFi.channel());
                break;
            case WL_NO_SSID_AVAIL:
                ESP_LOGE(TAG, "[WIFI] Connecting Failed AP not found.");
                break;
            case WL_CONNECT_FAILED:
                ESP_LOGE(TAG, "[WIFI] Connecting Failed.");
                break;
            default:
                ESP_LOGE(TAG, "[WIFI] Connecting Failed (%d).", status);
                break;
            }
        }
        else
        {
            ESP_LOGE(TAG, "[WIFI] no matching wifi found!");
        }
    }
    else
    {
        // start scan
        ESP_LOGD(TAG, "[WIFI] delete old wifi config...");
        WiFi.disconnect();

        ESP_LOGD(TAG, "[WIFI] start scan");
        // scan wifi async mode
        WiFi.scanNetworks(true);
    }

    return status;
}
