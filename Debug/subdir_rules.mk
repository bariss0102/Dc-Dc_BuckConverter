################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
%.obj: ../%.c $(GEN_OPTS) | $(GEN_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"C:/ti/ccs910/ccs/tools/compiler/ti-cgt-c2000_18.12.3.LTS/bin/cl2000" -v28 -ml -mt --float_support=fpu32 --include_path="C:/Users/Fatma/workspace_v9/DC-DC" --include_path="C:/Users/Fatma/workspace_v9/DC-DC/Headers_App" --include_path="C:/Users/Fatma/workspace_v9/DC-DC/Headers_Dev/DSP2833x_common/include" --include_path="C:/Users/Fatma/workspace_v9/DC-DC/Headers_Dev/DSP2833x_headers/include" --include_path="C:/Users/Fatma/workspace_v9/DC-DC/Headers_Dev/IQmath/v160/include" --include_path="C:/ti/c2000/C2000Ware_2_00_00_02/device_support/f2833x/common/include" --include_path="C:/ti/c2000/C2000Ware_2_00_00_02/device_support/f2833x/headers/include" --include_path="C:/ti/ccs910/ccs/tools/compiler/ti-cgt-c2000_18.12.3.LTS/include" -g --diag_warning=225 --diag_wrap=off --display_error_number --abi=coffabi --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

%.obj: ../%.asm $(GEN_OPTS) | $(GEN_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"C:/ti/ccs910/ccs/tools/compiler/ti-cgt-c2000_18.12.3.LTS/bin/cl2000" -v28 -ml -mt --float_support=fpu32 --include_path="C:/Users/Fatma/workspace_v9/DC-DC" --include_path="C:/Users/Fatma/workspace_v9/DC-DC/Headers_App" --include_path="C:/Users/Fatma/workspace_v9/DC-DC/Headers_Dev/DSP2833x_common/include" --include_path="C:/Users/Fatma/workspace_v9/DC-DC/Headers_Dev/DSP2833x_headers/include" --include_path="C:/Users/Fatma/workspace_v9/DC-DC/Headers_Dev/IQmath/v160/include" --include_path="C:/ti/c2000/C2000Ware_2_00_00_02/device_support/f2833x/common/include" --include_path="C:/ti/c2000/C2000Ware_2_00_00_02/device_support/f2833x/headers/include" --include_path="C:/ti/ccs910/ccs/tools/compiler/ti-cgt-c2000_18.12.3.LTS/include" -g --diag_warning=225 --diag_wrap=off --display_error_number --abi=coffabi --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

DSP2833x_CpuTimers.obj: C:/ti/c2000/C2000Ware_2_00_00_02/device_support/f2833x/common/source/DSP2833x_CpuTimers.c $(GEN_OPTS) | $(GEN_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"C:/ti/ccs910/ccs/tools/compiler/ti-cgt-c2000_18.12.3.LTS/bin/cl2000" -v28 -ml -mt --float_support=fpu32 --include_path="C:/Users/Fatma/workspace_v9/DC-DC" --include_path="C:/Users/Fatma/workspace_v9/DC-DC/Headers_App" --include_path="C:/Users/Fatma/workspace_v9/DC-DC/Headers_Dev/DSP2833x_common/include" --include_path="C:/Users/Fatma/workspace_v9/DC-DC/Headers_Dev/DSP2833x_headers/include" --include_path="C:/Users/Fatma/workspace_v9/DC-DC/Headers_Dev/IQmath/v160/include" --include_path="C:/ti/c2000/C2000Ware_2_00_00_02/device_support/f2833x/common/include" --include_path="C:/ti/c2000/C2000Ware_2_00_00_02/device_support/f2833x/headers/include" --include_path="C:/ti/ccs910/ccs/tools/compiler/ti-cgt-c2000_18.12.3.LTS/include" -g --diag_warning=225 --diag_wrap=off --display_error_number --abi=coffabi --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


