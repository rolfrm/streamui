#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <unistd.h>  // fork
#include <sys/wait.h> // wait

#include <iron/utils.h>
#include <iron/types.h>
#include <iron/time.h>
#include <iron/log.h>
#include <iron/mem.h>
#include "main.h"
#include "id_to_id.h"
#include "id_to_id.c"
#include "id_to_u64.h"
#include "id_to_u64.c"
#include "doubles.h"
#include "doubles.c"

typedef struct{
  // map node to sub node.
  id_to_id * subnodes;
  // map node to style / inheritor.
  id_to_id * style_nodes;
  
  doubles * doubles_table;
  id_to_u64 * node_name;
  int id_counter;

  string_hash_table * stable;
}icy_context;

icy_context * ctx;

void icy_context_make_current(icy_context * _ctx){
  ctx = _ctx;
}


object_id new_object2(object_id parent, int_str name){
  object_id new = ++ctx->id_counter;
  ASSERT(parent != new);
  id_to_id_set(ctx->subnodes, parent, new, 0);
  id_to_u64_set(ctx->node_name, new, name);
  
  return new;
}

object_id new_object(object_id parent, const char * name){
  return new_object2(parent, intern_string(ctx->stable, name));
}


void set_double_value(object_id key, double value){
  doubles_set(ctx->doubles_table, key, value);
  var subnodes = ctx->subnodes;
  size_t out = 0;
  id_to_id_lookup(subnodes, &key, &out, 1);
  if(out){
    subnodes->type[out] = 1;
    subnodes->value[out] = key;
  }else{
    //id_to_id_set(subnodes, key, key, 1);
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
  var subnodes = ctx->subnodes;
  var node_name = ctx->node_name;
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

bool try_get_double(object_id id, double * out){
  return doubles_try_get(ctx->doubles_table, &id, out);
}

object_id icy_parse_id(const char * cmd, const char ** cmdd, bool create){
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
    int_str id = intern_string(ctx->stable, buf);
    cmd = cmd3;
    object_id cid = get_id_by_name(id, parent);
    if(cid == 0){
      if(create)
	cid = new_object2(parent, id);
      else
	return 0;
    }
    parent = cid;
  }
  if(cmdd != NULL)
    *cmdd = cmd;
  return parent;
}
void icy_eval(const char * cmd){
  const char * cmd2 = NULL;
  object_id id = icy_parse_id(cmd, &cmd2, true);
  if(cmd2 == NULL){
    printf("Unable to parse '%s'\n", cmd);
    return;
  }
  cmd = cmd2;

  while(*cmd == ' ')
    cmd++;
  while(true){
    double dval;
    var next = try_parse_double(cmd, &dval);
    if(next != cmd){
      set_double_value(id, dval);
      cmd = next;
      continue;
    }
    object_id id2 = icy_parse_id(cmd,NULL, false);
    if(id2 != 0){
      id_to_id_set(ctx->style_nodes, id, id2, 0);
    }
    break;
  }
  
  return;
}

void print_scope(int parent, const char * parent_name){

  size_t cnt;
  size_t indexes[10] = {0};
  int parlen = strlen(parent_name);
  var style_nodes = ctx->style_nodes;
  var subnodes = ctx->subnodes;
  var node_name = ctx->node_name;
  size_t it = 0;
  while((cnt = id_to_id_iter(style_nodes, &parent, 1, NULL, indexes, array_count(indexes), &it))){
    for(size_t i =0 ;i < cnt; i++){
      var id = style_nodes->value[indexes[i]];
      print_scope(id, parent_name);
    }
  }

  it = 0;
  
  while((cnt = id_to_id_iter(subnodes, &parent, 1, NULL, indexes, array_count(indexes), &it))){
    for(size_t i = 0; i < cnt; i++){
      var id = subnodes->value[indexes[i]];
      size_t loc_str;
      if(id_to_u64_try_get(node_name, &id, &loc_str)){
	size_t l = intern_string_read(ctx->stable, loc_str, NULL, 0);
	char buf2[parlen + 1 + l];
	memset(buf2, 0, sizeof(buf2));
	memcpy(buf2, parent_name, parlen);
	buf2[parlen] = '/';
	buf2[parlen + l] = 0;
	intern_string_read(ctx->stable, loc_str, buf2 + parlen + 1, l);
	double v;
	if(try_get_double(id, &v)){
	  printf("%s %f\n", buf2, v);
	}else{
	  printf("%s\n", buf2);
	}
	print_scope(id, buf2);
      }
    }
  }
}

void test_string_intern(){

  string_hash_table * stable = intern_string_init();
  srand(timestamp());
  int cnt = 100;
  u32 * rands = alloc0(cnt * sizeof(rands[0]));
  int_str * ints = alloc0(cnt * sizeof(ints[0]));
  for(int i = 0; i < cnt; i++){
    rands[i]= (u32)rand();
  }
  for(int i = 0; i < cnt; i++){
    u32 _rand = rands[i];
    int chars = (rands[i] % 101) + 1;
    char buf[chars];
    for(int i = 0; i < chars; i++){
      buf[i] = (char)((_rand + i* 9377) & 0xFF);
      if(buf[i] == 0)
	buf[i] = 32;
    }
    buf[chars - 1] = 0;
    ints[i] = intern_string(stable, buf);
    u32 int2 = intern_string(stable, buf);
    ASSERT(ints[i] == int2);
    size_t l = intern_string_read(stable, int2, NULL, 0);
    char buf2[l];
    intern_string_read(stable, int2, buf2, l);
    ASSERT(strcmp(buf2, buf) == 0);
  }
  for(int i = 0; i < cnt; i++){
    u32 _rand = rands[i];
    int chars = (rands[i] % 101) + 1;
    char buf[chars];
    for(int i = 0; i < chars; i++){
      buf[i] = (char)((_rand + i* 9377) & 0xFF);
      if(buf[i] == 0)
	buf[i] = 32;
    }
    buf[chars - 1] = 0;
    int_str i2 = intern_string(stable, buf);
    ASSERT(i2 == ints[i]);
  }
}

void test_hashing(){
  var h1 = string_hash("");
  var h2 = string_hash("asd");
  var h3 = string_hash("asd123321");
  var h12 = string_hash("");
  var h22 = string_hash("asd");
  var h32 = string_hash("asd123321");
  //printf("%p %p %p %p %p %p\n", h1, h2, h3, h12, h22, h32);
  ASSERT(h1 == h12);
  ASSERT(h2 == h22);
  ASSERT(h3 == h32);
}

void test_eval(){

  var a = new_object(0, "a");
  var b = new_object(a, "b");
  var c = new_object(0, "c");
  ASSERT(a != b);
  ASSERT(b != c);
  return;

  icy_eval("/style/button/width 15.0");
  icy_eval("/style/button/height 30.0");
  icy_eval("/style/button/name 1337");
  //id_to_id_print(subnodes);
  //id_to_u64_print(node_name);

  icy_eval("/button1 /style/button");
  icy_eval("/button1/width 20");
  icy_eval("/button1/height 20");
}


icy_context * initialize(){
  icy_context ctx;
  ctx.subnodes = id_to_id_create(NULL);
  ctx.style_nodes = id_to_id_create(NULL);
  ctx.node_name = id_to_u64_create(NULL);
  ctx.doubles_table = doubles_create(NULL);
  ((bool*)(&ctx.subnodes->is_multi_table))[0] = true;
  ctx.stable = intern_string_init();
  ctx.id_counter = 0;
  return IRON_CLONE(ctx);
}

int main(int argc, char ** argv){
  for(int i = 0; i < 1; i++){
    int fk = fork();
    if (fk == 0) {
      icy_context_make_current(initialize());
      test_hashing();
      test_string_intern();

      test_eval();
      printf("done test\n");
      return 0;
    } else {

      wait(&fk);
      printf("Done\n");
      if(fk != 0){
	printf("TEST FAILED %i :(\n", fk);
	return 0;
      }
    }
  }

  UNUSED(argc);
  UNUSED(argv);
  icy_table_add(print_object_id);
  icy_table_add(print_object_type);
  icy_context_make_current(initialize());
  
  var a = new_object(0, "a");
  var b = new_object(a, "b");
  var c = new_object(0, "c");
  set_double_value(b, 5.0);
  set_double_value(c, 6.0);
  set_double_value(c, 16.0);
 
  printf("%i %i %i\n", a,b,c);
  icy_eval("/style/button/width 15.0");
  icy_eval("/style/button/height 30.0");
  icy_eval("/style/button/name 1337");
  //id_to_id_print(subnodes);
  //id_to_u64_print(node_name);

  icy_eval("/button1 /style/button");
  icy_eval("/button1/width 20");
  icy_eval("/button1/height 20");
  
  
  print_scope(0, "");
  var node_name = ctx->node_name;
  for(size_t i = 0; i < node_name->count; i++){
    int_str s = node_name->value[i + 1];
    size_t l = intern_string_read(ctx->stable, s, NULL, 0);
    char buf[l];
    intern_string_read(ctx->stable, s, buf, l);
    
    printf("Name: %i %s\n", s, buf);
  }
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
