/*
 * This file is part of the usb-relay project.
 *
 * Copyright (C) 2014 Daniel Thompson <daniel@redfelineninja.org.uk>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/desig.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <librfn/console.h>
#include <librfn/fibre.h>
#include <librfn/regdump.h>
#include <librfn/time.h>
#include <librfn/util.h>

static console_t cdcacm_console;

static void jump_to_bootloader(void)
{
	char * const marker = (char *)0x20004800; /* RAM@18K */
	const char key[] = "remain-in-loader";

	memcpy(marker, key, sizeof(key));
	scb_reset_system(); /* Will never return. */
}

static pt_state_t do_id(console_t *c)
{
	char serial_no[25];

	desig_get_unique_id_as_string(serial_no, sizeof(serial_no));
	fprintf(c->out, "%s\n", serial_no);

	return PT_EXITED;
}

static pt_state_t do_reboot(console_t *c)
{
	(void) c;
	jump_to_bootloader();
	return PT_EXITED;
}

static pt_state_t do_uptime(console_t *c)
{
	unsigned int hours, minutes, seconds, microseconds;

	uint64_t t = time64_now();

	/* get to 32-bit values as directly as possible */
	minutes = t / (60 * 1000000);
	microseconds = t % (60 * 1000000);

	hours = minutes / 60;
	minutes %= 60;
	seconds = microseconds / 1000000;
	microseconds %= 1000000;

	fprintf(c->out, "%02u:%02u:%02u.%03u\n", hours, minutes, seconds,
		microseconds / 1000);

	return PT_EXITED;
}

static pt_state_t do_version(console_t *c)
{
	fprintf(c->out, "usb-relay version %s (%s %s)\n", VERSION, __DATE__,
		__TIME__);
	return PT_EXITED;
}

static const console_cmd_t cmds[] = {
	CONSOLE_CMD_VAR_INIT("id", do_id),
	CONSOLE_CMD_VAR_INIT("reboot", do_reboot),
	CONSOLE_CMD_VAR_INIT("uptime", do_uptime),
	CONSOLE_CMD_VAR_INIT("version", do_version)
};

#define ON_WITH_OPEN_DRAIN (console_gpio_default_on | console_gpio_open_drain)
#define ON_AND_ACTIVE_LOW  (console_gpio_default_on | console_gpio_active_low)

#ifdef CONFIG_PLATFORM_MINI_STLINK
const console_gpio_t gpios[] = {
	/*
	 * SWIM and SWCLK are connected to multiple pins (hence the
	 * commented out alternatives below). There are few useful
	 * peripherals whose alternative functions are attached to
	 * these pins, although shorting PB9 with the neighbouring
	 * PB8 pin does make I2C1 available on PB6 (RST) and PB9
	 * (SWIM).
	 */
	CONSOLE_GPIO_VAR_INIT("rst", GPIOB, GPIO6, 0), // pulled down
	CONSOLE_GPIO_VAR_INIT("swdio", GPIOB, GPIO14, 0), // pulled up
	CONSOLE_GPIO_VAR_INIT("swim", GPIOB, GPIO8, 0), // pulled up
	//CONSOLE_GPIO_VAR_INIT("swim", GPIOB, GPIO11, 0),
	CONSOLE_GPIO_VAR_INIT("swclk", GPIOB, GPIO13, 0), // pulled up
	//CONSOLE_GPIO_VAR_INIT("swclk", GPIOA, GPIO5, 0),
	CONSOLE_GPIO_VAR_INIT("led", GPIOA, GPIO9, 0),
};

#else
const console_gpio_t gpios[] = {
	CONSOLE_GPIO_VAR_INIT("relay1", GPIOB, GPIO12, ON_WITH_OPEN_DRAIN),
	CONSOLE_GPIO_VAR_INIT("relay2", GPIOB, GPIO13, ON_WITH_OPEN_DRAIN),
	CONSOLE_GPIO_VAR_INIT("relay3", GPIOB, GPIO14, ON_WITH_OPEN_DRAIN),
	CONSOLE_GPIO_VAR_INIT("relay4", GPIOB, GPIO15, ON_WITH_OPEN_DRAIN),
	CONSOLE_GPIO_VAR_INIT("relay5", GPIOA, GPIO8, ON_WITH_OPEN_DRAIN),
	CONSOLE_GPIO_VAR_INIT("relay6", GPIOA, GPIO9, ON_WITH_OPEN_DRAIN),
	CONSOLE_GPIO_VAR_INIT("relay7", GPIOA, GPIO10, ON_WITH_OPEN_DRAIN),
	CONSOLE_GPIO_VAR_INIT("relay8", GPIOB, GPIO6, ON_WITH_OPEN_DRAIN),
	CONSOLE_GPIO_VAR_INIT("led", GPIOC, GPIO13, console_gpio_active_low |
						    console_gpio_default_on),
};
#endif

int main(void)
{
	rcc_clock_setup_in_hse_8mhz_out_72mhz();

	time_init();

	console_init(&cdcacm_console, stdout);
	for (unsigned int i=0; i<lengthof(cmds); i++)
		console_register(&cmds[i]);
	for (unsigned int i=0; i<lengthof(gpios); i++)
		console_gpio_register(&gpios[i]);

	fibre_scheduler_main_loop();
}

