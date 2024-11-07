#ifndef __HBY_TABLE_H
#define __HBY_TABLE_H

#include "common.h"
#include "val.h"

struct hby_State;

typedef struct {
  GcStr* key;
  Val val;
} TableItem;

typedef struct {
  int itemc;
  int item_cap;
  TableItem* items;
} Table;

void init_table(Table* table);
void free_table(struct hby_State* h, Table* table);
bool get_table(Table* table, GcStr* k, Val* out_v);
bool set_table(struct hby_State* h, Table* table, GcStr* k, Val v);
bool rem_table(Table* table, GcStr* k);
void copy_table(struct hby_State* h, Table* src, Table* dst);
GcStr* find_str_table(Table* table, const char* chars, int len, uint32_t hash);
void mark_table(struct hby_State* h, Table* table);
void rem_black_table(struct hby_State* h, Table* table);

#endif // __HBY_TABLE_H
