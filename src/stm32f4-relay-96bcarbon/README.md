usb-relay for Carbon
====================

[Carbon by Seeed and 96Boards](https://www.96boards.org/product/carbon/)
is an STM32F401 based BLE development board. It's perhaps a little
over-specified to use just to control relays, but if you have an unused
one lying around its a great alternative to STM32F103 boards.

Quickstart
----------

- Install the [Arm Embedded
  Toolchain](https://developer.arm.com/open-source/gnu-toolchain/gnu-rm)
  (or similar arm-none-eabi- compiler), openocd and dfu-util.
- Clone this repo:
  `git clone https://github.com/daniel-thompson/usb-relay.git`
- Fetch the libraries:
  `cd usb-relay; git submodule update --init --recursive`
- Build the firmware:
  `make`
- Connect via the OTG USB socket for programming (you may need to be
  root for this):
  `make -C src/stm32f4-relay-96bcarbon flash`

Hardware setup
--------------

Connections requires are:

- Connect via the UART USB socket and set the baud rate to 38400.
- USB5V (pin 11) and GND (pin 9) to relay board
- The relays should be connected using the pins allocated to LS-UART0 and
  LS-SPI0. This results in a 1:1 mapping between pin numbers and relay
  numbers; relay1 is pin 1 of the LS connector, relay 8 is pin 8 of the
  LS connector.

Note: The Carbon port uses the onboard FDTI USB -> serial bridge rather
      than emulating a CDC-ACM device. On Linux it will be presented
      as /dev/ttyUSBx.

Example session
---------------

~~~
> help
Available commands:
  echo
  help
  led
  relay1
  relay2
  relay3
  relay4
  relay5
  relay6
  relay7
  relay8
  uptime
> led on
> led off
> relay1 help
Usage: relay1 on|off|toggle|pulse
> relay1 toggle
~~~
