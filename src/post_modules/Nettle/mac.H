/* -*- C -*-
 *
 * Cmod header-file acting as a template for the MAC classes.
 */

DOCSTART() @class PIKE_NAME
 *!
 *! Implementation of the PIKE_NAME MAC algorithm.
 *!
DOCEND()
PIKECLASS PIKE_NAME
{
  DOCSTART() @decl inherit MAC
  DOCEND()
  INHERIT Nettle_MAC;

  static const struct pike_mac cmod_CONCAT_EVAL(pike_, NETTLE_NAME) =
    _PIKE_MAC(NETTLE_NAME, PIKE_NAME);

  INIT
    {
      struct Nettle_MAC_struct *mac;
      ASSIGN_CURRENT_STORAGE(mac, struct Nettle_MAC_struct, 1,
			     Nettle_MAC_program);
      mac->meta = &cmod_CONCAT_EVAL(pike_, NETTLE_NAME);
    }

  DOCSTART() @class State
    *!
    *! State for PIKE_NAME macing.
    *!
  DOCEND()
  PIKECLASS State
    program_flags PROGRAM_NEEDS_PARENT|PROGRAM_USES_PARENT;
  {
    DOCSTART() @decl inherit MAC::State
    DOCEND()

    EXTRA
    {
      /* Perform an inherit of the MAC.State class that our parent
       * contains via its inherit of MAC.
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
      struct Nettle_MAC_State_struct *instance;
      ASSIGN_CURRENT_STORAGE(instance, struct Nettle_MAC_State_struct, 1,
			     Nettle_MAC_State_program);
      instance->ctx = &THIS->NETTLE_NAME;
    }
  }
  DOCSTART() @endclass State
  DOCEND()

}
DOCSTART() @endclass PIKE_NAME
DOCEND()

