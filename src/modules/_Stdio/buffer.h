struct _Buffer
{
  unsigned char *buffer;

  size_t offset; /* reading */
  size_t len, allocated; /* writing */

  struct object *sub, *source, *this;
  struct program *error_mode;
  struct svalue output;
  struct pike_string *str;


  INT_TYPE num_malloc, num_move; /* debug mainly, for testsuite*/
  INT32 locked, locked_move;
  float max_waste;
  char malloced;
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

PMOD_EXPORT void io_ensure_malloced( Buffer *io, size_t bytes );
PMOD_EXPORT unsigned char *io_add_space_do_something( Buffer *io, size_t bytes, int force );
PMOD_EXPORT Buffer *io_buffer_from_object(struct object *o);
PMOD_EXPORT void io_trim( Buffer *io );
PMOD_EXPORT void io_actually_trigger_output( Buffer *io );

PIKE_UNUSED_ATTRIBUTE
static size_t io_len( Buffer *io )
{
  return io->len-io->offset;
}

PIKE_UNUSED_ATTRIBUTE
static unsigned char *io_read_pointer(Buffer *io)
{
  return io->buffer + io->offset;
}

PIKE_UNUSED_ATTRIBUTE
static unsigned char *io_add_space( Buffer *io, size_t bytes, int force )
{
  if( io->len == io->offset )
    io->offset = io->len = 0;
  if( !force && io->malloced && !io->locked && io->len+bytes < io->allocated &&
      (!bytes || io->len+bytes > io->len))
    return io->buffer+io->len;
  return io_add_space_do_something( io, bytes, force );
}

PIKE_UNUSED_ATTRIBUTE
static INT_TYPE io_consume( Buffer *io, ptrdiff_t num )
{
  io->offset += num;
  return io_len(io);
}

PIKE_UNUSED_ATTRIBUTE
static void io_trigger_output( Buffer *io )
{
  if( UNLIKELY(io->output.u.object) )
    io_actually_trigger_output(io);
}

