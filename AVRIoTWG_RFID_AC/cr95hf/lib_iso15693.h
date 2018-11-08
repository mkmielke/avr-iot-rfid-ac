/**
  ******************************************************************************
  * @file    lib_iso15693.h 
  * @author  MMY Application Team
  * @version V0.1
  * @date    15/03/2011
  * @brief   
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
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  */ 

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __ISO15693_H
#define __ISO15693_H

#include "lib_CR95HF.h"


/******************************************************************************/
/*                                 Defines                                    */
/******************************************************************************/
// error code
#define ISO15693_ERRORCODE_REQUESTFLAG				0xF1
#define ISO15693_ERRORCODE_PARAMETERLENGTH			0xF2
#define ISO15693_ERRORCODE_CRCRESIDUE				0xF3

// command code
#define ISO15693_CMDCODE_INVENTORY	    		0x01
#define ISO15693_CMDCODE_STAYQUIET	    		0x02
#define ISO15693_CMDCODE_READSINGLEBLOCK		0x20
#define ISO15693_CMDCODE_WRITESINGLEBLOCK		0x21
#define ISO15693_CMDCODE_LOCKBLOCK				0x22
#define ISO15693_CMDCODE_READMULBLOCKS			0x23
#define ISO15693_CMDCODE_WRITEMULBLOCKS			0x24
#define ISO15693_CMDCODE_SELECT					0x25
#define ISO15693_CMDCODE_RESETTOREADY			0x26
#define ISO15693_CMDCODE_WRITEAFI				0x27
#define ISO15693_CMDCODE_LOCKAFI				0x28
#define ISO15693_CMDCODE_WRITEDSFID				0x29
#define ISO15693_CMDCODE_LOCKDSFID				0x2A
#define ISO15693_CMDCODE_GETSYSINFO				0x2B
#define ISO15693_CMDCODE_GETSECURITYINFO		0x2C

// appendCrc constants for select commands
#define ISO15693_APPENDCRC  					1
#define ISO15693_DONTAPPENDCRC     				0

// CRC 16 constants
#define ISO15693_PRELOADCRC16 					0xFFFF
#define ISO15693_POLYCRC16 						0x8408
#define ISO15693_MASKCRC16 						0x0001
#define ISO15693_RESIDUECRC16 					0xF0B8

// byte offset for tag responses
#define ISO15693_OFFSET_FLAGS			 			0x00
#define ISO15693_OFFSET_ERRORCODE					0x01

#define ISO15693_INVENTORYOFFSET_DSFID	 			0x01
#define ISO15693_INVENTORYOFFSET_UID	 			0x02
#define ISO15693_INVENTORYOFFSET_CRC16	 			0x05
// ...more

// request flags
#define ISO15693_REQFLAG_SINGLESUBCARRIER  			0
#define ISO15693_REQFLAG_TWOSUBCARRIER     			1
#define ISO15693_REQFLAG_LOWDATARATE  				0
#define ISO15693_REQFLAG_HIGHDATARATE     			1
#define ISO15693_REQFLAG_INVENTORYFLAGNOTSET		0
#define ISO15693_REQFLAG_INVENTORYFLAGSET   		1
#define ISO15693_REQFLAG_NOPROTOCOLEXTENSION		0
#define ISO15693_REQFLAG_PROTOCOLEXTENSION  		1
// request flag 5 to 8 definition when inventory flag is not set
#define ISO15693_REQFLAG_NOTSELECTED				0
#define ISO15693_REQFLAG_SELECTED			   		1
#define ISO15693_REQFLAG_NOTADDRESSES				0
#define ISO15693_REQFLAG_ADDRESSED			   		1
#define ISO15693_REQFLAG_OPTIONFLAGNOTSET			0
#define ISO15693_REQFLAG_OPTIONFLAGSET			   	1
#define ISO15693_REQFLAG_RFUNOTSET					0
#define ISO15693_REQFLAG_RFUSET					   	1
// request flag 5 to 8 definition when inventory flag is set
#define ISO15693_REQFLAG_NOTAFI						0
#define ISO15693_REQFLAG_AFI				   		1
#define ISO15693_REQFLAG_16SLOTS					0
#define ISO15693_REQFLAG_1SLOT				   		1

// data rates constants for select commands
#define ISO15693_TRANSMISSION_26	    		0
#define ISO15693_TRANSMISSION_53	    		1
#define ISO15693_TRANSMISSION_6	   	 			2
#define ISO15693_TRANSMISSION_RFU				3
// constants for select commands
#define ISO15693_RESPECT_312	    			0
#define ISO15693_WAIT_FOR_SOF	    			1
// modulation constants for select commands
#define ISO15693_MODULATION_100					0
#define ISO15693_MODULATION_10					1
// sub carrier constants for select commands
#define ISO15693_SINGLE_SUBCARRIER   			0
#define ISO15693_DUAL_SUBCARRIER     			1
// appendCrc constants for select commands
#define ISO15693_APPENDCRC  					1
#define ISO15693_DONTAPPENDCRC     				0

// 	number of byte of parameters
#define ISO15693_NBBYTE_UID	 						0x08
#define ISO15693_NBBYTE_CRC16	 					0x02
#define ISO15693_NBBYTE_DSFID 						0x01
#define ISO15693_NBBYTE_AFI 						0x01
#define ISO15693_NBBYTE_BLOCKSECURITY   			0x01
#define ISO15693_NBBYTE_REPLYFLAG		   			0x01
#define ISO15693_NBBYTE_INFOFLAG		   			0x01
#define ISO15693_NBBYTE_MEMORYSIZE		   			0x02
#define ISO15693_NBBYTE_ICREF			   			0x01
#define ISO15693_NBBYTE_REQUESTFLAG					0x01


/******************************************************************************/
/*                             Public Functions                               */
/******************************************************************************/
int8_t ISO15693_CreateRequestFlag( const uint8_t SubCarrierFlag, const uint8_t DataRateFlag,
							  	   const uint8_t InventoryFlag, const uint8_t ProtExtFlag,
								   const uint8_t SelectOrAFIFlag, const uint8_t AddrOrNbSlotFlag,
								   const uint8_t OptionFlag, const uint8_t RFUFlag );
int8_t ISO15693_SelectProtocol( const uint8_t DataRate, const uint8_t TimeOrSOF,
								const uint8_t Modulation, const uint8_t SubCarrier,
								const uint8_t AppendCRC );
int8_t ISO15693_Inventory( const uint8_t Flags, const uint8_t AFI,
						   const uint8_t MaskLength, const uint8_t *MaskValue,
						   const uint8_t AppendCRC, const uint8_t *CRC16,
						   uint8_t *pResponse );
int8_t ISO15693_SplitInventoryResponse( const uint8_t *ReaderResponse, const uint8_t Length,
										uint8_t *Flags, uint8_t *DSFIDextract, uint8_t *UIDoutIndex );
int8_t ISO15693_GetUID( uint8_t *UIDout );

int8_t ISO15693_IsInventoryFlag( const uint8_t FlagsByte );
int8_t ISO15693_GetSubCarrierFlag( const uint8_t FlagsByte );
int8_t ISO15693_GetDataRateFlag( const uint8_t FlagsByte );
int8_t ISO15693_GetSelectOrAFIFlag( const uint8_t FlagsByte );
int8_t ISO15693_IsReaderConfigMatchWithFlag( const uint8_t ParameterSelected, const uint8_t Flags );

int8_t ISO15693_IsCorrectCRC16Residue( const uint8_t *DataIn, const uint8_t Length );
int16_t ISO15693_CRC16( const uint8_t *DataIn, const uint8_t NbByte );

#endif /* __ISO15693_H */

/******************* (C) COPYRIGHT 2010 STMicroelectronics *****END OF FILE****/
