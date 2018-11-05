/*
 * access_control.c
 *
 * Created: 11/2/2018 12:22:40
 *  Author: MMielke
 */ 

#include <atmel_start.h>
#include "access_control.h"
#include "clock_config.h"
#include <util/delay.h>
#include "timeout.h"
#include "led.h"

static absolutetime_t access_task( void *payload );
static timer_struct_t access_timer = { access_task };

static absolutetime_t access_task( void *payload )
{
	ACCESS_CONTROL_PIN_set_level( LOCK );
	LED_GREEN_set_level( LED_OFF );
	return 0;
}

void Access_Granted( void )
{
	LED_GREEN_set_level( LED_ON );
	ACCESS_CONTROL_PIN_set_level( UNLOCK );
	scheduler_timeout_create( &access_timer, ACCESS_DURATION );
}
