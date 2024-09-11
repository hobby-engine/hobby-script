#include "hbs.h"

int main(int argc, const char* args[]) {
  hbs_State* h = create_state();
  hbs_InterpretResult res = hbs_run(h, args[1]);
  free_state(h);

  if (res == hbs_result_compile_err) {
    return 65;
  }
  if (res == hbs_result_runtime_err) {
    return 70;
  }
  return 0;
}
