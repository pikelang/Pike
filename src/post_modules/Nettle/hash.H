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
  INHERIT Hash;

  INIT
    {
      /* FIXME: Ought to traverse the inherit graph for
       *        the current program.
       */
      struct Hash_struct *Hash
	= (struct Hash_struct *) get_storage(Pike_fp->current_object,
					     Hash_program);

      werror(cmod_STRFY_EVAL(PIKE_NAME) "->INIT\n");

      if (Hash && !Hash->meta)
	Hash->meta = &cmod_CONCAT_EVAL(nettle_, NETTLE_NAME);
      else {
	/* Can't call Pike_error here. 
	 * Pike_error("Can't initialize this object.\n"); */
	werror(cmod_STRFY_EVAL(PIKE_NAME) "->INIT failed\n");
      }
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
      /* Perform an inherit of the HashState class that our parent
       * contains via its inherit of HashInfo.
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
	  DOCSTART() @decl inherit Hash::State
	   *!
	   *! Inherits @[Nettle.Hash.State] via
	   *! the inherit of @[Nettle.Hash] in our
	   *! parent (@[Hash]).
	  DOCEND()
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
      /* FIXME: Ought to traverse the inherit graph for
       *        the current program.
       */
      struct Hash_State_struct *instance
	= (struct Hash_State_struct *)
	get_storage(Pike_fp->current_object, Hash_State_program);
      werror(cmod_STRFY_EVAL(PIKE_NAME) ".State->INIT\n");

      assert(instance);

      cmod_CONCAT_EVAL(NETTLE_NAME, _init)(&THIS->NETTLE_NAME);      
      instance->ctx = &THIS->NETTLE_NAME;
    }
  }
  DOCSTART() @endclass State
  DOCEND()

}
DOCSTART() @endclass PIKE_NAME
DOCEND()

