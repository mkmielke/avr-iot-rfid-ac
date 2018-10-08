/*
    \file   core.h

    \brief  Application Core header file.

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

#ifndef CORE_H
#define CORE_H

#include "timeout.h"
#include "umqtt/umqtt.h"
#include "../../winc/socket/include/socket.h"

#define NO_TIMEOUT 0xFFFFFFFF
#define MAX_PROJECT_METADATA_LENGTH 30
#define MAX_WIFI_CREDENTIALS_LENGTH 30
#define MAX_WIFI_SECURITY_LENGTH 2

typedef enum {
	NO_ERROR,
	WIFI_INIT_ERROR,
	WIFI_CONNECT_ERROR,
	UDP_SOCKET_ERROR,
	BIND_ERROR,
	NTP_REQ_SEND_ERROR,
	NO_NTP_RES_ERROR,
	DNS_REQ_ERROR,
	DNS_REPLY_ERROR,
	TCP_SOCKET_ERROR,
	TCP_CONNECT_ERROR,
	TLS_ERROR,
	JWT_CREATE_ERROR,
	MQTT_CONNECT_ERROR,
	BACK_OFF_FATAL_ERROR,
	TRANSMIT_ERROR,
	AP_ENABLE_ERROR,
	TIMEDOUT_ERROR,
	ATECC_SN_ERROR,
	ATECC_GET_KEY_ERROR,
} error_t;

typedef enum { EXECUTE = 0, PENDING, COMPLETED, TIMEDOUT } stateStatus_t;

typedef enum {
	AP_PROVISION_S = 0,
	WIFI_CONNECT_S,
	BIND_NTP_SOCKET_S,
	NTP_REQ_S,
	DNS_REQ_S,
	TLS_CONNECT_S,
	MQTT_CONNECT_S,
	READY_TO_SEND_S,
	STOP_S
} state_name_t;

typedef struct state {
	state_name_t nextState;
	error_t (*pStateFunc)(void);
	stateStatus_t  status;
	absolutetime_t timeout;
} state_t;

extern state_t state_array[];

extern SOCKET tcpClientSocket;
extern SOCKET udpSocket;

extern uint32_t epoch;
extern uint32   mqttGoogleApisComIP;
extern uint8_t  connectAttemptCnt;

extern char cid[];
extern char mqttTopic[];

extern char ssid[];
extern char pass[];
extern char authType[];

extern const char projectId[];
extern const char projectRegion[];
extern const char registryId[];
extern char       deviceId[];

extern timer_struct_t sendToCloudTimer;

#endif /* CORE_H */
