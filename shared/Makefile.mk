$(file >> $(LOGFILE),Shared C files included for building:)
SRC_SHARED_C += $(call rwildcard,shared,*.c)
$(foreach path,$(SRC_SHARED_C),$(file >> $(LOGFILE),$(TAB)$(strip $(path))))
$(file >> $(LOGFILE),$(NWLN))

SRC += $(SRC_SHARED_C)

COMPILER_FLAGS += -Ishared
COMPILER_FLAGS += -Ishared/debug
