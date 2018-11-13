#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
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
  double height;
  
  icy_query("/button1/height", &height);
  ASSERT(height == 30.0);
  icy_eval("/style/button/height 35.0");
  icy_query("/button1/height", &height);
  ASSERT(height == 35.0);
  
  icy_eval("/button1/width 15");
  icy_eval("/button1/width2/asd 1");

  icy_query("/button1/width", &height);
  printf("Height: %f\n", height);
  ASSERT(height == 15.0);
  
  icy_eval("/button1/height 20");

  icy_query("/button1/height", &height);
  ASSERT(height == 20.0);
  icy_query("/style/button/height", &height);
  ASSERT(height == 30.0);
  
  
  //object_id * buf = NULL;
  //size_t buf_size = 0;
  //icy_bytecode("/button1/height",(void **)  &buf, &buf_size);
  //printf("Bytecode buffer size: %i/%i\n", buf[0], buf[1]);
}


void run_tests(){
  icy_context_make_current(icy_context_initialize());
  test_hashing();
  test_string_intern();
  test_eval();
  print_scope();
  printf("done test\n");
}
