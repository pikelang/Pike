#include "stralloc.h"
#include "global.h"
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "mapping.h"
#include "array.h"
#include "builtin_functions.h"
#include "error.h"
#include "module_support.h"
#include "multiset.h"
#include "operators.h"

#include "streamed_parser.h"

/* streamed SGML parser, by wing */

/* State machine for parsing

notag		<	tag_start
	notag

tag_start	/	tag_end
		WS	tag_start
		>	notag
	tag_name

tag_end		WS	skip		(pop-tag-stack)
		>	notag	(error)	(pop-tag-stack)
	tag_end

tag_end_name	WS	tag_end_name
		>	notag
	tag_end_name
	
skip		>	notag
		"	skip_fnutt_fnutt
		'	skip_fnutt
	skip

skip_fnutt_fnutt	"	skip
	skip_fnutt_fnutt

skip_fnutt	'	skip
	skip_fnut

tag_name	WS	skip (or) tag_ws
		>	notag (or) content
	tag_name

tag_ws		WS	tag_ws
		>	notag (check if something changed) (or) content
	tag_arg_name

tag_arg_name	WS	tag_post_arg_name
		=	tag_pre_arg_name
		>	notag (check if something changed) (or) content
	tag_arg_name

tag_post_arg_name	WS	tag_post_arg_name
			=	tag_pre_arg_value
			>	notag (check if something changed) (or) content
	tag_arg_name

tag_pre_arg_value	WS	tag_pre_arg_value
			>	notag (error) (check if something changed)
				  (or) content
			"	tag_arg_value_fnutt_fnutt
			'	tag_arg_value_fnutt
	tag_arg_value

tag_arg_value_fnutt_fnutt	"	tag_ws
	tag_arg_value_fnutt_fnutt

tag_arg_value_fnutt	'	tag_ws
	tag_arg_value_fnutt

tag_arg_value	WS	tag_ws
		>	notag (check if something changed) (or) content
	tag_arg_value

content		<	content_tag_start
       content

content_tag_start	/	content_tag_end
	content_skip

content_tag_end		WS	content_tag_end
			>	(error) content
	content_tag_end_name

content_tag_end_name	WS	content_tag_end_name
			>	content (or) notag
	content_tag_end_name

content_skip	>	content
		"	content_skip_fnutt_fnutt
		'	content_skip_fnutt
	content_skip

content_skip_fnutt_fnutt	"	content_skip
	content_skip_fnutt_fnutt

content_skip_fnutt		'	content_skip
	content_skip_fnutt

*/

#define NOTAG				 0
#define TAG_START			 1
#define TAG_END				 2
#define TAG_END_NAME			 3
#define SKIP				 4
#define SKIP_FNUTT_FNUTT		 5
#define SKIP_FNUTT			 6
#define TAG_NAME			 7
#define TAG_WS				 8
#define TAG_ARG_NAME			 9
#define TAG_POST_ARG_NAME		10
#define TAG_PRE_ARG_VALUE		11
#define TAG_ARG_VALUE_FNUTT_FNUTT	12
#define TAG_ARG_VALUE_FNUTT		13
#define TAG_ARG_VALUE			14
#define CONTENT				15
#define CONTENT_TAG_START		16
#define CONTENT_TAG_END			17
#define CONTENT_TAG_END_NAME		18
#define CONTENT_SKIP			19
#define CONTENT_SKIP_FNUTT_FNUTT	20
#define CONTENT_SKIP_FNUTT		21

#define ARG_TYPE_NONE			 0
#define ARG_TYPE_IN			 1
#define ARG_TYPE_OUT			 2

#define WS ' ': case '\t': case '\n': case '\r'

#define DATA ((struct streamed_parser *)(fp->current_storage))

void streamed_parser_init(void)
{
  DATA->last_buffer = 0;
  DATA->last_buffer_size = 0;
  DATA->start_tags = 0;
  DATA->content_tags = 0;
  DATA->end_tags = 0;
}

void streamed_parser_destruct(void)
{
  if (DATA->last_buffer)
    free( DATA->last_buffer );
#if 0 /* Per, to clean up _some_ of the warnings */
  if (DATA->start_tags)
    ;
  if (DATA->end_tags)
    ;
#endif
}

void streamed_parser_set_data( INT32 args )
{
  get_all_args("spider.streamed_parser->set_data", args, "%m%m%m",
	       &(DATA->start_tags), &(DATA->content_tags), &(DATA->end_tags));
  DATA->start_tags->refs++;
  DATA->content_tags->refs++;
  DATA->end_tags->refs++;
  pop_n_elems(args);
}

#define SWAP \
	    *sp = sp[-2]; \
	    sp[-2] = sp[-1]; \
	    sp[-1] = *sp

static int handle_tag( struct svalue *data_arg )
{
  struct svalue *fun;

  push_svalue(data_arg);
  if(!(fun = low_mapping_lookup( DATA->start_tags, sp-3 )))
    error("Error in streamed parser.\n");

  apply_svalue(fun, 3);

  if (sp[-1].type == T_STRING)
  {
    return 1;
  } else {
    pop_stack();
    return 0;
  }
}

static int handle_content_tag( struct svalue *data_arg )
{
  struct svalue *fun;

  push_svalue(data_arg);
  if(!(fun = low_mapping_lookup( DATA->content_tags, sp-3 )))
    error("Error in streamed parser.\n");

  apply_svalue(fun, 3);
  if (sp[-1].type == T_STRING)
    return 1;
  else
  {
    pop_stack();
    return 0;
  }
}

static int handle_end_tag( struct svalue *data_arg )
{
  struct svalue *fun;

  fun = low_mapping_lookup( DATA->start_tags, sp-1);

  /* NB: fun would not be valid if the value popped here is an object,
   * fortunately it is not. */
  pop_stack();

  push_svalue(data_arg);
  if(fun)
  {
    apply_svalue(fun, 1);
    if (sp[-1].type == T_STRING)
      return 1;
    else
    {
      pop_stack();
      return 0;
    }
  }
  return 0;
}

static void add_arg(void)
{
  mapping_insert( sp[-3].u.mapping, sp-2, sp-1 );
  pop_stack();
  pop_stack();
}

void streamed_parser_parse( INT32 args )
{
  int c, length, state, begin, last, ind=0, ind2=0, ind3=0, ind4=0, ind5=0, content_tag=0;
  char *str;
  struct svalue *sp_save;
  struct svalue *sp_tag_save;
  struct svalue *data_arg;
  struct pike_string *to_parse;

  get_all_args("spider.streamed_parser->parse", args, "%S", &to_parse);
  
  state = NOTAG;
  begin = 0; 
  last = -1;
  SWAP;
  length = to_parse->len;
  if (!(str = alloca( DATA->last_buffer_size + length ))) {
    error("spider.streamed_parser->parse(): Out of memory\n");
  }
  if (DATA->last_buffer_size > 0)
  {
    MEMCPY( str, DATA->last_buffer, DATA->last_buffer_size );
    MEMCPY( str + DATA->last_buffer_size, to_parse->str, length );
    length += DATA->last_buffer_size;
    free( DATA->last_buffer );
    DATA->last_buffer = 0;
    DATA->last_buffer_size = 0;
  } else {
    MEMCPY(str, to_parse->str, length);
  }
  pop_stack();

  data_arg = sp-1;
  sp_save = sp;
  sp_tag_save = 0;
  for (c=0; c < length; c++)
    switch (state)
    {
     case NOTAG:
      switch (str[c])
      {
       case '<':
	state = TAG_START;
	break;
       default:
	last = c;
      }
      break;

     case TAG_START:
      switch (str[c])
      {
       case '/':
	state = TAG_END;
	break;
       case WS:
	state = TAG_START;
	break;
       case '>':
	last = c;
	state = NOTAG;
	break;
       default:
	ind = c;
	state = TAG_NAME;
	break;
      }
      break;

     case TAG_END:
      switch (str[c])
      {
       case WS:
	break;
       case '>': /* error */
	last = c;
	state = NOTAG;
	break;
       default:
	ind = c;
	ind2 = -1;
	state = TAG_END_NAME;
	break;
      }
      break;

     case TAG_END_NAME:
      switch (str[c])
      {
       case WS:
	if (ind2 == -1)
	  ind2 = c-1;
	break;
       case '>':
	if (ind2 == -1)
	  ind2 = c-1;
	push_string( make_shared_binary_string( str + ind, ind2 - ind ) );
	f_lower_case( 1 );
	if (low_mapping_lookup( DATA->end_tags, sp-1 ))
	{
	  if (handle_end_tag( data_arg ))
	  {
	    if (last >= begin)
	    {
	      push_string( make_shared_binary_string( str + begin, last - begin + 1 ) );
	      SWAP;
	    }
	    begin = c+1;
	  }
	}
	else
	  pop_stack();
	last = c;
	state = NOTAG;
	break;
       default:
	break;
      }
      break;

     case SKIP:
      switch (str[c])
      {
       case '>':
	last = c;
	state = NOTAG;
	break;
       case '"':
	state = SKIP_FNUTT_FNUTT;
	break;
       case '\'':
	state = SKIP_FNUTT;
	break;
      }
      break;

     case SKIP_FNUTT_FNUTT:
      switch (str[c])
      {
       case '"':
	state = SKIP;
	break;
      }
      break;

     case SKIP_FNUTT:
      switch (str[c])
      {
       case '\'':
	state = SKIP;
	break;
      }
      break;

     case TAG_NAME:
      switch (str[c])
      {
       case WS:
	push_string( make_shared_binary_string( str + ind, c - ind ) );
	f_lower_case( 1 );
	if (low_mapping_lookup( DATA->start_tags, sp-1 ))
	{
	  f_aggregate_mapping( 0 );
	  state = TAG_WS;
	  sp_tag_save = sp-1;
	  content_tag = 0;
	}
	else if (low_mapping_lookup( DATA->content_tags, sp-1 ))
	{
	  f_aggregate_mapping( 0 );
	  state = TAG_WS;
	  sp_tag_save = sp-1;
	  content_tag = 1;
	}
	else
	{
	  pop_stack();
	  state = SKIP;
	}
	break;
       case '>':
	push_string( make_shared_binary_string( str + ind, c - ind ) );
	f_lower_case( 1 );
	if (low_mapping_lookup( DATA->start_tags, sp-1 ))
	{
	  f_aggregate_mapping( 0 );
	  if (handle_tag( data_arg ))
	  {
	    if (last >= begin)
	    {
	      push_string( make_shared_binary_string( str + begin, last - begin + 1 ) );
	      SWAP;
	    }
	    begin = c+1;
	  }
#if 0 /* DITTO */
	  else
	    ;
#endif
	}
	else if (low_mapping_lookup( DATA->content_tags, sp-1 ))
	{
	  f_aggregate_mapping( 0 );
	  ind2 = c+1;
	  state = CONTENT;
	  break;
	}
	else
	  pop_stack();
	last = c;
	sp_tag_save = 0;
	state = NOTAG;
	break;
      }
      break;

     case TAG_WS:
      switch (str[c])
      {
       case WS:
	break;
       case '>':
	if (content_tag)
	{
	  ind2 = c+1;
	  state = CONTENT;
	  break;
	}
	if (handle_tag( data_arg ))
	{
	  if (last >= begin)
	  {
	    push_string( make_shared_binary_string( str + begin, last - begin + 1 ) );
	    SWAP;
	  }
	  begin = c+1;
	}
#if 0 /* DITTO */
	else
	  ;
#endif
	last = c;
	sp_tag_save = 0;
	state = NOTAG;
	break;
       default:
	ind = c;
	state = TAG_ARG_NAME;
	break;
      }
      break;

     case TAG_ARG_NAME:
      switch (str[c])
      {
       case WS:
	push_string( make_shared_binary_string( str + ind, c - ind ) );
	f_lower_case( 1 );
	state = TAG_POST_ARG_NAME;
	break;
       case '=':
	push_string( make_shared_binary_string( str + ind, c - ind ) );
	f_lower_case( 1 );
	state = TAG_PRE_ARG_VALUE;
	break;
       case '>':
	push_string( make_shared_binary_string( str + ind, c - ind ) );
	f_lower_case( 1 );
	push_text( "" );
	add_arg();
	if (content_tag)
	{
	  ind2 = c+1;
	  state = CONTENT;
	  break;
	}
	if (handle_tag( data_arg ))
	{
	  if (last >= begin)
	  {
	    push_string( make_shared_binary_string( str + begin, last - begin + 1 ) );
	    SWAP;
	  }
	  begin = c+1;
	}
#if 0 /* DITTO */
	else
	  ;
#endif
	last = c;
	sp_tag_save = 0;
	state = NOTAG;
	break;	
      }
      break;

     case TAG_POST_ARG_NAME:
      switch (str[c])
      {
       case WS:
	break;
       case '=':
	state = TAG_PRE_ARG_VALUE;
	break;
       case '>':
	push_text( "" );
	add_arg();
	if (content_tag)
	{
	  ind2 = c+1;
	  state = CONTENT;
	  break;
	}
	if (handle_tag( data_arg ))
	{
	  if (last >= begin)
	  {
	    push_string( make_shared_binary_string( str + begin, last - begin + 1 ) );
	    SWAP;
	  }
	  begin = c+1;
	}
#if 0 /* DITTO */
	else
	  ;
#endif
	last = c;
	sp_tag_save = 0;
	state = NOTAG;
	break;
       default:
	push_text( "" );
	add_arg();
	ind = c;
	state = TAG_ARG_NAME;
	break;
      }
      break;

     case TAG_PRE_ARG_VALUE:
      switch (str[c])
      {
       case WS:
	break;
       case '>':
	push_text( "" );
	add_arg();
	if (content_tag)
	{
	  ind2 = c+1;
	  state = CONTENT;
	  break;
	}
	if (handle_tag( data_arg ))
	{
	  if (last >= begin)
	  {
	    push_string( make_shared_binary_string( str + begin, last - begin + 1 ) );
	    SWAP;
	  }
	  begin = c+1;
	}
#if 0 /* DITTO */
	else
	  ;
#endif
	last = c;
	sp_tag_save = 0;
	state = NOTAG; /* error */
	break;
       case '"':
	state = TAG_ARG_VALUE_FNUTT_FNUTT;
	ind = c+1;
	break;
       case '\'':
	state = TAG_ARG_VALUE_FNUTT;
	ind = c+1;
	break;
       default:
	state = TAG_ARG_VALUE;
	ind = c;
	break;
      }
      break;

     case TAG_ARG_VALUE_FNUTT_FNUTT:
      switch (str[c])
      {
       case '"':
	push_string( make_shared_binary_string( str + ind, c - ind ) );
	add_arg();
	state = TAG_WS;
	break;
      }

     case TAG_ARG_VALUE_FNUTT:
      switch (str[c])
      {
       case '\'':
	push_string( make_shared_binary_string( str + ind, c - ind ) );
	add_arg();
	state = TAG_WS;
	break;
      }
      break;

     case TAG_ARG_VALUE:
      switch (str[c])
      {
       case WS:
	push_string( make_shared_binary_string( str + ind, c - ind ) );
	add_arg();
	state = TAG_WS;
	break;
       case '>':
	push_string( make_shared_binary_string( str + ind, c - ind ) );
	add_arg();
	state = TAG_WS;
	if (content_tag)
	{
	  ind2 = c+1;
	  state = CONTENT;
	  break;
	}
	if (handle_tag( data_arg ))
	{
	  if (last >= begin)
	  {
	    push_string( make_shared_binary_string( str + begin, last - begin + 1 ) );
	    SWAP;
	  }
	  begin = c+1;
	}
#if 0 /* DITTO ½ */
	else
	  ;
#endif
	last = c;
	sp_tag_save = 0;
	state = NOTAG;
	break;
      }
      break;

     case CONTENT:
      switch (str[c])
      {
       case '<':
	state = CONTENT_TAG_START;
	ind3 = c-1;
	break;
      }
      break;

     case CONTENT_TAG_START:
      switch (str[c])
      {
       case '/':
	state = CONTENT_TAG_END;
	break;
       default:
	state = CONTENT_SKIP;
	break;
      }
      break;
      
     case CONTENT_TAG_END:
      switch (str[c])
      {
       case WS:
	state = CONTENT_TAG_END;
	break;
       case '>': /* error */
	state = CONTENT;
	break;
       default:
	ind4 = c;
	ind5 = -1;
	state = CONTENT_TAG_END_NAME;
	break;
      }
      break;
      
     case CONTENT_TAG_END_NAME:
      switch (str[c])
      {
       case WS:
	if (ind5 == -1)
	  ind5 = c-1;
	break;
       case '>':
	if (ind5 == -1)
	  ind5 = c-1;
	push_string( make_shared_binary_string( str + ind4, ind5 - ind4 ) );
	f_lower_case( 1 );
	if (is_same_string( sp[-1].u.string, sp[-3].u.string ))
	{
	  pop_stack();
	  push_string( make_shared_binary_string( str + ind2, ind2 - ind3 ) );
	  if (handle_content_tag( data_arg ))
	  {
	    if (last >= begin)
	    {
	      push_string( make_shared_binary_string( str + begin, last - begin + 1 ) );
	      SWAP;
	    }
	    begin = c+1;
	  }
#if 0 /* DITTO */
	  else
	    ;
#endif
	  last = c;
	  sp_tag_save = 0;
	  state = NOTAG;
	  break;
	}
	else
	{
	  pop_stack();
	  state = CONTENT;
	}
	break;
       default:
	break;
      }
      break;
      
     case CONTENT_SKIP:
      switch (str[c])
      {
       case '>':
	last = c;
	state = CONTENT;
	break;
       case '"':
	state = CONTENT_SKIP_FNUTT_FNUTT;
	break;
       case '\'':
	state = CONTENT_SKIP_FNUTT;
	break;
      }
      break;

     case CONTENT_SKIP_FNUTT_FNUTT:
      switch (str[c])
      {
       case '"':
	state = CONTENT_SKIP;
	break;
      }
      break;

     case CONTENT_SKIP_FNUTT:
      switch (str[c])
      {
       case '\'':
	state = CONTENT_SKIP;
	break;
      }
      break;
#if 0
     default:
      /*  Make HPCC happy */
#endif
    }
  if (sp_tag_save)
    while (sp_tag_save <= sp)
      pop_stack();
  if (last >= begin)
    push_string( make_shared_binary_string( str + begin, last - begin + 1 ) );
  if (sp - sp_save == 0)
    push_text( "" );
  else if (sp - sp_save != 1)
    f_add( sp - sp_save ); /* fix? this is what we return */
  SWAP;
  pop_stack(); /* get rid of data_arg */
  if (last < length-1)
  {
    DATA->last_buffer = malloc( length - (last + 1) );
    MEMCPY( DATA->last_buffer, str + last + 1, length - (last + 1) );
    DATA->last_buffer_size = length - (last + 1);
  }
}

void streamed_parser_finish( INT32 args )
{
  if(args) error("FOO!\n"); /* Per ... */
  push_string( make_shared_binary_string( (char *)(DATA->last_buffer), DATA->last_buffer_size ) );
  if (DATA->last_buffer)
    free( DATA->last_buffer );
  DATA->last_buffer = 0;
  DATA->last_buffer_size = 0;
}

