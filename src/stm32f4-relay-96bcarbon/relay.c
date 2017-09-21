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

static console_t uart_console;

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

const console_gpio_t gpio_led = CONSOLE_GPIO_VAR_INIT("led", GPIOA, GPIO15, 0);

const console_gpio_t gpio_relays[] = {
	CONSOLE_GPIO_VAR_INIT("relay1", GPIOA, GPIO0, console_gpio_open_drain),
	CONSOLE_GPIO_VAR_INIT("relay2", GPIOB, GPIO12, console_gpio_open_drain),
	CONSOLE_GPIO_VAR_INIT("relay3", GPIOA, GPIO2, console_gpio_open_drain),
	CONSOLE_GPIO_VAR_INIT("relay4", GPIOB, GPIO15, console_gpio_open_drain),
	CONSOLE_GPIO_VAR_INIT("relay5", GPIOA, GPIO3, console_gpio_open_drain),
	CONSOLE_GPIO_VAR_INIT("relay6", GPIOB, GPIO14, console_gpio_open_drain),
	CONSOLE_GPIO_VAR_INIT("relay7", GPIOA, GPIO1, console_gpio_open_drain),
	CONSOLE_GPIO_VAR_INIT("relay8", GPIOB, GPIO13, console_gpio_open_drain),
};

int main(void)
{
	rcc_clock_setup_hse_3v3(&hse_16mhz_3v3[CLOCK_3V3_84MHZ]);

	time_init();
	console_init(&uart_console, stdout);
	console_register(&cmd_uptime);

	console_gpio_register(&gpio_led);
	for (unsigned int i=0; i<lengthof(gpio_relays); i++)
		console_gpio_register(&gpio_relays[i]);

	fibre_scheduler_main_loop();
}

