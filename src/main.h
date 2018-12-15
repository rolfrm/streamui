
typedef size_t object_id;
typedef size_t type_id;
typedef size_t int_str; 

struct _icy_type;
typedef struct _icy_type icy_type;
typedef struct _icy_object icy_object;

struct _string_hash_table;
typedef struct _string_hash_table string_hash_table;
typedef struct _icy_context icy_context;

// temporary data structure used for holding objects on the stack.
struct _icy_object{
  union
  {
    void * userdata;
    size_t id;
  };

  const icy_type * type;
  icy_object * parent;
  
};


// temporary data structure for iterating the children of an object.
typedef struct{
  union {
    void * localdata;
    size_t number;
  };
  icy_object object;
}icy_iter;


// Data structure used to hold information about a type.
// Type is the type of an object and it defines how to work with the data.
struct _icy_type{
  // custom data for the type to use.
  void * userdata;
  
  // write the value to the out pointer. Important for queries.
  void (* get_value)(icy_object * obj, void * out);
  void (* print)(icy_object *);
  void (* try_parse)(icy_object * obj, char ** str);
  bool (* sub_next)(icy_object *id, icy_iter * iter);
  void (* sub_parse)(icy_object *id, char * str, icy_object * out);
  void * (* data)(icy_object * parent, size_t * required_size);

  object_id (* sub_parse2)(icy_object * obj, char * str);
  void (* sub_get)(icy_object *obj, object_id id, icy_object * out);
  size_t (* sub_encode)(icy_object * obj, void * data, size_t buffer_size);
  size_t (* sub_decode)(icy_object * obj, void * data);
  type_id type_id;
};


string_hash_table * intern_string_init();

int_str intern_string(string_hash_table * tbl, const char * string);
size_t intern_string_read(string_hash_table * tbl, int_str str, char * buffer, size_t buflen);

size_t intern_string_count(string_hash_table * s);
size_t string_table_count(string_hash_table * tbl);
size_t string_hash(const char * buffer);
size_t data_hash(const void * buffer, size_t len);
void run_tests();
void icy_eval(const char * cmd);
void icy_query(const char * cmd, void * outdata);
void icy_query2(icy_object * obj, const char * cmd, void * outdata);
void icy_query_object(const char * cmd, icy_object * obj);
void icy_context_make_current(icy_context * _ctx);
void print_scope();
icy_context * icy_context_initialize();
void icy_bytecode(const char * cmd, void ** bytecode, size_t * bufsize);
void print_object(icy_object * thing);
void type_print(icy_object * obj);
