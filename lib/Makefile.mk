$(file >> $(LOGFILE),Library C files included for building:)
SRC_LIB_C += $(call rwildcard,lib,*.c)
$(foreach path,$(SRC_LIB_C),$(file >> $(LOGFILE),$(TAB)$(strip $(path))))
$(file >> $(LOGFILE),$(NWLN))

SRC += $(SRC_LIB_C)

COMPILER_FLAGS += -Ilib
COMPILER_FLAGS += -Ilib/collections
COMPILER_FLAGS += -Ilib/collections/linked_list
COMPILER_FLAGS += -Ilib/collections/shared_mutex
COMPILER_FLAGS += -Ilib/fatfs
COMPILER_FLAGS += -Ilib/mathlib
COMPILER_FLAGS += -Ilib/rtos
COMPILER_FLAGS += -Ilib/rtos/FreeRTOS-KernelV11.2.0/include
COMPILER_FLAGS += -Ilib/rtos/FreeRTOS-KernelV11.2.0/portable/GCC/ARM_CM7/r0p1
COMPILER_FLAGS += -Ilib/stringlib
COMPILER_FLAGS += -Ilib/time_date
