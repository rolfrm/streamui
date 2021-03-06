// This file is auto generated by icy-table.
#ifndef TABLE_COMPILER_INDEX
#define TABLE_COMPILER_INDEX
#define array_element_size(array) sizeof(array[0])
#define array_count(array) (sizeof(array)/array_element_size(array))
#include "icydb.h"
#include <stdlib.h>
#endif


id_to_id * id_to_id_create(const char * optional_name){
  static const char * const column_names[] = {(char *)"key", (char *)"value"};
  static const char * const column_types[] = {"object_id", "object_id"};
  id_to_id * instance = calloc(sizeof(id_to_id), 1);
  instance->column_names = (char **)column_names;
  instance->column_types = (char **)column_types;
  
  icy_table_init((icy_table * )instance, optional_name, 2, (unsigned int[]){sizeof(object_id), sizeof(object_id)}, (char *[]){(char *)"key", (char *)"value"});
  
  return instance;
}

void id_to_id_insert(id_to_id * table, object_id * key, object_id * value, size_t count){
  void * array[] = {(void* )key, (void* )value};
  icy_table_inserts((icy_table *) table, array, count);
}

void id_to_id_set(id_to_id * table, object_id key, object_id value){
  void * array[] = {(void* )&key, (void* )&value};
  icy_table_inserts((icy_table *) table, array, 1);
}

void id_to_id_lookup(id_to_id * table, object_id * keys, size_t * out_indexes, size_t count){
  icy_table_finds((icy_table *) table, keys, out_indexes, count);
}

void id_to_id_remove(id_to_id * table, object_id * keys, size_t key_count){
  size_t indexes[key_count];
  size_t index = 0;
  size_t cnt = 0;
  while(0 < (cnt = icy_table_iter((icy_table *) table, keys, key_count, NULL, indexes, array_count(indexes), &index))){
    icy_table_remove_indexes((icy_table *) table, indexes, cnt);
    index = 0;
  }
}

void id_to_id_clear(id_to_id * table){
  icy_table_clear((icy_table *) table);
}

void id_to_id_unset(id_to_id * table, object_id key){
  id_to_id_remove(table, &key, 1);
}

bool id_to_id_try_get(id_to_id * table, object_id * key, object_id * value){
  void * array[] = {(void* )key, (void* )value};
  void * column_pointers[] = {(void *)table->key, (void *)table->value};
  size_t __index = 0;
  icy_table_finds((icy_table *) table, array[0], &__index, 1);
  if(__index == 0) return false;
  unsigned int sizes[] = {sizeof(object_id), sizeof(object_id)};
  for(int i = 1; i < 2; i++){
    if(array[i] != NULL)
      memcpy(array[i], column_pointers[i] + __index * sizes[i], sizes[i]); 
  }
  return true;
}

void id_to_id_print(id_to_id * table){
  icy_table_print((icy_table *) table);
}

size_t id_to_id_iter(id_to_id * table, object_id * keys, size_t keycnt, object_id * optional_keys_out, size_t * indexes, size_t cnt, size_t * iterator){
  return icy_table_iter((icy_table *) table, keys, keycnt, optional_keys_out, indexes, cnt, iterator);
}
