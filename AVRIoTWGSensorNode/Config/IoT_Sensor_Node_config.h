#ifndef IOT_SENSOR_NODE_CONFIG_H
#define IOT_SENSOR_NODE_CONFIG_H

// <h> Application Configuration

// <o> Send Interval <0-100000>
// <i> Send interval in ms
// <id> application_send_interval
#define CFG_SEND_INTERVAL 1000

// <o> Timeout <0-100000>
// <i> Timeout
// <id> application_timeout
#define CFG_TIMEOUT 5000

// </h>

// <h> WLAN Configuration

// <s> SSID
// <i> Target WLAN SSID
// <id> main_wlan_ssid
#define CFG_MAIN_WLAN_SSID "MCHP.IOT"

// <y> Authentication
// <i> Target WLAN Authentication
// <M2M_WIFI_SEC_INVALID"> Invalid security type
// <M2M_WIFI_SEC_OPEN"> Wi-Fi network is not secured
// <M2M_WIFI_SEC_WPA_PSK"> Wi-Fi network is secured with WPA/WPA2 personal(PSK)
// <M2M_WIFI_SEC_WEP"> Security type WEP (40 or 104) OPEN OR SHARED
// <M2M_WIFI_SEC_802_1X"> Wi-Fi network is secured with WPA/WPA2 Enterprise.IEEE802.1x user-name/password authentication
// <id> main_wlan_auth
#define CFG_MAIN_WLAN_AUTH M2M_WIFI_SEC_WPA_PSK

// <s> Password
// <i> Target WLAN password
// <id> main_wlan_psk
#define CFG_MAIN_WLAN_PSK "microchip"

// </h>

// <h> Cloud Configuration

// <s> project id
// <i> Google Cloud Platform project id
// <id> project_id
#define CFG_PROJECT_ID "avr-iot"

// <s> project region
// <i> Google Cloud Platform project region
// <id> project_region
#define CFG_PROJECT_REGION "us-central1"

// <s> registry id
// <i> Google Cloud Platform registry id
// <id> registry_id
#define CFG_REGISTRY_ID "AVR-IOT"

// <s> mqtt host
// <i> mqtt host address
// <id> mqtt_host
#define CFG_MQTT_HOST "mqtt.googleapis.com"

// </h>

#endif // IOT_SENSOR_NODE_CONFIG_H
