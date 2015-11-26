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
#include <ctype.h>

#include "parser.h"

/*! @module Parser
 */

/*! @module _parser
 */

/*! @module _RCS
 *!
 *! Low-level helpers for @[Parser.RCS].
 *!
 *! @note
 *!   You probably want to use @[Parser.RCS] instead of this module.
 *!
 *! @seealso
 *!   @[Parser.RCS]
 */

static void push_token( const char * from, int start, int end )
{
    struct array *a = Pike_sp[-1].u.array;
    struct pike_string *token =
      make_shared_binary_string(from+start, end-start+1);
    if( a->malloced_size < a->size+1 )
    {
	Pike_sp[-1].u.array = a = resize_array( a, a->size+1 );
	a->size--;
    }
    SET_SVAL(a->item[a->size], PIKE_T_STRING, 0, string, token);
    a->size++;
}

/*! @decl array(array(string)) tokenize(string code)
 *!
 *!   Tokenize a string of RCS tokens.
 *!
 *! @note
 *!   Don't use this function directly.
 *!   Use @[Parser.RCS.tokenize()] instead.
 *!
 *! @seealso
 *!   @[Parser.RCS.tokenize()]
 */
static void tokenize( struct pike_string *s )
{
    int in_string = 0;
    unsigned int ts=0, i, len=s->len;
    const char *data = s->str;
    check_stack(200);
    BEGIN_AGGREGATE_ARRAY(1024) {
      // NB: An rcs file typically begins with "head 1.111;"
      push_array( allocate_array_no_init( 0, 2 ) );
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
		    // NB: We perform the incremental aggregate here
		    //     (just before we push a new token array),
		    //     to ensure that the active token array is
		    //     always at the top of the stack.
		    DO_AGGREGATE_ARRAY(100);
		    // NB: Typical histogram of array lengths (116 revisions):
		    //
		    //     1: 138
		    //     2: 399
		    //     3: 134
		    //     4: 0
		    //     5: 1
		    // [...]: 0
		    //   667: 1
		    //
		    // We therefore start with a tentative length of 2,
		    // and let resize_array() handle the special cases.
		    push_array( allocate_array_no_init( 0, 2 ) );
		    break;
	    }
	}
      }
      if( ts < len ) push_token( data, ts, len-1 );
    } END_AGGREGATE_ARRAY;
}

static void f_tokenize( INT32 args )
{
    if( !args || TYPEOF(Pike_sp[-args]) != PIKE_T_STRING )
	Pike_error("Illegal argument 1 to tokenize\n");
    tokenize( Pike_sp[-args].u.string );
    stack_swap();
    pop_stack();
}

/*! @endmodule
 */

/*! @endmodule
 */

/*! @endmodule
 */

void init_parser_rcs(void)
{
    ADD_FUNCTION("tokenize",f_tokenize,tFunc(tStr,tArray),0);
}

void exit_parser_rcs(void) {}
