/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: builtin_functions.c,v 1.588 2005/02/09 16:40:35 mast Exp $
*/

#include "global.h"
#include "interpret.h"
#include "svalue.h"
#include "pike_macros.h"
#include "object.h"
#include "program.h"
#include "array.h"
#include "pike_error.h"
#include "constants.h"
#include "mapping.h"
#include "stralloc.h"
#include "multiset.h"
#include "pike_types.h"
#include "pike_rusage.h"
#include "operators.h"
#include "fsort.h"
#include "callback.h"
#include "gc.h"
#include "backend.h"
#include "main.h"
#include "pike_memory.h"
#include "threads.h"
#include "time_stuff.h"
#include "version.h"
#include "encode.h"
#include <math.h>
#include <ctype.h>
#include "module_support.h"
#include "module.h"
#include "opcodes.h"
#include "cyclic.h"
#include "signal_handler.h"
#include "pike_security.h"
#include "builtin_functions.h"
#include "bignum.h"
#include "peep.h"
#include "docode.h"
#include "lex.h"
#include "pike_float.h"

#ifdef HAVE_POLL
#ifdef HAVE_POLL_H
#include <poll.h>
#endif /* HAVE_POLL_H */

#ifdef HAVE_SYS_POLL_H
#include <sys/poll.h>
#endif /* HAVE_SYS_POLL_H */
#endif /* HAVE_POLL */

#ifdef HAVE_CRYPT_H
#include <crypt.h>
#endif

/* #define DIFF_DEBUG */
/* #define ENABLE_DYN_DIFF */

/*! @decl int equal(mixed a, mixed b)
 *!
 *!   This function checks if the values @[a] and @[b] are equal.
 *!
 *!   For all types but arrays, multisets and mappings, this operation is
 *!   the same as doing @expr{@[a] == @[b]@}.
 *!   For arrays, mappings and multisets however, their contents are checked
 *!   recursively, and if all their contents are the same and in the same
 *!   place, they are considered equal.
 *!
 *! @seealso
 *!   @[copy_value()]
 */
PMOD_EXPORT void f_equal(INT32 args)
{
  int i;
  if(args != 2)
    SIMPLE_TOO_FEW_ARGS_ERROR("equal", 2);

  i=is_equal(Pike_sp-2,Pike_sp-1);
  pop_n_elems(args);
  push_int(i);
}

/*! @decl array aggregate(mixed ... elements)
 *!
 *!   Construct an array with the arguments as indices.
 *!
 *!   This function could be written in Pike as:
 *! @code
 *! array aggregate(mixed ... elems) { return elems; }
 *! @endcode
 *!
 *! @note
 *!   Arrays are dynamically allocated there is no need to declare them
 *!   like @expr{int a[10]=allocate(10);@} (and it isn't possible either) like
 *!   in C, just @expr{array(int) a=allocate(10);@} will do.
 *!
 *! @seealso
 *!   @[sizeof()], @[arrayp()], @[allocate()]
 */
PMOD_EXPORT void debug_f_aggregate(INT32 args)
{
  struct array *a;
  a=aggregate_array(args);
  push_array(a); /* beware, macro */
}


/*! @decl int hash_7_4(string s)
 *! @decl int hash_7_4(string s, int max)
 *!
 *!   Return an integer derived from the string @[s]. The same string
 *!   will always hash to the same value, also between processes.
 *!
 *!   If @[max] is given, the result will be >= 0 and < @[max],
 *!   otherwise the result will be >= 0 and <= 0x7fffffff.
 *!
 *! @note
 *!   This function is provided for backward compatibility reasons.
 *!
 *!   This function is byte-order dependant for wide strings.
 *!
 *! @seealso
 *!   @[hash()], @[hash_7_0()]
 */
static void f_hash_7_4(INT32 args)
{
  size_t i = 0;
  struct pike_string *s = Pike_sp[-args].u.string;

  if(!args)
    SIMPLE_TOO_FEW_ARGS_ERROR("hash_7_4",1);

  if(Pike_sp[-args].type != T_STRING)
    SIMPLE_BAD_ARG_ERROR("hash_7_4", 1, "string");

  i = simple_hashmem((unsigned char *)s->str, s->len<<s->size_shift,
		     100<<s->size_shift);

  if(args > 1)
  {
    if(Pike_sp[1-args].type != T_INT)
      SIMPLE_BAD_ARG_ERROR("hash_7_4",2,"int");
    
    if(!Pike_sp[1-args].u.integer)
      PIKE_ERROR("hash_7_4", "Modulo by zero.\n", Pike_sp, args);

    i%=(unsigned INT32)Pike_sp[1-args].u.integer;
  }
  pop_n_elems(args);
  push_int64(i);
}

/*! @decl int hash_7_0(string s)
 *! @decl int hash_7_0(string s, int max)
 *!
 *!   Return an integer derived from the string @[s]. The same string
 *!   always hashes to the same value, also between processes.
 *!
 *!   If @[max] is given, the result will be >= 0 and < @[max],
 *!   otherwise the result will be >= 0 and <= 0x7fffffff.
 *!
 *! @note
 *!   This function is provided for backward compatibility reasons.
 *!
 *!   This function is not NUL-safe, and is byte-order dependant.
 *!
 *! @seealso
 *!   @[hash()], @[hash_7_4()]
 */
static void f_hash_7_0( INT32 args )
{
  struct pike_string *s = Pike_sp[-args].u.string;
  unsigned int i;
  if(!args)
    SIMPLE_TOO_FEW_ARGS_ERROR("hash_7_0",1);
  if(Pike_sp[-args].type != T_STRING)
    SIMPLE_BAD_ARG_ERROR("hash_7_0", 1, "string");

  if( s->size_shift )
  {
    f_hash_7_4( args );
    return;
  }

  i = DO_NOT_WARN((unsigned int)hashstr( (unsigned char *)s->str,
					 MINIMUM(100,s->len)));
  if(args > 1)
  {
    if(Pike_sp[1-args].type != T_INT)
      SIMPLE_BAD_ARG_ERROR("hash_7_0",2,"int");
    
    if(!Pike_sp[1-args].u.integer)
      PIKE_ERROR("hash_7_0", "Modulo by zero.\n", Pike_sp, args);

    i%=(unsigned INT32)Pike_sp[1-args].u.integer;
  }
  pop_n_elems(args);
  push_int( i );
}

/*! @decl int hash(string s)
 *! @decl int hash(string s, int max)
 *!
 *!   Return an integer derived from the string @[s]. The same string
 *!   always hashes to the same value, also between processes,
 *!   architectures, and Pike versions (see compatibility notes below,
 *!   though).
 *!
 *!   If @[max] is given, the result will be >= 0 and < @[max],
 *!   otherwise the result will be >= 0 and <= 0x7fffffff.
 *!
 *! @note
 *!   The hash algorithm was changed in Pike 7.5. If you want a hash
 *!   that is compatible with Pike 7.4 and earlier, use @[hash_7_4()].
 *!   The difference only affects wide strings.
 *!
 *!   The hash algorithm was also changed in Pike 7.1. If you want a hash
 *!   that is compatible with Pike 7.0 and earlier, use @[hash_7_0()].
 *!
 *! @seealso
 *!   @[hash_7_0()], @[hash_7_4()], @[hash_value]
 */
PMOD_EXPORT void f_hash(INT32 args)
{
  size_t i = 0;
  struct pike_string *s;

  if(!args)
    SIMPLE_TOO_FEW_ARGS_ERROR("hash",1);

  if(Pike_sp[-args].type != T_STRING)
    SIMPLE_BAD_ARG_ERROR("hash", 1, "string");

  s = Pike_sp[-args].u.string;
  switch(s->size_shift) {
  case 0:
    i = simple_hashmem(STR0(s), s->len, 100);
    break;
  case 1:
    i = simple_hashmem1(STR1(s), s->len, 100);
    break;
  case 2:
    i = simple_hashmem2(STR2(s), s->len, 100);
    break;
  default:
    Pike_fatal("hash(): Unsupported string shift: %d\n", s->size_shift);
    break;
  }

  if(args > 1)
  {
    if(Pike_sp[1-args].type != T_INT)
      SIMPLE_BAD_ARG_ERROR("hash",2,"int");
    
    if(Pike_sp[1-args].u.integer <= 0)
      PIKE_ERROR("hash", "Modulo < 1.\n", Pike_sp, args);

    i%=(unsigned INT32)Pike_sp[1-args].u.integer;
  }
  pop_n_elems(args);
  push_int64(i);
}

/*! @decl int hash_value (mixed value)
 *!
 *! Return a hash value for the argument. It's an integer in the
 *! native integer range.
 *!
 *! The hash will be the same for the same value in the running
 *! process only (the memory address is typically used as the basis
 *! for the hash value).
 *!
 *! If the value is an object with an @[lfun::__hash], that function
 *! is called and its result is returned.
 *!
 *! @note
 *! This is the hashing method used by mappings.
 *!
 *! @seealso
 *! @[hash]
 */
void f_hash_value(INT32 args)
{
  unsigned INT32 h;

  if(!args)
    SIMPLE_TOO_FEW_ARGS_ERROR("hash_value",1);

  h = hash_svalue (Pike_sp - args);
  pop_n_elems (args);
  push_int (h);
}

/*! @decl mixed copy_value(mixed value)
 *!
 *!   Copy a value recursively.
 *!
 *!   If the result value is changed destructively (only possible for
 *!   multisets, arrays and mappings) the copied value will not be changed.
 *!
 *!   The resulting value will always be equal to the copied (as tested with
 *!   the function @[equal()]), but they may not the the same value (as tested
 *!   with @[`==()]).
 *!
 *! @seealso
 *!   @[equal()]
 */
PMOD_EXPORT void f_copy_value(INT32 args)
{
  if(!args)
    SIMPLE_TOO_FEW_ARGS_ERROR("copy_value",1);

  pop_n_elems(args-1);
  copy_svalues_recursively_no_free(Pike_sp,Pike_sp-1,1,0);
  free_svalue(Pike_sp-1);
  Pike_sp[-1]=Pike_sp[0];
  dmalloc_touch_svalue(Pike_sp-1);
}

struct case_info {
  INT32 low;	/* low end of range. */
  INT32 mode;
  INT32 data;
};

#define CIM_NONE	0	/* Case-less */
#define CIM_UPPERDELTA	1	/* Upper-case, delta to lower-case in data */
#define CIM_LOWERDELTA	2	/* Lower-case, -delta to upper-case in data */
#define CIM_CASEBIT	3	/* Some case, case mask in data */
#define CIM_CASEBITOFF	4	/* Same as above, but also offset by data */

static const struct case_info case_info[] = {
#include "case_info.h"
  { 0x7fffffff, CIM_NONE, 0x0000, },	/* End sentinel. */
};

static struct case_info *find_ci(INT32 c)
{
  static struct case_info *cache = NULL;
  struct case_info *ci = cache;
  int lo = 0;
  int hi = NELEM(case_info);

  if ((c < 0) || (c > 0xeffff)) {
    /* Negative, or plane 15 and above. */
    return NULL;
  }

  if ((ci) && (ci[0].low <= c) && (ci[1].low > c)) {
    return ci; 
  }

  while (lo != hi-1) {
    int mid = (lo + hi)/2;
    if (case_info[mid].low < c) {
      lo = mid;
    } else if (case_info[mid].low == c) {
      lo = mid;
      break;
    } else {
      hi = mid;
    }
  }
  return(cache = (struct case_info *)case_info + lo);
}

static struct case_info *find_ci_shift0(INT32 c)
{
  static struct case_info *cache = NULL;
  struct case_info *ci = cache;
  int lo = 0;
  int hi = CASE_INFO_SHIFT0_HIGH;

  if ((c < 0) || (c > 0xefffff)) {
    /* Negative, or plane 15 and above. */
    return NULL;
  }

  if ((ci) && (ci[0].low <= c) && (ci[1].low > c)) {
    return ci; 
  }

  while (lo != hi-1) {
    int mid = (lo + hi)>>1;
    if (case_info[mid].low < c) {
      lo = mid;
    } else if (case_info[mid].low == c) {
      lo = mid;
      break;
    } else {
      hi = mid;
    }
  }
  return(cache = (struct case_info *)case_info + lo);
}

#define DO_LOWER_CASE(C) do {\
    INT32 c = C; \
    if(c<0xb5){if(c >= 'A' && c <= 'Z' ) C=c+0x20;}else {\
    struct case_info *ci = find_ci(c); \
    if (ci) { \
      switch(ci->mode) { \
      case CIM_NONE: case CIM_LOWERDELTA: break; \
      case CIM_UPPERDELTA: C = c + ci->data; break; \
      case CIM_CASEBIT: C = c | ci->data; break; \
      case CIM_CASEBITOFF: C = ((c - ci->data) | ci->data) + ci->data; break; \
      default: Pike_fatal("lower_case(): Unknown case_info mode: %d\n", ci->mode); \
    } \
   }} \
  } while(0)

#define DO_LOWER_CASE_SHIFT0(C) do {\
    INT32 c = C; \
    if(c<0xb5){if(c >= 'A' && c <= 'Z' ) C=c+0x20;}else {\
    struct case_info *ci = find_ci_shift0(c); \
    if (ci) { \
      switch(ci->mode) { \
      case CIM_NONE: case CIM_LOWERDELTA: break; \
      case CIM_UPPERDELTA: C = c + ci->data; break; \
      case CIM_CASEBIT: C = c | ci->data; break; \
      case CIM_CASEBITOFF: C = ((c - ci->data) | ci->data) + ci->data; break; \
      default: Pike_fatal("lower_case(): Unknown case_info mode: %d\n", ci->mode); \
    } \
   }} \
  } while(0)

#define DO_UPPER_CASE(C) do {\
    INT32 c = C; \
    if(c<0xb5){if(c >= 'a' && c <= 'z' ) C=c-0x20;}else {\
    struct case_info *ci = find_ci(c); \
    if (ci) { \
      switch(ci->mode) { \
      case CIM_NONE: case CIM_UPPERDELTA: break; \
      case CIM_LOWERDELTA: C = c - ci->data; break; \
      case CIM_CASEBIT: C = c & ~ci->data; break; \
      case CIM_CASEBITOFF: C = ((c - ci->data)& ~ci->data) + ci->data; break; \
      default: Pike_fatal("upper_case(): Unknown case_info mode: %d\n", ci->mode); \
    } \
   }} \
  } while(0)

#define DO_UPPER_CASE_SHIFT0(C) do {\
    INT32 c = C; \
    if(c<0xb5){if(c >= 'a' && c <= 'z' ) C=c-0x20;}else {\
    struct case_info *ci = find_ci_shift0(c); \
    if (ci) { \
      switch(ci->mode) { \
      case CIM_NONE: case CIM_UPPERDELTA: break; \
      case CIM_LOWERDELTA: C = c - ci->data; break; \
      case CIM_CASEBIT: C = c & ~ci->data; break; \
      case CIM_CASEBITOFF: C = ((c - ci->data)& ~ci->data) + ci->data; break; \
      default: Pike_fatal("lower_case(): Unknown case_info mode: %d\n", ci->mode); \
    } \
   }} \
  } while(0)

/*! @decl string lower_case(string s)
 *! @decl int lower_case(int c)
 *!
 *!   Convert a string or character to lower case.
 *!
 *! @returns
 *!   Returns a copy of the string @[s] with all upper case characters
 *!   converted to lower case, or the character @[c] converted to lower
 *!   case.
 *!
 *! @note
 *!   Assumes the string or character to be coded according to
 *!   ISO-10646 (aka Unicode). If they are not, @[Locale.Charset.decoder]
 *!   can do the initial conversion for you.
 *!
 *! @note
 *!   Prior to Pike 7.5 this function only accepted strings.
 *!
 *! @seealso
 *!   @[upper_case()], @[Locale.Charset.decoder]
 */
PMOD_EXPORT void f_lower_case(INT32 args)
{
  ptrdiff_t i;
  struct pike_string *orig;
  struct pike_string *ret;

  check_all_args("lower_case", args, BIT_STRING|BIT_INT, 0);

  if (Pike_sp[-args].type == T_INT) {
    /* NOTE: Performs the case change in place. */
    DO_LOWER_CASE(Pike_sp[-args].u.integer);
    pop_n_elems(args-1);
    return;
  }
  
  orig = Pike_sp[-args].u.string;
  ret = begin_wide_shared_string(orig->len, orig->size_shift);

  MEMCPY(ret->str, orig->str, orig->len << orig->size_shift);

  i = orig->len;

  if (!orig->size_shift) {
    p_wchar0 *str = STR0(ret);

    while(i--) {
      DO_LOWER_CASE_SHIFT0(str[i]);
    }
  } else if (orig->size_shift == 1) {
    p_wchar1 *str = STR1(ret);

    while(i--) {
      DO_LOWER_CASE(str[i]);
    }
  } else if (orig->size_shift == 2) {
    p_wchar2 *str = STR2(ret);

    while(i--) {
      DO_LOWER_CASE(str[i]);
    }
  } else {
    Pike_fatal("lower_case(): Bad string shift:%d\n", orig->size_shift);
  }

  pop_n_elems(args);
  push_string(end_shared_string(ret));
}

/*! @decl string upper_case(string s)
 *! @decl int upper_case(int c)
 *!
 *!   Convert a string or character to upper case.
 *!
 *! @returns
 *!   Returns a copy of the string @[s] with all lower case characters
 *!   converted to upper case, or the character @[c] converted to upper
 *!   case.
 *!
 *! @note
 *!   Assumes the string or character to be coded according to
 *!   ISO-10646 (aka Unicode). If they are not, @[Locale.Charset.decoder]
 *!   can do the initial conversion for you.
 *!
 *! @note
 *!   Prior to Pike 7.5 this function only accepted strings.
 *!
 *! @seealso
 *!   @[lower_case()], @[Locale.Charset.decoder]
 */
PMOD_EXPORT void f_upper_case(INT32 args)
{
  ptrdiff_t i;
  struct pike_string *orig;
  struct pike_string *ret;
  check_all_args("upper_case", args, BIT_STRING|BIT_INT, 0);

  if (Pike_sp[-args].type == T_INT) {
    /* NOTE: Performs the case change in place. */
    DO_UPPER_CASE(Pike_sp[-args].u.integer);
    pop_n_elems(args-1);
    return;
  }
  
  orig = Pike_sp[-args].u.string;
  ret=begin_wide_shared_string(orig->len,orig->size_shift);
  MEMCPY(ret->str, orig->str, orig->len << orig->size_shift);

  i = orig->len;

  if (!orig->size_shift) {
    p_wchar0 *str = STR0(ret);

    while(i--) {
      if(str[i]!=0xff && str[i]!=0xb5) {
	DO_UPPER_CASE_SHIFT0(str[i]);
      } else {

	/* Ok, so our shiftsize 0 string contains 0xff or 0xb5 which
	   prompts for a shiftsize 1 string. */
	int j = orig->len;
	struct pike_string *wret = begin_wide_shared_string(j, 1);
	p_wchar1 *wstr = STR1(wret);

	/* Copy what we have done */
	while(--j>i)
	  wstr[j] = str[j];

	/* upper case the rest */
	i++;
	while(i--)
	  switch( str[i] ) {
	  case 0xff: wstr[i] = 0x178; break;
	  case 0xb5: wstr[i] = 0x39c; break;
	  default:
	    DO_UPPER_CASE_SHIFT0(str[i]);
	    wstr[i] = str[i];
	    break;
	  }

	/* Discard the too narrow string and use the new one instead. */
	do_really_free_pike_string(ret);
	ret = wret;
	break;
      }
    }
  } else if (orig->size_shift == 1) {
    p_wchar1 *str = STR1(ret);

    while(i--) {
      DO_UPPER_CASE(str[i]);
    }
  } else if (orig->size_shift == 2) {
    p_wchar2 *str = STR2(ret);

    while(i--) {
      DO_UPPER_CASE(str[i]);
    }
  } else {
    Pike_fatal("lower_case(): Bad string shift:%d\n", orig->size_shift);
  }

  pop_n_elems(args);
  push_string(end_shared_string(ret));
}

/*! @decl string random_string(int len)
 *!
 *!   Returns a string of random characters 0-255 with the length @[len].
 */
PMOD_EXPORT void f_random_string(INT32 args)
{
  struct pike_string *ret;
  INT_TYPE len, e;
  get_all_args("random_string",args,"%+",&len);
  ret = begin_shared_string(len);
  for(e=0;e<len;e++) ret->str[e] = DO_NOT_WARN((char)my_rand());
  pop_n_elems(args);
  push_string(end_shared_string(ret));
}

/*! @decl void random_seed(int seed)
 *!
 *!   This function sets the initial value for the random generator.
 *!
 *! @seealso
 *!   @[random()]
 */
PMOD_EXPORT void f_random_seed(INT32 args)
{
  INT_TYPE i;
#ifdef AUTO_BIGNUM
  check_all_args("random_seed",args,BIT_INT | BIT_OBJECT, 0);
  if(Pike_sp[-args].type == T_INT)
  {
    i=Pike_sp[-args].u.integer;
  }else{
    i=hash_svalue(Pike_sp-args);
  }
#else
  get_all_args("random_seed",args,"%i",&i);
#endif
  my_srand(i);
  pop_n_elems(args);
}

/*! @decl int query_num_arg()
 *!
 *!   Returns the number of arguments given when the previous function was
 *!   called.
 *!
 *!   This is useful for functions that take a variable number of arguments.
 *!
 *! @seealso
 *!   @[call_function()]
 */
void f_query_num_arg(INT32 args)
{
  pop_n_elems(args);
  push_int(Pike_fp ? Pike_fp->args : 0);
}

/*! @decl int search(string haystack, string|int needle, int|void start)
 *! @decl int search(array haystack, mixed needle, int|void start)
 *! @decl mixed search(mapping haystack, mixed needle, mixed|void start)
 *! @decl mixed search(object haystack, mixed needle, mixed|void start)
 *!
 *!   Search for @[needle] in @[haystack]. Return the position of @[needle] in
 *!   @[haystack] or @expr{-1@} if not found.
 *!
 *!   If the optional argument @[start] is present search is started at
 *!   this position.
 *!
 *!   @mixed haystack
 *!     @type string
 *!       When @[haystack] is a string @[needle] must be a string or an int,
 *!       and the first occurrence of the string or int is returned.
 *!
 *!     @type array
 *!       When @[haystack] is an array, @[needle] is compared only to
 *!       one value at a time in @[haystack].
 *!
 *!     @type mapping
 *!       When @[haystack] is a mapping, @[search()] tries to find the index
 *!       connected to the data @[needle]. That is, it tries to lookup the
 *!       mapping backwards. If @[needle] isn't present in the mapping, zero
 *!       is returned, and zero_type() will return 1 for this zero.
 *!
 *!     @type object
 *!       When @[haystack] is an object implementing @[lfun::_search()],
 *!       the result of calling @[lfun::_search()] with @[needle] will
 *!       be returned.
 *!
 *!       If @[haystack] is an object that doesn't implement @[lfun::_search()]
 *!       it is assumed to be an @[Iterator], and implement
 *!       @[Iterator()->index()], @[Iterator()->value()], and
 *!       @[Iterator()->next()]. @[search()] will then start comparing
 *!       elements with @[`==()] until a match with @[needle] is found.
 *!       If @[needle] is found @[haystack] will be advanced to the element,
 *!       and the iterator index will be returned. If @[needle] is not
 *!       found, @[haystack] will be advanced to the end (and will thus
 *!       evaluate to false), and a zero with zero_type 1 will be returned.
 *!   @endmixed
 *!
 *! @note
 *!   If @[start] is supplied to an iterator object without an
 *!   @[lfun::_search()], @[haystack] will need to implement
 *!   @[Iterator()->set_index()].
 *!
 *! @seealso
 *!   @[indices()], @[values()], @[zero_type()]
 */
PMOD_EXPORT void f_search(INT32 args)
{
  ptrdiff_t start;

  if(args < 2)
    SIMPLE_TOO_FEW_ARGS_ERROR("search", 2);

  switch(Pike_sp[-args].type)
  {
  case T_STRING:
  {
    struct pike_string *haystack = Pike_sp[-args].u.string;

    start=0;
    if(args > 2)
    {
      if(Pike_sp[2-args].type!=T_INT)
	SIMPLE_BAD_ARG_ERROR("search", 3, "int");

      start=Pike_sp[2-args].u.integer;
      if(start<0) {
	bad_arg_error("search", Pike_sp-args, args, 3, "int(0..)", Pike_sp+2-args,
		   "Start must be greater or equal to zero.\n");
      }
    }

    if(haystack->len < start)
      bad_arg_error("search", Pike_sp-args, args, 3, "int(0..)", Pike_sp-args,
		    "Start must not be greater than the "
		    "length of the string.\n");

    if(Pike_sp[1-args].type == T_STRING) {
      /* Handle searching for the empty string. */
      if (Pike_sp[1-args].u.string->len) {
	start = string_search(haystack,
			      Pike_sp[1-args].u.string,
			      start);
      }
    } else if (Pike_sp[1-args].type == T_INT) {
      INT_TYPE val = Pike_sp[1-args].u.integer;
      
      switch(Pike_sp[-args].u.string->size_shift) {
      case 0:
	{
	  p_wchar0 *str = STR0(haystack);
	  if (val >= 256) {
	    start = -1;
	    break;
	  }
	  while (start < haystack->len) {
	    if (str[start] == val) break;
	    start++;
	  }
	}
	break;
      case 1:
	{
	  p_wchar1 *str = STR1(haystack);
	  if (val >= 65536) {
	    start = -1;
	    break;
	  }
	  while (start < haystack->len) {
	    if (str[start] == val) break;
	    start++;
	  }
	}
	break;
      case 2:
	{
	  p_wchar2 *str = STR2(haystack);
	  while (start < haystack->len) {
	    if (str[start] == (p_wchar2)val) break;
	    start++;
	  }
	}
	break;
      default:
	Pike_fatal("search(): Unsupported string shift: %d!\n",
	      haystack->size_shift);
	break;
      }
      if (start >= haystack->len) {
	start = -1;
      }
    } else {
      SIMPLE_BAD_ARG_ERROR("search", 2, "string | int");
    }
    pop_n_elems(args);
    push_int64(start);
    break;
  }

  case T_ARRAY:
    start=0;
    if(args > 2)
    {
      if(Pike_sp[2-args].type!=T_INT)
	SIMPLE_BAD_ARG_ERROR("search", 3, "int");

      start=Pike_sp[2-args].u.integer;
      if(start<0) {
	bad_arg_error("search", Pike_sp-args, args, 3, "int(0..)", Pike_sp+2-args,
		   "Start must be greater or equal to zero.\n");
      }
    }
    start=array_search(Pike_sp[-args].u.array,Pike_sp+1-args,start);
    pop_n_elems(args);
    push_int64(start);
    break;

  case T_MAPPING:
    if(args > 2) {
      mapping_search_no_free(Pike_sp,Pike_sp[-args].u.mapping,Pike_sp+1-args,Pike_sp+2-args);
    } else {
      mapping_search_no_free(Pike_sp,Pike_sp[-args].u.mapping,Pike_sp+1-args,0);
    }
    free_svalue(Pike_sp-args);
    Pike_sp[-args]=*Pike_sp;
    dmalloc_touch_svalue(Pike_sp);
    pop_n_elems(args-1);
    return;

  case T_OBJECT:
    {
      struct program *p;
      if ((p = (Pike_sp[-args].u.object->prog))) {
	struct object *o = Pike_sp[-args].u.object;
	int id_level = p->inherits[Pike_sp[-args].subtype].identifier_level;
	int id;
	int next, ind;
	p = p->inherits[Pike_sp[-args].subtype].prog;

	/* NOTE: Fake lfun! */
	id = low_find_lfun(p, LFUN__SEARCH);
	/* First try lfun::_search(). */
	if (id >= 0) {
	  apply_low(o, id + id_level, args-1);
	  stack_pop_n_elems_keep_top(1);
	  return;
	}

	/* Check if we have an iterator. */
	if (((id = find_identifier("value", p)) >= 0) &&
	    ((next = find_identifier("next", p)) >= 0) &&
	    ((ind = find_identifier("index", p)) >= 0)) {
	  /* We have an iterator. */

	  id += id_level;
	  next += id_level;
	  ind += id_level;

	  /* Set the start position if needed. */
	  if (args > 2) {
	    int fun = find_identifier("set_index", p);
	    if (fun < 0)
	      Pike_error ("Cannot call unknown function \"%s\".\n", fun);
	    apply_low(o, fun + id_level, args-2);
	    pop_stack();
	  }

	  /* At this point we have two values on the stack. */

	  while(1) {
	    apply_low(o, id, 0);
	    if (is_eq(Pike_sp-2, Pike_sp-1)) {
	      /* Found. */
	      apply_low(o, ind, 0);
	      stack_pop_n_elems_keep_top(3);
	      return;
	    }
	    apply_low(o, next, 0);
	    if (UNSAFE_IS_ZERO(Pike_sp-1)) {
	      /* Not found. */
	      pop_n_elems(4);
	      /* FIXME: Should probably indicate not found in some other way.
	       *        On the other hand, the iterator should be false now.
	       */
	      push_undefined();	
	      return;
	    }
	    pop_n_elems(2);
	  }
	}
      }
    }
    /* FALL_THROUGH */
  default:
    SIMPLE_BAD_ARG_ERROR("search", 1, "string|array|mapping|object");
  }
}

/*! @decl int has_prefix(string s, string prefix)
 *!
 *!   Returns @expr{1@} if the string @[s] starts with @[prefix],
 *!   returns @expr{0@} (zero) otherwise.
 */
PMOD_EXPORT void f_has_prefix(INT32 args)
{
  struct pike_string *a, *b;

  if(args<2)
    SIMPLE_TOO_FEW_ARGS_ERROR("has_prefix", 2);
  if(Pike_sp[-args].type!=T_STRING)
    SIMPLE_ARG_TYPE_ERROR("has_prefix", 1, "string");
  if(Pike_sp[1-args].type!=T_STRING)
    SIMPLE_ARG_TYPE_ERROR("has_prefix", 2, "string");

  a = Pike_sp[-args].u.string;
  b = Pike_sp[1-args].u.string;

  /* First handle some common special cases. */
  if ((b->len > a->len) || (b->size_shift > a->size_shift)) {
    pop_n_elems(args);
    push_int(0);
    return;
  }

  /* Trivial cases. */
  if ((a == b)||(!b->len)) {
    pop_n_elems(args);
    push_int(1);
    return;
  }

  if (a->size_shift == b->size_shift) {
    int res = !MEMCMP(a->str, b->str, b->len << b->size_shift);
    pop_n_elems(args);
    push_int(res);
    return;
  }

  /* At this point a->size_shift > b->size_shift */
#define TWO_SHIFTS(S1, S2)	((S1)|((S2)<<2))
  switch(TWO_SHIFTS(a->size_shift, b->size_shift)) {
#define CASE_SHIFT(S1, S2) \
  case TWO_SHIFTS(S1, S2): \
    { \
      PIKE_CONCAT(p_wchar,S1) *s1 = PIKE_CONCAT(STR,S1)(a); \
      PIKE_CONCAT(p_wchar,S2) *s2 = PIKE_CONCAT(STR,S2)(b); \
      ptrdiff_t len = b->len; \
      while(len-- && (s1[len] == s2[len])) \
	; \
      pop_n_elems(args); \
      push_int(len == -1); \
      return; \
    } \
    break

    CASE_SHIFT(1,0);
    CASE_SHIFT(2,0);
    CASE_SHIFT(2,1);
  default:
    Pike_error("has_prefix(): Unexpected string shift combination: a:%d, b:%d!\n",
	  a->size_shift, b->size_shift);
    break;
  }
#undef CASE_SHIFT
#undef TWO_SHIFTS
}

/*! @decl int has_suffix(string s, string suffix)
 *!
 *!   Returns @expr{1@} if the string @[s] ends with @[suffix],
 *!   returns @expr{0@} (zero) otherwise.
 */
PMOD_EXPORT void f_has_suffix(INT32 args)
{
  struct pike_string *a, *b;

  if(args<2)
    SIMPLE_TOO_FEW_ARGS_ERROR("has_suffix", 2);
  if(Pike_sp[-args].type!=T_STRING)
    SIMPLE_ARG_TYPE_ERROR("has_suffix", 1, "string");
  if(Pike_sp[1-args].type!=T_STRING)
    SIMPLE_ARG_TYPE_ERROR("has_suffix", 2, "string");

  a = Pike_sp[-args].u.string;
  b = Pike_sp[1-args].u.string;

  /* First handle some common special cases. */
  if ((b->len > a->len) || (b->size_shift > a->size_shift)) {
    pop_n_elems(args);
    push_int(0);
    return;
  }

  /* Trivial cases. */
  if ((a == b)||(!b->len)) {
    pop_n_elems(args);
    push_int(1);
    return;
  }

  if (a->size_shift == b->size_shift) {
    int res = !MEMCMP(a->str + ((a->len - b->len)<<b->size_shift), b->str,
		      b->len << b->size_shift);
    pop_n_elems(args);
    push_int(res);
    return;
  }

  /* At this point a->size_shift > b->size_shift */
#define TWO_SHIFTS(S1, S2)	((S1)|((S2)<<2))
  switch(TWO_SHIFTS(a->size_shift, b->size_shift)) {
#define CASE_SHIFT(S1, S2) \
  case TWO_SHIFTS(S1, S2): \
    { \
      PIKE_CONCAT(p_wchar,S1) *s1 = PIKE_CONCAT(STR,S1)(a) + a->len - b->len; \
      PIKE_CONCAT(p_wchar,S2) *s2 = PIKE_CONCAT(STR,S2)(b); \
      ptrdiff_t len = b->len; \
      while(len-- && (s1[len] == s2[len])) \
	; \
      pop_n_elems(args); \
      push_int(len == -1); \
      return; \
    } \
    break

    CASE_SHIFT(1,0);
    CASE_SHIFT(2,0);
    CASE_SHIFT(2,1);
  default:
    Pike_error("has_prefix(): Unexpected string shift combination: a:%d, b:%d!\n",
	  a->size_shift, b->size_shift);
    break;
  }
#undef CASE_SHIFT
#undef TWO_SHIFTS
}

/*! @decl int has_index(string haystack, int index)
 *! @decl int has_index(array haystack, int index)
 *! @decl int has_index(mapping|multiset|object|program haystack, mixed index)
 *!
 *!   Search for @[index] in @[haystack].
 *!
 *! @returns
 *!   Returns @expr{1@} if @[index] is in the index domain of @[haystack],
 *!   or @expr{0@} (zero) if not found.
 *!
 *!   This function is equivalent to (but sometimes faster than):
 *!
 *! @code
 *! search(indices(haystack), index) != -1
 *! @endcode
 *!
 *! @note
 *!   A negative index in strings and arrays as recognized by the
 *!   index operators @expr{`[]()@} and @expr{`[]=()@} is not considered
 *!   a proper index by @[has_index()]
 *!
 *! @seealso
 *!   @[has_value()], @[indices()], @[search()], @[values()], @[zero_type()]
 */
PMOD_EXPORT void f_has_index(INT32 args)
{
  int t = 0;
  
  if(args < 2)
    SIMPLE_TOO_FEW_ARGS_ERROR("has_index", 2);
  if(args > 2)
    pop_n_elems(args-2);

  switch(Pike_sp[-2].type)
  {
    case T_STRING:
      if(Pike_sp[-1].type == T_INT)
	t = (0 <= Pike_sp[-1].u.integer && Pike_sp[-1].u.integer < Pike_sp[-2].u.string->len);
  
      pop_n_elems(args);
      push_int(t);
      break;
      
    case T_ARRAY:
      if(Pike_sp[-1].type == T_INT)
	t = (0 <= Pike_sp[-1].u.integer && Pike_sp[-1].u.integer < Pike_sp[-2].u.array->size);
      
      pop_n_elems(args);
      push_int(t);
      break;
      
    case T_MULTISET:
    case T_MAPPING:
      f_index(2);
      f_zero_type(1);
      
      if(Pike_sp[-1].type == T_INT)
	Pike_sp[-1].u.integer = !Pike_sp[-1].u.integer;
      else
	PIKE_ERROR("has_index",
		   "Function `zero_type' gave incorrect result.\n", Pike_sp, args);
      break;
      
    case T_OBJECT:
    case T_PROGRAM:
      /* FIXME: If the object behaves like an array, it will throw an
	 error for non-valid indices. Therefore it's not a good idea
	 to use the index operator.

	 Maybe we should use object->_has_index(index) provided that
	 the object implements it.
	 
	 /Noring */

      stack_swap();
      f_indices(1);
      stack_swap();
      f_search(2);
      
      if(Pike_sp[-1].type == T_INT)
	Pike_sp[-1].u.integer = (Pike_sp[-1].u.integer != -1);
      else
	PIKE_ERROR("has_index",
		   "Function `search' gave incorrect result.\n", Pike_sp, args);
      break;

    default:
      SIMPLE_ARG_TYPE_ERROR ("has_index", 1,
			     "string|array|mapping|multiset|object|program");
  }
}

/*! @decl int has_value(string haystack, string value)
 *! @decl int has_value(string haystack, int value)
 *! @decl int has_value(array|mapping|object|program haystack, mixed value)
 *!
 *!   Search for @[value] in @[haystack].
 *!
 *! @returns
 *!   Returns @expr{1@} if @[value] is in the value domain of @[haystack],
 *!   or @expr{0@} (zero) if not found.
 *!
 *!   This function is in all cases except when both arguments are strings
 *!   equivalent to (but sometimes faster than):
 *!
 *! @code
 *! search(values(@[haystack]), @[value]) != -1
 *! @endcode
 *!
 *!   If both arguments are strings, @[has_value()] is equivalent to:
 *!
 *! @code
 *! search(@[haystack], @[value]) != -1
 *! @endcode
 *!
 *! @seealso
 *!   @[has_index()], @[indices()], @[search()], @[values()], @[zero_type()]
 */
PMOD_EXPORT void f_has_value(INT32 args)
{
  if(args < 2)
    SIMPLE_TOO_FEW_ARGS_ERROR("has_value", 2);
  if(args > 2)
    pop_n_elems(args-2);

  switch(Pike_sp[-2].type)
  {
    case T_MAPPING:
      f_search(2);
      f_zero_type(1);
      
      if(Pike_sp[-1].type == T_INT)
	Pike_sp[-1].u.integer = !Pike_sp[-1].u.integer;
      else
	PIKE_ERROR("has_value",
		   "Function `zero_type' gave incorrect result.\n", Pike_sp, args);
      break;

    case T_PROGRAM:
    case T_OBJECT:
      /* FIXME: It's very sad that we always have to do linear search
	 with `values' in case of objects. The problem is that we cannot
	 use `search' directly since it's undefined whether it returns
	 -1 (array) or 0 (mapping) during e.g. some data type emulation.
	 
	 Maybe we should use object->_has_value(value) provided that
	 the object implements it.
	 
	 /Noring */

      /* FALL_THROUGH */

    case T_MULTISET:
      /* FIXME: This behavior for multisets isn't clean. It should be
       * compat only. */
      stack_swap();
      f_values(1);
      stack_swap();

      /* FALL_THROUGH */

    case T_STRING:   /* Strings are odd. /Noring */
    case T_ARRAY:
      f_search(2);

      if(Pike_sp[-1].type == T_INT)
	Pike_sp[-1].u.integer = (Pike_sp[-1].u.integer != -1);
      else
	PIKE_ERROR("has_value", "Search gave incorrect result.\n", Pike_sp, args);
      break;

    default:
      SIMPLE_ARG_TYPE_ERROR ("has_value", 1, "string|array|mapping|object|program");
  }
}

/*! @decl void add_constant(string name, mixed value)
 *! @decl void add_constant(string name)
 *!
 *!   Add a new predefined constant.
 *!
 *!   This function is often used to add builtin functions.
 *!   All programs compiled after the @[add_constant()] function has been
 *!   called can access @[value] by the name @[name].
 *!
 *!   If there is a constant called @[name] already, it will be replaced by
 *!   by the new definition. This will not affect already compiled programs.
 *!
 *!   Calling @[add_constant()] without a value will remove that name from
 *!   the list of constants. As with replacing, this will not affect already
 *!   compiled programs.
 *!
 *! @seealso
 *!   @[all_constants()]
 */
PMOD_EXPORT void f_add_constant(INT32 args)
{
  ASSERT_SECURITY_ROOT("add_constant");

  if(args<1)
    SIMPLE_TOO_FEW_ARGS_ERROR("add_constant", 1);

  if(Pike_sp[-args].type!=T_STRING)
    SIMPLE_BAD_ARG_ERROR("add_constant", 1, "string");

  if(args>1)
  {
    dmalloc_touch_svalue(Pike_sp-args+1);
    low_add_efun(Pike_sp[-args].u.string, Pike_sp-args+1);
  }else{
    low_add_efun(Pike_sp[-args].u.string, 0);
  }
  pop_n_elems(args);
}

/*! @decl string combine_path(string path, string ... paths)
 *! @decl string combine_path_unix(string path, string ... paths)
 *! @decl string combine_path_nt(string path, string ... paths)
 *! @decl string combine_path_amigaos(string path, string ... paths)
 *!
 *!   Concatenate a number of paths to a straightforward path without
 *!   any @expr{"//"@}, @expr{"/.."@} or @expr{"/."@}. If any path
 *!   argument is absolute then the result is absolute and the
 *!   preceding arguments are ignored. If the result is relative then
 *!   it might have leading @expr{".."@} components. If the last
 *!   nonempty argument ends with a directory separator then the
 *!   result ends with that too. If all components in a relative path
 *!   disappear due to subsequent @expr{".."@} components then the
 *!   result is @expr{"."@}.
 *!
 *!   @[combine_path_unix()] concatenates in UNIX style, which also is
 *!   appropriate for e.g. URL:s ("/" separates path components and
 *!   absolute paths start with "/"). @[combine_path_nt()]
 *!   concatenates according to NT filesystem conventions ("/" and "\"
 *!   separates path components and there might be a drive letter in
 *!   front of absolute paths). @[combine_path_amigaos()] concatenates
 *!   according to AmigaOS filesystem conventions.
 *!
 *!   @[combine_path()] is equivalent to @[combine_path_unix()] on UNIX-like
 *!   operating systems, and equivalent to @[combine_path_nt()] on NT-like
 *!   operating systems, and equivalent to @[combine_path_amigaos()] on
 *!   AmigaOS-like operating systems.
 *!
 *! @seealso
 *!   @[getcwd()], @[Stdio.append_path()]
 */

#define NT_COMBINE_PATH
#include "combine_path.h"

#define UNIX_COMBINE_PATH
#include "combine_path.h"

#define AMIGAOS_COMBINE_PATH
#include "combine_path.h"



/*! @decl int zero_type(mixed a)
 *!
 *!   Return the type of zero.
 *!
 *!   There are many types of zeros out there, or at least there are two.
 *!   One is returned by normal functions, and one returned by mapping
 *!   lookups and @[find_call_out()] when what you looked for wasn't there.
 *!   The only way to separate these two kinds of zeros is @[zero_type()].
 *!
 *! @returns
 *!   When doing a @[find_call_out()] or mapping lookup, @[zero_type()] on
 *!   this value will return @expr{1@} if there was no such thing present in
 *!   the mapping, or if no such @tt{call_out@} could be found.
 *!
 *!   If the argument to @[zero_type()] is a destructed object or a function
 *!   in a destructed object, @expr{2@} will be returned.
 *!
 *!   In all other cases @[zero_type()] will return @expr{0@} (zero).
 *!
 *! @seealso
 *!   @[find_call_out()]
 */
PMOD_EXPORT void f_zero_type(INT32 args)
{
  if(args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("zero_type",1);

  if((Pike_sp[-args].type==T_OBJECT || Pike_sp[-args].type==T_FUNCTION)
     && !Pike_sp[-args].u.object->prog)
  {
    pop_n_elems(args);
    push_int(NUMBER_DESTRUCTED);
  }
  else if(Pike_sp[-args].type != T_INT)
  {
    pop_n_elems(args);
    push_int(0);
  }
  else
  {
    pop_n_elems(args-1);
    Pike_sp[-1].u.integer=Pike_sp[-1].subtype;
    Pike_sp[-1].subtype=NUMBER_NUMBER;
  }
}

static int generate_zero_type(node *n)
{
  if(count_args(CDR(n)) != 1) return 0;
  if(do_docode(CDR(n),DO_NOT_COPY) != 1)
    Pike_fatal("Count args was wrong in generate_zero_type().\n");
  emit0(F_ZERO_TYPE);
  return 1;
}

/*
 * Some wide-strings related functions
 */

/*! @decl string string_to_unicode(string s)
 *!
 *!   Converts a string into an UTF16 compliant byte-stream.
 *!
 *! @note
 *!   Throws an error if characters not legal in an UTF16 stream are
 *!   encountered. Valid characters are in the range 0x00000 - 0x10ffff,
 *!   except for characters 0xfffe and 0xffff.
 *!
 *!   Characters in range 0x010000 - 0x10ffff are encoded using surrogates.
 *!
 *! @seealso
 *!   @[Locale.Charset.decoder()], @[string_to_utf8()], @[unicode_to_string()],
 *!   @[utf8_to_string()]
 */
PMOD_EXPORT void f_string_to_unicode(INT32 args)
{
  struct pike_string *in;
  struct pike_string *out = NULL;
  ptrdiff_t len;
  ptrdiff_t i;

  get_all_args("string_to_unicode", args, "%W", &in);

  switch(in->size_shift) {
  case 0:
    /* Just 8bit characters */
    len = in->len * 2;
    out = begin_shared_string(len);
    if (len) {
      MEMSET(out->str, 0, len);	/* Clear the upper (and lower) byte */
#ifdef PIKE_DEBUG
      if (d_flag) {
	for(i = len; i--;) {
	  if (out->str[i]) {
	    Pike_fatal("MEMSET didn't clear byte %ld of %ld\n",
		  PTRDIFF_T_TO_LONG(i+1),
		  PTRDIFF_T_TO_LONG(len));
	  }
	}
      }
#endif /* PIKE_DEBUG */
      for(i = in->len; i--;) {
	out->str[i * 2 + 1] = in->str[i];
      }
    }
    out = end_shared_string(out);
    break;
  case 1:
    /* 16 bit characters */
    /* FIXME: Should we check for 0xfffe & 0xffff here too? */
    len = in->len * 2;
    out = begin_shared_string(len);
#if (PIKE_BYTEORDER == 4321)
    /* Big endian -- We don't need to do much...
     *
     * FIXME: Future optimization: Check if refcount is == 1,
     * and perform sufficient magic to be able to convert in place.
     */
    MEMCPY(out->str, in->str, len);
#else
    /* Other endianness, may need to do byte-order conversion also. */
    {
      p_wchar1 *str1 = STR1(in);
      for(i = in->len; i--;) {
	unsigned INT32 c = str1[i];
	out->str[i * 2 + 1] = c & 0xff;
	out->str[i * 2] = c >> 8;
      }
    }
#endif
    out = end_shared_string(out);
    break;
  case 2:
    /* 32 bit characters -- Is someone writing in Klingon? */
    {
      p_wchar2 *str2 = STR2(in);
      ptrdiff_t j;
      len = in->len * 2;
      /* Check how many extra wide characters there are. */
      for(i = in->len; i--;) {
	if (str2[i] > 0xfffd) {
	  if (str2[i] < 0x10000) {
	    /* 0xfffe: Byte-order detection illegal character.
	     * 0xffff: Illegal character.
	     */
	    Pike_error("string_to_unicode(): Illegal character 0x%04x (index %ld) "
		  "is not a Unicode character.",
		  str2[i], PTRDIFF_T_TO_LONG(i));
	  }
	  if (str2[i] > 0x10ffff) {
	    Pike_error("string_to_unicode(): Character 0x%08x (index %ld) "
		  "is out of range (0x00000000..0x0010ffff).",
		  str2[i], PTRDIFF_T_TO_LONG(i));
	  }
	  /* Extra wide characters take two unicode characters in space.
	   * ie One unicode character extra.
	   */
	  len += 2;
	}
      }
      out = begin_shared_string(len);
      j = len;
      for(i = in->len; i--;) {
	unsigned INT32 c = str2[i];

	j -= 2;

	if (c > 0xffff) {
	  /* Use surrogates */
	  c -= 0x10000;
	  
	  out->str[j + 1] = c & 0xff;
	  out->str[j] = 0xdc | ((c >> 8) & 0x03);
	  j -= 2;
	  c >>= 10;
	  c |= 0xd800;
	}
	out->str[j + 1] = c & 0xff;
	out->str[j] = c >> 8;
      }
#ifdef PIKE_DEBUG
      if (j) {
	Pike_fatal("string_to_unicode(): Indexing error: len:%ld, j:%ld.\n",
	      PTRDIFF_T_TO_LONG(len), PTRDIFF_T_TO_LONG(j));
      }
#endif /* PIKE_DEBUG */
      out = end_shared_string(out);
    }
    break;
  default:
    Pike_error("string_to_unicode(): Bad string shift: %d!\n", in->size_shift);
    break;
  }
  pop_n_elems(args);
  push_string(out);
}

/*! @decl string unicode_to_string(string s)
 *!
 *!   Converts an UTF16 byte-stream into a string.
 *!
 *! @note
 *!   This function did not decode surrogates in Pike 7.2 and earlier.
 *!
 *! @seealso
 *!   @[Locale.Charset.decoder()], @[string_to_unicode()], @[string_to_utf8()],
 *!   @[utf8_to_string()]
 */
PMOD_EXPORT void f_unicode_to_string(INT32 args)
{
  struct pike_string *in;
  struct pike_string *out = NULL;
  ptrdiff_t len, i, num_surrogates = 0;
  int swab=0;
  p_wchar1 surr1, surr2, surrmask, *str0;

  get_all_args("unicode_to_string", args, "%S", &in);

  if (in->len & 1) {
    bad_arg_error("unicode_to_string", Pike_sp-args, args, 1, "string", Pike_sp-args,
		  "String length is odd.\n");
  }

  /* Check byteorder of UTF data */
  str0 = (p_wchar1 *)in->str;
  len = in->len;
  if (len && (str0[0] == 0xfeff)) {
    /* Correct byte order mark.  No swap necessary. */
    swab = 0;
    str0 ++;
    len -= 2;
  } else if (len && (str0[0] == 0xfffe)) {
    /* Reversed byte order mark.  Need to swap. */
    swab = 1;
    str0 ++;
    len -= 2;
  } else {
    /* No byte order mark.  Need to swap unless big endian */
#if (PIKE_BYTEORDER == 4321)
    swab = 0;
#else
    swab = 1;
#endif /* PIKE_BYTEORDER == 4321 */
  }

  /* Indentify surrogates by pre-swapped bitmasks, for efficiency */
  if (swab) {
    surr1 = 0xd8;
    surr2 = 0xdc;
    surrmask = 0xfc;
  } else {
    surr1 = 0xd800;
    surr2 = 0xdc00;
    surrmask = 0xfc00;
  }

  /* Count number of surrogates */
  for (i = len; i >= 4; i -= 2, str0++)
    if ( (str0[0]&surrmask) == surr1 &&
	 (str0[1]&surrmask) == surr2 )
      num_surrogates ++;

  /* Move str0 past the last word */
  str0++;

  len = len / 2 - num_surrogates;

  out = begin_wide_shared_string(len, (num_surrogates? 2 : 1));

  if (!swab) {
    /* Native endian */
    if (num_surrogates) {
      /* Convert surrogates */

      p_wchar2 *str2 = STR2(out);

      for (i = len; i--; --str0)

	if ((str0[-1]&surrmask) == surr2 && num_surrogates &&
	    (str0[-2]&surrmask) == surr1) {
	    
	  str2[i] = ((str0[-2]&0x3ff)<<10) + (str0[-1]&0x3ff) + 0x10000;

	  --str0;
	  --num_surrogates;

	} else

	  str2[i] = str0[-1];

    } else
    /*
     * FIXME: Future optimization: Perform sufficient magic
     * to do the conversion in place if the ref-count is == 1.
     */
      MEMCPY(out->str, (char *)(str0-len), len*2);
  } else {
    /* Reverse endian */
    
    if (num_surrogates) {
      /* Convert surrogates */

      p_wchar2 *str2 = STR2(out);

      for (i = len; i--; --str0) {

	if ((str0[-1]&surrmask) == surr2 && num_surrogates &&
	    (str0[-2]&surrmask) == surr1) {
	    
#if (PIKE_BYTEORDER == 4321)
	  str2[i] = ((((unsigned char *)str0)[-3]&3)<<18) +
	    (((unsigned char *)str0)[-4]<<10) +
	    ((((unsigned char *)str0)[-1]&3)<<8) +
	    ((unsigned char *)str0)[-2] +
	    0x10000;
#else /* PIKE_BYTEORDER != 4321 */
	  str2[i] = ((((unsigned char *)str0)[-4]&3)<<18) +
	    (((unsigned char *)str0)[-3]<<10) +
	    ((((unsigned char *)str0)[-2]&3)<<8) +
	    ((unsigned char *)str0)[-1] +
	    0x10000;
#endif /* PIKE_BYTEORDER == 4321 */
	  --str0;
	  --num_surrogates;

	} else {
#if (PIKE_BYTEORDER == 4321)
	  str2[i] = (((unsigned char *)str0)[-1]<<8) +
	    ((unsigned char *)str0)[-2];
#else /* PIKE_BYTEORDER != 4321 */
	  str2[i] = (((unsigned char *)str0)[-2]<<8) +
	    ((unsigned char *)str0)[-1];
#endif /* PIKE_BYTEORDER == 4321 */
	}
      }
    } else {
      /* No surrogates */

      p_wchar1 *str1 = STR1(out);

      for (i = len; i--; --str0) {
#if (PIKE_BYTEORDER == 4321)
	str1[i] = (((unsigned char *)str0)[-1]<<8) +
	  ((unsigned char *)str0)[-2];
#else /* PIKE_BYTEORDER != 4321 */
	str1[i] = (((unsigned char *)str0)[-2]<<8) +
	  ((unsigned char *)str0)[-1];
#endif /* PIKE_BYTEORDER == 4321 */
      }
    }
  }
  out = end_shared_string(out);
  pop_n_elems(args);
  push_string(out);
}

/*! @decl string string_to_utf8(string s)
 *! @decl string string_to_utf8(string s, int extended)
 *!
 *!   Converts a string into an UTF8 compliant byte-stream.
 *!
 *! @note
 *!   Throws an error if characters not valid in an UTF8 stream are
 *!   encountered. Valid characters are in the range 0x00000000 - 0x7fffffff.
 *!
 *!   If @[extended] is 1, characters in the range 0x80000000-0xfffffffff
 *!   will also be accepted, and encoded using a non-standard UTF8 extension.
 *!
 *! @seealso
 *!   @[Locale.Charset.encoder()], @[string_to_unicode()],
 *!   @[unicode_to_string()], @[utf8_to_string()]
 */
PMOD_EXPORT void f_string_to_utf8(INT32 args)
{
  ptrdiff_t len;
  struct pike_string *in;
  struct pike_string *out;
  ptrdiff_t i,j;
  INT_TYPE extended = 0;

  get_all_args("string_to_utf8", args, "%W.%i", &in, &extended);

  len = in->len;

  for(i=0; i < in->len; i++) {
    unsigned INT32 c = index_shared_string(in, i);
    if (c & ~0x7f) {
      /* 8bit or more. */
      len++;
      if (c & ~0x7ff) {
	/* 12bit or more. */
	len++;
	if (c & ~0xffff) {
	  /* 17bit or more. */
	  len++;
	  if (c & ~0x1fffff) {
	    /* 22bit or more. */
	    len++;
	    if (c & ~0x3ffffff) {
	      /* 27bit or more. */
	      len++;
	      if (c & ~0x7fffffff) {
		/* 32bit or more. */
		if (!extended) {
		  Pike_error("string_to_utf8(): "
			"Value 0x%08x (index %ld) is larger than 31 bits.\n",
			c, PTRDIFF_T_TO_LONG(i));
		}
		len++;
		/* FIXME: Needs fixing when we get 64bit chars... */
	      }
	    }
	  }
	}
      }
    }
  }
  if (len == in->len) {
    /* 7bit string -- already valid utf8. */
    pop_n_elems(args - 1);
    return;
  }
  out = begin_shared_string(len);

  for(i=j=0; i < in->len; i++) {
    unsigned INT32 c = index_shared_string(in, i);
    if (!(c & ~0x7f)) {
      /* 7bit */
      out->str[j++] = c;
    } else if (!(c & ~0x7ff)) {
      /* 11bit */
      out->str[j++] = 0xc0 | (c >> 6);
      out->str[j++] = 0x80 | (c & 0x3f);
    } else if (!(c & ~0xffff)) {
      /* 16bit */
      out->str[j++] = 0xe0 | (c >> 12);
      out->str[j++] = 0x80 | ((c >> 6) & 0x3f);
      out->str[j++] = 0x80 | (c & 0x3f);
    } else if (!(c & ~0x1fffff)) {
      /* 21bit */
      out->str[j++] = 0xf0 | (c >> 18);
      out->str[j++] = 0x80 | ((c >> 12) & 0x3f);
      out->str[j++] = 0x80 | ((c >> 6) & 0x3f);
      out->str[j++] = 0x80 | (c & 0x3f);
    } else if (!(c & ~0x3ffffff)) {
      /* 26bit */
      out->str[j++] = 0xf8 | (c >> 24);
      out->str[j++] = 0x80 | ((c >> 18) & 0x3f);
      out->str[j++] = 0x80 | ((c >> 12) & 0x3f);
      out->str[j++] = 0x80 | ((c >> 6) & 0x3f);
      out->str[j++] = 0x80 | (c & 0x3f);
    } else if (!(c & ~0x7fffffff)) {
      /* 31bit */
      out->str[j++] = 0xfc | (c >> 30);
      out->str[j++] = 0x80 | ((c >> 24) & 0x3f);
      out->str[j++] = 0x80 | ((c >> 18) & 0x3f);
      out->str[j++] = 0x80 | ((c >> 12) & 0x3f);
      out->str[j++] = 0x80 | ((c >> 6) & 0x3f);
      out->str[j++] = 0x80 | (c & 0x3f);
    } else {
      /* This and onwards is extended UTF-8 encoding. */
      /* 32 - 36bit */
      out->str[j++] = DO_NOT_WARN((char)0xfe);
      out->str[j++] = 0x80 | ((c >> 30) & 0x3f);
      out->str[j++] = 0x80 | ((c >> 24) & 0x3f);
      out->str[j++] = 0x80 | ((c >> 18) & 0x3f);
      out->str[j++] = 0x80 | ((c >> 12) & 0x3f);
      out->str[j++] = 0x80 | ((c >> 6) & 0x3f);
      out->str[j++] = 0x80 | (c & 0x3f);
    }
  }
#ifdef PIKE_DEBUG
  if (len != j) {
    Pike_fatal("string_to_utf8(): Calculated and actual lengths differ: "
	  "%ld != %ld\n",
	  PTRDIFF_T_TO_LONG(len), PTRDIFF_T_TO_LONG(j));
  }
#endif /* PIKE_DEBUG */
  out = end_shared_string(out);
  pop_n_elems(args);
  push_string(out);
}

/*! @decl string utf8_to_string(string s)
 *! @decl string utf8_to_string(string s, int extended)
 *!
 *!   Converts an UTF8 byte-stream into a string.
 *!
 *! @note
 *!   Throws an error if the stream is not a legal UFT8 byte-stream.
 *!
 *!   Accepts and decodes the extension used by @[string_to_utf8()], if
 *!   @[extended] is @expr{1@}.
 *!
 *! @seealso
 *!   @[Locale.Charset.encoder()], @[string_to_unicode()], @[string_to_utf8()],
 *!   @[unicode_to_string()]
 */
PMOD_EXPORT void f_utf8_to_string(INT32 args)
{
  struct pike_string *in;
  struct pike_string *out;
  int len = 0;
  int shift = 0;
  int i,j;
  INT_TYPE extended = 0;

  get_all_args("utf8_to_string", args, "%S.%i", &in, &extended);

  for(i=0; i < in->len; i++) {
    unsigned int c = ((unsigned char *)in->str)[i];
    len++;
    if (c & 0x80) {
      int cont = 0;
      if ((c & 0xc0) == 0x80) {
	Pike_error("utf8_to_string(): "
	      "Unexpected continuation block 0x%02x at index %d.\n",
	      c, i);
      }
      if ((c & 0xe0) == 0xc0) {
	/* 11bit */
	cont = 1;
	if (c & 0x1c) {
	  if (shift < 1) {
	    shift = 1;
	  }
	}
      } else if ((c & 0xf0) == 0xe0) {
	/* 16bit */
	cont = 2;
	if (shift < 1) {
	  shift = 1;
	}
      } else {
	shift = 2;
	if ((c & 0xf8) == 0xf0) {
	  /* 21bit */
	  cont = 3;
	} else if ((c & 0xfc) == 0xf8) {
	  /* 26bit */
	  cont = 4;
	} else if ((c & 0xfe) == 0xfc) {
	  /* 31bit */
	  cont = 5;
	} else if (c == 0xfe) {
	  /* 36bit */
	  if (!extended) {
	    Pike_error("utf8_to_string(): "
		  "Character 0xfe at index %d when not in extended mode.\n",
		  i);
	  }
	  cont = 6;
	} else {
	  Pike_error("utf8_to_string(): "
		"Unexpected character 0xff at index %d.\n",
		i);
	}
      }
      while(cont--) {
	i++;
	if (i >= in->len) {
	  Pike_error("utf8_to_string(): Truncated UTF8 sequence.\n");
	}
	c = ((unsigned char *)(in->str))[i];
	if ((c & 0xc0) != 0x80) {
	  Pike_error("utf8_to_string(): "
		"Expected continuation character at index %d (got 0x%02x).\n",
		i, c);
	}
      }
    }
  }
  if (len == in->len) {
    /* 7bit in == 7bit out */
    pop_n_elems(args-1);
    return;
  }

  out = begin_wide_shared_string(len, shift);
  
  for(j=i=0; i < in->len; i++) {
    unsigned int c = ((unsigned char *)in->str)[i];

    if (c & 0x80) {
      int cont = 0;

      /* NOTE: The tests aren't as paranoid here, since we've
       * already tested the string above.
       */
      if ((c & 0xe0) == 0xc0) {
	/* 11bit */
	cont = 1;
	c &= 0x1f;
      } else if ((c & 0xf0) == 0xe0) {
	/* 16bit */
	cont = 2;
	c &= 0x0f;
      } else if ((c & 0xf8) == 0xf0) {
	/* 21bit */
	cont = 3;
	c &= 0x07;
      } else if ((c & 0xfc) == 0xf8) {
	/* 26bit */
	cont = 4;
	c &= 0x03;
      } else if ((c & 0xfe) == 0xfc) {
	/* 31bit */
	cont = 5;
	c &= 0x01;
      } else {
	/* 36bit */
	cont = 6;
	c = 0;
      }
      while(cont--) {
	unsigned INT32 c2 = ((unsigned char *)(in->str))[++i] & 0x3f;
	c = (c << 6) | c2;
      }
    }
    low_set_index(out, j++, c);
  }
#ifdef PIKE_DEBUG
  if (j != len) {
    Pike_fatal("utf8_to_string(): Calculated and actual lengths differ: %d != %d\n",
	  len, j);
  }
#endif /* PIKE_DEBUG */
  out = end_shared_string(out);
  pop_n_elems(args);
  push_string(out);
}

/*! @decl string __parse_pike_type(string t)
 */
static void f_parse_pike_type( INT32 args )
{
  struct pike_type *t;

  if( !args || Pike_sp[-1].type != T_STRING ||
      Pike_sp[-1].u.string->size_shift )
    Pike_error( "__parse_pike_type requires a 8bit string as its first argument\n" );
  t = parse_type( (char *)STR0(Pike_sp[-1].u.string) );
  pop_stack();

  push_string(type_to_string(t));
  free_type(t);
}

/*! @decl mapping (string:mixed) all_constants()
 *!
 *!   Returns a mapping containing all global constants, indexed on the name
 *!   of the constant, and with the value of the constant as value.
 *!
 *! @seealso
 *!   @[add_constant()]
 */
PMOD_EXPORT void f_all_constants(INT32 args)
{
  pop_n_elems(args);
  ref_push_mapping(get_builtin_constants());
}

/*! @decl array allocate(int size)
 *! @decl array allocate(int size, mixed init)
 *!
 *!   Allocate an array of @[size] elements. If @[init] is specified
 *!   then each element is initialized by copying that value
 *!   recursively.
 *!
 *! @seealso
 *!   @[sizeof()], @[aggregate()], @[arrayp()]
 */
PMOD_EXPORT void f_allocate(INT32 args)
{
  INT_TYPE size;
  struct array *a;
  struct svalue *init;

  get_all_args("allocate", args, "%+.%*", &size, &init);
  if (size > MAX_INT32)
    SIMPLE_ARG_ERROR ("allocate", 1, "Integer too large to use as array size.");

  a=allocate_array(size);
  if(args>1)
  {
    INT32 e;
    push_array (a);
    for(e=0;e<size;e++)
      copy_svalues_recursively_no_free(a->item+e, init, 1, 0);
    a->type_field = 1 << init->type;
    stack_pop_n_elems_keep_top (args);
  }
  else {
    a->type_field = BIT_INT;
    pop_n_elems(args);
    push_array(a);
  }
}

/*! @decl object this_object(void|int level);
 *!
 *!   Returns the object we are currently evaluating in.
 *!
 *!   @[level] might be used to access the object of a surrounding
 *!   class: The object at level 0 is the current object, the object
 *!   at level 1 is the one belonging to the class that surrounds
 *!   the class that the object comes from, and so on.
 *!
 *! @note
 *!   As opposed to a qualified @expr{this@} reference such as
 *!   @expr{global::this@}, this function doesn't always access the
 *!   objects belonging to the lexically surrounding classes. If the
 *!   class containing the call has been inherited then the objects
 *!   surrounding the inheriting class are accessed.
 */
void f_this_object(INT32 args)
{
  int level, l;
  struct object *o;

  if (args) {
    if (Pike_sp[-args].type != T_INT || Pike_sp[-args].u.integer < 0)
      SIMPLE_BAD_ARG_ERROR ("this_object", 1, "a non-negative integer");
    level = Pike_sp[-args].u.integer;
  }
  else
    level = 0;

  pop_n_elems(args);

  o = Pike_fp->current_object;
  for (l = 0; l < level; l++) {
    struct program *p = o->prog;
    if (!p)
      Pike_error ("Object %d level(s) up is destructed - cannot get the parent.\n", l);
    if (!(p->flags & PROGRAM_USES_PARENT))
      /* FIXME: Ought to write out the object here. */
      Pike_error ("Object %d level(s) up lacks parent reference.\n", l);
    o = PARENT_INFO(o)->parent;
  }
  ref_push_object(o);
}

static node *optimize_this_object(node *n)
{
  int level = 0;

  if (CDR (n)) {
    struct program_state *state = Pike_compiler;
    if (CDR (n)->token != F_CONSTANT) {
      /* Not a constant expression. Make sure there are parent
       * pointers all the way. */
      int i;
      for (i = 0; i < compilation_depth; i++, state = state->previous)
	state->new_program->flags |= PROGRAM_USES_PARENT | PROGRAM_NEEDS_PARENT;
      return NULL;
    }
    else {
      int i;
#ifdef PIKE_DEBUG
      if (CDR (n)->u.sval.type != T_INT || CDR (n)->u.sval.u.integer < 0)
	Pike_fatal ("The type check for this_object() failed.\n");
#endif
      level = CDR (n)->u.sval.u.integer;
      for (i = MINIMUM(level, compilation_depth); i;
	   i--, state = state->previous) {
	state->new_program->flags |=
	  PROGRAM_USES_PARENT | PROGRAM_NEEDS_PARENT;
      }
    }
  }

  /* We can only improve the type when accessing the innermost object:
   * Since this_object always follows the object pointers it might not
   * access the lexically surrounding objects. Thus the
   * PROGRAM_USES_PARENT stuff above is a bit of a long shot, but it's
   * better than nothing. */
  if (!level) {
    free_type(n->type);
    type_stack_mark();
    /* We are rather sure that we contain ourselves... */
    /* push_object_type(1, Pike_compiler->new_program->id); */
    /* But it did not work yet, so... */
    push_object_type(0, Pike_compiler->new_program->id);
    n->type = pop_unfinished_type();
    if (n->parent) {
      n->parent->node_info |= OPT_TYPE_NOT_FIXED;
    }
  }

  return NULL;
}

static int generate_this_object(node *n)
{
  int level;

  if (CDR (n)) {
    if (CDR (n)->token != F_CONSTANT)
      /* Not a constant expression. Make a call to f_this_object. */
      return 0;
    else {
#ifdef PIKE_DEBUG
      if (CDR (n)->u.sval.type != T_INT || CDR (n)->u.sval.u.integer < 0)
	Pike_fatal ("The type check for this_object() failed.\n");
#endif
      level = CDR (n)->u.sval.u.integer;
    }
  }
  else level = 0;

  emit1(F_THIS_OBJECT, level);
  return 1;
}

/*! @decl void throw(mixed value)
 *!
 *!   Throw @[value] to a waiting @[catch].
 *!
 *!   If no @[catch] is waiting the global error handling will send the
 *!   value to @[master()->handle_error()].
 *!
 *!   If you throw an array with where the first index contains an error
 *!   message and the second index is a backtrace, (the output from
 *!   @[backtrace()]) then it will be treated exactly like a real error
 *!   by overlying functions.
 *!
 *! @seealso
 *!   @[catch]
 */
PMOD_EXPORT void f_throw(INT32 args)
{
  if(args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("throw", 1);
  assign_svalue(&throw_value,Pike_sp-args);
  pop_n_elems(args);
  throw_severity=0;
  pike_throw();
}

/*! @decl void exit(int returncode, void|string fmt, mixed ... extra)
 *!
 *!   Exit the whole Pike program with the given @[returncode].
 *!
 *!   Using @[exit()] with any other value than @expr{0@} (zero) indicates
 *!   that something went wrong during execution. See your system manuals
 *!   for more information about return codes.
 *!
 *!   The arguments after the @[returncode] will be used for a call to
 *!   @[werror] to output a message on stderr.
 *!
 *! @seealso
 *!   @[_exit()]
 */
PMOD_EXPORT void f_exit(INT32 args)
{
  static int in_exit=0;
  ASSERT_SECURITY_ROOT("exit");

  if(args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("exit", 1);

  if(Pike_sp[-args].type != T_INT)
    SIMPLE_BAD_ARG_ERROR("exit", 1, "int");

  if(in_exit) Pike_error("exit already called!\n");
  in_exit=1;

  if(args>1 && Pike_sp[1-args].type==T_STRING) {
    apply_svalue(simple_mapping_string_lookup(get_builtin_constants(),
					      "werror"), args-1);
    pop_stack();
    args=1;
  }

  assign_svalue(&throw_value, Pike_sp-args);
  throw_severity=THROW_EXIT;
  pike_throw();
}

/*! @decl void _exit(int returncode)
 *!
 *!   This function does the same as @[exit], but doesn't bother to clean
 *!   up the Pike interpreter before exiting. This means that no destructors
 *!   will be called, caches will not be flushed, file locks might not be
 *!   released, and databases might not be closed properly.
 *!
 *!   Use with extreme caution.
 *!
 *! @seealso
 *!   @[exit()]
 */
void f__exit(INT32 args)
{
  int code;
  ASSERT_SECURITY_ROOT("_exit");

  get_all_args("_exit", args, "%i", &code);

#ifdef PIKE_DEBUG
  {
    /* This will allow -p to work with _exit -Hubbe */
    exit_opcodes();
  }
#endif

  exit(code);
}

/*! @decl int time();
 *! @decl int time(int(1..1) one)
 *! @decl float time(int(2..) t)
 *!
 *!   This function returns the number of seconds since 00:00:00 UTC, 1 Jan 1970.
 *!
 *!   The second syntax does not query the system for the current
 *!   time. Instead the latest done by the pike process is returned
 *!   again. That's slightly faster but can be wildly inaccurate. Pike
 *!   queries the time internally when a thread has waited for
 *!   something, typically in @[sleep] or in a backend (see
 *!   @[Pike.Backend]).
 *!
 *!   The third syntax can be used to measure time more preciely than one
 *!   second. It return how many seconds has passed since @[t]. The precision
 *!   of this function varies from system to system.
 *!
 *! @seealso
 *!   @[ctime()], @[localtime()], @[mktime()], @[gmtime()],
 *!   @[System.gettimeofday], @[gethrtime]
 */
PMOD_EXPORT void f_time(INT32 args)
{
  if(!args)
  {
    GETTIMEOFDAY(&current_time);
  }else{
    if(Pike_sp[-args].type == T_INT && Pike_sp[-args].u.integer > 1)
    {
      struct timeval tmp;
      GETTIMEOFDAY(&current_time);
      tmp.tv_sec=Pike_sp[-args].u.integer;
      tmp.tv_usec=0;
      my_subtract_timeval(&tmp,&current_time);
      pop_n_elems(args);
      push_float( - (FLOAT_TYPE)tmp.tv_sec-((FLOAT_TYPE)tmp.tv_usec)/1000000 );
      return;
    }
  }
  pop_n_elems(args);
  push_int(current_time.tv_sec);
}

/*! @decl string crypt(string password)
 *! @decl int(0..1) crypt(string typed_password, string crypted_password)
 *!
 *!   This function crypts and verifies a short string (only the first
 *!   8 characters are significant).
 *!
 *!   The first syntax crypts the string @[password] into something that
 *!   is hopefully hard to decrypt.
 *!
 *!   The second syntax is used to verify @[typed_password] against
 *!   @[crypted_password], and returns @expr{1@} if they match, and
 *!   @expr{0@} (zero) otherwise.
 *!
 *! @note
 *!   Note that strings containing null characters will only be
 *!   processed up until the null character.
 */
PMOD_EXPORT void f_crypt(INT32 args)
{
  char salt[2];
  char *ret, *pwd, *saltp;
  char *choise =
    "cbhisjKlm4k65p7qrJfLMNQOPxwzyAaBDFgnoWXYCZ0123tvdHueEGISRTUV89./";

  get_all_args("crypt", args, "%s.%s", &pwd, &saltp);

  if(args>1)
  {
    if( Pike_sp[1-args].u.string->len < 2 )
    {
      pop_n_elems(args);
      push_int(0);
      return;
    }
  } else {
    unsigned int foo; /* Sun CC want's this :( */
    foo=my_rand();
    salt[0] = choise[foo % (size_t) strlen(choise)];
    foo=my_rand();
    salt[1] = choise[foo % (size_t) strlen(choise)];
    saltp=salt;
  }
#ifdef HAVE_CRYPT
  ret = (char *)crypt(pwd, saltp);
#else
#ifdef HAVE__CRYPT
  ret = (char *)_crypt(pwd, saltp);
#else
  ret = pwd;
#endif
#endif
  if(args < 2)
  {
    pop_n_elems(args);
    push_text(ret);
  }else{
    int i;
    i=!strcmp(ret,saltp);
    pop_n_elems(args);
    push_int(i);
  }
}

/*! @decl void destruct(void|object o)
 *!
 *!   Mark an object as destructed.
 *!
 *!   Calls @expr{o->destroy()@}, and then clears all variables in the
 *!   object. If no argument is given, the current object is destructed.
 *!
 *!   All pointers and function pointers to this object will become zero.
 *!   The destructed object will be freed from memory as soon as possible.
 */ 
PMOD_EXPORT void f_destruct(INT32 args)
{
  struct object *o;
  if(args)
  {
    if(Pike_sp[-args].type != T_OBJECT) {
      if ((Pike_sp[-args].type == T_INT) &&
	  (!Pike_sp[-args].u.integer)) {
	pop_n_elems(args);
	return;
      }
      SIMPLE_BAD_ARG_ERROR("destruct", 1, "object");
    }

    o=Pike_sp[-args].u.object;
  }else{
    if(!Pike_fp) {
      PIKE_ERROR("destruct",
		 "Destruct called without argument from callback function.\n",
		 Pike_sp, args);
    }
    o=Pike_fp->current_object;
  }
  if (o->prog && o->prog->flags & PROGRAM_NO_EXPLICIT_DESTRUCT)
    PIKE_ERROR("destruct", "Object can't be destructed explicitly.\n",
	       Pike_sp, args);
#ifdef PIKE_SECURITY
  if(!CHECK_DATA_SECURITY(o, SECURITY_BIT_DESTRUCT))
    Pike_error("Destruct permission denied.\n");
#endif
  debug_malloc_touch(o);
  destruct_object (o, DESTRUCT_EXPLICIT);
  pop_n_elems(args);
  destruct_objects_to_destruct();
}

/*! @decl array indices(string|array|mapping|multiset|object x)
 *!
 *!   Return an array of all valid indices for the value @[x].
 *!
 *!   For strings and arrays this is simply an array of ascending
 *!   numbers.
 *!
 *!   For mappings and multisets, the array may contain any value.
 *!
 *!   For objects which define @[lfun::_indices()] that return value
 *!   will be used.
 *!
 *!   For other objects an array with all non-static symbols will be
 *!   returned.
 *!
 *! @seealso
 *!   @[values()]
 */
PMOD_EXPORT void f_indices(INT32 args)
{
  ptrdiff_t size;
  struct array *a = NULL;

  if(args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("indices", 1);

  switch(Pike_sp[-args].type)
  {
  case T_STRING:
    size=Pike_sp[-args].u.string->len;
    goto qjump;

  case T_ARRAY:
    size=Pike_sp[-args].u.array->size;

  qjump:
    a=allocate_array_no_init(size,0);
    while(--size>=0)
    {
      /* Elements are already integers. */
      ITEM(a)[size].u.integer = DO_NOT_WARN((INT_TYPE)size);
    }
    a->type_field = BIT_INT;
    break;

  case T_MAPPING:
    a=mapping_indices(Pike_sp[-args].u.mapping);
    break;

  case T_MULTISET:
    a = multiset_indices (Pike_sp[-args].u.multiset);
    break;

  case T_OBJECT:
    /* FIXME: Object subtype! */
    a=object_indices(Pike_sp[-args].u.object);
    break;

  case T_PROGRAM:
    a = program_indices(Pike_sp[-args].u.program);
    break;

  case T_FUNCTION:
    {
      struct program *p = program_from_svalue(Pike_sp-args);
      if (p) {
	a = program_indices(p);
	break;
      }
    }
    /* FALL THROUGH */

  default:
    SIMPLE_BAD_ARG_ERROR("indices", 1,
			 "string|array|mapping|"
			 "multiset|object|program|function");
    return; /* make apcc happy */
  }
  pop_n_elems(args);
  push_array(a);
}

/* this should probably be moved to pike_types.c or something */
#define FIX_OVERLOADED_TYPE(n, lf, X) fix_overloaded_type(n,lf,X,CONSTANT_STRLEN(X))
/* FIXME: This function messes around with the implementation of pike_type,
 * and should probably be in pike_types.h instead.
 */
static node *fix_overloaded_type(node *n, int lfun, const char *deftype, int deftypelen)
{
  node **first_arg;
  struct pike_type *t, *t2;
  first_arg=my_get_arg(&_CDR(n), 0);
  if(!first_arg) return 0;
  t=first_arg[0]->type;
  if(!t || match_types(t, object_type_string))
  {
    /* Skip any name-nodes. */
    while(t && t->type == PIKE_T_NAME) {
      t = t->cdr;
    }

    /* FIXME: Ought to handle or-nodes here. */
    if(t && (t->type == T_OBJECT))
    {
      struct program *p = id_to_program(CDR_TO_INT(t));
      if(p)
      {
	int fun=FIND_LFUN(p, lfun);

	/* FIXME: function type string should really be compiled from
	 * the arguments so that or:ed types are handled correctly
	 */
	if(fun!=-1 &&
	   (t2 = check_call(function_type_string, ID_FROM_INT(p, fun)->type,
			    0)))
	{
	  free_type(n->type);
	  n->type = t2;
	  return 0;
	}
      }
    }

    /* If it is an object, it *may* be overloaded, we or with 
     * the deftype....
     */
#if 1
    if(deftype)
    {
      t2 = make_pike_type(deftype);
      t = n->type;
      n->type = or_pike_types(t,t2,0);
      free_type(t);
      free_type(t2);
    }
#endif
  }
  
  return 0; /* continue optimization */
}

static node *fix_indices_type(node *n)
{
  return FIX_OVERLOADED_TYPE(n, LFUN__INDICES, tArray);
}

static node *fix_values_type(node *n)
{
  return FIX_OVERLOADED_TYPE(n, LFUN__VALUES, tArray);
}

static node *fix_aggregate_mapping_type(node *n)
{
  struct pike_type *types[2] = { NULL, NULL };
  node *args = CDR(n);
  struct pike_type *new_type = NULL;

#ifdef PIKE_DEBUG
  if (l_flag > 2) {
    fprintf(stderr, "Fixing type for aggregate_mapping():\n");
    print_tree(n);

    fprintf(stderr, "Original type:");
    simple_describe_type(n->type);
  }
#endif /* PIKE_DEBUG */

  if (args) {
    node *arg = args;
    int argno = 0;

    /* Make it easier to find... */
    args->parent = 0;

    while(arg) {
#ifdef PIKE_DEBUG
      if (l_flag > 4) {
	fprintf(stderr, "Searching for arg #%d...\n", argno);
      }
#endif /* PIKE_DEBUG */
      if (arg->token == F_ARG_LIST) {
	if (CAR(arg)) {
	  CAR(arg)->parent = arg;
	  arg = CAR(arg);
	  continue;
	}
	if (CDR(arg)) {
	  CDR(arg)->parent = arg;
	  arg = CDR(arg);
	  continue;
	}
	/* Retrace */
      retrace:
#ifdef PIKE_DEBUG
	if (l_flag > 4) {
	  fprintf(stderr, "Retracing in search for arg %d...\n", argno);
	}
#endif /* PIKE_DEBUG */
	while (arg->parent &&
	       (!CDR(arg->parent) || (CDR(arg->parent) == arg))) {
	  arg = arg->parent;
	}
	if (!arg->parent) {
	  /* No more args. */
	  break;
	}
	arg = arg->parent;
	CDR(arg)->parent = arg;
	arg = CDR(arg);
	continue;
      }
      if (arg->token == F_PUSH_ARRAY) {
	/* FIXME: Should get the type from the pushed array. */
	/* FIXME: Should probably be fixed in las.c:fix_type_field() */
	/* FIXME: */
	MAKE_CONSTANT_TYPE(new_type, tMap(tMixed, tMixed));
	goto set_type;
      }
#ifdef PIKE_DEBUG
      if (l_flag > 4) {
	fprintf(stderr, "Found arg #%d:\n", argno);
	print_tree(arg);
	simple_describe_type(arg->type);
      }
#endif /* PIKE_DEBUG */
      do {
	if (types[argno]) {
	  struct pike_type *t = or_pike_types(types[argno], arg->type, 0);
	  free_type(types[argno]);
	  types[argno] = t;
#ifdef PIKE_DEBUG
	  if (l_flag > 4) {
	    fprintf(stderr, "Resulting type for arg #%d:\n", argno);
	    simple_describe_type(types[argno]);
	  }
#endif /* PIKE_DEBUG */
	} else {
	  copy_pike_type(types[argno], arg->type);
	}
	argno = !argno;
	/* Handle the special case where CAR & CDR are the same.
	 * Only occurrs with SHARED_NODES.
	 */
      } while (argno && arg->parent && CAR(arg->parent) == CDR(arg->parent));
      goto retrace;
    }

    if (argno) {
      yyerror("Odd number of arguments to aggregate_mapping().");
      goto done;
    }

    if (!types[0]) {
      MAKE_CONSTANT_TYPE(new_type, tMap(tZero, tZero));
      goto set_type;
    }

    type_stack_mark();
    push_finished_type(types[1]);
    push_finished_type(types[0]);
    push_type(T_MAPPING);
    new_type = pop_unfinished_type();
  } else {
    MAKE_CONSTANT_TYPE(new_type, tMap(tZero, tZero));
    goto set_type;
  }
  if (new_type) {
  set_type:
    free_type(n->type);
    n->type = new_type;

#ifdef PIKE_DEBUG
    if (l_flag > 2) {
      fprintf(stderr, "Result type: ");
      simple_describe_type(new_type);
    }
#endif /* PIKE_DEBUG */

    if (n->parent) {
      n->parent->node_info |= OPT_TYPE_NOT_FIXED;
    }    
  }
 done:
  if (args) {
    /* Not really needed, but... */
    args->parent = n;
  }
  if (types[1]) {
    free_type(types[1]);
  }
  if (types[0]) {
    free_type(types[0]);
  }
  return NULL;
}

/*! @decl array values(string|array|mapping|multiset|object x)
 *!
 *!   Return an array of all possible values from indexing the value
 *!   @[x].
 *!
 *!   For strings an array of int with the ISO10646 codes of the
 *!   characters in the string is returned.
 *!
 *!   For a multiset an array filled with ones (@expr{1@}) is
 *!   returned.
 *!
 *!   For arrays a single-level copy of @[x] is returned.
 *!
 *!   For mappings the array may contain any value.
 *!
 *!   For objects which define @[lfun::_values()] that return value
 *!   will be used.
 *!
 *!   For other objects an array with the values of all non-static
 *!   symbols will be returned.
 *!
 *! @seealso
 *!   @[indices()]
 */
PMOD_EXPORT void f_values(INT32 args)
{
  ptrdiff_t size;
  struct array *a = NULL;
  if(args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("values", 1);

  switch(Pike_sp[-args].type)
  {
  case T_STRING:
    size = Pike_sp[-args].u.string->len;
    a = allocate_array_no_init(size,0);
    while(--size >= 0)
    {
      /* Elements are already integers. */
      ITEM(a)[size].u.integer = index_shared_string(Pike_sp[-args].u.string, size);
    }
    a->type_field = BIT_INT;
    break;

  case T_ARRAY:
    a=copy_array(Pike_sp[-args].u.array);
    break;

  case T_MAPPING:
    a=mapping_values(Pike_sp[-args].u.mapping);
    break;

  case T_MULTISET:
    a = multiset_values (Pike_sp[-args].u.multiset);
    break;

  case T_OBJECT:
    /* FIXME: Object subtype! */
    a=object_values(Pike_sp[-args].u.object);
    break;

  case T_PROGRAM:
    a = program_values(Pike_sp[-args].u.program);
    break;

  case T_FUNCTION:
    {
      struct program *p = program_from_svalue(Pike_sp - args);
      if (p) {
	a = program_values(p);
	break;
      }
    }
    /* FALL THROUGH */

  default:
    SIMPLE_BAD_ARG_ERROR("values", 1,
			 "string|array|mapping|multiset|"
			 "object|program|function");
    return;  /* make apcc happy */
  }
  pop_n_elems(args);
  push_array(a);
}

/*! @decl object next_object(object o)
 *! @decl object next_object()
 *!
 *!   Returns the next object from the list of all objects.
 *!
 *!   All objects are stored in a linked list.
 *!
 *! @returns
 *!   If no arguments have been given @[next_object()] will return the first
 *!   object from the list.
 *!
 *!   If @[o] has been specified the object after @[o] on the list will be
 *!   returned.
 *!
 *! @note
 *!   This function is not recomended to use.
 *!
 *! @seealso
 *!   @[destruct()]
 */
PMOD_EXPORT void f_next_object(INT32 args)
{
  struct object *o;

  ASSERT_SECURITY_ROOT("next_object");

  if(args < 1)
  {
    o = first_object;
  }else{
    if(Pike_sp[-args].type != T_OBJECT)
      SIMPLE_BAD_ARG_ERROR("next_object", 1, "object");
    o = Pike_sp[-args].u.object->next;
  }
  while(o && !o->prog) o=o->next;
  pop_n_elems(args);
  if(!o)
  {
    push_int(0);
  }else{
    ref_push_object(o);
  }
}

/*! @decl program|function object_program(mixed o)
 *!
 *!   Return the program from which @[o] was instantiated. If the
 *!   object was instantiated from a class using parent references
 *!   the generating function will be returned.
 *!
 *!   If @[o] is not an object or has been destructed @expr{0@} (zero)
 *!   will be returned.
 */
PMOD_EXPORT void f_object_program(INT32 args)
{
  if(args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("object_program", 1);

  if(Pike_sp[-args].type == T_OBJECT)
  {
    struct object *o=Pike_sp[-args].u.object;
    struct program *p = o->prog;

#if 0
    /* This'd be nice, but it doesn't work well since the returned
     * function can't double as a program (program_from_svalue returns
     * NULL for it). */
    if (p == pike_trampoline_program) {
      struct pike_trampoline *t = (struct pike_trampoline *) o->storage;
      if (t->frame && t->frame->current_object) {
	add_ref (o = t->frame->current_object);
	pop_n_elems (args);
	push_function (o, t->func);
	return;
      }
    }
#endif

    if(p)
    {
      p = p->inherits[Pike_sp[-args].subtype].prog;
      /* FIXME: Does the following actually work for
       *        the object subtype case?
       */
      if((p->flags & PROGRAM_USES_PARENT) && 
	 PARENT_INFO(o)->parent &&
	 PARENT_INFO(o)->parent->prog)
      {
	INT32 id=PARENT_INFO(o)->parent_identifier;
	o=PARENT_INFO(o)->parent;
	add_ref(o);
	pop_n_elems(args);
	push_function(o, id);
	return;
      }else{
	add_ref(p);
	pop_n_elems(args);
	push_program(p);
	return;
      }
    }
  }

  pop_n_elems(args);
  push_int(0);
}

node *fix_object_program_type(node *n)
{
  /* Fix the type for a common case:
   *
   * object_program(object(is|implements foo))
   */
  node *nn;
  struct pike_type *new_type = NULL;

  if (!n->type) {
    copy_pike_type(n->type, program_type_string);
  }
  if (!(nn = CDR(n))) return NULL;
  if ((nn->token == F_ARG_LIST) && (!(nn = CAR(nn)))) return NULL;
  if (!nn->type) return NULL;

  /* Perform the actual conversion. */
  new_type = object_type_to_program_type(nn->type);
  if (new_type) {
    free_type(n->type);
    n->type = new_type;
  }
  return NULL;
}

/*! @decl string reverse(string s)
 *! @decl array reverse(array a)
 *! @decl int reverse(int i)
 *!
 *!   Reverses a string, array or int.
 *!
 *!   This function reverses a string, char by char, an array, value
 *!   by value or an int, bit by bit and returns the result. It's not
 *!   destructive on the input value.
 *!
 *!   Reversing strings can be particularly useful for parsing difficult
 *!   syntaxes which require scanning backwards.
 *!
 *! @seealso
 *!   @[sscanf()]
 */
PMOD_EXPORT void f_reverse(INT32 args)
{
  if(args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("reverse", 1);

  switch(Pike_sp[-args].type)
  {
  case T_STRING:
  {
    INT32 e;
    struct pike_string *s;
    s=begin_wide_shared_string(Pike_sp[-args].u.string->len,
			       Pike_sp[-args].u.string->size_shift);
    switch(Pike_sp[-args].u.string->size_shift)
    {
      case 0:
	for(e=0;e<Pike_sp[-args].u.string->len;e++)
	  STR0(s)[e]=STR0(Pike_sp[-args].u.string)[Pike_sp[-args].u.string->len-1-e];
	break;

      case 1:
	for(e=0;e<Pike_sp[-args].u.string->len;e++)
	  STR1(s)[e]=STR1(Pike_sp[-args].u.string)[Pike_sp[-args].u.string->len-1-e];
	break;

      case 2:
	for(e=0;e<Pike_sp[-args].u.string->len;e++)
	  STR2(s)[e]=STR2(Pike_sp[-args].u.string)[Pike_sp[-args].u.string->len-1-e];
	break;
    }
    s=low_end_shared_string(s);
    pop_n_elems(args);
    push_string(s);
    break;
  }

  case T_INT:
  {
    INT32 e;
    e=Pike_sp[-args].u.integer;
    e=((e & 0x55555555UL)<<1) + ((e & 0xaaaaaaaaUL)>>1);
    e=((e & 0x33333333UL)<<2) + ((e & 0xccccccccUL)>>2);
    e=((e & 0x0f0f0f0fUL)<<4) + ((e & 0xf0f0f0f0UL)>>4);
    e=((e & 0x00ff00ffUL)<<8) + ((e & 0xff00ff00UL)>>8);
    e=((e & 0x0000ffffUL)<<16)+ ((e & 0xffff0000UL)>>16);
    Pike_sp[-args].u.integer=e;
    pop_n_elems(args-1);
    break;
  }

  /* FIXME: Bignum support. */

  case T_ARRAY:
  {
    struct array *a;
    a=reverse_array(Pike_sp[-args].u.array);
    pop_n_elems(args);
    push_array(a);
    break;
  }

  default:
    SIMPLE_BAD_ARG_ERROR("reverse", 1, "string|int|array");    
  }
}

struct tupel
{
  int prefix;
  struct pike_string *ind;
  struct pike_string *val;
};

/* Magic, magic and more magic */
static int find_longest_prefix(char *str,
			       ptrdiff_t len,
			       int size_shift,
			       struct tupel *v,
			       INT32 a,
			       INT32 b)
{
  INT32 c,match=-1;
  ptrdiff_t tmp;

  check_c_stack(2048);

  while(a<b)
  {
    c=(a+b)/2;
    
    tmp=generic_quick_binary_strcmp(v[c].ind->str,
				    v[c].ind->len,
				    v[c].ind->size_shift,
				    str,
				    MINIMUM(len,v[c].ind->len),
				    size_shift);
    if(tmp<0)
    {
      INT32 match2=find_longest_prefix(str,
				       len,
				       size_shift,
				       v,
				       c+1,
				       b);
      if(match2!=-1) return match2;

      while(1)
      {
	if(v[c].prefix==-2)
	{
	  v[c].prefix=find_longest_prefix(v[c].ind->str,
					  v[c].ind->len,
					  v[c].ind->size_shift,
					  v,
					  0 /* can this be optimized? */,
					  c);
	}
	c=v[c].prefix;
	if(c<a || c<match) return match;

	if(!generic_quick_binary_strcmp(v[c].ind->str,
					v[c].ind->len,
					v[c].ind->size_shift,
					str,
					MINIMUM(len,v[c].ind->len),
					size_shift))
	   return c;
      }
    }
    else if(tmp>0)
    {
      b=c;
    }
    else
    {
      a=c+1; /* There might still be a better match... */
      match=c;
    }
  }
  return match;
}
			       

static int replace_sortfun(struct tupel *a,struct tupel *b)
{
  return DO_NOT_WARN((int)my_quick_strcmp(a->ind, b->ind));
}

struct replace_many_context
{
  struct string_builder ret;
  struct tupel *v;
};

static void free_replace_many_context (struct replace_many_context *ctx)
{
  free_string_builder (&ctx->ret);
  free ((char *) ctx->v);
}

static struct pike_string *replace_many(struct pike_string *str,
					struct array *from,
					struct array *to)
{
  INT32 e,num;
  ptrdiff_t s, length;
  struct replace_many_context ctx;
  ONERROR uwp;

  int set_start[256];
  int set_end[256];

  if(from->size != to->size)
    Pike_error("Replace must have equal-sized from and to arrays.\n");

  if(!from->size)
  {
    reference_shared_string(str);
    return str;
  }

  if( (from->type_field & ~BIT_STRING) &&
      (array_fix_type_field(from) & ~BIT_STRING) )
    Pike_error("replace: from array not array(string).\n");

  if( (to->type_field & ~BIT_STRING) &&
      (array_fix_type_field(to) & ~BIT_STRING) )
    Pike_error("replace: to array not array(string).\n");

  /* NOTE: The following test is needed, since sizeof(struct tupel)
   *       is somewhat greater than sizeof(struct svalue).
   */
  if (from->size > (ptrdiff_t)(LONG_MAX/sizeof(struct tupel)))
    Pike_error("Array too large (size %" PRINTPTRDIFFT "d "
	       "exceeds %" PRINTSIZET "u).\n",
	       from->size, (size_t)(LONG_MAX/sizeof(struct tupel)));
  ctx.v=(struct tupel *)xalloc(sizeof(struct tupel)*from->size);
  init_string_builder(&ctx.ret,str->size_shift);
  SET_ONERROR (uwp, free_replace_many_context, &ctx);

  for(num=e=0;e<from->size;e++)
  {
    if(ITEM(from)[e].u.string->size_shift > str->size_shift)
      continue;

    ctx.v[num].ind=ITEM(from)[e].u.string;
    ctx.v[num].val=ITEM(to)[e].u.string;
    ctx.v[num].prefix=-2; /* Uninitialized */
    num++;
  }

  fsort((char *)ctx.v,num,sizeof(struct tupel),(fsortfun)replace_sortfun);

  MEMSET(set_start, 0, sizeof(set_start));
  MEMSET(set_end, 0, sizeof(set_end));

  for(e=0;e<num;e++)
  {
    INT32 x;
    x=index_shared_string(ctx.v[num-1-e].ind,0);
    if((x<(INT32)NELEM(set_start)) && (x >= 0))
      set_start[x]=num-e-1;
    x=index_shared_string(ctx.v[e].ind,0);
    if((x<(INT32)NELEM(set_end)) && (x >= 0))
      set_end[x]=e+1;
  }

  length=str->len;

  for(s=0;length > 0;)
  {
    INT32 a,b;
    ptrdiff_t ch;

    ch=index_shared_string(str,s);
    if((ch<(ptrdiff_t)NELEM(set_end)) && (ch >= 0))
      b=set_end[ch];
    else
      b=num;

    if(b)
    {
      if((ch<(ptrdiff_t)NELEM(set_start)) && (ch >= 0))
	a=set_start[ch];
      else
	a=0;

      a=find_longest_prefix(str->str+(s << str->size_shift),
			    length,
			    str->size_shift,
			    ctx.v, a, b);

      if(a!=-1)
      {
	ch = ctx.v[a].ind->len;
	if(!ch) ch=1;
	s+=ch;
	length-=ch;
	string_builder_shared_strcat(&ctx.ret,ctx.v[a].val);
	continue;
      }
    }
    string_builder_putchar(&ctx.ret,
			   DO_NOT_WARN((INT32)ch));
    s++;
    length--;
  }

  UNSET_ONERROR (uwp);
  free((char *)ctx.v);
  return finish_string_builder(&ctx.ret);
}

/*! @decl string replace(string s, string from, string to)
 *! @decl string replace(string s, array(string) from, array(string) to)
 *! @decl string replace(string s, mapping(string:string) replacements)
 *! @decl array replace(array a, mixed from, mixed to)
 *! @decl mapping replace(mapping a, mixed from, mixed to)
 *!
 *!   Generic replace function.
 *!
 *!   This function can do several kinds replacement operations, the
 *!   different syntaxes do different things as follows:
 *! 
 *!   If all the arguments are strings, a copy of @[s] with every
 *!   occurrence of @[from] replaced with @[to] will be returned.
 *!   Special case: @[to] will be inserted between every character in
 *!   @[s] if @[from] is the empty string.
 *!
 *!   If the first argument is a string, and the others array(string), a string
 *!   with every occurrance of @[from][@i{i@}] in @[s] replaced with
 *!   @[to][@i{i@}] will be returned. Instead of the arrays @[from] and @[to]
 *!   a mapping equvivalent to @expr{@[mkmapping](@[from], @[to])@} can be
 *!   used.
 *!
 *!   If the first argument is an array or mapping, the values of @[a] which
 *!   are @[`==()] with @[from] will be replaced with @[to] destructively.
 *!   @[a] will then be returned.
 *!
 *! @note
 *!   Note that @[replace()] on arrays and mappings is a destructive operation.
 */
PMOD_EXPORT void f_replace(INT32 args)
{
  if(args < 3)
  {
     if (args==2 &&
	 Pike_sp[-1].type==T_MAPPING)
     {
       struct mapping *m = Pike_sp[-1].u.mapping;
       if( (m->data->ind_types & ~BIT_STRING) ||
	   (m->data->val_types & ~BIT_STRING) ) {
	 mapping_fix_type_field(Pike_sp[-1].u.mapping);
	 if( (m->data->ind_types & ~BIT_STRING) ||
	     (m->data->val_types & ~BIT_STRING) ) {
	   SIMPLE_BAD_ARG_ERROR("replace", 2, "mapping(string:string)");
	 }
       }

	stack_dup();
	f_indices(1);
	stack_swap();
	f_values(1);
	args++;
     }
     else
	SIMPLE_TOO_FEW_ARGS_ERROR("replace", 3);
  }

  switch(Pike_sp[-args].type)
  {
  case T_ARRAY:
  {
    array_replace(Pike_sp[-args].u.array,Pike_sp+1-args,Pike_sp+2-args);
    pop_n_elems(args-1);
    break;
  }

  case T_MAPPING:
  {
    mapping_replace(Pike_sp[-args].u.mapping,Pike_sp+1-args,Pike_sp+2-args);
    pop_n_elems(args-1);
    break;
  }

  case T_STRING:
  {
    struct pike_string *s;
    switch(Pike_sp[1-args].type)
    {
    default:
      SIMPLE_BAD_ARG_ERROR("replace", 2, "string|array");

    case T_STRING:
      if(Pike_sp[2-args].type != T_STRING)
	SIMPLE_BAD_ARG_ERROR("replace", 3, "string");

      s=string_replace(Pike_sp[-args].u.string,
		       Pike_sp[1-args].u.string,
		       Pike_sp[2-args].u.string);
      break;
      
    case T_ARRAY:
      if(Pike_sp[2-args].type != T_ARRAY)
	SIMPLE_BAD_ARG_ERROR("replace", 3, "array");

      s=replace_many(Pike_sp[-args].u.string,
		     Pike_sp[1-args].u.array,
		     Pike_sp[2-args].u.array);
    
    }
    pop_n_elems(args);
    push_string(s);
    break;
  }

  default:
    SIMPLE_BAD_ARG_ERROR("replace", 1, "array|mapping|string");
  }
}

node *optimize_replace(node *n)
{
  node **arg0 = my_get_arg(&_CDR(n), 0);
  struct pike_type *array_zero;
  struct pike_type *mapping_zero;

  MAKE_CONSTANT_TYPE(array_zero, tArr(tZero));
  MAKE_CONSTANT_TYPE(mapping_zero, tMap(tZero, tZero));

  if (arg0 &&
      (pike_types_le(array_zero, (*arg0)->type) ||
       pike_types_le(mapping_zero, (*arg0)->type))) {
    /* First argument might be an array or a mapping.
     *
     * replace() is destructive on arrays and mappings.
     */
    n->node_info |= OPT_SIDE_EFFECT;
    n->tree_info |= OPT_SIDE_EFFECT;
  } else {
    /* First argument is not an array or mapping,
     *
     * It must thus be a string.
     */
    node **arg1 = my_get_arg(&_CDR(n), 1);
    node **arg2 = my_get_arg(&_CDR(n), 2);

    if (arg1 && pike_types_le((*arg1)->type, array_type_string) &&
	arg2 && pike_types_le((*arg2)->type, array_type_string)) {
      /* The second and third arguments are arrays. */
      if (!is_const(*arg0) && is_const(*arg1) && is_const(*arg2)) {
	/* The second and third arguments are constants. */
	struct svalue *save_sp = Pike_sp;
	JMP_BUF tmp;
	if (SETJMP(tmp)) {
	  struct svalue thrown = throw_value;
	  struct pike_string *s;
	  throw_value.type = T_INT;
	  pop_n_elems(Pike_sp - save_sp);
	  yywarning("Optimizer failure in replace().");
	  s = format_exception_for_error_msg (&thrown);
	  if (s) {
	    yywarning ("%S", s);
	    free_string (s);
	  }
	  free_svalue(&thrown);
	} else {
	  extern struct program *multi_string_replace_program;
	  INT16 lfun;
	  struct object *replace_obj;
	  node *ret = NULL;
	  INT32 args = eval_low(*arg1,1);	/* NOTE: Addition splitted to ensure */
	  args += eval_low(*arg2,1);		/*       correct evaluation order.   */

	  replace_obj = clone_object(multi_string_replace_program, args);

	  push_object(replace_obj);
	  if (replace_obj->prog &&
	      ((lfun = FIND_LFUN(replace_obj->prog, LFUN_CALL)) != -1)) {
	    Pike_sp[-1].subtype = lfun;
	    Pike_sp[-1].type = PIKE_T_FUNCTION;
	    ADD_NODE_REF2(*arg0,
	      ret = mkapplynode(mkconstantsvaluenode(Pike_sp-1),
				 *arg0);
	    );

	    UNSETJMP(tmp);
	    pop_n_elems(Pike_sp - save_sp);

	    free_type(array_zero);
	    free_type(mapping_zero);

	    return ret;
	  }
	}
	UNSETJMP(tmp);
	pop_n_elems(Pike_sp - save_sp);
      }
    }
  }

  free_type(array_zero);
  free_type(mapping_zero);

  return NULL;
}

/*! @decl program compile(string source, object|void handler, @
 *!                       int|void major, int|void minor,@
 *!                       program|void target, object|void placeholder)
 *!
 *!   Compile a string to a program.
 *!
 *!   This function takes a piece of Pike code as a string and
 *!   compiles it into a clonable program.
 *!
 *!   The optional argument @[handler] is used to specify an alternative
 *!   error handler. If it is not specified the current master object will
 *!   be used.
 *!
 *!   The optional arguments @[major] and @[minor] are used to tell the
 *!   compiler to attempt to be compatible with Pike @[major].@[minor].
 *!
 *! @note
 *!   Note that @[source] must contain the complete source for a program.
 *!   It is not possible to compile a single expression or statement.
 *!
 *!   Also note that @[compile()] does not preprocess the program.
 *!   To preprocess the program you can use @[compile_string()] or
 *!   call the preprocessor manually by calling @[cpp()].
 *!
 *! @seealso
 *!   @[compile_string()], @[compile_file()], @[cpp()], @[master()]
 */
PMOD_EXPORT void f_compile(INT32 args)
{
  struct program *p=0;
  struct object *o;
  struct object *placeholder=0;
  int major=-1;
  int minor=-1;


  check_all_args("compile",args,
		 BIT_STRING,
		 BIT_VOID | BIT_INT | BIT_OBJECT,
		 BIT_VOID | BIT_INT,
		 BIT_VOID | BIT_INT,
		 BIT_VOID | BIT_INT | BIT_PROGRAM,
		 BIT_VOID | BIT_INT | BIT_OBJECT,
		 0);

  check_c_stack(65536);

  o=0;
  switch(args)
  {
    case 3:
      SIMPLE_BAD_ARG_ERROR("compile", 4, "int");
    default:
      if(Pike_sp[5-args].type == T_OBJECT) {
	if (Pike_sp[5-args].subtype) {
	  Pike_error("compile: "
		     "Subtyped placeholder objects are not supported yet.\n");
	}
	placeholder=Pike_sp[5-args].u.object;
      }

    case 5:
      if(Pike_sp[4-args].type == T_PROGRAM)
	p=Pike_sp[4-args].u.program;

    case 4:
      major=Pike_sp[2-args].u.integer;
      minor=Pike_sp[3-args].u.integer;
      
    case 2:
      if(Pike_sp[1-args].type == T_OBJECT) {
	if (Pike_sp[1-args].subtype) {
	  Pike_error("compile: "
		     "Subtyped handler objects are not supported yet.\n");
	}
	o=Pike_sp[1-args].u.object;
      }
      
    case 0: case 1: break;
  }

  p = compile(Pike_sp[-args].u.string, o, major, minor, p, placeholder);

  pop_n_elems(args);
  push_program(p);
}


/*! @decl array|mapping|multiset set_weak_flag(array|mapping|multiset m, @
 *!                                            int state)
 *!
 *!   Set the value @[m] to use weak or normal references in its
 *!   indices and/or values (whatever is applicable). @[state] is a
 *!   bitfield built by using @expr{|@} between the following flags:
 *!   
 *!   @int
 *!   	@value Pike.WEAK_INDICES
 *!   	  Use weak references for indices. Only applicable for
 *!   	  multisets and mappings.
 *!   	@value Pike.WEAK_VALUES
 *!   	  Use weak references for values. Only applicable for arrays
 *!   	  and mappings.
 *!   	@value Pike.WEAK
 *!   	  Shorthand for @expr{Pike.WEAK_INDICES|Pike.WEAK_VALUES@}.
 *!   @endint
 *!   
 *!   If a flag is absent, the corresponding field will use normal
 *!   references. @[state] can also be @expr{1@} as a compatibility
 *!   measure; it's treated like @[Pike.WEAK].
 *!
 *! @returns
 *!   @[m] will be returned.
 */
#define SETFLAG(FLAGS,FLAG,ONOFF) \
  FLAGS = (FLAGS & ~FLAG) | ( ONOFF ? FLAG : 0 )
void f_set_weak_flag(INT32 args)
{
  struct svalue *s;
  INT_TYPE ret;
  int flags;

  get_all_args("set_weak_flag",args,"%*%i",&s,&ret);

  if (ret == 1) ret = PIKE_WEAK_BOTH;

  switch(s->type)
  {
    case T_ARRAY:
      flags = array_get_flags(s->u.array);
      SETFLAG(flags,ARRAY_WEAK_FLAG,ret & PIKE_WEAK_VALUES);
      s->u.array = array_set_flags(s->u.array, flags);
      break;
    case T_MAPPING:
      flags = mapping_get_flags(s->u.mapping);
      flags = (flags & ~PIKE_WEAK_BOTH) | (ret & PIKE_WEAK_BOTH);
      mapping_set_flags(s->u.mapping, flags);
      break;
    case T_MULTISET:
      flags = multiset_get_flags (s->u.multiset);
      flags = (flags & ~PIKE_WEAK_BOTH) | (ret & PIKE_WEAK_BOTH);
      multiset_set_flags (s->u.multiset, flags);
      break;
    default:
      SIMPLE_BAD_ARG_ERROR("set_weak_flag",1,"array|mapping|multiset");
  }
  pop_n_elems(args-1);
}

/*! @decl int objectp(mixed arg)
 *!
 *!   Returns @expr{1@} if @[arg] is an object, @expr{0@} (zero) otherwise.
 *!
 *! @seealso
 *!   @[mappingp()], @[programp()], @[arrayp()], @[stringp()], @[functionp()],
 *!   @[multisetp()], @[floatp()], @[intp()]
 */
PMOD_EXPORT void f_objectp(INT32 args)
{
  if(args<1)
    SIMPLE_TOO_FEW_ARGS_ERROR("objectp", 1);
  if(Pike_sp[-args].type != T_OBJECT || !Pike_sp[-args].u.object->prog
#ifdef AUTO_BIGNUM
     || is_bignum_object(Pike_sp[-args].u.object)
#endif
     )
  {
    pop_n_elems(args);
    push_int(0);
  }else{
    pop_n_elems(args);
    push_int(1);
  }
}

/*! @decl int functionp(mixed arg)
 *!
 *!   Returns @expr{1@} if @[arg] is a function, @expr{0@} (zero) otherwise.
 *!
 *! @seealso
 *!   @[mappingp()], @[programp()], @[arrayp()], @[stringp()], @[objectp()],
 *!   @[multisetp()], @[floatp()], @[intp()]
 */
PMOD_EXPORT void f_functionp(INT32 args)
{
  int res = 0;
  if(args<1)
    SIMPLE_TOO_FEW_ARGS_ERROR("functionp", 1);
  if( Pike_sp[-args].type == T_FUNCTION &&
      (Pike_sp[-args].subtype == FUNCTION_BUILTIN || Pike_sp[-args].u.object->prog))
    res=1;
  pop_n_elems(args);
  push_int(res);
}

/*! @decl int callablep(mixed arg)
 *!
 *!   Returns @expr{1@} if @[arg] is a callable, @expr{0@} (zero) otherwise.
 *!
 *! @seealso
 *!   @[mappingp()], @[programp()], @[arrayp()], @[stringp()], @[objectp()],
 *!   @[multisetp()], @[floatp()], @[intp()]
 */
PMOD_EXPORT void f_callablep(INT32 args)
{
  int res = 0;
  if(args<1)
    SIMPLE_TOO_FEW_ARGS_ERROR("callablep", 1);

  switch( Pike_sp[-args].type )
  {
    case T_FUNCTION:
      if( Pike_sp[-args].subtype != FUNCTION_BUILTIN
	  && !Pike_sp[-args].u.object->prog)
	break;
      res = 1;
      break;
    case T_PROGRAM:
      res = 1;
      break;
    case T_OBJECT:
      {
	struct program *p;
	if((p = Pike_sp[-args].u.object->prog) &&
	   FIND_LFUN(p->inherits[Pike_sp[-args].subtype].prog,
		     LFUN_CALL ) != -1)
	  res = 1;
      }
      break;
    case T_ARRAY:
      /* FIXME: What about the recursive case? */
      array_fix_type_field(Pike_sp[-args].u.array);
      if( (Pike_sp[-args].u.array->type_field==BIT_CALLABLE) ||
	  !Pike_sp[-args].u.array->type_field) {
	res = 1;
      }
      else if( !(Pike_sp[-args].u.array->type_field &
		 ~(BIT_CALLABLE|BIT_INT)) ) {
	struct array *a = Pike_sp[-args].u.array;
	int i;
	res = 1;
	for(i=0; i<a->size; i++)
	  if( ITEM(a)[i].type == T_INT && ITEM(a)[i].u.integer ) {
	    res = 0;
	    break;
	  }
      }
      break;
  }
  pop_n_elems(args);
  push_int(res);
}
#ifndef HAVE_AND_USE_POLL
#undef HAVE_POLL
#endif

/*! @decl void sleep(int|float s, void|int abort_on_signal)
 *!
 *!   This function makes the program stop for @[s] seconds.
 *!
 *!   Only signal handlers can interrupt the sleep, and only when
 *!   @[abort_on_signal] is set. If more than one thread is running
 *!   the signal must be sent to the sleeping thread. Other callbacks
 *!   are not called during sleep.
 *!
 *!   If @[s] is zero then this thread will yield to other threads but
 *!   not sleep otherwise. Note that Pike yields internally at regular
 *!   intervals so it's normally not necessary to do this.
 *!
 *! @seealso
 *!   @[signal()], @[delay()]
 */
PMOD_EXPORT void f_sleep(INT32 args)
{
#ifdef HAVE_GETHRTIME
   hrtime_t t0,tv;
#else
   struct timeval t0,tv;
#endif

   double delay=0.0;
   int do_abort_on_signal;

#ifdef HAVE_GETHRTIME
   t0=tv=gethrtime();
#define GET_TIME_ELAPSED tv=gethrtime()
#define TIME_ELAPSED (tv-t0)*1e-9
#else
   GETTIMEOFDAY(&t0);
   tv=t0;
#define GET_TIME_ELAPSED GETTIMEOFDAY(&tv)
#define TIME_ELAPSED ((tv.tv_sec-t0.tv_sec) + (tv.tv_usec-t0.tv_usec)*1e-6)
#endif

#define FIX_LEFT() \
       GET_TIME_ELAPSED; \
       left = delay - TIME_ELAPSED;

   switch(Pike_sp[-args].type)
   {
      case T_INT:
	 delay=(double)Pike_sp[-args].u.integer;
	 break;

      case T_FLOAT:
	 delay=(double)Pike_sp[-args].u.float_number;
	 break;
   }

   /* Special case, sleep(0) means 'yield' */
   if(delay == 0.0)
   {
     check_threads_etc();
     pop_n_elems(args);
     return;
   }

   if(args > 1 && !UNSAFE_IS_ZERO(Pike_sp + 1-args))
   {
     do_abort_on_signal=1;
   }else{
     do_abort_on_signal=0;
   }

   pop_n_elems(args);

   while(1)
   {
     double left;
     /* THREADS_ALLOW may take longer time then POLL_SLEEP_LIMIT */
     THREADS_ALLOW();
     do {
       FIX_LEFT();
       if(left<=0.0) break;

#ifdef __NT__
       Sleep(DO_NOT_WARN((int)(left*1000)));
#elif defined(HAVE_POLL)
       {
	 /* MacOS X is stupid, and requires a non-NULL pollfd pointer. */
	 struct pollfd sentinel;
	 poll(&sentinel, 0, (int)(left*1000));
       }
#else
       {
	 struct timeval t3;
	 t3.tv_sec=left;
	 t3.tv_usec=(int)((left - (int)left)*1e6);
	 select(0,0,0,0,&t3);
       }
#endif
     } while(0);
     THREADS_DISALLOW();
     
     if(do_abort_on_signal) return;
     
     FIX_LEFT();
     
     if(left<=0.0)
     {
       break;
     }else{
       check_threads_etc();
     }
   }
}

#undef FIX_LEFT
#undef GET_TIME_ELAPSED
#undef TIME_ELAPSED

/*! @decl void delay(int|float s)
 *!
 *!   This function makes the program stop for @[s] seconds.
 *!
 *!   Only signal handlers can interrupt the sleep. Other callbacks are
 *!   not called during delay. Beware that this function uses busy-waiting
 *!   to achieve the highest possible accuracy.
 *!   
 *! @seealso
 *!   @[signal()], @[sleep()]
 */
PMOD_EXPORT void f_delay(INT32 args)
{
#define POLL_SLEEP_LIMIT 0.02

#ifdef HAVE_GETHRTIME
   hrtime_t t0,tv;
#else
   struct timeval t0,tv;
#endif

   double delay=0.0;
   int do_microsleep;
   int do_abort_on_signal;

#ifdef HAVE_GETHRTIME
   t0=tv=gethrtime();
#define GET_TIME_ELAPSED tv=gethrtime()
#define TIME_ELAPSED (tv-t0)*1e-9
#else
   GETTIMEOFDAY(&t0);
   tv=t0;
#define GET_TIME_ELAPSED GETTIMEOFDAY(&tv)
#define TIME_ELAPSED ((tv.tv_sec-t0.tv_sec) + (tv.tv_usec-t0.tv_usec)*1e-6)
#endif

#define FIX_LEFT() \
       GET_TIME_ELAPSED; \
       left = delay - TIME_ELAPSED; \
       if (do_microsleep) left-=POLL_SLEEP_LIMIT;

   switch(Pike_sp[-args].type)
   {
      case T_INT:
	 delay=(double)Pike_sp[-args].u.integer;
	 break;

      case T_FLOAT:
	 delay=(double)Pike_sp[-args].u.float_number;
	 break;
   }

   /* Special case, sleep(0) means 'yield' */
   if(delay == 0.0)
   {
     check_threads_etc();
     pop_n_elems(args);
     return;
   }

   if(args > 1 && !UNSAFE_IS_ZERO(Pike_sp + 1-args))
   {
     do_microsleep=0;
     do_abort_on_signal=1;
   }else{
     do_microsleep=delay<10;
     do_abort_on_signal=0;
   }

   pop_n_elems(args);

   if (delay>POLL_SLEEP_LIMIT || !do_microsleep)
   {
     while(1)
     {
       double left;
       /* THREADS_ALLOW may take longer time then POLL_SLEEP_LIMIT */
       THREADS_ALLOW();
       do {
	 FIX_LEFT();
	 if(left<=0.0) break;

#ifdef __NT__
	 Sleep(DO_NOT_WARN((int)(left*1000)));
#elif defined(HAVE_POLL)
	 {
	   /* MacOS X is stupid, and requires a non-NULL pollfd pointer. */
	   struct pollfd sentinel;
	   poll(&sentinel, 0, (int)(left*1000));
	 }
#else
	 {
	   struct timeval t3;
	   t3.tv_sec=left;
	   t3.tv_usec=(int)((left - (int)left)*1e6);
	   select(0,0,0,0,&t3);
	 }
#endif
       } while(0);
       THREADS_DISALLOW();

       if(do_abort_on_signal) return;
       
       FIX_LEFT();
       
       if(left<=0.0)
       {
	 break;
       }else{
	 check_threads_etc();
       }
     }
   }

   if (do_microsleep)
      while (delay>TIME_ELAPSED) 
	 GET_TIME_ELAPSED;
}

/*! @decl int gc()
 *!
 *!   Force garbage collection.
 *!
 *!   This function checks all the memory for cyclic structures such
 *!   as arrays containing themselves and frees them if appropriate.
 *!   It also frees up destructed objects and things with only weak
 *!   references.
 *!
 *!   Normally there is no need to call this function since Pike will
 *!   call it by itself every now and then. (Pike will try to predict
 *!   when 20% of all arrays/object/programs in memory is 'garbage'
 *!   and call this routine then.)
 *!
 *! @returns
 *!   The amount of garbage is returned. This is the number of arrays,
 *!   mappings, multisets, objects and programs that had no nonweak
 *!   external references during the garbage collection. It's normally
 *!   the same as the number of freed things, but there might be some
 *!   difference since destroy() functions are called during freeing,
 *!   which can cause more things to be freed or allocated.
 *!
 *! @seealso
 *!   @[Pike.gc_parameters], @[Debug.gc_status]
 */
void f_gc(INT32 args)
{
  pop_n_elems(args);
  push_int(do_gc(NULL, 1));
}

#ifdef TYPEP
#undef TYPEP
#endif


#define TYPEP(ID,NAME,TYPE,TYPE_NAME)					\
  PMOD_EXPORT void ID(INT32 args)					\
  {									\
    int t;								\
    struct program *p;							\
    if (args<1)								\
      SIMPLE_TOO_FEW_ARGS_ERROR(NAME, 1);				\
    if (Pike_sp[-args].type == T_OBJECT &&				\
	(p = Pike_sp[-args].u.object->prog))				\
    {									\
      int fun = FIND_LFUN(p->inherits[Pike_sp[-args].subtype].prog,	\
			  LFUN__IS_TYPE);				\
      if (fun != -1)							\
      {									\
	int id_level =							\
	  p->inherits[Pike_sp[-args].subtype].identifier_level;		\
	push_constant_text(TYPE_NAME);					\
	apply_low(Pike_sp[-args-1].u.object, fun + id_level, 1);	\
	stack_unlink(args);						\
	return;								\
      }									\
    }									\
    t = Pike_sp[-args].type == TYPE;					\
    pop_n_elems(args);							\
    push_int(t);							\
  }


/*! @decl int programp(mixed arg)
 *!
 *!   Returns @expr{1@} if @[arg] is a program, @expr{0@} (zero) otherwise.
 *!
 *! @seealso
 *!   @[mappingp()], @[intp()], @[arrayp()], @[stringp()], @[objectp()],
 *!   @[multisetp()], @[floatp()], @[functionp()]
 */
PMOD_EXPORT void f_programp(INT32 args)
{
  if(args<1)
    SIMPLE_TOO_FEW_ARGS_ERROR("programp", 1);
  switch(Pike_sp[-args].type)
  {
  case T_PROGRAM:
    pop_n_elems(args);
    push_int(1);
    return;

  case T_FUNCTION:
    if(program_from_function(Pike_sp-args))
    {
      pop_n_elems(args);
      push_int(1);
      return;
    }

  default:
    pop_n_elems(args);
    push_int(0);
  }
}

/*! @decl int intp(mixed arg)
 *!
 *!   Returns @expr{1@} if @[arg] is an int, @expr{0@} (zero) otherwise.
 *!
 *! @seealso
 *!   @[mappingp()], @[programp()], @[arrayp()], @[stringp()], @[objectp()],
 *!   @[multisetp()], @[floatp()], @[functionp()]
 */

/*! @decl int mappingp(mixed arg)
 *!
 *!   Returns @expr{1@} if @[arg] is a mapping, @expr{0@} (zero) otherwise.
 *!
 *! @seealso
 *!   @[intp()], @[programp()], @[arrayp()], @[stringp()], @[objectp()],
 *!   @[multisetp()], @[floatp()], @[functionp()]
 */

/*! @decl int arrayp(mixed arg)
 *!
 *!   Returns @expr{1@} if @[arg] is an array, @expr{0@} (zero) otherwise.
 *!
 *! @seealso
 *!   @[intp()], @[programp()], @[mappingp()], @[stringp()], @[objectp()],
 *!   @[multisetp()], @[floatp()], @[functionp()]
 */

/*! @decl int multisetp(mixed arg)
 *!
 *!   Returns @expr{1@} if @[arg] is a multiset, @expr{0@} (zero) otherwise.
 *!
 *! @seealso
 *!   @[intp()], @[programp()], @[arrayp()], @[stringp()], @[objectp()],
 *!   @[mappingp()], @[floatp()], @[functionp()]
 */

/*! @decl int stringp(mixed arg)
 *!
 *!   Returns @expr{1@} if @[arg] is a string, @expr{0@} (zero) otherwise.
 *!
 *! @seealso
 *!   @[intp()], @[programp()], @[arrayp()], @[multisetp()], @[objectp()],
 *!   @[mappingp()], @[floatp()], @[functionp()]
 */

/*! @decl int floatp(mixed arg)
 *!
 *!   Returns @expr{1@} if @[arg] is a float, @expr{0@} (zero) otherwise.
 *!
 *! @seealso
 *!   @[intp()], @[programp()], @[arrayp()], @[multisetp()], @[objectp()],
 *!   @[mappingp()], @[stringp()], @[functionp()]
 */


TYPEP(f_intp, "intp", T_INT, "int")
TYPEP(f_mappingp, "mappingp", T_MAPPING, "mapping")
TYPEP(f_arrayp, "arrayp", T_ARRAY, "array")
TYPEP(f_multisetp, "multisetp", T_MULTISET, "multiset")
TYPEP(f_stringp, "stringp", T_STRING, "string")
TYPEP(f_floatp, "floatp", T_FLOAT, "float")

/*! @decl array sort(array(mixed) index, array(mixed) ... data)
 *!
 *!   Sort arrays destructively.
 *!
 *!   This function sorts the array @[index] destructively. That means
 *!   that the array itself is changed and returned, no copy is created.
 *!
 *!   If extra arguments are given, they are supposed to be arrays of the
 *!   same size as @[index]. Each of these arrays will be modified in the
 *!   same way as @[index]. I.e. if index 3 is moved to position 0 in @[index]
 *!   index 3 will be moved to position 0 in all the other arrays as well.
 *!
 *!   The sort order is as follows:
 *!
 *!   @ul
 *!   @item
 *!     Integers and floats are sorted in ascending order.
 *!   @item
 *!     Strings are sorted primarily on the first characters that are
 *!     different, and secondarily with shorter strings before longer.
 *!     Different characters are sorted in ascending order on the
 *!     character value. Thus the sort order is not locale dependent.
 *!   @item
 *!     Arrays are sorted recursively on the first element. Empty
 *!     arrays are sorted before nonempty ones.
 *!   @item
 *!     Multisets are sorted recursively on the first index. Empty
 *!     multisets are sorted before nonempty ones.
 *!   @item
 *!     Objects are sorted in ascending order according to @[`<()],
 *!     @[`>()] and @[`==()].
 *!   @item
 *!     Other types aren't reordered.
 *!   @item
 *!     Different types are sorted in this order: Arrays, mappings,
 *!     multisets, objects, functions, programs, strings, types,
 *!     integers and floats. Note however that objects can control
 *!     their ordering wrt other types with @[`<], @[`>] and @[`==],
 *!     so this ordering of types only applies to objects without
 *!     those functions.
 *!   @endul
 *!
 *! @returns
 *!   The first argument is returned.
 *! 
 *! @note
 *!   The sort is stable, i.e. elements that are compare-wise equal
 *!   aren't reordered.
 *!
 *! @seealso
 *!   @[Array.sort_array], @[reverse()]
 */
PMOD_EXPORT void f_sort(INT32 args)
{
  INT32 e,*order;
  struct array *a;

  if(args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("sort", 1);
  if(Pike_sp[-args].type != T_ARRAY)
    SIMPLE_BAD_ARG_ERROR("sort", 1, "array");
  a = Pike_sp[-args].u.array;

  for(e=1;e<args;e++)
  {
    if(Pike_sp[e-args].type != T_ARRAY)
      SIMPLE_BAD_ARG_ERROR("sort", e+1, "array");

    if(Pike_sp[e-args].u.array->size != a->size)
      bad_arg_error("sort", Pike_sp-args, args, e+1, "array", Pike_sp+e-args,
		    "Argument %d has wrong size.\n", (e+1));
  }

  if(args > 1)
  {
    order = stable_sort_array_destructively(a);
    for(e=1;e<args;e++) order_array(Pike_sp[e-args].u.array,order);
    pop_n_elems(args-1);
    free((char *)order);
  }
  else {
    /* If there are only simple types in the array we can use unstable
     * sorting. */
    array_fix_unfinished_type_field (a);
    if (a->type_field & BIT_COMPLEX)
      free (stable_sort_array_destructively (a));
    else
      sort_array_destructively (a);
  }
}

/*! @decl array rows(mixed data, array index)
 *!
 *!   Select a set of rows from an array.
 *!
 *!   This function is en optimized equivalent to:
 *!
 *! @code
 *! map(@[index], lambda(mixed x) { return @[data][x]; })
 *! @endcode
 *!
 *!   That is, it indices data on every index in the array index and
 *!   returns an array with the results.
 *!
 *! @seealso
 *!   @[column()]
 */
PMOD_EXPORT void f_rows(INT32 args)
{
  INT32 e;
  struct array *a,*tmp;
  struct svalue *val;
  TYPE_FIELD types;

  get_all_args("rows", args, "%*%a", &val, &tmp);

  /* Optimization */
  if(tmp->refs == 1)
  {
    struct svalue sval;
    tmp->type_field = BIT_MIXED | BIT_UNFINISHED;
    types = 0;
    for(e=0;e<tmp->size;e++)
    {
      index_no_free(&sval, val, ITEM(tmp)+e);
      types |= 1 << sval.type;
      free_svalue(ITEM(tmp)+e);
      move_svalue (ITEM(tmp) + e, &sval);
    }
    tmp->type_field = types;
    stack_swap();
    pop_stack();
    return;
  }

  push_array(a=allocate_array(tmp->size));
  types = 0;
  for(e=0;e<a->size;e++) {
    index_no_free(ITEM(a)+e, val, ITEM(tmp)+e);
    types |= 1 << ITEM(a)[e].type;
  }
  a->type_field = types;
  
  Pike_sp--;
  dmalloc_touch_svalue(Pike_sp);
  pop_n_elems(args);
  push_array(a);
}

/*! @decl void verify_internals()
 *! @belongs Debug
 *!
 *!   Perform sanity checks.
 *!
 *!   This function goes through most of the internal Pike structures and
 *!   generates a fatal error if one of them is found to be out of order.
 *!   It is only used for debugging.
 *!
 *! @note
 *!   This function does a more thorough check if the Pike runtime has
 *!   been compiled with RTL debug.
 */
PMOD_EXPORT void f__verify_internals(INT32 args)
{
  INT32 tmp=d_flag;
  ASSERT_SECURITY_ROOT("_verify_internals");

  /* Keep below calls to low_thorough_check_short_svalue, or else we
   * get O(n!) or so, where n is the number of allocated things. */
  d_flag = 49;

#ifdef PIKE_DEBUG
  do_debug();			/* Calls do_gc() since d_flag > 3. */
#else
  do_gc(NULL, 1);
#endif
  d_flag=tmp;
  pop_n_elems(args);
}

#ifdef PIKE_DEBUG

/*! @decl int debug(int(0..) level)
 *! @belongs Debug
 *!
 *!   Set the run-time debug level.
 *!
 *! @returns
 *!   The old debug level will be returned.
 *! 
 *! @note
 *!   This function is only available if the Pike runtime has been compiled
 *!   with RTL debug.
 */
PMOD_EXPORT void f__debug(INT32 args)
{
  INT_TYPE d;

  ASSERT_SECURITY_ROOT("_debug");

  get_all_args("_debug", args, "%i", &d);
  pop_n_elems(args);
  push_int(d_flag);
  d_flag = d;
}

/*! @decl int optimizer_debug(int(0..) level)
 *! @belongs Debug
 *!
 *!   Set the optimizer debug level.
 *!
 *! @returns
 *!   The old optimizer debug level will be returned.
 *! 
 *! @note
 *!   This function is only available if the Pike runtime has been compiled
 *!   with RTL debug.
 */
PMOD_EXPORT void f__optimizer_debug(INT32 args)
{
  INT_TYPE l;

  ASSERT_SECURITY_ROOT("_optimizer_debug");

  get_all_args("_optimizer_debug", args, "%i", &l);
  pop_n_elems(args);
  push_int(l_flag);
  l_flag = l;
}


/*! @decl int assembler_debug(int(0..) level)
 *! @belongs Debug
 *!
 *!   Set the assembler debug level.
 *!
 *! @returns
 *!   The old assembler debug level will be returned.
 *! 
 *! @note
 *!   This function is only available if the Pike runtime has been compiled
 *!   with RTL debug.
 */
PMOD_EXPORT void f__assembler_debug(INT32 args)
{
  INT_TYPE l;

  ASSERT_SECURITY_ROOT("_assembler_debug");

  get_all_args("_assembler_debug", args, "%i", &l);
  pop_n_elems(args);
  push_int(a_flag);
  a_flag = l;
}


#ifdef YYDEBUG

/*! @decl int compiler_trace(int(0..) level)
 *! @belongs Debug
 *!
 *!   Set the compiler trace level.
 *!
 *! @returns
 *!   The old compiler trace level will be returned.
 *! 
 *! @note
 *!   This function is only available if the Pike runtime has been compiled
 *!   with RTL debug.
 */
PMOD_EXPORT void f__compiler_trace(INT32 args)
{
  extern int yydebug;
  INT_TYPE yyd;
  ASSERT_SECURITY_ROOT("_compiler_trace");

  get_all_args("_compiler_trace", args, "%i", &yyd);
  pop_n_elems(args);
  push_int(yydebug);
  yydebug = yyd;
}

#endif /* YYDEBUG */
#endif

#if defined(HAVE_LOCALTIME) || defined(HAVE_GMTIME)
static void encode_struct_tm(struct tm *tm)
{
  push_text("sec");
  push_int(tm->tm_sec);
  push_text("min");
  push_int(tm->tm_min);
  push_text("hour");
  push_int(tm->tm_hour);

  push_text("mday");
  push_int(tm->tm_mday);
  push_text("mon");
  push_int(tm->tm_mon);
  push_text("year");
  push_int(tm->tm_year);

  push_text("wday");
  push_int(tm->tm_wday);
  push_text("yday");
  push_int(tm->tm_yday);
  push_text("isdst");
  push_int(tm->tm_isdst);
}
#endif

#ifdef HAVE_GMTIME
/*! @decl mapping(string:int) gmtime(int timestamp)
 *!
 *!   Convert seconds since 00:00:00 UTC, Jan 1, 1970 into components.
 *!
 *!   This function works like @[localtime()] but the result is
 *!   not adjusted for the local time zone.
 *!
 *! @seealso
 *!   @[localtime()], @[time()], @[ctime()], @[mktime()]
 */
PMOD_EXPORT void f_gmtime(INT32 args)
{
  struct tm *tm;
  INT_TYPE tt;
  time_t t;

  get_all_args("gmtime", args, "%i", &tt);

  t = tt;
  tm = gmtime(&t);
  if (!tm) Pike_error ("gmtime() on this system cannot handle "
		       "the timestamp %ld.\n", (long) t);
  pop_n_elems(args);
  encode_struct_tm(tm);

  push_text("timezone");
  push_int(0);
  f_aggregate_mapping(20);
}
#endif

#ifdef HAVE_LOCALTIME
/*! @decl mapping(string:int) localtime(int timestamp)
 *!
 *!   Convert seconds since 00:00:00 UTC, 1 Jan 1970 into components.
 *!
 *! @returns
 *!   This function returns a mapping with the following components:
 *!   @mapping
 *!   	@member int(0..60) "sec"
 *!   	  Seconds over the minute.
 *!   	@member int(0..59) "min"
 *!   	  Minutes over the hour.
 *!   	@member int(0..23) "hour"
 *!   	  Hour of the day.
 *!   	@member int(1..31) "mday"
 *!   	  Day of the month.
 *!   	@member int(0..11) "mon"
 *!   	  Month of the year.
 *!   	@member int(0..) "year"
 *!   	  Year since 1900.
 *!   	@member int(0..6) "wday"
 *!   	  Day of week (0 = Sunday).
 *!   	@member int(0..365) "yday"
 *!   	  Day of the year.
 *!   	@member int(0..1) "isdst"
 *!   	  Is daylight savings time.
 *!   	@member int "timezone"
 *!   	  Offset from UTC, including daylight savings time adjustment.
 *!   @endmapping
 *!
 *! An error is thrown if the localtime(2) call failed on the system.
 *! It's platform dependent what time ranges that function can handle,
 *! e.g. Windows doesn't handle a negative @[timestamp].
 *!
 *! @note
 *!   Prior to Pike 7.5 the field @expr{"timezone"@} was sometimes not
 *!   present, and was sometimes not adjusted for daylight savings time.
 *!
 *! @seealso
 *!   @[Calendar], @[gmtime()], @[time()], @[ctime()], @[mktime()]
 */
PMOD_EXPORT void f_localtime(INT32 args)
{
  struct tm *tm;
  INT_TYPE tt;
  time_t t;

  get_all_args("localtime", args, "%i", &tt);

  t = tt;
  tm = localtime(&t);
  if (!tm) Pike_error ("localtime() on this system cannot handle "
		       "the timestamp %ld.\n", (long) t);
  pop_n_elems(args);
  encode_struct_tm(tm);

  push_text("timezone");
#ifdef STRUCT_TM_HAS_GMTOFF
  push_int(-tm->tm_gmtoff);
#elif defined(STRUCT_TM_HAS___TM_GMTOFF)
  push_int(-tm->__tm_gmtoff);
#elif defined(HAVE_EXTERNAL_TIMEZONE)
  /* Assume dst is one hour. */
  push_int(timezone - 3600*tm->tm_isdst);
#else
  /* Assume dst is one hour. */
  push_int(-3600*tm->tm_isdst);
#endif
  f_aggregate_mapping(20);
}
#endif

#if defined (HAVE_GMTIME) || defined (HAVE_LOCALTIME)
/* Returns the approximate difference in seconds between the
 * two struct tm's.
 */
static time_t my_tm_diff(const struct tm *t1, const struct tm *t2)
{
  time_t base;

  /* Win32 localtime() returns NULL for all dates before Jan 01, 1970. */
  if (!t2) return -1;

  base = (t1->tm_year - t2->tm_year) * 32140800;

  /* Overflow detection. (Should possibly be done on the other fields
   * too to cope with very large invalid dates.) */
  if ((t1->tm_year > t2->tm_year) && (base < 0))
    return MAX_TIME_T;
  if ((t1->tm_year < t2->tm_year) && (base > 0))
    return MIN_TIME_T;

  base +=
    (t1->tm_mon - t2->tm_mon) * 2678400 +
    (t1->tm_mday - t2->tm_mday) * 86400 +
    (t1->tm_hour - t2->tm_hour) * 3600 +
    (t1->tm_min - t2->tm_min) * 60 +
    (t1->tm_sec - t2->tm_sec);

  return base;
}

typedef struct tm *time_fn (const time_t *);

/* Inverse operation of gmtime or localtime. Unlike mktime(3), this
 * doesn't fill in a normalized time in target_tm.
 */
static int my_time_inverse (struct tm *target_tm, time_t *result, time_fn timefn)
{
  time_t current_ts = 0;
  time_t diff_ts, old_diff_ts = 0;
  struct tm *current_tm;
  int loop_cnt, tried_dst_displacement = 0;

  /* This loop seems stable, and usually converges in two passes.
   * The loop counter is for paranoia reasons.
   */
  for (loop_cnt = 0; loop_cnt < 20; loop_cnt++, old_diff_ts = diff_ts) {
    diff_ts = my_tm_diff(target_tm, current_tm = timefn(&current_ts));

#ifdef DEBUG_MY_TIME_INVERSE
    fprintf (stderr, "curr: y %d m %d d %d h %d m %d isdst %d\n",
	     current_tm->tm_year, current_tm->tm_mon, current_tm->tm_mday,
	     current_tm->tm_hour, current_tm->tm_min, current_tm->tm_isdst);
    fprintf (stderr, "diff: %ld\n", (long) diff_ts);
#endif

    if (!current_tm) {
#ifdef DEBUG_MY_TIME_INVERSE
      fprintf (stderr, "outside range for timefn().\n");
#endif
      return 0;
    }

    if (!diff_ts) {
      /* Got a satisfactory time, but if target_tm has an opinion on
       * DST we should check if we can return an alternative in the
       * same DST zone, to cope with the overlapping DST adjustment at
       * fall. */
      if (target_tm->tm_isdst >= 0 &&
	  target_tm->tm_isdst != current_tm->tm_isdst &&
	  !tried_dst_displacement) {
	/* Offset the time a day and iterate some more (only once
	 * more, really), so that we approach the target time from the
	 * right direction. */
	if (target_tm->tm_isdst)
	  current_ts -= 24 * 3600;
	else
	  current_ts += 24 * 3600;
	tried_dst_displacement = 1;
#ifdef DEBUG_MY_TIME_INVERSE
	fprintf (stderr, "dst displacement\n");
#endif
	continue;
      }
      break;
    }

    if (diff_ts == -old_diff_ts) {
      /* We're oscillating, which means the time in target_tm is
       * invalid. mktime(3) maps to the closest "convenient" valid
       * time in that case, so let's do that too. */
      if (diff_ts > -24 * 3600 && diff_ts < 24 * 3600) {
	/* The oscillation is not whole days, so it must be due to the
	 * DST gap in the spring. Prefer the same DST zone as the
	 * target tm, if it has one set. */
	if (target_tm->tm_isdst >= 0) {
	  if (target_tm->tm_isdst != current_tm->tm_isdst)
	    current_ts += diff_ts;
#ifdef DEBUG_MY_TIME_INVERSE
	  fprintf (stderr, "spring dst gap\n");
#endif
	  break;
	}
      }
      /* The oscillation is due to the gap between the 31 day months
       * used in my_tm_diff and the actual month lengths. In that case
       * it's correct to always go forward so that e.g. April 31st
       * maps to May 1st. */
      if (diff_ts > 0)
	current_ts += diff_ts;
#ifdef DEBUG_MY_TIME_INVERSE
      fprintf (stderr, "end of month gap\n");
#endif
      break;
    }

    if (INT_TYPE_ADD_OVERFLOW (current_ts, diff_ts)) {
      if (diff_ts > 0 && current_ts < MAX_TIME_T)
	current_ts = MAX_TIME_T;
      else if (diff_ts < 0 && current_ts > MIN_TIME_T)
	current_ts = MIN_TIME_T;
      else {
#ifdef DEBUG_MY_TIME_INVERSE
	fprintf (stderr, "outside time_t range\n");
#endif
	return 0;
      }
    }
    else
      current_ts += diff_ts;
  }

#ifdef DEBUG_MY_TIME_INVERSE
  fprintf (stderr, "res: y %d m %d d %d h %d m %d isdst %d\n",
	   current_tm->tm_year, current_tm->tm_mon, current_tm->tm_mday,
	   current_tm->tm_hour, current_tm->tm_min, current_tm->tm_isdst);
#endif
  *result = current_ts;
  return 1;
}
#endif /* HAVE_GMTIME || HAVE_LOCALTIME */

#if defined (HAVE_MKTIME) || defined (HAVE_LOCALTIME)
/*! @decl int mktime(mapping(string:int) tm)
 *! @decl int mktime(int sec, int min, int hour, int mday, int mon, int year, @
 *!                  int|void isdst, int|void tz)
 *!
 *!   This function converts information about date and time into an integer
 *!   which contains the number of seconds since 00:00:00 UTC, Jan 1, 1970.
 *!
 *!   You can either call this function with a mapping containing the
 *!   following elements:
 *!   @mapping
 *!   	@member int(0..60) "sec"
 *!   	  Seconds over the minute.
 *!   	@member int(0..59) "min"
 *!   	  Minutes over the hour.
 *!   	@member int(0..23) "hour"
 *!   	  Hour of the day.
 *!   	@member int(1..31) "mday"
 *!   	  Day of the month.
 *!   	@member int(0..11) "mon"
 *!   	  Month of the year.
 *!   	@member int(0..) "year"
 *!   	  Year since 1900.
 *!   	@member int(0..1) "isdst"
 *!   	  Is daylight savings time.
 *!   	@member int "timezone"
 *!   	  The timezone offset from UTC in seconds. If left out, the time
 *!       will be calculated in the local timezone.
 *!   @endmapping
 *!
 *!   Or you can just send them all on one line as the second syntax suggests.
 *!
 *! @note
 *!   On some operating systems (notably AIX and Win32), dates before
 *!   00:00:00 UTC, Jan 1, 1970 are not supported.
 *!
 *!   On most systems, the supported range of dates are Dec 13, 1901
 *!   20:45:52 UTC through Jan 19, 2038 03:14:07 UTC (inclusive).
 *!
 *! @seealso
 *!   @[time()], @[ctime()], @[localtime()], @[gmtime()]
 */
PMOD_EXPORT void f_mktime (INT32 args)
{
  INT_TYPE sec, min, hour, mday, mon, year;
  INT_TYPE isdst = -1, tz;
  struct tm date;
  time_t retval;

  if (args<1)
    SIMPLE_TOO_FEW_ARGS_ERROR("mktime", 1);

  if(args == 1)
  {
    MEMSET(&date, 0, sizeof(date));

    push_text("sec");
    push_text("min");
    push_text("hour");
    push_text("mday");
    push_text("mon");
    push_text("year");
    push_text("isdst");
    push_text("timezone");
    f_aggregate(8);
    f_rows(2);
    Pike_sp--;
    dmalloc_touch_svalue(Pike_sp);
    push_array_items(Pike_sp->u.array);

    args=8;
  }

  get_all_args("mktime",args, "%i%i%i%i%i%i.%i%i",
	       &sec, &min, &hour, &mday, &mon, &year, &isdst, &tz);

  MEMSET(&date, 0, sizeof(date));
  date.tm_sec=sec;
  date.tm_min=min;
  date.tm_hour=hour;
  date.tm_mday=mday;
  date.tm_mon=mon;
  date.tm_year=year;
  date.tm_isdst=isdst;

  /* date.tm_zone = NULL; */

#ifdef HAVE_GMTIME
  if((args > 7) && (Pike_sp[7-args].subtype == NUMBER_NUMBER))
  {
    /* UTC-relative time. Use gmtime. */
    if (!my_time_inverse (&date, &retval, gmtime))
      PIKE_ERROR("mktime", "Time conversion failed.\n", Pike_sp, args);
    retval += tz;
  } else
#endif /* HAVE_GMTIME */

  {
#ifndef HAVE_GMTIME
#ifdef STRUCT_TM_HAS_GMTOFF
    /* BSD-style */
    date.tm_gmtoff = 0;
#else
#ifdef STRUCT_TM_HAS___TM_GMTOFF
    /* (Old) Linux-style */
    date.__tm_gmtoff = 0;
#else
    if((args > 7) && (Pike_sp[7-args].subtype == NUMBER_NUMBER))
    {
      /* Pre-adjust for the timezone.
       *
       * Note that pre-adjustment must be done on AIX for dates
       * near Jan 1, 1970, since AIX mktime(3) doesn't support
       * negative time.
       */
      date.tm_sec += tz
#ifdef HAVE_EXTERNAL_TIMEZONE
	- timezone
#endif /* HAVE_EXTERNAL_TIMEZONE */
	;
    }
#endif /* STRUCT_TM_HAS___TM_GMTOFF */
#endif /* STRUCT_TM_HAS_GMTOFF */
#endif  /* !HAVE_GMTIME */

#ifdef HAVE_MKTIME
    retval = mktime(&date);
    if (retval == -1)
#endif
    {
#ifdef HAVE_LOCALTIME
      /* mktime might fail on dates before 1970 (e.g. GNU libc 2.3.2),
       * so try our own inverse function with localtime.
       *
       * Note that localtime on Win32 will also fail for dates before 1970.
       */
      if (!my_time_inverse (&date, &retval, localtime))
#endif
	PIKE_ERROR("mktime", "Time conversion unsuccessful.\n", Pike_sp, args);
    }

#if !defined (HAVE_GMTIME) && (defined(STRUCT_TM_HAS_GMTOFF) || defined(STRUCT_TM_HAS___TM_GMTOFF))
    if((args > 7) && (Pike_sp[7-args].subtype == NUMBER_NUMBER))
    {
      /* Post-adjust for the timezone.
       *
       * Note that tm_gmtoff has the opposite sign of timezone.
       *
       * Note also that it must be post-adjusted, since the gmtoff
       * field is set by mktime(3).
       */
#ifdef STRUCT_TM_HAS_GMTOFF
      retval += tz + date.tm_gmtoff;
#else
      retval += tz + date.__tm_gmtoff;
#endif /* STRUCT_TM_HAS_GMTOFF */
    }
#endif /* !HAVE_GMTIME && (STRUCT_TM_HAS_GMTOFF || STRUCT_TM_HAS___TM_GMTOFF) */

#if 0
    /* Disabled since the adjustment done here with a hardcoded one
     * hour is bogus in many time zones. mktime(3) in GNU libc is
     * documented to normalize the date spec, which means that e.g.
     * asking for DST time in a non-DST zone will override tm_isdst.
     * /mast */
    if ((isdst != -1) && (isdst != date.tm_isdst)) {
      /* Some stupid libc's (Hi Linux!) don't accept that we've set isdst... */
      retval += 3600 * (isdst - date.tm_isdst);
    }
#endif	/* 0 */
  }

  pop_n_elems(args);
#if SIZEOF_TIME_T > SIZEOF_INT_TYPE
  push_int64 (retval);
#else
  push_int(retval);
#endif
}
#define GOT_F_MKTIME
#endif	/* HAVE_MKTIME || HAVE_LOCALTIME */

/* Parse a sprintf/sscanf-style format string */
static ptrdiff_t low_parse_format(p_wchar0 *s, ptrdiff_t slen)
{
  ptrdiff_t i;
  ptrdiff_t offset = 0;
  struct svalue *old_sp = Pike_sp;

  for (i=offset; i < slen; i++) {
    if (s[i] == '%') {
      ptrdiff_t j;
      if (i != offset) {
	push_string(make_shared_binary_string0(s + offset, i));
	if ((Pike_sp != old_sp+1) && (Pike_sp[-2].type == T_STRING)) {
	  /* Concat. */
	  f_add(2);
	}
      }

      for (j = i+1;j<slen;j++) {
	int c = s[j];

	switch(c) {
	  /* Flags */
	case '!':
	case '#':
	case '$':
	case '-':
	case '/':
	case '0':
	case '=':
	case '>':
	case '@':
	case '^':
	case '_':
	case '|':
	  continue;
	  /* Padding */
	case ' ':
	case '\'':
	case '+':
	case '~':
	  continue;
	  /* Attributes */
	case '.':
	case ':':
	case ';':
	  continue;
	  /* Attribute value */
	case '1': case '2': case '3': case '4': case '5':
	case '6': case '7': case '8': case '9':
	  continue;
	  /* Specials */
	case '%':
	  push_constant_text("%");
	  if ((Pike_sp != old_sp+1) && (Pike_sp[-2].type == T_STRING)) {
	    /* Concat. */
	    f_add(2);
	  }
	  break;
	case '{':
	  i = j + 1 + low_parse_format(s + j + 1, slen - (j+1));
	  f_aggregate(1);
	  if ((i + 2 >= slen) || (s[i] != '%') || (s[i+1] != '}')) {
	    Pike_error("parse_format(): Expected %%}.\n");
	  }
	  i += 2;
	  break;
	case '}':
	  f_aggregate(DO_NOT_WARN(Pike_sp - old_sp));
	  return i;
	  /* Set */
	case '[':
	  
	  break;
	  /* Argument */
	default:
	  break;
	}
	break;
      }
      if (j == slen) {
	Pike_error("parse_format(): Unterminated %%-expression.\n");
      }
      offset = i = j;
    }
  }

  if (i != offset) {
    push_string(make_shared_binary_string0(s + offset, i));
    if ((Pike_sp != old_sp+1) && (Pike_sp[-2].type == T_STRING)) {
      /* Concat. */
      f_add(2);
    }
  }

  f_aggregate(DO_NOT_WARN(Pike_sp - old_sp));
  return i;
}

/** @decl array parse_format(string fmt)
 **
 **   Parses a sprintf/sscanf-style format string
 */
static void f_parse_format(INT32 args)
{
  struct pike_string *s = NULL;
  struct array *a;
  ptrdiff_t len;

  get_all_args("parse_format", args, "%W", &s);

  len = low_parse_format(STR0(s), s->len);
  if (len != s->len) {
    Pike_error("parse_format(): Unexpected %%} in format string at offset %ld\n",
	  PTRDIFF_T_TO_LONG(len));
  }
#ifdef PIKE_DEBUG
  if (Pike_sp[-1].type != T_ARRAY) {
    Pike_fatal("parse_format(): Unexpected result from low_parse_format()\n");
  }
#endif /* PIKE_DEBUG */
  a = (--Pike_sp)->u.array;
  debug_malloc_touch(a);

  pop_n_elems(args);
  push_array(a);
}


/* Check if the string s[0..len[ matches the glob m[0..mlen[ */
static int does_match(struct pike_string *s,int j,
		      struct pike_string *m,int i)
{
  for (; i<m->len; i++)
  {
    switch (index_shared_string(m,i))
    {
     case '?':
       if(j++>=s->len) return 0;
       break;

     case '*': 
      i++;
      if (i==m->len) return 1;	/* slut */

      for (;j<s->len;j++)
	if (does_match(s,j,m,i))
	  return 1;

      return 0;

     default: 
       if(j>=s->len ||
	  index_shared_string(m,i)!=index_shared_string(s,j)) return 0;
       j++;
    }
  }
  return j==s->len;
}

/*! @decl int(0..1) glob(string glob, string str)
 *! @decl array(string) glob(string glob, array(string) arr)
 *!
 *!   Match strings against globs.
 *!
 *!   In a glob string a question sign matches any character and
 *!   an asterisk matches any string.
 *!
 *!   When the second argument is a string and @[str] matches
 *!   the glob @[glob] @expr{1@} will be returned, @expr{0@} (zero) otherwise.
 *!
 *!   If the second array is an array and array containing the strings in
 *!   @[arr] that match @[glob] will be returned.
 *!
 *! @seealso
 *!   @[sscanf()], @[Regexp]
 */
PMOD_EXPORT void f_glob(INT32 args)
{
  INT32 i,matches;
  struct array *a;
  struct pike_string *glob;

  if(args < 2)
    SIMPLE_TOO_FEW_ARGS_ERROR("glob", 2);

  if(args > 2)
    pop_n_elems(args-2);
  args=2;

  if (Pike_sp[-args].type!=T_STRING)
    SIMPLE_BAD_ARG_ERROR("glob", 1, "string");

  glob=Pike_sp[-args].u.string;

  switch(Pike_sp[1-args].type)
  {
  case T_STRING:
    i=does_match(Pike_sp[1-args].u.string,0,glob,0);
    pop_n_elems(2);
    push_int(i);
    break;
    
  case T_ARRAY:
    a=Pike_sp[1-args].u.array;

    if( (a->type_field & ~BIT_STRING) &&
	(array_fix_type_field(a) & ~BIT_STRING) )
      SIMPLE_BAD_ARG_ERROR("glob", 2, "string|array(string)");

    matches=0;
    check_stack(120);
    BEGIN_AGGREGATE_ARRAY (MINIMUM (a->size, 120)) {
      for(i=0;i<a->size;i++)
	if(does_match(ITEM(a)[i].u.string,0,glob,0))
	{
	  ref_push_string(ITEM(a)[i].u.string);
	  matches++;
	  DO_AGGREGATE_ARRAY (120);
	}
    } END_AGGREGATE_ARRAY;
    Pike_sp[-1].u.array->type_field = BIT_STRING;
    stack_pop_n_elems_keep_top (2);
    break;

  default:
    SIMPLE_BAD_ARG_ERROR("glob", 2, "string|array(string)");
  }
}

/* comb_merge */

/*! @module Array
 */

/*! @decl array(int) interleave_array(array(mapping(int:mixed)) tab)
 *!
 *!   Interleave a sparse matrix.
 *!
 *!   Returns an array with offsets that describe how to shift the
 *!   rows of @[tab] so that only at most one non-zero value exists in
 *!   every column.
 */
static void f_interleave_array(INT32 args)
{
  struct array *arr = NULL;
  struct array *min = NULL;
  struct array *order = NULL;
  int max = 0;
  int nelems = 0;
  int i;

  get_all_args("interleave_array", args, "%a", &arr);

  /* We're not interrested in any other arguments. */
  pop_n_elems(args-1);

  if( (arr->type_field & ~BIT_MAPPING) &&
      (array_fix_type_field(arr) & ~BIT_MAPPING) )
    SIMPLE_BAD_ARG_ERROR("interleave_array", 1, "array(mapping(int:mixed))");

  /* The order array */
  ref_push_array(arr);
  f_indices(1);
  order = Pike_sp[-1].u.array;

  /* The min array */
  push_array(min = allocate_array(arr->size));

  /* Initialize the min array */
  for (i = 0; i < arr->size; i++) {
    struct mapping_data *md;
    /* e and k are used by NEW_MAPPING_LOOP() */
    INT32 e;
    struct keypair *k;
    INT_TYPE low = MAX_INT_TYPE;
#ifdef PIKE_DEBUG
    if (ITEM(arr)[i].type != T_MAPPING) {
      Pike_error("interleave_array(): Element %d is not a mapping!\n", i);
    }
#endif /* PIKE_DEBUG */
    md = ITEM(arr)[i].u.mapping->data;
    NEW_MAPPING_LOOP(md) {
      if (k->ind.type != T_INT) {
	Pike_error("interleave_array(): Index not an integer in mapping %d!\n", i);
      }
      if (low > k->ind.u.integer) {
	low = k->ind.u.integer;
	if (low < 0) {
	  Pike_error("interleave_array(): Index %"PRINTPIKEINT"d in mapping %d is negative!\n",
		low, i);
	}
      }
      if (max < k->ind.u.integer) {
	max = k->ind.u.integer;
      }
      nelems++;
    }
    ITEM(min)[i].u.integer = low;
  }

  min->type_field = BIT_INT;
  ref_push_array(order);
  f_sort(2);	/* Sort the order array on the minimum index */

  /* State on stack now:
   *
   * array(mapping(int:mixed))	arr
   * array(int)			order
   * array(int)			min (now sorted)
   */

  /* Now we can start with the real work... */
  {
    char *tab;
    int size;
    int minfree = 0;

    /* Initialize the lookup table */
    max += 1;
    max *= 2;
    /* max will be the padding at the end. */
    size = (nelems + max) * 8;	/* Initial size */
    if (!(tab = malloc(size + max))) {
      SIMPLE_OUT_OF_MEMORY_ERROR("interleave_array", size+max);
    }
    MEMSET(tab, 0, size + max);

    for (i = 0; i < order->size; i++) {
      int low = ITEM(min)[i].u.integer;
      int j = ITEM(order)[i].u.integer;
      int offset = 0;
      int ok = 0;
      struct mapping *m;
      struct mapping_data *md;
      INT32 e;
      struct keypair *k;

      if (! m_sizeof(m = ITEM(arr)[j].u.mapping)) {
	/* Not available */
	ITEM(min)[i].u.integer = -1;
	continue;
      }

      if (low < minfree) {
	offset = minfree - low;
      } else {
	minfree = offset;
      }

      md = m->data;
      while (!ok) {
	ok = 1;
	NEW_MAPPING_LOOP(md) {
	  int ind = k->ind.u.integer;
	  if (tab[offset + ind]) {
	    ok = 0;
	    while (tab[++offset + ind])
	      ;
	  }
	}
      }
      NEW_MAPPING_LOOP(md) {
	tab[offset + k->ind.u.integer] = 1;
      }
      while(tab[minfree]) {
	minfree++;
      }
      ITEM(min)[i].u.integer = offset;

      /* Check need for realloc */
      if (offset >= size) {
	char *newtab = realloc(tab, size*2 + max);
	if (!newtab) {
	  free(tab);
	  Pike_error("interleave_array(): Couldn't extend table!\n");
	}
	tab = newtab;
	MEMSET(tab + size + max, 0, size);
	size = size * 2;
      }
    }
    free(tab);
  }

  /* We want these two to survive the stackpopping. */
  add_ref(min);
  add_ref(order);

  pop_n_elems(3);

  /* Return value */
  ref_push_array(min);

  /* Restore the order */
  push_array(order);
  push_array(min);
  f_sort(2);
  pop_stack();
}

/* longest_ordered_sequence */

static int find_gt(struct array *a, int i, int *stack, int top)
{
  struct svalue *x = a->item + i;
  int l,h;

  /* FIXME: Should it perhaps be is_ge below instead? */
  if (!top || !is_lt(x, a->item + stack[top - 1])) return top;

  l = 0;
  h = top;

  while (l < h) {
    int middle = (l + h)/2;
    if (!is_gt(a->item + stack[middle], x)) {
      l = middle+1;
    } else {
      h = middle;
    }
  }
  return l;
}

static struct array *longest_ordered_sequence(struct array *a)
{
  int *stack;
  int *links;
  int i, top=0, ltop=-1;
  struct array *res;
  ONERROR tmp;
  ONERROR tmp2;

  if(!a->size)
    return allocate_array(0);

  stack = malloc(sizeof(int)*a->size);
  links = malloc(sizeof(int)*a->size);

  if (!stack || !links)
  {
    if (stack) free(stack);
    if (links) free(links);
    return 0;
  }

  /* is_gt(), is_lt() and low_allocate_array() can generate errors. */

  SET_ONERROR(tmp, free, stack);
  SET_ONERROR(tmp2, free, links);

  for (i=0; i<a->size; i++) {
    int pos;

    pos = find_gt(a, i, stack, top);

    if (pos == top) {
      top++;
      ltop = i;
    }
    if (pos != 0)
      links[i] = stack[pos-1];
    else
      links[i] = -1;
    stack[pos] = i;
  }

  /* FIXME(?) memory unfreed upon error here */
  res = low_allocate_array(top, 0); 
  while (ltop != -1)
  {
    ITEM(res)[--top].u.integer = ltop;
    ltop = links[ltop];
  }
  res->type_field = BIT_INT;

  UNSET_ONERROR(tmp2);
  UNSET_ONERROR(tmp);

  free(stack);
  free(links);
  return res;
}

/*! @decl array(int) longest_ordered_sequence(array a)
 *!
 *!   Find the longest ordered sequence of elements.
 *!
 *!   This function returns an array of the indices in the longest
 *!   ordered sequence of elements in the array.
 *!
 *! @seealso
 *!   @[diff()]
 */
static void f_longest_ordered_sequence(INT32 args)
{
  struct array *a = NULL;

  get_all_args("Array.longest_ordered_sequence", args, "%a", &a);

  /* THREADS_ALLOW(); */

  a = longest_ordered_sequence(a);

  /* THREADS_DISALLOW(); */

  if (!a) {
    SIMPLE_OUT_OF_MEMORY_ERROR("Array.longest_ordered_sequence",
			       (int)sizeof(int *)*a->size*2);
  }

  pop_n_elems(args);
  push_array(a);
}

/**** diff ************************************************************/

static struct array* diff_compare_table(struct array *a,struct array *b,int *u)
{
   struct array *res;
   struct mapping *map;
   struct svalue *pval;
   int i;
   TYPE_FIELD types;

   if (u) {
     *u = 0;	/* Unique rows in array b */
   }

   map=allocate_mapping(256);
   push_mapping(map); /* in case of out of memory */

   for (i=0; i<b->size; i++)
   {
      pval=low_mapping_lookup(map,b->item+i);
      if (!pval)
      {
	 struct svalue val;
	 val.type=T_ARRAY;
	 val.u.array=low_allocate_array(1,1);
	 ITEM(val.u.array)[0].u.integer=i;
	 val.u.array->type_field = BIT_INT;
	 mapping_insert(map,ITEM(b)+i,&val);
	 free_svalue(&val);
	 if (u) {
	   (*u)++;
	 }
      }
      else
      {
	 pval->u.array=resize_array(pval->u.array,pval->u.array->size+1);
	 pval->u.array->item[pval->u.array->size-1].type=T_INT;
	 pval->u.array->item[pval->u.array->size-1].subtype=NUMBER_NUMBER;
	 pval->u.array->item[pval->u.array->size-1].u.integer=i;
      }
   }

   res=low_allocate_array(a->size,0);
   types = 0;

   for (i=0; i<a->size; i++)
   {
      pval=low_mapping_lookup(map,a->item+i);
      if (!pval)
      {
	 ITEM(res)[i].type=T_ARRAY;
	 add_ref(ITEM(res)[i].u.array=&empty_array);
	 types |= BIT_ARRAY;
      }
      else
      {
	 assign_svalue(ITEM(res)+i,pval);
	 types |= 1 << ITEM(res)[i].type;
      }
   }

   res->type_field = types;
   pop_stack();
   return res;
}

struct diff_magic_link
{ 
   int x;
   int refs;
   struct diff_magic_link *prev;
};

struct diff_magic_link_pool
{
   struct diff_magic_link *firstfree;
   struct diff_magic_link_pool *next;
   int firstfreenum;
   struct diff_magic_link dml[1];
};

struct diff_magic_link_head
{
  unsigned int depth;
  struct diff_magic_link *link;
};

#define DMLPOOLSIZE 16384

static int dmls=0;

static INLINE struct diff_magic_link_pool*
         dml_new_pool(struct diff_magic_link_pool **pools)
{
   struct diff_magic_link_pool *new;

   new=malloc(sizeof(struct diff_magic_link_pool)+
	      sizeof(struct diff_magic_link)*DMLPOOLSIZE);
   if (!new) return NULL; /* fail */

   new->firstfreenum=0;
   new->firstfree=NULL;
   new->next=*pools;
   *pools=new;
   return *pools;
}

static INLINE struct diff_magic_link* 
       dml_new(struct diff_magic_link_pool **pools)
{
   struct diff_magic_link *new;
   struct diff_magic_link_pool *pool;

   dmls++;

   if ( *pools && (new=(*pools)->firstfree) )
   {
      (*pools)->firstfree=new->prev;
      new->prev=NULL;
      return new;
   }

   pool=*pools;
   while (pool)
   {
      if (pool->firstfreenum<DMLPOOLSIZE)
	 return pool->dml+(pool->firstfreenum++);
      pool=pool->next;
   }

   if ( (pool=dml_new_pool(pools)) )
   {
      pool->firstfreenum=1;
      return pool->dml;
   }
   
   return NULL;
}	

static INLINE void dml_free_pools(struct diff_magic_link_pool *pools)
{
   struct diff_magic_link_pool *pool;

   while (pools)
   {
      pool=pools->next;
      free(pools);
      pools=pool;
   }
}

static INLINE void dml_delete(struct diff_magic_link_pool *pools,
			      struct diff_magic_link *dml)
{
  struct diff_magic_link *prev;
  while(1)
  {
    prev=dml->prev;
    dmls--;
    dml->prev=pools->firstfree;
    pools->firstfree=dml;
    if (prev && !--prev->refs)
      dml=prev;
    else
      break;
  }
}

static INLINE int diff_ponder_stack(int x,
				    struct diff_magic_link **dml,
				    int top)
{
   int middle,a,b;
   
   a=0; 
   b=top;
   while (b>a)
   {
      middle=(a+b)/2;
      if (dml[middle]->x<x) a=middle+1;
      else if (dml[middle]->x>x) b=middle;
      else return middle;
   }
   if (a<top && dml[a]->x<x) a++;
   return a;
}

static INLINE int diff_ponder_array(int x,
				    struct svalue *arr,
				    int top)
{
   int middle,a,b;
   
   a=0; 
   b=top;
   while (b>a)
   {
      middle=(a+b)/2;
      if (arr[middle].u.integer<x) a=middle+1;
      else if (arr[middle].u.integer>x) b=middle;
      else return middle;
   }
   if (a<top && arr[a].u.integer<x) a++;
   return a;
}

/*
 * The Grubba-Mirar Longest Common Sequence algorithm.
 *
 * This algorithm is O((Na * Nb / K)*lg(Na * Nb / K)), where:
 *
 *  Na == sizeof(a)
 *  Nb == sizeof(b)
 *  K  == sizeof(correlation(a,b))
 *
 * For binary data:
 *  K == 256 => O(Na * Nb * lg(Na * Nb)),
 *  Na ~= Nb ~= N => O(N² * lg(N))
 *
 * For ascii data:
 *  K ~= C * min(Na, Nb), C constant => O(max(Na, Nb)*lg(max(Na,Nb))),
 *  Na ~= Nb ~= N => O(N * lg(N))
 *
 * diff_longest_sequence() takes two arguments:
 *  cmptbl == diff_compare_table(a, b)
 *  blen == sizeof(b) >= max(@(cmptbl*({})))
 */
static struct array *diff_longest_sequence(struct array *cmptbl, int blen)
{
   int i,j,top=0;
   struct array *a;
   struct diff_magic_link_pool *pools=NULL;
   struct diff_magic_link *dml;
   struct diff_magic_link **stack;
   char *marks;

   if(!cmptbl->size)
     return allocate_array(0);

   stack = malloc(sizeof(struct diff_magic_link*)*cmptbl->size);

   if (!stack) {
     int args = 0;
     SIMPLE_OUT_OF_MEMORY_ERROR("diff_longest_sequence",
				(int)sizeof(struct diff_magic_link*) *
				cmptbl->size);
   }

   /* NB: marks is used for optimization purposes only */
   marks = calloc(blen, 1);

   if (!marks && blen) {
     int args = 0;
     free(stack);
     SIMPLE_OUT_OF_MEMORY_ERROR("diff_longest_sequence", blen);
   }

#ifdef DIFF_DEBUG
   fprintf(stderr, "\n\nDIFF: sizeof(cmptbl)=%d, blen=%d\n",
	   cmptbl->size, blen);
#endif /* DIFF_DEBUG */

   for (i = 0; i<cmptbl->size; i++)
   {
      struct svalue *inner=cmptbl->item[i].u.array->item;

#ifdef DIFF_DEBUG
      fprintf(stderr, "DIFF: i=%d\n", i);
#endif /* DIFF_DEBUG */

      for (j = cmptbl->item[i].u.array->size; j--;)
      {
	 int x = inner[j].u.integer;

#ifdef DIFF_DEBUG
	 fprintf(stderr, "DIFF:  j=%d, x=%d\n", j, x);
#endif /* DIFF_DEBUG */
#ifdef PIKE_DEBUG
	 if (x >= blen) {
	   Pike_fatal("diff_longest_sequence(): x:%d >= blen:%d\n", x, blen);
	 } else if (x < 0) {
	   Pike_fatal("diff_longest_sequence(): x:%d < 0\n", x);
	 }
#endif /* PIKE_DEBUG */
	 if (!marks[x]) {
	   int pos;

	   if (top && x<=stack[top-1]->x) {
	     /* Find the insertion point. */
	     pos = diff_ponder_stack(x, stack, top);
	     if (pos != top) {
	       /* Not on the stack anymore. */
	       marks[stack[pos]->x] = 0;
	     }
	   } else
	     pos=top;

#ifdef DIFF_DEBUG
	   fprintf(stderr, "DIFF:  pos=%d\n", pos);
#endif /* DIFF_DEBUG */

	   /* This part is only optimization (j accelleration). */
	   if (pos && j)
	   {
	     if (!marks[inner[j-1].u.integer])
	     {
	       /* Find the element to insert. */
	       j = diff_ponder_array(stack[pos-1]->x+1, inner, j);
	       x = inner[j].u.integer;
	     }
	   }
	   else
	   {
	     j = 0;
	     x = inner->u.integer;
	   }

#ifdef DIFF_DEBUG
	   fprintf(stderr, "DIFF: New j=%d, x=%d\n", j, x);
#endif /* DIFF_DEBUG */
#ifdef PIKE_DEBUG
	   if (x >= blen) {
	     Pike_fatal("diff_longest_sequence(): x:%d >= blen:%d\n", x, blen);
	   } else if (x < 0) {
	     Pike_fatal("diff_longest_sequence(): x:%d < 0\n", x);
	   }
#endif /* PIKE_DEBUG */

	   /* Put x on the stack. */
	   marks[x] = 1;
	   if (pos == top)
	   {
#ifdef DIFF_DEBUG
	     fprintf(stderr, "DIFF:  New top element\n");
#endif /* DIFF_DEBUG */

	     if (! (dml=dml_new(&pools)) )
	     {
	       int args = 0;
	       dml_free_pools(pools);
	       free(stack);
	       SIMPLE_OUT_OF_MEMORY_ERROR("diff_longest_sequence",
					  sizeof(struct diff_magic_link_pool) +
					  sizeof(struct diff_magic_link) *
					  DMLPOOLSIZE);
	     }

	     dml->x = x;
	     dml->refs = 1;

	     if (pos)
	       (dml->prev = stack[pos-1])->refs++;
	     else
	       dml->prev = NULL;

	     top++;
	    
	     stack[pos] = dml;
	   } else if (pos && 
		      stack[pos]->refs == 1 &&
		      stack[pos-1] == stack[pos]->prev)
	   {
#ifdef DIFF_DEBUG
	     fprintf(stderr, "DIFF:  Optimized case\n");
#endif /* DIFF_DEBUG */

	     /* Optimization. */
	     stack[pos]->x = x;
	   } else {
#ifdef DIFF_DEBUG
	     fprintf(stderr, "DIFF:  Generic case\n");
#endif /* DIFF_DEBUG */

	     if (! (dml=dml_new(&pools)) )
	     {
	       int args = 0;
	       dml_free_pools(pools);
	       free(stack);
	       SIMPLE_OUT_OF_MEMORY_ERROR("diff_longest_sequence",
					  sizeof(struct diff_magic_link_pool) +
					  sizeof(struct diff_magic_link) *
					  DMLPOOLSIZE);
	     }

	     dml->x = x;
	     dml->refs = 1;

	     if (pos)
	       (dml->prev = stack[pos-1])->refs++;
	     else
	       dml->prev = NULL;

	     if (!--stack[pos]->refs)
	       dml_delete(pools, stack[pos]);
	    
	     stack[pos] = dml;
	   }
#ifdef DIFF_DEBUG
	 } else {
	   fprintf(stderr, "DIFF:  Already marked (%d)!\n", marks[x]);
#endif /* DIFF_DEBUG */
	 }
      }
#ifdef DIFF_DEBUG
      for(j=0; j < top; j++) {
	fprintf(stderr, "DIFF:  stack:%d, mark:%d\n",
		stack[j]->x, marks[stack[j]->x]);
      }
#endif /* DIFF_DEBUG */
   }

   /* No need for marks anymore. */

   free(marks);

   /* FIXME(?) memory unfreed upon error here. */
   a=low_allocate_array(top,0); 
   if (top)
   {
       dml=stack[top-1];
       while (dml)
       {
	  ITEM(a)[--top].u.integer=dml->x;
	  dml=dml->prev;
       }
       a->type_field = BIT_INT;
   }

   free(stack);
   dml_free_pools(pools);
   return a;
}

/*
 * The dynamic programming Longest Common Sequence algorithm.
 *
 * This algorithm is O(Na * Nb), where:
 *
 *  Na == sizeof(a)
 *  Nb == sizeof(b)
 *
 * This makes it faster than the G-M algorithm on binary data,
 * but slower on ascii data.
 *
 * NOT true! The G-M algorithm seems to be faster on most data anyway.
 *	/grubba 1998-05-19
 */
static struct array *diff_dyn_longest_sequence(struct array *cmptbl, int blen)
{
  struct array *res = NULL;
  struct diff_magic_link_head *table;
  struct diff_magic_link_pool *dml_pool = NULL;
  struct diff_magic_link *dml;
  unsigned int sz = (unsigned int)cmptbl->size;
  unsigned int i;
  unsigned int off1 = 0;
  unsigned int off2 = blen + 1;
  ONERROR err;

  table = calloc(sizeof(struct diff_magic_link_head)*2, off2);
  if (!table) {
    int args = 0;
    SIMPLE_OUT_OF_MEMORY_ERROR("diff_dyn_longest_sequence",
			       sizeof(struct diff_magic_link_head) * 2 * off2);
  }

  /* FIXME: Assumes NULL is represented with all zeroes */
  /* NOTE: Scan strings backwards to get the same result as the G-M
   * algorithm.
   */
  for (i = sz; i--;) {
    struct array *boff = cmptbl->item[i].u.array;

#ifdef DIFF_DEBUG
    fprintf(stderr, "  i:%d\n", i);
#endif /* DIFF_DEBUG */

    if (boff->size) {
      unsigned int bi;
      unsigned int base = blen;
      unsigned int tmp = off1;
      off1 = off2;
      off2 = tmp;

      for (bi = boff->size; bi--;) {
	unsigned int ib = boff->item[bi].u.integer;

#ifdef DIFF_DEBUG
	fprintf(stderr, "    Range [%d - %d] differ\n", base - 1, ib + 1);
#endif /* DIFF_DEBUG */
	while ((--base) > ib) {
	  /* Differ */
	  if (table[off1 + base].link) {
	    if (!--(table[off1 + base].link->refs)) {
	      dml_delete(dml_pool, table[off1 + base].link);
	    }
	  }
	  /* FIXME: Should it be > or >= here to get the same result
	   * as with the G-M algorithm?
	   */
	  if (table[off2 + base].depth > table[off1 + base + 1].depth) {
	    table[off1 + base].depth = table[off2 + base].depth;
	    dml = (table[off1 + base].link = table[off2 + base].link);
	  } else {
	    table[off1 + base].depth = table[off1 + base + 1].depth;
	    dml = (table[off1 + base].link = table[off1 + base + 1].link);
	  }
	  if (dml) {
	    dml->refs++;
	  }
	}
	/* Equal */
#ifdef DIFF_DEBUG
	fprintf(stderr, "    Equal\n");
#endif /* DIFF_DEBUG */

	if (table[off1 + ib].link) {
	  if (!--(table[off1 + ib].link->refs)) {
	    dml_delete(dml_pool, table[off1 + ib].link);
	  }
	}
	table[off1 + ib].depth = table[off2 + ib + 1].depth + 1;
	dml = (table[off1 + ib].link = dml_new(&dml_pool));
	if (!dml) {
	  int args = 0;
	  dml_free_pools(dml_pool);
	  free(table);
	  SIMPLE_OUT_OF_MEMORY_ERROR("diff_dyn_longest_sequence",
				     sizeof(struct diff_magic_link_pool) +
				     sizeof(struct diff_magic_link) *
				     DMLPOOLSIZE);
	}
	dml->refs = 1;
	dml->prev = table[off2 + ib + 1].link;
	if (dml->prev) {
	  dml->prev->refs++;
	}
	dml->x = ib;
      }
#ifdef DIFF_DEBUG
      fprintf(stderr, "    Range [0 - %d] differ\n", base-1);
#endif /* DIFF_DEBUG */
      while (base--) {
	/* Differ */
	if (table[off1 + base].link) {
	  if (!--(table[off1 + base].link->refs)) {
	    dml_delete(dml_pool, table[off1 + base].link);
	  }
	}
	/* FIXME: Should it be > or >= here to get the same result
	 * as with the G-M algorithm?
	 */
	if (table[off2 + base].depth > table[off1 + base + 1].depth) {
	  table[off1 + base].depth = table[off2 + base].depth;
	  dml = (table[off1 + base].link = table[off2 + base].link);
	} else {
	  table[off1 + base].depth = table[off1 + base + 1].depth;
	  dml = (table[off1 + base].link = table[off1 + base + 1].link);
	}
	if (dml) {
	  dml->refs++;
	}
      }
    }
  }

  /* Convert table into res */
  sz = table[off1].depth;
  dml = table[off1].link;
  free(table);
#ifdef DIFF_DEBUG
  fprintf(stderr, "Result array size:%d\n", sz);
#endif /* DIFF_DEBUG */

  if(dml_pool) SET_ONERROR(err, dml_free_pools, dml_pool);
  res = allocate_array(sz);
  if(dml_pool) UNSET_ONERROR(err);

  i = 0;
  while(dml) {
#ifdef PIKE_DEBUG
    if (i >= sz) {
      Pike_fatal("Consistency error in diff_dyn_longest_sequence()\n");
    }
#endif /* PIKE_DEBUG */
#ifdef DIFF_DEBUG
    fprintf(stderr, "  %02d: %d\n", i, dml->x);
#endif /* DIFF_DEBUG */
    res->item[i].u.integer = dml->x;
    dml = dml->prev;
    i++;
  }
  res->type_field = BIT_INT;
#ifdef PIKE_DEBUG
  if (i != sz) {
    Pike_fatal("Consistency error in diff_dyn_longest_sequence()\n");
  }
#endif /* PIKE_DEBUG */

  dml_free_pools(dml_pool);
  return(res);
}

static struct array* diff_build(struct array *a,
				struct array *b,
				struct array *seq)
{
   struct array *ad,*bd;
   ptrdiff_t bi, ai, lbi, lai, i, eqstart;

   /* FIXME(?) memory unfreed upon error here (and later) */
   ad=low_allocate_array(0,32);
   bd=low_allocate_array(0,32);
   
   eqstart=0;
   lbi=bi=ai=-1;
   for (i=0; i<seq->size; i++)
   {
      bi=seq->item[i].u.integer;

      if (bi!=lbi+1 || !is_equal(a->item+ai+1,b->item+bi))
      {
	 /* insert the equality */
	 if (lbi>=eqstart)
	 {
	    push_array(friendly_slice_array(b,eqstart,lbi+1));
	    ad=append_array(ad,Pike_sp-1);
	    bd=append_array(bd,Pike_sp-1);
	    pop_stack();
	 }
	 /* insert the difference */
	 lai=ai;
	 ai=array_search(a,b->item+bi,ai+1)-1;

	 push_array(friendly_slice_array(b,lbi+1,bi));
	 bd=append_array(bd, Pike_sp-1);
	 pop_stack();

	 push_array(friendly_slice_array(a,lai+1,ai+1));
	 ad=append_array(ad,Pike_sp-1);
	 pop_stack();

	 eqstart=bi;
      }
      ai++;
      lbi=bi;
   }

   if (lbi>=eqstart)
   {
      push_array(friendly_slice_array(b,eqstart,lbi+1));
      ad=append_array(ad,Pike_sp-1);
      bd=append_array(bd,Pike_sp-1);
      pop_stack();
   }

   if (b->size>bi+1 || a->size>ai+1)
   {
      push_array(friendly_slice_array(b,lbi+1,b->size));
      bd=append_array(bd, Pike_sp-1);
      pop_stack();
      
      push_array(friendly_slice_array(a,ai+1,a->size));
      ad=append_array(ad,Pike_sp-1);
      pop_stack();
   }

   push_array(ad);
   push_array(bd);
   return aggregate_array(2);
}

/*! @decl array permute(array in, int number)
 *!
 *!   Give a specified permutation of an array.
 *!
 *!   The number of permutations is equal to @expr{sizeof(@[in])!@}
 *!   (the factorial of the size of the given array).
 *!
 *! @seealso
 *!   @[shuffle()]
 */
PMOD_EXPORT void f_permute( INT32 args )
{
  INT_TYPE q, i=0, n;
  struct array *a;
  struct svalue *it;

  if( args != 2 )
    SIMPLE_TOO_FEW_ARGS_ERROR("permute", 2);
  if( Pike_sp[ -2 ].type != T_ARRAY )
     SIMPLE_BAD_ARG_ERROR("permute", 1, "array");
  if (Pike_sp[ -1 ].type != T_INT)
    SIMPLE_BAD_ARG_ERROR("permute", 2, "int");

  n  = Pike_sp[ -1 ].u.integer;
  a = copy_array( Pike_sp[ -2 ].u.array );
  pop_n_elems( args );
  q = a->size;
  it = a->item;
  while( n && q )
  {
    int x = n % q;
    n /= q;
    q--;
    if( x )
    {
      struct svalue tmp;
      tmp     = it[i];
      it[i]   = it[i+x];
      it[i+x] = tmp;
    }
    i++;
  }
  push_array( a );
}

/*! @decl array(array(array)) diff(array a, array b)
 *!
 *!   Calculates which parts of the arrays that are common to both, and
 *!   which parts that are not.
 *!
 *! @returns
 *!   Returns an array with two elements, the first is an array of parts in
 *!   array @[a], and the second is an array of parts in array @[b].
 *!
 *! @seealso
 *!   @[diff_compare_table()], @[diff_longest_sequence()],
 *!   @[String.fuzzymatch()]
 */
PMOD_EXPORT void f_diff(INT32 args)
{
   struct array *seq;
   struct array *cmptbl;
   struct array *diff;
   struct array *a, *b;
   int uniq;

   get_all_args("diff", args, "%a%a", &a, &b);

   cmptbl = diff_compare_table(a, b, &uniq);

   push_array(cmptbl);
#ifdef ENABLE_DYN_DIFF
   if (uniq * 100 > cmptbl->size) {
#endif /* ENABLE_DYN_DIFF */
#ifdef DIFF_DEBUG
     fprintf(stderr, "diff: Using G-M algorithm, u:%d, s:%d\n",
	     uniq, cmptbl->size);
#endif /* DIFF_DEBUG */
     seq = diff_longest_sequence(cmptbl, b->size);
#ifdef ENABLE_DYN_DIFF
   } else {
#ifdef DIFF_DEBUG
     fprintf(stderr, "diff: Using dyn algorithm, u:%d, s:%d\n",
	     uniq, cmptbl->size);
#endif /* DIFF_DEBUG */
     seq = diff_dyn_longest_sequence(cmptbl, b->size);
   }     
#endif /* ENABLE_DYN_DIFF */
   push_array(seq);
   
   diff=diff_build(a,b,seq);

   pop_n_elems(2+args);
   push_array(diff);
}

/*! @decl array(array(int)) diff_compare_table(array a, array b)
 *!
 *!   Returns an array which maps from index in @[a] to corresponding
 *!   indices in @[b].
 *!
 *! @pre{
 *! > Array.diff_compare_table( ({ "a","b","c" }), ({ "b", "b", "c", "d", "b" }));
 *! Result: ({
 *!             ({ }),
 *!             ({
 *!                 0,
 *!                 1,
 *!                 4
 *!             }),
 *!             ({
 *!                 2
 *! 	        })
 *!         })
 *! @}
 *!
 *! @seealso
 *!   @[diff()], @[diff_longest_sequence()], @[String.fuzzymatch()]
 */
PMOD_EXPORT void f_diff_compare_table(INT32 args)
{
  struct array *a;
  struct array *b;
  struct array *cmptbl;

  get_all_args("diff_compare_table", args, "%a%a", &a, &b);

  cmptbl = diff_compare_table(a, b, NULL);

  pop_n_elems(args);
  push_array(cmptbl);
}

/*! @decl array(int) diff_longest_sequence(array a, array b)
 *!
 *!   Gives the longest sequence of indices in @[b] that have corresponding
 *!   values in the same order in @[a].
 *!
 *! @seealso
 *!   @[diff()], @[diff_compare_table()], @[String.fuzzymatch()]
 */
PMOD_EXPORT void f_diff_longest_sequence(INT32 args)
{
  struct array *a;
  struct array *b;
  struct array *seq;
  struct array *cmptbl;

  get_all_args("diff_longest_sequence", args, "%a%a", &a, &b);

  cmptbl = diff_compare_table(a, b, NULL);

  push_array(cmptbl);

  seq = diff_longest_sequence(cmptbl, b->size);

  pop_n_elems(args+1);
  push_array(seq); 
}

/*! @decl array(int) diff_dyn_longest_sequence(array a, array b)
 *!
 *!   Gives the longest sequence of indices in @[b] that have corresponding
 *!   values in the same order in @[a].
 *!
 *!   This function performs the same operation as @[diff_longest_sequence()],
 *!   but uses a different algorithm, which in some rare cases might be faster
 *!   (usually it's slower though).
 *!
 *! @seealso
 *!   @[diff_longest_sequence()], @[diff()], @[diff_compare_table()],
 *!   @[String.fuzzymatch()]
 */
PMOD_EXPORT void f_diff_dyn_longest_sequence(INT32 args)
{
  struct array *a;
  struct array *b;
  struct array *seq;
  struct array *cmptbl;

  get_all_args("diff_dyn_longest_sequence", args, "%a%a", &a, &b);

  cmptbl=diff_compare_table(a, b, NULL);

  push_array(cmptbl);

  seq = diff_dyn_longest_sequence(cmptbl, b->size);

  pop_n_elems(args+1);
  push_array(seq); 
}

/*! @endmodule
 */

/**********************************************************************/

static struct callback_list memory_usage_callback;

struct callback *add_memory_usage_callback(callback_func call,
					  void *arg,
					  callback_func free_func)
{
  return add_to_callback(&memory_usage_callback, call, arg, free_func);
}

/*! @decl mapping(string:int) memory_usage()
 *! @belongs Debug
 *!
 *!   Check memory usage.
 *!
 *!   This function is mostly intended for debugging. It delivers a mapping
 *!   with information about how many arrays/mappings/strings etc. there
 *!   are currently allocated and how much memory they use.
 *!
 *! @note
 *!   Exactly what this function returns is version dependant.
 *!
 *! @seealso
 *!   @[_verify_internals()]
 */
PMOD_EXPORT void f__memory_usage(INT32 args)
{
  INT32 num,size;
  struct svalue *ss;
  pop_n_elems(args);
  ss=Pike_sp;

  count_memory_in_mappings(&num, &size);
  push_text("num_mappings");
  push_int(num);
  push_text("mapping_bytes");
  push_int(size);

  count_memory_in_strings(&num, &size);
  push_text("num_strings");
  push_int(num);
  push_text("string_bytes");
  push_int(size);

  count_memory_in_arrays(&num, &size);
  push_text("num_arrays");
  push_int(num);
  push_text("array_bytes");
  push_int(size);

  count_memory_in_programs(&num,&size);
  push_text("num_programs");
  push_int(num);
  push_text("program_bytes");
  push_int(size);

  count_memory_in_multisets(&num, &size);
  push_text("num_multisets");
  push_int(num);
  push_text("multiset_bytes");
  push_int(size);

  count_memory_in_objects(&num, &size);
  push_text("num_objects");
  push_int(num);
  push_text("object_bytes");
  push_int(size);

  count_memory_in_callbacks(&num, &size);
  push_text("num_callbacks");
  push_int(num);
  push_text("callback_bytes");
  push_int(size);

  count_memory_in_callables(&num, &size);
  push_text("num_callables");
  push_int(num);
  push_text("callable_bytes");
  push_int(size);

  count_memory_in_pike_frames(&num, &size);
  push_text("num_frames");
  push_int(num);
  push_text("frame_bytes");
  push_int(size);

#ifdef DEBUG_MALLOC
  {
    extern void count_memory_in_memory_maps(INT32*, INT32*);
    extern void count_memory_in_memory_map_entrys(INT32*, INT32*);
    extern void count_memory_in_memlocs(INT32*, INT32*);
    extern void count_memory_in_memhdrs(INT32*, INT32*);

    count_memory_in_memory_maps(&num, &size);
    push_text("num_memory_maps");
    push_int(num);
    push_text("memory_map_bytes");
    push_int(size);

    count_memory_in_memory_map_entrys(&num, &size);
    push_text("num_memory_map_entries");
    push_int(num);
    push_text("memory_map_entrie_bytes");
    push_int(size);

    count_memory_in_memlocs(&num, &size);
    push_text("num_memlocs");
    push_int(num);
    push_text("memloc_bytes");
    push_int(size);

    count_memory_in_memhdrs(&num, &size);
    push_text("num_memhdrs");
    push_int(num);
    push_text("memhdr_bytes");
    push_int(size);
  }
#endif

  call_callback(&memory_usage_callback, NULL);

  f_aggregate_mapping(DO_NOT_WARN(Pike_sp - ss));
}

/*! @decl mixed _next(mixed x)
 *!
 *!   Find the next object/array/mapping/multiset/program or string.
 *!
 *!   All objects, arrays, mappings, multisets, programs and strings are
 *!   stored in linked lists inside Pike. This function returns the next
 *!   item on the corresponding list. It is mainly meant for debugging
 *!   the Pike runtime, but can also be used to control memory usage.
 *!
 *! @seealso
 *!   @[next_object()], @[_prev()]
 */
PMOD_EXPORT void f__next(INT32 args)
{
  struct svalue tmp;

  ASSERT_SECURITY_ROOT("_next");

  if(!args)
    SIMPLE_TOO_FEW_ARGS_ERROR("_next", 1);
  
  pop_n_elems(args-1);
  args = 1;
  tmp=Pike_sp[-1];
  switch(tmp.type)
  {
  case T_OBJECT:  tmp.u.object=tmp.u.object->next; break;
  case T_ARRAY:   tmp.u.array=tmp.u.array->next; break;
  case T_MAPPING: tmp.u.mapping=tmp.u.mapping->next; break;
  case T_MULTISET:tmp.u.multiset=tmp.u.multiset->next; break;
  case T_PROGRAM: tmp.u.program=tmp.u.program->next; break;
  case T_STRING:  tmp.u.string=next_pike_string(tmp.u.string); break;
  default:
    SIMPLE_BAD_ARG_ERROR("_next", 1,
			 "object|array|mapping|multiset|program|string");
  }
  if(tmp.u.refs)
  {
    assign_svalue(Pike_sp-1,&tmp);
  }else{
    pop_stack();
    push_int(0);
  }
}

/*! @decl mixed _prev(mixed x)
 *!
 *!   Find the previous object/array/mapping/multiset or program.
 *!
 *!   All objects, arrays, mappings, multisets and programs are
 *!   stored in linked lists inside Pike. This function returns the previous
 *!   item on the corresponding list. It is mainly meant for debugging
 *!   the Pike runtime, but can also be used to control memory usage.
 *!
 *! @note
 *!   Unlike @[_next()] this function does not work on strings.
 *!
 *! @seealso
 *!   @[next_object()], @[_next()]
 */
PMOD_EXPORT void f__prev(INT32 args)
{
  struct svalue tmp;

  ASSERT_SECURITY_ROOT("_prev");

  if(!args)
    SIMPLE_TOO_FEW_ARGS_ERROR("_prev", 1);
  
  pop_n_elems(args-1);
  args = 1;
  tmp=Pike_sp[-1];
  switch(tmp.type)
  {
  case T_OBJECT:  tmp.u.object=tmp.u.object->prev; break;
  case T_ARRAY:   tmp.u.array=tmp.u.array->prev; break;
  case T_MAPPING: tmp.u.mapping=tmp.u.mapping->prev; break;
  case T_MULTISET:tmp.u.multiset=tmp.u.multiset->prev; break;
  case T_PROGRAM: tmp.u.program=tmp.u.program->prev; break;
  default:
    SIMPLE_BAD_ARG_ERROR("_prev", 1, "object|array|mapping|multiset|program");
  }
  if(tmp.u.refs)
  {
    assign_svalue(Pike_sp-1,&tmp);
  }else{
    pop_stack();
    push_int(0);
  }
}

/*! @decl int _refs(string|array|mapping|multiset|function|object|program o)
 *!
 *!   Return the number of references @[o] has.
 *!
 *!   It is mainly meant for debugging the Pike runtime, but can also be
 *!   used to control memory usage.
 *!
 *! @note
 *!   Note that the number of references will always be at least one since
 *!   the value is located on the stack when this function is executed.
 *!
 *! @seealso
 *!   @[_next()], @[_prev()]
 */
PMOD_EXPORT void f__refs(INT32 args)
{
  INT32 i;

  if(!args)
    SIMPLE_TOO_FEW_ARGS_ERROR("_refs", 1);

  if(Pike_sp[-args].type > MAX_REF_TYPE)
    SIMPLE_BAD_ARG_ERROR("refs", 1,
			 "array|mapping|multiset|object|"
			 "function|program|string");

  i=Pike_sp[-args].u.refs[0];
  pop_n_elems(args);
  push_int(i);
}

#ifdef PIKE_DEBUG
/* This function is for debugging *ONLY*
 * do not document please. /Hubbe
 */
PMOD_EXPORT void f__leak(INT32 args)
{
  INT32 i;

  if(!args)
    SIMPLE_TOO_FEW_ARGS_ERROR("_leak", 1);

  if(Pike_sp[-args].type > MAX_REF_TYPE)
    SIMPLE_BAD_ARG_ERROR("_leak", 1,
			 "array|mapping|multiset|object|"
			 "function|program|string");

  add_ref(Pike_sp[-args].u.dummy);
  i=Pike_sp[-args].u.refs[0];
  pop_n_elems(args);
  push_int(i);
}
#endif

/*! @decl type _typeof(mixed x)
 *!
 *!   Return the runtime type of @[x].
 *!
 *! @seealso
 *!   @[typeof()]
 */
PMOD_EXPORT void f__typeof(INT32 args)
{
  struct pike_type *t;

  if(!args)
    SIMPLE_TOO_FEW_ARGS_ERROR("_typeof", 1);

  t = get_type_of_svalue(Pike_sp-args);

  pop_n_elems(args);
  push_type_value(t);
}

/*! @decl void replace_master(object o)
 *!
 *!   Replace the master object with @[o].
 *!
 *!   This will let you control many aspects of how Pike works, but beware that
 *!   @tt{master.pike@} may be required to fill certain functions, so it is
 *!   usually a good idea to have your master inherit the original master and
 *!   only re-define certain functions.
 *!
 *!   FIXME: Tell how to inherit the master.
 *!
 *! @seealso
 *!   @[master()]
 */
PMOD_EXPORT void f_replace_master(INT32 args)
{
  ASSERT_SECURITY_ROOT("replace_master");

  if(!args)
    SIMPLE_TOO_FEW_ARGS_ERROR("replace_master", 1);
  if(Pike_sp[-args].type != T_OBJECT)
    SIMPLE_BAD_ARG_ERROR("replace_master", 1, "object");
  if(!Pike_sp[-args].u.object->prog)
    bad_arg_error("replace_master", Pike_sp-args, args, 1, "object", Pike_sp-args,
		  "Called with destructed object.\n");

  if (Pike_sp[-args].subtype)
    bad_arg_error("replace_master", Pike_sp-args, args, 1, "object", Pike_sp-args,
		  "Subtyped master objects are not supported yet.\n");
    
  free_object(master_object);
  master_object=Pike_sp[-args].u.object;
  add_ref(master_object);

  free_program(master_program);
  master_program=master_object->prog;
  add_ref(master_program);

  pop_n_elems(args);
}

/*! @decl object master();
 *!
 *!   Return the current master object.
 *!
 *! @note
 *!   May return @[UNDEFINED] if no master has been loaded yet.
 *!
 *! @seealso
 *!   @[replace_master()]
 */
PMOD_EXPORT void f_master(INT32 args)
{
  struct object *o;
  pop_n_elems(args);
  o = get_master();
  if (o) ref_push_object(o);
  else push_undefined();
}

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

/*! @decl int gethrvtime (void|int nsec)
 *!
 *! Return the CPU time that has been consumed by this process or
 *! thread. -1 is returned if the system couldn't determine it. The
 *! time is normally returned in microseconds, but if the optional
 *! argument @[nsec] is nonzero it's returned in nanoseconds.
 *!
 *! The CPU time includes both user and system time, i.e. it's
 *! approximately the same thing you would get by adding together the
 *! "utime" and "stime" fields returned by @[System.getrusage] (but
 *! perhaps with better accuracy). It's however system dependent
 *! whether or not it's the time consumed in all threads or in the
 *! current one only; @[System.CPU_TIME_IS_THREAD_LOCAL] tells which.
 *!
 *! @note
 *!   The actual accuracy on many systems is significantly less than
 *!   milliseconds or nanoseconds.
 *!
 *! @note
 *!   The garbage collector might run automatically at any time. The
 *!   time it takes is not included in the figure returned by this
 *!   function, so that normal measurements aren't randomly clobbered
 *!   by it. Explicit calls to @[gc] are still included, though.
 *!
 *! @note
 *!   The special function @[gauge] is implemented with this function.
 *!
 *! @seealso
 *!   @[System.CPU_TIME_IS_THREAD_LOCAL], @[gauge()],
 *!   @[System.getrusage()], @[gethrtime()]
 */
PMOD_EXPORT void f_gethrvtime(INT32 args)
{
  int nsec = 0;
  cpu_time_t time = get_cpu_time();

  if (time == (cpu_time_t) -1) {
    pop_n_elems (args);
    push_int (-1);
    return;
  }

  /* Don't subtract the gc time at all if we don't know whether it's
   * thread local or not, since if we do it wrong we might end up
   * returning a negative number. */
#if CPU_TIME_IS_THREAD_LOCAL == PIKE_YES
  time -= Pike_interpreter.thread_state->auto_gc_time;
#elif CPU_TIME_IS_THREAD_LOCAL == PIKE_NO
  time -= auto_gc_time;
#endif

  nsec = args && !UNSAFE_IS_ZERO(Pike_sp-args);

  pop_n_elems(args);
  if (nsec)
    push_int64(time);
  else
    push_int64(time/1000);
}

/*! @decl int gethrtime (void|int nsec)
 *!
 *! Return the real time since some arbitrary event in the past. The
 *! time is normally returned in microseconds, but if the optional
 *! argument @[nsec] is nonzero it's returned in nanoseconds.
 *!
 *! @note
 *!   The actual accuracy on many systems is significantly less than
 *!   milliseconds or nanoseconds.
 *!
 *! @seealso
 *!   @[time()], @[System.gettimeofday()], @[gethrvtime()]
 */
#ifdef HAVE_GETHRTIME
PMOD_EXPORT void f_gethrtime(INT32 args)
{
  int nsec = 0;
  nsec = args && !UNSAFE_IS_ZERO(Pike_sp-args);

  pop_n_elems(args);
  if(nsec)
    push_int64(gethrtime()); 
  else
    push_int64(gethrtime()/1000);
}
#else
PMOD_EXPORT void f_gethrtime(INT32 args)
{
  int nsec = 0;
  nsec = args && !UNSAFE_IS_ZERO(Pike_sp-args);

  pop_n_elems(args);
  GETTIMEOFDAY(&current_time);
#ifdef INT64
  if(nsec)
    push_int64((((INT64)current_time.tv_sec * 1000000) + current_time.tv_usec)*1000);
  else
    push_int64(((INT64)current_time.tv_sec * 1000000) + current_time.tv_usec);
#else /* !INT64 */
  if(nsec)
    push_int64(((current_time.tv_sec * 1000000) + current_time.tv_usec)*1000);
  else
    push_int64((current_time.tv_sec * 1000000) + current_time.tv_usec);
#endif /* INT64 */
}
#endif /* HAVE_GETHRTIME */

#ifdef PROFILING
/*! @decl array(int|mapping(string:array(int))) @
 *!           get_profiling_info(program prog)
 *!
 *!   Get profiling information.
 *!
 *! @returns
 *!   Returns an array with two elements.
 *!   @array
 *!   	@elem int num_clones
 *!   	  The first element is the number of times the program @[prog] has been
 *!   	  instantiated.
 *!   	@elem mapping(string:array(int)) fun_prof_info
 *!   	  The second element is mapping from function name to an
 *!   	  array with three elements.
 *!   	  @array
 *!   	    @elem int num_calls
 *!   	      The first element is the number of times the function has been
 *!   	      called.
 *!   	    @elem int total_time
 *!   	      The second element is the total time (in milliseconds) spent
 *!   	      executing this function, and any functions called from it.
 *!   	    @elem int self_time
 *!   	      The third element is the time (in milliseconds) actually spent
 *!   	      in this function so far.
 *!   	  @endarray
 *!   @endarray
 *!
 *! @note
 *!   This function is only available if the runtime was compiled with
 *!   the option @tt{--with-profiling@}.
 */
static void f_get_prof_info(INT32 args)
{
  struct program *prog = 0;
  int num_functions;
  int i;

  if (!args) {
    SIMPLE_TOO_FEW_ARGS_ERROR("get_profiling_info", 1);
  }
  prog = program_from_svalue(Pike_sp-args);
  if(!prog)
    SIMPLE_BAD_ARG_ERROR("get_profiling_info", 1, "program");

  /* ({ num_clones, ([ "fun_name":({ num_calls, total_time, self_time }) ]) })
   */

  pop_n_elems(args-1);
  args = 1;

  push_int(prog->num_clones);

  for(num_functions=i=0; i<(int)prog->num_identifiers; i++) {
    if (prog->identifiers[i].num_calls)
    {
      num_functions++;
      ref_push_string(prog->identifiers[i].name);

      push_int(prog->identifiers[i].num_calls);
      if (CPU_TIME_TICKS == 1000) {
	push_int64(prog->identifiers[i].total_time);
	push_int64(prog->identifiers[i].self_time);
      } else {
	push_int64(prog->identifiers[i].total_time/1000000);
	push_int64(prog->identifiers[i].self_time/1000000);
      }
      f_aggregate(3);
    }
  }
  f_aggregate_mapping(num_functions * 2);
  f_aggregate(2);

  stack_swap();
  pop_stack();
}
#endif /* PROFILING */

/*! @decl int(0..1) object_variablep(object o, string var)
 *!
 *!   Find out if an object identifier is a variable.
 *!
 *! @returns
 *!   This function returns @expr{1@} if @[var] exists as a non-static variable
 *!   in @[o], and returns @expr{0@} (zero) otherwise.
 *!
 *! @seealso
 *!   @[indices()], @[values()]
 */
PMOD_EXPORT void f_object_variablep(INT32 args)
{
  struct object *o;
  struct pike_string *s;
  int ret;

  get_all_args("variablep",args,"%o%S",&o, &s);

  if(!o->prog)
    bad_arg_error("variablep", Pike_sp-args, args, 1, "object", Pike_sp-args,
		  "Called on destructed object.\n");

  /* FIXME: Ought to be overloadable, since `[]=() is... */

  ret=find_shared_string_identifier(s,o->prog);
  if(ret!=-1)
  {
    ret=IDENTIFIER_IS_VARIABLE(ID_FROM_INT(o->prog, ret)->identifier_flags);
  }else{
    ret=0;
  }
  pop_n_elems(args);
  push_int(!!ret);
}

/*! @module Array
 */

/*! @decl array uniq(array a)
 *!
 *!   Remove elements that are duplicates.
 *!
 *! @returns
 *!   This function returns an copy of the array @[a] with all
 *!   duplicate values removed. The order of the values is kept in the
 *!   result; it's always the first of several equal elements that is
 *!   kept.
 *!
 *! @note
 *!   Elements are compared with @[`==]. They are also hashed (see
 *!   @[lfun::__hash] for further details if the array contains
 *!   objects).
 */
PMOD_EXPORT void f_uniq_array(INT32 args)
{
  struct array *a, *b;
  struct mapping *m;
  struct svalue one;
  int i, j=0,size=0;

  get_all_args("uniq", args, "%a", &a);
  push_mapping(m = allocate_mapping(a->size));
  push_array(b = allocate_array(a->size));

  one.type = T_INT;
  one.u.integer = 1;
  for(i =0; i< a->size; i++)
  {
    mapping_insert(m, ITEM(a)+i, &one);
    if(m_sizeof(m) != size)
    {
      size=m_sizeof(m);
      assign_svalue_no_free(ITEM(b)+ j++, ITEM(a)+i);
    }
  }
  dmalloc_touch_svalue(Pike_sp-1);
  Pike_sp--; /* keep the ref to 'b' */
  ACCEPT_UNFINISHED_TYPE_FIELDS {
    b=resize_array(b,  j);
  } END_ACCEPT_UNFINISHED_TYPE_FIELDS;
  b->type_field = a->type_field;
  pop_n_elems(args-1); /* pop args and the mapping */
  push_array(b);
}

/*! @decl array(mixed) splice(array(mixed) arr1, array(mixed) arr2, @
 *!                           array(mixed) ... more_arrays)
 *!
 *!   Splice two or more arrays.
 *!
 *!   This means that the returned array has the first element in the
 *!   first given array, then the first argument in next array and so
 *!   on for all arrays. Then the second elements are added, etc.
 *!
 *! @seealso
 *!   @[`/()], @[`*()], @[`+()], @[`-()], @[everynth()]
 */
PMOD_EXPORT void f_splice(INT32 args)
{
  struct array *out;
  INT32 size=0x7fffffff;
  INT32 i,j,k;

  for(i=0;i<args;i++)
    if (Pike_sp[i-args].type!=T_ARRAY) 
      SIMPLE_BAD_ARG_ERROR("splice", i+1, "array");
    else
      if (Pike_sp[i-args].u.array->size < size)
	size=Pike_sp[i-args].u.array->size;

  out=allocate_array(args * size);
  if (!args)
  {
    push_array(out);
    return;
  }

  out->type_field=0;
  for(i=-args; i<0; i++) out->type_field|=Pike_sp[i].u.array->type_field;

  for(k=j=0; j<size; j++)
    for(i=-args; i<0; i++)
      assign_svalue_no_free(out->item+(k++), Pike_sp[i].u.array->item+j);

  pop_n_elems(args);
  push_array(out);
  return;
}

/*! @decl array(mixed) everynth(array(mixed) a, void|int n, @
 *!                             void|int start)
 *!
 *!   Return an array with every @[n]:th element of the array @[a].
 *!
 *!   If @[n] is zero every other element will be returned.
 *!
 *! @seealso
 *!   @[splice()], @[`/()]
 */
PMOD_EXPORT void f_everynth(INT32 args)
{
  INT32 k,n=2;
  INT32 start=0;
  struct array *a;
  struct array *ina;
  TYPE_FIELD types;
  INT32 size=0;

  check_all_args("everynth", args,
		 BIT_ARRAY, BIT_INT | BIT_VOID, BIT_INT | BIT_VOID , 0);

  switch(args)
  {
    default:
    case 3:
     start=Pike_sp[2-args].u.integer;
     if(start<0)
       bad_arg_error("everynth", Pike_sp-args, args, 3, "int", Pike_sp+2-args,
		     "Argument negative.\n");
    case 2:
      n=Pike_sp[1-args].u.integer;
      if(n<1)
	bad_arg_error("everynth", Pike_sp-args, args, 2, "int", Pike_sp+1-args,
		      "Argument negative.\n");
    case 1:
      ina=Pike_sp[-args].u.array;
  }

  a=allocate_array(((size=ina->size)-start+n-1)/n);
  types = 0;
  for(k=0; start<size; k++, start+=n) {
    assign_svalue_no_free(ITEM(a) + k, ina->item+start);
    types |= 1 << ITEM(a)[k].type;
  }
  a->type_field=types;

  pop_n_elems(args);
  push_array(a);
  return;
}

/*! @decl array(array) transpose(array(array) matrix)
 *! Takes an array of equally sized arrays (essentially a matrix of size M*N)
 *! and returns the transposed (N*M) version of it, where rows and columns
 *! are exchanged for one another.
 */
PMOD_EXPORT void f_transpose(INT32 args)
{
  struct array *out;
  struct array *in;
  struct array *outinner;
  INT32 sizeininner=0,sizein=0;
  INT32 j,i;
  TYPE_FIELD type=0;

  if (args<1)
    SIMPLE_TOO_FEW_ARGS_ERROR("transpose", 1);

  if (Pike_sp[-args].type!=T_ARRAY) 
    SIMPLE_BAD_ARG_ERROR("transpose", 1, "array(array)");

  in=Pike_sp[-args].u.array;
  sizein=in->size;

  if(!sizein)
  {
    pop_n_elems(args);
    out=allocate_array(0);
    push_array(out);
    return; 
  }

  if( (in->type_field != BIT_ARRAY) &&
      (array_fix_type_field(in) != BIT_ARRAY) )
    SIMPLE_BAD_ARG_ERROR("transpose", 1, "array(array)");

  sizeininner=in->item->u.array->size;

  for(i=1 ; i<sizein; i++)
    if (sizeininner!=(in->item+i)->u.array->size)
      Pike_error("The array given as argument 1 to transpose must contain arrays of the same size.\n");

  out=allocate_array(sizeininner);

  for(i=0; i<sizein; i++)
    type|=in->item[i].u.array->type_field;
  
  for(j=0; j<sizeininner; j++)
  {
    struct svalue * ett;
    struct svalue * tva;

    outinner=allocate_array(sizein);
    ett=outinner->item;
    tva=in->item;
    for(i=0; i<sizein; i++)
      assign_svalue_no_free(ett+i, tva[i].u.array->item+j);

    outinner->type_field=type;
    out->item[j].u.array=outinner;
    out->item[j].type=T_ARRAY;
  }

  out->type_field=BIT_ARRAY;
  pop_n_elems(args);
  push_array(out);
  return;
}

/*! @endmodule
 */

#ifdef DEBUG_MALLOC
/*! @decl void reset_dmalloc()
 *! @belongs Debug
 *!
 *! @note
 *!   Only available when compiled with dmalloc.
 */
PMOD_EXPORT void f__reset_dmalloc(INT32 args)
{
  ASSERT_SECURITY_ROOT("_reset_dmalloc");
  pop_n_elems(args);
  reset_debug_malloc();
}

/*! @decl void dmalloc_set_name(string filename, int linenumber)
 *! @belongs Debug
 *!
 *! @note
 *!   Only available when compiled with dmalloc.
 */
PMOD_EXPORT void f__dmalloc_set_name(INT32 args)
{
  char *s;
  INT_TYPE i;
  extern char * dynamic_location(const char *file, int line);
  extern char * dmalloc_default_location;

  if(args)
  {
    get_all_args("_dmalloc_set_name", args, "%s%i", &s, &i);
    dmalloc_default_location = dynamic_location(s, i);
  }else{
    dmalloc_default_location=0;
  }
  pop_n_elems(args);
}

/*! @decl void list_open_fds()
 *! @belongs Debug
 *!
 *! @note
 *!   Only available when compiled with dmalloc.
 */
PMOD_EXPORT void f__list_open_fds(INT32 args)
{
  extern void list_open_fds(void);
  list_open_fds();
}
#endif

#ifdef PIKE_DEBUG
/*! @decl void locate_references(string|array|mapping| @
 *!                              multiset|function|object| @
 *!                              program|type o)
 *! @belongs Debug
 *!
 *!   This function is mostly intended for debugging. It will search through
 *!   all data structures in Pike looking for @[o] and print the
 *!   locations on stderr. @[o] can be anything but @expr{int@} or
 *!   @expr{float@}.
 *!
 *! @note
 *!   This function only exists if the Pike runtime has been compiled
 *!   with RTL debug.
 */
PMOD_EXPORT void f__locate_references(INT32 args)
{
  ASSERT_SECURITY_ROOT("_locate_references");
  if(args)
    locate_references(Pike_sp[-args].u.refs);
  pop_n_elems(args-1);
}

/*! @decl mixed describe(mixed x)
 *! @belongs Debug
 *!
 *!   Prints out a description of the thing @[x] to standard error.
 *!   The description contains various internal info associated with
 *!   @[x].
 *!
 *! @note
 *!   This function only exists if the Pike runtime has been compiled
 *!   with RTL debug.
 */
PMOD_EXPORT void f__describe(INT32 args)
{
  struct svalue *s;
  ASSERT_SECURITY_ROOT("_describe");
  get_all_args("_describe", args, "%*", &s);
  debug_describe_svalue(debug_malloc_pass(s));
  pop_n_elems(args-1);
}

/*! @decl void gc_set_watch(array|multiset|mapping|object|function|program|string x)
 *! @belongs Debug
 *!
 *!   Sets a watch on the given thing, so that the gc will print a
 *!   message whenever it's encountered. Intended to be used together
 *!   with breakpoints to debug the garbage collector.
 *!
 *! @note
 *!   This function only exists if the Pike runtime has been compiled
 *!   with RTL debug.
 */
PMOD_EXPORT void f__gc_set_watch(INT32 args)
{
  ASSERT_SECURITY_ROOT("_gc_set_watch");

  if (args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("_gc_set_watch", 1);
  if (Pike_sp[-args].type > MAX_REF_TYPE)
    SIMPLE_BAD_ARG_ERROR("_gc_set_watch", 1, "reference type");
  gc_watch(Pike_sp[-args].u.refs);
  pop_n_elems(args);
}

/*! @decl void dump_backlog()
 *! @belongs Debug
 *!
 *!   Dumps the 1024 latest executed opcodes, along with the source
 *!   code lines, to standard error. The backlog is only collected on
 *!   debug level 1 or higher, set with @[_debug] or with the @tt{-d@}
 *!   argument on the command line.
 *!
 *! @note
 *!   This function only exists if the Pike runtime has been compiled
 *!   with RTL debug.
 */
PMOD_EXPORT void f__dump_backlog(INT32 args)
{
  ASSERT_SECURITY_ROOT("_dump_backlog");
  pop_n_elems(args);
  dump_backlog();
}

#endif

/*! @decl mixed map(mixed arr, void|mixed fun, mixed ... extra)
 *!
 *!   Applies @[fun] to the elements in @[arr] and collects the results.
 *!
 *!   @[arr] is treated as a set of elements, as follows:
 *!
 *!   @dl
 *!     @item array
 *!     @item multiset
 *!     @item string
 *!       @[fun] is applied in order to each element. The results are
 *!       collected, also in order, to a value of the same type as
 *!       @[arr], which is returned.
 *!
 *!     @item mapping
 *!       @[fun] is applied to the values, and each result is assigned
 *!       to the same index in a new mapping, which is returned.
 *!
 *!     @item program
 *!       The program is treated as a mapping containing the
 *!       identifiers that are indexable from it and their values.
 *!
 *!     @item object
 *!       If there is a @[lfun::cast] method in the object, it's
 *!       called
 *!       to try to cast the object to an array, a mapping, or a
 *!       multiset, in that order, which is then handled as described
 *!       above.
 *!   @enddl
 *!
 *!   @[fun] is applied in different ways depending on its type:
 *!
 *!   @dl
 *!     @item function
 *!       @[fun] is called for each element. It gets the current
 *!       element as the first argument and @[extra] as the rest. The
 *!       result of the call is collected.
 *!
 *!     @item object
 *!       @[fun] is used as a function like above, i.e. the
 *!       @[lfun::`()] method in it is called.
 *!
 *!     @item multiset
 *!     @item mapping
 *!       @[fun] is indexed with each element. The result of that is
 *!       collected.
 *!
 *!     @item "zero or left out"
 *!       Each element that is callable is called with @[extra] as
 *!       arguments. The result of the calls are collected. Elements
 *!       that aren't callable gets zero as result.
 *!
 *!     @item string
 *!       Each element is indexed with the given string. If the result
 *!       of that is zero then a zero is collected, otherwise it's
 *!       called with @[extra] as arguments and the result of that
 *!       call is collected.
 *!
 *!       This is typically used when @[arr] is a collection of
 *!       objects, and @[fun] is the name of some function in them.
 *!   @enddl
 *!
 *! @note
 *!   The function is never destructive on @[arr].
 *!
 *! @seealso
 *!   @[filter()], @[enumerate()], @[foreach()]
 */
PMOD_EXPORT void f_map(INT32 args)
{
   struct svalue *mysp;
   struct array *a,*d;
   int splice,i,n;
   TYPE_FIELD types;

   if (args<1)
      SIMPLE_TOO_FEW_ARGS_ERROR("map", 1);
   else if (args<2)
      { push_int(0); args++; }

   switch (Pike_sp[-args].type)
   {
      case T_ARRAY:
	 break;

      case T_MAPPING:
      case T_PROGRAM:
      case T_FUNCTION:
	 /* mapping ret =                             
	       mkmapping(indices(arr),                
	                 map(values(arr),fun,@extra)); */
	 f_aggregate(args-2);
	 mysp=Pike_sp;
	 splice=mysp[-1].u.array->size;

	 push_svalue(mysp-3); /* arr */
	 f_values(1);
	 push_svalue(mysp-2); /* fun */
	 *Pike_sp=mysp[-1];        /* extra */
	 mysp[-1].type=T_INT;
	 dmalloc_touch_svalue(Pike_sp);
	 push_array_items(Pike_sp->u.array);
	 f_map(splice+2);     /* ... arr fun extra -> ... retval */
	 stack_pop_2_elems_keep_top(); /* arr fun extra ret -> arr retval */
	 stack_swap();        /* retval arr */
	 f_indices(1);        /* retval retind */
	 stack_swap();        /* retind retval */
	 f_mkmapping(2);      /* ret :-) */
	 return;

      case T_MULTISET:
	 /* multiset ret =                             
	       (multiset)(map(indices(arr),fun,@extra)); */
	 push_svalue(Pike_sp-args);      /* take indices from arr */
	 free_svalue(Pike_sp-args-1);    /* move it to top of stack */
	 Pike_sp[-args-1].type=T_INT;    
	 f_indices(1);              /* call f_indices */
	 Pike_sp--;
	 dmalloc_touch_svalue(Pike_sp);
	 Pike_sp[-args]=Pike_sp[0];           /* move it back */
	 f_map(args);               

	 /* FIXME: Handle multisets with values like mappings. */
	 push_multiset (mkmultiset_2 (Pike_sp[-1].u.array, NULL, NULL));
	 free_array (Pike_sp[-2].u.array);
	 dmalloc_touch_svalue(Pike_sp-1);
	 Pike_sp[-2] = Pike_sp[-1];
	 Pike_sp--;
	 return;

      case T_STRING:
	 /* multiset ret =                             
	       (string)(map((array)arr,fun,@extra)); */
	 push_svalue(Pike_sp-args);      /* take indices from arr */
	 free_svalue(Pike_sp-args-1);    /* move it to top of stack */
	 Pike_sp[-args-1].type=T_INT;    
	 o_cast(NULL,T_ARRAY);      /* cast the string to an array */
	 Pike_sp--;                       
	 dmalloc_touch_svalue(Pike_sp);
	 Pike_sp[-args]=Pike_sp[0];           /* move it back */
	 f_map(args);               
	 o_cast(NULL,T_STRING);     /* cast the array to a string */
	 return;

      case T_OBJECT:
	 /* if arr->cast :              
               try map((array)arr,fun,@extra);
               try map((mapping)arr,fun,@extra);
               try map((multiset)arr,fun,@extra); */

	 mysp=Pike_sp+3-args;

	 push_svalue(mysp-3);
	 push_constant_text("cast");
	 f_arrow(2);
	 if (!UNSAFE_IS_ZERO(Pike_sp-1))
	 {
	    pop_stack();

	    push_constant_text("array");
	    /* FIXME: Object subtype! */
	    safe_apply(mysp[-3].u.object,"cast",1);
	    if (Pike_sp[-1].type==T_ARRAY)
	    {
	       free_svalue(mysp-3);
	       mysp[-3]=*(--Pike_sp);
	       dmalloc_touch_svalue(Pike_sp);
	       f_map(args);
	       return;
	    }
	    pop_stack();

	    push_constant_text("mapping");
	    /* FIXME: Object subtype! */
	    safe_apply(mysp[-3].u.object,"cast",1);
	    if (Pike_sp[-1].type==T_MAPPING)
	    {
	       free_svalue(mysp-3);
	       mysp[-3]=*(--Pike_sp);
	       dmalloc_touch_svalue(Pike_sp);
	       f_map(args);
	       return;
	    }
	    pop_stack();

	    push_constant_text("multiset");
	    /* FIXME: Object subtype! */
	    safe_apply(mysp[-3].u.object,"cast",1);
	    if (Pike_sp[-1].type==T_MULTISET)
	    {
	       free_svalue(mysp-3);
	       mysp[-3]=*(--Pike_sp);
	       dmalloc_touch_svalue(Pike_sp);
	       f_map(args);
	       return;
	    }
	    pop_stack();
	 }
	 pop_stack();

         /* if arr->_sizeof && arr->`[] 
               array ret; ret[i]=arr[i];
               ret=map(ret,fun,@extra); */

	 /* class myarray { int a0=1,a1=2; int `[](int what) { return ::`[]("a"+what); } int _sizeof() { return 2; } } 
	    map(myarray(),lambda(int in){ werror("in=%d\n",in); }); */

	 push_svalue(mysp-3);
	 push_constant_text("`[]");
	 f_arrow(2);
	 push_svalue(mysp-3);
	 push_constant_text("_sizeof");
	 f_arrow(2);
	 if (!UNSAFE_IS_ZERO(Pike_sp-2)&&!UNSAFE_IS_ZERO(Pike_sp-1))
	 {
	    f_call_function(1);
	    if (Pike_sp[-1].type!=T_INT)
	       SIMPLE_BAD_ARG_ERROR("map", 1, 
				    "object sizeof() returning integer");
	    n=Pike_sp[-1].u.integer;
	    pop_stack();
	    push_array(d=allocate_array(n));
	    types = 0;
	    stack_swap();
	    for (i=0; i<n; i++)
	    {
	       stack_dup(); /* `[] */
	       push_int(i);
	       f_call_function(2);
	       stack_pop_to_no_free (ITEM(d) + i);
	       types |= 1 << ITEM(d)->type;
	    }
	    d->type_field = types;
	    pop_stack();
	    free_svalue(mysp-3);
	    mysp[-3]=*(--Pike_sp);
	    dmalloc_touch_svalue(Pike_sp);
	    f_map(args);
	    return;
	 }
	 pop_stack();
	 pop_stack();

	 SIMPLE_BAD_ARG_ERROR("map",1,
			      "object that works in map");

      default:
	 SIMPLE_BAD_ARG_ERROR("map",1,
			      "array|mapping|program|function|"
			      "multiset|string|object");
   }

   if (UNSAFE_IS_ZERO (Pike_sp-args+1)) {
     free_svalue (Pike_sp-args+1);
     move_svalue (Pike_sp-args+1, Pike_sp-args);
     Pike_sp[-args].type = T_INT;
     mega_apply (APPLY_STACK, args-1, 0, 0);
     stack_pop_keep_top();
     return;
   }

   f_aggregate(args-2);
   mysp=Pike_sp;
   splice=mysp[-1].u.array->size;

   a=mysp[-3].u.array;
   n=a->size;

   switch (mysp[-2].type)
   {
      case T_FUNCTION:
      case T_PROGRAM:
      case T_OBJECT:
      case T_ARRAY:
	 /* ret[i]=fun(arr[i],@extra); */
         push_array(d=allocate_array(n));
	 d=Pike_sp[-1].u.array;
	 types = 0;

	 if(mysp[-2].type == T_FUNCTION &&
	    mysp[-2].subtype == FUNCTION_BUILTIN)
	 {
	   c_fun fun=mysp[-2].u.efun->function;
	   struct svalue *spbase=Pike_sp;

	   if(splice)
	   {
	     for (i=0; i<n; i++)
	     {
	       push_svalue(a->item+i);
	       add_ref_svalue(mysp-1);
	       push_array_items(mysp[-1].u.array);
	       (* fun)(1+splice);
	       if(Pike_sp>spbase)
	       {
		 stack_pop_to_no_free (ITEM(d) + i);
		 types |= 1 << ITEM(d)[i].type;
		 pop_n_elems(Pike_sp-spbase);
	       }
	     }
	   }else{
	     for (i=0; i<n; i++)
	     {
	       push_svalue(ITEM(a)+i);
	       (* fun)(1);
	       if(Pike_sp>spbase)
	       {
		 stack_pop_to_no_free (ITEM(d) + i);
		 types |= 1 << ITEM(d)[i].type;
		 pop_n_elems(Pike_sp-spbase);
	       }
	     }
	   }
	 }else{
	   for (i=0; i<n; i++)
	   {
	     push_svalue(ITEM(a)+i);
	     if (splice) 
	     {
	       add_ref_svalue(mysp-1);
	       push_array_items(mysp[-1].u.array);
	       apply_svalue(mysp-2,1+splice);
	     }
	     else
	     {
	       apply_svalue(mysp-2,1);
	     }
	     stack_pop_to_no_free (ITEM(d) + i);
	     types |= 1 << ITEM(d)[i].type;
	   }
	 }
	 d->type_field = types;
	 stack_pop_n_elems_keep_top(3); /* fun arr extra d -> d */
	 return;

      case T_MAPPING:
      case T_MULTISET:
	 /* ret[i]=fun[arr[i]]; */
	 pop_stack();
	 stack_swap();
	 f_rows(2);
	 return; 

      case T_STRING:
	 /* ret[i]=arr[i][fun](@extra); */
         push_array(d=allocate_array(n));
	 types = 0;
	 for (i=0; i<n; i++)
	 {
	    push_svalue(ITEM(a)+i);
	    push_svalue(mysp-2);
	    f_arrow(2);
	    if(UNSAFE_IS_ZERO(Pike_sp-1))
	    {
	      pop_stack();
	      continue;
	    }
	    add_ref_svalue(mysp-1);
	    push_array_items(mysp[-1].u.array);
	    f_call_function(splice+1);
	    stack_pop_to_no_free (ITEM(d) + i);
	    types |= 1 << ITEM(d)[i].type;
	 }
	 d->type_field = types;
	 stack_pop_n_elems_keep_top(3); /* fun arr extra d -> d */
	 return;

      default:
	 SIMPLE_BAD_ARG_ERROR("map",2,
			      "function|program|object|"
			      "string|int(0)|multiset");
   }      
}

/*! @decl mixed filter(mixed arr, void|mixed fun, mixed ...extra)
 *!
 *!   Filters the elements in @[arr] through @[fun].
 *!
 *!   @[arr] is treated as a set of elements to be filtered, as
 *!   follows:
 *!
 *!   @dl
 *!     @item array
 *!     @item multiset
 *!     @item string
 *!       Each element is filtered with @[fun]. The return value is of
 *!       the same type as @[arr] and it contains the elements that
 *!       @[fun] accepted. @[fun] is applied in order to each element,
 *!       and that order is retained between the kept elements.
 *!
 *!       If @[fun] is an array, it should have the same length as
 *!       @[arr]. In this case, the elements in @[arr] are kept where
 *!       the corresponding positions in @[fun] are nonzero. Otherwise
 *!       @[fun] is used as described below.
 *!
 *!     @item mapping
 *!       The values are filtered with @[fun], and the index/value
 *!       pairs it accepts are kept in the returned mapping.
 *!
 *!     @item program
 *!       The program is treated as a mapping containing the
 *!       identifiers that are indexable from it and their values.
 *!
 *!     @item object
 *!       If there is a @[lfun::cast] method in the object, it's called
 *!       to try to cast the object to an array, a mapping, or a
 *!       multiset, in that order, which is then filtered as described
 *!       above.
 *!   @enddl
 *!
 *!   Unless something else is mentioned above, @[fun] is used as
 *!   filter like this:
 *!
 *!   @dl
 *!     @item function
 *!       @[fun] is called for each element. It gets the current
 *!       element as the first argument and @[extra] as the rest. The
 *!       element is kept if it returns true, otherwise it's filtered
 *!       out.
 *!
 *!     @item object
 *!       The object is used as a function like above, i.e. the
 *!       @[lfun::`()] method in it is called.
 *!
 *!     @item multiset
 *!     @item mapping
 *!       @[fun] is indexed with each element. The element is kept if
 *!       the result is nonzero, otherwise it's filtered out.
 *!
 *!     @item "zero or left out"
 *!       Each element that is callable is called with @[extra] as
 *!       arguments. The element is kept if the result of the call is
 *!       nonzero, otherwise it's filtered out. Elements that aren't
 *!       callable are also filtered out.
 *!
 *!     @item string
 *!       Each element is indexed with the given string. If the result
 *!       of that is zero then the element is filtered out, otherwise
 *!       the result is called with @[extra] as arguments. The element
 *!       is kept if the return value is nonzero, otherwise it's
 *!       filtered out.
 *!
 *!       This is typically used when @[arr] is a collection of
 *!       objects, and @[fun] is the name of some predicate function
 *!       in them.
 *!   @enddl
 *!
 *! @note
 *!   The function is never destructive on @[arr].
 *!
 *! @seealso
 *!   @[map()], @[foreach()]
 */
PMOD_EXPORT void f_filter(INT32 args)
{
   int n,i,m,k;
   struct array *a,*y,*f;
   struct svalue *mysp;

   if (args<1)
      SIMPLE_TOO_FEW_ARGS_ERROR("filter", 1);
   
   switch (Pike_sp[-args].type)
   {
      case T_ARRAY:
	 if (args >= 2 && Pike_sp[1-args].type == T_ARRAY) {
	   if (Pike_sp[1-args].u.array->size != Pike_sp[-args].u.array->size)
	     SIMPLE_BAD_ARG_ERROR("filter", 2, "array of same size as the first");
	   pop_n_elems(args-2);
	 }
	 else {
	   MEMMOVE(Pike_sp-args+1,Pike_sp-args,args*sizeof(*Pike_sp));
	   dmalloc_touch_svalue(Pike_sp);
	   Pike_sp++;
	   add_ref_svalue(Pike_sp-args);
	   f_map(args);
	 }

	 f=Pike_sp[-1].u.array;
	 a=Pike_sp[-2].u.array;
	 n=a->size;
	 for (k=m=i=0; i<n; i++)
	    if (!UNSAFE_IS_ZERO(f->item+i))
	    {
	       push_svalue(a->item+i);
	       if (m++>32) 
	       {
		  f_aggregate(m);
		  m=0;
		  if (++k>32) {
		    f_add(k);
		    k=1;
		  }
	       }
	    }
	 if (m || !k) {
	   f_aggregate(m);
	   k++;
	 }
	 if (k > 1) f_add(k);
	 stack_pop_2_elems_keep_top();
	 return;

      case T_MAPPING:
      case T_PROGRAM:
      case T_FUNCTION:
	 /* mapping ret =                             
	       mkmapping(indices(arr),                
	                 map(values(arr),fun,@extra)); */
	 MEMMOVE(Pike_sp-args+2,Pike_sp-args,args*sizeof(*Pike_sp));
	 Pike_sp+=2;
	 Pike_sp[-args-2].type=T_INT;
	 Pike_sp[-args-1].type=T_INT;

	 push_svalue(Pike_sp-args);
	 f_indices(1);
	 dmalloc_touch_svalue(Pike_sp-1);
	 Pike_sp--;
	 Pike_sp[-args-2]=*Pike_sp;
	 dmalloc_touch_svalue(Pike_sp);
	 push_svalue(Pike_sp-args);
	 f_values(1);
	 Pike_sp--;
	 Pike_sp[-args-1]=*Pike_sp;
	 dmalloc_touch_svalue(Pike_sp);

	 assign_svalue(Pike_sp-args,Pike_sp-args-1); /* loop values only */
	 f_map(args);

	 y=Pike_sp[-3].u.array;
	 a=Pike_sp[-2].u.array;
	 f=Pike_sp[-1].u.array;
	 n=a->size;

	 for (m=i=0; i<n; i++)
	    if (!UNSAFE_IS_ZERO(f->item+i)) m++;

	 push_mapping(allocate_mapping(MAXIMUM(m,4)));

	 for (i=0; i<n; i++)
	    if (!UNSAFE_IS_ZERO(f->item+i))
	       mapping_insert(Pike_sp[-1].u.mapping,y->item+i,a->item+i);

	 stack_pop_n_elems_keep_top(3);
	 return;

      case T_MULTISET:
	 push_svalue(Pike_sp-args);      /* take indices from arr */
	 free_svalue(Pike_sp-args-1);    /* move it to top of stack */
	 Pike_sp[-args-1].type=T_INT;    
	 f_indices(1);              /* call f_indices */
	 Pike_sp--;                       
	 dmalloc_touch_svalue(Pike_sp);
	 Pike_sp[-args]=Pike_sp[0];           /* move it back */
	 f_filter(args);

	 /* FIXME: Handle multisets with values like mappings. */
	 push_multiset (mkmultiset_2 (Pike_sp[-1].u.array, NULL, NULL));
	 free_array (Pike_sp[-2].u.array);
	 Pike_sp[-2] = Pike_sp[-1];
	 dmalloc_touch_svalue(Pike_sp-1);
	 Pike_sp--;
	 return;

      case T_STRING:
	 push_svalue(Pike_sp-args);      /* take indices from arr */
	 free_svalue(Pike_sp-args-1);    /* move it to top of stack */
	 Pike_sp[-args-1].type=T_INT;    
	 o_cast(NULL,T_ARRAY);      /* cast the string to an array */
	 Pike_sp--;                       
	 dmalloc_touch_svalue(Pike_sp);
	 Pike_sp[-args]=Pike_sp[0];           /* move it back */
	 f_filter(args);               
	 o_cast(NULL,T_STRING);     /* cast the array to a string */
	 return;

      case T_OBJECT:
	 mysp=Pike_sp+3-args;

	 push_svalue(mysp-3);
	 push_constant_text("cast");
	 f_arrow(2);
	 if (!UNSAFE_IS_ZERO(Pike_sp-1))
	 {
	    pop_stack();

	    push_constant_text("array");
	    /* FIXME: Object subtype! */
	    safe_apply(mysp[-3].u.object,"cast",1);
	    if (Pike_sp[-1].type==T_ARRAY)
	    {
	       free_svalue(mysp-3);
	       mysp[-3]=*(--Pike_sp);
	       dmalloc_touch_svalue(Pike_sp);
	       f_filter(args);
	       return;
	    }
	    pop_stack();

	    push_constant_text("mapping");
	    /* FIXME: Object subtype! */
	    safe_apply(mysp[-3].u.object,"cast",1);
	    if (Pike_sp[-1].type==T_MAPPING)
	    {
	       free_svalue(mysp-3);
	       mysp[-3]=*(--Pike_sp);
	       dmalloc_touch_svalue(Pike_sp);
	       f_filter(args);
	       return;
	    }
	    pop_stack();

	    push_constant_text("multiset");
	    /* FIXME: Object subtype! */
	    safe_apply(mysp[-3].u.object,"cast",1);
	    if (Pike_sp[-1].type==T_MULTISET)
	    {
	       free_svalue(mysp-3);
	       mysp[-3]=*(--Pike_sp);
	       dmalloc_touch_svalue(Pike_sp);
	       f_filter(args);
	       return;
	    }
	    pop_stack();
	 }
	 pop_stack();

	 SIMPLE_BAD_ARG_ERROR("filter",1,
			      "...|object that can be cast to array, multiset or mapping");

      default:
	 SIMPLE_BAD_ARG_ERROR("filter",1,
			      "array|mapping|program|function|"
			      "multiset|string|object");
   }
}

/* map() and filter() inherit sideeffects from their
 * second argument.
 */
static node *fix_map_node_info(node *n)
{
  int argno;
  node **cb_;
  /* Assume worst case. */
  int node_info = OPT_SIDE_EFFECT | OPT_EXTERNAL_DEPEND;

  /* Note: argument 2 has argno 1. */
  for (argno = 1; (cb_ = my_get_arg(&_CDR(n), argno)); argno++) {
    node *cb = *cb_;

    if ((cb->token == F_CONSTANT) &&
	(cb->u.sval.type == T_FUNCTION) &&
	(cb->u.sval.subtype == FUNCTION_BUILTIN)) {
      if (cb->u.sval.u.efun->optimize == fix_map_node_info) {
	/* map() or filter(). */
	continue;
      }
      node_info &= cb->u.sval.u.efun->flags;
    }
    /* FIXME: Type-checking? */
    break;
  }

  if (!cb_) {
    yyerror("Too few arguments to map() or filter()!\n");
  }

  n->node_info |= node_info;
  n->tree_info |= node_info;

  return 0;	/* continue optimization */
}

/*! @decl array(int) enumerate(int n)
 *! @decl array enumerate(int n, void|mixed step, void|mixed start, @
 *!                       void|function operator)
 *!
 *!   Create an array with an enumeration, useful for initializing arrays
 *!   or as first argument to @[map()] or @[foreach()].
 *!
 *!   The defaults are: @[step] = 1, @[start] = 0, @[operator] = @[`+]
 *!
 *!   @section Advanced use
 *!   	The resulting array is calculated like this:
 *! @code
 *! array enumerate(int n, mixed step, mixed start, function operator)
 *! {
 *!   array res = allocate(n);
 *!   for (int i=0; i < n; i++)
 *!   {
 *!     res[i] = start;
 *!     start = operator(start, step);
 *!   }
 *!   return res;
 *! }
 *! @endcode
 *!   @endsection
 *!
 *! @seealso
 *!   @[map()], @[foreach()]
 */
void f_enumerate(INT32 args)
{
   struct array *d;
   int i;
   INT_TYPE n;

   if (args<1)
      SIMPLE_TOO_FEW_ARGS_ERROR("enumerate", 1);
   if (args<2) 
   {
      push_int(1);
      args++;
   }
   if (args<3)
   {
      push_int(0);
      args++;
   }

   if (args<=3 &&
       (Pike_sp[1-args].type==T_INT &&
	Pike_sp[2-args].type==T_INT))
   {
      INT_TYPE step,start;

      get_all_args("enumerate", args, "%i%i%i", &n, &step, &start);
      if (n<0) 
	 SIMPLE_BAD_ARG_ERROR("enumerate",1,"int(0..)");

      pop_n_elems(args);
      push_array(d=allocate_array(n));
      for (i=0; i<n; i++)
      {
	 ITEM(d)[i].u.integer=start;
#ifdef AUTO_BIGNUM
	 if ((step>0 && start+step<start) ||
	     (step<0 && start+step>start)) /* overflow */
	 {
	    pop_stack();
	    push_int(n);
	    push_int(step);
	    convert_stack_top_to_bignum();
	    push_int(start);
	    convert_stack_top_to_bignum();
	    f_enumerate(3);
	    return;
	 }
#endif
	 start+=step;
      }
      d->type_field = BIT_INT;
   }
   else if (args<=3 &&
	    ((Pike_sp[1-args].type==T_INT ||
	      Pike_sp[1-args].type==T_FLOAT) &&
	     (Pike_sp[2-args].type==T_INT ||
	      Pike_sp[2-args].type==T_FLOAT) ) )
   {
      FLOAT_TYPE step, start;

      get_all_args("enumerate", args, "%i%F%F", &n, &step, &start);
      if (n<0) 
	 SIMPLE_BAD_ARG_ERROR("enumerate",1,"int(0..)");

      pop_n_elems(args);

      push_array(d=allocate_array(n));
      for (i=0; i<n; i++)
      {
	 d->item[i].u.float_number=start;
	 d->item[i].type=T_FLOAT;
	 start+=step;
      }
      d->type_field = BIT_FLOAT;
   }
   else
   {
      TYPE_FIELD types = 0;
      get_all_args("enumerate", args, "%i", &n);
      if (n<0) SIMPLE_BAD_ARG_ERROR("enumerate",1,"int(0..)");
      if (args>4) pop_n_elems(args-4);
      push_array(d=allocate_array(n));
      if (args<4)
      {
	 push_svalue(Pike_sp-2); /* start */
	 for (i=0; i<n; i++)
	 {
	    assign_svalue_no_free(ITEM(d)+i,Pike_sp-1);
	    types |= 1 << ITEM(d)[i].type;
	    if (i<n-1)
	    {
	       push_svalue(Pike_sp-4); /* step */
	       f_add(2);
	    }
	 }
      }
      else
      {
	 push_svalue(Pike_sp-3); /* start */
	 for (i=0; i<n; i++)
	 {
	    assign_svalue_no_free(ITEM(d)+i,Pike_sp-1);
	    types |= 1 << ITEM(d)[i].type;
	    if (i<n-1)
	    {
	       push_svalue(Pike_sp-3); /* function */
	       stack_swap();
	       push_svalue(Pike_sp-6); /* step */
	       f_call_function(3);
	    }
	 }
      }
      d->type_field = types;
      pop_stack();
      stack_pop_n_elems_keep_top(args);
   }
}

/*! @module Program
 */

/*! @decl array(program) inherit_list(program p)
 *!
 *!   Returns an array with the programs that @[p] has inherited.
 */
PMOD_EXPORT void f_inherit_list(INT32 args)
{
  struct program *p;
  struct svalue *arg;
  struct object *par;
  int parid,e,q=0;

  get_all_args("inherit_list",args,"%*",&arg);
  if(Pike_sp[-args].type == T_OBJECT)
    f_object_program(1);
  
  p=program_from_svalue(arg);
  if(!p) 
    SIMPLE_BAD_ARG_ERROR("inherit_list", 1, "program");

  if(arg->type == T_FUNCTION)
  {
    par=arg->u.object;
    parid=arg->subtype;
  }else{
    par=0;
    parid=-1;
  }

  check_stack(p->num_inherits);
  for(e=0;e<p->num_inherits;e++)
  {
    struct inherit *in=p->inherits+e;

    if(in->inherit_level==1)
    {
      if(in->prog->flags & PROGRAM_USES_PARENT)
      {
	switch(in->parent_offset)
	{
	  default:
	  {
	    struct external_variable_context tmp;
	    if(!par)
	    {
	      ref_push_program(in->prog);
	    }else{
	      tmp.o=par;
	      tmp.parent_identifier=parid;
	      tmp.inherit=INHERIT_FROM_INT(par->prog,parid);
	      
	      find_external_context(&tmp, in->parent_offset-1);
	      ref_push_function(tmp.o,
				in->parent_identifier +
				tmp.inherit->identifier_level);
	    }
	  }
	  break;
	  
	  case INHERIT_PARENT:
	    ref_push_function(in->parent, in->parent_identifier);
	    break;
	    
	  case OBJECT_PARENT:
	    if(par)
	    {
	      ref_push_function(par, parid);
	    }else{
	      ref_push_program(in->prog);
	    }
	    break;
	}
      }else{
	ref_push_program(in->prog);
      }
      q++;
    }
  }
  f_aggregate(q);
}

/*! @endmodule
 */

/*! @module Function
 */

/*! @decl string defined(function fun)
 *!
 *!   Returns a string with filename and linenumber where @[fun]
 *!   was defined.
 *!
 *!   Returns @expr{0@} (zero) when no line can be found, e.g. for
 *!   builtin functions and functions in destructed objects.
 */
PMOD_EXPORT void f_function_defined(INT32 args)
{
  check_all_args("Function.defined",args,BIT_FUNCTION, 0);

  if(Pike_sp[-args].subtype != FUNCTION_BUILTIN &&
     Pike_sp[-args].u.object->prog)
  {
    struct program *p = Pike_sp[-args].u.object->prog;
    struct program *id_prog, *p2;
    int func = Pike_sp[-args].subtype;
    struct identifier *id;
    INT32 line;
    struct pike_string *file = NULL;

    if (p == pike_trampoline_program) {
      struct pike_trampoline *t =
	(struct pike_trampoline *) Pike_sp[-args].u.object->storage;
      if (t->frame->current_object->prog) {
	p = t->frame->current_object->prog;
	func = t->func;
      }
    }

    id=ID_FROM_INT(p, func);
    id_prog = PROG_FROM_INT (p, func);

    if(IDENTIFIER_IS_PIKE_FUNCTION( id->identifier_flags ) &&
      id->func.offset != -1)
      file = low_get_line(id_prog->program + id->func.offset, id_prog, &line);
    else if (IDENTIFIER_IS_CONSTANT (id->identifier_flags) &&
	     id->func.offset >= 0 &&
	     (p2 = program_from_svalue (&id_prog->constants[id->func.offset].sval)))
      file = low_get_program_line (p2, &line);
    else
      /* The program line is better than nothing for C functions. */
      file = low_get_program_line (p, &line);

    if (file)
    {
      pop_n_elems(args);
      if (line) {
	push_string(file);
	push_constant_text(":");
	push_int(line);
	f_add(3);
      }
      else
	push_string (file);
      return;
    }
  }

  pop_n_elems(args);
  push_int(0);
  
}

/*! @endmodule Function
 */

void init_builtin_efuns(void)
{
  struct program *pike___master_program;

  ADD_EFUN("gethrvtime",f_gethrvtime,
	   tFunc(tOr(tInt,tVoid),tInt), OPT_EXTERNAL_DEPEND);
  ADD_EFUN("gethrtime", f_gethrtime,
	   tFunc(tOr(tInt,tVoid),tInt), OPT_EXTERNAL_DEPEND);

#ifdef PROFILING
  ADD_EFUN("get_profiling_info", f_get_prof_info,
	   tFunc(tPrg(tObj),tArray), OPT_EXTERNAL_DEPEND);
#endif /* PROFILING */

  ADD_EFUN("_refs",f__refs,tFunc(tRef,tInt),OPT_EXTERNAL_DEPEND);
#ifdef PIKE_DEBUG
  ADD_EFUN("_leak",f__leak,tFunc(tRef,tInt),OPT_EXTERNAL_DEPEND);
#endif
  ADD_EFUN("_typeof", f__typeof, tFunc(tSetvar(0, tMix), tType(tVar(0))), 0);

  /* class __master
   * Used to prototype the master object.
   */
  start_new_program();
  ADD_PROTOTYPE("_main", tFunc(tArr(tStr) tArr(tStr),tVoid), 0);

  ADD_PROTOTYPE("cast_to_object", tFunc(tStr tStr tOr(tVoid, tObj), tObj), 0);
  ADD_PROTOTYPE("cast_to_program", tFunc(tStr tStr tOr(tVoid, tObj), tPrg(tObj)), 0);
  ADD_PROTOTYPE("compile_error", tFunc(tStr tInt tStr, tVoid), 0);
  ADD_PROTOTYPE("compile_warning", tFunc(tStr tInt tStr, tVoid), 0);
  ADD_PROTOTYPE("decode_charset", tFunc(tStr tStr, tStr), 0);
  ADD_PROTOTYPE("describe_backtrace", tFunc(tOr(tObj, tArr(tMix)) tOr(tVoid, tInt), tStr), 0);
  ADD_PROTOTYPE("handle_error", tFunc(tOr(tArr(tMix),tObj), tVoid), 0);
  ADD_PROTOTYPE("handle_import",
		tFunc(tStr tOr(tStr, tVoid) tOr(tObj, tVoid), tMix), 0);
  ADD_PROTOTYPE("handle_include", tFunc(tStr tStr tInt, tStr), 0);
  ADD_PROTOTYPE("handle_inherit", tFunc(tStr tStr tOr(tObj, tVoid), tPrg(tObj)), 0);
  ADD_PROTOTYPE("write", tFunc(tStr tOr(tVoid,tMix), tVoid), OPT_SIDE_EFFECT);
  ADD_PROTOTYPE("werror", tFunc(tStr tOr(tVoid,tMix), tVoid), OPT_SIDE_EFFECT);
  
  /* FIXME: Are these three actually supposed to be used?
   * They are called by encode.c:rec_restore_value
   *	/grubba 2000-03-13
   */

#if 0 /* they are not required - Hubbe */
  ADD_PROTOTYPE("functionof", tFunc(tStr, tFunction), ID_OPTIONAL);
  ADD_PROTOTYPE("objectof", tFunc(tStr, tObj), ID_OPTIONAL);
  ADD_PROTOTYPE("programof", tFunc(tStr, tPrg(tObj)), ID_OPTIONAL);
#endif

  ADD_PROTOTYPE("read_include", tFunc(tStr, tStr), 0);
  ADD_PROTOTYPE("resolv",
		tFunc(tStr tOr(tStr,tVoid) tOr(tObj,tVoid), tMix), 0);

#if 0
  /* Getenv and putenv are efuns, they do not HAVE to be defined in the
   * master object. -Hubbe
   */

  /* These two aren't called from C-code, but are popular from other code. */
  ADD_PROTOTYPE("getenv",
		tOr(tFunc(tStr,tStr), tFunc(tNone, tMap(tStr, tStr))),
		ID_OPTIONAL);
  ADD_PROTOTYPE("putenv", tFunc(tStr tStr, tVoid), ID_OPTIONAL);

#endif


  pike___master_program = end_program();
  add_program_constant("__master", pike___master_program, 0);

  /* FIXME: */
  ADD_EFUN("replace_master", f_replace_master,
	   tFunc(tObj, tVoid), OPT_SIDE_EFFECT);
  ADD_EFUN("master", f_master,
	   tFunc(tNone, tObj), OPT_EXTERNAL_DEPEND);
#if 0 /* FIXME: dtFunc isn't USE_PIKE_TYPE compatible */
  ADD_EFUN_DTYPE("replace_master", f_replace_master,
		 dtFunc(dtObjImpl(pike___master_program), dtVoid),
		 OPT_SIDE_EFFECT);

  /* function(:object) */
  /* FIXME: */
  ADD_EFUN_DTYPE("master", f_master,
		 dtFunc(dtNone, dtObjImpl(pike___master_program)),
		 OPT_EXTERNAL_DEPEND);
#endif /* 0 */
  
  /* __master still contains a reference */
  free_program(pike___master_program);
  
/* function(string,void|mixed:void) */
  ADD_EFUN("add_constant", f_add_constant,
	   tFunc(tStr tOr(tVoid,tMix),tVoid),OPT_SIDE_EFFECT);

/* function(0=mixed ...:array(0)) */
  ADD_EFUN("aggregate",debug_f_aggregate,
	   tFuncV(tNone,tSetvar(0,tMix),tArr(tVar(0))),OPT_TRY_OPTIMIZE);
  
/* function(0=mixed ...:multiset(0)) */
  ADD_EFUN("aggregate_multiset",f_aggregate_multiset,
	   tFuncV(tNone,tSetvar(0,tMix),tSet(tVar(0))),OPT_TRY_OPTIMIZE);
  
/* function(0=mixed ...:mapping(0:0)) */
  ADD_EFUN2("aggregate_mapping",f_aggregate_mapping,
	    tFuncV(tNone,tSetvar(0,tMix),tMap(tVar(0),tVar(0))),
	    OPT_TRY_OPTIMIZE, fix_aggregate_mapping_type, 0);
  
/* function(:mapping(string:mixed)) */
  ADD_EFUN("all_constants",f_all_constants,
	   tFunc(tNone,tMap(tStr,tMix)),OPT_EXTERNAL_DEPEND);
  
/* function(int,void|0=mixed:array(0)) */
  ADD_EFUN("allocate", f_allocate,
	   tFunc(tInt tOr(tVoid,tSetvar(0,tMix)),tArr(tVar(0))), 0);
  
/* function(mixed:int) */
  ADD_EFUN("arrayp", f_arrayp,tFunc(tMix,tInt),0);

/* function(string...:string) */
  ADD_EFUN("combine_path_nt",f_combine_path_nt,tFuncV(tNone,tStr,tStr),0);
  ADD_EFUN("combine_path_unix",f_combine_path_unix,tFuncV(tNone,tStr,tStr),0);
  ADD_EFUN("combine_path_amigaos",f_combine_path_amigaos,tFuncV(tNone,tStr,tStr),0);
#ifdef __NT__
  ADD_EFUN("combine_path",f_combine_path_nt,tFuncV(tNone,tStr,tStr),0);
#else
#ifdef __amigaos__
  ADD_EFUN("combine_path",f_combine_path_amigaos,tFuncV(tNone,tStr,tStr),0);
#else
  ADD_EFUN("combine_path",f_combine_path_unix,tFuncV(tNone,tStr,tStr),0);
#endif
#endif
  
  ADD_EFUN("compile", f_compile,
	   tFunc(tStr tOr(tObj, tVoid) tOr(tInt, tVoid) tOr(tInt, tVoid) tOr(tPrg(tObj), tVoid) tOr(tObj, tVoid) ,tPrg(tObj)),
	   OPT_EXTERNAL_DEPEND);
  
/* function(1=mixed:1) */
  ADD_EFUN("copy_value",f_copy_value,tFunc(tSetvar(1,tMix),tVar(1)),0);
  
/* function(string:string)|function(string,string:int) */
  ADD_EFUN("crypt",f_crypt,
	   tOr(tFunc(tStr,tStr),tFunc(tStr tStr,tInt)),OPT_EXTERNAL_DEPEND);
  
/* function(object|void:void) */
  ADD_EFUN("destruct",f_destruct,tFunc(tOr(tObj,tVoid),tVoid),OPT_SIDE_EFFECT);
  
/* function(mixed,mixed:int) */
  ADD_EFUN("equal",f_equal,tFunc(tMix tMix,tInt),OPT_TRY_OPTIMIZE);

  /* function(array(0=mixed),int|void,int|void:array(0)) */
  ADD_FUNCTION2("everynth",f_everynth,
		tFunc(tArr(tSetvar(0,tMix)) tOr(tInt,tVoid) tOr(tInt,tVoid),
		      tArr(tVar(0))), 0, OPT_TRY_OPTIMIZE);
  
/* function(int:void) */
  ADD_EFUN("exit",f_exit,tFuncV(tInt tOr(tVoid,tStr),tOr(tVoid,tMix),tVoid),
	   OPT_SIDE_EFFECT);
  
/* function(int:void) */
  ADD_EFUN("_exit",f__exit,tFunc(tInt,tVoid),OPT_SIDE_EFFECT);
  
/* function(mixed:int) */
  ADD_EFUN("floatp",  f_floatp,tFunc(tMix,tInt),OPT_TRY_OPTIMIZE);
  
/* function(mixed:int) */
  ADD_EFUN("functionp",  f_functionp,tFunc(tMix,tInt),OPT_TRY_OPTIMIZE);

/* function(mixed:int) */
  ADD_EFUN("callablep",  f_callablep,tFunc(tMix,tInt),OPT_TRY_OPTIMIZE);
  
/* function(string,string:int)|function(string,string*:array(string)) */
  ADD_EFUN("glob",f_glob,
	   tOr(tFunc(tStr tStr,tInt),tFunc(tStr tArr(tStr),tArr(tStr))),
	   OPT_TRY_OPTIMIZE);
  
/* function(string,int|void:int) */
  ADD_EFUN("hash",f_hash,tFunc(tStr tOr(tInt,tVoid),tInt),OPT_TRY_OPTIMIZE);

  ADD_EFUN("hash_7_0",f_hash_7_0,
           tFunc(tStr tOr(tInt,tVoid),tInt),OPT_TRY_OPTIMIZE);

  ADD_EFUN("hash_7_4",f_hash_7_4,
           tFunc(tStr tOr(tInt,tVoid),tInt),OPT_TRY_OPTIMIZE);

  ADD_EFUN("hash_value",f_hash_value,tFunc(tMix,tInt),OPT_TRY_OPTIMIZE);

/* function(string|array:int*)|function(mapping(1=mixed:mixed)|multiset(1=mixed):array(1))|function(object|program:string*) */
  ADD_EFUN2("indices",f_indices,
	   tOr3(tFunc(tOr(tStr,tArray),tArr(tInt)),
		tFunc(tOr(tMap(tSetvar(1,tMix),tMix),tSet(tSetvar(1,tMix))),
		      tArr(tVar(1))),
		tFunc(tOr(tObj,tPrg(tObj)),tArr(tStr))),
	    OPT_TRY_OPTIMIZE,fix_indices_type,0);
  
/* function(mixed:int) */
  ADD_EFUN("intp", f_intp,tFunc(tMix,tInt),OPT_TRY_OPTIMIZE);
  
/* function(mixed:int) */
  ADD_EFUN("multisetp", f_multisetp,tFunc(tMix,tInt),OPT_TRY_OPTIMIZE);
  
/* function(string:string)|function(int:int) */
  ADD_EFUN("lower_case",f_lower_case,
	   tOr(tFunc(tStr,tStr), tFunc(tInt,tInt)),OPT_TRY_OPTIMIZE);
  
/* function(mixed:int) */
  ADD_EFUN("mappingp",f_mappingp,tFunc(tMix,tInt),OPT_TRY_OPTIMIZE);
  
/* function(1=mixed,int:1) */
  ADD_EFUN("set_weak_flag",f_set_weak_flag,
	   tFunc(tSetvar(1,tMix) tInt,tVar(1)),OPT_SIDE_EFFECT);

  ADD_INT_CONSTANT("PIKE_WEAK_INDICES", PIKE_WEAK_INDICES, 0);
  ADD_INT_CONSTANT("PIKE_WEAK_VALUES", PIKE_WEAK_VALUES, 0);

/* function(void|object:object) */
  ADD_EFUN("next_object",f_next_object,
	   tFunc(tOr(tVoid,tObj),tObj),OPT_EXTERNAL_DEPEND);
  
/* function(string:string)|function(object:object)|function(mapping:mapping)|function(multiset:multiset)|function(program:program)|function(array:array) */
  ADD_EFUN("_next",f__next,
	   tOr6(tFunc(tStr,tStr),
		tFunc(tObj,tObj),
		tFunc(tMapping,tMapping),
		tFunc(tMultiset,tMultiset),
		tFunc(tPrg(tObj),tPrg(tObj)),
		tFunc(tArray,tArray)),OPT_EXTERNAL_DEPEND);
  
/* function(object:object)|function(mapping:mapping)|function(multiset:multiset)|function(program:program)|function(array:array) */
  ADD_EFUN("_prev",f__prev,
	   tOr5(tFunc(tObj,tObj),
		tFunc(tMapping,tMapping),
		tFunc(tMultiset,tMultiset),
		tFunc(tPrg(tObj),tPrg(tObj)),
		tFunc(tArray,tArray)),OPT_EXTERNAL_DEPEND);
  
  /* function(mixed:program|function) */
  ADD_EFUN2("object_program", f_object_program,
	    tFunc(tMix, tOr(tPrg(tObj),tFunction)),
	    OPT_TRY_OPTIMIZE, fix_object_program_type, 0);
  
/* function(mixed:int) */
  ADD_EFUN("objectp", f_objectp,tFunc(tMix,tInt),0);
  
/* function(mixed:int) */
  ADD_EFUN("programp",f_programp,tFunc(tMix,tInt),0);
  
/* function(:int) */
  ADD_EFUN("query_num_arg",f_query_num_arg,
	   tFunc(tNone,tInt),OPT_EXTERNAL_DEPEND);
  
/* function(int:void) */
  ADD_EFUN("random_seed",f_random_seed,
	   tFunc(tInt,tVoid),OPT_SIDE_EFFECT);

  ADD_EFUN("random_string",f_random_string,
	   tFunc(tInt,tString), OPT_EXTERNAL_DEPEND);
  
  ADD_EFUN2("replace", f_replace,
	    tOr5(tFunc(tStr tStr tStr,tStr),
		 tFunc(tStr tArr(tStr) tArr(tStr),tStr),
		 tFunc(tStr tMap(tStr,tStr),tStr),
		 tFunc(tSetvar(0,tArray) tMix tMix,tVar(0)),
		 tFunc(tSetvar(1,tMapping) tMix tMix,tVar(1))),
	    OPT_TRY_OPTIMIZE, optimize_replace, 0);
  
/* function(int:int)|function(string:string)|function(0=array:0) */
  ADD_EFUN("reverse",f_reverse,
	   tOr3(tFunc(tInt,tInt),
		tFunc(tStr,tStr),
		tFunc(tSetvar(0, tArray),tVar(0))),0);
  
/* function(mixed,array:array) */
  ADD_EFUN("rows",f_rows,
	   tOr6(tFunc(tMap(tSetvar(0,tMix),tSetvar(1,tMix)) tArr(tVar(0)),
		      tArr(tVar(1))),
		tFunc(tSet(tSetvar(0,tMix)) tArr(tVar(0)), tArr(tInt01)),
		tFunc(tString tArr(tInt), tArr(tInt)),
		tFunc(tArr(tSetvar(0,tMix)) tArr(tInt), tArr(tVar(1))),
		tFunc(tArray tArr(tNot(tInt)), tArray),
		tFunc(tOr4(tObj,tFunction,tPrg(tObj),tInt) tArray, tArray)), 0);

  /* FIXME: Is the third arg a good idea when the first is a mapping? */
  ADD_EFUN("search",f_search,
	   tOr4(tFunc(tStr tOr(tStr,tInt) tOr(tVoid,tInt),
		      tInt),
		tFunc(tArr(tSetvar(0,tMix)) tVar(0) tOr(tVoid,tInt),
		      tInt),
		tFunc(tMap(tSetvar(1,tMix),tSetvar(2,tMix)) tVar(2) tOr(tVoid,tVar(1)),
		      tVar(1)),

		tIfnot(tOr(tFunc(tNot(tArray) tMix tOr(tVoid,tInt), tMix),
			   tFunc(tNot(tMapping) tMix tOr(tVoid,tInt), tMix)),
		       tFunc(tOr(tMapping, tArray) tMix tOr(tVoid,tInt),
			     tZero))),
	   0);
  
  ADD_EFUN2("has_prefix", f_has_prefix, tFunc(tStr tStr,tInt01),
	    OPT_TRY_OPTIMIZE, 0, 0);

  ADD_EFUN2("has_suffix", f_has_suffix, tFunc(tStr tStr,tInt01),
	    OPT_TRY_OPTIMIZE, 0, 0);

  ADD_EFUN("has_index",f_has_index,
	   tOr5(tFunc(tStr tIntPos, tInt01),
		tFunc(tArray tIntPos, tInt01),
		tFunc(tSet(tSetvar(0,tMix)) tVar(0), tInt01),
		tFunc(tMap(tSetvar(1,tMix),tMix) tVar(1), tInt01),
		tFunc(tObj tMix, tInt01)),
	   OPT_TRY_OPTIMIZE);

  ADD_EFUN("has_value",f_has_value,
	   tOr5(tFunc(tStr tOr(tStr, tInt), tInt01),
		tFunc(tArr(tSetvar(0,tMix)) tVar(0), tInt01),
		tFunc(tMultiset tInt, tInt01),
		tFunc(tMap(tMix,tSetvar(1,tMix)) tVar(1), tInt01),
		tFunc(tObj tMix, tInt01)),
	   OPT_TRY_OPTIMIZE);

/* function(float|int,int|void:void) */
  ADD_EFUN("sleep", f_sleep,
	   tFunc(tOr(tFlt,tInt) tOr(tInt,tVoid),tVoid),OPT_SIDE_EFFECT);
  ADD_EFUN("delay", f_delay,
	   tFunc(tOr(tFlt,tInt) tOr(tInt,tVoid),tVoid),OPT_SIDE_EFFECT);
  
/* function(array(0=mixed),array(mixed)...:array(0)) */
  ADD_EFUN("sort",f_sort,
	   tFuncV(tArr(tSetvar(0,tMix)),tArr(tMix),tArr(tVar(0))),
	   OPT_SIDE_EFFECT);

  /* function(array(0=mixed)...:array(0)) */
  ADD_FUNCTION2("splice",f_splice,
		tFuncV(tNone,tArr(tSetvar(0,tMix)),tArr(tVar(0))), 0,
		OPT_TRY_OPTIMIZE);

  /* function(array:array) */
  ADD_FUNCTION2("uniq_array", f_uniq_array,
		tFunc(tArr(tSetvar(0,tMix)), tArr(tVar(0))), 0,
		OPT_TRY_OPTIMIZE);
  
/* function(mixed:int) */
  ADD_EFUN("stringp", f_stringp, tFunc(tMix,tInt01), 0);

  ADD_EFUN2("this_object", f_this_object,tFunc(tOr(tVoid,tIntPos),tObj),
	    OPT_EXTERNAL_DEPEND, optimize_this_object, generate_this_object);
  
/* function(mixed:void) */
  ADD_EFUN("throw",f_throw,tFunc(tMix,tVoid),OPT_SIDE_EFFECT);
  
/* function(void|int(0..1):int(2..))|function(int(2..):float) */
  ADD_EFUN("time",f_time,
	   tOr(tFunc(tOr(tVoid,tInt01),tInt2Plus),
	       tFunc(tInt2Plus,tFlt)),
	   OPT_EXTERNAL_DEPEND);
  
  /* function(array(0=mixed):array(0)) */
  ADD_FUNCTION2("transpose",f_transpose,
		tFunc(tArr(tSetvar(0,tMix)),tArr(tVar(0))), 0,
		OPT_TRY_OPTIMIZE);
  
/* function(string:string)|function(int:int) */
  ADD_EFUN("upper_case",f_upper_case,
	   tOr(tFunc(tStr,tStr),tFunc(tInt,tInt)),OPT_TRY_OPTIMIZE);

/* function(string|multiset:array(int))|function(array(0=mixed)|mapping(mixed:0=mixed)|object|program:array(0)) */
  ADD_EFUN2("values",f_values,
	   tOr(tFunc(tOr(tStr,tMultiset),tArr(tInt)),
	       tFunc(tOr4(tArr(tSetvar(0,tMix)),
			  tMap(tMix,tSetvar(0,tMix)),
			  tObj,tPrg(tObj)),
		     tArr(tVar(0)))),0,fix_values_type,0);
  
/* function(mixed:int) */
  ADD_EFUN2("zero_type",f_zero_type,tFunc(tMix,tInt01),0,0,generate_zero_type);
  
/* function(string,string:array) */
  ADD_EFUN("array_sscanf",f_sscanf,tFunc(tStr tStr,tArray),0);

  /* Some Wide-string stuff */
  
/* function(string:string) */
  ADD_EFUN("string_to_unicode", f_string_to_unicode,
	   tFunc(tStr,tStr), OPT_TRY_OPTIMIZE);
  
/* function(string:string) */
  ADD_EFUN("unicode_to_string", f_unicode_to_string,
	   tFunc(tStr,tStr), OPT_TRY_OPTIMIZE);
  
/* function(string,int|void:string) */
  ADD_EFUN("string_to_utf8", f_string_to_utf8,
	   tFunc(tStr tOr(tInt,tVoid),tStr), OPT_TRY_OPTIMIZE);
  
/* function(string,int|void:string) */
  ADD_EFUN("utf8_to_string", f_utf8_to_string,
	   tFunc(tStr tOr(tInt,tVoid),tStr), OPT_TRY_OPTIMIZE);


  ADD_EFUN("__parse_pike_type", f_parse_pike_type,
	   tFunc(tStr,tStr),OPT_TRY_OPTIMIZE);

#ifdef HAVE_LOCALTIME
  
/* function(int:mapping(string:int)) */
  ADD_EFUN("localtime",f_localtime,
	   tFunc(tInt,tMap(tStr,tInt)),OPT_EXTERNAL_DEPEND);
#endif
#ifdef HAVE_GMTIME
  
/* function(int:mapping(string:int)) */
  ADD_EFUN("gmtime",f_gmtime,tFunc(tInt,tMap(tStr,tInt)),OPT_TRY_OPTIMIZE);
#endif

#ifdef GOT_F_MKTIME
  
/* function(int,int,int,int,int,int,int,void|int:int)|function(object|mapping:int) */
  ADD_EFUN("mktime",f_mktime,
	   tOr(tFunc(tInt tInt tInt tInt tInt tInt
		     tOr(tVoid,tInt) tOr(tVoid,tInt),tInt),
	       tFunc(tOr(tObj,tMapping),tInt)),OPT_TRY_OPTIMIZE);
#endif

/* function(:void) */
  ADD_EFUN("_verify_internals",f__verify_internals,
	   tFunc(tNone,tVoid),OPT_SIDE_EFFECT|OPT_EXTERNAL_DEPEND);

#ifdef PIKE_DEBUG
  
/* function(int:int) */
  ADD_EFUN("_debug",f__debug,
	   tFunc(tInt,tInt),OPT_SIDE_EFFECT|OPT_EXTERNAL_DEPEND);

/* function(int:int) */
  ADD_EFUN("_optimizer_debug",f__optimizer_debug,
	   tFunc(tInt,tInt),OPT_SIDE_EFFECT|OPT_EXTERNAL_DEPEND);

/* function(int:int) */
  ADD_EFUN("_assembler_debug",f__assembler_debug,
	   tFunc(tInt,tInt), OPT_SIDE_EFFECT|OPT_EXTERNAL_DEPEND);

#ifdef YYDEBUG
  
/* function(int:int) */
  ADD_EFUN("_compiler_trace",f__compiler_trace,
	   tFunc(tInt,tInt),OPT_SIDE_EFFECT|OPT_EXTERNAL_DEPEND);
#endif /* YYDEBUG */
#endif
  
/* function(:mapping(string:int)) */
  ADD_EFUN("_memory_usage",f__memory_usage,
	   tFunc(tNone,tMap(tStr,tInt)),OPT_EXTERNAL_DEPEND);

  
/* function(:int) */
  ADD_EFUN("gc",f_gc,tFunc(tNone,tInt),OPT_SIDE_EFFECT);
  
/* function(:string) */
  ADD_EFUN("version", f_version,tFunc(tNone,tStr), OPT_TRY_OPTIMIZE);

  /* Note: The last argument to the encode and decode functions is
   * intentionally not part of the prototype, to keep it free for
   * other uses in the future. */

/* function(mixed,void|object:string) */
  ADD_EFUN("encode_value", f_encode_value,
	   tFunc(tMix tOr(tVoid,tObj),tStr), OPT_TRY_OPTIMIZE);

  /* function(mixed,void|object:string) */
  ADD_EFUN("encode_value_canonic", f_encode_value_canonic,
	   tFunc(tMix tOr(tVoid,tObj),tStr), OPT_TRY_OPTIMIZE);

/* function(string,void|object:mixed) */
  ADD_EFUN("decode_value", f_decode_value,
	   tFunc(tStr tOr(tVoid,tObj),tMix), OPT_TRY_OPTIMIZE);
  
/* function(object,string:int) */
  ADD_EFUN("object_variablep", f_object_variablep,
	   tFunc(tObj tStr,tInt), OPT_EXTERNAL_DEPEND);

  /* function(array(mapping(int:mixed)):array(int)) */
  ADD_FUNCTION2("interleave_array", f_interleave_array,
		tFunc(tArr(tMap(tInt, tMix)), tArr(tInt)), 0,
		OPT_TRY_OPTIMIZE);
  /* function(array(0=mixed),array(1=mixed):array(array(array(0)|array(1))) */
  ADD_FUNCTION2("diff", f_diff,
		tFunc(tArr(tSetvar(0,tMix)) tArr(tSetvar(1,tMix)),
		      tArr(tArr(tOr(tArr(tVar(0)),tArr(tVar(1)))))), 0,
		OPT_TRY_OPTIMIZE);

  /* Generate the n:th permutation of the array given as the first argument */
  ADD_FUNCTION2("permute", f_permute, tFunc(tArray tInt,tArray), 0,
		OPT_TRY_OPTIMIZE);

  /* function(array,array:array(int)) */
  ADD_FUNCTION2("diff_longest_sequence", f_diff_longest_sequence,
		tFunc(tArray tArray,tArr(tInt)), 0, OPT_TRY_OPTIMIZE);
  /* function(array,array:array(int)) */
  ADD_FUNCTION2("diff_dyn_longest_sequence", f_diff_dyn_longest_sequence,
		tFunc(tArray tArray,tArr(tInt)), 0, OPT_TRY_OPTIMIZE);
  /* function(array,array:array(array)) */
  ADD_FUNCTION2("diff_compare_table", f_diff_compare_table,
		tFunc(tArray tArray, tArr(tArr(tInt))), 0, OPT_TRY_OPTIMIZE);
  /* function(array:array(int)) */
  ADD_FUNCTION2("longest_ordered_sequence", f_longest_ordered_sequence,
		tFunc(tArray,tArr(tInt)), 0, OPT_TRY_OPTIMIZE);

#define tMapStuff(IN,SUB,OUTFUN,OUTSET,OUTPROG,OUTMIX,OUTARR,OUTMAP) \
  tOr7( tFuncV(IN tFuncV(SUB,tMix,tSetvar(2,tAny)),tMix,OUTFUN), \
        tIfnot(tFuncV(IN tFunction,tMix,tMix), \
	       tOr(tFuncV(IN tPrg(tObj), tMix, OUTPROG), \
		   tFuncV(IN tObj, tMix, OUTMIX))), \
	tFuncV(IN tSet(tMix),tMix,OUTSET), \
	tFuncV(IN tMap(tMix, tSetvar(2,tMix)), tMix, OUTMAP), \
        tFuncV(IN tArray, tMix, OUTARR), \
        tFuncV(IN tInt0, tMix, OUTMIX), \
	tFuncV(IN, tVoid, OUTMIX) )

  ADD_EFUN2("map", f_map,
	    tOr7(tMapStuff(tArr(tSetvar(1,tMix)),tVar(1),
			   tArr(tVar(2)),
			   tArr(tInt01),
			   tArr(tObj),
			   tArr(tMix),
			   tArr(tArr(tMix)),
			   tArr(tOr(tInt0,tVar(2)))),

		 tMapStuff(tMap(tSetvar(3,tMix),tSetvar(1,tMix)),tVar(1),
			   tMap(tVar(3),tVar(2)),
			   tMap(tVar(3),tInt01),
			   tMap(tVar(3),tObj),
			   tMap(tVar(3),tMix),
			   tMap(tVar(3),tArr(tMix)),
			   tMap(tVar(3),tOr(tInt0,tVar(2)))),
		
 		 tMapStuff(tSet(tSetvar(1,tMix)),tVar(1),
			   tSet(tVar(2)),
			   tSet(tInt01),
			   tSet(tObj),
			   tSet(tMix),
			   tSet(tArr(tMix)),
			   tSet(tOr(tInt0,tVar(2)))),

		 tMapStuff(tOr(tPrg(tObj),tFunction),tMix,
			   tMap(tStr,tVar(2)),
			   tMap(tStr,tInt01),
			   tMap(tStr,tObj),
			   tMap(tStr,tMix),
			   tMap(tStr,tArr(tMix)),
			   tMap(tStr,tOr(tInt0,tVar(2)))),

		 tOr4( tFuncV(tString tFuncV(tInt,tMix,tInt),tMix,tString), 
		       tFuncV(tString tFuncV(tInt,tMix,tInt),tMix,tString),
		       tFuncV(tString tSet(tMix),tMix,tString),
		       tFuncV(tString tMap(tMix,tInt), tMix, tString) ),

		 tOr4 (tFuncV(tArr(tStringIndicable) tString,tMix,tArray),
		       tFuncV(tMap(tSetvar(3,tMix),tStringIndicable) tString,tMix,
			      tMap(tVar(3),tMix)),
		       tFuncV(tSet(tStringIndicable) tString,tMix,tSet(tMix)),
		       tFuncV(tOr(tPrg(tObj),tFunction) tString,tMix,tMapping)),

		 tFuncV(tObj,tMix,tMix) ),
	    OPT_TRY_OPTIMIZE, fix_map_node_info, 0);
  
  ADD_EFUN2("filter", f_filter,
	    tOr3(tFuncV(tSetvar(1,tOr4(tArray,tMapping,tMultiset,tString)),
			tMixed,tVar(1)),
		 tFuncV(tOr(tPrg(tObj),tFunction),tMixed,tMap(tString,tMix)),
		 tFuncV(tObj,tMix,tMix) ) ,
	    OPT_TRY_OPTIMIZE, fix_map_node_info, 0);

  ADD_EFUN("enumerate",f_enumerate,
	   tOr8(tFunc(tIntPos,tArr(tInt)),
		tFunc(tIntPos tInt,tArr(tInt)),
		tFunc(tIntPos tInt tOr(tVoid,tInt),tArr(tInt)),
		tFunc(tIntPos tFloat tOr3(tVoid,tInt,tFloat),tArr(tFloat)),
		tFunc(tIntPos tOr(tInt,tFloat) tFloat,tArr(tFloat)),
		tFunc(tIntPos tMix tObj,tArr(tVar(1))),
		tFunc(tIntPos tObj tOr(tVoid,tMix),tArr(tVar(1))),
		tFunc(tIntPos tMix tMix 
		      tFuncV(tNone,tMix,tSetvar(1,tMix)),tArr(tVar(1)))),
	   OPT_TRY_OPTIMIZE);
		
  ADD_FUNCTION2("inherit_list", f_inherit_list,
		tFunc(tOr(tObj,tPrg(tObj)),tArr(tPrg(tObj))), 0, OPT_TRY_OPTIMIZE);
  ADD_FUNCTION2("function_defined", f_function_defined,
	       tFunc(tFunction,tString), 0, OPT_TRY_OPTIMIZE);

#ifdef DEBUG_MALLOC
  
/* function(void:void) */
  ADD_EFUN("_reset_dmalloc",f__reset_dmalloc,
	   tFunc(tVoid,tVoid),OPT_SIDE_EFFECT);
  ADD_EFUN("_dmalloc_set_name",f__dmalloc_set_name,
	   tOr(tFunc(tStr tInt,tVoid), tFunc(tVoid,tVoid)),OPT_SIDE_EFFECT);
  ADD_EFUN("_list_open_fds",f__list_open_fds,
	   tFunc(tVoid,tVoid),OPT_SIDE_EFFECT);
#endif
#ifdef PIKE_DEBUG
  
/* function(1=mixed:1) */
  ADD_EFUN("_locate_references",f__locate_references,
	   tFunc(tSetvar(1,tMix),tVar(1)),OPT_SIDE_EFFECT);
  ADD_EFUN("_describe",f__describe,
	   tFunc(tSetvar(1,tMix),tVar(1)),OPT_SIDE_EFFECT);
  ADD_EFUN("_gc_set_watch", f__gc_set_watch,
	   tFunc(tComplex,tVoid), OPT_SIDE_EFFECT);
  ADD_EFUN("_dump_backlog", f__dump_backlog,
	   tFunc(tNone,tVoid), OPT_SIDE_EFFECT);
  ADD_EFUN("_gdb_breakpoint", pike_gdb_breakpoint,
	   tFuncV(tNone,tMix,tVoid), OPT_SIDE_EFFECT);
#endif

  ADD_EFUN("_gc_status",f__gc_status,
	   tFunc(tNone,tMap(tString,tOr(tInt,tFloat))),
	   OPT_EXTERNAL_DEPEND);

  ADD_INT_CONSTANT ("NATIVE_INT_MAX", MAX_INT_TYPE, 0);
  ADD_INT_CONSTANT ("NATIVE_INT_MIN", MIN_INT_TYPE, 0);

  /* Maybe make PIKEFLOAT_MANT_DIG, PIKEFLOAT_MIN_EXP and
   * PIKEFLOAT_MAX_EXP available, but do we have to export FLT_RADIX
   * too? It'd be nice to always assume it's 2 to save the pike
   * programmer from that headache. */
  ADD_INT_CONSTANT ("FLOAT_DIGITS_10", PIKEFLOAT_DIG, 0);
  ADD_INT_CONSTANT ("FLOAT_MIN_10_EXP", PIKEFLOAT_MIN_10_EXP, 0);
  ADD_INT_CONSTANT ("FLOAT_MAX_10_EXP", PIKEFLOAT_MAX_10_EXP, 0);
  ADD_FLOAT_CONSTANT ("FLOAT_MAX", PIKEFLOAT_MAX, 0);
  ADD_FLOAT_CONSTANT ("FLOAT_MIN", PIKEFLOAT_MIN, 0);
  ADD_FLOAT_CONSTANT ("FLOAT_EPSILON", PIKEFLOAT_EPSILON, 0);

#ifdef WITH_DOUBLE_PRECISION_SVALUE
  ADD_INT_CONSTANT("__DOUBLE_PRECISION_FLOAT__",1,0);
#else 
#ifdef WITH_LONG_DOUBLE_PRECISION_SVALUE
  ADD_INT_CONSTANT("__LONG_DOUBLE_PRECISION_FLOAT__",1,0);
#else
  ADD_INT_CONSTANT("__FLOAT_PRECISION_FLOAT__",1,0);
#endif
#endif

  ADD_INT_CONSTANT ("DESTRUCT_EXPLICIT", DESTRUCT_EXPLICIT, 0);
  ADD_INT_CONSTANT ("DESTRUCT_NO_REFS", DESTRUCT_NO_REFS, 0);
  ADD_INT_CONSTANT ("DESTRUCT_GC", DESTRUCT_GC, 0);
  ADD_INT_CONSTANT ("DESTRUCT_CLEANUP", DESTRUCT_CLEANUP, 0);
}
