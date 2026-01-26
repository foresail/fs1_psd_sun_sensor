TARGET = PSD_SUNSENSOR

BUILD_DIR = build

#MODIFY TO YOUR OWN MSP-GCC LOCATION!
PREFIX = /home/skog/ti/msp430-gcc/bin/msp430-elf-

CC = $(PREFIX)gcc
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
HEX = $(CP) -O ihex
GDB = $(PREFIX)gdb

CPU = -mmcu=msp430fr2311
FPU = 
FLOAT-ABI = 
MCU = $(CPU) $(FPU) $(FLOAT-ABI)

C_DEFS = \
-DMSP430FR2311 \

# List the directories containing header files
C_INCLUDES +=  \
-I. \
-Iplatform \
-Ibus \

SOURCE_DIR = .

#Recursive file search. Pure make. Should work on all platforms
rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

# '/' mandatory. Finds all *.c files in given directory
C_SOURCES = $(call rwildcard,$(SOURCE_DIR)/,*.c)

CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -Wno-format -Os
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"
CFLAGS += -mhwmult=none -mtiny-printf -msilicon-errata-warn=cpu11,cpu12,cpu13,cpu19 -mwarn-mcu

LIBS =
LDSCRIPT = -Tmsp430fr2311.ld
LDFLAGS = \
$(MCU) $(LDSCRIPT) $(LIBDIR) $(LIBS) \
-Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections \


all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex

flash: $(BUILD_DIR)/$(TARGET).hex
	@MSP430Flasher -n MSP430FR2311 -w $< -v -g -z [VCC]

unbrick: 
	@MSP430Flasher -e ERASE_USER_CODE

#Starts mspdebug server gdb port and connects to it
debug: $(BUILD_DIR)/$(TARGET).elf flash
	@mspdebug tilib gdb &
	@$(TERM) -e $(GDB) --eval-command="target remote localhost:2000" $<

OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
# list of ASM program objects
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR) 
	@$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@
	$(info [$(TARGET)] Compiling $@)

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	@$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS)
	@$(CC) $(OBJECTS) $(LDFLAGS) -Wl,--print-memory-usage -o $@
	
#MSP430Flasher only accepts intel hex or TI-txt files
$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf
	@$(CP) -O ihex $< $@
	$(SZ) $@

$(BUILD_DIR):
	mkdir $@		

clean:
	-rm -fR $(BUILD_DIR)

-include $(wildcard $(BUILD_DIR)/*.d)
