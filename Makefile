CC = gcc
AR = ar rcs
CFLAG = -Wall -Wextra -Wno-unused-parameter -fPIC -Isrc
SHARED_CFLAG =
LDFLAG = -lm

RM = rm -f
MKDIR = mkdir -p
RMDIR = rmdir 2>/dev/null

ifeq (release,$(config))
	CFLAG += -O3
else
	config = debug
	CFLAG += -O2 -g
	CFLAG += -fsanitize=address -fsanitize=address
endif

ifeq (,$(findstring Windows,$(OS)))
	HOST_SYS = $(shell uname -s)
else
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

BIN = bin
HBY_EXE = $(BIN)/hobbyc
HBY_SO = $(BIN)/libhobbyc.so

HBY_SRC = \
	src/hby.c src/api.c src/arr.c src/chunk.c src/compiler.c src/debug.c \
  src/lexer.c src/lib_arr.c src/lib_core.c src/lib_ease.c src/lib_io.c \
  src/lib_math.c src/lib_rand.c src/lib_str.c src/lib_sys.c src/map.c \
  src/mem.c src/obj.c src/state.c src/tostr.c src/val.c src/vm.c

HBY_OBJ = $(HBY_SRC:%.c=$(BIN)/%.o)
HBY_DEPENDS = $(HBY_OBJ:%.o=$(BIN)/%.d)

HBY_EXE_SRC = src/main.c
HBY_EXE_OBJ = $(HBY_EXE_SRC:%.c=$(BIN)/%.o)
HBY_EXE_DEPENDS = $(HBY_EXE_OBJ:%.o=$(BIN)/%.d)

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
	@$(CC) -o $@ $(HBY_OBJ) $(HBY_EXE_OBJ) $(CFLAG) $(LDFLAG)

$(HBY_SO): $(HBY_OBJ)
	@$(MKDIR) $(@D)
	@echo "compiling $@"
	@$(CC) -o $@ $< $(CFLAG) -shared $(SHARED_FLAG) $(LDFLAG)

$(BIN)/%.o: %.c
	@$(MKDIR) $(@D)
	@echo "compiling $< -> $@"
	@$(CC) -o $@ -c $< $(CFLAG) -MMD -MD

clean:
	$(RM) $(ALL_GEN)

clangd_compile_flags:
	@echo "" > compile_flags.txt
	@$(foreach flag,$(CFLAG),echo $(flag) >> compile_flags.txt;)

-include $(HBY_DEPENDS)
-include $(HBY_EXE_DEPENDS)
