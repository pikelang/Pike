/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: module_support.c,v 1.54 2004/02/28 20:21:32 mast Exp $
*/

#include "global.h"
#include "module_support.h"
#include "interpret.h"
#include "svalue.h"
#include "stralloc.h"
#include "pike_types.h"
#include "pike_error.h"
#include "mapping.h"
#include "object.h"
#include "opcodes.h"

#define sp Pike_sp

RCSID("$Id: module_support.c,v 1.54 2004/02/28 20:21:32 mast Exp $");

/* Checks that args_to_check arguments are OK.
 * Returns 1 if everything worked ok, zero otherwise.
 * If something went wrong, 'exepect_result' tells you what went wrong.
 * Make sure to finish the argument list with a zero.
 */
static int va_check_args(struct svalue *s,
			 int args_to_check,
			 struct expect_result *res,
			 va_list arglist)
{
  res->error_type = ERR_NONE;
  res->expected = 0;
  
  for (res->argno=0; res->argno < args_to_check; res->argno++)
  {
    if(!(res->expected & BIT_MANY))
    {
      res->expected = va_arg(arglist, unsigned int);
      if(!res->expected)
      {
	res->error_type = ERR_TOO_MANY;
	return 0;
      }
    }

    if (!((1UL << s[res->argno].type) & res->expected))
    {
      res->got = DO_NOT_WARN((unsigned char)s[res->argno].type);
      res->error_type = ERR_BAD_ARG;
      return 0;
    }
  }

  if(!(res->expected & BIT_MANY))
    res->expected = va_arg(arglist, unsigned int);

  if(!res->expected ||
     (res->expected & BIT_VOID)) return 1;
  res->error_type = ERR_TOO_FEW;
  return 0;
}

/* Returns the number of the first bad argument,
 * -X if there were too few arguments
 * or 0 if all arguments were OK.
 */
PMOD_EXPORT int check_args(int args, ...)
{
  va_list arglist;
  struct expect_result tmp;
  
  va_start(arglist, args);
  va_check_args(sp - args, args, &tmp, arglist);
  va_end(arglist);

  if(tmp.error_type == ERR_NONE) return 0;
  return tmp.argno+1;
}

/* This function generates errors if any of the minargs first arguments
 * is not OK.
 */
PMOD_EXPORT void check_all_args(const char *fnname, int args, ... )
{
  va_list arglist;
  struct expect_result tmp;

  va_start(arglist, args);
  va_check_args(sp - args, args, &tmp, arglist);
  va_end(arglist);

  switch(tmp.error_type)
  {
  case ERR_NONE: return;
  case ERR_TOO_FEW:
    new_error(fnname, "Too few arguments.\n", sp, args, NULL, 0);
  case ERR_TOO_MANY:
    new_error(fnname, "Too many arguments.\n", sp, args, NULL, 0);

  case ERR_BAD_ARG:
  {
    char buf[1000];
    int e;
    buf[0]=0;
    for(e=0;e<16;e++)
    {
      if(tmp.expected & (1<<e))
      {
	if(buf[0])
	{
	  if(tmp.expected & 0xffff & (0xffff << e))
	    strcat(buf,", ");
	  else
	    strcat(buf," or ");
	}
	strcat(buf, get_name_of_type(e));
      }
    }
	
    Pike_error("Bad argument %d to %s(), (expecting %s, got %s)\n", 
	  tmp.argno+1,
	  fnname,
	  buf,
	  get_name_of_type(tmp.got));
  }
  }
}

/* This function does NOT generate errors, it simply returns how
 * many arguments were actually matched.
 * usage: get_args(sp-args, args, "%i",&an_int)
 * format specifiers:
 *   %i: INT_TYPE
 *   %l: LONGEST
 *   %s: char *				Only 8bit strings
 *   %S: struct pike_string *		Only 8bit strings
 *   %W: struct pike_string *		Allow wide strings
 *   %a: struct array *
 *   %A: struct array * or NULL
 *   %f: float -> FLOAT_TYPE
 *   %F: float or int -> FLOAT_TYPE
 *   %m: struct mapping *
 *   %M: struct multiset *
 *   %o: struct object *
 *   %O: struct object * or NULL
 *   %p: struct program *
 *   %*: struct svalue *
 */

int va_get_args(struct svalue *s,
		INT32 num_args,
		const char *fmt,
		va_list ap)
{
  int ret=0;

  while(*fmt)
  {
    if(*fmt != '%')
      Pike_fatal("Error in format for get_args.\n");

    if(ret == num_args) return ret;

    switch(*++fmt)
    {
    case 'd':
      if(s->type != T_INT) return ret;
      *va_arg(ap, int *)=s->u.integer;
      break;
    case 'i':
      if(s->type != T_INT) return ret;
      *va_arg(ap, INT_TYPE *)=s->u.integer;
      break;
    case '+':
      if(s->type != T_INT) return ret;
      if(s->u.integer<0) return ret;
      *va_arg(ap, INT_TYPE *)=s->u.integer;
      break;
    case 'D':
      if(s->type == T_INT)
	 *va_arg(ap, int *)=s->u.integer;
      else if(s->type == T_FLOAT)
        *va_arg(ap, int *)=
	  DO_NOT_WARN((int)s->u.float_number);
      else 
      {
        ref_push_type_value(int_type_string);
        push_svalue( s );
        f_cast( );
	if(sp[-1].type == T_INT)
	  *va_arg(ap, int *)=sp[-1].u.integer;
	else if(s->type == T_FLOAT)
	  *va_arg(ap, int *)=
	    DO_NOT_WARN((int)sp[-1].u.float_number);
	else
	  Pike_error("Cast to int failed.\n");
        pop_stack();
      }
      break;
    case 'I':
      if(s->type == T_INT)
	 *va_arg(ap, INT_TYPE *)=s->u.integer;
      else if(s->type == T_FLOAT)
        *va_arg(ap, INT_TYPE *) = DO_NOT_WARN((INT_TYPE)s->u.float_number);
      else 
      {
        ref_push_type_value(int_type_string);
        push_svalue( s );
        f_cast( );
	if(sp[-1].type == T_INT)
	  *va_arg(ap, INT_TYPE *)=sp[-1].u.integer;
	else if(s->type == T_FLOAT)
	  *va_arg(ap, INT_TYPE *)=
	    DO_NOT_WARN((INT_TYPE)sp[-1].u.float_number);
	else
	  Pike_error("Cast to int failed.\n");
        pop_stack();
      }
      break;
    case 'l':
      if (s->type == T_INT) {
	*va_arg(ap, LONGEST *)=s->u.integer;
	break;
      } else if (!is_bignum_object_in_svalue(s) ||
		 !int64_from_bignum(va_arg(ap, LONGEST *), s->u.object)) {
	return ret;
      }
      break;
    case 's':
      if(s->type != T_STRING) return ret;
      if(s->u.string->size_shift) return ret;
      *va_arg(ap, char **)=s->u.string->str;
      break;
    case 'S':
      if(s->type != T_STRING) return ret;
      if(s->u.string->size_shift) return ret;
      *va_arg(ap, struct pike_string **)=s->u.string;
      break;
    case 'W':
      if(s->type != T_STRING) return ret;
      *va_arg(ap, struct pike_string **)=s->u.string;
      break;
    case 'a':
      if(s->type != T_ARRAY) return ret;
      *va_arg(ap, struct array **)=s->u.array;
      break;
    case 'A':
      if(s->type == T_ARRAY)
	*va_arg(ap, struct array **)=s->u.array;
      else if (UNSAFE_IS_ZERO(s))
	*va_arg(ap, struct array **)=NULL;
      else
	return ret;
      break;
    case 'f':
      if(s->type != T_FLOAT) return ret;
      *va_arg(ap, FLOAT_TYPE *)=s->u.float_number;
      break;
    case 'F':
      if(s->type == T_FLOAT)
	 *va_arg(ap, FLOAT_TYPE *)=s->u.float_number;
      else if(s->type == T_INT)
	 *va_arg(ap, FLOAT_TYPE *)=(float)s->u.integer;
      else 
      {
        ref_push_type_value(float_type_string);
        push_svalue( s );
        f_cast( );
        *va_arg(ap, FLOAT_TYPE *)=sp[-1].u.float_number;
        pop_stack();
      }
      break;

    case 'm':
      if(s->type != T_MAPPING) return ret;
      *va_arg(ap, struct mapping **)=s->u.mapping;
      break;
    case 'M':
      if(s->type != T_MULTISET) return ret;
      *va_arg(ap, struct multiset **)=s->u.multiset;
      break;
    case 'o':
      if(s->type != T_OBJECT) return ret;
      *va_arg(ap, struct object **)=s->u.object;
      break;
    case 'O':
      if(s->type == T_OBJECT) 
        *va_arg(ap, struct object **)=s->u.object;
      else if(UNSAFE_IS_ZERO(s))
        *va_arg(ap, struct object **)=NULL;
      else
        return ret;
      break;
    case 'p':
      switch(s->type)
      {
	case T_PROGRAM:
	  *va_arg(ap, struct program **)=s->u.program;
	  break;

	case T_FUNCTION:
	  if((*va_arg(ap, struct program **)=program_from_svalue(s)))
	    break;

	default:
	  return ret;
      }
      break;

    case '*':
      *va_arg(ap, struct svalue **)=s;
      break;
      
    default:
      Pike_fatal("Unknown format character in get_args.\n");
    }
    ret++;
    s++;
    fmt++;
  }
  return ret;
}

PMOD_EXPORT int get_args(struct svalue *s,
			 INT32 num_args,
			 const char *fmt, ...)
{
  va_list ptr;
  int ret;
  va_start(ptr, fmt);
  ret=va_get_args(s, num_args, fmt, ptr);
  va_end(ptr);
  return ret;
}

PMOD_EXPORT void get_all_args(const char *fname, INT32 args,
			      const char *format,  ... )
{
  va_list ptr;
  int ret;
  va_start(ptr, format);
  ret=va_get_args(sp-args, args, format, ptr);
  va_end(ptr);
  if((ptrdiff_t)ret*2 != (ptrdiff_t)strlen(format)) {
    char *expected_type;
    switch(format[ret*2+1]) {
    case 'd': case 'i': case 'l': expected_type = "int"; break;
    case 'D': case 'I': expected_type = "int|float"; break;
    case 's': case 'S': expected_type = "string (8bit)"; break;
    case 'W': expected_type = "string"; break;
    case 'a': expected_type = "array"; break;
    case 'f': expected_type = "float"; break;
    case 'F': expected_type = "float|int"; break;
    case 'm': expected_type = "mapping"; break;
    case 'M': expected_type = "multiset"; break;
    case 'o': expected_type = "object"; break;
    case 'O': expected_type = "object or zero"; break;
    case 'p': expected_type = "program"; break;
    case '+': expected_type = "int(0..)"; break;
    case '*': expected_type = "mixed"; break;
    default: expected_type = "Unknown"; break;
    }
    if (ret < args) {
      bad_arg_error(
	fname, sp-args, args,
	ret+1,
	expected_type,
	sp+ret-args,
	"Bad argument %d to %s(). Expected %s\n",
	ret+1, fname, expected_type);
    } else if ((ptrdiff_t)(args*2) < (ptrdiff_t)strlen(format)) {
      bad_arg_error(
	fname, sp-args, args,
	ret+1,
	expected_type,
	0,
	"Too few arguments to %s(). Expected %ld arguments, got %d.\n"
	"The type of the next argument is expected to be %s\n",
	fname, PTRDIFF_T_TO_LONG(strlen(format)/2), args, expected_type);
    }
  }
}

/* NOTA BENE:
 * The code below assumes that dynamic modules are not
 * unloaded from memory...
 */
   
static struct mapping *exported_symbols;

PMOD_EXPORT void pike_module_export_symbol(const char *name,
					   int len,
					   void *ptr)
{
  struct pike_string *str=make_shared_binary_string(name,len);
  struct svalue s;
  if(!exported_symbols) exported_symbols=allocate_mapping(10);
  s.u.ptr=ptr;
  s.type=T_INT;
  s.subtype=4711;
  mapping_string_insert(exported_symbols, str, &s);
  free_string(str);
}

PMOD_EXPORT void *pike_module_import_symbol(const char *name,
					    int len,
					    const char *module,
					    int module_len)
{
  struct svalue *s;
  struct pike_string *str=make_shared_binary_string(name,len);
  if(exported_symbols)
  {
    s=low_mapping_string_lookup(exported_symbols, str);
    if(s)
    {
#ifdef PIKE_DEBUG
      if (s->type != T_INT || s->subtype != 4711)
	Pike_fatal("Unexpected value in exported_symbols.\n");
#endif
      free_string(str);
      return s->u.ptr;
    }
  }

  /* Load the module */
  push_string(make_shared_binary_string(module,module_len));
  SAFE_APPLY_MASTER("resolv",1);
  pop_stack();

  if(exported_symbols)
  {
    s=low_mapping_string_lookup(exported_symbols, str);

    if(s)
    {
#ifdef PIKE_DEBUG
      if (s->type != T_INT || s->subtype != 4711)
	Pike_fatal("Unexpected value in exported_symbols.\n");
#endif
      free_string(str);
      return s->u.ptr;
    }
  }

  free_string(str);
  return 0;
}

#ifdef DO_PIKE_CLEANUP
void cleanup_module_support (void)
{
  if (exported_symbols) {
    free_mapping (exported_symbols);
    exported_symbols = NULL;
  }
}
#endif
