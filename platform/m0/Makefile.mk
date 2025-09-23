# Memory description
FLASH_START_ADDR := 0x08000000
FLASH_BOOT_ADDR := 0x08000000
FLASH_BOOT_SIZE := 0x00020000 #128K
FLASH_INFO_ADDR := 0x08020000
FLASH_INFO_SIZE := 0x00010000 #64K
FLASH_APP_ADDR	:= 0x08030000

# Definition of platform options flags
PLATFORM_OPT += -mcpu=cortex-m7
PLATFORM_OPT += -mfpu=fpv5-d16
PLATFORM_OPT += -mfloat-abi=hard
PLATFORM_OPT += -mthumb

ifeq ($(FW_PLATFORM_CFG),0)
	PLATFORM_OPT += -DSTM32H743xx
	PLATFORM_OPT += -DUSE_FULL_LL_DRIVER
	PLATFORM_OPT += -DUSE_FULL_ASSERT
	PLATFORM_OPT += -DHSE_VALUE=25000000
	PLATFORM_OPT += -DLSE_VALUE=32768
	PLATFORM_OPT += -DHSI_VALUE=64000000
	PLATFORM_OPT += -DLSI_VALUE=32000
	# PLATFORM_OPT += -DVDD_VALUE=3300
endif

# Definition of linker flags
LINKER_FLAGS += -mcpu=cortex-m7
LINKER_FLAGS += -mfpu=fpv5-d16
LINKER_FLAGS += -mfloat-abi=hard
LINKER_FLAGS += -Wl,-cref
LINKER_FLAGS += -Wl,-u,Reset_Handler
LINKER_FLAGS += -Wl,--defsym=malloc_getpagesize_P=0x80
LINKER_FLAGS += -Wl,-Map=$(BIN_DIR)/$(TARGET).map

LD_SCRIPT := $(PLATFORM_DIR)/m0/stm32h743xihx.ld

# Files collection

# Collect MCU depended files
$(file >> $(LOGFILE),MCU depended ASM/C files included for building:)
SRC_MCU_ASM += $(PLATFORM_DIR)/m0/startup_stm32h743xihx.s
SRC_MCU_C += $(call rwildcard,$(PLATFORM_DIR)/m0/core,*.c)
SRC_MCU_C += $(call rwildcard,$(PLATFORM_DIR)/m0/usb_device,*.c)
SRC_MCU_C += $(PLATFORM_DIR)/m0/platform.c
$(foreach path,$(SRC_MCU_ASM) $(SRC_MCU_C),$(file >> $(LOGFILE),$(TAB)$(strip $(path))))
$(file >> $(LOGFILE),$(NWLN))

# Collect configuration depended files
$(file >> $(LOGFILE),Platform configuration depended C files included for building:)
SRC_CFG_C += $(call rwildcard,$(PLATFORM_DIR)/m0/c$(FW_PLATFORM_CFG),*.c)
$(foreach path,$(SRC_CFG_C),$(file >> $(LOGFILE),$(TAB)$(strip $(path))))
$(file >> $(LOGFILE),$(NWLN))

# Combine .c and .s files
SRC_ASM += $(SRC_MCU_ASM)
SRC += $(SRC_MCU_C) $(SRC_CFG_C)

# Definition of compiler flags
COMPILER_FLAGS += -I$(PLATFORM_DIR)
COMPILER_FLAGS += -I$(PLATFORM_DIR)/m0
COMPILER_FLAGS += -I$(PLATFORM_DIR)/m0/c$(FW_PLATFORM_CFG)
COMPILER_FLAGS += -I$(PLATFORM_DIR)/m0/core
COMPILER_FLAGS += -I$(PLATFORM_DIR)/m0/core/CMSIS
COMPILER_FLAGS += -I$(PLATFORM_DIR)/m0/core/HAL/Inc
COMPILER_FLAGS += -I$(PLATFORM_DIR)/m0/core/LL/Inc
COMPILER_FLAGS += -I$(PLATFORM_DIR)/m0/usb_device
COMPILER_FLAGS += -I$(PLATFORM_DIR)/m0/usb_device/Class/CDC/Inc
COMPILER_FLAGS += -I$(PLATFORM_DIR)/m0/usb_device/Class/MSC/Inc
COMPILER_FLAGS += -I$(PLATFORM_DIR)/m0/usb_device/Core/Inc
