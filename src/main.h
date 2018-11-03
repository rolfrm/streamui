
typedef size_t object_id;
typedef size_t type_id;
typedef size_t int_str; 

struct _string_hash_table;
typedef struct _string_hash_table string_hash_table;

string_hash_table * intern_string_init();

int_str intern_string(string_hash_table * tbl, const char * string);
size_t intern_string_read(string_hash_table * tbl, int_str str, char * buffer, size_t buflen);

size_t intern_string_count(string_hash_table * s);
size_t string_table_count(string_hash_table * tbl);
size_t string_hash(const char * buffer);
size_t data_hash(const void * buffer, size_t len);
