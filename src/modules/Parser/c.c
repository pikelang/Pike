/* array(array(string),string) split( string x )
 * Returns an array with C-level tokens and the remainder (a partial
 * token), if any.
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

static int m_isidchar( unsigned int x )
{
  if( (x >= 'a' && x <= 'z') || (x>='A' && x<='Z') || x>128 || x == '_')
    return 1;
  return 0;
}

static int m_isidchar2( unsigned int x )
{
  if( (x >= 'a' && x <= 'z')||(x>='A' && x<='Z')||x>128||x=='_' ||(x>='0'&&x<='9')||x=='$')
    return 1;
  return 0;
}


#define PUSH_TOKEN(X) \
void PIKE_CONCAT(push_token,X)( struct array **_a, PIKE_CONCAT(p_wchar,X) *x, int l )\
{                                                                                    \
  struct array *a = *_a;                                                             \
  if( a->size == a->malloced_size ) {                                                \
    int q = a->size;                                                                 \
    a = *_a = resize_array( a, a->malloced_size + 10 );        \
    a->size = q;                                                                     \
  }                                                                                  \
  a->item[a->size].type = PIKE_T_STRING;                                             \
  a->item[a->size].u.string = PIKE_CONCAT(make_shared_binary_string,X)( x, l );      \
  a->size++;                                                                         \
}

PUSH_TOKEN(0);
PUSH_TOKEN(1);
PUSH_TOKEN(2);
#undef PUSH_TOKEN


#define PUSH_TOKEN push_token0
#define TOKENIZE     tokenize0
#define CHAR       p_wchar0
#include "c_tokenizer.h"
#undef TOKENIZE
#undef CHAR
#undef PUSH_TOKEN

#define PUSH_TOKEN push_token1
#define TOKENIZE   tokenize1
#define CHAR       p_wchar1
#include "c_tokenizer.h"
#undef TOKENIZE
#undef CHAR
#undef PUSH_TOKEN


#define PUSH_TOKEN push_token2
#define TOKENIZE   tokenize2
#define CHAR       p_wchar2
#include "c_tokenizer.h"
#undef TOKENIZE
#undef CHAR
#undef PUSH_TOKEN

static void do_free_arrayptr( struct array **x )
{
  free_array( *x );
}			   

static void f_tokenize( INT32 args )
{
  struct array *res = allocate_array_no_init( 0, 128 );
  struct pike_string *left_s = 0; /* Make gcc happy. */
  int left;
  ONERROR tmp;

  if( Pike_sp[-1].type != PIKE_T_STRING )
    Pike_error("Expected string argument\n");

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
  }
  push_array(res);
  push_string( left_s );
  f_aggregate( 2 );
  stack_pop_keep_top();
}


void init_parser_c()
{
  ADD_FUNCTION("tokenize",f_tokenize,tFunc(tStr,tArr(tStr)),0);
}

void exit_parser_c()
{
}
