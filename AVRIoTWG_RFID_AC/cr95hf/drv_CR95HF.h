/**
  ******************************************************************************
  * @file    drv_CR95HF.h
  * @author  MMY Application Team
  * @version V0.2
  * @date    30/03/2011
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
#ifndef __DRIVER_CR95HF_H
#define __DRIVER_CR95HF_H


/******************************************************************************/
/*                                Includes                                    */
/******************************************************************************/
#include <string.h>
#include <stdbool.h>
#include "config/clock_config.h"
#include <util/delay.h>
#include "atmel_start_pins.h"


/******************************************************************************/
/*                                 Defines                                    */
/******************************************************************************/
#define DUMMY_BYTE									0xFF
#define MAX_BUFFER_SIZE  							256

#define FALSE	true
#define TRUE	false

//Chip Select handle for SPI interface
#define CR95HF_NSS_LOW()	RFID_CLICK_SPI_CS_set_level( 0 )
#define CR95HF_NSS_HIGH()  	RFID_CLICK_SPI_CS_set_level( 1 )

#define CR95HF_IRQIN_LOW() 	RFID_CLICK_INT_I_set_level( 0 )
#define CR95HF_IRQIN_HIGH() RFID_CLICK_INT_I_set_level( 1 )


/******************************************************************************/
/*                             Typedefs/Enums                                 */
/******************************************************************************/
typedef enum {
	CR95HF_INTERFACE_UART = 0,
	CR95HF_INTERFACE_SPI,
	CR95HF_INTERFACE_TWI
} CR95HF_INTERFACE;

typedef enum {
	SPI_POLLING = 0,
	SPI_INTERRUPT,
} SPI_MODE;


/******************************************************************************/
/*                             Public Functions                               */
/******************************************************************************/
uint8_t SPI_exchange_byte(uint8_t data);
void SPI_exchange_block(void *block, uint8_t size);
void StartTimeOut( uint16_t delay );
void StopTimeOut( void );

void delay_ms( uint32_t x ); 


#endif /* __CR95HF_H */

/******************* (C) COPYRIGHT 2010 STMicroelectronics *****END OF FILE****/



