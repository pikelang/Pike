/* $Id: html.c,v 1.136 2004/03/22 14:50:08 mast Exp $ */

#include "global.h"
#include "config.h"

#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "threads.h"
#include "array.h"
#include "pike_error.h"
#include "operators.h"
#include "builtin_functions.h"
#include "module_support.h"
#include "mapping.h"
#include "stralloc.h"
#include "program_id.h"
#include "block_alloc.h"
#include <ctype.h>

#include "parser.h"

/* must be included last */
#include "module_magic.h"

extern struct program *parser_html_program;

/* #define SCAN_DEBUG */
/* #define DEBUG */

#ifdef DEBUG
#undef DEBUG
#define DEBUG(X) if (THIS->flags & FLAG_DEBUG_MODE) fprintf X
#define DEBUG_MARK_SPOT debug_mark_spot
#define HTML_DEBUG
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
**!	<p>The simple way to use it is to give it some information
**!	about available tags and containers, and what 
**!	callbacks those is to call. 
**!
**!	<p>The object is easily reused, by calling the <ref>clone</ref>()
**!	function. 
**!
**! see also: add_tag, add_container, clone
*/

struct location
{
   int byteno;     /* current byte, first=0 */
   int lineno;     /* line number, first=1 */
   int linestart;  /* byte current line started at */
};

static struct location init_pos = {0, 1, 0};

struct piece
{
   struct pike_string *s;
   struct piece *next;
};

#undef INIT_BLOCK
#define INIT_BLOCK(p) p->next = NULL;
#undef EXIT_BLOCK
#define EXIT_BLOCK(p) free_string (p->s);

BLOCK_ALLOC (piece, 53);

struct out_piece
{
   struct svalue v;
   struct out_piece *next;
};

#undef INIT_BLOCK
#define INIT_BLOCK(p) p->next = NULL;
#undef EXIT_BLOCK
#define EXIT_BLOCK(p) free_svalue (&p->v);

BLOCK_ALLOC (out_piece, 211);

struct feed_stack
{
   int ignore_data, free_feed, parse_tags;
   
   struct feed_stack *prev;

   /* this is a feed-stack, ie
      these contains the result of callbacks,
      if they are to be parsed.
      
      The bottom stack element has no local feed,
      it's just for convenience */

   /* current position; if not local feed, use global feed */
   struct piece *local_feed;
   ptrdiff_t c;

   struct location pos;
};

#undef BLOCK_ALLOC_NEXT
#define BLOCK_ALLOC_NEXT prev
#undef INIT_BLOCK
#define INIT_BLOCK(p) p->local_feed = NULL;
#undef EXIT_BLOCK
#define EXIT_BLOCK(p)							\
  if (p->free_feed)							\
    while (p->local_feed)						\
    {									\
      struct piece *f=p->local_feed;					\
      p->local_feed=f->next;						\
      really_free_piece (f);						\
    }

BLOCK_ALLOC (feed_stack, 31);

enum types {
  TYPE_TAG,			/* empty tag callback */
  TYPE_CONT,			/* container tag callback */
  TYPE_ENTITY,			/* entity */
  TYPE_QTAG,			/* quote tag callback */
  TYPE_DATA			/* data callback */
};

#define MAX_ARGQ 8

enum contexts {
  CTX_DATA,			/* in normal text data */
  CTX_TAG,			/* in a tag, before an arg name */
  CTX_SPLICE_ARG,		/* in a splice arg */
  CTX_TAG_ARG,			/* before an unquoted tag arg */
  CTX_TAG_QUOTED_ARG/*+ n*/,	/* in a tag arg quoted with quote n */
};

struct subparse_save
{
  struct parser_html_storage *this;
  struct object *thisobj;
  struct feed_stack *st;
  struct piece *feed;
  int prev_free_feed;
  ptrdiff_t c;
  struct location pos;
  struct out_piece *cond_out, *cond_out_end;
  enum contexts out_ctx;
};

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

/* flag: xml tag syntax */
#define FLAG_XML_TAGS			0x00000080

/* flag: strict html or xml syntax, corresponding to flags 0 or 3 to
 * the xml_tag_syntax() function */
#define FLAG_STRICT_TAGS		0x00000100

/* flag: allow whitespace before the tag name */
#define FLAG_WS_BEFORE_TAG_NAME		0x00000200

/* flag: parse tags */
#define FLAG_PARSE_TAGS			0x00000400

/* flag: understand nestled entities (send it all to entity callback) */
/*       disabled by FLAG_LAZY_ENTITY_END */
#define FLAG_NESTLING_ENTITY_END	0x00000800

/* flag: reparse plain strings used as tag/container/entity callbacks */
#define FLAG_REPARSE_STRINGS		0x00001000

struct parser_html_storage
{
/*--- current state -----------------------------------------------*/

   /* feeded info */
   struct piece *feed,*feed_end;

   /* resulting data */
   struct out_piece *out,*out_end;

   /* Data that have been fed out but may be drawn back again. If
    * activated, this contains a placeholder struct first. */
   struct out_piece *cond_out, *cond_out_end;

   /* The current context in the output queue. */
   enum contexts out_ctx;
   
   /* parser stack */
   struct feed_stack *stack;
   int stack_count;
   int max_stack_depth;

   /* current range (ie, current tag/entity/whatever) */
   /* start is also used to flag callback recursion */
   struct piece *start,*end;
   ptrdiff_t cstart,cend;

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

   /* string to use for magic splice argument */
   struct pike_string *splice_arg;

   /* callback functions */
   struct svalue callback__tag;
   struct svalue callback__data;
   struct svalue callback__entity;

   /* flag bitfield; see FLAG_* above */
   int flags;

   p_wchar2 tag_start,tag_end,tag_fin; /* tag_fin is the '/'s in <t/><c></c> */
   p_wchar2 entity_start,entity_end;
   int nargq;
   p_wchar2 argq_start[MAX_ARGQ],argq_stop[MAX_ARGQ];
   p_wchar2 arg_eq; /* = as in foo=bar */

   p_wchar2 *lazy_entity_ends;
   int n_lazy_entity_ends;
  
   p_wchar2 *ws;
   int n_ws;

   /* pre-calculated */
   /* whitespace + end argument ('=', '>' and '/') */
   p_wchar2 *ws_or_endarg; 
   int n_ws_or_endarg;

   /* whitespace + end argument ('=', '>' and '/') + start quote + start entity */
   p_wchar2 *arg_break_chars; 
   int n_arg_break_chars;

   /* end argument ('=', '>' and '/') + start quote */
   p_wchar2 look_for_start[MAX_ARGQ+5];
   int num_look_for_start;

   /* end(s) of _this_ arg quote */
   p_wchar2 look_for_end[MAX_ARGQ][MAX_ARGQ+4];
   int num_look_for_end[MAX_ARGQ];
};

/* default characters */
#define DEF_WS		' ', '\n', '\r', '\t', '\v'
#define DEF_TAG_START	'<'
#define DEF_TAG_END	'>'
#define DEF_TAG_FIN	'/'
#define DEF_ENT_START	'&'
#define DEF_ENT_END	';'
#define DEF_ARGQ_STARTS	'\"', '\''
#define DEF_ARGQ_STOPS	'\"', '\''
#define DEF_EQ		'='
#define DEF_LAZY_EENDS	DEF_WS, DEF_TAG_START, DEF_TAG_END, DEF_TAG_FIN, \
			DEF_ENT_START, DEF_ENT_END, DEF_ARGQ_STARTS, DEF_EQ

/* P_WAIT was already used by MSVC++ :(  /Hubbe */
typedef enum { STATE_DONE=0, STATE_WAIT, STATE_REREAD, STATE_REPARSE } newstate;

#ifdef THIS
#undef THIS /* Needed for NT */
#endif

#define THIS ((struct parser_html_storage*)(Pike_fp->current_storage))
#define THISOBJ (Pike_fp->current_object)

static struct pike_string *empty_string;

static void tag_name(struct parser_html_storage *this,
		     struct piece *feed, ptrdiff_t c, int skip_tag_start);
static void tag_args(struct parser_html_storage *this,
		     struct piece *feed, ptrdiff_t c,struct svalue *def,
		     int skip_name, int to_tag_end);

static int quote_tag_lookup (struct parser_html_storage *this,
			     struct piece *feed, ptrdiff_t c,
			     struct piece **destp, ptrdiff_t *d_p,
			     int finished, struct svalue **mapqentry);

/****** debug helper ********************************/

/* Avoid loss of precision warnings. */
#ifdef __ECL
static inline long TO_LONG(ptrdiff_t x)
{
  return DO_NOT_WARN((long)x);
}
#else /* !__ECL */
#define TO_LONG(x)	((long)(x))
#endif /* __ECL */

#ifdef HTML_DEBUG
void debug_mark_spot(char *desc,struct piece *feed,int c)
{
   ptrdiff_t l, i, i0, m;
   char buf[40];

   if (!(THIS->flags & FLAG_DEBUG_MODE)) return;

   l=strlen(desc)+1;
   if (l<40) l=40;
   m=75-l; if (m<10) m=10;
   fprintf(stderr,"%-*s »",DO_NOT_WARN((int)l),desc);
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

   sprintf(buf,"(%ld) %p:%d/%ld    ^",
	   DO_NOT_WARN((long)i0),
	   (void *)feed, c,
	   DO_NOT_WARN((long)feed->s->len));
   fprintf(stderr,"»\n%*s\n",
	   DO_NOT_WARN((int)(l+c-i0+3)),
	   buf);
}
#endif

/****** init & exit *********************************/

static INLINE void reset_stack_head (struct parser_html_storage *this)
{
   struct feed_stack *st = this->stack;
   st->ignore_data=0;
   st->free_feed=1;
   st->parse_tags=this->flags & FLAG_PARSE_TAGS;
   st->pos = init_pos;
   st->c=0;
}

static void reset_feed(struct parser_html_storage *this)
{
   struct feed_stack *st;

   /* kill feed */

   while (this->feed)
   {
      struct piece *f=this->feed;
      this->feed=f->next;
      really_free_piece (f);
   }
   this->feed_end=NULL;

   /* kill out-feed */

   while (this->out)
   {
      struct out_piece *f=this->out;
      this->out=f->next;
      really_free_out_piece (f);
   }
   this->out_ctx = CTX_DATA;

   /* kill conditional out-feed */

   while (this->cond_out)
   {
      struct out_piece *f=this->cond_out;
      this->cond_out=f->next;
      really_free_out_piece (f);
   }

   /* Free stack and init new stack head. */

   if (this->stack)
     while (1) {
       st=this->stack;
       if (!st->prev) break;
       this->stack=st->prev;
       really_free_feed_stack (st);
     }
   else {
     st=this->stack=alloc_feed_stack();
     st->prev=NULL;
   }
   this->stack_count=0;
   reset_stack_head (this);
}

static void recalculate_argq(struct parser_html_storage *this)
{
   int n,i,j,k;
   int check_fin = (this->flags & (FLAG_STRICT_TAGS|FLAG_XML_TAGS)) != FLAG_STRICT_TAGS;

   /* prepare look for start of argument quote or end of tag */
   this->look_for_start[0]=this->tag_end;
   this->look_for_start[1]=this->arg_eq;
   this->look_for_start[2]=this->tag_start;
   if (check_fin) {
     this->look_for_start[3]=this->tag_fin;
     n=4;
   }
   else n=3;
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
      this->look_for_end[k][n++]=this->entity_start;
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
      if (this->flags & FLAG_LAZY_END_ARG_QUOTE) {
	 this->look_for_end[k][n++]=this->tag_end;
	 if (check_fin) this->look_for_end[k][n++]=this->tag_fin;
      }

      this->num_look_for_end[k]=n;
   }

   if (THIS->ws_or_endarg)
   {
      free(THIS->ws_or_endarg);
      THIS->ws_or_endarg=NULL;
   }
   n = check_fin ? 4 : 3;
   THIS->n_ws_or_endarg=THIS->n_ws+n;
   THIS->ws_or_endarg=(p_wchar2*)xalloc(sizeof(p_wchar2)*THIS->n_ws_or_endarg);
   MEMCPY(THIS->ws_or_endarg+n,
	  THIS->ws,THIS->n_ws*sizeof(p_wchar2));
   THIS->ws_or_endarg[0]=THIS->arg_eq;
   THIS->ws_or_endarg[1]=THIS->tag_end;
   THIS->ws_or_endarg[2]=THIS->tag_start;
   if (check_fin) THIS->ws_or_endarg[3]=THIS->tag_fin;

   if (THIS->arg_break_chars) 
   {
      free(THIS->arg_break_chars);
      THIS->arg_break_chars=NULL;
   }

   THIS->n_arg_break_chars=
      THIS->n_ws_or_endarg+THIS->nargq+1;
   THIS->arg_break_chars=
      (p_wchar2*)xalloc(sizeof(p_wchar2)*(THIS->n_arg_break_chars));

   MEMCPY(THIS->arg_break_chars, THIS->ws_or_endarg,
	  THIS->n_ws_or_endarg*sizeof(p_wchar2));

   MEMCPY(THIS->arg_break_chars+THIS->n_ws_or_endarg,
	  THIS->argq_start, THIS->nargq*sizeof(p_wchar2));

   THIS->arg_break_chars[THIS->n_arg_break_chars-1] = THIS->entity_start;
}

static void init_html_struct(struct object *o)
{
   static p_wchar2 whitespace[]={DEF_WS};
   static p_wchar2 argq_starts[]={DEF_ARGQ_STARTS};
   static p_wchar2 argq_stops[]={DEF_ARGQ_STOPS};
   static p_wchar2 lazy_eends[]={DEF_LAZY_EENDS};

#ifdef HTML_DEBUG
   THIS->flags=0;
#endif
   DEBUG((stderr,"init_html_struct %p\n",THIS));

   /* state */
   THIS->start=NULL;

   /* default set */
   THIS->tag_start=DEF_TAG_START;
   THIS->tag_end=DEF_TAG_END;
   THIS->tag_fin=DEF_TAG_FIN;
   THIS->entity_start=DEF_ENT_START;
   THIS->entity_end=DEF_ENT_END;
   THIS->nargq=sizeof(argq_starts)/sizeof(argq_starts[0]);
   MEMCPY(THIS->argq_start,argq_starts,sizeof(argq_starts));
   MEMCPY(THIS->argq_stop,argq_stops,sizeof(argq_stops));
   THIS->arg_eq=DEF_EQ;
   
   /* allocated stuff */
   THIS->lazy_entity_ends=NULL;
   THIS->ws=NULL;
   THIS->ws_or_endarg=NULL;
   THIS->arg_break_chars=NULL;

   THIS->flags=FLAG_MATCH_TAG|FLAG_PARSE_TAGS;

   /* initialize feed */
   THIS->feed=NULL;
   THIS->out=NULL;
   THIS->cond_out=NULL;
   THIS->stack=NULL;
   reset_feed(THIS);

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
   DEBUG((stderr,"exit_html_struct %p\n",THIS));

   reset_feed(THIS);
   really_free_feed_stack (THIS->stack); /* Only stack head left to free. */

   if (THIS->lazy_entity_ends) free(THIS->lazy_entity_ends);
   if (THIS->ws) free(THIS->ws);
   if (THIS->ws_or_endarg) free(THIS->ws_or_endarg);
   if (THIS->arg_break_chars) free(THIS->arg_break_chars);

   DEBUG((stderr,"exit_html_struct %p done\n",THIS));
}

static void save_subparse_state (struct parser_html_storage *this,
				 struct object *thisobj,
				 struct subparse_save *save)
{
  save->this = this;
  add_ref (save->thisobj = thisobj);
  save->st = this->stack;
  save->prev_free_feed = this->stack->free_feed;
  this->stack->free_feed = 0;
  save->feed = this->stack->prev ? this->stack->local_feed : this->feed;
  save->c = this->stack->c;
  save->pos = this->stack->pos;
  save->cond_out = this->cond_out;
  save->cond_out_end = this->cond_out_end;
  save->out_ctx = this->out_ctx;
  if (!this->cond_out) {
    struct out_piece *ph = alloc_out_piece();
    ph->v.type = T_INT;
    ph->next = NULL;
    this->cond_out = this->cond_out_end = ph;
  }
}

static void finalize_subparse_state (struct subparse_save *save)
{
  struct parser_html_storage *this = save->this;

  this->stack->free_feed = save->prev_free_feed;
  if (this->stack->free_feed) {
    struct piece *cur = this->stack->prev ? this->stack->local_feed : this->feed;
    while (save->feed != cur) {
      struct piece *p = save->feed;
      save->feed = p->next;
      really_free_piece (p);
    }
  }

  if (save->cond_out) {		/* Got a parent subparse save. */
    save->cond_out_end->next = this->cond_out->next;
    this->cond_out->next = save->cond_out->next;
    really_free_out_piece (save->cond_out); /* Remove the placeholder. */
  }
  else {			/* Append the cond queue to the real one. */
    if (this->out)
      this->out_end->next = this->cond_out->next;
    else
      this->out = this->cond_out->next;
    this->out_end = this->cond_out_end;
    really_free_out_piece (this->cond_out); /* Remove the placeholder. */
  }

  free_object (save->thisobj);

#ifdef PIKE_DEBUG
  save->this = (struct parser_html_storage *)(ptrdiff_t) -1;
  save->thisobj = (struct object *)(ptrdiff_t) -1;
  save->st = (struct feed_stack *)(ptrdiff_t) -1;
  save->feed = (struct piece *)(ptrdiff_t) -1;
  save->cond_out = (struct out_piece *)(ptrdiff_t) -1;
  save->cond_out_end = (struct out_piece *)(ptrdiff_t) -1;
#endif
}

static void unwind_subparse_state (struct subparse_save *save)
{
  if (save->thisobj->prog) {    /* Object not destructed. */
    struct parser_html_storage *this = save->this;

    /* Restore the feed and position. */
    this->stack->free_feed = save->prev_free_feed;
    if (this->stack->prev) this->stack->local_feed = save->feed;
    else this->feed = save->feed;
    this->stack->c = save->c;
    this->stack->pos = save->pos;

    /* Free all local feeds below the saved feed_stack entry. */
    while (this->stack != save->st) {
      struct feed_stack *st = this->stack;
      this->stack = st->prev;
      st->free_feed = 1;
      really_free_feed_stack (st);
    }

    /* Free the cond out queue in it and restore the saved one. */
    while (this->cond_out) {
      struct out_piece *f=this->cond_out;
      this->cond_out=f->next;
      really_free_out_piece (f);
    }
    this->cond_out = save->cond_out;
    this->cond_out_end = save->cond_out_end;

    this->out_ctx = save->out_ctx;
  }

  else {			/* Object destructed. */
    /* Free the saved feed. */
    if (save->prev_free_feed)
      while (save->feed) {
	struct piece *p = save->feed;
	save->feed = p->next;
	really_free_piece (p);
      }

    /* Free the saved cond out queue. */
    while (save->cond_out) {
      struct out_piece *f=save->cond_out;
      save->cond_out=f->next;
      really_free_out_piece (f);
    }
  }

  free_object (save->thisobj);
}

/****** setup callbacks *****************************/

/*
**! method Parser.HTML _set_tag_callback(function to_call)
**! method Parser.HTML _set_entity_callback(function to_call)
**! method Parser.HTML _set_data_callback(function to_call)
**!	These functions set up the parser object to
**!	call the given callbacks upon tags, entities
**!	and/or data.
**!
**!	<p>The callbacks will <i>only</i> be called if there isn't
**!	another tag/container/entity handler for these.
**!
**!	<p>The function will be called with the parser
**!	object as first argument, and the active string
**!	as second. 
**!
**!	<p>Note that no parsing of the contents has been done.
**!	Both endtags and normal tags are called, there is
**!	no container parsing.
**!
**! returns the called object
**!
*/

static void html__set_tag_callback(INT32 args)
{
   if (!args) Pike_error("_set_tag_callback: too few arguments\n");
   dmalloc_touch_svalue(sp-args);
   assign_svalue(&(THIS->callback__tag),sp-args);
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

static void html__set_data_callback(INT32 args)
{
   if (!args) Pike_error("_set_data_callback: too few arguments\n");
   dmalloc_touch_svalue(sp-args);
   assign_svalue(&(THIS->callback__data),sp-args);
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

static void html__set_entity_callback(INT32 args)
{
   if (!args) Pike_error("_set_entity_callback: too few arguments\n");
   dmalloc_touch_svalue(sp-args);
   assign_svalue(&(THIS->callback__entity),sp-args);
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*
**! method Parser.HTML add_tag(string name,mixed to_do)
**! method Parser.HTML add_container(string name,mixed to_do)
**! method Parser.HTML add_entity(string entity,mixed to_do)
**! method Parser.HTML add_quote_tag(string name,mixed to_do,string end)
**! method Parser.HTML add_tags(mapping(string:mixed))
**! method Parser.HTML add_containers(mapping(string:mixed))
**! method Parser.HTML add_entities(mapping(string:mixed))
**!	Registers the actions to take when parsing various things.
**!	Tags, containers, entities are as usual. add_quote_tag() adds
**!	a special kind of tag that reads any data until the next
**!	occurrence of the end string immediately before a tag end.
**!
**!	<tt>to_do</tt> can be:
**!	<ul>
**!
**!	<p><li><b>a function</b> to be called. The function is on the form
**!	<pre>
**!     mixed tag_callback(Parser.HTML parser,mapping args,mixed ...extra)
**!	mixed container_callback(Parser.HTML parser,mapping args,string content,mixed ...extra)
**!	mixed entity_callback(Parser.HTML parser,mixed ...extra)
**!	mixed quote_tag_callback(Parser.HTML parser,string content,mixed ...extra)
**!	</pre>
**!	depending on what realm the function is called by.
**!
**!	<p><li><b>a string</b>. This tag/container/entity is then replaced
**!	by the string. The string is normally not reparsed, i.e. it's
**!	equivalent to writing a function that returns the string in an
**!	array (but a lot faster). If <ref>reparse_strings</ref> is
**!	set the string will be reparsed, though.
**!
**!	<p><li><b>an array</b>. The first element is a function as above.
**!	It will receive the rest of the array as extra arguments. If
**!	extra arguments are given by <ref>set_extra</ref>(), they will
**!	appear after the ones in this array.
**!
**!	<p><li><b>zero</b>. If there is a tag/container/entity with the
**!	given name in the parser, it's removed.
**!
**!	</ul>
**!
**!     <p>The callback function can return:
**!	<ul>
**!
**!	<p><li><b>a string</b>; this string will be pushed on the parser
**!	stack and be parsed. Be careful not to return anything
**!	in this way that could lead to a infinite recursion.
**!
**!	<p><li><b>an array</b>; the element(s) of the array is the result
**!	of the function. This will not be parsed. This is useful for
**!	avoiding infinite recursion. The array can be of any size,
**!	this means the empty array is the most effective to return if
**!	you don't care about the result. If the parser is operating in
**!	<ref>mixed_mode</ref>, the array can contain anything.
**!	Otherwise only strings are allowed.
**!
**!	<p><li><b>zero</b>; this means "don't do anything", ie the
**!	item that generated the callback is left as it is, and
**!	the parser continues.
**!
**!	<p><li><b>one</b>; reparse the last item again. This is useful to
**!	parse a tag as a container, or vice versa: just add or remove
**!	callbacks for the tag and return this to jump to the right
**!	callback.
**!
**!	</ul>
**!
**! returns the called object
**!
**! see also: tags, containers, entities
**!	
*/

static void html_add_tag(INT32 args)
{
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
   }

   if (THIS->flags & FLAG_CASE_INSENSITIVE_TAG) {
     stack_swap();
     f_lower_case(1);
     stack_swap();
   }

   if (IS_ZERO(sp-1))
     map_delete(THIS->maptag,sp-2);
   else
     mapping_insert(THIS->maptag,sp-2,sp-1);
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

static void html_add_container(INT32 args)
{
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

   if (args > 2) {
     pop_n_elems(args-2);
     args = 2;
   }

   if (THIS->mapcont->refs>1)
   {
      push_mapping(THIS->mapcont);
      THIS->mapcont=copy_mapping(THIS->mapcont);
      pop_stack();
   }

   if (THIS->flags & FLAG_CASE_INSENSITIVE_TAG) {
     stack_swap();
     f_lower_case(1);
     stack_swap();
   }

   if (IS_ZERO(sp-1))
     map_delete(THIS->mapcont,sp-2);
   else
     mapping_insert(THIS->mapcont,sp-2,sp-1);
   pop_n_elems(args);
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
  struct mapping *map;
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
    /* Note: Deferred copy; the arrays might still have only one ref. */
    THIS->mapqtag=copy_mapping(THIS->mapqtag);
    pop_stack();
  }
  map = THIS->mapqtag;

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

  val = low_mapping_string_lookup (map, prefix);
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
	    map_delete (map, &tmp);
	  }
	  else {
	    if (arr->refs > 1 || map->data->refs > 1) {
	      push_array (arr = copy_array (arr));
	      mapping_string_insert (map, prefix, sp - 1);
	      pop_stack();
	    }
	    free_svalues (arr->item+i, 3, BIT_MIXED);
	    MEMCPY (arr->item+i, arr->item+i+3, (arr->size-i-3) * sizeof(struct svalue));
	    arr->size -= 3;
	  }
	else {
	  if (arr->refs > 1 || map->data->refs > 1) {
	    push_array (arr = copy_array (arr));
	    mapping_string_insert (map, prefix, sp - 1);
	    pop_stack();
	  }
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
	  if (arr->refs > 1 || map->data->refs > 1) {
	    arr = copy_array (arr);
	    push_array (arr = resize_array (arr, arr->size+3));
	    mapping_string_insert (map, prefix, sp - 1);
	    pop_stack();
	  }
	  else
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
      if (arr->refs > 1 || map->data->refs > 1) {
	arr = copy_array (arr);
	push_array (arr = resize_array (arr, arr->size+3));
	mapping_string_insert (map, prefix, sp - 1);
	pop_stack();
      }
      else
	arr = val->u.array = resize_array (arr, arr->size+3);
      MEMCPY (arr->item+arr->size-3, sp-=3, 3 * sizeof(struct svalue));
    }

  done:	;
  }

  else if (!remove) {
    f_aggregate (3);
    mapping_string_insert (map, prefix, sp-1);
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
**! method Parser.HTML clear_tags()
**! method Parser.HTML clear_containers()
**! method Parser.HTML clear_entities()
**! method Parser.HTML clear_quote_tags()
**!	Removes all registered definitions in the different
**!	categories.
**!
**! returns the called object
**!
**! see also: add_tag, add_tags, add_container, add_containers, add_entity, add_entities
*/

static void html_clear_tags (INT32 args)
{
  pop_n_elems(args);
  free_mapping (THIS->maptag);
  THIS->maptag = allocate_mapping (32);
  ref_push_object (THISOBJ);
}

static void html_clear_containers (INT32 args)
{
  pop_n_elems(args);
  free_mapping (THIS->mapcont);
  THIS->mapcont = allocate_mapping (32);
  ref_push_object (THISOBJ);
}

static void html_clear_entities (INT32 args)
{
  pop_n_elems(args);
  free_mapping (THIS->mapentity);
  THIS->mapentity = allocate_mapping (32);
  ref_push_object (THISOBJ);
}

static void html_clear_quote_tags (INT32 args)
{
  pop_n_elems(args);
  free_mapping (THIS->mapqtag);
  THIS->mapqtag = allocate_mapping (32);
  ref_push_object (THISOBJ);
}

/*
**! method mapping tags()
**! method mapping containers()
**! method mapping entities()
**! method mapping quote_tags()
**!	Returns the current callback settings. For quote_tags, the
**!	values are arrays ({callback, end_quote}).
**!
**!	<p>Note that when matching is done case insensitively, all names
**!	will be returned in lowercase.
**!
**!	<p>Implementation note: With the exception of quote_tags(), these
**!	run in constant time since they return copy-on-write mappings.
**!	However, quote_tags() allocates a new mapping and thus runs in
**!	linear time.
**!
**! see also: add_tag, add_tags, add_container, add_containers, add_entity, add_entities
*/

static void html_tags(INT32 args)
{
   pop_n_elems(args);
   push_mapping(copy_mapping(THIS->maptag)); /* deferred copy */
}

static void html_containers(INT32 args)
{
   pop_n_elems(args);
   push_mapping(copy_mapping(THIS->mapcont)); /* deferred copy */
}

static void html_entities(INT32 args)
{
   pop_n_elems(args);
   push_mapping(copy_mapping(THIS->mapentity)); /* deferred copy */
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
       push_svalue (arr->item+i+1);
#ifdef PIKE_DEBUG
       if (arr->item[i+2].type != T_STRING)
	 fatal ("Expected string as end in mapqtag.\n");
#endif
       end = arr->item[i+2].u.string;
       push_string (string_slice (end, 0, end->len-1));
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
				int *scan_entity)
{
   if (this->callback__entity.type!=T_INT ||
       m_sizeof(this->mapentity))
      *scan_entity=1;
   else 
      *scan_entity=0;
}

/* -------------- */
/* feed to output */

static void put_out_feed(struct parser_html_storage *this,
			 struct svalue *v, int force_real_out)
{
   struct out_piece *f;

   f = alloc_out_piece();
   assign_svalue_no_free(&f->v,v);

   f->next=NULL;

   if (!force_real_out && this->cond_out) {
     this->cond_out_end->next = f;
     this->cond_out_end = f;
   }
   else {
     if (this->out==NULL)
       this->out=this->out_end=f;
     else
     {
       this->out_end->next=f;
       this->out_end=f;
     }
   }
}

/* ---------------------------- */
/* feed range in feed to output */

static void put_out_feed_range(struct parser_html_storage *this,
			       struct piece *head,
			       ptrdiff_t c_head,
			       struct piece *tail,
			       ptrdiff_t c_tail)
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
	   put_out_feed(this,sp-1,0);
	   pop_stack();
	 }
	 return;
      }
      if (head->s->len-c_head)	/* Ignore empty strings. */
      {
	push_string(string_slice(head->s,c_head,head->s->len-c_head));
	put_out_feed(this,sp-1,0);
	pop_stack();
      }
      c_head=0;
      head=head->next;
   }
   fatal("internal error: tail not found in feed (put_out_feed_range)\n");
}

/* ------------------------ */
/* push feed range on stack */

static INLINE void push_feed_range(struct piece *head,
				   ptrdiff_t c_head,
				   struct piece *tail,
				   ptrdiff_t c_tail)
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
      fatal("internal error: tail not found in feed (push_feed_range)\n");
   if (!n)
      ref_push_string(empty_string);
   else if (n>1)
      f_add(n);
   DEBUG((stderr,"push len=%d\n",sp[-1].u.string->len));
}

/* -------------------------------------------------------- */
/* go forward by adjusting for a positive displacement in c */

#define FORWARD_CHAR(feed, c, dest, dp) do {				\
  (dest) = (struct piece *) (feed);					\
  (dp) = (ptrdiff_t) (c) + 1;						\
  while ((dp) == (dest)->s->len && (dest)->next) {			\
    (dp) = 0;								\
    (dest) = (dest)->next;						\
  }									\
} while (0)

static INLINE int n_pos_forward (struct piece *feed, ptrdiff_t c,
				 struct piece **dest, ptrdiff_t *dp)
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

static INLINE int cmp_feed_pos(struct piece *piece_a, ptrdiff_t pos_a,
			       struct piece *piece_b, ptrdiff_t pos_b)
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
				    ptrdiff_t start,
				    ptrdiff_t stop)
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
	 Pike_error("unknown width of string\n");
   }
   loc->byteno=b;
}

static void skip_feed_range(struct feed_stack *st,
			    struct piece **headp,
			    ptrdiff_t *c_headp,
			    struct piece *tail,
			    ptrdiff_t c_tail)
{
   struct piece *head=*headp;
   ptrdiff_t c_head=*c_headp;

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
      if (st->free_feed)
	really_free_piece (head);
      head=*headp;
      c_head=0;
   }

   *c_headp = 0;
}

/* ------------------------------ */
/* scan forward for certain chars */

static int scan_forward(struct piece *feed,
			ptrdiff_t c,
			struct piece **destp,
			ptrdiff_t *d_p,
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
	 if (i > 30) {
	   fprintf (stderr, "\nnum_look_for suspiciously large: %d", num_look_for);
	   break;
	 }
	 else if (look_for[i]<33 || (look_for[i]>126 && look_for[i]<160)
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

   while (c >= feed->s->len) {
     c -= feed->s->len;
     if (!feed->next) {
       *destp = feed;
       *d_p = feed->s->len;
       SCAN_DEBUG_MARK_SPOT("scan_forward end (start offset beyond feed end)",
			    *destp,*d_p);
       return 0;
     }
     feed = feed->next;
   }

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
	 if (!rev) {
	    while (feed)
	    {
	       ptrdiff_t ce = feed->s->len - c;
	       p_wchar2 f=(p_wchar2)*look_for;
	       SCAN_DEBUG_MARK_SPOT("scan_forward piece loop (1)",feed,c);
	       switch (feed->s->size_shift)
	       {
		  case 0:
		  {
		     p_wchar0*s=((p_wchar0*)feed->s->str)+c;
		     while (ce--)
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
		     while (ce--)
			if ((p_wchar2)*(s++)==f)
			{
			   c=feed->s->len-ce;
			   goto found;
			}
		  }
		  break;
		  case 2:
		  {
		     p_wchar2*s=((p_wchar2*)feed->s->str)+c;
		     while (ce--)
			if (*(s++)==f)
			{
			   c=feed->s->len-ce;
			   goto found;
			}
		  }
		  break;
		  default:
		     Pike_error("unknown width of string\n");
	       }
	       if (!feed->next) break;
	       c=0;
	       feed=feed->next;
	    }
	    break;
	 }
	 /* FALL THROUGH */

      default:  /* num_look_for > 1 */
	 while (feed)
	 {
	    ptrdiff_t ce = feed->s->len - c;
	    SCAN_DEBUG_MARK_SPOT("scan_forward piece loop (>1)",feed,c);
	    switch (feed->s->size_shift)
	    {
	       case 0:
	       {
		  int n;
		  p_wchar0*s=((p_wchar0*)feed->s->str)+c;
		  while (ce--)
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
		  while (ce--)
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
		  while (ce--)
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
	       default:
		  Pike_error("unknown width of string\n");
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
   c-=!rev;
   while (c == feed->s->len && feed->next) {
     c = 0;
     feed = feed->next;
   }
   *destp=feed;
   *d_p=c;
   SCAN_DEBUG_MARK_SPOT("scan_forward found",feed,c);
   return 1;
}

static int scan_for_string (struct parser_html_storage *this,
			    struct piece *feed,
			    ptrdiff_t c,
			    struct piece **destp,
			    ptrdiff_t *d_p,
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
      ptrdiff_t cdst;							\
      if (!scan_forward (feed, c, &feed, &c, &look_for, 1)) {		\
	*destp = feed;							\
	*d_p = c;							\
	return 0;							\
      }									\
									\
      p = (TYPE *) str->str + 1;					\
      e = (TYPE *) str->str + str->len;					\
      dst = feed;							\
      cdst = c + 1;							\
      for (; p < e; p++, cdst++) {					\
	while (cdst == dst->s->len) {					\
	  if (dst->next) dst = dst->next;				\
	  else {							\
	    *destp = dst;						\
	    *d_p = dst->s->len;						\
	    return 0;							\
	  }								\
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
    default: Pike_error ("Unknown width of string.\n");
  }

#undef LOOP

  return 0;
}

enum scan_arg_do {SCAN_ARG_ONLY, SCAN_ARG_PUSH, SCAN_ARG_ENT_BREAK};

static int scan_forward_arg(struct parser_html_storage *this,
			    struct piece *feed,
			    ptrdiff_t c,
			    struct piece **destp,
			    ptrdiff_t *d_p,
			    enum scan_arg_do what,
			    int finished,
			    int *quote)
/* Returns 1 if end is found, 2 if broken for entity, 0 otherwise. If
 * 2 is returned, *quote is set to the index in this->argq_start for
 * the current quote we're in, or -1 if outside quotes. *quote is also
 * read on entry to continue inside quotes. */
{
   p_wchar2 ch;
   int res,i;
   int n=0;
   int q=0;

   DEBUG_MARK_SPOT("scan_forward_arg: start",feed,c);

   if (quote && *quote >= 0) {
     *destp = feed;
     *d_p = c-1;
     i = *quote;
     goto in_quote_cont;	/* jump right into the loop */
   }

   for (;;)
   {
      /* we are here: */
      /* < f'o'"o" = bar > */
      /*   ^     ->        */
      /*      this is the end */

      DEBUG_MARK_SPOT("scan_forward_arg: loop",feed,c);

      res=scan_forward(feed,c,destp,d_p,
		       this->arg_break_chars,
		       this->n_arg_break_chars);

      if (what == SCAN_ARG_PUSH) push_feed_range(feed,c,*destp,*d_p),n++;

      if (!res) {
	 if (!finished) 
	 {
	    DEBUG_MARK_SPOT("scan_forward_arg: wait",feed,c);
	    if (what == SCAN_ARG_PUSH) pop_n_elems(n);
	    return 0;
	 }
	 else
	 {
	    DEBUG_MARK_SPOT("scan_forward_arg: forced end",destp[0],*d_p);
	    break;
	 }
      }

      ch=index_shared_string(destp[0]->s,*d_p);

      if (ch==this->arg_eq)
      {
	 DEBUG_MARK_SPOT("scan_forward_arg: end by arg_eq",
			 destp[0],*d_p);
	 break;
      }

      if (ch==this->tag_end) {
	 if ((this->flags & FLAG_MATCH_TAG) && q--)
	 {
	    DEBUG_MARK_SPOT("scan_forward_arg: inner tag end",
			    destp[0],*d_p);
	    if (what == SCAN_ARG_PUSH) push_feed_range(*destp,*d_p,*destp,*d_p+1),n++;
	    goto next;
	 }
	 else
	 {
	    DEBUG_MARK_SPOT("scan_forward_arg: end by tag end",
			    destp[0],*d_p);
	    break;
	 }
      }

      if ((this->flags & FLAG_MATCH_TAG) && ch==this->tag_start)
      {
	 DEBUG_MARK_SPOT("scan_forward_arg: inner tag start",
			 destp[0],*d_p);
	 q++;
	 if (what == SCAN_ARG_PUSH) push_feed_range(*destp,*d_p,*destp,*d_p+1),n++;
	 goto next;
      }

      if (ch == this->entity_start) {
	DEBUG_MARK_SPOT("scan_forward_arg: entity start char",
			destp[0],*d_p);
	if (what == SCAN_ARG_ENT_BREAK) {
	  if (quote) *quote = -1;
	  return 2;
	}
	else {
	  if (what == SCAN_ARG_PUSH) push_feed_range(*destp,*d_p,*destp,*d_p+1),n++;
	  goto next;
	}
      }

      /* scan for (possible) end(s) of this argument quote */

      for (i=0; i<this->nargq; i++)
	 if (ch==this->argq_start[i]) break;
      if (i==this->nargq) { /* it was whitespace */
	if (ch == this->tag_fin) {
	  FORWARD_CHAR (*destp, *d_p, feed, c);
	  if (((this->flags & FLAG_MATCH_TAG) && q) ||
	      index_shared_string (feed->s, c) != this->tag_end) {
	    DEBUG_MARK_SPOT("scan_forward_arg: tag fin char",
			    destp[0],*d_p);
	    if (what == SCAN_ARG_PUSH) push_feed_range (*destp, *d_p, feed, c), n++;
	    goto next;
	  }
	  else {
	    DEBUG_MARK_SPOT("scan_forward_arg: end by tag fin",
			    destp[0],*d_p);
	    break;
	  }
	}
	else {
	  DEBUG_MARK_SPOT("scan_forward_arg: end by ws",
			  destp[0],*d_p);
	  break;
	}
      }

in_quote_cont:
      DEBUG_MARK_SPOT("scan_forward_arg: quoted",destp[0],*d_p);
      while (1) {
	res=scan_forward(feed=*destp,c=d_p[0]+1,destp,d_p,
			 this->look_for_end[i],this->num_look_for_end[i]);
	if (what == SCAN_ARG_PUSH) push_feed_range(feed,c,*destp,*d_p),n++;

	if (!res) {
	  if (!finished) 
	  {
	    DEBUG_MARK_SPOT("scan_forward_arg: wait (quote)",feed,c);
	    if (what == SCAN_ARG_PUSH) pop_n_elems(n);
	    return 0;
	  }
	  else
	  {
	    DEBUG_MARK_SPOT("scan_forward_arg: forced end (quote)",destp[0],*d_p);
	    break;
	  }
	}

	if (index_shared_string(destp[0]->s,*d_p) == this->entity_start) {
	  DEBUG_MARK_SPOT("scan_forward_arg: entity start char",
			  destp[0],*d_p);
	  if (what == SCAN_ARG_ENT_BREAK) {
	    if (quote) *quote = i;
	    return 2;
	  }
	  else {
	    if (what == SCAN_ARG_PUSH) push_feed_range(*destp,*d_p,*destp,*d_p+1),n++;
	    continue;
	  }
	}

	goto next;
      }

next:
      if (n > 20) {
	f_add(n);
	n = 1;
      }

      feed=*destp;
      c=d_p[0]+1;
   }

   if (what == SCAN_ARG_PUSH)
   {
      if (n>1) f_add(n);
      else if (!n) ref_push_string(empty_string);
   }
   return 1;
}

static int scan_for_end_of_tag(struct parser_html_storage *this,
			       struct piece *feed,
			       ptrdiff_t c,
			       struct piece **destp,
			       ptrdiff_t *d_p,
			       int finished,
			       int match_tag,
			       int *got_fin)
/* match_tag == 0: don't match inner tags, > 0: match inner tags, < 0:
 * consider it an unbalanced tag and break. got_fin is assigned
 * nonzero if there's a '/' before the tag end, zero otherwise. */
{
   p_wchar2 ch;
   int res,i;
   int q=0;

   /* maybe these should be cached */

   /* bla bla <tag foo 'bar' "gazonk" > */
   /*          ^                      ^ */
   /*       here now             scan here */

   DEBUG((stderr,"scan for end of tag: %p:%d\n",feed,c));

   if (got_fin) *got_fin = 0;

   for (;;)
   {
      /* scan for start of argument quote or end of tag */

      res=scan_forward(feed,c,destp,d_p,
		       this->look_for_start,this->num_look_for_start);
      if (!res) {
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

      if (ch == this->tag_fin) {
	DEBUG_MARK_SPOT("scan for end of tag: tag_fin",
			 destp[0],*d_p);
	FORWARD_CHAR (*destp, *d_p, feed, c);
	if (match_tag && q) continue;
	else {
	  ch = index_shared_string (feed->s, c);
	  if (ch == this->tag_end) {
	    if (got_fin) *got_fin = 1;
	    *destp = feed;
	    *d_p = c;
	    DEBUG_MARK_SPOT("scan for end of tag: end by tag_fin + tag_end",
			    destp[0],*d_p);
	    return 1;
	  }
	  else continue;
	}
      }

      if (ch==this->tag_end) {
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
      }

      if (ch==this->tag_start) {
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
      }

      /* scan for (possible) end(s) of this argument quote */

      for (i=0; i<this->nargq; i++)
	 if (ch==this->argq_start[i]) break;

      do {
	res=scan_forward(*destp,d_p[0]+1,destp,d_p,
			 this->look_for_end[i],this->num_look_for_end[i]);
	if (!res) {
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
	}
      }
      while (index_shared_string(destp[0]->s, *d_p) == this->entity_start);

      feed=*destp;
      c=d_p[0]+1;
   }
}

/* ---------------------------------- */

struct chr_buf_len2
{
  p_wchar2 str[2];
  struct piece *p[2];
  ptrdiff_t c[2];
};

static int quote_tag_lookup (struct parser_html_storage *this,
			     struct piece *feed, ptrdiff_t c,
			     struct piece **destp, ptrdiff_t *d_p,
			     int finished, struct svalue **mapqentry)
/* Checks for a quote tag starting at the given position. If one is
 * found, scans to the end of it and sets *mapqentry to the first
 * svalue in the ({name, to_do, end}) tuple in this->mapqtag. */
{
  struct chr_buf_len2 buf;
  struct piece *dst = feed;
  ptrdiff_t cdst = c;
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
	  default: Pike_error ("Unknown width of string.\n");
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
#ifdef HTML_DEBUG
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

static INLINE void add_local_feed (struct parser_html_storage *this,
				   struct pike_string *str)
{
  struct piece *p = alloc_piece();
  struct feed_stack *new = alloc_feed_stack();
  /* Note: Assumes that above alloc never fails with an exception or a
   * zero value (i.e. it exits the process immediately instead). */

  new->local_feed = p;

  copy_shared_string(p->s,str);
  p->next=NULL;
  new->ignore_data=0;
  new->free_feed=1;
  new->parse_tags=this->stack->parse_tags && this->out_ctx == CTX_DATA;
  new->pos=init_pos;
  new->prev=this->stack;
  new->c=0;
  this->stack=new;
  this->stack_count++;
}

static newstate handle_result(struct parser_html_storage *this,
			      struct feed_stack *st,
			      struct piece **head,
			      ptrdiff_t *c_head,
			      struct piece *tail,
			      ptrdiff_t c_tail)
{
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

	 add_local_feed (this, sp[-1].u.string);
	 pop_stack();
	 return STATE_REREAD; /* please reread stack head */

      case T_INT:
	 switch (sp[-1].u.integer)
	 {
	    case 0:
	       if ((this->type == TYPE_TAG ||
		    this->type == TYPE_CONT) &&
		   (this->callback__entity.type != T_INT ||
		    m_sizeof (this->mapentity))) {
		 /* If it's a tag and we got entities, just output the
		  * tag starter and name and switch to CTX_TAG to
		  * parse any entities in the tag. */
		 struct piece *pos;
		 ptrdiff_t cpos;
		 if (this->flags & FLAG_WS_BEFORE_TAG_NAME)
		   scan_forward (*head, *c_head + 1, &pos, &cpos,
				 this->ws, -this->n_ws);
		 else pos = *head, cpos = *c_head + 1;
		 scan_forward_arg (this, pos, cpos, &pos, &cpos,
				   SCAN_ARG_ONLY, 1, NULL);
		 put_out_feed_range(this,*head,*c_head,pos,cpos);
		 skip_feed_range(st,head,c_head,pos,cpos);
		 this->out_ctx = CTX_TAG;
		 return STATE_REREAD;
	       }
	       else if (*head) {
		 /* just output the whole range */
		 put_out_feed_range(this,*head,*c_head,tail,c_tail);
		 skip_feed_range(st,head,c_head,tail,c_tail);
	       }
	       pop_stack();
	       if (this->stack != st) /* got more feed recursively - reread */
		  return STATE_REREAD;
	       return STATE_DONE; /* continue */
	    case 1:
	       /* reparse the last thing */
	       pop_stack();
	       return STATE_REPARSE;
	 }
	 Pike_error("Parser.HTML: illegal result from callback: %d, "
		    "not 0 (skip) or 1 (wait)\n",
		    sp[-1].u.integer);

      case T_ARRAY:
	 /* output element(s) */
	 for (i=0; i<sp[-1].u.array->size; i++)
	 {
	    if (!(THIS->flags & FLAG_MIXED_MODE) &&
		sp[-1].u.array->item[i].type!=T_STRING)
	       Pike_error("Parser.HTML: illegal result from callback: "
			  "element in array not string\n");
	    push_svalue(sp[-1].u.array->item+i);
	    put_out_feed(this,sp-1,0);
	    pop_stack();
	 }
	 skip_feed_range(st,head,c_head,tail,c_tail);
	 pop_stack();
	 if (this->stack != st) /* got more feed recursively - reread */
	    return STATE_REREAD;
	 return STATE_DONE; /* continue */

      default:
	 Pike_error("Parser.HTML: illegal result from callback: "
		    "not 0, string or array(string)\n");
   }
   /* NOT_REACHED */
   return STATE_DONE;
}

static void clear_start(struct parser_html_storage *this)
{
   this->start=NULL;
}

static void do_callback(struct parser_html_storage *this,
			struct object *thisobj,
			struct svalue *callback_function,
			struct piece *start,
			ptrdiff_t cstart,
			struct piece *end,
			ptrdiff_t cend)
{
   ONERROR uwp;

   this->start=start;
   this->cstart=cstart;
   this->end=end;
   this->cend=cend;

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
				struct piece **cutstart, ptrdiff_t *ccutstart,
				struct piece *cutend, ptrdiff_t ccutend)
{
   int args;
   ONERROR uwp;

   switch (v->type)
   {
      case T_STRING:
	 skip_feed_range(st,cutstart,ccutstart,cutend,ccutend);
	 if (this->flags & FLAG_REPARSE_STRINGS) {
	   add_local_feed (this, v->u.string);
	   return STATE_REREAD;
	 }
	 else {
	   put_out_feed(this,v,0);
	   return STATE_DONE;
	 }
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
	 Pike_error("Parser.HTML: illegal type found "
		    "when trying to call entity callback\n");
   }

   this->start=*cutstart;
   this->cstart=*ccutstart;
   this->end=cutend;
   this->cend=ccutend;

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
			     struct piece **cutstart, ptrdiff_t *ccutstart,
			     struct piece *cutend, ptrdiff_t ccutend)
{
   int args;
   ONERROR uwp;

   switch (v->type)
   {
      case T_STRING:
	 skip_feed_range(st,cutstart,ccutstart,cutend,ccutend);
	 if (this->flags & FLAG_REPARSE_STRINGS) {
	   add_local_feed (this, v->u.string);
	   return STATE_REREAD;
	 }
	 else {
	   put_out_feed(this,v,0);
	   return STATE_DONE;
	 }
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
	 Pike_error("Parser.HTML: illegal type found "
		    "when trying to call tag callback\n");
   }

   this->start=*cutstart;
   this->cstart=*ccutstart;
   this->end=cutend;
   this->cend=ccutend;
   this->type=TYPE_TAG;

   SET_ONERROR(uwp,clear_start,this);

   args=3;
   ref_push_object(thisobj);
   tag_args(this,this->start,this->cstart,NULL,1,1);

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
				   struct piece *startc, ptrdiff_t cstartc,
				   struct piece *endc, ptrdiff_t cendc,
				   struct feed_stack *st,
				   struct piece **cutstart, ptrdiff_t *ccutstart,
				   struct piece *cutend, ptrdiff_t ccutend)
{
   int args;
   ONERROR uwp;

   switch (v->type)
   {
      case T_STRING:
	 skip_feed_range(st,cutstart,ccutstart,cutend,ccutend);
	 if (this->flags & FLAG_REPARSE_STRINGS) {
	   add_local_feed (this, v->u.string);
	   return STATE_REREAD;
	 }
	 else {
	   put_out_feed(this,v,0);
	   return STATE_DONE;
	 }
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
	 Pike_error("Parser.HTML: illegal type found "
		    "when trying to call container callback\n");
   }

   this->start=*cutstart;
   this->cstart=*ccutstart;
   this->end=cutend;
   this->cend=ccutend;
   this->type=TYPE_CONT;

   SET_ONERROR(uwp,clear_start,this);

   args=4;
   ref_push_object(thisobj);
   tag_args(this,this->start,this->cstart,NULL,1,1);
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
				   struct piece *startc, ptrdiff_t cstartc,
				   struct piece *endc, ptrdiff_t cendc,
				   struct feed_stack *st,
				   struct piece **cutstart, ptrdiff_t *ccutstart,
				   struct piece *cutend, ptrdiff_t ccutend)
{
   int args;
   ONERROR uwp;

   switch (v->type)
   {
      case T_STRING:
	 skip_feed_range(st,cutstart,ccutstart,cutend,ccutend);
	 if (this->flags & FLAG_REPARSE_STRINGS) {
	   add_local_feed (this, v->u.string);
	   return STATE_REREAD;
	 }
	 else {
	   put_out_feed(this,v,0);
	   return STATE_DONE;
	 }
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
	 Pike_error("Parser.HTML: illegal type found "
		    "when trying to call quote tag callback\n");
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
				      struct piece *feed, ptrdiff_t c,
				      struct piece **e1, ptrdiff_t *ce1,
				      struct piece **e2, ptrdiff_t *ce2,
				      int finished)
/* e1/ce1 is set to the first position of the end tag. e2/ce2 is set
 * to the first position after the end tag. Both are set to the end of
 * the feed if finished is set and an end tag wasn't found. */
{
   struct piece *s1,*s2,*s3;
   ptrdiff_t c1,c2,c3;
   newstate res;

   DEBUG_MARK_SPOT("find_end_of_container",feed,c);

   for (;;)
   {
      int found, endtag, got_fin;

      DEBUG_MARK_SPOT("find_end_of_cont : loop",feed,c);
      if (!scan_forward(feed,c,&s1,&c1,&(this->tag_start),1))
      {
	 if (!finished) 
	 {
	    DEBUG_MARK_SPOT("find_end_of_cont : wait",s1,c1);
	    return STATE_WAIT; /* please wait */
	 }
	 else
	 {
	    DEBUG_MARK_SPOT("find_end_of_cont : forced end",s1,c1);
	    *e1=*e2=s1;
	    *ce1=*ce2=c1;
	    return STATE_DONE; /* end of tag, sure... */
	 }
      }
      DEBUG_MARK_SPOT("find_end_of_cont : found tag",s1,c1);

      /* scan ws to start of tag name */
      if (this->flags & FLAG_WS_BEFORE_TAG_NAME) {
	if (!scan_forward(s1,c1+1,&s2,&c2,this->ws,-this->n_ws)) {
	  DEBUG_MARK_SPOT("find_end_of_cont : wait at pre tag ws",s2,c2);
	  return STATE_WAIT; /* come again */
	}
      }
      else FORWARD_CHAR (s1, c1, s2, c2);

      if (m_sizeof(this->mapqtag)) {
	/* must check nested quote tags, since they affect the syntax itself */
	struct svalue *v;
	if (!quote_tag_lookup(this,s2,c2,&s3,&c3,finished,&v)) {
	  DEBUG_MARK_SPOT("find_end_of_cont : wait at quote tag",s2,c2);
	  return STATE_WAIT; /* come again */
	}
	if (v) {
	  DEBUG_MARK_SPOT("find_end_of_cont : quote tag",s2,c2);
	  if (!scan_for_string(this,s3,c3,&s3,&c3,v[2].u.string)) {
	    if (finished) {
	      DEBUG_MARK_SPOT("find_end_of_cont : forced end in quote tag",s1,c1);
	      *e1 = *e2 = s3;
	      *ce1 = *ce2 = c3;
	      return STATE_DONE;
	    }
	    else {
	      DEBUG_MARK_SPOT("find_end_of_cont : wait in quote tag",s1,c1);
	      return STATE_WAIT;
	    }
	  }
	  n_pos_forward(s3,c3+v[2].u.string->len,&feed,&c);
	  DEBUG_MARK_SPOT("find_end_of_cont : quote tag end",feed,c);
	  continue;
	}
      }

      /* tag or end tag? */
      endtag = index_shared_string (s2->s, c2) == this->tag_fin;
      if (endtag) c2++;

      /* scan tag name as argument and push */
      if (!scan_forward_arg(this,s2,c2,&s3,&c3,SCAN_ARG_PUSH,finished,NULL))
      {
	DEBUG_MARK_SPOT("find_end_of_cont : wait at tag name",s2,c2);
	return STATE_WAIT; /* come again */
      }
#ifdef HTML_DEBUG
      if (endtag)
	DEBUG_MARK_SPOT("find_end_of_cont : got end tag",s2,c2);
      else
	DEBUG_MARK_SPOT("find_end_of_cont : got tag",s2,c2);
#endif

      if (this->flags & FLAG_CASE_INSENSITIVE_TAG)
	f_lower_case(1);
      found = is_eq(sp-1,tagname);

      /* When we should treat unknown tags as text, we have to parse
       * and lookup just the tag name before continuing. This is an
       * awkward situation, since there's no telling whether the
       * parser used for the contents will define the same tags. */
      if (!found && (THIS->flags & FLAG_IGNORE_UNKNOWN)) {
	if (endtag) {
	  struct pike_string *s;
	  push_string (make_shared_binary_string2 (&this->tag_fin, 1));
	  s = add_shared_strings (sp[-1].u.string, sp[-2].u.string);
	  pop_stack();
	  push_string (s);
	}
	else
	  stack_dup();
	if (!((m_sizeof(this->maptag) &&
	       (low_mapping_lookup(this->maptag,sp-1) ||
	       ((THIS->flags & (FLAG_XML_TAGS|FLAG_STRICT_TAGS)) ==
		(FLAG_XML_TAGS|FLAG_STRICT_TAGS) &&
		low_mapping_lookup(this->maptag,sp-2)))) ||
	      (m_sizeof(this->mapcont) &&
	       (low_mapping_lookup(this->mapcont,sp-1) ||
		low_mapping_lookup(this->mapcont,sp-2))))) {
	  DEBUG((stderr,"find_end_of_cont : treating unknown tag as text\n"));
	  feed = s3;
	  c = c3;
	  pop_n_elems (2);
	  continue;
	}
	pop_stack();
      }

      if (tagname->u.string->len > sp[-1].u.string->len &&
	  c3 < s3->s->len && index_shared_string (s3->s,c3) == this->tag_start) {
	struct pike_string *cmp =
	  string_slice (tagname->u.string, 0, sp[-1].u.string->len);
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
			       found && endtag ? -1 : this->flags & FLAG_MATCH_TAG,
			       &got_fin))
      {
	DEBUG_MARK_SPOT("find_end_of_cont : wait at tag end",s2,c2);
	 return STATE_WAIT;
      }

      if (found) {
	if (!endtag) { /* push a new level */
	  if (got_fin && this->flags & FLAG_XML_TAGS) {
	    DEBUG_MARK_SPOT("find_end_of_cont : skipping empty element tag",s2,c2+1);
	    feed = s2;
	    c = c2+1;
	    continue;
	  }

	  DEBUG_MARK_SPOT("find_end_of_cont : push",s2,c2+1);
	  res=find_end_of_container(this,tagname,
				    s2,++c2,e1,ce1,&feed,&c,finished);
	  DEBUG_MARK_SPOT("find_end_of_cont : push end",feed,c);
	  if (res)
	  {
	    DEBUG((stderr,"find_end_of_cont : (pushed) return %d %p:%d\n",
		   res,s1,c1));
	    return res;
	  }
	}

	else {
	  if (index_shared_string (s2->s, c2) == this->tag_start) {
	    DEBUG_MARK_SPOT("find_end_of_cont : skipping open endtag",s2,c2);
	    feed = s2;
	    c = c2;
	    continue;
	  }

	  DEBUG_MARK_SPOT("find_end_of_cont : got cont end   --> ",s1,c1);
	  DEBUG_MARK_SPOT("find_end_of_cont : got endtag end --> ",s2,c2);

	  *e1=s1;
	  *ce1=c1;
	  *e2=s2;
	  *ce2=c2+1;

	  return STATE_DONE;
	}
      }

      else {
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
   struct piece *dst = *feed;
   ptrdiff_t cdst = st->c;
   newstate res;
   int flags = this->flags;
   enum contexts ctx = this->out_ctx;

   int scan_entity;

   recheck_scan(this,&scan_entity);

   for (;;)
   {
    switch (ctx) {

      case CTX_DATA:
      DEBUG((stderr,"%*d do_try_feed scan in data "
	     "scan_entity=%d ignore_data=%d\n",
	     this->stack_count,this->stack_count,
	     scan_entity,st->ignore_data));

#ifdef PIKE_DEBUG
      if (*feed && feed[0]->s->len < st->c)
	 fatal("len (%ld) < st->c (%ld)\n",
	       TO_LONG(feed[0]->s->len), TO_LONG(st->c));
      if (*feed && cmp_feed_pos (*feed, st->c, dst, cdst) > 0)
	fatal ("Going backwards from %p:%ld to %p:%ld.\n",
	       (void *)(*feed), TO_LONG(st->c), (void *)dst, TO_LONG(cdst));
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
	   DEBUG((stderr,"%*d do_try_feed scan for data %p:%d\n",
		  this->stack_count,this->stack_count,
		  dst,cdst));
	    n=0;
	    if (scan_entity) look_for[n++]=this->entity_start;
	    if (st->parse_tags) look_for[n++]=this->tag_start;
	    scan_forward(dst,cdst,&dst,&cdst,look_for,n);
	    DEBUG((stderr,"%*d do_try_feed scan for data %p:%d\n",
		   this->stack_count,this->stack_count,
		   dst,cdst));
	 }
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

	       this->type = TYPE_DATA;
	       do_callback(this,thisobj,
			   &(this->callback__data),
			   *feed,st->c,dst,cdst);

	       if ((res=handle_result(this,st,
				      feed,&(st->c),dst,cdst)))
	       {
		  DEBUG((stderr,"%*d do_try_feed return %d %p:%d\n",
			 this->stack_count,this->stack_count,
			 res,*feed,st->c));
		  st->ignore_data=(res==STATE_WAIT);
		  return res;
	       }
	       recheck_scan(this,&scan_entity);
	    }
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

      if (!*feed || cdst == dst->s->len)
      {
	 DEBUG((stderr,"%*d do_try_feed end\n",
		this->stack_count,this->stack_count));
	 return STATE_DONE; /* done */
      }

#ifdef PIKE_DEBUG
      if (*feed != dst || st->c != cdst)
	fatal ("Internal position confusion: feed: %p:%ld, dst: %p:%ld.\n",
	       (void *)(*feed), TO_LONG(st->c), (void *)dst, TO_LONG(cdst));
#endif

      ch=index_shared_string(dst->s,cdst);
      if (ch==this->tag_start)	/* tag */
      {
	 struct piece *tagend = NULL;
	 ptrdiff_t ctagend;

	 DEBUG((stderr,"%*d do_try_feed scan tag %p:%d\n",
		this->stack_count,this->stack_count,
		*feed,st->c));
#ifdef PIKE_DEBUG
	 if (!st->parse_tags)
	   fatal ("What am I doing parsing tags now?\n");
#endif

	 /* skip ws to start of tag name */
	 if (flags & FLAG_WS_BEFORE_TAG_NAME) {
	   if (!scan_forward(*feed,st->c+1,&dst,&cdst,
			     this->ws,-this->n_ws) && !finished)
	   {
	     st->ignore_data=1;
	     return STATE_WAIT; /* come again */
	   }
	 }
	 else {
	   FORWARD_CHAR (*feed, st->c, dst, cdst);
	   if (cdst >= dst->s->len && !finished) return STATE_WAIT;
	 }

	 if (m_sizeof(this->mapqtag)) {
	   struct piece *e1,*e2; /* <![CDATA[ ... ]]>  */
	   ptrdiff_t ce1,ce2;    /*       e1 ^    ^ e2 */
	   struct svalue *entry;

	   if (!quote_tag_lookup(this,dst,cdst,&e1,&ce1,finished,&entry)) {
	     st->ignore_data=1;
	     return STATE_WAIT; /* come again */
	   }

	   if (entry) {		/* quote tag */
	     if (scan_for_string(this,e1,ce1,&e2,&ce2,entry[2].u.string))
	       n_pos_forward(e2,ce2+entry[2].u.string->len,&dst,&cdst);
	     else {
	       if (!finished) {
		 st->ignore_data=1;
		 return STATE_WAIT; /* come again */
	       }
	       dst = e2;
	       cdst = ce2;
	     }

	     push_svalue(entry+1);
	     if ((res=quote_tag_callback(this,thisobj,sp-1,
					 e1,ce1,e2,ce2,
					 st,feed,&(st->c),dst,cdst)))
	     {
	       DEBUG((stderr,"%*d quote tag callback return %d %p:%d\n",
		      this->stack_count,this->stack_count,
		      res,*feed,st->c));
	       st->ignore_data=(res==STATE_WAIT);
	       pop_stack();
	       return res;
	     }

	     DEBUG((stderr,"%*d quote tag callback done %p:%d\n",
		    this->stack_count,this->stack_count,
		    *feed,st->c));

	     pop_stack();
	     recheck_scan(this,&scan_entity);
	     goto done;
	   }
	 }

	 /* scan tag name as argument and push */
	 if (!scan_forward_arg(this,dst,cdst,&dst,&cdst,SCAN_ARG_PUSH,finished,NULL))
	 {
	   st->ignore_data=1;
	   return STATE_WAIT; /* come again */
	 }
	 
	 if (m_sizeof(this->maptag) ||
	     m_sizeof(this->mapcont))
	 {
	    int empty_tag;
	    struct svalue *tag, *cont;
	    struct piece *e1,*e2; /* <tag> ... </tag>     */
	    ptrdiff_t ce1,ce2;    /*        e1 ^     ^ e2 */

	    if (!scan_for_end_of_tag(this,dst,cdst,&tagend,&ctagend,finished,
				     flags & FLAG_MATCH_TAG, &empty_tag))
	    {
	      st->ignore_data=1;
	      pop_stack();
	      return STATE_WAIT; /* come again */
	    }

	    if (flags & FLAG_CASE_INSENSITIVE_TAG)
	      f_lower_case(1);

	    tag = NULL, cont = NULL;
	    if (flags & FLAG_XML_TAGS) { /* decide what to do from tag syntax */
	      if (empty_tag) {	/* <foo/> */
		tag=low_mapping_lookup(this->maptag,sp-1);
		if (!tag) cont=low_mapping_lookup(this->mapcont,sp-1);
	      }
	      else {		/* <foo> */
		cont=low_mapping_lookup(this->mapcont,sp-1);
		if (!cont) {
		  tag=low_mapping_lookup(this->maptag,sp-1);
		  if (!tag || !(flags & FLAG_STRICT_TAGS)) empty_tag = 1;
		}
	      }
	    }
	    else {		/* decide what to do from registered callbacks */
	      tag=low_mapping_lookup(this->maptag,sp-1);
	      if (!tag && (cont=low_mapping_lookup(this->mapcont,sp-1)))
		empty_tag = 0;
	      else empty_tag = 1;
	    }

	    if (empty_tag) {	/* no content */
	      e1 = e2 = tagend;
	      ce1 = ce2 = ctagend + 1;
	      pop_stack();
	    }
	    else {
	       /* this is the hardest part : find the corresponding end tag */
#ifdef PIKE_DEBUG
	       if (!tag && !cont) fatal ("You push that stone yourself!\n");
#endif

	       if ((res=find_end_of_container(this,
					      sp-1,
					      tagend,ctagend+1,
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
	    }

	    if (tag) /* tag callback */
	    {
	       /* low-level tag call */
	       if ((res=tag_callback(this,thisobj,tag,
				     st,feed,&(st->c),dst=e2,cdst=ce2)))
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

	       recheck_scan(this,&scan_entity);
	       goto done;
	    }

	    else if (cont) /* container callback */
	    {
	       /* low-level container call */
	       if ((res=container_callback(this,thisobj,cont,
					   tagend,ctagend+1,e1,ce1,
					   st,feed,&(st->c),dst=e2,cdst=ce2)))
	       {
		  DEBUG((stderr,"%*d container callback return %d %p:%d\n",
			 this->stack_count,this->stack_count,
			 res,*feed,st->c));
		  st->ignore_data=(res==STATE_WAIT);
		  return res;
	       }

	       recheck_scan(this,&scan_entity);
	       goto done;
	    }
	 }
	 else pop_stack();

	 dmalloc_touch_svalue(&(this->callback__tag));

	 if (this->callback__tag.type!=T_INT && !ignore_tag_cb)
	 {
	    if (!tagend &&
		!scan_for_end_of_tag(this,dst,cdst,&tagend,&ctagend,finished,
				     flags & FLAG_MATCH_TAG,NULL))
	    {
	       st->ignore_data=1;
	       return STATE_WAIT; /* come again */
	    }

	    DEBUG((stderr,"%*d calling _tag callback %p:%d..%p:%d\n",
		   this->stack_count,this->stack_count,
		   *feed,st->c+1,tagend,ctagend));

	    /* low-level tag call */
	    this->type = TYPE_TAG;
	    do_callback(this,thisobj,
			&(this->callback__tag),
			*feed,st->c,dst=tagend,cdst=ctagend+1);

	    if ((res=handle_result(this,st,feed,&(st->c),dst,cdst)))
	    {
	       DEBUG((stderr,"%*d do_try_feed return %d %p:%d\n",
		      this->stack_count,this->stack_count,
		      res,*feed,st->c));
	       st->ignore_data=(res==STATE_WAIT);
	       return res;
	    }
	    recheck_scan(this,&scan_entity);
	 }
	 else if (flags & FLAG_IGNORE_UNKNOWN) {
	   /* Send it to callback__data. */
	   dst=*feed;
	   cdst=st->c+1;
	   goto done;
	 }
	 else
	   /* Begin parsing inside the tag for entities in arguments. */
	   ctx = CTX_TAG;
      }
      else			/* entity */
      {
#ifdef PIKE_DEBUG
	if (ch!=this->entity_start)
	  fatal ("Oups! How did I end up here? There's no entity around.\n");
#endif
	goto parse_entity;	/* same code as in tag arguments */
      }
      break;

      case CTX_TAG: {
	struct piece *s1 = dst, *s2, *s3;
        ptrdiff_t c1 = cdst, c2, c3;
	int pushed = 0;

	/* NOTE: This somewhat duplicates tag_args(). */

	DEBUG((stderr,"%*d do_try_feed scan in tag\n",
	       this->stack_count,this->stack_count));

	/* Skip ws. */
	if (!scan_forward (s1, c1, &s2, &c2, this->ws, -this->n_ws))
	  return STATE_WAIT;

	while (1) {
	  if (s1 != s2 || c1 != c2) {
	    if (this->splice_arg && pushed) { /* Check for splice_arg name. */
	      if (pushed > 1) {
		f_add (pushed);
		pushed = 1;
	      }
	      if (sp[-1].u.string == this->splice_arg) {
		DEBUG_MARK_SPOT ("do_try_feed arg ent scan: at empty splice arg",
				 s1, c1);
		break;
	      }
	      else {
		pop_stack();
		pushed = 0;
	      }
	    }

	    dst = s2, cdst = c2; /* New arg name. */
	  }

	  /* Scan arg name. */
	  DEBUG_MARK_SPOT ("do_try_feed at arg name", s2, c2);
	  if (!scan_forward_arg (this, s2, c2, &s3, &c3,
				 this->splice_arg ? SCAN_ARG_PUSH : SCAN_ARG_ONLY,
				 finished, NULL)) {
	    pop_n_elems (pushed);
	    return STATE_WAIT;
	  }
	  if (this->splice_arg) pushed++;

	  /* Skip ws. */
	  scan_forward (s3, c3, &s1, &c1, this->ws, -this->n_ws);
	  if (c1 == s1->s->len) {
	    pop_n_elems (pushed);
	    return STATE_WAIT;
	  }

	  DEBUG_MARK_SPOT ("do_try_feed after arg name", s1, c1);
	  ch = index_shared_string (s1->s, c1);

	  if (ch == this->arg_eq) {
	    c1++;
	    DEBUG_MARK_SPOT ("do_try_feed arg ent scan: before tag arg", s1, c1);
	    ctx = CTX_TAG_ARG;
	    break;
	  }

	  else if (ch == this->tag_fin && this->splice_arg &&
		   s3 == s1 && c3 == c1) {
	    DEBUG_MARK_SPOT ("do_try_feed arg ent scan: at tag fin", s1, c1);
	    /* Got to build the arg name. '/' may be in the middle of it. */
	    FORWARD_CHAR (s1, c1, s1, c1);
	    if ((flags & (FLAG_STRICT_TAGS|FLAG_XML_TAGS)) != FLAG_STRICT_TAGS) {
	      /* But then again, maybe it isn't.. */
	      if (c1 == s1->s->len) {
		pop_n_elems (pushed);
		return STATE_WAIT;
	      }
	      ch = index_shared_string (s1->s, c1);
	      if (ch == this->tag_end) {
		c1++;
		DEBUG_MARK_SPOT ("do_try_feed arg ent scan: at tag end", s1, c1);
		ctx = CTX_DATA;
		break;
	      }
	    }
	    push_string (make_shared_binary_string2 (&this->tag_fin, 1));
	    pushed++;
	  }

	  else if (ch == this->tag_end) {
	    c1++;
	    DEBUG_MARK_SPOT ("do_try_feed arg ent scan: at tag end", s1, c1);
	    ctx = CTX_DATA;
	    break;
	  }

	  else if (s2 == s3 && c2 == c3) {
	    /* scan_forward_arg() didn't skip -- bogus character. */
	    DEBUG_MARK_SPOT ("do_try_feed bogus char", s1, c1);
	    c1++;
	    dst = s1, cdst = c1; /* New arg name. */
	    pop_n_elems (pushed);
	    pushed = 0;
	  }

	  else if (s3 != s1 || c3 != c1) { /* Has already skipped ws. */
	    s2 = s1, c2 = c1;
	    s1 = s3, c1 = c3;
	    continue;
	  }

	  /* Skip ws. */
	  if (!scan_forward (s1, c1, &s2, &c2, this->ws, -this->n_ws)) {
	    pop_n_elems (pushed);
	    return STATE_WAIT;
	  }
	}

	if (pushed) {
	  if (pushed > 1) f_add (pushed);
	  if (sp[-1].u.string == this->splice_arg) { /* Got a splice arg. */
	    pop_stack();

	    if (ctx == CTX_TAG_ARG) { /* Got an arg value to splice. */
	      if (!scan_forward (s1, c1, &s1, &c1, this->ws, -this->n_ws) ||
		  !scan_forward_arg (this, s1, c1, &s2, &c2,
				     SCAN_ARG_PUSH, finished, NULL)) {
		DEBUG((stderr,"%*d do_try_feed wait in splice arg at %p:%d\n",
		       this->stack_count,this->stack_count,s1,c1));

		/* Put out collected data up to the splice arg name. */
		put_out_feed_range(this,*feed,st->c,dst,cdst);
		skip_feed_range(st,feed,&(st->c),dst,cdst);
		this->out_ctx = CTX_TAG_ARG;
		return STATE_WAIT;
	      }

	      DEBUG((stderr,"%*d do_try_feed got splice arg %p:%d..%p:%d\n",
		     this->stack_count,this->stack_count,s1,c1,s2,c2));

	      /* Put out collected data, but skip the splice arg itself. */
	      put_out_feed_range(this,*feed,st->c,dst,cdst);
	      skip_feed_range(st,feed,&(st->c),s2,c2);

	      /* Put the arg value in a nested feed and parse it in
	       * the CTX_SPLICE_ARG context. */
	      add_local_feed (this, sp[-1].u.string);
	      this->out_ctx = CTX_SPLICE_ARG;
	      return STATE_REREAD;
	    }

	    else {
	      /* Just a splice arg without value. Ignore it. */
	      put_out_feed_range(this,*feed,st->c,dst,cdst);
	      if (ctx == CTX_DATA)
		put_out_feed_range(this,s3,c3,s1,c1); /* Put trailing tag end. */
	      skip_feed_range(st,feed,&(st->c),dst=s1,cdst=c1);
	      this->out_ctx = ctx;
	      break;
	    }
	  }
	  else pop_stack();
	}

	put_out_feed_range(this,*feed,st->c,dst=s1,cdst=c1);
	skip_feed_range(st,feed,&(st->c),dst,cdst);
	this->out_ctx = ctx;
	break;
      }

      case CTX_SPLICE_ARG:
	if (!*feed) {
	  DEBUG((stderr,"%*d do_try_feed end in splice tag\n",
		 this->stack_count,this->stack_count));
	  this->out_ctx = CTX_TAG;
	  return STATE_DONE;
	}

	if (!scan_entity) {	/* Just put out everything. */
	  DEBUG((stderr,"%*d do_try_feed put out whole splice tag\n",
		 this->stack_count,this->stack_count));
	  while (dst->next) dst = dst->next;
	  put_out_feed_range(this,*feed,st->c,dst,cdst=dst->s->len);
	  skip_feed_range(st,feed,&(st->c),dst,cdst);
	  this->out_ctx = CTX_TAG;
	  return STATE_DONE;
	}

	/* Search for entities. Let anything through otherwise. */

	DEBUG((stderr,"%*d do_try_feed entity scan in splice tag\n",
	       this->stack_count,this->stack_count));

	res = scan_forward (dst, cdst, &dst, &cdst, &this->entity_start, 1);
	put_out_feed_range(this,*feed,st->c,dst,cdst);
	skip_feed_range(st,feed,&(st->c),dst,cdst);

	if (!res) {
	  this->out_ctx = CTX_TAG;
	  return STATE_DONE;
	}
	goto parse_entity;

      default: {
	int quote;
	p_wchar2 end_found;

	if (!*feed) {
	  DEBUG((stderr,"%*d do_try_feed end in tag arg\n",
		 this->stack_count,this->stack_count));
	  this->out_ctx = ctx;
	  return STATE_DONE;
	}

	DEBUG((stderr,"%*d do_try_feed scan in tag arg\n",
	       this->stack_count,this->stack_count));

	/* Skip ws. */
	if (!scan_forward (dst, cdst, &dst, &cdst, this->ws, -this->n_ws))
	  return STATE_WAIT;

      continue_in_arg:
	quote = ctx - CTX_TAG_QUOTED_ARG;

	DEBUG_MARK_SPOT ("do_try_feed at arg val", dst, cdst);
	if (!(res = scan_forward_arg (this, dst, cdst, &dst, &cdst,
				      scan_entity ? SCAN_ARG_ENT_BREAK : SCAN_ARG_ONLY,
				      finished, &quote)))
	  return STATE_WAIT;

	if (res == 1) {		/* arg end */
	  ctx = CTX_TAG;
	  break;
	}

	/* entity in tag argument */
	DEBUG_MARK_SPOT ("do_try_feed arg ent scan: at entity start", dst, cdst);
	put_out_feed_range(this,*feed,st->c,dst,cdst);
	skip_feed_range(st,feed,&(st->c),dst,cdst);
	ctx = this->out_ctx = CTX_TAG_QUOTED_ARG + quote;
      }

        /* FALL THROUGH */
    parse_entity: {
	p_wchar2 end_found;

	DEBUG((stderr,"%*d do_try_feed scan entity %p:%d\n",
	       this->stack_count,this->stack_count,
	       *feed,st->c));

#ifdef PIKE_DEBUG
	if (!scan_entity) fatal ("Shouldn't parse entities now.\n");
	if (*feed != dst || st->c != cdst)
	  fatal ("Internal position confusion: feed: %p:%ld, dst: %p:%ld\n",
		 (void *)(*feed), TO_LONG(st->c), (void *)dst, TO_LONG(cdst));
#endif
	/* just search for end of entity */

	if (flags & FLAG_LAZY_ENTITY_END)
	  end_found =
	    scan_forward(dst,cdst+1,&dst,&cdst,
			 this->lazy_entity_ends,this->n_lazy_entity_ends) ?
	    index_shared_string(dst->s,cdst) : 0;
	else if (flags & FLAG_NESTLING_ENTITY_END)
	{
	   p_wchar2 look_for[2];
	   int level=1;

	   look_for[0]=this->entity_start;
	   look_for[1]=this->entity_end;

	   for (;;)
	   {
	      if (!scan_forward(dst,cdst+1,&dst,&cdst,look_for,2))
	      {
		 end_found=0;
		 break;
	      }
	      if (index_shared_string(dst->s,cdst) == THIS->entity_end)
	      {
		 if (!--level)
		 {
		    end_found=this->entity_end;
		    break;
		 }
	      }
	      else
		 level++;
	   }
	}
	else
	  end_found =
	    scan_forward(dst,cdst+1,&dst,&cdst,&this->entity_end,1) ?
	    this->entity_end : 0;

	if (end_found != this->entity_end) {
	  if (finished) {	/* got no entity end */
	    cdst=st->c+1;
	    goto done;
	  }
	  else {
	    st->ignore_data=1; /* no data first at next call */
	    return STATE_WAIT; /* no end, call again */
	  }
	}

	if (m_sizeof(this->mapentity))
	{
	  struct svalue *v;
	    
	  push_feed_range(*feed,st->c+1,dst,cdst);
	  cdst++;

	  v=low_mapping_lookup(this->mapentity,sp-1);
	  pop_stack();
	  if (v) /* entity we want, do a callback */
	  {
	    /* low-level entity call */
	    this->type = TYPE_ENTITY;
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

	    recheck_scan(this,&scan_entity);

	    switch (ctx) {
	      case CTX_DATA:
		goto done;
	      case CTX_SPLICE_ARG:
		break;
	      default:
		goto continue_in_arg; /* Shouldn't skip ws here. */
	    }
	  }
	}
	else cdst++;

	dmalloc_touch_svalue(&(this->callback__entity));

	if (this->callback__entity.type!=T_INT && !ignore_tag_cb)
	{
	  DEBUG((stderr,"%*d calling _entity callback %p:%d..%p:%d\n",
		 this->stack_count,this->stack_count,
		 *feed,st->c,dst,cdst));

	  /* low-level entity call */
	  this->type = TYPE_ENTITY;
	  do_callback(this,thisobj,
		      &(this->callback__entity),
		      *feed,st->c,dst,cdst);

	  if ((res=handle_result(this,st,feed,&(st->c),dst,cdst)))
	  {
	    DEBUG((stderr,"%*d do_try_feed return %d %p:%d\n",
		   this->stack_count,this->stack_count,
		   res,*feed,st->c));
	    st->ignore_data=(res==1);
	    return res;
	  }
	  recheck_scan(this,&scan_entity);

	  switch (ctx) {
	    case CTX_DATA:
	      goto done;
	    case CTX_SPLICE_ARG:
	      break;
	    default:
	      goto continue_in_arg; /* Shouldn't skip ws here. */
	  }
	}
	else if (flags & FLAG_IGNORE_UNKNOWN) {
	  dst=*feed;
	  cdst=st->c+1;
	}
	else if (*feed)
	{
	  put_out_feed_range(this,*feed,st->c,dst,cdst);
	  skip_feed_range(st,feed,&(st->c),dst,cdst);
	}
      }
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
      struct piece **feed;
      st=THIS->stack;
      feed = st->prev ? &st->local_feed : &THIS->feed;
      if (*feed)
	res = do_try_feed(THIS,THISOBJ,
			  st,
			  feed,
			  finished||(st->prev!=NULL),
			  ignore_tag_cb);
      else
	res = STATE_WAIT;	/* Got no data. Try waiting. */
      ignore_tag_cb = 0;
      switch (res)
      {
	 case STATE_WAIT: /* incomplete, call again */
	    st = THIS->stack;

	    if (!finished && st->prev == NULL)
	      return;

	    /* finished anyway, just output last bit */
	    feed = st->prev ? &st->local_feed : &THIS->feed;
	    if (*feed) {
	      struct piece *end;
	      DEBUG((stderr, "%*d try_feed finishing from STATE_WAIT\n",
		     THIS->stack_count, THIS->stack_count));
	      for (end = *feed; end->next; end = end->next) {}
	      put_out_feed_range (THIS, *feed, st->c, end, end->s->len);
	      skip_feed_range (st, feed, &(st->c), end, end->s->len);
	    }

	    /* FALL THROUGH */
	 case STATE_DONE: /* done, pop stack */
	    if (!THIS->feed) THIS->feed_end=NULL;

	    st=THIS->stack->prev;
	    if (!st) {
	       if (finished) reset_stack_head (THIS);
	       return; /* all done, but keep last stack elem */
	    }

	    if (THIS->stack->local_feed && THIS->stack->free_feed)
	       fatal("internal wierdness in Parser.HTML: feed left\n");

	    really_free_feed_stack(THIS->stack);
	    THIS->stack=st;
	    THIS->stack_count--;
	    break;

	 case STATE_REPARSE: /* user requested another go at the current data */
	    if (st == THIS->stack) ignore_tag_cb = 1;
	    /* FALL THROUGH */

	 case STATE_REREAD: /* reread stack head */
	    if (THIS->stack_count>THIS->max_stack_depth)
	       Pike_error("Parser.HTML: too deep recursion\n");
	    break;
      }
   }
}

/****** feed ****************************************/


static void low_feed(struct pike_string *ps)
{
   struct piece *f;

   if (!ps->len) return;

   f = alloc_piece();
   copy_shared_string(f->s,ps);

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
**! method Parser.HTML feed()
**! method Parser.HTML feed(string s)
**! method Parser.HTML feed(string s,int do_parse)
**!	Feed new data to the <ref>Parser.HTML</ref>
**!	object. This will start a scan and may result in
**!	callbacks. Note that it's possible that all 
**!	data feeded isn't processed - to do that, call
**!	<ref>finish</ref>().
**!
**!	<p>If the function is called without arguments,
**!	no data is feeded, but the parser is run.
**!
**!	<p>If the string argument is followed by a 0,
**!	<tt>-&gt;feed(s,0);</tt>, the string is feeded,
**!	but the parser isn't run.
**!
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
**! method Parser.HTML feed_insert(string s)
**!	This pushes a string on the parser stack.
**!	(I'll write more about this mechanism later.)
**!
**! returns the called object
**! note: don't use
*/

static void html_feed_insert(INT32 args)
{
   if (!args)
      SIMPLE_TOO_FEW_ARGS_ERROR("feed_insert",1);

   if (sp[-args].type!=T_STRING)
      SIMPLE_BAD_ARG_ERROR("feed_insert",1,"string");

   DEBUG((stderr,"html_feed_insert: "
	  "pushing string (len=%d) on feedstack\n",
	  sp[-args].u.string->len));

   add_local_feed (THIS, sp[-args].u.string);

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
**! method Parser.HTML finish()
**! method Parser.HTML finish(string s)
**!	Finish a parser pass. A string may be sent here, similar to
**!	feed().
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
**! returns a string of parsed data if the parser isn't in
**!	<ref>mixed_mode</ref>, an array of arbitrary data otherwise.
*/

static void html_read(INT32 args)
{
   int n = 0x7fffffff;	/* a lot */
   int m=0; /* items on stack */

   if (args) {
      if (sp[-args].type==T_INT)
         n=sp[-args].u.integer;
      else
         Pike_error("read: illegal argument\n");
   }

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
	    if (got_arr) f_add(2);
	    else got_arr = 1;
	 }
	 THIS->out = THIS->out->next;
	 really_free_out_piece (z);
      }
      if (m)
      {
	 f_aggregate(m);
	 if (got_arr) f_add(2);
      }
      else
	if (!got_arr)
	  ref_push_array(&empty_array);
   }
   else
   {
      /* collect up to n characters */
      while (THIS->out && n)
      {
	 struct out_piece *z;

	 if (THIS->out->v.type != T_STRING)
	    Pike_error("Parser.HTML: Got nonstring in parsed data\n");

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
	 m++;
	 if (m==32)
	 {
	    f_add(32);
	    m=1;
	 }
	 z=THIS->out;
	 THIS->out=THIS->out->next;
	 really_free_out_piece (z);
      }

      if (!m)
	 ref_push_string(empty_string);
      else
	 f_add(m);
   }
}

/*
**! method Parser.HTML write_out(mixed...)
**!	Send data to the output stream, i.e. it won't be parsed and
**!	it won't be sent to the data callback, if any.
**!
**!	<p>Any data is allowed when the parser is running in
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
	 Pike_error("write_out: not a string argument\n");
      put_out_feed(THIS,sp-i,1);
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
   struct feed_stack *st = THIS->stack;
   pop_n_elems(args);
   while (st->prev) st = st->prev;
   push_int(st->pos.lineno);
   push_int(st->pos.byteno);
   push_int(st->pos.byteno-st->pos.linestart);
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
**! method mapping(string:mixed) tag_args()
**! method string tag_content()
**! method array tag(mixed default_value)
**! method string tag_args(mixed default_value)
**!     These give parsed information about the current thing being
**!     parsed, e.g. the current tag, container or entity. They return
**!     zero if they're not applicable.
**!
**!	<p><tt>tag_name</tt> gives the name of the current tag. If used
**!	from an entity callback, it gives the string inside the
**!	entity.
**!
**!	<p><tt>tag_args</tt> gives the arguments of the current tag,
**!	parsed to a convenient mapping consisting of key:value pairs.
**!	If the current thing isn't a tag, it gives zero. default_value
**!	is used for arguments which have no value in the tag. If
**!	default_value isn't given, the value is set to the same string
**!	as the key.
**!
**!	<p><tt>tag_content</tt> gives the content of the current tag, if
**!	it's a container or quote tag.
**!
**!	<p><tt>tag()</tt> gives the equivalent of
**!	<tt>({tag_name(),tag_args(), tag_content()})</tt>.
*/

static void tag_name(struct parser_html_storage *this,struct piece *feed,
		     ptrdiff_t c, int skip_tag_start)
{
   struct piece *s1=NULL,*s2=NULL;
   ptrdiff_t c1=0,c2=0;
   int pushed = 0;

   if (skip_tag_start) {
     p_wchar2 ch=index_shared_string(feed->s,c);
     if (c < feed->s->len && ch==this->tag_start)
       FORWARD_CHAR(feed,c,feed,c);
   }

   if (c < feed->s->len &&
       index_shared_string (feed->s, c) == this->tag_fin) {
     c++;
     push_string (make_shared_binary_string2 (&this->tag_fin, 1));
     pushed = 1;
   }

   /* scan ws to start of tag name */
   if (this->flags & FLAG_WS_BEFORE_TAG_NAME)
     scan_forward(feed,c,&s1,&c1,
		  this->ws,-this->n_ws);
   else
     s1 = feed, c1 = c;

   /* scan as argument and push*/
   scan_forward_arg(this,s1,c1,&s2,&c2,SCAN_ARG_PUSH,1,NULL);
   if (pushed) f_add (2);
}

static INLINE void tag_push_default_arg(struct svalue *def)
{
   if (def) push_svalue(def);
   else stack_dup();
}

static void tag_args(struct parser_html_storage *this,struct piece *feed,ptrdiff_t c,
		     struct svalue *def, int skip_name, int to_tag_end)
{
   struct piece *s1=NULL,*s2=NULL,*s3;
   int flags = this->flags;
   ptrdiff_t c1=0,c2=0,c3;
   int n=0;
#ifdef PIKE_DEBUG
   struct piece *prev_s = NULL;
   ptrdiff_t prev_c = 0;
#endif

   /* NOTE: This somewhat duplicates the CTX_TAG clause in
    * do_try_feed(). */

   p_wchar2 ch=index_shared_string(feed->s,c);
   if (ch==this->tag_start) c++;

   if (skip_name) {
     /* scan past tag name */
     if (flags & FLAG_WS_BEFORE_TAG_NAME)
       scan_forward(feed,c,&s1,&c1,
		    this->ws,-this->n_ws);
     else
       s1 = feed, c1 = c;
     scan_forward_arg(this,s1,c1,&s2,&c2,SCAN_ARG_ONLY,1,NULL);
   }
   else s2 = feed, c2 = c;

   for (;;)
   {
      /* skip whitespace */
      scan_forward(s2,c2,&s1,&c1,this->ws,-this->n_ws);

new_arg:
#ifdef PIKE_DEBUG
      if (prev_s && cmp_feed_pos (prev_s, prev_c, s1, c1) >= 0)
	fatal ("Not going forward in tag args loop (from %p:%ld to %p:%ld).\n",
	       (void *)prev_s, PTRDIFF_T_TO_LONG(prev_c),
	       (void *)s1, PTRDIFF_T_TO_LONG(c1));
      prev_s = s1, prev_c = c1;
#endif

      DEBUG_MARK_SPOT("html_tag_args arg start",s1,c1);

      /* end of tag? */
      if (c1==s1->s->len) { /* end<tm> */
	DEBUG_MARK_SPOT("html_tag_args hard tag end (1)",s1,c1);
	break;
      }
      ch=index_shared_string(s1->s,c1);
      if ((flags & (FLAG_STRICT_TAGS|FLAG_XML_TAGS)) != FLAG_STRICT_TAGS &&
	  ch==this->tag_fin) {
	FORWARD_CHAR(s1, c1, s3, c3);
	if ((c3==s3->s->len || index_shared_string(s3->s,c3)==this->tag_end) &&
	    to_tag_end) {
	  DEBUG_MARK_SPOT("html_tag_args empty element tag end",s3,c3);
	  break;
	}
	else if (n && s1==s2 && c1==c2) { /* previous arg value didn't really end */
	  DEBUG_MARK_SPOT("html_tag_args arg val continues",s3,c3);
	  push_string (make_shared_binary_string2 (&this->tag_fin, 1));
	  scan_forward_arg (this,s3,c3,&s2,&c2,SCAN_ARG_PUSH,1,NULL);
	  f_add (3);
	  continue;
	}
      }
      else if (ch==this->tag_end && to_tag_end) { /* end */
	DEBUG_MARK_SPOT("html_tag_args soft tag end (1)",s1,c1);
	break;
      }

      /* scan this argument name and push*/
      scan_forward_arg(this,s1,c1,&s2,&c2,SCAN_ARG_PUSH,1,NULL);
      if (flags & FLAG_CASE_INSENSITIVE_TAG)
	f_lower_case(1);
      n++;

      if (!(n & 127))
	custom_check_stack(256, "Out of stack after parsing %d tag arguments.\n", n);

      do {
	/* scan for '=', '>' or next argument */
	/* skip whitespace */
	scan_forward(s2,c2,&s3,&c3,this->ws,-this->n_ws);
	if (c3==s3->s->len) /* end<tm> */
	{
	  DEBUG_MARK_SPOT("html_tag_args hard tag end (2)",s3,c3);
	  tag_push_default_arg(def);
	  goto done;
	}

	ch=index_shared_string(s3->s,c3);

	if (ch==this->tag_fin && s2==s3 && c2==c3) {
	  struct piece *s4;
	  ptrdiff_t c4;
	  /* a '/' that might have been part of the argument name */
	  FORWARD_CHAR (s3, c3, s4, c4);
	  ch = index_shared_string (s4->s,c4);
	  if (ch == this->tag_end && to_tag_end) break;
	  DEBUG_MARK_SPOT("html_tag_args arg name continues",s4,c4);
	  push_string (make_shared_binary_string2 (&this->tag_fin, 1));
	  scan_forward_arg (this,s4,c4,&s2,&c2,SCAN_ARG_PUSH,1,NULL);
	  if (flags & FLAG_CASE_INSENSITIVE_TAG)
	    f_lower_case(1);
	  f_add (3);
	}
	else break;
      } while (1);

      if (ch==this->tag_end && to_tag_end) /* end */
      {
	 DEBUG_MARK_SPOT("html_tag_args soft tag end (2)",s3,c3);
	 tag_push_default_arg(def);
	 break;
      }

      if (ch!=this->arg_eq) {
	if (c1!=c3 || s1!=s3)	/* end of _this_ argument */
	{
	  DEBUG_MARK_SPOT("html_tag_args empty arg end",s3,c3);
	  s1 = s3, c1 = c3;
	  tag_push_default_arg(def);
	  goto new_arg;
	}
	else {			/* a stray bogus character */
	  DEBUG_MARK_SPOT("html_tag_args bogus char",s3,c3);
	  s2 = s3, c2 = c3 + 1;
	  pop_stack();
	  n--;
	  continue;
	}
      }
      
      /* left: case of '=' */
      c3++; /* skip it */
      
      /* skip whitespace */
      scan_forward(s3,c3,&s2,&c2,this->ws,-this->n_ws);

      DEBUG_MARK_SPOT("html_tag_args value start",s2,c2);

      /* scan the argument value */
      scan_forward_arg(this,s2,c2,&s1,&c1,SCAN_ARG_PUSH,1,NULL);

      DEBUG_MARK_SPOT("html_tag_args value end",s1,c1);

      /* next argument in the loop */
      s2 = s1;
      c2 = c1;
   }

done:
   f_aggregate_mapping(n*2);
}

static void html_tag_name(INT32 args)
{
   /* get rid of arguments */
   pop_n_elems(args);

   if (!THIS->start) Pike_error ("Parser.HTML: There's no current range.\n");
   switch (THIS->type) {
     case TYPE_TAG:
     case TYPE_CONT:
       tag_name(THIS,THIS->start,THIS->cstart,1);
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
	 ptrdiff_t end = THIS->cend;
	 if (index_shared_string(THIS->end->s,end-1) == THIS->entity_end) end--;
	 push_feed_range(THIS->start,THIS->cstart+1,THIS->end,end);
       }
       break;
     case TYPE_QTAG: {
       struct svalue *v;
       struct piece *beg;
       ptrdiff_t cbeg;
       if (THIS->flags & FLAG_WS_BEFORE_TAG_NAME)
	 scan_forward (THIS->start, THIS->cstart+1, &beg, &cbeg, THIS->ws, -THIS->n_ws);
       else
	 beg = THIS->start, cbeg = THIS->cstart + 1;
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

   if (!THIS->start) Pike_error ("Parser.HTML: There's no current range.\n");

   switch (THIS->type) {
     case TYPE_TAG:
     case TYPE_CONT:
       if (args)
       {
	 tag_args(THIS,THIS->start,THIS->cstart,&def,1,1);
	 free_svalue(&def);
       }
       else tag_args(THIS,THIS->start,THIS->cstart,NULL,1,1);
       break;
     default:
       push_int(0);
   }
}

static void html_tag_content(INT32 args)
{
  struct piece *beg = THIS->start;
  ptrdiff_t cbeg = THIS->cstart+1;

  pop_n_elems(args);

  if (!THIS->start) Pike_error ("Parser.HTML: There's no current range.\n");

  if (THIS->flags & FLAG_WS_BEFORE_TAG_NAME &&
      !scan_forward (beg, cbeg, &beg, &cbeg, THIS->ws, -THIS->n_ws)) {
    push_int(0);
    return;
  }

  switch (THIS->type) {
    case TYPE_CONT: {
      struct piece *end, *dummy;
      ptrdiff_t cend, cdummy;
      if (scan_forward_arg (THIS, beg, cbeg, &beg, &cbeg, SCAN_ARG_PUSH, 1, NULL)) {
	if (scan_for_end_of_tag (THIS, beg, cbeg, &beg, &cbeg, 1,
				 THIS->flags & FLAG_MATCH_TAG, NULL) &&
	    !find_end_of_container (THIS, sp-1, beg, cbeg+1,
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
      ptrdiff_t cend;
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

/*
**! method string context()
**!	Returns the current output context as a string:
**!	<ul>
**!
**!	<p><li><b>"data"</b>: In top level data. This is always returned
**!	when called from tag or container callbacks.
**!
**!	<p><li><b>"arg"</b>: In an unquoted argument.
**!
**!	<p><li><b>A single character string</b>: In a quoted argument.
**!	The string contains the starting quote character.
**!
**!	<p><li><b>"splice_arg"</b>: In a splice argument.
**!
**!	</ul>
**!
**!	<p>This function is typically only useful in entity callbacks,
**!	which can be called both from text and argument values of
**!	different sorts.
**!
**! see also: splice_arg
*/

static void html_context(INT32 args)
{
  pop_n_elems (args);
  switch (THIS->out_ctx) {
    case CTX_DATA: push_constant_text ("data"); break;
    case CTX_TAG: push_constant_text ("tag"); break; /* should never happen */
    case CTX_SPLICE_ARG: push_constant_text ("splice_arg"); break;
    case CTX_TAG_ARG: push_constant_text ("arg"); break;
    default:
      push_string (make_shared_binary_string2 (
		     THIS->argq_start + (THIS->out_ctx - CTX_TAG_QUOTED_ARG), 1));
  }
}

/*
**! method string parse_tag_name(string tag)
**!	Parses the tag name from a tag string without the surrounding
**!	brackets, i.e. a string on the form "<tt>tagname some="tag"
**!	args</tt>".
**! returns the tag name or an empty string if none
*/

static void html_parse_tag_name(INT32 args)
{
   struct piece feed;
   check_all_args("parse_tag_name",args,BIT_STRING,0);
   feed.s=sp[-args].u.string;
   feed.next=NULL;
   tag_name(THIS,&feed,0,0);
   stack_pop_n_elems_keep_top(args);
}

/*
**! method mapping parse_tag_args(string tag)
**!	Parses the tag arguments from a tag string without the name
**!	and surrounding brackets, i.e. a string on the form
**!	"<tt>some="tag" args</tt>".
**! returns a mapping containing the tag arguments
**! see also: tag_args
*/

static void html_parse_tag_args(INT32 args)
{
   struct piece feed;
   check_all_args("parse_tag_args",args,BIT_STRING,0);
   feed.s=sp[-args].u.string;
   feed.next=NULL;
   tag_args(THIS,&feed,0,NULL,0,0);
   stack_pop_n_elems_keep_top(args);
}

/** debug *******************************************/

/*
**! method mapping _inspect()
**! 	This is a low-level way of debugging a parser.
**!	This gives a mapping of the internal state
**!	of the Parser.HTML object.
**!
**!	<p>The format and contents of this mapping may
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
      push_int(DO_NOT_WARN(st->c));
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

   push_text("cond_outfeed");
   p=0;
   of=THIS->cond_out;
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

   push_text("splice_arg");
   if (THIS->splice_arg)
     ref_push_string(THIS->splice_arg);
   else
     push_int(0);
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
**! method Parser.HTML clone(mixed ...)
**!	Clones the <ref>Parser.HTML</ref> object.
**!	A new object of the same class is created,
**!	filled with the parse setup from the 
**!	old object.
**!
**!	<p>This is the simpliest way of flushing a
**!	parse feed/output.
**!
**!	<p>The arguments to clone is sent to the
**!	new object, simplifying work for custom classes 
**!	that inherits <ref>Parser.HTML</ref>.
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
   if (THISOBJ->parent)
     push_object(o=parent_clone_object(THISOBJ->prog,THISOBJ->parent,
				       THISOBJ->parent_identifier,args));
   else
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

   if (p->splice_arg) free_string(p->splice_arg);
   if (THIS->splice_arg)
     add_ref(p->splice_arg=THIS->splice_arg);
   else
     p->splice_arg=NULL;

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
   p->max_stack_depth=THIS->max_stack_depth;
   p->stack->parse_tags = THIS->flags & FLAG_PARSE_TAGS;

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
**! method Parser.HTML set_extra(mixed ...args)
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
**! method string splice_arg(void|string name)
**!	If given a string, it sets the splice argument name to it. It
**!	returns the old splice argument name.
**!
**!	<p>If a splice argument name is set, it's parsed in all tags,
**!	both those with callbacks and those without. Wherever it
**!	occurs, its value (after being parsed for entities in the
**!	normal way) is inserted directly into the tag. E.g:
**!	<pre>
**!	&lt;foo arg1="val 1" splice="arg2='val 2' arg3" arg4&gt;
**!	</pre>
**!	becomes
**!	<pre>
**!	&lt;foo arg1="val 1" arg2='val 2' arg3 arg4&gt;
**!	</pre>
**!	if "splice" is set as the splice argument name.
*/

static void html_splice_arg (INT32 args)
{
  struct pike_string *old = THIS->splice_arg;
   check_all_args("splice_arg",args,BIT_VOID|BIT_STRING|BIT_INT,0);
   if (args) {
     if (sp[-args].type == T_STRING)
       add_ref (THIS->splice_arg = sp[-args].u.string);
     else if (sp[-args].u.integer)
       SIMPLE_BAD_ARG_ERROR ("splice_arg", 1, "string or zero");
     else
       THIS->splice_arg = NULL;
   }
   pop_n_elems(args);
   if (old) push_string (old);
   else push_int (0);
}

/*
**! method int case_insensitive_tag(void|int value)
**! method int ignore_tags(void|int value)
**! method int ignore_unknown(void|int value)
**! method int lazy_argument_end(void|int value)
**! method int lazy_entity_end(void|int value)
**! method int match_tag(void|int value)
**! method int max_parse_depth(void|int value)
**! method int mixed_mode(void|int value)
**! method int reparse_strings(void|int value)
**! method int ws_before_tag_name(void|int value)
**! method int xml_tag_syntax(void|int value)
**!	Functions to query or set flags. These set the associated flag
**!	to the value if any is given and returns the old value.
**!
**!	<p>The flags are:
**!	<ul>
**!
**!	<p><li><b>case_insensitive_tag</b>: All tags and containers
**!	are matched case insensitively, and argument names are
**!	converted to lowercase. Tags added with
**!	<ref>add_quote_tag</ref>() are not affected, though. Switching
**!	to case insensitive mode and back won't preserve the case of
**!	registered tags and containers.
**!
**!	<p><li><b>ignore_tags</b>: Do not look for tags at all.
**!	Normally tags are matched even when there's no callbacks for
**!	them at all. When this is set, the tag delimiters '&lt;' and
**!	'&gt;' will be treated as any normal character.
**!
**!	<p><li><b>ignore_unknown</b>: Treat unknown tags and entities
**!	as text data, continuing parsing for tags and entities inside
**!	them.
**!
**!	<p><li><b>lazy_argument_end</b>: A '&gt;' in a tag argument
**!	closes both the argument and the tag, even if the argument is
**!	quoted.
**!
**!	<p><li><b>lazy_entity_end</b>: Normally, the parser search
**!	indefinitely for the entity end character (i.e. ';'). When
**!	this flag is set, the characters '&', '&lt;', '&gt;', '"',
**!	''', and any whitespace breaks the search for the entity end,
**!	and the entity text is then ignored, i.e. treated as data.
**!
**!	<p><li><b>match_tag</b>: Unquoted nested tag starters and
**!	enders will be balanced when parsing tags. This is the
**!	default.
**!
**!	<p><li><b>max_stack_depth</b>: Maximum recursion depth during
**!	parsing. Recursion occurs when a tag/container/entity/quote
**!	tag callback function returns a string to be reparsed. The
**!	default value is 10.
**!
**!	<p><li><b>mixed_mode</b>: Allow callbacks to return arbitrary
**!	data in the arrays, which will be concatenated in the output.
**!
**!	<p><li><b>reparse_strings</b>: When a plain string is used as
**!	a tag/container/entity/quote tag callback, it's not reparsed
**!	if this flag is unset. Setting it causes all such strings to
**!	be reparsed.
**!
**!	<p><li><b>ws_before_tag_name</b>: Allow whitespace between the
**!	tag start character and the tag name.
**!
**!	<p><li><b>xml_tag_syntax</b>: Whether or not to use XML syntax
**!	to tell empty tags and container tags apart:<br>
**!
**!	<b>0</b>: Use HTML syntax only. If there's a '/' last in a
**!	tag, it's just treated as any other argument.<br>
**!
**!	<b>1</b>: Use HTML syntax, but ignore a '/' if it comes
**!	last in a tag. This is the default.<br>
**!
**!	<b>2</b>: Use XML syntax, but when a tag that does not end
**!	with '/&gt;' is found which only got a non-container tag
**!	callback, treat it as a non-container (i.e. don't start to
**!	seek for the container end).<br>
**!
**!	<b>3</b>: Use XML syntax only. If a tag got both container
**!	and non-container callbacks, the non-container callback is
**!	called when the empty element form (i.e. the one ending with
**!	'/&gt;') is used, and the container callback otherwise. If
**!	only a container callback exists, it gets the empty string as
**!	content when there's none to be parsed. If only a
**!	non-container callback exists, it will be called (without the
**!	content argument) for both kinds of tags.
**!
**!     </ul>
**!
**! note:
**!	When functions are specified with
**!	<ref>_set_tag_callback</ref>() or
**!	<ref>_set_entity_callback</ref>(), all tags or entities,
**!	respectively, are considered known. However, if one of those
**!	functions return 1 and ignore_unknown is set, they are treated
**!	as text data instead of making another call to the same
**!	function again.
**!
*/

static void html_ignore_tags(INT32 args)
{
   int o=!(THIS->flags & FLAG_PARSE_TAGS);
   check_all_args("ignore_tags",args,BIT_VOID|BIT_INT,0);
   if (args) {
     if (sp[-args].u.integer) THIS->flags &= ~FLAG_PARSE_TAGS;
     else THIS->flags |= FLAG_PARSE_TAGS;
     THIS->stack->parse_tags = THIS->flags & FLAG_PARSE_TAGS;
   }
   pop_n_elems(args);
   push_int(o);
}

static void html_case_insensitive_tag(INT32 args)
{
   int o=!!(THIS->flags & FLAG_CASE_INSENSITIVE_TAG);
   check_all_args("case_insensitive_tag",args,BIT_VOID|BIT_INT,0);
   if (args) {
     if (sp[-args].u.integer) THIS->flags |= FLAG_CASE_INSENSITIVE_TAG;
     else THIS->flags &= ~FLAG_CASE_INSENSITIVE_TAG;
   }
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

static void html_lazy_argument_end(INT32 args)
{
   int o=!!(THIS->flags & FLAG_LAZY_END_ARG_QUOTE);
   check_all_args("lazy_argument_end",args,BIT_VOID|BIT_INT,0);
   if (args) {
     if (sp[-args].u.integer) THIS->flags |= FLAG_LAZY_END_ARG_QUOTE;
     else THIS->flags &= ~FLAG_LAZY_END_ARG_QUOTE;
     recalculate_argq(THIS);
   }
   pop_n_elems(args);
   push_int(o);
}

static void html_lazy_entity_end(INT32 args)
{
   int o=!!(THIS->flags & FLAG_LAZY_ENTITY_END);
   check_all_args("lazy_entity_end",args,BIT_VOID|BIT_INT,0);
   if (args) {
     if (sp[-args].u.integer) THIS->flags |= FLAG_LAZY_ENTITY_END;
     else THIS->flags &= ~FLAG_LAZY_ENTITY_END;
   }
   pop_n_elems(args);
   push_int(o);
}

static void html_nestling_entity_end(INT32 args)
{
   int o=!!(THIS->flags & FLAG_NESTLING_ENTITY_END);
   check_all_args("nestling_entity_end",args,BIT_VOID|BIT_INT,0);
   if (args) {
     if (sp[-args].u.integer) THIS->flags |= FLAG_NESTLING_ENTITY_END;
     else THIS->flags &= ~FLAG_NESTLING_ENTITY_END;
   }
   pop_n_elems(args);
   push_int(o);
}

static void html_match_tag(INT32 args)
{
   int o=!!(THIS->flags & FLAG_MATCH_TAG);
   check_all_args("match_tag",args,BIT_VOID|BIT_INT,0);
   if (args) {
     if (sp[-args].u.integer) THIS->flags |= FLAG_MATCH_TAG;
     else THIS->flags &= ~FLAG_MATCH_TAG;
   }
   pop_n_elems(args);
   push_int(o);
}

static void html_mixed_mode(INT32 args)
{
   int o=!!(THIS->flags & FLAG_MIXED_MODE);
   check_all_args("mixed_mode",args,BIT_VOID|BIT_INT,0);
   if (args) {
     if (sp[-args].u.integer) THIS->flags |= FLAG_MIXED_MODE;
     else THIS->flags &= ~FLAG_MIXED_MODE;
   }
   pop_n_elems(args);
   push_int(o);
}

static void html_reparse_strings(INT32 args)
{
   int o=!!(THIS->flags & FLAG_REPARSE_STRINGS);
   check_all_args("reparse_strings",args,BIT_VOID|BIT_INT,0);
   if (args) {
     if (sp[-args].u.integer) THIS->flags |= FLAG_REPARSE_STRINGS;
     else THIS->flags &= ~FLAG_REPARSE_STRINGS;
   }
   pop_n_elems(args);
   push_int(o);
}

static void html_ignore_unknown(INT32 args)
{
   int o=!!(THIS->flags & FLAG_IGNORE_UNKNOWN);
   check_all_args("ignore_unknown",args,BIT_VOID|BIT_INT,0);
   if (args) {
     if (sp[-args].u.integer) THIS->flags |= FLAG_IGNORE_UNKNOWN;
     else THIS->flags &= ~FLAG_IGNORE_UNKNOWN;
   }
   pop_n_elems(args);
   push_int(o);
}

static void html_xml_tag_syntax(INT32 args)
{
   int o=THIS->flags;
   check_all_args("xml_tag_syntax",args,BIT_VOID|BIT_INT,0);
   if (args) {
     THIS->flags &= ~(FLAG_XML_TAGS|FLAG_STRICT_TAGS);
     switch (sp[-args].u.integer) {
       case 0: THIS->flags |= FLAG_STRICT_TAGS; break;
       case 1: break;
       case 2: THIS->flags |= FLAG_XML_TAGS; break;
       case 3: THIS->flags |= FLAG_XML_TAGS|FLAG_STRICT_TAGS; break;
       default:
	 SIMPLE_BAD_ARG_ERROR ("xml_tag_syntax", 1, "integer 0..3");
     }
     recalculate_argq(THIS);
   }
   pop_n_elems(args);
   if (o & FLAG_XML_TAGS)
     o = o & FLAG_STRICT_TAGS ? 3 : 2;
   else
     o = o & FLAG_STRICT_TAGS ? 0 : 1;
   push_int(o);
}

static void html_ws_before_tag_name(INT32 args)
{
   int o=!!(THIS->flags & FLAG_WS_BEFORE_TAG_NAME);
   check_all_args("ws_before_tag_name",args,BIT_VOID|BIT_INT,0);
   if (args) {
     if (sp[-args].u.integer) THIS->flags |= FLAG_WS_BEFORE_TAG_NAME;
     else THIS->flags &= ~FLAG_WS_BEFORE_TAG_NAME;
   }
   pop_n_elems(args);
   push_int(o);
}

static void html_max_stack_depth(INT32 args)
{
   int o=THIS->max_stack_depth;
   check_all_args("max_stack_depth",args,BIT_VOID|BIT_INT,0);
   if (args) {
     THIS->max_stack_depth = sp[-args].u.integer;
   } 
   pop_n_elems(args);
   push_int(o);
}

#ifdef HTML_DEBUG
static void html_debug_mode(INT32 args)
{
   int o=!!(THIS->flags & FLAG_DEBUG_MODE);
   check_all_args("debug_mode",args,BIT_VOID|BIT_INT,0);
   if (args) {
     if (sp[-args].u.integer) THIS->flags |= FLAG_DEBUG_MODE;
     else THIS->flags &= ~FLAG_DEBUG_MODE;
   }
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
   size_t offset;

   empty_string = make_shared_binary_string("", 0);

   offset = ADD_STORAGE(struct parser_html_storage);

   PIKE_MAP_VARIABLE(" maptag", offset + OFFSETOF(parser_html_storage, maptag),
		tMap(tStr,tTodo(tTagargs)),
		T_MAPPING, ID_STATIC|ID_PRIVATE);
   PIKE_MAP_VARIABLE(" mapcont", offset + OFFSETOF(parser_html_storage, mapcont),
		tMap(tStr,tTodo(tTagargs tStr)),
		T_MAPPING, ID_STATIC|ID_PRIVATE);
   PIKE_MAP_VARIABLE(" mapentity", offset + OFFSETOF(parser_html_storage, mapentity),
		tMap(tStr,tTodo(tNone)),
		T_MAPPING, ID_STATIC|ID_PRIVATE);
   PIKE_MAP_VARIABLE(" mapqtag", offset + OFFSETOF(parser_html_storage, mapqtag),
		tMap(tStr,tTodo(tStr)),
		T_MAPPING, ID_STATIC|ID_PRIVATE);
   PIKE_MAP_VARIABLE(" callback__tag", offset + OFFSETOF(parser_html_storage, callback__tag),
		tFuncV(tObjImpl_PARSER_HTML tStr,tMix,tCbret),
		T_MIXED, ID_STATIC|ID_PRIVATE);
   PIKE_MAP_VARIABLE(" callback__data", offset + OFFSETOF(parser_html_storage, callback__data),
		tFuncV(tObjImpl_PARSER_HTML tStr,tMix,tCbret),
		T_MIXED, ID_STATIC|ID_PRIVATE);
   PIKE_MAP_VARIABLE(" callback__entity", offset + OFFSETOF(parser_html_storage, callback__entity),
		tFuncV(tObjImpl_PARSER_HTML tStr,tMix,tCbret),
		T_MIXED, ID_STATIC|ID_PRIVATE);
   PIKE_MAP_VARIABLE(" splice_arg", offset + OFFSETOF(parser_html_storage, splice_arg),
		tString,
		T_STRING, ID_STATIC|ID_PRIVATE);
   PIKE_MAP_VARIABLE(" extra_args", offset + OFFSETOF(parser_html_storage, extra_args),
		tArray,
		T_ARRAY, ID_STATIC|ID_PRIVATE);

   set_init_callback(init_html_struct);
   set_exit_callback(exit_html_struct);

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
   ADD_FUNCTION("context",html_context,
		tFunc(tNone,tStr),0);

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

   ADD_FUNCTION("clear_tags",html_clear_tags,
		tFunc(tNone,tObjImpl_PARSER_HTML),0);
   ADD_FUNCTION("clear_containers",html_clear_containers,
		tFunc(tNone,tObjImpl_PARSER_HTML),0);
   ADD_FUNCTION("clear_entities",html_clear_entities,
		tFunc(tNone,tObjImpl_PARSER_HTML),0);
   ADD_FUNCTION("clear_quote_tags",html_clear_quote_tags,
		tFunc(tNone,tObjImpl_PARSER_HTML),0);

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

   ADD_FUNCTION("splice_arg",html_splice_arg,
		tFunc(tOr(tVoid,tStr),tStr),0);

   ADD_FUNCTION("ignore_tags",html_ignore_tags,
		tFunc(tOr(tVoid,tInt),tInt),0);
   ADD_FUNCTION("max_stack_depth",html_max_stack_depth,
		tFunc(tOr(tVoid,tInt),tInt),0);
   ADD_FUNCTION("case_insensitive_tag",html_case_insensitive_tag,
		tFunc(tOr(tVoid,tInt),tInt),0);
   ADD_FUNCTION("lazy_argument_end",html_lazy_argument_end,
		tFunc(tOr(tVoid,tInt),tInt),0);
   ADD_FUNCTION("lazy_entity_end",html_lazy_entity_end,
		tFunc(tOr(tVoid,tInt),tInt),0);
   ADD_FUNCTION("nestling_entity_end",html_nestling_entity_end,
		tFunc(tOr(tVoid,tInt),tInt),0);
   ADD_FUNCTION("match_tag",html_match_tag,
		tFunc(tOr(tVoid,tInt),tInt),0);
   ADD_FUNCTION("mixed_mode",html_mixed_mode,
		tFunc(tOr(tVoid,tInt),tInt),0);
   ADD_FUNCTION("reparse_strings",html_reparse_strings,
		tFunc(tOr(tVoid,tInt),tInt),0);
   ADD_FUNCTION("ignore_unknown",html_ignore_unknown,
		tFunc(tOr(tVoid,tInt),tInt),0);
   ADD_FUNCTION("xml_tag_syntax",html_xml_tag_syntax,
		tFunc(tOr(tVoid,tInt),tInt03),0);
   ADD_FUNCTION("ws_before_tag_name",html_ws_before_tag_name,
		tFunc(tOr(tVoid,tInt),tInt),0);
#ifdef HTML_DEBUG
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
		tFunc(tStr,tMap(tStr,tStr)),0);
}

void exit_parser_html()
{
   free_string(empty_string);
   free_all_piece_blocks();
   free_all_out_piece_blocks();
   free_all_feed_stack_blocks();
}

/*

class Parse_HTML
{
   void feed(string something); // more data in stream
   void finish(); // stream ends here

   string read(void|int chars); // read out-feed

   Parser.HTML clone(); // new object, fresh stream

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
