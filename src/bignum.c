#include "global.h"

#ifdef AUTO_BIGNUM

#include "interpret.h"
#include "program.h"
#include "object.h"
#include "svalue.h"
#include "error.h"

struct svalue auto_bignum_program = { T_INT };

int gmp_library_loaded=0;
int gmp_library_resolving=0;

static void resolve_auto_bignum_program(void)
{
  if(auto_bignum_program.type == T_INT)
  {
    gmp_library_resolving=1;
    push_text("Gmp.bignum");
    SAFE_APPLY_MASTER("resolv", 1);
    
    if(sp[-1].type != T_FUNCTION)
      error("Failed to resolv Gmp.mpz!\n");
    
    auto_bignum_program=sp[-1];
    sp--;
    dmalloc_touch_svalue(sp);
    gmp_library_resolving=0;
  }
}

struct program *get_auto_bignum_program(void)
{
  resolve_auto_bignum_program();
  return program_from_function(&auto_bignum_program);
}

struct program *get_auto_bignum_program_or_zero(void)
{
  if(!gmp_library_loaded ||
     gmp_library_resolving  ||
     !get_master()) return 0;
  resolve_auto_bignum_program();
  return program_from_function(&auto_bignum_program);
}

void exit_auto_bignum(void)
{
  free_svalue(&auto_bignum_program);
  auto_bignum_program.type=T_INT;
}

void convert_stack_top_to_bignum(void)
{
  resolve_auto_bignum_program();
  apply_svalue(&auto_bignum_program, 1);

  if(sp[-1].type != T_OBJECT)
    error("Gmp.mpz conversion failed.\n");
}

void convert_stack_top_with_base_to_bignum(void)
{
  resolve_auto_bignum_program();
  apply_svalue(&auto_bignum_program, 2);

  if(sp[-1].type != T_OBJECT)
    error("Gmp.mpz conversion failed.\n");
}

int is_bignum_object(struct object *o)
{
  /* Note:
   * This function should *NOT* try to resolv Gmp.mpz unless
   * it is already loaded into memory.
   * /Hubbe
   */

  if(!gmp_library_loaded ||
     gmp_library_resolving ||
    !get_master())
    return 0; /* not possible */
 
  resolve_auto_bignum_program();
  return o->prog == program_from_function(&auto_bignum_program);
}

int is_bignum_object_in_svalue(struct svalue *sv)
{
  return sv->type == T_OBJECT && is_bignum_object(sv->u.object);
}

struct object *make_bignum_object(void)
{
  convert_stack_top_to_bignum();
  return (--sp)->u.object;
}

struct object *bignum_from_svalue(struct svalue *s)
{
  push_svalue(s);
  convert_stack_top_to_bignum();
  return (--sp)->u.object;
}

struct pike_string *string_from_bignum(struct object *o, int base)
{
  push_int(base);
  safe_apply(o, "digits", 1);
  
  if(sp[-1].type != T_STRING)
    error("Gmp.mpz string conversion failed.\n");
  
  return (--sp)->u.string;
}

void convert_svalue_to_bignum(struct svalue *s)
{
  push_svalue(s);
  convert_stack_top_to_bignum();
  free_svalue(s);
  *s=sp[-1];
  sp--;
  dmalloc_touch_svalue(sp);
}

#ifdef INT64
/* These routines can be made much more optimized. */

#define BIGNUM_INT64_MASK  0xffff
#define BIGNUM_INT64_SHIFT 16

void push_int64(INT64 i)
{
  if(i == (INT_TYPE)i)
  {
    push_int((INT_TYPE)i);
  }
  else
  {
    int neg, pos, lshfun, orfun;

    neg = (i < 0);

    if(neg)
      i = ~i;

    push_int(i & BIGNUM_INT64_MASK);
    i >>= BIGNUM_INT64_SHIFT;
    convert_stack_top_to_bignum();
    
    lshfun = FIND_LFUN(sp[-1].u.object->prog, LFUN_LSH);
    orfun = FIND_LFUN(sp[-1].u.object->prog, LFUN_OR);
    
    for(pos = BIGNUM_INT64_SHIFT; i; pos += BIGNUM_INT64_SHIFT)
    {
      push_int(i & BIGNUM_INT64_MASK);
      i >>= BIGNUM_INT64_SHIFT;
      convert_stack_top_to_bignum();
      
      push_int(pos);
      apply_low(sp[-2].u.object, lshfun, 1);
      stack_swap();
      pop_stack();
      
      apply_low(sp[-2].u.object, orfun, 1);
      stack_swap();
      pop_stack();

      if(sp[-1].type == T_INT)
	convert_stack_top_to_bignum();
    }
    
    if(neg)
      apply_low(sp[-1].u.object,FIND_LFUN(sp[-1].u.object->prog,LFUN_COMPL),0);
  }
}

int int64_from_bignum(INT64 *i, struct object *bignum)
{
  int neg, pos, rshfun, andfun;

  *i = 0;

  push_int(0);
  apply_low(bignum, FIND_LFUN(bignum->prog, LFUN_LT), 1);
  if(sp[-1].type != T_INT)
    error("Result from Gmp.bignum->`< not an integer.\n");
  neg = (--sp)->u.integer;

  if(neg)
    apply_low(bignum, FIND_LFUN(bignum->prog, LFUN_COMPL), 0);

  rshfun = FIND_LFUN(bignum->prog, LFUN_RSH);
  andfun = FIND_LFUN(bignum->prog, LFUN_AND);
  
  ref_push_object(bignum);
    
  for(pos = 0; sp[-1].type != T_INT; )
  {
    push_int(BIGNUM_INT64_MASK);
    apply_low(sp[-2].u.object, andfun, 1);
    if(sp[-1].type != T_INT)
      error("Result from Gmp.bignum->`& not an integer.\n");
    *i |= (INT64)(--sp)->u.integer << (INT64)pos;
    pos += BIGNUM_INT64_SHIFT;
    
    push_int(BIGNUM_INT64_SHIFT);
    apply_low(sp[-2].u.object, rshfun, 1);
    stack_swap();
    pop_stack();
  }
  
  *i |= (INT64)(--sp)->u.integer << (INT64)pos;

  if(neg)
    *i = ~*i;
  
  return 1;   /* We may someday return 0 if the conversion fails. */
}
#endif /* INT64 */

#endif /* AUTO_BIGNUM */
