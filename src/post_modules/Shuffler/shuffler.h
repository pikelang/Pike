struct data
{
  int len, do_free, off;
  char *data;
};

struct source
{
  struct source *next;
  int eof;

  /* Must be implemented by all sources */
  struct data (*get_data)(struct source *s,int len);
  void (*free_source)(struct source *s);

  /* These can be defined in any source, however, they are mostly
   * useful for nonblocking sources.
   */
  void (*setup_callbacks)(struct source *s);
  void (*remove_callbacks)(struct source *s);

  /* The following is only used by nonblocking sources. A nonblocking
   * source is defined as a source that returns a data struct from
   * get_data with a 'len' value of -2.
   */
  void (*set_callback)( struct source *s, void (*cb)( void *a ), void *a );
};


typedef enum
{
  INITIAL,
  RUNNING,
  PAUSED,
  DONE,
  WRITE_ERROR,
  READ_ERROR,
  USER_ABORT,
} ShuffleState;
