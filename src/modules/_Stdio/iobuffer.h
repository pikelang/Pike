struct _IOBuffer
{
  unsigned char *buffer;

  size_t offset; /* reading */
  size_t len, allocated; /* writing */

  struct object *sub, *source, *this;
  struct program *error_mode;
  struct object *output;
  struct pike_string *str;
  INT32 locked;
  char malloced, output_triggered;
};
typedef struct _IOBuffer IOBuffer;

extern void init_stdio_buffer(void);
extern void exit_stdio_buffer(void);
