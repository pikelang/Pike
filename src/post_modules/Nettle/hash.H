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

