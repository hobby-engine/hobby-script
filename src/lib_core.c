#include "lib.h"

#include "hby.h"
#include "state.h"
#include "obj.h"

static bool core_tostr(hby_State* h, int argc) {
  hby_tostr(h, 1);
  return true;
}

static bool core_tonum(hby_State* h, int argc) {
  switch (hby_get_type(h, 1)) {
    case hby_type_number:
      hby_push(h, 1);
      return true;
    case hby_type_str:
      hby_push_num(h, strtod(hby_get_str(h, 1, NULL), NULL));
      return true;
    case hby_type_bool:
      hby_push_num(h, hby_get_bool(h, 1) ? 1 : 0);
      return true;
    case hby_type_null:
      hby_push_num(h, 0);
      return true;
    default:
      hby_err(h, "Cannot convert given value to number");
      break;
  }
  return false;
}

static bool core_import(hby_State* h, int argc) {
  size_t len;
  const char* path = hby_get_str(h, 1, &len);
  if (file_imported(h, path)) {
    Val val;
    if (get_map(&h->files, copy_str(h, path, len), &val)) {
      push(h, val);
    } else {
      hby_push_null(h);
    }
    return true;
  }

  hby_InterpretResult res = hby_run(h, path);
  // TODO: Handle this better
  if (res == hby_result_compile_err) {
    hby_err(h, "Compile error");
  }

  hby_push(h, -1);
  return true;
}

static bool core_err(hby_State* h, int argc) {
  hby_err(h, hby_get_tostr(h, 1, NULL));
  return false;
}

static bool core_assert(hby_State* h, int argc) {
  if (!hby_get_bool(h, 1)) {
    hby_err(h, hby_get_tostr(h, 2, NULL));
  }
  return false;
}

static bool core_typestr(hby_State* h, int argc) {
  size_t len;
  const char* type_name = hby_get_type_name(hby_get_type(h, 1), &len);
  hby_push_lstrcpy(h, type_name, len);
  return true;
}

hby_CFnArgs core_mod[] = {
  {"tostr", core_tostr, 1},
  {"tonum", core_tonum, 1},
  {"err", core_err, 1},
  {"assert", core_assert, 2},
  {"import", core_import, 1},
  {"typestr", core_typestr, 1},
  {NULL, NULL, 0},
};

bool open_core(hby_State* h, int argc) {
  for (hby_CFnArgs* args = core_mod; args->name != NULL; args++) {
    hby_push_cfunc(h, args->name, args->fn, args->argc);
    hby_set_global(h, NULL, -1);
  }

  hby_push_struct(h, "Number");
  h->number_struct = as_struct(*(h->top - 1));
  hby_set_global(h, NULL, -1);

  hby_push_struct(h, "Boolean");
  h->boolean_struct = as_struct(*(h->top - 1));
  hby_set_global(h, NULL, -1);

  hby_push_struct(h, "Function");
  h->function_struct = as_struct(*(h->top - 1));
  hby_set_global(h, NULL, -1);

  hby_push_struct(h, "UData");
  h->udata_struct = as_struct(*(h->top - 1));
  hby_set_global(h, NULL, -1);

  return false;
}
