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

typedef struct {
	console_cmd_t cmd;
	uintptr_t port;
	uint32_t pin;
} console_gpio_t;

static pt_state_t do_gpio(console_t *c)
{
	console_gpio_t *gpio = containerof(c->cmd, console_gpio_t, cmd);

	if (0 == strcmp(c->argv[1], "on"))
		gpio_set(gpio->port, gpio->pin);
	else if (0 == strcmp(c->argv[1], "off"))
		gpio_clear(gpio->port, gpio->pin);
	else if (0 == strcmp(c->argv[1], "toggle"))
		gpio_toggle(gpio->port, gpio->pin);
	else
		fprintf(c->out, "Usage: %s on|off|toggle\n", c->cmd->name);

	return PT_EXITED;	
}

const console_gpio_t gpio_led = {
	CONSOLE_CMD_VAR_INIT("led", do_gpio), GPIOD, GPIO12
};

static void gpio_io_toggle(uintptr_t port, uint32_t pin)
{
	for (int i=0; i<32; i++) {
		if (pin & (1 << i)) {
			if (GPIO_MODER(port) & GPIO_MODE_MASK(i))
				gpio_mode_setup(port, GPIO_MODE_INPUT,
						GPIO_PUPD_NONE, 1 << i);
			else
				gpio_mode_setup(port, GPIO_MODE_OUTPUT,
						GPIO_PUPD_NONE, 1 << i);
		}
	}
}

/* Can't get GPIO_OTYPER_OD to work properly so for now we implement
 * hi-z by switching to input mode.
 */
static pt_state_t do_relay(console_t *c)
{
	console_gpio_t *gpio = containerof(c->cmd, console_gpio_t, cmd);
	uint32_t *t = &c->scratch.u32[0]; /* "rename" a scratch register */

	PT_BEGIN(&c->pt);

	/* signal is active when pulled down to 0v (but 3.3v is not
	 * sufficient for relay board to treat it as logic high)
	 */

	gpio_clear(gpio->port, gpio->pin);
	if (0 == strcmp(c->argv[1], "on"))
		gpio_mode_setup(gpio->port, GPIO_MODE_INPUT,
				GPIO_PUPD_NONE, gpio->pin);
	else if (0 == strcmp(c->argv[1], "off"))
		gpio_mode_setup(gpio->port, GPIO_MODE_OUTPUT,
				GPIO_PUPD_NONE, gpio->pin);
	else if (0 == strcmp(c->argv[1], "toggle"))
		gpio_io_toggle(gpio->port, gpio->pin);
	else if (0 == strcmp(c->argv[1], "pulse")) {
		gpio_io_toggle(gpio->port, gpio->pin);
		*t = time_now() + 1000000;
		PT_WAIT_UNTIL(fibre_timeout(*t));
		gpio_io_toggle(gpio->port, gpio->pin);

	} else
		fprintf(c->out, "Usage: %s on|off|toggle|pulse\n",
			c->cmd->name);

	PT_END();
}

const console_gpio_t gpio_relays[] = {
	{ CONSOLE_CMD_VAR_INIT("relay1", do_relay), GPIOD, GPIO8 },
	{ CONSOLE_CMD_VAR_INIT("relay2", do_relay), GPIOD, GPIO9 },
	{ CONSOLE_CMD_VAR_INIT("relay3", do_relay), GPIOD, GPIO10 },
	{ CONSOLE_CMD_VAR_INIT("relay4", do_relay), GPIOD, GPIO11 }
};

int main(void)
{
	rcc_clock_setup_hse_3v3(&hse_8mhz_3v3[CLOCK_3V3_120MHZ]);

	time_init();
	console_init(&cdcacm_console, stdout);
	console_register(&cmd_uptime);

	rcc_periph_clock_enable(RCC_GPIOD);

#ifdef STM32F1
	gpio_set_mode(gpio_led.port, GPIO_MODE_OUTPUT_2_MHZ,
		      GPIO_CNF_OUTPUT_PUSHPULL, gpio_led.pin);
#else
	gpio_mode_setup(gpio_led.port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
			gpio_led.pin);
#endif
	console_register(&gpio_led.cmd);

	for (unsigned int i=0; i<lengthof(gpio_relays); i++) {
#ifdef STM32F1
		gpio_set_mode(gpio_relays[i].port, GPIO_MODE_OUTPUT_2_MHZ,
			      GPIO_CNF_OUTPUT_PUSHPULL, gpio_relays[i].pin);
#else
		gpio_mode_setup(gpio_relays[i].port, GPIO_MODE_INPUT,
				GPIO_PUPD_NONE, gpio_relays[i].pin);
#endif
		console_register(&gpio_relays[i].cmd);
	}

	fibre_scheduler_main_loop();
}

