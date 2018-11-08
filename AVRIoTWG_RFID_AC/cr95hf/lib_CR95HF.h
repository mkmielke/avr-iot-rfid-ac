/**
  ******************************************************************************
  * @file    drv_CR95HF.h 
  * @author  MMY Application Team
  * @version V1.1.1
  * @date    03/03/2010
  * @brief   CR95HF driver header.
  ******************************************************************************
  * @copy
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2010 STMicroelectronics</center></h2>
  */ 

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _LIB_CR95HF_H
#define _LIB_CR95HF_H


/******************************************************************************/
/*                                Includes                                    */
/******************************************************************************/
#include "drv_CR95HF.h"


/******************************************************************************/
/*                                 Defines                                    */
/******************************************************************************/
#define RESULTOK 				0
#define ERRORCODE_GENERIC 		1

/*** CR95HF Status ***/
#define CR95HF_SUCCESS_CODE							0x00
#define CR95HF_NOREPLY_CODE							0x01

// CR95HF polling status
#define CR95HF_POLLING_CR95HF						0x00
#define CR95HF_POLLING_TIMEOUT						0x01

#define SENDRECV_ERRORCODE_SOFT						0xFF

/*** Error Codes ***/
#define	CR95HF_ERRORCODE_DEFAULT					0xFE
#define	CR95HF_ERRORCODE_TIMEOUT					0xFD
#define	CR95HF_ERRORCODE_UARTDATARATEUNCHANGED		0xFC
#define	CR95HF_ERRORCODE_UARTDATARATEPROCESS		0xFB
#define CR95HF_ERROR_CODE							0x40
#define CR95HF_ERRORCODE_PARAMETERLENGTH			0x41
#define CR95HF_ERRORCODE_PARAMETER					0x42
#define CR95HF_ERRORCODE_COMMANDUNKNOWN				0x43
#define CR95HF_ERRORCODE_PORERROR					0x44

#define ERRORCODE_LENGTH							0x01

// ECHO response
#define ECHORESPONSE								0x55

/*** protocol select status ***/
#define PROTOCOLSELECT_LENGTH						0x02
#define PROTOCOLSELECT_RESULTSCODE_OK				0x00
#define PROTOCOLSELECT_ERRORCODE_CMDLENGTH			0x82
#define PROTOCOLSELECT_ERRORCODE_INVALID			0x83

/*** CR95HF Command Codes ***/
#define IDN											0x01
#define PROTOCOL_SELECT 							0x02
#define POLL_FIELD 									0x03
#define SEND_RECEIVE								0x04
#define IDLE										0x07
#define READ_REGISTER								0x08
#define WRITE_REGISTER								0x09
#define BAUD_RATE									0x0A
#define ECHO										0x55

/*** Offset Definitions for Buffers ***/
#define CR95HF_COMMAND_OFFSET						0x00
#define CR95HF_LENGTH_OFFSET						0x01
#define CR95HF_DATA_OFFSET							0x02
#define TAGREPPLY_OFFSET_UID						CR95HF_DATA_OFFSET + 0x02

/* mask of selected command */
#define CR95HF_SELECTMASK_DATARATE					0x30
#define CR95HF_SELECTMASK_SUBCARRIER				0x02

#define ECHOREPLY_OFFSET							0x00
#define ECHOREPLY_LENGTH							0x01

#define READERREPLY_STATUSOFFSET					0x00

/* protocol allowed */
#define PROTOCOL_TAG_FIELDOFF						0x00
#define PROTOCOL_TAG_ISO15693						0x01
#define PROTOCOL_TAG_ISO14443A						0x02
#define PROTOCOL_TAG_ISO14443B						0x03
#define PROTOCOL_TAG_FELICA							0x04
// ...more

/* control byte according to protocl*/
#define CONTROL_MAX_NBBYTE							0x03
#define CONTROL_15693_NBBYTE						0x01
#define CONTROL_15693_CRCMASK						0x02
#define CONTROL_15693_COLISIONMASK					0x01

#define CONTROL_14443A_NBBYTE						0x03
#define CONTROL_14443A_COLISIONMASK					0x80
#define CONTROL_14443A_CRCMASK						0x20
#define CONTROL_14443A_PARITYMASK					0x10
#define CONTROL_14443A_NBSIGNIFICANTBITMASK			0x0F
#define CONTROL_14443A_FIRSTCOLISIONBITMASK			0x0F

#define CONTROL_14443B_NBBYTE						0x01
#define CONTROL_14443B_CRCMASK						0x02
#define CONTROL_14443B_COLISIONMASK					0x01

#define CONTROL_FELICA_NBBYTE						0x01
#define CONTROL_FELICA_CRCMASK						0x02
#define CONTROL_FELICA_COLISIONMASK					0x01


/******************************************************************************/
/*                                Structures                                  */
/******************************************************************************/
typedef struct {
	CR95HF_INTERFACE 		Interface;
	SPI_MODE 				SpiMode;
	int8_t					CurrentProtocol;
} ReaderConfigStruct;


/******************************************************************************/
/*                             Public Functions                               */
/******************************************************************************/
/*** Available CR95HF Commands ***/
//int8_t CR95HF_IDN( uint8_t *pResponse );
//int8_t CR95HF_ProtocolSelect( const uint8_t Length, const uint8_t Protocol, const uint8_t *Parameters, uint8_t *pResponse );
int8_t CR95HF_SendRecv( const uint8_t Length, const uint8_t *Parameters, uint8_t *pResponse);
//int8_t CR95HF_Idle( const uint8_t Length, const uint8_t *Data, uint8_t *pResponse);
//int8_t CR95HF_RdReg( const uint8_t Length, const uint8_t Address, const uint8_t RegCount, const uint8_t Flags, uint8_t *pResponse);
//int8_t CR95HF_WrReg( const uint8_t Length, const uint8_t Address, const uint8_t Flags, const uint8_t *pData, uint8_t *pResponse);
//int8_t CR95HF_BaudRate( const uint8_t BaudRate, uint8_t *pResponse );
int8_t CR95HF_Echo( uint8_t *pResponse );

int8_t CR95HF_PORsequence( void );
int8_t CR95HF_ProtocolSelect( const uint8_t Length, const uint8_t Protocol, const uint8_t *Parameters, uint8_t *pResponse );

int8_t SplitReaderReply( uint8_t CmdCodeToReader, uint8_t ProtocolSelected,
					  	 const uint8_t *ReaderReply, uint8_t *ResultCode,
						 uint8_t *NbTagByte, uint8_t *TagReplyDataIndex,
						 uint8_t *NbControlByte, uint8_t *ControlIndex );

int8_t CR95HF_IsReaderResultCodeOk( uint8_t CmdCode, const uint8_t *ReaderReply );
int8_t CR95HF_IsCommandExists( uint8_t CmdCode );

#endif /* __CR95HF_H */

/******************* (C) COPYRIGHT 2010 STMicroelectronics *****END OF FILE****/



