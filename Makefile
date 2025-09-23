include ./make_glob_cfg.mk

# Temporary change recipe prefix to perform tabbed conditionals
.RECIPEPREFIX := >

# # Include .env file if it exists, e.x. for choosing different shell
# ifneq ("$(wildcard .env)","")
# 	include .env
# endif

define NWLN
$(1)
endef

define TAB
	$(1)
endef

ifeq ($(COMPILER_OUTPUT), mute)
	OUTPUT_FLAG := @
else
	OUTPUT_FLAG := 
endif

# This function searches directories for files, e.x. $(call rwildcard,$(DIR_TO_SEARCH),$(FILENAME).$(EXT))
rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

# -----------------------------------------------------------------------------

ifeq ($(OS),Windows_NT)
  NULL_FILE_PATH := nul
else
  NULL_FILE_PATH := /dev/null
endif

ifeq ($(strip $(ARM_NONE_EABI_PATH)),)
CC       := arm-none-eabi-gcc
OBJCPY   := arm-none-eabi-objcopy
SIZECALC := arm-none-eabi-size
OBJDUMP  := arm-none-eabi-objdump
CPREPROC := arm-none-eabi-cpp
else
CC       := $(ARM_NONE_EABI_PATH)/arm-none-eabi-gcc
OBJCPY   := $(ARM_NONE_EABI_PATH)/arm-none-eabi-objcopy
SIZECALC := $(ARM_NONE_EABI_PATH)/arm-none-eabi-size
OBJDUMP  := $(ARM_NONE_EABI_PATH)/arm-none-eabi-objdump
CPREPROC := $(ARM_NONE_EABI_PATH)/arm-none-eabi-cpp
endif

OBJDUMP_ARGS := $(objdump_args)

# -----------------------------------------------------------------------------

$(shell mkdir -p $(OBJECT_DIR) 2> $(NULL_FILE_PATH) || echo off)

$(file > $(LOGFILE),Log file started $(NWLN)$(NWLN))
$(file >> $(LOGFILE),CC: $(CC))
$(file >> $(LOGFILE),OBJCPY: $(OBJCPY))
$(file >> $(LOGFILE),SIZECALC: $(SIZECALC))
$(file >> $(LOGFILE),OBJDUMP: $(OBJDUMP))
$(file >> $(LOGFILE),CPREPROC: $(CPREPROC))
$(file >> $(LOGFILE),OBJDUMP_ARGS: $(OBJDUMP_ARGS))
$(file >> $(LOGFILE),$(NWLN))

# if build targets not include `clean` or `objdump`
ifeq ($(filter clean objdump,$(MAKECMDGOALS)),)
	include ./make_user_flags.mk

# -----------------------------------------------------------------------------

	ifeq ($(FW_TYPE),$(FW_NAME_TYPE_APP))
# FW_VER_MAJOR := $(FW_VER_MAJOR_APP)
# FW_VER_MINOR := $(FW_VER_MINOR_APP)
# FW_VER_BUILD := $(FW_VER_BUILD_APP)
# LINKER_FLAGS += -DFW_TYPE_APP
	else ifeq ($(FW_TYPE),$(FW_NAME_TYPE_BOOT))
# FW_VER_MAJOR := $(FW_VER_MAJOR_BOOT)
# FW_VER_MINOR := $(FW_VER_MINOR_BOOT)
# FW_VER_BUILD := $(FW_VER_BUILD_BOOT)
# LINKER_FLAGS += -DFW_TYPE_BOOT
	else
		FW_TYPE := UNKNOWN
		$(error FW type '$(FW_TYPE)' is not supported)
	endif

# Extra fw_type folder check if exists
	ifeq ($(wildcard $(FW_TYPE)/*),)
		$(error '$(FW_TYPE)' dir is not found)
	endif

	ifeq ($(and $(FW_VER_MAJOR),$(FW_VER_MINOR),$(FW_VER_BUILD)),)
		$(error FW version set is missing)
	endif

	COMPILER_FLAGS += -DFW_VER_MAJOR=$(FW_VER_MAJOR)
	COMPILER_FLAGS += -DFW_VER_MINOR=$(FW_VER_MINOR)
	COMPILER_FLAGS += -DFW_VER_BUILD=$(FW_VER_BUILD)

	$(file >> $(LOGFILE),FW_VER_MAJOR: $(FW_VER_MAJOR))
	$(file >> $(LOGFILE),FW_VER_MINOR: $(FW_VER_MINOR))
	$(file >> $(LOGFILE),FW_VER_BUILD: $(FW_VER_BUILD))

# -----------------------------------------------------------------------------

	ifeq ($(FW_PROJECT),)
		$(error Project name is missing)
	endif

	COMPILER_FLAGS += -DFW_PROJECT=\"$(FW_PROJECT)\"
	COMPILER_FLAGS += -DFW_TYPE=\"$(FW_TYPE)\"

	$(file >> $(LOGFILE),FW_PROJECT: $(FW_PROJECT))
	$(file >> $(LOGFILE),FW_TYPE: $(FW_TYPE) $(NWLN))

# -----------------------------------------------------------------------------

	COMPILER_FLAGS += -DFW_TAG=\"$(FW_TAG)\"

	$(file >> $(LOGFILE),FW_TAG: $(FW_TAG))

# -----------------------------------------------------------------------------

	COMPILER_FLAGS += -DFW_OPT=\"$(FW_OPT)\"

	$(file >> $(LOGFILE),FW_OPT: $(FW_OPT))

# -----------------------------------------------------------------------------

	TUPLE := $(subst m, ,$(FW_PLATFORM))
	TUPLE := $(subst r, ,$(TUPLE))
	TUPLE := $(subst c, ,$(TUPLE))
	FW_PLATFORM_MCU := $(word 1,$(TUPLE))
	FW_PLATFORM_REV := $(word 2,$(TUPLE))
	FW_PLATFORM_CFG := $(word 3,$(TUPLE))
	ifeq ($(wildcard $(PLATFORM_DIR)/m$(FW_PLATFORM_MCU)/c$(FW_PLATFORM_CFG)),)
		$(error No '$(PLATFORM_DIR)/m$(FW_PLATFORM_MCU)/c$(FW_PLATFORM_CFG)' directory)
	endif

	ifeq ($(FW_BSP),)
		$(error BSP version is not specified)
	endif

	COMPILER_FLAGS += -DFW_PLATFORM=\"$(FW_PLATFORM)\"
	COMPILER_FLAGS += -DFW_PLATFORM_M$(FW_PLATFORM_MCU)
	COMPILER_FLAGS += -DFW_PLATFORM_R$(FW_PLATFORM_REV)
	COMPILER_FLAGS += -DFW_PLATFORM_C$(FW_PLATFORM_CFG)
	COMPILER_FLAGS += -DFW_BSP_$(FW_BSP)
	COMPILER_FLAGS += -DFW_BSP="\"$(FW_BSP)"\"

	TARGET := fw

	$(file >> $(LOGFILE),FW_PLATFORM: $(FW_PLATFORM))
	$(file >> $(LOGFILE),FW_BSP: $(FW_BSP))

# -----------------------------------------------------------------------------

	COMPILER_FLAGS += -DFW_GIT_HASH="\"$(FW_GIT_HASH)\""
	COMPILER_FLAGS += -DFW_GIT_BRANCH="\"$(FW_GIT_BRANCH)\""

	$(file >> $(LOGFILE),FW_GIT_HASH: $(FW_GIT_HASH))

# -----------------------------------------------------------------------------

	ifneq ($(FW_COMMENT),)
		FW_COMMENT := -$(FW_COMMENT)
	endif

	COMPILER_FLAGS += -DFW_COMMENT=\"$(FW_COMMENT)\"

	$(file >> $(LOGFILE),FW_COMMENT: $(FW_COMMENT))

# -----------------------------------------------------------------------------

	FW_NAME := $(FW_VER_MAJOR).$(FW_VER_MINOR).$(FW_VER_BUILD).$(FW_PROJECT).$\
		$(FW_TYPE).$(FW_TAG).$(FW_PLATFORM).$(FW_BSP).$(FW_GIT_HASH)$\
		$(FW_COMMENT)

	COMPILER_FLAGS += -DFW_NAME=\"$(FW_NAME)\"

	$(file >> $(LOGFILE),FW_NAME: $(FW_NAME).bin)
	$(file >> $(LOGFILE),$(NWLN))

# -----------------------------------------------------------------------------

# Include app or boot or other project type files
	include $(FW_TYPE)/Makefile.mk
	include lib/Makefile.mk
	include shared/Makefile.mk
	include thirdparty/Makefile.mk
# Include platform specific Makefile to add platform dependent .c and .s files
	include $(PLATFORM_DIR)/m$(FW_PLATFORM_MCU)/Makefile.mk

# # Object files creation settings
# COMPILER_FLAGS += -DFLASH_START_ADDR=$(FLASH_START_ADDR)
# COMPILER_FLAGS += -DFLASH_FULL_SIZE=$(FLASH_FULL_SIZE)
# COMPILER_FLAGS += -DFLASH_BOOT_ADDR=$(FLASH_BOOT_ADDR)
# COMPILER_FLAGS += -DFLASH_BOOT_SIZE=$(FLASH_BOOT_SIZE)
# COMPILER_FLAGS += -DFLASH_INFO_ADDR=$(FLASH_INFO_ADDR)
# COMPILER_FLAGS += -DFLASH_INFO_SIZE=$(FLASH_INFO_SIZE)
# COMPILER_FLAGS += -DFLASH_APP_ADDR=$(FLASH_APP_ADDR)
# COMPILER_FLAGS += -DFLASH_APP_SIZE=$(FLASH_APP_SIZE)

# Create list of prerequisites. Basically, changing files' extensions from .c and .s to .o
	OBJS := $(SRC:%.c=$(OBJECT_DIR)/%.o) $(SRC_ASM:%.s=$(OBJECT_DIR)/%.o)

	$(file >> $(LOGFILE),CFLAGS: $(CFLAGS))
	$(file >> $(LOGFILE),COMPILER_FLAGS: $(COMPILER_FLAGS))
	$(file >> $(LOGFILE),LINKER_FLAGS: $(LINKER_FLAGS))
	$(file >> $(LOGFILE),PLATFORM_OPT: $(PLATFORM_OPT))
	$(file >> $(LOGFILE),OBJS: $(OBJS))

# -----------------------------------------------------------------------------

else ifneq ($(filter objdump,$(MAKECMDGOALS)),)
	ifeq ($(wildcard $(BIN_DIR)/*.elf),)
		$(error No .elf file)
	endif
endif # ifeq ($(filter clean objdump,$(MAKECMDGOALS)),)

# Return default recipe prefix before recipes
.RECIPEPREFIX :=$(TAB)

# -----------------------------------------------------------------------------

build: $(OBJS)
	@echo "" >> $(LOGFILE)
	@echo "USER MACRO:" >> $(LOGFILE)
	@$(CC) $(filter-out -MMD,$(CFLAGS)) $(PLATFORM_OPT) $(COMPILER_FLAGS) -E -dM -xc $(NULL_FILE_PATH) | grep -v '^#define __' >> $(LOGFILE)
	@echo "" >> $(LOGFILE)
	@echo "SYSTEM MACRO:" >> $(LOGFILE)
	@$(CC) $(filter-out -MMD,$(CFLAGS)) $(PLATFORM_OPT) $(COMPILER_FLAGS) -E -dM -xc $(NULL_FILE_PATH) | grep '^#define __' >> $(LOGFILE)

	@$(CC) -o "$(BIN_DIR)/$(TARGET).elf" $(OBJS) $(LINKER_FLAGS) -T$(LD_SCRIPT) $(LIBS)
	@$(SIZECALC) "$(BIN_DIR)/$(TARGET).elf"
	@$(OBJCPY) -O ihex "$(BIN_DIR)/$(TARGET).elf" "$(BIN_DIR)/$(FW_NAME).hex"
	@$(OBJCPY) -O binary "$(BIN_DIR)/$(TARGET).elf" "$(BIN_DIR)/$(FW_NAME).bin"
	@$(OBJDUMP) -S "$(BIN_DIR)/$(TARGET).elf" > "$(BIN_DIR)/$(TARGET).lst"

clean:
	$(foreach dir,$(sort $(dir $(wildcard $(OBJECT_DIR)/*/*))),$(shell rm -rf $(call rwildcard,$(dir)/,*.*)))
	@rm -rf $(filter-out $(LOGFILE),$(wildcard $(OBJECT_DIR)/*.*))

$(OBJECT_DIR)/%.o: %.c
	@mkdir -p $(dir $@) 2> $(NULL_FILE_PATH) || echo off
	@echo "$@"
	$(OUTPUT_FLAG) $(CC) $(CFLAGS) $(PLATFORM_OPT) $(COMPILER_FLAGS)  $< -o $@

$(OBJECT_DIR)/%.o: %.s
	@mkdir -p $(dir $@) 2> $(NULL_FILE_PATH) || echo off
	@echo "$@"
	$(OUTPUT_FLAG) $(CC) $(CFLAGS) $(PLATFORM_OPT) $< -o $@

-include $(OBJS:.o=.d)
