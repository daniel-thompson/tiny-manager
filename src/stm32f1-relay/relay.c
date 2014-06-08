/*
 * This file is part of the i2c-star-usb project.
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
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <librfn/console.h>
#include <librfn/fibre.h>
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

static pt_state_t do_reboot(console_t *c)
{
	(void) c;
	jump_to_bootloader();
	return PT_EXITED;
}
static const console_cmd_t cmd_reboot =
    CONSOLE_CMD_VAR_INIT("reboot", do_reboot);

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

typedef struct {
	console_cmd_t cmd;
	uintptr_t port;
	uint32_t pin;
} console_gpio_t;

static pt_state_t do_gpio(console_t *c)
{
	console_gpio_t *gpio = containerof(c->cmd, console_gpio_t, cmd);
	uint32_t *t = &c->scratch.u32[0];

	PT_BEGIN(&c->pt);

	/* the relays are active low (i.e. gpio_clear means on) */

	if (0 == strcmp(c->argv[1], "on"))
		gpio_clear(gpio->port, gpio->pin);
	else if (0 == strcmp(c->argv[1], "off"))
		gpio_set(gpio->port, gpio->pin);
	else if (0 == strcmp(c->argv[1], "toggle"))
		gpio_toggle(gpio->port, gpio->pin);
	else if (0 == strcmp(c->argv[1], "pulse")) {
		gpio_toggle(gpio->port, gpio->pin);
		*t = time_now() + 1000000;
		PT_WAIT_UNTIL(fibre_timeout(*t));
		gpio_toggle(gpio->port, gpio->pin);
	} else
		fprintf(c->out, "Usage: %s on|off|toggle|pulse\n", c->cmd->name);

	PT_END();
}

const console_gpio_t gpio_relays[] = {
	{ CONSOLE_CMD_VAR_INIT("relay1", do_gpio), GPIOA, GPIO8 },
	{ CONSOLE_CMD_VAR_INIT("relay2", do_gpio), GPIOA, GPIO10 },
	{ CONSOLE_CMD_VAR_INIT("relay3", do_gpio), GPIOA, GPIO9 },
	{ CONSOLE_CMD_VAR_INIT("relay4", do_gpio), GPIOB, GPIO7 },
};

int main(void)
{
	rcc_clock_setup_in_hse_8mhz_out_72mhz();

	time_init();
	console_init(&cdcacm_console, stdout);
	console_register(&cmd_reboot);
	console_register(&cmd_uptime);

	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);

	for (unsigned int i=0; i<lengthof(gpio_relays); i++) {
		gpio_set(gpio_relays[i].port, gpio_relays[i].pin);
		gpio_set_mode(gpio_relays[i].port, GPIO_MODE_OUTPUT_2_MHZ,
			      GPIO_CNF_OUTPUT_OPENDRAIN, gpio_relays[i].pin);
		console_register(&gpio_relays[i].cmd);
	}

	fibre_scheduler_main_loop();
}

