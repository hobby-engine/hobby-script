#ifndef __HOBBY_HBY_ERR_H
#define __HOBBY_HBY_ERR_H

typedef enum {
#define errmsg(name, msg) \
  err_##name, err_##name##_ = err_##name + sizeof(msg)-1,
#include "errmsg.h"
} ErrorMsg;

#endif // __HOBBY_HBY_ERR_H
