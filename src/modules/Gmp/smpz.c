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

/* #define ENABLE_SMPZ_INVERT */

#undef THIS
#define DECLARE_THIS() struct pike_frame *_fp = Pike_fp
#define THIS ((MP_INT *)(_fp->current_storage))
#define THIS_PROGRAM (_fp->context->prog)

static struct program *smpz_program = NULL;

#ifdef HAVE_GMP5
static void smpz_powm(INT32 args)
{
  DECLARE_THIS();
  struct object *res = NULL;
  MP_INT *n, *e;

  if(args != 2)
    SIMPLE_WRONG_NUM_ARGS_ERROR ("powm", 2);

  e = get_mpz(Pike_sp - 2, 1, "powm", 1, 2);
  n = get_mpz(Pike_sp - 1, 1, "powm", 2, 2);

  if (!mpz_sgn(n))
    SIMPLE_DIVISION_BY_ZERO_ERROR ("powm");
  res = fast_clone_object(THIS_PROGRAM);
  mpz_powm_sec(OBTOMPZ(res), THIS, e, n);
  pop_n_elems(args);
  push_object(res);
}
#endif

#ifdef HAVE_GMP6

#ifdef ENABLE_SMPZ_INVERT
/* This function has multiple issues:
 *
 *   * It doesn't work as implemented (eg argument 6 to mpb_sec_invert()
 *     is wrong).
 *
 *   * It would clobber THIS. Gmp manual 8.1:
 *     "In either case, the input A is destroyed."
 *
 *   * To work, the number of limbs in THIS, modulo and res MUST
 *     be the same (aka n). This can probably be accomplished
 *     by using mpz_realloc2(), of which the Gmp manual 5.1 says:
 *     "Calling this function is never necessary; reallocation is
 *     handled automatically by GMP when needed."
 *
 * Fixing the above issues while still keeping the _sec property
 * is non-trivial, and best left to the Gmp people, so we wait for
 * an mpz_invert_sec().
 *
 *	/grubba 2014-12-18
 */

/* int mpn_sec_invert (mp_limb_t *rp, mp_limb_t *ap, const mp_limb_t *mp,
                       mp_size_t n, mp_bitcnt_t nbcnt, mp_limb_t *tp) */
/* int mpz_invert (mpz_t rop, const mpz_t op1, const mpz_t op2) */
static void smpz_invert(INT32 args)
{
  DECLARE_THIS();
  MP_INT *modulo;
  struct object *res;

  if (args != 1)
    SIMPLE_WRONG_NUM_ARGS_ERROR ("invert", 1);
  modulo = get_mpz(Pike_sp-1, 1, "invert", 1, 1);

  if (!mpz_sgn(modulo))
    SIMPLE_DIVISION_BY_ZERO_ERROR ("invert");
  res = fast_clone_object(THIS_PROGRAM);
  if (mpn_sec_invert(LIMBS(OBTOMPZ(res)), LIMBS(THIS), LIMBS(modulo),
                     NLIMBS(OBTOMPZ(res)), NLIMBS(THIS), NLIMBS(modulo)) == 0)
  {
    free_object(res);
    Pike_error("Not invertible.\n");
  }
  pop_n_elems(args);
  push_object(res);
}
#endif /* ENABLE_SMPZ_INVERT */
#endif

void pike_init_smpz_module(void)
{
  start_new_program();
  low_inherit(mpzmod_program, NULL, -1, 0, 0, NULL, NULL);

#ifdef HAVE_GMP5
  ADD_INT_CONSTANT("sec_powm", 1, 0);
  ADD_FUNCTION("powm",smpz_powm,tFunc(tMpz_arg tMpz_arg,tMpz_ret),0);
#endif

#ifdef HAVE_GMP6
#ifdef ENABLE_SMPZ_INVERT
  ADD_INT_CONSTANT("sec_invert", 1, 0);
  ADD_FUNCTION("invert", smpz_invert,tFunc(tMpz_arg,tMpz_ret),0);
#endif /* ENABLE_SMPZ_INVERT */
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
