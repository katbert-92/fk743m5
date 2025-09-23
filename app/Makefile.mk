$(file >> $(LOGFILE),App C files included for building:)
SRC_APP_C += $(call rwildcard,app,*.c)
$(foreach path,$(SRC_APP_C),$(file >> $(LOGFILE),$(TAB)$(strip $(path))))
$(file >> $(LOGFILE),$(NWLN))

SRC += $(SRC_APP_C)

COMPILER_FLAGS += -Iapp
COMPILER_FLAGS += -Iapp/conf
COMPILER_FLAGS += -Iapp/features
COMPILER_FLAGS += -Iapp/features/rtos_analyzer
COMPILER_FLAGS += -Iapp/features/health_check
COMPILER_FLAGS += -Iapp/shell
COMPILER_FLAGS += -Iapp/shell/cmd
COMPILER_FLAGS += -Iapp/storage
COMPILER_FLAGS += -Iapp/storage/fs_wrapper
COMPILER_FLAGS += -Iapp/storage/io
COMPILER_FLAGS += -Iapp/system
