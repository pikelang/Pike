struct _Buffer
{
  unsigned char *buffer;

  size_t offset; /* reading */
  size_t len, allocated; /* writing */

  struct object *this;
  struct program *error_mode;
  struct _Buffer *child;

  union {
    struct pike_string *str;	/* PIKE_T_STRING */
    struct object *obj;		/* PIKE_T_OBJECT */
    struct _Buffer *parent;	/* PIKE_T_RING */
  } source;
  INT32 sourcetype;		/* type for the source union */
  INT32 locked_move;
  float max_waste;

  struct svalue output;

#ifdef PIKE_DEBUG
  INT_TYPE num_malloc, num_move; /* debug mainly, for testsuite*/
#endif
};

struct rewind_to {
    struct _Buffer *io;
    size_t rewind_to;
#ifdef PIKE_DEBUG
    int old_locked_move;
#endif
};

typedef struct _Buffer Buffer;

void init_stdio_buffer(void);
void exit_stdio_buffer(void);
PMOD_EXPORT unsigned char *io_add_space_do_something( Buffer *io, size_t bytes, int force );
PMOD_EXPORT void io_actually_trigger_output( Buffer *io );

PMOD_EXPORT Buffer *io_buffer_from_object(struct object *o);

PIKE_UNUSED_ATTRIBUTE
static size_t io_len( Buffer *io )
{
  return io->len - io->offset;
}

PIKE_UNUSED_ATTRIBUTE
static unsigned char *io_read_pointer(Buffer *io)
{
  return io->buffer + io->offset;
}

PIKE_UNUSED_ATTRIBUTE
static unsigned char *io_add_space( Buffer *io, size_t bytes, int force )
{
  if (LIKELY(!io->child)) {
    if (UNLIKELY(io->len == io->offset) && LIKELY(!io->locked_move))
      io->offset = io->len = 0;
    if (LIKELY(!force && io->len+bytes < io->allocated
            && io->len+bytes >= io->len))
      return io->buffer+io->len;
  }
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
  if (UNLIKELY(TYPEOF(io->output) == PIKE_T_OBJECT))
    io_actually_trigger_output(io);
}
