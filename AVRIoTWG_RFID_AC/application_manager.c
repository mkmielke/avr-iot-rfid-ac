/*
 * application_manager.c
 *
 * Created: 10/4/2018 1:37:19 PM
 *  Author: I17643
 */

#include <string.h>
#include <time.h>
#include <stdio.h>
#include <atomic.h>
#include <avr/wdt.h>
#include "application_manager.h"
#include "atmel_start_pins.h"
#include "atmel_start.h"
#include "Config/IoT_Sensor_Node_config.h"
#include "cloud/cloud_service.h"
#include "cloud/crypto_client/cryptoauthlib_main.h"
#include "cloud/crypto_client/crypto_client.h"
#include "cloud/wifi_service.h"
#include "cli/cli.h"
#include "credentials_storage/credentials_storage.h"
#include "led.h"
#include "debug_print.h"
#include "cr95hf/lib_iso15693.h"
#include "access_control.h"
#include "cloud/mqtt_packetPopulation/mqtt_packetPopulate.h"

#define MAIN_DATATASK_INTERVAL 100

// This will contain the device ID, before we have it this dummy value is the init value which is non-0
char                            attDeviceID[] = "10DEADBABE000BADF00D";
struct shared_networking_params shared_networking_params;

absolutetime_t MAIN_dataTask(void *payload);
timer_struct_t MAIN_dataTasksTimer = {MAIN_dataTask};

void wifiConnectionStateChanged(uint8_t status);
void RFID_Scan(void);

void application_init()
{
	wdt_disable();

	// Initialization of modules before interrupts are enabled
	atmel_start_init();
	CLI_init();
	CLI_setdeviceId(attDeviceID);

	LED_test();

	ENABLE_INTERRUPTS();

	// Initialization of modules where the init needs interrupts to be enabled
	cryptoauthlib_init();

	CREDENTIALS_STORAGE_read(ssid, pass, authType);

	// If the EEPROM is blank we use the default credentials
	if (ssid[0] == 0xFF) {
		strcpy(ssid, CFG_MAIN_WLAN_SSID);
		strcpy(pass, CFG_MAIN_WLAN_PSK);
		itoa(CFG_MAIN_WLAN_AUTH, (char *)authType, 10);
	}

	// Get serial number from the ECC608 chip
	CRYPTO_CLIENT_printSerialNumber(attDeviceID);

	debug_init(attDeviceID);
	// Default not to EEPROM value but to NONE
	// debug_setSeverity(CREDENTIALS_STORAGE_getDebugSeverity());
	// debug_setSeverity(SEVERITY_DEBUG); // Use this to start up in debug mode always, TODO: Do this via define we can
	// pass in without modifying the code!

	uint8_t mode = SW0_get_level();

	wifi_init(wifiConnectionStateChanged, mode);

	if (mode > 0) {
		CLOUD_init(attDeviceID);
		scheduler_timeout_create(&MAIN_dataTasksTimer, MAIN_DATATASK_INTERVAL);
	}
}

// React to the WIFI state change here. Status of 1 means connected, Status of 0 means disconnected
void wifiConnectionStateChanged(uint8_t status)
{
	// If we have no AP access we want to retry
	if (status != 1) {
		// Restart the WIFI module if we get disconnected from the WiFi Access Point (AP)
		CLOUD_reset();
	}
}

// This scheduler will check all tasks and timers that are due and service them
void runScheduler(void)
{
	scheduler_timeout_call_next_callback();
}

// This gets called by the scheduler approximately every 100ms
absolutetime_t MAIN_dataTask(void *payload)
{
	static time_t previousTransmissionTime = 0;

	// Get the current time. This uses the C standard library time functions
	time_t timeNow = time(NULL);

	// Example of how to send data when MQTT is connected every 1 second based on the system clock
	if (CLOUD_isConnected()) {
		// How many seconds since the last time this loop ran?
		int32_t delta = difftime(timeNow, previousTransmissionTime);

		if (delta >= CFG_SCAN_INTERVAL) {
			previousTransmissionTime = timeNow;

			// Call the data task in main.c
			RFID_Scan();
		}
	}

	// Example of how to read the SW0 and SW1 buttons
	// Check if the disconnect button was pressed
	if (SW0_get_level() == 0) {
		CLOUD_disconnect();
	}

	if (SW1_get_level() == 0) {
		shared_networking_params.haveAPConnection = 1;
	}

	LED_BLUE_set_level(!shared_networking_params.haveAPConnection);

	// This is milliseconds managed by the RTC and the scheduler, this return makes the
	//      timer run another time, returning 0 will make it stop
	return MAIN_DATATASK_INTERVAL;
}

// This will get called every 1 second only while we have a valid Cloud connection
void RFID_Scan(void)
{
	static char json[30];
	static uint8_t TagUID[ISO15693_NBBYTE_UID]; // this MUST be static
	
	// This part runs every CFG_SCAN_INTERVAL seconds
	if ( ISO15693_GetUID( TagUID ) == RESULTOK )
	{
		// UID is stored in reverse byte order
		sprintf( json, "{\"UID\":\"%02X%02X%02X%02X%02X%02X%02X%02X\"}",
			TagUID[7], TagUID[6], TagUID[5], TagUID[4], TagUID[3], TagUID[2], TagUID[1], TagUID[0] );

		CLOUD_publishData((uint8_t *)json, strlen(json));
		
		debug_printInfo( "RFID: %s", json );
	}

	LED_flashYellow();
}

void process_cloud_command( uint8_t* topic, uint8_t* payload )
{
	// mqttSubscribe = "/devices/d012313D1E521D10BFE/commands/#"
	// need to ignore the "/#" -> 37 bytes
	if ( memcmp( topic, mqttSubscribe, 37 ) == 0 )
	{
		if ( strcmp( (char*)payload, "YES" ) == 0 )
		{
			Access_Granted();
		}
		else
		{
			LED_flashRed();
		}
	}
}