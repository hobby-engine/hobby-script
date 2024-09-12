#include "lib.h"

#include "hbs.h"
#include "state.h"
#include "obj.h"

static bool core_tostr(hbs_State* h, int argc) {
  hbs_tostr(h, 1);
  return true;
}

static bool core_tonum(hbs_State* h, int argc) {
  switch (hbs_get_type(h, 1)) {
    case hbs_type_number:
      hbs_push(h, 1);
      return true;
    case hbs_type_string:
      hbs_push_num(h, strtod(hbs_get_string(h, 1, NULL), NULL));
      return true;
    case hbs_type_bool:
      hbs_push_num(h, hbs_get_bool(h, 1) ? 1 : 0);
      return true;
    case hbs_type_null:
      hbs_push_num(h, 0);
      return true;
    default:
      hbs_err(h, "Cannot convert given value to number");
      break;
  }
  return false;
}

static bool core_import(hbs_State* h, int argc) {
  size_t len;
  const char* path = hbs_get_string(h, 1, &len);
  if (file_imported(h, path)) {
    Val val;
    if (get_map(&h->files, copy_str(h, path, len), &val)) {
      push(h, val);
    } else {
      hbs_push_null(h);
    }
    return true;
  }

  hbs_InterpretResult res = hbs_run(h, path);
  // TODO: Handle this better
  if (res == hbs_result_compile_err) {
    hbs_err(h, "Compile error");
  }

  hbs_push(h, -1);
  return true;
}

static bool core_err(hbs_State* h, int argc) {
  hbs_err(h, hbs_get_tostring(h, 1, NULL));
  return false;
}

static bool core_assert(hbs_State* h, int argc) {
  if (!hbs_get_bool(h, 1)) {
    hbs_err(h, hbs_get_tostring(h, 2, NULL));
  }
  return false;
}

static bool core_typestr(hbs_State* h, int argc) {
  size_t len;
  const char* type_name = hbs_typestr(hbs_get_type(h, 1), &len);
  hbs_push_string_copy(h, type_name, len);
  return true;
}

static bool core_typeof(hbs_State* h, int argc) {
  hbs_push_num(h, hbs_get_type(h, 1));
  return true;
}

hbs_CFnArgs core_mod[] = {
  {"tostr", core_tostr, 1},
  {"tonum", core_tonum, 1},
  {"err", core_err, 1},
  {"assert", core_assert, 2},
  {"import", core_import, 1},
  {"typeof", core_typeof, 1},
  {"typestr", core_typestr, 1},
  {NULL, NULL, 0},
};

bool open_core(hbs_State* h, int argc) {
  hbs_push_enum(h, "Type");

  for (int i = 0; i < hbs_type_count; i++) {
    size_t len;
    const char* type_name = hbs_typestr(i, &len);
    hbs_add_enum(h, type_name, -1);
  }

  hbs_set_global(h, NULL); // Type enum

  for (hbs_CFnArgs* args = core_mod; args->name != NULL; args++) {
    hbs_push_cfunction(h, args->name, args->fn, args->argc);
    hbs_set_global(h, NULL);
  }
  return false;
}
