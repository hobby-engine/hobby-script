#ifndef __HBY_VM_H
#define __HBY_VM_H

#include "hby.h"
#include "state.h"

void vm_call(hby_State* h, Val val, int argc);
void vm_invoke(hby_State* h, GcStr* name, int argc);
hby_InterpretResult vm_interp(hby_State* h, const char* path, const char* c);

#endif // __HBY_VM_H
