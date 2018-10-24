// This file is auto generated by icy-table.
#include "icydb.h"
typedef struct _interned_strings{
  char ** column_names;
  char ** column_types;
  size_t count;
  const bool is_multi_table;
  const int column_count;
  int (*cmp) (const u32 * k1, const u32 * k2);
  const size_t sizes[2];

  u32 * key;
  string_table_indexes * str;
  icy_mem * key_area;
  icy_mem * str_area;
}interned_strings;

interned_strings * interned_strings_create(const char * optional_name);
void interned_strings_set(interned_strings * table, u32 key, string_table_indexes str);
void interned_strings_insert(interned_strings * table, u32 * key, string_table_indexes * str, size_t count);
void interned_strings_lookup(interned_strings * table, u32 * keys, size_t * out_indexes, size_t count);
void interned_strings_remove(interned_strings * table, u32 * keys, size_t key_count);
void interned_strings_clear(interned_strings * table);
void interned_strings_unset(interned_strings * table, u32 key);
bool interned_strings_try_get(interned_strings * table, u32 * key, string_table_indexes * str);
void interned_strings_print(interned_strings * table);
size_t interned_strings_iter(interned_strings * table, u32 * keys, size_t keycnt, u32 * optional_keys_out, size_t * indexes, size_t cnt, size_t * iterator);