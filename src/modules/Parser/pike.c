/* array(array(string),string) split( string x )
 * Returns an array with Pike-level tokens and the remainder (a
 * partial token), if any.
 */
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

INLINE static int m_isidchar( unsigned int x )
{
  if( (x >= 'a' && x <= 'z') || (x>='A' && x<='Z') || x>128 || x == '_')
    return 1;
  return 0;
}

INLINE static int m_isidchar2( unsigned int x )
{
  if( (x >= 'a' && x <= 'z') || (x>='A' && x<='Z') || x>128 || x=='_' ||
      (x>='0'&&x<='9')||x=='$')
    return 1;
  return 0;
}


#define PUSH_TOKEN push_token0
#define TOKENIZE     tokenize0
#define CHAR       p_wchar0
#include "pike_tokenizer.h"
#undef TOKENIZE
#undef CHAR
#undef PUSH_TOKEN

#define PUSH_TOKEN push_token1
#define TOKENIZE   tokenize1
#define CHAR       p_wchar1
#include "pike_tokenizer.h"
#undef TOKENIZE
#undef CHAR
#undef PUSH_TOKEN

#define PUSH_TOKEN push_token2
#define TOKENIZE   tokenize2
#define CHAR       p_wchar2
#include "pike_tokenizer.h"
#undef TOKENIZE
#undef CHAR
#undef PUSH_TOKEN

static void do_free_arrayptr( struct array **x )
{
  free_array( *x );
}			   

static void f_tokenize( INT32 args )
{
  struct array *res;
  struct pike_string *left_s = 0; /* Make gcc happy. */
  int left;
  ONERROR tmp;

  if( Pike_sp[-1].type != PIKE_T_STRING )
    Pike_error("Expected string argument\n");

  if( Pike_sp[-1].u.string->len==0 )
  {
    pop_n_elems(args);
    push_array(&empty_array);
    push_string(empty_pike_string);
    f_aggregate(2);
    return;
  }

  res = allocate_array_no_init( 0, 128 );
  SET_ONERROR(tmp, do_free_arrayptr, &res);
  
  switch( Pike_sp[-1].u.string->size_shift )
  {
    case 0:
      left=tokenize0(&res,(p_wchar0*)Pike_sp[-1].u.string->str,Pike_sp[-1].u.string->len);
      left_s = make_shared_binary_string0( (p_wchar0*)Pike_sp[-1].u.string->str+left,
					   Pike_sp[-1].u.string->len-left);
      break;
    case 1:
      left=tokenize1(&res,(p_wchar1*)Pike_sp[-1].u.string->str,Pike_sp[-1].u.string->len);
      left_s = make_shared_binary_string1( (p_wchar1*)Pike_sp[-1].u.string->str+left,
					   Pike_sp[-1].u.string->len-left);
      break;
    case 2:
      left=tokenize2(&res,(p_wchar2*)Pike_sp[-1].u.string->str,Pike_sp[-1].u.string->len);
      left_s = make_shared_binary_string1( (p_wchar1*)Pike_sp[-1].u.string->str+left,
					   Pike_sp[-1].u.string->len-left);
      break;
#ifdef PIKE_DEBUG
    default:
      Pike_error("Unknown shift size %d.\n", Pike_sp[-1].u.string->size_shift);
#endif
  }
  pop_n_elems(args);
  push_array(res);
  push_string( left_s );
  f_aggregate( 2 );
}


void init_parser_pike()
{
  ADD_FUNCTION("tokenize",f_tokenize,tFunc(tStr,tArr(tStr)),0);
}

void exit_parser_pike()
{
}
