/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
RCSID("$Id: mpz_glue.c,v 1.8 1996/11/17 02:17:02 nisse Exp $");
#include "gmp_machine.h"
#include "types.h"

#if !defined(HAVE_LIBGMP)
#undef HAVE_GMP_H
#endif

#ifdef HAVE_GMP_H

#include "interpret.h"
#include "svalue.h"
#include "stralloc.h"
#include "array.h"
#include "macros.h"
#include "program.h"
#include "stralloc.h"
#include "object.h"
#include "pike_types.h"

#include <gmp.h>
#include <assert.h>

#define THIS ((MP_INT *)(fp->current_storage))
#define OBTOMPZ(o) ((MP_INT *)(o->storage))

static struct program *mpzmod_program;

static void get_mpz_from_digits(MP_INT *tmp,
				struct svalue *s, struct svalue *b)
{
  INT32 base;
  struct pike_string *digits;

  if ((s->type != T_STRING) || (b->type != T_INT))
    error("wrong types, cannot convert to mpz");

  digits = s->u.string;
  base = b->u.integer;
  
  if ((base >= 2) && (base <= 36))
    {
      if (mpz_set_str(tmp, digits->str, base))
	error("invalid digits, cannot convert to mpz");
    }
  else if (base == 256)
    {

      INT8 i;
      mpz_t digit;

      mpz_init(digit);
      mpz_set_ui(tmp, 0);
      for (i = 0; i < digits->len; i++)
	{
	  mpz_set_ui(digit, EXTRACT_UCHAR(digits->str + i));
	  mpz_mul_2exp(digit, digit, (digits->len - i - 1) * 8);
	  mpz_ior(tmp, tmp, digit);
	}
    }
  else
    error("invalid base.\n");
}

static void get_new_mpz(MP_INT *tmp, struct svalue *s)
{
  switch(s->type)
    {
    case T_INT:
      mpz_set_si(tmp, (signed long int) s->u.integer);
      break;
    
    case T_FLOAT:
      mpz_set_d(tmp, (double) s->u.float_number);
      break;

    case T_OBJECT:
      if(s->u.object->prog != mpzmod_program)
	error("Wrong type of object, cannot convert to mpz.\n");

      mpz_set(tmp, OBTOMPZ(s->u.object));
      break;
#if 0    
    case T_STRING:
      mpz_set_str(tmp, s->u.string->str, 0);
      break;

    case T_ARRAY:   /* Experimental */
      if ( (s->u.array->size != 2)
	   || (ITEM(s->u.array)[0].type != T_STRING)
	   || (ITEM(s->u.array)[1].type != T_INT))
	error("cannot convert array to mpz.\n");
      get_mpz_from_digits(tmp, ITEM(s->u.array)[0].u.string,
			  ITEM(s->u.array)[1]);
      break;
#endif
    default:
      error("cannot convert argument to mpz.\n");
    }
}

static void mpzmod_create(INT32 args)
{
  switch(args)
    {
    case 1:
      get_new_mpz(THIS, sp-args);
      break;
    case 2: /* Args are string of digits and integer base */
      if ((sp-args)->type != T_STRING)
	error("wrong type, invalid string of digits");
      get_mpz_from_digits(THIS, sp-args, sp-args+1);
      break;
    }
  pop_n_elems(args);
}

static void mpzmod_get_int(INT32 args)
{
  pop_n_elems(args);
  push_int(mpz_get_si(THIS));
}

static void mpzmod_get_float(INT32 args)
{
  pop_n_elems(args);
  push_float((float)mpz_get_d(THIS));
}

static void mpzmod_get_string(INT32 args)
{
  struct pike_string *s;
  INT32 len;

  pop_n_elems(args);
  len=mpz_sizeinbase(THIS,10);
  len++; /* For a zero */
  if(mpz_sgn(THIS) < 0) len++; /* For the - sign */

  s=begin_shared_string(len);
  mpz_get_str(s->str,10,THIS);

  len-=4;
  if(len < 0) len = 0;
  while(s->str[len]) len++;
  s->len=len;
  s=end_shared_string(s);
  push_string(s);
}

static void mpzmod_digits(INT32 args)
{
  int base;
  struct pike_string *s;
  INT32 len;

  base = sp[-args].u.integer;
  if ( (base >= 2) && (base <= 36))
    {
      len = mpz_sizeinbase(THIS, base) + 2;
      s = begin_shared_string(len);
      mpz_get_str(s->str, base, THIS);
      /* Find NULL character */
      len-=4;
      if (len < 0) len = 0;
      while(s->str[len]) len++;
      s->len=len;
      s=end_shared_string(s);
    }
  else if (base == 256)
    {
      INT8 i;
      mpz_t tmp;
      
      if (mpz_sgn(THIS) < 0)
	error("only non-negative numbers can be converted to base 256.\n");
      len = (mpz_sizeinbase(THIS, 2) + 7) / 8;
      s = begin_shared_string(len);
      mpz_init_set(tmp, THIS);
      for (i = len - 1; i>= 0; i-- )
	{
	  s->str[i] = mpz_get_ui(tmp) & 0xff;
	  mpz_tdiv_q_2exp(tmp, tmp, 8);
	}
      assert(mpz_sgn(tmp) == 0);
      mpz_clear(tmp);
      s = end_shared_string(s);
    }
  else
    error("invalid base.\n");
  
  pop_n_elems(args);
  push_string(s);
}
  
static void mpzmod_cast(INT32 args)
{
  if(args < 1)
    error("mpz->cast() called without arguments.\n");
  if(sp[-args].type != T_STRING)
    error("Bad argument 1 to mpz->cast().\n");

  switch(sp[-args].u.string->str[0])
  {
  case 'i':
    if(!strcmp(sp[-args].u.string->str, "int"))
    {
      mpzmod_get_int(args);
      return;
    }
    break;

  case 's':
    if(!strcmp(sp[-args].u.string->str, "string"))
    {
      mpzmod_get_string(args);
      return;
    }
    break;

  case 'f':
    if(!strcmp(sp[-args].u.string->str, "float"))
    {
      mpzmod_get_float(args);
      return;
    }
    break;

  case 'o':
    if(!strcmp(sp[-args].u.string->str, "object"))
    {
      pop_n_elems(args);
      push_object(this_object());
    }
    break;

  case 'm':
    if(!strcmp(sp[-args].u.string->str, "mixed"))
    {
      pop_n_elems(args);
      push_object(this_object());
    }
    break;
    
  }

  error("mpz->cast() to other type than string, int or float.\n");
}

static MP_INT *get_mpz(struct svalue *s)
{
  struct object *o;
  switch(s->type)
  {
  default:
    error("Wrong type of object, cannot convert to mpz.\n");
    return 0;

  case T_INT:
  case T_FLOAT:
#if 0
  case T_STRING:
  case T_ARRAY:
#endif
    o=clone(mpzmod_program,0);
    get_new_mpz(OBTOMPZ(o), s);
    free_svalue(s);
    s->u.object=o;
    s->type=T_OBJECT;
    return (MP_INT *)o->storage;
    
  case T_OBJECT:
    if(s->u.object->prog != mpzmod_program)
      error("Wrong type of object, cannot convert to mpz.\n");

    return (MP_INT *)s->u.object->storage;
  }
}

/* These two functions are here so we can allocate temporary
 * objects without having to worry about them leaking in
 * case of errors..
 */
static struct object *temporary;
MP_INT *get_tmp()
{
  if(!temporary)
    temporary=clone(mpzmod_program,0);

  return (MP_INT *)temporary->storage;
}

static void return_temporary(INT32 args)
{
  pop_n_elems(args);
  push_object(temporary);
  temporary=0;
}

#define BINFUN(name, fun)				\
static void name(INT32 args)				\
{							\
  INT32 e;						\
  MP_INT *tmp=get_tmp();				\
  mpz_set(tmp, THIS);					\
  for(e=0;e<args;e++)					\
    fun(tmp, tmp, get_mpz(sp+e-args));			\
  return_temporary(args);				\
}

BINFUN(mpzmod_add,mpz_add)
BINFUN(mpzmod_mul,mpz_mul)
BINFUN(mpzmod_gcd,mpz_gcd)

static void mpzmod_sub(INT32 args)
{
  INT32 e;
  MP_INT *tmp=get_tmp();
  mpz_set(tmp, THIS);

  if(args)
  {
    for(e=0;e<args;e++)
      mpz_sub(tmp, tmp, get_mpz(sp+e-args));
  }else{
    mpz_neg(tmp, tmp);
  }

  return_temporary(args);
}

static void mpzmod_div(INT32 args)
{
  INT32 e;
  MP_INT *tmp=get_tmp();
  mpz_set(tmp, THIS);

  for(e=0;e<args;e++)
  {
    MP_INT *tmp2;
    tmp2=get_mpz(sp+e-args);
    if(!mpz_sgn(tmp2))
      error("Division by zero.\n");
    mpz_tdiv_q(tmp, tmp, tmp2);
  }
  return_temporary(args);
}

static void mpzmod_mod(INT32 args)
{
  INT32 e;
  MP_INT *tmp=get_tmp();
  mpz_set(tmp, THIS);

  for(e=0;e<args;e++)
  {
    MP_INT *tmp2;
    tmp2=get_mpz(sp+e-args);
    if(!mpz_sgn(tmp2))
      error("Modulo by zero.\n");
    mpz_tdiv_r(tmp, tmp, tmp2);
  }
  return_temporary(args);
}

static void mpzmod_gcdext(INT32 args)
{
  struct object *g, *s, *t;
  MP_INT *a;

  a = get_mpz(sp-args);
  
  g = clone(mpzmod_program, 0);
  s = clone(mpzmod_program, 0);
  t = clone(mpzmod_program, 0);

  mpz_gcdext(OBTOMPZ(g), OBTOMPZ(s), OBTOMPZ(t), THIS, a);
  pop_n_elems(args);
  push_object(g); push_object(s); push_object(t);
  f_aggregate(3);
}

static void mpzmod_gcdext2(INT32 args)
{
  struct object *g, *s;
  MP_INT *a;

  a = get_mpz(sp-args);
  
  g = clone(mpzmod_program, 0);
  s = clone(mpzmod_program, 0);

  mpz_gcdext(OBTOMPZ(g), OBTOMPZ(s), NULL, THIS, a);
  pop_n_elems(args);
  push_object(g); push_object(s); 
  f_aggregate(2);
}

BINFUN(mpzmod_and,mpz_and)
BINFUN(mpzmod_or,mpz_ior)

static void mpzmod_compl(INT32 args)
{
  struct object *o;
  pop_n_elems(args);
  o=clone(mpzmod_program,0);
  push_object(o);
  mpz_com(OBTOMPZ(o), THIS);
}


#define CMPFUN(name,cmp)				\
static void name(INT32 args)				\
{							\
  INT32 i;						\
  if(!args) error("Comparison with one argument?\n");	\
  i=mpz_cmp(THIS, get_mpz(sp-args)) cmp 0;		\
  pop_n_elems(args);					\
  push_int(i);						\
}

CMPFUN(mpzmod_gt, >)
CMPFUN(mpzmod_lt, <)
CMPFUN(mpzmod_ge, >=)
CMPFUN(mpzmod_le, <=)
CMPFUN(mpzmod_eq, ==)
CMPFUN(mpzmod_nq, !=)

static void mpzmod_probably_prime_p(INT32 args)
{
  pop_n_elems(args);
  push_int(mpz_probab_prime_p(THIS, 25));
}

static void mpzmod_sgn(INT32 args)
{
  pop_n_elems(args);
  push_int(mpz_sgn(THIS));
}


static void mpzmod_sqrt(INT32 args)
{
  struct object *o;
  pop_n_elems(args);
  if(mpz_sgn(THIS)<0)
    error("mpz->sqrt() on negative number.\n");

  o=clone(mpzmod_program,0);
  push_object(o);
  mpz_sqrt(OBTOMPZ(o), THIS);
}

static void mpzmod_sqrtrem(INT32 args)
{
  struct object *root, *rem;
  
  pop_n_elems(args);
  if(mpz_sgn(THIS)<0)
    error("mpz->sqrtrem() on negative number.\n");

  root = clone(mpzmod_program,0);
  rem = clone(mpzmod_program,0);
  mpz_sqrtrem(OBTOMPZ(root), OBTOMPZ(rem), THIS);
  push_object(root); push_object(rem);
  f_aggregate(2);
}

static void mpzmod_lsh(INT32 args)
{
  MP_INT *tmp;
  pop_n_elems(args-1);
  push_string(int_type_string);
  int_type_string->refs++;
  f_cast(2);
  tmp=get_tmp();
  if(sp[-1].u.integer < 0)
    error("mpz->lsh on negative number.\n");
  mpz_mul_2exp(tmp, THIS, sp[-1].u.integer);
  return_temporary(1);
}

static void mpzmod_rsh(INT32 args)
{
  MP_INT *tmp;
  pop_n_elems(args-1);
  push_string(int_type_string);
  int_type_string->refs++;
  f_cast(2);
  tmp=get_tmp();
  mpz_set_ui(tmp,1);
  mpz_mul_2exp(tmp, tmp, sp[-1].u.integer);
  mpz_tdiv_q(tmp, THIS, tmp);
  return_temporary(1);
}

static void mpzmod_powm(INT32 args)
{
  MP_INT *tmp;
  if(args < 2)
    error("Too few arguments to mpzmod->powm()\n");

  tmp=get_tmp();
  mpz_powm(tmp, THIS, get_mpz(sp-args), get_mpz(sp+1-args));
  return_temporary(args);
}

static void mpzmod_not(INT32 args)
{
  pop_n_elems(args);
  push_int(!mpz_sgn(THIS));
}

static void init_mpz_glue(struct object *o)
{
  mpz_init(THIS);
}

static void exit_mpz_glue(struct object *o)
{
  mpz_clear(THIS);
}
#endif

void init_gmpmod_efuns(void) {}
void exit_gmpmod(void)
{
#ifdef HAVE_GMP_H
  if(temporary) free_object(temporary);
  free_program(mpzmod_program);
#endif
}

void init_gmpmod_programs(void)
{
#ifdef HAVE_GMP_H
  start_new_program();
  add_storage(sizeof(MP_INT));
  
  add_function("create", mpzmod_create,
  "function(void|string|int|float|object:void)"
  "|function(string,int:void)", 0);

#define MPZ_ARG_TYPE "int|float|object"
#define MPZ_BINOP_TYPE ("function(" MPZ_ARG_TYPE "...:object)")

  add_function("`+",mpzmod_add,MPZ_BINOP_TYPE,0);
  add_function("`-",mpzmod_sub,MPZ_BINOP_TYPE,0);
  add_function("`*",mpzmod_mul,MPZ_BINOP_TYPE,0);
  add_function("`/",mpzmod_div,MPZ_BINOP_TYPE,0);
  add_function("`%",mpzmod_mod,MPZ_BINOP_TYPE,0);
  add_function("`&",mpzmod_and,MPZ_BINOP_TYPE,0);
  add_function("`|",mpzmod_or,MPZ_BINOP_TYPE,0);

#define MPZ_SHIFT_TYPE "function(int|float|object:object)"
  add_function("`<<",mpzmod_lsh,MPZ_SHIFT_TYPE,0);
  add_function("`>>",mpzmod_rsh,MPZ_SHIFT_TYPE,0);

#define MPZ_CMPOP_TYPE ("function(" MPZ_ARG_TYPE ":int)")

  add_function("`>", mpzmod_gt,MPZ_CMPOP_TYPE,0);
  add_function("`<", mpzmod_lt,MPZ_CMPOP_TYPE,0);
  add_function("`>=",mpzmod_ge,MPZ_CMPOP_TYPE,0);
  add_function("`<=",mpzmod_le,MPZ_CMPOP_TYPE,0);

  add_function("`==",mpzmod_le,MPZ_CMPOP_TYPE,0);
  add_function("`!=",mpzmod_le,MPZ_CMPOP_TYPE,0);

  add_function("`!",mpzmod_not,"function(:int)",0);

  add_function("__hash",mpzmod_get_int,"function(:int)",0);
  add_function("cast",mpzmod_cast,"function(string:mixed)",0);

  add_function("digits", mpzmod_digits, "function(int:string)", 0);
  add_function("cast_to_int",mpzmod_get_int,"function(:int)",0);
  add_function("cast_to_string",mpzmod_get_string,"function(:string)",0);
  add_function("cast_to_float",mpzmod_get_float,"function(:float)",0);

  add_function("probably_prime_p",mpzmod_probably_prime_p,"function(:int)",0);
  add_function("gcd",mpzmod_gcd, MPZ_BINOP_TYPE, 0);
  add_function("gcdext", mpzmod_gcdext,
  "function(" MPZ_ARG_TYPE "," MPZ_ARG_TYPE ":array(object))", 0);
  add_function("gcdext2", mpzmod_gcdext2,
  "function(" MPZ_ARG_TYPE "," MPZ_ARG_TYPE ":array(object))", 0);
  add_function("sqrt",mpzmod_gcd,"function(:object)",0);
  add_function("sqrtrem", mpzmod_sqrtrem, "function(:array(object))", 0);
  add_function("`~",mpzmod_gcd,"function(:object)",0);
  add_function("powm",mpzmod_powm,
  "function(" MPZ_ARG_TYPE "," MPZ_ARG_TYPE ":object)", 0);
#if 0
  /* These are not implemented yet */
  add_function("squarep", mpzmod_squarep, "function(:int)", 0);
  add_function("divmod", mpzmod_divmod, "function(" MPZ_ARG_TYPE ":array(object))", 0);
  add_function("pow", mpzmod_pow, "function(int:object)", 0);
  add_function("divm", mpzmod_divm, "function(string|int|float|object, string|int|float|object:object)", 0);
  add_function("invert", mpzmod_invert, "function(:object)", 0);
  add_function("size_in_base", mpz_size, "function(:int)", 0);
  add_efun("mpz_pow", mpz_pow, "function(int, int)", 0);
  add_efun("mpz_fac", mpz_fac, "function(int|object:object)", 0);
#endif
  set_init_callback(init_mpz_glue);
  set_exit_callback(exit_mpz_glue);

  mpzmod_program=end_c_program("/precompiled/mpz");
  mpzmod_program->refs++;
#endif
}

