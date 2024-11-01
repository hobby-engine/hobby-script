#include <stdio.h>
#include "hby.h"
#include "mem.h"

static bool io_write(hby_State* h, int argc) {
  size_t len;
  const char* str = hby_get_string(h, 1, &len);
  fwrite(str, sizeof(char), len, stdout);
  return false;
}

static bool io_print(hby_State* h, int argc) {
  for (int i = 1; i <= argc; i++) {
    const char* s = hby_get_tostring(h, i, NULL);
    if (i > 1) {
      putc('\t', stdout);
    }
    fputs(s, stdout);
  }
  putc('\n', stdout);

  return false;
}

static bool io_input(hby_State* h, int argc) {
  int cap = 8;
  int len = 0;
  char* i = allocate(h, char, cap);

  int c = EOF;
  while ((c = getchar()) != '\n' && c != EOF) {
    if (len + 1 > cap) {
      int old_cap = cap;
      cap = grow_cap(cap);
      i = grow_arr(h, char, i, old_cap, cap);
    }
    i[len++] = c;
  }

  i = grow_arr(h, char, i, cap, len + 1);
  i[len] = '\0';

  hby_push_string(h, i, len);
  return true;
}

typedef struct {
  FILE* handle;
  bool closed;
  size_t size;
} File;

static bool file_gc(hby_State* h, int argc) {
  File* file = (File*)hby_get_udata(h, 1);
  if (!file->closed) {
    file->closed = true;
    fclose(file->handle);
  }
  return false;
}

static bool file_open(hby_State* h, int argc) {
  size_t path_len;
  const char* path = hby_get_string(h, 1, &path_len);
  const char* mode = hby_get_string(h, 2, NULL);

  File* file = (File*)hby_create_udata(h, sizeof(File));
  hby_get_global(h, "File");
  hby_set_udata_struct(h, -2);
  hby_set_udata_finalizer(h, file_gc);

  file->handle = fopen(path, mode);

  if (file->handle == NULL) {
    file->closed = true;
    return false;
  }

  file->closed = false;

  fseek(file->handle, 0L, SEEK_END);
  file->size = ftell(file->handle);
  rewind(file->handle);
  return true;
}

static bool file_write(hby_State* h, int argc) {
  File* file = (File*)hby_get_udata(h, 0);
  size_t data_len;
  const char* data = hby_get_string(h, 1, &data_len);
  size_t bytes_written = fwrite(data, sizeof(char), data_len, file->handle);

  if (bytes_written < data_len) {
    hby_push_bool(h, false);
    return true;
  }

  hby_push_bool(h, true);
  return true;
}

static bool file_readall(hby_State* h, int argc) {
  File* file = (File*)hby_get_udata(h, 0);
  char* buf = (char*)malloc(file->size + 1);
  if (buf == NULL) {
    return false;
  }

  size_t bytes_read = fread(buf, sizeof(char), file->size, file->handle);
  if (bytes_read < file->size) {
    free(buf);
    return false;
  }

  buf[bytes_read] = '\0';

  hby_push_string(h, buf, file->size);
  return true;
}

static bool file_close(hby_State* h, int argc) {
  File* file = (File*)hby_get_udata(h, 0);
  file->closed = true;
  fclose(file->handle);
  return false;
}

hby_StructMethod file[] = {
  {"open", file_open, 2, hby_static_fn},
  {"close", file_close, 0, hby_method},
  {"write", file_write, 1, hby_method},
  {"readall", file_readall, 0, hby_method},
  {NULL, NULL, 0, 0},
};

hby_StructMethod io[] = {
  {"print", io_print, -1, hby_static_fn},
  {"write", io_write,  1, hby_static_fn},
  {"input", io_input,  0, hby_static_fn},
  {NULL, NULL, 0, 0},
};

bool open_io(hby_State* h, int argc) {
  hby_push_struct(h, "io");
  hby_add_members(h, io, -2);
  hby_set_global(h, "io");
  hby_pop(h, 1);

  hby_push_struct(h, "File");
  hby_add_members(h, file, -2);
  hby_set_global(h, "File");
  hby_pop(h, 1);

  return false;
}
