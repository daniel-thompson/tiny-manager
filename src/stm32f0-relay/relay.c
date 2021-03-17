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

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <libopencm3/cm3/cortex.h>
#include <libopencm3/stm32/crs.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/syscfg.h>
#include <librfn/console.h>
#include <librfn/fibre.h>
#include <librfn/time.h>
#include <librfn/util.h>

static console_t cdcacm_console;

const console_gpio_t gpio_relays[] = {
    CONSOLE_GPIO_VAR_INIT("power", GPIOB, GPIO5,
			  console_gpio_open_drain | console_gpio_active_low),
    CONSOLE_GPIO_VAR_INIT("reset", GPIOB, GPIO4,
			  console_gpio_active_low),
    CONSOLE_GPIO_VAR_INIT("recovery", GPIOA, GPIO15,
			  console_gpio_open_drain | console_gpio_active_low),
};

/*
 * For I4 atomic fetch actions the compiler (according to the gcc diagnostics
 * expects 'unsigned int(volatile void *, unsigned int,  int)
 */

#define I1 unsigned char
#define I4 unsigned int

#define ATOMIC_FETCH(width, action, operator)                                  \
	I##width __atomic_fetch_##action##_##width(volatile void *mem,         \
						   I##width arg, int model)    \
	{                                                                      \
		volatile I##width *ptr = mem;                                  \
		I##width oldval;                                               \
		(void)model;                                                   \
                                                                               \
		CM_ATOMIC_BLOCK()                                              \
		{                                                              \
			oldval = *ptr;                                         \
			*ptr = oldval operator arg;                            \
		}                                                              \
                                                                               \
		return oldval;                                                 \
	}

#define ATOMIC_COMPARE_EXCHANGE(width)                                         \
	bool __atomic_compare_exchange_##width(                                \
	    volatile void *mem, void *expected, I##width desired, bool weak,   \
	    int success_memorder, int failure_memorder)                        \
	{                                                                      \
		volatile I##width *ptr = mem;                                  \
		I##width *exp = expected;                                      \
		(void)weak;                                                    \
		(void)success_memorder;                                        \
		(void)failure_memorder;                                        \
                                                                               \
		CM_ATOMIC_BLOCK()                                              \
		{                                                              \
			I##width actual = *ptr;                                \
			if (actual != *exp) {                                  \
				*exp = actual;                                 \
				return false;                                  \
			}                                                      \
			*ptr = desired;                                        \
		}                                                              \
                                                                               \
		return true;                                                   \
	}

ATOMIC_FETCH(4, or, |)
ATOMIC_FETCH(4, and, &)
ATOMIC_FETCH(1, add, +)
ATOMIC_FETCH(1, sub, -)

ATOMIC_COMPARE_EXCHANGE(1)

#undef stdout
FILE *stdout = (void *) 1;
int _write(int fd, const char *ptr, int len);

int fprintf(FILE *f, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	while (*fmt) {
		switch (*fmt) {
		case '%':
			fmt++;
			if (*fmt == 's') {
				fputs(va_arg(ap, char *), f);
				fmt++;
			} else {
				fputc(*fmt++, f);
			}
			break;
		default:
			fputc(*fmt++, f);
		}
	}

	va_end(ap);


	/*
	 * nobody checks the return value of printf()... so let's not
	 * waste time being standards compliant...
	 */
	return 0;
};

int fputc(int c, FILE *f)
{
	char ch = c;
	return _write((int) f, &ch, 1);
}

int fputs(const char *s, FILE *f)
{
	return _write((int) f, s, strlen(s));
}

int fflush(FILE *stream)
{
	(void) stream;
	return 0;
}

int main(void)
{
	rcc_clock_setup_in_hsi_out_48mhz();
	crs_autotrim_usb_enable();
	rcc_set_usbclk_source(RCC_HSI48);
	
	// Accomodate USB using (shared) pins PA11 and PA12
	rcc_periph_clock_enable(RCC_SYSCFG_COMP);
	SYSCFG_CFGR1 |= SYSCFG_CFGR1_PA11_PA12_RMP;

	time_init();
	console_init(&cdcacm_console, stdout);

	for (unsigned int i=0; i<lengthof(gpio_relays); i++)
		console_gpio_register(&gpio_relays[i]);

	fibre_scheduler_main_loop();
}

