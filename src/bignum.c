#include "global.h"

#ifdef AUTO_BIGNUM

#include "interpret.h"
#include "program.h"
#include "object.h"
#include "svalue.h"
#include "error.h"

/* FIXME: Cache the mpz-function? */
struct object *make_bignum_object(void)
{
  struct object *o;
  
  push_text("Gmp.mpz");
  SAFE_APPLY_MASTER("resolv", 1);
  
  if(sp[-1].type != T_FUNCTION)
    error("Failed to resolv Gmp.mpz!\n");
  
  stack_swap();
  f_call_function(2);
  
  o = sp[-1].u.object;
  sp--;

  return o;
}

#if 0   /* Hubbe's example. */

struct program *auto_bignum_program=0;

void exit_auto_bignum(void)
{
  if(auto_bignum_program)
  {
    free_program(auto_bignum_program);
    auto_bignum_program=0;
  }
}

struct program *get_auto_bignum_program()
{
  if(auto_bignum_program)
    return auto_bignum_program;

  push_text("Gmp.mpz");
  APPLY_MASTER("resolv",1);
  if(sp[-1].type != T_PROGRAM)
    error("Failed to resolv Gmp.mpz!\n");
  auto_bignum_program=sp[-1].u.program;
  sp--;
}
#endif

#endif /* AUTO_BIGNUM */
