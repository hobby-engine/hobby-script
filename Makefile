CC = gcc
AR = ar rcs
CFLAG = -Wall -Wextra -Wno-unused-parameter -fPIC -Isrc
SHARED_CFLAG =
LDFLAG = -lm

RM = rm -f
MKDIR = mkdir -p
FRMDIR = rm -rf

TARGET_SUFFIX = 

ifeq (,$(findstring Windows,$(OS)))
	HOST_SYS = $(shell uname -s)
else
	RM = del /q
	MKDIR = mkdir
	FRMDIR = del /s /q
	HOST_SYS = Windows
endif

ifeq (Linux,$(HOST_SYS))
	CFLAG += -Dhby_linux
	CFLAG += -Dhby_posix
endif
ifeq (Darwin,$(HOST_SYS))
	CFLAG += -Dhby_apple
	CFLAG += -Dhby_posix
endif
ifeq (iOS,$(HOST_SYS))
	CFLAG += -Dhby_apple
	CFLAG += -Dhby_posix
endif
ifeq (Windows,$(HOST_SYS))
	CFLAG += -Dhby_windows
endif


ifeq (release,$(config))
	CFLAG += -O2
else
	config = debug
	CFLAG += -O2 -g
	ifeq (Linux,$(HOST_SYS))
		CFLAG += -fsanitize=address -fsanitize=address
	endif
	TARGET_SUFFIX = _debug
endif

BIN = bin
HBY_EXE = $(BIN)/hobbyc$(TARGET_SUFFIX)
HBY_SO = $(BIN)/libhobbyc$(TARGET_SUFFIX).so

HBY_SRC = \
	src/hby.c src/arr.c src/chunk.c src/parser.c src/debug.c src/lexer.c \
	src/lib_arr.c src/lib_core.c src/lib_ease.c src/lib_io.c src/lib_math.c \
	src/lib_rng.c src/lib_str.c src/lib_sys.c src/map.c src/mem.c src/obj.c \
	src/state.c src/tostr.c src/val.c src/vm.c

HBY_OBJ = $(HBY_SRC:%.c=$(BIN)/%$(TARGET_SUFFIX).o)
HBY_DEPENDS = $(HBY_OBJ:.o=.d)

HBY_EXE_SRC = src/main.c
HBY_EXE_OBJ = $(HBY_EXE_SRC:%.c=$(BIN)/%$(TARGET_SUFFIX).o)
HBY_EXE_DEPENDS = $(HBY_EXE_OBJ:.o=.d)

ALL_OBJ = $(HBY_OBJ) $(HBY_EXE_OBJ)
ALL_TARGETS = $(HBY_EXE) $(HBY_A) $(HBY_SO)
ALL_GEN = $(ALL_OBJ) $(ALL_TARGETS)

.PHONY: default all clean clangd_compile_flags

default: all

all: $(HBY_SO) $(HBY_EXE)
	@echo "CC: $(CC)"
	@echo "CFLAG: $(CFLAG)"
	@echo "LDFLAG: $(LDFLAG)"

$(HBY_EXE): $(HBY_OBJ) $(HBY_EXE_OBJ)
	@$(MKDIR) $(@D)
	@echo "compiling $@"
	$(CC) -o $@ $(HBY_OBJ) $(HBY_EXE_OBJ) $(CFLAG) $(LDFLAG)

$(HBY_SO): $(HBY_OBJ)
	@$(MKDIR) $(@D)
	@echo "compiling $@"
	@$(CC) -o $@ $(HBY_OBJ) $(CFLAG) -shared $(SHARED_FLAG) $(LDFLAG)

$(BIN)/%$(TARGET_SUFFIX).o: %.c
	@$(MKDIR) $(@D)
	@echo "compiling $< -> $@"
	@$(CC) -o $@ -c $< $(CFLAG) -MMD -MD

clean:
	$(FRMDIR) $(BIN)

clangd_compile_flags:
	@echo "" > compile_flags.txt
	@$(foreach flag,$(CFLAG),echo $(flag) >> compile_flags.txt;)

-include $(HBY_DEPENDS)
-include $(HBY_EXE_DEPENDS)
