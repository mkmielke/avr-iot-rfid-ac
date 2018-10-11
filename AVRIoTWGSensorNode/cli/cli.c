/*
    \file   cli.c

    \brief  Command Line Interpreter source file.

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

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <avr/wdt.h>

#include "usart_basic.h"
#include "cli.h"
#include "../cloud/crypto_client/crypto_client.h"
#include "../credentials_storage/credentials_storage.h"
#include "../mqtt/mqtt_core/mqtt_core.h"
#include "debug_print.h"

#define NUM_COMMANDS 8
#define WIFI_PARAMS_OPEN_CNT 1
#define WIFI_PARAMS_PSK_CNT 2
#define MAX_COMMAND_SIZE 100
#define MAX_PUB_KEY_LEN 200
#define UNKNOWN_CMD_MSG                                                                                                \
	"\
--------------------------------------------\r\n\
Unknown command. List of available commands:\r\n\
reset\r\n\
device\r\n\
key\r\n\
reconnect\r\n\
version\r\n\
cli_version\r\n\
wifi <ssid>[,<pass>, [authType]]\r\n\
debug\r\n\
--------------------------------------------\r\n\4"

static volatile char    command[MAX_COMMAND_SIZE];
static volatile bool    isCommandReceived  = false;
static volatile uint8_t index              = 0;
static volatile bool    commandTooLongFlag = false;

const char command_reset[]            = "reset";
const char command_reconnect[]        = "reconnect";
const char command_wifi_auth[]        = "wifi";
const char command_key[]              = "key";
const char command_device[]           = "device";
const char command_cli_version[]      = "cli_version";
const char command_firmware_version[] = "version";
const char command_debug[]            = "debug";

const char cli_version_number[]      = "1.0";
const char firmware_version_number[] = "1.0";

static void command_received(char *command_text);
static void reset_cmd(char *pArg);
static void reconnect_cmd(char *pArg);
static void set_wifi_auth(char *ssid_pwd_auth);
static void get_public_key(char *pArg);
static void get_device_id(char *pArg);
static void get_cli_version(char *pArg);
static void get_firmware_version(char *pArg);
static void set_debug_level(char *pArg);

static void enableUsartRxInterrupts(void);
static void receiveCommandCharacter(void);

#define CLI_TASK_INTERVAL 50
absolutetime_t CLI_task(void *);
timer_struct_t CLI_task_timer = {CLI_task};

struct cmd {
	const char *command;
	void (*handler)(char *argument);
};

const struct cmd commands[NUM_COMMANDS] = {{command_reset, reset_cmd},
                                           {command_reconnect, reconnect_cmd},
                                           {command_wifi_auth, set_wifi_auth},
                                           {command_key, get_public_key},
                                           {command_device, get_device_id},
                                           {command_cli_version, get_cli_version},
                                           {command_firmware_version, get_firmware_version},
                                           {command_debug, set_debug_level}};

void CLI_init(void)
{
	enableUsartRxInterrupts();
	scheduler_timeout_create(&CLI_task_timer, CLI_TASK_INTERVAL);
}

absolutetime_t CLI_task(void *param)
{
	// read all the EUSART bytes in the queue
	while (USART_0_is_rx_ready() && !isCommandReceived) {
		receiveCommandCharacter();
	}

	if (isCommandReceived) {
		char command_copy[MAX_COMMAND_SIZE];
		strncpy(command_copy, (char *)command, strlen((char *)command) + 1); // copying the null terminator as well
		isCommandReceived = false;
		index             = 0;
		command_received((char *)command_copy);
	}

	if (commandTooLongFlag) {
		printf("\n\rCommand too long!\r\n");
		commandTooLongFlag = false;
	}

	return CLI_TASK_INTERVAL;
}

void receiveCommandCharacter(void)
{
	char c = USART_0_read();
	if (c != '\n' && c != 0 && c != '\r') {
		command[index++] = c;
		if (index >= MAX_COMMAND_SIZE) {
			index              = 0;
			commandTooLongFlag = true;
		}
	} else {
		if (index > 0) {
			command[index]    = '\0';
			isCommandReceived = true;
		}
	}
}

static void set_wifi_auth(char *ssid_pwd_auth)
{
	char *  credentials[4];
	char *  pch;
	uint8_t paramCnt = 0;

	pch = strtok(ssid_pwd_auth, ",");
	while (pch != NULL && paramCnt <= WIFI_PARAMS_PSK_CNT) {
		credentials[paramCnt++] = pch;
		pch                     = strtok(NULL, ", ");
	}

	switch (paramCnt) {
	case WIFI_PARAMS_OPEN_CNT:
		strncpy(ssid, credentials[0], MAX_WIFI_CREDENTIALS_LENGTH - 1);
		strcpy(pass, "\0");
		strcpy(authType, "1");
		CREDENTIALS_STORAGE_save(ssid, pass, authType);
		printf("OK\r\n\4");
		break;

	case WIFI_PARAMS_PSK_CNT:
	case WIFI_PARAMS_PSK_CNT + 1:
		strncpy(ssid, credentials[0], MAX_WIFI_CREDENTIALS_LENGTH - 1);
		strncpy(pass, credentials[1], MAX_WIFI_CREDENTIALS_LENGTH - 1);
		strncpy(authType, "2", 2);
		CREDENTIALS_STORAGE_save(ssid, pass, authType);
		printf("OK\r\n\4");
		break;

	default:
		printf("Error. Wi-Fi command format is wifi <ssid>[,<pass>].\r\n\4");
		break;
	}
}

static void reconnect_cmd(char *pArg)
{
	(void)pArg;

	MQTT_Disconnect(MQTT_GetClientConnectionInfo());
	printf("OK\r\n");
}

static void reset_cmd(char *pArg)
{
	(void)pArg;

	wdt_enable(WDTO_30MS);
	while (1) {
	};
}

static void set_debug_level(char *pArg)
{
	debug_severity_t level = SEVERITY_NONE;
	if (*pArg >= '0' && *pArg <= '4') {
		level = *pArg - '0';
		CREDENTIALS_STORAGE_setDebugSeverity(level);
		debug_setSeverity(level);
		printf("OK\r\n");
	} else {
		printf("debug parameter must be between 0 (Least) and 4 (Most) \r\n");
	}
}

static void get_public_key(char *pArg)
{
	char key_pem_format[MAX_PUB_KEY_LEN];
	(void)pArg;

	if (CRYPTO_CLIENT_printPublicKey(key_pem_format) == NO_ERROR) {
		printf(key_pem_format);
	} else {
		printf("Error getting key.\r\n\4");
	}
}

static char *ateccsn = NULL;
void         CLI_setdeviceId(char *id)
{
	ateccsn = id;
}

static void get_device_id(char *pArg)
{
	(void)pArg;
	if (ateccsn) {
		printf("%s\r\n\4", ateccsn);
	} else {
		printf("Unknown.\r\n\4");
	}
}

static void get_cli_version(char *pArg)
{
	(void)pArg;
	printf("v%s\r\n\4", cli_version_number);
}

static void get_firmware_version(char *pArg)
{
	(void)pArg;
	printf("v%s\r\n\4", firmware_version_number);
}

static void command_received(char *command_text)
{
	char *  argument = strstr(command_text, " ");
	uint8_t cmp;
	uint8_t ct_len;
	uint8_t cc_len;

	if (argument != NULL) {
		/* Replace the delimiter with string terminator */
		*argument = '\0';
		/* Point after the string terminator, to the actual string */
		argument++;
	}

	for (uint8_t i = 0; i < NUM_COMMANDS; i++) {
		cmp    = strcmp(command_text, commands[i].command);
		ct_len = strlen(command_text);
		cc_len = strlen(commands[i].command);

		if (cmp == 0 && ct_len == cc_len) {
			if (commands[i].handler != NULL) {
				commands[i].handler(argument);
				return;
			}
		}
	}

	printf(UNKNOWN_CMD_MSG);
}

static void enableUsartRxInterrupts(void)
{
	// Empty RX buffer
	do {
		(void)USART2.RXDATAL;
	} while ((USART2.STATUS & USART_RXCIF_bm) != 0);

	// Enable RX interrupt
	USART2.CTRLA |= 1 << USART_RXCIE_bp;
}
