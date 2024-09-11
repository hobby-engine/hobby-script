#ifndef __HBS_VM_H
#define __HBS_VM_H

#include "hbs.h"
#include "state.h"

void vm_call(hbs_State* h, Val val, int argc);
void vm_invoke(hbs_State* h, GcStr* name, int argc);
hbs_InterpretResult vm_interp(hbs_State* h, const char* path, const char* c);

#endif // __HBS_VM_H
