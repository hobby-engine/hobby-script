#include "table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "state.h"
#include "mem.h"
#include "obj.h"
#include "val.h"

#define table_max_load 0.75

void init_table(Table* table) {
  table->count = 0;
  table->cap = 0;
  table->items = NULL;
}

void free_table(hby_State* h, Table* table) {
  release_arr(h, TableItem, table->items, table->cap);
  init_table(table);
}

static TableItem* find_item(TableItem* items, int cap, GcStr* k) {
  u32 idx = k->hash & (cap - 1);
  TableItem* tombstone = NULL;
  while (true) {
    TableItem* item = &items[idx];
    if (item->k == NULL) {
      if (is_null(item->v)) {
        // This entry is empty. Return result.
        return tombstone != NULL ? tombstone : item;
      } else {
        if (tombstone == NULL) {
          tombstone = item;
        }
      }
    } else if (item->k == k) {
      return item;
    }

    idx = (idx + 1) & (cap - 1);
  }
}

static void adjust_cap(hby_State* h, Table* table, int cap) {
  TableItem* items = allocate(h, TableItem, cap);
  for (int i = 0; i < cap; i++) {
    items[i].k = NULL;
    items[i].v = create_null();
  }

  table->count = 0;
  for (int i = 0; i < table->cap; i++) {
    TableItem* item = &table->items[i];
    if (item->k == NULL) {
      continue;
    }

    TableItem* dst = find_item(items, cap, item->k);
    dst->k = item->k;
    dst->v = item->v;
    table->count++;
  }

  release_arr(h, TableItem, table->items, table->cap);
  table->items = items;
  table->cap = cap;
}

bool get_table(Table* table, GcStr* k, Val* out_v) {
  if (table->count == 0) {
    return false;
  }

  TableItem* item = find_item(table->items, table->cap, k);
  if (item->k == NULL) {
    return false;
  }

  *out_v = item->v;
  return true;
}

bool set_table(hby_State* h, Table* table, GcStr* k, Val v) {
  if (table->count + 1 > table->cap * table_max_load) {
    int cap = grow_cap(table->cap);
    adjust_cap(h, table, cap);
  }

  TableItem* item = find_item(table->items, table->cap, k);
  bool is_new = item->k == NULL;
  if (is_new && is_null(item->v)) {
    table->count++;
  }

  item->k = k;
  item->v = v;
  return is_new;
}

bool rem_table(Table* table, GcStr* k) {
  if (table->count == 0) {
    return false;
  }

  TableItem* item = find_item(table->items, table->cap, k);
  if (item->k == NULL) {
    return false;
  }

  item->k = NULL;
  item->v = create_bool(true);
  return true;
}

void copy_table(struct hby_State* h, Table* src, Table* dst) {
  for (int i = 0; i < src->cap; i++) {
    TableItem* item = &src->items[i];
    if (item->k != NULL) {
      set_table(h, dst, item->k, item->v);
    }
  }
}

GcStr* find_str_table(Table* table, const char* chars, int len, u32 hash) {
  if (table->count == 0) {
    return NULL;
  }

  u32 idx = hash & (table->cap - 1);

  while (true) {
    TableItem* item = &table->items[idx];
    if (item->k == NULL) {
      if (is_null(item->v)) {
        // We reached the end.
        return NULL;
      }
    } else if (item->k->len == len && item->k->hash == hash 
        && memcmp(item->k->chars, chars, len) == 0) {
      return item->k;
    }

    idx = (idx + 1) & (table->cap - 1);
  }
}

void mark_table(hby_State* h, Table* table) {
  for (int i = 0; i < table->cap; i++) {
    TableItem* item = &table->items[i];
    mark_obj(h, (GcObj*)item->k);
    mark_val(h, item->v);
  }
}

void rem_black_table(struct hby_State* h, Table* table) {
  for (int i = 0; i < table->cap; i++) {
    TableItem* item = &table->items[i];
    if (item->k != NULL && !item->k->obj.marked) {
      rem_table(table, item->k);
    }
  }
}
