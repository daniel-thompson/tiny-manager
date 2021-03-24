tiny-manager - Microcontroller firmware for baseboard management
=================================================================

tiny-manager will, one day, become a general purpose firmware for
baseboard management.

Currently however is a single purpose firmware that can toggle the power
and reset lines for the STM32F042 found on the Solidrun Honeycomb LX2.
Nevertheless this is sufficient to be a proof-of-concept showing how to
squeeze a simple USB ACM command processor into the tiny, tiny flash of
the STM32F042. Now we have working USB then how hard can the rest of it
really be ;-) !

Quickstart
----------

- Install the [Arm Embedded
  Toolchain](https://developer.arm.com/open-source/gnu-toolchain/gnu-rm)
  (or similar arm-none-eabi- compiler) and dfu-util.
- Clone this repo:
  `git clone https://github.com/daniel-thompson/tiny-manager.git`
- Fetch the libraries:
  `cd tiny-manager; git submodule update --init --recursive`
- Build the firmware:
  `make`
- Use the jumper link to force the Micro-BMC on the Honeycomb to DFU
  mode and power up the board:
  `make -C src/stm32f0-relay flash`
- Unset the jumper and cycle the power.

Usage
-----

The firmware registers itself as a CDC-ACM device and, under Linux, will
be presented as ttyACMx where x is typically 0 on "simple" setups
although will be larger if there is any USB communication or modem device
installed (for example an unused 3G modem).

Type `help` to get a list of available commands.

- `echo` - test command that echos back its input
- `help` - shows a list of commands
- `power` - manipulate the power line (hold it on for 7 seconds to
  for a power off)
- `reset` -  manipulate the reset line, note that the BMC reset line
  does not work correctly on early Honeycomb boards (due to an
  incorrectly designed combination of bidirectional level shifters
  and current limiting resistors) meaning the reset command may not
  work on all boards.

udev rules
----------

Note: `This section is aspirational... but hopefully it will be implemented
soon.`

The firmware uses the STM32 unique device ID to provide every physical
instance with a unique serial number. This allows udev rules to be
introduced to ensure stable device enumeration regardless of any changes
to the USB topology.

    SUBSYSTEM=="tty", ATTRS{manufacturer}=="redfelineninja.org.uk", ATTRS{serial}=="045101780587252555FFC660", SYMLINK+="ttyrelayC660"
    SUBSYSTEM=="tty", ATTRS{manufacturer}=="redfelineninja.org.uk", ATTRS{serial}=="7301C2152B72E52DE2744F3B", SYMLINK+="ttyrelay4F3B"

The serial number can picked up from the kernel log (`dmesg`) or using udevadm:
`udevadm info --attribute-walk /dev/ttyACM0 | grep serial` .

udevadm can also be used to re-apply the rule if any changes are made to the symlink: `sudo udevadm trigger /dev/ttyACM0`
