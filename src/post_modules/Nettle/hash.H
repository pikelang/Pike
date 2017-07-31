/* -*- C -*-
 *
 * Cmod header-file acting as a template for the hash classes.
 */

DOCSTART() @class PIKE_NAME
 *!
 *! Implementation of the PIKE_NAME hash algorithm.
 *!
DOCEND()
PIKECLASS PIKE_NAME
{
  DOCSTART() @decl inherit Hash
  DOCEND()
  INHERIT Nettle_Hash;

  INIT
    {
      struct Nettle_Hash_struct *hash;
      ASSIGN_CURRENT_STORAGE(hash, struct Nettle_Hash_struct, 1,
			     Nettle_Hash_program);

      werror(cmod_STRFY_EVAL(PIKE_NAME) "->INIT\n");

      hash->meta = &cmod_CONCAT_EVAL(nettle_, NETTLE_NAME);
    }

#ifdef SPECIAL_PBKDF2
  PIKEFUN string pbkdf2(string(8bit) password, string(8bit) salt,
                        int rounds, int bytes)
  {
    struct pike_string *dst = begin_shared_string(bytes);
    NO_WIDE_STRING(password);
    NO_WIDE_STRING(salt);
    cmod_CONCAT_EVAL(pbkdf2_hmac_,NETTLE_NAME)(password->len,
                                               (const uint8_t *)password->str,
                                               rounds,
                                               salt->len,
                                               (const uint8_t *)salt->str,
                                               bytes,
                                               (uint8_t *)dst->str);
    push_string(end_shared_string(dst));
  }
#endif

  DOCSTART() @class State
    *!
    *! State for PIKE_NAME hashing.
    *!
  DOCEND()
  PIKECLASS State
    program_flags PROGRAM_NEEDS_PARENT|PROGRAM_USES_PARENT;
  {
    DOCSTART() @decl inherit Hash::State
    DOCEND()

    EXTRA
    {
      /* Perform an inherit of the Hash.State class that our parent
       * contains via its inherit of Hash.
       */
      inherit_internal_class(MK_STRING("State"));
    }

  CVAR struct cmod_CONCAT_EVAL(NETTLE_NAME, _ctx) NETTLE_NAME;

  INIT
    {
      struct Nettle_Hash_State_struct *instance;
      ASSIGN_CURRENT_STORAGE(instance, struct Nettle_Hash_State_struct, 1,
			     Nettle_Hash_State_program);

      werror(cmod_STRFY_EVAL(PIKE_NAME) ".State->INIT\n");

      cmod_CONCAT_EVAL(NETTLE_NAME, _init)(&THIS->NETTLE_NAME);      
      instance->ctx = &THIS->NETTLE_NAME;
    }
  }
  DOCSTART() @endclass State
  DOCEND()

}
DOCSTART() @endclass PIKE_NAME
DOCEND()

