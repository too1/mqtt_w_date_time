/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <stdio.h>
#include <drivers/uart.h>
#include <string.h>

#include <net/mqtt.h>
#include <net/socket.h>
#include <modem/lte_lc.h>
#if defined(CONFIG_LWM2M_CARRIER)
#include <lwm2m_carrier.h>
#endif
#include "app_mqtt.h"
#include <date_time.h>

#if defined(CONFIG_BSD_LIBRARY)

/**@brief Recoverable BSD library error. */
void bsd_recoverable_error_handler(uint32_t err)
{
	printk("bsdlib recoverable error: %u\n", (unsigned int)err);
}

#endif /* defined(CONFIG_BSD_LIBRARY) */

#if defined(CONFIG_LWM2M_CARRIER)
K_SEM_DEFINE(carrier_registered, 0, 1);

void lwm2m_carrier_event_handler(const lwm2m_carrier_event_t *event)
{
	switch (event->type) {
	case LWM2M_CARRIER_EVENT_BSDLIB_INIT:
		printk("LWM2M_CARRIER_EVENT_BSDLIB_INIT\n");
		break;
	case LWM2M_CARRIER_EVENT_CONNECT:
		printk("LWM2M_CARRIER_EVENT_CONNECT\n");
		break;
	case LWM2M_CARRIER_EVENT_DISCONNECT:
		printk("LWM2M_CARRIER_EVENT_DISCONNECT\n");
		break;
	case LWM2M_CARRIER_EVENT_READY:
		printk("LWM2M_CARRIER_EVENT_READY\n");
		k_sem_give(&carrier_registered);
		break;
	case LWM2M_CARRIER_EVENT_FOTA_START:
		printk("LWM2M_CARRIER_EVENT_FOTA_START\n");
		break;
	case LWM2M_CARRIER_EVENT_REBOOT:
		printk("LWM2M_CARRIER_EVENT_REBOOT\n");
		break;
	}
}
#endif /* defined(CONFIG_LWM2M_CARRIER) */


/**@brief Configures modem to provide LTE link. Blocks until link is
 * successfully established.
 */
static void modem_configure(void)
{
#if defined(CONFIG_LTE_LINK_CONTROL)
	if (IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT)) {
		/* Do nothing, modem is already turned on
		 * and connected.
		 */
	} else {
#if defined(CONFIG_LWM2M_CARRIER)
		/* Wait for the LWM2M_CARRIER to configure the modem and
		 * start the connection.
		 */
		printk("Waitng for carrier registration...\n");
		k_sem_take(&carrier_registered, K_FOREVER);
		printk("Registered!\n");
#else /* defined(CONFIG_LWM2M_CARRIER) */
		int err;

		printk("LTE Link Connecting ...\n");
		err = lte_lc_init_and_connect();
		__ASSERT(err == 0, "LTE link could not be established.");
		printk("LTE Link Connected!\n");
#endif /* defined(CONFIG_LWM2M_CARRIER) */
	}
#endif /* defined(CONFIG_LTE_LINK_CONTROL) */
}

#define TIMER_RELOAD_VALUE_MS 10000
s64_t unix_time_ms = 0;

void get_current_time_callback(struct k_timer *timer_id)
{
	int err = 0;

	// If the time stamp has not been set, try to acquire it from the date_time library
	if(unix_time_ms == 0){
		printk("Attempting to acquire time and date from date_time library...\n");
		err = date_time_now(&unix_time_ms);
	}
	//   otherwise, simply increment the timestamp based on the reload value
	else {
		unix_time_ms += TIMER_RELOAD_VALUE_MS;
	}

	// Check if we have successfully acquired a timestamp, and print it out if we have
	if(err == 0){
		printk("Time update:\n");
		printk("  Unix time ms: %i%06i\n", (u32_t)(unix_time_ms/1000000), (u32_t)(unix_time_ms%1000000));
	}
	else {
		printk("Error getting date_time (%i)!\n", err);
	}
}

K_TIMER_DEFINE(get_current_time_timer, get_current_time_callback, NULL);

void main(void)
{
	int err;

	printk("MQTT simple sample with date_time started\n");

	modem_configure();

	app_mqtt_init();

	date_time_update();

	// Schedule a callback for reading out date_time. 
	// Delay the first callback by 10 seconds to give the date_time library time to access the remote NTP server. 
	k_timer_start(&get_current_time_timer, K_SECONDS(10), K_MSEC(TIMER_RELOAD_VALUE_MS));

	do {
		err = app_mqtt_run();
	}while(err == 0);

	app_mqtt_disconnect();
}
