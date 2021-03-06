#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <iron/types.h>
#include <iron/mem.h>
// most of this code is taken from metrohash.
inline static uint64_t rotate_right(uint64_t v, unsigned k)
{
    return (v >> k) | (v << (64 - k));
}

// unaligned reads, fast and safe on Nehalem and later microarchitectures
inline static uint64_t read_u64(const void * const ptr)
{
  return *(const uint64_t*)(ptr);
}

inline static uint64_t read_u32(const void * const ptr)
{
  return *(const uint32_t*)(ptr);
}

inline static uint64_t read_u16(const void * const ptr)
{
  return *(const uint16_t*)(ptr);
}

inline static uint64_t read_u8 (const void * const ptr)
{
  return *(const uint8_t *)(ptr);
}

void metrohash64(const uint8_t * key, uint64_t len, uint32_t seed, uint8_t * out)
{
    static const uint64_t k0 = 0xD6D018F5;
    static const uint64_t k1 = 0xA2AA033B;
    static const uint64_t k2 = 0x62992FC1;
    static const uint64_t k3 = 0x30BC5B29; 

    const uint8_t * ptr = key;
    const uint8_t * const end = ptr + len;
    
    uint64_t hash = ((seed + k2) * k0) + len;
    
    if (len >= 32)
    {
        uint64_t v[4];
        v[0] = hash;
        v[1] = hash;
        v[2] = hash;
        v[3] = hash;
        
        do
        {
            v[0] += read_u64(ptr) * k0; ptr += 8; v[0] = rotate_right(v[0],29) + v[2];
            v[1] += read_u64(ptr) * k1; ptr += 8; v[1] = rotate_right(v[1],29) + v[3];
            v[2] += read_u64(ptr) * k2; ptr += 8; v[2] = rotate_right(v[2],29) + v[0];
            v[3] += read_u64(ptr) * k3; ptr += 8; v[3] = rotate_right(v[3],29) + v[1];
        }
        while (ptr <= (end - 32));

        v[2] ^= rotate_right(((v[0] + v[3]) * k0) + v[1], 30) * k1;
        v[3] ^= rotate_right(((v[1] + v[2]) * k1) + v[0], 30) * k0;
        v[0] ^= rotate_right(((v[0] + v[2]) * k0) + v[3], 30) * k1;
        v[1] ^= rotate_right(((v[1] + v[3]) * k1) + v[2], 30) * k0;
        hash += v[0] ^ v[1];
    }
    
    if ((end - ptr) >= 16)
    {
        uint64_t v0 = hash + (read_u64(ptr) * k2); ptr += 8; v0 = rotate_right(v0,29) * k3;
        uint64_t v1 = hash + (read_u64(ptr) * k2); ptr += 8; v1 = rotate_right(v1,29) * k3;
        v0 ^= rotate_right(v0 * k0, 34) + v1;
        v1 ^= rotate_right(v1 * k3, 34) + v0;
        hash += v1;
    }
    
    if ((end - ptr) >= 8)
    {
        hash += read_u64(ptr) * k3; ptr += 8;
        hash ^= rotate_right(hash, 36) * k1;
    }
    
    if ((end - ptr) >= 4)
    {
        hash += read_u32(ptr) * k3; ptr += 4;
        hash ^= rotate_right(hash, 15) * k1;
    }
    
    if ((end - ptr) >= 2)
    {
        hash += read_u16(ptr) * k3; ptr += 2;
        hash ^= rotate_right(hash, 15) * k1;
    }
    
    if ((end - ptr) >= 1)
    {
        hash += read_u8 (ptr) * k3;
        hash ^= rotate_right(hash, 23) * k1;
    }
    
    hash ^= rotate_right(hash, 28);
    hash *= k0;
    hash ^= rotate_right(hash, 29);

    memcpy(out, &hash, 8);
}

#include "string_table.h"
#include "string_table.c"
#include "interned_strings.h"
#include "interned_strings.c"
#include <stdio.h>
#include <iron/utils.h>
#include "main.h"

struct _string_hash_table {
  string_table * table1;
  interned_strings * table2;
};

string_hash_table * intern_string_init(){

  var table1 = string_table_create(NULL);
  var table2 = interned_strings_create(NULL);
  ((bool *)(&table2->is_multi_table))[0] = true;

  string_hash_table t =
    {.table1 = table1,
     .table2 = table2
    };

  return IRON_CLONE(t);
}

  
size_t intern_string_read(string_hash_table * t, int_str str, char * buffer, size_t buflen){
  var table1 = t->table1;
  char * start = (char *) (table1->data + str);
  if(buffer == NULL)
    return strlen(start) + 1;
  
  size_t copied= MIN(strlen(start) + 1, buflen);
  memcpy(buffer, start, copied);
  return copied;
}

int_str intern_string(string_hash_table * t, const char * string){

  var table1 = t->table1;
  var table2 = t->table2;

  u64 hash;
  int slen = strlen(string) + 1;
  hash = data_hash(string, slen);
  
  u32 indexer = ((u32) hash);
  size_t index;
  size_t iterator = 0;
  while(interned_strings_iter(table2, &indexer, 1, NULL, &index, 1, &iterator) > 0){
    string_table_indexes idxes = table2->str[index];
    char * str = (char *)(table1->data + idxes.index);
    if(strcmp(str, string) == 0)
      return idxes.index;
  }

  string_table_indexes idx = string_table_alloc_sequence(table1, (slen - 1) / 4 + 1);
  memcpy(table1->data + idx.index, string, slen);
  interned_strings_set(table2, indexer, idx);
  return idx.index;
}

size_t intern_string_count(string_hash_table * s){
  return s->table2->count;
}

size_t data_hash(const void * buffer, size_t len){
  size_t hash;
  metrohash64(buffer, len, 0x12513,(void *) &hash);
  return hash;
}

size_t string_hash(const char * buffer){
  return data_hash(buffer, strlen(buffer) + 1);
}
