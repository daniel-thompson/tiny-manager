##
## This file is part of the usb-relay project
##
## Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
##
## This library is free software: you can redistribute it and/or modify
## it under the terms of the GNU Lesser General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This library is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU Lesser General Public License for more details.
##
## You should have received a copy of the GNU Lesser General Public License
## along with this library.  If not, see <http://www.gnu.org/licenses/>.
##

BINARY = relay

DEFS += -DF_CPU=84000000 -DCONFIG_USB_MAX_POWER=500

OBJS += console.o console_stm32.o console_cmd_gpio.o

OOCD_BOARD = board/stm32f496bcarbon
DFU_ADDRESS = 0x08000000

LDSCRIPT = ../stm32f4-96bcarbon.ld
include ../../mk/libopencm3.stm32f4xx.mk
