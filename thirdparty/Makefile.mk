$(file >> $(LOGFILE),Thirdparty C files included for building:)
SRC_THIRDPARTY_C += $(wildcard thirdparty/wsh-shell/src/*.c)
$(foreach path,$(SRC_THIRDPARTY_C),$(file >> $(LOGFILE),$(TAB)$(strip $(path))))
$(file >> $(LOGFILE),$(NWLN))

SRC += $(SRC_THIRDPARTY_C)

COMPILER_FLAGS += -Ithirdparty/wsh-shell/src
