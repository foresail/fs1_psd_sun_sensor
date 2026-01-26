################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
platform/%.obj: ../platform/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: MSP430 Compiler'
	"/opt/ti/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/bin/cl430" -vmsp --code_model=small --data_model=small -O4 --opt_for_speed=3 --include_path="/opt/ti/ccs/ccs_base/msp430/include" --include_path="/mnt/data/aalto/Foresail1p/fs1p_adcs/psd_sunsensor/sw" --include_path="/mnt/data/aalto/Foresail1p/fs1p_adcs/psd_sunsensor/sw/bus" --include_path="/mnt/data/aalto/Foresail1p/fs1p_adcs/psd_sunsensor/sw/platform" --include_path="/opt/ti/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/include" --advice:power="all" --advice:hw_config="all" --define=__MSP430FR2311__ --define=BUS_EARLY_ADDRESS_SKIP --define=V4X -g --c11 --float_operations_allowed=32 --printf_support=minimal --diag_wrap=off --display_error_number --emit_warnings_as_errors --verbose_diagnostics --silicon_errata=CPU12 --silicon_errata=CPU13 --silicon_errata=CPU15 --silicon_errata=CPU18 --silicon_errata=CPU19 --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU23 --silicon_errata=CPU40 --preproc_with_compile --preproc_dependency="platform/$(basename $(<F)).d_raw" --obj_directory="platform" $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: "$<"'
	@echo ' '


