/* Not currently used, If i make one, I will make a _real_ one */

#define SHM_HASH_SIZE 255

typedefe struct {
  char data[4096]; /* Max string len, currently. Sad, but true. */
  int len;
} value_string;

typedefe struct {
  char data[16]; 
/* Max string len, currently. Sad, but true. This is due to the method used
 * for the sharing of the memory (there is no _real_ shm_free() call, so we 
 * cannot just free some memory. It could be solved by using the mmap librabry
 */
  int len;
} key_string;


typedef struct {
  enum { STRING, INTEGER } type;
  union {
    key_string str;
    int integer;
  } u;
} key_thing;

typedef struct {
  enum { STRING, INTEGER } type;
  union {
    value_string str;
    int integer;
  } u;
} value_thing;

typedef struct
{
  key_thing key;
  value_thing value;
  thing *next;
} item;

struct shmmap {
  item *item_hash_table[ SHM_HASH_SIZE ];
};


int shm_hash_thing(thing *what)
{
  int i, res;
  switch(what->type)
  {
   case TYPE_STRING:
    for(i=0; i<what->u.str.len; i++)
      res += what->u.str.data[i];
    return res & 255;
   case TYPE_INTEGER:
    return what->u.integer & 255;
  }
}


struct item *shm_find_index(thing *value)
{
  int i = shm_hash_thing( value );
  item *ths;
  
  for(;ths;ths=ths->next)
    if(ths->type == value->type)
      if((value->type == TYPE_INTEGER)
	 && (ths->u.integer == value->u.integer))
	return ths;
      else if((value->type == TYPE_STRING)
	      && (ths->u.str.len == value->u.str.len)
	      && !MEMCMP(ths->u.str.data, value->u.str.data, value->u.str.len))
	return ths;
  return 0;
}

