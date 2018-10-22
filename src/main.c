#include <stdlib.h>
#include <stdio.h>
#include <iron/utils.h>
#include "main.h"
#include "id_to_id.h"
#include "id_to_id.c"
#include "doubles.h"
#include "doubles.c"

id_to_id * subnodes;
doubles * doubles_table;

int id_counter = 0;
object_id new_object(object_id parent, const char * name){
  UNUSED(name);
  object_id new = ++id_counter;
  id_to_id_set(subnodes, parent, new, 0);
  
  return new;
}

void set_double_value(object_id key, double value){
  doubles_set(doubles_table, key, value);
  size_t out = 0;
  id_to_id_lookup(subnodes, &key, &out, 1);
  if(out){
    subnodes->type[out] = 1;
    subnodes->value[out] = key;
  }else{
    id_to_id_set(subnodes, key, key, 1);
  }
}

bool print_object_id(void * ptr, const char * type){
  UNUSED(type);
  if(strcmp(type, "object_id") != 0)
   return false;
 printf("%i", ((int *)ptr)[0]);
 return true;
}

bool print_object_type(void * ptr, const char * type){
UNUSED(type);
 if(strcmp(type, "type_id") != 0)
   return false;
 printf("%i", ((int *)ptr)[0]);
 return true;
}


void metrohash64(const uint8_t * key, uint64_t len, uint32_t seed, uint8_t * out);
size_t intern_string(const char * string);
int main(int argc, char ** argv){
  for(int i = 0; i < 2; i++){
    printf("%i\n", intern_string(""));
    
    printf("%i\n", intern_string("a"));
    printf("%i\n", intern_string("adwasds"));
    printf("%i\n", intern_string("avcxvc"));
    printf("%i\n", intern_string("adsada"));
    printf("%i\n", intern_string("a"));
    printf("%i\n", intern_string("dsa"));
    printf("%i\n", intern_string("dsa"));
  }

  UNUSED(argc);
  UNUSED(argv);
  icy_table_add(print_object_id);
  icy_table_add(print_object_type);
  subnodes = id_to_id_create(NULL);
  doubles_table = doubles_create(NULL);
  ((bool*)(&subnodes->is_multi_table))[0] = true;
  var a = new_object(0, "a");
  var b = new_object(a, "b");
  var c = new_object(0, "c");
  set_double_value(b, 5.0);
  set_double_value(c, 6.0);
  set_double_value(c, 16.0);
 
  printf("%i %i %i\n", a,b,c);
  id_to_id_print(subnodes);
  
  return 0;
}

/*
object_id icy_parse_id(const char * cmd){
  if(cmd[0] != '/')
    return 0;
  const char * cmd2 = cmd + 1;
  char * cmd3_1 = strchrnul(cmd, '/');
  char * cmd3_2 = strchrnul(cmd, ' ');
  char * cmd3 = MIN(cmd3_1, cmd3_2);
  size_t l = cmd3 - cmd2;
  char buf[l + 1];
  memcpy(buf, cmd2, l);
  buf[l] = 0;
  
  
}

void icy_cmd(const char * cmd){
  object_id id = icy_parse_id(cmd);
}


void go_test(){
  icy_cmd("/a");
  icy_cmd("/a/b 1.0");
  icy_cmd("/a/c 2.0");
  icy_cmd("/b /a");
  icy_cmd("/b/c 3.0");
  double v1,v2,v3,v4;
  icy_qry("/a/b", &v1);
  icy_qry("/a/c", &v2);
  icy_qry("/b/c", &v3);

  icy_cmd("/a/b 4.0");
  icy_qry("/b/c", &v4);
  
  ASSERT(v1 == 1.0);
  ASSERT(v2 == 2.0);
  ASSERT(v3 == 3.0);
  ASSERT(v4 == 4.0);
  

}
*/
