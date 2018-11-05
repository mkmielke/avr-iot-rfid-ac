/*
\file   main.c

\brief  Main source file.

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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "application_manager.h"
#include "led.h"
#include "cr95hf/lib_iso15693.h"

ReaderConfigStruct ReaderConfig; 
uint8_t GloParameterSelected;	   	// select parameter

int8_t rfid_click_init( ReaderConfigStruct* ReaderConfig, CR95HF_INTERFACE bus )
{
	// initialize pins
	RFID_CLICK_SPI_CS_set_level( 1 );
	RFID_CLICK_INT_I_set_level( 1 );
	
	// select serial communication interface (SPI)
	RFID_CLICK_SSI0_set_level( 1 );
	RFID_CLICK_SSI1_set_level( 0 );
	
	ReaderConfig->Interface = bus;
	
	// TODO: is SPI bus initialized?
	
	return CR95HF_PORsequence();
}

int main(void)
{		
	application_init();
	
	if ( rfid_click_init( &ReaderConfig, CR95HF_INTERFACE_SPI ) != CR95HF_SUCCESS_CODE )
	{
		// If init fails, turn on red LED and stall
		LED_RED_set_level( LED_ON );
		while ( 1 );
	}

	while (1) 
	{
		runScheduler();
	}

	return 0;
}
