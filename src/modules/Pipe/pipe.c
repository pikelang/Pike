#include <config.h>
#include "machine.h"

#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#else
#ifdef HAVE_LINUX_MMAN_H
#include <linux/mman.h>
#else
#ifdef HAVE_MMAP
/* sys/mman.h is _probably_ there anyway. */
#include <sys/mman.h>
#endif
#endif
#endif

#include <fcntl.h>

#include "global.h"
RCSID("$Id: pipe.c,v 1.7 1997/05/07 23:07:17 per Exp $");

#include "stralloc.h"
#include "types.h"
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "error.h"
#include "builtin_functions.h"

#ifndef S_ISREG
#ifdef S_IFREG
#define S_ISREG(mode)   (((mode) & (S_IFMT)) == (S_IFREG))
#else
#define S_ISREG(mode)   (((mode) & (_S_IFMT)) == (_S_IFREG))
#endif
#endif

/*
#define PIPE_STRING_DEBUG
#define PIPE_MMAP_DEBUG
#define PIPE_FILE_DEBUG
#define BLOCKING_CLOSE
*/

#if 0
#define INSISTANT_WRITE
#endif

#ifndef MAP_FILE
#  define MAP_FILE 0
#endif

#define READ_BUFFER_SIZE 32768
#define MAX_BYTES_IN_BUFFER 65536

/*
 *  usage:
 *  single socket output  
 *  or regular file output and (multiple, adding) socket output 
 *     with no mmap input 
 *
 *  multiple socket output without regular file output illegal
 */

static struct program *pipe_program, *output_program;

#define THIS ((struct pipe *)(fp->current_storage))
#define THISOBJ (fp->current_object)

struct input
{
  enum { I_NONE,I_OBJ,I_STRING,I_MMAP } type;
  union
  {
    struct object *obj; 
    struct pike_string *str;
    char *mmap;
  } u;
  unsigned long len;		/* current input: string or mmap len */
  int set_blocking_offset, set_nonblocking_offset;
  struct input *next;
};

struct output
{
  struct object *obj;
  int write_offset, set_blocking_offset, set_nonblocking_offset;
  int fd;
      
  enum 
  { 
    O_RUN,			/* waiting for callback */
    O_SLEEP			/* sleeping; waiting for more data */
  } mode;

  unsigned long pos; /* position in buffer */
  struct object *next;
  struct pipe *the_pipe;
};

struct buffer
{
  struct pike_string *s;
  struct buffer *next;
};

struct pipe
{
  int living_outputs;		/* number of output objects */

  struct svalue done_callback;
  struct svalue output_closed_callback;
  struct svalue id;

  /*
   *  if fd is -1: use fd
   *  else firstinput's type is I_MMAP: use firstinput's mmap
   *  else use buffer
   */

  int fd;			/* buffer fd or -1 */

  unsigned long bytes_in_buffer;
  unsigned long pos; 
  /* fd: size of buffer file */
  /* current position of first element (buffer or mmap) */
  struct buffer *firstbuffer,*lastbuffer;
  short sleeping;			/* sleeping; buffer is full */
  short done;
  struct input *firstinput,*lastinput;
  struct object *firstoutput;
};

static int offset_input_read_callback;
static int offset_input_close_callback;
static int offset_output_write_callback;
static int offset_output_close_callback;
static int mmapped, nobjects, nstrings, noutputs, ninputs, nbuffers, sbuffers;

static char static_buffer[READ_BUFFER_SIZE];

void close_and_free_everything(struct object *o,struct pipe *);
static INLINE void output_finish(struct object *obj);
static INLINE void output_try_write_some(struct object *obj);

/********** internal ********************************************************/

/* Push a callback to this object given the internal function number.
 */
static void push_callback(int no)
{
  sp->u.object=THISOBJ;
  THISOBJ->refs++;
  sp->subtype=no+fp->context.identifier_level;
  sp->type=T_FUNCTION;
  sp++;
}

/* Allocate a new struct input, link it last in the linked list */
static INLINE struct input *new_input(void)
{
  struct input *i;
  ninputs++;
  i=ALLOC_STRUCT(input);
  i->type=I_NONE;
  i->next=NULL;
  if (THIS->lastinput)
    THIS->lastinput->next=i;
  else
    THIS->firstinput=i;
  THIS->lastinput=i;
  return i;
}

/* Free an input struct and all that it stands for */
static INLINE void free_input(struct input *i)
{
  ninputs--;
  switch (i->type)
  {
  case I_OBJ:
    if (!i->u.obj) break;
    if (i->u.obj->prog)
    {
#ifdef BLOCKING_CLOSE
      apply_low(i->u.obj,i->set_blocking_offset,0);
      pop_stack();
#endif
      apply(i->u.obj,"close",0);
      pop_stack();
      destruct(i->u.obj);
    }
    free_object(i->u.obj);
    nobjects--;
    i->u.obj=0;
    break;

  case I_STRING:
    free_string(i->u.str);
    nstrings--;
    break;

  case I_MMAP:
#if defined(HAVE_MMAP) && defined(HAVE_MUNMAP)
    munmap(i->u.mmap,i->len);
    mmapped -= i->len;  
#else
    error("I_MMAP input when MMAP is diabled!");
#endif
    break;

  case I_NONE: break;
  }
  free((char *)i);
}

/* do the done_callback, then close and free everything */
static INLINE void pipe_done(void)
{
  if (THIS->done_callback.type!=T_INT)
  {
    assign_svalue_no_free(sp++,&THIS->id);
    apply_svalue(&(THIS->done_callback),1);
    pop_stack();

    if(!THISOBJ->prog) /* We will not free anything in this case. */
      return;
    /*  error("Pipe done callback destructed pipe.\n");  */
  }
  close_and_free_everything(THISOBJ,THIS);
}

static void finished_p(void)
{
  if(THIS->done) return;

  if(THIS->fd != -1) 
  {
    if(THIS->living_outputs > 1) return;
    if(THIS->firstinput) return;

  }else{
    if(THIS->living_outputs) return;
  }
  pipe_done();
}

/* Allocate a new buffer and put it at the end of the chain of buffers
 * scheduled for output. Return 1 if we have more bytes in buffers
 * than allowed afterwards.
 */
static INLINE int append_buffer(struct pike_string *s)
   /* 1=buffer full */
{
   struct buffer *b;

   if(THIS->fd!= -1)
   {
     lseek(THIS->fd,THIS->pos,0);
     write(THIS->fd,s->str,s->len);
     THIS->pos+=s->len;
     return 0;
   }
   else
   {
     nbuffers++;
     b=ALLOC_STRUCT(buffer);
     b->next=NULL;
     b->s=s;
     sbuffers += s->len;
     s->refs++;

     if (THIS->lastbuffer)
       THIS->lastbuffer->next=b;
     else
       THIS->firstbuffer=b;

     THIS->lastbuffer=b;

     THIS->bytes_in_buffer+=s->len;
   }
   return THIS->bytes_in_buffer > MAX_BYTES_IN_BUFFER;
}

/* Wake up the sleepers */
static void low_start()
{
  struct object *obj, *next;
  struct output *o;


  THISOBJ->refs++;		/* dont kill yourself now */
  for(obj=THIS->firstoutput;obj;obj=next)
  {
    obj->refs++; /* Hang on PLEASE!! /hubbe */
    o=(struct output *)(obj->storage);
    if (o->obj && o->mode==O_SLEEP)
    {
      if (!o->obj->prog)
      {
	output_finish(obj);
      }
      else
      {
#if 0
	push_int(0);
	push_callback(offset_output_write_callback);
	push_callback(offset_output_close_callback);
	apply_low(o->obj,o->set_nonblocking_offset,3);
#endif
	output_try_write_some(obj);
	o->mode=O_RUN;		/* Hubbe */
      }
    }
    next=o->next;
    free_object(obj);
  }

  free_object(THISOBJ);
}

/* Let's guess what this function does....
 *
 */
static INLINE void input_finish(void)
{
  struct input *i;

  while(1)
  {
    i=THIS->firstinput->next;
    free_input(THIS->firstinput);
    THIS->firstinput=i;

    if(!i) break;

    switch(i->type)
    {
    case I_OBJ:
      THIS->sleeping=0;
      push_callback(offset_input_read_callback);
      push_int(0);
      push_callback(offset_input_close_callback);
      apply_low(i->u.obj,i->set_nonblocking_offset,3);
      pop_stack();
      return;

    case I_MMAP:
      if (THIS->fd==-1) return;
      continue;

    case I_STRING:
      append_buffer(i->u.str);

    case I_NONE: break;
    }
  }
  THIS->sleeping=0;

  low_start();
  finished_p();
}

/* This function reads some data from the file cache..
 * Called when we want some data to send.
 */
static INLINE struct pike_string* gimme_some_data(unsigned long pos)
{
   struct buffer *b;
   unsigned long len;

   /* We have a file cache, read from it */
   if (THIS->fd!=-1)
   {
      if (THIS->pos<=pos) return NULL; /* no data */
      len=THIS->pos-pos;
      if (len>READ_BUFFER_SIZE) len=READ_BUFFER_SIZE;
      lseek(THIS->fd,pos,0); /* SEEK_SET */
      read(THIS->fd,static_buffer,len);
      return make_shared_binary_string(static_buffer,len);
   }

   if (pos<THIS->pos)
     return make_shared_string("buffer underflow"); /* shit */

   /* We want something in the next buffer */
   while (THIS->firstbuffer && pos>=THIS->pos+THIS->firstbuffer->s->len) 
   {
     /* Free the first buffer, and update THIS->pos */
      b=THIS->firstbuffer;
      THIS->pos+=b->s->len;
      THIS->bytes_in_buffer-=b->s->len;
      THIS->firstbuffer=b->next;
      if (!b->next)
	THIS->lastbuffer=NULL;
      sbuffers-=b->s->len;
      nbuffers--;
      free_string(b->s);
      free((char *)b);

      /* Wake up first input if it was sleeping and we
       * have room for more in the buffer.
       */
      if (THIS->sleeping &&
	  THIS->firstinput &&
	  THIS->bytes_in_buffer<MAX_BYTES_IN_BUFFER)
      {
	THIS->sleeping=0;
	push_callback(offset_input_read_callback);
	push_int(0);
	push_callback(offset_input_close_callback);
	apply(THIS->firstinput->u.obj, "set_nonblocking", 3);
	pop_stack();
      }
   }

   while (!THIS->firstbuffer)
   {
     if (THIS->firstinput)
     {
#if defined(HAVE_MMAP) && defined(HAVE_MUNMAP)
       if (THIS->firstinput->type==I_MMAP)
       {
	 if (pos >= THIS->firstinput->len + THIS->pos) /* end of mmap */
	 {
	   THIS->pos+=THIS->firstinput->len;
	   input_finish();
	   continue;
	 }
	 len=THIS->firstinput->len+THIS->pos-pos;
	 if (len>READ_BUFFER_SIZE) len=READ_BUFFER_SIZE;
	 return make_shared_binary_string(THIS->firstinput->u.mmap+
					  pos-THIS->pos,
					  len);
       }
       else
#endif
       if (THIS->firstinput->type!=I_OBJ)
       {
	 input_finish();       /* shouldn't be anything else ... maybe a finished object */
       }
     }
     return NULL;		/* no data */
   } 

   if (pos==THIS->pos)
   {
      THIS->firstbuffer->s->refs++;
      return THIS->firstbuffer->s;
   }
   return make_shared_binary_string(THIS->firstbuffer->s->str+
				    pos-THIS->pos,
				    THIS->firstbuffer->s->len-
				    pos+THIS->pos);
}


/*
 * close and free the contents of a struct output
 * Note that the output struct is not freed or unlinked here,
 * that is taken care of later.
 */
static INLINE void output_finish(struct object *obj)
{
  struct output *o;

  o=(struct output *)(obj->storage);

  if (o->obj)
  {
    if(o->obj->prog)
    {
#ifdef BLOCKING_CLOSE
      apply_low(o->obj,o->set_blocking_offset,0);
      pop_stack();
#endif
      push_int(0);
      apply(o->obj,"set_id",1);
      pop_stack();

      apply(o->obj,"close",0);
      pop_stack();
      if(!THISOBJ->prog)
	error("Pipe done callback destructed pipe.\n");
      destruct(o->obj);
    }
    free_object(o->obj);
    noutputs--;
    o->obj=NULL;

    THIS->living_outputs--;

    finished_p(); /* Moved by per, one line down.. :) */

    free_object(THISOBJ);		/* What? /Hubbe */
  }
}

/*
 * Try to write some data to our precious output
 */
static INLINE void output_try_write_some(struct object *obj)
{
  struct output *out;
  struct pike_string *s;
  unsigned long len;
  INT32 ret;
  
  out=(struct output*)(obj->storage);

#ifdef INSISTANT_WRITE   
  do
  {
#endif
    /* Get some data to write */
    s=gimme_some_data(out->pos);
    if (!s)			/* out of data */
    {
      /* out of data, goto sleep */
      if (!THIS->firstinput || !out->obj->prog) /* end of life */
      {
	output_finish(obj);
      }
      else
      {
#if 0
	apply_low(out->obj, out->set_blocking_offset, 0);
	pop_stack();		/* from apply */
#endif
	out->mode=O_SLEEP;
      }
      return;
    }
    len=s->len;
    push_string(s);
    apply_low(out->obj,out->write_offset,1);
    out->mode=O_RUN;

    ret=-1;
    if(sp[-1].type == T_INT) ret=sp[-1].u.integer;
    pop_stack();

    if (ret==-1)		/* error, byebye */
    {
      output_finish(obj);
      return;
    }
    out->pos+=ret;

#ifdef INSISTANT_WRITE
  } while(ret == len);
#endif
}

/********** methods *********************************************************/

/* Add an input to this pipe */
static void pipe_input(INT32 args)
{
   struct input *i;
   int fd=-1;			/* Per, one less warning to worry about... */
   char *m;
   struct stat s;
   struct object *obj;

   if (args<1 || sp[-args].type != T_OBJECT)
     error("Bad/missing argument 1 to pipe->input().\n");

   obj=sp[-args].u.object;
   if(!obj || !obj->prog)
     error("pipe->input() on destructed object.\n");

   push_int(0);
   apply(sp[-args-1].u.object,"set_id", 1);
   pop_stack();

   i=new_input();

#if defined(HAVE_MMAP) && defined(HAVE_MUNMAP)

   /* We do not handle mmaps if we have a buffer */
   if(THIS->fd == -1)
   {
     apply(obj, "query_fd", 0);
     if(sp[-1].type == T_INT) fd=sp[-1].u.integer;
     pop_stack();

     if (fd != -1 && fstat(fd,&s)==0)
     {
       int filep=lseek(fd, 0L, SEEK_CUR); /* keep the file pointer */
       if(S_ISREG(s.st_mode)	/* regular file */
	  && ((long)(m=(char *)mmap(0,s.st_size - filep,PROT_READ,
				    MAP_FILE|MAP_SHARED,fd,filep))!=-1))
       {
	 mmapped += s.st_size;

	 i->type=I_MMAP;
	 i->len=s.st_size;
	 i->u.mmap=m;
       
	 pop_n_elems(args);
	 push_int(0);
	 return;
       }
     }
   }
#endif

   i->u.obj=obj;
   nobjects++;
   i->type=I_OBJ;
   i->u.obj->refs++;
   i->set_nonblocking_offset=find_identifier("set_nonblocking",i->u.obj->prog);
   i->set_blocking_offset=find_identifier("set_blocking",i->u.obj->prog);

   if (i->set_nonblocking_offset<0 ||
       i->set_blocking_offset<0) 
   {
      free_object(i->u.obj);
      i->u.obj=NULL;
      i->type=I_NONE;

      nobjects--;
      error("illegal file object%s%s\n",
	    ((i->set_nonblocking_offset<0)?"; no set_nonblocking":""),
	    ((i->set_blocking_offset<0)?"; no set_blocking":""));
   }
  
   if (i==THIS->firstinput)
   {
     push_callback(offset_input_read_callback);
     push_int(0);
     push_callback(offset_input_close_callback);
     apply_low(i->u.obj,i->set_nonblocking_offset,3);
     pop_stack();
   }
   else
   {
     /* DOESN'T WORK!!! */
     push_int(0);
     push_int(0);
     push_callback(offset_input_close_callback);
     apply_low(i->u.obj,i->set_nonblocking_offset,3);
     pop_stack();
   }

   pop_n_elems(args);
   push_int(0);
}

static void pipe_write(INT32 args)
{
  struct input *i;

  if (args<1 || sp[-args].type!=T_STRING)
    error("illegal argument to pipe->write()\n");

  if (!THIS->firstinput)
  {
    append_buffer(sp[-args].u.string);
    pop_n_elems(args);
    push_int(0);
    return;
  }

  i=new_input();
  i->type=I_STRING;
  nstrings++;
  i->u.str=sp[-args].u.string;
  i->u.str->refs++;
  pop_n_elems(args-1);
}

void f_mark_fd(INT32 args);

static void pipe_output(INT32 args)
{
  struct object *obj;
  struct output *o;
  int fd;
  struct stat s;
  struct buffer *b;

  if (args<1 || 
      sp[-args].type != T_OBJECT ||
      !sp[-args].u.object ||
      !sp[-args].u.object->prog)
    error("Bad/missing argument 1 to pipe->output().\n");

  if (THIS->fd==-1)		/* no buffer */
  {
    /* test if usable as buffer */ 
    apply(sp[-args].u.object,"query_fd",0);

    if ((sp[-1].type==T_INT)
	&& (fd=sp[-1].u.integer)>=0
	&& (fstat(fd,&s)==0)
	&& S_ISREG(s.st_mode)
	&& (THIS->fd=dup(fd))!=-1 )
    {
      /* keep the file pointer of the duped fd */
      THIS->pos=lseek(fd, 0L, SEEK_CUR);
      
#if 0
      /* This won't work if the spider-module is dynamically linked. */
      push_int(THIS->fd);
      push_string(make_shared_string("pipe.c: file buffer"));
      f_mark_fd(2);
      pop_stack();
#endif /* 0 */

      THIS->living_outputs++;

      while (THIS->firstbuffer)
      {
	b=THIS->firstbuffer;
	THIS->firstbuffer=b->next;
	lseek(THIS->fd,THIS->pos,0);
	write(THIS->fd,b->s->str,b->s->len);
	sbuffers-=b->s->len;
	nbuffers--;
	free_string(b->s);
	free((char *)b);
      }
      THIS->lastbuffer=NULL;

      /* keep the file pointer of the duped fd
	 THIS->pos=0; */
      push_int(0);
      apply(sp[-args-2].u.object,"set_id", 1);
      pop_n_elems(args+2);	/* ... and from apply x 2  */
      return;
    }
    pop_stack();		/* from apply */
  } 

  THIS->living_outputs++;
  THISOBJ->refs++;		/* Weird */

  /* Allocate a new struct output */
  obj=clone_object(output_program,0);
  o=(struct output *)(obj->storage);
  o->next=THIS->firstoutput;
  THIS->firstoutput=obj;
  noutputs++;
  o->obj=NULL;

  o->obj=sp[-args].u.object;
  o->obj->refs++;

  o->write_offset=find_identifier("write",o->obj->prog);
  o->set_nonblocking_offset=find_identifier("set_nonblocking",o->obj->prog);
  o->set_blocking_offset=find_identifier("set_blocking",o->obj->prog);

  if (o->write_offset<0 || o->set_nonblocking_offset<0 ||
      o->set_blocking_offset<0) 
  {
    free_object(o->obj);
    error("illegal file object%s%s%s\n",
	  ((o->write_offset<0)?"; no write":""),
	  ((o->set_nonblocking_offset<0)?"; no set_nonblocking":""),
	  ((o->set_blocking_offset<0)?"; no set_blocking":""));
  }

  o->mode=O_RUN;
  /* keep the file pointer of the duped fd
     o->pos=0; */
  o->pos=THIS->pos;

  push_object(obj);
  obj->refs++;
  apply(o->obj,"set_id",1);
  pop_stack();

  push_int(0);
  push_callback(offset_output_write_callback);
  push_callback(offset_output_close_callback);
  apply_low(o->obj,o->set_nonblocking_offset,3);
  pop_stack();
   
  pop_n_elems(args-1);
}

static void pipe_set_done_callback(INT32 args)
{
  if (args==0)
  {
    free_svalue(&THIS->done_callback);
    THIS->done_callback.type=T_INT;
    return;
  }
  if (args<1 || (sp[-args].type!=T_FUNCTION && sp[-args].type!=T_ARRAY))
    error("Illegal argument to set_done_callback()\n");

  if (args>1)
  {
     free_svalue(&THIS->id);
     assign_svalue_no_free(&(THIS->id),sp-args+1);
  }

  free_svalue(&THIS->done_callback);
  assign_svalue_no_free(&(THIS->done_callback),sp-args); 
  pop_n_elems(args-1); 
}

static void pipe_set_output_closed_callback(INT32 args)
{
  if (args==0)
  {
    free_svalue(&THIS->done_callback);
    THIS->output_closed_callback.type=T_INT;
    return;
  }
  if (args<1 || (sp[-args].type!=T_FUNCTION && sp[-args].type!=T_ARRAY))
    error("Illegal argument to set_output_closed_callback()\n");

  if (args>1)
  {
     free_svalue(&THIS->id);
     assign_svalue_no_free(&(THIS->id),sp-args+1);
  }
  free_svalue(&THIS->output_closed_callback);
  assign_svalue_no_free(&(THIS->output_closed_callback),sp-args); 
  pop_n_elems(args-1); 
}

static void pipe_finish(INT32 args)
{
   pop_n_elems(args);
   push_int(0);
   pipe_done();
}

static void pipe_start(INT32 args) /* force start */
{
  low_start();
  if(args)
    pop_n_elems(args-1);
}

/********** callbacks *******************************************************/

static void pipe_write_output_callback(INT32 args)
{
   if (args<1 || sp[-args].type!=T_OBJECT)
     error("Illegal argument to pipe->write_output_callback\n");

   if(!sp[-args].u.object->prog) return;

   if(sp[-args].u.object->prog != output_program)
     error("Illegal argument to pipe->write_output_callback\n");

   output_try_write_some(sp[-args].u.object);
   pop_n_elems(args-1);
}

static void pipe_close_output_callback(INT32 args)
{
  struct output *o;
   if (args<1 || sp[-args].type!=T_OBJECT)

   if(!sp[-args].u.object->prog) return;

   if(sp[-args].u.object->prog != output_program)
     error("Illegal argument to pipe->close_output_callback\n");

  o=(struct output *)(sp[-args].u.object->storage);

  if (THIS->output_closed_callback.type!=T_INT)
  {
    assign_svalue_no_free(sp++,&THIS->id);
    push_object(o->obj);
    apply_svalue(&(THIS->output_closed_callback),2);
    pop_stack();
  }

  output_finish(sp[-args].u.object);
  pop_n_elems(args-1);
}

static void pipe_read_input_callback(INT32 args)
{
  struct input *i;
  struct pike_string *s;

  if (args<2 || sp[1-args].type!=T_STRING)
    error("Illegal argument to pipe->read_input_callback\n");
   
  i=THIS->firstinput;

  if (!i)
    error("Pipe read callback without any inputs left.\n");

  s=sp[1-args].u.string;

  if(append_buffer(s))
  {
    /* THIS DOES NOT WORK */
    push_int(0);
    push_int(0);
    push_callback(offset_input_close_callback);
    apply_low(i->u.obj,i->set_nonblocking_offset,3);
    pop_stack();
    THIS->sleeping=1;
  }

  low_start();
  pop_n_elems(args-1);
}

static void pipe_close_input_callback(INT32 args)
{
   struct input *i;
   i=THIS->firstinput;

   if(!i)
     error("Input close callback without inputs!\n");

   if(i->type != I_OBJ)
     error("Premature close callback on pipe!.\n");

   if (i->u.obj->prog)
   {
#ifdef BLOCKING_CLOSE
      apply_low(i->u.obj,i->set_blocking_offset,0);
      pop_stack();
#endif
      apply(i->u.obj,"close",0);
      pop_stack();
   }
   nobjects--;
   free_object(i->u.obj);
   i->type=I_NONE;

   input_finish();
   if(args)
     pop_n_elems(args-1);
}

static void pipe_version(INT32 args)
{
   pop_n_elems(args);
   push_string(make_shared_string("PIPE ver 2.0"));
}

/********** init/exit *******************************************************/

void close_and_free_everything(struct object *thisobj,struct pipe *p)
{
   struct buffer *b;
   struct input *i;
   struct output *o;
   struct object *obj;
   
   p->done=1;

   if (thisobj) 
      thisobj->refs++; /* don't kill object during this */

   while (p->firstbuffer)
   {
      b=p->firstbuffer;
      p->firstbuffer=b->next;
      sbuffers-=b->s->len;
      nbuffers--;
      free_string(b->s);
      b->next=NULL;
      free((char *)b); /* Hubbe */
   }
   p->lastbuffer=NULL;


   while (p->firstinput)
   {
      i=p->firstinput;
      p->firstinput=i->next;
      free_input(i);
   }
   p->lastinput=NULL;

   while (p->firstoutput)
   {
     obj=p->firstoutput;
     o=(struct output *)(obj->storage);
     p->firstoutput=o->next;
     output_finish(obj);
     free_object(obj);
   }
   if (p->fd!=-1)
   {
     close(p->fd);
     p->fd=-1;
   }

   p->living_outputs=0;
   
   if (thisobj)
     free_object(thisobj);

   free_svalue(& p->done_callback);
   free_svalue(& p->output_closed_callback);
   free_svalue(& p->id);

   p->done_callback.type=T_INT;
   p->output_closed_callback.type=T_INT;
   p->id.type=T_INT;

   p->done=0;
}

static void init_pipe_struct(struct object *o)
{
   THIS->firstbuffer=THIS->lastbuffer=NULL;
   THIS->firstinput=THIS->lastinput=NULL;
   THIS->firstoutput=NULL;
   THIS->bytes_in_buffer=0;
   THIS->pos=0;
   THIS->sleeping=0;
   THIS->done=0;
   THIS->fd=-1;
   THIS->done_callback.type=T_INT;
   THIS->output_closed_callback.type=T_INT;
   THIS->id.type=T_INT;
   THIS->id.u.integer=0;
   THIS->living_outputs=0;
}

static void exit_pipe_struct(struct object *o)
{
  close_and_free_everything(NULL,THIS);
}

static void exit_output_struct(struct object *obj)
{
  struct output *o;
  
  o=(struct output *)(fp->current_storage);
  if (o->obj)
  {
    if(o->obj->prog)
    {
#ifdef BLOCKING_CLOSE
      apply_low(o->obj,o->set_blocking_offset,0);
      pop_stack();
#endif
      push_int(0);
      apply(o->obj,"set_id",1);
      pop_stack();

      apply(o->obj,"close",0);
      pop_stack();

      if(!THISOBJ->prog)
	error("Pipe done callback destructed pipe.\n");
    }
    free_object(o->obj);
    noutputs--;
    o->obj=0;
  }
}

static void init_output_struct(struct object *ob)
{
  struct output *o;
  o=(struct output *)(fp->current_storage);
  o->obj=0;
}


/********** Pike init *******************************************************/

void port_setup_program(void);

void f__pipe_debug(INT32 args)
{
  pop_n_elems(args);
  push_int(noutputs);
  push_int(ninputs);
  push_int(nstrings);
  push_int(nobjects);
  push_int(mmapped);
  push_int(nbuffers);
  push_int(sbuffers);
  f_aggregate(7);
}

void pike_module_init()
{
   start_new_program();
   add_storage(sizeof(struct pipe));
   add_efun("_pipe_debug", f__pipe_debug, "function(:array)", 0);
   add_function("input",pipe_input,"function(object:void)",0);
   add_function("output",pipe_output,"function(object:void)",0);
   add_function("write",pipe_write,"function(string:void)",0);

   add_function("start",pipe_start,"function(:void)",0);
   add_function("finish",pipe_finish,"function(:void)",0);
   
   add_function("set_output_closed_callback",pipe_set_output_closed_callback,
		"function(void|function(mixed,object:mixed),void|mixed:void)",0);
   add_function("set_done_callback",pipe_set_done_callback,
		"function(void|function(mixed:mixed),void|mixed:void)",0);
   
   add_function("_output_close_callback",pipe_close_output_callback,
		"function(int:void)",0);
   add_function("_input_close_callback",pipe_close_input_callback,
		"function(int:void)",0);
   add_function("_output_write_callback",pipe_write_output_callback,
		"function(int:void)",0);
   add_function("_input_read_callback",pipe_read_input_callback,
		"function(int,string:void)",0);

   add_function("version",pipe_version,"function(:string)",0);
   
   set_init_callback(init_pipe_struct);
   set_exit_callback(exit_pipe_struct);
   
   pipe_program=end_program();
   add_program_constant("pipe",pipe_program, 0);
   
   offset_output_close_callback=find_identifier("_output_close_callback",
						pipe_program);
   offset_input_close_callback=find_identifier("_input_close_callback",
					       pipe_program);
   offset_output_write_callback=find_identifier("_output_write_callback",
						pipe_program);
   offset_input_read_callback=find_identifier("_input_read_callback",
					      pipe_program);


   start_new_program();
   add_storage(sizeof(struct output));
   set_init_callback(init_output_struct);
   set_exit_callback(exit_output_struct);
   output_program=end_program();
   add_program_constant("__output",output_program, 0);
}

void pike_module_exit(void) 
{
  if(pipe_program) free_program(pipe_program);
  pipe_program=0;
  if(output_program) free_program(output_program);
  output_program=0;
}







