/*
    \file   main.c

    \brief  Main source file.

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

#include <stdbool.h>
#include <string.h>
#include <atmel_start.h>
#include <atomic.h>
#include <util/delay.h>
#include <math.h>

#include "../../winc/driver/include/m2m_wifi.h"
#include "cloud/cloud.h"
#include "cloud/network_control/network_control.h"
#include "clock_config.h"
#include "sensors_handling.h"
#include "IoT_Sensor_Node_config.h"

#define SEND_INTERVAL CFG_SEND_INTERVAL

#define PACKET_SIZE 70
#define LED_ON false
#define LED_OFF true
#define LEDS_TEST_INTERVAL 50
#define LEDS_TEST_SEQ_NO 2

#ifndef ATCA_NO_HEAP
#error : This project uses CryptoAuthLibrary V2. Please add "ATCA_NO_HEAP" to toolchain symbols.
#endif

#ifndef ATCA_NO_POLL
#error : This project uses ATCA_NO_POLL option. Please add "ATCA_NO_POLL" to toolchain symbols.
#endif

#ifndef ATCA_HAL_I2C
#error : This project uses I2C interface. Please add "ATCA_HAL_I2C" to toolchain symbols.
#endif

#ifndef ATCA_PRINTF
#error : This project uses ATCA_PRINTF. Please add "ATCA_PRINTF" to toolchain symbols.
#endif

absolutetime_t sendToCloud(void *payload)
{
	char    json[PACKET_SIZE];
	int16_t temp = 0;

	if (CLOUD_isConnected()) {
		LED_RED_set_level(LED_OFF);
		LED_GREEN_set_level(LED_ON);
		LED_YELLOW_set_level(LED_ON);

		temp = sensors_GetTempValue();
		sprintf(json, "{\"Light\":%d,\"Temp\":\"%d.%02d\"}", sensors_GetLightValue(), temp / 100, abs(temp) % 100);
		CLOUD_send((uint8_t *)&json, strlen(json) * sizeof(char));

		_delay_ms(20);
		LED_YELLOW_set_level(LED_OFF);
	}

	return SEND_INTERVAL;
}

void USER_ledsTestSequence(uint8_t ledState)
{
	LED_BLUE_set_level(ledState);
	_delay_ms(LEDS_TEST_INTERVAL);
	LED_GREEN_set_level(ledState);
	_delay_ms(LEDS_TEST_INTERVAL);
	LED_YELLOW_set_level(ledState);
	_delay_ms(LEDS_TEST_INTERVAL);
	LED_RED_set_level(ledState);
	_delay_ms(LEDS_TEST_INTERVAL);
}

void USER_ledsTest(void)
{
	uint8_t indexLed = LEDS_TEST_SEQ_NO;
	while (indexLed) {
		USER_ledsTestSequence(LED_ON);
		USER_ledsTestSequence(LED_OFF);
		indexLed--;
	}
}

int main(void)
{
	tstrWifiInitParam param;

	param.pfAppWifiCb = NETWORK_CONTROL_wifiHandler;
	ENABLE_INTERRUPTS();
	atmel_start_init();

	USER_ledsTest();

	nm_bsp_init();
	m2m_wifi_init(&param);

	CLOUD_init();

	while (1) {
		CLOUD_task();
	}

	return 0;
}
