/******************************************************************************/
/*                                 Includes                                   */
/******************************************************************************/
#include "lib_CR95HF.h"


/******************************************************************************/
/*                                 Defines                                    */
/******************************************************************************/
/*** CR95HF SPI Control Bytes ***/
#define CR95HF_COMMAND_SEND							0x00
#define CR95HF_COMMAND_RESET						0x01
#define CR95HF_COMMAND_RECEIVE						0x02
#define CR95HF_COMMAND_POLLING						0x03

#define READERREPLY_STATUSOFFSET					0x00

#define IDN_RESULTSCODE_OK							0x00

//  send receive field status
#define SENDRECV_RESULTSCODE_OK						0x80
#define SENDRECV_ERRORCODE_COMERROR					0x86
#define SENDRECV_ERRORCODE_FRAMEWAIT				0x87
#define SENDRECV_ERRORCODE_SOF						0x88
#define SENDRECV_ERRORCODE_OVERFLOW					0x89
#define SENDRECV_ERRORCODE_FRAMING					0x8A
#define SENDRECV_ERRORCODE_EGT						0x8B
#define SENDRECV_ERRORCODE_LENGTH					0x8C
#define SENDRECV_ERRORCODE_CRC						0x8D
#define SENDRECV_ERRORCODE_RECEPTIONLOST			0x8E
//  Idle command field status
#define IDLE_RESULTSCODE_OK							0x00
#define IDLE_ERRORCODE_LENGTH						0x82
//  read register command field status
#define READREG_RESULTSCODE_OK						0x00
#define READREG_ERRORCODE_LENGTH					0x82
//  write register command field status
#define WRITEREG_RESULTSCODE_OK						0x00

#define CR95HF_FLAG_DATA_READY						0x08
#define CR95HF_FLAG_DATA_READY_MASK					0xF8

/* Nb of bytes of reader commands */
#define SELECT_BUFFER_SIZE							6
#define SENDRECV_BUFFER_SIZE						257
#define SEND_BUFFER_SIZE							257
#define IDLE_BUFFER_SIZE							16
#define RDREG_BUFFER_SIZE							5
#define WRREG_BUFFER_SIZE							257
#define BAUDRATE_BUFFER_SIZE						3
#define SUBFREQRES_BUFFER_SIZE						2
#define ACFILTER_BUFFER_SIZE						19
#define TESTMODE_BUFFER_SIZE						4
#define SLEEPMODE_BUFFER_SIZE						4
/* Nb of bytes of reader response */
#define CR95HF_RESPONSEBUFFER_SIZE		 			255




/******************************************************************************/
/*                            Global Variables                                */
/******************************************************************************/
extern ReaderConfigStruct ReaderConfig;
extern volatile bool CR95HF_TimeOut;


/******************************************************************************/
/*                            Private Functions                               */
/******************************************************************************/
int8_t 	SPIUART_SendReceive( const uint8_t *pCommand, uint8_t *pResponse );
void CR95HF_Send_SPI_Command( const uint8_t *pData ); // TODO: static?
static void CR95HF_Receive_SPI_Response(uint8_t *pData);
void CR95HF_Send_IRQIN_NegativePulse( void );
void CR95HF_Send_SPI_ResetSequence( void );
static int8_t CR95HF_PollingCommand( int timeout );
static uint8_t IsAnAvailableProtocol( uint8_t Protocol );
static uint8_t IsAnAvailableSelectLength( uint8_t Protocol, uint8_t Length );
static uint8_t IsAnAvailableSelectParameters( const uint8_t Protocol, const uint8_t Length, const uint8_t *parameters );
static uint8_t ForceSelectRFUBitsToNull( const uint8_t Protocol, const uint8_t Length, uint8_t *parameters );
static int8_t GetNbControlByte( int8_t ProtocolSelected );


/******************************************************************************/
/*                           Function Definitions                             */
/******************************************************************************/
/**
 *	@brief  this function send a command to CR95HF device over SPI or UART bus and receive its response
 *  @param  *pCommand  : pointer on the buffer to send to the CR95HF ( Command | Length | Data)
 *  @param  *pResponse : pointer on the CR95HF response ( Command | Length | Data)
 *  @retval 
 */
int8_t SPIUART_SendReceive(const uint8_t *pCommand, uint8_t *pResponse)
{
	int8_t i = 0;

	*pResponse = CR95HF_ERRORCODE_DEFAULT;
	*(pResponse + 1) = 0x00;

	if ( ReaderConfig.Interface == CR95HF_INTERFACE_SPI )
	{
		// First step  - Sending command 
		CR95HF_Send_SPI_Command( pCommand );
		
		for ( i = 0 ; i < 50 ; i++ );
		
		// Second step - Polling
		if ( CR95HF_PollingCommand( 1000 ) != CR95HF_SUCCESS_CODE )
		{	
			*pResponse = CR95HF_ERRORCODE_TIMEOUT;
			return CR95HF_POLLING_CR95HF;	
		}
		
		for ( i = 0 ; i < 50 ; i++ );
		
		// Third step  - Receiving bytes 
		CR95HF_Receive_SPI_Response( pResponse );
	}
	else if ( ReaderConfig.Interface == CR95HF_INTERFACE_UART )
	{/*** NOT IMPLEMENTED ***
		// First step  - Sending command
		CR95HF_Send_UART_Command( pCommand );
		// Second step - Receiving bytes 
		CR95HF_Receive_UART_Response( pResponse ); */
	}

	return CR95HF_SUCCESS_CODE; 
}


/**
 *	@brief  this function send a command over SPI bus
 *  @param  *pData : pointer on data to send to the CR95HF
 *  @return None
 */
//static void CR95HF_Send_SPI_Command(uc8 *pData)
void CR95HF_Send_SPI_Command( const uint8_t *pData )
{
	uint8_t Buffer[MAX_BUFFER_SIZE];
	  
	// Select CR95HF over SPI 
	CR95HF_NSS_LOW();

	// Send a sending request to CR95HF 
	SPI_exchange_byte( CR95HF_COMMAND_SEND );

	if ( *pData == ECHO )
	{
		// Send a sending request to CR95HF
		SPI_exchange_byte( ECHO );
	}
	else
	{
		// TODO: Necessary? Can I just use pData? Should I?
		memcpy( Buffer, pData, pData[CR95HF_LENGTH_OFFSET] + CR95HF_DATA_OFFSET );
		// Transmit the buffer over SPI
		SPI_exchange_block( Buffer, pData[CR95HF_LENGTH_OFFSET] + CR95HF_DATA_OFFSET );
	}

	//De-select CR95HF over SPI 
	CR95HF_NSS_HIGH();
}


/**
 *	@brief  this function recovers a response from CR95HF device
 *  @param  *pData : pointer on data received from CR95HF device
 *  @return None
 */
static void CR95HF_Receive_SPI_Response(uint8_t *pData)
{
	uint8_t DummyBuffer[MAX_BUFFER_SIZE];

	// Select CR95HF over SPI 
	CR95HF_NSS_LOW();

	// Request a response from CR95HF 
	SPI_exchange_byte( CR95HF_COMMAND_RECEIVE );

	// Recover the "Command" byte 
	pData[CR95HF_COMMAND_OFFSET] = SPI_exchange_byte( DUMMY_BYTE );

	if ( pData[CR95HF_COMMAND_OFFSET] == ECHO )
	{
		 pData[CR95HF_LENGTH_OFFSET]  = 0x00;
	}
	else if ( pData[CR95HF_COMMAND_OFFSET] == 0xFF )
	{
		pData[CR95HF_LENGTH_OFFSET]  = 0x00;
	}
	else
	{
		// Recover the "Length" byte 
		pData[CR95HF_LENGTH_OFFSET] = SPI_exchange_byte( DUMMY_BYTE );
		// Checks the data length 
		if ( pData[CR95HF_LENGTH_OFFSET] != 0x00 )
		{
			// Recover data TODO: TEST! remove DummyBuffer?
			SPI_exchange_block( &pData[CR95HF_DATA_OFFSET], pData[CR95HF_LENGTH_OFFSET] );
		}
		
	}

	// De-select CR95HF over SPI 
	CR95HF_NSS_HIGH();

}


/**
 *	@brief  Send a negative pulse on IRQin pin
 *  @param  none
 *  @return None
 */
void CR95HF_Send_IRQIN_NegativePulse( void )
{
	CR95HF_IRQIN_HIGH() ;
	delay_ms( 1 );
	CR95HF_IRQIN_LOW() ;
	delay_ms( 1 );
	CR95HF_IRQIN_HIGH() ;
}


/**
 *	@brief  Send a reset sequence over SPI bus (Reset command + wait 5µs + 
 *	@brief  negative pulse on IRQin.
 *  @param  none
 *  @return None
 */
void CR95HF_Send_SPI_ResetSequence( void )
{
	// Select CR95HF over SPI 
	CR95HF_NSS_LOW();
	// Send reset control byte
	SPI_exchange_byte( CR95HF_COMMAND_RESET );
	// De-select CR95HF over SPI 
	CR95HF_NSS_HIGH();	
	delay_ms( 10 );
	// send a pulse on IRQ_in
	CR95HF_Send_IRQIN_NegativePulse();

}


/**
 *	@brief  this function send a SendRecv command to CR95HF
 *  @param  Length 		: Number of bytes
 *  @param	Parameters 	: data depenps on protocl selected
 *  @param  pResponse : pointer on CR95HF response
 *  @return CR95HF_SUCCESS_CODE : the command was succedfully sent
 *  @return CR95HF_ERROR_CODE : CR95HF returned an error code
 *  @return CR95HF_ERRORCODE_PARAMETERLENGTH : Length parameter is erroneous
 */
int8_t CR95HF_SendRecv( const uint8_t Length, const uint8_t *Parameters, uint8_t *pResponse )
{
	uint8_t DataToSend[SENDRECV_BUFFER_SIZE];							  
	uint8_t i = 0;

	// initialize the result code to 0xFF and length to 0
	*pResponse = CR95HF_ERRORCODE_DEFAULT;
	*(pResponse + 1) = 0x00;
	

	// check the function parameters
	if ( ( Length < 1 ) || ( Length > 255 ) )
	{
		return CR95HF_ERRORCODE_PARAMETERLENGTH; 
	}

	DataToSend[CR95HF_COMMAND_OFFSET] = SEND_RECEIVE;
	DataToSend[CR95HF_LENGTH_OFFSET] = Length;

	// DataToSend CodeCmd Length Data
	// Parameters[0] first byte to emit
	for (i = 0 ; i < Length ; i++ )
	{
		DataToSend[CR95HF_DATA_OFFSET + i ] = Parameters[i];
	}

	SPIUART_SendReceive( DataToSend, pResponse );

	if ( CR95HF_IsReaderResultCodeOk( SEND_RECEIVE, pResponse ) != CR95HF_SUCCESS_CODE )
	{
		return CR95HF_ERROR_CODE;
	}

	return CR95HF_SUCCESS_CODE;
}


/**
 *	@brief  Send Echo command
*  @param  pResponse : pointer on CR95HF response
 *  @retval CR95HF_SUCCESS_CODE : the command was successfully sent
 */
int8_t CR95HF_Echo(uint8_t *pResponse)
{
	const uint8_t command[] = { ECHO };

	SPIUART_SendReceive( command, pResponse );

	return CR95HF_SUCCESS_CODE;
}


/**
 *	@brief  this function send polling control byte 
 *  @param  timeout : timeout in ms
 *  @return None
 */
static int8_t CR95HF_PollingCommand( int timeout )
{
	uint8_t Polling_Status = 0;

	//StartTimeOut( 10000 );		
	CR95HF_TimeOut = FALSE;
	
	if ( ReaderConfig.SpiMode == SPI_POLLING )
	{
		CR95HF_NSS_LOW();
		
		while ( Polling_Status != CR95HF_FLAG_DATA_READY && CR95HF_TimeOut != TRUE )
		{
			// Send a polling request to CR95HF 
			SPI_exchange_byte( CR95HF_COMMAND_POLLING );
	
			// poll the CR95HF until he's ready ! 
			Polling_Status = SPI_exchange_byte( CR95HF_COMMAND_POLLING );
			Polling_Status &= CR95HF_FLAG_DATA_READY_MASK;
			
			// TODO: use a timer!!!
			if ( timeout <= 0 )
			{
				CR95HF_TimeOut = TRUE;
			}
			else
			{
				delay_ms( 5 );
				timeout -= 5;
			}					
		}
		
		CR95HF_NSS_HIGH();
	}	
	else if ( ReaderConfig.SpiMode == SPI_INTERRUPT )
	{/*** NOT IMPLEMENTED ***
		// reset the CR95HF data status 
		CR95HF_DataReady = FALSE;
		
		// Enable Interrupt on the falling edge on the IRQ pin of CR95HF 
		EXTI_ClearITPendingBit( EXTI_CR95HF_LINE );
		EXTI->IMR |= EXTI_CR95HF_LINE;

		// Wait a low level on the IRQ pin or the timeout 
		//while( (CR95HF_DataReady != TRUE) && CR95HF_TimeOut != TRUE );
		while( ( CR95HF_DataReady == FALSE ) & ( CR95HF_TimeOut == FALSE ) );

		if ( CR95HF_TimeOut == TRUE )
		{		
			// Disable CR95HF EXTI 
			EXTI->IMR &= ~EXTI_CR95HF_LINE;
		} */
	}

	//StopTimeOut();

	if ( CR95HF_TimeOut == TRUE )
	{
		return CR95HF_POLLING_TIMEOUT;
	}

	return CR95HF_SUCCESS_CODE;	
}


/**
 *	@brief  This function send POR sequence. It might be use to initialize CR95HF after a POR.
 *  @param  none
 *  @return CR95HF_ERRORCODE_PORERROR : the POR sequence doesn't succeeded
 *  @return CR95HF_SUCCESS_CODE : CR95HF is ready
 */
int8_t CR95HF_PORsequence( void )
{
	uint8_t pResponse[10], NthAttempt = 1;

	do
	{
		// send an ECHO command and checks CR95HF response 		
		CR95HF_Echo( pResponse );
		if ( pResponse[0] == ECHORESPONSE )
		{
			return CR95HF_SUCCESS_CODE;
		}
		else if ( ReaderConfig.Interface == CR95HF_INTERFACE_SPI )
		{	
			// send a pulse on IRQ in case of the chip is on sleep state
			if ( NthAttempt == 2 )
			{
				CR95HF_Send_IRQIN_NegativePulse();
			}
			else if ( ReaderConfig.Interface == CR95HF_INTERFACE_SPI )
			{
				CR95HF_Send_SPI_ResetSequence();
			}
		}
		else
		{
			NthAttempt = 5;
		}
		
		delay_ms( 50 );
	} while ( pResponse[0] != ECHORESPONSE && NthAttempt++ < 5 );

	return CR95HF_ERRORCODE_PORERROR;
}


/**
 *	@brief  this function send a ProtocolSeclect command to CR95HF
 *  @param  Length  : number of byte of protocol select command parameters
 *  @param  Protocol : RF protocol (ISO 14443 A or B or 15 693 or Fellica)
 *  @param  Parameters: protocol parameters (see reader datasheet)
 *  @param  pResponse : pointer on CR95HF response
 *  @return CR95HF_SUCCESS_CODE : the command was successfully sent
 *  @return CR95HF_ERRORCODE_PARAMETERLENGTH : the Length parameter is erroneous
 *  @return CR95HF_ERRORCODE_PARAMETER : a parameter is erroneous
 */
int8_t CR95HF_ProtocolSelect( const uint8_t Length, const uint8_t Protocol, const uint8_t *Parameters, uint8_t *pResponse )
{
	uint8_t DataToSend[SELECT_BUFFER_SIZE];
	uint8_t SelectParameters[SELECT_BUFFER_SIZE];
	int8_t i = 0; 

	if ( ( Length < 1 ) || ( Length > SELECT_BUFFER_SIZE ) )
	{
		return CR95HF_ERRORCODE_PARAMETERLENGTH;
	}
	
	// initialize the result code to 0xFF and length to in case of error
	*pResponse = CR95HF_ERRORCODE_DEFAULT;
	*(pResponse + 1) = 0x00;

	// check the function parameters
	if ( ( IsAnAvailableProtocol( Protocol ) != CR95HF_SUCCESS_CODE ) ||
		 ( IsAnAvailableSelectLength( Protocol, Length ) != CR95HF_SUCCESS_CODE ) ||
		 ( IsAnAvailableSelectParameters( Protocol, Length, Parameters ) != CR95HF_SUCCESS_CODE ) )
	{
		return CR95HF_ERRORCODE_PARAMETER;
	}

	memcpy( SelectParameters, Parameters, Length );

	// force the RFU bits to 0
	if ( ForceSelectRFUBitsToNull( Protocol, Length, SelectParameters ) != CR95HF_SUCCESS_CODE )
	{
		return CR95HF_ERRORCODE_PARAMETER;
	}

	DataToSend[CR95HF_COMMAND_OFFSET] = PROTOCOL_SELECT;
	DataToSend[CR95HF_LENGTH_OFFSET] = Length;
	DataToSend[CR95HF_DATA_OFFSET] = Protocol;

	// DataToSend CodeCmd Length Data
	// Parameters[0] first byte to emit
	for ( i = 0 ; i < Length - 1 ; i++ )
	{
		DataToSend[CR95HF_DATA_OFFSET + 1 + i] = SelectParameters[i];
	}

  	SPIUART_SendReceive( DataToSend, pResponse );

	return CR95HF_SUCCESS_CODE;	
}


/**
 *	@brief  this functions returns CR95HF_SUCCESS_CODE if the protocol is available, otherwise CR95HF_ERRORCODE_PARAMETER
 *  @param  Protocol : RF protocol (ISO 14443 A or B or 15 693 or Fellica)
 *  @return CR95HF_SUCCESS_CODE	: the protocol is available
 *  @return CR95HF_ERRORCODE_PARAMETER : the protocol isn't available
 */
static uint8_t IsAnAvailableProtocol( uint8_t Protocol ) 
{
	switch( Protocol )
	{
		case PROTOCOL_TAG_FIELDOFF:
			return CR95HF_SUCCESS_CODE;
		case PROTOCOL_TAG_ISO15693:
			return CR95HF_SUCCESS_CODE;
		case PROTOCOL_TAG_ISO14443A:
			return CR95HF_SUCCESS_CODE;
		case PROTOCOL_TAG_ISO14443B:
			return CR95HF_SUCCESS_CODE;
		case PROTOCOL_TAG_FELICA:
			return CR95HF_SUCCESS_CODE;
		default: 
			return CR95HF_ERRORCODE_PARAMETER;
	}	
}


/**
 *	@brief  this functions returns CR95HF_SUCCESS_CODE if length value is allowed, otherwise ERRORCODE_GENERIC
 *	@brief  this functions is used by ProtocolSelect function.
 *  @param  Protocol : RF protocol (ISO 14443 A or B or 15 693 or Fellica)
 *  @param  Length : Number of byte of Protocol Select command
 *  @return CR95HF_SUCCESS_CODE	: the length value is correct
 *  @return ERRORCODE_GENERIC : the length value isn't correct
 */
static uint8_t IsAnAvailableSelectLength( uint8_t Protocol, uint8_t Length ) 
{
	switch( Protocol )
	{
		case PROTOCOL_TAG_FIELDOFF:
			if (Length == 2 )
				return CR95HF_SUCCESS_CODE;
			else 
				return ERRORCODE_GENERIC;
		case PROTOCOL_TAG_ISO15693:
			if ( Length == 2 )
				return CR95HF_SUCCESS_CODE;
			else 
				return ERRORCODE_GENERIC;
		case PROTOCOL_TAG_ISO14443A:
			// length == 2 protocol + parameters 
			// length == 4 protocol + parameters + AFDT (2 bytes)
			if ( Length == 2 || Length == 4 )
				return CR95HF_SUCCESS_CODE;
			else 
				return CR95HF_ERRORCODE_PARAMETER;
		case PROTOCOL_TAG_ISO14443B:
			if ( Length == 2 || Length == 4 )
				return CR95HF_SUCCESS_CODE;
			else 
				return CR95HF_ERRORCODE_PARAMETER;
		case PROTOCOL_TAG_FELICA:
			if ( Length == 2 )
				return CR95HF_SUCCESS_CODE;
			else 
				return CR95HF_ERRORCODE_PARAMETER;
		default: 
			return CR95HF_ERRORCODE_PARAMETER;
	}	
}


/**
 *	@brief  this functions returns CR95HF_SUCCESS_CODE if parameter value is correct, otherwise ERRORCODE_GENERIC
 *	@brief  this functions is used by ProtocolSelect function.
 *  @param  Protocol : RF protocol (ISO 14443 A or B or 15 693 or Fellica)
 *  @param  Length : Number of byte of parameters
 *  @param  parameters : pointer on parameter of ProtocolSelect parameter
 *  @return CR95HF_SUCCESS_CODE	: the parameter value is correct
 *  @return CR95HF_ERRORCODE_PARAMETER : the length value isn't correct
 */
static uint8_t IsAnAvailableSelectParameters( const uint8_t Protocol, const uint8_t Length, const uint8_t *parameters ) 
{
	switch( Protocol )
	{
		case PROTOCOL_TAG_FIELDOFF:
			return CR95HF_SUCCESS_CODE;
		case PROTOCOL_TAG_ISO15693:
			if ( ( parameters[0] & 0x30 ) == 0x30 )
				return CR95HF_ERRORCODE_PARAMETER;
			else 
				return CR95HF_SUCCESS_CODE;
		case PROTOCOL_TAG_ISO14443A:
			return CR95HF_SUCCESS_CODE;
		case PROTOCOL_TAG_ISO14443B:
			return CR95HF_SUCCESS_CODE;
		case PROTOCOL_TAG_FELICA:
			if ( ( ( parameters[0] & 0x30 ) == 0x30 ) || ( ( parameters[0] & 0x30 ) == 0x00 ) )
				return CR95HF_ERRORCODE_PARAMETER;
			return CR95HF_SUCCESS_CODE;
		default: 
			return CR95HF_ERRORCODE_PARAMETER;
	}	
}


/**
 *	@brief  this functions reset RFU bits of parameters
 *  @param  Protocol : RF protocol (ISO 14443 A or B or 15 693 or Fellica)
 *  @param  Length : Number of byte of parameters
 *  @param  parameters : pointer on parameter of ProtocolSelect parameter 
 *  @return CR95HF_SUCCESS_CODE	: RFU bits was reset
 *  @return CR95HF_ERRORCODE_PARAMETER : the protocol isn't available
 */
static uint8_t ForceSelectRFUBitsToNull( const uint8_t Protocol, const uint8_t Length, uint8_t *parameters ) 
{
	switch( Protocol )
	{
		case PROTOCOL_TAG_FIELDOFF:
			parameters[0] = 0x00;
			return CR95HF_SUCCESS_CODE;
		case PROTOCOL_TAG_ISO15693:
			// bit 7:6 RFU
			parameters[0] &= 0x3F;
			return CR95HF_SUCCESS_CODE;
		case PROTOCOL_TAG_ISO14443A:
			//bits 2:0 of byte 0 defined as RFU
			parameters[Length - 2] &= 0xF8;
			return CR95HF_SUCCESS_CODE;
		case PROTOCOL_TAG_ISO14443B:
			//bits 3:1 of byte 0 defined as RFU
			parameters[Length - 2] &= 0xF1;
			return CR95HF_SUCCESS_CODE;
		case PROTOCOL_TAG_FELICA:
			// bits 3:1 & 7:6 of byte 0 defined as RFU
			parameters[1] &= 0x31;
			parameters[0] &= 0x1F;
			return CR95HF_SUCCESS_CODE;
		default: 
			return CR95HF_ERRORCODE_PARAMETER;
	}	
}


/**
* @brief  	this function returns CR95HF_SUCCESS_CODE is the reader reply is a successful code.
* @param  	CmdCode		:  	code command send to the reader
* @param  	ReaderReply	:  	pointer on CR95HF response
* @retval  	CR95HF_SUCCESS_CODE :  CR95HF returned a successful code
* @retval  	CR95HF_ERROR_CODE  :  CR95HF didn't return a successful code
* @retval  	CR95HF_NOREPLY_CODE : no CR95HF response
*/
int8_t CR95HF_IsReaderResultCodeOk( uint8_t CmdCode, const uint8_t *ReaderReply )
{
	CmdCode = CmdCode & 0xFF;

	if ( ReaderReply[READERREPLY_STATUSOFFSET] == CR95HF_ERRORCODE_DEFAULT )
	{
		return CR95HF_NOREPLY_CODE;
	}

	switch ( CmdCode )
	{
		case ECHO:
			if ( ReaderReply[ECHOREPLY_OFFSET] == ECHO )
				return CR95HF_SUCCESS_CODE;
			else
				return CR95HF_ERROR_CODE;
		case IDN:
			if ( ReaderReply[READERREPLY_STATUSOFFSET] == IDN_RESULTSCODE_OK )
				return CR95HF_SUCCESS_CODE;
			else
				return CR95HF_ERROR_CODE;
		case PROTOCOL_SELECT:
			switch ( ReaderReply[READERREPLY_STATUSOFFSET] )
			{
				case IDN_RESULTSCODE_OK:
					return CR95HF_SUCCESS_CODE;
				case PROTOCOLSELECT_ERRORCODE_CMDLENGTH:
					return CR95HF_ERROR_CODE;
				case PROTOCOLSELECT_ERRORCODE_INVALID:
					return CR95HF_ERROR_CODE;
				default:
					return CR95HF_ERROR_CODE;
			}
		case SEND_RECEIVE:
			switch ( ReaderReply[READERREPLY_STATUSOFFSET] )
			{
				case SENDRECV_RESULTSCODE_OK:
					return CR95HF_SUCCESS_CODE;
				case SENDRECV_ERRORCODE_COMERROR:
					return CR95HF_ERROR_CODE;
				case SENDRECV_ERRORCODE_FRAMEWAIT:
					return CR95HF_ERROR_CODE;
				case SENDRECV_ERRORCODE_SOF:
					return CR95HF_ERROR_CODE;
				case SENDRECV_ERRORCODE_OVERFLOW:
					return CR95HF_ERROR_CODE;
				case SENDRECV_ERRORCODE_FRAMING:
					return CR95HF_ERROR_CODE;
				case SENDRECV_ERRORCODE_EGT:
					return CR95HF_ERROR_CODE;
				case SENDRECV_ERRORCODE_LENGTH:
					return CR95HF_ERROR_CODE;
				case SENDRECV_ERRORCODE_CRC:
					return CR95HF_ERROR_CODE;
				case SENDRECV_ERRORCODE_RECEPTIONLOST:
					return CR95HF_ERROR_CODE;
				default:
					return CR95HF_ERROR_CODE;
			}
		case IDLE:
			switch ( ReaderReply[READERREPLY_STATUSOFFSET] )
			{
				case IDLE_RESULTSCODE_OK:
					return CR95HF_SUCCESS_CODE;
				case IDLE_ERRORCODE_LENGTH:
					return CR95HF_ERROR_CODE;
				default: 
					return CR95HF_ERROR_CODE;
			}
		case READ_REGISTER:
			switch ( ReaderReply[READERREPLY_STATUSOFFSET] )
			{
				case READREG_RESULTSCODE_OK:
					return CR95HF_SUCCESS_CODE;
				case READREG_ERRORCODE_LENGTH:
					return CR95HF_ERROR_CODE;
				default:
					return CR95HF_ERROR_CODE;
			}
		case WRITE_REGISTER:
			switch ( ReaderReply[READERREPLY_STATUSOFFSET] )
			{
				case WRITEREG_RESULTSCODE_OK:
					return CR95HF_SUCCESS_CODE;
				default: 
					return CR95HF_ERROR_CODE;
			}
		case BAUD_RATE:
			return CR95HF_ERROR_CODE;
		default:
			return ERRORCODE_GENERIC;
	}
}


/**
* @brief    this function split a CR95HF response and extract the different fields
* @param  	CmdCodeToReader : code command send to the reader
* @param	ProtocolSelected : protocol selected (select command)
* @param 	ReaderReply 	: reader reply
* @param 	ResultCode		: TRUE is response is Ok, false otherwise
* @param 	NbTagByte		: Number of byte of tag reply
* @param 	TagReplyDataIndex : data of the reader reply is exists
* @param 	NbControlByte	: Number of control byte
* @param 	ControlIndex	: control byte(s)
* @retval 	CR95HF_SUCCESS_CODE : CR95HF was successful slitted
* @retval 	CR95HF_ERRORCODE_COMMANDUNKNOWN : The command code is unknown
*/
int8_t SplitReaderReply( uint8_t CmdCodeToReader, uint8_t ProtocolSelected,
						 const uint8_t *ReaderReply, uint8_t *ResultCode,
						 uint8_t *NbTagByte, uint8_t *TagReplyDataIndex,
						 uint8_t *NbControlByte, uint8_t *ControlIndex )
{
	// output parameters initialization
	*NbTagByte = 0;
	*ResultCode = CR95HF_ERROR_CODE;

	if ( CR95HF_IsCommandExists( CmdCodeToReader ) != CR95HF_SUCCESS_CODE )
	{
		return CR95HF_ERRORCODE_COMMANDUNKNOWN;
	}
	
	*ResultCode = CR95HF_SUCCESS_CODE;

	// the ECHO and Baud rate commands reply with a pseudo response (0x55)
	if ( ( CmdCodeToReader == ECHO ) || ( CmdCodeToReader == BAUD_RATE ) )
	{
		//memcpy(TagReplyData,&ReaderReply[PSEUDOREPLY_OFFSET],PSEUDOREPLY_LENGTH);
		*TagReplyDataIndex = ECHOREPLY_OFFSET;
		*NbTagByte = ECHOREPLY_LENGTH;
	}
	else if ( CR95HF_IsReaderResultCodeOk( CmdCodeToReader, ReaderReply ) == CR95HF_SUCCESS_CODE )
	{
		// reply 00 00 (Length = 0)
		if ( ( ( ReaderReply[READERREPLY_STATUSOFFSET] & 0x80 ) == 0 ) &&
			 ( ReaderReply[READERREPLY_STATUSOFFSET + 1] == 0x00 ) )
		{
			//memcpy(TagReplyData,&ReaderReply[CR95HF_DATA_OFFSET-1],1);
			*TagReplyDataIndex = CR95HF_DATA_OFFSET;
			*NbTagByte = 1;
		}
		// case (Length > 0)
		else
		{
			*NbControlByte = GetNbControlByte( ProtocolSelected );
			*TagReplyDataIndex = CR95HF_DATA_OFFSET;
			*ControlIndex = CR95HF_DATA_OFFSET + *NbTagByte;
			*NbTagByte = ReaderReply[CR95HF_LENGTH_OFFSET] - *NbControlByte;
		}
	}
	// Error Code
	else
	{
		//memcpy(TagReplyData,&ReaderReply[CR95HF_DATA_OFFSET-2],ERRORCODE_LENGTH);
		*TagReplyDataIndex = CR95HF_DATA_OFFSET - 2;
		*NbTagByte = ERRORCODE_LENGTH;
		*ResultCode = CR95HF_ERROR_CODE;
	}
	
	return CR95HF_SUCCESS_CODE;
}


/**
* @brief  this function returns TRUE is the command code exists, False otherwise
* @param  	CmdCode		:  	code command send to the reader
* @retval boolean
*/
int8_t CR95HF_IsCommandExists( uint8_t CmdCode )
{
	CmdCode = CmdCode & 0x0F;
	switch ( CmdCode )
	{
		case ECHO:
		return CR95HF_SUCCESS_CODE;
		case IDN:
		return CR95HF_SUCCESS_CODE;
		case PROTOCOL_SELECT:
		return CR95HF_SUCCESS_CODE;
		case SEND_RECEIVE:
		return CR95HF_SUCCESS_CODE;
		case IDLE:
		return CR95HF_SUCCESS_CODE;
		case READ_REGISTER:
		return CR95HF_SUCCESS_CODE;
		case WRITE_REGISTER:
		return CR95HF_SUCCESS_CODE;
		case BAUD_RATE:
		return CR95HF_SUCCESS_CODE;
		default:
		return CR95HF_ERRORCODE_COMMANDUNKNOWN;
	}
}


/**
* @brief  	this function returns the number of control byte according to RF protocol
* @param  	ProtocolSelected : Rf protocol selected
* @retval 	CONTROL_15693_NBBYTE 	: number of control byte for 15 693 protocol
* @retval 	CONTROL_14443A_NBBYTE	: number of control byte for 14 443 A protocol
* @retval 	CONTROL_14443B_NBBYTE  	: number of control byte for 14 443 B protocol
* @retval 	CONTROL_FELICA_NBBYTE  	: number of control byte for Felica protocol
* @retval 	0 : error the protocol in unknown
*/
static int8_t GetNbControlByte( int8_t ProtocolSelected )
{
	switch(ProtocolSelected)
	{
		case PROTOCOL_TAG_ISO15693:
		return CONTROL_15693_NBBYTE;
		case PROTOCOL_TAG_ISO14443A:
		return CONTROL_14443A_NBBYTE;
		case PROTOCOL_TAG_ISO14443B:
		return CONTROL_14443B_NBBYTE;
		case PROTOCOL_TAG_FELICA:
		return CONTROL_FELICA_NBBYTE;
		default:
		return 0;
	}
}