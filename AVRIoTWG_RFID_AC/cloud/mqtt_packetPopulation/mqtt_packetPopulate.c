/*
    \file   mqtt_packetParameters.c

    \brief  MQTT Packet Parameters source file.

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

// ToDo This file needs to be renamed as app_mqttClient.c
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../mqtt/mqtt_core/mqtt_core.h"
#include "mqtt_packetPopulate.h"
#include "../../Config/IoT_Sensor_Node_config.h"
#include "debug_print.h"

#define MQTT_CID_LENGTH 100
#define MQTT_TOPIC_LENGTH 38
#define MQTT_SUBSCRIBE_LENGTH 41
#define MQTT_SUBSCRIBE_PACKET_ID 1234 // arbitrary value

char mqttPassword[456];
char cid[MQTT_CID_LENGTH];
char mqttTopic[MQTT_TOPIC_LENGTH];
char mqttSubscribe[MQTT_SUBSCRIBE_LENGTH]; // Note: set in updateJWT() - cloud_service.c
char mqttHostName[] = "mqtt.googleapis.com";

void MQTT_CLIENT_publish(uint8_t *data, uint16_t len)
{
	mqttPublishPacket cloudPublishPacket;

	// Fixed header
	cloudPublishPacket.publishHeaderFlags.duplicate = 0;
	cloudPublishPacket.publishHeaderFlags.qos       = 0;
	cloudPublishPacket.publishHeaderFlags.retain    = 0;

	// Variable header
	cloudPublishPacket.topic = (uint8_t *)mqttTopic;

	// Payload
	cloudPublishPacket.payload = data;
	// ToDo Check whether sizeof can be used for integers and strings
	cloudPublishPacket.payloadLength = len;

	if (MQTT_CreatePublishPacket(&cloudPublishPacket) != true) {
		debug_printError("MQTT: Connection lost PUBLISH failed");
	}
}

// set parameters for subscribe packet and call the create() function
void MQTT_CLIENT_subscribe( void )
{
	mqttSubscribePacket cloudSubscribePacket;
	
	// Fixed header
	cloudSubscribePacket.subscribeHeaderFlags.duplicate = 0;
	cloudSubscribePacket.subscribeHeaderFlags.qos       = 1;
	cloudSubscribePacket.subscribeHeaderFlags.retain    = 0;
	
	// Variable header
	cloudSubscribePacket.packetIdentifierMSB = ( MQTT_SUBSCRIBE_PACKET_ID >> 8 ) & 0xFF;
	cloudSubscribePacket.packetIdentifierLSB = MQTT_SUBSCRIBE_PACKET_ID & 0xFF;
	
	// Payload
	cloudSubscribePacket.subscribePayload[0].requestedQoS = 0;
	cloudSubscribePacket.subscribePayload[0].topic = (uint8_t*)mqttSubscribe;
	cloudSubscribePacket.subscribePayload[0].topicLength = strlen( mqttSubscribe );
	
	cloudSubscribePacket.subscribePayload[1].requestedQoS = 0;
	cloudSubscribePacket.subscribePayload[1].topic[0] = '\0';
	cloudSubscribePacket.subscribePayload[1].topicLength = 0;
	
	if ( MQTT_CreateSubscribePacket( &cloudSubscribePacket ) != true ) 
	{
		debug_printError( "MQTT: Connection lost SUBSCRIBE failed" );
	}
}

void MQTT_CLIENT_receive(uint8_t *data, uint8_t len)
{
	MQTT_GetReceivedData(data, len);
}

// ToDo This function is not currently being used.
void MQTT_CLIENT_connect(void)
{
	mqttConnectPacket cloudConnectPacket;

	memset(&cloudConnectPacket, 0, sizeof(mqttConnectPacket));

	cloudConnectPacket.connectVariableHeader.connectFlagsByte.All = 0x02;
	cloudConnectPacket.connectVariableHeader.keepAliveTimer       = 10;
	cloudConnectPacket.clientID                                   = (uint8_t *)cid;
	cloudConnectPacket.password                                   = (uint8_t *)mqttPassword;
	cloudConnectPacket.passwordLength                             = strlen(mqttPassword);
	cloudConnectPacket.username                                   = NULL;
	cloudConnectPacket.usernameLength                             = 0;

	MQTT_CreateConnectPacket(&cloudConnectPacket);
}
