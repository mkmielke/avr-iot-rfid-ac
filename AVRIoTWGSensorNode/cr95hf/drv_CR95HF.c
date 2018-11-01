/******************************************************************************/
/*                                 Includes                                   */
/******************************************************************************/
#include "drv_CR95HF.h"
#include "include/timeout.h"
#include "spi_basic.h"
#include "debug_print.h"


/******************************************************************************/
/*                            Private Functions                               */
/******************************************************************************/
absolutetime_t CR95HF_TimeoutTask(void *payload);


/******************************************************************************/
/*                            Private Variables                               */
/******************************************************************************/
volatile bool CR95HF_TimeOut;

timer_struct_t CR95HF_TimeoutTaskTimer = { CR95HF_TimeoutTask };


/******************************************************************************/
/*                           Function Definitions                             */
/******************************************************************************/
void delay_ms( uint32_t x )
{
	int i;
	for ( i = 0 ; i < x ; i++ )
	{
		_delay_ms( 1 );
	}
}


uint8_t SPI_exchange_byte(uint8_t data)
{
	return SPI_0_exchange_byte( data );
}


void SPI_exchange_block(void *block, uint8_t size)
{
	SPI_0_exchange_block( block, size );
}


/**
 *	@brief  Starts the time out used to avoid the STM32 freeze
 *  @param  delay : delay in milliseconds
 *  @return None.
 */
void StartTimeOut( uint16_t delay )
{
	/* Set the TimeOut flag to false */
	CR95HF_TimeOut = false;
	
	// Start the CR95HF Polling timeout
	scheduler_timeout_create( &CR95HF_TimeoutTaskTimer, delay );
}


/**
 *	@brief  Stop the timer used for the time out
 *  @param  None.
 *  @return None.
 */
void StopTimeOut( void )
{	
	/* Disable the Time out timer */
	scheduler_timeout_delete( &CR95HF_TimeoutTaskTimer );	
}


absolutetime_t CR95HF_TimeoutTask( void *payload )
{
	debug_printError( "READER: CR95HF Polling Timeout" );

	CR95HF_TimeOut = false;

	return 0;
}