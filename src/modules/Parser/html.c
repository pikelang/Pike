#include "global.h"
#include "config.h"

#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "threads.h"
#include "array.h"
#include "error.h"
#include "operators.h"

#include "parser.h"

struct location
{
   struct piece *p; 

   int byteno;     /* current byte, first=1 */
   int lineno;     /* line number, first=1 */
   int linestart;  /* byte current line started at */
};

struct piece
{
   struct pike_string *s;
   struct piece *next;
};

struct feed_stack
{
   struct piece *start;
   struct piece *location;
   
   struct feed_stack *prev;
};

struct parser_html_storage
{
   /* feeded info */
   struct piece *feed,*feed_end;

   /* resulting data */
   struct piece *out,*out_end;
   
   /* location */
   struct location head; /* first byte in feed is.. */

   /* parser stack */
   struct feed_stack *stack;

   struct svalue callback__tag;
   struct svalue callback__data;
   struct svalue callback__entity;
};

#ifdef THIS
#undef THIS /* Needed for NT */
#endif

#define THIS ((struct parser_html_storage*)(fp->current_storage))
#define THISOBJ (fp->current_object)

/****** init & exit *********************************/

void _reset_feed()
{
   struct piece *f;
   struct feed_stack *st;

   /* kill feed */

   while (THIS->feed)
   {
      f=THIS->feed->next;
      free(THIS->feed);
      THIS->feed=f;
   }
   THIS->feed_end=NULL;

   /* kill out-feed */

   while (THIS->out)
   {
      f=THIS->out->next;
      free(THIS->out);
      THIS->out=f;
   }
   THIS->out_end=NULL;

   /* free stack */

   while (THIS->stack)
   {
      st=THIS->stack;
      THIS->stack=st->prev;
      free(st);
   }

   THIS->head.p=NULL;
   THIS->head.byteno=1;
   THIS->head.lineno=1;
   THIS->head.linestart=1;
}

static void init_html_struct(struct object *o)
{
   THIS->feed=NULL;
   THIS->out=NULL;
   THIS->stack=NULL;
   _reset_feed();

   THIS->callback__tag.type=T_INT;
   THIS->callback__data.type=T_INT;
   THIS->callback__entity.type=T_INT;
}

static void exit_html_struct(struct object *o)
{
   _reset_feed(); /* frees feed & out */

   free_svalue(&(THIS->callback__tag));
   free_svalue(&(THIS->callback__data));
   free_svalue(&(THIS->callback__entity));
}

/****** setup callbacks *****************************/

static void html__set_tag_callback(INT32 args)
{
   if (!args) error("_set_tag_callback: too few arguments\n");
   assign_svalue(&(THIS->callback__tag),sp-args);
   pop_n_elems(args);
   push_int(0);
}

static void html__set_data_callback(INT32 args)
{
   if (!args) error("_set_data_callback: too few arguments\n");
   assign_svalue(&(THIS->callback__data),sp-args);
   pop_n_elems(args);
   push_int(0);
}

static void html__set_entity_callback(INT32 args)
{
   if (!args) error("_set_entity_callback: too few arguments\n");
   assign_svalue(&(THIS->callback__entity),sp-args);
   pop_n_elems(args);
   push_int(0);
}

/****** try_feed - internal main ********************/

static void try_feed(int finished)
{
   /* 
      o if tag_stack:
          o pop & parse that
          o ev put on stack
   */

   if (THIS->out_end)
      THIS->out_end->next=THIS->feed;
   else
      THIS->out=THIS->out_end=THIS->feed;

   while (THIS->out_end && THIS->out_end->next)
      THIS->out_end=THIS->out_end->next;

   THIS->feed=THIS->feed_end=NULL;
}

/****** feed ****************************************/

static void html_feed(INT32 args)
{
   struct piece *f;

   if (!args ||
       sp[-args].type!=T_STRING)
      error("feed: illegal arguments\n");

   f=malloc(sizeof(struct piece));
   if (!f)
      error("feed: out of memory\n");
   copy_shared_string(f->s,sp[-args].u.string);

   pop_n_elems(args);

   f->next=NULL;

   if (THIS->feed_end==NULL)
     THIS->feed=THIS->feed_end=f;
   else
   {
      THIS->feed_end->next=f;
      THIS->feed_end=f;
   }

   try_feed(0);

   ref_push_object(THISOBJ);
}

static void put_out_feed(struct pike_string *s)
{
   struct piece *f=malloc(sizeof(struct piece));
   if (!f)
      error("Parser.HTML(): out of memory\n");
   copy_shared_string(f->s,s);

   f->next=NULL;

   if (THIS->out_end==NULL)
     THIS->out=THIS->out_end=f;
   else
   {
      THIS->out_end->next=f;
      THIS->out_end=f;
   }
}

static void html_finish(INT32 args)
{
   pop_n_elems(args);
   try_feed(1);
   ref_push_object(THISOBJ);
}

static void html_read(INT32 args)
{
   int n;
   int m=0; // strings on stack

   if (!args) 
      n=0x7fffffff; /* a lot */
   else  if (sp[-args].type==T_INT)
      n=sp[-args].u.integer;
   else
      error("read: illegal argument\n");

   pop_n_elems(args);

   /* collect up to n characters */

   while (THIS->out && n)
   {
      struct piece *z;

      if (THIS->out->s->len>n)
      {
	 struct pike_string *ps;
	 push_string(string_slice(THIS->out->s,0,n));
	 m++;
	 ps=string_slice(THIS->out->s,n,THIS->out->s->len-n);
	 free_string(THIS->out->s);
	 THIS->out->s=ps;
	 break;
      }
      n-=THIS->out->s->len;
      push_string(THIS->out->s);
      m++;
      z=THIS->out;
      THIS->out=THIS->out->next;
      free(z);
   }

   if (!THIS->out) 
      THIS->out_end=NULL;

   if (!m) 
      push_text("");
   else
      f_add(m);
}

/****** module init *********************************/

void init_parser_html(void)
{
   ADD_STORAGE(struct parser_html_storage);

   set_init_callback(init_html_struct);
   set_exit_callback(exit_html_struct);

#define CBRET "string|array(string)" /* 0|string|({string}) */

   /* feed control */

   add_function("feed",html_feed,
		"function(string:object)",0);
   add_function("finish",html_finish,
		"function(:object)",0);
   add_function("read",html_read,
		"function(void|int:string)",0);

   /* special callbacks */

   add_function("_set_tag_callback",html__set_tag_callback,
		"function(function(object,mixed ...:"CBRET"):void)",0);
   add_function("_set_data_callback",html__set_data_callback,
		"function(function(object,mixed ...:"CBRET"):void)",0);
   add_function("_set_entity_callback",html__set_entity_callback,
		"function(function(object,mixed ...:"CBRET"):void)",0);
}


/*

class Parse_HTML
{
   void feed(string something); // more data in stream
   void finish(); // stream ends here

   string read(void|int chars); // read out-feed

   void reset();  // reset stream

   object clone(); // new object, fresh stream

   // argument quote ( < ... foo="bar" foo='bar' ...> )
   void set_quote(string start,string end);
   void set_quote(string start,string end,
                  string start2,string end2, ...);      // tupels

   // tag quote
   void set_tag_quote(string start,string end); // "<" ">"

   // call to_call(this,mapping(string:string) args,...extra)
   void add_tag(string tag,function to_call);

   // call to_call(this,mapping(string:string) args,string cont,...extra)
   void add_container(string tag,function to_call);

   // same as above, but tries globs (slower<tm>)
   void add_glob_tag(string tag,function to_call);
   void add_glob_container(string tag,function to_call);

   // set extra args
   void extra(mixed ...extra);

   // query where we are now
   string at_tag();  // tag name
   string tag_data();// tag string (< foo bar=z > -> " foo bar=z ")
   int at_line();    // line number (first=1)
   int at_char();    // char (first=1)
   int at_column();  // column (first=1)

   // low-level callbacks
   // calls to_call(this,string data)
   void _set_tag_callback(function to_call);
   void _set_data_callback(function to_call);

   // just useful
   mapping parse_get_tag(string tag);
   mapping parse_get_args(string tag);

   // entity quote
   void set_entity_quote(string start,string end); // "&",";"

   int set_allow_open_entity(void|int yes);        // &entfoo<bar> -> ent

   // call to_call(this,string entity,...extra);
   void add_entity(string entity,function to_call);
   void add_glob_entity(string entity,function to_call);

   // calls to_call(this,string data)
   void _set_entity_callback(function to_call);
}

*/
