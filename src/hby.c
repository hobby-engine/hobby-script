#include "hby.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "state.h"
#include "vm.h"

static char* read_file(const char* path) {
  FILE* file = fopen(path, "rb");
  if (file == NULL) {
    fprintf(stderr, "Could not open file '%s'.\n", path);
    exit(4);
  }

  fseek(file, 0L, SEEK_END);
  size_t file_size = ftell(file);
  rewind(file);

  char* buf = (char*)malloc(file_size + 1);
  if (buf == NULL) {
    fprintf(stderr, "File '%s' too large to read.\n", path);
    exit(4);
  }

  size_t bytes_read = fread(buf, sizeof(char), file_size, file);
  if (bytes_read < file_size) {
    fprintf(stderr, "Could not read file '%s'.", path);
    exit(4);
  }

  buf[bytes_read] = '\0';

  fclose(file);
  return buf;
}

hby_InterpretResult hby_run_string(
    hby_State* h, const char* file_name, const char* str) {
  hby_InterpretResult res = vm_interp(h, file_name, str);
  return res;
}

hby_InterpretResult hby_run(hby_State* h, const char* path) {
  char* src = read_file(path);
  hby_InterpretResult res = hby_run_string(h, path, src);
  free(src);
  return res;
}
