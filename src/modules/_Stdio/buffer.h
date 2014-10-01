struct _Buffer
{
  unsigned char *buffer;

  size_t offset; /* reading */
  size_t len, allocated; /* writing */

  struct object *sub, *source, *this;
  struct program *error_mode;
  struct object *output;
  struct pike_string *str;

  struct {
      unsigned char *ptr;
      size_t len;
  } stash;

  INT_TYPE num_malloc, num_move; // debug mainly, for testsuite
  INT32 locked, locked_move;
  char malloced, output_triggered;
};

struct rewind_to {
    struct _Buffer *io;
    size_t rewind_to;
#ifdef PIKE_DEBUG
    int old_locked_move;
#endif
};

typedef struct _Buffer Buffer;

extern void init_stdio_buffer(void);
extern void exit_stdio_buffer(void);
