/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
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
#include "operators.h"
#include "bignum.h"

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

    if (!((1UL << TYPEOF(s[res->argno])) & res->expected) &&
	!(res->expected & BIT_ZERO &&
	  TYPEOF(s[res->argno]) == T_INT && s[res->argno].u.integer == 0))
    {
      res->got = (unsigned char)TYPEOF(s[res->argno]);
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
  va_check_args(Pike_sp - args, args, &tmp, arglist);
  va_end(arglist);

  if(tmp.error_type == ERR_NONE) return 0;
  return tmp.argno+1;
}

static const char* get_fname(const char *fname)
{
  struct identifier *id;
  if (fname) return fname;
  id = ID_FROM_INT(Pike_fp->current_program, Pike_fp->fun);
  return id->name->str;
}

/* This function generates errors if any of the args first arguments
 * is not OK.
 */
PMOD_EXPORT void check_all_args(const char *fnname, int args, ... )
{
  va_list arglist;
  struct expect_result tmp;

  va_start(arglist, args);
  va_check_args(Pike_sp - args, args, &tmp, arglist);
  va_end(arglist);

  switch(tmp.error_type)
  {
  case ERR_NONE: return;
  case ERR_TOO_FEW:
    new_error(get_fname(fnname), "Too few arguments.\n", Pike_sp, args);
    break;
  case ERR_TOO_MANY:
    new_error(get_fname(fnname), "Too many arguments.\n", Pike_sp, args);
    break;

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
          get_fname(fnname),
	  buf,
	  get_name_of_type(tmp.got));
    break;
  }
  }
}

/* get_args and get_all_args type specifiers:
 *
 *   %i: INT_TYPE
 *   %I: int or float -> INT_TYPE
 *   %d: int (the c type "int" which may vary from INT_TYPE)
 *   %D: int of float -> int
 *   %+: positive int -> INT_TYPE
 *   %l: int or bignum -> INT64
 *   %c: char *				Only narrow (8 bit) strings without NUL.
 *   %C: char *	or NULL			Only narrow (8 bit) strings without NUL.
 *   %n: struct pike_string *           Only narrow (8 bit) strings.
 *   %N: struct pike_string * or NULL   Only narrow (8 bit) strings.
 *   %t: struct pike_string *           Any string width. (*)
 *   %T: struct pike_string * or NULL   Any string width. (*)
 *   %a: struct array *
 *   %A: struct array * or NULL
 *   %f: float -> FLOAT_TYPE
 *   %F: float or int -> FLOAT_TYPE
 *   %m: struct mapping *
 *   %G: struct mapping * or NULL       (*)
 *   %u: struct multiset *
 *   %U: struct multiset * or NULL
 *   %o: struct object *
 *   %O: struct object * or NULL
 *   %p: struct program *
 *   %P: struct program * or NULL
 *   %*: struct svalue *
 *
 * For compatibility:
 *
 *   %s: char *				Only 8 bit strings without NUL.
 *   %S: struct pike_string *		Only 8 bit strings
 *   %W: struct pike_string *		Allow wide strings
 *   %M: struct multiset *
 *
 * A period can be specified between type specifiers to mark the start
 * of optional arguments. If the real arguments run out in the list of
 * optional arguments, or if a real argument is UNDEFINED in an
 * optional argument position, the corresponding pointer won't be
 * assigned at all.
 *
 * WARNING: If you use a period to parse optional arguments, then you
 * should _always_ initialize the corresponding variables before
 * calling get_args/get_all_args. Just looking at num_args afterwards
 * is not safe since the user might have passed UNDEFINED, in which
 * case the variable won't be assigned anyway.
 *
 * Note: If there are more arguments than there are type specifiers
 * the excessive arguments will be silently ignored. This may change
 * in the future, in which case an extra marker must be added to get
 * this behaviour.
 *
 * Note: A lowercase type specifier (i.e. one that doesn't accept
 * NULL) in the optional args list leads to behavior that breaks
 * common coding conventions. Try to avoid it.
 *
 * *)  Contrived letter since the logical one is taken for historical
 *     reasons. :\
 */

/* Values for the info flag: */
#define ARGS_OK		0  /* At end of args and fmt. */
#define ARGS_OPT	-1 /* At end of args but not fmt and has passed a period. */
#define ARGS_SHORT	-2 /* At end of args but not fmt and has not passed a period. */
#define ARGS_LONG	-3 /* At end of fmt but not args. */
#define ARGS_NUL_IN_STRING -4	/* 8 bit string got embedded NUL. */
#define ARGS_SUBTYPED_OBJECT -5	/* An object with a subtype. */
/* Positive values: Stopped at arg with invalid type. The value is the
 * format letter for the expected type. */

static int va_get_args_2(struct svalue *s,
			 INT32 num_args,
			 const char *fmt,
			 va_list ap,
			 int *info)
{
  int ret=0;
  int optional = 0;

  while(*fmt)
  {
    void *ptr;

    if (*fmt == '.' && !optional) {
      fmt++;
      optional = 1;
      continue;
    }

    if(*fmt != '%')
      Pike_fatal("Error in format for get_args or get_all_args.\n");

    if(ret == num_args) {
      if (optional)
	*info = ARGS_OPT;
      else
	*info = ARGS_SHORT;
      return ret;
    }

    ptr = va_arg(ap, void *);

#define cast_arg(PTR, TYPE)	((TYPE)(PTR))

#ifdef PIKE_DEBUG
    /* Try to check that the caller has given the variable for every
     * optional argument a value, because otherwise the caller is
     * almost certainly doing something stupid. See the warning in the
     * blurb above. */
    if (optional && PIKE_MEM_NOT_DEF (*cast_arg (ptr, char *)))
      Pike_fatal ("Detected undefined default value for optional argument.\n");
#endif

    if (optional && IS_UNDEFINED (s)) {
      /* An optional argument with an undefined value should be
       * treated as if it isn't given at all, i.e. don't assign this
       * argument. */
      fmt++;
    }
    else switch(*++fmt)
    {
    case 'd':
      if(TYPEOF(*s) != T_INT) goto type_err;
      /* FIXME: Range checks, including bignum objects. */
      *cast_arg(ptr, int *)=s->u.integer;
      break;
    case 'i':
      if(TYPEOF(*s) != T_INT) goto type_err;
      /* FIXME: Error reporting for bignum objects. */
      *cast_arg(ptr, INT_TYPE *)=s->u.integer;
      break;
    case '+':
      if(TYPEOF(*s) != T_INT) goto type_err;
      if(s->u.integer<0) goto type_err;
      /* FIXME: Error reporting for bignum objects. */
      *cast_arg(ptr, INT_TYPE *)=s->u.integer;
      break;

    case 'D':
      if(TYPEOF(*s) == T_INT)
	/* FIXME: Range checks. */
	 *cast_arg(ptr, int *)=s->u.integer;
      else if(TYPEOF(*s) == T_FLOAT)
	/* FIXME: Range checks. */
        *cast_arg(ptr, int *)=(int)s->u.float_number;
      else
      {
        ref_push_type_value(int_type_string);
        push_svalue( s );
        f_cast( );
	if(TYPEOF(Pike_sp[-1]) == T_INT)
	  /* FIXME: Range checks. */
	  *cast_arg(ptr, int *)=Pike_sp[-1].u.integer;
	else if(TYPEOF(*s) == T_FLOAT)
	  /* FIXME: Range checks. Btw, does this case occur? */
          *cast_arg(ptr, int *)=(int)Pike_sp[-1].u.float_number;
	else
	  Pike_error("Cast to int failed.\n");
        pop_stack();
      }
      break;

    case 'I':
      if(TYPEOF(*s) == T_INT)
	 *cast_arg(ptr, INT_TYPE *)=s->u.integer;
      else if(TYPEOF(*s) == T_FLOAT)
	/* FIXME: Range checks. */
        *cast_arg(ptr, INT_TYPE *) = (INT_TYPE)s->u.float_number;
      else
      {
	/* FIXME: Error reporting for bignum objects. */
        ref_push_type_value(int_type_string);
        push_svalue( s );
        f_cast( );
	if(TYPEOF(Pike_sp[-1]) == T_INT)
	  *cast_arg(ptr, INT_TYPE *)=Pike_sp[-1].u.integer;
	else if(TYPEOF(*s) == T_FLOAT)
	  /* FIXME: Range checks. Btw, does this case occur? */
          *cast_arg(ptr, INT_TYPE *)=(INT_TYPE)Pike_sp[-1].u.float_number;
	else
	  Pike_error("Cast to int failed.\n");
        pop_stack();
      }
      break;

    case 'l':
      if (TYPEOF(*s) == T_INT) {
        *cast_arg(ptr, INT64 *)=s->u.integer;
	break;
      } else if (is_bignum_object_in_svalue(s) &&
                 int64_from_bignum(cast_arg(ptr, INT64 *), s->u.object) == 1) {
        break;
      }
      /* FIXME: Error reporting for bignum objects. */
      goto type_err;

    case 'C':
      if(TYPEOF(*s) != T_STRING && UNSAFE_IS_ZERO (s)) {
	*cast_arg(ptr, char **) = NULL;
	break;
      }
      /* FALLTHRU */
    case 'c':
    case 's':
      if(TYPEOF(*s) != T_STRING) goto type_err;
      if(s->u.string->size_shift) goto type_err;

      if(string_has_null(s->u.string)) {
	*info = ARGS_NUL_IN_STRING;
	return ret;
      }

      *cast_arg(ptr, char **)=s->u.string->str;
      break;

    case 'N':
      if(TYPEOF(*s) != T_STRING && UNSAFE_IS_ZERO (s)) {
	*cast_arg(ptr, struct pike_string **) = NULL;
	break;
      }
      /* FALLTHRU */
    case 'n':
    case 'S':
      if(TYPEOF(*s) != T_STRING) goto type_err;
      if(s->u.string->size_shift) goto type_err;
      *cast_arg(ptr, struct pike_string **)=s->u.string;
      break;

    case 'T':
      if(TYPEOF(*s) != T_STRING && UNSAFE_IS_ZERO (s)) {
	*cast_arg(ptr, struct pike_string **) = NULL;
	break;
      }
      /* FALLTHRU */
    case 't':
    case 'W':
      if(TYPEOF(*s) != T_STRING) goto type_err;
      *cast_arg(ptr, struct pike_string **)=s->u.string;
      break;

    case 'A':
      if(TYPEOF(*s) != T_ARRAY && UNSAFE_IS_ZERO (s)) {
	*cast_arg(ptr, struct array **) = NULL;
	break;
      }
      /* FALLTHRU */
    case 'a':
      if(TYPEOF(*s) != T_ARRAY) goto type_err;
      *cast_arg(ptr, struct array **)=s->u.array;
      break;

    case 'f':
      if(TYPEOF(*s) != T_FLOAT) goto type_err;
      *cast_arg(ptr, FLOAT_TYPE *)=s->u.float_number;
      break;
    case 'F':
      if(TYPEOF(*s) == T_FLOAT)
	 *cast_arg(ptr, FLOAT_TYPE *)=s->u.float_number;
      else if(TYPEOF(*s) == T_INT)
	 *cast_arg(ptr, FLOAT_TYPE *)=(FLOAT_TYPE)s->u.integer;
      else
      {
        ref_push_type_value(float_type_string);
        push_svalue( s );
        f_cast( );
        *cast_arg(ptr, FLOAT_TYPE *)=Pike_sp[-1].u.float_number;
        pop_stack();
      }
      break;

    case 'G':
      if(TYPEOF(*s) != T_MAPPING && UNSAFE_IS_ZERO (s)) {
	*cast_arg(ptr, struct mapping **) = NULL;
	break;
      }
      /* FALLTHRU */
    case 'm':
      if(TYPEOF(*s) != T_MAPPING) goto type_err;
      *cast_arg(ptr, struct mapping **)=s->u.mapping;
      break;

    case 'U':
      if(TYPEOF(*s) != T_MULTISET && UNSAFE_IS_ZERO (s)) {
	*cast_arg(ptr, struct multiset **) = NULL;
	break;
      }
      /* FALLTHRU */
    case 'u':
    case 'M':
      if(TYPEOF(*s) != T_MULTISET) goto type_err;
      *cast_arg(ptr, struct multiset **)=s->u.multiset;
      break;

    case 'O':
      if(TYPEOF(*s) != T_OBJECT && UNSAFE_IS_ZERO (s)) {
	*cast_arg(ptr, struct object **) = NULL;
	break;
      }
      /* FALLTHRU */
    case 'o':
      if(TYPEOF(*s) != T_OBJECT) goto type_err;
      if (SUBTYPEOF(*s)) {
	*info = ARGS_SUBTYPED_OBJECT;
	return ret;
      }
      *cast_arg(ptr, struct object **)=s->u.object;
      break;

    case 'P':
    case 'p':
      switch(TYPEOF(*s))
      {
	case T_PROGRAM:
	  *cast_arg(ptr, struct program **)=s->u.program;
	  break;

	case T_FUNCTION:
	  if((*cast_arg(ptr, struct program **)=program_from_svalue(s)))
	    break;
	  /* FALLTHRU */

	default:
	  if (*fmt == 'P' && UNSAFE_IS_ZERO(s)) {
	    *cast_arg(ptr, struct program **) = NULL;
            break;
          }
	  goto type_err;
      }
      break;

    case '*':
      *cast_arg(ptr, struct svalue **)=s;
      break;

    default:
      Pike_fatal("Unknown format character %d.\n", *fmt);
    }
    ret++;
    s++;
    fmt++;
  }

  if (ret == num_args)
    *info = ARGS_OK;
  else
    *info = ARGS_LONG;
  return ret;

type_err:
  *info = *fmt;
  return ret;
}

/* Compat wrapper, just in case. */
int va_get_args(struct svalue *s,
		INT32 num_args,
		const char *fmt,
		va_list ap)
{
  int info;
  return va_get_args_2 (s, num_args, fmt, ap, &info);
}

#ifdef NOT_USED
/* get_args does NOT generate errors, it simply returns how
 * many arguments were actually matched.
 * usage: get_args(Pike_sp-args, args, "%i",&an_int)
 */
PMOD_EXPORT int get_args(struct svalue *s,
			 INT32 num_args,
			 const char *fmt, ...)
{
  va_list ptr;
  int ret, info;
  va_start(ptr, fmt);
  ret=va_get_args_2(s, num_args, fmt, ptr, &info);
  va_end(ptr);
  return ret;
}
#endif

PMOD_EXPORT void get_all_args(const char *fname, INT32 args,
			      const char *format,  ... )
{
  va_list ptr;
  int ret, info;
  va_start(ptr, format);
  ret=va_get_args_2(Pike_sp-args, args, format, ptr, &info);
  va_end(ptr);

  switch (info) {
    case ARGS_OK:
    case ARGS_OPT:
    case ARGS_LONG:
      break;
    case ARGS_NUL_IN_STRING:
      fname = get_fname(fname);
      bad_arg_error(fname, args, ret+1,	"string(1..255)",
	Pike_sp+ret-args,
	"Bad argument %d to %s(). NUL in string.\n",
        ret+1, fname);
      UNREACHABLE();
      break;

    case ARGS_SUBTYPED_OBJECT:
      bad_arg_error(get_fname(fname), args, ret+1, "object",
		    Pike_sp+ret-args,
		    "Subtyped objects are not supported.\n");
      UNREACHABLE();
      break;

    case ARGS_SHORT:
    default: {
      char *expected_type;

      /* NB: For ARGS_SHORT we know there's no period in format that
       * might offset the format specs. */
      switch(info == ARGS_SHORT ? format[ret*2+1] : info) {
	case 'd': case 'i': case 'l': expected_type = "int"; break;
	case 'D': case 'I': expected_type = "int|float"; break;
	case '+': expected_type = "int(0..)"; break;
        case 'c': case 's':
          expected_type = "string(1..255)"; break;
        case 'n': case 'S':
	  expected_type = "string(8bit)"; break;
	case 'C':
          expected_type = "string(1..255) or zero"; break;
        case 'N':
          expected_type = "string(8bit) or zero"; break;
	case 't': case 'W': expected_type = "string"; break;
	case 'T': expected_type = "string or zero"; break;
	case 'a': expected_type = "array"; break;
	case 'A': expected_type = "array or zero"; break;
	case 'f': expected_type = "float"; break;
	case 'F': expected_type = "float|int"; break;
	case 'm': expected_type = "mapping"; break;
	case 'G': expected_type = "mapping or zero"; break;
	case 'M': case 'u': expected_type = "multiset"; break;
	case 'U': expected_type = "multiset or zero"; break;
	case 'o': expected_type = "object"; break;
	case 'O': expected_type = "object or zero"; break;
	case 'p': expected_type = "program"; break;
	case 'P': expected_type = "program or zero"; break;
	case '*': expected_type = "mixed"; break;
	default:
#ifdef PIKE_DEBUG
	  Pike_fatal ("get_all_args not in sync with low_va_get_args.\n");
#else
	  expected_type = NULL;	/* To avoid warnings. */
#endif
      }

      fname = get_fname(fname);
      if (info != ARGS_SHORT) {
        bad_arg_error(fname, args, ret+1, expected_type,
	  Pike_sp+ret-args,
	  "Bad argument %d to %s(). Expected %s.\n",
	  ret+1, fname, expected_type);
      } else {
	const char *req_args_end = strchr (format, '.');
	if (!req_args_end) req_args_end = strchr (format, 0);
        bad_arg_error(fname, args, ret+1, expected_type,
	  0,
	  "Too few arguments to %s(). Expected %"PRINTPTRDIFFT"d arguments, got %d.\n"
	  "The type of the next argument is expected to be %s.\n",
	  fname, (req_args_end - format) / 2, args, expected_type);
      }
      UNREACHABLE();
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
  SET_SVAL(s, T_INT, NUMBER_NUMBER, ptr, ptr);
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
      if (TYPEOF(*s) != T_INT || SUBTYPEOF(*s) != NUMBER_NUMBER)
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
      if (TYPEOF(*s) != T_INT || SUBTYPEOF(*s) != NUMBER_NUMBER)
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
