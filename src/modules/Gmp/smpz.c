/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "gmp_machine.h"
#include "my_gmp.h"

#include "interpret.h"
#include "program.h"

#define sp Pike_sp
#define fp Pike_fp

#undef THIS
#define THIS ((MP_INT *)(fp->current_storage))
#define THIS_PROGRAM (fp->context->prog)

#define LIMBS(X) THIS->_mp_alloc
#define NLIMBS(X) THIS->_mp_size

struct program *smpz_program = NULL;

#ifdef HAVE_GMP5
static void smpz_powm(INT32 args)
{
  struct object *res = NULL;
  MP_INT *n, *e;
  
  if(args != 2)
    SIMPLE_WRONG_NUM_ARGS_ERROR ("powm", 2);

  e = get_mpz(sp - 2, 1, "powm", 1, 2);
  n = get_mpz(sp - 1, 1, "powm", 2, 2);

  if (!mpz_sgn(n))
    SIMPLE_DIVISION_BY_ZERO_ERROR ("powm");
  res = fast_clone_object(THIS_PROGRAM);
  mpz_powm_sec(OBTOMPZ(res), THIS, e, n);
  pop_n_elems(args);
  push_object(res);
}
#endif

#ifdef HAVE_GMP6

/* int mpn_sec_invert (mp_limb_t *rp, mp_limb_t *ap, const mp_limb_t *mp,
                       mp_size_t n, mp_bitcnt_t nbcnt, mp_limb_t *tp) */
/* int mpz_invert (mpz_t rop, const mpz_t op1, const mpz_t op2) */
static void smpz_invert(INT32 args)
{
  MP_INT *modulo;
  struct object *res;

  if (args != 1)
    SIMPLE_WRONG_NUM_ARGS_ERROR ("invert", 1);
  modulo = get_mpz(sp-1, 1, "invert", 1, 1);

  if (!mpz_sgn(modulo))
    SIMPLE_DIVISION_BY_ZERO_ERROR ("invert");
  res = fast_clone_object(THIS_PROGRAM);
  if (mpn_sec_invert(LIMBS(OBTOMPZ(res)), LIMBS(THIS), LIMBS(modulo),
                     NLIMBS(OBTOMPZ(res)), NLIMBS(THIS), NLIMBS(module)) == 0)
  {
    free_object(res);
    Pike_error("Not invertible.\n");
  }
  pop_n_elems(args);
  push_object(res);
}
#endif

#define tMpz_arg tOr3(tInt,tFloat,tObj)
#define tMpz_ret tObjIs_GMP_MPZ

void pike_init_smpz_module(void)
{
  start_new_program();
  low_inherit(mpzmod_program, NULL, -1, 0, 0, NULL);

#ifdef HAVE_GMP5
  ADD_INT_CONSTANT("sec_powm", 1, 0);
  ADD_FUNCTION("powm",smpz_powm,tFunc(tMpz_arg tMpz_arg,tMpz_ret),0);
#endif

#ifdef HAVE_GMP6
  ADD_INT_CONSTANT("sec_invert", 1, 0);
  ADD_FUNCTION("invert", smpz_invert,tFunc(tMpz_arg,tMpz_ret),0);
#endif

  smpz_program = end_program();
  add_program_constant("smpz", smpz_program, 0);
}

void pike_exit_smpz_module(void)
{
  if( smpz_program )
  {
    free_program(smpz_program);
    smpz_program = NULL;
  }
}
