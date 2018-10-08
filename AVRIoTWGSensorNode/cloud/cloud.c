/*
    \file   cloud.c

    \brief  Cloud application Main source file.

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
#include "timeout.h"

#include "core/core.h"
#include "atmel_start_pins.h"
#include "../winc/driver/include/m2m_wifi.h"
#include "network_control/network_control.h"
#include "credentials_storage/credentials_storage.h"
#include "cli/cli.h"
#include "mqtt_client/mqtt_client.h"
#include "cloud.h"

#define CLOUD_CLI
#define MAX_BACK_OFF_RETRIES 8

static uint8_t        backOffIntervals[MAX_BACK_OFF_RETRIES] = {1, 2, 4, 8, 16, 32, 32, 32};
static state_name_t   current_state;
static timer_struct_t wait_timer, back_off_timer;
static uint16_t       back_off_time;

static void           error_handler(error_t err);
static absolutetime_t wait_timer_cb(void *payload);
static absolutetime_t back_off_timer_cb(void *payload);
static void           setTimeout(state_name_t current_state);

uint8_t connectAttemptCnt;

static bool isProvisionRequested()
{
	if (!SW0_get_level()) {
		return true;
	} else {
		return false;
	}
}

void CLOUD_init(void)
{
	if (isProvisionRequested()) {
		current_state = AP_PROVISION_S;
	} else {
		current_state = WIFI_CONNECT_S;
	}
}

void CLOUD_sleep(void)
{
	close(tcpClientSocket);
	tcpClientSocket = -1;
	m2m_wifi_disconnect();
	current_state = STOP_S;
	LED_RED_set_level(false);
}

void CLOUD_reconnect(void)
{
	close(tcpClientSocket);
	tcpClientSocket                   = -1;
	current_state                     = WIFI_CONNECT_S;
	state_array[current_state].status = EXECUTE;
	LED_RED_set_level(true);
	LED_GREEN_set_level(true);
	LED_BLUE_set_level(true);
}

bool CLOUD_isConnected(void)
{
	return current_state == READY_TO_SEND_S;
}

void cloud_state_manage()
{
	error_t err;

	switch (state_array[current_state].status) {
	case EXECUTE:
		err = state_array[current_state].pStateFunc();
		if (err == NO_ERROR) {
			setTimeout(current_state);
		} else {
			error_handler(err);
		}
		break;

	case PENDING:
		scheduler_timeout_call_next_callback();
		break;

	case COMPLETED:
		current_state                     = state_array[current_state].nextState;
		state_array[current_state].status = EXECUTE;
		scheduler_timeout_delete(&wait_timer);
		break;

	case TIMEDOUT:
		scheduler_timeout_delete(&wait_timer);
		error_handler(TIMEDOUT_ERROR);
		break;

	default:
		break;
	}
}

void CLOUD_task(void)
{
	m2m_wifi_handle_events(NULL);

#ifdef CLOUD_CLI
	CLI_commandReceive();
#endif

	cloud_state_manage();
}

void CLOUD_send(uint8_t *data, uint8_t datalen)
{
	MQTT_CLIENT_publish(data, datalen);
}

static void error_handler(error_t err)
{
	LED_RED_set_level(false);

	switch (err) {
	case BACK_OFF_FATAL_ERROR:
		CLOUD_sleep();
		break;
	case TIMEDOUT_ERROR:
		switch (current_state) {
		case WIFI_CONNECT_S:
		case NTP_REQ_S:
		case DNS_REQ_S:
		case TLS_CONNECT_S:
			state_array[current_state].status = EXECUTE;
			break;
		case MQTT_CONNECT_S:
			// Exponential back off variant with cvasi-constant max delay
			if (connectAttemptCnt < MAX_BACK_OFF_RETRIES) {
				state_array[current_state].status = PENDING;
				back_off_time                     = backOffIntervals[connectAttemptCnt++] * 1000 + rand() % 1000;
				back_off_timer.callback_ptr       = back_off_timer_cb;
				scheduler_timeout_delete(&back_off_timer);
				scheduler_timeout_create(&back_off_timer, back_off_time);
			} else {
				error_handler(BACK_OFF_FATAL_ERROR);
			}
			break;
		default:
			CLOUD_init();
			break;
		}
		break;
	case DNS_REQ_ERROR:
	case NTP_REQ_SEND_ERROR:
	case WIFI_CONNECT_ERROR:
	case UDP_SOCKET_ERROR:
	case TCP_SOCKET_ERROR:
	case TCP_CONNECT_ERROR:
	default:
		CLOUD_init();
		break;
	}
}

absolutetime_t wait_timer_cb(void *payload)
{
	state_array[current_state].status = TIMEDOUT;
	return 0;
}

absolutetime_t back_off_timer_cb(void *payload)
{
	current_state                     = TLS_CONNECT_S;
	state_array[current_state].status = EXECUTE;
	return 0;
}

static void setTimeout(state_name_t current_state)
{
	if (state_array[current_state].timeout) {
		scheduler_timeout_delete(&wait_timer);
		wait_timer.callback_ptr = wait_timer_cb;
		scheduler_timeout_create(&wait_timer, state_array[current_state].timeout);
	}
	state_array[current_state].status = PENDING;
}
