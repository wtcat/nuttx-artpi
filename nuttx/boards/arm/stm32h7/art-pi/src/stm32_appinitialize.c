/****************************************************************************
 * boards/arm/stm32h7/nucleo-h743zi/src/stm32_appinitialize.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <arch/board/board.h>
#include <nuttx/board.h>
#include <nuttx/clock.h>
#include <nuttx/compiler.h>
#include <nuttx/config.h>
#include <nuttx/wqueue.h>
#include <syslog.h>

#include "stm32_artpi.h"
#include "stm32_gpio.h"

struct led_work_s {
	struct work_s work;
	clock_t delay[2];
	bool enable;
	bool current_state;
	bool active_state;
};

static struct led_work_s led_work;

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef OK
#define OK 0
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/
#ifndef CONFIG_ARCH_LEDS
static void system_run_work(void *arg) {
	struct led_work_s *work = arg;

	work->current_state = !work->current_state;
	stm32_gpiowrite(BOARD_LED_GREEN, work->current_state);

	if (work->enable) {
		work_queue(LPWORK, &work->work, system_run_work, work,
				   work->delay[work->current_state]);
	} else {
		stm32_gpiowrite(BOARD_LED_GREEN, !work->active_state);
	}
}

static int system_run_post(void) {
	stm32_gpiowrite(BOARD_LED_RED, 1);
	led_work.enable = true;
	led_work.current_state = true;
	led_work.active_state = false;
	led_work.delay[0] = MSEC2TICK(50);
	led_work.delay[1] = MSEC2TICK(1000);
	return work_queue(LPWORK, &led_work.work, system_run_work, &led_work, 0);
}
#endif

/****************************************************************************
 * Name: board_app_initialize
 *
 * Description:
 *   Perform application specific initialization.  This function is never
 *   called directly from application code, but only indirectly via the
 *   (non-standard) boardctl() interface using the command BOARDIOC_INIT.
 *
 * Input Parameters:
 *   arg - The boardctl() argument is passed to the board_app_initialize()
 *         implementation without modification.  The argument has no
 *         meaning to NuttX; the meaning of the argument is a contract
 *         between the board-specific initialization logic and the
 *         matching application logic.  The value could be such things as a
 *         mode enumeration value, a set of DIP switch switch settings, a
 *         pointer to configuration data read from a file or serial FLASH,
 *         or whatever you would like to do with it.  Every implementation
 *         should accept zero/NULL as a default configuration.
 *
 * Returned Value:
 *   Zero (OK) is returned on success; a negated errno value is returned on
 *   any failure to indicate the nature of the failure.
 *
 ****************************************************************************/

void nuttx_main(void);

int board_app_initialize(uintptr_t arg) {
#ifdef CONFIG_BOARD_LATE_INITIALIZE
	/* Board initialization already performed by board_late_initialize() */

	return OK;
#else
	/* Perform board-specific initialization */

	return stm32_bringup();
#endif
}

int board_app_finalinitialize(uintptr_t arg) {
	int err = 0;
#ifdef CONFIG_IEEE80211_BROADCOM_BCM43438
	err = stm32_sdiowifi_initialize();
	if (err < 0) {
		syslog(LOG_ERR, "ERROR: Failed to initialize ap6212: %d\n", err);
	}
#endif

#ifndef CONFIG_ARCH_LEDS
	err = system_run_post();
	if (err < 0) {
		syslog(LOG_ERR, "ERROR: Failed to start led service: %d\n", err);
	}
#endif

	nuttx_main();

	return err;
}
