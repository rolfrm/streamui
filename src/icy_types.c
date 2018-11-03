// This file is auto generated by icy-vector.
#ifndef TABLE_COMPILER_INDEX
#define TABLE_COMPILER_INDEX
#define array_element_size(array) sizeof(array[0])
#define array_count(array) (sizeof(array)/array_element_size(array))
#include "icydb.h"
#include <stdlib.h>
#endif

icy_types * icy_types_create(const char * optional_name){
  static const char * const column_names[] = {(char *)"type"};
  static const char * const column_types[] = {"icy_type"};

  icy_types * instance = calloc(sizeof(icy_types), 1);
  unsigned int sizes[] = {sizeof(icy_type)};
  
  instance->column_names = (char **)column_names;
  instance->column_types = (char **)column_types;
  
  ((size_t *)&instance->column_count)[0] = 1;
  for(unsigned int i = 0; i < 1; i++)
    ((size_t *)instance->column_sizes)[i] = sizes[i];

  icy_vector_abs_init((icy_vector_abs * )instance, optional_name);

  return instance;
}


icy_types_index icy_types_alloc(icy_types * table){
  icy_index idx = icy_vector_abs_alloc((icy_vector_abs *) table);
  icy_types_index index;
  index.index = idx.index;
  return index;
}

icy_types_indexes icy_types_alloc_sequence(icy_types * table, size_t count){
  icy_indexes idx = icy_vector_abs_alloc_sequence((icy_vector_abs *) table, count);
  icy_types_indexes index;
  index.index = idx.index.index;
  index.count = count;
  return index;
}

void icy_types_remove(icy_types * table, icy_types_index index){
  icy_index idx = {.index = index.index};
  icy_vector_abs_remove((icy_vector_abs *) table, idx);
}

void icy_types_remove_sequence(icy_types * table, icy_types_indexes * indexes){
  icy_indexes idxs;
  idxs.index.index = (unsigned int)indexes->index;
  idxs.count = indexes->count;
  icy_vector_abs_remove_sequence((icy_vector_abs *) table, &idxs);
  memset(&indexes, 0, sizeof(indexes));
  
}
void icy_types_clear(icy_types * table){
  icy_vector_abs_clear((icy_vector_abs *) table);
}

void icy_types_optimize(icy_types * table){
  icy_vector_abs_optimize((icy_vector_abs *) table);
}
void icy_types_destroy(icy_types ** table){
  icy_vector_abs_destroy((icy_vector_abs **) table);
}