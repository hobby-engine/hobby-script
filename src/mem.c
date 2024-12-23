#include <stdlib.h>

#include "mem.h"
#include "chunk.h"
#include "parser.h"
#include "tostr.h"
#include "val.h"
#include "arr.h"
#include "map.h"
#include "state.h"
#include "vm.h"
#include "obj.h"

# include <stdio.h>
#ifdef hby_log_gc
# include <stdio.h>
# include "debug.h"
#endif

#define gc_grow_factor 2

void* reallocate(hby_State* h, void* ptr, size_t plen, size_t len) {
  h->gc.alloced += len - plen;
  if (len > plen) {
#ifdef hby_stress_gc
    gc(h);
#else
    if (h->gc.alloced > h->gc.next_gc) {
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

static void free_obj(hby_State* h, GcObj* obj) {
#ifdef hby_log_gc
  printf("%p free type %d\n", (void*)obj, obj->type);
#endif

  switch (obj->type) {
    case obj_method: {
      release(h, GcMethod, obj);
      break;
    }
    case obj_struct: {
      GcStruct* s = (GcStruct*)obj;
      free_table(h, &s->staticm);
      free_table(h, &s->methods);
      free_table(h, &s->members);
      release(h, GcStruct, obj);
      break;
    }
    case obj_inst: {
      GcInst* inst = (GcInst*)obj;
      free_table(h, &inst->fields);
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
      free_table(h, &_enum->vals);
      release(h, GcEnum, obj);
      break;
    }
    case obj_arr: {
      GcArr* arr = (GcArr*)obj;
      free_varr(h, &arr->varr);
      release(h, GcArr, obj);
      break;
    }
    case obj_map: {
      GcMap* map = (GcMap*)obj;
      release_arr(h, MapItem, map->items, map->item_cap);
      release(h, GcMap, obj);
      break;
    }
    case obj_str: {
      GcStr* str = (GcStr*)obj;
      release_arr(h, char, str->chars, str->len + 1);
      release(h, GcStr, obj);
      break;
    }
    case obj_udata: {
      GcUData* udata = (GcUData*)obj;
      reallocate(h, udata->data, udata->size, 0);
      release(h, GcUData, obj);
      break;
    }
  }
}

void mark_obj(hby_State* h, GcObj* obj) {
  if (obj == NULL || obj->marked) {
    return;
  }
#ifdef hby_log_gc
  printf("%p mark %d\n", (void*)obj, obj->type);
#endif
  obj->marked = true;

  if (h->gc.gray_cap < h->gc.grayc + 1) {
    h->gc.gray_cap = grow_cap(h->gc.gray_cap);
    h->gc.gray_stack = (GcObj**)realloc(
      h->gc.gray_stack, sizeof(GcObj*) * h->gc.gray_cap);
    if (h->gc.gray_stack == NULL) {
      exit(1);
    }
  }

  h->gc.gray_stack[h->gc.grayc++] = obj;
}

void mark_val(hby_State* h, Val val) {
  if (is_obj(val)) {
    mark_obj(h, as_obj(val));
  }
}

static void mark_arr(hby_State* h, VArr* arr) {
  for (int i = 0; i < arr->len; i++) {
    mark_val(h, arr->items[i]);
  }
}

static void blacken_obj(hby_State* h, GcObj* obj) {
#ifdef hby_log_gc
  printf("%p blacken %d\n", (void*)obj, obj->type);
#endif

  switch (obj->type) {
    case obj_method: {
      GcMethod* method = (GcMethod*)obj;
      mark_val(h, method->owner);
      mark_obj(h, (GcObj*)method->fn.hby);
      break;
    }
    case obj_struct: {
      GcStruct* s = (GcStruct*)obj;
      mark_obj(h, (GcObj*)s->name);
      mark_table(h, &s->staticm);
      mark_table(h, &s->methods);
      mark_table(h, &s->members);
      break;
    }
    case obj_inst: {
      GcInst* inst = (GcInst*)obj;
      mark_obj(h, (GcObj*)inst->_struct);
      mark_table(h, &inst->fields);
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
      mark_table(h, &_enum->vals);
      break;
    }
    case obj_arr:
      mark_arr(h, &((GcArr*)obj)->varr);
      break;
    case obj_map:
      mark_map(h, (GcMap*)obj);
      break;
    case obj_upval: {
      GcUpval* up = (GcUpval*)obj;
      mark_val(h, up->closed);
      break;
    }
    case obj_udata: {
      GcUData* udata = (GcUData*)obj;
      mark_obj(h, (GcObj*)udata->metastruct);
      mark_obj(h, (GcObj*)udata->finalizer);
      break;
    }
    case obj_str:
      break;
  }
}

static void mark_roots(hby_State* h) {
  for (Val* slot = h->stack; slot < h->top; slot++) {
    mark_val(h, *slot);
  }

  for (CallFrame* frame = h->frame; frame > h->frame_stack; frame--) {
    if (frame->type == call_type_c) {
      mark_obj(h, (GcObj*)frame->fn.c);
    } else {
      mark_obj(h, (GcObj*)frame->fn.hby);
    }
  }

  for (GcUpval* upval = h->open_upvals; upval != NULL; upval = upval->next) {
    mark_obj(h, (GcObj*)upval);
  }

  mark_table(h, &h->globals);
  mark_table(h, &h->global_consts);
  mark_table(h, &h->files);
  mark_obj(h, (GcObj*)h->args);
  mark_obj(h, (GcObj*)h->registry);
  mark_compiler_roots(h->parser);
}

static void trace_refs(hby_State* h) {
  while (h->gc.grayc > 0) {
    GcObj* obj = h->gc.gray_stack[--h->gc.grayc];
    blacken_obj(h, obj);
  }
}

static void sweep(hby_State* h, GcObj** list) {
  GcObj* prev = NULL;
  GcObj* obj = *list;

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
        *list = obj;
      }

      free_obj(h, unreached);
    }
  }
}

static void finalize_udata(hby_State* h, bool need_unmarked) {
  GcObj* obj = h->gc.udata;
  while (obj != NULL) {
    GcUData* udata = (GcUData*)obj;
    if ((!need_unmarked || !obj->marked) && udata->finalizer != NULL) {
      push(h, create_obj(udata->finalizer));
      push(h, create_obj(udata));
      hby_call(h, 1);
    }
    obj = obj->next;
  }
}

void gc(hby_State* h) {
  if (!h->gc.can_gc) {
    return;
  }

#ifdef hby_log_gc
  printf("-- GC BEGIN --\n");
  size_t before = h->gc.alloced;
#endif

  mark_roots(h);
  trace_refs(h);
  rem_black_table(h, &h->strs);

  finalize_udata(h, true);

  sweep(h, &h->gc.udata);
  sweep(h, &h->gc.objs);

  h->gc.next_gc = h->gc.alloced * gc_grow_factor;

#ifdef hby_log_gc
  printf(
    "COLLECTED %zu BYTES (FROM %zu TO %zu) NEXT AT %zu\n",
    before - h->gc.alloced, before, h->gc.alloced, h->gc.next_gc);
  printf("-- GC END --\n");
#endif
}

static void free_linked_list(hby_State* h, GcObj* head) {
  GcObj* obj = head;
  while (obj != NULL) {
    GcObj* next = obj->next;
    free_obj(h, obj);
    obj = next;
  }
}

void free_objs(hby_State* h) {
  finalize_udata(h, false);
  free_linked_list(h, h->gc.udata);
  free_linked_list(h, h->gc.objs);
  free(h->gc.gray_stack);
}
