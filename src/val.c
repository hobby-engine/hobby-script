#include "val.h"

bool vals_eql(Val a, Val b) {
#ifdef nan_boxing
  return a == b;
#else
  if (a.type != b.type) {
    return false;
  }

  switch (a.type) {
    case val_bool: return as_bool(a) == as_bool(b);
    case val_null: return true;
    case val_num: return as_num(a) == as_num(b);
    case val_obj: return as_obj(a) == as_obj(b);
  }

  return false;
#endif
}
