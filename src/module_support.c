#include "global.h"
#include "module_support.h"
#include "interpret.h"
#include "svalue.h"
#include "stralloc.h"

/* Checks that minargs arguments are OK.
 *
 * Returns the number of the first bad argument,
 * or 0 if all arguments were OK.
 */
int va_check_args(struct svalue *s, int minargs, va_list arglist)
{
  int argno;

  for (argno=0; argno < minargs; argno++)
  {
    int type_mask = va_arg(arglist, unsigned INT32);

    if (!((1UL << s[argno].type) & type_mask))
    {
      return(argno+1);
    }
  }

  return(0);
}

/* Returns the number of the first bad argument,
 * or 0 if all arguments were OK.
 */
int check_args(int args, int minargs, ...)
{
  va_list arglist;
  int argno;
  
  if (args < minargs) {
    return(args+1);
  }

  va_start(arglist, minargs);
  argno = va_check_args(sp - args, minargs, arglist);
  va_end(arglist);

  return(argno);
}

/* This function generates errors if any of the minargs first arguments
 * is not OK.
 */
void check_all_args(const char *fnname, int args, int minargs, ... )
{
  va_list arglist;
  int argno;

  if (args < minargs) {
    error("Too few arguments to %s()\n", fnname);
  }

  va_start(arglist, minargs);
  argno = va_check_args(sp - args, minargs, arglist);
  va_end(arglist);

  if (argno) {
    error("Bad argument %d to %s()\n", argno, fnname);
  }
}

/* This function does NOT generate errors, it simply returns how
 * many arguments were actually matched.
 * usage: get_args(sp-args, args, "%i",&an_int)
 * format specifiers:
 *   %i: INT32
 *   %s: char *
 *   %S: struct pike_string *
 *   %a: struct array *
 *   %f: float
 *   %m: struct mapping *
 *   %M: struct multiset *
 *   %o: struct object *
 *   %p: struct program *
 *   %*: struct svalue *
 */

int va_get_args(struct svalue *s,
		INT32 num_args,
		char *fmt,
		va_list ap)
{
  int ret=0;
  while(*fmt)
  {
    if(*fmt != '%')
      fatal("Error in format for get_args.\n");

    if(ret == num_args) return ret;

    switch(*++fmt)
    {
    case 'd':
      if(s->type != T_INT) return ret;
      *va_arg(ap, int *)=s->u.integer;
      break;
    case 'i':
      if(s->type != T_INT) return ret;
      *va_arg(ap, INT32 *)=s->u.integer; break;
    case 's':
      if(s->type != T_STRING) return ret;
      *va_arg(ap, char **)=s->u.string->str;
      break;
    case 'S':
      if(s->type != T_STRING) return ret;
      *va_arg(ap, struct pike_string **)=s->u.string;
      break;
    case 'a':
      if(s->type != T_ARRAY) return ret;
      *va_arg(ap, struct array **)=s->u.array;
      break;
    case 'f':
      if(s->type != T_FLOAT) return ret;
      *va_arg(ap, float *)=s->u.float_number;
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
    case 'p':
      if(s->type != T_PROGRAM) return ret;
      *va_arg(ap, struct program **)=s->u.program;
      break;
    case '*':
      *va_arg(ap, struct svalue **)=s;
      break;
      
    default:
      fatal("Unknown format character in get_args.\n");
    }
    ret++;
    s++;
    fmt++;
  }
  return ret;
}

int get_args(struct svalue *s,
	     INT32 num_args,
	     char *fmt, ...)
{
  va_list ptr;
  int ret;
  va_start(ptr, fmt);
  ret=va_get_args(s, num_args, fmt, ptr);
  va_end(fmt);
}

void get_all_args(char *fname, INT32 args, char *format,  ... )
{
  va_list ptr;
  int ret;
  va_start(ptr, format);
  ret=va_get_args(sp-args, args, format, ptr);
  va_end(ptr);
  if((long)ret*2 != (long)strlen(format)) {
    char *expected_type;
    switch(format[ret*2+1]) {
    case 'd': case 'i': expected_type = "int"; break;
    case 's': case 'S': expected_type = "string"; break;
    case 'a': expected_type = "array"; break;
    case 'f': expected_type = "float"; break;
    case 'm': expected_type = "mapping"; break;
    case 'M': expected_type = "multiset"; break;
    case 'o': expected_type = "object"; break;
    case 'p': expected_type = "program"; break;
    case '*': expected_type = "mixed"; break;
    default: expected_type = "Unknown"; break;
    }
    error("Bad argument %d to %s(). Expected %s\n", ret+1, fname,
	  expected_type);
  }
}
