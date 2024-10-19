/* -*- c -*-
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
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
#include <ctype.h>

#include "parser.h"

/*! @module Parser
 */

/*! @module _parser
 */

/*! @module _C
 *!
 *! Low-level helpers for @[Parser.C].
 *!
 *! @note
 *!   You probably want to use @[Parser.C] instead of this module.
 *!
 *! @seealso
 *!   @[Parser.C], @[_Pike].
 */

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


#define MAKE_PUSH_TOKEN(X)						\
  void PIKE_CONCAT(push_token,X)(struct array **_a,			\
				 PIKE_CONCAT(p_wchar,X) *x,		\
				 int l)					\
{									\
  struct array *a = *_a;						\
  int sz = a->size;							\
  if( sz == a->malloced_size ) {					\
    a = *_a = resize_array( a, a->malloced_size + 10 );			\
    a->size = sz;							\
  }									\
  SET_SVAL(a->item[sz], PIKE_T_STRING, 0, string,			\
	   PIKE_CONCAT(make_shared_binary_string,X)(x, l));		\
  a->size++;								\
}

MAKE_PUSH_TOKEN(0);
MAKE_PUSH_TOKEN(1);
MAKE_PUSH_TOKEN(2);
#undef MAKE_PUSH_TOKEN


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

/*! @decl array(array(string)|string) tokenize(string code)
 *!
 *!   Tokenize a string of C tokens.
 *!
 *! @note
 *!   Don't use this function directly.
 *!   Use @[Parser.C.tokenize()] instead.
 *!
 *! @returns
 *!   Returns an array with an array with C-level tokens, and
 *!   the remainder (a partial token), if any.
 */
static void f_tokenize( INT32 args )
{
  struct array *res = allocate_array_no_init( 0, 128 );
  struct pike_string *left_s = 0; /* Make gcc happy. */
  struct pike_string *data;
  int left;
  ONERROR tmp;

  get_all_args(NULL, args, "%t", &data);

  SET_ONERROR(tmp, do_free_arrayptr, &res);

  switch(data->size_shift)
  {
    case 0:
      left = tokenize0(&res, STR0(data), data->len);
      left_s = make_shared_binary_string0(STR0(data)+left, data->len-left);
      break;
    case 1:
      left = tokenize1(&res, STR1(data), data->len);
      left_s = make_shared_binary_string1(STR1(data)+left, data->len-left);
      break;
    case 2:
      left = tokenize2(&res,STR2(data), data->len);
      left_s = make_shared_binary_string2(STR2(data)+left, data->len-left);
      break;
  }

  UNSET_ONERROR(tmp);
  if (!res->size) {
    free_array(res);
    add_ref(res = &empty_array);
  }
  push_array(res);
  push_string( left_s );
  f_aggregate( 2 );
  stack_pop_n_elems_keep_top(args);
}

/*! @endmodule
 */

/*! @endmodule
 */

/*! @endmodule
 */

void init_parser_c(void)
{
  ADD_FUNCTION2("tokenize", f_tokenize,
		tFunc(tStr, tArr(tOr(tArr(tStr),tStr))), 0, 0);
}

void exit_parser_c(void)
{
}
