/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "main.h"
#include "svalue.h"
#include "stralloc.h"
#include "array.h"
#include "mapping.h"
#include "multiset.h"
#include "object.h"
#include "program.h"
#include "constants.h"
#include "pike_error.h"
#include "buffer.h"
#include "interpret.h"
#include "gc.h"
#include "pike_macros.h"
#include "pike_types.h"
#include <ctype.h>
#include "queue.h"
#include "bignum.h"
#include "cyclic.h"
#include "pike_float.h"

PMOD_EXPORT const struct svalue svalue_undefined = SVALUE_INIT (T_INT, NUMBER_UNDEFINED, 0);
PMOD_EXPORT const struct svalue svalue_int_zero = SVALUE_INIT_INT (0);
PMOD_EXPORT const struct svalue svalue_int_one = SVALUE_INIT_INT (1);

#ifdef PIKE_DEBUG
PMOD_EXPORT const char msg_type_error[] =
  "Type error: %d\n";
PMOD_EXPORT const char msg_assign_svalue_error[] =
  "assign_svalue_no_free(): to and from are both %p.\n";
PMOD_EXPORT const char msg_sval_obj_wo_refs[] =
  "Svalue to object without references.\n";
PMOD_EXPORT const char msg_ssval_obj_wo_refs[] =
  "(short) Svalue to object without references.\n";
#endif

PMOD_EXPORT void check_destructed(struct svalue *s)
{
  if(IS_DESTRUCTED(s)) {
    free_object(s->u.object);
    SET_SVAL(*s, PIKE_T_INT,
	     NUMBER_DESTRUCTED,
	     integer, 0);
  }
}


/*
 * This routine frees a short svalue given a pointer to it and
 * its type.
 */

PMOD_EXPORT void really_free_short_svalue_ptr(void **s, TYPE_T type)
{
  union anything tmp;
  tmp.ptr = *s;
  *s = NULL; /* Prevent cyclic calls */
  switch(type)
  {
    case T_ARRAY:
      really_free_array(tmp.array);
      break;

    case T_MAPPING:
      really_free_mapping(tmp.mapping);
      break;

    case T_MULTISET:
      really_free_multiset(tmp.multiset);
      break;

    case T_OBJECT:
      schedule_really_free_object(tmp.object);
      break;

    case T_PROGRAM:
      really_free_program(tmp.program);
      break;

    case T_STRING:
      really_free_string(tmp.string);
      break;

    case T_TYPE:
      really_free_pike_type(tmp.type);
      break;

#ifdef PIKE_DEBUG
    default:
	Pike_fatal("Bad type in free_short_svalue.\n");
#endif
  }
}

PMOD_EXPORT void really_free_svalue(struct svalue *s)
{
  struct svalue tmp;
  move_svalue (&tmp, s);
  mark_free_svalue (s);

  switch(TYPEOF(tmp))
  {
  case T_ARRAY:
    really_free_array(tmp.u.array);
    break;

  case T_MAPPING:
    really_free_mapping(tmp.u.mapping);
    break;

  case T_MULTISET:
    really_free_multiset(tmp.u.multiset);
    break;

  case T_FUNCTION:
    if(SUBTYPEOF(tmp) == FUNCTION_BUILTIN)
    {
      really_free_callable(tmp.u.efun);
      break;
    }
    /* fall through */

  case T_OBJECT:
    schedule_really_free_object(tmp.u.object);
    return;

  case T_PROGRAM:
    really_free_program(tmp.u.program);
    break;

  case T_TYPE:
    /* Add back the reference, and call the normal free_type(). */
    add_ref(tmp.u.type);
    free_type(tmp.u.type);
    break;

  case T_STRING:
    really_free_string(tmp.u.string);
    break;

  case PIKE_T_FREE:
    break;

#ifdef PIKE_DEBUG
  default:
    Pike_fatal("Bad type in really_free_svalue: %d.\n", TYPEOF(tmp));
#endif
  }

  assert_free_svalue (s);
}

PMOD_EXPORT void do_free_svalue(struct svalue *s)
{
  free_svalue(s);
}

/* Free a bunch of normal svalues.
 * We put this routine here so the compiler can optimize the call
 * inside the loop if it wants to
 */
PMOD_EXPORT void debug_free_svalues(struct svalue *s, size_t num, INT32 type_hint DMALLOC_LINE_ARGS)
{
  switch(type_hint)
  {
  case 0:
  case BIT_INT:
  case BIT_FLOAT:
  case BIT_FLOAT | BIT_INT:
    return;

#define DOTYPE(X,Y,Z) case X:						\
   while(num--) {							\
    DO_IF_DMALLOC(debug_malloc_update_location(s->u.Z, dmalloc_location)); \
    Y(s->u.Z);								\
    assert_free_svalue (s);						\
    s++;								\
   }return

    DOTYPE(BIT_STRING, free_string, string);
    DOTYPE(BIT_ARRAY, free_array, array);
    DOTYPE(BIT_MAPPING, free_mapping, mapping);
    DOTYPE(BIT_MULTISET, free_multiset, multiset);
    DOTYPE(BIT_OBJECT, free_object, object);
    DOTYPE(BIT_PROGRAM, free_program, program);

  case BIT_FUNCTION:
    while(num--)
    {
#ifdef DEBUG_MALLOC
      debug_malloc_update_location(s->u.refs  DMALLOC_PROXY_ARGS);
#endif
      if(!sub_ref(s->u.dummy))
      {
	if(SUBTYPEOF(*s) == FUNCTION_BUILTIN)
	  really_free_callable(s->u.efun);
	else
	  schedule_really_free_object(s->u.object);
	assert_free_svalue (s);
      }
      s++;
    }
    return;

#undef DOTYPE
  default:
    if (type_hint & (BIT_FLOAT | BIT_INT | BIT_UNFINISHED)) {
      while(num--)
      {
#ifdef DEBUG_MALLOC
	if(REFCOUNTED_TYPE(TYPEOF(*s)))
	  debug_malloc_update_location(s->u.refs  DMALLOC_PROXY_ARGS);
#endif
	free_svalue(s++);
      }
    } else {
      while(num--)
      {
#ifdef DEBUG_MALLOC
	debug_malloc_update_location(s->u.refs  DMALLOC_PROXY_ARGS);
#endif
	if(!sub_ref(s->u.dummy))
	{
	  really_free_svalue(s);
	}
	s++;
      }
    }
    break;
  }
}

PMOD_EXPORT void debug_free_mixed_svalues(struct svalue *s, size_t num, INT32 UNUSED(type_hint) DMALLOC_LINE_ARGS)
{
  while(num--)
  {
#ifdef DEBUG_MALLOC
    if(REFCOUNTED_TYPE(TYPEOF(*s)))
      debug_malloc_update_location(s->u.refs  DMALLOC_PROXY_ARGS);
#endif
    free_svalue(s++);
  }
}


PMOD_EXPORT TYPE_FIELD assign_svalues_no_free(struct svalue *to,
					      const struct svalue *from,
					      size_t num,
					      TYPE_FIELD type_hint)
{
  TYPE_FIELD masked_type;

  check_type_hint (from, num, type_hint);
  memcpy(to, from, sizeof(struct svalue) * num);

  if (!(masked_type = (type_hint & BIT_REF_TYPES)))
    return type_hint;

  if(masked_type == type_hint)
  {
    while(num--) {
      add_ref(from->u.dummy);
      from++;
    }
    return type_hint;
  }

  type_hint = 0;
  while(num--) {
    type_hint |= 1 << TYPEOF(*from);
    if (REFCOUNTED_TYPE(TYPEOF(*from))) {
      add_ref(from->u.dummy);
    }
    from++;
  }
  return type_hint;
}

PMOD_EXPORT TYPE_FIELD assign_svalues(struct svalue *to,
				      const struct svalue *from,
				      size_t num,
				      TYPE_FIELD type_hint)
{
  check_type_hint (from, num, type_hint);
  if (type_hint & BIT_REF_TYPES) free_mixed_svalues(to,num);
  return assign_svalues_no_free(to,from,num,type_hint);
}

PMOD_EXPORT void assign_no_ref_svalue(struct svalue *to,
				      const struct svalue *val,
				      const struct object *owner)
{
  if (((TYPEOF(*to) != PIKE_T_OBJECT) &&
       (TYPEOF(*to) != PIKE_T_FUNCTION)) ||
      (to->u.object != owner)) {
    free_svalue(to);
  }
  if (val) {
    if (((TYPEOF(*val) != PIKE_T_OBJECT) &&
	 (TYPEOF(*val) != PIKE_T_FUNCTION)) ||
	(val->u.object != owner)) {
      assign_svalue_no_free(to, val);
    } else {
      *to = *val;
    }
  } else {
    SET_SVAL(*to, PIKE_T_INT, NUMBER_UNDEFINED, integer, 0);
  }
}

PMOD_EXPORT void assign_to_short_svalue(union anything *u,
			    TYPE_T type,
			    const struct svalue *s)
{
  check_svalue_type (s);
  check_refs(s);

  if(TYPEOF(*s) == type)
  {
    switch(type)
    {
      case T_INT: u->integer=s->u.integer; break;
      case T_FLOAT: u->float_number=s->u.float_number; break;
      default:
	if(u->refs && !sub_ref(u->dummy)) really_free_short_svalue(u,type);
	u->refs = s->u.refs;
	add_ref(u->dummy);
    }
  }else if(REFCOUNTED_TYPE(type) && UNSAFE_IS_ZERO(s)){
    if(u->refs && !sub_ref(u->dummy)) really_free_short_svalue(u,type);
    u->refs=0;
  }else{
    Pike_error("Wrong type in assignment, expected %s, got %s.\n",
	       get_name_of_type(type),
	       get_name_of_type(TYPEOF(*s)));
  }
}

PMOD_EXPORT void assign_to_short_svalue_no_free(union anything *u,
				    TYPE_T type,
				    const struct svalue *s)
{
  check_svalue_type (s);
  check_refs(s);

  if(TYPEOF(*s) == type)
  {
    switch(type)
    {
      case T_INT: u->integer=s->u.integer; break;
      case T_FLOAT: u->float_number=s->u.float_number; break;
      default:
	u->refs = s->u.refs;
	add_ref(u->dummy);
    }
  }else if(REFCOUNTED_TYPE(type) && UNSAFE_IS_ZERO(s)){
    u->refs=0;
  }else{
    Pike_error("Wrong type in assignment, expected %s, got %s.\n",
	       get_name_of_type(type),
	       get_name_of_type(TYPEOF(*s)));
  }
}


PMOD_EXPORT void assign_from_short_svalue_no_free(struct svalue *s,
				      const union anything *u,
				      TYPE_T type)
{
  check_type(type);
  check_refs2(u,type);

  switch(type)
  {
    case T_FLOAT:
      SET_SVAL(*s, T_FLOAT, 0, float_number, u->float_number);
      break;
    case T_INT:
      SET_SVAL(*s, T_INT, NUMBER_NUMBER, integer, u->integer);
      break;
    default:
      if(u->refs)
      {
	SET_SVAL(*s, type, 0, refs, u->refs);
	add_ref(u->dummy);
      }else{
	SET_SVAL(*s, T_INT, NUMBER_NUMBER, integer, 0);
      }
  }
}

PMOD_EXPORT void assign_short_svalue_no_free(union anything *to,
				 const union anything *from,
				 TYPE_T type)
{
  INT32 *tmp;
  check_type(type);
  check_refs2(from,type);

  switch(type)
  {
    case T_INT: to->integer=from->integer; break;
    case T_FLOAT: to->float_number=from->float_number; break;
    default:
      to->refs = tmp = from->refs;
      if(tmp) tmp[0]++;
  }
}

PMOD_EXPORT void assign_short_svalue(union anything *to,
			 const union anything *from,
			 TYPE_T type)
{
  INT32 *tmp;
  check_type(type);
  check_refs2(from,type);

  switch(type)
  {
    case T_INT: to->integer=from->integer; break;
    case T_FLOAT: to->float_number=from->float_number; break;
    default:
      if(to->refs && !sub_ref(to->dummy)) really_free_short_svalue(to,type);
      to->refs = tmp = from->refs;
      if(tmp) tmp[0]++;
  }
}

static inline size_t hash_ptr(void * ptr) {
  return (size_t)(PTR_TO_INT(ptr));
}

PMOD_EXPORT size_t hash_svalue(const struct svalue *s)
{
  size_t q;

  check_svalue_type (s);
  check_refs(s);

  switch(TYPEOF(*s))
  {
  case T_OBJECT:
    {
      struct program * p;
      int fun;

      if(!(p = s->u.object->prog))
      {
	q=0;
	break;
      }

      if((fun = FIND_LFUN(p->inherits[SUBTYPEOF(*s)].prog, LFUN___HASH)) != -1)
      {
	safe_apply_low2(s->u.object,
			fun + p->inherits[SUBTYPEOF(*s)].identifier_level,
			0, "__hash");
	if(TYPEOF(Pike_sp[-1]) == T_INT)
	{
	  q=Pike_sp[-1].u.integer;
	}else{
	  q=0;
	}
	pop_stack();
	/* do not mix the return value of __hash, since this makes using
	 * hash_value pointless */
	return q;
      }
    }
    /* FALLTHRU */
  default:
    q = hash_ptr(s->u.ptr);
    break;
  case T_FLOAT:
    /* this is true for both +0.0 and -0.0 */
    if (s->u.float_number == 0.0) {
	q = 0;
	break;
    }
#if SIZEOF_FLOAT_TYPE != SIZEOF_INT_TYPE 
    else 
    {
	union {
	    FLOAT_TYPE f;
#if SIZEOF_FLOAT_TYPE == 8
            UINT64 i;
#elif SIZEOF_FLOAT_TYPE == 4
	    unsigned INT32 i;
#elif SIZEOF_FLOAT_TYPE == 16
            UINT64 i[2];
#else
#error Size of FLOAT_TYPE not supported.
#endif
	} ufloat;
	ufloat.f = s->u.float_number;
#if SIZEOF_FLOAT_TYPE == 16
	q = (size_t)(ufloat.i[0] ^ ufloat.i[1]);
#else
	q = (size_t)ufloat.i;
#endif
	break;
    }
#endif
    /* FALLTHRU */
  case T_INT:
    q=(size_t)s->u.integer;
    break;
  case T_FUNCTION:
    if ((SUBTYPEOF(*s) != FUNCTION_BUILTIN) && s->u.object
        && s->u.object->prog == pike_trampoline_program) {
      struct pike_trampoline *tramp = get_storage(s->u.object, pike_trampoline_program);
      if (tramp) {
        q = (size_t)tramp->func ^ hash_ptr(tramp->frame);
        break;
      }
    }
    q = hash_ptr(s->u.ptr);
    break;
  }

  /*
   * This simple mixing function comes from the java HashMap implementation.
   * It makes sure that the resulting hash value can be used for hash tables
   * with power of 2 size and simple masking instead of modulo prime.
   */
  q ^= (q >> 20) ^ (q >> 12);
  return q ^ (q >> 7) ^ (q >> 4);
}

int complex_svalue_is_true( const struct svalue *s )
{
  if(TYPEOF(*s) == T_FUNCTION)
  {
    struct program *p;
    struct reference *ref;
    struct identifier *i;
    if (SUBTYPEOF(*s) == FUNCTION_BUILTIN) return 1;
    if(!(p = s->u.object->prog)) return 0;
    if (p == pike_trampoline_program) {
      /* Trampoline */
      struct pike_trampoline *tramp =
        get_storage(s->u.object, pike_trampoline_program);
      if (!tramp || !tramp->frame || !tramp->frame->current_object ||
	  !tramp->frame->current_object->prog) {
	/* Uninitialized trampoline, or trampoline to destructed object. */
	return 0;
      }
    }
    ref = PTR_FROM_INT(p, SUBTYPEOF(*s));
    i = ID_FROM_PTR(p, ref);
    if ((i->func.offset == -1) &&
	((i->identifier_flags & IDENTIFIER_TYPE_MASK) ==
	 IDENTIFIER_PIKE_FUNCTION)) {
      /* Prototype. */
      return 0;
    }
    return 1;
  }
  {
      struct program *p;
      int fun;

      if(!(p = s->u.object->prog)) return 0;

      if (SUBTYPEOF(*s)) {
	if((fun = FIND_LFUN(p->inherits[SUBTYPEOF(*s)].prog, LFUN_NOT)) == -1)
	  return 1;
      } else {
	if((fun = FIND_LFUN(p, LFUN_NOT)) == -1)
	  return 1;
      }

      apply_low(s->u.object,
                fun + p->inherits[SUBTYPEOF(*s)].identifier_level, 0);
      if(TYPEOF(Pike_sp[-1]) == T_INT && Pike_sp[-1].u.integer == 0)
      {
	  pop_stack();
	  return 1;
      } else {
	  pop_stack();
	  return 0;
      }
  }
  return 1;
}

PMOD_EXPORT int svalue_is_true(const struct svalue *s)
{
  check_svalue_type (s);
  check_refs(s);
  switch(TYPEOF(*s))
  {
  case T_FUNCTION:
  case T_OBJECT:
      return complex_svalue_is_true( s );
  case T_INT: if(!s->u.integer) return 0;
  }
  return 1;
}

PMOD_EXPORT int safe_svalue_is_true(const struct svalue *s)
{
  check_svalue_type (s);
  check_refs(s);

  switch(TYPEOF(*s))
  {
  case T_INT:
    if(s->u.integer) return 1;
    return 0;

  case T_FUNCTION:
    if (SUBTYPEOF(*s) == FUNCTION_BUILTIN) return 1;
    if(!s->u.object->prog) return 0;
    if (s->u.object->prog == pike_trampoline_program) {
      /* Trampoline */
      struct pike_trampoline *tramp =
        get_storage(s->u.object, pike_trampoline_program);
      if (!tramp || !tramp->frame || !tramp->frame->current_object ||
	  !tramp->frame->current_object->prog) {
	/* Uninitialized trampoline, or trampoline to destructed object. */
	return 0;
      }
    }
    return 1;

  case T_OBJECT:
    {
      struct program *p;
      int fun;

      if(!(p = s->u.object->prog)) return 0;

      if((fun = FIND_LFUN(p->inherits[SUBTYPEOF(*s)].prog, LFUN_NOT)) != -1)
      {
	safe_apply_low2(s->u.object,
			fun + p->inherits[SUBTYPEOF(*s)].identifier_level, 0,
			"`!");
	if(TYPEOF(Pike_sp[-1]) == T_INT && Pike_sp[-1].u.integer == 0)
	{
	  pop_stack();
	  return 1;
	} else {
	  pop_stack();
	  return 0;
	}
      }
    }
    /* FALLTHRU */

  default:
    return 1;
  }

}

#define TWO_TYPES(X,Y) (((X)<<8)|(Y))

PMOD_EXPORT int is_identical(const struct svalue *a, const struct svalue *b)
{
  if(TYPEOF(*a) != TYPEOF(*b)) return 0;
  switch(TYPEOF(*a))
  {
  case T_OBJECT:
    return (a->u.refs == b->u.refs) && (SUBTYPEOF(*a) == SUBTYPEOF(*b));

  case T_TYPE:
  case T_STRING:
  case T_MULTISET:
  case T_PROGRAM:
  case T_ARRAY:
  case T_MAPPING:
    return a->u.refs == b->u.refs;

  case T_INT:
    return a->u.integer == b->u.integer;

  case T_FUNCTION:
    return (SUBTYPEOF(*a) == SUBTYPEOF(*b) && a->u.object == b->u.object);

  case T_FLOAT:
    return a->u.float_number == b->u.float_number;

  default:
    Pike_fatal("Unknown type %x\n", TYPEOF(*a));
    UNREACHABLE();
    break;
  }

}

PMOD_EXPORT int is_eq(const struct svalue *a, const struct svalue *b)
{
  check_svalue_type (a);
  check_svalue_type (b);
  check_refs(a);
  check_refs(b);

  safe_check_destructed(a);
  safe_check_destructed(b);

  if (TYPEOF(*a) != TYPEOF(*b))
  {
    switch(TWO_TYPES((1<<TYPEOF(*a)),(1<<TYPEOF(*b))))
    {
    case TWO_TYPES(BIT_FUNCTION,BIT_PROGRAM):
      return program_from_function(a) == b->u.program;

    case TWO_TYPES(BIT_PROGRAM,BIT_FUNCTION):
      return program_from_function(b) == a->u.program;

    case TWO_TYPES(BIT_OBJECT, BIT_ARRAY):
    case TWO_TYPES(BIT_OBJECT, BIT_MAPPING):
    case TWO_TYPES(BIT_OBJECT, BIT_MULTISET):
    case TWO_TYPES(BIT_OBJECT, BIT_OBJECT):
    case TWO_TYPES(BIT_OBJECT, BIT_FUNCTION):
    case TWO_TYPES(BIT_OBJECT, BIT_PROGRAM):
    case TWO_TYPES(BIT_OBJECT, BIT_STRING):
    case TWO_TYPES(BIT_OBJECT, BIT_INT):
    case TWO_TYPES(BIT_OBJECT, BIT_FLOAT):
      {
	struct program *p;
	int fun;

      a_is_obj:
	if ((p = a->u.object->prog) &&
	    ((fun = FIND_LFUN(p->inherits[SUBTYPEOF(*a)].prog, LFUN_EQ)) != -1))
	{
	  /* FIXME: CYCLIC */
	  push_svalue(b);
	  apply_low(a->u.object,
		    fun + p->inherits[SUBTYPEOF(*a)].identifier_level, 1);
	  if(UNSAFE_IS_ZERO(Pike_sp-1))
	  {
	    pop_stack();
	    return 0;
	  }else{
	    pop_stack();
	    return 1;
	  }
	}
	if(TYPEOF(*b) != T_OBJECT) return 0;
      }

      /* FALLTHRU */

    case TWO_TYPES(BIT_ARRAY,BIT_OBJECT):
    case TWO_TYPES(BIT_MAPPING,BIT_OBJECT):
    case TWO_TYPES(BIT_MULTISET,BIT_OBJECT):
    case TWO_TYPES(BIT_FUNCTION,BIT_OBJECT):
    case TWO_TYPES(BIT_PROGRAM,BIT_OBJECT):
    case TWO_TYPES(BIT_STRING,BIT_OBJECT):
    case TWO_TYPES(BIT_INT,BIT_OBJECT):
    case TWO_TYPES(BIT_FLOAT,BIT_OBJECT):
      {
	struct program *p;
	int fun;

      b_is_obj:
	if ((p = b->u.object->prog) &&
	    ((fun = FIND_LFUN(p->inherits[SUBTYPEOF(*b)].prog, LFUN_EQ)) != -1))
	{
	  /* FIXME: CYCLIC */
	  push_svalue(a);
	  apply_low(b->u.object,
		    fun + p->inherits[SUBTYPEOF(*b)].identifier_level, 1);
	  if(UNSAFE_IS_ZERO(Pike_sp-1))
	  {
	    pop_stack();
	    return 0;
	  }else{
	    pop_stack();
	    return 1;
	  }
	}
      }
    }

    return 0;
  }
  switch(TYPEOF(*a))
  {
  case T_OBJECT:
    {
      struct program *p;

      if ((a->u.object == b->u.object) &&
	  (TYPEOF(*a) == TYPEOF(*b)) &&
	  (SUBTYPEOF(*a) == SUBTYPEOF(*b))) return 1;
      /* FIXME: What if both have lfun::`==(), and they disagree? */
      if((p = a->u.object->prog) &&
	 (FIND_LFUN(p->inherits[SUBTYPEOF(*a)].prog, LFUN_EQ) != -1))
	goto a_is_obj;

      if((p = b->u.object->prog) &&
	 (FIND_LFUN(p->inherits[SUBTYPEOF(*b)].prog, LFUN_EQ) != -1))
	goto b_is_obj;
      return 0;
    }
  case T_MULTISET:
  case T_PROGRAM:
  case T_ARRAY:
  case T_MAPPING:
    return a->u.refs == b->u.refs;

  case T_INT:
    return a->u.integer == b->u.integer;

  case T_STRING:
    return is_same_string(a->u.string,b->u.string);

  case T_TYPE:
    return a->u.type == b->u.type;

  case T_FUNCTION:
    if (SUBTYPEOF(*a) != SUBTYPEOF(*b)) return 0;
    if (a->u.object == b->u.object) return 1;
    if ((SUBTYPEOF(*a) != FUNCTION_BUILTIN) &&
	(a->u.object->prog == pike_trampoline_program) &&
	(b->u.object->prog == pike_trampoline_program)) {
      /* Trampoline. */
      struct pike_trampoline *a_tramp =
    get_storage(a->u.object, pike_trampoline_program);
      struct pike_trampoline *b_tramp =
	get_storage(b->u.object, pike_trampoline_program);
      if (a_tramp == b_tramp) return 1;
      if (!a_tramp || !b_tramp) return 0;
      /* Trampolines are equal if they are the same function,
       * and have been spawned from the same frame.
       */
      return ((a_tramp->func == b_tramp->func) &&
	      (a_tramp->frame == b_tramp->frame));
    }
    return 0;

  case T_FLOAT:
    if (PIKE_ISUNORDERED(a->u.float_number, b->u.float_number)) {
      return 0;
    }
    return a->u.float_number == b->u.float_number;

  default:
#ifdef PIKE_DEBUG
    Pike_fatal("Unknown type %x\n", TYPEOF(*a));
#endif
    UNREACHABLE();
    break;
  }
}

/* Returns 0 or 1. */
PMOD_EXPORT int low_is_equal(const struct svalue *a,
			     const struct svalue *b,
			     struct processing *proc)
{
  struct program *p;

  check_svalue_type (a);
  check_svalue_type (b);
  check_refs(a);
  check_refs(b);

  {
    int fun;

    if(TYPEOF(*a) == T_OBJECT) {
      if ((TYPEOF(*a) == TYPEOF(*b)) && (SUBTYPEOF(*a) == SUBTYPEOF(*b)) &&
	  (a->u.object == b->u.object)) return 1;

      if ((p = a->u.object->prog) &&
	  (fun = FIND_LFUN(p->inherits[SUBTYPEOF(*a)].prog, LFUN__EQUAL)) != -1)
      {
	push_svalue(b);
	apply_low(a->u.object,
		  fun + p->inherits[SUBTYPEOF(*a)].identifier_level, 1);
	if(UNSAFE_IS_ZERO(Pike_sp-1))
	{
	  pop_stack();
	  return 0;
	}else{
	  pop_stack();
	  return 1;
	}
      }
    }

    if(TYPEOF(*b) == T_OBJECT && (p = b->u.object->prog) &&
       (fun = FIND_LFUN(p->inherits[SUBTYPEOF(*b)].prog, LFUN__EQUAL)) != -1)
    {
      push_svalue(a);
      apply_low(b->u.object,
		fun + p->inherits[SUBTYPEOF(*b)].identifier_level, 1);
      if(UNSAFE_IS_ZERO(Pike_sp-1))
      {
	pop_stack();
	return 0;
      }else{
	pop_stack();
	return 1;
      }
    }
  }

  if(is_eq(a,b)) return 1;

  if(UNSAFE_IS_ZERO(a) && UNSAFE_IS_ZERO(b)) return 1;

  /* NB: Don't allow DWIM casting of objects to programs here,
   *     as that leads to unwanted matches...
   *
   * The other cases (functions, programs and types) that
   * program_from_svalue() supports are fine.
   */
  if ((TYPEOF(*a) != T_OBJECT) && (TYPEOF(*b) != T_OBJECT) &&
      (TYPEOF(*a) != T_ARRAY) && (TYPEOF(*b) != T_ARRAY) &&
      (p = program_from_svalue(a)) && (p == program_from_svalue(b))) {
    return 1;
  }

  if(TYPEOF(*a) != TYPEOF(*b)) return 0;

  switch(TYPEOF(*a))
  {
    case T_INT:
    case T_STRING:
    case T_FLOAT:
    case T_PROGRAM:
      return 0;

    case T_FUNCTION:
      {
	/* Consider functions the same if they are references to the same
	 * identifier in the same class.
	 */
	struct object *a_obj = NULL, *b_obj = NULL;
	int a_fun = SUBTYPEOF(*a), b_fun = SUBTYPEOF(*b);
	if ((a_fun == FUNCTION_BUILTIN) || (b_fun == FUNCTION_BUILTIN)) {
	  /* NB: Handled by is_eq() above. */
	  return 0;
	}
	a_obj = a->u.object;
	b_obj = b->u.object;
	if ((a_obj->prog == pike_trampoline_program) &&
	    (a_fun == QUICK_FIND_LFUN(pike_trampoline_program, LFUN_CALL))) {
	  struct pike_trampoline *a_tramp = get_storage(a_obj, pike_trampoline_program);
	  if (!a_tramp || !a_tramp->frame) return 0;
	  a_obj = a_tramp->frame->current_object;
	  a_fun = a_tramp->func;
	}
	if ((b_obj->prog == pike_trampoline_program) &&
	    (b_fun == QUICK_FIND_LFUN(pike_trampoline_program, LFUN_CALL))) {
	  struct pike_trampoline *b_tramp = get_storage(b_obj, pike_trampoline_program);
	  if (!b_tramp || !b_tramp->frame) return 0;
	  b_obj = b_tramp->frame->current_object;
	  b_fun = b_tramp->func;
	}
	if (a_obj->prog == b_obj->prog) {
	  /* Common case. */
	  if (a_fun == b_fun) return 1;
	}
	/* Consider a and b the same if they are the same identifier. */
	return ID_FROM_INT(a_obj->prog, a_fun) == ID_FROM_INT(b_obj->prog, b_fun);
      }

    case T_TYPE:
      return pike_types_le(a->u.type, b->u.type, 0, 0) &&
	pike_types_le(b->u.type, a->u.type, 0, 0);

    case T_OBJECT:
      return object_equal_p(a->u.object, b->u.object, proc);

    case T_ARRAY:
      check_array_for_destruct(a->u.array);
      check_array_for_destruct(b->u.array);
      return array_equal_p(a->u.array, b->u.array, proc);

    case T_MAPPING:
      return mapping_equal_p(a->u.mapping, b->u.mapping, proc);

    case T_MULTISET:
      return multiset_equal_p(a->u.multiset, b->u.multiset, proc);

    default:
      Pike_fatal("Unknown type in is_equal.\n");
  }
  return 1; /* survived */
}

PMOD_EXPORT int low_short_is_equal(const union anything *a,
		       const union anything *b,
		       TYPE_T type,
		       struct processing *p)
{
  struct svalue sa,sb;

  check_type(type);
  check_refs2(a,type);
  check_refs2(b,type);

  switch(type)
  {
    case T_INT: return a->integer == b->integer;
    case T_FLOAT: return a->float_number == b->float_number;
  }

  if(a->refs)
  {
    SET_SVAL(sa, type, 0, refs, a->refs);
  }else{
    SET_SVAL(sa, T_INT, NUMBER_NUMBER, integer, 0);
  }

  if(b->refs)
  {
    SET_SVAL(sb, type, 0, refs, b->refs);
  }else{
    SET_SVAL(sb, T_INT, NUMBER_NUMBER, integer, 0);
  }

  return low_is_equal(&sa,&sb,p);
}

PMOD_EXPORT int is_equal(const struct svalue *a, const struct svalue *b)
{
  return low_is_equal(a,b,0);
}

static int complex_is_lt( const struct svalue *a, const struct svalue *b );

PMOD_EXPORT int is_lt(const struct svalue *a, const struct svalue *b)
{
  check_type(TYPEOF(*a));
  check_type(TYPEOF(*b));
  check_refs(a);
  check_refs(b);

  if ((TYPEOF(*a) == TYPEOF(*b)) && (TYPEOF(*a) == T_INT)) {
    /* Common case...
     * Note: the case in the switch still needs to be kept,
     *       since a or b may be a destructed object.
     */
    return a->u.integer < b->u.integer;
  }
  return complex_is_lt( a, b );
}

static int complex_is_lt( const struct svalue *a, const struct svalue *b )
{
  safe_check_destructed(a);
  safe_check_destructed(b);

  if (TYPEOF(*a) != TYPEOF(*b))
  {
    int a_is_obj_without_lt;

    if(TYPEOF(*a) == T_FLOAT && TYPEOF(*b) == T_INT) {
#ifdef HAVE_ISLESS
      return isless(a->u.float_number, (FLOAT_TYPE)b->u.integer);
#else
      return a->u.float_number < (FLOAT_TYPE)b->u.integer;
#endif
    }

    if(TYPEOF(*a) == T_INT && TYPEOF(*b) == T_FLOAT) {
#ifdef HAVE_ISLESS
      return isless((FLOAT_TYPE)a->u.integer, b->u.float_number);
#else
      return (FLOAT_TYPE)a->u.integer < b->u.float_number;
#endif
    }

    if (((TYPEOF(*a) == T_TYPE) ||
	 (TYPEOF(*a) == T_FUNCTION) || (TYPEOF(*a) == T_PROGRAM)) &&
	((TYPEOF(*b) == T_FUNCTION) ||
	 (TYPEOF(*b) == T_PROGRAM) || (TYPEOF(*b) == T_TYPE)))
      goto compare_types;

    if(TYPEOF(*a) == T_OBJECT)
    {
      struct program *p;
      int fun;

    a_is_object:
      p = a->u.object->prog;
      if((fun = FIND_LFUN(p->inherits[SUBTYPEOF(*a)].prog, LFUN_LT)) != -1)
      {
	push_svalue(b);
	apply_low(a->u.object,
		  fun + p->inherits[SUBTYPEOF(*a)].identifier_level, 1);
	if(UNSAFE_IS_ZERO(Pike_sp-1))
	{
	  if(!SUBTYPEOF(Pike_sp[-1]))
	  {
	    pop_stack();
	    return 0;
	  }else{
	    pop_stack();
	  }
	}else{
	  pop_stack();
	  return 1;
	}
	a_is_obj_without_lt = 0;
      }
      else
	a_is_obj_without_lt = 1;
    }
    else
      a_is_obj_without_lt = 0;

    if(TYPEOF(*b) == T_OBJECT)
    {
      struct program *p;
      int fun;

      p = b->u.object->prog;

      if((fun = FIND_LFUN(p->inherits[SUBTYPEOF(*b)].prog, LFUN_GT)) == -1) {
	if (a_is_obj_without_lt)
	  Pike_error ("Object a lacks `< and object b lacks `> "
		      "in comparison on the form a < b.\n");
	else
	  Pike_error ("Object b lacks `> "
		      "in comparison on the form a < b.\n");
      }
      push_svalue(a);
      apply_low(b->u.object,
		fun + p->inherits[SUBTYPEOF(*b)].identifier_level, 1);
      if(UNSAFE_IS_ZERO(Pike_sp-1))
      {
	if(!SUBTYPEOF(Pike_sp[-1]))
	{
	  pop_stack();
	  return 0;
	}else{
	  pop_stack();
	}
      }else{
	pop_stack();
	return 1;
      }
    }

    if (a_is_obj_without_lt)
      Pike_error ("Object a lacks `< "
		  "in comparison on the form a < b.\n");
    else
      Pike_error ("Cannot compare different types.\n");
  }

  switch(TYPEOF(*a))
  {
    case T_OBJECT:
      goto a_is_object;

    default:
      Pike_error("Cannot compare values of type %s.\n",
		 get_name_of_type (TYPEOF(*a)));

    case T_INT:
      return a->u.integer < b->u.integer;

    case T_STRING:
      return my_quick_strcmp(a->u.string, b->u.string) < 0;

    case T_FLOAT:
#ifdef HAVE_ISLESS
      return isless(a->u.float_number, b->u.float_number);
#else
      if (PIKE_ISUNORDERED(a->u.float_number, b->u.float_number)) {
	return 0;
      }
      return a->u.float_number < b->u.float_number;
#endif

    case T_PROGRAM:
    case T_FUNCTION:
  compare_types:
      if (TYPEOF(*a) != T_TYPE) {
      /* Try converting a to a program, and then to a type. */
      struct svalue aa;
      int res;
      aa.u.program = program_from_svalue(a);
      if (!aa.u.program) {
	Pike_error("Bad argument to comparison.\n");
      }
      type_stack_mark();
      push_object_type(0, aa.u.program->id);
      SET_SVAL(aa, T_TYPE, 0, type, pop_unfinished_type());
      res = is_lt(&aa, b);
      free_type(aa.u.type);
      return res;
    }
      if (TYPEOF(*b) != T_TYPE) {
      /* Try converting b to a program, and then to a type. */
      struct svalue bb;
      int res;
      bb.u.program = program_from_svalue(b);
      if (!bb.u.program) {
	Pike_error("Bad argument to comparison.\n");
      }
      type_stack_mark();
      push_object_type(0, bb.u.program->id);
      SET_SVAL(bb, T_TYPE, 0, type, pop_unfinished_type());
      res = is_lt(a, &bb);
      free_type(bb.u.type);
      return res;
    }

    /* At this point both a and b have type T_TYPE */
#ifdef PIKE_DEBUG
    if ((TYPEOF(*a) != T_TYPE) || (TYPEOF(*b) != T_TYPE)) {
      Pike_fatal("Unexpected types in comparison.\n");
    }
#endif /* PIKE_DEBUG */
    /* fall through */

    case T_TYPE:
      return pike_types_le(a->u.type, b->u.type, 0, 0) &&
	!pike_types_le(b->u.type, a->u.type, 0, 0);
  }
}

PMOD_EXPORT int is_le(const struct svalue *a, const struct svalue *b)
{
  /* Can't optimize this to !is_gt (a, b) since that'd assume total order. */
  int mask = (1 << TYPEOF(*a))|(1 << TYPEOF(*b));
  if (!(mask & ~(BIT_FUNCTION|BIT_PROGRAM|BIT_TYPE))) {
    /* NOTE: Special case for types, since is_eq() below only does
     * a pointer comparison.
     */
    struct pike_type *a_type = NULL;
    struct pike_type *b_type = NULL;
    int res;
    if ((TYPEOF(*a) == TYPEOF(*b)) &&
	(a->u.ptr == b->u.ptr)) {
      if ((TYPEOF(*a) != PIKE_T_FUNCTION) ||
	  (SUBTYPEOF(*a) == SUBTYPEOF(*b))) {
	return 1;  /* eq */
      }
    }
    if (TYPEOF(*a) == T_TYPE) {
      add_ref(a_type = a->u.type);
    } else {
      struct program *p = program_from_svalue(a);
      if (p) {
	int id = p->id;
	type_stack_mark();
	push_object_type(0, id);
	a_type = pop_unfinished_type();
      } else {
	Pike_error("Bad argument to comparison.\n");
      }
    }
    if (TYPEOF(*b) == T_TYPE) {
      add_ref(b_type = b->u.type);
    } else {
      struct program *p = program_from_svalue(b);
      if (p) {
	int id = p->id;
	type_stack_mark();
	push_object_type(0, id);
	b_type = pop_unfinished_type();
      } else {
	free_type(a_type);
	Pike_error("Bad argument to comparison.\n");
      }
    }

    res = pike_types_le(a_type, b_type, 0, 0);
    free_type(a_type);
    free_type(b_type);
    return res;
  } else if (mask == (BIT_INT | BIT_FLOAT)) {
    /* Special case, since is_eq() won't promote ints to floats... */
    FLOAT_TYPE aa, bb;
    if (TYPEOF(*a) == PIKE_T_INT) {
      aa = (FLOAT_TYPE)a->u.integer;
      bb = b->u.float_number;
    } else {
      aa = a->u.float_number;
      bb = (FLOAT_TYPE)b->u.integer;
    }
    if (aa == bb) return 1;
#ifdef HAVE_ISLESS
    return isless(aa, bb);
#else
    if (PIKE_ISUNORDERED(aa, bb)) {
      return 0;
    }
    return aa < bb;
#endif
  }
  return is_lt (a, b) || is_eq (a, b);
}

static void dsv_add_string_to_buf (struct byte_buffer *buf, struct pike_string *str)
{
  int i, backslashes = 0;
  ptrdiff_t len = str->len;
  for(i=0; i < len; i++)
  {
    unsigned j = index_shared_string (str, i);
    buffer_ensure_space(buf, 16);
    if (j == '\\') {
      backslashes++;
      buffer_add_char_unsafe(buf, '\\');
    }
    else {
      if ((j == 'u' || j == 'U') && backslashes % 2) {
	/* Got a unicode escape in the input. Quote it using the
	 * double-u method to ensure unambiguousness. */
	buffer_add_char_unsafe(buf, j);
	buffer_add_char_unsafe(buf, j);
      }
      else if ((j < 256) && (isprint(j) || (j=='\n' || j=='\r')))
	buffer_add_char_unsafe(buf, j);
      else {
	if (backslashes % 2)
	  /* Got an odd number of preceding backslashes, so adding a
	   * unicode escape here would make it quoted. Have to escape
	   * the preceding backslash to avoid that. */
	  buffer_add_str_unsafe(buf, "u005c");	/* The starting backslash is already there. */
	if (j > 0xffff)
	  buffer_advance(buf, sprintf(buffer_dst(buf), "\\U%08x", j));
	else
	  buffer_advance(buf, sprintf(buffer_dst(buf), "\\u%04x", j));
      }
      backslashes = 0;
    }
  }
}

static int no_pike_calls = 0;	/* FIXME: Use TLS for this. */

/* FIXME: Ought to be rewritten to use string_builder. */
PMOD_EXPORT void describe_svalue(struct byte_buffer *buf, const struct svalue *s, int indent,
                                 struct processing *p)
{
  /* This needs to be a bit lower than LOW_C_STACK_MARGIN so that the
   * the raw error can be printed in exit_on_error. */
  check_c_stack(250);

  if (!s) {
    buffer_add_str(buf, "NULL");
    return;
  }

  check_svalue_type (s);
  check_refs(s);

  /* fprintf(stderr, "Describing svalue: %s\n", get_name_of_type(s->type)); */

  indent+=2;
  switch(TYPEOF(*s))
  {
    case T_INT:
      {
        int len = sprintf(buffer_ensure_space(buf, INT_SPRINTF_SIZE(INT_TYPE)),
                          "%"PRINTPIKEINT"d", s->u.integer);
        buffer_advance(buf, len);
        break;
      }
    case T_TYPE:
      {
	struct pike_string *t = describe_type(s->u.type);
	buffer_memcpy(buf,t->str, t->len);
	free_string(t);
      }
      break;

    case T_STRING:
      {
	struct pike_string *str = s->u.string;
	ptrdiff_t i, len = str->len;
        buffer_add_char(buf, '"');
	for(i=0; i < len; i++)
        {
	  p_wchar2 j;
          /* The longest possible escape sequences are unicode escapes,
	   * which are \U plus 8 byte hex.
	   * Note that due to the use of sprintf(3C) we also need
	   * space for the terminating NUL.
           */
          buffer_ensure_space(buf, 11);
	  switch(j = index_shared_string(str,i))
          {
	  case '\n':
	    if (i == len-1) {
	      /* String ends with a new-line. */
	      buffer_add_str_unsafe(buf, "\\n");
	    } else {
	      int e;
	      /* Add line breaks to make the output easier to read. */
	      buffer_add_str_unsafe(buf, "\\n\"\n");
	      buffer_ensure_space(buf, indent);
	      for (e = 2; e < indent; e++) {
		buffer_add_char_unsafe(buf, ' ');
	      }
	      buffer_add_char_unsafe(buf, '\"');
	    }
	    break;

	  case '\t':
	    buffer_add_char_unsafe(buf, '\\');
	    buffer_add_char_unsafe(buf, 't');
	    break;

	  case '\b':
	    buffer_add_char_unsafe(buf, '\\');
	    buffer_add_char_unsafe(buf, 'b');
	    break;

	  case '\r':
	    buffer_add_char_unsafe(buf, '\\');
	    buffer_add_char_unsafe(buf, 'r');
	    break;

	  case '\f':
	    buffer_add_char_unsafe(buf, '\\');
	    buffer_add_char_unsafe(buf, 'f');
	    break;

	  case '\a':
	    buffer_add_char_unsafe(buf, '\\');
	    buffer_add_char_unsafe(buf, 'a');
	    break;

	  case '\v':
	    buffer_add_char_unsafe(buf, '\\');
	    buffer_add_char_unsafe(buf, 'v');
	    break;

            case '"':
            case '\\':
              buffer_add_char_unsafe(buf, '\\');
              buffer_add_char_unsafe(buf, j);
	      break;

            default:
	      if((unsigned INT32) j < 256) {
		if (isprint(j)) {
		  buffer_add_char_unsafe(buf, j);

                } else {
		  /* Use octal escapes for eight bit chars since
		   * they're more compact than unicode escapes. */
		  int char_after = index_shared_string(s->u.string,i+1);
		  int len = sprintf(buffer_dst(buf), "\\%o",j);
                  buffer_advance(buf, len);
		  if (char_after >= '0' && char_after <= '9') {
		    /* Strictly speaking we don't need to do this if
		     * char_after is '8' or '9', but I guess it
		     * improves the readability a bit. */
		    buffer_add_char_unsafe(buf, '"');
		    buffer_add_char_unsafe(buf, '"');
		  }
		}
	      }

	      else {
		/* Use unicode escapes for wide chars to avoid the
		 * double quote trickery. Also, hex is easier to read
		 * than octal. */
		if (j > 0xffff)
		  buffer_advance(buf, sprintf (buffer_dst(buf), "\\U%08x", j));
		else
		  buffer_advance(buf, sprintf (buffer_dst(buf), "\\u%04x", j));
	      }
	  }
        }
        buffer_add_char(buf, '"');
      }
      break;

    case T_FUNCTION:
      if(SUBTYPEOF(*s) == FUNCTION_BUILTIN)
      {
	dsv_add_string_to_buf(buf, s->u.efun->name);
      }else{
	struct object *obj = s->u.object;
	struct program *prog = obj->prog;

	/* What's the difference between this and the trampoline stuff
	 * just below? /mast */
	if (prog == pike_trampoline_program) {
	  struct pike_trampoline *t = (struct pike_trampoline *) obj->storage;
	  if (t->frame->current_object->prog) {
	    struct svalue f;
	    SET_SVAL(f, T_FUNCTION, t->func, object, t->frame->current_object);
	    describe_svalue (buf, &f, indent, p);
	    break;
	  }
	}

	if(!prog)
	  buffer_add_char(buf, '0');
	else {
	  struct pike_string *name = NULL;
	  struct identifier *id;

	  if (prog == pike_trampoline_program) {
	    /* Trampoline */
	    struct pike_trampoline *tramp =
                get_storage(obj, pike_trampoline_program);
	    if (!tramp || !tramp->frame || !tramp->frame->current_object ||
		!tramp->frame->current_object->prog) {
	      /* Uninitialized trampoline, or
	       * trampoline to destructed object. */
	      id = NULL;
	    } else {
	      obj = tramp->frame->current_object;
	      id = ID_FROM_INT(prog, tramp->func);
	    }
	  } else {
	    id = ID_FROM_INT(prog, SUBTYPEOF(*s));
	  }
	  if (id) name = id->name;

	  if(name && (prog->flags & PROGRAM_FINISHED) &&
	     Pike_interpreter.evaluator_stack &&
	     (Pike_in_gc <= GC_PASS_PREPARE || Pike_in_gc >= GC_PASS_FREE) &&
	     master_object &&
	     !no_pike_calls) {
	    DECLARE_CYCLIC();
	    debug_malloc_touch(obj);
	    if (!BEGIN_CYCLIC(obj, 0)) {
	      /* We require some tricky coding to make this work
	       * with tracing...
	       */
	      int save_t_flag=Pike_interpreter.trace_level;

	      Pike_interpreter.trace_level=0;
	      SET_CYCLIC_RET(1);

	      ref_push_object(obj);
	      SAFE_APPLY_MASTER("describe_module", 1);

	      debug_malloc_touch(s->u.program);

	      if(!SAFE_IS_ZERO(Pike_sp-1))
		{
		  if(TYPEOF(Pike_sp[-1]) != T_STRING)
		    {
		      pop_stack();
		      push_static_text("(master returned illegal value from describe_module)");
		    }

		  Pike_interpreter.trace_level=save_t_flag;

                  dsv_add_string_to_buf(buf, Pike_sp[-1].u.string );
		  dsv_add_string_to_buf(buf, name);

		  pop_stack();
		  END_CYCLIC();
		  break;
		}

	      Pike_interpreter.trace_level=save_t_flag;
	      pop_stack();
	      prog = obj->prog;
	    }
	    END_CYCLIC();
	  }

	  if(name) {
	    dsv_add_string_to_buf(buf, name);
	    break;
	  }
	  else if (!prog) {
	    buffer_add_char(buf, '0');
	    break;
	  }
	  else if (id && id->func.offset != -1) {
	    char *file;
	    INT_TYPE line;
	    if ((file = low_get_line_plain (prog->program + id->func.offset,
					    prog, &line, 1))) {
	      buffer_add_str(buf, "function(");
	      buffer_add_str(buf, file);
	      free(file);
	      if (line) {
                buffer_advance(buf,
                               sprintf(buffer_ensure_space(buf, INT_SPRINTF_SIZE(long)+1),
                                       ":%ld", (long)line));
	      }
	      buffer_add_char(buf, ')');
	      break;
	    }
	  }

	  buffer_add_str(buf, "function");
	}
      }
      break;

    case T_OBJECT: {
      struct object *obj = s->u.object;
      struct program *prog = obj->prog;

      if (!prog)
	buffer_add_char(buf, '0');
      else {
	int describe_nicely;
	struct inherit *inh;
	prog = (inh = prog->inherits + SUBTYPEOF(*s))->prog;

	describe_nicely =
	  (prog->flags & PROGRAM_FINISHED) &&
	  Pike_interpreter.evaluator_stack &&
	  (Pike_in_gc <= GC_PASS_PREPARE || Pike_in_gc >= GC_PASS_FREE) &&
	  !no_pike_calls;
	if (describe_nicely) {
	  /* Don't call _sprintf's or other pike code when we're low
	   * on stack, since that code tends to do sprintf("%O",...)
	   * on stuff which means we usually end up here again
	   * quickly. */
	  if (low_stack_check (50)) describe_nicely = 0;
	  else low_check_c_stack (Pike_interpreter.c_stack_margin + 1000,
				  describe_nicely = 0);
	}

	if (describe_nicely) {
	  DECLARE_CYCLIC();
	  int fun=FIND_LFUN(prog, LFUN__SPRINTF);
	  debug_malloc_touch(prog);
	  if(fun != -1) {
	    if (!BEGIN_CYCLIC(obj, (ptrdiff_t)fun)) {
	      /* We require some tricky coding to make this work
	       * with tracing...
	       */
	      int save_t_flag=Pike_interpreter.trace_level;

	      Pike_interpreter.trace_level=0;
	      SET_CYCLIC_RET(1);

	      debug_malloc_touch(obj);

	      push_int('O');
	      push_constant_text("indent");
	      push_int(indent);
	      f_aggregate_mapping(2);
	      safe_apply_low2(obj, fun + inh->identifier_level, 2,
			      master_object?"_sprintf":NULL);

	      debug_malloc_touch(obj);

	      if(!SAFE_IS_ZERO(Pike_sp-1))
		{
		  if(TYPEOF(Pike_sp[-1]) != T_STRING)
		    {
		      pop_stack();
		      push_static_text("(object returned illegal value from _sprintf)");
		    }

		  Pike_interpreter.trace_level=save_t_flag;

                  dsv_add_string_to_buf(buf, Pike_sp[-1].u.string );

		  pop_stack();
		  END_CYCLIC();
		  break;
		}

	      Pike_interpreter.trace_level=save_t_flag;
	      pop_stack();
	      if (!obj->prog) prog = NULL;
	    }
	    END_CYCLIC();
	  }

	  if (!BEGIN_CYCLIC(0, obj) && master_object) {
	    /* We require some tricky coding to make this work
	     * with tracing...
	     */
	    int save_t_flag=Pike_interpreter.trace_level;

	    Pike_interpreter.trace_level=0;
	    SET_CYCLIC_RET(1);

	    debug_malloc_touch(obj);

	    ref_push_object_inherit(obj, SUBTYPEOF(*s));
	    SAFE_APPLY_MASTER("describe_object", 1);

	    debug_malloc_touch(obj);

	    if(!SAFE_IS_ZERO(Pike_sp-1))
	      {
		if(TYPEOF(Pike_sp[-1]) != T_STRING)
		  {
		    pop_stack();
		    push_static_text("(master returned illegal value from describe_object)");
		  }

		Pike_interpreter.trace_level=save_t_flag;

                dsv_add_string_to_buf(buf, Pike_sp[-1].u.string );

		pop_stack();
		END_CYCLIC();
		break;
	      }

	    Pike_interpreter.trace_level=save_t_flag;
	    pop_stack();
	    if (!obj->prog) prog = NULL;
	  }
	  END_CYCLIC();
	}

	if (!prog) {
	  buffer_add_char(buf, '0');
	  break;
	}
	else {
	  char *file;
	  INT_TYPE line;
	  if ((file = low_get_program_line_plain (prog, &line, 1))) {
	    buffer_add_str(buf, "object(");
	    buffer_add_str(buf, file);
	    free(file);
	    if (line) {
              buffer_advance(buf,
                             sprintf(buffer_ensure_space(buf, INT_SPRINTF_SIZE(long)+1),
                                     ":%ld", (long)line));
	    }
	    buffer_add_char(buf, ')');
	    break;
	  }
	}

	buffer_add_str(buf, "object");
      }
      break;
    }

    case T_PROGRAM: {
      struct program *prog = s->u.program;

      if((prog->flags & PROGRAM_FINISHED) &&
	 Pike_interpreter.evaluator_stack &&
	 (Pike_in_gc <= GC_PASS_PREPARE || Pike_in_gc >= GC_PASS_FREE) &&
	 master_object &&
	 !no_pike_calls) {
	DECLARE_CYCLIC();
	debug_malloc_touch(prog);
	if (!BEGIN_CYCLIC(prog, 0)) {
	  /* We require some tricky coding to make this work
	   * with tracing...
	   */
	  int save_t_flag=Pike_interpreter.trace_level;

	  Pike_interpreter.trace_level=0;
	  SET_CYCLIC_RET(1);

	  debug_malloc_touch(prog);

	  ref_push_program(prog);
	  SAFE_APPLY_MASTER("describe_program", 1);

	  debug_malloc_touch(prog);

	  if(!SAFE_IS_ZERO(Pike_sp-1))
	    {
	      if(TYPEOF(Pike_sp[-1]) != T_STRING)
		{
		  pop_stack();
		  push_static_text("(master returned illegal value from describe_program)");
		}

	      Pike_interpreter.trace_level=save_t_flag;

              dsv_add_string_to_buf(buf, Pike_sp[-1].u.string );

	      pop_stack();
	      END_CYCLIC();
	      break;
	    }

	  Pike_interpreter.trace_level=save_t_flag;
	  pop_stack();
	}
	END_CYCLIC();
      }

      {
	char *file;
	INT_TYPE line;
	if ((file = low_get_program_line_plain (prog, &line, 1))) {
	  buffer_add_str(buf, "program(");
	  buffer_add_str(buf, file);
	  free(file);
	  if (line) {
            buffer_advance(buf,
                           sprintf(buffer_ensure_space(buf, INT_SPRINTF_SIZE(long)+1),
                                   ":%ld", (long)line));
	  }
	  buffer_add_char(buf, ')');
	  break;
	}
      }

      buffer_add_str(buf, "program");
      break;
    }

    case T_FLOAT:
      format_pike_float (buffer_ensure_space(buf, MAX_FLOAT_SPRINTF_LEN), s->u.float_number);
      /* Advance buffer ptr until null byte */
      buffer_advance(buf, strlen(buffer_dst(buf)));
      break;

    case T_ARRAY:
      describe_array(buf, s->u.array, p, indent);
      break;

    case T_MULTISET:
      describe_multiset(buf, s->u.multiset, p, indent);
      break;

    case T_MAPPING:
      describe_mapping(buf, s->u.mapping, p, indent);
      break;

    case T_VOID:
      buffer_add_str(buf, "<void>");
      break;

    case PIKE_T_FREE:
      buffer_add_str(buf, "<free>");
      break;

    case PIKE_T_UNKNOWN:
      buffer_add_str(buf, "<unknown>");
      break;

    case T_SVALUE_PTR:
      buffer_advance(buf, sprintf(buffer_ensure_space(buf, 50), "<Svalue %p>", s->u.lval));
      break;

    case PIKE_T_ARRAY_LVALUE:
      buffer_add_str(buf, "<Array lvalue>");
      break;

    default:
      buffer_advance(buf, sprintf(buffer_ensure_space(buf, 50), "<Unknown %d>", TYPEOF(*s)));
      break;
  }
}

/* Variant of describe_svalue that never calls pike code nor releases
 * the interpreter lock. */
PMOD_EXPORT void safe_describe_svalue (struct byte_buffer *buf, const struct svalue *s,
                                       int i, struct processing *p)
{
  no_pike_calls++;
  describe_svalue (buf, s, i, p);
  no_pike_calls--;
}

PMOD_EXPORT void print_svalue (FILE *out, const struct svalue *s)
{
  struct byte_buffer str = BUFFER_INIT();
  SIZE_T off = 0;
  describe_svalue (&str, s, 0, NULL);
  while (off < buffer_content_length(&str)) {
    SIZE_T num = fwrite ((char*)buffer_ptr(&str) + off, 1, buffer_content_length(&str) - off, out);
    if (num) off += num;
    else break;
  }
  buffer_free(&str);
}

PMOD_EXPORT void safe_print_svalue (FILE *out, const struct svalue *s)
{
  no_pike_calls++;
  print_svalue (out, s);
  no_pike_calls--;
}

PMOD_EXPORT void print_short_svalue (FILE *out, const union anything *a, TYPE_T type)
{
  if (REFCOUNTED_TYPE(type) && !a->dummy)
    fputc ('0', out);
  else {
    struct svalue sval;
    SET_SVAL(sval, type, 0, ptr, a->ptr);
    sval.u = *a;
    print_svalue (out, &sval);
  }
}

PMOD_EXPORT void safe_print_short_svalue (FILE *out, const union anything *a, TYPE_T type)
{
  no_pike_calls++;
  print_short_svalue (out, a, type);
  no_pike_calls--;
}

PMOD_EXPORT void print_svalue_compact (FILE *out, const struct svalue *s)
{
  switch (TYPEOF(*s)) {
    case T_ARRAY:
      fprintf (out, "array of size %d", s->u.array->size);
      break;
    case T_MAPPING:
      fprintf (out, "mapping of size %d", m_sizeof (s->u.mapping));
      break;
    case T_MULTISET:
      fprintf (out, "multiset of size %d", multiset_sizeof (s->u.multiset));
      break;
    case T_STRING:
      if (s->u.string->len > 80) {
	struct svalue sval;
	SET_SVAL(sval, T_STRING, 0, string, string_slice(s->u.string, 0, 80));
	print_svalue (out, &sval);
	free_string (sval.u.string);
	fprintf (out, "... (%" PRINTPTRDIFFT "d chars more)",
		 s->u.string->len - 80);
	break;
      }
      /* Fall through. */
    default:
      print_svalue (out, s);
      break;
  }
}

PMOD_EXPORT void safe_print_svalue_compact (FILE *out, const struct svalue *s)
{
  no_pike_calls++;
  print_svalue_compact (out, s);
  no_pike_calls--;
}

PMOD_EXPORT void print_short_svalue_compact (FILE *out, const union anything *a, TYPE_T type)
{
  if (REFCOUNTED_TYPE(type) && !a->dummy)
    fputs ("0", out);
  else {
    struct svalue sval;
    SET_SVAL(sval, type, 0, ptr, a->ptr);
    sval.u = *a;
    print_svalue_compact (out, &sval);
  }
}

PMOD_EXPORT void safe_print_short_svalue_compact (FILE *out, const union anything *a, TYPE_T type)
{
  no_pike_calls++;
  print_short_svalue_compact (out, a, type);
  no_pike_calls--;
}

PMOD_EXPORT void pike_vfprintf (FILE *out, const char *fmt, va_list args)
{
  struct string_builder s;
  init_string_builder (&s, 0);
  string_builder_vsprintf (&s, fmt, args);
  /* FIXME: Handle wide strings. */
  fwrite(s.s->str, s.s->len, 1, out);
  free_string_builder (&s);
}

PMOD_EXPORT void pike_fprintf (FILE *out, const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  pike_vfprintf (out, fmt, args);
  va_end (args);
}

#ifdef PIKE_DEBUG
/* These are only defined in debug mode since no_pike_calls ought to
 * be controlled with a flag per format spec instead. When that's
 * fixed these will go. */

PMOD_EXPORT void safe_pike_vfprintf (FILE *out, const char *fmt, va_list args)
{
  no_pike_calls++;
  pike_vfprintf(out, fmt, args);
  no_pike_calls--;
}

PMOD_EXPORT void safe_pike_fprintf (FILE *out, const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  safe_pike_vfprintf (out, fmt, args);
  va_end (args);
}

#endif

PMOD_EXPORT void copy_svalues_recursively_no_free(struct svalue *to,
						  const struct svalue *from,
						  size_t num,
						  struct mapping *m)
{
  ONERROR err;
  int allocated_here = 0;
  while(num--)
  {
    struct svalue *tmp;
    int from_type = TYPEOF(*from);

    check_svalue_type (from);
    check_refs(from);

    if (from_type == T_ARRAY ||
	from_type == T_MAPPING ||
	from_type == T_MULTISET) {
      /* Recursive data */
      if (m && (tmp = low_mapping_lookup(m, from))) {
	*to = *tmp;
	if (REFCOUNTED_TYPE(TYPEOF(*tmp))) add_ref(tmp->u.dummy);
      } else {
#define ALLOC_DUPL_MAPPING(type_hint)				\
	do if (!m && (type_hint) & BIT_COMPLEX) {		\
	    m = allocate_mapping(num && 1);			\
	    allocated_here = 1;					\
	    SET_ONERROR(err, do_free_mapping, m);		\
	  } while (0)

	if (from_type == T_ARRAY) {
	  struct array *ar = from->u.array;
	  ALLOC_DUPL_MAPPING(ar->type_field);
	  SET_SVAL(*to, T_ARRAY, 0, array, copy_array_recursively(ar, m));
	} else if (from_type == T_MAPPING) {
	  struct mapping *ma = from->u.mapping;
	  ALLOC_DUPL_MAPPING(m_ind_types(ma) | m_val_types(ma));
	  SET_SVAL(*to, T_MAPPING, 0, mapping, copy_mapping_recursively(ma, m));
	} else {
	  struct multiset *mu = from->u.multiset;
	  ALLOC_DUPL_MAPPING(multiset_ind_types(mu) | multiset_val_types(mu));
	  SET_SVAL(*to, T_MULTISET, 0, multiset,
		   copy_multiset_recursively(mu, m));
	}
#undef ALLOC_DUPL_MAPPING
      }
    } else {
      *to = *from;
      if (REFCOUNTED_TYPE(from_type)) add_ref(from->u.array);
    }

    to++;
    from++;
  }
  if (allocated_here) {
    CALL_AND_UNSET_ONERROR(err);
  }
}

#ifdef PIKE_DEBUG

struct thorough_check_thing
{
  void *thing;
  struct thorough_check_thing *next;
};
static struct thorough_check_thing *thoroughly_checking = NULL;

void low_thorough_check_short_svalue (const union anything *u, TYPE_T type)
{
  struct thorough_check_thing checking;
  struct thorough_check_thing *chk;
  TYPE_T found_type;

  if (type == T_STRING) {
    if(!debug_findstring(u->string))
      Pike_fatal("Shared string not shared!\n");
    return;
  }

  /* Note: This fails if there are thread switches. */
  checking.thing = u->ptr;
  for (chk = thoroughly_checking; chk; chk = chk->next)
    if (chk->thing == checking.thing)
      return;
  checking.next = thoroughly_checking;
  thoroughly_checking = &checking;

  found_type = attempt_to_identify (u->ptr, NULL);

  if ((type == T_FUNCTION ? T_OBJECT : type) != found_type) {
    if (found_type == PIKE_T_UNKNOWN && u->object->next == u->object)
      {}			/* Ignore fake objects. */
    else if (found_type == T_STRUCT_CALLABLE && type == T_FUNCTION)
      {}			/* Built-in function. */
    else {
      describe (u->ptr);
      Pike_fatal ("Thing at %p should be %s but is found to be %s.\n",
		  u->ptr, get_name_of_type (type), get_name_of_type (found_type));
    }
  }

  switch (type) {
    case T_MAPPING: check_mapping(u->mapping); break;
    case T_ARRAY: check_array(u->array); break;
    case T_PROGRAM: check_program(u->program); break;
    case T_OBJECT: check_object(u->object); break;
    case T_MULTISET: check_multiset(u->multiset, 0); break;
  }

  if (thoroughly_checking != &checking)
    Pike_fatal ("Non-thread assumption doesn't hold.\n");
  thoroughly_checking = checking.next;
}

static void low_check_short_svalue(const union anything *u, TYPE_T type)
{
  check_type(type);
  if (!REFCOUNTED_TYPE(type) || (!u->refs)) return;

  switch(type)
  {
  case T_STRING:
    if(!debug_findstring(u->string))
      Pike_fatal("Shared string not shared!\n");
    break;

  default:
    if(d_flag > 50)
      low_thorough_check_short_svalue (u, type);
    break;
  }
}

void check_short_svalue(const union anything *u, TYPE_T type)
{
  if(REFCOUNTED_TYPE(type) &&
     ((PIKE_POINTER_ALIGNMENT-1) & (ptrdiff_t)(u->refs)))
    Pike_fatal("Odd pointer! type=%d u->refs=%p\n",type,u->refs);

  check_refs2(u,type);
  low_check_short_svalue(u,type);
}

PMOD_EXPORT void debug_svalue_type_error (const struct svalue *s)
{
  /* This is only called if s->type is invalid. */
  if (TYPEOF(*s) == PIKE_T_FREE || TYPEOF(*s) == PIKE_T_UNKNOWN) {
#ifdef DEBUG_MALLOC
    Pike_fatal ("Using %s freed svalue at %p.\nIt was freed at %s.\n",
		TYPEOF(*s) == PIKE_T_FREE ? "marked" : "unmarked", s, s->u.loc);
#else
    Pike_fatal ("Using %s freed svalue at %p.\n",
		TYPEOF(*s) == PIKE_T_FREE ? "marked" : "unmarked", s);
#endif
  }
  else
    Pike_fatal ("Invalid type %d in svalue at %p.\n", TYPEOF(*s), s);
}

PMOD_EXPORT int is_invalid_stack_type(unsigned int t)
{
  if (t <= PIKE_T_STACK_MAX) return 0;
  return 1;
}

PMOD_EXPORT void debug_check_svalue(const struct svalue *s)
{
  check_svalue_type (s);
  if(REFCOUNTED_TYPE(TYPEOF(*s)) &&
     ((PIKE_POINTER_ALIGNMENT-1) & (ptrdiff_t)(s->u.refs)))
    Pike_fatal("Odd pointer! type=%d u->refs=%p, align: %d\n",
	       TYPEOF(*s), s->u.refs, PIKE_POINTER_ALIGNMENT);

  if(TYPEOF(*s) == T_INT) {
    if(SUBTYPEOF(*s) != NUMBER_NUMBER &&
       SUBTYPEOF(*s) != NUMBER_UNDEFINED &&
       SUBTYPEOF(*s) != NUMBER_DESTRUCTED) {
      Pike_fatal("Unknown integer subtype %d\n", SUBTYPEOF(*s));
    }
    if (SUBTYPEOF(*s) != NUMBER_NUMBER && s->u.integer && (s->u.integer != -1))
      Pike_fatal ("Invalid subtype %d in nonzero integer.\n", SUBTYPEOF(*s));
  }

  check_refs(s);
  low_check_short_svalue(& s->u, TYPEOF(*s));
}

void debug_check_type_hint (const struct svalue *svals, size_t num, TYPE_FIELD type_hint)
{
  if(d_flag)
  {
    size_t e;
    for(e=0;e<num;e++)
      if(!(type_hint & (1 << TYPEOF(svals[e]))))
	Pike_fatal("Type hint lies (%"PRINTSIZET"d %ld %d)!\n",
		   e, (long)type_hint, TYPEOF(svals[e]));
  }
}

/* NOTE: Must handle num being negative. */
PMOD_EXPORT void real_gc_mark_external_svalues(const struct svalue *s, ptrdiff_t num,
					       const char *place)
{
  ptrdiff_t e;

  if (!s) {
    return;
  }

  for(e=0;e<num;e++,s++)
  {
#ifdef PIKE_DEBUG
    if (TYPEOF(*s) != PIKE_T_FREE)
      check_svalue((struct svalue *) s);
#endif

    gc_svalue_location=(void *)s;

    if(REFCOUNTED_TYPE(TYPEOF(*s)))
      gc_mark_external (s->u.refs, place);
  }
  gc_svalue_location=0;
}

#endif	/* PIKE_DEBUG */


/* Macro mania follows. We construct the gc 1) check, 2) mark, and 3)
 * cycle check functions on 1) svalues and 2) short svalues containing
 * 1) normal or 2) weak references. I.e. 12 very similar functions. */

#define GC_CHECK_SWITCH(U, T, ZAP, GC_DO, PRE, DO_FUNC, DO_OBJ)		\
  switch (T) {								\
    case T_FUNCTION:							\
      PRE DO_FUNC(U, T, ZAP, GC_DO) /* FALLTHRU */			\
    case T_OBJECT:							\
      PRE DO_OBJ(U, T, ZAP, GC_DO) break;				\
    case T_STRING:							\
      PRE DO_IF_DEBUG(if (d_flag) gc_check(U.string);) break;		\
    case T_PROGRAM:							\
    case T_ARRAY:							\
    case T_MULTISET:							\
    case T_MAPPING:							\
    case T_TYPE:							\
      PRE GC_DO(U.refs); break;						\
  }

#define DO_CHECK_FUNC_SVALUE(U, T, ZAP, GC_DO)				\
  if (SUBTYPEOF(*s) == FUNCTION_BUILTIN) {				\
	DO_IF_DEBUG_OR_CLEANUP (					\
	  if (!gc_check (s->u.efun)) {					\
	    DO_IF_DEBUG (if (d_flag) gc_check (s->u.efun->name));	\
	    gc_check (s->u.efun->type);					\
	  }								\
	)								\
	break;								\
      }									\
      /* Fall through to T_OBJECT. */

#define DO_FUNC_SHORT_SVALUE(U, T, ZAP, GC_DO)				\
      Pike_fatal("Cannot have a function in a short svalue.\n");

#define DO_CHECK_OBJ(U, T, ZAP, GC_DO)					\
      GC_DO(U.object);

#define DO_CHECK_OBJ_WEAK(U, T, ZAP, GC_DO)				\
      if (U.object->prog &&						\
	  !(U.object->prog->flags & PROGRAM_NO_WEAK_FREE))		\
	gc_check_weak(U.object);					\
      else								\
	gc_check(U.object);

#define NEVER_ZAP()

#define SET_SUB_SVALUE(V) SET_SVAL_SUBTYPE(*s, (V))

#define SET_SUB_SHORT_SVALUE(V)

PMOD_EXPORT void real_gc_check_svalues(const struct svalue *s, size_t num)
{
#ifdef PIKE_DEBUG
  extern void * check_for;
#endif
  size_t e;
  for(e=0;e<num;e++,s++)
  {
#ifdef PIKE_DEBUG
    if (TYPEOF(*s) != PIKE_T_FREE)
      check_svalue((struct svalue *) s);
    gc_svalue_location=(void *)s;
#endif
    GC_CHECK_SWITCH((s->u), TYPEOF(*s), NEVER_ZAP, gc_check,
		    {}, DO_CHECK_FUNC_SVALUE,
		    DO_CHECK_OBJ);
  }
#ifdef PIKE_DEBUG
  gc_svalue_location=0;
#endif
}

void gc_check_weak_svalues(const struct svalue *s, size_t num)
{
#ifdef PIKE_DEBUG
  extern void * check_for;
#endif
  size_t e;
  for(e=0;e<num;e++,s++)
  {
#ifdef PIKE_DEBUG
    if (TYPEOF(*s) != PIKE_T_FREE)
      check_svalue((struct svalue *) s);
    gc_svalue_location=(void *)s;
#endif
    GC_CHECK_SWITCH((s->u), TYPEOF(*s), NEVER_ZAP, gc_check_weak,
		    {}, DO_CHECK_FUNC_SVALUE,
		    DO_CHECK_OBJ_WEAK);
  }
#ifdef PIKE_DEBUG
  gc_svalue_location=0;
#endif
}

PMOD_EXPORT void real_gc_check_short_svalue(const union anything *u, TYPE_T type)
{
#ifdef PIKE_DEBUG
  extern void * check_for;
  gc_svalue_location=(void *)u;
#endif
  debug_malloc_touch((void *) u);
  GC_CHECK_SWITCH((*u), type, NEVER_ZAP, gc_check,
		  {if (!u->refs) return;}, DO_FUNC_SHORT_SVALUE,
		  DO_CHECK_OBJ);
#ifdef PIKE_DEBUG
  gc_svalue_location=0;
#endif
}

void gc_check_weak_short_svalue(const union anything *u, TYPE_T type)
{
#ifdef PIKE_DEBUG
  extern void * check_for;
  gc_svalue_location=(void *)u;
#endif
  debug_malloc_touch((void *) u);
  GC_CHECK_SWITCH((*u), type, NEVER_ZAP, gc_check_weak,
		  {if (!u->refs) return;}, DO_FUNC_SHORT_SVALUE,
		  DO_CHECK_OBJ_WEAK);
#ifdef PIKE_DEBUG
  gc_svalue_location=0;
#endif
}

#define ZAP_SVALUE()							\
      do {								\
	gc_free_svalue(s);						\
	SET_SVAL(*s, T_INT, NUMBER_DESTRUCTED, integer, 0);		\
      } while (0)

#define ZAP_SHORT_SVALUE()						\
      do {								\
	gc_free_short_svalue(u, type);					\
	u->refs = 0;							\
      } while (0)

#define GC_RECURSE_SWITCH(U, T, ZAP, FREE_WEAK, GC_DO, PRE,		\
			  DO_FUNC, DO_OBJ, DO_STR, DO_TYPE, DO_SKIP)	\
  switch (T) {								\
    case T_FUNCTION:							\
      PRE DO_FUNC(U, T, ZAP, GC_DO) /* FALLTHRU */			\
    case T_OBJECT:							\
      PRE								\
      if (!U.object->prog) {						\
	ZAP();								\
	freed = 1;							\
	break;								\
      }									\
      FREE_WEAK(U, T, ZAP) DO_OBJ(U, object);				\
      break;								\
    case T_STRING:							\
      DO_STR(U); break;							\
    case T_PROGRAM:							\
      PRE FREE_WEAK(U, T, ZAP) GC_DO(U, program); break;		\
    case T_ARRAY:							\
      PRE FREE_WEAK(U, T, ZAP) GC_DO(U, array); break;			\
    case T_MULTISET:							\
      PRE FREE_WEAK(U, T, ZAP) GC_DO(U, multiset); break;		\
    case T_MAPPING:							\
      PRE FREE_WEAK(U, T, ZAP) GC_DO(U, mapping); break;		\
    DO_IF_DEBUG_OR_CLEANUP (						\
    case T_TYPE:							\
      PRE DO_TYPE(U, type); break;					\
    );									\
    case PIKE_T_FREE:							\
      /* Skip to next svalue. Typically continue or break. */		\
      DO_SKIP;								\
  }

#define DONT_FREE_WEAK(U, T, ZAP)

#define FREE_WEAK(U, T, ZAP)						\
      if (gc_do_weak_free(U.refs)) {					\
	ZAP();								\
	freed = 1;							\
	break;								\
      }

#define GC_DO_MARK(U, TN)						\
      gc_mark_enqueue((queue_call) PIKE_CONCAT3(gc_mark_, TN, _as_referenced), \
		      U.TN)

#define GC_DONT_MARK(U, TN) do {} while (0)

#define MARK_PRE {							\
    DO_IF_DEBUG (							\
      if (!s->u.refs)							\
	(gc_fatal_2 (s->u.ptr, TYPEOF(*s),				\
		     0, "Marking thing without refs.\n"));		\
    );									\
  }

#define DO_MARK_FUNC_SVALUE(U, T, ZAP, GC_DO)				\
      if (SUBTYPEOF(*s) == FUNCTION_BUILTIN) {				\
	DO_IF_DEBUG (if (d_flag) gc_mark (s->u.efun->name, T_STRING));	\
	DO_IF_DEBUG_OR_CLEANUP (GC_DO_MARK ((*s->u.efun), type));	\
	break;								\
      }									\
      /* Fall through to T_OBJECT. */

#define DONT_MARK_FUNC_SVALUE(U, T, ZAP, GC_DO)				\
      if (SUBTYPEOF(*s) == FUNCTION_BUILTIN)				\
	break;								\
      /* Fall through to T_OBJECT. */

#define DO_MARK_OBJ_WEAK(U, TN)						\
      if (U.object->prog &&						\
	  (U.object->prog->flags & PROGRAM_NO_WEAK_FREE))		\
	GC_DO_MARK(U, TN)

#define DO_MARK_STRING(U)						\
      DO_IF_DEBUG(if (U.refs && d_flag) gc_mark(U.string, T_STRING))

#define DONT_MARK_STRING(U)

PMOD_EXPORT TYPE_FIELD real_gc_mark_svalues(struct svalue *s, size_t num)
{
  TYPE_FIELD t = 0;
  int freed = 0;
  size_t e;
  for(e=0;e<num;e++,s++)
  {
    dmalloc_touch_svalue(s);
    GC_RECURSE_SWITCH((s->u), TYPEOF(*s), ZAP_SVALUE, DONT_FREE_WEAK,
		      GC_DO_MARK, MARK_PRE,
		      DO_MARK_FUNC_SVALUE, GC_DO_MARK,
		      DO_MARK_STRING, GC_DO_MARK, continue);
    t |= BITOF(*s);
    if (!(t & BIT_INT) && (TYPEOF(*s) == PIKE_T_OBJECT) &&
	(s->u.object->prog == bignum_program)) {
      /* Lie, and claim that the array contains integers too. */
      t |= BIT_INT;
    }
  }
  return freed ? t : 0;
}

TYPE_FIELD gc_mark_weak_svalues(struct svalue *s, size_t num)
{
  TYPE_FIELD t = 0;
  int freed = 0;
  size_t e;
  for(e=0;e<num;e++,s++)
  {
    dmalloc_touch_svalue(s);
    GC_RECURSE_SWITCH((s->u), TYPEOF(*s), ZAP_SVALUE, FREE_WEAK,
		      GC_DONT_MARK, MARK_PRE,
		      DO_MARK_FUNC_SVALUE, DO_MARK_OBJ_WEAK,
		      DO_MARK_STRING, GC_DO_MARK, continue);
    t |= BITOF(*s);
    if (!(t & BIT_INT) && (TYPEOF(*s) == PIKE_T_OBJECT) &&
	(s->u.object->prog == bignum_program)) {
      /* Lie, and claim that the array contains integers too. */
      t |= BIT_INT;
    }
  }
  return freed ? t : 0;
}

int real_gc_mark_short_svalue(union anything *u, TYPE_T type)
{
  int freed = 0;
  do {
    debug_malloc_touch(u);
    GC_RECURSE_SWITCH((*u), type, ZAP_SHORT_SVALUE, DONT_FREE_WEAK,
		      GC_DO_MARK, {if (!u->refs) return 0;},
		      DO_FUNC_SHORT_SVALUE, GC_DO_MARK,
		      DO_MARK_STRING, GC_DO_MARK, break);
  } while (0);
  return freed;
}

int gc_mark_weak_short_svalue(union anything *u, TYPE_T type)
{
  int freed = 0;
  do {
    debug_malloc_touch(u);
    GC_RECURSE_SWITCH((*u), type, ZAP_SHORT_SVALUE, FREE_WEAK,
		      GC_DONT_MARK, {if (!u->refs) return 0;},
		      DO_FUNC_SHORT_SVALUE, DO_MARK_OBJ_WEAK,
		      DO_MARK_STRING, GC_DO_MARK, break);
  } while (0);
  return freed;
}

int gc_mark_without_recurse(struct svalue *s)
{
  int freed = 0;
  do {
    dmalloc_touch_svalue(s);
    GC_RECURSE_SWITCH((s->u), TYPEOF(*s), ZAP_SVALUE, DONT_FREE_WEAK,
		      GC_DONT_MARK, MARK_PRE,
		      DONT_MARK_FUNC_SVALUE, GC_DONT_MARK,
		      DONT_MARK_STRING, GC_DONT_MARK, break);
  } while (0);
  return freed;
}

int gc_mark_weak_without_recurse(struct svalue *s)
{
  int freed = 0;
  do {
    dmalloc_touch_svalue(s);
    GC_RECURSE_SWITCH((s->u), TYPEOF(*s), ZAP_SVALUE, FREE_WEAK,
		      GC_DONT_MARK, MARK_PRE,
		      DONT_MARK_FUNC_SVALUE, GC_DONT_MARK,
		      DONT_MARK_STRING, GC_DONT_MARK, break);
  } while (0);
  return freed;
}

#define DO_CYCLE_CHECK_FUNC_SVALUE(U, T, ZAP, GC_DO)			\
      if (SUBTYPEOF(*s) == FUNCTION_BUILTIN) break;			\
      /* Fall through to T_OBJECT. */

#define DONT_CYCLE_CHECK_STRING(U)

#define DO_CYCLE_CHECK(U, TN) PIKE_CONCAT(gc_cycle_check_, TN)(U.TN, 0)
#define DO_CYCLE_CHECK_WEAK(U, TN) PIKE_CONCAT(gc_cycle_check_, TN)(U.TN, 1)
#define DONT_CYCLE_CHECK(U, TN)

PMOD_EXPORT TYPE_FIELD real_gc_cycle_check_svalues(struct svalue *s, size_t num)
{
  TYPE_FIELD t = 0;
  int freed = 0;
  size_t e;
  for(e=0;e<num;e++,s++)
  {
    dmalloc_touch_svalue(s);
    GC_RECURSE_SWITCH((s->u), TYPEOF(*s), ZAP_SVALUE, DONT_FREE_WEAK,
		      DO_CYCLE_CHECK, {},
		      DO_CYCLE_CHECK_FUNC_SVALUE, DO_CYCLE_CHECK,
		      DONT_CYCLE_CHECK_STRING, DONT_CYCLE_CHECK, continue);
    t |= BITOF(*s);
    if (!(t & BIT_INT) && (TYPEOF(*s) == PIKE_T_OBJECT) &&
	(s->u.object->prog == bignum_program)) {
      /* Lie, and claim that the array contains integers too. */
      t |= BIT_INT;
    }
  }
  return freed ? t : 0;
}

TYPE_FIELD gc_cycle_check_weak_svalues(struct svalue *s, size_t num)
{
  TYPE_FIELD t = 0;
  int freed = 0;
  size_t e;
  for(e=0;e<num;e++,s++)
  {
    dmalloc_touch_svalue(s);
    GC_RECURSE_SWITCH((s->u), TYPEOF(*s), ZAP_SVALUE, DONT_FREE_WEAK,
		      DO_CYCLE_CHECK_WEAK, {},
		      DO_CYCLE_CHECK_FUNC_SVALUE, DO_CYCLE_CHECK_WEAK,
		      DONT_CYCLE_CHECK_STRING, DONT_CYCLE_CHECK, continue);
    t |= BITOF(*s);
    if (!(t & BIT_INT) && (TYPEOF(*s) == PIKE_T_OBJECT) &&
	(s->u.object->prog == bignum_program)) {
      /* Lie, and claim that the array contains integers too. */
      t |= BIT_INT;
    }
  }
  return freed ? t : 0;
}

PMOD_EXPORT int real_gc_cycle_check_short_svalue(union anything *u, TYPE_T type)
{
  int freed = 0;
  do {
    debug_malloc_touch(u);
    GC_RECURSE_SWITCH((*u), type, ZAP_SHORT_SVALUE, DONT_FREE_WEAK,
		      DO_CYCLE_CHECK, {if (!u->refs) return 0;},
		      DO_FUNC_SHORT_SVALUE, DO_CYCLE_CHECK,
		      DONT_CYCLE_CHECK_STRING, DONT_CYCLE_CHECK, break);
  } while (0);
  return freed;
}

int gc_cycle_check_weak_short_svalue(union anything *u, TYPE_T type)
{
  int freed = 0;
  do {
    debug_malloc_touch(u);
    GC_RECURSE_SWITCH((*u), type, ZAP_SHORT_SVALUE, DONT_FREE_WEAK,
		      DO_CYCLE_CHECK_WEAK, {if (!u->refs) return 0;},
		      DO_FUNC_SHORT_SVALUE, DO_CYCLE_CHECK_WEAK,
		      DONT_CYCLE_CHECK_STRING, DONT_CYCLE_CHECK, break);
  } while (0);
  return freed;
}

/* gc_free_svalue() and gc_free_short_svalue() can be used to free
 * things in general during gc mark and cycle check passes, where
 * normal freeing is prohibited. If the thing runs out of refs, they
 * record them so that they're freed in the free pass along with the
 * rest. If the gc isn't running, they behave just like free_svalue
 * and free_short_svalue.
 *
 * Note that the gc will bug out if these are used on references that
 * have been accounted for by the recursing gc mark or cycle check
 * functions above. */

void real_gc_free_svalue(struct svalue *s)
{
  if (Pike_in_gc > GC_PASS_PREPARE && Pike_in_gc < GC_PASS_FREE) {
#ifdef PIKE_DEBUG
    if (Pike_in_gc != GC_PASS_MARK && Pike_in_gc != GC_PASS_CYCLE &&
	Pike_in_gc != GC_PASS_ZAP_WEAK)
      Pike_fatal("gc_free_svalue() called in invalid gc pass.\n");
#endif
    if (REFCOUNTED_TYPE(TYPEOF(*s)) && TYPEOF(*s) <= MAX_COMPLEX &&
	*(s->u.refs) == 1)
      gc_delayed_free(s->u.refs, TYPEOF(*s));
  }
  free_svalue(s);
}

void real_gc_free_short_svalue(union anything *u, TYPE_T type)
{
  if (Pike_in_gc > GC_PASS_PREPARE && Pike_in_gc < GC_PASS_FREE) {
#ifdef PIKE_DEBUG
    if (Pike_in_gc != GC_PASS_MARK && Pike_in_gc != GC_PASS_CYCLE &&
	Pike_in_gc != GC_PASS_ZAP_WEAK)
      Pike_fatal("gc_free_short_svalue() called in invalid gc pass.\n");
#endif
    if (REFCOUNTED_TYPE(type) && type <= MAX_COMPLEX && *u->refs == 1)
      gc_delayed_free(u->refs, type);
  }
  free_short_svalue(u, type);
}

PMOD_EXPORT INT_TYPE pike_sizeof(const struct svalue *s)
{
  switch(TYPEOF(*s))
  {
  case T_STRING:
    return (INT32)s->u.string->len;
  case T_ARRAY: return s->u.array->size;
  case T_MAPPING: return m_sizeof(s->u.mapping);
  case T_MULTISET: return l_sizeof(s->u.multiset);
  case T_OBJECT:
    {
      struct program *p;
      int fun;

      if(!(p = s->u.object->prog))
	Pike_error("sizeof() on destructed object.\n");
      if((fun = FIND_LFUN(p->inherits[SUBTYPEOF(*s)].prog, LFUN__SIZEOF)) == -1)
      {
	return p->inherits[SUBTYPEOF(*s)].prog->num_identifier_index;
      }else{
	apply_low(s->u.object,
		  fun + p->inherits[SUBTYPEOF(*s)].identifier_level, 0);
	if(TYPEOF(Pike_sp[-1]) != T_INT)
	  Pike_error("Bad return type from o->_sizeof() (not int)\n");
	dmalloc_touch_svalue(Pike_sp-1);
	Pike_sp--;
	return Pike_sp->u.integer;
      }
    }
  default:
    Pike_error("Bad argument 1 to sizeof().\n");
  }
}

int svalues_are_constant(const struct svalue *s,
			 INT32 num,
			 TYPE_FIELD hint,
			 struct processing *p)
{
  check_type_hint (s, num, hint);
  if(hint & ~(BIT_STRING | BIT_INT | BIT_FLOAT))
  {
    INT32 e;
    for(e=0;e<num;e++)
    {
      switch(TYPEOF(*s))
      {
	case T_ARRAY:
	case T_MAPPING:
	case T_MULTISET:
	{
	  struct processing curr;
	  curr.pointer_a = s->u.refs;
	  curr.next = p;

	  for( ;p ;p=p->next)
	    if(p->pointer_a == (void *)s->u.refs)
	      return 1;

	  switch(TYPEOF(*s))
	  {
	    case T_ARRAY:
	      if(!array_is_constant(s->u.array,&curr))
		return 0;
	      break;

	    case T_MAPPING:
	      if(!mapping_is_constant(s->u.mapping,&curr))
		return 0;
	      break;

	    case T_MULTISET:
	      if(!multiset_is_constant(s->u.multiset,&curr))
		return 0;
	      break;
	  }
          break;
	}

	case T_FUNCTION:
	  if(SUBTYPEOF(*s) == FUNCTION_BUILTIN) continue;
	  /* Fall through */

	case T_OBJECT:
	  if(s->u.object -> next == s->u.object)
	  {
	    /* This is a fake object used during the
	     * compilation!
	     */
	    return 0;
	  }
      }
      s++;
    }
  }
  return 1;
}
