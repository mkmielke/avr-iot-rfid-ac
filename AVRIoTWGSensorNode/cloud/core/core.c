/*
    \file   core.c

    \brief  Application Core source file.

    (c) 2018 Microchip Technology Inc. and its subsidiaries.

    Subject to your compliance with these terms, you may use Microchip software and any
    derivatives exclusively with Microchip products. It is your responsibility to comply with third party
    license terms applicable to your use of third party software (including open source software) that
    may accompany Microchip software.

    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY
    IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS
    FOR A PARTICULAR PURPOSE.

    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP
    HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO
    THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL
    CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT
    OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS
    SOFTWARE.
*/

#include <string.h>
#include <stdio.h>
#include "winc/driver/include/m2m_wifi.h"

#include "timeout.h"
#include "core.h"
#include "../cloud.h"
#include "../network_control/network_control.h"
#include "../mqtt_client/mqtt_client.h"
#include "../crypto_client/crypto_client.h"
#include "../credentials_storage/credentials_storage.h"
#include "adc_basic.h"
#include "atmel_start_pins.h"
#include "config/IoT_Sensor_Node_config.h"

#define MAIN_WLAN_SSID CFG_MAIN_WLAN_SSID /**< Destination SSID */
#define MAIN_WLAN_AUTH CFG_MAIN_WLAN_AUTH
#define MAIN_WLAN_PSK CFG_MAIN_WLAN_PSK /**< Password for Destination SSID */
#define WLAN_AP_NAME "AVR.IoT"
#define WLAN_AP_CHANNEL 1
#define WLAN_AP_WEP_INDEX 0
#define WLAN_AP_WEP_SIZE WEP_40_KEY_STRING_SIZE
#define WLAN_AP_WEP_KEY "1234567890"
#define WLAN_AP_SECURITY M2M_WIFI_SEC_OPEN
#define WLAN_AP_MODE SSID_MODE_VISIBLE
#define WLAN_AP_DOMAIN_NAME "AVR.IoT"
#define WLAN_AP_IP_ADDRESS                                                                                             \
	{                                                                                                                  \
		192, 168, 1, 1                                                                                                 \
	}
#define MQTT_CID_LENGTH 100
#define MQTT_TOPIC_LENGTH 30

char mqttHostName[] = CFG_MQTT_HOST;

SOCKET tcpClientSocket = -1;
SOCKET udpSocket       = -1;

uint32_t epoch = 0;
uint32   mqttGoogleApisComIP;

char cid[MQTT_CID_LENGTH];
char mqttTopic[MQTT_TOPIC_LENGTH];

char ssid[MAX_WIFI_CREDENTIALS_LENGTH];
char pass[MAX_WIFI_CREDENTIALS_LENGTH];
char authType[2];

const char projectId[MAX_PROJECT_METADATA_LENGTH]     = CFG_PROJECT_ID;
const char projectRegion[MAX_PROJECT_METADATA_LENGTH] = CFG_PROJECT_REGION;
const char registryId[MAX_PROJECT_METADATA_LENGTH]    = CFG_REGISTRY_ID;
char       deviceId[MAX_PROJECT_METADATA_LENGTH];

static error_t enable_provision_ap(void);
static error_t wifi_connect(void);
static error_t bind_ntp_socket(void);
static error_t ntp_req(void);
static error_t dns_req(void);
static error_t tls_connect();
static error_t cloud_connect(void);
static error_t set_up_send_timer(void);
static error_t do_nothig(void);

state_t state_array[] = {
    {.pStateFunc = enable_provision_ap, .timeout = NO_TIMEOUT},                            // AP_PROVISION_S
    {.pStateFunc = &wifi_connect, .nextState = BIND_NTP_SOCKET_S, .timeout = CFG_TIMEOUT}, // WIFI_CONNECT_S
    {.pStateFunc = bind_ntp_socket, .nextState = NTP_REQ_S, .timeout = CFG_TIMEOUT},       // BIND_NTP_SOCKET_S
    {.pStateFunc = ntp_req, .nextState = DNS_REQ_S, .timeout = CFG_TIMEOUT},               // NTP_REQ_S
    {.pStateFunc = dns_req, .nextState = TLS_CONNECT_S, .timeout = CFG_TIMEOUT},           // DNS_REQ_S
    {
        .pStateFunc = tls_connect,
        .nextState  = MQTT_CONNECT_S,
        .timeout    = CFG_TIMEOUT,
    }, // TLS_CONNECT_S
    {
        .pStateFunc = cloud_connect,
        .nextState  = READY_TO_SEND_S,
        .timeout    = 1000,
    },                                                                                        // MQTT_CONNECT_S
    {.pStateFunc = set_up_send_timer, .nextState = BIND_NTP_SOCKET_S, .timeout = NO_TIMEOUT}, // READY_TO_SEND_S
    {.pStateFunc = do_nothig, .timeout = NO_TIMEOUT}                                          // STOP_S
};

timer_struct_t sendToCloudTimer;

static error_t enable_provision_ap(void)
{
	tstrM2MAPConfig apConfig = {
	    WLAN_AP_NAME,      // Access Point Name.
	    WLAN_AP_CHANNEL,   // Channel to use.
	    WLAN_AP_WEP_INDEX, // Wep key index.
	    WLAN_AP_WEP_SIZE,  // Wep key size.
	    WLAN_AP_WEP_KEY,   // Wep key.
	    WLAN_AP_SECURITY,  // Security mode.
	    WLAN_AP_MODE,      // SSID visible.
	    WLAN_AP_IP_ADDRESS // DHCP server IP
	};
	static CONST char gacHttpProvDomainName[] = WLAN_AP_DOMAIN_NAME;
	LED_RED_set_level(true);
	m2m_wifi_start_provision_mode((tstrM2MAPConfig *)&apConfig, (char *)gacHttpProvDomainName, 1);

	return NO_ERROR;
}

static error_t wifi_connect(void)
{
	CREDENTIALS_STORAGE_read(ssid, pass, authType);

	if (ssid[0] == 0xFF) {
		for (uint8_t j = 0; j < MAX_WIFI_CREDENTIALS_LENGTH; j++) {
			ssid[j] = 0;
			pass[j] = 0;
		}
		strcpy(ssid, MAIN_WLAN_SSID);
		strcpy(pass, MAIN_WLAN_PSK);
		itoa(MAIN_WLAN_AUTH, (char *)authType, 10);
	}

	/* Connect to router. */
	if (M2M_SUCCESS
	    != m2m_wifi_connect((char *)ssid, sizeof(ssid), atoi((char *)authType), (char *)pass, M2M_WIFI_CH_ALL)) {
		return WIFI_CONNECT_ERROR;
	}

	socketInit();
	registerSocketCallback(NETWORK_CONTROL_socketHandler, NETWORK_CONTROL_dnsHandler);

	return NO_ERROR;
}

static error_t bind_ntp_socket(void)
{
	struct sockaddr_in addrIn;
	close(udpSocket);
	// Get NPT
	udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (udpSocket < 0) {
		return UDP_SOCKET_ERROR;
	}

	/* Initialize default socket address structure. */
	addrIn.sin_family      = AF_INET;
	addrIn.sin_addr.s_addr = _htonl(0xffffffff);
	addrIn.sin_port        = _htons(6666);

	bind(udpSocket, (struct sockaddr *)&addrIn, sizeof(struct sockaddr_in));

	return NO_ERROR;
}

static error_t ntp_req(void)
{
	struct sockaddr_in addr;
	uint8_t            cDataBuf[48];
	int16_t            ret;

	memset(cDataBuf, 0, sizeof(cDataBuf));
	cDataBuf[0] = 0x1B; /* time query */

	addr.sin_family      = AF_INET;
	addr.sin_port        = _htons(123);
	addr.sin_addr.s_addr = _htonl(0xD8EF2308); // 0xD8EF2308 time.google.com

	/*Send an NTP time query to the NTP server*/
	ret = sendto(udpSocket, (int8_t *)&cDataBuf, sizeof(cDataBuf), 0, (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0) {
		return NTP_REQ_SEND_ERROR;
	}

	return NO_ERROR;
}

static error_t dns_req(void)
{
	int8_t retVal = gethostbyname((uint8_t *)mqttHostName);
	if (retVal != SOCK_ERR_NO_ERROR) {
		return DNS_REQ_ERROR;
	}
	connectAttemptCnt = 0;
	return NO_ERROR;
}

static error_t tls_connect(void)
{
	struct sockaddr_in addr;
	close(tcpClientSocket);
	if ((tcpClientSocket = socket(AF_INET, SOCK_STREAM, 1)) < 0) {
		return TCP_SOCKET_ERROR;
	}

	addr.sin_family      = AF_INET;
	addr.sin_port        = _htons(443);
	addr.sin_addr.s_addr = mqttGoogleApisComIP;

	/* Connect server */
	sint8 ret = connect(tcpClientSocket, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));

	if (ret < 0) {
		close(tcpClientSocket);
		tcpClientSocket = -1;
		return TCP_CONNECT_ERROR;
	}

	return NO_ERROR;
}

static error_t cloud_connect(void)
{
	char ateccsn[20];

	if (CRYPTO_CLIENT_createJWT((char *)mqttPassword, PASSWORD_SPACE, (uint32_t)epoch) != NO_ERROR) {
		return JWT_CREATE_ERROR;
	}

	CRYPTO_CLIENT_printSerialNumber(ateccsn);
	sprintf(deviceId, "d%s", ateccsn);

	sprintf(cid, "projects/%s/locations/%s/registries/%s/devices/%s", projectId, projectRegion, registryId, deviceId);
	sprintf(mqttTopic, "/devices/%s/events", deviceId);

	MQTT_CLIENT_connect();

	return NO_ERROR;
}

static error_t set_up_send_timer(void)
{
	sendToCloudTimer.callback_ptr = sendToCloud;
	scheduler_timeout_delete(&sendToCloudTimer);
	scheduler_timeout_create(&sendToCloudTimer, 1);
	return NO_ERROR;
}

static error_t do_nothig(void)
{
	return NO_ERROR;
}
