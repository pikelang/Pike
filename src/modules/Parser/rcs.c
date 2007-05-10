#include "global.h"
#include "config.h"

#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
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

static void push_token( const char * from, int start, int end )
{
    struct array *a = Pike_sp[-1].u.array;
    struct pike_string *token = make_shared_binary_string(from+start, end-start+1);
    if( a->malloced_size < a->size+1 )
    {
	Pike_sp[-1].u.array = a = resize_array( a, a->size+1 );
	a->size--;
    }
    a->item[a->size].type = PIKE_T_STRING;
    a->item[a->size].subtype = 0;
    a->item[a->size].u.string = token;
    a->size++;
}

static void tokenize( struct pike_string *s )
{
    int in_string = 0;
    unsigned int ts=0, i, len=s->len;
    const char *data = s->str;
    struct svalue *osp = Pike_sp;
    push_array( allocate_array_no_init( 0, 100 ) );
    for( i=0; i<len; i++ )
    {
	if( in_string )
	{
	    if( (data[i]=='@') )
	    {
		if( data[i+1]!='@' )
		{
		    push_token( data, ts, i-1 );
		    in_string=0;
		    ts=i+1;
		}
		else
		    i++;
	    }
	}
	else
	{
	    switch( data[i] )
	    {
		case '@': ts=i+1; in_string=1; break;
		case ':':
		case ' ': case '\t': case '\n':	case '\r':
		    if( ts < i ) push_token( data, ts, i-1 );
		    ts=i+1;
		    break;
		case ';':
		    if( ts < i ) push_token( data, ts, i-1 );
		    ts=i+1;
		    push_array( allocate_array_no_init( 0, 10 ) );
		    break;
	    }
	}
    }
    if( ts < len ) push_token( data, ts, len-1 );
    f_aggregate( Pike_sp-osp );
}

static void f_tokenize( INT32 args )
{
    if( !args || Pike_sp[-args].type != PIKE_T_STRING )
	Pike_error("Illegal argument 1 to tokenize\n");
    tokenize( Pike_sp[-args].u.string );
    stack_swap();
    pop_stack();
}

void init_parser_rcs(void)
{
    ADD_FUNCTION("tokenize",f_tokenize,tFunc(tStr,tArray),0);
}

void exit_parser_rcs(void) {}
