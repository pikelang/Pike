/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: shuffler.h,v 1.6 2004/10/16 07:27:29 agehall Exp $
*/

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
