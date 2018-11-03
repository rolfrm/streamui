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
#include "u64_to_u64.h"
#include "u64_to_u64.c"

#include "doubles.h"
#include "doubles.c"

struct _icy_type;
typedef struct _icy_type icy_type;
typedef struct _icy_object icy_object;

struct _icy_object{
  bool is_id;
  union
  {
    void * userdata;
    size_t id;
  };


  icy_type * type;
  icy_object * parent;
  
};

typedef struct{
  union {
    void * localdata;
    size_t number;
  };
  icy_object object;
}icy_iter;


struct _icy_type{
  void * userdata;
  // set the value of an object from a given type.
  void (* try_parse)(object_id id, char ** str, void * userdata);
  void (* set_value)(object_id id, void * data, void * userdata);
  void (* get_value)(object_id id, void * out, void * userdata);
  void (* unset_value)(object_id id, void * userdata);
  void (* print)(object_id id, void * userdata);
  void (* print2)(icy_object *);

  bool (* sub_next)(icy_object *id, icy_iter * iter);
  void (* sub_parse)(icy_object *id, char * str, icy_object * out);
  type_id type_id;
};



#include "icy_types.h"
#include "icy_types.c"

typedef struct{
  // map node to sub node.
  id_to_id * subnodes;
  // map node to style / inheritor.
  id_to_id * style_nodes;

  id_to_u64 * object_type;

  id_to_u64 * node_name;

  int id_counter;

  string_hash_table * stable;
  icy_types * types;

  type_id object_type_id;
  u64_to_u64 * type_name;
}icy_context;

icy_context * ctx;

void icy_context_make_current(icy_context * _ctx){
  ctx = _ctx;
}

object_id new_object2(object_id parent, int_str name){
  object_id new = ++ctx->id_counter;
  ASSERT(parent != new);
  id_to_id_set(ctx->subnodes, parent, new);
  id_to_u64_set(ctx->node_name, new, name);
  
  return new;
}

object_id new_object(object_id parent, const char * name){
  return new_object2(parent, intern_string(ctx->stable, name));
}


icy_type * icy_get_type(type_id id){
  ASSERT(ctx->types->count[0] > (size_t)id);
  icy_type * tp = ctx->types->type + id;
  return tp;
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
  size_t indexes[1];
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


void icy_parse_id2(icy_object * root, const char ** str, icy_object * out){
  icy_object obj = {0};
  if(root == NULL){
    obj.type = icy_get_type(ctx->object_type_id);
    root = &obj;
  }
  
  const char * cmd = *str;
  *out = *root;
  while(true){
    
    if(out->type->sub_parse == NULL)
      break;
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
    if(out->type->sub_parse != NULL)
      out->type->sub_parse(root, buf, out);
    else break;
    root = out;
    cmd = cmd3;
  }
  *str = cmd;
}

void icy_parse_id3(icy_object * root, const char * cmd, icy_object * obj){
  icy_parse_id2(root, &cmd, obj);
}

void icy_eval(const char * cmd){

  icy_object out;
  
  const char * cmd2 = cmd;
  icy_parse_id2(NULL, &cmd2, &out);
  if(cmd2 == cmd){
    printf("Unable to find object\n");
    return;
  }
  cmd = cmd2;
  while(*cmd == ' ')
    cmd++;
  printf("REST: %s\n", cmd);
  /*
  bool found = false;
  for(size_t i = 0 ; i < ctx->types->count[0]; i++){
    var tp = ctx->types->type[i];
    if(tp.try_parse == NULL) continue;
    
    }*/
  /*


  while(true){
  bool found = false;
    for(size_t i = 0 ; i < ctx->types->count[0]; i++){
    var tp = ctx->types->type[i];
      if(tp.try_parse == NULL) continue;
      char * cmd2 = (char *)cmd;
      tp.try_parse(id, &cmd2, tp.userdata);
      if(cmd2 != cmd){
	cmd = cmd2;
	id_to_u64_set(ctx->object_type, id, tp.type_id);
	found = true;
	break;
      }
    }
    if(!found){
      id_to_u64_unset(ctx->object_type, id);
    }
    break;	
    }*/
}


void icy_query(const char * cmd, void * outdata){
  const char * orig = cmd;
  object_id id = icy_parse_id(cmd, &cmd, false);
  if(cmd != orig){
    u64 _type;
    if(id_to_u64_try_get(ctx->object_type, &id, &_type)){
      type_id t = _type;
      ASSERT(ctx->types->count[0] > (size_t)t);
      icy_type * tp = ctx->types->type + t;
      if(tp->get_value != NULL){
	tp->get_value(id, outdata, tp->userdata);
      }else{
	printf("Unable to print type of '%s'", orig);
      }
      
    }else{
      printf("Warning: object does not have a type (%i: %s).\n", id, orig);
    }
  }else{
    printf("Unable to parse %s\n", orig);
  }
}

void icy_query_object(const char * cmd, icy_object * obj){
  icy_parse_id2(NULL, &cmd, obj);
}

void print_object(icy_object * thing){
  if(thing->parent == NULL) return;
  print_object(thing->parent);
  
  var tp = thing->type;
  printf("/");
  if(tp->print2 != NULL){
    tp->print2(thing);
  }
}

void print_scope3(icy_object * parent){
  icy_iter iter = {0};
  var tp = parent->type;
  
  print_object(parent);
  printf("\n");
  if(tp->sub_next != NULL){
    while(tp->sub_next(parent, &iter)){
      print_scope3(&iter.object);
    }
  }
}


void print_scope2(){
  icy_object obj = {0};
  obj.type = icy_get_type(ctx->object_type_id);
   
  print_scope3(&obj);
}

void print_scope(object_id parent, const char * parent_name){

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
  //var types = ctx->types;
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
	printf("%s", buf2);
	u64 _typeid;
	if(id_to_u64_try_get(ctx->object_type, &id, &_typeid)){
	  type_id typeid = _typeid;
	  icy_type * tp = ctx->types->type + typeid;
	  if(tp->print != NULL){
	    printf(" ");
	    tp->print(id, tp->userdata);
	  }
	}
	printf("\n");
	
	print_scope(id, buf2);
      }
    }
  }
}

void type_name(type_id type, const char * name){
  var istr = intern_string(ctx->stable, name);
  u64_to_u64_set(ctx->type_name, type, istr);
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
  icy_eval("/style/array");
  icy_object obj;
  icy_query_object("/style/array", &obj);  
  
  icy_eval("/style/button/width 15.0");
  return;
  icy_eval("/style/button/height 30.0");
  icy_eval("/style/button/name 1337");
  //id_to_id_print(subnodes);
  //id_to_u64_print(node_name);

  icy_eval("/button1 /style/button");
  icy_eval("/button1/width 20");

  double height;
  icy_query("/button1/height", &height);
  ASSERT(height == 15.0);
  
  icy_eval("/button1/height 20");

  icy_query("/button1/height", &height);
  ASSERT(height == 20.0);
  print_scope(0, "");
  
}

void icy_context_add_type(icy_context * ctx, icy_type * type){
  var idx = icy_types_alloc(ctx->types);
  type->type_id = idx.index;
  ctx->types->type[idx.index] = *type;
  printf("Loaded type: %i\n", type->type_id);
}

typedef struct {
  doubles * doubles_table;

}icy_double_type;

void double_try_parse(object_id id, char ** str, void * userdata){
  icy_double_type * type = userdata;
  var orig = *str;
  double out = strtod(orig,str);
  if(*str != orig){
    doubles_set(type->doubles_table, id, out);
  }
}

void double_set_value(object_id id, void * data, void * userdata){
  icy_double_type * type = userdata;
  doubles_set(type->doubles_table, id, ((double *)data)[0]);
}

void double_get_value(object_id id, void * out, void * userdata){
  icy_double_type * type = userdata;
  size_t idx = 0;
  doubles_lookup(type->doubles_table, &id, &idx, 1);
  if(idx != 0)
    ((double *)out)[0] = type->doubles_table->value[idx];
}

void double_unset_value(object_id id, void * userdata){
  icy_double_type * type = userdata;
  doubles_unset(type->doubles_table, id);
}

void double_print(object_id id, void * userdata){
  icy_double_type * type = userdata;
  double value = 0.0;
  ASSERT(doubles_try_get(type->doubles_table, &id, &value));
  printf("%f", value);
}


void double_load_type(icy_context * ctx){
  icy_double_type dt = {
    .doubles_table = doubles_create(NULL)
  };
  icy_type type =
    {
      .userdata = IRON_CLONE(dt),
      .try_parse = double_try_parse,
      .set_value = double_set_value,
      .get_value = double_get_value,
      .unset_value = double_unset_value,
      .print = double_print,
      .sub_parse = NULL,
      .sub_next = NULL
    };
  icy_context_add_type(ctx, &type);
  type_name(type.type_id, "double");
  
}

typedef struct{
  icy_context * ctx;
}icy_object_type;

void object_try_parse(object_id id, char ** str, void * userdata){
  UNUSED(userdata);
  char * orig = *str;
  object_id id2 = icy_parse_id(orig, (const char **) &orig, false);
  if(orig != *str){
    id_to_id_set(ctx->style_nodes, id, id2);
  }
}

bool object_sub_next(icy_object * id, icy_iter * iter){

  object_id id2 = id->id;
  var ctx = (icy_context *) id->type->userdata;
  size_t index;
  size_t c = id_to_id_iter(ctx->subnodes, &id2, 1, NULL, &index, 1, &iter->number);
  if(c == 0) return false;
  iter->object.id = ctx->subnodes->value[index];
  type_id type_id;
  if(id_to_u64_try_get(ctx->object_type, &iter->object.id, &type_id)){
    iter->object.type = ctx->types->type + type_id;
  }else{
    iter->object.type = id->type;
  }

  iter->object.parent = id;
  return true;
}

void object_sub_parse(icy_object *id, char * str, icy_object * out){
  object_id id2 = id->id;
  var ctx = (icy_context *) id->type->userdata;

  var stable = ctx->stable;
  int_str idstr = intern_string(stable, str);
  object_id cid = get_id_by_name(idstr, id2);
  bool create = true;
  if(cid == 0){
    if(create)
      cid = new_object2(id2, idstr);
  }
  out->id = cid;
  out->type = id->type;
  
}

void object_print(object_id id, void * userdata){
  UNUSED(id);
  UNUSED(userdata);
  printf("Object");
}


void object_print2(icy_object * obj){
  UNUSED(obj);
  size_t loc_str;
  if(id_to_u64_try_get(ctx->node_name, &obj->id, &loc_str)){
    size_t l = intern_string_read(ctx->stable, loc_str, NULL, 0);
    char buf[l];
    intern_string_read(ctx->stable, loc_str, buf, l);
    printf("%s", buf);
  }else{
    printf("Object");
  }
}

void object_load_type(icy_context * ctx){

  printf("load ctx: %i\n", ctx);
  icy_type type =
    {
      .userdata = ctx,
      .try_parse = object_try_parse,
      .set_value = NULL,
      .get_value = NULL,
      .unset_value = NULL,
      .print = object_print,
      .print2 = object_print2,
      .sub_next = object_sub_next,
      .sub_parse = object_sub_parse
    };
  icy_context_add_type(ctx, &type);
  ctx->object_type_id = type.type_id;
  type_name(type.type_id, "object");
}

void type_print(object_id id, void * userdata){

  UNUSED(id);UNUSED(userdata);
  printf("Type..\n");

}

void type_print2(icy_object * obj){
  icy_context * ctx = obj->type->userdata;;
  u64 nameid;

  if(u64_to_u64_try_get(ctx->type_name, &obj->id, &nameid)){
    size_t l = intern_string_read(ctx->stable, nameid, NULL, 0);
    char buf[l];
    intern_string_read(ctx->stable, nameid, buf, l);
    printf("%s", buf);
  }else{
    printf("[type]");
  }
}

type_id type_load_type(icy_context * ctx){
  icy_type type =
    {
      .userdata = ctx,
      .try_parse = NULL,
      .set_value = NULL,
      .get_value = NULL,
      .unset_value = NULL,
      .print = type_print,
      .print2 = type_print2,
      .sub_next = NULL,
      .sub_parse = NULL
    };
  
  icy_context_add_type(ctx, &type);
  type_name(type.type_id, "type");
  return type.type_id;
}
typedef struct{
  icy_context * ctx;
  type_id type_type_id;

}types_data;

bool types_sub_next(icy_object * id, icy_iter * iter){

  var tctx = (types_data *) id->type->userdata;
  var ctx = tctx->ctx;

  size_t * i =  &iter->number;
  if(ctx->types->count[0] <= *i)
    return false;
    
  icy_type * tp = ctx->types->type + tctx->type_type_id;
  iter->object.type = tp;
  iter->object.id = *i;
  iter->object.parent = id;

  (*i)++;
  
  
  return true;
}

type_id types_load_type(icy_context * ctx, type_id type_type_id){
  types_data * d = alloc0(sizeof(types_data));
  d->ctx = ctx;
  d->type_type_id = type_type_id;
  icy_type type =
    {
      .userdata = d,
      .try_parse = NULL,
      .set_value = NULL,
      .get_value = NULL,
      .unset_value = NULL,
      .print = object_print,
      .print2 = object_print2,
      .sub_next = types_sub_next,
      .sub_parse = NULL
    };
  
  icy_context_add_type(ctx, &type);
  type_name(type.type_id, "types");
  return type.type_id;
}

icy_context * icy_context_initialize(){
  {
    icy_context ctx;
    ctx.subnodes = id_to_id_create(NULL);
    ctx.style_nodes = id_to_id_create(NULL);
    ctx.object_type = id_to_u64_create(NULL);
    ctx.node_name = id_to_u64_create(NULL);
    ctx.type_name = u64_to_u64_create(NULL);
    
    ((bool*)(&ctx.subnodes->is_multi_table))[0] = true;
    ctx.stable = intern_string_init();
    ctx.id_counter = 0;
    ctx.types = icy_types_create(NULL);
    
    var ctx2 = IRON_CLONE(ctx);
    icy_context_make_current(ctx2);
  }
  object_load_type(ctx);
  type_id type_type = type_load_type(ctx);
  type_id types_type =  types_load_type(ctx, type_type);
  double_load_type(ctx);

  icy_object obj;
  icy_parse_id3(NULL, "/typer", &obj);
  icy_parse_id3(NULL, "/type", &obj);

  printf("ID: %i\n", obj.id);
  id_to_u64_set(ctx->object_type, obj.id, types_type);

  return ctx;
}

int main(int argc, char ** argv){
  for(int i = 0; i < 1; i++){
    int fk = 0;//fork();
    if (fk == 0) {
      icy_context_make_current(icy_context_initialize());
      //test_hashing();
      //test_string_intern();

      test_eval();
      print_scope2();
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
  icy_context_make_current(icy_context_initialize());

  /*
  var a = new_object(0, "a");
  var b = new_object(a, "b");
  var c = new_object(0, "c");
  set_double_value(b, 5.0);
  set_double_value(c, 6.0);
  set_double_value(c, 16.0);
  */
  //printf("%i %i %i\n", a,b,c);
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
