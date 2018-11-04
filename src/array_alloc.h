// This file is auto generated by icy-table.
#include "icydb.h"
typedef struct _array_alloc{
  char ** column_names;
  char ** column_types;
  size_t count;
  const bool is_multi_table;
  const int column_count;
  int (*cmp) (const u64 * k1, const u64 * k2);
  const size_t sizes[2];

  u64 * key;
  array_index * value;
  icy_mem * key_area;
  icy_mem * value_area;
}array_alloc;

array_alloc * array_alloc_create(const char * optional_name);
void array_alloc_set(array_alloc * table, u64 key, array_index value);
void array_alloc_insert(array_alloc * table, u64 * key, array_index * value, size_t count);
void array_alloc_lookup(array_alloc * table, u64 * keys, size_t * out_indexes, size_t count);
void array_alloc_remove(array_alloc * table, u64 * keys, size_t key_count);
void array_alloc_clear(array_alloc * table);
void array_alloc_unset(array_alloc * table, u64 key);
bool array_alloc_try_get(array_alloc * table, u64 * key, array_index * value);
void array_alloc_print(array_alloc * table);
size_t array_alloc_iter(array_alloc * table, u64 * keys, size_t keycnt, u64 * optional_keys_out, size_t * indexes, size_t cnt, size_t * iterator);
