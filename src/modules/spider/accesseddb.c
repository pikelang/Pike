#include "stralloc.h"
#include "global.h"
#include "types.h"
#include "macros.h"
#include "backend.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "mapping.h"
#include "array.h"
#include "builtin_functions.h"

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

#include <sys/stat.h>
#include <fcntl.h>

#define COOKIE 0x11223344

/* Must be (2**n)-1 */
#define CACHESIZE 2047

#define BASE_OFFSET 48

struct string {
  unsigned int len;
  unsigned int hval; /* Hash value. Used in string comparison */
  char s[1]; /* Expands. MUST BE LAST IN STRUCT */
};

typedef struct node
{
  /* THEWSE TWO MUST BE FIRST (optimization in write_entry) */
  INT32 hits;
  INT32 mtime;

  unsigned INT32 offset;
  INT32 creation_time;
  INT32 reserved_1;
  INT32 reserved_2;

  struct string name; /* MUST BE LAST */
} node_t;


struct file_head {
  unsigned int cookie;
  unsigned int creation_time;
  unsigned int next_free;

  unsigned int reserved[8];
  /* Not saved */
  int fd, fast_hits, other_hits, cache_conflicts;
  node_t *cache_table[CACHESIZE+1];
};

#define FUZZ 60
#define THIS ((struct file_head *)(fp->current_storage))
#define N_SIZE(S) (((MAX((S).len,64)+sizeof(struct node)-1)/8+1)*8)
#define HASH(X) MAX(((int)(((X)>>10)^(X))&CACHESIZE)-FUZZ,0)


static node_t *entry(int offset, struct file_head *this)
{
  node_t *me;
  int amnt;

  if(lseek(this->fd, offset+BASE_OFFSET, SEEK_SET)<0) {
    perror("Cannot seek");
    return 0;
  }

  me = malloc(sizeof(node_t)+63);
  amnt=read(this->fd, me, sizeof(node_t)+63);
  if((amnt <= 0) || (amnt < (int)(sizeof(node_t)+63)))
  {
    perror("Failed to read full entry");
    free(me);
    return NULL;
  }
  if(me->name.len > 64)
  {
    int wl = sizeof(node_t)+me->name.len;
    free(me);
    me = malloc(wl);
    amnt=read(this->fd, me, wl);
    if(amnt <= 0 || amnt < wl)
    {
      fprintf(stderr, "read only %d bytes, wanted %d\n", amnt, wl);
      free(me);
      return NULL;
    }
  }
  me->name.s[me->name.len]=0;
  return me;
}

static void save_head( struct file_head *this )
{
  if(lseek(this->fd, 0, SEEK_SET)<0) return;
  write(this->fd, this, 32);
}


static void load_head( struct file_head *this )
{
  if(lseek(this->fd, 0, SEEK_SET)<0) return;
  read(this->fd, this, 32);
}

#define MAX(a,b) ((a)<(b)?(b):(a))

static void write_entry( node_t *e, struct file_head *this, int shortp )
{
  if(lseek(this->fd, e->offset+BASE_OFFSET, SEEK_SET)<0)
  {
    perror("Cannot seek");
    return;
  }
  if(shortp)
    write(this->fd, e, 8); /* <--- */
  else
    write(this->fd, e, sizeof(node_t)+MAX(e->name.len, 64)-1);
}

static void free_entry( node_t *e )
{
  free( e );
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

  me = malloc(sizeof(node_t)+file_name->len-1+64);
  me->creation_time = t;
  me->hits=0;
  me->mtime = t;
  MEMCPY(&me->name, file_name, sizeof(string)+file_name->len);
  me->offset = pos;
  write_entry( me, this, 0 );

  if(this->cache_table[HASH(me->name.hval)])
    free_entry(this->cache_table[HASH(me->name.hval)]);
  this->cache_table[HASH(me->name.hval)]=me;
  return me;
}

static node_t *slow_find_entry(struct string *name, struct file_head *this)
{
  unsigned int i;
  struct node *me;
  for(i=0; i<this->next_free; )
  {
    if(!(me=entry(i,this))) return 0;
    if((me->name.hval==name->hval) && (me->name.len==name->len)
       && !strncmp(me->name.s, name->s, name->len))
    {
      int of = HASH(name->hval), oof=of;
      while(this->cache_table[of]) {
	of++;
	if(of>CACHESIZE || (of-oof>FUZZ))
	  break;
      }
      if(of<=CACHESIZE && (of-oof)<=FUZZ)
	this->cache_table[of]=me;
      else {
	this->cache_conflicts++;
	free_entry(this->cache_table[oof]);
	this->cache_table[oof]=me;
      }
      return me;
    }
    i+=N_SIZE(me->name);
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
  if(me = fast_find_entry( name, this ))
  {
    this->fast_hits++;
    return me;
  }
  if(me=slow_find_entry( name, this ))
  {
    this->other_hits++;
    return me;
  }
  return 0;
}

static struct string *make_string(struct svalue *s)
{
  struct string *res;
  if(s->type != T_STRING) return 0;
  res = malloc(sizeof(struct string) + s->u.string->len-1);
  res->len = s->u.string->len;
  memcpy(res->s, s->u.string->str, res->len);
  res->hval = hashmem(res->s, (INT32)res->len, (INT32)res->len);
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

  f_aggregate_mapping(6);
}

static void f_add( INT32 args )
{
  node_t *me;
  struct string *s;

  if(!THIS->fd) error("No open accessdb.\n");
  
  if(args<2) error("Wrong number of arguments to add(string fname,int num)\n");

  if(!(s=make_string(sp-args)))
    error("Wrong type of argument to add(string fname,int num)\n");

  if(!(me = find_entry( s, THIS ))) me=new_entry( s, THIS );
  if(!me) error("Failed to create entry.\n");

  if(sp[-1].u.integer)
  {
    me->hits += sp[-1].u.integer;
    me->mtime = current_time.tv_sec;
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
  push_text("conflicts");
  push_int( THIS->cache_conflicts );
  THIS->fast_hits=0;
  THIS->other_hits=0;
  THIS->cache_conflicts=0;
  pop_n_elems(args);
  f_aggregate_mapping( 6 );
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

void init_accessdb_program()
{
   start_new_program();
   add_storage(sizeof(struct file_head));
   add_function("create", f_create, "function(string:void)",
		OPT_EXTERNAL_DEPEND);
   add_function("add", f_add, "function(string,int:mapping(string:int))",
		OPT_EXTERNAL_DEPEND|OPT_SIDE_EFFECT);
   add_function("debug", f_debug, "function(void:mapping)",OPT_EXTERNAL_DEPEND);
   set_init_callback(init_file_head);
   set_exit_callback(free_file_head);
   end_c_program("/precompiled/AccessDB")->refs++;
}

void exit_accessdb_program()
{
}
