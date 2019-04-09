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
      struct Nettle_Hash_struct *hash;
      ASSIGN_CURRENT_STORAGE(hash, struct Nettle_Hash_struct, 1,
			     Nettle_Hash_program);

      werror(cmod_STRFY_EVAL(PIKE_NAME) "->INIT\n");

      hash->meta = &cmod_CONCAT_EVAL(nettle_, NETTLE_NAME);
    }

#ifdef SPECIAL_PBKDF2
  PIKEFUN string(8bit) pbkdf2(string(8bit) password, string(8bit) salt,
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

#ifdef HAVE_NETTLE_HMAC_H
  DOCSTART() @module HMAC
    *!
    *! Accellerated implementation of HMAC (Hashing for Message Authenticity
    *! Control) with the PIKE_NAME hash algorithm.
    *!
    *! @seealso
    *!   @[Crypto.HMAC]
  DOCEND()
  PIKECLASS _HMAC
    flags ID_PROTECTED;
  {
    EXTRA
    {
      lexical_inherit(1, MK_STRING("_HMAC"), 0, REPORT_ERROR);
    }

    DOCSTART() @class PIKE_NAME
      *! The HMAC hash state.
    DOCEND()
    PIKECLASS State
    {
      CVAR struct HMAC_CTX(struct cmod_CONCAT_EVAL(NETTLE_NAME, _ctx)) ctx;
      CVAR const struct nettle_hash *meta;

      static int cmod_CONCAT_EVAL(f_Hash__HMAC_, NETTLE_NAME, _create_fun_num) = -1;

      EXTRA
      {
	int hmac_create_fun_num = -1;
        lexical_inherit(1, MK_STRING("State"), 0, REPORT_ERROR);
	hmac_create_fun_num =
	  FIND_LFUN(Pike_compiler->new_program->inherits[1].prog, LFUN_CREATE);
	if (hmac_create_fun_num >= 0) {
	  cmod_CONCAT_EVAL(f_Hash__HMAC_, NETTLE_NAME, _create_fun_num) =
	    really_low_reference_inherited_identifier(NULL, 1, hmac_create_fun_num);
	}
      }

      DOCSTART() @decl void create(string(8bit) passwd, void|int b)
	*! @param passwd
	*!   The secret password (K).
	*!
	*! @param b
	*!   Block size. Must @expr{0@} (zero) or equal to the @[block_size()].
      DOCEND()
      PIKEFUN void create(string(8bit) passwd, void|int b)
      {
        const struct nettle_hash *meta = THIS->meta = ((struct Nettle_Hash_struct *)parent_storage(2, Nettle_Hash_program))->meta;
        NO_WIDE_STRING(passwd);
        HMAC_SET_KEY(&THIS->ctx, meta, passwd->len, (const uint8_t *)passwd->str);
	if (cmod_CONCAT_EVAL(f_Hash__HMAC_, NETTLE_NAME, _create_fun_num) != -1) {
	  /* Pass along the password to the inherited pike-level code. */
	  apply_current(cmod_CONCAT_EVAL(f_Hash__HMAC_, NETTLE_NAME,
					 _create_fun_num), args);
	}
      }

      PIKEFUN string(8bit) `()(string(8bit) text)
	flags ID_PROTECTED;
      {
        struct cmod_CONCAT_EVAL(NETTLE_NAME, _ctx) state;
        int bytes = THIS->meta->digest_size;
        struct pike_string *dst = begin_shared_string(bytes);
        NO_WIDE_STRING(text);

        memcpy(&state, &THIS->ctx.inner, THIS->meta->context_size);
        THIS->meta->update(&state, text->len, (const uint8_t *)text->str);
        hmac_digest(&THIS->ctx.outer, &THIS->ctx.inner, &state,
                    THIS->meta, THIS->meta->digest_size, (uint8_t *)dst->str);
        push_string(end_shared_string(dst));
      }

      PIKEFUN object update(string(8bit) data)
        optflags OPT_SIDE_EFFECT;
        rawtype tFunc(tStr8, tObjImpl_NETTLE_HASH_STATE);
      {
        NO_WIDE_STRING(data);
        THIS->meta->update(&THIS->ctx.state, data->len, (const uint8_t *)data->str);
        push_object(this_object());
      }

      PIKEFUN object init(string(8bit)|void data)
        optflags OPT_SIDE_EFFECT;
        rawtype tFunc(tOr(tVoid, tStr8), tObjImpl_NETTLE_HASH_STATE);
      {
        memcpy(&THIS->ctx.state, &THIS->ctx.inner, THIS->meta->context_size);
        if(data)
        {
          NO_WIDE_STRING(data);
          THIS->meta->update(&THIS->ctx.state, data->len, (const uint8_t *)data->str);
        }
        push_object(this_object());
      }

      PIKEFUN string(8bit) digest(int(0..)|void length)
      {
        int bytes = THIS->meta->digest_size;
        struct pike_string *dst;
        if(length)
          bytes = MINIMUM(bytes, length->u.integer);
        dst = begin_shared_string(bytes);

        hmac_digest(&THIS->ctx.outer, &THIS->ctx.inner, &THIS->ctx.state,
                    THIS->meta, bytes, (uint8_t *)dst->str);

        push_string(end_shared_string(dst));
      }

      PIKEFUN string(8bit) digest_info(string(8bit) text)
      {
        struct external_variable_context loc;
        apply_current(cmod_CONCAT_EVAL(f_Nettle_,PIKE_NAME,_cq__HMAC_State_update_fun_num),1);

        /* Basically apply_external() with find_identifier() added. */
        loc.o = Pike_fp->current_object;
        loc.parent_identifier = Pike_fp->fun;
        if (loc.o->prog) {
	  int id;
          loc.inherit = INHERIT_FROM_INT(loc.o->prog, loc.parent_identifier);
          find_external_context(&loc, 2);
          id = find_identifier("pkcs_digest", loc.o->prog);
          if( id<0 )
            Pike_error("Could not find pkcs_digest.\n");
          apply_low(loc.o, id + loc.inherit->identifier_level, 1);
        }
        else
          Pike_error("Apply on parent of destructed object.\n");
      }
    }
    DOCSTART() @endclass PIKE_NAME
    DOCEND()
  }
#endif
  DOCSTART() @endmodule HMAC
  DOCEND()

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
      lexical_inherit(1, MK_STRING("State"), 0, REPORT_ERROR);
    }

  CVAR struct cmod_CONCAT_EVAL(NETTLE_NAME, _ctx) NETTLE_NAME;

  INIT
    {
      struct Nettle_Hash_State_struct *instance;
      ASSIGN_CURRENT_STORAGE(instance, struct Nettle_Hash_State_struct, 1,
			     Nettle_Hash_State_program);

      cmod_CONCAT_EVAL(NETTLE_NAME, _init)(&THIS->NETTLE_NAME);
      instance->ctx = &THIS->NETTLE_NAME;
    }
  }
  DOCSTART() @endclass State
  DOCEND()

}
DOCSTART() @endclass PIKE_NAME
DOCEND()

