/* -*- C -*-
 *
 * Cmod header-file acting as a template for the cipher classes.
 */

#cmod_ifndef CIPHER_MODE

#cmod_error "CIPHER_MODE MUST be defined!"

#cmod_elif (CIPHER_MODE == STREAM_MODE)
/* Stream cipher. */

/* Base name of inherited class. */
#cmod_define BASE_CIPHER	Cipher
/* Number of inherits to get to the Nettle_Cipher inherit. */
#cmod_define INHERIT_DEPTH	1

#cmod_elif (CIPHER_MODE == BLOCK16_MODE)
/* Block cipher with a 16 byte block size. */

#cmod_define BASE_CIPHER	BlockCipher16
#cmod_define INHERIT_DEPTH	(BLOCKCIPHER16_CIPHER_INHERIT + 1)

#cmod_else
/* Block cipher with a block size other than 16 bytes. */

#cmod_define BASE_CIPHER	BlockCipher
#cmod_define INHERIT_DEPTH	(BLOCKCIPHER_CIPHER_INHERIT + 1)

#cmod_endif

DOCSTART() @class PIKE_NAME
 *!
 *! Implementation of the PIKE_NAME cipher.
 *!
DOCEND()
PIKECLASS PIKE_NAME
{
  DOCSTART() @decl inherit BASE_CIPHER
  DOCEND()
  INHERIT cmod_CONCAT_EVAL(Nettle_, BASE_CIPHER);

  static const struct pike_cipher cmod_CONCAT_EVAL(pike_, NETTLE_NAME) =
    _PIKE_CIPHER(NETTLE_NAME, PIKE_NAME);

  INIT
    {
      struct Nettle_Cipher_struct *cipher;
      ASSIGN_CURRENT_STORAGE(cipher, struct Nettle_Cipher_struct,
			     INHERIT_DEPTH, Nettle_Cipher_program);
      cipher->meta = &cmod_CONCAT_EVAL(pike_, NETTLE_NAME);
    }

  DOCSTART() @class State
    *!
    *! State for PIKE_NAME encyption.
    *!
  DOCEND()
  PIKECLASS State
    program_flags PROGRAM_NEEDS_PARENT|PROGRAM_USES_PARENT;
  {
    DOCSTART() @decl inherit Cipher::State
    DOCEND()

    EXTRA
    {
      cmod___SLASH__* Perform an inherit of the Cipher.State class that our parent
       * contains via its inherit of BASE_CIPHER.
       *cmod___SLASH__
      struct program *parent_prog = Pike_compiler->previous->new_program;
      struct object *parent_obj = Pike_compiler->previous->fake_object;
      int parent_State_fun_num =
	really_low_find_shared_string_identifier(MK_STRING("State"),
						 parent_prog,
						 SEE_PROTECTED|SEE_PRIVATE);
      if (parent_State_fun_num >= 0) {
	struct program *parent_State_prog =
	  low_program_from_function(parent_obj, parent_State_fun_num);
	if (parent_State_prog) {
	  low_inherit(parent_State_prog, 0, parent_State_fun_num,
		      1 + 42, 0, NULL);
	}
      }
    }

    CVAR struct cmod_CONCAT_EVAL(NETTLE_NAME, _ctx) NETTLE_NAME;

    INIT
    {
      struct Nettle_Cipher_State_struct *state;
      ASSIGN_CURRENT_STORAGE(state, struct Nettle_Cipher_State_struct, 1,
			     Nettle_Cipher_State_program);
      state->ctx = &THIS->NETTLE_NAME;
    }
  }
  DOCSTART() @endclass State
  DOCEND()

}

DOCSTART() @endclass PIKE_NAME
DOCEND()

#cmod_undef CIPHER_MODE
#cmod_undef BASE_CIPHER
#cmod_undef INHERIT_DEPTH
