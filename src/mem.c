#include <stdlib.h>

#include "mem.h"
#include "chunk.h"
#include "compiler.h"
#include "tostr.h"
#include "val.h"
#include "arr.h"
#include "state.h"
#include "obj.h"

# include <stdio.h>
#ifdef hbs_log_gc
# include <stdio.h>
# include "debug.h"
#endif

#define gc_grow_factor 2

void* reallocate(hbs_State* h, void* ptr, size_t plen, size_t len) {
  h->alloced += len - plen;
  if (len > plen) {
#ifdef hbs_stress_gc
    gc(h);
#else
    if (h->alloced > h->next_gc) {
      gc(h);
    }
#endif
  }

  if (len == 0) {
    free(ptr);
    return NULL;
  }

  void* res = realloc(ptr, len);
  if (res == NULL) {
    exit(1);
  }

  return res;
}

static void free_obj(hbs_State* h, GcObj* obj) {
#ifdef hbs_log_gc
  printf("%p free type %d\n", (void*)obj, obj->type);
#endif

  switch (obj->type) {
    case obj_method: {
      release(h, GcMethod, obj);
      break;
    }
    case obj_struct: {
      GcStruct* s = (GcStruct*)obj;
      free_map(h, &s->staticm);
      free_map(h, &s->methods);
      free_map(h, &s->members);
      release(h, GcStruct, obj);
      break;
    }
    case obj_inst: {
      GcInst* inst = (GcInst*)obj;
      free_map(h, &inst->fields);
      release(h, GcInst, obj);
      break;
    }
    case obj_upval:
      release(h, GcUpval, obj);
      break;
    case obj_closure: {
      GcClosure* closure = (GcClosure*)obj;
      release_arr(h, GcUpval*, closure->upvals, closure->upvalc);
      release(h, GcClosure, obj);
      break;
    }
    case obj_c_fn:
      release(h, GcCFn, obj);
      break;
    case obj_fn: {
      GcFn* fn = (GcFn*)obj;
      free_chunk(h, &fn->chunk);
      release(h, GcFn, obj);
      break;
    }
    case obj_enum: {
      GcEnum* _enum = (GcEnum*)obj;
      free_map(h, &_enum->vals);
      release(h, GcEnum, obj);
      break;
    }
    case obj_arr: {
      GcArr* arr = (GcArr*)obj;
      free_varr(h, &arr->varr);
      release(h, GcArr, obj);
      break;
    }
    case obj_str: {
      GcStr* str = (GcStr*)obj;
      release_arr(h, char, str->chars, str->len + 1);
      release(h, GcStr, obj);
      break;
    }
  }
}

void mark_obj(hbs_State* h, GcObj* obj) {
  if (obj == NULL || obj->marked) {
    return;
  }
#ifdef hbs_log_gc
  printf("%p mark %d\n", (void*)obj, obj->type);
#endif
  obj->marked = true;

  if (h->gray_cap < h->grayc + 1) {
    h->gray_cap = grow_cap(h->gray_cap);
    h->gray_stack = (GcObj**)realloc(
      h->gray_stack, sizeof(GcObj*) * h->gray_cap);
    if (h->gray_stack == NULL) {
      exit(1);
    }
  }

  h->gray_stack[h->grayc++] = obj;
}

void mark_val(hbs_State* h, Val val) {
  if (is_obj(val)) {
    mark_obj(h, as_obj(val));
  }
}

static void mark_arr(hbs_State* h, VArr* arr) {
  for (int i = 0; i < arr->len; i++) {
    mark_val(h, arr->items[i]);
  }
}

static void blacken_obj(hbs_State* h, GcObj* obj) {
#ifdef hbs_log_gc
  printf("%p blacken %d\n", (void*)obj, obj->type);
#endif

  switch (obj->type) {
    case obj_method: {
      GcMethod* method = (GcMethod*)obj;
      mark_val(h, method->owner);
      mark_obj(h, (GcObj*)method->fn.hbs);
      break;
    }
    case obj_struct: {
      GcStruct* s = (GcStruct*)obj;
      mark_obj(h, (GcObj*)s->name);
      mark_map(h, &s->staticm);
      mark_map(h, &s->methods);
      mark_map(h, &s->members);
      break;
    }
    case obj_inst: {
      GcInst* inst = (GcInst*)obj;
      mark_obj(h, (GcObj*)inst->_struct);
      mark_map(h, &inst->fields);
      break;
    }
    case obj_closure: {
      GcClosure* closure = (GcClosure*)obj;
      mark_obj(h, (GcObj*)closure->fn);
      for (int i = 0; i < closure->upvalc; i++) {
        mark_obj(h, (GcObj*)closure->upvals[i]);
      }
      break;
    }
    case obj_fn: {
      GcFn* fn = (GcFn*)obj;
      mark_obj(h, (GcObj*)fn->name);
      mark_obj(h, (GcObj*)fn->path);
      mark_arr(h, &fn->chunk.consts);
      break;
    }
    case obj_c_fn:
      mark_obj(h, (GcObj*)((GcCFn*)obj)->name);
      break;
    case obj_enum: {
      GcEnum* _enum = (GcEnum*)obj;
      mark_obj(h, (GcObj*)_enum->name);
      mark_map(h, &_enum->vals);
      break;
    }
    case obj_arr:
      mark_arr(h, &((GcArr*)obj)->varr);
      break;
    case obj_upval: {
      GcUpval* up = (GcUpval*)obj;
      mark_val(h, up->closed);
      break;
    }
    case obj_str:
      break;
  }
}

static void mark_roots(hbs_State* h) {
  for (Val* slot = h->stack; slot < h->top; slot++) {
    mark_val(h, *slot);
  }

  for (CallFrame* frame = h->frame; frame > h->frame_stack; frame--) {
    if (frame->type == call_type_c) {
      mark_obj(h, (GcObj*)frame->fn.c);
    } else {
      mark_obj(h, (GcObj*)frame->fn.hbs);
    }
  }

  for (GcUpval* upval = h->open_upvals; upval != NULL; upval = upval->next) {
    mark_obj(h, (GcObj*)upval);
  }

  mark_map(h, &h->globals);
  mark_map(h, &h->files);
  mark_compiler_roots(h->parser);
}

static void trace_refs(hbs_State* h) {
  while (h->grayc > 0) {
    GcObj* obj = h->gray_stack[--h->grayc];
    blacken_obj(h, obj);
  }
}

static void sweep(hbs_State* h) {
  GcObj* prev = NULL;
  GcObj* obj = h->objs;

  while (obj != NULL) {
    if (obj->marked) {
      obj->marked = false;
      prev = obj;
      obj = obj->next;
    } else {
      GcObj* unreached = obj;
      obj = obj->next;
      if (prev != NULL) {
        prev->next = obj;
      } else {
        h->objs = obj;
      }

      free_obj(h, unreached);
    }
  }
}

void gc(hbs_State* h) {
  if (!h->can_gc) {
    return;
  }

#ifdef hbs_log_gc
  printf("-- GC BEGIN --\n");
  size_t before = h->alloced;
#endif

  mark_roots(h);
  trace_refs(h);
  rem_black_map(h, &h->strs);
  sweep(h);

  h->next_gc = h->alloced * gc_grow_factor;

#ifdef hbs_log_gc
  printf(
    "COLLECTED %zu BYTES (FROM %zu TO %zu) NEXT AT %zu\n",
    before - h->alloced, before, h->alloced, h->next_gc);
  printf("-- GC END --\n");
#endif
}

void free_objs(hbs_State* h) {
  GcObj* obj = h->objs;
  while (obj != NULL) {
    GcObj* next = obj->next;
    free_obj(h, obj);
    obj = next;
  }

  free(h->gray_stack);
}
