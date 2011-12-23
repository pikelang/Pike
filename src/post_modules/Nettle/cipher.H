/* -*- C -*-
 *
 * Cmod header-file acting as a template tor the cipher classes.
 */

DOCSTART() @class cmod_CONCAT_EVAL(PIKE_NAME, _Info)
 *!
 *! Internal mixin class, intended to be multiply inherited
 *! together with CipherInfo.
DOCEND()
PIKECLASS cmod_CONCAT_EVAL(PIKE_NAME, _Info)
{
  INIT
    {
      static const struct pike_cipher cmod_CONCAT_EVAL(pike_, NETTLE_NAME) =
	_PIKE_CIPHER(NETTLE_NAME, PIKE_NAME);
      struct CipherInfo_struct *CipherInfo = GET_INFO(Pike_fp->current_object);

      if (CipherInfo && !CipherInfo->meta)
	CipherInfo->meta = &cmod_CONCAT_EVAL(pike_, NETTLE_NAME);
    }
}

DOCSTART() @endclass cmod_CONCAT_EVAL(PIKE_NAME, _Info)
DOCEND()

DOCSTART() @class cmod_CONCAT_EVAL(PIKE_NAME, _State)
 *!
 *! State for PIKE_NAME encyption
DOCEND()
PIKECLASS cmod_CONCAT_EVAL(PIKE_NAME, _State)
{
  INHERIT cmod_CONCAT_EVAL(PIKE_NAME, _Info);
  INHERIT CipherState;
  CVAR struct cmod_CONCAT_EVAL(NETTLE_NAME, _ctx) NETTLE_NAME;

  INIT
    {
      struct CipherState_struct *state
	= (struct CipherState_struct *) get_storage(Pike_fp->current_object,
						    CipherState_program);

      assert(state);
      state->ctx = &THIS->NETTLE_NAME;
    }
}
DOCSTART() @endclass cmod_CONCAT_EVAL(PIKE_NAME, _State)
DOCEND()
