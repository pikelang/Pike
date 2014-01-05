/* -*- C -*-
 *
 * Cmod header-file acting as a template for the cipher classes.
 */

DOCSTART() @class PIKE_NAME
 *!
 *! Implementation of the PIKE_NAME cipher.
 *!
DOCEND()
PIKECLASS PIKE_NAME
{
  DOCSTART() @decl inherit Cipher
  DOCEND()
  INHERIT Cipher;

  static const struct pike_cipher cmod_CONCAT_EVAL(pike_, NETTLE_NAME) =
    _PIKE_CIPHER(NETTLE_NAME, PIKE_NAME);

  INIT
    {
      struct Cipher_struct *cipher;
      ASSIGN_CURRENT_STORAGE(cipher, struct Cipher_struct, 1,
			     Cipher_program);
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
      /* Perform an inherit of the Cipher.State class that our parent
       * contains via its inherit of Cipher.
       */
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
	  low_inherit(parent_State_prog, 0,
		      parent_State_fun_num +
		      parent_prog->inherits[1].identifier_level,
		      1 + 42, 0, NULL);
	}
      }
    }

    CVAR struct cmod_CONCAT_EVAL(NETTLE_NAME, _ctx) NETTLE_NAME;

    INIT
    {
      struct Cipher_State_struct *state;
      ASSIGN_CURRENT_STORAGE(state, struct Cipher_State_struct, 1,
			     Cipher_State_program);
      state->ctx = &THIS->NETTLE_NAME;
    }
  }
  DOCSTART() @endclass State
  DOCEND()

}

DOCSTART() @endclass PIKE_NAME
DOCEND()
