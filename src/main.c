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

#include "array_table.h"
#include "array_table.c"
typedef array_table_indexes array_index;
#include "array_alloc.h"
#include "array_alloc.c"

struct _icy_type;
typedef struct _icy_type icy_type;
typedef struct _icy_object icy_object;

struct _icy_object{
  union
  {
    void * userdata;
    size_t id;
  };

  const icy_type * type;
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
  void (* get_value)(icy_object * obj, void * out);
  
  void (* unset_value)(object_id id, void * userdata);
  void (* print)(icy_object *);
  void (* try_parse2)(icy_object * obj, char ** str);
  bool (* sub_next)(icy_object *id, icy_iter * iter);
  void (* sub_parse)(icy_object *id, char * str, icy_object * out);
  void * (* data)(icy_object * parent, size_t * required_size);
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
  array_table * data_table;
  array_alloc * object_data;
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
    printf("OUT type: %i %i %i\n", out->type->type_id, obj.type, ctx->object_type_id);
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

  var tp = out.type;
  cmd2 = cmd;
  printf("Got thing of type: %i\n", tp->type_id);
  if(tp->try_parse2 != NULL)
    tp->try_parse2(&out, (char **)&cmd);
  if(cmd2 == cmd){
    printf("Unable to parse %s\n", cmd);
  }
}


void icy_query(const char * cmd, void * outdata){
  const char * orig = cmd;
  icy_object obj;
  icy_parse_id2(NULL, &cmd, &obj);
  if(cmd != orig){
    var tp = obj.type;
    if(tp->get_value != NULL){
      tp->get_value(&obj, outdata);
    }else{
      printf("Unable to print type of '%s'\n", orig);
    }
  }else{
    printf("Unable to parse %s\n", orig);
  }
}

void icy_query_object(const char * cmd, icy_object * obj){
  icy_parse_id2(NULL, &cmd, obj);
}

void print_object_tree(icy_object * thing){
  if(thing->parent == NULL) return;
  print_object_tree(thing->parent);
  
  var tp = thing->type;
  printf("/");
  if(tp->print != NULL)
    tp->print(thing);
}

void print_object(icy_object * thing){
  print_object_tree(thing);
  var tp = thing->type;

  printf(" ");

  if(tp->print != NULL){
    icy_object sub = {
      .parent = thing,
      .userdata = NULL,
      .type = tp
    };
    tp->print(&sub);
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

  icy_eval("/style/button/height 30.0");
  icy_eval("/style/button/name 1337");
  icy_eval("/style/button/width 35.0");
  
  //id_to_id_print(subnodes);
  //id_to_u64_print(node_name);

  icy_eval("/button1 /style/button");
  icy_eval("/button1/width 15");
  icy_eval("/button1/width2/asd 1");

  double height;
  icy_query("/button1/width", &height);
  printf("Height: %f\n", height);
  ASSERT(height == 15.0);
  return;
  
  icy_eval("/button1/height 20");

  icy_query("/button1/height", &height);
  ASSERT(height == 20.0);
  
}

void icy_context_add_type(icy_context * ctx, icy_type * type){
  var idx = icy_types_alloc(ctx->types);
  type->type_id = idx.index;
  ctx->types->type[idx.index] = *type;
  printf("Loaded type: %i\n", type->type_id);
}


void double_try_parse2(icy_object * obj, char ** str){
  var orig = *str;
  double value = strtod(orig,str);
  if(*str != orig){
    size_t size = sizeof(double);
    var parent = obj->parent;
    if(parent->type->data == NULL){
      ERROR("data() cannot be NULL\n");
    }
    printf("setting double value to %f\n", value);
    double * val = parent->type->data(parent, &size);
    *val = value;
  }
}

void double_get_value(icy_object * id, void * out){
  var parent = id->parent;
  void * ptr = parent->type->data(parent, NULL);
  memcpy(out, ptr, sizeof(double));
}

void double_print(icy_object * id){
  double val;
  double_get_value(id, &val);
  printf("%f", val);
}

void double_load_type(icy_context * ctx){

  icy_type type =
    {
      .userdata = ctx,
      .try_parse2 = double_try_parse2,
      .get_value = double_get_value,
      .print = double_print,
      .unset_value = NULL,
      .sub_parse = NULL,
      .sub_next = NULL
    };
  icy_context_add_type(ctx, &type);
  type_name(type.type_id, "double");
  
}

typedef struct{
  icy_context * ctx;
  size_t garbage;
  id_to_u64 * value_type;
}object_context;

bool object_sub_next(icy_object * id, icy_iter * iter){

  object_id id2 = id->id;

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


void object_try_parse2(icy_object * obj, char ** str){
  var ctx2 = (object_context *) obj->type->userdata;
  printf("parsing sub object..\n");
  if(obj->id == 0) return;
  icy_object sub = {
    .parent = obj,
    .userdata = NULL,
  };
  var cmd = *str;
  var cmd2 = *str;
  for(size_t i = 0 ; i < ctx->types->count[0]; i++){
    var tp = ctx->types->type + i;
    if(tp->try_parse2 == NULL) continue;
    sub.type = tp;
    tp->try_parse2(&sub, (char **)&cmd);
    if(cmd2 != cmd){
      id_to_u64_set(ctx2->value_type, obj->id, tp->type_id);
      *str = cmd;
      break;
    }
  }  
}

void object_print(icy_object * obj){
  var ctx2 = (object_context *) obj->type->userdata;
  //ASSERT(obj->type->type_id == 1);
  size_t loc_str;
  if(id_to_u64_try_get(ctx->node_name, &obj->id, &loc_str)){
    size_t l = intern_string_read(ctx->stable, loc_str, NULL, 0);
    char buf[l];
    intern_string_read(ctx->stable, loc_str, buf, l);
    printf("%s", buf);
  }else{
    if(obj->userdata == NULL && obj->type->type_id == 1){
      type_id subtype;

      if(id_to_u64_try_get(ctx2->value_type, &obj->parent->id, &subtype)){
	var tp = ctx->types->type + subtype;
	icy_object obj2 = {
	  .type = tp,
	  .id = 0,
	  .parent = obj->parent
	};
	if(tp->print != NULL){
	  tp->print(&obj2);
	}
      }
    }

  }
}

void * object_data(icy_object * obj, size_t * size){
  array_index index = {0};
  size_t new_count;
  
  if(size != NULL)
    new_count = (*size - 1) / 4 + 1;
  
  if(array_alloc_try_get(ctx->object_data, &obj->id, &index)){
    
    if(size != NULL && index.count != new_count){
      
      array_table_remove_sequence(ctx->data_table, &index);
    }else{
      return ctx->data_table->data + index.index;
    }
  }
  if(size == NULL) return NULL;
  index = array_table_alloc_sequence(ctx->data_table, new_count);
  array_alloc_set(ctx->object_data, obj->id, index);
  return ctx->data_table->data + index.index;
}

void object_get_value(icy_object * obj, void * out){
  var ctx2 = (object_context *) obj->type->userdata;
  type_id type = 0;
  if(id_to_u64_try_get(ctx2->value_type, &obj->id, &type) == false)
    return;
  var tp = ctx->types->type + type;
  if(tp->get_value != NULL){
    icy_object obj2 = {.parent = obj,
		       .type = tp
    };
    tp->get_value(&obj2, out);
  }
}

void object_load_type(icy_context * ctx){
  object_context ctx2 = {
    .ctx = ctx,
    .garbage = 0xFF112233,
    .value_type = id_to_u64_create(NULL)
  };
  printf("load ctx: %i\n", ctx);
  object_context * octx = IRON_CLONE(ctx2);
  icy_type type =
    {
      .userdata = octx,
      .get_value = object_get_value,
      .unset_value = NULL,
      .try_parse2 = object_try_parse2,
      .print = object_print,
      .sub_next = object_sub_next,
      .sub_parse = object_sub_parse,
      .data = object_data
    };
  icy_context_add_type(ctx, &type);
  ctx->object_type_id = type.type_id;
  type_name(type.type_id, "object");
}

void type_print(icy_object * obj){
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
      .get_value = NULL,
      .unset_value = NULL,
      .print = type_print,
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
      .get_value = NULL,
      .unset_value = NULL,
      .print = object_print,
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
    ctx.data_table = array_table_create(NULL);
    ctx.object_data = array_alloc_create(NULL);
    
    ((bool*)(&ctx.subnodes->is_multi_table))[0] = true;
    ctx.stable = intern_string_init();
    ctx.id_counter = 0;
    ctx.types = icy_types_create(NULL);
    
    var ctx2 = IRON_CLONE(ctx);
    icy_context_make_current(ctx2);
  }
  
  icy_type garbage_type =
    {
      .userdata = NULL,
      .get_value = NULL,
      .unset_value = NULL,
      .print = NULL,
      .sub_next = NULL,
      .sub_parse = NULL
    };
  
  icy_context_add_type(ctx, &garbage_type);
  
  object_load_type(ctx);
  type_id type_type = type_load_type(ctx);
  type_id types_type =  types_load_type(ctx, type_type);
  double_load_type(ctx);

  icy_object obj;
  icy_parse_id3(NULL, "/typer", &obj);
  icy_parse_id3(NULL, "/type", &obj);

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
