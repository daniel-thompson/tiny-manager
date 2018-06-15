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
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <librfn/console.h>
#include <librfn/fibre.h>
#include <librfn/time.h>
#include <librfn/util.h>

static console_t cdcacm_console;

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
static const console_cmd_t cmd_uptime =
    CONSOLE_CMD_VAR_INIT("uptime", do_uptime);

const console_gpio_t gpio_led = CONSOLE_GPIO_VAR_INIT("led", GPIOD, GPIO12, 0);

const console_gpio_t gpio_relays[] = {
	CONSOLE_GPIO_VAR_INIT("relay1", GPIOD, GPIO8, console_gpio_open_drain),
	CONSOLE_GPIO_VAR_INIT("relay2", GPIOD, GPIO9, console_gpio_open_drain),
	CONSOLE_GPIO_VAR_INIT("relay3", GPIOD, GPIO10, console_gpio_open_drain),
	CONSOLE_GPIO_VAR_INIT("relay4", GPIOD, GPIO11, console_gpio_open_drain),
};

int main(void)
{
	rcc_clock_setup_hse_3v3(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_120MHZ]);

	time_init();
	console_init(&cdcacm_console, stdout);
	console_register(&cmd_uptime);

	console_gpio_register(&gpio_led);
	for (unsigned int i=0; i<lengthof(gpio_relays); i++)
		console_gpio_register(&gpio_relays[i]);

	fibre_scheduler_main_loop();
}

