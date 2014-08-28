struct _IOBuffer
{
  unsigned char *buffer;

  size_t offset; /* reading */
  size_t len, allocated; /* writing */

  struct object *sub;
  struct pike_string *str;
  char malloced, error_mode;
  INT32 locked;
};
typedef struct _IOBuffer IOBuffer;

extern void init_stdio_buffer(void);
extern void exit_stdio_buffer(void);
