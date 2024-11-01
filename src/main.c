#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "hby.h"

const char help_str[] =
  "usage: %s [options] [script] [-- args]\n"
  "  -h, --help     show help text\n"
  "  -e stat        execute string 'stat'\n"
  "  -i             enter the REPL after executing 'script'\n"
  "  -v, --version  show version info\n"
  "  --             start script args\n";

#define bit_flag(n) (1 << n)

#define invalid_flag   bit_flag(0)
#define help_flag      bit_flag(1)
#define doexpr_flag    bit_flag(2)
#define replafter_flag bit_flag(3)
#define version_flag   bit_flag(4)

typedef struct {
  uint8_t flags;
  const char* doexpr_str;
  const char* path;
  int argc;
  const char** args;
} Args;

static void repl(hby_State* h) {
  char line[1024];
  while (true) {
    printf(">");
    if (!fgets(line, sizeof(line), stdin)) {
      putc('\n', stdout);
      break;
    }

    hby_run_string(h, "repl", line);
  }
}

static void show_help(const char* program_name) {
  fprintf(stderr, help_str, program_name);
}

static bool strs_eql(const char* a, const char* b) {
  int alen = strlen(a);
  int blen = strlen(b);
  return alen == blen && memcmp(a, b, alen) == 0;
}

static Args collect_args(int argc, const char* args[]) {
  Args collected;
  collected.flags = 0;
  collected.doexpr_str = NULL;
  collected.path = NULL;
  collected.args = NULL;
  collected.argc = 0;

  for (int i = 1; i < argc; i++) {
    const char* arg = args[i];
    int len = strlen(arg);

    char first = arg[0];
    switch (first) {
      case '-':
        if (len >= 2) {
          switch (arg[1]) {
            case '-':
              if (strs_eql(arg, "--help")) {
                collected.flags |= help_flag;
              } else if (strs_eql(arg, "--version")) {
                collected.flags |= version_flag;
              } else if (len == 2) {
                collected.args = args + i + 1;
                collected.argc = argc - i - 1;
                return collected;
              } else {
                fprintf(
                  stderr,
                  "%s: unrecognized option '%s'\n",
                  args[0], arg);
                collected.flags |= invalid_flag;
              }
              break;
            case 'h': collected.flags |= help_flag; break;
            case 'i': collected.flags |= replafter_flag; break;
            case 'v': collected.flags |= version_flag; break;
            case 'e':
              collected.flags |= doexpr_flag;
              collected.doexpr_str = args[++i];
              break;
            default:
              fprintf(
                stderr,
                "%s: unrecognized option '%s'\n",
                args[0], arg);
              collected.flags |= invalid_flag;
              break;
          }
        }
        break;
      default:
        collected.path = arg;
        break;
    }
  }

  return collected;
}

int main(int argc, const char* args[]) {
  Args collected = collect_args(argc, args);

  if (collected.flags & invalid_flag) {
    show_help(args[0]);
    return 1;
  }

  if (collected.flags & help_flag) {
    show_help(args[0]);
    return 0;
  }

  if (collected.flags & version_flag) {
    printf("%s %s\n", args[0], hby_version);
    return 0;
  }

  hby_State* h = create_state();
  hby_cli_args(h, collected.argc, collected.args);

  if (collected.flags & doexpr_flag) {
    hby_run_string(h, "<cli>", collected.doexpr_str);
  } else if (collected.path == NULL) {
    repl(h);
  } else {
    hby_InterpretResult res = hby_run(h, collected.path);

    if (collected.flags & replafter_flag) {
      repl(h);
    } else {
      if (res == hby_result_compile_err) {
        free_state(h);
        return 65;
      } else if (res == hby_result_runtime_err) {
        free_state(h);
        return 70;
      }
    }
  }

  free_state(h);
  return 0;
}
