/******************************************************************************/
/*                                 Includes                                   */
/******************************************************************************/
#include "lib_iso15693.h"


/******************************************************************************/
/*                                 Defines                                    */
/******************************************************************************/
// TODO: .h?
#define RFU 									0
#define ISO15693_PROTOCOL              			0x01
#define ISO15693_SELECTLENGTH          			0x02
// mask for parameter byte (select command) // TODO: .h?
#define ISO15693_MASK_APPENDCRC 					0x01
#define ISO15693_MASK_SUBCARRIER 					0x02
#define ISO15693_MASK_MODULATION 					0x04
#define ISO15693_MASK_WAITORSOF 					0x08
#define ISO15693_MASK_DATARATE	 					0x30
#define ISO15693_MASK_RFU		 					0xC0
// bits offset for parameter byte (select command) // TODO: .h?
#define ISO15693_OFFSET_APPENDCRC 					0x00
#define ISO15693_OFFSET_SUBCARRIER 					0x01
#define ISO15693_OFFSET_MODULATION 					0x02
#define ISO15693_OFFSET_WAITORSOF 					0x03
#define ISO15693_OFFSET_DATARATE	 				0x04
#define ISO15693_OFFSET_RFU		 					0x06
// data rates constants for select commands
#define ISO15693_TRANSMISSION_26	    		0
#define ISO15693_TRANSMISSION_53	    		1
#define ISO15693_TRANSMISSION_6	   	 			2
#define ISO15693_TRANSMISSION_RFU				3

// command length
#define ISO15693_MAXLENGTH_INVENTORY 				13	 	// 8 + 8 + 8 + 64 + 16 = 104bits => 13 bytes

#define ISO15693_NBBITS_MASKPARAMETER   			64

// mask of request flag
#define ISO15693_MASK_SUBCARRIERFLAG    		0x01
#define ISO15693_MASK_DATARATEFLAG	    		0x02
#define ISO15693_MASK_INVENTORYFLAG	  			0x04
#define ISO15693_MASK_PROTEXTFLAG    			0x08
#define ISO15693_MASK_SELECTORAFIFLAG 			0x10
#define ISO15693_MASK_ADDRORNBSLOTSFLAG 		0x20
#define ISO15693_MASK_OPTIONFLAG 				0x40
#define ISO15693_MASK_RFUFLAG 					0x80
// mask of response flag
#define ISO15693_MASK_ERRORFLAG    				0x01


/******************************************************************************/
/*                            Global Variables                                */
/******************************************************************************/
extern uint8_t GloParameterSelected;


/******************************************************************************/
/*                            Private Functions                               */
/******************************************************************************/
static int8_t ISO15693_IsAnAvailableDataRate( const uint8_t DataRate );


/******************************************************************************/
/*                           Function Definitions                             */
/******************************************************************************/

/**
* @brief  	this function returns a Request flags on one byte, which is concatenation of inventory flags
* @param  	SubCarrierFlag	: 0 for single subcarrier & 1 for double subcarrier
* @param	DataRateFlag	: 0 for 6 kbps & 0 for 26 kbps
* @param	InventoryFlag	: 1 for inventory command & 0 for the others
* @param	ProtExtFlag		: Not used by ISO 15693 (shall be reset (0 value))
* @param	SelectOrAFIFlag	: the signification depends on InventoryFlag
* @param	AddrOrNbSlotFlag: the signification depends on InventoryFlag
* @param	OptionFlag		: depends on command
* @param	RFUFlag			: shall be reset (0 value)
* @retval 	Request flags byte
*/
int8_t ISO15693_CreateRequestFlag( const uint8_t SubCarrierFlag, const uint8_t DataRateFlag, 
								   const uint8_t InventoryFlag, const uint8_t ProtExtFlag, 
								   const uint8_t SelectOrAFIFlag, const uint8_t AddrOrNbSlotFlag, 
								   const uint8_t OptionFlag, const uint8_t RFUFlag )
{
	int32_t FlagsByteBuf = 0;

	FlagsByteBuf = ( SubCarrierFlag   & 0x01 )        |
		 		 ( ( DataRateFlag     & 0x01 ) << 1 ) |
				 ( ( InventoryFlag    & 0x01 ) << 2 ) |
				 ( ( ProtExtFlag      & 0x01 ) << 3 ) |
				 ( ( SelectOrAFIFlag  & 0x01 ) << 4 ) |
				 ( ( AddrOrNbSlotFlag & 0x01 ) << 5 ) |
				 ( ( OptionFlag       & 0x01 ) << 6 ) |
				 ( ( RFUFlag          & 0x01 ) << 7 );

	return (int8_t)FlagsByteBuf;
}


/**
* @brief  	This function selects 15693 protocol according to input parameters
* @param  	DataRate	:  	tag data rate ( 6 or 26 or 52k)
* @param 	TimeOrSOF	: 	wait for SOF or respect 312 µs delay
* @param	Modulation	: 	10 or 100% modulation depth
* @param	SubCarrier	: 	single or double sub-carrier
* @param	AppendCRC	: 	if = 1 CR95HF computes the CRC command
* @retval 	RESULTOK   	: 	CR95HF return a successful code
* @retval 	ERRORCODE_GENERIC   : 	CR95HF return an error code or parameter error
*/
int8_t ISO15693_SelectProtocol( const uint8_t DataRate, const uint8_t TimeOrSOF, 
								const uint8_t Modulation, const uint8_t SubCarrier, 
								const uint8_t AppendCRC )
{
	uint8_t ParametersByte = 0;
	uint8_t pResponse[PROTOCOLSELECT_LENGTH];
	 
	 
	memset( pResponse, 0xFF, PROTOCOLSELECT_LENGTH );
	if ( ISO15693_IsAnAvailableDataRate( DataRate ) != CR95HF_SUCCESS_CODE )
	{
		return ERRORCODE_GENERIC;
	}
	 
	ParametersByte = ( ( AppendCRC  << ISO15693_OFFSET_APPENDCRC ) 	&  ISO15693_MASK_APPENDCRC ) |
					 ( ( SubCarrier << ISO15693_OFFSET_SUBCARRIER ) & ISO15693_MASK_SUBCARRIER ) |
					 ( ( Modulation << ISO15693_OFFSET_MODULATION ) & ISO15693_MASK_MODULATION ) |
					 ( ( TimeOrSOF  << ISO15693_OFFSET_WAITORSOF )  & ISO15693_MASK_WAITORSOF )  |
					 ( ( DataRate   << ISO15693_OFFSET_DATARATE )   & ISO15693_MASK_DATARATE );
	 
	if ( CR95HF_ProtocolSelect( ISO15693_SELECTLENGTH, ISO15693_PROTOCOL, &ParametersByte, pResponse ) != CR95HF_SUCCESS_CODE )
	{
		return ERRORCODE_GENERIC;
	}

	if ( CR95HF_IsReaderResultCodeOk( PROTOCOL_SELECT, pResponse ) == ERRORCODE_GENERIC )
	{
		return ERRORCODE_GENERIC;
	}

	
	// save the parameter of protocol in order to check coherence with request flag
	GloParameterSelected = ParametersByte; 
	 
	return RESULTOK;
}
 
 
/**
* @brief  this function returns RESULTOK if the data rate is available, otherwise ERRORCODE_GENERIC
* @param  	DataRate	:  	reader Data Rate
* @retval RESULTOK
* @retval ERRORCODE_GENERIC
*/
static int8_t ISO15693_IsAnAvailableDataRate( const uint8_t DataRate )
{
	switch (DataRate)
	{
		case ISO15693_TRANSMISSION_26:
			return RESULTOK;
		case ISO15693_TRANSMISSION_53:
			return RESULTOK;
		case ISO15693_TRANSMISSION_6:
			return RESULTOK;
		default : 
			return ERRORCODE_GENERIC;
	}
}


/**
* @brief  	this function send an inventory command to a contact-less tag.
* @param  	Flags		:  	Request flags
* @param	AFI			:	AFI byte (optional)
* @param	MaskLength 	: 	Number of bits of mask value
* @param	MaskValue	:  	mask value which is compare to Contact-less tag UID
* @param	AppendCRC	:  	CRC16 management. If set CR95HF appends CRC16.
* @param	CRC16		: 	pointer on CRC16 (optional) in case of user has chosen to manage CRC16 (see ProtocolSelect command CR95HF layer)
* @param	pResponse	: 	pointer on CR95HF response
* @retval 	RESULTOK	: 	CR95HF returns a successful code
* @retval 	ISO15693_ERRORCODE_PARAMETERLENGTH	: 	MaskLength value is erroneous
* @retval 	ERRORCODE_GENERIC	: 	 CR95HF returns an error code
*/
int8_t ISO15693_Inventory( const uint8_t Flags, const uint8_t AFI, 
						   const uint8_t MaskLength, const uint8_t *MaskValue, 
						   const uint8_t AppendCRC, const uint8_t *CRC16, 
						   uint8_t *pResponse )
{
	uint8_t NthByte = 0;
	uint8_t InventoryBuf[ISO15693_MAXLENGTH_INVENTORY];
	uint8_t NbMaskBytes = 0;
	uint8_t NbSignificantBits = 0;
	int8_t FirstByteMask;
	int8_t NthMaskByte = 0;
	
	// initialize the result code to 0xFF and length to 0  in case of error
	*pResponse = SENDRECV_ERRORCODE_SOFT;
	*(pResponse + 1) = 0x00;

	if ( MaskLength > ISO15693_NBBITS_MASKPARAMETER )
	{
		return ISO15693_ERRORCODE_PARAMETERLENGTH;
	}
	
	if ( ( ISO15693_IsInventoryFlag( Flags ) != CR95HF_SUCCESS_CODE ) || 
		 ( ISO15693_IsReaderConfigMatchWithFlag( GloParameterSelected, Flags ) != CR95HF_SUCCESS_CODE ) )
	{
		return ERRORCODE_GENERIC;
	}	
	
	InventoryBuf[NthByte++] = Flags;
	InventoryBuf[NthByte++] = ISO15693_CMDCODE_INVENTORY;
	
	if ( ISO15693_GetSelectOrAFIFlag( Flags ) == TRUE )
	{
		InventoryBuf[NthByte++] = AFI;
	}
	
	InventoryBuf[NthByte++] = MaskLength;

	if ( MaskLength != 0 )
	{
		// compute the number of bytes of mask value(2 border exceptions)
		if ( MaskLength == 64 )
		{
			NbMaskBytes = 8;
		}
		else
		{
			NbMaskBytes = MaskLength / 8 + 1;
		}
		
		NbSignificantBits = MaskLength - ( NbMaskBytes - 1 ) * 8;
		if ( NbSignificantBits != 0 )
		{
			FirstByteMask = ( 0x01 << NbSignificantBits ) - 1;
		}
		else
		{
			FirstByteMask = 0xFF;
		}
		
		// copy the mask value
		if ( NbMaskBytes > 1 )
		{
			for ( NthMaskByte = 0 ; NthMaskByte < NbMaskBytes - 1 ; NthMaskByte++ )
			{
				InventoryBuf[NthByte++] = MaskValue[NthMaskByte];
			}
		}
		
		if ( NbSignificantBits != 0 )
		{
			InventoryBuf[NthByte++] = MaskValue[NthMaskByte] & FirstByteMask;
		}
	}

	if ( AppendCRC == ISO15693_DONTAPPENDCRC )
	{
		InventoryBuf[NthByte++] = CRC16[0];
		InventoryBuf[NthByte++] = CRC16[1];
	}
	if ( CR95HF_SendRecv( NthByte, InventoryBuf, pResponse ) != CR95HF_SUCCESS_CODE )
	{
		return ERRORCODE_GENERIC;
	}

	if ( CR95HF_IsReaderResultCodeOk( SEND_RECEIVE, pResponse ) == ERRORCODE_GENERIC )
	{
		return ERRORCODE_GENERIC;
	}

	return RESULTOK;
}


/**
* @brief  	this function returns RESULTOK if Inventory flag is set
* @param  	FlagsByte	: the bytes containing the eight flags
* @retval 	Inventory flag
*/
int8_t ISO15693_IsInventoryFlag( const uint8_t FlagsByte )
{
	if ( ( FlagsByte & ISO15693_MASK_INVENTORYFLAG ) != 0x00 )
	{
		return RESULTOK;
	}
	else
	{
		return ERRORCODE_GENERIC;
	}
}


/**
* @brief  	this function returns RESULTOK if request flag is coherent with parameter of
* @brief  	the select command (reader command). the subcarrier and data rate shall be equal.
* @param  	Flags				:  	Request flags
* @param  	ParameterSelected	: Select parameter of ProtocolSelect command sent to CR95HF (see CR95HF_ProtocolSelect in lib_CR95HF.c)
* @retval 	RESULTOK : CR95HF configuration matches with Request flags
* @retval 	ERRORCODE_GENERIC : CR95HF config doesn't match with Request flags
*/
int8_t ISO15693_IsReaderConfigMatchWithFlag( const uint8_t ParameterSelected, const uint8_t Flags )
{
	// (acc to ISO spec) Data rate flag = 0 => low data rates
	// (acc to reader datasheet) Data rate value = 0b10 => 6k (low data rate)
	if ( ( ISO15693_GetDataRateFlag( Flags ) == FALSE ) & 
	   ( ( ParameterSelected & CR95HF_SELECTMASK_DATARATE ) != ( ISO15693_TRANSMISSION_6 << 4 ) ) )
	{
		return ERRORCODE_GENERIC;
	}
	
	// (acc to ISO spec) Data rate flag = 1 => high data rates
	// (acc to reader datasheet) Data rate value = 0b00 => 26k (high data rate)
	if ( ( ISO15693_GetDataRateFlag( Flags ) == TRUE ) & 
	   ( ( ParameterSelected & CR95HF_SELECTMASK_DATARATE ) != ISO15693_TRANSMISSION_26 ) )
	{
		return ERRORCODE_GENERIC;
	}
	
	// Sub carrier flag shall be equal to Sub carrier value
	if ( ( ISO15693_GetSubCarrierFlag( Flags ) == TRUE ) & 
	   ( ( ParameterSelected & CR95HF_SELECTMASK_SUBCARRIER ) != CR95HF_SELECTMASK_SUBCARRIER ) )
	{
		return ERRORCODE_GENERIC;
	}
	if ( ( ISO15693_GetSubCarrierFlag( Flags ) == FALSE ) & 
	   ( ( ParameterSelected & CR95HF_SELECTMASK_SUBCARRIER ) != 0 ) )
	{
		return ERRORCODE_GENERIC;
	}

	return RESULTOK;
}


/**
* @brief  	this function returns Data rate flag
* @param  	FlagsByte	: Request flags on one byte
* @retval 	data rate flag
*/
int8_t ISO15693_GetDataRateFlag( const uint8_t FlagsByte )
{
	if ( ( FlagsByte & ISO15693_MASK_DATARATEFLAG ) != 0x00 )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


/**
* @brief  	this function returns Sub carrier flag
* @param  	FlagsByte	: Request flags on one byte
* @retval 	SubCarrier flag
*/
int8_t ISO15693_GetSubCarrierFlag( const uint8_t FlagsByte )
{
	if ( ( FlagsByte & ISO15693_MASK_SUBCARRIERFLAG ) != 0x00 )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


/**
* @brief  	this function returns Select Or AFI flag  (depending on inventory flag)
* @param  	FlagsByte	: Request flags on one byte
* @retval 	Select Or AFI
*/
int8_t ISO15693_GetSelectOrAFIFlag( const uint8_t FlagsByte )
{
	if ( ( FlagsByte & ISO15693_MASK_SELECTORAFIFLAG ) != 0x00 )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


/**
* @brief  	this function splits inventory response. If the residue of tag response	is incorrect the function returns ERRORCODE_GENERIC, otherwise RESULTOK
* @param  	ReaderResponse	:  	pointer on CR95HF response
* @param	Length		:	Number of byte of Reader Response
* @param  	Flags		:  	Response flags
* @param	DSFIDextract: 	DSPID of tag response
* @param	UIDoutIndex		:  	index of UIDout of tag response
* @retval 	RESULTOK : Contactless tag response validated
* @retval 	CR95HF_ERROR_CODE : CR95HF returned an error code
* @retval 	ISO15693_ERRORCODE_CRCRESIDUE : CRC16 residue is erroneous
*/
int8_t ISO15693_SplitInventoryResponse( const uint8_t *ReaderResponse, const uint8_t Length, 
										uint8_t *Flags, uint8_t *DSFIDextract, uint8_t *UIDoutIndex )
{
	uint8_t	ResultCode;
	uint8_t	NbTagReplyByte;
	uint8_t	NbControlByte;
	uint8_t	TagReplyIndex;
	uint8_t	ControlIndex;

	SplitReaderReply( SEND_RECEIVE, PROTOCOL_TAG_ISO15693, ReaderResponse, 
					  &ResultCode, &NbTagReplyByte, &TagReplyIndex, &NbControlByte, 
					  &ControlIndex );
	
	// CR95HF returned an error code
	if ( ResultCode == CR95HF_ERROR_CODE )
	{
		return CR95HF_ERROR_CODE;
	}
	
	if ( ISO15693_IsCorrectCRC16Residue( &ReaderResponse[TagReplyIndex], NbTagReplyByte ) != CR95HF_SUCCESS_CODE )
	{
		return ISO15693_ERRORCODE_CRCRESIDUE;
	}
	
	*Flags = ReaderResponse[TagReplyIndex + ISO15693_OFFSET_FLAGS];
	*DSFIDextract =	ReaderResponse[TagReplyIndex + ISO15693_INVENTORYOFFSET_DSFID];

	*UIDoutIndex = TagReplyIndex + ISO15693_INVENTORYOFFSET_UID;
	
	return RESULTOK;
}


/**
* @brief  	this function computes the CRC16 residue as defined by CRC ISO/IEC 13239
* @param  	DataIn		:	input to data
* @param	Length 		: 	Number of bits of DataIn
* @retval 	RESULTOK  	:   CRC16 residue is correct
* @retval 	ERRORCODE_GENERIC  	:  CRC16 residue is false
*/
int8_t ISO15693_IsCorrectCRC16Residue( const uint8_t *DataIn, const uint8_t Length )
{
	int16_t ResCRC = 0;

	// check the CRC16 Residue
	if ( Length != 0 )
	ResCRC = ISO15693_CRC16( DataIn, Length );
	
	if ( ( ~ResCRC & 0xFFFF) != ISO15693_RESIDUECRC16 )
	{
		return ERRORCODE_GENERIC;
	}
		
	return RESULTOK;
}


/**
* @brief  	this function computes the CRC16 as defined by CRC ISO/IEC 13239
* @param  	DataIn		:	input data
* @param	NbByte 		: 	Number of byte of DataIn
* @retval	ResCrc		: 	CRC16 computed
*/
int16_t ISO15693_CRC16( const uint8_t *DataIn, const uint8_t NbByte )
{
	int8_t i, j;
	int32_t ResCrc = ISO15693_PRELOADCRC16;
	
	for ( i = 0 ; i < NbByte ; i++ )
	{
		ResCrc = ResCrc ^ DataIn[i];
		for ( j = 8 ; j > 0 ; j-- )
		{
			if ( ResCrc & ISO15693_MASKCRC16 )
			{
				ResCrc = ( ResCrc >> 1 ) ^ ISO15693_POLYCRC16;
			}
			else
			{
				ResCrc >>= 1;
			}
		}
	}

	return ( ~ResCrc & 0xFFFF );
}


/**
* @brief  	this function return a tag UID of a contactless tag
* @param  	UIDout		: 	UID of a tag in the field
* @retval 	RESULTOK	: 	CR95HF returns a succesful code
* @retval 	ERRORCODE_GENERIC	: 	 CR95HF returns an error code
*/
int8_t ISO15693_GetUID( uint8_t *UIDout )
{
	int8_t FlagsByteData;
	uint8_t	TagReply[ISO15693_NBBYTE_UID + 5];

	memset( UIDout, 0x00, ISO15693_NBBYTE_UID );
	
	// select 15693 protocol
	if ( ISO15693_SelectProtocol( ISO15693_TRANSMISSION_26,
									ISO15693_WAIT_FOR_SOF,
									ISO15693_MODULATION_100,
									ISO15693_SINGLE_SUBCARRIER,
									ISO15693_APPENDCRC ) != CR95HF_SUCCESS_CODE )
	{
		return ERRORCODE_GENERIC;
	}
							
	FlagsByteData = ISO15693_CreateRequestFlag( ISO15693_REQFLAG_SINGLESUBCARRIER,
												ISO15693_REQFLAG_HIGHDATARATE,
												ISO15693_REQFLAG_INVENTORYFLAGSET,
												ISO15693_REQFLAG_NOPROTOCOLEXTENSION,
												ISO15693_REQFLAG_NOTAFI,
												ISO15693_REQFLAG_1SLOT,
												ISO15693_REQFLAG_OPTIONFLAGNOTSET,
												ISO15693_REQFLAG_RFUNOTSET );
	
	if ( ISO15693_Inventory( FlagsByteData, 0x00, 0x00, 0x00, ISO15693_APPENDCRC, 0x00, TagReply ) != RESULTOK )
	{
		return ERRORCODE_GENERIC;
	}

	memcpy( UIDout, &(TagReply[TAGREPPLY_OFFSET_UID]), ISO15693_NBBYTE_UID );

	return RESULTOK;
}