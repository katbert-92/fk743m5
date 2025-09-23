# Global build configuration
OBJECT_DIR := debug
PLATFORM_DIR := platform
BIN_DIR := $(OBJECT_DIR)
LOGFILE := $(OBJECT_DIR)/make_build.log
PLATFORM_OPT :=
COMPILER_OUTPUT := $(output)
OUTPUT_FLAG := 

# Firmware naming and some settings
FW_PROJECT := $(prj)
FW_TYPE := $(type)
FW_TAG := $(tag)
FW_PLATFORM := $(platform)
FW_BSP := $(bsp)
FW_VER_MAJOR := $(major)
FW_VER_MINOR := $(minor)
FW_VER_BUILD := $(build)
FW_GIT_HASH := $(hash)
FW_GIT_BRANCH := $(branch)
FW_COMMENT := 
FW_OPT := $(opt)

FW_NAME_TYPE_APP := app
FW_NAME_TYPE_BOOT := boot
FW_NAME_TAG_PRD := prd
FW_NAME_TAG_DEV := dev

# Platform dependent customization for linker script
PL_LD_SCRIPT_SECTION_CUSTOM_WILDCARD_FEATURE :=

# Compilation global settings
# CFLAGS= -Wpedantic 
# CFLAGS= -Wextra
CFLAGS += -c
CFLAGS += -g
CFLAGS += -specs=nano.specs
CFLAGS += -ffunction-sections
CFLAGS += -fdata-sections
CFLAGS += -fstack-usage
CFLAGS += -std=gnu11
CFLAGS += -Wall
CFLAGS += -Wno-unused-function
CFLAGS += -Wno-switch
CFLAGS += -Wno-format
CFLAGS += -Wno-comment
CFLAGS += -MMD
CFLAGS += -fvisibility=hidden
CFLAGS += -s

# Linker global settings
LINKER_FLAGS += -specs=nano.specs
LINKER_FLAGS += -specs=nosys.specs
LINKER_FLAGS += -u _printf_float
LINKER_FLAGS += -u _scanf_float
LINKER_FLAGS += -static
LINKER_FLAGS += -Wl,--start-group -lc -lm -Wl,--end-group
LINKER_FLAGS += -Wl,--gc-sections
LINKER_FLAGS += -Wl,--print-memory-usage
LINKER_FLAGS += -Wl,--no-warn-rwx-segments
# LINKER_FLAGS += -Wl,--verbose # for debug
# LINKER_FLAGS += -v # for debug
# LINKER_FLAGS += -Wl,--trace # for debug
