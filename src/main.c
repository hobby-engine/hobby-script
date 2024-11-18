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

bool on_error(hby_State* h, int argc) {
  const char* err_msg = hby_get_str(h, 1, NULL);
  fprintf(stderr, "stack trace:\n");
  for (int i = 2; i <= argc; i++) {
    fprintf(stderr, "\t%s\n", hby_get_str(h, i, NULL));
  }
  fprintf(stderr, "[error] %s\n", err_msg);
  return false;
}

static void repl(hby_State* h) {
  char line[1024];
  hby_push_cfunc(h, "on_error", on_error, -1);
  while (true) {
    printf(">");
    if (!fgets(line, sizeof(line), stdin)) {
      putc('\n', stdout);
      break;
    }

    int errc = hby_compile(h, "<repl>", line);
    if (errc > 0) {
      for (int i = errc - 1; i >= 0; i--) {
        fprintf(stderr, "[error] %s\n", hby_get_str(h, -1 - i, NULL));
      }
      continue;
    }

    hby_pcall(h, -2, 0);
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

  hby_push_cfunc(h, "on_error", on_error, -1);
  if (collected.flags & doexpr_flag) {
    int errc = hby_compile(h, "<cli>", collected.doexpr_str);
    if (errc > 0) {
      for (int i = errc - 1; i >= 0; i--) {
        fprintf(stderr, "[error] %s\n", hby_get_str(h, -1 - i, NULL));
      }
      free_state(h);
      return 65;
    }
    bool is_ok = hby_pcall(h, -2, 0);
    if (!is_ok) {
      free_state(h);
      return 70;
    }
    // hby_run_str(h, "<cli>", collected.doexpr_str);
  } else if (collected.path == NULL) {
    repl(h);
  } else {
    int errc = hby_compile_file(h, collected.path);
    if (errc > 0) {
      for (int i = errc - 1; i >= 0; i--) {
        fprintf(stderr, "[error] %s\n", hby_get_str(h, -1 - i, NULL));
      }
      free_state(h);
      return 65;
    }
    
    bool is_ok = hby_pcall(h, -2, 0);
    if (!is_ok) {
      free_state(h);
      return 70;
    }

    if (collected.flags & replafter_flag) {
      repl(h);
    }
  }

  free_state(h);
  return 0;
}
