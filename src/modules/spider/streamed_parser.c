#include "stralloc.h"
#include "global.h"
#include "types.h"
#include "macros.h"
#include "object.h"
#include "add_efun.h"
#include "interpret.h"
#include "svalue.h"
#include "mapping.h"
#include "array.h"
#include "builtin_efuns.h"

#include "streamed_parser.h"

/* streamed SGML parser, by wing */

/* State machine for parsing

notag		<	tag_start
	notag

tag_start	/	tag_end
		WS	tag_start	(error) 
		>	notag
	tag_name

tag_end		WS	skip		(pop-tag-stack)
		>	notag		(pop-tag-stack)
	tag_end
	
skip		>	notag
		"	skip_fnutt_fnutt
		'	skip_fnutt
	skip

skip_fnutt_fnutt	"	skip
	skip_fnutt_fnutt

skip_fnutt	'	skip
	skip_fnut

tag_name	WS	skip (or) tag_ws
		>	notag
	tag_name

tag_ws		WS	tag_ws
		>	notag (check if something changed)
	tag_arg_name

tag_arg_name	WS	tag_post_arg_name
		=	tag_pre_arg_name
		>	notag (check if something changed)
	tag_arg_name

tag_post_arg_name	WS	tag_post_arg_name
			=	tag_pre_arg_value
			>	notag (check if something changed)
	tag_arg_name

tag_pre_arg_value	WS	tag_pre_arg_value
			>	notag (error) (check if something changed)
			"	tag_arg_value_fnutt_fnutt
			'	tag_arg_value_fnutt
	tag_arg_value

tag_arg_value_fnutt_fnutt	"	tag_ws
	tag_arg_value_fnutt_fnutt

tag_arg_value_fnutt	'	tag_ws
	tag_arg_value_fnutt

tag_arg_value	WS	tag_ws
		>	notag (check if something changed)
	tag_arg_value

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

#define ARG_TYPE_NONE			 0
#define ARG_TYPE_IN			 1
#define ARG_TYPE_OUT			 2

#define WS ' ': case '\t': case '\n': case '\r'

#define DATA ((struct streamed_parser *)(fp->current_storage))

void streamed_parser_init()
{
  DATA->last_buffer = 0;
  DATA->last_buffer_size = 0;
  DATA->start_tags = 0;
  DATA->end_tags = 0;
}

void streamed_parser_destruct()
{
  if (DATA->last_buffer)
    free( DATA->last_buffer );
  if (DATA->start_tags)
    ;
  if (DATA->end_tags)
    ;
}

void streamed_parser_set_data( INT32 args )
{
  DATA->end_tags = sp[-1].u.mapping;
  sp--;
  DATA->start_tags = sp[-1].u.mapping;
  sp--;
}

#define SWAP \
	    *sp = sp[-2]; \
	    sp[-2] = sp[-1]; \
	    sp[-1] = *sp

static int handle_tag( struct svalue *data_arg )
{
  int lookup;

  assign_svalue_no_free( sp, data_arg );
  sp++;
  lookup = set_lookup( DATA->start_tags->ind, sp-3 );
  apply_svalue( ITEM( DATA->start_tags->val ) + lookup, 3 );
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
  int lookup;

  lookup = set_lookup( DATA->start_tags->ind, sp-1 );
  pop_stack();
  assign_svalue_no_free( sp, data_arg );
  sp++;
  if (lookup != -1)
  {
    apply_svalue( ITEM( DATA->start_tags->val ) + lookup, 1 );
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

static void add_arg()
{
  mapping_insert( sp[-3].u.mapping, sp-2, sp-1 );
  pop_stack();
  pop_stack();
}

void streamed_parser_parse( INT32 args )
{
  int c, length, state, begin, last, ind, ind2;
  char *str;
  struct svalue *sp_save;
  struct svalue *sp_tag_save;
  struct svalue *data_arg;
  
  state = NOTAG;
  begin = 0; 
  last = -1;
  SWAP;
  length = sp[-1].u.string->len;
  if (DATA->last_buffer_size > 0)
  {
    str = alloca( DATA->last_buffer_size + length );
    MEMCPY( str, DATA->last_buffer, DATA->last_buffer_size );
    MEMCPY( str + DATA->last_buffer_size, sp[-1].u.string->str, length );
    length += DATA->last_buffer_size;
    free( DATA->last_buffer );
    DATA->last_buffer = 0;
    DATA->last_buffer_size = 0;
    pop_stack();
  }
  else
  {
    str = sp[-1].u.string->str;
    sp--;
  }
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
	ind = c;
	state = TAG_END;
	break;
       case WS:
	state = TAG_START; /* error... */
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
	ind = c;
	break;
       case '>':
	last = c;
	state = NOTAG;
	break;
       default:
	ind2 = -1;
	state = TAG_END_NAME;
	break;
      }
      break;

     case TAG_END_NAME:
      switch (str[c])
      {
       case WS:
	ind2 = c-1;
	break;
       case '>':
	if (ind2 == -1)
	  ind2 = c-1;
	push_string( make_shared_binary_string( str + ind, ind2 - ind ) );
	f_lower_case( 1 );
	if (list_member( DATA->end_tags, sp-1 ))
	  if (handle_end_tag( data_arg ))
	  {
	    if (last >= begin)
	    {
	      push_string( make_shared_binary_string( str + begin, last - begin + 1 ) );
	      SWAP;
	    }
	    begin = c+1;
	  }
	  else
	    ;
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
	if (list_member( DATA->start_tags, sp-1 ))
	{
	  f_aggregate_mapping( 0 );
	  state = TAG_WS;
	  sp_tag_save = sp-1;
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
	if (list_member( DATA->start_tags, sp-1 ))
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
	  else
	    ;
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
	if (handle_tag( data_arg ))
	{
	  if (last >= begin)
	  {
	    push_string( make_shared_binary_string( str + begin, last - begin + 1 ) );
	    SWAP;
	  }
	  begin = c+1;
	}
	else
	  ;
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
	if (handle_tag( data_arg ))
	{
	  if (last >= begin)
	  {
	    push_string( make_shared_binary_string( str + begin, last - begin + 1 ) );
	    SWAP;
	  }
	  begin = c+1;
	}
	else
	  ;
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
	if (handle_tag( data_arg ))
	{
	  if (last >= begin)
	  {
	    push_string( make_shared_binary_string( str + begin, last - begin + 1 ) );
	    SWAP;
	  }
	  begin = c+1;
	}
	else
	  ;
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
	if (handle_tag( data_arg ))
	{
	  if (last >= begin)
	  {
	    push_string( make_shared_binary_string( str + begin, last - begin + 1 ) );
	    SWAP;
	  }
	  begin = c+1;
	}
	else
	  ;
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
	if (handle_tag( data_arg ))
	{
	  if (last >= begin)
	  {
	    push_string( make_shared_binary_string( str + begin, last - begin + 1 ) );
	    SWAP;
	  }
	  begin = c+1;
	}
	else
	  ;
	last = c;
	sp_tag_save = 0;
	state = NOTAG;
	break;
      }
      break;
     default:      
    }
  if (sp_tag_save)
    while (sp_tag_save <= sp)
      pop_stack();
  if (last >= begin)
    push_string( make_shared_binary_string( str + begin, last - begin + 1 ) );
  if (sp - sp_save == 0)
    push_text( "" );
  else if (sp - sp_save == 1)
    ;
  else
    f_sum( sp - sp_save ); /* fix? this is what we return */
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
  push_string( make_shared_binary_string( DATA->last_buffer, DATA->last_buffer_size ) );
  if (DATA->last_buffer)
    free( DATA->last_buffer );
  DATA->last_buffer = DATA->last_buffer_size = 0;
}

