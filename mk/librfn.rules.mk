#
# This file of the part of the usb-relay project.
#
# Copyright (C) 2014 Dainiel Thompson <daniel@redfelineninja.org.uk>
#
# This library is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this library.  If not, see <http://www.gnu.org/licenses/>.
#

-include $(OBJS:.o=.d)

LIBRFN_DIR = ../../librfn

OBJS += \
	fibre.o \
	fibre_default.o \
	list.o \
	messageq.o \
	ringbuf.o \
	time_libopencm3.o \
	util.o

vpath %.c $(LIBRFN_DIR)/librfn
vpath %.c $(LIBRFN_DIR)/librfn/libopencm3

CPPFLAGS += -DNDEBUG
CPPFLAGS += -DCONFIG_CONSOLE_FROM_ISR=1
CPPFLAGS += -I$(LIBRFN_DIR)/include
