#include "lib.h"

#include "hbs.h"
#include "state.h"
#include "obj.h"

static bool core_tostr(hbs_State* h, int argc) {
  hbs_to_str(h, 1);
  return true;
}

static bool core_tonum(hbs_State* h, int argc) {
  switch (hbs_get_type(h, 1)) {
    case hbs_type_number:
      hbs_push(h, 1);
      return true;
    case hbs_type_string:
      hbs_push_num(h, strtod(hbs_get_str(h, 1, NULL), NULL));
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
  const char* path = hbs_get_str(h, 1, &len);
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
  hbs_err(h, hbs_get_and_to_str(h, 1, NULL));
  return false;
}

static bool core_assert(hbs_State* h, int argc) {
  if (!hbs_get_bool(h, 1)) {
    hbs_err(h, hbs_get_and_to_str(h, 2, NULL));
  }
  return false;
}

bool open_core(hbs_State* h, int argc) {
  hbs_push_c_fn(h, "tostr", core_tostr, 1);
  hbs_set_global(h, "tostr");
  hbs_pop(h, 2);

  hbs_push_c_fn(h, "tonum", core_tonum, 1);
  hbs_set_global(h, "tonum");
  hbs_pop(h, 2);

  hbs_push_c_fn(h, "import", core_import, 1);
  hbs_set_global(h, "import");
  hbs_pop(h, 2);

  hbs_push_c_fn(h, "err", core_err, 1);
  hbs_set_global(h, "err");
  hbs_pop(h, 2);

  hbs_push_c_fn(h, "assert", core_assert, 2);
  hbs_set_global(h, "assert");
  hbs_pop(h, 2);
  return false;
}
