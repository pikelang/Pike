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



static struct svalue *aggregate_tokens(struct svalue *x) 
{
    f_aggregate( Pike_sp-x );
    return Pike_sp;
}

static void push_token( const char * from, int start, int end )
{
    push_string( make_shared_binary_string(from+start, end-start+1) );
}

static void tokenize( struct pike_string *s )
{
    int in_string = 0;
    unsigned int ts=0, i, len=s->len;
    const char *data = (const char *)s->str;
    struct svalue *osp = Pike_sp;
    struct svalue *ots = Pike_sp;

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
		    ots=aggregate_tokens(ots);
		    break;
	    }
	}
    }
    if( ts < len ) push_token( data, ts, len-1 );
    f_aggregate( aggregate_tokens(ots)-osp );
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

void exit_parser_rcs(void)
{
}
