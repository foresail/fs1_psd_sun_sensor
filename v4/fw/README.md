# Building with Make:

The ti msp build tools can be downloaded from [here](https://www.ti.com/tool/MSP430-GCC-OPENSOURCE#downloads).
==Remember to set the `PREFIX` variable in the Makefile to point to your ti install location!==

requirements: msp-flasher mspdebugger msp430-elf-gcc

Usage:
- `make` compiles the PSD\_SUNSENSOR elf and ihex files
- `make flash` flashes with MSP430Flasher
- `make unlock` Erases user code from the device and unlocks debugger
- `make debug` Flashes the device, starts mspdebug gdb server and launches a gdb client
