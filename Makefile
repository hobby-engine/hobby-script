CC = gcc
AR = ar rcs
CFLAGS = -Wall -Wextra -Wno-unused-parameter -fPIC -Isrc
SHARED_CFLAGS =
LDFLAG = -lm

RM = rm -f
MKDIR = mkdir -p
FRMDIR = rm -rf

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

ifeq (release,$(config))
	CFLAGS += -O2
else
	config = debug
	CFLAGS += -O2 -g -Dhby_stress_gc
	ifeq (Linux,$(HOST_SYS))
		CFLAGS += -fsanitize=address -fsanitize=address
	endif
endif

HBY_EXE = hobbyc
HBY_SO = libhobbyc.so
HBY_A = libhobbyc.a

HBY_CORE_C = \
	src/hby.c src/arr.c src/chunk.c src/parser.c src/debug.c src/lexer.c \
	src/lib_arr.c src/lib_core.c src/lib_ease.c src/lib_io.c src/lib_map.c \
	src/lib_math.c src/lib_rng.c src/lib_str.c src/lib_sys.c src/map.c \
	src/table.c src/mem.c src/obj.c src/state.c src/tostr.c src/val.c src/vm.c
HBY_CORE_O = $(HBY_CORE_C:src/%.c=bin/%.o)
HBY_CORE_D = $(HBY_CORE_O:%.o=%.d)

HBY_EXE_C = src/main.c
HBY_EXE_O = $(HBY_EXE_C:src/%.c=bin/%.o)
HBY_EXE_D = $(HBY_EXE_O:%.o=%.d)

.PHONY: default all clean clangd_compile_flags

default: all

all: $(HBY_SO) $(HBY_EXE) $(HBY_A)
	@echo "CC: $(CC)"
	@echo "CFLAG: $(CFLAG)"
	@echo "LDFLAG: $(LDFLAG)"

$(HBY_EXE): $(HBY_A) $(HBY_EXE_O)
	@echo "compiling $@"
	@$(CC) -o $@ $(HBY_EXE_O) $(HBY_A) $(CFLAGS) $(LDFLAG)

$(HBY_SO): $(HBY_CORE_O)
	@echo "compiling $@"
	@$(CC) -o $@ $(HBY_CORE_O) $(CFLAGS) -shared $(SHARED_FLAG) $(LDFLAG)

$(HBY_A): $(HBY_CORE_O)
	@echo "compiling $@"
	@$(AR) $@ $?

bin/%.o: src/%.c
	@echo "compiling $< -> $@"
	@$(CC) -o $@ -c $< $(CFLAGS) -MMD -MD

clean:
	$(RM) $(HBY_CORE_O) $(HBY_EXE_O) $(HBY_EXE) $(HBY_SO) $(HBY_A)

clangd_compile_flags:
	@echo "" > compile_flags.txt
	@$(foreach flag,$(CFLAGS),echo $(flag) >> compile_flags.txt;)

-include $(HBY_CORE_D)
-include $(HBY_EXE_D)
