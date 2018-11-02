/*
 * wifi_service.h
 *
 * Created: 10/4/2018 2:22:04 PM
 *  Author: I17643
 */

#ifndef WIFI_SERVICE_H_
#define WIFI_SERVICE_H_

#include <stdint.h>

#define MAX_WIFI_CRED_LENGTH 31
struct wifi_params {
	char wifi_ssid[MAX_WIFI_CRED_LENGTH];
	char wifi_password[MAX_WIFI_CRED_LENGTH];
	char wifi_authType[2];
};
extern struct wifi_params wifi_params;

// If you pass a callback function in here it will be called when the AP state changes. Pass NULL if you do not want
// that.
void wifi_init(void (*funcPtr)(uint8_t), uint8_t mode);
void wifi_reinit();

#endif /* WIFI_SERVICE_H_ */
