/* -*- C -*-
 *
 * Cmod header-file acting as a template for the AEAD classes.
 */

DOCSTART() @class PIKE_NAME
 *!
 *! Implementation of the PIKE_NAME AEAD algorithm.
 *!
DOCEND()
PIKECLASS PIKE_NAME
{
  DOCSTART() @decl inherit AEAD
  DOCEND()
  INHERIT Nettle_AEAD;

  static const struct pike_aead cmod_CONCAT_EVAL(pike_, NETTLE_NAME) =
    _PIKE_AEAD(NETTLE_NAME, PIKE_NAME);

  INIT
    {
      struct Nettle_AEAD_struct *aead;
      ASSIGN_CURRENT_STORAGE(aead, struct Nettle_AEAD_struct, 1,
			     Nettle_AEAD_program);
      aead->meta = &cmod_CONCAT_EVAL(pike_, NETTLE_NAME);
    }

  DOCSTART() @class State
    *!
    *! State for PIKE_NAME encyption.
    *!
  DOCEND()
  PIKECLASS State
    program_flags PROGRAM_NEEDS_PARENT|PROGRAM_USES_PARENT;
  {
    DOCSTART() @decl inherit AEAD::State
    DOCEND()

    EXTRA
    {
      /* Perform an inherit of the AEAD.State class that our parent
       * contains via its inherit of AEAD.
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
	  parent_State_fun_num =
	    really_low_reference_inherited_identifier(Pike_compiler->previous,
						      0, parent_State_fun_num);
	  low_inherit(parent_State_prog, 0,
		      parent_State_fun_num,
		      1 + 42, 0, NULL);
	}
      }
    }

    CVAR struct cmod_CONCAT_EVAL(NETTLE_NAME, _ctx) NETTLE_NAME;

    INIT
    {
      struct Nettle_AEAD_State_struct *state;
      ASSIGN_CURRENT_STORAGE(state, struct Nettle_AEAD_State_struct, 1,
			     Nettle_AEAD_State_program);
      state->ctx = &THIS->NETTLE_NAME;
    }
  }
  DOCSTART() @endclass State
  DOCEND()

}

DOCSTART() @endclass PIKE_NAME
DOCEND()
