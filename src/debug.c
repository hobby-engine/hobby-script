#include "debug.h"

#include <stdio.h>
#include "tostr.h"
#include "obj.h"
#include "chunk.h"
#include "val.h"

static int simple_bc(const char* name, int idx) {
  printf("%s\n", name);
  return idx + 1;
}

static int const_bc(hbs_State* h, const char* name, Chunk* c, int idx) {
  u8 constant = c->code[idx + 1];
  printf(
    "%-16s %4d '%s'\n", 
    name, constant, to_str(h, c->consts.items[constant])->chars);
  return idx + 2;
}

static int byte_bc(const char* name, Chunk* c, int idx) {
  u8 slot = c->code[idx + 1];
  printf("%-16s %4d\n", name, slot);
  return idx + 2;
}

static int jmp_bc(const char* name, int sign, Chunk* c, int idx) {
  u16 jmp = (u16)(c->code[idx + 1] << 8);
  jmp |= c->code[idx + 2];
  printf("%-16s %4d -> %d\n", name, idx, idx + 3 + sign * jmp);
  return idx + 3;
}

static int invoke_bc(hbs_State* h, const char* name, Chunk* c, int offset) {
  u8 constant = c->code[offset + 1];
  u8 argc = c->code[offset + 2];
  printf(
    "%-16s (%d args) %4d '%s'\n",
    name, argc, constant, to_str(h, c->consts.items[constant])->chars);
  return offset + 3;
}


int print_bc(hbs_State* h, Chunk *c, int idx) {
  printf("%04d ", idx);
  if (idx > 0 && c->lines[idx] == c->lines[idx - 1]) {
    printf("   | ");
  } else {
    printf("%4d ", c->lines[idx]);
  }

  u8 bc = c->code[idx];
  switch (bc) {
    case bc_pop: return simple_bc("pop", idx);
    case bc_close_upval: return simple_bc("close_upval", idx);
    case bc_def_global: return const_bc(h, "def_global", c, idx);
    case bc_get_global: return const_bc(h, "get_global", c, idx);
    case bc_set_global: return const_bc(h, "set_global", c, idx);
    case bc_get_local: return byte_bc("get_local", c, idx);
    case bc_set_local: return byte_bc("set_local", c, idx);
    case bc_get_upval: return byte_bc("get_upval", c, idx);
    case bc_set_upval: return byte_bc("set_upval", c, idx);
    case bc_get_prop: return const_bc(h, "get_prop", c, idx);
    case bc_set_prop: return const_bc(h, "set_prop", c, idx);
    case bc_init_prop: return const_bc(h, "init_prop", c, idx);
    case bc_get_subscript: return simple_bc("get_subscript", idx);
    case bc_set_subscript: return simple_bc("set_subscript", idx);
    case bc_destruct_array: {
      u8 element = c->code[idx + 1];
      printf("%-16s %4d\n", "destruct_array", element);
      return idx + 2;
    }
    case bc_push_subscript: return simple_bc("push_subscript", idx);
    case bc_get_static: return const_bc(h, "get_static", c, idx);
    case bc_const: return const_bc(h, "const", c, idx);
    case bc_true: return simple_bc("true", idx);
    case bc_false: return simple_bc("false", idx);
    case bc_null: return simple_bc("null", idx);
    case bc_array: return simple_bc("array", idx);
    case bc_array_item: return simple_bc("array_item", idx);
    case bc_add: return simple_bc("add", idx);
    case bc_sub: return simple_bc("sub", idx);
    case bc_mul: return simple_bc("mul", idx);
    case bc_div: return simple_bc("div", idx);
    case bc_mod: return simple_bc("mod", idx);
    case bc_eql: return simple_bc("eql", idx);
    case bc_neql: return simple_bc("neql", idx);
    case bc_gt: return simple_bc("gt", idx);
    case bc_gte: return simple_bc("gte", idx);
    case bc_lt: return simple_bc("lt", idx);
    case bc_lte: return simple_bc("lte", idx);
    case bc_cat: return simple_bc("cat", idx);
    case bc_is: return simple_bc("is", idx);
    case bc_neg: return simple_bc("neg", idx);
    case bc_not: return simple_bc("not", idx);
    case bc_jmp: return jmp_bc("jmp", 1, c, idx);
    case bc_ineq_jmp: return jmp_bc("ineq_jmp", 1, c, idx);
    case bc_false_jmp: return jmp_bc("false_jmp", 1, c, idx);
    case bc_loop: return jmp_bc("loop", -1, c, idx);
    case bc_call: return byte_bc("call", c, idx);
    case bc_invoke: return invoke_bc(h, "invoke", c, idx);
    case bc_closure: {
      idx++;
      u8 constant = c->code[idx++];
      printf(
        "%-16s %4d %s\n",
        "closure", constant, to_str(h, c->consts.items[constant])->chars);

      GcFn* fn = as_fn(c->consts.items[constant]);
      for (int j = 0; j < fn->upvalc; j++) {
        int is_local = c->code[idx++];
        int uv_idx = c->code[idx++];
        printf(
          "%04d      |                     %s %d\n",
          idx - 2, is_local ? "local" : "upvalue", uv_idx);
      }

      return idx;
    }
    case bc_ret: return simple_bc("ret", idx);
    case bc_struct: return const_bc(h, "struct", c, idx);
    case bc_member: return const_bc(h, "member", c, idx);
    case bc_method: return const_bc(h, "method", c, idx);
    case bc_enum: {
      idx++;
      u8 name = c->code[idx++];
      u8 count = c->code[idx++];
      printf(
        "%-16s %4d '%s' %4d\n",
        "enum", name, as_cstr(c->consts.items[name]), count);

      for (int i = 0; i < count; i++) {
        u8 name = c->code[idx++];
        printf(
          "%04d      |                   %s\n",
          idx - 2, as_cstr(c->consts.items[name]));
      }
      return idx;
    }
    case bc_def_static: return const_bc(h, "def_static", c, idx);
    case bc_inst: return simple_bc("inst", idx);
    case bc_break: return simple_bc("break", idx);
    default: {
      printf("Unknown bytecode %d\n", bc);
      return idx + 1;
    }
  }
}

void print_chunk(hbs_State* h, Chunk* c, const char* name) {
  printf("== %s ==\n", name);

  for (int idx = 0; idx < c->len;) {
    idx = print_bc(h, c, idx);
  }
}
