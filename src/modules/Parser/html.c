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
#include "builtin_functions.h"
#include "module_support.h"
#include "mapping.h"
#include "stralloc.h"
#include "gc.h"
#include "program_id.h"
#include <ctype.h>

#include "parser.h"

extern struct program *parser_html_program;

/*
#define SCAN_DEBUG
#define DEBUG
*/

#ifdef DEBUG
#undef DEBUG
#define DEBUG(X) if (THIS->flags & FLAG_DEBUG_MODE) fprintf X
#define DEBUG_MARK_SPOT debug_mark_spot
#else
#define DEBUG(X) do; while(0)
#define DEBUG_MARK_SPOT(TEXT,FEED,C) do; while(0)
#endif

#if 0
#define free(X) fprintf(stderr,"free line %d: %p\n",__LINE__,X); free(X)
#endif

#define MAX_FEED_STACK_DEPTH 10

/*
**! module Parser
**! class HTML
**!	This is a simple parser for SGML structured markups.
**!	It's not really HTML, but it's useful for that 
**!	purpose. 
**!
**!	The simple way to use it is to give it some information
**!	about available tags and containers, and what 
**!	callbacks those is to call. 
**!
**!	The object is easily reused, by calling the <ref>clone</ref>()
**!	function. 
**!
**! see also: add_tag, add_container, clone
*/

struct location
{
   int byteno;     /* current byte, first=1 */
   int lineno;     /* line number, first=1 */
   int linestart;  /* byte current line started at */
};

struct piece
{
   struct pike_string *s;
   struct piece *next;
};

struct out_piece
{
   struct svalue v;
   struct out_piece *next;
};

struct feed_stack
{
   int ignore_data;
   
   struct feed_stack *prev;

   /* this is a feed-stack, ie
      these contains the result of callbacks,
      if they are to be parsed.
      
      The bottom stack element has no local feed,
      it's just for convinience */

   /* current position; if not local feed, use global feed */
   struct piece *local_feed;
   int c; 

   struct location pos;
};

enum types {TYPE_TAG, TYPE_CONT, TYPE_ENTITY, TYPE_QTAG, TYPE_DATA};

/* flag: case sensitive tag matching */
#define FLAG_CASE_INSENSITIVE_TAG	0x00000001

/* flag: arg quote may have tag_end to end quote and tag */
#define FLAG_LAZY_END_ARG_QUOTE		0x00000002

/* flag: the chars in lazy_entity_ends ends the search for entity ends */
#define FLAG_LAZY_ENTITY_END		0x00000004

/* flag: match '<' and '>' for in-tag-tags (<foo <bar>>) */
#define FLAG_MATCH_TAG			0x00000008

/* flag: handle mixed data from callbacks */
#define FLAG_MIXED_MODE			0x00000010

/* flag: treat unknown tags and entities as text */
#define FLAG_IGNORE_UNKNOWN		0x00000020

/* flag: debug output */
#define FLAG_DEBUG_MODE			0x00000040

struct parser_html_storage
{
/*--- current state -----------------------------------------------*/

   /* feeded info */
   struct piece *feed,*feed_end;

   /* resulting data */
   struct out_piece *out,*out_end;
   
   /* parser stack */
   struct feed_stack *stack;
   int stack_count;
   int max_stack_depth;

   /* current range (ie, current tag/entity/whatever) */
   /* start is also used to flag callback recursion */
   struct piece *start,*end;
   int cstart,cend;

   /* the type of the current thing being parsed */
   enum types type;

/*--- user configurable -------------------------------------------*/

  /* extra arguments */
   struct array *extra_args;

   /* named stuff */
   struct mapping *maptag;
   struct mapping *mapcont;
   struct mapping *mapentity;
   struct mapping *mapqtag;

   /* mapqtag is indexed on the first two chars in each tag name. The
    * values are concatenated tuples of ({name, to_do, end}), where
    * every name that's a prefix of some other name comes after it. */

   /* callback functions */
   struct svalue callback__tag;
   struct svalue callback__data;
   struct svalue callback__entity;

   /* flag bitfield; see FLAG_* above */
   int flags;

   p_wchar2 tag_start,tag_end;
   p_wchar2 entity_start,entity_end;
   int nargq;
#define MAX_ARGQ 8
   p_wchar2 argq_start[MAX_ARGQ],argq_stop[MAX_ARGQ];
   p_wchar2 arg_eq; /* = as in foo=bar */

   p_wchar2 *lazy_entity_ends;
   int n_lazy_entity_ends;
  
   p_wchar2 *ws;
   int n_ws;

   /* pre-calculated */
   /* whitespace + end argument ('=' and '>') */
   p_wchar2 *ws_or_endarg; 
   int n_ws_or_endarg;

   /* whitespace + end argument ('=' and '>') + start quote*/
   p_wchar2 *ws_or_endarg_or_quote; 
   int n_ws_or_endarg_or_quote;

   /* end of tag, arg_eq or start of arg quote */
   p_wchar2 look_for_start[MAX_ARGQ+4];
   int num_look_for_start;

   /* end(s) of _this_ arg quote */
   p_wchar2 look_for_end[MAX_ARGQ][MAX_ARGQ+2];
   int num_look_for_end[MAX_ARGQ];
};

/* default characters */
#define DEF_WS		' ', '\n', '\r', '\t', '\v'
#define DEF_TAG_START	'<'
#define DEF_TAG_END	'>'
#define DEF_ENT_START	'&'
#define DEF_ENT_END	';'
#define DEF_ARGQ_STARTS	'\"', '\''
#define DEF_ARGQ_STOPS	'\"', '\''
#define DEF_EQ		'='
#define DEF_LAZY_EENDS	DEF_WS, DEF_TAG_START, DEF_TAG_END, \
			DEF_ENT_START, DEF_ENT_END, DEF_ARGQ_STARTS, DEF_EQ

/* P_WAIT was already used by MSVC++ :(  /Hubbe */
typedef enum { STATE_DONE=0, STATE_WAIT, STATE_REREAD, STATE_REPARSE } newstate;

#ifdef THIS
#undef THIS /* Needed for NT */
#endif

#define THIS ((struct parser_html_storage*)(fp->current_storage))
#define THISOBJ (fp->current_object)

static struct pike_string *empty_string;

static void tag_name(struct parser_html_storage *this,
		     struct piece *feed,int c);
static void tag_args(struct parser_html_storage *this,
		     struct piece *feed,int c,struct svalue *def);

static int quote_tag_lookup (struct parser_html_storage *this,
			     struct piece *feed, int c,
			     struct piece **destp, int *d_p,
			     int finished, struct svalue **mapqentry);

/****** debug helper ********************************/

#ifdef DEBUG
void debug_mark_spot(char *desc,struct piece *feed,int c)
{
   int l,m,i,i0;
   char buf[40];

   if (!(THIS->flags & FLAG_DEBUG_MODE)) return;

   l=strlen(desc)+1;
   if (l<40) l=40;
   m=75-l; if (m<10) m=10;
   fprintf(stderr,"%-*s »",l,desc);
   i=c-m/2;
   if (i+m>=feed->s->len) i=feed->s->len-m;
   if (i<0) i=0; 
   for (i0=i;i<feed->s->len && i-i0<m;i++)
   {
      p_wchar2 ch=index_shared_string(feed->s,i);
      /* if (i==c) { fprintf(stderr,"^"); continue; }*/
      if (ch<32 || (ch>126 && ch<160) || ch>255)
	 fprintf(stderr,".");
      else
	 fprintf(stderr,"%c",ch);
   }

   sprintf(buf,"(%d) %p:%d/%d    ^",i0,feed,c,feed->s->len);
   fprintf(stderr,"»\n%*s\n",l+c-i0+3,buf);
   
}
#endif

/****** init & exit *********************************/

void reset_feed(struct parser_html_storage *this)
{
   struct feed_stack *st;

   /* kill feed */

   while (this->feed)
   {
      struct piece *f=this->feed;
      this->feed=f->next;
      free_string(f->s);
      free(f);
   }
   this->feed_end=NULL;

   /* kill out-feed */

   while (this->out)
   {
      struct out_piece *f=this->out;
      this->out=f->next;
      free_svalue(&f->v);
      free(f);
   }
   this->out_end=NULL;

   /* free stack */

   while (this->stack)
   {
      st=this->stack;
      while (st->local_feed)
      {
	 struct piece *f=st->local_feed;
	 st->local_feed=f->next;
	 free_string(f->s);
	 free(f);
      }
      this->stack=st->prev;
      free(st);
   }

   /* new stack head */

   this->stack=(struct feed_stack*)xalloc(sizeof(struct feed_stack));

   this->stack->prev=NULL;
   this->stack->local_feed=NULL;
   this->stack->ignore_data=0;
   this->stack->pos.byteno=1;
   this->stack->pos.lineno=1;
   this->stack->pos.linestart=1;
   this->stack->c=0;

   this->stack_count=0;
}

static void recalculate_argq(struct parser_html_storage *this)
{
   int n,i,j,k;

   /* prepare look for start of argument quote or end of tag */
   this->look_for_start[0]=this->tag_end;
   this->look_for_start[1]=this->arg_eq;
   if (this->flags & FLAG_MATCH_TAG) {
     this->look_for_start[2]=this->tag_start;
     n=3;
   }
   else n=2;
   for (i=0; i<this->nargq; i++)
   {
      for (j=0; j<n; j++)
	 if (this->look_for_start[j]==this->argq_start[i]) goto found_start;
      this->look_for_start[n++]=this->argq_start[i];
found_start:
      ;
   }
   this->num_look_for_start=n;

   for (k=0; k<this->nargq; k++)
   {
      n=0;
      this->look_for_end[k][n++]=this->argq_stop[k];
      for (i=0; i<this->nargq; i++)
	 if (this->argq_start[k]==this->argq_start[i])
	 {
	    for (j=0; j<this->nargq; j++)
	       if (this->look_for_end[k][j]==this->argq_start[i])
		  goto found_end;
	    this->look_for_end[k][n++]=this->argq_start[i];
   found_end:
	    ;
	 }
      if (this->flags & FLAG_LAZY_END_ARG_QUOTE)
	 this->look_for_end[k][n++]=this->tag_end;

      this->num_look_for_end[k]=n;
   }

   if (THIS->ws_or_endarg) 
   {
      free(THIS->ws_or_endarg);
      THIS->ws_or_endarg=NULL;
   }
   THIS->n_ws_or_endarg=THIS->n_ws+(THIS->flags & FLAG_MATCH_TAG ? 3 : 2);
   THIS->ws_or_endarg=(p_wchar2*)xalloc(sizeof(p_wchar2)*THIS->n_ws_or_endarg);
   MEMCPY(THIS->ws_or_endarg+(THIS->flags & FLAG_MATCH_TAG ? 3 : 2),
	  THIS->ws,THIS->n_ws*sizeof(p_wchar2));
   THIS->ws_or_endarg[0]=THIS->arg_eq;
   THIS->ws_or_endarg[1]=THIS->tag_end;
   if (THIS->flags & FLAG_MATCH_TAG)
      THIS->ws_or_endarg[2]=THIS->tag_start;

   if (THIS->ws_or_endarg_or_quote) 
   {
      free(THIS->ws_or_endarg_or_quote);
      THIS->ws_or_endarg_or_quote=NULL;
   }

   THIS->n_ws_or_endarg_or_quote=
      THIS->n_ws_or_endarg+THIS->nargq;
   THIS->ws_or_endarg_or_quote=
      (p_wchar2*)xalloc(sizeof(p_wchar2)*(THIS->n_ws_or_endarg_or_quote));

   MEMCPY(THIS->ws_or_endarg_or_quote, THIS->ws_or_endarg,
	  THIS->n_ws_or_endarg*sizeof(p_wchar2));

   MEMCPY(THIS->ws_or_endarg_or_quote+THIS->n_ws_or_endarg,
	  THIS->argq_start, THIS->nargq*sizeof(p_wchar2));

}

static void init_html_struct(struct object *o)
{
   static p_wchar2 whitespace[]={DEF_WS};
   static p_wchar2 argq_starts[]={DEF_ARGQ_STARTS};
   static p_wchar2 argq_stops[]={DEF_ARGQ_STOPS};
   static p_wchar2 lazy_eends[]={DEF_LAZY_EENDS};

#ifdef DEBUG
   THIS->flags=0;
#endif
   DEBUG((stderr,"init_html_struct %p\n",THIS));

   /* state */
   THIS->start=NULL;

   /* default set */
   THIS->tag_start=DEF_TAG_START;
   THIS->tag_end=DEF_TAG_END;
   THIS->entity_start=DEF_ENT_START;
   THIS->entity_end=DEF_ENT_END;
   THIS->nargq=sizeof(argq_starts)/sizeof(argq_starts[0]);
   MEMCPY(THIS->argq_start,argq_starts,sizeof(argq_starts));
   MEMCPY(THIS->argq_stop,argq_stops,sizeof(argq_stops));
   THIS->arg_eq='=';
   
   /* allocated stuff */
   THIS->lazy_entity_ends=NULL;
   THIS->ws=NULL;
   THIS->ws_or_endarg=NULL;
   THIS->ws_or_endarg_or_quote=NULL;

   THIS->maptag=NULL;
   THIS->mapcont=NULL;
   THIS->mapentity=NULL;
   THIS->mapqtag=NULL;

   THIS->flags=FLAG_MATCH_TAG;

   THIS->extra_args=NULL;

   /* initialize feed */
   THIS->feed=NULL;
   THIS->out=NULL;
   THIS->stack=NULL;
   reset_feed(THIS);
   
   /* clear callbacks */
   THIS->callback__tag.type=T_INT;
   THIS->callback__tag.u.integer=0;
   THIS->callback__data.type=T_INT;
   THIS->callback__data.u.integer=0;
   THIS->callback__entity.type=T_INT;
   THIS->callback__entity.u.integer=0;

   /* settings */
   THIS->max_stack_depth=MAX_FEED_STACK_DEPTH;

   /* this may now throw */
   THIS->lazy_entity_ends=(p_wchar2*)xalloc(sizeof(lazy_eends));
   MEMCPY(THIS->lazy_entity_ends,lazy_eends,sizeof(lazy_eends));
   THIS->n_lazy_entity_ends=NELEM(lazy_eends);

   THIS->ws=(p_wchar2*)xalloc(sizeof(whitespace));
   MEMCPY(THIS->ws,whitespace,sizeof(whitespace));
   THIS->n_ws=NELEM(whitespace);

   THIS->maptag=allocate_mapping(32);
   THIS->mapcont=allocate_mapping(32);
   THIS->mapentity=allocate_mapping(32);
   THIS->mapqtag=allocate_mapping(8);

   recalculate_argq(THIS);
}

static void exit_html_struct(struct object *o)
{
   struct feed_stack *fsp;

   DEBUG((stderr,"exit_html_struct %p\n",THIS));

   reset_feed(THIS); /* frees feed & out */

   if (THIS->maptag) free_mapping(THIS->maptag);
   if (THIS->mapcont) free_mapping(THIS->mapcont);
   if (THIS->mapentity) free_mapping(THIS->mapentity);
   if (THIS->mapqtag) free_mapping(THIS->mapqtag);

   dmalloc_touch_svalue(&(THIS->callback__tag));
   dmalloc_touch_svalue(&(THIS->callback__data));
   dmalloc_touch_svalue(&(THIS->callback__entity));
   free_svalue(&(THIS->callback__tag));
   free_svalue(&(THIS->callback__data));
   free_svalue(&(THIS->callback__entity));

   while ((fsp = THIS->stack)) {
     THIS->stack = fsp->prev;
     free(fsp);
   }

   if (THIS->lazy_entity_ends) free(THIS->lazy_entity_ends);
   if (THIS->ws) free(THIS->ws);
   if (THIS->ws_or_endarg) free(THIS->ws_or_endarg);
   if (THIS->ws_or_endarg_or_quote) free(THIS->ws_or_endarg_or_quote);

   if (THIS->extra_args) free_array(THIS->extra_args);

   DEBUG((stderr,"exit_html_struct %p done\n",THIS));
}

static void gc_check_html(struct object *o)
{
   gc_check(THIS->maptag);
   gc_check(THIS->mapcont);
   gc_check(THIS->mapentity);
   gc_check(THIS->mapqtag);

   gc_check_svalues(&(THIS->callback__tag),1);
   gc_check_svalues(&(THIS->callback__data),1);
   gc_check_svalues(&(THIS->callback__entity),1);
   
   if (THIS->extra_args)
      gc_check(THIS->extra_args);
}

static void gc_mark_html(struct object *o)
{
   gc_mark_mapping_as_referenced(THIS->maptag);
   gc_mark_mapping_as_referenced(THIS->mapcont);
   gc_mark_mapping_as_referenced(THIS->mapentity);
   gc_mark_mapping_as_referenced(THIS->mapqtag);

   gc_mark_svalues(&(THIS->callback__tag),1);
   gc_mark_svalues(&(THIS->callback__data),1);
   gc_mark_svalues(&(THIS->callback__entity),1);
   
   if (THIS->extra_args)
      gc_mark_array_as_referenced(THIS->extra_args);
}

/****** setup callbacks *****************************/

/*
**! method object _set_tag_callback(function to_call)
**! method object _set_entity_callback(function to_call)
**! method object _set_data_callback(function to_call)
**!	These functions set up the parser object to
**!	call the given callbacks upon tags, entities
**!	and/or data.
**!
**!	The callbacks will <em>only</em> be called if there isn't
**!	another tag/container/entity handler for these.
**!
**!	The function will be called with the parser
**!	object as first argument, and the active string
**!	as second. 
**!
**!	Note that no parsing of the contents has been done.
**!	Both endtags and normal tags are called, there is
**!	no container parsing.
**!
*/

static void html__set_tag_callback(INT32 args)
{
   if (!args) error("_set_tag_callback: too few arguments\n");
   dmalloc_touch_svalue(sp-args);
   assign_svalue(&(THIS->callback__tag),sp-args);
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

static void html__set_data_callback(INT32 args)
{
   if (!args) error("_set_data_callback: too few arguments\n");
   dmalloc_touch_svalue(sp-args);
   assign_svalue(&(THIS->callback__data),sp-args);
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

static void html__set_entity_callback(INT32 args)
{
   if (!args) error("_set_entity_callback: too few arguments\n");
   dmalloc_touch_svalue(sp-args);
   assign_svalue(&(THIS->callback__entity),sp-args);
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*
**! method object add_tag(string name,mixed to_do)
**! method object add_container(string name,mixed to_do)
**! method object add_entity(string entity,mixed to_do)
**! method object add_quote_tag(string name,mixed to_do,string end)
**! method object add_tags(mapping(string:mixed))
**! method object add_containers(mapping(string:mixed))
**! method object add_entities(mapping(string:mixed))
**!	Registers the actions to take when parsing various things.
**!	Tags, containers, entities are as usual. add_quote_tag() adds
**!	a special kind of tag that reads any data until the next
**!	occurrence of the end string immediately before a tag end.
**!
**!	<tt>to_do</tt> can be:
**!	<ul>
**!	<li><b>a function</b> to be called. The function is on the form
**!	<pre>
**!     mixed tag_callback(object parser,mapping args,mixed ...extra)
**!	mixed container_callback(object parser,mapping args,string content,mixed ...extra)
**!	mixed entity_callback(object parser,mixed ...extra)
**!	mixed quote_tag_callback(object parser,string content,mixed ...extra)
**!	</pre>
**!	depending on what realm the function is called by.
**!	<li><b>a string</b>. This tag/container/entity is then replaced
**!	by the string.
**!	<li><b>an array</b>. The first element is a function as above.
**!	It will receive the rest of the array as extra arguments. If
**!	extra arguments are given by <ref>set_extra</ref>(), they will
**!	appear after the ones in this array.
**!	<li><b>zero</b>. If there is a tag/container/entity with the
**!	given name in the parser, it's removed.
**!	</ul>
**!
**!     The callback function can return:
**!	<ul>
**!	<li><b>a string</b>; this string will be pushed on the parser
**!	stack and be parsed. Be careful not to return anything
**!	in this way that could lead to a infinite recursion.
**!	<li><b>an array</b>; the element(s) of the array is the result
**!	of the function. This will not be parsed. This is useful for
**!	avoiding infinite recursion. The array can be of any size,
**!	this means the empty array is the most effective to return if
**!	you don't care about the result. If the parser is operating in
**!	<ref>mixed_mode</ref>, the array can contain anything.
**!	Otherwise only strings are allowed.
**!	<li><b>zero</b>; this means "don't do anything", ie the
**!	item that generated the callback is left as it is, and
**!	the parser continues.
**!	<li><b>one</b>; reparse the last item again. This is useful to
**!	parse a tag as a container, or vice versa: just add or remove
**!	callbacks for the tag and return this to jump to the right
**!	callback.
**!	</ul>
**!
**! see also: tags, containers, entities
**!	
*/

static void html_add_tag(INT32 args)
{
   struct svalue s;
   check_all_args("add_tag",args,BIT_STRING,
		  BIT_INT|BIT_STRING|BIT_ARRAY|BIT_FUNCTION|BIT_OBJECT|BIT_PROGRAM,0);
   if (sp[1-args].type == T_ARRAY) {
     struct array *a = sp[1-args].u.array;
     if (!a->size ||
	 (a->item[0].type != T_FUNCTION && a->item[0].type != T_OBJECT &&
	  a->item[0].type != T_PROGRAM))
       SIMPLE_BAD_ARG_ERROR("add_tag", 1, "array with function as first element");
   }
   else if (sp[1-args].type == T_INT && sp[1-args].u.integer)
     SIMPLE_BAD_ARG_ERROR("add_tag", 1, "zero, string, array or function");

   if (THIS->maptag->refs>1)
   {
      push_mapping(THIS->maptag);
      THIS->maptag=copy_mapping(THIS->maptag);
      pop_stack();
      DEBUG((stderr,"COPY\n"));
   }
   s=*--sp;
   if (THIS->flags & FLAG_CASE_INSENSITIVE_TAG)
     f_lower_case(1);
   if (IS_ZERO(&s))
     map_delete(THIS->maptag,sp-1);
   else
     mapping_insert(THIS->maptag,sp-1,&s);
   free_svalue(&s);
   pop_n_elems(args-1);
   ref_push_object(THISOBJ);
}

static void html_add_container(INT32 args)
{
   struct svalue s;
   check_all_args("add_container",args,BIT_STRING,
		  BIT_INT|BIT_STRING|BIT_ARRAY|BIT_FUNCTION|BIT_OBJECT|BIT_PROGRAM,0);
   if (sp[1-args].type == T_ARRAY) {
     struct array *a = sp[1-args].u.array;
     if (!a->size ||
	 (a->item[0].type != T_FUNCTION && a->item[0].type != T_OBJECT &&
	  a->item[0].type != T_PROGRAM))
       SIMPLE_BAD_ARG_ERROR("add_container", 1, "array with function as first element");
   }
   else if (sp[1-args].type == T_INT && sp[1-args].u.integer)
     SIMPLE_BAD_ARG_ERROR("add_tag", 1, "zero, string, array or function");

   if (THIS->mapcont->refs>1)
   {
      push_mapping(THIS->mapcont);
      THIS->mapcont=copy_mapping(THIS->mapcont);
      pop_stack();
   }
   s=*--sp;
   if (THIS->flags & FLAG_CASE_INSENSITIVE_TAG)
     f_lower_case(1);
   if (IS_ZERO(&s))
     map_delete(THIS->mapcont,sp-1);
   else
     mapping_insert(THIS->mapcont,sp-1,&s);
   free_svalue(&s);
   pop_n_elems(args-1);
   ref_push_object(THISOBJ);
}

static void html_add_entity(INT32 args)
{
   check_all_args("add_entity",args,BIT_STRING,
		  BIT_INT|BIT_STRING|BIT_ARRAY|BIT_FUNCTION|BIT_OBJECT|BIT_PROGRAM,0);
   if (sp[1-args].type == T_ARRAY) {
     struct array *a = sp[1-args].u.array;
     if (!a->size ||
	 (a->item[0].type != T_FUNCTION && a->item[0].type != T_OBJECT &&
	  a->item[0].type != T_PROGRAM))
       SIMPLE_BAD_ARG_ERROR("add_entity", 1, "array with function as first element");
   }
   else if (sp[1-args].type == T_INT && sp[1-args].u.integer)
     SIMPLE_BAD_ARG_ERROR("add_tag", 1, "zero, string, array or function");

   if (THIS->mapentity->refs>1)
   {
      push_mapping(THIS->mapentity);
      THIS->mapentity=copy_mapping(THIS->mapentity);
      pop_stack();
   }
   if (IS_ZERO(sp-1))
     map_delete(THIS->mapentity,sp-2);
   else
     mapping_insert(THIS->mapentity,sp-2,sp-1);
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

static void html_add_quote_tag(INT32 args)
{
  int remove;
  struct pike_string *name, *end;
  struct pike_string *prefix;
  struct svalue *val;
  struct svalue cb;
  ONERROR uwp;

  check_all_args("add_quote_tag",args,BIT_STRING,
		 BIT_INT|BIT_STRING|BIT_ARRAY|BIT_FUNCTION|BIT_OBJECT|BIT_PROGRAM,
		 BIT_STRING|BIT_VOID,0);
   if (sp[1-args].type == T_ARRAY) {
     struct array *a = sp[1-args].u.array;
     if (!a->size ||
	 (a->item[0].type != T_FUNCTION && a->item[0].type != T_OBJECT &&
	  a->item[0].type != T_PROGRAM))
       SIMPLE_BAD_ARG_ERROR("add_quote_tag", 1, "array with function as first element");
   }
   else if (sp[1-args].type == T_INT && sp[1-args].u.integer)
     SIMPLE_BAD_ARG_ERROR("add_tag", 1, "zero, string, array or function");

  remove = IS_ZERO (sp+1-args);
  if (!remove && args < 3)
    SIMPLE_TOO_FEW_ARGS_ERROR ("add_quote_tag", 3);

  if (THIS->mapqtag->refs>1)
  {
    push_mapping(THIS->mapqtag);
    THIS->mapqtag=copy_mapping(THIS->mapqtag);
    pop_stack();
    DEBUG((stderr,"COPY\n"));
  }

  if (!remove) {
    struct pike_string *end = make_shared_binary_string2 (&THIS->tag_end, 1);
    pop_n_elems (args-3);
    args = 3;
    push_string (end);
    f_add (2);
  }

  name = dmalloc_touch (struct pike_string *, sp[-args].u.string);
  if (name->len > 2)
    prefix = string_slice (name, 0, 2);
  else
    copy_shared_string (prefix, name);
  SET_ONERROR (uwp, do_free_string, prefix);

  val = low_mapping_string_lookup (THIS->mapqtag, prefix);
  if (val) {
    int i;
    struct array *arr;
#ifdef PIKE_DEBUG
    if (val->type != T_ARRAY) fatal ("Expected array as value in mapqtag.\n");
#endif
    arr = val->u.array;

    for (i = 0; i < arr->size; i += 3) {
      struct pike_string *curname;
#ifdef PIKE_DEBUG
      if (arr->item[i].type != T_STRING) fatal ("Expected string as name in mapqtag.\n");
#endif
      curname = dmalloc_touch (struct pike_string *, arr->item[i].u.string);

      if (curname == name) {
	if (remove)
	  if (arr->size == 3) {
	    struct svalue tmp;
	    tmp.type = T_STRING;
	    tmp.u.string = prefix;
	    map_delete (THIS->mapqtag, &tmp);
	  }
	  else {
	    if (arr->refs > 1) {
	      arr = copy_array (arr);
	      free_array (val->u.array);
	      val->u.array = arr;
	    }
	    free_svalues (arr->item+i, 3, BIT_MIXED);
	    MEMCPY (arr->item+i, arr->item+i+3, (arr->size-i-3) * sizeof(struct svalue));
	    arr->size -= 3;
	  }
	else {
	  assign_svalue (arr->item+i+1, sp-2);
	  assign_svalue (arr->item+i+2, sp-1);
	}
	pop_n_elems (args);
	goto done;
      }

      else if (curname->len < name->len && !remove) {
	struct pike_string *cmp = string_slice (name, 0, curname->len);
	if (cmp == curname) { /* Found a shorter prefix to name; insert before. */
	  free_string (cmp);
	  if (arr->refs > 1) {
	    arr = copy_array (arr);
	    free_array (val->u.array);
	    val->u.array = arr;
	  }
	  arr = val->u.array = resize_array (arr, arr->size+3);
	  MEMCPY (arr->item+i+3, arr->item+i,
		  (arr->size-i-3) * sizeof(struct svalue));
	  MEMCPY (arr->item+i, sp-=3, 3 * sizeof(struct svalue));
	  goto done;
	}
	else free_string (cmp);
      }
    }

    if (!remove) {
      if (arr->refs > 1) {
	arr = copy_array (arr);
	free_array (val->u.array);
	val->u.array = arr;
      }
      arr = val->u.array = resize_array (arr, arr->size+3);
      MEMCPY (arr->item+arr->size-3, sp-=3, 3 * sizeof(struct svalue));
    }

  done:	;
  }

  else if (!remove) {
    f_aggregate (3);
    mapping_string_insert (THIS->mapqtag, prefix, sp-1);
    pop_stack();
  }

  free_string (prefix);
  UNSET_ONERROR (uwp);

  ref_push_object(THISOBJ);
}

static void html_add_tags(INT32 args)
{
   int sz;
   INT32 e;
   struct keypair *k;
   check_all_args("add_tags",args,BIT_MAPPING,0);

   sz=m_sizeof(sp[-1].u.mapping);

   MAPPING_LOOP(sp[-1].u.mapping)
      {
	 push_svalue(&k->ind);
	 push_svalue(&k->val);
	 html_add_tag(2);
	 pop_stack();
      }
   
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

static void html_add_containers(INT32 args)
{
   int sz;
   INT32 e;
   struct keypair *k;
   check_all_args("add_containers",args,BIT_MAPPING,0);

   sz=m_sizeof(sp[-1].u.mapping);

   MAPPING_LOOP(sp[-1].u.mapping)
      {
	 push_svalue(&k->ind);
	 push_svalue(&k->val);
	 html_add_container(2);
	 pop_stack();
      }
   
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

static void html_add_entities(INT32 args)
{
   int sz;
   INT32 e;
   struct keypair *k;
   check_all_args("add_entities",args,BIT_MAPPING,0);

   sz=m_sizeof(sp[-1].u.mapping);

   MAPPING_LOOP(sp[-1].u.mapping)
      {
	 push_svalue(&k->ind);
	 push_svalue(&k->val);
	 html_add_entity(2);
	 pop_stack();
      }
   
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*
**! method mapping tags()
**! method mapping containers()
**! method mapping entities()
**! method mapping quote_tags()
**!	Returns the current callback settings. For quote_tags, the
**!	values are arrays ({callback, end_quote}).
**!
**!	Note that when matching is done case insensitively, all names
**!	will be returned in lowercase.
**! see also: add_tag, add_tags, add_container, add_containers, add_entity, add_entities
*/

static void html_tags(INT32 args)
{
   pop_n_elems(args);
   ref_push_mapping(THIS->maptag);
}

static void html_containers(INT32 args)
{
   pop_n_elems(args);
   ref_push_mapping(THIS->mapcont);
}

static void html_entities(INT32 args)
{
   pop_n_elems(args);
   ref_push_mapping(THIS->mapentity);
}

static void html_quote_tags(INT32 args)
{
   struct mapping *res = allocate_mapping (32);
   INT32 e;
   struct keypair *k;
   pop_n_elems(args);
   MAPPING_LOOP(THIS->mapqtag) {
     int i;
     struct array *arr = k->val.u.array;
     for (i = 0; i < arr->size; i += 3) {
       struct pike_string *end;
#ifdef PIKE_DEBUG
      if (arr->item[i+2].type != T_STRING)
	fatal ("Expected string as end in mapqtag.\n");
#endif
       end = arr->item[i+2].u.string;
       end = string_slice (end, 0, end->len-2);
       push_svalue (arr->item+i+1);
       push_string (end);
       f_aggregate (2);
       mapping_insert (res, arr->item+i, sp-1);
       pop_stack();
     }
   }
   push_mapping (res);
}

/* ---------------------------------------- */
/* helper function to figure out what to do */

static INLINE void recheck_scan(struct parser_html_storage *this,
				int *scan_entity,
				int *scan_tag)
{
   if (this->callback__tag.type!=T_INT ||
       m_sizeof(this->maptag) ||
       m_sizeof(this->mapcont) ||
       m_sizeof(this->mapqtag))
      *scan_tag=1;
   else 
      *scan_tag=0;

   if (this->callback__entity.type!=T_INT ||
       m_sizeof(this->mapentity))
      *scan_entity=1;
   else 
      *scan_entity=0;
}

/* -------------- */
/* feed to output */

static void put_out_feed(struct parser_html_storage *this,
			 struct svalue *v)
{
   struct out_piece *f;

   f=malloc(sizeof(struct out_piece));
   if (!f)
      error("Parser.HTML(): out of memory\n");
   assign_svalue_no_free(&f->v,v);

   f->next=NULL;

   if (this->out_end==NULL)
     this->out=this->out_end=f;
   else
   {
      this->out_end->next=f;
      this->out_end=f;
   }
}

/* ---------------------------- */
/* feed range in feed to output */

static void put_out_feed_range(struct parser_html_storage *this,
			       struct piece *head,
			       int c_head,
			       struct piece *tail,
			       int c_tail)
{
   DEBUG((stderr,"put out feed range %p:%d - %p:%d\n",
	  head,c_head,tail,c_tail));
   /* fit it in range (this allows other code to ignore eof stuff) */
   if (c_tail>tail->s->len) c_tail=tail->s->len;
   while (head)
   {
      if (head==tail)
      {
	 if (c_tail-c_head)	/* Ignore empty strings. */
	 {
	   push_string(string_slice(head->s,c_head,c_tail-c_head));
	   put_out_feed(this,sp-1);
	   pop_stack();
	 }
	 return;
      }
      if (head->s->len-c_head)	/* Ignore empty strings. */
      {
	push_string(string_slice(head->s,c_head,head->s->len-c_head));
	put_out_feed(this,sp-1);
	pop_stack();
      }
      c_head=0;
      head=head->next;
   }
   error("internal error: tail not found in feed (put_out_feed_range)\n");
}

/* ------------------------ */
/* push feed range on stack */

static INLINE void push_feed_range(struct piece *head,
				   int c_head,
				   struct piece *tail,
				   int c_tail)
{
   int n=0;
   if (head==tail && c_head==c_tail)
   {
      DEBUG((stderr,"push len=0\n"));
      ref_push_string(empty_string);
      return;
   }
   /* fit it in range (this allows other code to ignore eof stuff) */
   if (c_tail>tail->s->len) c_tail=tail->s->len;
   while (head)
   {
      if (head==tail)
      {
	 if (c_head >= c_tail)
	   ref_push_string(empty_string);
	 else
	   push_string(string_slice(head->s,c_head,c_tail-c_head));
	 n++;
	 break;
      }
      push_string(string_slice(head->s,c_head,head->s->len-c_head));
      n++;
      if (n==32)
      {
	 f_add(32);
	 n=1;
      }
      c_head=0;
      head=head->next;
   }
   if (!head)
      error("internal error: tail not found in feed (push_feed_range)\n");
   if (!n)
      ref_push_string(empty_string);
   else if (n>1)
      f_add(n);
   DEBUG((stderr,"push len=%d\n",sp[-1].u.string->len));
}

/* -------------------------------------------------------- */
/* go forward by adjusting for a positive displacement in c */

static INLINE int n_pos_forward (struct piece *feed, int c,
				 struct piece **dest, int *dp)
{
  while (feed->s->len <= c) {
    if (!feed->next) {
      *dest = feed;
      *dp = feed->s->len;
      return 0;
    }
    c -= feed->s->len;
    feed = feed->next;
  }
  *dest = feed;
  *dp = c;
  return 1;
}

/* ------------------------- */
/* compare positions in feed */

static INLINE int cmp_feed_pos(struct piece *piece_a, int pos_a,
			       struct piece *piece_b, int pos_b)
{
  struct piece *a_save = piece_a;

  if (piece_a == piece_b)
    return pos_a < pos_b ? -1 : (pos_a > pos_b ? 1 : 0);

  while (piece_a && piece_a != piece_b)
    piece_a = piece_a->next;
  if (piece_a) return -1;

  while (piece_b && piece_b != a_save)
    piece_b = piece_b->next;
  if (piece_b) return 1;

  return 17;			/* Not in the same feed! */
}

/* ------------------------------------ */
/* skip feed range and count lines, etc */

/* count lines, etc */
static INLINE void skip_piece_range(struct location *loc,
				    struct piece *p,
				    int start,
				    int stop)
{
   int b=loc->byteno;
   switch (p->s->size_shift)
   {
      case 0:
      {
	 p_wchar0 *s=(p_wchar0 *)p->s->str;
	 for (;start<stop;start++)
	 {
	    if (*(s++)=='\n') 
	    {
	       loc->linestart=b;
	       loc->lineno++;
	    }
	    b++;
	 }
      }
      break;
      case 1:
      {
	 p_wchar1 *s=(p_wchar1*)p->s->str;
	 for (;start<stop;start++)
	 {
	    if (*(s++)=='\n') 
	    {
	       loc->linestart=b;
	       loc->lineno++;
	    }
	    b++;
	 }
      }
      break;
      case 2:
      {
	 p_wchar2 *s=(p_wchar2*)p->s->str;
	 for (;start<stop;start++)
	 {
	    if (*(s++)=='\n') 
	    {
	       loc->linestart=b;
	       loc->lineno++;
	    }
	    b++;
	 }
      }
      break;
      default:
	 error("unknown width of string\n");
   }
   loc->byteno=b;
}

static void skip_feed_range(struct feed_stack *st,
			    struct piece **headp,
			    int *c_headp,
			    struct piece *tail,
			    int c_tail)
{
   struct piece *head=*headp;
   int c_head=*c_headp;

   if (!*headp)
   {
      DEBUG((stderr,"skip_feed_range: already at end<tm>\n"));
      return;
   }

   DEBUG_MARK_SPOT("skip_feed_range from",*headp,*c_headp);
   DEBUG_MARK_SPOT("                  to",tail,c_tail);

   while (head)
   {
      if (head==tail && c_tail<tail->s->len)
      {
	 skip_piece_range(&(st->pos),head,c_head,c_tail);
	 *c_headp=c_tail;
	 return;
      }
      skip_piece_range(&(st->pos),head,c_head,head->s->len);
      *headp=head->next;
      free_string(head->s);
      free(head);
      head=*headp;
      c_head=0;
   }
}

/* ------------------------------ */
/* scan forward for certain chars */

static int scan_forward(struct piece *feed,
			int c,
			struct piece **destp,
			int *d_p,
			p_wchar2 *look_for,
			int num_look_for) /* negative = skip those */
{
   int rev=0;

   if (num_look_for<0) num_look_for=-num_look_for,rev=1;

#ifdef SCAN_DEBUG
   DEBUG_MARK_SPOT("scan_forward",feed,c);
   do
   {
      int i=0;
      fprintf(stderr,"    n=%d%s; ",num_look_for,rev?"; rev":"");
      for (i=0; i<num_look_for; i++)
	 if (look_for[i]<33 || (look_for[i]>126 && look_for[i]<160)
	     || look_for[i]>255)
	    fprintf(stderr,"%d ",look_for[i]);
	 else
	    fprintf(stderr,"%d:'%c' ",look_for[i],look_for[i]);
      fprintf(stderr,"\n");
   }
   while(0);
#define SCAN_DEBUG_MARK_SPOT(A,B,C) DEBUG_MARK_SPOT(A,B,C)
#else
#define SCAN_DEBUG_MARK_SPOT(A,B,C) do {} while (0);
#endif


   switch (num_look_for)
   {
      case 0: /* optimize, skip to end */
	 while (feed->next)
	    feed=feed->next;
	 *destp=feed;
	 *d_p=feed->s->len;
	 SCAN_DEBUG_MARK_SPOT("scan_forward end (nothing to look for)",
			      *destp,*d_p);
	 return 0; /* not found :-) */
	 
      case 1: 
	 if (!rev)
	    while (feed)
	    {
	       int ce=feed->s->len-c;
	       p_wchar2 f=*look_for;
	       SCAN_DEBUG_MARK_SPOT("scan_forward piece loop (1)",feed,c);
	       switch (feed->s->size_shift)
	       {
		  case 0:
		  {
		     p_wchar0*s=((p_wchar0*)feed->s->str)+c;
		     while (ce-- > 0)
			if ((p_wchar2)*(s++)==f)
			{
			   c=feed->s->len-ce;
			   goto found;
			}
		  }
		  break;
		  case 1:
		  {
		     p_wchar1*s=((p_wchar1*)feed->s->str)+c;
		     while (ce-- > 0)
			if ((p_wchar2)*(s++)==f)
			{
			   c=feed->s->len-ce;
			   goto found;
			}
		  }
		  break;
		  case 2:
		  {
		     p_wchar2 f=(p_wchar2)*look_for;
		     p_wchar2*s=((p_wchar2*)feed->s->str)+c;
		     while (ce-- > 0)
			if (*(s++)==f)
			{
			   c=feed->s->len-ce;
			   goto found;
			}
		  }
		  break;
		  default:
		     error("unknown width of string\n");
	       }
	       if (!feed->next) break;
	       c=0;
	       feed=feed->next;
	    }
	 break;

      default:  /* num_look_for > 1 */
	 while (feed)
	 {
	    int ce=feed->s->len-c;
	    SCAN_DEBUG_MARK_SPOT("scan_forward piece loop (>1)",feed,c);
	    switch (feed->s->size_shift)
	    {
	       case 0:
	       {
		  int n;
		  p_wchar0*s=((p_wchar0*)feed->s->str)+c;
		  while (ce-- > 0)
		  {
		     for (n=0; n<num_look_for; n++)
			if (((p_wchar2)*s)==look_for[n])
			{
			   c=feed->s->len-ce;
			   if (!rev) goto found; else break;
			}
		     if (rev && n==num_look_for) goto found;
		     s++;
		  }
	       }
	       break;
	       case 1:
	       {
		  int n;
		  p_wchar1*s=((p_wchar1*)feed->s->str)+c;
		  while (ce-- > 0)
		  {
		     for (n=0; n<num_look_for; n++)
			if (((p_wchar2)*s)==look_for[n])
			{
			   c=feed->s->len-ce;
			   if (!rev) goto found; else break;
			}
		     if (rev && n==num_look_for) goto found;
		     s++;
		  }
	       }
	       break;
	       case 2:
	       {
		  int n;
		  p_wchar2*s=((p_wchar2*)feed->s->str)+c;
		  while (ce-- > 0)
		  {
		     for (n=2; n<num_look_for; n++)
			if (((p_wchar2)*s)==look_for[n])
			{
			   c=feed->s->len-ce;
			   if (!rev) goto found; else break;
			}
		     if (rev && n==num_look_for) goto found;
		     s++;
		  }
	       }
	       break;
	       default:
		  error("unknown width of string\n");
	    }
	    if (!feed->next) break;
	    c=0;
	    feed=feed->next;
	 }
	 break;
   }

   SCAN_DEBUG_MARK_SPOT("scan_forward not found",feed,feed->s->len);
   *destp=feed;
   *d_p=feed->s->len;
   return 0; /* not found */

found:
   *destp=feed;
   *d_p=c-!rev; 
   SCAN_DEBUG_MARK_SPOT("scan_forward found",feed,c-!rev);
   return 1;
}

static int scan_for_string (struct parser_html_storage *this,
			    struct piece *feed,
			    int c,
			    struct piece **destp,
			    int *d_p,
			    struct pike_string *str)
{
  if (!str->len) {
    *destp = feed;
    *d_p = c;
    return 1;
  }

#define LOOP(TYPE) {							\
    p_wchar2 look_for = (p_wchar2) ((TYPE *) str->str)[0];		\
    for (;;) {								\
      TYPE *p, *e;							\
      struct piece *dst;						\
      int cdst;								\
      if (!scan_forward (feed, c, &feed, &c, &look_for, 1)) return 0;	\
									\
      p = (TYPE *) str->str + 1;					\
      e = (TYPE *) str->str + str->len;					\
      dst = feed;							\
      cdst = c + 1;							\
      for (; p < e; p++, cdst++) {					\
	while (cdst == dst->s->len) {					\
	  if (!(dst = dst->next)) return 0;				\
	  cdst = 0;							\
	}								\
	if ((p_wchar2) *p !=						\
	    INDEX_CHARP (dst->s->str, cdst, dst->s->size_shift))	\
	  goto PIKE_CONCAT (cont, TYPE);				\
      }									\
									\
      *destp = feed;							\
      *d_p = c;								\
      return 1;								\
									\
    PIKE_CONCAT (cont, TYPE):						\
      c++;								\
    }									\
  }

  switch (str->size_shift) {
    case 0: LOOP (p_wchar0); break;
    case 1: LOOP (p_wchar1); break;
    case 2: LOOP (p_wchar2); break;
    default: error ("Unknown width of string.\n");
  }

#undef LOOP

  return 0;
}

static int scan_forward_arg(struct parser_html_storage *this,
			    struct piece *feed,
			    int c,
			    struct piece **destp,
			    int *d_p,
			    int do_push,
			    int finished)
{
   p_wchar2 ch;
   int res,i;
   int n=0;
   int q=0;
   int end=0;

   DEBUG_MARK_SPOT("scan_forward_arg: start",feed,c);

   for (;;)
   {
      /* we are here: */
      /* < f'o'"o" = bar > */
      /*   ^     ->        */
      /*      this is the end */

      DEBUG_MARK_SPOT("scan_forward_arg: loop",feed,c);

      res=scan_forward(feed,c,destp,d_p,
		       this->ws_or_endarg_or_quote,
		       this->n_ws_or_endarg_or_quote);

      if (do_push) push_feed_range(feed,c,*destp,*d_p),n++;

      if (!res)
	 if (!finished) 
	 {
	    DEBUG_MARK_SPOT("scan for end of arg: wait",feed,c);
	    if (do_push) pop_n_elems(n);
	    return 0;
	 }
	 else
	 {
	    DEBUG_MARK_SPOT("scan for end of arg: forced end",destp[0],*d_p);
	    break;
	 }

      ch=index_shared_string(destp[0]->s,*d_p);

      if (ch==this->arg_eq)
      {
	 DEBUG_MARK_SPOT("scan for end of arg: end by arg_eq",
			 destp[0],*d_p);
	 break;
      }

      if (ch==this->tag_end)
	 if ((this->flags & FLAG_MATCH_TAG) && q--)
	 {
	    DEBUG_MARK_SPOT("scan for end of arg: inner tag end",
			    destp[0],*d_p);
	    if (do_push) push_feed_range(*destp,*d_p,*destp,*d_p+1),n++;
	    goto next;
	 }
	 else
	 {
	    DEBUG_MARK_SPOT("scan for end of arg: end by tag end",
			    destp[0],*d_p);
	    break;
	 }

      if ((this->flags & FLAG_MATCH_TAG) && ch==this->tag_start)
      {
	 DEBUG_MARK_SPOT("scan for end of arg: inner tag start",
			 destp[0],*d_p);
	 q++;
	 if (do_push) push_feed_range(*destp,*d_p,*destp,*d_p+1),n++;
	 goto next;
      }

      /* scan for (possible) end(s) of this argument quote */

      for (i=0; i<this->nargq; i++)
	 if (ch==this->argq_start[i]) break;
      if (i==this->nargq) /* it was whitespace */
      {
	 DEBUG_MARK_SPOT("scan for end of arg: end by whitespace",
			 destp[0],*d_p);
	 break;
      }

      DEBUG_MARK_SPOT("scan for end of arg: quoted",destp[0],*d_p);
      res=scan_forward(feed=*destp,c=d_p[0]+1,destp,d_p,
		       this->look_for_end[i],this->num_look_for_end[i]);
      if (do_push) push_feed_range(feed,c,*destp,*d_p),n++;

      if (!res)
	 if (!finished) 
	 {
	    DEBUG_MARK_SPOT("scan for end of arg: wait (quote)",feed,c);
	    if (do_push) pop_n_elems(n);
	    return 0;
	 }
	 else
	 {
	    DEBUG_MARK_SPOT("scan for end of arg: forced end (quote)",destp[0],*d_p);
	    break;
	 }

next:

      feed=*destp;
      c=d_p[0]+1;
   }

   if (do_push) 
   {
      if (n>1) f_add(n);
      else if (!n) ref_push_string(empty_string);
   }
   return 1;
}

static int scan_for_end_of_tag(struct parser_html_storage *this,
			       struct piece *feed,
			       int c,
			       struct piece **destp,
			       int *d_p,
			       int finished,
			       int match_tag)
/* match_tag == 0: don't match inner tags, > 0: match inner tags, < 0:
 * consider it an unbalanced tag and break */
{
   p_wchar2 ch;
   int res,i;
   int q=0;

   /* maybe these should be cached */

   /* bla bla <tag foo 'bar' "gazonk" > */
   /*          ^                      ^ */
   /*       here now             scan here */

   DEBUG((stderr,"scan for end of tag: %p:%d\n",feed,c));

   for (;;)
   {
      /* scan for start of argument quote or end of tag */

      res=scan_forward(feed,c,destp,d_p,
		       this->look_for_start,this->num_look_for_start);
      if (!res) 
	 if (!finished) 
	 {
	    DEBUG((stderr,"scan for end of tag: wait at %p:%d\n",feed,c));
	    return 0; /* not found - no end of tag, yet */
	 }
	 else
	 {
	    DEBUG((stderr,"scan for end of tag: forced end at %p:%d\n",
		   destp[0],*d_p));
	    return 1; /* end of tag, sure... */
	 }

      ch=index_shared_string(destp[0]->s,*d_p);
      if (ch==this->arg_eq)
      {
	 DEBUG_MARK_SPOT("scan for end of tag: arg_eq",
			 destp[0],*d_p);
	 feed=*destp;
	 c=d_p[0]+1;
	 continue;
      }

      if (ch==this->tag_end)
	if (match_tag && q--)
	 {
	    DEBUG_MARK_SPOT("scan for end of tag: inner tag end",
			    destp[0],*d_p);
	    feed=*destp;
	    c=d_p[0]+1;
	    continue; /* Aja baja /hubbe: //scan more  */
	 }
	 else
	 {
	    DEBUG_MARK_SPOT("scan for end of tag: end by tag end",
			    destp[0],*d_p);
	    return 1;
	 }

      if (ch==this->tag_start)
	if (match_tag > 0)
	{
	  DEBUG_MARK_SPOT("scan for end of tag: inner tag start",
			  destp[0],*d_p);
	  q++;
	  feed=*destp;
	  c=d_p[0]+1;
	  continue;
	}
	else if (match_tag < 0)
	{
	  DEBUG_MARK_SPOT("scan for end of tag: unbalanced tag start",
			  destp[0],*d_p);
	  return 1;
	}
	else
	{
	  DEBUG_MARK_SPOT("scan for end of tag: ignored inner tag start",
			  destp[0],*d_p);
	  feed=*destp;
	  c=d_p[0]+1;
	  continue;
	}

      /* scan for (possible) end(s) of this argument quote */

      for (i=0; i<this->nargq; i++)
	 if (ch==this->argq_start[i]) break;
      res=scan_forward(*destp,d_p[0]+1,destp,d_p,
		       this->look_for_end[i],this->num_look_for_end[i]);
      if (!res)
	 if (!finished) 
	 {
	    DEBUG((stderr,"scan for end of tag: wait at %p:%d\n",
		   destp[0],*d_p));
	    return 0; /* not found - no end of tag, yet */
	 }
	 else
	 {
	    DEBUG((stderr,"scan for end of tag: forced end at %p:%d\n",
		   feed,c));
	    return 1; /* end of tag, sure... */
	 }

      feed=*destp;
      c=d_p[0]+1;
   }
}

/* ---------------------------------- */

struct chr_buf_len2
{
  p_wchar2 str[2];
  struct piece *p[2];
  int c[2];
};

static int quote_tag_lookup (struct parser_html_storage *this,
			     struct piece *feed, int c,
			     struct piece **destp, int *d_p,
			     int finished, struct svalue **mapqentry)
/* Checks for a quote tag starting at the given position. If one is
 * found, scans to the end of it and sets *mapqentry to the first
 * svalue in the ({name, to_do, end}) tuple in this->mapqtag. */
{
  struct chr_buf_len2 buf;
  struct piece *dst = feed;
  int cdst = c;
  size_t checklen;
  *mapqentry = NULL;

  DEBUG_MARK_SPOT ("quote tag lookup", feed, c);

  for (checklen = 0; checklen < sizeof (buf.str) / sizeof (buf.str[0]); checklen++) {
    while (cdst == dst->s->len) {
      if (!(dst = dst->next)) {
	/* This is not entirely correct; we should continue if mapqtag
	 * doesn't contain any longer indices whose prefixes match
	 * what we got so far. But it's not much of a problem as long
	 * as we're indexing on only two chars or less. */
	DEBUG ((stderr, "quote tag lookup: wait at prefix\n"));
	return finished;	/* 0 => wait, 1 => no go - *mapqentry still NULL */
      }
      cdst = 0;
    }
    buf.str[checklen] = index_shared_string (dst->s, cdst);
    buf.p[checklen] = dst;
    buf.c[checklen] = ++cdst;
  }

  for (; checklen; checklen--) {
    struct pike_string *indstr = make_shared_binary_string2 (buf.str, checklen);
    struct svalue *val = low_mapping_string_lookup (this->mapqtag, indstr);
    free_string (indstr);

    if (val) {
      int i;
      struct array *arr;
      DEBUG ((stderr, "quote tag lookup: found entry %c%c at %d\n",
	      isprint (buf.str[0]) ? buf.str[0] : '.',
	      isprint (buf.str[1]) ? buf.str[1] : '.', checklen));
#ifdef PIKE_DEBUG
      if (val->type != T_ARRAY) fatal ("Expected array as value in mapqtag.\n");
#endif
      arr = val->u.array;

      for (i = 0; i < arr->size; i += 3) {
	struct pike_string *tag;
#ifdef PIKE_DEBUG
	if (arr->item[i].type != T_STRING)
	  fatal ("Expected string as name in mapqtag.\n");
#endif
	tag = arr->item[i].u.string;
	dst = buf.p[checklen-1];
	cdst = buf.c[checklen-1];

#define LOOP(TYPE) {							\
	  TYPE *p = (TYPE *) tag->str + checklen;			\
	  TYPE *e = (TYPE *) tag->str + tag->len;			\
	  for (; p < e; p++, cdst++) {					\
	    while (cdst == dst->s->len) {				\
	      if (!(dst = dst->next)) {					\
		DEBUG ((stderr, "quote tag lookup: wait\n"));		\
		return finished; /* 0/1 => see above */			\
	      }								\
	      cdst = 0;							\
	    }								\
	    if ((p_wchar2) *p !=					\
		INDEX_CHARP (dst->s->str, cdst, dst->s->size_shift))	\
	      goto cont;						\
	  }								\
	}

	switch (tag->size_shift) {
	  case 0: LOOP (p_wchar0); break;
	  case 1: LOOP (p_wchar1); break;
	  case 2: LOOP (p_wchar2); break;
	  default: error ("Unknown width of string.\n");
	}

#undef LOOP

	DEBUG_MARK_SPOT ("quote tag lookup: matched quote tag", dst, cdst);
	*mapqentry = arr->item+i;
	*destp = dst;
	*d_p = cdst;
	return 1;

      cont: ;
      }
    }
#ifdef DEBUG
    else
      DEBUG ((stderr, "quote tag lookup: no entry %c%c at %d\n",
	      isprint (buf.str[0]) ? buf.str[0] : '.',
	      isprint (buf.str[1]) ? buf.str[1] : '.', checklen));
#endif
  }

  DEBUG ((stderr, "quote tag lookup: no quote tag\n"));
  return 1;			/* 1 => no go - *mapqentry still NULL */
}

/* ---------------------------------------------------------------- */
/* this is called to get data from callbacks and do the right thing */

static newstate handle_result(struct parser_html_storage *this,
			      struct feed_stack *st,
			      struct piece **head,
			      int *c_head,
			      struct piece *tail,
			      int c_tail)
{
   struct feed_stack *st2;
   int i;

   /* on sp[-1]:
      string: push it to feed stack 
      int: noop, output range
      array(string): output string */

   switch (sp[-1].type)
   {
      case T_STRING: /* push it to feed stack */
	 
	 /* first skip this in local feed*/
	 skip_feed_range(st,head,c_head,tail,c_tail);

	 DEBUG((stderr,"handle_result: pushing string (len=%d) on feedstack\n",
		sp[-1].u.string->len));

	 st2=malloc(sizeof(struct feed_stack));
	 if (!st2)
	    error("out of memory\n");

	 st2->local_feed=malloc(sizeof(struct piece));
	 if (!st2->local_feed)
	 {
	    free(st2);
	    error("out of memory\n");
	 }
	 copy_shared_string(st2->local_feed->s,sp[-1].u.string);
	 st2->local_feed->next=NULL;
	 pop_stack();
	 st2->ignore_data=0;
	 st2->pos=this->stack->pos;
	 st2->prev=this->stack;
	 st2->c=0;
	 this->stack=st2;
	 THIS->stack_count++;
	 return STATE_REREAD; /* please reread stack head */

      case T_INT:
	 switch (sp[-1].u.integer)
	 {
	    case 0:
	       /* just output range */
	       put_out_feed_range(this,*head,*c_head,tail,c_tail);
	       skip_feed_range(st,head,c_head,tail,c_tail);
	       pop_stack();
	       if (this->stack != st) /* got more feed recursively - reread */
		  return STATE_REREAD;
	       return STATE_DONE; /* continue */
	    case 1:
	       /* reparse the last thing */
	       pop_stack();
	       return STATE_REPARSE;
	 }
	 error("Parser.HTML: illegal result from callback: %d, "
	       "not 0 (skip) or 1 (wait)\n",
	       sp[-1].u.integer);

      case T_ARRAY:
	 /* output element(s) */
	 for (i=0; i<sp[-1].u.array->size; i++)
	 {
	    if (!(THIS->flags & FLAG_MIXED_MODE) &&
		sp[-1].u.array->item[i].type!=T_STRING)
	       error("Parser.HTML: illegal result from callback: element in array not string\n");
	    push_svalue(sp[-1].u.array->item+i);
	    put_out_feed(this,sp-1);
	    pop_stack();
	 }
	 skip_feed_range(st,head,c_head,tail,c_tail);
	 pop_stack();
	 if (this->stack != st) /* got more feed recursively - reread */
	    return STATE_REREAD;
	 return STATE_DONE; /* continue */

      default:
	 error("Parser.HTML: illegal result from callback: not 0, string or array(string)\n");
   }   
}

static void clear_start(struct parser_html_storage *this)
{
   this->start=NULL;
}

static void do_callback(struct parser_html_storage *this,
			struct object *thisobj,
			struct svalue *callback_function,
			struct piece *start,
			int cstart,
			struct piece *end,
			int cend,
			enum types type)
{
   ONERROR uwp;

   this->start=start;
   this->cstart=cstart;
   this->end=end;
   this->cend=cend;
   this->type=type;

   SET_ONERROR(uwp,clear_start,this);

   ref_push_object(thisobj);
   if (start)
      push_feed_range(start,cstart,end,cend);
   else 
      ref_push_string(empty_string);

   if (this->extra_args)
   {
      add_ref(this->extra_args);
      push_array_items(this->extra_args);

      DEBUG((stderr,"_-callback args=%d\n",2+this->extra_args->size));

      dmalloc_touch_svalue(callback_function);
      apply_svalue(callback_function,2+this->extra_args->size);  
   }
   else
   {
      DEBUG((stderr,"_-callback args=%d\n",2));

      dmalloc_touch_svalue(callback_function);
      apply_svalue(callback_function,2);  
   }

   UNSET_ONERROR(uwp);
   this->start=NULL;
}

static newstate entity_callback(struct parser_html_storage *this,
				struct object *thisobj,
				struct svalue *v,
				struct feed_stack *st,
				struct piece **cutstart, int *ccutstart,
				struct piece *cutend, int ccutend)
{
   int args;
   ONERROR uwp;

   switch (v->type)
   {
      case T_STRING:
	 push_svalue(v);
	 put_out_feed(this,sp-1);
	 pop_stack();
	 skip_feed_range(st,cutstart,ccutstart,cutend,ccutend);
	 return STATE_DONE;
      case T_FUNCTION:
      case T_OBJECT:
	 push_svalue(v);
	 break;
      case T_ARRAY:
	 if (v->u.array->size)
	 {
	    push_svalue(v->u.array->item);
	    break;
	 }
      default:
	 error("Parser.HTML: illegal type found when trying to call callback\n");
   }

   this->start=*cutstart;
   this->cstart=*ccutstart;
   this->end=cutend;
   this->cend=ccutend;
   this->type=TYPE_ENTITY;

   SET_ONERROR(uwp,clear_start,this);

   args=2;
   ref_push_object(thisobj);

   if (v->type==T_ARRAY && v->u.array->size>1)
   {
      assign_svalues_no_free(sp,v->u.array->item+1,
			     v->u.array->size-1,v->u.array->type_field);
      sp+=v->u.array->size-1;
      args+=v->u.array->size-1;
   }

   if (this->extra_args)
   {
      add_ref(this->extra_args);
      push_array_items(this->extra_args);
      args+=this->extra_args->size;
   }

   DEBUG((stderr,"entity_callback args=%d\n",args));
   f_call_function(args);

   UNSET_ONERROR(uwp);
   this->start=NULL;

   return handle_result(this,st,cutstart,ccutstart,cutend,ccutend);
}

static newstate tag_callback(struct parser_html_storage *this,
			     struct object *thisobj,
			     struct svalue *v,
			     struct feed_stack *st,
			     struct piece **cutstart, int *ccutstart,
			     struct piece *cutend, int ccutend)
{
   int args;
   ONERROR uwp;

   switch (v->type)
   {
      case T_STRING:
	 push_svalue(v);
	 put_out_feed(this,sp-1);
	 pop_stack();
	 skip_feed_range(st,cutstart,ccutstart,cutend,ccutend);
	 return STATE_DONE;
      case T_FUNCTION:
      case T_OBJECT:
	 push_svalue(v);
	 break;
      case T_ARRAY:
	 if (v->u.array->size)
	 {
	    push_svalue(v->u.array->item);
	    break;
	 }
      default:
	 error("Parser.HTML: illegal type found when trying to call callback\n");
   }

   this->start=*cutstart;
   this->cstart=*ccutstart;
   this->end=cutend;
   this->cend=ccutend;
   this->type=TYPE_TAG;

   SET_ONERROR(uwp,clear_start,this);

   args=3;
   ref_push_object(thisobj);
   tag_args(this,this->start,this->cstart,NULL);

   if (v->type==T_ARRAY && v->u.array->size>1)
   {
      assign_svalues_no_free(sp,v->u.array->item+1,
			     v->u.array->size-1,v->u.array->type_field);
      sp+=v->u.array->size-1;
      args+=v->u.array->size-1;
   }

   if (this->extra_args)
   {
      add_ref(this->extra_args);
      push_array_items(this->extra_args);
      args+=this->extra_args->size;
   }

   DEBUG((stderr,"tag_callback args=%d\n",args));
   f_call_function(args);

   UNSET_ONERROR(uwp);
   this->start=NULL;

   return handle_result(this,st,cutstart,ccutstart,cutend,ccutend);
}

static newstate container_callback(struct parser_html_storage *this,
				   struct object *thisobj,
				   struct svalue *v,
				   struct piece *startc, int cstartc,
				   struct piece *endc, int cendc,
				   struct feed_stack *st,
				   struct piece **cutstart, int *ccutstart,
				   struct piece *cutend, int ccutend)
{
   int args;
   ONERROR uwp;

   switch (v->type)
   {
      case T_STRING:
	 push_svalue(v);
	 put_out_feed(this,sp-1);
	 pop_stack();
	 skip_feed_range(st,cutstart,ccutstart,cutend,ccutend);
	 return STATE_DONE; /* done */
      case T_FUNCTION:
      case T_OBJECT:
	 push_svalue(v);
	 break;
      case T_ARRAY:
	 if (v->u.array->size)
	 {
	    push_svalue(v->u.array->item);
	    break;
	 }
      default:
	 error("Parser.HTML: illegal type found when trying to call callback\n");
   }

   this->start=*cutstart;
   this->cstart=*ccutstart;
   this->end=cutend;
   this->cend=ccutend;
   this->type=TYPE_CONT;

   SET_ONERROR(uwp,clear_start,this);

   args=4;
   ref_push_object(thisobj);
   tag_args(this,this->start,this->cstart,NULL);
   push_feed_range(startc,cstartc,endc,cendc);

   if (v->type==T_ARRAY && v->u.array->size>1)
   {
      assign_svalues_no_free(sp,v->u.array->item+1,
			     v->u.array->size-1,v->u.array->type_field);
      sp+=v->u.array->size-1;
      args+=v->u.array->size-1;
   }

   if (this->extra_args)
   {
      add_ref(this->extra_args);
      push_array_items(this->extra_args);
      args+=this->extra_args->size;
   }

   DEBUG((stderr,"container_callback args=%d\n",args));
   f_call_function(args);

   UNSET_ONERROR(uwp);
   this->start=NULL;

   return handle_result(this,st,cutstart,ccutstart,cutend,ccutend);
}

static newstate quote_tag_callback(struct parser_html_storage *this,
				   struct object *thisobj,
				   struct svalue *v,
				   struct piece *startc, int cstartc,
				   struct piece *endc, int cendc,
				   struct feed_stack *st,
				   struct piece **cutstart, int *ccutstart,
				   struct piece *cutend, int ccutend)
{
   int args;
   ONERROR uwp;

   switch (v->type)
   {
      case T_STRING:
	 push_svalue(v);
	 put_out_feed(this,sp-1);
	 pop_stack();
	 skip_feed_range(st,cutstart,ccutstart,cutend,ccutend);
	 return STATE_DONE; /* done */
      case T_FUNCTION:
      case T_OBJECT:
	 push_svalue(v);
	 break;
      case T_ARRAY:
	 if (v->u.array->size)
	 {
	    push_svalue(v->u.array->item);
	    break;
	 }
      default:
	 error("Parser.HTML: illegal type found when trying to call callback\n");
   }

   this->start=*cutstart;
   this->cstart=*ccutstart;
   this->end=cutend;
   this->cend=ccutend;
   this->type=TYPE_QTAG;

   SET_ONERROR(uwp,clear_start,this);

   args=3;
   ref_push_object(thisobj);
   push_feed_range(startc,cstartc,endc,cendc);

   if (v->type==T_ARRAY && v->u.array->size>1)
   {
      assign_svalues_no_free(sp,v->u.array->item+1,
			     v->u.array->size-1,v->u.array->type_field);
      sp+=v->u.array->size-1;
      args+=v->u.array->size-1;
   }

   if (this->extra_args)
   {
      add_ref(this->extra_args);
      push_array_items(this->extra_args);
      args+=this->extra_args->size;
   }

   DEBUG((stderr,"quote_tag_callback args=%d\n",args));
   f_call_function(args);

   UNSET_ONERROR(uwp);
   this->start=NULL;

   return handle_result(this,st,cutstart,ccutstart,cutend,ccutend);
}

static newstate find_end_of_container(struct parser_html_storage *this,
				      struct svalue *tagname,
				      struct svalue *endtagname,
				      struct piece *feed,int c,
				      struct piece **e1,int *ce1,
				      struct piece **e2,int *ce2,
				      int finished)
{
   struct piece *s1,*s2,*s3;
   int c1,c2,c3;
   newstate res;
   struct svalue e;

   if (!endtagname)
   {
      push_constant_text("/");
      push_svalue(tagname);
      f_add(2);
      e=sp[-1];
      endtagname=&e;
      sp--;
      dmalloc_touch_svalue(sp);
   }
   else
      add_ref_svalue(endtagname);

   DEBUG_MARK_SPOT("find_end_of_container",feed,c);

   for (;;)
   {
      int found;

      DEBUG_MARK_SPOT("find_end_of_cont loop",feed,c);
      if (!scan_forward(feed,c,&s1,&c1,&(this->tag_start),1))
      {
	 if (!finished) 
	 {
	    DEBUG_MARK_SPOT("find_end_of_cont : wait",s1,c1);
	    free_svalue(endtagname);
	    return STATE_WAIT; /* please wait */
	 }
	 else
	 {
	    DEBUG_MARK_SPOT("find_end_of_cont : forced end",s1,c1);
	    *e1=*e2=s1;
	    *ce1=*ce2=c1;
	    free_svalue(endtagname);
	    return STATE_DONE; /* end of tag, sure... */
	 }
      }
      DEBUG_MARK_SPOT("find_end_of_container found tag",s1,c1);

      /* scan start of tag name */
      if (!scan_forward(s1,c1+1,&s2,&c2,this->ws,-this->n_ws)) {
	free_svalue(endtagname);
	return STATE_WAIT; /* come again */
      }

      if (m_sizeof(this->mapqtag)) {
	/* must check nested quote tags, since they affect the syntax itself */
	struct svalue *v;
	if (!quote_tag_lookup(this,s2,c2,&s3,&c3,finished,&v)) {
	  free_svalue(endtagname);
	  return STATE_WAIT; /* come again */
	}
	if (v) {
	  DEBUG_MARK_SPOT("find_end_of_cont : quote tag",s2,c2);
	  if (!scan_for_string(this,s3,c3,&s3,&c3,v[2].u.string)) {
	    free_svalue(endtagname);
	    return finished ? STATE_DONE : STATE_WAIT;
	  }
	  n_pos_forward(s3,c3+v[2].u.string->len,&feed,&c);
	  DEBUG_MARK_SPOT("find_end_of_cont : quote tag end",feed,c);
	  continue;
	}
      }

      /* scan tag name as argument and push */
      if (!scan_forward_arg(this,s2,c2,&s3,&c3,1,finished))
      {
	free_svalue(endtagname);
	return STATE_WAIT; /* come again */
      }
      DEBUG_MARK_SPOT("find_end_of_cont : got tag",s2,c2);

      if (this->flags & FLAG_CASE_INSENSITIVE_TAG)
	f_lower_case(1);
      if (is_eq(sp-1,tagname)) found = 't';
      else if (is_eq(sp-1,endtagname)) found = 'e';
      else found = 0;

      /* When we should treat unknown tags as text, we have to parse
       * and lookup just the tag name before continuing. */
      if (!found && (THIS->flags & FLAG_IGNORE_UNKNOWN) &&
	  (!m_sizeof(this->maptag) || !low_mapping_lookup(this->maptag,sp-1)) &&
	  (!m_sizeof(this->mapcont) || !low_mapping_lookup(this->mapcont,sp-1))) {
	DEBUG((stderr,"find_end_of_cont : treating unknown tag as text\n"));
	feed = s3;
	c = c3;
	pop_stack();
	continue;
      }

      if (c3 < s3->s->len && index_shared_string (s3->s,c3) == this->tag_start) {
	struct pike_string *cmp =
	  string_slice (endtagname->u.string, 0, sp[-1].u.string->len);
	if (cmp == sp[-1].u.string) {
	  DEBUG_MARK_SPOT("find_end_of_cont : skipping open endtag prefix",s2,c2);
	  free_string (cmp);
	  feed = s3;
	  c = c3;
	  pop_stack();
	  continue;
	}
	free_string (cmp);
      }

      pop_stack();

      if (!scan_for_end_of_tag(this,s3,c3,&s2,&c2,finished,
			       found == 'e' ? -1 : this->flags & FLAG_MATCH_TAG))
      {
	 free_svalue(endtagname);
	 return STATE_WAIT;
      }

      switch (found) {
	case 't': /* push a new level */
	 DEBUG_MARK_SPOT("find_end_of_cont : push",s2,c2+1);
	 res=find_end_of_container(this,tagname,endtagname,
				   s2,++c2,e1,ce1,&feed,&c,finished);
	 DEBUG_MARK_SPOT("find_end_of_cont : push end",feed,c);
	 if (res)
	 {
	    DEBUG((stderr,"find_end_of_cont (pushed) return %d %p:%d\n",
		   res,s1,c1));
	    free_svalue(endtagname);
	    return res;
	 }
	 break;

	case 'e':
	 if (index_shared_string (s2->s, c2) == this->tag_start) {
	   DEBUG_MARK_SPOT("find_end_of_cont : skipping open endtag",s2,c2);
	   feed = s2;
	   c = c2;
	   continue;
	 }

	 free_svalue(endtagname);
	 
	 DEBUG_MARK_SPOT("find_end_of_cont got cont end   --> ",s1,c1);
	 DEBUG_MARK_SPOT("find_end_of_cont got endtag end --> ",s2,c2);

	 *e1=s1;
	 *ce1=c1;
	 *e2=s2;
	 *ce2=c2+1;

	 return STATE_DONE;

	default:
	 feed=s2;
	 c=c2;
      }
   }
}

/* ---------------------------------------------------------------- */

static newstate do_try_feed(struct parser_html_storage *this,
			    struct object *thisobj,
			    struct feed_stack *st,
			    struct piece **feed,
			    int finished,
			    int ignore_tag_cb)
{
   p_wchar2 look_for[2 /* entity or tag */];
   p_wchar2 ch;
   int n;
   struct piece *dst;
   int cdst;
   newstate res;
   int got_data=0;

   int scan_entity,scan_tag;

   recheck_scan(this,&scan_entity,&scan_tag);

   for (;;)
   {
      DEBUG((stderr,"%*d do_try_feed scan loop "
	     "scan_entity=%d scan_tag=%d ignore_data=%d\n",
	     this->stack_count,this->stack_count,
	     scan_entity,scan_tag,st->ignore_data));

#ifdef DEBUG
      if (*feed && feed[0]->s->len < st->c) 
	 fatal("len (%d) < st->c (%d)\n",feed[0]->s->len,st->c);
#endif

      /* do we need to check data? */
      if (!st->ignore_data)
      {
	 DEBUG((stderr,"%*d do_try_feed scan for data %p:%d (len=%d)\n",
		this->stack_count,this->stack_count,
		*feed,st->c,feed[0]?feed[0]->s->len:0));

	 /* we are to get data first */
	 /* look for tag or entity */
	 if (*feed)
	 {
	    n=0;
	    if (scan_entity) look_for[n++]=this->entity_start;
	    if (scan_tag) look_for[n++]=this->tag_start;
	    if (got_data)
	      /* Already got some data scanned by. Continue after that. */
	      scan_forward(dst,cdst,&dst,&cdst,look_for,n);
	    else
	      scan_forward(*feed,st->c,&dst,&cdst,look_for,n);
	 }
	 got_data=0;
	 if (*feed != dst || st->c != cdst) ignore_tag_cb = 0;

	 dmalloc_touch_svalue(&(this->callback__data));

	 if (this->callback__data.type!=T_INT)
	 {
	    struct feed_stack *prev = st->prev;
	    if (prev && !prev->ignore_data &&
		(!*feed || (!dst->next && cdst==dst->s->len)))
	    {
	       DEBUG((stderr,"%*d putting trailing data on parent feed %p:%d..%p:%d\n",
		      this->stack_count,this->stack_count,
		      *feed,st->c,dst,cdst));
	    
	       /* last bit of a nested feed - put it in the parent
		* feed to avoid unnecessary calls to the data cb */
	       if (*feed)
	       {
		  struct piece **f=prev->prev ? &prev->local_feed : &this->feed;
		  if (prev->c && *f)
		  {
		     struct pike_string *s=(*f)->s;
		     (*f)->s=string_slice((*f)->s,prev->c,(*f)->s->len-prev->c);
		     free_string(s);
		  }
		  dst->next=*f;
		  *f=*feed;
		  prev->c=st->c;
		  *feed=NULL;
	       }
	    }
	    else if (*feed != dst || st->c != cdst)
	    {
	       DEBUG((stderr,"%*d calling _data callback %p:%d..%p:%d\n",
		      this->stack_count,this->stack_count,
		      *feed,st->c,dst,cdst));

	       do_callback(this,thisobj,
			   &(this->callback__data),
			   *feed,st->c,dst,cdst,
			   TYPE_DATA);

	       if ((res=handle_result(this,st,
				      feed,&(st->c),dst,cdst)))
	       {
		  DEBUG((stderr,"%*d do_try_feed return %d %p:%d\n",
			 this->stack_count,this->stack_count,
			 res,*feed,st->c));
		  st->ignore_data=(res==STATE_WAIT);
		  return res;
	       }
	    }
	    recheck_scan(this,&scan_entity,&scan_tag);
	 }
	 else
	 {
	    if (*feed)
	    {
	       put_out_feed_range(this,*feed,st->c,dst,cdst);
	       skip_feed_range(st,feed,&(st->c),dst,cdst);
	    }
	 }

	 DEBUG((stderr,"%*d do_try_feed scan data done %p:%d\n",
		this->stack_count,this->stack_count,
		*feed,st->c));
      }
      /* at end, entity or tag */

      if (!*feed)
      {
	 DEBUG((stderr,"%*d do_try_feed end\n",
		this->stack_count,this->stack_count));
	 return STATE_DONE; /* done */
      }

      ch=index_shared_string(feed[0]->s,st->c);
      if (scan_tag && ch==this->tag_start) /* tag */
      {
	 struct svalue *v;

	 DEBUG((stderr,"%*d do_try_feed scan tag %p:%d\n",
		this->stack_count,this->stack_count,
		*feed,st->c));

	 /* scan start of tag name */
	 if (!scan_forward(*feed,st->c+1,&dst,&cdst,
			   this->ws,-this->n_ws))
	 {
	   st->ignore_data=1;
	   return STATE_WAIT; /* come again */
	 }

	 if (m_sizeof(this->mapqtag)) {
	   struct piece *e1,*e2; /* <![CDATA[ ... ]]>  */
	   int ce1,ce2;          /*       e1 ^    ^ e2 */

	   if (!quote_tag_lookup(this,dst,cdst,&e1,&ce1,finished,&v)) {
	     st->ignore_data=1;
	     return STATE_WAIT; /* come again */
	   }

	   if (v) {		/* quote tag */
	     if (scan_for_string(this,e1,ce1,&e2,&ce2,v[2].u.string))
	       n_pos_forward(e2,ce2+v[2].u.string->len,&dst,&cdst);
	     else {
	       if (!finished) {
		 st->ignore_data=1;
		 return STATE_WAIT; /* come again */
	       }
	       dst = e2;
	       cdst = ce2;
	     }

	     if ((res=quote_tag_callback(this,thisobj,v+1,
					 e1,ce1,e2,ce2,
					 st,feed,&(st->c),dst,cdst)))
	     {
	       DEBUG((stderr,"%*d quote tag callback return %d %p:%d\n",
		      this->stack_count,this->stack_count,
		      res,*feed,st->c));
	       st->ignore_data=(res==STATE_WAIT);
	       return res;
	     }

	     DEBUG((stderr,"%*d quote tag callback done %p:%d\n",
		    this->stack_count,this->stack_count,
		    *feed,st->c));

	     goto done;
	   }
	 }

	 /* scan tag name as argument and push */
	 if (!scan_forward_arg(this,dst,cdst,&dst,&cdst,1,finished))
	 {
	   st->ignore_data=1;
	   return STATE_WAIT; /* come again */
	 }
	 
	 if (m_sizeof(this->maptag) ||
	     m_sizeof(this->mapcont))
	 {
	    if (this->flags & FLAG_CASE_INSENSITIVE_TAG)
	      f_lower_case(1);
	    v=low_mapping_lookup(this->maptag,sp-1);
	    if (v) /* tag */
	    {
	       pop_stack();

	       if (!scan_for_end_of_tag(this,dst,cdst,&dst,&cdst,
					finished,this->flags & FLAG_MATCH_TAG))
	       {
		 st->ignore_data=1;
		 return STATE_WAIT; /* come again */
	       }

	       /* low-level tag call */
	       if ((res=tag_callback(this,thisobj,v,
				     st,feed,&(st->c),dst,cdst+1)))
	       {
		  DEBUG((stderr,"%*d tag callback return %d %p:%d\n",
			 this->stack_count,this->stack_count,
			 res,*feed,st->c));
		  st->ignore_data=(res==STATE_WAIT);
		  return res;
	       }

	       DEBUG((stderr,"%*d tag callback done %p:%d\n",
		      this->stack_count,this->stack_count,
		      *feed,st->c));

	       goto done;
	    }
	    v=low_mapping_lookup(this->mapcont,sp-1);
	    if (v) /* container */
	    {
	       struct piece *e1,*e2; /* <tag> ... </tag>     */
	       int ce1,ce2;          /*        e1 ^     ^ e2 */

	       if (!scan_for_end_of_tag(this,dst,cdst,&dst,&cdst,
					finished,this->flags & FLAG_MATCH_TAG))
	       {
		 st->ignore_data=1;
		 pop_stack();
		 return STATE_WAIT; /* come again */
	       }

	       /* this is the hardest part : find the corresponding end tag */

	       if ((res=find_end_of_container(this,
					      sp-1,
					      NULL,
					      dst,cdst+1,
					      &e1,&ce1,&e2,&ce2,
					      finished)))
	       {
		  DEBUG((stderr,"%*d find end of cont return %d %p:%d\n",
			 this->stack_count,this->stack_count,
			 res,*feed,st->c));
		  st->ignore_data=(res==STATE_WAIT);
		  pop_stack();
		  return res;
	       }
	       pop_stack();

	       /* low-level container call */
	       if ((res=container_callback(this,thisobj,v,
					   dst,cdst+1,e1,ce1,
					   st,feed,&(st->c),e2,ce2)))
	       {
		  DEBUG((stderr,"%*d container callback return %d %p:%d\n",
			 this->stack_count,this->stack_count,
			 res,*feed,st->c));
		  st->ignore_data=(res==STATE_WAIT);
		  return res;
	       }

	       goto done;
	    }
	 }
	 pop_stack();

	 dmalloc_touch_svalue(&(this->callback__tag));

	 if (this->callback__tag.type!=T_INT && !ignore_tag_cb)
	 {
	    if (!scan_for_end_of_tag(this,dst,cdst,&dst,&cdst,
				     finished,this->flags & FLAG_MATCH_TAG))
	    {
	       st->ignore_data=1;
	       return STATE_WAIT; /* come again */
	    }

	    DEBUG((stderr,"%*d calling _tag callback %p:%d..%p:%d\n",
		   this->stack_count,this->stack_count,
		   *feed,st->c+1,dst,cdst));

	    /* low-level tag call */
	    do_callback(this,thisobj,
			&(this->callback__tag),
			*feed,st->c,dst,cdst+1,
			TYPE_TAG);

	    if ((res=handle_result(this,st,
				   feed,&(st->c),dst,cdst+1)))
	    {
	       DEBUG((stderr,"%*d do_try_feed return %d %p:%d\n",
		      this->stack_count,this->stack_count,
		      res,*feed,st->c));
	       st->ignore_data=(res==STATE_WAIT);
	       return res;
	    }
	    recheck_scan(this,&scan_entity,&scan_tag);
	 }
	 else if (this->flags & FLAG_IGNORE_UNKNOWN) {
	   /* Send it to callback__data. */
	   dst=*feed;
	   cdst=st->c+1;
	   got_data=1;
	   goto done;
	 }
	 else
	 {
	    if (!scan_for_end_of_tag(this,dst,cdst,&dst,&cdst,
				     finished,this->flags & FLAG_MATCH_TAG))
	      return STATE_WAIT; /* come again */

	    put_out_feed_range(this,*feed,st->c,dst,cdst+1);
	    skip_feed_range(st,feed,&(st->c),dst,cdst+1);
	 }
      }
      else if (scan_entity && ch==this->entity_start) /* entity */
      {
	 int end_found;
	 DEBUG((stderr,"%*d do_try_feed scan entity %p:%d\n",
		this->stack_count,this->stack_count,
		*feed,st->c));
	 /* just search for end of entity */

	 if (this->flags & FLAG_LAZY_ENTITY_END) {
	   end_found=scan_forward(*feed,st->c+1,&dst,&cdst,
				  this->lazy_entity_ends,this->n_lazy_entity_ends);
	   if (end_found && index_shared_string(dst->s,cdst) != this->entity_end) {
	     /* Got no entity end; send it to callback__data. */
	     dst=*feed;
	     cdst=st->c+1;
	     got_data=1;
	     goto done;
	   }
	 }
	 else {
	   look_for[0]=this->entity_end;
	   end_found=scan_forward(*feed,st->c+1,&dst,&cdst,look_for,1);
	 }
	 if (!end_found && !finished)
	 {
	    st->ignore_data=1; /* no data first at next call */
	    return STATE_WAIT; /* no end, call again */
	 }

	 if (m_sizeof(this->mapentity))
	 {
	    struct svalue *v;
	    
	    push_feed_range(*feed,st->c+1,dst,cdst);
	    cdst+=1;

	    v=low_mapping_lookup(this->mapentity,sp-1);
	    if (v) /* entity we want, do a callback */
	    {
	       pop_stack();

	       /* low-level entity call */
	       if ((res=entity_callback(this,thisobj,v,
					st,feed,&(st->c),dst,cdst)))
	       {
		  DEBUG((stderr,"%*d entity callback return %d %p:%d\n",
			 this->stack_count,this->stack_count,
			 res,*feed,st->c));
		  st->ignore_data=(res==STATE_WAIT);
		  return res;
	       }

	       DEBUG((stderr,"%*d entity callback done %p:%d\n",
		      this->stack_count,this->stack_count,
		      *feed,st->c));

	       goto done;
	    }
	    pop_stack();
	 }
	 else cdst+=1;

	 dmalloc_touch_svalue(&(this->callback__entity));

	 if (this->callback__entity.type!=T_INT && !ignore_tag_cb)
	 {
	    DEBUG((stderr,"%*d calling _entity callback %p:%d..%p:%d\n",
		   this->stack_count,this->stack_count,
		   *feed,st->c,dst,cdst));

	    /* low-level entity call */
	    do_callback(this,thisobj,
			&(this->callback__entity),
			*feed,st->c,dst,cdst,
			TYPE_ENTITY);

	    if ((res=handle_result(this,st,
				   feed,&(st->c),dst,cdst)))
	    {
	       DEBUG((stderr,"%*d do_try_feed return %d %p:%d\n",
		      this->stack_count,this->stack_count,
		      res,*feed,st->c));
	       st->ignore_data=(res==1);
	       return res;
	    }
	    recheck_scan(this,&scan_entity,&scan_tag);
	 }
	 else if (this->flags & FLAG_IGNORE_UNKNOWN) {
	   /* Send it to callback__data. */
	   dst=*feed;
	   cdst=st->c+1;
	   got_data=1;
	   goto done;
	 }
	 else
	 {
	    put_out_feed_range(this,*feed,st->c,dst,cdst);
	    skip_feed_range(st,feed,&(st->c),dst,cdst);
	 }

	 DEBUG((stderr,"%*d do_try_feed scan entity %p:%d\n",
		this->stack_count,this->stack_count,
		*feed,st->c));
      }
done:

      st->ignore_data=0;
      ignore_tag_cb=0;
   }
}

/****** try_feed - internal main ********************/

static void try_feed(int finished)
{
   /* 
      o if tag_stack:
          o pop & parse that
          o ev put on stack
   */

   struct feed_stack *st;
   int ignore_tag_cb = 0;

   if (THIS->start) return;	/* called from a callback - avoid recursion */

   for (;;)
   {
      newstate res;
      st=THIS->stack;
      res = do_try_feed(THIS,THISOBJ,
			st,
			st->prev
			?&(st->local_feed)
			:&(THIS->feed),
			finished||(st->prev!=NULL),
			ignore_tag_cb);
      ignore_tag_cb = 0;
      switch (res)
      {
	 case STATE_DONE: /* done, pop stack */
	    if (!THIS->feed) THIS->feed_end=NULL;

	    st=THIS->stack->prev;
	    if (!st) 
	    {
	       THIS->stack->c=0;
	       return; /* all done, but keep last stack elem */
	    }

	    if (THIS->stack->local_feed)
	       error("internal wierdness in Parser.HTML: feed left\n");

	    free(THIS->stack);
	    THIS->stack=st;
	    THIS->stack_count--;
	    break;

	 case STATE_WAIT: /* incomplete, call again */
	    return;

	 case STATE_REPARSE: /* user requested another go at the current data */
	    if (st == THIS->stack) ignore_tag_cb = 1;
	    /* FALL THROUGH */

	 case STATE_REREAD: /* reread stack head */
	    if (THIS->stack_count>THIS->max_stack_depth)
	       error("Parser.HTML: too deep recursion\n");
	    break;
      }
   }
}

/****** feed ****************************************/


static void low_feed(struct pike_string *ps)
{
   struct piece *f;

   if (!ps->len) return;

   f=malloc(sizeof(struct piece));
   if (!f)
      error("feed: out of memory\n");
   copy_shared_string(f->s,ps);

   f->next=NULL;

   if (THIS->feed_end==NULL)
   {
      DEBUG((stderr,"  (new feed)\n"));
      THIS->feed=THIS->feed_end=f;
   }
   else
   {
      DEBUG((stderr,"  (attached to feed end)\n"));
      THIS->feed_end->next=f;
      THIS->feed_end=f;
   }
}

/*
**! method object feed()
**! method object feed(string s)
**! method object feed(string s,int do_parse)
**!	Feed new data to the <ref>Parser.HTML</ref>
**!	object. This will start a scan and may result in
**!	callbacks. Note that it's possible that all 
**!	data feeded isn't processed - to do that, call
**!	<ref>finish</ref>().
**!
**!	If the function is called without arguments,
**!	no data is feeded, but the parser is run.
**!
**!	If the string argument is followed by a 0,
**!	<tt>->feed(s,1);</tt>, the string is feeded,
**!	but the parser isn't run.
**! returns the called object
**! see also: finish, read, feed_insert
*/

static void html_feed(INT32 args)
{
   DEBUG((stderr,"feed %d chars\n",
	  (args&&sp[-args].type==T_STRING)?
	  sp[-args].u.string->len:-1));

   if (args)
   {
      if (sp[-args].type==T_STRING)
	 low_feed(sp[-args].u.string);
      else if (sp[-args].type!=T_INT || sp[-args].u.integer)
	 SIMPLE_BAD_ARG_ERROR("feed",1,"string");
   }

   if (args<2 || sp[1-args].type!=T_INT || sp[1-args].u.integer)
   {
      pop_n_elems(args);
      try_feed(0);
   }
   else
      pop_n_elems(args);

   ref_push_object(THISOBJ);
}

/*
**! method object feed_insert(string s)
**!	This pushes a string on the parser stack.
**!	(I'll write more about this mechanism later.)
**! note: don't use
*/

static void html_feed_insert(INT32 args)
{
   struct feed_stack *st2;

   if (!args)
      SIMPLE_TOO_FEW_ARGS_ERROR("feed_insert",1);

   if (sp[-args].type!=T_STRING)
      SIMPLE_BAD_ARG_ERROR("feed_insert",1,"string");

   DEBUG((stderr,"html_feed_insert: "
	  "pushing string (len=%d) on feedstack\n",
	  sp[-args].u.string->len));

   st2=malloc(sizeof(struct feed_stack));
   if (!st2)
      error("out of memory\n");

   st2->local_feed=malloc(sizeof(struct piece));
   if (!st2->local_feed)
   {
      free(st2);
      error("out of memory\n");
   }
   copy_shared_string(st2->local_feed->s,sp[-args].u.string);
   st2->local_feed->next=NULL;
   pop_stack();
   st2->ignore_data=0;
   st2->pos=THIS->stack->pos;
   st2->prev=THIS->stack;
   st2->c=0;
   THIS->stack=st2;
   THIS->stack_count++;

   if (args<2 || sp[1-args].type!=T_INT || sp[1-args].u.integer)
   {
      pop_n_elems(args);
      try_feed(0);
   }
   else
      pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*
**! method object finish()
**! method object finish(string s)
**!	Finish a parser pass. 
**!
**!	A string may be sent here, similar to feed().
**!
**! returns the called object
*/

static void html_finish(INT32 args)
{
   if (args)
   {
      if (sp[-args].type==T_STRING)
	 low_feed(sp[-args].u.string);
      else if (sp[-args].type!=T_INT || sp[-args].u.integer)
	 SIMPLE_BAD_ARG_ERROR("finish",1,"string");
   }
   if (THIS->feed || THIS->stack->prev) try_feed(1);
   ref_push_object(THISOBJ);
}

/*
**! method string|array(mixed) read()
**! method string|array(mixed) read(int max_elems)
**!	Read parsed data from the parser object. 
**!
**!	Returns a string of parsed data if the parser isn't in
**!	<ref>mixed_mode</ref>, an array of arbitrary data otherwise.
*/

static void html_read(INT32 args)
{
   int n;
   int m=0; /* items on stack */

   if (!args) 
      n=0x7fffffff; /* a lot */
   else  if (sp[-args].type==T_INT)
      n=sp[-args].u.integer;
   else
      error("read: illegal argument\n");

   pop_n_elems(args);

   if (THIS->flags & FLAG_MIXED_MODE)
   {
      int got_arr = 0;
      /* collect up to n items */
      while (THIS->out && n)
      {
	 struct out_piece *z = THIS->out;
	 push_svalue(&z->v);
	 n--;
	 if (++m == 32)
	 {
	    f_aggregate(32);
	    m = 0;
	    if (got_arr) f_add(2), got_arr = 1;
	 }
	 THIS->out = THIS->out->next;
	 free_svalue(&z->v);
	 free(z);
      }
      if (m)
      {
	 f_aggregate(m);
	 if (got_arr) f_add(2);
      }
   }
   else
   {
      /* collect up to n characters */
      while (THIS->out && n)
      {
	 struct out_piece *z;

	 if (THIS->out->v.type != T_STRING)
	    error("Parser.HTML: Got nonstring in parsed data\n");

	 if (THIS->out->v.u.string->len>n)
	 {
	    struct pike_string *ps;
	    ref_push_string(string_slice(THIS->out->v.u.string,0,n));
	    m++;
	    ps=string_slice(THIS->out->v.u.string,n,THIS->out->v.u.string->len-n);
	    free_string(THIS->out->v.u.string);
	    THIS->out->v.u.string=ps;
	    break;
	 }
	 n-=THIS->out->v.u.string->len;
	 push_svalue(&THIS->out->v);
	 free_svalue(&THIS->out->v);
	 m++;
	 if (m==32)
	 {
	    f_add(32);
	    m=1;
	 }
	 z=THIS->out;
	 THIS->out=THIS->out->next;
	 free(z);
      }

      if (!m)
	 ref_push_string(empty_string);
      else
	 f_add(m);
   }

   if (!THIS->out) 
      THIS->out_end=NULL;
}

/*
**! method object write_out(mixed...)
**!	Send data to the output stream (i.e. it won't be parsed).
**!
**!	Any data is allowed when the parser is running in
**!	<ref>mixed_mode</ref>. Only strings are allowed otherwise.
**!
**! returns the called object
*/

void html_write_out(INT32 args)
{
   int i;
   for (i = args; i; i--)
   {
      if (!(THIS->flags & FLAG_MIXED_MODE) && sp[-i].type!=T_STRING)
	 error("write_out: not a string argument\n");
      put_out_feed(THIS,sp-i);
   }
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/** query *******************************************/

/* 
**! method array(int) at();
**! method int at_line();
**! method int at_char();
**! method int at_column();
**!	Get the current position.
**!	<ref>at</ref>() gives an array consisting of
**!	({<i>line</i>,<i>char</i>,<i>column</i>}), in that order.
*/

static void html_at_line(INT32 args)
{
   struct feed_stack *st = THIS->stack;
   pop_n_elems(args);
   while (st->prev) st = st->prev;
   push_int(st->pos.lineno);
}

static void html_at_char(INT32 args)
{
   struct feed_stack *st = THIS->stack;
   pop_n_elems(args);
   while (st->prev) st = st->prev;
   push_int(st->pos.byteno);
}

static void html_at_column(INT32 args)
{
   struct feed_stack *st = THIS->stack;
   pop_n_elems(args);
   while (st->prev) st = st->prev;
   push_int(st->pos.byteno-st->pos.linestart);
}

static void html_at(INT32 args)
{
   pop_n_elems(args);
   push_int(THIS->stack->pos.lineno);
   push_int(THIS->stack->pos.byteno);
   push_int(THIS->stack->pos.byteno-THIS->stack->pos.linestart);
   f_aggregate(3);
}

/*
**! method string current()
**!	Gives the current range of data, ie the whole tag/entity/etc
**!	being parsed in the current callback. Returns zero if there's
**!	no current range, i.e. when the function is not called in a
**!	callback.
*/

static void html_current(INT32 args)
{
   pop_n_elems(args);
   if (THIS->start)
   {
      push_feed_range(THIS->start,THIS->cstart,THIS->end,THIS->cend);
   }
   else
      push_int(0);
}

/*
**! method array tag()
**! method string tag_name()
**! method string tag_args()
**! method string tag_content()
**! method array tag(mixed default_value)
**! method string tag_args(mixed default_value)
**!     These give parsed information about the current thing being
**!     parsed, e.g. the current tag, container or entity. They return
**!     zero if they're not applicable.
**!
**!	<tt>tag_name</tt> gives the name of the current tag. If used
**!	from an entity callback, it gives the string inside the
**!	entity.
**!
**!	<tt>tag_args</tt> gives the arguments of the current tag,
**!	parsed to a convenient mapping consisting of key:value pairs.
**!	If the current thing isn't a tag, it gives zero. default_value
**!	is used for arguments which have no value in the tag. If
**!	default_value isn't given, the value is set to the same string
**!	as the key.
**!
**!	<tt>tag_content</tt> gives the content of the current tag, if
**!	it's a container or quote tag.
**!
**!	<tt>tag()</tt> gives the equivalent of
**!	<tt>({tag_name(),tag_args(), tag_content()})</tt>.
*/

static void tag_name(struct parser_html_storage *this,struct piece *feed,int c)
{
   struct piece *s1=NULL,*s2=NULL;
   int c1=0,c2=0;

   p_wchar2 ch=index_shared_string(feed->s,c);
   if (ch==this->tag_start) c++;

   /* scan start of tag name */
   scan_forward(feed,c,&s1,&c1,
		this->ws,-this->n_ws);

   /* scan as argument and push*/
   scan_forward_arg(this,s1,c1,&s2,&c2,1,1);
}

static INLINE void tag_push_default_arg(struct svalue *def)
{
   if (def) push_svalue(def);
   else stack_dup();
}

static void tag_args(struct parser_html_storage *this,struct piece *feed,int c,
		     struct svalue *def)
{
   struct piece *s1=NULL,*s2=NULL,*s3;
   int c1=0,c2=0,c3;
   int n=0;

   p_wchar2 ch=index_shared_string(feed->s,c);
   if (ch==this->tag_start) c++;

   /* scan past tag name */
   scan_forward(feed,c,&s1,&c1,
		this->ws,-this->n_ws);
   scan_forward_arg(this,s1,c1,&s2,&c2,0,1);

   for (;;)
   {
      /* skip whitespace */
      scan_forward(s2,c2,&s1,&c1,this->ws,-this->n_ws);

      /* end of tag? */
      if (c1==s1->s->len) /* end<tm> */
	 break;
      ch=index_shared_string(s1->s,c1);
      if (ch==this->tag_end) /* end */
	 break;

new_arg:

      DEBUG_MARK_SPOT("html_tag_args arg start",s1,c1);

      /* scan this argument name and push*/
      scan_forward_arg(this,s1,c1,&s2,&c2,1,1);
      if (this->flags & FLAG_CASE_INSENSITIVE_TAG)
	f_lower_case(1);
      n++;

      /* scan for '=', '>' or next argument */
      /* skip whitespace */
      scan_forward(s2,c2,&s3,&c3,this->ws,-this->n_ws);
      if (c3==s3->s->len) /* end<tm> */
      {
	 tag_push_default_arg(def);
	 break;
      }
      ch=index_shared_string(s3->s,c3);
      if (ch==this->tag_end) /* end */
      {
	 tag_push_default_arg(def);
	 break;
      }
      if (ch!=this->arg_eq)
	if (c1!=c3 || s1!=s3)	/* end of _this_ argument */
	{
	  s1 = s3, c1 = c3;
	  tag_push_default_arg(def);
	  goto new_arg;
	}
	else {			/* a stray bogus character */
	  s1 = s3, c1 = c3 + 1;
	  pop_stack();
	  n--;
	  goto new_arg;
	}
      
      /* left: case of '=' */
      c3++; /* skip it */
      
      /* skip whitespace */
      scan_forward(s3,c3,&s2,&c2,this->ws,-this->n_ws);

      DEBUG_MARK_SPOT("html_tag_args value start",s2,c2);

      /* scan the argument value */
      scan_forward_arg(this,s2,c2,&s1,&c1,1,1);

      /* next argument in the loop */
      s2 = s1;
      c2 = c1;
   }

   f_aggregate_mapping(n*2);
}

static void html_tag_name(INT32 args)
{
   /* get rid of arguments */
   pop_n_elems(args);

   if (!THIS->start) error ("Parser.HTML: There's no current range.\n");
   switch (THIS->type) {
     case TYPE_TAG:
     case TYPE_CONT:
       tag_name(THIS,THIS->start,THIS->cstart);
       break;
     case TYPE_ENTITY:
       if (THIS->cend == 0) {
	 push_feed_range(THIS->start,THIS->cstart+1,THIS->end,THIS->cend);
	 if (sp[-1].u.string->len &&
	     index_shared_string(sp[-1].u.string,sp[-1].u.string->len-1) ==
	     THIS->entity_end) {
	   struct pike_string *s=string_slice(sp[-1].u.string,0,sp[-1].u.string->len-1);
	   pop_stack();
	   push_string(s);
	 }
       }
       else {
	 int end=THIS->cend;
	 if (index_shared_string(THIS->end->s,end-1) == THIS->entity_end) end--;
	 push_feed_range(THIS->start,THIS->cstart+1,THIS->end,end);
       }
       break;
     case TYPE_QTAG: {
       struct svalue *v;
       struct piece *beg;
       int cbeg;
       scan_forward (THIS->start, THIS->cstart+1, &beg, &cbeg, THIS->ws, -THIS->n_ws);
       quote_tag_lookup (THIS, beg, cbeg, &beg, &cbeg, 1, &v);
       if (!v) push_int (0);
       else push_svalue (v);
       break;
     }
     default:
       push_int(0);
   }
}

static void html_tag_args(INT32 args)
{
   struct svalue def;
   check_all_args("tag_args",args,BIT_MIXED|BIT_VOID,0);
   if (args) assign_svalue_no_free(&def,sp-args);
   pop_n_elems(args);

   if (!THIS->start) error ("Parser.HTML: There's no current range.\n");

   switch (THIS->type) {
     case TYPE_TAG:
     case TYPE_CONT:
       if (args)
       {
	 tag_args(THIS,THIS->start,THIS->cstart,&def);
	 free_svalue(&def);
       }
       else tag_args(THIS,THIS->start,THIS->cstart,NULL);
       break;
     default:
       push_int(0);
   }
}

static void html_tag_content(INT32 args)
{
  struct piece *beg;
  int cbeg;

  pop_n_elems(args);

  if (!THIS->start) error ("Parser.HTML: There's no current range.\n");

  if (!scan_forward (THIS->start, THIS->cstart+1, &beg, &cbeg, THIS->ws, -THIS->n_ws)) {
    push_int(0);
    return;
  }

  switch (THIS->type) {
    case TYPE_CONT: {
      struct piece *end, *dummy;
      int cend, cdummy;
      if (scan_forward_arg (THIS, beg, cbeg, &beg, &cbeg, 1, 1)) {
	if (scan_for_end_of_tag (THIS, beg, cbeg, &beg, &cbeg, 1,
				 THIS->flags & FLAG_MATCH_TAG) &&
	    !find_end_of_container (THIS, sp-1, NULL, beg, cbeg+1,
				    &end, &cend, &dummy, &cdummy, 1)) {
	  pop_stack();
	  if (cmp_feed_pos (end, cend, THIS->end, THIS->cend) < 0)
	    push_feed_range (beg, cbeg+1, end, cend);
	  else
	    push_int(0);
	}
	else {
	  pop_stack();
	  push_int(0);
	}
      }
      else push_int(0);
      break;
    }
    case TYPE_QTAG: {
      struct svalue *v;
      struct piece *end;
      int cend;
      if (quote_tag_lookup (THIS, beg, cbeg, &beg, &cbeg, 1, &v) &&
	  scan_for_string (THIS, beg, cbeg, &end, &cend, v[2].u.string))
	push_feed_range (beg, cbeg, end, cend);
      else
	push_int(0);
      break;
    }
    default:
      push_int(0);
  }
}

static void html_tag(INT32 args)
{
   check_all_args("tag",args,BIT_MIXED|BIT_VOID,0);

   html_tag_args(args);
   html_tag_name(0);
   stack_swap();
   html_tag_content(0);
   f_aggregate(3);
}

static void html_parse_tag_args(INT32 args)
{
   struct piece feed;
   check_all_args("parse_tag_args",args,BIT_STRING,0);
   feed.s=sp[-args].u.string;
   feed.next=NULL;
   tag_args(THIS,&feed,0,NULL);
   stack_pop_n_elems_keep_top(args);
}

static void html_parse_tag_name(INT32 args)
{
   struct piece feed;
   check_all_args("parse_tag_name",args,BIT_STRING,0);
   feed.s=sp[-args].u.string;
   feed.next=NULL;
   tag_name(THIS,&feed,0);
   stack_pop_n_elems_keep_top(args);
}

/** debug *******************************************/

/*
**! method mapping _inspect()
**! 	This is a low-level way of debugging a parser.
**!	This gives a mapping of the internal state
**!	of the Parser.HTML object.
**!
**!	The format and contents of this mapping may
**!	change without further notice.
*/

void html__inspect(INT32 args)
{
   int n=0,m,o,p;
   struct piece *f;
   struct out_piece *of;
   struct feed_stack *st;

   pop_n_elems(args);

   push_text("feed");
   m=0;
   
   st=THIS->stack;
   while (st)
   {
      o=0;
      p=0;

      push_text("feed");

      if (st->local_feed)
	 f=st->local_feed;
      else 
	 f=THIS->feed;

      while (f)
      {
	 ref_push_string(f->s);
	 p++;
	 f=f->next;
      }

      f_aggregate(p);
      o++;

      push_text("position");
      push_int(st->c);
      o++;
      
      push_text("byteno");
      push_int(st->pos.byteno);
      o++;

      push_text("lineno");
      push_int(st->pos.lineno);
      o++;

      push_text("linestart");
      push_int(st->pos.linestart);
      o++;

      f_aggregate_mapping(o*2);
      st=st->prev;

      m++;
   }

   f_aggregate(m);
   n++;

   push_text("outfeed");
   p=0;
   of=THIS->out;
   while (of)
   {
      push_svalue(&of->v);
      p++;
      of=of->next;
   }
   f_aggregate(p);
   n++;

   push_text("tags");
   ref_push_mapping(THIS->maptag);
   n++;

   push_text("containers");
   ref_push_mapping(THIS->mapcont);
   n++;

   push_text("entities");
   ref_push_mapping(THIS->mapentity);
   n++;

   push_text("quote_tags");
   ref_push_mapping(THIS->mapqtag);
   n++;

   push_text("callback__tag");
   push_svalue(&(THIS->callback__tag));
   n++;

   push_text("callback__entity");
   push_svalue(&(THIS->callback__entity));
   n++;

   push_text("callback__data");
   push_svalue(&(THIS->callback__data));
   n++;

   push_text("flags");
   push_int(THIS->flags);
   n++;

   f_aggregate_mapping(n*2);
}

/** create, clone ***********************************/

void html_create(INT32 args)
{
   pop_n_elems(args);
}

/*
**! method object clone(mixed ...)
**!	Clones the <ref>Parser.HTML</ref> object.
**!	A new object of the same class is created,
**!	filled with the parse setup from the 
**!	old object.
**!
**!	This is the simpliest way of flushing a 
**!	parse feed/output.
**!
**!	The arguments to clone is sent to the 
**!	new object, simplifying work for custom classes that
**!	inherits <ref>Parser.HTML</ref>.
**! returns the new object.
**!
**! note:
**!	create is called _before_ the setup is copied.
*/

static void html_clone(INT32 args)
{
   struct object *o;
   struct parser_html_storage *p;
   int i;
   p_wchar2 *newstr;

   DEBUG((stderr,"parse_html_clone object %p\n",THISOBJ));

   /* clone the current object, same class (!) */
   push_object(o=clone_object(THISOBJ->prog,args));

   p=(struct parser_html_storage*)get_storage(o,parser_html_program);

   if (p->maptag) free_mapping(p->maptag);
   add_ref(p->maptag=THIS->maptag);
   if (p->mapcont) free_mapping(p->mapcont);
   add_ref(p->mapcont=THIS->mapcont);
   if (p->mapentity) free_mapping(p->mapentity);
   add_ref(p->mapentity=THIS->mapentity);
   if (p->mapqtag) free_mapping(p->mapqtag);
   add_ref(p->mapqtag=THIS->mapqtag);

   dmalloc_touch_svalue(&p->callback__tag);
   dmalloc_touch_svalue(&p->callback__data);
   dmalloc_touch_svalue(&p->callback__entity);
   dmalloc_touch_svalue(&THIS->callback__tag);
   dmalloc_touch_svalue(&THIS->callback__data);
   dmalloc_touch_svalue(&THIS->callback__entity);
   assign_svalue(&p->callback__tag,&THIS->callback__tag);
   assign_svalue(&p->callback__data,&THIS->callback__data);
   assign_svalue(&p->callback__entity,&THIS->callback__entity);

   if (p->extra_args) free_array(p->extra_args);
   if (THIS->extra_args)
      add_ref(p->extra_args=THIS->extra_args);
   else
      p->extra_args=NULL;

   p->flags=THIS->flags;

   p->tag_start=THIS->tag_start;
   p->tag_end=THIS->tag_end;
   p->entity_start=THIS->entity_start;
   p->entity_end=THIS->entity_end;
   p->arg_eq=THIS->arg_eq;

   p->nargq=THIS->nargq;
   for (i=0; i<p->nargq; i++)
   {
      p->argq_start[i]=THIS->argq_start[i];
      p->argq_stop[i]=THIS->argq_stop[i];
   }

   p->n_lazy_entity_ends=THIS->n_lazy_entity_ends;
   newstr=(p_wchar2*)xalloc(sizeof(p_wchar2)*p->n_lazy_entity_ends);
   MEMCPY(newstr,THIS->lazy_entity_ends,sizeof(p_wchar2)*p->n_lazy_entity_ends);
   if (p->lazy_entity_ends) free(p->lazy_entity_ends);
   p->lazy_entity_ends=newstr;

   p->n_ws=THIS->n_ws;
   newstr=(p_wchar2*)xalloc(sizeof(p_wchar2)*p->n_ws);
   MEMCPY(newstr,THIS->ws,sizeof(p_wchar2)*p->n_ws);
   if (p->ws) free(p->ws); 
   p->ws=newstr;

   DEBUG((stderr,"done clone\n"));

   /* all copied, object on stack */
}

/****** setup ***************************************/

/*
**! method object set_extra(mixed ...args)
**!	Sets the extra arguments passed to all tag, container and
**!	entity callbacks.
**!
**! returns the called object
*/

static void html_set_extra(INT32 args)
{
   if (THIS->extra_args) {
     free_array(THIS->extra_args);
     THIS->extra_args=NULL;
   }
   if (args) {
     f_aggregate(args);
     THIS->extra_args=sp[-1].u.array;
     sp--;
   }
   dmalloc_touch_svalue(sp);
   ref_push_object(THISOBJ);
}

/*
**! method array get_extra()
**!	Gets the extra arguments set by <ref>set_extra</ref>().
**!
**! returns the called object
*/

static void html_get_extra(INT32 args)
{
  pop_n_elems(args);
  if (THIS->extra_args)
    ref_push_array (THIS->extra_args);
  else
    ref_push_array (&empty_array);
}

/*
**! method int case_insensitive_tag(void|int value)
**! method int lazy_entity_end(void|int value)
**! method int match_tag(void|int value)
**! method int mixed_mode(void|int value)
**! method int ignore_unknown(void|int value)
**!	Functions to query or set flags. These set the associated flag
**!	to the value if any is given and returns the old value.
**!
**!	The flags are:
**!	<ul>
**!
**!	<li><b>case_insensitive_tag</b>: All tags and containers are
**!	matched case insensitively, and argument names are converted
**!	to lowercase. Tags added with <ref>add_quote_tag</ref>() are
**!	not affected, though. Switching to case insensitive mode and
**!	back won't preserve the case of registered tags and
**!	containers.
**!
**!	<li><b>lazy_entity_end</b>: Normally, the parser search
**!	indefinitely for the entity end character (i.e. ';'). When
**!	this flag is set, the characters '&', '<', '>', '"', ''', and
**!	any whitespace breaks the search for the entity end, and the
**!	entity text is then treated as data.
**!
**!	<li><b>match_tag</b>: Unquoted nested tag starters and enders
**!	will be balanced when parsing tags. This is the default.
**!
**!	<li><b>mixed_mode</b>: Allow callbacks to return arbitrary
**!	data in the arrays, which will be concatenated in the output.
**!
**!	<li><b>ignore_unknown</b>: Treat unknown tags and entities as
**!	text data, continuing parsing for tags and entities inside
**!	them.
**!
**!	Note: When functions are specified with
**!	<ref>_set_tag_callback</ref>() or
**!	<ref>_set_entity_callback</ref>(), all tags or entities,
**!	respectively, are considered known. However, if one of those
**!	functions return 1 and ignore_unknown is set, they are treated
**!	as text data instead of making another call to the same
**!	function again.
**!
**!	</ul>
*/

static void html_case_insensitive_tag(INT32 args)
{
   int o=THIS->flags & FLAG_CASE_INSENSITIVE_TAG;
   check_all_args("case_insensitive_tag",args,BIT_VOID|BIT_INT,0);
   if (args)
     if (sp[-args].u.integer) THIS->flags |= FLAG_CASE_INSENSITIVE_TAG;
     else THIS->flags &= ~FLAG_CASE_INSENSITIVE_TAG;
   pop_n_elems(args);

   if (args && (THIS->flags & FLAG_CASE_INSENSITIVE_TAG) && !o) {
     INT32 e;
     struct keypair *k;

     MAPPING_LOOP(THIS->maptag) {
       push_svalue(&k->ind);
       f_lower_case(1);
       push_svalue(&k->val);
     }
     f_aggregate_mapping(m_sizeof(THIS->maptag) * 2);
     free_mapping(THIS->maptag);
     THIS->maptag=(--sp)->u.mapping;

     MAPPING_LOOP(THIS->mapcont) {
       push_svalue(&k->ind);
       f_lower_case(1);
       push_svalue(&k->val);
     }
     f_aggregate_mapping(m_sizeof(THIS->mapcont) * 2);
     free_mapping(THIS->mapcont);
     THIS->mapcont=(--sp)->u.mapping;
   }

   push_int(o);
}

static void html_lazy_entity_end(INT32 args)
{
   int o=THIS->flags & FLAG_LAZY_ENTITY_END;
   check_all_args("lazy_entity_end",args,BIT_VOID|BIT_INT,0);
   if (args)
     if (sp[-args].u.integer) THIS->flags |= FLAG_LAZY_ENTITY_END;
     else THIS->flags &= ~FLAG_LAZY_ENTITY_END;
   pop_n_elems(args);
   push_int(o);
}

static void html_match_tag(INT32 args)
{
   int o=THIS->flags & FLAG_MATCH_TAG;
   check_all_args("match_tag",args,BIT_VOID|BIT_INT,0);
   if (args)
     if (sp[-args].u.integer) THIS->flags |= FLAG_MATCH_TAG;
     else THIS->flags &= ~FLAG_MATCH_TAG;
   pop_n_elems(args);
   push_int(o);
}

static void html_mixed_mode(INT32 args)
{
   int o=THIS->flags & FLAG_MIXED_MODE;
   check_all_args("mixed_mode",args,BIT_VOID|BIT_INT,0);
   if (args)
     if (sp[-args].u.integer) THIS->flags |= FLAG_MIXED_MODE;
     else THIS->flags &= ~FLAG_MIXED_MODE;
   pop_n_elems(args);
   push_int(o);
}

static void html_ignore_unknown(INT32 args)
{
   int o=THIS->flags & FLAG_IGNORE_UNKNOWN;
   check_all_args("ignore_unknown",args,BIT_VOID|BIT_INT,0);
   if (args)
     if (sp[-args].u.integer) THIS->flags |= FLAG_IGNORE_UNKNOWN;
     else THIS->flags &= ~FLAG_IGNORE_UNKNOWN;
   pop_n_elems(args);
   push_int(o);
}

#ifdef DEBUG
static void html_debug_mode(INT32 args)
{
   int o=THIS->flags & FLAG_DEBUG_MODE;
   check_all_args("debug_mode",args,BIT_VOID|BIT_INT,0);
   if (args)
     if (sp[-args].u.integer) THIS->flags |= FLAG_DEBUG_MODE;
     else THIS->flags &= ~FLAG_DEBUG_MODE;
   pop_n_elems(args);
   push_int(o);
}
#endif

/****** module init *********************************/

#define tCbret tOr4(tZero,tInt1,tStr,tArr(tMixed))
#define tCbfunc(X) tOr(tFunc(tNone,tCbret),tFunc(tObjImpl_PARSER_HTML X,tCbret))
#define tTodo(X) tOr4(tZero,tStr,tCbfunc(X),tArray)
#define tTagargs tMap(tStr,tStr)

void init_parser_html(void)
{
   empty_string = make_shared_binary_string("", 0);

   ADD_STORAGE(struct parser_html_storage);

   set_init_callback(init_html_struct);
   set_exit_callback(exit_html_struct);
   set_gc_check_callback(gc_check_html);
   set_gc_mark_callback(gc_mark_html);

   ADD_FUNCTION("create",html_create,tFunc(tNone,tVoid),ID_STATIC);
   ADD_FUNCTION("clone",html_clone,tFuncV(tNone,tMixed,tObjImpl_PARSER_HTML),0);

   /* feed control */

   ADD_FUNCTION("feed",html_feed,
		tOr(tFunc(tNone,tObjImpl_PARSER_HTML),
		    tFunc(tStr tOr(tVoid,tInt),tObjImpl_PARSER_HTML)),0);
   ADD_FUNCTION("finish",html_finish,
		tFunc(tOr(tVoid,tStr),tObjImpl_PARSER_HTML),0);
   ADD_FUNCTION("read",html_read,
		tFunc(tOr(tVoid,tInt),tOr(tStr,tArr(tMixed))),0);

   ADD_FUNCTION("write_out",html_write_out,
		tFuncV(tNone,tOr(tStr,tMixed),tObjImpl_PARSER_HTML),0);
   ADD_FUNCTION("feed_insert",html_feed_insert,
		tFunc(tStr,tObjImpl_PARSER_HTML),0);

   /* query */

   ADD_FUNCTION("current",html_current,tFunc(tNone,tStr),0);

   ADD_FUNCTION("at",html_at,tFunc(tNone,tArr(tInt)),0);
   ADD_FUNCTION("at_line",html_at_line,tFunc(tNone,tInt),0);
   ADD_FUNCTION("at_char",html_at_char,tFunc(tNone,tInt),0);
   ADD_FUNCTION("at_column",html_at_column,tFunc(tNone,tInt),0);

   ADD_FUNCTION("tag_name",html_tag_name,tFunc(tNone,tStr),0);
   ADD_FUNCTION("tag_args",html_tag_args,
		tFunc(tOr(tVoid,tSetvar(1,tMixed)),
		      tMap(tStr,tOr(tStr,tVar(1)))),0);
   ADD_FUNCTION("tag_content",html_tag_content,tFunc(tNone,tStr),0);
   ADD_FUNCTION("tag",html_tag,
		tFunc(tOr(tVoid,tSetvar(1,tMixed)),
		      tArr(tOr(tStr,tMap(tStr,tOr(tStr,tVar(1)))))),0);

   /* callback setup */

   ADD_FUNCTION("add_tag",html_add_tag,
		tFunc(tStr tTodo(tTagargs),tObjImpl_PARSER_HTML),0);
   ADD_FUNCTION("add_container",html_add_container,
		tFunc(tStr tTodo(tTagargs tStr),tObjImpl_PARSER_HTML),0);
   ADD_FUNCTION("add_entity",html_add_entity,
		tFunc(tStr tTodo(""),tObjImpl_PARSER_HTML),0);
   ADD_FUNCTION("add_quote_tag",html_add_quote_tag,
		tOr(tFunc(tStr tTodo(tStr) tStr,tObjImpl_PARSER_HTML),
		    tFunc(tStr tZero,tObjImpl_PARSER_HTML)),0);

   ADD_FUNCTION("add_tags",html_add_tags,
		tFunc(tMap(tStr,tTodo( tTagargs )),tObjImpl_PARSER_HTML),0);
   ADD_FUNCTION("add_containers",html_add_containers,
		tFunc(tMap(tStr,tTodo( tTagargs tStr )),tObjImpl_PARSER_HTML),0);
   ADD_FUNCTION("add_entities",html_add_entities,
		tFunc(tMap(tStr,tTodo( "" )),tObjImpl_PARSER_HTML),0);

   ADD_FUNCTION("tags",html_tags,
		tFunc(tNone,tMap(tStr,tTodo( tTagargs ))),0);
   ADD_FUNCTION("containers",html_containers,
		tFunc(tNone,tMap(tStr,tTodo( tTagargs tStr ))),0);
   ADD_FUNCTION("entities",html_entities,
		tFunc(tNone,tMap(tStr,tTodo( "" ))),0);
   ADD_FUNCTION("quote_tags",html_quote_tags,
		tFunc(tNone,tMap(tStr,tTodo( tStr ))),0);

   /* setup */

   ADD_FUNCTION("set_extra",html_set_extra,
		tFuncV(tNone,tMix,tObjImpl_PARSER_HTML),0);
   ADD_FUNCTION("get_extra",html_get_extra,
		tFunc(tNone,tArray),0);

   ADD_FUNCTION("case_insensitive_tag",html_case_insensitive_tag,
		tFunc(tOr(tVoid,tInt),tInt),0);
   ADD_FUNCTION("lazy_entity_end",html_lazy_entity_end,
		tFunc(tOr(tVoid,tInt),tInt),0);
   ADD_FUNCTION("match_tag",html_match_tag,
		tFunc(tOr(tVoid,tInt),tInt),0);
   ADD_FUNCTION("mixed_mode",html_mixed_mode,
		tFunc(tOr(tVoid,tInt),tInt),0);
   ADD_FUNCTION("ignore_unknown",html_ignore_unknown,
		tFunc(tOr(tVoid,tInt),tInt),0);
#ifdef DEBUG
   ADD_FUNCTION("debug_mode",html_debug_mode,
		tFunc(tOr(tVoid,tInt),tInt),0);
#endif

   /* special callbacks */

   ADD_FUNCTION("_set_tag_callback",html__set_tag_callback,
		tFunc(tFuncV(tObjImpl_PARSER_HTML tStr,tMix,tCbret),
		      tObjImpl_PARSER_HTML),0);
   ADD_FUNCTION("_set_data_callback",html__set_data_callback,
		tFunc(tFuncV(tObjImpl_PARSER_HTML tStr,tMix,tCbret),
		      tObjImpl_PARSER_HTML),0);
   ADD_FUNCTION("_set_entity_callback",html__set_entity_callback,
		tFunc(tFuncV(tObjImpl_PARSER_HTML tStr,tMix,tCbret),
		      tObjImpl_PARSER_HTML),0);

   /* debug, whatever */
   
   ADD_FUNCTION("_inspect",html__inspect,tFunc(tNone,tMapping),0);

   /* just useful */

   ADD_FUNCTION("parse_tag_name",html_parse_tag_name,
		tFunc(tStr,tStr),0);
   ADD_FUNCTION("parse_tag_args",html_parse_tag_args,
		tFunc(tStr,tStr),0);
}

void exit_parser_html()
{
   free_string(empty_string);
}

/*

class Parse_HTML
{
   void feed(string something); // more data in stream
   void finish(); // stream ends here

   string read(void|int chars); // read out-feed

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
   string current();  // current tag/entity/etc data string 

   array tag();       // tag data: ({string name,mapping arguments})
   string tag_args(); // tag arguments only
   string tag_name(); // tag name only

   array at();        // current position: ({line,char,column})
   int at_line();     // line number (first=1)
   int at_char();     // char (first=1)
   int at_column();   // column (first=1)

   // low-level callbacks
   // calls to_call(this,string data)
   void _set_tag_callback(function to_call);
   void _set_data_callback(function to_call);
   void _set_entity_callback(function to_call);

   // entity quote
   void set_entity_quote(string start,string end); // "&",";"

   int set_allow_open_entity(void|int yes);        // &entfoo<bar> -> ent

   // call to_call(this,string entity,...extra);
   void add_entity(string entity,function to_call);
   void add_glob_entity(string entity,function to_call);

   // just useful
   string parse_tag_name(string tag);
   mapping parse_tag_args(string tag);
}

*/
