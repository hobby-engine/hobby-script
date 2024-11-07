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
  table->itemc = 0;
  table->item_cap = 0;
  table->items = NULL;
}

void free_table(hby_State* h, Table* table) {
  release_arr(h, TableItem, table->items, table->item_cap);
  init_table(table);
}

static TableItem* find_item(TableItem* items, int cap, GcStr* k) {
  uint32_t index = k->hash & (cap - 1);
  TableItem* tombstone = NULL;
  while (true) {
    TableItem* item = &items[index];
    if (item->key == NULL) {
      if (is_null(item->val)) {
        // This entry is empty. Return result.
        return tombstone != NULL ? tombstone : item;
      } else {
        if (tombstone == NULL) {
          tombstone = item;
        }
      }
    } else if (item->key == k) {
      return item;
    }

    index = (index + 1) & (cap - 1);
  }
}

static void adjust_cap(hby_State* h, Table* table, int cap) {
  TableItem* items = allocate(h, TableItem, cap);
  for (int i = 0; i < cap; i++) {
    items[i].key = NULL;
    items[i].val = create_null();
  }

  table->itemc = 0;
  for (int i = 0; i < table->item_cap; i++) {
    TableItem* item = &table->items[i];
    if (item->key == NULL) {
      continue;
    }

    TableItem* dst = find_item(items, cap, item->key);
    dst->key = item->key;
    dst->val = item->val;
    table->itemc++;
  }

  release_arr(h, TableItem, table->items, table->item_cap);
  table->items = items;
  table->item_cap = cap;
}

bool get_table(Table* table, GcStr* k, Val* out_v) {
  if (table->itemc == 0) {
    return false;
  }

  TableItem* item = find_item(table->items, table->item_cap, k);
  if (item->key == NULL) {
    return false;
  }

  *out_v = item->val;
  return true;
}

bool set_table(hby_State* h, Table* table, GcStr* k, Val v) {
  if (table->itemc + 1 > table->item_cap * table_max_load) {
    int cap = grow_cap(table->item_cap);
    adjust_cap(h, table, cap);
  }

  TableItem* item = find_item(table->items, table->item_cap, k);
  bool is_new = item->key == NULL;
  if (is_new && is_null(item->val)) {
    table->itemc++;
  }

  item->key = k;
  item->val = v;
  return is_new;
}

bool rem_table(Table* table, GcStr* k) {
  if (table->itemc == 0) {
    return false;
  }

  TableItem* item = find_item(table->items, table->item_cap, k);
  if (item->key == NULL) {
    return false;
  }

  item->key = NULL;
  item->val = create_bool(true);
  return true;
}

void copy_table(hby_State* h, Table* src, Table* dst) {
  for (int i = 0; i < src->item_cap; i++) {
    TableItem* item = &src->items[i];
    if (item->key != NULL) {
      set_table(h, dst, item->key, item->val);
    }
  }
}

GcStr* find_str_table(Table* table, const char* chars, int len, uint32_t hash) {
  if (table->itemc == 0) {
    return NULL;
  }

  uint32_t index = hash & (table->item_cap - 1);

  while (true) {
    TableItem* item = &table->items[index];
    if (item->key == NULL) {
      if (is_null(item->val)) {
        // We reached the end.
        return NULL;
      }
    } else if (item->key->len == len && item->key->hash == hash 
        && memcmp(item->key->chars, chars, len) == 0) {
      return item->key;
    }

    index = (index + 1) & (table->item_cap - 1);
  }
}

void mark_table(hby_State* h, Table* table) {
  for (int i = 0; i < table->item_cap; i++) {
    TableItem* item = &table->items[i];
    mark_obj(h, (GcObj*)item->key);
    mark_val(h, item->val);
  }
}

void rem_black_table(hby_State* h, Table* table) {
  for (int i = 0; i < table->item_cap; i++) {
    TableItem* item = &table->items[i];
    if (item->key != NULL && !item->key->obj.marked) {
      rem_table(table, item->key);
    }
  }
}
