#include <stdio.h>
#include "hby.h"
#include "mem.h"

static bool io_write(hby_State* h, int argc) {
  size_t len;
  const char* str = hby_get_str(h, 1, &len);
  fwrite(str, sizeof(char), len, stdout);
  return false;
}

static bool io_print(hby_State* h, int argc) {
  for (int i = 1; i <= argc; i++) {
    const char* s = hby_get_tostr(h, i, NULL);
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

  hby_push_lstr(h, i, len);
  return true;
}

typedef struct {
  FILE* handle;
  char* path;
  size_t path_len;
  size_t size;
  bool closed;
} File;

static bool file_gc(hby_State* h, int argc) {
  File* file = (File*)hby_get_udata(h, 1);
  if (!file->closed) {
    file->closed = true;
    fclose(file->handle);
  }

  release_arr(h, char, file->path, file->path_len);
  return false;
}

static bool file_open(hby_State* h, int argc) {
  size_t path_len;
  const char* path = hby_get_str(h, 1, &path_len);
  const char* mode = hby_get_str(h, 2, NULL);

  File* file = (File*)hby_push_udata(h, sizeof(File));
  hby_get_global(h, "io");
  hby_struct_get_const(h, "File");
  hby_udata_set_metastruct(h, -3);
  hby_pop(h, 1); // io
  hby_udata_set_finalizer(h, file_gc);

  file->handle = fopen(path, mode);

  if (file->handle == NULL) {
    file->closed = true;
    return false;
  }

  file->closed = false;

  file->path = allocate(h, char, path_len + 1);
  memcpy(file->path, path, path_len);
  file->path[path_len] = '\0';
  file->path_len = path_len;

  fseek(file->handle, 0L, SEEK_END);
  file->size = ftell(file->handle);
  rewind(file->handle);
  return true;
}

static bool file_write(hby_State* h, int argc) {
  File* file = (File*)hby_get_udata(h, 0);
  if (file->closed) {
    hby_push_bool(h, false);
    return true;
  }

  size_t data_len;
  const char* data = hby_get_str(h, 1, &data_len);
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
  if (file->closed) {
    return false;
  }

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

  hby_push_lstr(h, buf, file->size);
  return true;
}

static bool file_close(hby_State* h, int argc) {
  File* file = (File*)hby_get_udata(h, 0);
  if (file->closed) {
    return false;
  }

  file->closed = true;
  fclose(file->handle);
  return false;
}

static bool file_tostr(hby_State* h, int argc) {
  File* file = (File*)hby_get_udata(h, 0);

  hby_push_strcpy(h, "<File '");
  hby_push_lstrcpy(h, file->path, file->path_len);
  hby_concat(h);
  hby_push_strcpy(h, "'>");
  hby_concat(h);
  return true;
}

hby_StructMethod file[] = {
  {"open", file_open, 2, hby_static_fn},
  {"close", file_close, 0, hby_method},
  {"write", file_write, 1, hby_method},
  {"readall", file_readall, 0, hby_method},
  {"tostr", file_tostr, 0, hby_method},
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
  hby_struct_add_members(h, io, -1);

  hby_push_struct(h, "File");
  hby_struct_add_members(h, file, -1);
  hby_struct_add_const(h, NULL, -2); // File

  hby_set_global(h, NULL, -1); // io

  hby_get_global(h, "io");

  File* _stdout = (File*)hby_push_udata(h, sizeof(File));
  hby_push(h, -2);
  hby_struct_get_const(h, "File");
  hby_udata_set_metastruct(h, -3);
  hby_pop(h, 1);
  _stdout->handle = stdout;
  _stdout->size = 0;
  _stdout->closed = true;

  hby_struct_add_const(h, "stdout", -2);

  File* _stderr = (File*)hby_push_udata(h, sizeof(File));
  hby_push(h, -2);
  hby_struct_get_const(h, "File");
  hby_udata_set_metastruct(h, -3);
  hby_pop(h, 1);
  _stderr->handle = stderr;
  _stderr->size = 0;
  _stderr->closed = true;

  hby_struct_add_const(h, "stderr", -2);

  File* _stdin = (File*)hby_push_udata(h, sizeof(File));
  hby_push(h, -2);
  hby_struct_get_const(h, "File");
  hby_udata_set_metastruct(h, -3);
  hby_pop(h, 1);
  _stdin->handle = stdin;
  _stdin->size = 0;
  _stdin->closed = true;

  hby_struct_add_const(h, "stdin", -2);

  hby_pop(h, 1); // io

  return false;
}
