/*
 * wifi_service.c
 *
 * Created: 10/4/2018 2:21:45 PM
 *  Author: I17643
 */
#include "wifi_service.h"
#include "winc/driver/include/m2m_wifi.h"
#include "include/timeout.h"
#include "application_manager.h"
#include <time.h>
#include <string.h>
#include "debug_print.h"
#include "IoT_Sensor_Node_config.h"
#include "winc/socket/include/socket.h"

#define CLOUD_WIFI_TASK_INTERVAL 50L
#define CLOUD_NTP_TASK_INTERVAL 500L

// Scheduler
absolutetime_t ntpTimeFetchTask(void *payload);
absolutetime_t wifiHandlerTask(void *param);

timer_struct_t ntpTimeFetchTimer = {ntpTimeFetchTask};
timer_struct_t wifiHandlerTimer  = {wifiHandlerTask};

// Callback function pointer for indicating status updates upwards
void (*wifiConnectionStateChangedCallback)(uint8_t status) = NULL;

// Function to be called by WifiModule on status updates from below
static void wifiCallback(uint8_t msgType, void *pMsg);

// This is a workaround to wifi_deinit being broken in the winc, so we can de-init without hanging up
int8_t hif_deinit(void *arg);

// funcPtr passed in here will be called indicating AP state changes with the following values
// Wi-Fi state is disconnected   == 0
// Wi-Fi state is connected      == 1
// Wi-Fi state is undefined      == 0xff
void wifi_init(void (*funcPtr)(uint8_t))
{
	tstrWifiInitParam param;

	/* Initialize Wi-Fi parameters structure. */
	memset((uint8_t *)&param, 0, sizeof(tstrWifiInitParam));

	param.pfAppWifiCb = wifiCallback;
	socketDeinit();
	hif_deinit(NULL);
	nm_bsp_deinit();

	wifiConnectionStateChangedCallback = funcPtr;

	nm_bsp_init();
	m2m_wifi_init(&param);
	socketInit();

	scheduler_timeout_create(&ntpTimeFetchTimer, CLOUD_NTP_TASK_INTERVAL);
	scheduler_timeout_create(&wifiHandlerTimer, CLOUD_WIFI_TASK_INTERVAL);
}

// Update the system time every CLOUD_NTP_TASK_INTERVAL milliseconds
absolutetime_t ntpTimeFetchTask(void *payload)
{
	m2m_wifi_get_sytem_time();
	return CLOUD_NTP_TASK_INTERVAL;
}

absolutetime_t wifiHandlerTask(void *param)
{
	m2m_wifi_handle_events(NULL);
	return CLOUD_WIFI_TASK_INTERVAL;
}

static void wifiCallback(uint8_t msgType, void *pMsg)
{
	switch (msgType) {
	case M2M_WIFI_RESP_CON_STATE_CHANGED: {
		tstrM2mWifiStateChanged *pstrWifiState = (tstrM2mWifiStateChanged *)pMsg;
		if (pstrWifiState->u8CurrState == M2M_WIFI_CONNECTED) {
			debug_printGOOD("wifi_cb: M2M_WIFI_RESP_CON_STATE_CHANGED: CONNECTED");
			// We need more than AP to have an APConnection, we also need a DHCP IP address!
		} else if (pstrWifiState->u8CurrState == M2M_WIFI_DISCONNECTED) {
			debug_printError("wifi_cb: M2M_WIFI_RESP_CON_STATE_CHANGED: DISCONNECTED");
			shared_networking_params.haveAPConnection = 0;
			shared_networking_params.haveERROR        = 1;
		}

		if (wifiConnectionStateChangedCallback != NULL) {
			wifiConnectionStateChangedCallback(pstrWifiState->u8CurrState);
		}
		break;
	}

	case M2M_WIFI_REQ_DHCP_CONF: {
		// Now we are really connected, we have AP and we have DHCP, start off the MQTT host lookup now, response in
		// dnsHandler
		if (gethostbyname((uint8_t *)CFG_MQTT_HOST) == M2M_SUCCESS) {
			shared_networking_params.haveAPConnection = 1;
			shared_networking_params.haveERROR        = 0;
			debug_printGOOD("CLOUD: DHCP CONF");
		}
		break;
	}

	case M2M_WIFI_RESP_GET_SYS_TIME: {
		tstrSystemTime *WINCTime = (tstrSystemTime *)pMsg;
		struct tm       theTime;

		// Convert to UNIX_EPOCH, this mktime uses years since 1900 and months are 0 based so we
		//    are doing a couple of adjustments here.
		if (WINCTime->u16Year) {
			theTime.tm_hour = WINCTime->u8Hour;
			theTime.tm_min  = WINCTime->u8Minute;
			theTime.tm_sec  = WINCTime->u8Second;
			theTime.tm_year = WINCTime->u16Year - 1900;
			theTime.tm_mon  = WINCTime->u8Month - 1;
			theTime.tm_mday = WINCTime->u8Day;

			set_system_time(mktime(&theTime));
		}
	}

	default: {
		break;
	}
	}
}
