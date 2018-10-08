/*
    \file   mqtt_client.c

    \brief  MQTT Client source file.

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
#include "../core/core.h"
#define TX_BUFF_SIZE 400
#define RX_BUFF_SIZE 100
#define USER_LENGTH 0
#define MQTT_KEEP_ALIVE_TIME 120

static uint8_t mqttTxBuff[TX_BUFF_SIZE];
static uint8_t mqttRxBuff[RX_BUFF_SIZE];
char           mqttPassword[456];

static void umqtt_connected_cb(struct umqtt_connection *conn)
{
	state_array[MQTT_CONNECT_S].status = COMPLETED;
}

static void umqtt_send_packet(struct umqtt_connection *conn)
{
	send(tcpClientSocket, conn->txbuff.start, conn->txbuff.datalen, 0);
	umqtt_circ_init(&conn->txbuff);
}

struct umqtt_connection umqttConn = {
	.txbuff = {
		.start = mqttTxBuff,
		.length = TX_BUFF_SIZE,
	},
	.rxbuff = {
		.start = mqttRxBuff,
		.length = RX_BUFF_SIZE,
	},

	.user = NULL,
	.user_len = USER_LENGTH,
	.password = (uint8_t*)mqttPassword,

	.clientid = cid,
	.kalive = MQTT_KEEP_ALIVE_TIME,

	.connected_callback = umqtt_connected_cb,
	.new_packet_callback = umqtt_send_packet
};

void MQTT_CLIENT_publish(uint8_t *pData, uint8_t dataLen)
{
	umqtt_publish(&umqttConn, mqttTopic, pData, dataLen);
}

void MQTT_CLIENT_receive(uint8_t *pData, uint8_t len)
{
	umqtt_circ_push(&umqttConn.rxbuff, pData, len);
	umqtt_process(&umqttConn);
}

void MQTT_CLIENT_connect(void)
{
	umqttConn.password_len = strlen((char *)umqttConn.password);
	umqtt_circ_init(&umqttConn.txbuff);
	umqtt_circ_init(&umqttConn.rxbuff);
	umqtt_connect(&umqttConn);
}

bool MQTT_CLIENT_isConnected(void)
{
	return umqttConn.state == UMQTT_STATE_CONNECTED;
}
