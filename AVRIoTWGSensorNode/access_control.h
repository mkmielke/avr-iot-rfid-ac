/*
 * access_control.h
 *
 * Created: 11/2/2018 12:22:59
 *  Author: MMielke
 */ 


#ifndef ACCESS_CONTROL_H_
#define ACCESS_CONTROL_H_

#include <stdint.h>

#define UNLOCK true
#define LOCK false
#define ACCESS_DURATION 2000 // 2 seconds

void Access_Granted( void );

#endif /* ACCESS_CONTROL_H_ */