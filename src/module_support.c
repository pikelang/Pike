#include "global.h"
#include "module_support.h"
#include "interpret.h"
#include "svalue.h"
#include "stralloc.h"
#include "pike_types.h"
#include "error.h"

RCSID("$Id: module_support.c,v 1.10 1998/04/10 23:29:59 grubba Exp $");

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
      res->got=s[res->argno].type;
      res->error_type = ERR_BAD_ARG;
      return 0;
    }
  }

  if(!(res->expected & BIT_MANY))
  {
    res->expected = va_arg(arglist, unsigned int);
  }

  if(!res->expected || (res->expected & BIT_VOID)) return 1;
  res->error_type = ERR_TOO_FEW;
  return 0;
}

/* Returns the number of the first bad argument,
 * -X if there were too few arguments
 * or 0 if all arguments were OK.
 */
int check_args(int args, ...)
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
void check_all_args(const char *fnname, int args, ... )
{
  va_list arglist;
  struct expect_result tmp;
  int argno;

  va_start(arglist, args);
  va_check_args(sp - args, args, &tmp, arglist);
  va_end(arglist);

  switch(tmp.error_type)
  {
  case ERR_NONE: return;
  case ERR_TOO_FEW:
    new_error(fnname, "Too few arguments to %s()\n", sp, args, NULL, 0);
  case ERR_TOO_MANY:
    new_error(fnname, "Too many arguments to %s()\n", sp, args, NULL, 0);

  case ERR_BAD_ARG:
  {
    char buf[1000];
    int e,d;
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
	
    error("Bad argument %d to %s(), (expecting %s, got %s)\n", 
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
  return ret;
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
