/*
 * $Id: accesseddb.c,v 1.18 1999/02/10 21:54:20 hubbe Exp $
 */

#include "global.h"

#include "config.h"

#include "stralloc.h"
#include "pike_macros.h"
#include "backend.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "mapping.h"
#include "array.h"
#include "error.h"
#include "builtin_functions.h"

RCSID("$Id: accesseddb.c,v 1.18 1999/02/10 21:54:20 hubbe Exp $");

#include <stdio.h>

#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif

#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_SYS_CONF_H
#include <sys/conf.h>
#endif

#ifdef HAVE_STROPTS_H
#include <stropts.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif /* HAVE_SYS_FILE_H */

#include <sys/stat.h>
#include <fcntl.h>

#include "dmalloc.h"
#include "accesseddb.h"

#ifndef SEEK_SET
#ifdef L_SET
#define SEEK_SET	L_SET
#else /* !L_SET */
#define SEEK_SET	0
#endif /* L_SET */
#endif /* SEEK_SET */
#ifndef SEEK_CUR
#ifdef L_INCR
#define SEEK_SET	L_INCR
#else /* !L_INCR */
#define SEEK_CUR	1
#endif /* L_INCR */
#endif /* SEEK_CUR */
#ifndef SEEK_END
#ifdef L_XTND
#define SEEK_END	L_XTND
#else /* !L_XTND */
#define SEEK_END	2
#endif /* L_XTND */
#endif /* SEEK_END */


#define COOKIE 0x11223344
/* Must be (2**n)-1 */
#define CACHESIZE 2047

#define BASE_OFFSET 48
#define BUFFLEN 8192
#define FUZZ 60
#undef THIS
#define THIS ((struct file_head *)(fp->current_storage))
#define N_SIZE(S) (((MAX((S).len,SHORT_FILE_NAME)+sizeof(struct node)-1)/8+1)*8)
#define HASH(X) MAX(((int)(((X)>>10)^(X))&CACHESIZE)-FUZZ,0)
#define SHORT_FILE_NAME 16


struct string {
  unsigned int len;
  unsigned int hval; /* Hash value. Used in string comparison */
  char s[1]; /* Expands. MUST BE LAST IN STRUCT */
};

typedef struct node
{
  /* THEESE TWO MUST BE FIRST (optimization in write_entry) */
  INT32 hits;
  INT32 mtime;

  unsigned INT32 offset;
  INT32 creation_time;
  INT32 val1;
  INT32 val2;

  struct string name; /* MUST BE LAST */
} node_t;


struct file_head {
  unsigned int cookie;
  unsigned int creation_time;
  unsigned int next_free;

  unsigned int reserved[8];
  /* Not saved */
  int fd, fast_hits, other_hits, cache_conflicts, misses;
  char buffer[BUFFLEN];
  unsigned int buffer_inited;
  unsigned int bpos; /* Start of the buffer */
  unsigned int pos;      /* Current position in file */
  node_t *cache_table[CACHESIZE+1];
};

int mread(struct file_head *this, char *data, int len, int pos)
{
  int buffert_pos = this->pos-this->bpos;
  this->pos = pos;
  if(this->buffer_inited
     && (this->pos>this->bpos)
     && (buffert_pos+len<BUFFLEN))
  {
    MEMCPY(data, this->buffer+buffert_pos, len);
    return len;
  }

  this->bpos = this->pos;
  lseek(this->fd, this->pos, SEEK_SET);
  read(this->fd, this->buffer, BUFFLEN);
  this->buffer_inited = 1;

  MEMCPY(data, this->buffer, len);
  return len;
}

void mwrite(struct file_head *this, char *data, int len, int pos)
{
  this->pos = pos;
  lseek(this->fd, this->pos, SEEK_SET);
  write(this->fd, data, len);
}

static node_t *entry(int offset, struct file_head *this)
{
  node_t *me = malloc(sizeof(node_t)+63);

  mread(this, (char *)me, sizeof(node_t)+63,offset+BASE_OFFSET);
  if(me->name.len > SHORT_FILE_NAME)
  {
    int wl = sizeof(node_t)+me->name.len;
    free(me);
    me = malloc(wl);
    mread(this,(char *)me,wl,offset+BASE_OFFSET);
  }
  me->name.s[me->name.len]=0;
  return me;
}

static void save_head( struct file_head *this )
{
  mwrite(this, (char *)this, 32, 0);
}

static void load_head( struct file_head *this )
{
  mread(this, (char *)this, 32, 0);
}

#ifndef MAX
#define MAX(a,b) ((a)<(b)?(b):(a))
#endif

static void write_entry( node_t *e, struct file_head *this, int shortp )
{
  if(shortp)
    mwrite(this,(char*)e,8,e->offset+BASE_OFFSET);
  else
    mwrite(this,(char*)e,sizeof(node_t)+MAX(e->name.len, SHORT_FILE_NAME)-1,
	   e->offset+BASE_OFFSET);
}

static void free_entry( node_t *e )
{
  free( e );
}

void insert_in_cache(node_t *me, struct file_head *this)
{
  int of = HASH(me->name.hval), oof=of;
  while(this->cache_table[of]) {
    of++;
    if(of>CACHESIZE || (of-oof>FUZZ))
      break;
  }
  if(of<=CACHESIZE && (of-oof)<=FUZZ)
    this->cache_table[of]=me;
  else {
    if(this->cache_table[oof]->hits < me->hits)
    {
      this->cache_conflicts++;
      free_entry(this->cache_table[oof]);
      this->cache_table[oof]=me;
    }
  }
}


static node_t *new_entry(struct string *file_name, struct file_head *this)
{
  int entry_size = N_SIZE(*file_name);
  node_t *me;
  unsigned int pos;
  int t = current_time.tv_sec;

  pos = this->next_free;
  this->next_free += entry_size;
  save_head(this);

  me = malloc(sizeof(node_t)+file_name->len-1+SHORT_FILE_NAME);
  me->creation_time = t;
  me->hits=0;
  me->mtime = t;
  MEMCPY(&me->name, file_name, sizeof(string)+file_name->len);
  me->offset = pos;
  write_entry( me, this, 0 );
  
  insert_in_cache(me,this);
  return me;
}

static node_t *slow_find_entry(struct string *name, struct file_head *this)
{
  unsigned int i,j=0;
  struct node *me;

  if(!this->next_free) return 0; /* Empty */

  /* Start with the _next_ entry. */
  i=this->pos?(this->pos-BASE_OFFSET):0;
  me=entry(i,this);
  i+=N_SIZE(me->name);
  if(i>=this->next_free) i=0;


  /* Loop until found, if found, insert in cache, otherwise, return 0. */

  while(j<(this->next_free-BASE_OFFSET))
  {
    me=entry(i,this);
    if((me->name.hval==name->hval)
       && (me->name.len==name->len)
       && !strncmp(me->name.s, name->s, name->len))
    {
      /* MATCH Insert in table */
      insert_in_cache(me,this);
      return me;
    }
    /* Step forward to the next entry */

    i+=N_SIZE(me->name);
    j+=N_SIZE(me->name);

    free_entry(me);
    if(i>=this->next_free) i=0;
  }
  return NULL;
}

static node_t *fast_find_entry(struct string *name, struct file_head *this)
{
  int of = (HASH(name->hval)), oof;
  oof=of;
  while(this->cache_table[of])
  {
    if((this->cache_table[of]->name.hval == name->hval)
       && (name->len==this->cache_table[of]->name.len)
       && !strncmp(this->cache_table[of]->name.s, name->s, name->len))
      return this->cache_table[of];
    of++;
    if(of>CACHESIZE || (of-oof>FUZZ))
      break;
  }
  return NULL;
}


static node_t *find_entry(struct string *name, struct file_head *this)
{
  struct node *me;
  if((me = fast_find_entry( name, this )))
  {
    this->fast_hits++;
    return me;
  }
  if((me=slow_find_entry( name, this )))
  {
    this->other_hits++;
    return me;
  }
  this->misses++;
  return 0;
}

static struct string *make_string(struct svalue *s)
{
  struct string *res;
  if(s->type != T_STRING) return 0;
  res = malloc(sizeof(struct string) + s->u.string->len-1);
  res->len = s->u.string->len;
  MEMCPY(res->s, s->u.string->str, res->len);
  res->hval = hashmem((unsigned char *)res->s, (INT32)res->len,
		      (INT32)res->len);
  return res;
}

static void new_head( struct file_head *this )
{
  this->cookie = COOKIE;
  this->creation_time = current_time.tv_sec;
  this->next_free=0;
}

static void f_create(INT32 args)
{
  if(!args) error("Wrong number of arguments to create(string fname)\n");

  if(sp[-args].type != T_STRING)
    error("Wrong type of argument to create(string fname)\n");

  THIS->fd = open( (char *)sp[-args].u.string->str, O_RDWR|O_CREAT, 0666 );
  if(THIS->fd < 0)
  {
    THIS->fd=0;
    error("Failed to open db.\n");
  }
  load_head( THIS );
  if(!THIS->cookie)
  {
    new_head( THIS );
    save_head( THIS );
  } else if(THIS->cookie != COOKIE) {
    error("Wrong magic cookie. File created on computer with"
	  " different byteorder?\n");
    close(THIS->fd);
    THIS->fd = 0;
    THIS->cookie=0;
  }
}


static void push_entry( node_t *t )
{
  push_text("hits");
  push_int(t->hits);

  push_text("mtime");
  push_int(t->mtime);

  push_text("creation_time");
  push_int(t->creation_time);

  push_text("value_1");
  push_int(t->val1);

  push_text("value_2");
  push_int(t->val2);

  f_aggregate_mapping(10);
}

static void f_add( INT32 args )
{
  node_t *me;
  int modified = 0;
  struct string *s;

  if(!THIS->fd) error("No open accessdb.\n");
  
  if(args<2) error("Wrong number of arguments to add(string fname,int num[, int arg1, int arg2])\n");

  if(!(s=make_string(sp-args)))
    error("Wrong type of argument to add(string fname,int num)\n");

  if(!(me = find_entry( s, THIS ))) me=new_entry( s, THIS );
  if(!me) error("Failed to create entry.\n");

  if(sp[-1].u.integer)
  {
    me->hits += sp[-1].u.integer;
    me->mtime = current_time.tv_sec;
    modified=1;
  }
  if(args>2)
  {
    me->val1 = sp[-args+2].u.integer;
    if(args>3)
      me->val2 = sp[-args+3].u.integer;
    me->mtime = current_time.tv_sec;
    write_entry( me, THIS, 0);
  } else if(modified)
    write_entry( me, THIS, 1);

  pop_n_elems( args );
  push_entry( me );
  free(s);
}

static void f_set( INT32 args )
{
  node_t *me;
  int modified = 0;
  struct string *s;

  if(!THIS->fd) error("No open accessdb.\n");
  
  if(args<2) error("Wrong number of arguments to set(string fname,int num[, int arg1, int arg2])\n");

  if(!(s=make_string(sp-args)))
    error("Wrong type of argument to set(string fname,int num,...)\n");

  if(!(me = find_entry( s, THIS ))) me=new_entry( s, THIS );
  if(!me) error("Failed to create entry.\n");

  if(sp[-1].u.integer)
  {
    me->hits = sp[-1].u.integer;
    me->mtime = current_time.tv_sec;
    modified=1;
  }
  if(args>2)
  {
    me->val1 = sp[-args+2].u.integer;
    if(args>3)
      me->val2 = sp[-args+3].u.integer;
    write_entry( me, THIS, 0);
  } else if(modified)
    write_entry( me, THIS, 1);

  pop_n_elems( args );
  push_entry( me );
  free(s);
}


static void f_new( INT32 args )
{
  node_t *me;
  struct string *s;

  if(!THIS->fd) error("No open accessdb.\n");
  
  if(args<2) error("Wrong number of arguments to new(string fname,int num[, int val1, int val2])\n");

  if(!(s=make_string(sp-args)))
    error("Wrong type of argument to new(string fname,int num[, int val1, int val2])\n");

  me=new_entry( s, THIS );
  if(!me) error("Failed to create entry.\n");

  if(sp[-1].u.integer)
  {
    me->hits = sp[-args+1].u.integer;
    me->mtime = current_time.tv_sec;
    if(args>2)
    {
      me->val1 = sp[-args+2].u.integer;
      if(args>3)
	me->val2 = sp[-args+3].u.integer;
      write_entry( me, THIS, 0);
    } else 
      write_entry( me, THIS, 1);
  }
  pop_n_elems( args );
  push_entry( me );
  free(s);
}


static void f_debug( INT32 args )
{
  push_text("cachehits");
  push_int( THIS->fast_hits );
  push_text("slowhits");
  push_int( THIS->other_hits );
  push_text("misses");
  push_int( THIS->misses );
  push_text("conflicts");
  push_int( THIS->cache_conflicts );
  THIS->fast_hits=0;
  THIS->other_hits=0;
  THIS->misses=0;
  THIS->cache_conflicts=0;
  pop_n_elems(args);
  f_aggregate_mapping( 8 );
}

static void init_file_head(struct object *o)
{
  MEMSET(THIS, 0, sizeof(struct file_head));
}

static void free_file_head(struct object *o)
{
  int i;
  if(THIS->fd) close(THIS->fd);
  for(i=0; i<CACHESIZE; i++)
    if(THIS->cache_table[i])
      free_entry(THIS->cache_table[i]);
}

void init_accessdb_program(void)
{
   start_new_program();
   ADD_STORAGE(struct file_head);
   /* function(string:void) */
  ADD_FUNCTION("create", f_create,tFunc(tStr,tVoid), ID_PUBLIC);
   /* function(string,int ...:mapping(string:int)) */
  ADD_FUNCTION("add", f_add,tFuncV(tStr,tInt,tMap(tStr,tInt)),
		ID_PUBLIC);
   /* function(string,int ...:mapping(string:int)) */
  ADD_FUNCTION("set", f_set,tFuncV(tStr,tInt,tMap(tStr,tInt)),
		ID_PUBLIC);
   /* function(string,int ...:mapping(string:int)) */
  ADD_FUNCTION("new", f_new,tFuncV(tStr,tInt,tMap(tStr,tInt)),
		ID_PUBLIC);
   /* function(void:mapping) */
  ADD_FUNCTION("debug", f_debug,tFunc(tVoid,tMapping), ID_PUBLIC);
   set_init_callback(init_file_head);
   set_exit_callback(free_file_head);
   end_class("accessdb",0);
}

void exit_accessdb_program(void)
{
}
