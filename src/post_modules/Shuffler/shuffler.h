/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

struct data
{
  int len;
  char *data;
};

struct source
{
  struct source *next;
  int eof;
  int dataref;	/* points to the last connected iov chunk */

  struct svalue wrap_callback;
  struct svalue wrap_array;	/* the last return value from wrap_callback */

  /* Must be implemented by all sources */
  struct data (*get_data)(struct source *s,off_t len);
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
  void (*set_callback)( struct source *s, void (*cb)( void *a ),
    struct object *a );
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
