/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: pike_types.c,v 1.236 2005/05/18 12:36:53 mast Exp $
*/

#include "global.h"
RCSID("$Id: pike_types.c,v 1.236 2005/05/18 12:36:53 mast Exp $");
#include <ctype.h>
#include "svalue.h"
#include "pike_types.h"
#include "stralloc.h"
#include "stuff.h"
#include "array.h"
#include "program.h"
#include "constants.h"
#include "object.h"
#include "multiset.h"
#include "mapping.h"
#include "pike_macros.h"
#include "pike_error.h"
#include "las.h"
#include "lex.h"
#include "pike_memory.h"
#include "bignum.h"
#include "main.h"
#include "opcodes.h"
#include "cyclic.h"
#include "block_alloc.h"

#ifdef PIKE_DEBUG
#define PIKE_TYPE_DEBUG
#endif /* PIKE_DEBUG */

/*
 * Flags used by low_match_types().
 */
#define A_EXACT 1
#define B_EXACT 2
#define NO_MAX_ARGS 4
#define NO_SHORTCUTS 8

/*
 * Flags used by pike_types_le()
 */
#define LE_WEAK_OBJECTS	1	/* Perform weaker checking of objects. */

/* Number of entries in the struct pike_type hash-table. */
#define PIKE_TYPE_HASH_SIZE	32768


#ifdef PIKE_TYPE_DEBUG
static int indent=0;
#endif

int max_correct_args;

PMOD_EXPORT struct pike_type *string_type_string;
PMOD_EXPORT struct pike_type *int_type_string;
PMOD_EXPORT struct pike_type *float_type_string;
PMOD_EXPORT struct pike_type *function_type_string;
PMOD_EXPORT struct pike_type *object_type_string;
PMOD_EXPORT struct pike_type *program_type_string;
PMOD_EXPORT struct pike_type *array_type_string;
PMOD_EXPORT struct pike_type *multiset_type_string;
PMOD_EXPORT struct pike_type *mapping_type_string;
PMOD_EXPORT struct pike_type *type_type_string;
PMOD_EXPORT struct pike_type *mixed_type_string;
PMOD_EXPORT struct pike_type *void_type_string;
PMOD_EXPORT struct pike_type *zero_type_string;
PMOD_EXPORT struct pike_type *any_type_string;
PMOD_EXPORT struct pike_type *weak_type_string;	/* array|mapping|multiset|function */

#ifdef DEBUG_MALLOC
struct pike_type_location *all_pike_type_locations = NULL;
#endif /* DEBUG_MALLOC */

static struct pike_type *a_markers[10], *b_markers[10];

static struct program *implements_a;
static struct program *implements_b;
static int implements_mode;

#ifdef PIKE_DEBUG
void TYPE_STACK_DEBUG(const char *fun)
{
#if 0
  fprintf(stderr, "%25s(): stack_depth:%ld   mark_stack_depth:%ld\n",
	  fun, (long)(Pike_compiler->type_stackp - type_stack),
	  (long)(Pike_compiler->pike_type_mark_stackp - pike_type_mark_stack));
#endif /* 0 */
}
#endif /* PIKE_DEBUG */

static void clear_markers(void)
{
  unsigned int e;
  for(e=0;e<NELEM(a_markers);e++)
  {
    if(a_markers[e])
    {
      free_type(a_markers[e]);
      a_markers[e]=0;
    }
    if(b_markers[e])
    {
      free_type(b_markers[e]);
      b_markers[e]=0;
    }
  }
}

struct pike_type *debug_pop_type(void)
{
  struct pike_type *t = pop_unfinished_type();
  TYPE_STACK_DEBUG("pop_type");
  type_stack_mark();
  return t;
}

struct pike_type *debug_compiler_pop_type(void)
{
  TYPE_STACK_DEBUG("compiler_pop_type");
  if(Pike_compiler->num_parse_error)
  {
    ptrdiff_t len = pop_stack_mark();
    struct pike_type *res;

    TYPE_STACK_DEBUG("paranoid_pop_type");
    if (len > 0) {
      for (;len > 1; len--) {
	/* Get rid of excess junk. */
	free_type(*(Pike_compiler->type_stackp--));
      }
      res = *(Pike_compiler->type_stackp--);
    } else {
      add_ref(res = mixed_type_string);
    }
    type_stack_mark();
    return res;
  }else{
    return debug_pop_type();
  }
}

char *get_name_of_type(TYPE_T t)
{
  switch(t)
  {
    case T_ARRAY: return "array";
    case T_FLOAT: return "float";
    case T_FUNCTION: return "function";
    case T_INT: return "int";
    case T_SVALUE_PTR: return "svalue_ptr";
    case T_OBJ_INDEX: return "obj_index";
    case T_MAPPING: return "mapping";
    case T_MULTISET: return "multiset";
    case T_OBJECT: return "object";
    case T_PROGRAM: return "program";
    case T_STRING: return "string";
    case T_TYPE: return "type";
    case T_ZERO: return "zero";
    case T_VOID: return "void";
    case T_MIXED: return "mixed";
    case T_STORAGE: return "object storage";
    case T_MAPPING_DATA: return "mapping_data";
    case T_PIKE_FRAME: return "pike_frame";
    case T_MULTISET_DATA: return "multiset_data";
    case T_STRUCT_CALLABLE: return "callable";
    default: return "unknown";
  }
}


#define TWOT(X,Y) (((X) << 8)+(Y))

static int low_pike_types_le(struct pike_type *a, struct pike_type *b,
			     int array_cnt, unsigned int flags);
static int low_check_indexing(struct pike_type *type,
			      struct pike_type *index_type,
			      node *n);
static void internal_parse_type(const char **s);

/*
 * New and improved type representation system.
 *
 * This representation is new in Pike 7.3.
 *
 * Node:	Car:		Cdr:
 * ---------------------------------------------
 * SCOPE	num vars (int)	type
 * ASSIGN	variable (int)	type
 * NAME		name (string)	type
 * FUNCTION	type		FUNCTION|MANY
 * MANY		many type	return type
 * RING		type		type
 * TUPLE	type		type
 * MAPPING	index type	value type
 * OR		type		type
 * AND		type		type
 * ARRAY	type		-
 * MULTISET	type		-
 * NOT		type		-
 * '0'-'9'	-		-
 * FLOAT	-		-
 * STRING	-		-
 * TYPE		type		-
 * PROGRAM	type		-
 * MIXED	-		-
 * VOID		-		-
 * ZERO		-		-
 * UNKNOWN	-		-
 * INT		min (int)	max (int)
 * OBJECT	implements/is	object id(int)
 *
 * Note that the cdr of a FUNCTION is a valid FUNCTION for the rest of
 * the arguments.
 *
 * Note also that functions that don't take any arguments, or just
 * a many argument just have a MANY node, and no FUNCTION node.
 *
 */
#define PIKE_TYPE_CHUNK	128
BLOCK_ALLOC(pike_type, PIKE_TYPE_CHUNK)

struct pike_type **pike_type_hash = NULL;
size_t pike_type_hash_size = 0;

void debug_free_type(struct pike_type *t)
{
#ifdef DEBUG_MALLOC
  if (t == (struct pike_type *)(size_t)0x55555555) {
    Pike_fatal("Freeing dead type.\n");
  }
#endif /* DEBUG_MALLOC */
 loop:
  if (!sub_ref(t)) {
    unsigned INT32 hash = t->hash % pike_type_hash_size;
    struct pike_type **t2 = pike_type_hash + hash;
    struct pike_type *car, *cdr;
    unsigned INT32 type;

    while (*t2) {
      if (*t2 == t) {
	*t2 = t->next;
	break;
      }
      t2 = &((*t2)->next);
    }

    car = t->car;
    cdr = t->cdr;
    type = t->type;

    really_free_pike_type((struct pike_type *)debug_malloc_pass(t));

    /* FIXME: Recursion: Should we use a stack? */
    switch(type) {
    case T_FUNCTION:
    case T_MANY:
    case T_TUPLE:
    case T_MAPPING:
    case T_OR:
    case T_AND:
    case PIKE_T_RING:
      /* Free car & cdr */
      free_type(car);
      t = (struct pike_type *)debug_malloc_pass(cdr);
      goto loop;

    case T_ARRAY:
    case T_MULTISET:
    case T_NOT:
    case T_TYPE:
    case T_PROGRAM:
      /* Free car */
      t = (struct pike_type *)debug_malloc_pass(car);
      goto loop;
	
    case T_SCOPE:
    case T_ASSIGN:
      /* Free cdr */
      t = (struct pike_type *)debug_malloc_pass(cdr);
      goto loop;

    case PIKE_T_NAME:
      free_string((struct pike_string *)car);
      t = (struct pike_type *)debug_malloc_pass(cdr);
      goto loop;

#ifdef PIKE_DEBUG
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case T_FLOAT:
    case T_STRING:
    case T_MIXED:
    case T_VOID:
    case T_ZERO:
    case PIKE_T_UNKNOWN:
    case T_INT:
    case T_OBJECT:
      break;

    default:
      Pike_fatal("free_type(): Unhandled type-node: %d\n", type);
      break;
#endif /* PIKE_DEBUG */
    }
  }
}

/* Flags used as flag_method: */
#define PT_COPY_CAR	1
#define PT_COPY_CDR	2
#define PT_COPY_BOTH	3
#define PT_SET_MARKER	4

static inline struct pike_type *debug_mk_type(unsigned INT32 type,
					      struct pike_type *car,
					      struct pike_type *cdr,
					      int flag_method)
{
  unsigned INT32 hash = DO_NOT_WARN((unsigned INT32)
				    ((ptrdiff_t)type*0x10204081)^
				    (0x8003*PTR_TO_INT(car))^
				    ~(0x10001*PTR_TO_INT(cdr)));
  unsigned INT32 index = hash % pike_type_hash_size;
  struct pike_type *t;

  /* PIKE_DEBUG code */
  if (index >= pike_type_hash_size) {
    Pike_fatal("Modulo operation failed for hash:%u, index:%u, "
	       "size:%"PRINTSIZET"d.\n",
	       hash, index, pike_type_hash_size);
  }
  /* End PIKE_DEBUG code */

  for(t = pike_type_hash[index]; t; t = t->next) {
    if ((t->hash == hash) && (t->type == type) &&
	(t->car == car) && (t->cdr == cdr)) {
      /* Free car & cdr as appropriate. */
      switch(type) {
      case T_FUNCTION:
      case T_MANY:
      case T_TUPLE:
      case T_MAPPING:
      case T_OR:
      case T_AND:
      case PIKE_T_RING:
	/* Free car & cdr */
	free_type((struct pike_type *)debug_malloc_pass(car));
	free_type((struct pike_type *)debug_malloc_pass(cdr));
	break;

      case T_ARRAY:
      case T_MULTISET:
      case T_NOT:
      case T_TYPE:
      case T_PROGRAM:
	/* Free car */
	free_type((struct pike_type *)debug_malloc_pass(car));
	break;
	
      case T_SCOPE:
      case T_ASSIGN:
	/* Free cdr */
	free_type((struct pike_type *)debug_malloc_pass(cdr));
	break;

      case PIKE_T_NAME:
	free_string((struct pike_string *)debug_malloc_pass(car));
	free_type((struct pike_type *)debug_malloc_pass(cdr));
	break;

#ifdef PIKE_DEBUG
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      case T_FLOAT:
      case T_STRING:
      case T_MIXED:
      case T_VOID:
      case T_ZERO:
      case PIKE_T_UNKNOWN:
      case T_INT:
      case T_OBJECT:
	break;

      default:
	Pike_fatal("mk_type(): Unhandled type-node: %d\n", type);
	break;
#endif /* PIKE_DEBUG */
      }
      add_ref((struct pike_type *)debug_malloc_pass(t));
      return t;
    }
  }
      
  debug_malloc_pass(t = alloc_pike_type());

  t->refs = 0;
  add_ref(t);	/* For DMALLOC... */
  t->type = type;
  t->flags = 0;
  t->car = car;
  t->cdr = cdr;

  t->hash = hash;
  t->next = pike_type_hash[index];
  pike_type_hash[index] = t;

  if (flag_method) {
    if (flag_method == PT_SET_MARKER) {
      t->flags = PT_FLAG_MARKER;
    } else {
      if (car && (flag_method & PT_COPY_CAR)) {
	t->flags = car->flags;
      }
      if (cdr && (flag_method & PT_COPY_CDR)) {
	t->flags |= cdr->flags;
      }
    }
  }

#ifdef DEBUG_MALLOC
  switch(type) {
  case T_FUNCTION:
  case T_MANY:
  case T_TUPLE:
  case T_MAPPING:
  case T_OR:
  case T_AND:
  case PIKE_T_RING:
    debug_malloc_pass(car);
    debug_malloc_pass(cdr);
    break;
    
  case T_ARRAY:
  case T_MULTISET:
  case T_NOT:
  case T_TYPE:
  case T_PROGRAM:
    debug_malloc_pass(car);
    break;
	
  case T_SCOPE:
  case T_ASSIGN:
    debug_malloc_pass(cdr);
    break;

  case PIKE_T_NAME:
    debug_malloc_pass(car);
    debug_malloc_pass(cdr);
    break;

  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
  case T_FLOAT:
  case T_STRING:
  case T_MIXED:
  case T_VOID:
  case T_ZERO:
  case PIKE_T_UNKNOWN:
  case T_INT:
  case T_OBJECT:
    break;

  default:
    Pike_fatal("mk_type(): Unhandled type-node: %d\n", type);
    break;
  }
#endif /* DEBUG_MALLOC */

  return t;
}

#ifdef DEBUG_MALLOC
#define mk_type(T,CAR,CDR,FLAG)	((struct pike_type *)debug_malloc_pass(debug_mk_type(T,CAR,CDR,FLAG)))
#else /* !DEBUG_MALLOC */
#define mk_type debug_mk_type
#endif /* DEBUG_MALLOC */

#ifdef PIKE_DEBUG
void debug_check_type_string(struct pike_type *s)
{
  /* FIXME: Add verification code here */
}

#endif /* PIKE_DEBUG */

struct pike_type *type_stack[PIKE_TYPE_STACK_SIZE];
struct pike_type **pike_type_mark_stack[PIKE_TYPE_STACK_SIZE/4];

ptrdiff_t pop_stack_mark(void)
{ 
  Pike_compiler->pike_type_mark_stackp--;
  if(Pike_compiler->pike_type_mark_stackp<pike_type_mark_stack)
    Pike_fatal("Type mark stack underflow\n");

  TYPE_STACK_DEBUG("pop_stack_mark");

  return Pike_compiler->type_stackp - *Pike_compiler->pike_type_mark_stackp;
}

void type_stack_pop_to_mark(void)
{
  pop_stack_mark();
  while(Pike_compiler->type_stackp > *Pike_compiler->pike_type_mark_stackp) {
    free_type(*(Pike_compiler->type_stackp--));
  }

  TYPE_STACK_DEBUG("type_stack_pop_to_mark");
}

struct pike_type *debug_peek_type_stack(void)
{
  return *(Pike_compiler->type_stackp);
}

void debug_push_int_type(INT_TYPE min, INT_TYPE max)
{
#if SIZEOF_INT_TYPE > 4
/* a bit kludgy: should maybe really allow 64 bit INT_TYPE */
/* see also extract_type_int */
  if (min<MIN_INT32) min=MIN_INT32;
  else if (min>MAX_INT32) min=MAX_INT32;
  if (max<MIN_INT32) max=MIN_INT32;
  else if (max>MAX_INT32) max=MAX_INT32;

#if 0
  if (min!=(INT32)min ||
      max!=(INT32)max)
    Pike_fatal("push_int_type(): int outside INT32 range (sorry)"
	       " (%"PRINTPIKEINT"d..%"PRINTPIKEINT"d)\n",
	       min,max);
#endif
#endif

#ifdef PIKE_DEBUG
  if (min > max)
    Pike_fatal("push_int_type(): Bad integer range:"
	       " min:%"PRINTPIKEINT"d, max:%"PRINTPIKEINT"d.\n",
	       min, max);
#endif /* PIKE_DEBUG */
  
  *(++Pike_compiler->type_stackp) = mk_type(T_INT,
					    (void *)(ptrdiff_t)min,
					    (void *)(ptrdiff_t)max, 0);

  TYPE_STACK_DEBUG("push_int_type");
}

void debug_push_object_type(int flag, INT32 id)
{
  *(++Pike_compiler->type_stackp) = mk_type(T_OBJECT,
                                            (void *)(ptrdiff_t)flag,
                                            (void *)(ptrdiff_t)id, 0);

  TYPE_STACK_DEBUG("push_object_type");
}

void debug_push_object_type_backwards(int flag, INT32 id)
{
  push_object_type(flag, id);
}

void debug_push_scope_type(int level)
{
  *Pike_compiler->type_stackp = mk_type(T_SCOPE,
					(void *)(ptrdiff_t)level,
					*Pike_compiler->type_stackp,
					PT_COPY_CDR);

  TYPE_STACK_DEBUG("push_scope_type");
}

void debug_push_assign_type(int marker)
{
  marker -= '0';
#ifdef PIKE_DEBUG 
  if ((marker < 0) || (marker > 9)) {
    Pike_fatal("Bad assign marker: %d\n", marker);
  }
#endif /* PIKE_DEBUG */

  *Pike_compiler->type_stackp = mk_type(T_ASSIGN,
					(void *)(ptrdiff_t)marker,
					*Pike_compiler->type_stackp,
					PT_COPY_CDR);
  TYPE_STACK_DEBUG("push_assign_type");
}

void debug_push_type_name(struct pike_string *name)
{
  /* fprintf(stderr, "push_type_name(\"%s\")\n", name->str); */
  add_ref(name);
  *Pike_compiler->type_stackp = mk_type(PIKE_T_NAME,
					(void *)name,
					*Pike_compiler->type_stackp,
					PT_COPY_CDR);
  TYPE_STACK_DEBUG("push_type_name");
}

void debug_push_finished_type(struct pike_type *t)
{
  copy_pike_type(*(++Pike_compiler->type_stackp), t);

  TYPE_STACK_DEBUG("push_finished_type");
}

void debug_push_type(unsigned int type)
{
  /* fprintf(stderr, "push_type(%d)\n", type); */

  switch(type) {
  case T_FUNCTION:
  case T_MANY:
  case T_TUPLE:
  case T_MAPPING:
  case T_OR:
  case T_AND:
  case PIKE_T_RING:
    /* Make a new type of the top two types. */
    --Pike_compiler->type_stackp;
    *Pike_compiler->type_stackp = mk_type(type,
					  *(Pike_compiler->type_stackp+1),
					  *Pike_compiler->type_stackp,
					  PT_COPY_BOTH);
    break;

  case T_ARRAY:
  case T_MULTISET:
  case T_NOT:
  case T_TYPE:
  case T_PROGRAM:
    /* Make a new type of the top type, and put it in car. */
    *Pike_compiler->type_stackp = mk_type(type,
					  *Pike_compiler->type_stackp, NULL,
					  PT_COPY_CAR);
    break;

  case T_SCOPE:
  case T_ASSIGN:
  case T_INT:
  case T_OBJECT:
  case PIKE_T_NAME:
  default:
    /* Should not occurr. */
    Pike_fatal("Unsupported argument to push_type().\n");
    break;

  case T_FLOAT:
  case T_STRING:
  case T_MIXED:
  case T_VOID:
  case T_ZERO:
  case PIKE_T_UNKNOWN:
    /* Leaf type. */
    *(++Pike_compiler->type_stackp) = mk_type(type, NULL, NULL, 0);
    break;

  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
    /* Marker. */
    *(++Pike_compiler->type_stackp) = mk_type(type, NULL, NULL, PT_SET_MARKER);
    break;
  }

  TYPE_STACK_DEBUG("push_type");
}

/* Pop one level of types. This is the inverse of push_type() */
void debug_pop_type_stack(unsigned int expected)
{ 
  struct pike_type *top;
  if(Pike_compiler->type_stackp<type_stack)
    Pike_fatal("Type stack underflow\n");

  top = *(Pike_compiler->type_stackp);
  /* Special case... */
  if (top->type == T_MIXED) return;	/* Probably due to an earlier error */

  Pike_compiler->type_stackp--;
#ifdef PIKE_DEBUG
  if ((top->type != expected) && (top->type != PIKE_T_NAME)) {
    Pike_fatal("Unexpected type on stack: %d (expected %d)\n", top->type, expected);
  }
#endif /* PIKE_DEBUG */
  /* OPTIMIZE: It looks like this function is always called with
   * expected == T_ARRAY.
   */
  switch(top->type) {
  case T_FUNCTION:
  case T_MANY:
  case T_TUPLE:
  case T_MAPPING:
  case T_OR:
  case T_AND:
  case PIKE_T_RING:
    /* Both car & cdr. */
    push_finished_type(top->cdr);
    push_finished_type(top->car);
    break;
  case T_ARRAY:
  case T_MULTISET:
  case T_NOT:
  case T_TYPE:
  case T_PROGRAM:
    /* car */
    push_finished_type(top->car);
    break;
  case T_SCOPE:
  case T_ASSIGN:
    /* cdr */
    push_finished_type(top->cdr);
    break;
  case T_INT:
  case T_OBJECT:
  case T_FLOAT:
  case T_STRING:
  case T_MIXED:
  case T_VOID:
  case T_ZERO:
  case PIKE_T_UNKNOWN:
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
    /* Leaf */
    break;
  case PIKE_T_NAME:
    /* Pop the name and recurse. */
    push_finished_type(top->cdr);
    pop_type_stack(expected);
    break;
  default:
    Pike_error("pop_type_stack(): Unhandled node type: %d\n", top->type);
  }
  free_type(top);

  TYPE_STACK_DEBUG("pop_type_stack");
}

void debug_push_reverse_type(unsigned int type)
{
  /* fprintf(stderr, "push_reverse_type(%d)\n", type); */

  switch(type) {
  case T_FUNCTION:
  case T_MANY:
  case T_TUPLE:
  case T_MAPPING:
  case T_OR:
  case T_AND:
  case PIKE_T_RING:
    {
      /* Binary type-node. -- swap the types. */
      struct pike_type *tmp = Pike_compiler->type_stackp[0];
      Pike_compiler->type_stackp[0] = Pike_compiler->type_stackp[-1];
      Pike_compiler->type_stackp[-1] = tmp;
      break;
    }
  }
  push_type(type);

  TYPE_STACK_DEBUG("push_reverse_type");
}

void debug_push_finished_type_with_markers(struct pike_type *type,
					   struct pike_type **markers)
{
 recurse:
#if 0
  fprintf(stderr, "push_finished_type_with_markers((%d[%x]),...)...\n",
	  type->type, type->flags);
#endif /* 0 */
  if (!(type->flags & PT_FLAG_MARKER)) {
    /* No markers in this sub-tree */
#if 0
    fprintf(stderr, "No markers in this subtree.\n");
#endif /* 0 */
    push_finished_type(type);
    return;
  }
  if ((type->type >= '0') && (type->type <= '9')) {
    unsigned int m = type->type - '0';
    if (markers[m]) {
      type = markers[m];
#if 0
      fprintf(stderr, "Marker %d.\n", m);
#endif /* 0 */
      goto recurse;
    } else {
      push_type(T_ZERO);
    }
    TYPE_STACK_DEBUG("push_finished_type_with_markers");
    return;
  }
  if (type->type == T_ASSIGN) {
    /* Strip assign */
#if 0
    fprintf(stderr, "Assign to marker %"PRINTPTRDIFFT"d.\n",
	    CAR_TO_INT(type);
#endif /* 0 */
    type = type->cdr;
    goto recurse;
  }
  if (type->type == PIKE_T_NAME) {
    /* Strip the name, since it won't be correct anymore. */
    type = type->cdr;
    goto recurse;
  }
  /* FIXME: T_SCOPE */
  if (type->cdr) {
    push_finished_type_with_markers(type->cdr, markers);
  }
  /* In all other cases type->car will be a valid node. */
  push_finished_type_with_markers(type->car, markers);
  /* push_type has sufficient magic to recreate the type. */
  push_type(type->type);
  TYPE_STACK_DEBUG("push_finished_type_with_markers");
}

static void push_type_field(TYPE_FIELD field)
{
  field &= (BIT_BASIC|BIT_COMPLEX);
  if (!field) {
    /* No values. */
    push_type(T_ZERO);
  } else if (field == (BIT_BASIC|BIT_COMPLEX)) {
    /* All values. */
    push_type(T_MIXED);
  } else {
    /* Check the bits... */
    push_type(T_ZERO);

    if (field & BIT_COMPLEX) {
      if (field & BIT_ARRAY) {
	push_type(T_MIXED);
	push_type(T_ARRAY);
	push_type(T_OR);
      }
      if (field & BIT_MAPPING) {
	push_type(T_MIXED);
	push_type(T_MIXED);
	push_type(T_MAPPING);
	push_type(T_OR);
      }
      if (field & BIT_MULTISET) {
	push_type(T_MIXED);
	push_type(T_MULTISET);
	push_type(T_OR);
      }
      if (field & BIT_OBJECT) {
	push_object_type(0, 0);
	push_type(T_OR);
      }
      if (field & BIT_FUNCTION) {
	push_type(T_ZERO);
	push_type(T_ZERO);
	push_type(T_MIXED);
	push_type(T_OR);
	push_type(T_MANY);
	push_type(T_OR);
      }
      if (field & BIT_PROGRAM) {
	push_object_type(0, 0);
	push_type(T_PROGRAM);
	push_type(T_OR);
      }
    }
    if (field & BIT_BASIC) {
      if (field & BIT_STRING) {
	push_type(T_STRING);
	push_type(T_OR);
      }
      if (field & BIT_TYPE) {
	push_type(T_MIXED);
	push_type(T_TYPE);
	push_type(T_OR);
      }
      if (field & BIT_INT) {
	push_int_type(MIN_INT32, MAX_INT32);
	push_type(T_OR);
      }
      if (field & BIT_FLOAT) {
	push_type(T_FLOAT);
	push_type(T_OR);
      }
    }
  }
}

INT32 extract_type_int(char *p)
{
  int e;
  INT32 ret=0;
  for(e=0;e<(int)sizeof(INT32);e++)
    ret=(ret<<8) | EXTRACT_UCHAR(p+e);
  return ret;
}

struct pike_type *debug_pop_unfinished_type(void)
{
  ptrdiff_t len;

  len = pop_stack_mark();

  if (len != 1) {
    Pike_fatal("pop_unfinished_type(): Unexpected len: %"PRINTPTRDIFFT"d\n", len);
  }

  TYPE_STACK_DEBUG("pop_unfinished_type");

  return *(Pike_compiler->type_stackp--);
}

/******/

static void internal_parse_typeA(const char **_s)
{
  char buf[80];
  unsigned int len;
  const unsigned char **s = (const unsigned char **)_s;
  
  while(ISSPACE(**s)) ++*s;

  for(len=0;isidchar(EXTRACT_UCHAR(s[0]+len));len++)
  {
    if(len>=sizeof(buf)) {
      my_yyerror("Buffer overflow in parse_type(\"%s\") (limit %"PRINTSIZET"d).",
		 *s, sizeof(buf));
      push_type(T_MIXED);
      return;
    }
    buf[len] = s[0][len];
  }
  buf[len]=0;
  *s += len;
  
  switch(buf[0])
  {
    case 'z':
      if(!strcmp(buf,"zero")) { push_type(T_ZERO); break; }
      goto bad_type;

    case 'i':
      if(!strcmp(buf,"int"))
      {
	while(ISSPACE(**s)) ++*s;
	if(**s=='(')
	{
	  INT32 min,max;
	  ++*s;
	  while(ISSPACE(**s)) ++*s;
	  if (**s != '.') {
	    min=STRTOL((const char *)*s,(char **)s,0);
	    while(ISSPACE(**s)) ++*s;
	  } else {
	    min = -0x80000000;
	  }
	  if(s[0][0]=='.' && s[0][1]=='.')
	    s[0]+=2;
	  else {
	    yyerror("Missing .. in integer type.");
	  }
	  
	  while(ISSPACE(**s)) ++*s;
	  if (**s != ')') {
	    max=STRTOL((const char *)*s,(char **)s,0);
	    while(ISSPACE(**s)) ++*s;
	  } else {
	    max = 0x7fffffff;
	  }

	  if(**s != ')') yyerror("Missing ')' in integer range.");
	  else
	    ++*s;
	  push_int_type(min, max);
	}else{
	  push_int_type(MIN_INT32, MAX_INT32);
	}
	break;
      }
      goto bad_type;

    case 'f':
      if(!strcmp(buf,"function"))
      {
	while(ISSPACE(**s)) ++*s;
	if(**s == '(')
	{
	  int nargs = 0;
	  ++*s;
	  while(ISSPACE(**s)) ++*s;
	  while(1)
	  {
	    if(**s == ':')
	    {
	      push_type(T_VOID);
	      break;
	    }
	    internal_parse_type(_s);
	    if(**s==',')
	    {
	      nargs++;
	      ++*s;
	      while(ISSPACE(**s)) ++*s;
	    }
	    else if(s[0][0]=='.' && s[0][1]=='.' && s[0][2]=='.')
	    {
	      *s+=3;
	      while(ISSPACE(**s)) ++*s;
	      if(**s != ':') {
		yyerror("Missing ':' after ... in function type.");
		*s--;
	      }
	      break;
	    } else {
	      nargs++;
	    }
	  }
	  /* Skip the colon. */
	  ++*s;
	  internal_parse_type(_s);  /* return type */
	  push_reverse_type(T_MANY);

	  while (nargs-- > 0) {
	    push_reverse_type(T_FUNCTION);
	  }

	  if(**s != ')') yyerror("Missing ')' in function type.");
	  else
	    ++*s;
	}else{
	  push_type(T_VOID);
	  push_type(T_MIXED);
	  push_type(T_OR);
	  push_type(T_VOID);
	  push_type(T_ZERO);
	  push_type(T_OR);
	  push_type(T_MANY);
	}
	break;
      }
      if(!strcmp(buf,"float")) { push_type(T_FLOAT); break; }
      goto bad_type;

    case 'o':
      if(!strcmp(buf,"object"))
      {
	while(ISSPACE(**s)) ++*s;
	if(**s == '(') /* object({,is,implements} {id,this_program}) */
	{
	  int is = 0, id;
	  ++*s;
	  while(ISSPACE(**s)) ++*s;
	  if( **s != 'i' )
	    goto no_is_implements;
	  ++*s;
	  if( **s == 's' ) {
	    ++*s;
	    if (**s != ' ') {
	      goto bad_type;
	    }
	    is = 1;
	    ++*s;
	  } else {
	    if (strncmp(*s, "mplements ", 10)) {
	      goto bad_type;
	    }
	    *s += 10;
	  }
	  while(ISSPACE(**s)) ++*s;
	no_is_implements:
	  if( !**s )
	    goto bad_type;
	  if (!strncmp(*s, "this_program", 12)) {
	    id = Pike_compiler->new_program->id;
	    *s += 12;
	  } else {
	    id = atoi( *s );	
	    while( **s >= '0' && **s <= '9' )
	      ++*s;
	  }
	  while(ISSPACE(**s)) ++*s;
	  if( !**s || **s != ')' )
	    goto bad_type;
	  ++*s;
	  push_object_type(is, id);
	}
	else
	  push_object_type(0, 0);
	break;
      }
      goto bad_type;


    case 'p':
      if(!strcmp(buf,"program")) {
	push_object_type(0, 0);
	push_type(T_PROGRAM);
	break;
      }
      goto bad_type;


    case 's':
      if(!strcmp(buf,"string")) { push_type(T_STRING); break; }
      goto bad_type;

    case 'v':
      if(!strcmp(buf,"void")) { push_type(T_VOID); break; }
      goto bad_type;

    case 't':
      if (!strcmp(buf,"tuple"))
      {
	while(ISSPACE(**s)) ++*s;
	if(**s == '(')
	{
	  ++*s;
	  internal_parse_type(_s);
	  if(**s != ',') yyerror("Expected ','.");
	  else
	    ++*s;
	  internal_parse_type(_s);
	  if(**s != ')') yyerror("Expected ')'.");
	  else
	    ++*s;
	}else{
	  push_type(T_MIXED);
	  push_type(T_MIXED);
	}
	push_reverse_type(T_TUPLE);
	break;
      }
      /* FIXME: Handle type(T) */
      if(!strcmp(buf,"type")) { push_type(T_MIXED); push_type(T_TYPE); break; }
      goto bad_type;

    case 'm':
      if(!strcmp(buf,"mixed")) { push_type(T_MIXED); break; }
      if(!strcmp(buf,"mapping"))
      {
	while(ISSPACE(**s)) ++*s;
	if(**s == '(')
	{
	  ++*s;
	  internal_parse_type(_s);
	  if(**s != ':') yyerror("Expected ':'.");
	  else
	    ++*s;
	  internal_parse_type(_s);
	  if(**s != ')') yyerror("Expected ')'.");
	  else
	    ++*s;
	}else{
	  push_type(T_MIXED);
	  push_type(T_MIXED);
	}
	push_reverse_type(T_MAPPING);
	break;
      }
      if(!strcmp(buf,"multiset"))
      {
	while(ISSPACE(**s)) ++*s;
	if(**s == '(')
	{
	  ++*s;
	  internal_parse_type(_s);
	  if(**s != ')') yyerror("Expected ')'.");
	  else
	    ++*s;
	}else{
	  push_type(T_MIXED);
	}
	push_type(T_MULTISET);
	break;
      }
      goto bad_type;

    case 'u':
      if(!strcmp(buf,"unknown")) { push_type(PIKE_T_UNKNOWN); break; }
      goto bad_type;

    case 'a':
      if(!strcmp(buf,"array"))
      {
	while(ISSPACE(**s)) ++*s;
	if(**s == '(')
	{
	  ++*s;
	  internal_parse_type(_s);
	  if(**s != ')') yyerror("Expected ')'.");
	  else
	    ++*s;
	}else{
	  push_type(T_MIXED);
	}
	push_type(T_ARRAY);
	break;
      }
      goto bad_type;

    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      if(atoi(buf)<10)
      {
	while(ISSPACE(**s)) ++*s;
	if(**s=='=')
	{
	  ++*s;
	  internal_parse_type(_s);
	  push_assign_type(buf[0]);
	}else{
	  push_type(buf[0]);
	}
	break;
      }

    default:
  bad_type:
      push_type(T_MIXED);
      my_yyerror("Couldn't parse type. (%s).", buf);
  }

  while(ISSPACE(**s)) ++*s;
}


static void internal_parse_typeB(const char **s)
{
  while(ISSPACE(EXTRACT_UCHAR(*s))) ++*s;
  switch(**s)
  {
  case '!':
    ++*s;
    internal_parse_typeB(s);
    push_type(T_NOT);
    break;

  case '(':
    ++*s;
    internal_parse_type(s);
    while(ISSPACE(EXTRACT_UCHAR(*s))) ++*s;
    if(**s != ')') {
      yyerror("Expected ')' in type.");
    }
    ++*s;
    break;
    
  default:

    internal_parse_typeA(s);
  }
}

static void internal_parse_typeCC(const char **s)
{
  internal_parse_typeB(s);

  while(ISSPACE(EXTRACT_UCHAR(*s))) ++*s;
  
  while(**s == '*')
  {
    ++*s;
    while(ISSPACE(EXTRACT_UCHAR(*s))) ++*s;
    push_type(T_ARRAY);
  }
}

static void internal_parse_typeC(const char **s)
{
  internal_parse_typeCC(s);

  if(**s == '&')
  {
    ++*s;
    internal_parse_typeC(s);
    push_reverse_type(T_AND);
  }
}

static void internal_parse_type(const char **s)
{
  internal_parse_typeC(s);

  while(**s == '|')
  {
    ++*s;
    internal_parse_typeC(s);
    push_type(T_OR);
  }
}

/* This function is used when adding simul efuns so that
 * the types for the functions can be easily stored in strings.
 * It takes a string on the exact same format as Pike and returns a type
 * struct.
 */
struct pike_type *parse_type(const char *s)
{
  struct pike_type *ret;
#ifdef PIKE_DEBUG
  struct pike_type **ts=Pike_compiler->type_stackp;
  struct pike_type ***ptms=Pike_compiler->pike_type_mark_stackp;
#endif

  /* fprintf(stderr, "parse_type(\"%s\")...\n", s); */

  TYPE_STACK_DEBUG("parse_type");

  type_stack_mark();

  internal_parse_type(&s);

  if( *s )
    yyerror("Extra junk at end of type definition.");

  ret=pop_unfinished_type();

#ifdef PIKE_DEBUG
  if(ts!=Pike_compiler->type_stackp || ptms!=Pike_compiler->pike_type_mark_stackp)
    Pike_fatal("Type stack whacked in parse_type.\n");
#endif

  return ret;
}

#ifdef PIKE_DEBUG
/* FIXME: */
void stupid_describe_type_string(char *a, ptrdiff_t len)
{
  ptrdiff_t e;
  for(e=0;e<len;e++)
  {
    if(e) fprintf(stderr, " ");
    switch(EXTRACT_UCHAR(a+e))
    {
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
	fprintf(stderr, "%c",EXTRACT_UCHAR(a+e));
	break;

      case T_SCOPE: fprintf(stderr, "scope"); break;
      case T_TUPLE: fprintf(stderr, "tuple"); break;
      case T_ASSIGN: fprintf(stderr, "="); break;
      case T_INT:
	{
	  INT32 min=extract_type_int(a+e+1);
	  INT32 max=extract_type_int(a+e+1+sizeof(INT32));
	  fprintf(stderr, "int");
	  if(min!=MIN_INT32 || max!=MAX_INT32)
	    fprintf(stderr, "(%ld..%ld)",(long)min,(long)max);
	  e+=sizeof(INT32)*2;
	  break;
	}
      case T_FLOAT: fprintf(stderr, "float"); break;
      case T_STRING: fprintf(stderr, "string"); break;
      case T_TYPE: fprintf(stderr, "type"); break;
      case T_PROGRAM: fprintf(stderr, "program"); break;
      case T_OBJECT:
	fprintf(stderr, "object(%s %ld)",
	       EXTRACT_UCHAR(a+e+1)?"is":"implements",
	       (long)extract_type_int(a+e+2));
	e+=sizeof(INT32)+1;
	break;
      case T_FUNCTION: fprintf(stderr, "function"); break;
      case T_ARRAY: fprintf(stderr, "array"); break;
      case T_MAPPING: fprintf(stderr, "mapping"); break;
      case T_MULTISET: fprintf(stderr, "multiset"); break;
	
      case PIKE_T_UNKNOWN: fprintf(stderr, "unknown"); break;
      case T_MANY: fprintf(stderr, "many"); break;
      case T_OR: fprintf(stderr, "or"); break;
      case T_AND: fprintf(stderr, "and"); break;
      case T_NOT: fprintf(stderr, "not"); break;
      case T_VOID: fprintf(stderr, "void"); break;
      case T_ZERO: fprintf(stderr, "zero"); break;
      case T_MIXED: fprintf(stderr, "mixed"); break;
	
      default: fprintf(stderr, "%d",EXTRACT_UCHAR(a+e)); break;
    }
  }
  fprintf(stderr, "\n");
}

void simple_describe_type(struct pike_type *s)
{
  if (s) {
    switch(s->type) {
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
	fprintf(stderr, "%d", s->type-'0');
	break;

      case PIKE_T_NAME:
	fprintf(stderr, "{ %s = ", ((struct pike_string *)s->car)->str);
	simple_describe_type(s->cdr);
	fprintf(stderr, " }");
	break;

      case T_SCOPE:
	fprintf(stderr, "scope(%"PRINTPTRDIFFT"d, ", CAR_TO_INT(s));
	simple_describe_type(s->cdr);
	fprintf(stderr, ")");
	break;
      case T_TUPLE:
	fprintf(stderr, "tuple(");
	simple_describe_type(s->car);
	fprintf(stderr, ", ");
	simple_describe_type(s->cdr);
	fprintf(stderr, ")");	
	break;
      case T_ASSIGN:
	fprintf(stderr, "%"PRINTPTRDIFFT"d = ", CAR_TO_INT(s));
	simple_describe_type(s->cdr);
	break;
      case T_INT:
	{
	  INT32 min = CAR_TO_INT(s);
	  INT32 max = CDR_TO_INT(s);
	  fprintf(stderr, "int");
	  if(min!=MIN_INT32 || max!=MAX_INT32)
	    fprintf(stderr, "(%ld..%ld)",(long)min,(long)max);
	  break;
	}
      case T_FLOAT: fprintf(stderr, "float"); break;
      case T_STRING: fprintf(stderr, "string"); break;
      case T_TYPE:
	fprintf(stderr, "type(");
	simple_describe_type(s->car);
	fprintf(stderr, ")");
	break;
      case T_PROGRAM:
	fprintf(stderr, "program(");
	simple_describe_type(s->car);
	fprintf(stderr, ")");
	break;
      case T_OBJECT:
	fprintf(stderr, "object(%s %"PRINTPTRDIFFT"d)",
	       s->car?"is":"implements",
		CDR_TO_INT(s));
	break;
      case T_FUNCTION:
      case T_MANY:
	fprintf(stderr, "function(");
	while(s->type == T_FUNCTION) {
	  simple_describe_type(s->car);
	  s = s->cdr;
	  if ((s->type == T_FUNCTION) ||
	      (s->car->type != T_VOID)) {
	    fprintf(stderr, ", ");
	  }
	}
	if (s->car->type != T_VOID) {
	  simple_describe_type(s->car);
	  fprintf(stderr, "...");
	}
	fprintf(stderr, ":");
	simple_describe_type(s->cdr);
	fprintf(stderr, ")");
	break;
      case T_ARRAY:
	fprintf(stderr, "array(");
	simple_describe_type(s->car);
	fprintf(stderr, ")");
	break;
      case T_MAPPING:
	fprintf(stderr, "mapping(");
	simple_describe_type(s->car);
	fprintf(stderr, ":");
	simple_describe_type(s->cdr);
	fprintf(stderr, ")");
	break;
      case T_MULTISET:
	fprintf(stderr, "multiset(");
	simple_describe_type(s->car);
	fprintf(stderr, ")");
	break;
	
      case PIKE_T_UNKNOWN: fprintf(stderr, "unknown"); break;
      case PIKE_T_RING:
	fprintf(stderr, "ring(");
	simple_describe_type(s->car);
	fprintf(stderr, "°");
	simple_describe_type(s->cdr);
	fprintf(stderr, ")");
	break;
      case T_OR:
	fprintf(stderr, "or(");
	simple_describe_type(s->car);
	fprintf(stderr, "|");
	simple_describe_type(s->cdr);
	fprintf(stderr, ")");
	break;
      case T_AND:
	fprintf(stderr, "and(");
	simple_describe_type(s->car);
	fprintf(stderr, "&");
	simple_describe_type(s->cdr);
	fprintf(stderr, ")");
	break;
      case T_NOT:
	fprintf(stderr, "not(");
	simple_describe_type(s->car);
	fprintf(stderr, ")");
	break;
      case T_VOID: fprintf(stderr, "void"); break;
      case T_ZERO: fprintf(stderr, "zero"); break;
      case T_MIXED: fprintf(stderr, "mixed"); break;
	
      default:
	fprintf(stderr, "Unknown type node: %d, %p:%p",
	       s->type, s->car, s->cdr);
	break;
    }
  } else {
    fprintf(stderr, "NULL");
  }
}

#ifdef DEBUG_MALLOC
void describe_all_types(void)
{
  unsigned INT32 index;

  for(index = 0; index < pike_type_hash_size; index++) {
    struct pike_type *t;
    for (t = pike_type_hash[index]; t; t = t->next) {
      if (t->refs) {
	fprintf(stderr, "Type at 0x%p: ", t);
	simple_describe_type(t);
	fprintf(stderr, " (refs:%ld)\n", (long)t->refs);
      }
    }
  }
}
#endif /* DEBUG_MALLOC */
#endif

static void low_describe_type(struct pike_type *t)
{
  /**** FIXME: ****/
  switch(t->type)
  {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      my_putchar(t->type);
      break;
      
    case T_ASSIGN:
      my_putchar('(');
      my_putchar('0' + CAR_TO_INT(t));
      my_putchar('=');
      my_describe_type(t->cdr);
      my_putchar(')');
      break;

    case T_SCOPE:
      my_putchar('{');
      my_putchar(CAR_TO_INT(t));
      my_putchar(',');
      my_describe_type(t->cdr);
      my_putchar('}');
      break;

    case T_TUPLE:
      my_putchar('[');
      my_describe_type(t->car);
      my_putchar(',');
      my_describe_type(t->cdr);
      my_putchar(']');
      break;

    case T_VOID: my_strcat("void"); break;
    case T_ZERO: my_strcat("zero"); break;
    case T_MIXED: my_strcat("mixed"); break;
    case PIKE_T_UNKNOWN: my_strcat("unknown"); break;
    case T_INT:
    {
      INT32 min=CAR_TO_INT(t);
      INT32 max=CDR_TO_INT(t);
      my_strcat("int");
      
      if(min!=MIN_INT32 || max!=MAX_INT32)
      {
	char buffer[100];
	sprintf(buffer,"(%ld..%ld)",(long)min,(long)max);
	my_strcat(buffer);
      }
      break;
    }
    case T_FLOAT: my_strcat("float"); break;
    case T_PROGRAM:
      if ((t->car->type == T_OBJECT) &&
	  (!t->car->cdr)) {
	my_strcat("program");
      } else {
	my_strcat("program(");
	my_describe_type(t->car);
	my_strcat(")");
      }
      break;
    case T_OBJECT:
      if (t->cdr)
      {
	char buffer[100];
	sprintf(buffer,"object(%s %"PRINTPTRDIFFT"d)",
		t->car?"is":"implements",
		CDR_TO_INT(t));
	my_strcat(buffer);
      }else{
	my_strcat("object");
      }
      break;

    case T_STRING: my_strcat("string"); break;
    case T_TYPE:
      my_strcat("type(");
      my_describe_type(t->car);
      my_strcat(")");
      break;

    case PIKE_T_NAME:
      if (!((struct pike_string *)t->car)->size_shift) {
	my_strcat("{ ");
	my_binary_strcat(((struct pike_string *)t->car)->str,
			 ((struct pike_string *)t->car)->len);
	my_strcat(" = ");
	my_describe_type(t->cdr);
	my_strcat(" }");
      } else {
	my_describe_type(t->cdr);
      }
      break;
      
    case T_FUNCTION:
    case T_MANY:
    {
      int s;
      my_strcat("function");
      if(t->type == T_MANY &&
	 t->cdr->type == T_OR &&
	 ((t->cdr->car->type == T_MIXED && t->cdr->cdr->type == T_VOID) ||
	  (t->cdr->cdr->type == T_MIXED && t->cdr->car->type == T_VOID)) &&
	 (t->car->type == T_ZERO ||
	  (t->car->type == T_OR &&
	   ((t->car->car->type == T_ZERO && t->car->cdr->type == T_VOID) ||
	    (t->car->cdr->type == T_ZERO && t->car->car->type == T_VOID)))))
      {
	/* function == function(zero...:mixed|void) or
	 *             function(zero|void...:mixed|void)
	 */
	/* done */
      } else {
	my_strcat("(");
	s=0;
	while(t->type != T_MANY)
	{
	  if(s++) my_strcat(", ");
	  my_describe_type(t->car);
	  t = t->cdr;
	}
	if(t->car->type != T_VOID)
	{
	  if(s++) my_strcat(", ");
	  my_describe_type(t->car);
	  my_strcat(" ...");
	}
	my_strcat(" : ");
	my_describe_type(t->cdr);
	my_strcat(")");
      }
      break;
    }
    
    case T_ARRAY:
      my_strcat("array");
      if(t->car->type != T_MIXED) {
	my_strcat("(");
	my_describe_type(t->car);
	my_strcat(")");
      }
      break;
      
    case T_MULTISET:
      my_strcat("multiset");
      if(t->car->type != T_MIXED) {
	my_strcat("(");
	my_describe_type(t->car);
	my_strcat(")");
      }
      break;
      
    case T_NOT:
      my_strcat("!");
      if (t->car->type > T_NOT) {
	my_strcat("(");
	my_describe_type(t->car);
	my_strcat(")");
      } else {
	my_describe_type(t->car);
      }
      break;

    case PIKE_T_RING:
      /* FIXME: Should be renumbered for correct parenthesing. */
      my_strcat("(");
      my_describe_type(t->car);
      my_strcat(")°(");
      my_describe_type(t->cdr);
      my_strcat(")");
      break;
      
    case T_OR:
      if (t->car->type > T_OR) {
	my_strcat("(");
	my_describe_type(t->car);
	my_strcat(")");
      } else {
	my_describe_type(t->car);
      }
      my_strcat(" | ");
      if (t->cdr->type > T_OR) {
	my_strcat("(");
	my_describe_type(t->cdr);
	my_strcat(")");
      } else {
	my_describe_type(t->cdr);
      }
      break;
      
    case T_AND:
      if (t->car->type > T_AND) {
	my_strcat("(");
	my_describe_type(t->car);
	my_strcat(")");
      } else {
	my_describe_type(t->car);
      }
      my_strcat(" & ");
      if (t->cdr->type > T_AND) {
	my_strcat("(");
	my_describe_type(t->cdr);
	my_strcat(")");
      } else {
	my_describe_type(t->cdr);
      }
      break;
      
    case T_MAPPING:
      my_strcat("mapping");
      if(t->car->type != T_MIXED || t->cdr->type != T_MIXED) {
	my_strcat("(");
	my_describe_type(t->car);
	my_strcat(":");
	my_describe_type(t->cdr);
	my_strcat(")");
      }
      break;
    default:
      {
	char buf[20];
	my_strcat("unknown code(");
	sprintf(buf, "%d", t->type);
	my_strcat(buf);
	my_strcat(")");
	break;
      }
  }
}

void my_describe_type(struct pike_type *type)
{
  low_describe_type(type);
}

struct pike_string *describe_type(struct pike_type *type)
{
  dynamic_buffer save_buf;
  check_type_string(type);
  if(!type) return make_shared_string("mixed");
  init_buf(&save_buf);
  low_describe_type(type);
  return free_buf(&save_buf);
}


/******/

static int low_is_same_type(struct pike_type *a, struct pike_type *b)
{
  return a == b;
}

TYPE_T compile_type_to_runtime_type(struct pike_type *t)
{
  switch(t->type)
  {
  case PIKE_T_RING:
    return compile_type_to_runtime_type(t->car);

  case T_OR:
    {
      TYPE_T tmp = compile_type_to_runtime_type(t->car);
      if(tmp == compile_type_to_runtime_type(t->cdr))
	return tmp;

      /* FALL_THROUGH */
    }
  case T_TUPLE:
    /* FIXME: Shouldn't occur/should be converted to array? */
    /* FALL_THROUGH */
  default:
    return T_MIXED;

  case T_ZERO:
    return T_INT;

  case T_SCOPE:
  case PIKE_T_NAME:
    return compile_type_to_runtime_type(t->cdr);

  case T_MANY:
    return T_FUNCTION;

  case T_ARRAY:
  case T_MAPPING:
  case T_MULTISET:

  case T_OBJECT:
  case T_PROGRAM:
  case T_FUNCTION:
    
  case T_STRING:
  case T_TYPE:
  case T_INT:
  case T_FLOAT:
    return t->type;
  }
}


static int low_find_exact_type_match(struct pike_type *needle,
				     struct pike_type *haystack,
				     unsigned int separator)
{
  while(haystack->type == separator)
  {
    if(low_find_exact_type_match(needle, haystack->car, separator))
      return 1;
    haystack = haystack->cdr;
  }
  return low_is_same_type(needle, haystack);
}

static void very_low_or_pike_types(struct pike_type *to_push,
				   struct pike_type *not_push)
{
  while(to_push->type == T_OR)
  {
    very_low_or_pike_types(to_push->car, not_push);
    to_push = to_push->cdr;
  }
  /* FIXME:
   * this might use the 'le' operator
   */

  if(!low_find_exact_type_match(to_push, not_push, T_OR))
  {
    push_finished_type(to_push);
    push_type(T_OR);
  }
}

static void low_or_pike_types(struct pike_type *t1,
			      struct pike_type *t2,
			      int zero_implied)
{
  if(!t1)
  {
    if(!t2)
      push_type(T_VOID);
    else
      push_finished_type(t2);
  }
  else if((!t2)
	  || (t2->type == T_ZERO && zero_implied)

    )
  {
    push_finished_type(t1);
  }
  else if (t1->type == T_ZERO && zero_implied)
  {
    push_finished_type(t2);
  }
  else if(t1->type == T_MIXED || t2->type == T_MIXED)
  {
    push_type(T_MIXED);
  }
  else if(t1->type == T_INT && t2->type == T_INT)
  {
    INT32 min1 = CAR_TO_INT(t1);
    INT32 max1 = CDR_TO_INT(t1);
    INT32 min2 = CAR_TO_INT(t2);
    INT32 max2 = CDR_TO_INT(t2);

    if ((max1 + 1 < min2) || (max2 + 1 < min1)) {
      /* No overlap. */
      push_finished_type(t1);
      push_finished_type(t2);
      push_type(T_OR);
    } else {
      /* Overlap */
      min1 = MINIMUM(min1, min2);
      max1 = MAXIMUM(max1, max2);

      push_int_type(min1, max1);
    }
  }
  else if (t1->type == T_SCOPE)
  {
    if (t2->type == T_SCOPE) {
      low_or_pike_types(t1->cdr, t2->cdr, zero_implied);
      if (t1->car > t2->car)
	push_scope_type(CAR_TO_INT(t1));
      else
	push_scope_type(CAR_TO_INT(t2));
    } else {
      low_or_pike_types(t1->cdr, t2, zero_implied);
      push_scope_type(CAR_TO_INT(t1));
    }
  }
  else if (t2->type == T_SCOPE)
  {
    low_or_pike_types(t1, t2->cdr, zero_implied);
    push_scope_type(CAR_TO_INT(t2));
    push_type(T_SCOPE);
  }
  else
  {
    push_finished_type(t1);
    very_low_or_pike_types(t2,t1);
  }
}

struct pike_type *or_pike_types(struct pike_type *a,
				struct pike_type *b,
				int zero_implied)
{
  type_stack_mark();
  low_or_pike_types(a,b,1 /*zero_implied*/);
  return pop_unfinished_type();
}

static void very_low_and_pike_types(struct pike_type *to_push,
				    struct pike_type *not_push)
{
  while(to_push->type == T_AND)
  {
    very_low_and_pike_types(to_push->car, not_push);
    to_push = to_push->cdr;
  }
  if(!low_find_exact_type_match(to_push, not_push, T_AND))
  {
    push_finished_type(to_push);
    push_type(T_AND);
  }
}

static void even_lower_and_pike_types(struct pike_type *t1,
				      struct pike_type *t2)
{
  while(t2->type == T_OR)
  {
    even_lower_and_pike_types(t1, t2->car);
    t2 = t2->cdr;
  }
  if (t1->type == t2->type) {
    if (t1->type == T_INT) {
      INT32 i1, i2;
      INT32 upper_bound, lower_bound;
      i1 = CDR_TO_INT(t1);
      i2 = CDR_TO_INT(t2);
      upper_bound = MINIMUM(i1,i2);

      i1 = CAR_TO_INT(t1);
      i2 = CAR_TO_INT(t2);
      lower_bound = MAXIMUM(i1,i2);

      if (upper_bound >= lower_bound) {
	push_int_type(lower_bound, upper_bound);
	push_type(T_OR);
      }
    } else {
      push_finished_type(t1);
      push_type(T_OR);
    }
  }
}

static int lower_and_pike_types(struct pike_type *t1, struct pike_type *t2)
{
  int is_complex = 0;
  while(t1->type == T_OR)
  {
    is_complex |= lower_and_pike_types(t1->car, t2);
    t1 = t1->cdr;
  }
  switch(t1->type) {
  case T_ZERO:
  case T_VOID:
    break;
  case T_STRING:
  case T_FLOAT:
  case T_INT:
    even_lower_and_pike_types(t1, t2);
    break;
  default:
    return 1;
  }
  return is_complex;
}

static int low_and_push_complex_pike_type(struct pike_type *type)
{
  int is_complex = 0;
  while(type->type == T_OR)
  {
    int new_complex;
    new_complex = low_and_push_complex_pike_type(type->car);
    if (new_complex) {
      if (is_complex) {
	push_type(T_OR);
      } else {
	is_complex = 1;
      }
    }
    type = type->cdr;
  }
  switch(type->type) {
  case T_VOID:
  case T_ZERO:
  case T_STRING:
  case T_FLOAT:
  case T_INT:
    /* Simple type. Already handled. */
    break;
  default:
    push_finished_type(type);
    if (is_complex) {
      push_type(T_OR);
    }
    return 1;
  }
  return is_complex;
}

static void low_and_pike_types(struct pike_type *t1,
			       struct pike_type *t2)
{
  if(!t1 || t1->type == T_VOID ||
     !t2 || t2->type == T_VOID)
  {
    push_type(T_VOID);
  }
  else if(t1->type == T_ZERO ||
	  t2->type == T_ZERO)
  {
    push_type(T_ZERO);
  }
  else if(t1->type == T_MIXED)
  {
    push_finished_type(t2);
  }
  else if(t2->type == T_MIXED)
  {
    push_finished_type(t1);
  }
  else if(t1->type == T_INT && t2->type == T_INT)
  {
    INT32 i1,i2;
    INT32 upper_bound, lower_bound;
    i1 = CDR_TO_INT(t1);
    i2 = CDR_TO_INT(t2);
    upper_bound = MINIMUM(i1,i2);

    i1 = CAR_TO_INT(t1);
    i2 = CAR_TO_INT(t2);
    lower_bound = MAXIMUM(i1,i2);

    if (upper_bound >= lower_bound) {
      push_int_type(lower_bound, upper_bound);
    } else {
      /* No overlap! */
      /* FIXME: Warn? */
      push_type(T_VOID);
    }
  }
  else if (t1->type == T_SCOPE)
  {
    if (t2->type == T_SCOPE) {
      low_and_pike_types(t1->cdr, t2->cdr);
      if (t1->car > t2->car)
	push_scope_type(CAR_TO_INT(t1));
      else
	push_scope_type(CAR_TO_INT(t2));
    } else {
      low_and_pike_types(t1->cdr, t2);
      push_scope_type(CAR_TO_INT(t1));
    }
  }
  else if (t2->type == T_SCOPE)
  {
    low_and_pike_types(t1, t2->cdr);
    push_scope_type(CAR_TO_INT(t2));
  }
  else if((t1->type == t2->type) &&
	  ((t1->type == T_STRING) ||
	   (t1->type == T_FLOAT)))
  {
    push_finished_type(t1);
  }
  else if(low_pike_types_le(t1, t2, 0, 0))
  {
    push_finished_type(t1);
  }
  else if(low_pike_types_le(t2, t1, 0, 0))
  {
    push_finished_type(t2);
  }
  else
  {
    push_type(T_ZERO);

    if (lower_and_pike_types(t1, t2)) {
      /* t1 contains complex types. */
      if (low_and_push_complex_pike_type(t2)) {
	/* t2 also contains complex types. */
	low_and_push_complex_pike_type(t1);
	push_type(T_AND);
	push_type(T_OR);
      }
    }
    /*     push_finished_type(t1); */
    /*     very_low_and_pike_types(t2,t1); */
  }
}

struct pike_type *and_pike_types(struct pike_type *a,
				 struct pike_type *b)
{
  type_stack_mark();
  low_and_pike_types(a, b);
  return pop_unfinished_type();
}

static struct pike_type *low_object_lfun_type(struct pike_type *t, short lfun)
{
  struct program *p;
  int i;
  p = id_to_program(CDR_TO_INT(t));
  if(!p) return 0;
  i=FIND_LFUN(p, lfun);
  if(i==-1) return 0;
  return ID_FROM_INT(p, i)->type;
}



/******/

/*
 * match two type strings, return zero if they don't match, and return
 * the part of 'a' that _did_ match if it did.
 */
static struct pike_type *low_match_types(struct pike_type *a,
					 struct pike_type *b,
					 int flags)
#ifdef PIKE_TYPE_DEBUG
{
  int e;
  char *s;
  static struct pike_type *low_match_types2(struct pike_type *a,
					    struct pike_type *b,
					    int flags);

  if (l_flag>2) {
    dynamic_buffer save_buf;
    init_buf(&save_buf);
    for(e=0;e<indent;e++) my_strcat("  ");
    my_strcat("low_match_types(");
    my_describe_type(a);
    my_strcat(",\n");
    for(e=0;e<indent;e++) my_strcat("  ");
    my_strcat("                ");
    my_describe_type(b);
    my_strcat(",\n");
    for(e=0;e<indent;e++) my_strcat("  ");
    my_strcat("                ");

    if (flags) {
      int f = 0;
      if (flags & A_EXACT) {
	my_strcat("A_EXACT");
	f = 1;
      }
      if (flags & B_EXACT) {
	if (f) {
	  my_strcat(" | ");
	}
	my_strcat("B_EXACT");
	f = 1;
      }
      if (flags & NO_MAX_ARGS) {
	if (f) {
	  my_strcat(" | ");
	}
	my_strcat("NO_MAX_ARGS");
	f = 1;
      }
      if (flags & NO_SHORTCUTS) {
	if (f) {
	  my_strcat(" | ");
	}
	my_strcat("NO_SHORTCUTS");
	f = 1;
      }
    } else {
      my_strcat("0");
    }
    my_strcat(");\n");
    fprintf(stderr,"%s",(s=simple_free_buf(&save_buf)));
    free(s);
    indent++;
  }

  a = low_match_types2(a, b, flags);

  if (l_flag>2) {
    dynamic_buffer save_buf;
    indent--;
    init_buf(&save_buf);
    for(e=0;e<indent;e++) my_strcat("  ");
    my_strcat("= ");
    if(a)
      my_describe_type(a);
    else
      my_strcat("NULL");
    my_strcat("\n");
    fprintf(stderr,"%s",(s=simple_free_buf(&save_buf)));
    free(s);
  }
  return a;
}

static struct pike_type *low_match_types2(struct pike_type *a,
					  struct pike_type *b,
					  int flags)
#endif

{
  int correct_args;
  struct pike_type *ret;
  if(a == b) return a;

  switch(a->type)
  {
  case T_AND:
    ret = low_match_types(a->car, b, flags);
    if(!ret) return 0;
    return low_match_types(a->cdr, b, flags);

  case T_OR:
    ret = low_match_types(a->car, b, flags);
    if(ret && !(flags & NO_SHORTCUTS)) return ret;
    if(ret)
    {
      low_match_types(a->cdr, b, flags);
      return ret;
    }else{
      return low_match_types(a->cdr, b, flags);
    }

  case PIKE_T_RING:
    return low_match_types(a->car, b, flags);

  case PIKE_T_NAME:
    return low_match_types(a->cdr, b, flags);

  case T_NOT:
    if(low_match_types(a->car, b, (flags ^ B_EXACT ) | NO_MAX_ARGS))
      return 0;
    return a;

    case T_ASSIGN:
      ret = low_match_types(a->cdr, b, flags);
      if(ret && (b->type != T_VOID))
      {
	int m = CAR_TO_INT(a);
	struct pike_type *tmp;

#ifdef PIKE_DEBUG
	if ((m < 0) || (m > 9)) {
	  Pike_fatal("marker out of range: %d\n", m);
	}
#endif /* PIKE_DEBUG */

	type_stack_mark();
	push_finished_type_with_markers(b, b_markers);
	tmp = pop_unfinished_type();

	type_stack_mark();
	low_or_pike_types(a_markers[m], tmp, 0);
	if(a_markers[m]) free_type(a_markers[m]);
	free_type(tmp);
	a_markers[m] = pop_unfinished_type();

#ifdef PIKE_TYPE_DEBUG
	if (l_flag>2) {
	  dynamic_buffer save_buf;
	  char *s;
	  int e;
	  init_buf(&save_buf);
	  for(e=0;e<indent;e++) my_strcat("  ");
	  my_strcat("a_markers[");
	  my_putchar((char)(m+'0'));
	  my_strcat("]=");
	  my_describe_type(a_markers[m]);
	  my_strcat("\n");
	  fprintf(stderr,"%s",(s=simple_free_buf(&save_buf)));
	  free(s);
	}
#endif
#ifdef PIKE_DEBUG
	if(a_markers[m]->type == m+'0')
	  Pike_fatal("Cyclic type!\n");
#endif
      }
      return ret;

    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
    {
      int m = a->type - '0';
      if(a_markers[m])
      {
#ifdef PIKE_DEBUG
	if(a_markers[m]->type == a->type)
	  Pike_fatal("Cyclic type!\n");
	if(a_markers[m]->type == T_OR &&
	   a_markers[m]->car->type == a->type)
	  Pike_fatal("Cyclic type!\n");
#endif
	return low_match_types(a_markers[m], b, flags);
      }
      else
	return low_match_types(mixed_type_string, b, flags);
    }
  }

  switch(b->type)
  {
  case T_AND:
    ret = low_match_types(a, b->car, flags);
    if(!ret) return 0;
    return low_match_types(a, b->cdr, flags);

  case T_OR:
    ret = low_match_types(a, b->car, flags);
    if(ret && !(flags & NO_SHORTCUTS)) return ret;
    if(ret)
    {
      low_match_types(a, b->cdr, flags);
      return ret;
    }else{
      return low_match_types(a, b->cdr, flags);
    }

  case PIKE_T_RING:
    return low_match_types(a, b->car, flags);

  case PIKE_T_NAME:
    return low_match_types(a, b->cdr, flags);

  case T_NOT:
    if(low_match_types(a, b->car, (flags ^ A_EXACT ) | NO_MAX_ARGS))
      return 0;
    return a;

    case T_ASSIGN:
      ret = low_match_types(a, b->cdr, flags);
      if(ret && (a->type != T_VOID))
      {
	int m = CAR_TO_INT(b);
	struct pike_type *tmp;
	type_stack_mark();
	push_finished_type_with_markers(a, a_markers);
	tmp=pop_unfinished_type();

	type_stack_mark();
	low_or_pike_types(b_markers[m], tmp, 0);
	if(b_markers[m]) free_type(b_markers[m]);
	free_type(tmp);
	b_markers[m] = pop_unfinished_type();
#ifdef PIKE_TYPE_DEBUG
	if (l_flag>2) {
	  dynamic_buffer save_buf;
	  char *s;
	  int e;
	  init_buf(&save_buf);
	  for(e=0;e<indent;e++) my_strcat("  ");
	  my_strcat("b_markers[");
	  my_putchar((char)(m+'0'));
	  my_strcat("]=");
	  my_describe_type(b_markers[m]);
	  my_strcat("\n");
	  fprintf(stderr,"%s",(s=simple_free_buf(&save_buf)));
	  free(s);
	}
#endif
#ifdef PIKE_DEBUG
	if(b_markers[m]->type == m+'0')
	  Pike_fatal("Cyclic type!\n");
#endif
      }
      return ret;

    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
    {
      int m = b->type - '0';
      if(b_markers[m])
      {
#ifdef PIKE_DEBUG
	if(b_markers[m]->type == b->type)
	  Pike_fatal("Cyclic type!\n");
#endif
	return low_match_types(a, b_markers[m], flags);
      }
      else
	return low_match_types(a, mixed_type_string, flags);
    }
  }

  /* 'mixed' matches anything */

  if((a->type == T_ZERO || a->type == T_MIXED) &&
     !(flags & (A_EXACT|B_EXACT)) &&
     (b->type != T_VOID))
  {
#if 1
    switch(b->type)
    {
      /* These types can contain sub-types */
      case T_ARRAY:
	low_match_types(array_type_string, b, flags);
	break;
      case T_MAPPING:
	low_match_types(mapping_type_string, b, flags);
	break;
      case T_FUNCTION:
      case T_MANY:
	low_match_types(function_type_string, b, flags);
	break;
      case T_MULTISET:
	low_match_types(multiset_type_string, b, flags);
	break;
    }
#endif
    return a;
  }

  if((b->type == T_ZERO || b->type == T_MIXED) &&
     !(flags & (A_EXACT|B_EXACT)) &&
     (a->type != T_VOID))
  {
#if 1
    switch(a->type)
    {
      /* These types can contain sub-types */
      case T_ARRAY:
	low_match_types(a, array_type_string, flags);
	break;
      case T_MAPPING:
	low_match_types(a, mapping_type_string, flags);
	break;
      case T_FUNCTION:
      case T_MANY:
	low_match_types(a, function_type_string, flags);
	break;
      case T_MULTISET:
	low_match_types(a, multiset_type_string, flags);
	break;
    }
#endif
    return a;
  }

  /* Special cases (tm) */
  switch(TWOT(a->type, b->type))
  {
  case TWOT(T_PROGRAM, T_FUNCTION):
  case TWOT(T_FUNCTION, T_PROGRAM):
  case TWOT(T_PROGRAM, T_MANY):
  case TWOT(T_MANY, T_PROGRAM):
    /* FIXME: Should look at the sub-type of the program
     * to determine the prototype to use.
     */
    return a;

  case TWOT(T_OBJECT, T_FUNCTION):
  case TWOT(T_OBJECT, T_MANY):
  {
    struct pike_type *s;
    if((s = low_object_lfun_type(a, LFUN_CALL)))
       return low_match_types(s, b, flags);
    if (flags & B_EXACT) {
      /* A function isn't an object */
      return 0;
    }
    return a;
  }

  case TWOT(T_FUNCTION, T_OBJECT):
  case TWOT(T_MANY, T_OBJECT):
  {
    struct pike_type *s;
    if((s=low_object_lfun_type(b, LFUN_CALL)))
       return low_match_types(a, s, flags);
    if (flags & A_EXACT) {
      /* A function isn't an object */
      return 0;
    }
    return a;
  }
  case TWOT(T_INT, T_ZERO):
  {
    if ((CAR_TO_INT(a) > 0) || (CDR_TO_INT(a) < 0)) {
      return 0;
    }
    return a;
  }
  case TWOT(T_ZERO, T_INT):
  {
    if ((CAR_TO_INT(b) > 0) || (CDR_TO_INT(b) < 0)) {
      return 0;
    }
    return a;
  }
  case TWOT(T_FUNCTION, T_FUNCTION):
  case TWOT(T_FUNCTION, T_MANY):
  case TWOT(T_MANY, T_FUNCTION):
  case TWOT(T_MANY, T_MANY):
    ret = a;
    correct_args=0;
    while ((a->type != T_MANY) || (b->type != T_MANY))
    {
      struct pike_type *a_tmp, *b_tmp;

      a_tmp = a->car;
      if (a->type == T_FUNCTION)
      {
	a = a->cdr;
      }

      b_tmp = b->car;
      if (b->type == T_FUNCTION)
      {
	b = b->cdr;
      }

      if(!low_match_types(a_tmp, b_tmp,
			  (flags | NO_MAX_ARGS) & ~(A_EXACT|B_EXACT)))
	return 0;
      if(++correct_args > max_correct_args)
	if(!(flags & NO_MAX_ARGS))
	  max_correct_args=correct_args;
    }
    /* check the 'many' type */
    if(b->car->type != T_VOID &&
       a->car->type != T_VOID)
    {
      if(!low_match_types(a->car, b->car,
			  (flags | NO_MAX_ARGS) & ~(A_EXACT|B_EXACT)))
	return 0;
    }
    if(!(flags & NO_MAX_ARGS))
       max_correct_args=0x7fffffff;
    /* check the returntype */
    a = a->cdr;
    b = b->cdr;
    if ((b->type == T_VOID) && (a->type != T_VOID)) {
      /* Promote b to a function returning zero. */
      if (!low_match_types(a, zero_type_string, flags & ~(A_EXACT|B_EXACT)))
	return 0;
    } else if ((a->type == T_VOID) && (b->type != T_VOID)) {
      /* Promote a to a function returning zero. */
      if(!low_match_types(zero_type_string, b, flags & ~(A_EXACT|B_EXACT)))
	return 0;
    } else if(!low_match_types(a, b, flags & ~(A_EXACT|B_EXACT))) return 0;
    return ret;
  }

  if(a->type != b->type) return 0;

  ret=a;
  switch(a->type)
  {
  case T_MAPPING:
    if(!low_match_types(a->car, b->car,
			flags & ~(A_EXACT|B_EXACT))) return 0;
    if(!low_match_types(a->cdr, b->cdr,
			flags & ~(A_EXACT|B_EXACT))) return 0;
    break;

  case T_OBJECT:
#if 0
    if(a->cdr || b->cdr)
    {
      fprintf(stderr,"Type match1: ");
      stupid_describe_type(a,type_length(a));
      fprintf(stderr,"Type match2: ");
      stupid_describe_type(b,type_length(b));
    }
#endif

    /* object(* 0) matches any object */
    if(!a->cdr || !b->cdr) break;

    /* object(x *) =? object(x *) */
    if(TEST_COMPAT(7,4) && a->car == b->car)
    {
      /* x? */
      if(a->car)
      {
       /* object(1 x) =? object(1 x) */
       if(a->cdr != b->cdr) return 0;
      }else{
       /* object(0 *) =? object(0 *) */
       break;
      }
    }

    {
      struct program *ap,*bp;
      ap = id_to_program(CDR_TO_INT(a));
      bp = id_to_program(CDR_TO_INT(b));

      if(!ap || !bp) break;

      implements_mode = 0;

      if (TEST_COMPAT(7,4)) {
	if(a->car)
	{
	  if (!is_compatible(implements_a=ap,implements_b=bp))
	    return 0;
	}else{
	  if(!is_compatible(implements_a=bp,implements_b=ap))
	    return 0;
	}
      }
      else {
	if (!is_compatible(implements_a=ap,implements_b=bp))
	  return 0;
      }
    }
    break;

  case T_INT:
  {
    INT32 amin = CAR_TO_INT(a);
    INT32 amax = CDR_TO_INT(a);

    INT32 bmin = CAR_TO_INT(b);
    INT32 bmax = CDR_TO_INT(b);
    
    if(amin > bmax || bmin > amax) return 0;
    break;
  }
    

  case T_PROGRAM:
  case T_TYPE:
  case T_MULTISET:
  case T_ARRAY:
    if(!low_match_types(a->car, b->car,
			flags & ~(A_EXACT|B_EXACT))) return 0;

  case T_FLOAT:
  case T_STRING:
  case T_ZERO:
  case T_VOID:
  case T_MIXED:
    break;

  default:
    Pike_fatal("Error in type string.\n");
  }
  return ret;
}

/*
 * Check the partial ordering relation.
 *
 *                 mixed
 *
 * int float string program function object
 *
 *                 zero
 *
 *                 void
 *
 * Note that non-destructive operations are assumed.
 * ie it's assumed that calling a function(mapping(string|int:string|int):void)
 * with a mapping(int:int) won't change the type of the mapping after the
 * operation.
 */
static int low_pike_types_le(struct pike_type *a, struct pike_type *b,
			     int array_cnt, unsigned int flags)
#ifdef PIKE_TYPE_DEBUG
{
  int e;
  char *s;
  static int low_pike_types_le2(struct pike_type *a, struct pike_type *b,
				int array_cnt, unsigned int flags);
  int res;
  char buf[50];

  if (l_flag>2) {
    dynamic_buffer save_buf;
    init_buf(&save_buf);
    for(e=0;e<indent;e++) my_strcat("  ");
    my_strcat("low_pike_types_le(");
    my_describe_type(a);
    my_strcat(",\n");
    for(e=0;e<indent;e++) my_strcat("  ");
    my_strcat("                ");
    my_describe_type(b);
    my_strcat(",\n");
    for(e=0;e<indent;e++) my_strcat("  ");
    my_strcat("                ");
    sprintf(buf, "%d", array_cnt);
    my_strcat(buf);
    my_strcat(", ");
    sprintf(buf, "0x%08x", flags);
    my_strcat(buf);
    my_strcat(");\n");
    fprintf(stderr,"%s",(s=simple_free_buf(&save_buf)));
    free(s);
    indent++;
  }

  res = low_pike_types_le2(a, b, array_cnt, flags);

  if (l_flag>2) {
    indent--;

    for(e=0;e<indent;e++) fprintf(stderr, "  ");
    fprintf(stderr, "= %d\n", res);
  }
  return res;
}

static int low_pike_types_le2(struct pike_type *a, struct pike_type *b,
			      int array_cnt, unsigned int flags)
#endif /* PIKE_TYPE_DEBUG */

{
  int ret;

 recurse:
#if 0
  fprintf(stderr, "low_pike_types_le(%d, %d, %d, 0x%08x)\n",
	  a->type, b->type, array_cnt, flags);
#endif /* 0 */

  if(a == b) return 1;

  switch(a->type)
  {
  case T_AND:
    /* OK if either of the parts is a subset. */
    /* FIXME: What if b also contains an AND? */
    ret = low_pike_types_le(a->car, b, array_cnt, flags);
    if(ret) return ret;
    a = a->cdr;
    goto recurse;

  case T_OR:
    /* OK, if both of the parts are a subset */
    if (a->car->type == T_VOID) {
      /* Special case for T_VOID */
      /* FIXME: Should probably be handled as T_ZERO. */
      a = a->cdr;
      goto recurse;
    } else {
      ret = low_pike_types_le(a->car, b, array_cnt, flags);
      if (!ret) return 0;
      if (a->cdr->type == T_VOID) {
	/* Special case for T_VOID */
	/* FIXME: Should probably be handled as T_ZERO. */
	return 1;
      } else {
	a = a->cdr;
	goto recurse;
      }
    }

  case PIKE_T_RING:
    a = a->car;
    goto recurse;

  case PIKE_T_NAME:
    a = a->cdr;
    goto recurse;

  case T_NOT:
    if (b->type == T_NOT) {
      struct pike_type *tmp = a->car;
      a = b->car;
      b = tmp;
      array_cnt = -array_cnt;
      goto recurse;
    }
    if (a->car->type == T_NOT) {
      a = a->car->car;
      goto recurse;
    }
    if (low_pike_types_le(a->car, b, array_cnt, flags)) {
      return 0;
    }
    /* FIXME: This is wrong... */
    return !low_pike_types_le(b, a->car, -array_cnt, flags);

  case T_ASSIGN:
    ret = low_pike_types_le(a->cdr, b, array_cnt, flags);
    if(ret && (b->type != T_VOID))
    {
      int m = CAR_TO_INT(a);
      struct pike_type *tmp;
      int i;
      type_stack_mark();
      push_finished_type_with_markers(b, b_markers);
      for(i=array_cnt; i > 0; i--)
	push_type(T_ARRAY);
      tmp=pop_unfinished_type();
      
      type_stack_mark();
      low_or_pike_types(a_markers[m], tmp, 0);
      if(a_markers[m]) free_type(a_markers[m]);
      free_type(tmp);
      a_markers[m] = pop_unfinished_type();
#ifdef PIKE_TYPE_DEBUG
      if (l_flag>2) {
	dynamic_buffer save_buf;
	char *s;
	int e;
	init_buf(&save_buf);
	for(e=0;e<indent;e++) my_strcat("  ");
	my_strcat("a_markers[");
	my_putchar((char)(m+'0'));
	my_strcat("]=");
	my_describe_type(a_markers[m]);
	my_strcat("\n");
	fprintf(stderr,"%s",(s=simple_free_buf(&save_buf)));
	free(s);
      }
#endif
    }
    return ret;

    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
    {
      int m = a->type - '0';
      if(a_markers[m]) {
	a = a_markers[m];
      } else {
	a = mixed_type_string;
      }
      goto recurse;
    }
  }

  switch(b->type)
  {
  case T_AND:
    /* OK, if a is a subset of both parts. */
    ret = low_pike_types_le(a, b->car, array_cnt, flags);
    if(!ret) return 0;
    b = b->cdr;
    goto recurse;

  case T_OR:
    /* OK if a is a subset of either of the parts. */
    ret=low_pike_types_le(a, b->car, array_cnt, flags);
    if (ret) return ret;
    b = b->cdr;
    goto recurse;

  case PIKE_T_RING:
    b = b->car;
    goto recurse;

  case PIKE_T_NAME:
    b = b->cdr;
    goto recurse;

  case T_NOT:
    if (b->car->type == T_NOT) {
      b = b->car->car;
      goto recurse;
    }
    if (low_pike_types_le(a, b->car, array_cnt, flags)) {
      return 0;
    }
    /* FIXME: This is wrong... */
    return !low_pike_types_le(b->car, a, -array_cnt, flags);

  case T_ASSIGN:
    ret = low_pike_types_le(a, b->cdr, array_cnt, flags);
    if(ret && (a->type != T_VOID))
    {
      int m = CAR_TO_INT(b);
      struct pike_type *tmp;
      int i;
      type_stack_mark();
      push_finished_type_with_markers(a, a_markers);
      for(i = array_cnt; i < 0; i++)
	push_type(T_ARRAY);
      tmp=pop_unfinished_type();
      
      type_stack_mark();
      low_or_pike_types(b_markers[m], tmp, 0);
      if(b_markers[m]) free_type(b_markers[m]);
      free_type(tmp);
      b_markers[m]=pop_unfinished_type();
#ifdef PIKE_TYPE_DEBUG
      if (l_flag>2) {
	dynamic_buffer save_buf;
	char *s;
	int e;
	init_buf(&save_buf);
	for(e=0;e<indent;e++) my_strcat("  ");
	my_strcat("b_markers[");
	my_putchar((char)(m+'0'));
	my_strcat("]=");
	my_describe_type(b_markers[m]);
	my_strcat("\n");
	fprintf(stderr,"%s",(s=simple_free_buf(&save_buf)));
	free(s);
      }
#endif
    }
    return ret;

  case '0': case '1': case '2': case '3': case '4':
  case '5': case '6': case '7': case '8': case '9':
    {
      int m =  b->type - '0';
      if(b_markers[m]) {
	b = b_markers[m];
      } else {
	b = mixed_type_string;
      }
      goto recurse;
    }
  }

  if ((array_cnt < 0) && (b->type == T_ARRAY)) {
    while (b->type == T_ARRAY) {
      b = b->car;
      if (!++array_cnt) break;
    }
    goto recurse;
  } else if ((array_cnt > 0) && (a->type == T_ARRAY)) {
    while (a->type == T_ARRAY) {
      a = a->car;
      if (!--array_cnt) break;
    }
    goto recurse;
  }

  /* NOTE: void only matches void. */
  if (a->type == T_VOID) {
    /* void <= any_type */
    if (array_cnt >= 0) {
      /* !array(void) */
      if (!array_cnt && (b->type == T_VOID)) {
	return 1;
      }
      return 0;
    }
  }

  if (b->type == T_VOID) {
    if (array_cnt <= 0) {
      /* !array(void) */
      return 0;
    }
  }

  if (b->type == T_MIXED) {
    /* any_type <= 'mixed' */
    if (array_cnt <= 0) {
      /* !array(mixed) */
      return 1;
    }
  }

  if (a->type == T_MIXED) {
    if (array_cnt >= 0) {
      /* !array(mixed) */
      return 0;
    }
  }

  if (a->type == T_ZERO) {
    /* void <= zero <= any_type */
    if (array_cnt >= 0) {
      /* !array(zero) */
      return 1;
    }
  }

  if (b->type == T_ZERO) {
    if (array_cnt <= 0) {
      /* !array(zero) */
      return 0;
    }
  }

  /* Special cases (tm) */
  switch(TWOT(a->type, b->type))
  {
  case TWOT(T_PROGRAM, T_FUNCTION):
  case TWOT(T_FUNCTION, T_PROGRAM):
  case TWOT(T_PROGRAM, T_MANY):
  case TWOT(T_MANY, T_PROGRAM):
    /* FIXME: Not really... Should check the return value. */
    /* FIXME: Should also look at the subtype of the program. */
    return 1;

  case TWOT(T_OBJECT, T_FUNCTION):
  case TWOT(T_OBJECT, T_MANY):
    {
      if((a = low_object_lfun_type(a, LFUN_CALL))) {
	goto recurse;
      }
      return 1;
    }

  case TWOT(T_FUNCTION, T_OBJECT):
  case TWOT(T_MANY, T_OBJECT):
    {
      if((b=low_object_lfun_type(b, LFUN_CALL))) {
	goto recurse;
      }
      return 1;
    }

  case TWOT(T_FUNCTION, T_ARRAY):
  case TWOT(T_MANY, T_ARRAY):
    {
      while (b->type == T_ARRAY) {
	b = b->car;
	array_cnt++;
      }
      goto recurse;
    }

  case TWOT(T_ARRAY, T_FUNCTION):
  case TWOT(T_ARRAY, T_MANY):
    {
      while (a->type == T_ARRAY) {
	a = a->car;
	array_cnt--;
      }
      goto recurse;
    }

  case TWOT(T_FUNCTION, T_FUNCTION):
  case TWOT(T_FUNCTION, T_MANY):
  case TWOT(T_MANY, T_FUNCTION):
    /*
     * function(A...:B) <= function(C...:D)	iff C <= A && B <= D
     */
    /*
     * function(:int) <= function(int:int)
     * function(int|string:int) <= function(int:int)
     * function(:int) <= function(:void)
     * function(string:int) != function(int:int)
     * function(int:int) != function(:int)
     *
     * FIXME: Enforcing of all required arguments?
     */
    while((a->type != T_MANY) || (b->type != T_MANY))
    {
      struct pike_type *a_tmp, *b_tmp;

      a_tmp = a->car;
      if (a->type == T_FUNCTION)
      {
	a = a->cdr;
      }

      b_tmp = b->car;
      if (b->type == T_FUNCTION)
      {
	b = b->cdr;
      }

      if (a_tmp->type != T_VOID) {
	if (!low_pike_types_le(b_tmp, a_tmp, 0, flags)) {
	  return 0;
	}
      }
    }
    /* FALL_THROUGH */
  case TWOT(T_MANY, T_MANY):
    /* check the 'many' type */
    if ((a->car->type != T_VOID) && (b->car->type != T_VOID)) {
      if (!low_pike_types_le(b->car, a->car, 0, flags)) {
	return 0;
      }
    }

    a = a->cdr;
    b = b->cdr;

    /* check the returntype */
    /* FIXME: Check array_cnt */
    if ((b->type != T_VOID) && (a->type != T_VOID)) {
      if(!low_pike_types_le(a, b, array_cnt, flags)) return 0;
    }
    return 1;
  }

  if(a->type != b->type) return 0;

  if (array_cnt) return 0;

  switch(a->type)
  {
  case T_MAPPING:
    /*
     *  mapping(A:B) <= mapping(C:D)   iff A <= C && B <= D.
     */
    if(!low_pike_types_le(a->car, b->car, 0, flags)) return 0;
    array_cnt = 0;
    a = a->cdr;
    b = b->cdr;
    goto recurse;

  case T_OBJECT:
#if 0
    if(a->cdr || b->cdr)
    {
      fprintf(stderr,"Type match1: ");
      stupid_describe_type(a,type_length(a));
      fprintf(stderr,"Type match2: ");
      stupid_describe_type(b,type_length(b));
    }
#endif

    /*
     * object(0|1 x) <= object(0|1 0)
     * object(0|1 0) <=! object(0|1 !0)
     * object(1 x) <= object(0|1 x)
     * object(1 x) <= object(1 y)	iff x == y
     * object(1 x) <= object(0 y)	iff x implements y
     * Not WEAK_OBJECTS:
     *   object(0 x) <= object(0 y)	iff x implements y
     * WEAK_OBJECTS:
     *   object(0 x) <= object(0 y)	iff x is_compatible y
     */

    /* object(* 0) matches any object */
    if(!b->cdr)
      return 1;

    if(!a->cdr)
      return 0;

    /* The 'is' flag is now ignored.
     *	/grubba 2003-11-11
     */

    if (TEST_COMPAT(7,4)) {
      if ((a->car || !b->car) &&
	  (a->cdr == b->cdr))
	return 1;

      if (b->car) {
	return 0;
      }
    }
    else {
      if (a->cdr == b->cdr)
	return 1;
    }

    {
      struct program *ap = id_to_program(CDR_TO_INT(a));
      struct program *bp = id_to_program(CDR_TO_INT(b));

      if (!ap || !bp) {
	/* Shouldn't happen... */
	return 0;
      }
      if ((flags & LE_WEAK_OBJECTS) &&
	  (!TEST_COMPAT(7,4) || (!a->car))) {
	implements_mode = 0;
	return is_compatible(implements_a=ap, implements_b=bp);
      }
      implements_mode = 1;
      return implements(implements_a=ap, implements_b=bp);
    }
    break;

  case T_INT:
  {
    INT32 amin = CAR_TO_INT(a);
    INT32 amax = CDR_TO_INT(a);

    INT32 bmin = CAR_TO_INT(b);
    INT32 bmax = CDR_TO_INT(b);
    
    if(amin < bmin || amax > bmax) return 0;
    break;
  }
    

  case T_TYPE:
  case T_PROGRAM:
  case T_MULTISET:
  case T_ARRAY:
    a = a->car;
    b = b->car;
    array_cnt = 0;
    goto recurse;

  case T_FLOAT:
  case T_STRING:
  case T_ZERO:
  case T_VOID:
  case T_MIXED:
    break;

  default:
    Pike_fatal("Error in type string.\n");
  }
  return 1;
}

/*
 * Check the function parameters.
 * Note: The difference between this function, and pike_types_le()
 *       is the more lenient check for T_OR, and the handling of T_ARRAY.
 */
int strict_check_call(struct pike_type *fun_type,
		      struct pike_type *arg_type)
{
  while ((fun_type->type == T_OR) ||
	 (fun_type->type == T_ARRAY)) {
    if (fun_type->type == T_OR) {
      int res = strict_check_call(fun_type->car, arg_type);
      if (res) return res;
      fun_type = fun_type->cdr;
    } else {
      fun_type = fun_type->car;
    }
  }
  return low_pike_types_le(fun_type, arg_type, 0, 0);
}

/*
 * Check validity of soft-cast.
 * Note: This uses a weaker check of function arguments, since
 *       people get confused otherwise.
 */
int check_soft_cast(struct pike_type *to, struct pike_type *from)
{
  return low_pike_types_le(to, from, 0, LE_WEAK_OBJECTS);
}

/*
 * Return the return type from a function call.
 */
static int low_get_return_type(struct pike_type *a, struct pike_type *b)
{
  int tmp;
  switch(a->type)
  {
  case T_OR:
    {
      struct pike_type *o1, *o2;
      o1=o2=0;

      type_stack_mark();
      if(low_get_return_type(a->car, b)) 
      {
	o1=pop_unfinished_type();
	type_stack_mark();
      }

      if(low_get_return_type(a->cdr, b))
	o2=pop_unfinished_type();
      else
	pop_stack_mark();

      if(!o1 && !o2) return 0;

      low_or_pike_types(o1, o2, 0);

      if(o1) free_type(o1);
      if(o2) free_type(o2);

      return 1;
    }

  case T_AND:
    type_stack_mark();
    tmp = low_get_return_type(a->car, b);
    type_stack_pop_to_mark();
    if(!tmp) return 0;
    return low_get_return_type(a->cdr, b);

  case PIKE_T_RING:
    return low_get_return_type(a->car, b);

  case PIKE_T_NAME:
    return low_get_return_type(a->cdr, b);

  case T_ARRAY:
    tmp = low_get_return_type(a->car, b);
    if(!tmp) return 0;
    push_type(T_ARRAY);
    return 1;
  }

  a = low_match_types(a, b, NO_SHORTCUTS);
  if(a)
  {
#if 0
    if ((lex.pragmas & ID_STRICT_TYPES) &&
	!low_pike_types_le(a, b, 0, 0)) {
      yywarning("Type mismatch");
    }
#endif /* 0 */
    switch(a->type)
    {
    case T_FUNCTION:
      a = a->cdr;
      while(a->type == T_FUNCTION) {
	a = a->cdr;
      }
      /* FALL_THROUGH */
    case T_MANY:
      a = a->cdr;
      push_finished_type_with_markers(a, a_markers );
      return 1;

    case T_PROGRAM:
      push_finished_type(a->car);
      return 1;

    default:
      push_type(T_MIXED);
      return 1;
    }
  }
  return 0;
}


int match_types(struct pike_type *a, struct pike_type *b)
{
  check_type_string(a);
  check_type_string(b);
  clear_markers();
  return !!low_match_types(a, b, 0);
}

int pike_types_le(struct pike_type *a,struct pike_type *b)
{
  check_type_string(a);
  check_type_string(b);
  clear_markers();
  return low_pike_types_le(a, b, 0, 0);
}


#ifdef DEBUG_MALLOC
#define low_index_type(X,Y,Z) ((struct pike_type *)debug_malloc_pass(debug_low_index_type((X),(Y),(Z))))
#else
#define low_index_type debug_low_index_type
#endif

/* FIXME, add the index */
static struct pike_type *debug_low_index_type(struct pike_type *t,
					      struct pike_type *index_type,
					      node *n)
{
  struct pike_type *tmp;
  struct program *p;

  switch(low_check_indexing(t, index_type, n))
  {
    case 0: return 0;
    case -1:
      add_ref(zero_type_string);
      return zero_type_string;
  }

  while(t->type == PIKE_T_NAME) {
    t = t->cdr;
  }
  while(index_type->type == PIKE_T_NAME) {
    index_type = index_type->cdr;
  }

  switch(t->type)
  {
  case T_OBJECT:
  {
    p = id_to_program(CDR_TO_INT(t));

  comefrom_int_index:
    if(p && n)
    {
      INT32 i;
      if(n->token == F_ARROW)
      {
	/* FIXME: make this stricter */
	if((i=FIND_LFUN(p,LFUN_ARROW))!=-1)
	{
	  /* FIXME: function_type_string should be replaced with something
	   * derived from type_string
	   */
	  if(i!=-1 &&
	     (tmp=check_call(function_type_string, ID_FROM_INT(p, i)->type,
			     0)))
	    return tmp;

	  add_ref(mixed_type_string);
	  return mixed_type_string;
	}
      }else{
	if((i=FIND_LFUN(p,LFUN_INDEX)) != -1)
	{
	  /* FIXME: function_type_string should be replaced with something
	   * derived from type_string
	   */
	  if(i!=-1 &&
	     (tmp=check_call(function_type_string, ID_FROM_INT(p, i)->type,
			     0)))
	    return tmp;

	  add_ref(mixed_type_string);
	  return mixed_type_string;
	}
      }
      if(CDR(n)->token == F_CONSTANT && CDR(n)->u.sval.type==T_STRING)
      {
	i = find_shared_string_identifier(CDR(n)->u.sval.u.string, p);
	if(i==-1)
	{
	  add_ref(mixed_type_string);
	  return mixed_type_string;
	}else{
	  add_ref(ID_FROM_INT(p, i)->type);
	  return ID_FROM_INT(p, i)->type;
	}
      }
    }
  }
  default:
    add_ref(mixed_type_string);
    return mixed_type_string;

  case T_MIXED:
    if (lex.pragmas & ID_STRICT_TYPES) {
      yywarning("Indexing mixed.");
    }
    add_ref(mixed_type_string);
    return mixed_type_string;    

    case T_INT:
#ifdef AUTO_BIGNUM
      /* Don't force Gmp.mpz to be loaded here since this function
       * is called long before the master object is compiled...
       * /Hubbe
       */
      p=get_auto_bignum_program_or_zero();
      goto comefrom_int_index;
#endif
    case T_ZERO:
    case T_TYPE:
    case PIKE_T_RING:
    case T_VOID:
    case T_FLOAT:
      return 0;

  case T_OR:
  {
    struct pike_type *a,*b;
    a = low_index_type(t->car, index_type, n);
    b = low_index_type(t->cdr, index_type,n);
    if(!b) return a;
    if(!a) return b;
    type_stack_mark();
    low_or_pike_types(a,b,1);
    free_type(a);
    free_type(b);
    return pop_unfinished_type();
  }

  case T_AND:
    /* FIXME: Shouldn't both branches be looked at? */
    return low_index_type(t->cdr, index_type, n);

  case T_STRING: /* always int */
  case T_MULTISET: /* always int */
    add_ref(int_type_string);
    return int_type_string;

  case T_MAPPING:
    add_ref(t = t->cdr);
    return t;

  case T_ARRAY:
    {
      struct pike_type *a;

      if(low_pike_types_le(string_type_string, index_type, 0, 0) &&
	 (a = low_index_type(t->car, string_type_string, n))) {
	/* Possible to index the array with a string. */
	type_stack_mark();
	push_finished_type(a);
	free_type(a);
	push_type(T_ARRAY);

	if (low_match_types(int_type_string, index_type, 0)) {
	  /* Also possible to index the array with an int. */
	  push_finished_type(t->car);
	  push_type(T_OR);
	}
	return pop_unfinished_type();
      }
      if (low_match_types(int_type_string, index_type, 0)) {
	/* Possible to index the array with an int. */
	add_ref(t->car);
	return t->car;
      }
      /* Bad index type. */
      return 0;
    }
  }
}

struct pike_type *index_type(struct pike_type *type,
			     struct pike_type *index_type,
			     node *n)
{
  struct pike_type *t;
  clear_markers();
  t = low_index_type(type, index_type, n);
  if(!t) {
    copy_pike_type(t, mixed_type_string);
  }
  return t;
}

#ifdef DEBUG_MALLOC
#define low_range_type(X,Y,Z) ((struct pike_type *)debug_malloc_pass(debug_low_range_type((X),(Y),(Z))))
#else
#define low_range_type debug_low_range_type
#endif

/* FIXME, add the index
 *
 * FIXME: Is the above fixme valid for this function too?
 */
static struct pike_type *debug_low_range_type(struct pike_type *t,
					      struct pike_type *index1_type,
					      struct pike_type *index2_type)
{
  struct pike_type *tmp;
  struct program *p;

  while(t->type == PIKE_T_NAME) {
    t = t->cdr;
  }
  while(index1_type->type == PIKE_T_NAME) {
    index1_type = index1_type->cdr;
  }
  while(index2_type->type == PIKE_T_NAME) {
    index2_type = index2_type->cdr;
  }

  switch(t->type)
  {
  case T_OBJECT:
  {
    p = id_to_program(CDR_TO_INT(t));

    if(p)
    {
      INT32 i;
      if((i = FIND_LFUN(p, LFUN_INDEX)) != -1)
      {
	struct pike_type *call_type = NULL;
	/* FIXME: function_type_string should be replaced with something
	 * derived from type_string
	 */
	type_stack_mark();
	push_finished_type(mixed_type_string);
	push_finished_type(void_type_string);
	push_type(T_OR);			/* Return type */
	push_finished_type(void_type_string);	/* Many type */
	push_type(T_MANY);
	push_finished_type(index2_type);	/* arg2 type */
	push_type(T_FUNCTION);
	push_finished_type(index1_type);	/* arg1 type */
	push_type(T_FUNCTION);
	call_type = pop_unfinished_type();
	
	if((tmp = check_call(call_type, ID_FROM_INT(p, i)->type, 0))) {
	  free_type(call_type);
	  return tmp;
	}

	add_ref(mixed_type_string);
	return mixed_type_string;
      }
      yywarning("Ranging object without index operator.");
      return 0;
    }
    if (lex.pragmas & ID_STRICT_TYPES) {
      yywarning("Ranging generic object.");
    }
    add_ref(mixed_type_string);
    return mixed_type_string;    
  }

  case T_MIXED:
    if (lex.pragmas & ID_STRICT_TYPES) {
      yywarning("Ranging mixed.");
    }
    add_ref(mixed_type_string);
    return mixed_type_string;    

  case T_INT:
  case T_ZERO:
  case T_TYPE:
  case PIKE_T_RING:
  case T_VOID:
  case T_FLOAT:
  case T_MULTISET:
  case T_MAPPING:
    /* Illegal range operation. */
    /* FIXME: Strict type warning. */
    return 0;

  case T_ARRAY:
  case T_STRING:
    /* Check that the index types are compatible with int. */
    {
      if (!low_match_types(int_type_string, index1_type, 0)) {
	struct pike_string *s = describe_type(t);
	yywarning("Bad argument 1 to range operator on %s.",
		  s->str);
	free_string(s);
	yyexplain_nonmatching_types(int_type_string, index1_type,
				    YYTE_IS_WARNING);
	/* Bad index1 type. */
	return 0;
      }
      if (!low_match_types(int_type_string, index2_type, 0)) {
	struct pike_string *s = describe_type(t);
	yywarning("Bad argument 2 to range operator on %s.",
		  s->str);
	free_string(s);
	yyexplain_nonmatching_types(int_type_string, index2_type,
				    YYTE_IS_WARNING);
	/* Bad index2 type. */
	return 0;
      }
    }
    /* FALLTHROUGH */
  default:
    /* Identity. */
    add_ref(t);
    return t;

  case T_OR:
  {
    struct pike_type *a,*b;
    a = low_range_type(t->car, index1_type, index2_type);
    b = low_range_type(t->cdr, index1_type, index2_type);
    if(!b) return a;
    if(!a) return b;
    type_stack_mark();
    low_or_pike_types(a,b,1);
    free_type(a);
    free_type(b);
    return pop_unfinished_type();
  }

  case T_AND:
    /* FIXME: Shouldn't both branches be looked at? */
    return low_range_type(t->cdr, index1_type, index2_type);
  }
}

struct pike_type *range_type(struct pike_type *type,
			     struct pike_type *index1_type,
			     struct pike_type *index2_type)
{
  struct pike_type *t;
  clear_markers();
  t = low_range_type(type, index1_type, index2_type);
  if(!t) {
    yyerror("Invalid range operation.");
    copy_pike_type(t, type);
  }
  return t;
}


static struct pike_type *low_array_value_type(struct pike_type *arr_t)
{
  struct pike_type *res = NULL;
  struct pike_type *sub_t;

  while (arr_t->type == T_OR) {
    sub_t = low_array_value_type(arr_t->car);
    arr_t = arr_t->cdr;
    if (sub_t) {
      if (res) {
	struct pike_type *new = or_pike_types(res, sub_t, 1);
	free_type(res);
	free_type(sub_t);
	res = new;
      } else {
	res = sub_t;
      }
    }
  }
  if (arr_t->type != T_ARRAY)
    return res;

  copy_pike_type(sub_t, arr_t->car);

  if (res) {
    struct pike_type *new = or_pike_types(res, sub_t, 1);
    free_type(res);
    free_type(sub_t);
    return new;
  }
  return sub_t;
}

struct pike_type *array_value_type(struct pike_type *array_type)
{
  struct pike_type *t = low_array_value_type(array_type);
  if (!t) {
    copy_pike_type(t, mixed_type_string);
  }
  return t;
}


#ifdef DEBUG_MALLOC
#define low_key_type(X,Y) ((struct pike_type *)debug_malloc_pass(debug_low_key_type((X),(Y))))
#else
#define low_key_type debug_low_key_type
#endif

/* FIXME, add the index */
static struct pike_type *debug_low_key_type(struct pike_type *t, node *n)
{
  switch(t->type)
  {
  case T_OBJECT:
  {
    struct program *p = id_to_program(CDR_TO_INT(t));
    if(p && n)
    {
      if(n->token == F_ARROW)
      {
	if(FIND_LFUN(p,LFUN_ARROW)!=-1 || FIND_LFUN(p,LFUN_ASSIGN_ARROW)!=-1)
	{
	  add_ref(mixed_type_string);
	  return mixed_type_string;
	}
      }else{
	if(FIND_LFUN(p,LFUN_INDEX) != -1 || FIND_LFUN(p,LFUN_ASSIGN_INDEX) != -1)
	{
	  add_ref(mixed_type_string);
	  return mixed_type_string;
	}
      }
    }
    add_ref(string_type_string);
    return string_type_string;
  }
  default:
    add_ref(mixed_type_string);
    return mixed_type_string;

    case T_VOID:
    case T_ZERO:
    case T_TYPE:
    case PIKE_T_RING:
    case T_FLOAT:
    case T_INT:
      return 0;

  case T_OR:
  {
    struct pike_type *a,*b;
    a = low_key_type(t->car, n);
    b = low_key_type(t->cdr, n);
    if(!b) return a;
    if(!a) return b;
    type_stack_mark();
    low_or_pike_types(a,b,1);
    free_type(a);
    free_type(b);
    return pop_unfinished_type();
  }

  case T_AND:
    /* FIXME: Shouldn't this look at both branches? */
    return low_key_type(t->cdr, n);

  case PIKE_T_NAME:
    return low_key_type(t->cdr, n);

  case T_ARRAY:
  case T_STRING: /* always int */
    add_ref(int_type_string);
    return int_type_string;

  case T_MAPPING:
  case T_MULTISET:
    copy_pike_type(t, t->car);
    return t;
  }
}

struct pike_type *key_type(struct pike_type *type, node *n)
{
  struct pike_type *t;
  clear_markers();
  t = low_key_type(type,n);
  if(!t) {
    copy_pike_type(t, mixed_type_string);
  }
  return t;
}



static int low_check_indexing(struct pike_type *type,
			      struct pike_type *index_type,
			      node *n)
{
  switch(type->type)
  {
  case T_OR:
    return low_check_indexing(type->car, index_type, n) ||
      low_check_indexing(type->cdr, index_type, n);

  case T_AND:
    return low_check_indexing(type->car, index_type, n) &&
      low_check_indexing(type->cdr, index_type, n);

  case T_NOT:
    return low_check_indexing(type->car, index_type, n) != 1;

  case PIKE_T_NAME:
    return low_check_indexing(type->cdr, index_type, n);

  case T_ARRAY:
    if(low_match_types(string_type_string, index_type, 0) &&
       low_check_indexing(type->car, index_type, n))
      return 1;

  case T_STRING:
    return !!low_match_types(int_type_string, index_type, 0);

  case T_OBJECT:
  {
    struct program *p = id_to_program(CDR_TO_INT(type));
    if(p)
    {
      if(n->token == F_ARROW)
      {
	if(FIND_LFUN(p,LFUN_ARROW)!=-1 || FIND_LFUN(p,LFUN_ASSIGN_ARROW)!=-1)
	  return 1;
      }else{
	if(FIND_LFUN(p,LFUN_INDEX)!=-1 || FIND_LFUN(p,LFUN_ASSIGN_INDEX)!=-1)
	  return 1;
      }
      return !!low_match_types(string_type_string, index_type,0);
    }else{
      return 1;
    }
  }

  case T_MULTISET:
  case T_MAPPING:
    /* FIXME: Why -1 and not 0? */
    return low_match_types(type->car, index_type, 0) ? 1 : -1;

#ifdef AUTO_BIGNUM
    case T_INT:
#endif
    case T_PROGRAM:
      return !!low_match_types(string_type_string, index_type, 0);

  case T_MIXED:
    return 1;
    
  default:
    return 0;
  }
}
				 
int check_indexing(struct pike_type *type,
		   struct pike_type *index_type,
		   node *n)
{
  check_type_string(type);
  check_type_string(index_type);

  return low_check_indexing(type, index_type, n);
}

static int low_count_arguments(struct pike_type *q)
{
  int num=0, num2;

  switch(q->type)
  {
    case T_OR:
      num = low_count_arguments(q->car);
      num2 = low_count_arguments(q->cdr);
      if(num<0 && num2>0) return num;
      if(num2<0 && num>0) return num2;
      if(num2<0 && num<0) return ~num>~num2?num:num2;
      return num>num2?num:num2;

    case T_AND:
      num = low_count_arguments(q->car);
      num2 = low_count_arguments(q->cdr);
      if(num<0 && num2>0) return num2;
      if(num2<0 && num>0) return num;
      if(num2<0 && num<0) return ~num<~num2?num:num2;
      return num<num2?num:num2;

    case PIKE_T_NAME:
      return low_count_arguments(q->cdr);

    default: return 0x7fffffff;

    case T_FUNCTION:
      while(q->type == T_FUNCTION)
      {
	num++;
	q = q->cdr;
      }
      /* FALL_THROUGH */
    case T_MANY:
      q = q->car;
      if(q->type != T_VOID) return ~num;
      return num;
  }
}

/* Count the number of arguments for a funciton type.
 * return -1-n if the function can take number of arguments
 * >= n  (varargs)
 */
int count_arguments(struct pike_type *s)
{
  check_type_string(s);

  return low_count_arguments(s);
}


static int low_minimum_arguments(struct pike_type *q)
{
  int num;

  switch(q->type)
  {
    case T_OR:
    case T_AND:
      return MAXIMUM(low_count_arguments(q->car),
		     low_count_arguments(q->cdr));

    default: return 0;

    case PIKE_T_NAME:
      return low_minimum_arguments(q->cdr);

    case T_FUNCTION:
      num = 0;
      while(q->type == T_FUNCTION)
      {
	if(low_match_types(void_type_string, q->car, B_EXACT))
	  return num;

	num++;
	q = q->cdr;
      }
      return num;
    case T_MANY:
      return 0;
  }
}

/* Count the minimum number of arguments for a funciton type.
 */
int minimum_arguments(struct pike_type *s)
{
  int ret;
  check_type_string(s);

  ret = low_minimum_arguments(s);

#if 0
  fprintf(stderr,"minimum_arguments(");
  simple_describe_type(s);
  fprintf(stderr," ) -> %d\n",ret);
#endif

  return ret;
}

struct pike_type *check_call(struct pike_type *args,
			     struct pike_type *type,
			     int strict)
{
  check_type_string(args);
  check_type_string(type);
  clear_markers();
  type_stack_mark();
  max_correct_args=0;
  
  if(low_get_return_type(type, args))
  {
    if (strict) {
      if (!strict_check_call(type, args)) {
	struct pike_string *type_t = describe_type(type);
	struct pike_type *func_zero_type;

	MAKE_CONSTANT_TYPE(func_zero_type, tFuncV(tNone,tZero,tMix));

	if (!low_pike_types_le(type, func_zero_type, 0, 0)) {
	  yywarning("Calling non-function value.");
	  yywarning("Type called: %s", type_t->str);
	} else {
	  struct pike_string *arg_t = describe_type(args);
	  yywarning("Arguments not strictly compatible.");
	  yywarning("Expected: %s", type_t->str);
	  yywarning("Got     : %s", arg_t->str);
	  free_string(arg_t);
	}

	free_type(func_zero_type);
	free_string(type_t);
      }
    }
    return pop_unfinished_type();
  }else{
    pop_stack_mark();
    return 0;
  }
}

/* NOTE: type loses a reference. */
struct pike_type *new_check_call(node *fun, int *argno,
				 struct pike_type *type, node *args)
{
  struct pike_type *tmp_type = NULL;

  while (args && (args->token == F_ARG_LIST)) {
    type = new_check_call(fun, argno, type, CAR(args));
    args = CDR(args);
  }
  if (!args) {
    return type;
  }

  switch(type->type) {
  case T_NOT:
    break;

  case T_FUNCTION:
    if (!pike_types_le(args->type, type->car)) {
      if (!match_types(args->type, type->car)) {
	/* Bad arg */
      } else {
	/* Not strict arg */
      }
    }
    copy_pike_type(tmp_type, type->cdr);
    free_type(type);
    type = tmp_type;
    (*argno)++;
    break;

  case T_MANY:
    if (!pike_types_le(args->type, type->car)) {
      if (!match_types(args->type, type->car)) {
	/* Bad arg */
      } else {
	/* Not strict arg */
      }
    }
    (*argno)++;
    break;
  }

  return type;
}

struct pike_type *zzap_function_return(struct pike_type *a, INT32 id)
{
  switch(a->type)
  {
    case T_OR:
    {
      struct pike_type *ar, *br, *ret=0;
      ar = zzap_function_return(a->car, id);
      br = zzap_function_return(a->cdr, id);
      if(ar && br) ret = or_pike_types(ar, br, 0);
      if(ar) free_type(ar);
      if(br) free_type(br);
      return ret;
    }
      
    case T_FUNCTION:
    case T_MANY:
    {
      int nargs=0;
      type_stack_mark();
      
      while(a->type == T_FUNCTION)
      {
	push_finished_type(a->car);
	nargs++;
	a = a->cdr;
      }
      push_finished_type(a->car);
      push_object_type(1, id);
      push_reverse_type(T_MANY);
      while(nargs-- > 0) {
	push_reverse_type(T_FUNCTION);
      }
      return pop_unfinished_type();
    }

    case T_ARRAY:
      return zzap_function_return(a->car, id);

    case PIKE_T_NAME:
      return zzap_function_return(a->cdr, id);

    case T_MIXED:
      /* I wonder when this occurrs, but apparently it does... */
      /* FIXME: */
      type_stack_mark();
      push_object_type(1, id);
      push_type(T_VOID);
      push_type(T_MIXED);
      push_type(T_OR);
      push_type(T_MANY);
      return pop_unfinished_type();
  }
/* This error is bogus /Hubbe
  Pike_fatal("zzap_function_return() called with unexpected value: %d\n",
	EXTRACT_UCHAR(a));
*/
  return NULL;
}

struct pike_type *get_type_of_svalue(struct svalue *s)
{
  struct pike_type *ret;
  switch(s->type)
  {
  case T_FUNCTION:
    if(s->subtype == FUNCTION_BUILTIN)
    {
      copy_pike_type(ret, s->u.efun->type);
    }else{
      struct program *p;

      p=s->u.object->prog;
      if(!p)
      {
	copy_pike_type(ret, zero_type_string);
      }else{
	copy_pike_type(ret, ID_FROM_INT(p,s->subtype)->type);
      }
    }
    return ret;
       
  case T_ARRAY:
    {
      struct pike_type *arg_type;
      struct array *a = s->u.array;
#if 0
      int i;

      /* FIXME: Circular structures? */
      copy_pike_type(arg_type, zero_type_string);
      for (i = 0; i < a->size; i++) {
	struct pike_type *tmp1 = get_type_of_svalue(a->item+i);
	struct pike_type *tmp2 = or_pike_types(arg_type, tmp1, 1);
	free_type(arg_type);
	free_type(tmp1);
	arg_type = tmp2;
      }
#else /* !0 */
      if (a->size)
	copy_pike_type(arg_type, mixed_type_string);
      else
	copy_pike_type(arg_type, zero_type_string);
#endif /* 0 */
      type_stack_mark();
      push_finished_type(arg_type);
      free_type(arg_type);
      push_type(s->type);
      return pop_unfinished_type();
    }

  case T_MULTISET:
    type_stack_mark();
    if (multiset_sizeof(s->u.multiset)) {
      push_type(T_MIXED);
    }
    else {
      push_type(T_ZERO);
    }
    push_type(T_MULTISET);
    return pop_unfinished_type();

  case T_MAPPING:
    type_stack_mark();
    if (m_sizeof(s->u.mapping)) {
      push_type_field(s->u.mapping->data->val_types);
      push_type_field(s->u.mapping->data->ind_types);
    }
    else {
      push_type(T_ZERO);
      push_type(T_ZERO);
    }
    push_type(T_MAPPING);
    return pop_unfinished_type();

  case T_OBJECT:
    type_stack_mark();
    if(s->u.object->prog)
    {
#ifdef AUTO_BIGNUM
      if(is_bignum_object(s->u.object))
      {
	push_int_type(MIN_INT32, MAX_INT32);
      }
      else
#endif
      {
	push_object_type(1, s->u.object->prog->id);
      }
    }else{
      /* Destructed object */
      push_type(T_ZERO);
    }
    return pop_unfinished_type();

  case T_INT:
    if(s->u.integer)
    {
      type_stack_mark();
      /* Fixme, check that the integer is in range of MIN_INT32 .. MAX_INT32!
       */
      push_int_type(s->u.integer, s->u.integer);
      return pop_unfinished_type();
    }else{
      copy_pike_type(ret, zero_type_string);
      return ret;
    }

  case T_PROGRAM:
  {
    /* FIXME: An alternative would be to push program(object(1,p->id)). */
    struct pike_type *a;
    struct pike_string *tmp;
    int id;

    if(s->u.program->identifiers)
    {
      id=FIND_LFUN(s->u.program,LFUN_CREATE);
      if(id>=0)
      {
	a = ID_FROM_INT(s->u.program, id)->type;
	if((a = zzap_function_return(a, s->u.program->id)))
	  return a;
	tmp=describe_type(ID_FROM_INT(s->u.program, id)->type);
	/* yywarning("Failed to zzap function return for type: %s.", tmp->str);*/
	free_string(tmp);
      }
      if (!(s->u.program->flags & PROGRAM_PASS_1_DONE)) {
	/* We haven't added all identifiers in s->u.program yet,
	 * so we might find a create() later.
	 */
	if((a = zzap_function_return(function_type_string, s->u.program->id)))
	  return a;
      }
    } else {
      if((a = zzap_function_return(function_type_string, s->u.program->id)))
	return a;
    }

    type_stack_mark();
    push_object_type(1, s->u.program->id);
    push_type(T_VOID);
    push_type(T_MANY);
    return pop_unfinished_type();
  }

  case T_TYPE:
    type_stack_mark();
    push_finished_type(s->u.type);
    push_type(T_TYPE);
    return pop_unfinished_type();

  default:
    type_stack_mark();
    push_type(s->type);
    return pop_unfinished_type();
  }
}


static struct pike_type *low_object_type_to_program_type(struct pike_type *obj_t)
{
  struct pike_type *res = NULL;
  struct pike_type *sub;
  struct svalue sval;
  int id;

  while(obj_t->type == T_OR) {
    sub = low_object_type_to_program_type(obj_t->car);
    if (!sub) {
      if (res) {
	free_type(res);
      }
      return NULL;
    }
    if (res) {
      struct pike_type *tmp = or_pike_types(res, sub, 1);
      free_type(res);
      free_type(sub);
      res = tmp;
    } else {
      res = sub;
    }
    obj_t = obj_t->cdr;
  }
  sval.type = T_PROGRAM;
  if ((obj_t->type != T_OBJECT) ||
      (!(id = CDR_TO_INT(obj_t))) ||
      (!(sval.u.program = id_to_program(id))) ||
      (!(sub = get_type_of_svalue(&sval)))) {
    if (res) {
      free_type(res);
    }
    return NULL;
  }
  /* FIXME: obj_t->car should propagate to the return-type in sub. */
  if (res) {
    struct pike_type *tmp = or_pike_types(res, sub, 1);
    free_type(res);
    free_type(sub);
    return tmp;
  }
  return sub;
}

/* Used by fix_object_program_type() */
struct pike_type *object_type_to_program_type(struct pike_type *obj_t)
{
  return low_object_type_to_program_type(obj_t);
}



int type_may_overload(struct pike_type *type, int lfun)
{
  switch(type->type)
  {
    case T_ASSIGN:
      return type_may_overload(type->cdr, lfun);
      
    case T_FUNCTION:
    case T_MANY:
    case T_ARRAY:
      /* might want to check for `() */
      
    default:
      return 0;

    case PIKE_T_NAME:
      return type_may_overload(type->cdr, lfun);

    case PIKE_T_RING:
      return type_may_overload(type->car, lfun);

    case T_OR:
      return type_may_overload(type->car, lfun) ||
	type_may_overload(type->cdr, lfun);
      
    case T_AND:
      return type_may_overload(type->car, lfun) &&
	type_may_overload(type->cdr, lfun);
      
    case T_NOT:
      return !type_may_overload(type->car, lfun);

    case T_MIXED:
      return 1;
      
    case T_OBJECT:
    {
      struct program *p = id_to_program(CDR_TO_INT(type));
      if(!p) return 1;
      return FIND_LFUN(p, lfun)!=-1;
    }
  }
}


void yyexplain_nonmatching_types(struct pike_type *type_a,
				 struct pike_type *type_b,
				 int flags)
{
  DECLARE_CYCLIC();

  implements_a=0;
  implements_b=0;
  implements_mode=0;

  match_types(type_a, type_b);

#if 0
  if(!(implements_a && implements_b &&
       type_a->str[0]==T_OBJECT &&
       type_b->str[0]==T_OBJECT))
#endif /* 0 */
  {
    struct pike_string *s1, *s2;
    s1 = describe_type(type_a);
    s2 = describe_type(type_b);
    if(flags & YYTE_IS_WARNING)
    {
      yywarning("Expected: %s",s1->str);
      yywarning("Got     : %s",s2->str);
    }else{
      my_yyerror("Expected: %s",s1->str);
      my_yyerror("Got     : %s",s2->str);
    }
    free_string(s1);
    free_string(s2);
  }

  /* Protect against circularities. */
  if (BEGIN_CYCLIC(type_a, type_b)) {
    END_CYCLIC();
    return;
  }
  SET_CYCLIC_RET(1);

  if(implements_a && implements_b) {
    if (implements_mode) {
      yyexplain_not_implements(implements_a, implements_b, flags);
    } else {
      yyexplain_not_compatible(implements_a, implements_b, flags);
    }
  }
  END_CYCLIC();
}


/******/

#ifdef DEBUG_MALLOC
#define low_make_pike_type(T,C) ((struct pike_type *)debug_malloc_pass(debug_low_make_pike_type(T,C)))
#define low_make_function_type(T,C) ((struct pike_type *)debug_malloc_pass(debug_low_make_function_type(T,C)))
#else /* !DEBUG_MALLOC */
#define low_make_pike_type debug_low_make_pike_type
#define low_make_function_type debug_low_make_function_type
#endif /* DEBUG_MALLOC */
  
static struct pike_type *debug_low_make_pike_type(unsigned char *type_string,
						  unsigned char **cont);

static struct pike_type *debug_low_make_function_type(unsigned char *type_string,
						      unsigned char **cont)
{
  struct pike_type *tmp;

  if (*type_string == T_MANY) {
    tmp = low_make_pike_type(type_string+1, cont);
    return mk_type(T_MANY, tmp,
		   low_make_pike_type(*cont, cont), PT_COPY_BOTH);
  }
  tmp = low_make_pike_type(type_string, cont);
  return mk_type(T_FUNCTION, tmp,
		 low_make_function_type(*cont, cont), PT_COPY_BOTH);
}

static struct pike_type *debug_low_make_pike_type(unsigned char *type_string,
						  unsigned char **cont)
{
  unsigned INT32 type;
  struct pike_type *tmp;

  switch(type = *type_string) {
  case T_SCOPE:
  case T_ASSIGN:
    if ((type_string[1] < '0') || (type_string[1] > '9')) {
      Pike_fatal("low_make_pike_type(): Bad marker: %d\n", type_string[1]);
    }
    return mk_type(type, (void *)(ptrdiff_t)(type_string[1] - '0'),
		   low_make_pike_type(type_string+2, cont), PT_COPY_CDR);
  case T_FUNCTION:
    /* Multiple ordered values. */
    /* FIXME: */
    return low_make_function_type(type_string+1, cont);
  case T_TUPLE:
  case T_MAPPING:
  case PIKE_T_RING:
    /* Order dependant */
    tmp = low_make_pike_type(type_string+1, cont);
    return mk_type(type, tmp,
		   low_make_pike_type(*cont, cont), PT_COPY_BOTH);
  case T_OR:
  case T_AND:
    /* Order independant */
    /* FIXME: */
    tmp = low_make_pike_type(type_string+1, cont);
    return mk_type(type, tmp,
		   low_make_pike_type(*cont, cont), PT_COPY_BOTH);
  case T_ARRAY:
  case T_MULTISET:
  case T_TYPE:
  case T_NOT:
  case T_PROGRAM:
    /* Single type */
    return mk_type(type, low_make_pike_type(type_string+1, cont), NULL,
		   PT_COPY_CAR);
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
    /* Marker type */
    *cont = type_string+1;
    return mk_type(type, NULL, NULL, PT_SET_MARKER);

  case T_FLOAT:
  case T_STRING:
  case T_MIXED:
  case T_VOID:
  case T_ZERO:
  case PIKE_T_UNKNOWN:
    /* Leaf type */
    *cont = type_string+1;
    return mk_type(type, NULL, NULL, 0);

  case T_INT:
    *cont = type_string + 9;	/* 2*sizeof(INT32) + 1 */
    return mk_type(T_INT,
		   (void *)(ptrdiff_t)extract_type_int(type_string+1),
		   (void *)(ptrdiff_t)extract_type_int(type_string+5), 0);
  case T_OBJECT:
    *cont = type_string + 6;	/* 1 + sizeof(INT32) + 1 */
    return mk_type(T_OBJECT, (void *)(ptrdiff_t)(type_string[1]),
		   (void *)(ptrdiff_t)extract_type_int(type_string+2), 0);
  case PIKE_T_NAME:
    {
      int size_shift = type_string[1] & 0x3;
      struct pike_string *str;
      INT32 bytes;
      /* bit 0 & 1: size_shift
       * bit 2 ==> little endian.
       *
       * The loops check the lsb first, since it's most likely to
       * be non-zero.
       */
      switch(type_string[1]) {
      case 0: case 4:
	bytes = strlen(type_string+2);
	break;
      case 1:
	for(bytes=0; ; bytes+=2)
	  if(!type_string[bytes+3] && !type_string[bytes+2])
	    break;
	break;
      case 5:
	for(bytes=0; ; bytes+=2)
	  if(!type_string[bytes+2] && !type_string[bytes+3])
	    break;
	break;
      case 2:
	for(bytes=0; ; bytes+=4)
	  if(!type_string[bytes+5] && !type_string[bytes+4] &&
	     !type_string[bytes+3] && !type_string[bytes+2])
	    break;
	break;
      case 6:
	for(bytes=0; ; bytes+=4)
	  if(!type_string[bytes+2] && !type_string[bytes+3] &&
	     !type_string[bytes+4] && !type_string[bytes+5])
	    break;
	break;
      default: /* will not happen? */
	 bytes=0;
	 Pike_fatal("unexpected case in make_pike_type (%d)\n",
		    type_string[1]);
      }
      str = begin_wide_shared_string(bytes>>size_shift, size_shift);
      MEMCPY(str->str, type_string+2, bytes);
      if (size_shift &&
#if (PIKE_BYTEORDER == 1234)
	  /* Little endian */
	  !(type_string[1] & 0x04)
#else /* PIKE_BYTEORDER != 1234 */
	  /* Big endian */
	  (type_string[1] & 0x04)
#endif /* PIKE_BYTEORDER == 1234 */
	  ) {
	int len;
	char tmp;
	if (size_shift == 1) {
	  for (len = 0; len < bytes; len += 2) {
	    tmp = str->str[len];
	    str->str[len] = str->str[len+1];
	    str->str[len+1] = tmp;
	  }
	} else {
	  for (len = 0; len < bytes; len += 4) {
	    tmp = str->str[len];
	    str->str[len] = str->str[len+3];
	    str->str[len+3] = tmp;
	    tmp = str->str[len+1];
	    str->str[len+1] = str->str[len+2];
	    str->str[len+2] = tmp;
	  }
	}
      }
      return mk_type(PIKE_T_NAME, (void *)end_shared_string(str),
		     low_make_pike_type(type_string + 2 + bytes +
					(1<<size_shift), cont),
		     PT_COPY_CDR);
    }
  default:
    Pike_fatal("compile_type_string(): Error in type string %d.\n", type);
    /* NOT_REACHED */
    break;
  }
  /* NOT_REACHED */
  return NULL;
}

/* Make a pike-type from a serialized (old-style) type. */
struct pike_type *debug_make_pike_type(const char *serialized_type)
{
  unsigned char *dummy;
  return low_make_pike_type((unsigned char *)serialized_type, &dummy);
}

int pike_type_allow_premature_toss(struct pike_type *type)
{
 again:
#if 0
  fprintf(stderr, "pike_type_allow_premature_toss(): Type: %d\n",
	  type->type);
#endif /* 0 */
  switch(type->type)
  {
    case T_NOT:
      return !pike_type_allow_premature_toss(type->car);

    case T_OBJECT:
    case T_MIXED:
    case T_FUNCTION:
    case T_MANY:
      return 0;

    case PIKE_T_NAME:
    case T_SCOPE:
    case T_ASSIGN:
      type = type->cdr;
      goto again;

    case PIKE_T_RING:
      type = type->car;
      goto again;

    case T_OR:
    case T_MAPPING:
      if(!pike_type_allow_premature_toss(type->car)) return 0;
      type = type->cdr;
      goto again;

    case T_AND:
      /* FIXME: Should probably look at both branches. */
      type = type->cdr;
      goto again;

    case T_ARRAY:
    case T_MULTISET:
      type = type->car;
      goto again;

    case T_PROGRAM:
    case T_TYPE:
    case T_INT:
    case T_FLOAT:
    case T_STRING:
    case T_VOID:
      return 1;
  default:
    Pike_fatal("pike_type_allow_premature_toss: Unknown type code (%d)\n",
	  ((unsigned char *)type)[-1]);
    /* NOT_REACHED */
    return 0;
  }
}

static void low_type_to_string(struct pike_type *t)
{
 recurse:
  switch(t->type) {
  case T_ARRAY:
  case T_MULTISET:
  case T_TYPE:
  case T_NOT:
  case T_PROGRAM:
    my_putchar(t->type);
    /* FALL_THROUGH */
  case PIKE_T_NAME:
    t = t->car;
    goto recurse;

  case PIKE_T_RING:
  case T_TUPLE:
  case T_MAPPING:
  case T_OR:
  case T_AND:
    my_putchar(t->type);
    low_type_to_string(t->car);
    t = t->cdr;
    goto recurse;

  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
  case T_STRING:
  case T_FLOAT:
  case T_ZERO:
  case T_VOID:
  case T_MIXED:
    my_putchar(t->type);
    break;

  case T_OBJECT:
    {
      INT32 i;
      my_putchar(T_OBJECT);
      i = (INT32)CAR_TO_INT(t);
      my_putchar( i );
      i = (INT32)CDR_TO_INT(t);

      if( i > 65535 )  i = 0; /* Not constant between recompilations */

      my_putchar((i >> 24) & 0xff);
      my_putchar((i >> 16) & 0xff);
      my_putchar((i >> 8)  & 0xff);
      my_putchar(i & 0xff);
    }
    break;

  case T_INT:
    {
      INT32 i;
      my_putchar(T_INT);
      i = (INT32)CAR_TO_INT(t);
      my_putchar((i >> 24) & 0xff);
      my_putchar((i >> 16) & 0xff);
      my_putchar((i >> 8) & 0xff);
      my_putchar(i & 0xff);
      i = (INT32)CDR_TO_INT(t);
      my_putchar((i >> 24) & 0xff);
      my_putchar((i >> 16) & 0xff);
      my_putchar((i >> 8) & 0xff);
      my_putchar(i & 0xff);
    }
    break;

  case T_FUNCTION:
  case T_MANY:
    my_putchar(T_FUNCTION);
    while(t->type == T_FUNCTION) {
      low_type_to_string(t->car);
      t = t->cdr;
    }
    my_putchar(T_MANY);
    low_type_to_string(t->car);
    t = t->cdr;
    goto recurse;

  case T_SCOPE:
  case T_ASSIGN:
    my_putchar(t->type);
    my_putchar('0' + CAR_TO_INT(t));
    t = t->cdr;
    goto recurse;

  default:
    Pike_error("low_type_to_string(): Unsupported node: %d\n", t->type);
    break;
  }
}

struct pike_string *type_to_string(struct pike_type *t)
{
  dynamic_buffer save_buf;
  init_buf(&save_buf);
  low_type_to_string(t);
  return free_buf(&save_buf);
}

void init_types(void)
{
  /* Initialize hashtable here. */
  pike_type_hash = (struct pike_type **)xalloc(sizeof(struct pike_type *) *
					       PIKE_TYPE_HASH_SIZE);
  MEMSET(pike_type_hash, 0, sizeof(struct pike_type *) * PIKE_TYPE_HASH_SIZE);
  pike_type_hash_size = PIKE_TYPE_HASH_SIZE;
  init_pike_type_blocks();

  string_type_string = CONSTTYPE(tString);
  int_type_string = CONSTTYPE(tInt);
  object_type_string = CONSTTYPE(tObj);
  program_type_string = CONSTTYPE(tPrg(tObj));
  float_type_string = CONSTTYPE(tFloat);
  mixed_type_string = CONSTTYPE(tMix);
  array_type_string = CONSTTYPE(tArray);
  multiset_type_string = CONSTTYPE(tMultiset);
  mapping_type_string = CONSTTYPE(tMapping);
  function_type_string = CONSTTYPE(tFunction);
  type_type_string = CONSTTYPE(tType(tMix));
  void_type_string = CONSTTYPE(tVoid);
  zero_type_string = CONSTTYPE(tZero);
  any_type_string = CONSTTYPE(tOr(tVoid,tMix));
  weak_type_string = CONSTTYPE(tOr4(tArray,tMultiset,tMapping,
				    tFuncV(tNone,tZero,tOr(tMix,tVoid))));
}

void cleanup_pike_types(void)
{
#ifdef DEBUG_MALLOC
  struct pike_type_location *t = all_pike_type_locations;

  while(t) {
    free_type(t->t);
    t = t->next;
  }
#endif /* DEBUG_MALLOC */

  free_type(string_type_string);
  free_type(int_type_string);
  free_type(float_type_string);
  free_type(function_type_string);
  free_type(object_type_string);
  free_type(program_type_string);
  free_type(array_type_string);
  free_type(multiset_type_string);
  free_type(mapping_type_string);
  free_type(type_type_string);
  free_type(mixed_type_string);
  free_type(void_type_string);
  free_type(zero_type_string);
  free_type(any_type_string);
  free_type(weak_type_string);
}

void cleanup_pike_type_table(void)
{
  /* Free the hashtable here. */
  if (pike_type_hash) {
    free(pike_type_hash);
    /* Don't do this, it messes up stuff...
     *
     * It's needed for dmalloc to survive.
     */
    pike_type_hash = NULL;
  }
  /* Don't do this, it messes up stuff...
   *
   * It's needed for dmalloc to survive.
   */
  pike_type_hash_size = 0;
#ifdef DO_PIKE_CLEANUP
  free_all_pike_type_blocks();
#endif /* DO_PIKE_CLEANUP */
}
