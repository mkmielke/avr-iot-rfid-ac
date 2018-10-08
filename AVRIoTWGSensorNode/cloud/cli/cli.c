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

#include "../core/core.h"
#include "../crypto_client/crypto_client.h"
#include "../credentials_storage/credentials_storage.h"
#include "../cloud.h"
#include "usart_basic.h"
#include "cli.h"

#define NUM_COMMANDS 5
#define WIFI_PARAMS_CNT 3
#define MAX_COMMAND_SIZE 100
#define MAX_PUB_KEY_LEN 200
#define UNKNOWN_CMD_MSG                                                                                                \
	"Unknown command.Available commands are:\r\n\
version\r\n\
reconnect\r\n\
device\r\n\
key\r\n\
wifi <ssid>,<password>,<security>\r\n\4"

static char command[MAX_COMMAND_SIZE];

char command_reconnect[]  = "reconnect";
char command_wifi_auth[]  = "wifi";
char command_key[]        = "key";
char command_device[]     = "device";
char command_version[]    = "version";
char cli_version_number[] = "1.0";

static void command_received(char *command_text);
static void reconnect_cmd(char *pArg);
static void set_wifi_auth(char *ssid_pwd_auth);
static void get_public_key(char *pArg);
static void get_device_id(char *pArg);
static void get_cli_version(char *pArg);

struct cmd {
	char *command;
	void (*handler)(char *argument);
};

const struct cmd commands[NUM_COMMANDS] = {
    {command_reconnect, reconnect_cmd},
    {command_wifi_auth, set_wifi_auth},
    {command_key, get_public_key},
    {command_device, get_device_id},
    {command_version, get_cli_version},
};

void CLI_commandReceive(void)
{
	static uint8_t index = 0;
	char           c;

	if (USART_0_is_rx_ready()) {
		c = USART_0_read();
		if (c != '\n' && c != 0 && c != '\r') {
			command[index++] = c;
			if (index >= MAX_COMMAND_SIZE) {
				index = 0;
				printf("Command too long!\r\n");
			}
		} else if (c == '\n') {
			command[index] = '\0';
			command_received(command);
			index = 0;
		}
	}
}

static void set_wifi_auth(char *ssid_pwd_auth)
{
	char *  credentials[4];
	char *  pch;
	uint8_t paramCnt = 0;

	pch = strtok(ssid_pwd_auth, ",");
	while (pch != NULL && paramCnt <= WIFI_PARAMS_CNT) {
		credentials[paramCnt++] = pch;
		pch                     = strtok(NULL, ",");
	}

	if (paramCnt == WIFI_PARAMS_CNT) {
		if ((strlen(credentials[0]) < MAX_WIFI_CREDENTIALS_LENGTH)
		    && (strlen(credentials[1]) < MAX_WIFI_CREDENTIALS_LENGTH)
		    && (strlen(credentials[2]) < MAX_WIFI_SECURITY_LENGTH)) {
			strcpy(ssid, credentials[0]);
			strcpy(pass, credentials[1]);
			strcpy(authType, credentials[2]);
			CREDENTIALS_STORAGE_save(ssid, pass, authType);
			CLOUD_reconnect();
			printf("OK\r\n\4");
		} else {
			printf("Error. Wi-Fi SSID and password must be at most 29 characters and security type must be 1 "
			       "character.\r\n\4");
		}
	} else {
		printf("Error. Wi-Fi command format is wifi <ssid>,<password>,<security>.\r\n\4");
	}
}

static void reconnect_cmd(char *pArg)
{
	(void)pArg;

	CLOUD_reconnect();
	printf("OK\r\n");
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

static void get_device_id(char *pArg)
{
	char ateccsn[20];
	(void)pArg;

	if (CRYPTO_CLIENT_printSerialNumber(ateccsn) == NO_ERROR) {
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

static void command_received(char *command_text)
{
	char *  argument = strstr(command_text, " ");
	uint8_t cmp;
	uint8_t ct_len;
	uint8_t cc_len;

	if (argument != NULL) {
		*argument = '\0';
	}

	for (uint8_t i = 0; i < NUM_COMMANDS; i++) {
		cmp    = strcmp(command_text, commands[i].command);
		ct_len = strlen(command_text);
		cc_len = strlen(commands[i].command);

		if (cmp == 0 && ct_len == cc_len) {
			argument++;
			if (commands[i].handler != NULL) {
				commands[i].handler(argument);
				return;
			}
		}
	}

	printf(UNKNOWN_CMD_MSG);
}
