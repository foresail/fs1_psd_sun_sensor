################################################################################
# Automatically-generated file. Do not edit!
################################################################################

CG_TOOL_ROOT := /opt/ti/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS

GEN_OPTS__FLAG := 
GEN_CMDS__FLAG := 

ORDERED_OBJS += \
"./adc.obj" \
"./calc.obj" \
"./main.obj" \
"./telecommands.obj" \
"./bus/bus.obj" \
"./bus/bus_frame.obj" \
"./platform/timestamp.obj" \
"../lnk_msp430fr2311.cmd" \
$(GEN_CMDS__FLAG) \
-llibc.a \

-include ../makefile.init

RM := rm -rf
RMDIR := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir_vars.mk
-include bus/subdir_vars.mk
-include platform/subdir_vars.mk
-include subdir_rules.mk
-include bus/subdir_rules.mk
-include platform/subdir_rules.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C55_DEPS)),)
-include $(C55_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(S67_DEPS)),)
-include $(S67_DEPS)
endif
ifneq ($(strip $(S62_DEPS)),)
-include $(S62_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(OPT_DEPS)),)
-include $(OPT_DEPS)
endif
ifneq ($(strip $(C??_DEPS)),)
-include $(C??_DEPS)
endif
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(S??_DEPS)),)
-include $(S??_DEPS)
endif
ifneq ($(strip $(C64_DEPS)),)
-include $(C64_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(S64_DEPS)),)
-include $(S64_DEPS)
endif
ifneq ($(strip $(INO_DEPS)),)
-include $(INO_DEPS)
endif
ifneq ($(strip $(CLA_DEPS)),)
-include $(CLA_DEPS)
endif
ifneq ($(strip $(S55_DEPS)),)
-include $(S55_DEPS)
endif
ifneq ($(strip $(SV7A_DEPS)),)
-include $(SV7A_DEPS)
endif
ifneq ($(strip $(C62_DEPS)),)
-include $(C62_DEPS)
endif
ifneq ($(strip $(C67_DEPS)),)
-include $(C67_DEPS)
endif
ifneq ($(strip $(PDE_DEPS)),)
-include $(PDE_DEPS)
endif
ifneq ($(strip $(K_DEPS)),)
-include $(K_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C43_DEPS)),)
-include $(C43_DEPS)
endif
ifneq ($(strip $(S43_DEPS)),)
-include $(S43_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(SA_DEPS)),)
-include $(SA_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
EXE_OUTPUTS += \
PSDSunSensor.out 

EXE_OUTPUTS__QUOTED += \
"PSDSunSensor.out" 


# All Target
all: $(OBJS) $(CMD_SRCS) $(LD_SRCS) $(GEN_CMDS)
	@$(MAKE) --no-print-directory -Onone "PSDSunSensor.out"

# Tool invocations
PSDSunSensor.out: $(OBJS) $(CMD_SRCS) $(LD_SRCS) $(GEN_CMDS)
	@echo 'Building target: "$@"'
	@echo 'Invoking: MSP430 Linker'
	"/opt/ti/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/bin/cl430" -vmsp --code_model=small --data_model=small -O4 --opt_for_speed=3 --advice:power="all" --advice:hw_config="all" --define=__MSP430FR2311__ --define=BUS_EARLY_ADDRESS_SKIP --define=V4X -g --c11 --float_operations_allowed=32 --printf_support=minimal --diag_wrap=off --display_error_number --emit_warnings_as_errors --verbose_diagnostics --silicon_errata=CPU12 --silicon_errata=CPU13 --silicon_errata=CPU15 --silicon_errata=CPU18 --silicon_errata=CPU19 --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU23 --silicon_errata=CPU40 -z -m"PSDSunSensor.map" --heap_size=160 --stack_size=160 --cinit_hold_wdt=on -i"/opt/ti/ccs/ccs_base/msp430/include" -i"/opt/ti/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/lib" -i"/opt/ti/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/include" --reread_libs --diag_wrap=off --display_error_number --verbose_diagnostics --warn_sections --xml_link_info="PSDSunSensor_linkInfo.xml" --use_hw_mpy=none --rom_model --unused_section_elimination=on -o "PSDSunSensor.out" $(ORDERED_OBJS)
	@echo 'Finished building target: "$@"'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(EXE_OUTPUTS__QUOTED)
	-$(RM) "adc.obj" "calc.obj" "main.obj" "telecommands.obj" "bus/bus.obj" "bus/bus_frame.obj" "platform/timestamp.obj" 
	-$(RM) "adc.d" "calc.d" "main.d" "telecommands.d" "bus/bus.d" "bus/bus_frame.d" "platform/timestamp.d" 
	-@echo 'Finished clean'
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets

