#include "debug.h"

#include <stdio.h>
#include "tostr.h"
#include "obj.h"
#include "chunk.h"
#include "val.h"

static int simple_bc(const char* name, int index) {
  printf("%s\n", name);
  return index + 1;
}

static int const_bc(hby_State* h, const char* name, Chunk* c, int index) {
  uint8_t constant = c->code[index + 1];
  printf(
    "%-16s %4d '%s'\n", 
    name, constant, to_str(h, c->consts.items[constant])->chars);
  return index + 2;
}

static int byte_bc(const char* name, Chunk* c, int index) {
  uint8_t slot = c->code[index + 1];
  printf("%-16s %4d\n", name, slot);
  return index + 2;
}

static int jmp_bc(const char* name, int sign, Chunk* c, int index) {
  uint16_t jmp = (uint16_t)(c->code[index + 1] << 8);
  jmp |= c->code[index + 2];
  printf("%-16s %4d -> %d\n", name, index, index + 3 + sign * jmp);
  return index + 3;
}

static int invoke_bc(hby_State* h, const char* name, Chunk* c, int offset) {
  uint8_t constant = c->code[offset + 1];
  uint8_t argc = c->code[offset + 2];
  printf(
    "%-16s (%d args) %4d '%s'\n",
    name, argc, constant, to_str(h, c->consts.items[constant])->chars);
  return offset + 3;
}


int print_bc(hby_State* h, Chunk *c, int index) {
  printf("%04d ", index);
  if (index > 0 && c->lines[index] == c->lines[index - 1]) {
    printf("   | ");
  } else {
    printf("%4d ", c->lines[index]);
  }

  uint8_t bc = c->code[index];
  switch (bc) {
    case bc_pop: return simple_bc("pop", index);
    case bc_close_upval: return simple_bc("close_upval", index);
    case bc_get_global: return const_bc(h, "get_global", c, index);
    case bc_get_local: return byte_bc("get_local", c, index);
    case bc_set_local: return byte_bc("set_local", c, index);
    case bc_get_upval: return byte_bc("get_upval", c, index);
    case bc_set_upval: return byte_bc("set_upval", c, index);
    case bc_get_prop: return const_bc(h, "get_prop", c, index);
    case bc_set_prop: return const_bc(h, "set_prop", c, index);
    case bc_init_prop: return const_bc(h, "init_prop", c, index);
    case bc_get_subscript: return simple_bc("get_subscript", index);
    case bc_set_subscript: return simple_bc("set_subscript", index);
    case bc_destruct_array: {
      uint8_t element = c->code[index + 1];
      printf("%-16s %4d\n", "destruct_array", element);
      return index + 2;
    }
    case bc_push_subscript: return simple_bc("push_subscript", index);
    case bc_get_static: return const_bc(h, "get_static", c, index);
    case bc_const: return const_bc(h, "const", c, index);
    case bc_true: return simple_bc("true", index);
    case bc_false: return simple_bc("false", index);
    case bc_null: return simple_bc("null", index);
    case bc_array: return simple_bc("array", index);
    case bc_array_item: return simple_bc("array_item", index);
    case bc_map: return simple_bc("map", index);
    case bc_map_item: return simple_bc("map_item", index);
    case bc_add: return simple_bc("add", index);
    case bc_sub: return simple_bc("sub", index);
    case bc_mul: return simple_bc("mul", index);
    case bc_div: return simple_bc("div", index);
    case bc_mod: return simple_bc("mod", index);
    case bc_eql: return simple_bc("eql", index);
    case bc_neql: return simple_bc("neql", index);
    case bc_gt: return simple_bc("gt", index);
    case bc_gte: return simple_bc("gte", index);
    case bc_lt: return simple_bc("lt", index);
    case bc_lte: return simple_bc("lte", index);
    case bc_cat: return simple_bc("cat", index);
    case bc_is: return simple_bc("is", index);
    case bc_neg: return simple_bc("neg", index);
    case bc_not: return simple_bc("not", index);
    case bc_jmp: return jmp_bc("jmp", 1, c, index);
    case bc_ineq_jmp: return jmp_bc("ineq_jmp", 1, c, index);
    case bc_false_jmp: return jmp_bc("false_jmp", 1, c, index);
    case bc_loop: return jmp_bc("loop", -1, c, index);
    case bc_call: return byte_bc("call", c, index);
    case bc_invoke: return invoke_bc(h, "invoke", c, index);
    case bc_closure: {
      index++;
      uint8_t constant = c->code[index++];
      printf(
        "%-16s %4d %s\n",
        "closure", constant, to_str(h, c->consts.items[constant])->chars);

      GcFn* fn = as_fn(c->consts.items[constant]);
      for (int j = 0; j < fn->upvalc; j++) {
        int is_local = c->code[index++];
        int uv_index = c->code[index++];
        printf(
          "%04d      |                     %s %d\n",
          index - 2, is_local ? "local" : "upvalue", uv_index);
      }

      return index;
    }
    case bc_ret: return simple_bc("ret", index);
    case bc_struct: return const_bc(h, "struct", c, index);
    case bc_member: return const_bc(h, "member", c, index);
    case bc_method: return const_bc(h, "method", c, index);
    case bc_enum: {
      index++;
      uint8_t name = c->code[index++];
      uint8_t count = c->code[index++];
      printf(
        "%-16s %4d '%s' %4d\n",
        "enum", name, as_cstr(c->consts.items[name]), count);

      for (int i = 0; i < count; i++) {
        uint8_t name = c->code[index++];
        printf(
          "%04d      |                   %s\n",
          index - 2, as_cstr(c->consts.items[name]));
      }
      return index;
    }
    case bc_def_static: return const_bc(h, "def_static", c, index);
    case bc_inst: return simple_bc("inst", index);
    case bc_err: return const_bc(h, "err", c, index);
    case bc_break: return simple_bc("break", index);
  }

  printf("Unknown bytecode %d\n", bc);
  return index + 1;
}

void print_chunk(hby_State* h, Chunk* c, const char* name) {
  printf("== %s ==\n", name);

  for (int index = 0; index < c->len;) {
    index = print_bc(h, c, index);
  }
}
