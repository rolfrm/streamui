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
#include "id_to_u64.h"
#include "u64_to_u64.h"

#include "doubles.h"

#include "array_table.h"

#include "array_alloc.h"

#include "id_to_id.c"
#include "id_to_u64.c"
#include "u64_to_u64.c"
#include "array_alloc.c"
#include "doubles.c"
#include "array_table.c"

#include "icy_types.h"
#include "icy_types.c"

struct _icy_context {
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
};

icy_context * ctx;

void icy_context_make_current(icy_context * _ctx){
  ctx = _ctx;
}

icy_type * icy_get_type(type_id id){
  ASSERT(ctx->types->count[0] > (size_t)id);
  return ctx->types->type + id;
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
  size_t cnt = 0;
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
	//printf("warning, unable to get id name%i %i\n", id, parent);
	//ASSERT(false);
      }
    }
  }
  return 0;
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

void icy_bytecode(const char * cmd, void ** bytecode, size_t * bufsize){
  icy_object obj = {0};
  obj.type = icy_get_type(ctx->object_type_id);
  size_t buffer_offset = 0;

  void pushv(size_t v){
    if(buffer_offset + sizeof(v) >= *bufsize){
      *bufsize = 2 * *bufsize + sizeof(v);
      *bytecode = realloc(*bytecode, *bufsize);
    }
    memcpy(*bytecode + buffer_offset, &v, sizeof(v));
    buffer_offset += sizeof(v);

  }
  
  void proc(icy_object * out, const char * cmd){
    
    if(cmd[0] != '/'){
      pushv(0);
      out->type->sub_encode(out, *bytecode + buffer_offset, *bufsize - buffer_offset);
      // Now encode the value.
      return;
    }
    const char * cmd2 = cmd + 1;
    char * cmd3_1 = strchrnul(cmd2, '/');
    char * cmd3_2 = strchrnul(cmd2, ' ');
    char * cmd3 = MIN(cmd3_1, cmd3_2);
    size_t l = cmd3 - cmd2;
    char buf[l + 1];
    memcpy(buf, cmd2, l);
    buf[l] = 0;
    ASSERT(out->type->sub_parse2 != NULL);
    object_id i = out->type->sub_parse2(&obj, buf);
    pushv(i);
    icy_object child = {0 };
    ASSERT(out->type->sub_get != NULL);
    out->type->sub_get(out, i, &child);
    proc(&child, cmd3);
    
  }
  proc(&obj, cmd);

  
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
  if(*cmd == 0) return;
  var tp = out.type;
  cmd2 = cmd;
  //printf("Got thing of type: %i\n", tp->type_id);
  if(tp->try_parse != NULL)
    tp->try_parse(&out, (char **)&cmd);
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

void print_scope(){
  icy_object obj = {0};
  obj.type = icy_get_type(ctx->object_type_id);
   
  print_scope3(&obj);
}

void type_name(type_id type, const char * name){
  var istr = intern_string(ctx->stable, name);
  u64_to_u64_set(ctx->type_name, type, istr);
}

void icy_context_add_type(icy_context * ctx, icy_type * type){
  var idx = icy_types_alloc(ctx->types);
  type->type_id = idx.index;
  ctx->types->type[idx.index] = *type;
  printf("Loaded type: %i\n", type->type_id);
}


void double_try_parse(icy_object * obj, char ** str){
  var orig = *str;
  double value = strtod(orig,str);
  if(*str != orig){
    size_t size = sizeof(double);
    var parent = obj->parent;
    if(parent->type->data == NULL){
      ERROR("data() cannot be NULL\n");
    }
    //printf("setting double value to %f\n", value);
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
      .try_parse = double_try_parse,
      .get_value = double_get_value,
      .print = double_print,
      .sub_parse = NULL,
      .sub_next = NULL
    };
  icy_context_add_type(ctx, &type);
  type_name(type.type_id, "double");
  
}

typedef struct{
  icy_context * ctx;
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
  static bool object_sub_parse_create = true;
  object_id parent = id->id;
  
  var stable = ctx->stable;
  int_str idstr = intern_string(stable, str);
  object_id cid = get_id_by_name(idstr, parent);
  
  if(cid == 0){
    var ctx2 = (object_context*) id->type->userdata;
    type_id subtype;
    id_to_u64_try_get(ctx2->value_type, &id->id, &subtype);
    if(subtype == id->type->type_id){
      size_t s = sizeof(u64);
      u64 * subobj_id = id->type->data(id,&s);
      if(*subobj_id != 0){
	icy_object subobj2 = {
	  .parent = id,
	  .type = id->type,
	  .id = *subobj_id
	};
	printf("Recurse!\n");
	var prev = object_sub_parse_create;
	object_sub_parse_create = false;
	out->id = 0;
	object_sub_parse(&subobj2, str, out);
	object_sub_parse_create = prev;
	cid = out->id;
	printf("Recurse found %i\n",cid);
      }
    }
    if(cid == 0){
      var name = intern_string(ctx->stable, str);
      object_id new = ++ctx->id_counter;
      ASSERT(parent != new);
      id_to_id_set(ctx->subnodes, parent, new);
      id_to_u64_set(ctx->node_name, new, name);
      cid = new;
    }
  }
  out->id = cid;
  out->type = id->type;
}

object_id object_sub_parse2(icy_object *id, char * str){
  object_id parent = id->id;
  if(strcmp(str, "type") == 0){
    return -1;
  }
  var stable = ctx->stable;
  int_str idstr = intern_string(stable, str);
  object_id cid = get_id_by_name(idstr, parent);
  if(cid == 0){
    var name = intern_string(ctx->stable, str);
    object_id new = ++ctx->id_counter;
    ASSERT(parent != new);
    id_to_id_set(ctx->subnodes, parent, new);
    id_to_u64_set(ctx->node_name, new, name);
    cid = new;
  }
  return (object_id)cid;
}


void object_sub_get(icy_object * obj, object_id id, icy_object * out){
  if(id == (object_id)-1){
    return; //special type case.
  }

  type_id obj_type = {0};
  if(id_to_u64_try_get(ctx->object_type, &id, &obj_type) == false){
    obj_type = obj->type->type_id;
  }
  
  out->id = id;
  out->type = ctx->types->type + obj_type;
  out->parent = obj->parent;
}



void object_try_parse(icy_object * obj, char ** str){
  var ctx2 = (object_context *) obj->type->userdata;
  if(obj->id == 0) return;
  icy_object sub = {
    .parent = obj,
    .userdata = NULL,
  };
  var cmd = *str;
  var cmd2 = *str;
  for(size_t i = 0 ; i < ctx->types->count[0]; i++){
    var tp = ctx->types->type + i;
    if(tp->try_parse == NULL) continue;
    sub.type = tp;
    tp->try_parse(&sub, (char **)&cmd);
    if(cmd2 != cmd){
      id_to_u64_set(ctx2->value_type, obj->id, tp->type_id);
      *str = cmd;
      return;
    }
  }

  icy_object out;
  icy_parse_id2(NULL, (const char **) &cmd, &out);
  if(*cmd != *cmd2){
     if(out.type == obj->type){
      id_to_u64_set(ctx2->value_type, obj->id, out.type->type_id);
      size_t size = sizeof(obj->id);
      void * p = obj->type->data(obj, &size);
      memcpy(p, &out.id, size);
    }else{
      ERROR("Unable to translate type");
    }
    return;
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
	if(tp->print != NULL && tp != obj->type){
	  tp->print(&obj2);
	}else{
	  printf("[object]");
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
    .value_type = id_to_u64_create(NULL)
  };
  printf("load ctx: %i\n", ctx);
  object_context * octx = IRON_CLONE(ctx2);
  icy_type type =
    {
      .userdata = octx,
      .get_value = object_get_value,
      .try_parse = object_try_parse,
      .print = object_print,
      .sub_next = object_sub_next,
      .sub_parse = object_sub_parse,
      .sub_parse2 = object_sub_parse2,
      .sub_get = object_sub_get,
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

  // the first type. Used to ward against null values.
  // todo: Consider making all its methods throw exceptions.
  icy_type garbage_type =
    {
      .userdata = NULL,
      .get_value = NULL,
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

void eval_by_line(FILE * stream){
  size_t n = 16;
  char * lineptr = alloc0(n);
  memset(lineptr, 0, n);
  ssize_t linebuffer_size = 4;
  char * linebuffer = alloc0(linebuffer_size);
  int bufferoffset = 0;
  bool inquote = false;
  ssize_t read;
  printf("Read from stream %i\n", stream);
  while((read = getline(&lineptr, &n, stream)) > 0){
    //printf("Line %i: %s", n, lineptr);
    if(linebuffer_size < bufferoffset + read){
      linebuffer_size = bufferoffset + read;
      linebuffer = realloc(linebuffer, linebuffer_size);
    }
    memcpy(linebuffer + bufferoffset, lineptr, read);
    bufferoffset += read;
    for(ssize_t i = 0; i < read; i++){
      if(lineptr[i] == '\"' && (i + 1) < read){
	if(lineptr[i + 1] == '\"'){
	  i++;
	}else{
	  inquote = !inquote;
	}
      }
    }
    if(!inquote){
      linebuffer[bufferoffset] = 0;
      if(bufferoffset >0){
	if(linebuffer[bufferoffset - 1] == '\n')
	  linebuffer[bufferoffset - 1] = 0;
      }
      printf("buffer: %s\n", linebuffer);
      icy_eval(linebuffer);
      bufferoffset = 0;
    }else{
      
    }
    
  }
  dealloc(lineptr);
  dealloc(linebuffer);
}

int main(int argc, char ** argv){
  bool test = false;
  bool repl = false;
  bool help = false;
  char * path = NULL;
  for(int i = 0; i < argc; i++){

    bool isarg(const char * arg){
      return strcmp(argv[i], arg) == 0;
    }
    
    if(isarg("--test"))
      test = true;
    else if (isarg("--repl"))
      repl = true;
    else if (isarg("--help"))
      help = true;
    else if(isarg("--file")){
      i++;
      if(i < argc){
	path = argv[i];
      }
    }
  }

  if(help){
    printf("--test - run tests\n--help  -  show this help\n");
    return 0;
  }
  icy_context_make_current(icy_context_initialize());
  
  if(path != NULL){
    var f = fopen(path, "r");
    if(f == NULL){
      ERROR("could not read file");
    }
    eval_by_line(f);
    fclose(f);
  }
  
  if(test){
    run_tests();
    return 0;
  }
  if(repl){
    eval_by_line(stdin);
    printf("Implement repl\n");
    return 0;
  }
  
  return 0;
}


