#include <stdio.h>
#include "hbs.h"

const char help_str[] = "usage: hobbyc [file]\n";

#define bit_flag(n) (1 << n)

typedef enum {
  help = bit_flag(0),
} Args;

static void repl(hbs_State* h) {
  char line[1024];
  while (true) {
    printf(">");
    if (!fgets(line, sizeof(line), stdin)) {
      putc('\n', stdout);
      break;
    }

    hbs_run_string(h, "repl", line);
  }
}

static void show_help() {
  fputs(help_str, stderr);
}

int main(int argc, const char* args[]) {
  hbs_State* h = create_state();

  if (argc == 1) {
    repl(h);
  } else if (argc == 2) {
    hbs_InterpretResult res = hbs_run(h, args[1]);
    free_state(h);

    if (res == hbs_result_compile_err) {
      return 65;
    } else if (res == hbs_result_runtime_err) {
      return 70;
    }
  } else {
    show_help();
    return 1;
  }

  return 0;
}
