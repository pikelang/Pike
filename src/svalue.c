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
#include "dynamic_buffer.h"
#include "interpret.h"
#include "gc.h"
#include "pike_macros.h"
#include "pike_types.h"
#include <ctype.h>
#include "queue.h"
#include "bignum.h"
#include "cyclic.h"
#include "pike_float.h"
#include <math.h>

#define sp Pike_sp

PMOD_EXPORT const struct svalue svalue_undefined =
  SVALUE_INIT (T_INT, NUMBER_UNDEFINED, 0);
PMOD_EXPORT const struct svalue svalue_int_zero = SVALUE_INIT_INT (0);
PMOD_EXPORT
#ifdef HAVE_UNION_INIT
const
#endif
struct svalue svalue_int_one = SVALUE_INIT_INT (1);

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
  MEMCPY((char *)to, (char *)from, sizeof(struct svalue) * num);

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

PMOD_EXPORT unsigned INT32 hash_svalue(const struct svalue *s)
{
  unsigned INT32 q;

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
	STACK_LEVEL_START(0);
	safe_apply_low2(s->u.object,
			fun + p->inherits[SUBTYPEOF(*s)].identifier_level,
			0, "__hash");
	STACK_LEVEL_CHECK(1);
	if(TYPEOF(sp[-1]) == T_INT)
	{
	  q=sp[-1].u.integer;
	}else{
	  q=0;
	}
	pop_stack();
	STACK_LEVEL_DONE(0);
	/* do not mix the return value of __hash, since this makes using
	 * hash_value pointless */
	return q;
      }
    }
    /* FALL THROUGH */
  default:
#if SIZEOF_CHAR_P > 4
    q=DO_NOT_WARN((unsigned INT32)(PTR_TO_INT(s->u.ptr) >> 2));
#else
    q=DO_NOT_WARN((unsigned INT32)(PTR_TO_INT(s->u.ptr)));
#endif
    break;
  case T_INT:
    q=(unsigned INT32) s->u.integer;
#if SIZEOF_INT_TYPE == 8
    q^=(unsigned INT32)(s->u.integer >> 32);
#endif
    break;
  case T_FLOAT:
    /* this is true for both +0.0 and -0.0 */
    if (s->u.float_number == 0.0) {
	q = 0;
    } else {
	union {
	    FLOAT_TYPE f;
#if SIZEOF_FLOAT_TYPE == 8
	    unsigned INT64 i;
#elif SIZEOF_FLOAT_TYPE == 4
	    unsigned INT32 i;
#else
#error Size of FLOAT_TYPE not supported.
#endif
	} ufloat;
	ufloat.f = s->u.float_number;
	q = (unsigned INT32)ufloat.i;
#if SIZEOF_FLOAT_TYPE == 8
	q ^= (unsigned INT32)(ufloat.i >> 32);
#endif
    }
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
    if (SUBTYPEOF(*s) == FUNCTION_BUILTIN) return 1;
    if(!s->u.object->prog) return 0;
    if (s->u.object->prog == pike_trampoline_program) {
      /* Trampoline */
      struct pike_trampoline *tramp = (struct pike_trampoline *)
	get_storage(s->u.object, pike_trampoline_program);
      if (!tramp || !tramp->frame || !tramp->frame->current_object ||
	  !tramp->frame->current_object->prog) {
	/* Uninitialized trampoline, or trampoline to destructed object. */
	return 0;
      }
    } else {
#if 0
      /* We should never get a function svalue for a prototype. */
      struct identifier *i = ID_FROM_INT(s->u.object->prog, SUBTYPEOF(*s));
      if (IDENTIFIER_IS_PIKE_FUNCTION(i->identifier_flags) &&
	  (i->func.offset == -1)) {
	/* Prototype. */
	return 0;
      }
#endif
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
      if(TYPEOF(sp[-1]) == T_INT && sp[-1].u.integer == 0)
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
      struct pike_trampoline *tramp = (struct pike_trampoline *)
	get_storage(s->u.object, pike_trampoline_program);
      if (!tramp || !tramp->frame || !tramp->frame->current_object ||
	  !tramp->frame->current_object->prog) {
	/* Uninitialized trampoline, or trampoline to destructed object. */
	return 0;
      }
    } else {
#if 0
      /* We should never get a function svalue for a prototype. */
      struct identifier *i = ID_FROM_INT(s->u.object->prog, SUBTYPEOF(*s));
      if (IDENTIFIER_IS_PIKE_FUNCTION(i->identifier_flags) &&
	  (i->func.offset == -1)) {
	/* Prototype. */
	return 0;
      }
#endif
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
	if(TYPEOF(sp[-1]) == T_INT && sp[-1].u.integer == 0)
	{
	  pop_stack();
	  return 1;
	} else {
	  pop_stack();
	  return 0;
	}
      }
    }

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
    return 0; /* make gcc happy */
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
	  if(UNSAFE_IS_ZERO(sp-1))
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
	  if(UNSAFE_IS_ZERO(sp-1))
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
      struct pike_trampoline *a_tramp = (struct pike_trampoline *)
	get_storage(a->u.object, pike_trampoline_program);
      struct pike_trampoline *b_tramp = (struct pike_trampoline *)
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
    return 0; /* make gcc happy */
  }
}

/* Returns 0 or 1. */
PMOD_EXPORT int low_is_equal(const struct svalue *a,
			     const struct svalue *b,
			     struct processing *proc)
{
  check_svalue_type (a);
  check_svalue_type (b);
  check_refs(a);
  check_refs(b);

  {
    struct program *p;
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
	if(UNSAFE_IS_ZERO(sp-1))
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
      if(UNSAFE_IS_ZERO(sp-1))
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

  if(TYPEOF(*a) != TYPEOF(*b)) return 0;

  switch(TYPEOF(*a))
  {
    case T_INT:
    case T_STRING:
    case T_FLOAT:
    case T_FUNCTION:
    case T_PROGRAM:
      return 0;

    case T_TYPE:
      return pike_types_le(a->u.type, b->u.type) &&
	pike_types_le(b->u.type, a->u.type);

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
#if 0
      /* safe_check_destructed should avoid this. */
      if(!a->u.object->prog)
	Pike_error("Comparison on destructed object.\n");
#endif
      p = a->u.object->prog;
      if((fun = FIND_LFUN(p->inherits[SUBTYPEOF(*a)].prog, LFUN_LT)) != -1)
      {
	push_svalue(b);
	apply_low(a->u.object,
		  fun + p->inherits[SUBTYPEOF(*a)].identifier_level, 1);
	if(UNSAFE_IS_ZERO(sp-1))
	{
	  if(!SUBTYPEOF(sp[-1]))
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

#if 0
      /* safe_check_destructed should avoid this. */
      if(!b->u.object->prog)
	Pike_error("Comparison on destructed object.\n");
#endif
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
      if(UNSAFE_IS_ZERO(sp-1))
      {
	if(!SUBTYPEOF(sp[-1]))
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
      return pike_types_le(a->u.type, b->u.type) &&
	!pike_types_le(b->u.type, a->u.type);
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
	(a->u.ptr == b->u.ptr)) return 1;  /* eq */
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
    
    res = pike_types_le(a_type, b_type);
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

static void dsv_add_string_to_buf (struct pike_string *str)
{
  int i, backslashes = 0;
  ptrdiff_t len = str->len;
  for(i=0; i < len; i++)
  {
    unsigned j = index_shared_string (str, i);
    if (j == '\\') {
      backslashes++;
      my_putchar ('\\');
    }
    else {
      if ((j == 'u' || j == 'U') && backslashes % 2) {
	/* Got a unicode escape in the input. Quote it using the
	 * double-u method to ensure unambiguousness. */
	my_putchar (j);
	my_putchar (j);
      }
      else if ((j < 256) && (isprint(j) || (j=='\n' || j=='\r')))
	my_putchar (j);
      else {
	char buf[11];
	if (backslashes % 2)
	  /* Got an odd number of preceding backslashes, so adding a
	   * unicode escape here would make it quoted. Have to escape
	   * the preceding backslash to avoid that. */
	  my_strcat ("u005c");	/* The starting backslash is already there. */
	if (j > 0xffff)
	  sprintf (buf, "\\U%08x", j);
	else
	  sprintf (buf, "\\u%04x", j);
	my_strcat (buf);
      }
      backslashes = 0;
    }
  }
}

static int no_pike_calls = 0;	/* FIXME: Use TLS for this. */

/* FIXME: Ought to be rewritten to use string_builder.
 * FIXME: Ought not to have global state.
 */
PMOD_EXPORT void describe_svalue(const struct svalue *s,int indent,struct processing *p)
{
  char buf[MAXIMUM (50, MAX_FLOAT_SPRINTF_LEN)];

  /* This needs to be a bit lower than LOW_C_STACK_MARGIN so that the
   * the raw error can be printed in exit_on_error. */
  check_c_stack(250);

  check_svalue_type (s);
  check_refs(s);

  /* fprintf(stderr, "Describing svalue: %s\n", get_name_of_type(s->type)); */
    
  indent+=2;
  switch(TYPEOF(*s))
  {
    case T_INT:
      sprintf(buf,"%"PRINTPIKEINT"d",s->u.integer);
      my_strcat(buf);
      break;

    case T_TYPE:
      my_describe_type(s->u.type);
      break;

    case T_STRING:
      {
	struct pike_string *str = s->u.string;
	int i, len = str->len;
	my_putchar('"');
	for(i=0; i < len; i++)
        {
	  p_wchar2 j;
	  switch(j = index_shared_string(str,i))
          {
	  case '\n':
	    if (i == len-1) {
	      /* String ends with a new-line. */
	      my_strcat("\\n");
	    } else {
	      int e;
	      /* Add line breaks to make the output easier to read. */
	      my_strcat("\\n\"\n");
	      for (e = 2; e < indent; e++) {
		my_putchar(' ');
	      }
	      my_putchar('\"');
	    }
	    break;

	  case '\t':
	    my_putchar('\\');
	    my_putchar('t');
	    break;

	  case '\b':
	    my_putchar('\\');
	    my_putchar('b');
	    break;

	  case '\r':
	    my_putchar('\\');
	    my_putchar('r');
	    break;

	  case '\f':
	    my_putchar('\\');
	    my_putchar('f');
	    break;

	  case '\a':
	    my_putchar('\\');
	    my_putchar('a');
	    break;

	  case '\v':
	    my_putchar('\\');
	    my_putchar('v');
	    break;

            case '"':
            case '\\':
              my_putchar('\\');
              my_putchar(j);
	      break;

            default:
	      if((unsigned INT32) j < 256) {
		if (isprint(j))
		  my_putchar(j);

		else {
		  /* Use octal escapes for eight bit chars since
		   * they're more compact than unicode escapes. */
		  int char_after = index_shared_string(s->u.string,i+1);
		  sprintf(buf,"\\%o",j);
		  my_strcat(buf);
		  if (char_after >= '0' && char_after <= '9') {
		    /* Strictly speaking we don't need to do this if
		     * char_after is '8' or '9', but I guess it
		     * improves the readability a bit. */
		    my_putchar('"');
		    my_putchar('"');
		  }
		}
	      }

	      else {
		/* Use unicode escapes for wide chars to avoid the
		 * double quote trickery. Also, hex is easier to read
		 * than octal. */
		if (j > 0xffff)
		  sprintf (buf, "\\U%08x", j);
		else
		  sprintf (buf, "\\u%04x", j);
		my_strcat (buf);
	      }
	  }
        }
        my_putchar('"');
      }
      break;

    case T_FUNCTION:
      if(SUBTYPEOF(*s) == FUNCTION_BUILTIN)
      {
	dsv_add_string_to_buf(s->u.efun->name);
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
	    describe_svalue (&f, indent, p);
	    break;
	  }
	}

	if(!prog)
	  my_strcat("0");
	else {
	  struct pike_string *name = NULL;
	  struct identifier *id;

	  if (prog == pike_trampoline_program) {
	    /* Trampoline */
	    struct pike_trampoline *tramp = (struct pike_trampoline *)
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
	      dynamic_buffer save_buf;
	      save_buffer (&save_buf);

	      Pike_interpreter.trace_level=0;
	      SET_CYCLIC_RET(1);
	    
	      ref_push_object(obj);
	      SAFE_APPLY_MASTER("describe_module", 1);
	    
	      debug_malloc_touch(s->u.program);
	    
	      if(!SAFE_IS_ZERO(sp-1))
		{
		  if(TYPEOF(sp[-1]) != T_STRING)
		    {
		      pop_stack();
		      push_text("(master returned illegal value from describe_module)");
		    }
	      
		  restore_buffer (&save_buf);
		  Pike_interpreter.trace_level=save_t_flag;
		
		  dsv_add_string_to_buf( sp[-1].u.string );
		  dsv_add_string_to_buf(name);

		  pop_stack();
		  END_CYCLIC();
		  break;
		}

	      restore_buffer (&save_buf);
	      Pike_interpreter.trace_level=save_t_flag;
	      pop_stack();
	      prog = obj->prog;
	    }
	    END_CYCLIC();
	  }

	  if(name) {
	    dsv_add_string_to_buf(name);
	    break;
	  }
	  else if (!prog) {
	    my_strcat("0");
	    break;
	  }
	  else if (id && id->func.offset != -1) {
	    char *file;
	    INT_TYPE line;
	    if ((file = low_get_line_plain (prog->program + id->func.offset,
					    prog, &line, 1))) {
	      my_strcat("function(");
	      my_strcat(file);
	      free(file);
	      if (line) {
		sprintf(buf, ":%ld", (long)line);
		my_strcat(buf);
	      }
	      my_putchar(')');
	      break;
	    }
	  }

	  my_strcat("function");
	}
      }
      break;

    case T_OBJECT: {
      struct object *obj = s->u.object;
      struct program *prog = obj->prog;

      if (!prog)
	my_strcat("0");
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
	      dynamic_buffer save_buf;
	      save_buffer (&save_buf);
	      
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

	      if(!SAFE_IS_ZERO(sp-1))
		{
		  if(TYPEOF(sp[-1]) != T_STRING)
		    {
		      pop_stack();
		      push_text("(object returned illegal value from _sprintf)");
		    }

		  restore_buffer (&save_buf);
		  Pike_interpreter.trace_level=save_t_flag;

		  dsv_add_string_to_buf( sp[-1].u.string );

		  pop_stack();
		  END_CYCLIC();
		  break;
		}

	      restore_buffer (&save_buf);
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
	    dynamic_buffer save_buf;
	    save_buffer (&save_buf);

	    Pike_interpreter.trace_level=0;
	    SET_CYCLIC_RET(1);
	    
	    debug_malloc_touch(obj);
	    
	    ref_push_object_inherit(obj, SUBTYPEOF(*s));
	    SAFE_APPLY_MASTER("describe_object", 1);
	    
	    debug_malloc_touch(obj);
	    
	    if(!SAFE_IS_ZERO(sp-1))
	      {
		if(TYPEOF(sp[-1]) != T_STRING)
		  {
		    pop_stack();
		    push_text("(master returned illegal value from describe_object)");
		  }
		
		restore_buffer (&save_buf);
		Pike_interpreter.trace_level=save_t_flag;
		
		dsv_add_string_to_buf( sp[-1].u.string );

		pop_stack();
		END_CYCLIC();
		break;
	      }

	    restore_buffer (&save_buf);
	    Pike_interpreter.trace_level=save_t_flag;
	    pop_stack();
	    if (!obj->prog) prog = NULL;
	  }
	  END_CYCLIC();
	}

	if (!prog) {
	  my_strcat("0");
	  break;
	}
	else {
	  char *file;
	  INT_TYPE line;
	  if ((file = low_get_program_line_plain (prog, &line, 1))) {
	    my_strcat("object(");
	    my_strcat(file);
	    free(file);
	    if (line) {
	      sprintf(buf, ":%ld", (long)line);
	      my_strcat(buf);
	    }
	    my_putchar(')');
	    break;
	  }
	}

	my_strcat("object");
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
	  dynamic_buffer save_buf;
	  save_buffer (&save_buf);

	  Pike_interpreter.trace_level=0;
	  SET_CYCLIC_RET(1);
	    
	  debug_malloc_touch(prog);
	    
	  ref_push_program(prog);
	  SAFE_APPLY_MASTER("describe_program", 1);
	    
	  debug_malloc_touch(prog);
	    
	  if(!SAFE_IS_ZERO(sp-1))
	    {
	      if(TYPEOF(sp[-1]) != T_STRING)
		{
		  pop_stack();
		  push_text("(master returned illegal value from describe_program)");
		}
	      
	      restore_buffer (&save_buf);
	      Pike_interpreter.trace_level=save_t_flag;
		
	      dsv_add_string_to_buf( sp[-1].u.string );

	      pop_stack();
	      END_CYCLIC();
	      break;
	    }

	  restore_buffer (&save_buf);
	  Pike_interpreter.trace_level=save_t_flag;
	  pop_stack();
	}
	END_CYCLIC();
      }

      {
	char *file;
	INT_TYPE line;
	if ((file = low_get_program_line_plain (prog, &line, 1))) {
	  my_strcat("program(");
	  my_strcat(file);
	  free(file);
	  if (line) {
	    sprintf(buf, ":%ld", (long)line);
	    my_strcat(buf);
	  }
#if 0
	  sprintf(buf, " %p", s->u.program);
	  my_strcat(buf);
#endif
	  my_putchar(')');
	  break;
	}
      }

      my_strcat("program");
      break;
    }

    case T_FLOAT:
      format_pike_float (buf, s->u.float_number);
      my_strcat (buf);
      break;

    case T_ARRAY:
      describe_array(s->u.array, p, indent);
      break;

    case T_MULTISET:
      describe_multiset(s->u.multiset, p, indent);
      break;

    case T_MAPPING:
      describe_mapping(s->u.mapping, p, indent);
      break;

    default:
      sprintf(buf,"<Unknown %d>", TYPEOF(*s));
      my_strcat(buf);
  }
}

/* Variant of describe_svalue that never calls pike code nor releases
 * the interpreter lock. */
PMOD_EXPORT void safe_describe_svalue (const struct svalue *s, int i,
				       struct processing *p)
{
  no_pike_calls++;
  describe_svalue (s, i, p);
  no_pike_calls--;
}

PMOD_EXPORT void print_svalue (FILE *out, const struct svalue *s)
{
  dynamic_buffer save_buf;
  dynbuf_string str;
  SIZE_T off = 0;
  init_buf(&save_buf);
  describe_svalue (s, 0, NULL);
  str = complex_free_buf(&save_buf);
  while (off < str.len) {
    SIZE_T num = fwrite (str.str + off, 1, str.len - off, out);
    if (num) off += num;
    else break;
  }
  free (str.str);
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

#ifdef PIKE_DEBUG
/* These are only defined in debug mode since no_pike_calls ought to
 * be controlled with a flag per format spec instead. When that's
 * fixed these will go. */

PMOD_EXPORT void safe_pike_vfprintf (FILE *out, const char *fmt, va_list args)
{
  struct string_builder s;
  init_string_builder (&s, 0);
  no_pike_calls++;
  string_builder_vsprintf (&s, fmt, args);
  no_pike_calls--;
  low_set_index (s.s, s.s->len, 0);
  fputs (s.s->str, out);
  free_string_builder (&s);
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
      PRE DO_FUNC(U, T, ZAP, GC_DO)					\
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
			  DO_FUNC, DO_OBJ, DO_STR, DO_TYPE)		\
  switch (T) {								\
    case T_FUNCTION:							\
      PRE DO_FUNC(U, T, ZAP, GC_DO)					\
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
      /* Assume use inside a for loop. */				\
      continue;								\
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
		      DO_MARK_STRING, GC_DO_MARK);
    t |= 1 << TYPEOF(*s);
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
		      DO_MARK_STRING, GC_DO_MARK);
    t |= 1 << TYPEOF(*s);
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
		      DO_MARK_STRING, GC_DO_MARK);
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
		      DO_MARK_STRING, GC_DO_MARK);
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
		      DONT_MARK_STRING, GC_DONT_MARK);
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
		      DONT_MARK_STRING, GC_DONT_MARK);
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
		      DONT_CYCLE_CHECK_STRING, DONT_CYCLE_CHECK);
    t |= 1 << TYPEOF(*s);
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
		      DONT_CYCLE_CHECK_STRING, DONT_CYCLE_CHECK);
    t |= 1 << TYPEOF(*s);
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
		      DONT_CYCLE_CHECK_STRING, DONT_CYCLE_CHECK);
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
		      DONT_CYCLE_CHECK_STRING, DONT_CYCLE_CHECK);
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

PMOD_EXPORT INT32 pike_sizeof(const struct svalue *s)
{
  switch(TYPEOF(*s))
  {
  case T_STRING:
    return DO_NOT_WARN((INT32)s->u.string->len);
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
	if(TYPEOF(sp[-1]) != T_INT)
	  Pike_error("Bad return type from o->_sizeof() (not int)\n");
	dmalloc_touch_svalue(Pike_sp-1);
	sp--;
	return sp->u.integer;
      }
    }
  default:
    Pike_error("Bad argument 1 to sizeof().\n");
    return 0; /* make apcc happy */
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
