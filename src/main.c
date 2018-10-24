#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <iron/utils.h>
#include <iron/types.h>
#include "main.h"
#include "id_to_id.h"
#include "id_to_id.c"
#include "id_to_u64.h"
#include "id_to_u64.c"
#include "doubles.h"
#include "doubles.c"


id_to_id * subnodes;
doubles * doubles_table;
id_to_u64 * node_name;
int id_counter = 0;

object_id new_object2(object_id parent, int_str name){
  object_id new = ++id_counter;
  id_to_id_set(subnodes, parent, new, 0);
  id_to_u64_set(node_name, new, name);
  
  return new;
}

object_id new_object(object_id parent, const char * name){
  return new_object2(parent, intern_string(name));
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
  
  if(strcmp(type, "object_id") != 0)
   return false;
 printf("%i", ((int *)ptr)[0]);
 return true;
}

bool print_object_type(void * ptr, const char * type){
 if(strcmp(type, "type_id") != 0)
   return false;
 printf("%i", ((int *)ptr)[0]);
 return true;
}

object_id get_id_by_name(int_str str, object_id parent){
  size_t it = 0;
  size_t cnt;
  size_t indexes[10];
  
  while((cnt = id_to_id_iter(subnodes, &parent, 1, NULL, indexes, array_count(indexes), &it))){
    for(size_t i = 0; i < cnt; i++){
      var id = subnodes->value[indexes[i]];
      size_t loc_str;
      if(id_to_u64_try_get(node_name, &id, &loc_str)){
	if(loc_str == str)
	  return id;
      }else{
	printf("warning, unable to get id name%i\n", id);
      }
    }
  }
  return 0;
}

const char * try_parse_double(const char * cmd, double * out){
  char * outp;
  *out = strtod(cmd, &outp);
  return outp;
}

object_id icy_parse_id(const char * cmd){
  object_id parent = 0;
  while(true){
    if(cmd[0] != '/')
      break;
    const char * cmd2 = cmd + 1;
    char * cmd3_1 = strchrnul(cmd2, '/');
    char * cmd3_2 = strchrnul(cmd2, ' ');
    char * cmd3 = MIN(cmd3_1, cmd3_2);
    
    size_t l = cmd3 - cmd2;
    char buf[l + 1];
    memcpy(buf, cmd2, l);
    buf[l] = 0;
    int_str id = intern_string(buf);
    cmd = cmd3;
    object_id cid = get_id_by_name(id, parent);
    if(cid == 0){
      cid = new_object2(parent, id);
    }
    parent = cid;
  }

  while(*cmd == ' ')
    cmd++;
  while(true){
    double dval;
    var next = try_parse_double(cmd, &dval);
    if(next != cmd){
      set_double_value(parent, dval);
      cmd = next;
      continue;
    }
    break;
  }
  
  return parent;
}

void print_scope(int parent, char * parentName){
  size_t it = 0;
  size_t cnt;
  size_t indexes[10];
  
  while((cnt = id_to_id_iter(subnodes, &parent, 1, NULL, indexes, array_count(indexes), &it))){
    for(size_t i = 0; i < cnt; i++){
      var id = subnodes->value[indexes[i]];
      size_t loc_str;
      if(id_to_u64_try_get(node_name, &id, &loc_str)){
	
      }else{
	
      }
    }
  }
  return 0;
}

void metrohash64(const uint8_t * key, uint64_t len, uint32_t seed, uint8_t * out);

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
  node_name = id_to_u64_create(NULL);
  doubles_table = doubles_create(NULL);
  ((bool*)(&subnodes->is_multi_table))[0] = true;
  var a = new_object(0, "a");
  var b = new_object(a, "b");
  var c = new_object(0, "c");
  set_double_value(b, 5.0);
  set_double_value(c, 6.0);
  set_double_value(c, 16.0);
 
  printf("%i %i %i\n", a,b,c);
  icy_parse_id("/style/button/width 15.0");
  icy_parse_id("/style/button/height 30.0");
  icy_parse_id("/style/button/name \"hello\"");
  id_to_id_print(subnodes);
  id_to_u64_print(node_name);
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
