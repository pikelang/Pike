/* -*- C -*-
 *
 * Cmod header-file acting as a template for the hash classes.
 */

DOCSTART() @class cmod_CONCAT_EVAL(PIKE_NAME, _Info)
 *!
 *! Internal mixin class, intended to be multiply inherited
 *! together with HashInfo.
DOCEND()
PIKECLASS cmod_CONCAT_EVAL(PIKE_NAME, _Info)
{
  INIT
    {
      struct HashInfo_struct *HashInfo
	= (struct HashInfo_struct *) get_storage(Pike_fp->current_object,
						 HashInfo_program);

      werror(cmod_STRFY_EVAL(PIKE_NAME) "_Info->INIT\n");
      
      if (HashInfo && !HashInfo->meta)
	HashInfo->meta = &cmod_CONCAT_EVAL(nettle_, NETTLE_NAME);
      else {
	/* Can't call Pike_error here. 
	 * Pike_error("Can't initialize this object.\n"); */
	werror(cmod_STRFY_EVAL(PIKE_NAME) "_Info->INIT failed\n");
      }
    }
}

DOCSTART() @endclass cmod_CONCAT_EVAL(PIKE_NAME, _Info)
DOCEND()

DOCSTART() @class cmod_CONCAT_EVAL(PIKE_NAME, _State)
 *!
 *! State for PIKE_NAME hashing.
DOCEND()
PIKECLASS cmod_CONCAT_EVAL(PIKE_NAME, _State)
{
  INHERIT cmod_CONCAT_EVAL(PIKE_NAME, _Info);
  INHERIT HashState;
  CVAR struct cmod_CONCAT_EVAL(NETTLE_NAME, _ctx) NETTLE_NAME;

  INIT
    {
      struct HashState_struct *instance
	= (struct HashState_struct *) get_storage(Pike_fp->current_object,
						  HashState_program);
      werror(cmod_STRFY_EVAL(PIKE_NAME) "_State->INIT\n");

      assert(instance);

      cmod_CONCAT_EVAL(NETTLE_NAME, _init)(&THIS->NETTLE_NAME);      
      instance->ctx = &THIS->NETTLE_NAME;
    }
}
DOCSTART() @endclass cmod_CONCAT_EVAL(PIKE_NAME, _State)
DOCEND()
