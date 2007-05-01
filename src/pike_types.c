/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: pike_types.c,v 1.304 2007/05/01 11:41:12 grubba Exp $
*/

#include "global.h"
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
#include "gc.h"
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
#define LE_A_B_SWAPPED	2	/* Argument A and B have been swapped.
				 * Relevant for markers.
				 */

/*
 * Flags used by low_get_first_arg_type()
 */
#define FILTER_KEEP_VOID 1	/* Keep void during the filtering. */

/*
 * Flags used as flag_method to mk_type()
 */
#define PT_COPY_CAR	1
#define PT_COPY_CDR	2
#define PT_COPY_BOTH	3
#define PT_IS_MARKER	4	/* The node is a marker. */

/* Number of entries in the struct pike_type hash-table. */
#define PIKE_TYPE_HASH_SIZE	32768


#ifdef PIKE_TYPE_DEBUG
static int indent=0;
#endif

int max_correct_args;

PMOD_EXPORT struct pike_type *string0_type_string;
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

#ifdef DO_PIKE_CLEANUP
struct pike_type_location *all_pike_type_locations = NULL;
#endif /* DO_PIKE_CLEANUP */

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

PMOD_EXPORT char *get_name_of_type(TYPE_T t)
{
  switch(t)
  {
    case T_ARRAY: return "array";
    case T_FLOAT: return "float";
    case T_MANY: case T_FUNCTION: return "function";
    case T_INT: return "int";
    case T_SVALUE_PTR: return "svalue_ptr";
    case T_OBJ_INDEX: return "obj_index";
    case T_MAPPING: return "mapping";
    case T_MULTISET: return "multiset";
    case T_OBJECT: return "object";
    case T_PROGRAM: return "program";
    case T_STRING: return "string";
    case PIKE_T_NSTRING: return "narrow_string";
    case T_TYPE: return "type";
    case T_ZERO: return "zero";
    case T_VOID: return "void";
    case T_MIXED: return "mixed";
    case T_STORAGE: return "object storage";
    case T_MAPPING_DATA: return "mapping_data";
    case T_PIKE_FRAME: return "pike_frame";
    case T_MULTISET_DATA: return "multiset_data";
    case T_STRUCT_CALLABLE: return "callable";
    case PIKE_T_GET_SET: return "getter/setter";
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
 * ATTRIBUTE	name (string)	type
 * FUNCTION	type		FUNCTION|MANY
 * MANY		many type	return type
 * RING		type		type
 * TUPLE	type		type
 * MAPPING	index type	value type
 * OR		type (not OR)	type
 * AND		type		type
 * ARRAY	type		-
 * MULTISET	type		-
 * NOT		type		-
 * '0'-'9'	-		-
 * FLOAT	-		-
 * STRING	bitwidth (int)	-			Width added in 7.7
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

    /* PIKE_DEBUG code */
    if (hash >= pike_type_hash_size) {
      Pike_fatal("Modulo operation failed for hash:%u, index:%u, size:%u.\n",
		 t->hash, hash, pike_type_hash_size);
    }
    /* End PIKE_DEBUG code */

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

    case PIKE_T_ATTRIBUTE:
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

static inline struct pike_type *debug_mk_type(unsigned INT32 type,
					      struct pike_type *car,
					      struct pike_type *cdr,
					      int flag_method)
{
  /* FIXME: The hash ought to be based on the tree contents, regardless
   *        of what the adresses of the type nodes are.
   */
  unsigned INT32 hash = DO_NOT_WARN((unsigned INT32)
				    ((ptrdiff_t)type*0x10204081)^
				    (0x8003*PTR_TO_INT(car))^
				    ~(0x10001*PTR_TO_INT(cdr)));
  unsigned INT32 index = hash % pike_type_hash_size;
  struct pike_type *t;
#ifdef PIKE_EXTRA_DEBUG
  static unsigned INT32 extra_debug_index = (unsigned INT32)~0;
#endif /* PIKE_EXTRA_DEBUG */

  /* PIKE_DEBUG code */
  if (type & ~255) {
    /* The bad type node on OSF/1 seems to be:
     *
     * type: 0xffff
     * car:  valid pointer.
     * cdr:  0x400000000
     * next: 0x100000000
     */
    Pike_fatal("Attempt to create an invalid type node: %d(%s)\n"
	       "  car: %p\n"
	       "  cdr: %p\n",
	       type, get_name_of_type(type),
	       car, cdr);
  }
  if (index >= pike_type_hash_size) {
    Pike_fatal("Modulo operation failed for hash:%u, index:%u, "
	       "size:%"PRINTSIZET"d.\n",
	       hash, index, pike_type_hash_size);
  }
  /* End PIKE_DEBUG code */

#ifdef PIKE_EXTRA_DEBUG
  if ((!~extra_debug_index) &&
      (type == T_FUNCTION) &&
      (car->type == T_STRING) &&
      (cdr->type == T_FUNCTION) &&
      (cdr->car->type == T_STRING) &&
      (cdr->cdr->type == T_MANY) &&
      (cdr->cdr->car->type == T_VOID) &&
      (cdr->cdr->cdr->type == T_STRING)) {
    /* Attempt to detect why we get a core-dump on OSF/1
     * when loading Unicode.so from test_resolv.
     *
     * The problem triggs when the type for normalize() is created.
     * function(string,string:string)
     *	/grubba 2005-02-04
     *
     * Load order:
     *   Module		Hashtable status	Note
     *   Nettle.so	OK
     *   ___Oracle.so	-			load_module() fails.
     *   Image.so	-			loads ok.
     *   Unicode.so	FAIL
     *
     * pike_type node:
     *   Field		Before			After
     *     t		1404b5020		1404b5020
     *     t->type	4 (function)		65535 (unknown)
     *     t->car	1404863f8 (1404863f8)	140557560 (1404863f8)
     *     t->cdr	1404b43d8 (1404b43d8)	400000000 (1404b43d8)
     *     t->next	0			100000000
     *  /grubba 2005-06-03
     */
    extra_debug_index = index;
  }
#endif /* PIKE_EXTRA_DEBUG */
  for(t = pike_type_hash[index]; t; t = t->next) {
#ifdef PIKE_EXTRA_DEBUG
    if (index == extra_debug_index) {
      fprintf(stderr,
	      "  %s:%d:PIKE_EXTRA_DEBUG:\n"
	      "    t: %p\n",
	      __FILE__, __LINE__,
	      t);
      fprintf(stderr,
	      "    t->type:%d (%s)\n"
	      "    t->car: %p (%p)\n"
	      "    t->cdr: %p (%p)\n"
	      "    t->next:%p\n",
	      t->type, get_name_of_type(t->type),
	      t->car, car,
	      t->cdr, cdr,
	      t->next);
    }
#endif /* PIKE_EXTRA_DEBUG */
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

      case PIKE_T_ATTRIBUTE:
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

#ifdef PIKE_DEBUG
  if ((type == T_OR) && (car->type == T_OR)) {
    Pike_fatal("Invalid CAR to OR node.\n");
  }
#endif
      
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
    if (flag_method == PT_IS_MARKER) {
      t->flags = PT_FLAG_MARKER_0 << (type-'0');
    } else {
      if (car && (flag_method & PT_COPY_CAR)) {
	t->flags |= car->flags;
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
	
  case T_ASSIGN:
    t->flags |= PT_FLAG_ASSIGN_0 << PTR_TO_INT(car);
    /* FALL_THROUGH */
  case T_SCOPE:
    debug_malloc_pass(cdr);
    break;

  case PIKE_T_ATTRIBUTE:
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
#else /* !DEBUG_MALLOC */
  if (type == T_ASSIGN) {
    t->flags |= PT_FLAG_ASSIGN_0 << PTR_TO_INT(car);
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

void debug_push_string_type(INT32 bitwidth)
{
  if ((bitwidth < 0) || (bitwidth > 32)) {
    my_yyerror("Invalid string width: %ld (expected 0..32).",
	       (long)bitwidth);
    bitwidth = 32;
  }

  *(++Pike_compiler->type_stackp) = mk_type(T_STRING,
					    (void *)(ptrdiff_t)bitwidth,
					    NULL, 0);

  TYPE_STACK_DEBUG("push_string_type");
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

void debug_push_type_attribute(struct pike_string *attr)
{
  /* fprintf(stderr, "push_type_attribute(\"%s\")\n", attr->str); */
  add_ref(attr);
  *Pike_compiler->type_stackp = mk_type(PIKE_T_ATTRIBUTE,
					(void *)attr,
					*Pike_compiler->type_stackp,
					PT_COPY_CDR);
  TYPE_STACK_DEBUG("push_type_name");
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

/* Only to be used from {or,and}_pike_types() et al! */
static void push_joiner_type(unsigned int type)
{
  /* fprintf(stderr, "push_joiner_type(%d)\n", type); */

  switch(type) {
  case T_OR:
  case T_AND:
    /* Special case: Check if the two top elements are equal. */
    if (Pike_compiler->type_stackp[-1] == Pike_compiler->type_stackp[0]) {
      free_type(*(Pike_compiler->type_stackp--));
      return;
    }
    /* Make a new type of the top two types. */
    --Pike_compiler->type_stackp;
#ifdef PIKE_DEBUG
    if ((*Pike_compiler->type_stackp+1)->type == type) {
      Pike_fatal("Invalid CAR to push_joiner_type().\n");
    }
#endif /* PIKE_DEBUG */
    *Pike_compiler->type_stackp = mk_type(type,
					  *(Pike_compiler->type_stackp+1),
					  *Pike_compiler->type_stackp,
					  PT_COPY_BOTH);
    break;
  default:
    Pike_fatal("Illegal joiner type: %d\n", type);
  }
}

static void push_reverse_joiner_type(unsigned int type)
{
  /* fprintf(stderr, "push_reverse_joiner_type(%d)\n", type); */

  switch(type) {
  case T_OR:
  case T_AND:
    /* Special case: Check if the two top elements are equal. */
    if (Pike_compiler->type_stackp[-1] == Pike_compiler->type_stackp[0]) {
      free_type(*(Pike_compiler->type_stackp--));
      return;
    }
    /* Make a new type of the top two types. */
    --Pike_compiler->type_stackp;
#ifdef PIKE_DEBUG
    if ((*Pike_compiler->type_stackp)->type == type) {
      Pike_fatal("Invalid CAR to push_reverse_joiner_type().\n");
    }
#endif /* PIKE_DEBUG */
    *Pike_compiler->type_stackp = mk_type(type,
					  *Pike_compiler->type_stackp,
					  *(Pike_compiler->type_stackp+1),
					  PT_COPY_BOTH);
    break;
  default:
    Pike_fatal("Illegal reverse joiner type: %d\n", type);
  }
}

static void low_or_pike_types(struct pike_type *t1,
			      struct pike_type *t2,
			      int zero_implied);

void debug_push_type(unsigned int type)
{
  /* fprintf(stderr, "push_type(%d)\n", type); */

  switch(type) {
  case T_OR:
  case T_AND:
    /* Special case: Check if the two top elements are equal. */
    if (Pike_compiler->type_stackp[-1] == Pike_compiler->type_stackp[0]) {
      free_type(*(Pike_compiler->type_stackp--));
      return;
    }
    if (Pike_compiler->type_stackp[0]->type == type) {
      /* The top type is the same as our type.
       * Split it and join the parts with the other type.
       */
      struct pike_type *top = *(Pike_compiler->type_stackp--);
      push_finished_type(top->cdr);
      push_type(type);
      push_finished_type(top->car);
      push_type(type);
      free_type(top);
      return;
    }
    if (type == T_OR) {
      struct pike_type *t1 = *(Pike_compiler->type_stackp--);
      struct pike_type *t2 = *(Pike_compiler->type_stackp--);
      low_or_pike_types(t1, t2, 0);
      free_type(t2);
      free_type(t1);
      return;
    }
    /* FALL_THROUGH */
  case T_FUNCTION:
  case T_MANY:
  case T_TUPLE:
  case T_MAPPING:
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
  case PIKE_T_ATTRIBUTE:
  case PIKE_T_NAME:
  default:
    /* Should not occur. */
    Pike_fatal("Unsupported argument to push_type(): %d\n", type);
    break;

  case T_STRING:
    Pike_fatal("String types should not be created with push_type().\n");
    break;

  case T_FLOAT:
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
    *(++Pike_compiler->type_stackp) = mk_type(type, NULL, NULL, PT_IS_MARKER);
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
  if ((top->type != expected) &&
      (top->type != PIKE_T_NAME) &&
      (top->type != PIKE_T_ATTRIBUTE)) {
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
  case PIKE_T_ATTRIBUTE:
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

/* The marker_set is used as follows:
 *
 *   PT_FLAG_MARKER_n	Indicates that marker #n should be kept after
 *			expansion.
 *
 *   PT_FLAG_ASSIGN_n	Indicates that the assign to marker #n should
 *			NOT be removed.
 */
static void debug_push_finished_type_with_markers(struct pike_type *type,
						  struct pike_type **markers,
						  INT32 marker_set)
{
  INT32 car_set, cdr_set;
 recurse:
#ifdef PIKE_TYPE_DEBUG
  if (l_flag > 2) {
    fprintf(stderr, "push_finished_type_with_markers((");
    simple_describe_type(type);
    fprintf(stderr, "),..., 0x%08x)...\n", marker_set);
  }
#endif /* PIKE_TYPE_DEBUG */
  /* We need to replace if there are any markers, or if there's a
   * non-masked assign.
   */
  if (!(type->flags & (~marker_set | PT_FLAG_MARKER) & PT_FLAG_MARK_ASSIGN)) {
    /* Nothing to replace in this subtree. */
#ifdef PIKE_TYPE_DEBUG
    if (l_flag > 2) {
      fprintf(stderr, "Nothing to replace in this subtree.\n");
      simple_describe_type(type);
      fprintf(stderr, "\n");
    }
#endif /* PIKE_TYPE_DEBUG */
    push_finished_type(type);
    return;
  }
  if ((type->type >= '0') && (type->type <= '9')) {
    /* Marker. */
    unsigned int m = type->type - '0';
#ifdef PIKE_TYPE_DEBUG
    if ((l_flag > 2) && m) {
      fprintf(stderr, "Marker %d: %p.\n", m, markers[m]);
    }
#endif /* PIKE_TYPE_DEBUG */
    if (markers[m]) {
      /* The marker has a value. */
      struct pike_type *type = dmalloc_touch(struct pike_type *, markers[m]);
#ifdef PIKE_TYPE_DEBUG
      if (l_flag > 2) {
	fprintf(stderr, "Marker value.\n");
      }
#endif
      /* FIXME: We probably ought to switch to the other marker set here. */
      markers[m] = NULL;
      push_finished_type_with_markers(type, markers, 0);
      if (markers[m]) free_type(markers[m]);
      markers[m] = dmalloc_touch(struct pike_type *, type);
    } else {
	/* The marker has not been set. */
#ifdef PIKE_TYPE_DEBUG
      if (l_flag > 2) {
	fprintf(stderr, "No marker value.\n");
      }
#endif
    }
    if (marker_set & (PT_FLAG_MARKER_0 << m)) {
      /* The marker should be kept. */
#ifdef PIKE_TYPE_DEBUG
      if (l_flag > 2) {
	fprintf(stderr, "Keep marker.\n");
      }
#endif
      push_type(type->type);
      if (markers[m]) push_type(T_OR);
    } else if (!markers[m]) {
      push_type(T_ZERO);
    }
    TYPE_STACK_DEBUG("push_finished_type_with_markers");
    return;
  } else if (type->type == T_ASSIGN) {
    /* Assign. */
    int marker = PTR_TO_INT(type->car);
#ifdef PIKE_TYPE_DEBUG
    if (l_flag > 2) {
      fprintf(stderr, "Assign to marker %"PRINTPTRDIFFT"d.\n",
	      CAR_TO_INT(type));
    }
#endif /* PIKE_TYPE_DEBUG */
    if (marker_set & (PT_FLAG_ASSIGN_0 << marker)) {
      /* The assignment should be kept as-is. */
#ifdef PIKE_TYPE_DEBUG
      if (l_flag > 2) {
	fprintf(stderr, "Keep assignment.\n");
      }
#endif /* PIKE_TYPE_DEBUG */
      /* Clear the flag. */
      push_finished_type_with_markers(type->cdr, markers,
				      marker_set &
				      ~(PT_FLAG_ASSIGN_0 << marker));
      push_assign_type('0' + marker);
      TYPE_STACK_DEBUG("push_finished_type_with_markers");
      return;
    } else {
#ifdef PIKE_TYPE_DEBUG
      if (l_flag > 2) {
	fprintf(stderr, "Strip assignment.\n");
      }
#endif /* PIKE_TYPE_DEBUG */
      type = type->cdr;
      goto recurse;
    }
  } else if (type->type == PIKE_T_NAME) {
    /* Strip the name, since it won't be correct anymore. */
    type = type->cdr;
    goto recurse;
  } else if (type->type == PIKE_T_ATTRIBUTE) {
    /* Keep the attribute. */
    push_finished_type_with_markers(type->cdr, markers, marker_set);
    push_type_attribute((struct pike_string *)type->car);
  }
  /* FIXME: T_SCOPE */

  if (type->car) {
    /* Keep markers for assigns in the car. */
    cdr_set = marker_set |
      ((type->car->flags & PT_FLAG_ASSIGN)>>PT_ASSIGN_SHIFT);
  } else {
    cdr_set = marker_set;
  }
  if (type->cdr) {
    /* Keep assigns for markers in the cdr. */
    car_set = marker_set |
      ((type->cdr->flags & PT_FLAG_MARKER)<<PT_ASSIGN_SHIFT);
  } else {
    car_set = marker_set;
  }

  if ((type->type == T_OR) || (type->type == T_AND)) {
    /* Special case handling for implicit zero. */
    /* FIXME: Probably ought to use {or,and}_pike_types() here.
     *        Problem is that they may mess with the markers...
     */

    type_stack_mark();
    /* We want to keep markers that have assigns in the car. */
    push_finished_type_with_markers(type->cdr, markers, cdr_set);
    if (type->type == T_OR) {
      struct pike_type *first = pop_type();
      struct pike_type *second;
      struct pike_type *res;
      push_finished_type_with_markers(type->car, markers, car_set);
      second = pop_unfinished_type();
      push_finished_type(res = or_pike_types(first, second, 1));
      free_type(second);
      free_type(first);
      free_type(res);
    } else if (peek_type_stack() == zero_type_string) {
      pop_stack_mark();
    } else {
      type_stack_mark();
      push_finished_type_with_markers(type->car, markers, car_set);
      if (peek_type_stack() == zero_type_string) {
	free_type(pop_unfinished_type());
	free_type(pop_unfinished_type());
	push_finished_type(zero_type_string);
      } else {
	pop_stack_mark();
	pop_stack_mark();
	push_type(T_AND);
      }
    }
  } else {
    if (type->cdr) {
      /* In all other cases type->cdr will be a valid node if is not NULL. */
      push_finished_type_with_markers(type->cdr, markers, cdr_set);
    }
    /* In all other cases type->car will be a valid node. */
    push_finished_type_with_markers(type->car, markers, car_set);
    /* push_type has sufficient magic to recreate the type. */
    push_type(type->type);
  }
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
	push_string_type(32);
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
    if(len>=sizeof(buf)-1) {
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
	    min = MIN_INT32;
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
	    max = MAX_INT32;
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
	    if (strncmp((const char *)*s, "mplements ", 10)) {
	      goto bad_type;
	    }
	    *s += 10;
	  }
	  while(ISSPACE(**s)) ++*s;
	no_is_implements:
	  if( !**s )
	    goto bad_type;
	  if (!strncmp((const char *)*s, "this_program", 12)) {
	    id = Pike_compiler->new_program->id;
	    *s += 12;
	  } else {
	    id = atoi( (const char *)*s );	
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
      if(!strcmp(buf,"string")) {
	while(ISSPACE(**s)) ++*s;
	if(**s == '(')
	{
	  INT32 bitwidth;
	  ++*s;
	  while(ISSPACE(**s)) ++*s;
	  bitwidth=STRTOL((const char *)*s,(char **)s,0);
	  while(ISSPACE(**s)) ++*s;
	  if(**s != ')') yyerror("Missing ')' in string width.");
	  else
	    ++*s;
	  push_string_type(bitwidth);
	} else {
	  push_string_type(32);
	}
	break;
      }
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
#endif

void simple_describe_type(struct pike_type *s)
{
  if (s) {
    /* fprintf(stderr, "[[[%p]]]", s); */
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

      case PIKE_T_ATTRIBUTE:
	fprintf(stderr, "attribute(%s, ",
		((struct pike_string *)s->car)->str);
	simple_describe_type(s->cdr);
	fprintf(stderr, ")");
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
	fprintf(stderr, "(%"PRINTPTRDIFFT"d = ", CAR_TO_INT(s));
	simple_describe_type(s->cdr);
	fprintf(stderr, ")");	
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
      case T_STRING:
	{
	  INT32 bitwidth = CAR_TO_INT(s);
	  if (bitwidth != 32) {
	    fprintf(stderr, "string(%d)", bitwidth);
	  } else {
	    fprintf(stderr, "string");
	  }
	  break;
	}
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
#ifdef DEBUG_MALLOC
	debug_malloc_dump_references(s, 0, 2, 0);
#endif
	break;
    }
    if (s->flags) {
      fprintf(stderr, "[%06x]", s->flags);
    }
  } else {
    fprintf(stderr, "NULL");
  }
}

static void low_describe_type(struct pike_type *t)
{
  char buffer[100];

  check_c_stack(1024);
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
	struct svalue s;
	if (t->car) {
	  my_strcat("object(is ");
	} else {
	  my_strcat("object(implements ");
	}
	if ((s.u.program = id_to_program(CDR_TO_INT(t)))) {
	  s.type = T_PROGRAM;
	  s.subtype = 0;
	  describe_svalue(&s, 0, NULL);
	  my_strcat(")");
	} else {
	  char buffer[100];
	  sprintf(buffer,"%"PRINTPTRDIFFT"d)",
		  CDR_TO_INT(t));
	  my_strcat(buffer);
	}
      }else{
	my_strcat("object");
      }
      break;

    case T_STRING:
      {
	INT32 bitwidth=CAR_TO_INT(t);
	my_strcat("string");
	if (bitwidth != 32) {
	  sprintf(buffer,"(%ld)",(long)bitwidth);
	  my_strcat(buffer);
	}
	break;
      }
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
      
    case PIKE_T_ATTRIBUTE:
      if (!((struct pike_string *)t->car)->size_shift) {
	my_strcat("attribute(");
	my_binary_strcat(((struct pike_string *)t->car)->str,
			 ((struct pike_string *)t->car)->len);
	my_strcat(", ");
	my_describe_type(t->cdr);
	my_strcat(")");
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

#if defined (PIKE_DEBUG) || defined (DO_PIKE_CLEANUP)

void debug_gc_check_all_types (void)
{
  if (gc_keep_markers || Pike_in_gc == GC_PASS_LOCATE) {
    unsigned INT32 index;
    for (index = 0; index < pike_type_hash_size; index++) {
      struct pike_type *t;
      for (t = pike_type_hash[index]; t; t = t->next) {
	GC_ENTER (t, T_TYPE) {
	  switch (t->type) {
	    case T_FUNCTION:
	    case T_MANY:
	    case T_TUPLE:
	    case T_MAPPING:
	    case T_OR:
	    case T_AND:
	    case PIKE_T_RING:
	    case PIKE_T_ATTRIBUTE:
	    case PIKE_T_NAME:
	      debug_gc_check (t->car, " as car in a type");
	      debug_gc_check (t->cdr, " as cdr in a type");
	      break;
	    case T_ARRAY:
	    case T_MULTISET:
	    case T_NOT:
	    case T_TYPE:
	    case T_PROGRAM:
	      debug_gc_check (t->car, " as car in a type");
	      break;
	    case T_SCOPE:
	    case T_ASSIGN:
	      debug_gc_check (t->cdr, " as cdr in a type");
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
	      Pike_fatal("debug_gc_check_all_types: "
			 "Unhandled type-node: %d\n", t->type);
	      break;
#endif /* PIKE_DEBUG */
	  }
	} GC_LEAVE;
      }
    }
  }
}

/* Leak reporting similar to the exit_with_cleanup code in
 * exit_builtin_modules. */

void report_all_type_leaks (void)
{
  unsigned INT32 index;
  if (!gc_keep_markers)
    Pike_fatal ("Should only be called in final cleanup.\n");
  for (index = 0; index < pike_type_hash_size; index++) {
    struct pike_type *t;
    for (t = pike_type_hash[index]; t; t = t->next) {
      struct marker *m = find_marker (t);
      /* We aren't hooked in to the gc mark pass so we don't have
       * markers for types with only external references. */
      INT32 m_refs = m ? m->refs : 0;
      if (t->refs != m_refs) {
	fprintf (stderr, "Type at %p got %d unaccounted refs: ",
		 t, t->refs - m_refs);
	simple_describe_type (t);
	fputc ('\n', stderr);
#ifdef DEBUG_MALLOC
	debug_malloc_dump_references (t, 2, 1, 0);
#endif
      }
    }
  }
}

void free_all_leaked_types (void)
{
  unsigned INT32 index;
  if (!gc_keep_markers)
    Pike_fatal ("Should only be called in final cleanup.\n");
  for (index = 0; index < pike_type_hash_size; index++) {
    struct pike_type *t;
    for (t = pike_type_hash[index]; t; t = t->next) {
      struct marker *m = find_marker (t);
      INT32 m_refs = m ? m->refs : 0;
      INT32 refs = t->refs;
      if (refs > m_refs) {
#ifdef PIKE_DEBUG
	if (m) m->flags |= GC_CLEANUP_FREED;
#endif /* PIKE_DEBUG */
	do {
	  free_type (t);
	  refs--;
	} while (refs > m_refs);
	/* t is invalid here, as is its next pointer.
	 * Start over from the top of this hash entry.
	 */
	index--;
	break;
      }
    }
  }
}

#endif  /* PIKE_DEBUG || DO_PIKE_CLEANUP */


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
  case PIKE_T_ATTRIBUTE:
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
			      int zero_implied);

/* Push either t1, t2 or the OR of t1 and t2.
 * Returns -1 if t1 was pushed.
 *          0 if the OR was pushed. (Successful join)
 *          1 if t2 was pushed.
 */
static int lower_or_pike_types(struct pike_type *t1,
			       struct pike_type *t2,
			       int zero_implied,
			       int elem_on_stack)
{
  int ret = 0;
  struct pike_type *t = NULL;
  struct pike_type *top = NULL;
#if 0
  fprintf(stderr, "    lower_or_pike_types(");
  simple_describe_type(t1);
  fprintf(stderr, ", ");
  simple_describe_type(t2);
  fprintf(stderr, ")\n");
#endif
  if (t1 == t2) {
    t = t1;
  } else if (t1->type < t2->type) {
    t = t1;
    ret = -1;
  } else if (t1->type > t2->type) {
    t = t2;
    ret = 1;
  } else {
#ifdef PIKE_DEBUG
    if (t1->type != t2->type) {
      Pike_fatal("Lost track of types t1->type: %d, t2->type: %d\n",
		 t1->type, t2->type);
    }
#endif /* PIKE_DEBUG */
    switch(t1->type) {
    case T_INT:
      if (CAR_TO_INT(t1) < CAR_TO_INT(t2)) {
	t = t1;
	ret = -1;
      } else {
	t = t2;
	ret = 1;
      }
      break;
    case T_STRING:
      {
	INT32 w1 = CAR_TO_INT(t1);
	INT32 w2 = CAR_TO_INT(t2);
	if (w1 >= w2) {
	  t = t1;
	} else {
	  t = t2;
	}
      }
      break;
    case T_OBJECT:
      if (!CDR_TO_INT(t1)) {
	t = t1;
      } else if (!CDR_TO_INT(t2)) {
	t = t2;
      } else if (CDR_TO_INT(t1) < CDR_TO_INT(t2)) {
	t = t1;
	ret = -1;
      } else {
	t = t2;
	ret = 1;
      }
      break;
    case T_MAPPING:
      if (t1->car->type < t2->car->type) {
	t = t1;
	ret = -1;
	break;
      }
      if (t1->car->type > t2->car->type) {
	t = t2;
	ret = 1;
	break;
      }
      if (t1->cdr->type < t2->cdr->type) {
	t = t1;
	ret = -1;
	break;
      }
      if (t1->cdr->type > t2->cdr->type) {
	t = t2;
	ret = 1;
	break;
      }
      t = t1;
      ret = -1;
      break;
    case T_ARRAY:
    case T_MULTISET:
      if (t1->car->type < t2->car->type) {
	t = t1;
	ret = -1;
	break;
      } else if (t1->car->type > t2->car->type) {
	t = t2;
	ret = 1;
	break;
      }
      /* FALL_THOUGH */
    default:
      t = t1;
      ret = -1;
      break;
    }
  }
  if (!elem_on_stack || ((top = peek_type_stack())->type != t->type)) {
    push_finished_type(t);
  } else {
    switch(t->type) {
    case T_FLOAT:
    case T_MIXED:
    case T_VOID:
    case T_ZERO:
      /* There can only be one. */
      break;
    case T_INT:
      {
	INT32 min1 = CAR_TO_INT(t);
	INT32 max1 = CDR_TO_INT(t);
	INT32 min2 = CAR_TO_INT(top);
	INT32 max2 = CDR_TO_INT(top);

	if (zero_implied) {
	  if (min1 == 1) min1 = 0;
	  if (min2 == 1) min2 = 0;
	  if (max1 == -1) max1 = 0;
	  if (max2 == -1) max2 = 0;
	}
    
	if ((min1 > max2) && (min1 > max2 + 1)) {
	  /* No overlap. */
	  push_finished_type(t);
#ifdef PIKE_DEBUG
	} else if ((min2 > max1) && (min2 > max1 + 1)) {
	  /* No overlap and wrong order! */
	  Pike_fatal("Bad integer ordering in lower_or_pike_types().\n");
#endif
	} else {
	  Pike_compiler->type_stackp--;
	  free_type(top);
	  /* Overlap */
	  min1 = MINIMUM(min1, min2);
	  max1 = MAXIMUM(max1, max2);

	  push_int_type(min1, max1);
	}
      }
      break;
    case T_STRING:
      {
	INT32 w1 = CAR_TO_INT(t);
	INT32 w2 = CAR_TO_INT(top);
	if (w1 >= w2) {
	  Pike_compiler->type_stackp--;
	  free_type(top);
	  push_finished_type(t);
	}
      }
      break;
    case T_OBJECT:
      if (CDR_TO_INT(top)) {
	push_finished_type(t);
      }
      break;
    case T_ARRAY:
    case T_MULTISET:
      Pike_compiler->type_stackp--;
      low_or_pike_types(t->car, top->car, zero_implied);
      push_type(t->type);
      free_type(top);
      break;
    case T_SCOPE:
      Pike_compiler->type_stackp--;
      low_or_pike_types(t->cdr, top->cdr, zero_implied);
      if (CAR_TO_INT(t) > CAR_TO_INT(top))
	push_scope_type(CAR_TO_INT(t));
      else
	push_scope_type(CAR_TO_INT(top));
      free_type(top);
      break;
    case T_MAPPING:
      if (t->car == top->car) {
	Pike_compiler->type_stackp--;
	push_finished_type(t->car);
	low_or_pike_types(t->cdr, top->cdr, zero_implied);
	push_reverse_type(T_MAPPING);
	free_type(top);
	break;
      } else if (t->cdr == top->cdr) {
	Pike_compiler->type_stackp--;
	push_finished_type(t->cdr);
	low_or_pike_types(t->car, top->car, zero_implied);
	push_type(T_MAPPING);
	free_type(top);
	break;
      }
      /* FALL_THROUGH */
    default:
      push_finished_type(t);
      break;
    }
  }
  return ret;
}

static void low_or_pike_types(struct pike_type *t1,
			      struct pike_type *t2,
			      int zero_implied)
{
#ifdef PIKE_DEBUG
  struct pike_type *arg1 = t1;
  struct pike_type *arg2 = t2;
#endif
#if 0
  fprintf(stderr, "  low_or_pike_types(");
  simple_describe_type(t1);
  fprintf(stderr, ", ");
  simple_describe_type(t2);
  fprintf(stderr, ")\n");
#endif
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
  else if (t1 == t2) {
    push_finished_type(t1);
  }
  else if (t1->type == T_OR) {
    int on_stack = 0;
    type_stack_mark();
    while (t1 || t2) {
      if (!t1) {
	if (t2->type == T_OR) {
	  push_finished_type(t2->car);
	  t2 = t2->cdr;
	} else {
	  push_finished_type(t2);
	  t2 = NULL;
	}
      } else if (!t2) {
	if (t1->type == T_OR) {
	  push_finished_type(t1->car);
	  t1 = t1->cdr;
	} else {
	  push_finished_type(t1);
	  t1 = NULL;
	}
      } else {
	struct pike_type *a = t1;
	struct pike_type *b = t2;
	struct pike_type *n1 = NULL;
	struct pike_type *n2 = NULL;
	int val;
	if (t1->type == T_OR) {
	  a = t1->car;
	  n1 = t1->cdr;
	}
	if (t2->type == T_OR) {
	  b = t2->car;
	  n2 = t2->cdr;
	}
#ifdef PIKE_DEBUG
	if ((a->type == T_OR) || (b->type == T_OR)) {
	  fprintf(stderr, "  low_or_pike_types(");
	  simple_describe_type(arg1);
	  fprintf(stderr, ", ");
	  simple_describe_type(arg2);
	  fprintf(stderr, ")\n  a:");
	  simple_describe_type(a);
	  fprintf(stderr, "\n  b:");
	  simple_describe_type(b);
	  fprintf(stderr, ")\n");
	  Pike_fatal("Invalid type to lower_or_pike_types!\n");
	}
#endif
	val = lower_or_pike_types(a, b, zero_implied, on_stack);
	if (val <= 0) t1 = n1;
	if (val >= 0) t2 = n2;
      }
      on_stack = 1;
    }
    on_stack = pop_stack_mark();
    while (on_stack > 1) {
      push_reverse_joiner_type(T_OR);
      on_stack--;
    }
  }
  else if (t2->type == T_OR) {
    low_or_pike_types(t2, t1, zero_implied);
  }
  else {
    int val = lower_or_pike_types(t1, t2, zero_implied, 0);
    if (val < 0) {
      push_finished_type(t2);
      push_reverse_joiner_type(T_OR);
    } else if (val > 0) {
      push_finished_type(t1);
      push_reverse_joiner_type(T_OR);
    }
  }
}

struct pike_type *or_pike_types(struct pike_type *a,
				struct pike_type *b,
				int zero_implied)
{
  struct pike_type *res;
  type_stack_mark();
  low_or_pike_types(a,b,1 /*zero_implied*/);
  res = pop_unfinished_type();
#if 0
  fprintf(stderr, "  ==> ");
  simple_describe_type(res);
  fprintf(stderr, "\n");
#endif
  return res;
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
  else if ((t1->type == T_STRING) && (t2->type == T_STRING)) {
    INT32 w1, w2;
    w1 = CAR_TO_INT(t1);
    w2 = CAR_TO_INT(t2);
    if (w1 < w2) {
      push_finished_type(t1);
    } else {
      push_finished_type(t2);
    }
  }
  else if((t1->type == T_FLOAT) && (t2->type == T_FLOAT))
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
#ifdef PIKE_TYPE_DEBUG
static struct pike_type *low_match_types2(struct pike_type *a,
					  struct pike_type *b,
					  int flags);
#endif
static struct pike_type *low_match_types(struct pike_type *a,
					 struct pike_type *b,
					 int flags)
#ifdef PIKE_TYPE_DEBUG
{
  int e;
  char *s;

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

  fatal_check_c_stack(1024);

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
  case PIKE_T_ATTRIBUTE:
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
	push_finished_type_with_markers(b, b_markers, 0);
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
	struct pike_type *t = a_markers[m];
	struct pike_type *res;
#ifdef PIKE_DEBUG
	if(a_markers[m]->type == a->type)
	  Pike_fatal("Cyclic type!\n");
	if(a_markers[m]->type == T_OR &&
	   a_markers[m]->car->type == a->type)
	  Pike_fatal("Cyclic type!\n");
#endif
	a_markers[m] = NULL;
	res = low_match_types(t, b, flags);
	if (a_markers[m]) {
	  struct pike_type *tmp;
	  a_markers[m] = or_pike_types(tmp = a_markers[m], t, 0);
	  free_type(tmp);
	  free_type(t);
	} else {
	  a_markers[m] = t;
	}
	return res;
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
  case PIKE_T_ATTRIBUTE:
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
	push_finished_type_with_markers(a, a_markers, 0);
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
	struct pike_type *t = b_markers[m];
	struct pike_type *res;
#ifdef PIKE_DEBUG
	if(b_markers[m]->type == b->type)
	  Pike_fatal("Cyclic type!\n");
#endif
	b_markers[m] = NULL;
	res = low_match_types(a, t, flags);
	if (b_markers[m]) {
	  struct pike_type *tmp;
	  b_markers[m] = or_pike_types(tmp = b_markers[m], t, 0);
	  free_type(tmp);
	  free_type(t);
	} else {
	  b_markers[m] = t;
	}
	return res;
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

    /* object(* x) =? object(* x) */
    if (a->cdr == b->cdr) break;

    /* object(x *) =? object(x *) */
    if(TEST_COMPAT(7,4) && a->car == b->car)
    {
      /* x? */
      if(a->car)
      {
	/* object(1 x) =? object(1 x) */
	return 0;
      }else{
	/* object(0 *) =? object(0 *) */
	break;
      }
    }

    /* Note: In Pike 7.4 and earlier the following was only done
     *       when a->car != b->car.
     */
    {
      struct program *ap,*bp;
      ap = id_to_program(CDR_TO_INT(a));
      bp = id_to_program(CDR_TO_INT(b));

      if(!ap || !bp) break;

      implements_mode = 0;

      if (!is_compatible(implements_a=ap,implements_b=bp))
	return 0;
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
#ifdef PIKE_TYPE_DEBUG
static int low_pike_types_le2(struct pike_type *a, struct pike_type *b,
			      int array_cnt, unsigned int flags);
#endif /* PIKE_TYPE_DEBUG */
static int low_pike_types_le(struct pike_type *a, struct pike_type *b,
			     int array_cnt, unsigned int flags)
#ifdef PIKE_TYPE_DEBUG
{
  int e;
  char *s;
  int res;
  char buf[50];

  if (l_flag>2) {
    fprintf(stderr, "%*slow_pike_types_le(", indent*2, "");
    simple_describe_type(a);
    fprintf(stderr, ",\n%*s", indent*2 + 18, "");
    simple_describe_type(b);
    fprintf(stderr, ",\n%*s%d, 0x%08x);\n",
	    indent*2 + 18, "", array_cnt, flags);
    indent++;
  }

  res = low_pike_types_le2(a, b, array_cnt, flags);

  if (l_flag>2) {
    indent--;

    fprintf(stderr, "%*s= %d\n", indent*2, "", res);
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
  case PIKE_T_ATTRIBUTE:
    a = a->cdr;
    goto recurse;

  case T_NOT:
    if (b->type == T_NOT) {
      struct pike_type *tmp = a->car;
      a = b->car;
      b = tmp;
      array_cnt = -array_cnt;
      flags ^= LE_A_B_SWAPPED;
      goto recurse;
    }
    /* Some common cases. */
    switch(a->car->type) {
    case T_NOT:
      a = a->car->car;
      goto recurse;
    case T_MIXED:
      a = zero_type_string;
      goto recurse;
    case T_ZERO:
    case T_VOID:
      a = mixed_type_string;
      goto recurse;
    }
    if (low_pike_types_le(a->car, b, array_cnt, flags)) {
      return 0;
    }
    /* FIXME: This is wrong... */
    return !low_pike_types_le(b, a->car, -array_cnt, flags ^ LE_A_B_SWAPPED);

  case T_ASSIGN:
    ret = low_pike_types_le(a->cdr, b, array_cnt, flags);
    if(ret && (b->type != T_VOID))
    {
      int m = CAR_TO_INT(a);
      struct pike_type *tmp;
      int i;

      type_stack_mark();
      if (flags & LE_A_B_SWAPPED) {
	push_finished_type_with_markers(b, a_markers, 0);
      } else {
	push_finished_type_with_markers(b, b_markers, 0);
      }
      for(i=array_cnt; i > 0; i--)
	push_type(T_ARRAY);
      tmp=pop_unfinished_type();
      
      type_stack_mark();
      if (flags & LE_A_B_SWAPPED) {
	low_or_pike_types(b_markers[m], tmp, 0);
	if(b_markers[m]) free_type(b_markers[m]);
	free_type(tmp);
	b_markers[m] = pop_unfinished_type();
      } else {
	low_or_pike_types(a_markers[m], tmp, 0);
	if(a_markers[m]) free_type(a_markers[m]);
	free_type(tmp);
	a_markers[m] = pop_unfinished_type();
      }
#ifdef PIKE_TYPE_DEBUG
      if (l_flag>2) {
	if (flags & LE_A_B_SWAPPED) {
	  fprintf(stderr, "%*sb_markers[%c]=",
		  indent * 2, "", (char)(m+'0'));
	  simple_describe_type(b_markers[m]);
	} else {
	  fprintf(stderr, "%*sa_markers[%c]=",
		  indent * 2, "", (char)(m+'0'));
	  simple_describe_type(a_markers[m]);
	}
	fprintf(stderr, "\n");
      }
#endif
    }
    return ret;

    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
    {
      int m = a->type - '0';
      if (flags & LE_A_B_SWAPPED) {
	if(b_markers[m]) {
	  a = b_markers[m];
	} else {
	  a = mixed_type_string;
	}
      } else {
	if(a_markers[m]) {
	  a = a_markers[m];
	} else {
	  a = mixed_type_string;
	}
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
  case PIKE_T_ATTRIBUTE:
    b = b->cdr;
    goto recurse;

  case T_NOT:
    /* Some common cases. */
    switch(b->car->type) {
    case T_NOT:
      b = b->car->car;
      goto recurse;
    case T_MIXED:
      b = zero_type_string;
      goto recurse;
    case T_ZERO:
    case T_VOID:
      b = mixed_type_string;
      goto recurse;
    }
    if (low_pike_types_le(a, b->car, array_cnt, flags)) {
      return 0;
    }
    /* FIXME: This is wrong... */
    return !low_pike_types_le(b->car, a, -array_cnt, flags ^ LE_A_B_SWAPPED);

  case T_ASSIGN:
    ret = low_pike_types_le(a, b->cdr, array_cnt, flags);
    if(ret && (a->type != T_VOID))
    {
      int m = CAR_TO_INT(b);
      struct pike_type *tmp;
      int i;

      type_stack_mark();
      if (flags & LE_A_B_SWAPPED) {
	push_finished_type_with_markers(a, b_markers, 0);
      } else {
	push_finished_type_with_markers(a, a_markers, 0);
      }
      for(i = array_cnt; i < 0; i++)
	push_type(T_ARRAY);
      tmp=pop_unfinished_type();
      
      type_stack_mark();
      if (flags & LE_A_B_SWAPPED) {
	low_or_pike_types(a_markers[m], tmp, 0);
	if(a_markers[m]) free_type(a_markers[m]);
	free_type(tmp);
	a_markers[m] = pop_unfinished_type();
      } else {
	low_or_pike_types(b_markers[m], tmp, 0);
	if(b_markers[m]) free_type(b_markers[m]);
	free_type(tmp);
	b_markers[m] = pop_unfinished_type();
      }
#ifdef PIKE_TYPE_DEBUG
      if (l_flag>2) {
	if (flags & LE_A_B_SWAPPED) {
	  fprintf(stderr, "%*sa_markers[%c]=",
		  indent * 2, "", (char)(m+'0'));
	  simple_describe_type(a_markers[m]);
	} else {
	  fprintf(stderr, "%*sb_markers[%c]=",
		  indent * 2, "", (char)(m+'0'));
	  simple_describe_type(b_markers[m]);
	}
	fprintf(stderr, "\n");
      }
#endif
    }
    return ret;

  case '0': case '1': case '2': case '3': case '4':
  case '5': case '6': case '7': case '8': case '9':
    {
      int m = b->type - '0';
      if (flags & LE_A_B_SWAPPED) {
	if(a_markers[m]) {
	  b = a_markers[m];
	} else {
	  b = mixed_type_string;
	}
      } else {
	if(b_markers[m]) {
	  b = b_markers[m];
	} else {
	  b = mixed_type_string;
	}
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
	if (!low_pike_types_le(b_tmp, a_tmp, 0, flags ^ LE_A_B_SWAPPED)) {
	  return 0;
	}
      }
    }
    /* FALL_THROUGH */
  case TWOT(T_MANY, T_MANY):
    /* check the 'many' type */
    if ((a->car->type != T_VOID) && (b->car->type != T_VOID)) {
      if (!low_pike_types_le(b->car, a->car, 0, flags ^ LE_A_B_SWAPPED)) {
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

#if 0
      fprintf(stderr,
	      "id_to_program(%d) ==> %p\n"
	      "id_to_program(%d) ==> %p\n",
	      CDR_TO_INT(a), ap,
	      CDR_TO_INT(b), bp);
#endif /* 0 */

      if (!ap || !bp) {
	/* Shouldn't happen... */
	/* fprintf(stderr, "ap:%p bp:%p\n", ap, bp); */
	return 0;
      }
      if ((flags & LE_WEAK_OBJECTS) &&
	  (!TEST_COMPAT(7,4) || (!a->car))) {
	implements_mode = 0;
#if 0
	fprintf(stderr, "  is_compat(%p(%d), %p(%d))\n",
		ap, ap->id, bp, bp->id);
#endif /* 0 */
	return is_compatible(implements_a=ap, implements_b=bp);
      }
      implements_mode = 1;
#if 0
      fprintf(stderr, "  implements(%p(%d), %p(%d))\n",
	      ap, ap->id, bp, bp->id);
#endif /* 0 */
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

  case T_STRING:
    {
      INT32 awidth = CAR_TO_INT(a);
      INT32 bwidth = CAR_TO_INT(b);
      return awidth <= bwidth;
    }

  case T_FLOAT:
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

  case PIKE_T_ATTRIBUTE:
    if (low_get_return_type(a->cdr, b)) {
      push_type_attribute((struct pike_string *)a->car);
      return 1;
    }
    return 0;

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
      push_finished_type_with_markers(a, a_markers, 0);
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

  while((t->type == PIKE_T_NAME) ||
	(t->type == PIKE_T_ATTRIBUTE)) {
    t = t->cdr;
  }
  while((index_type->type == PIKE_T_NAME) ||
        (index_type->type == PIKE_T_ATTRIBUTE)) {
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
    {
      int width = CAR_TO_INT(t);
      if (width != 32) {
	/* Narrow string */
	type_stack_mark();
	push_int_type(0, (1<<width)-1);
	return pop_unfinished_type();
      }
    }
    /* FALL_THROUGH */
  case T_MULTISET: /* always int */
    add_ref(int_type_string);
    return int_type_string;

  case T_MAPPING:
    add_ref(t = t->cdr);
    return t;

  case T_ARRAY:
    {
      struct pike_type *a;

      if(low_pike_types_le(string0_type_string, index_type, 0, 0) &&
	 (a = low_index_type(t->car, string0_type_string, n))) {
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

  while((t->type == PIKE_T_NAME) ||
	(t->type == PIKE_T_ATTRIBUTE)) {
    t = t->cdr;
  }
  if (index1_type)
    while((index1_type->type == PIKE_T_NAME) ||
	  (index1_type->type == PIKE_T_ATTRIBUTE)) {
      index1_type = index1_type->cdr;
    }
  if (index2_type)
    while((index2_type->type == PIKE_T_NAME) ||
	  (index2_type->type == PIKE_T_ATTRIBUTE)) {
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

      if((i = FIND_LFUN(p, LFUN_RANGE)) != -1)
      {
	struct pike_type *call_type = NULL;
	type_stack_mark();
	push_finished_type(mixed_type_string);
	push_finished_type(void_type_string);
	push_type(T_OR);			/* Return type */
	push_finished_type(void_type_string);	/* Many type */
	push_type(T_MANY);
	push_int_type (INDEX_FROM_BEG, OPEN_BOUND); /* arg4 type */
	push_type(T_FUNCTION);
	if (index2_type)			/* arg3 type */
	  push_finished_type(index2_type);
	else
	  push_type (T_ZERO);
	push_type(T_FUNCTION);
	push_int_type (INDEX_FROM_BEG, OPEN_BOUND); /* arg2 type */
	push_type(T_FUNCTION);
	if (index1_type)			/* arg1 type */
	  push_finished_type(index1_type);
	else
	  push_type (T_ZERO);
	push_type(T_FUNCTION);
	call_type = pop_unfinished_type();
	
	if((tmp = check_call(call_type, ID_FROM_INT(p, i)->type, 0))) {
	  free_type(call_type);
	  return tmp;
	}
	free_type(call_type);

	add_ref(mixed_type_string);
	return mixed_type_string;
      }

      if((i = FIND_LFUN(p, LFUN_INDEX)) != -1)
      {
	/* FIXME: Should check for a _sizeof operator if from-the-end
	 * indexing is done. */

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
	if (index2_type)			/* arg2 type */
	  push_finished_type(index2_type);
	else
	  push_int_type (MAX_INT_TYPE, MAX_INT_TYPE);
	push_type(T_FUNCTION);
	if (index1_type)			/* arg1 type */
	  push_finished_type(index1_type);
	else
	  push_type (T_ZERO);
	push_type(T_FUNCTION);
	call_type = pop_unfinished_type();
	
	if((tmp = check_call(call_type, ID_FROM_INT(p, i)->type, 0))) {
	  free_type(call_type);
	  return tmp;
	}
	free_type(call_type);

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
      if (index1_type && !low_match_types(int_type_string, index1_type, 0)) {
	struct pike_string *s = describe_type(t);
	yywarning("Bad argument 1 to range operator on %s.",
		  s->str);
	free_string(s);
	yyexplain_nonmatching_types(int_type_string, index1_type,
				    YYTE_IS_WARNING);
	/* Bad index1 type. */
	return 0;
      }
      if (index2_type && !low_match_types(int_type_string, index2_type, 0)) {
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
  case PIKE_T_ATTRIBUTE:
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
  case PIKE_T_ATTRIBUTE:
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
    case PIKE_T_ATTRIBUTE:
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
    case PIKE_T_ATTRIBUTE:
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

/* Count the minimum number of arguments for a function type.
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

/* Get the type for the specified argument in a function type.
 * Argument number -1 is the return type.
 * True arguments are counted from zero.
 */
struct pike_type *get_argument_type(struct pike_type *fun, int arg_no)
{
 loop:
  switch(fun->type) {
  case T_OR:
    return or_pike_types(get_argument_type(fun->car, arg_no),
			 get_argument_type(fun->cdr, arg_no),
			 1);
  case T_FUNCTION:
    if (arg_no > 0) {
      arg_no--;
      fun = fun->cdr;
      goto loop;
    }
    /* FALL_THROUGH */
  case T_MANY:
    if (arg_no < 0) {
      add_ref(fun->cdr);
      return fun->cdr;
    }
    add_ref(fun->car);
    return fun->car;

  case T_MIXED:
    add_ref(fun);
    return fun;

  case T_ARRAY:
    if (arg_no < 0) {
      type_stack_mark();
      push_finished_type(fun = get_argument_type(fun->car, arg_no));
      push_type(T_ARRAY);
      free_type(fun);
      return pop_unfinished_type();
    }
    return get_argument_type(fun->car, arg_no);

  default:
    add_ref(zero_type_string);
    return zero_type_string;
  }
}

/* Get the resulting type from a soft cast.
 *
 * Flags:
 *   1 SOFT_WEAKER	Weaker type.
 */
struct pike_type *soft_cast(struct pike_type *soft_type,
			    struct pike_type *orig_type,
			    int flags)
{
  struct pike_type *res = NULL;
  struct pike_type *tmp = NULL;
  struct pike_type *tmp2 = NULL;
  struct pike_type *tmp3 = NULL;

  if (soft_type == orig_type) {
    copy_pike_type(res, soft_type);
    return res;
  }

 loop:
  switch(soft_type->type) {
  case T_OR:
    res = or_pike_types(tmp = soft_cast(soft_type->car, orig_type, flags),
			tmp2 = soft_cast(soft_type->cdr, orig_type, flags), 1);
    break;
  case T_AND:
    res = and_pike_types(tmp = soft_cast(soft_type->car, orig_type, flags),
			 tmp2 = soft_cast(soft_type->cdr, orig_type, flags));
    break;
  case T_SCOPE:
  case T_ASSIGN:
  case PIKE_T_NAME:
    soft_type = soft_type->cdr;
    goto loop;
    /* FIXME: TUPLE, RING */
  case PIKE_T_ATTRIBUTE:
    if ((res = soft_cast(soft_type->cdr, orig_type, flags))) {
      type_stack_mark();
      push_finished_type(res);
      push_type_attribute((struct pike_string *)soft_type->car);
      free_type(res);
      res = pop_unfinished_type();
    }
    return res;
  case T_MIXED:
    if (flags & SOFT_WEAKER) {
      copy_pike_type(res, soft_type);
    } else if (orig_type->type == T_VOID) {
      copy_pike_type(res, zero_type_string);
    } else {
      copy_pike_type(res, orig_type);
    }
    break;
  case T_ZERO:
    if (!(flags & SOFT_WEAKER) || (orig_type->type == T_VOID)) {
      copy_pike_type(res, soft_type);
    } else {
      copy_pike_type(res, orig_type);
    }
    break;
  }
  if (!res) {
  loop2:
    switch(orig_type->type) {
    case T_OR:
      res = or_pike_types(tmp = soft_cast(soft_type, orig_type->car, flags),
			  tmp2 = soft_cast(soft_type, orig_type->cdr, flags),
			  1);
      break;
    case T_AND:
      res = and_pike_types(tmp = soft_cast(soft_type, orig_type->car, flags),
			   tmp2 = soft_cast(soft_type, orig_type->cdr, flags));
      break;
    case T_SCOPE:
    case T_ASSIGN:
    case PIKE_T_NAME:
      orig_type = orig_type->cdr;
      goto loop2;
    case PIKE_T_ATTRIBUTE:
      if ((res = soft_cast(soft_type, orig_type->cdr, flags))) {
	type_stack_mark();
	push_finished_type(res);
	push_type_attribute((struct pike_string *)orig_type->car);
	free_type(res);
	res = pop_unfinished_type();
      }
      return res;
    case T_MIXED:
      if (flags & SOFT_WEAKER) {
	copy_pike_type(res, orig_type);
      } else {
	copy_pike_type(res, soft_type);
      }
      break;
    case T_VOID:
    case T_ZERO:
      if (flags & SOFT_WEAKER) {
	copy_pike_type(res, soft_type);
      } else {
	copy_pike_type(res, zero_type_string);
      }
      break;
    }
  }
  if (!res) {
    switch(soft_type->type) {
    case T_VOID:
      if (orig_type->type == T_VOID) {
	copy_pike_type(res, soft_type);
      } else {
	return NULL;
      }
      break;
    case T_PROGRAM:
      /* Convert to a function returning an object. */
      copy_pike_type(tmp3, soft_type->car);	/* Return type */
      if ((tmp2 = low_object_lfun_type(tmp3, LFUN_CREATE))) {
	soft_type = tmp2;
	tmp2 = NULL;
      } else {
      /* FIXME: Multiple cases. */
	soft_type = function_type_string;
      }
      /* FALL_THROUGH */
    case T_FUNCTION:
    case T_MANY:
      {
	int array_cnt = 0;
	int loop_cnt = 0;
	while(orig_type->type == T_ARRAY) {
	  array_cnt++;
	  orig_type = orig_type->car;
	}
	if (orig_type->type == T_PROGRAM) {
	  copy_pike_type(tmp, orig_type->car);	/* Return type */
	  if ((tmp2 = low_object_lfun_type(tmp, LFUN_CREATE))) {
	    orig_type = tmp2;
	    tmp2 = NULL;
	  } else {
	    /* FIXME: Multiple cases. */
	    tmp2 = soft_type;
	    while(tmp2->type == T_FUNCTION) tmp2 = tmp2->cdr;
	    if (tmp2->type == T_MANY) {
	      if ((tmp2 = soft_cast(tmp2->car, tmp, flags ^ SOFT_WEAKER))) {
		/* FIXME: Adjust the return type to tmp2! */
		copy_pike_type(res, soft_type);
	      }
	    } else {
	      tmp2 = NULL;
	      copy_pike_type(res, soft_type);
	    }
	    break;
	  }
	} else if (orig_type->type == T_OBJECT) {
	  if ((tmp = low_object_lfun_type(orig_type, LFUN_CALL))) {
	    orig_type = tmp;
	    tmp = NULL;
	  } else {
	    /* FIXME: Multiple cases. */
	    copy_pike_type(res, orig_type);
	    break;
	  }
	}
	/* FIXME: Loop above until function? */
	if ((orig_type->type != T_FUNCTION) &&
	    (orig_type->type != T_MANY)) {
	  /* Failure. */
	  break;
	}
	type_stack_mark();
	while((soft_type->type == T_FUNCTION) ||
	      (orig_type->type == T_FUNCTION)) {
	  if (!(tmp2 = soft_cast(soft_type->car, orig_type->car,
				 flags ^ SOFT_WEAKER)) ||
	      (tmp2 == void_type_string)) {
	    goto function_cast_fail;
	  }
	  push_finished_type(tmp2);
	  free_type(tmp2);
	  tmp2 = NULL;
	  if (soft_type->type == T_FUNCTION) soft_type = soft_type->cdr;
	  if (orig_type->type == T_FUNCTION) orig_type = orig_type->cdr;
	  loop_cnt++;
	}
#ifdef PIKE_DEBUG
	if ((soft_type->type != T_MANY) || (orig_type->type != T_MANY)) {
	  fprintf(stderr,
		  "Strange function type (expected MANY node).\n"
		  "Orig type: ");
	  simple_describe_type(orig_type);
	  fprintf(stderr, "\n"
		  "Soft type: ");
	  simple_describe_type(soft_type);
	  fprintf(stderr, "\n");
	  Pike_fatal("Strange function type in soft cast.\n");
	}
#endif /* PIKE_DEBUG */
	if (!(tmp2 = soft_cast(soft_type->car, orig_type->car,
			       flags ^ SOFT_WEAKER))) {
	  goto function_cast_fail;
	}
	push_finished_type(tmp2);
	free_type(tmp2);
	tmp2 = NULL;
	/* Note: Special case for the return type in case of create(). */
	if (tmp) {
	  orig_type = tmp;
	} else {
	  orig_type = orig_type->cdr;
	}
	if (tmp3) {
	  soft_type = tmp3;
	} else {
	  soft_type = soft_type->cdr;
	}
	if (!(tmp2 = soft_cast(soft_type, orig_type, flags))) {
	  goto function_cast_fail;
	}
	push_finished_type(tmp2);
	free_type(tmp2);
	tmp2 = NULL;
	while(array_cnt--) push_type(T_ARRAY);
	push_reverse_type(T_MANY);
	while(loop_cnt--) push_reverse_type(T_FUNCTION);
	res = pop_unfinished_type();
	break;
      }
    function_cast_fail:
      type_stack_pop_to_mark();
      break;
    default:
      if (soft_type->type != orig_type->type) break;
      switch(soft_type->type) {
      case T_MAPPING:
	type_stack_mark();
	if ((tmp = soft_cast(soft_type->car, orig_type->car,
			     flags ^ SOFT_WEAKER))) {
	  push_finished_type(tmp);
	} else {
	  push_finished_type(zero_type_string);
	}
	if ((tmp2 = soft_cast(soft_type->cdr, orig_type->cdr, flags))) {
	  push_finished_type(tmp2);
	} else {
	  push_finished_type(zero_type_string);
	}
	push_reverse_type(T_MAPPING);
	res = pop_unfinished_type();
	break;
      case T_ARRAY:
      case T_MULTISET:
      case T_TYPE:
	type_stack_mark();
	if ((tmp = soft_cast(soft_type->car, orig_type->car, flags))) {
	  push_finished_type(tmp);
	} else if (flags & SOFT_WEAKER) {
	  push_finished_type(mixed_type_string);
	} else {
	  push_finished_type(zero_type_string);
	}
	push_type(soft_type->type);
	res = pop_unfinished_type();
	break;
      case T_FLOAT:
	copy_pike_type(res, soft_type);
	break;
      case T_STRING:
	if ((CAR_TO_INT(soft_type) <= CAR_TO_INT(orig_type)) ==
	    !(flags & SOFT_WEAKER)) {
	  copy_pike_type(res, soft_type);
	} else {
	  copy_pike_type(res, orig_type);
	}
	break;
      case T_INT:
	{
	  INT32 min, max;
	  if (flags & SOFT_WEAKER) {
	    if ((min = CAR_TO_INT(soft_type)) > CAR_TO_INT(orig_type)) {
	      min = CAR_TO_INT(orig_type);
	    }
	    if ((max = CDR_TO_INT(soft_type)) < CDR_TO_INT(orig_type)) {
	      max = CDR_TO_INT(orig_type);
	    }
	  } else {
	    if ((min = CAR_TO_INT(soft_type)) < CAR_TO_INT(orig_type)) {
	      min = CAR_TO_INT(orig_type);
	    }
	    if ((max = CDR_TO_INT(soft_type)) > CDR_TO_INT(orig_type)) {
	      max = CDR_TO_INT(orig_type);
	    }
	    if (min > max) break;
	  }
	  type_stack_mark();
	  push_int_type(min, max);
	  res = pop_unfinished_type();
	  break;
	}
      case T_OBJECT:
	if (flags & SOFT_WEAKER) {
	  if (!CDR_TO_INT(orig_type)) {
	    copy_pike_type(res, orig_type);
	  } else if (!CDR_TO_INT(soft_type)) {
	    copy_pike_type(res, soft_type);
	  } else if (CDR_TO_INT(soft_type) == CDR_TO_INT(orig_type)) {
	    if (!CAR_TO_INT(orig_type)) {
	      copy_pike_type(res, orig_type);
	    } else {
	      copy_pike_type(res, soft_type);
	    }
	  } else if (pike_types_le(soft_type, orig_type)) {
	    copy_pike_type(res, orig_type);
	  } else if (pike_types_le(orig_type, soft_type)) {
	    copy_pike_type(res, soft_type);
	  } else {
	    copy_pike_type(res, object_type_string);
	  }
	} else {
	  if (!CDR_TO_INT(orig_type)) {
	    copy_pike_type(res, soft_type);
	  } else if (!CDR_TO_INT(soft_type)) {
	    copy_pike_type(res, orig_type);
	  } else if (CDR_TO_INT(soft_type) == CDR_TO_INT(orig_type)) {
	    if (CAR_TO_INT(orig_type)) {
	      copy_pike_type(res, orig_type);
	    } else {
	      copy_pike_type(res, soft_type);
	    }
	  } else if (pike_types_le(soft_type, orig_type)) {
	    copy_pike_type(res, soft_type);
	  } else if (pike_types_le(orig_type, soft_type)) {
	    copy_pike_type(res, orig_type);
	  }
	}
	break;
      }
      break;
    }
  }
  if (tmp) free_type(tmp);
  if (tmp2) free_type(tmp2);
  if (tmp3) free_type(tmp3);
  return res;
}

/*! @class CompilationHandler
 */

/*! @decl type handle_attribute_constant(string attr, mixed value, @
 *!                                      type arg_type, type cont_type)
 *!
 *!   Handle constant arguments to attributed parameter types.
 *!
 *!   This function is called when a function parameter with the
 *!   attribute @[attr] is called with the constant value @[value].
 *!
 *!   This function is typically used to perform specialized
 *!   argument checking.
 *!
 *! @param arg_type
 *!   The declared type of the parameter.
 *!
 *! @param cont_type
 *!   The type for the continued function.
 *!
 *! @returns
 *!   Returns a new continuation type if it succeeded in strengthening
 *!   the type.
 *!
 *!   Returns @expr{UNDEFINED@} otherwise (this is not an error indication).
 */

/*! @endclass
 */

/* Check whether arg_type may be used as the type of the first argument
 * in a call to fun_type.
 *
 * The first argument has no OR or AND nodes.
 *
 * Returns NULL on failure.
 *
 * Returns continuation function type on success.
 */
static struct pike_type *lower_new_check_call(struct pike_type *fun_type,
					      struct pike_type *arg_type,
					      INT32 flags,
					      struct svalue *sval
#ifdef PIKE_TYPE_DEBUG
					      , INT32 indent
#define CHECK_CALL_ARGS	, indent+1
#else
#define CHECK_CALL_ARGS
#endif /* PIKE_TYPE_DEBUG */
					      )
{
  struct pike_type *res = NULL;
  struct pike_type *tmp;
  struct pike_type *tmp2;
  INT32 array_cnt = 0;

#ifdef PIKE_DEBUG
  if (l_flag>2) {
    fprintf(stderr, "%*slower_new_check_call(", indent*2, "");
    simple_describe_type(fun_type);
    fprintf(stderr, ", ");
    simple_describe_type(arg_type);
    fprintf(stderr, ", 0x%04x, %p)...\n", flags, sval);
  }
#endif /* PIKE_DEBUG */

 loop:
  /* Count the number of array levels. */
  while(fun_type->type == PIKE_T_ARRAY) {
    array_cnt++;
    fun_type = fun_type->car;
  }

  switch(fun_type->type) {
  case T_SCOPE:
  case T_ASSIGN:
  case PIKE_T_NAME:
  case PIKE_T_ATTRIBUTE:
    fun_type = fun_type->cdr;
    goto loop;


  case T_OR:
    res = lower_new_check_call(fun_type->car, arg_type, flags, sval CHECK_CALL_ARGS);
    if (!res) {
      res = lower_new_check_call(fun_type->cdr, arg_type, flags, sval CHECK_CALL_ARGS);
      break;
    }
    tmp = lower_new_check_call(fun_type->cdr, arg_type, flags, sval CHECK_CALL_ARGS);
    if (!tmp) break;
    res = or_pike_types(tmp2 = res, tmp, 1);
    free_type(tmp);
    free_type(tmp2);
    break;

  case T_AND:
    res = lower_new_check_call(fun_type->car, arg_type, flags, sval CHECK_CALL_ARGS);
    if (!res) break;
    tmp = lower_new_check_call(fun_type->cdr, arg_type, flags, sval CHECK_CALL_ARGS);
    if (!tmp) {
      free_type(res);
      res = NULL;
      break;
    }
    if (res == tmp) {
      /* Common case. */
      free_type(tmp);
      break;
    }
    /* and_pike_types() doesn't handle and of functions
     * in the way we want here.
     */
    type_stack_mark();
    push_finished_type(tmp);
    push_finished_type(res);
    push_type(T_AND);
    free_type(tmp);
    free_type(res);
    res = pop_unfinished_type();
    break;

  case T_NOT:
    if (arg_type->type == T_NOT) {
      /* Both sides are inverted. Pop both inversions. */
      arg_type = arg_type->car;
      fun_type = fun_type->car;
      goto loop;
    } else {
      /* Move the inversion to the argument type. */
      type_stack_mark();
      push_finished_type(arg_type);
      push_type(T_NOT);
      arg_type = pop_unfinished_type();
      res = lower_new_check_call(fun_type->car, arg_type, flags, sval CHECK_CALL_ARGS);
      free_type(arg_type);
      if (res) {
	/* Move the inversion back to the function type. */
	if (res->type == T_NOT) {
	  tmp = res->car;
	  free_type(res);
	  res = tmp;
	} else {
	  type_stack_mark();
	  if ((res->type == T_MANY) &&
	      (fun_type->car->type == T_MANY) &&
	      (res->car->type == T_NOT)) {
	    /* Exist criteria is fulfilled.
	     * Reduce !function(!type...:type) to function(mixed...:type).
	     * FIXME: Probably ought to move the inner inversion
	     *        to the result type, but that is incompatible
	     *        with current types.
	     * FIXME: What about the limited number of args case?
	     */
	    push_finished_type(mixed_type_string);
	    free_type(res);
	  } else {
	    /* More arguments to check. */
	    push_finished_type(res);
	    free_type(res);
	    push_type(T_NOT);
	  }
	  res = pop_unfinished_type();
	}
      } else if (!(flags & CALL_LAST_ARG) &&
		 (fun_type->car->type == T_MANY)) {
	/* The next argument might match. */
	add_ref(fun_type);
	res = fun_type;
      }
    }
    break;

  case PIKE_T_PROGRAM:
    tmp = low_object_lfun_type(fun_type->car, LFUN_CREATE);
    if (!tmp) {
      /* FIXME: Multiple cases:
       *          Untyped object.		function(mixed|void...:obj)
       *          Failed to lookup program id.	function(mixed|void...:obj)
       *          Program does not have a create().	function(:obj)
       */

      /* No create() -- No arguments. */
      /* return NULL; */
      copy_pike_type(res, mixed_type_string);
      break;
    } else {
      fun_type = zzap_function_return(tmp, CDR_TO_INT(fun_type->car));
    }
    res = lower_new_check_call(fun_type, arg_type, flags, sval CHECK_CALL_ARGS);
    free_type(fun_type);
    break;

  case PIKE_T_OBJECT:
    fun_type = low_object_lfun_type(fun_type, LFUN_CALL);
    if (fun_type) goto loop;
    
    /* FIXME: Multiple cases:
     *          Untyped object.				mixed
     *          Failed to lookup program id.		mixed
     *          Program does not have the lfun `()().	NULL
     */

    /* FALL_THROUGH */
  case PIKE_T_MIXED:
    copy_pike_type(res, mixed_type_string);
    break;

  case PIKE_T_FUNCTION:
  case T_MANY:
    /* Special case to detect workarounds for the old
     * function call checker.
     */
    tmp = NULL;
    if ((fun_type->car->type == T_NOT) &&
	(fun_type->car->car->type == T_OR) &&
	((fun_type->car->car->car->type == T_MIXED) ||
	 (fun_type->car->car->cdr->type == T_MIXED))) {
      /* Rebuild the function type without the negated mixed
       * in the first argument.
       */
      type_stack_mark();
      push_finished_type(fun_type->cdr);
      if (fun_type->car->car->car->type == T_MIXED) {
	push_finished_type(fun_type->car->car->cdr);
      } else {
	push_finished_type(fun_type->car->car->car);
      }
      push_type(T_NOT);
      push_type(fun_type->type);
      tmp = fun_type = pop_unfinished_type();
    }
    /* Note: Use the low variants of pike_types_le and match_types,
     *       so that markers get set and kept. */
#ifdef PIKE_DEBUG
    if (l_flag>2) {
      fprintf(stderr, "%*sChecking argument type ", indent*2+2, "");
      simple_describe_type(arg_type);
      fprintf(stderr, " against function type ");
      simple_describe_type(fun_type);
      fprintf(stderr, ".\n");
    }
#endif /* PIKE_DEBUG */
    if (!low_pike_types_le(arg_type, tmp2 = fun_type->car, 0, 0) &&
	((flags & CALL_STRICT) ||
 	 !low_match_types(arg_type, tmp2, NO_SHORTCUTS))) {
      /* No match. */
#ifdef PIKE_DEBUG
      if (l_flag>2) {
	fprintf(stderr, "%*sNo match.\n", indent*2+2, "");
      }
#endif /* PIKE_DEBUG */
      res = NULL;
      if (tmp) free_type(tmp);
      break;
    }
    /* Match. */
    if (fun_type->type == PIKE_T_FUNCTION) {
      /* Advance to the next argument. */
      fun_type = fun_type->cdr;
      if ((flags & CALL_LAST_ARG) &&
	  (fun_type->type == PIKE_T_FUNCTION)) {
	/* There are more required arguments. */
#ifdef PIKE_DEBUG
	if (l_flag>2) {
	  fprintf(stderr, "%*sMore arguments required.\n", indent*2+2, "");
	}
#endif /* PIKE_DEBUG */
	res = NULL;
	if (tmp) free_type(tmp);
	break;
      }
    }
    type_stack_mark();
    push_finished_type_with_markers(fun_type, b_markers, 0);
    res = pop_unfinished_type();
    if (tmp) free_type(tmp);

    if ((Pike_compiler->compiler_pass == 2) && sval) {
      while (tmp2->type == PIKE_T_NAME) {
	tmp2 = tmp2->cdr;
      }

      if (tmp2->type == PIKE_T_ATTRIBUTE) {
	/* Perform extra argument checking based on the attribute. */
	/* FIXME: Support multiple attributes. */
	ref_push_string((struct pike_string *)tmp2->car);
	push_svalue(sval);
	ref_push_type_value(tmp2->cdr);
	ref_push_type_value(res);
	if (safe_apply_handler("handle_attribute_constant", error_handler,
			       compat_handler, 4, 0)) {
	  if ((Pike_sp[-1].type == PIKE_T_TYPE)) {
	    type_stack_mark();
	    push_finished_type(Pike_sp[-1].u.type);
	    push_finished_type(res);
	    push_type(T_AND);
	    free_type(res);
	    res = pop_unfinished_type();
	  }
	  pop_stack();
	}
      }
    }
#ifdef PIKE_DEBUG
    if (l_flag>2) {
      fprintf(stderr, "%*sSuccess.\n", indent*2+2, "");
    }
#endif /* PIKE_DEBUG */
    break;
  default:
    /* Not a callable. */
    break;
  }
  if (!array_cnt || !res) {
#ifdef PIKE_DEBUG
    if (l_flag>2) {
      if (res) {
	fprintf(stderr, "%*s==> ", indent*2, "");
	simple_describe_type(res);
      } else {
	fprintf(stderr, "%*s==> NULL", indent*2, "");
      }
      fprintf(stderr, "\n");
    }
#endif /* PIKE_DEBUG */
    return res;
  }

  type_stack_mark();
  push_finished_type(res);
  free_type(res);
  while(array_cnt--) {
    push_type(PIKE_T_ARRAY);
  }
  res = pop_type();

#ifdef PIKE_DEBUG
  if (l_flag>2) {
    fprintf(stderr, "%*s==> ", indent*2, "");
    simple_describe_type(res);
    fprintf(stderr, "\n");
  }
#endif /* PIKE_DEBUG */

  return res;
}

/* Check whether arg_type may be used as the type of the first argument
 * in a call to fun_type.
 *
 * If the argument is a constant, sval will contain a pointer to it.
 *
 * Returns NULL on failure.
 *
 * Returns continuation function type on success.
 */
struct pike_type *low_new_check_call(struct pike_type *fun_type,
				     struct pike_type *arg_type,
				     INT32 flags,
				     struct svalue *sval)
{
  struct pike_type *tmp;
  struct pike_type *tmp2;
  struct pike_type *res;

  /* FIXME: In strict mode we need to differentiate between
   *        two different kinds of OR:
   *          * Complex types, eg
   *              function(int:int)|function(float:float)
   *            or
   *              mapping(string:int)|mapping(int:string)
   *            where a value can have both types at the
   *            same time.
   *          * Alternate types, eg
   *              int|string
   *            where a value only can have one of the
   *            types at a time.
   *        In strict mode the former should be split here,
   *        and the latter kept.
   *        In non-strict mode both should be split here.
   * Suggestion:
   *   Introduce a new operator (UNION?) for the former case.
   */

 loop:
  clear_markers();
  /* First split the argument type into basic types. */
  switch(arg_type->type) {
  case PIKE_T_SCOPE:
  case T_ASSIGN:
  case PIKE_T_NAME:
    arg_type = arg_type->cdr;
    goto loop;

  case T_OR:
    if (!(tmp = low_new_check_call(fun_type, arg_type->car, flags, sval))) {
      if (flags & CALL_STRICT) {
	return NULL;
      }
      arg_type = arg_type->cdr;
      goto loop;
    }
    if (!(tmp2 = low_new_check_call(fun_type, arg_type->cdr, flags, sval))) {
      if (flags & CALL_STRICT) {
	return NULL;
      }
      return tmp;
    }
    res = or_pike_types(tmp, tmp2, 1);
    free_type(tmp);
    free_type(tmp2);
    return res;

  case T_AND:
    if (!(tmp = low_new_check_call(fun_type, arg_type->car, flags, sval))) {
      return NULL;
    }
    if (!(tmp2 = low_new_check_call(fun_type, arg_type->cdr, flags, sval))) {
      free_type(tmp);
      return NULL;
    }
    res = and_pike_types(tmp, tmp2);
    free_type(tmp);
    free_type(tmp2);
    return res;

  case T_VOID:
    if (!(flags & CALL_7_6)) {
      /* Promote void arguments to zero. */
      arg_type = zero_type_string;
    }
    break;
  }

  if (!(tmp = lower_new_check_call(fun_type, arg_type, flags, sval
#ifdef PIKE_TYPE_DEBUG
				   , 0
#endif
				   ))) {
    return NULL;
  }
  return tmp;
}

/* Return the return type for the function type fun_type, if
 * no further arguments are passed.
 *
 * Returns NULL if more arguments are required.
 *
 * Returns a the type of the return value otherwise.
 */
struct pike_type *new_get_return_type(struct pike_type *fun_type,
				      INT32 flags)
{
  struct pike_type *res = NULL;
  struct pike_type *tmp;
  struct pike_type *tmp2;
  INT32 array_cnt = 0;

#ifdef PIKE_DEBUG
  if (l_flag>2) {
    fprintf(stderr, "  Getting return type for ");
    simple_describe_type(fun_type);
    fprintf(stderr, "... ");
  }
#endif /* PIKE_DEBUG */

 loop:
  /* Count the number of array levels. */
  while(fun_type->type == PIKE_T_ARRAY) {
    array_cnt++;
    fun_type = fun_type->car;
  }

  switch(fun_type->type) {
  case PIKE_T_SCOPE:
  case T_ASSIGN:
  case PIKE_T_NAME:
  case PIKE_T_ATTRIBUTE:
    fun_type = fun_type->cdr;
    goto loop;

  case PIKE_T_RING:
    fun_type = fun_type->car;
    goto loop;

  case T_OR:
    if (!(res = new_get_return_type(fun_type->car, flags))) {
      fun_type = fun_type->cdr;
      goto loop;
    }
    if (!(tmp = new_get_return_type(fun_type->cdr, flags))) {
      break;
    }
    if ((res == void_type_string) || (tmp == void_type_string)) {
      /* Note: Promote void to zero in the return value
       *       when there's another non-void result.
       */
      if (tmp == void_type_string) {
	/* Keep res as is. */
	free_type(tmp);
      } else {
	free_type(res);
	res = tmp;
      }
      break;
    }
    res = or_pike_types(tmp2 = res, tmp, 1);
    free_type(tmp2);
    free_type(tmp);
    break;
  case T_AND:
    if (!(res = new_get_return_type(fun_type->car, flags))) {
      break;
    }
    if (!(tmp = new_get_return_type(fun_type->cdr, flags))) {
      free_type(res);
      res = NULL;
      break;
    }
    res = and_pike_types(tmp2 = res, tmp);
    free_type(tmp);
    free_type(tmp2);
    break;
  case T_NOT:
    /* Doesn't match. */
    break;
  case PIKE_T_PROGRAM:
    tmp = low_object_lfun_type(fun_type->car, LFUN_CREATE);
    if (!tmp) {
      /* No create(). */
      add_ref(fun_type->car);
      res = fun_type->car;
      break;
    } else {
      fun_type = zzap_function_return(tmp, CDR_TO_INT(fun_type->car));
    }
    res = new_get_return_type(fun_type, flags);
    free_type(fun_type);
    break;
  case PIKE_T_OBJECT:
    fun_type = low_object_lfun_type(fun_type, LFUN_CALL);
    if (fun_type) goto loop;
    /* FIXME: Multiple cases:
     *          Untyped object.
     *          Failed to lookup program id.
     *          Program does not have the lfun `()().
     */

    /* FALL_THROUGH */
  case PIKE_T_MIXED:
    copy_pike_type(res, mixed_type_string);
    break;

  case PIKE_T_FUNCTION:
    do {
      if (!match_types(fun_type->car, void_type_string)) {
	/* Too few arguments. */
	break;
      }
      fun_type = fun_type->cdr;
    } while(fun_type->type == PIKE_T_FUNCTION);
    if (fun_type->type != T_MANY) {
      /* Still too few arguments. */
      break;
    }
  case T_MANY:
    copy_pike_type(res, fun_type->cdr);
    break;

  default:
    /* Not a callable. */
    break;
  }

  if (!res) {
#ifdef PIKE_DEBUG
    if (l_flag>2) {
      fprintf(stderr, "Failed.\n");
    }
#endif /* PIKE_DEBUG */
    return NULL;
  }

#ifdef PIKE_DEBUG
  if (l_flag>2) {
    fprintf(stderr, "Ok, cleaning up markers... ");
  }
#endif /* PIKE_DEBUG */

  type_stack_mark();

  /* Get rid of any remaining markers. */
  clear_markers();
  push_finished_type_with_markers(res, a_markers, 0);

  free_type(res);

  while(array_cnt--) {
    push_type(PIKE_T_ARRAY);
  }

#ifdef PIKE_DEBUG
  if (l_flag>2) {
    fprintf(stderr, "Done.\n");
  }
#endif /* PIKE_DEBUG */

  return pop_unfinished_type();
}

/* Adjust the argument type.
 *
 * Get rid of void and setvar.
 */
static struct pike_type *low_get_first_arg_type(struct pike_type *arg_type,
						INT32 flags)
{
  struct pike_type *tmp;
  struct pike_type *tmp2;
  struct pike_type *res;

  if (!arg_type) return NULL;

 loop:
  if (!(flags & FILTER_KEEP_VOID) ||
      (arg_type->flags & (PT_FLAG_MARKER|PT_FLAG_ASSIGN))) {
    /* There's markers, assigns or void's to to take care of. */
    switch(arg_type->type) {
    case T_OR:
      if ((tmp = low_get_first_arg_type(arg_type->cdr, flags))) {
	res =
	  or_pike_types(tmp,
			tmp2 = low_get_first_arg_type(arg_type->car, flags),
			1);
	if (tmp2) free_type(tmp2);
	free_type(tmp);
	return res;
      }
      arg_type = arg_type->car;
      goto loop;

    case T_ASSIGN:
      arg_type = arg_type->cdr;
      goto loop;

    case T_NOT:
      /* Recognize some common workarounds for the old function
       * call checker.
       */
      if ((arg_type->car->type == T_OR) &&
	  ((arg_type->car->car->type == T_MIXED) ||
	   (arg_type->car->cdr->type == T_MIXED))) {
	/* Workaround used for some operators; typically
	 *
	 * function(mixed...:mixed) & !function(!(object|mixed):mixed)
	 *
	 * In this case we only want the object, and not the mixed.
	 */
	if (arg_type->car->car->type == T_MIXED) {
	  tmp = low_get_first_arg_type(arg_type->car->cdr,
				       flags|FILTER_KEEP_VOID);
	} else {
	  tmp = low_get_first_arg_type(arg_type->car->car,
				       flags|FILTER_KEEP_VOID);
	}
	type_stack_mark();
	push_finished_type(tmp);
	free_type(tmp);
	push_type(arg_type->type);
	return pop_unfinished_type();
      }
    case T_ARRAY:
    case T_MULTISET:
      /* Keep void! */
      tmp = low_get_first_arg_type(arg_type->car, flags|FILTER_KEEP_VOID);
      type_stack_mark();
      push_finished_type(tmp);
      free_type(tmp);
      push_type(arg_type->type);
      return pop_unfinished_type();

    case T_MAPPING:
    case T_TUPLE:
      /* Keep void! */
      type_stack_mark();
      tmp = low_get_first_arg_type(arg_type->cdr, flags|FILTER_KEEP_VOID);
      push_finished_type(tmp);
      free_type(tmp);
      tmp = low_get_first_arg_type(arg_type->car, flags|FILTER_KEEP_VOID);
      push_finished_type(tmp);
      free_type(tmp);
      push_type(arg_type->type);
      return pop_unfinished_type();

    case T_VOID:
      if (!(flags & FILTER_KEEP_VOID)) {
	return NULL;
      }
      /* FALL_THROUGH */
    default:
      break;
    }
  }
  add_ref(arg_type);
  return arg_type;
}

/* Return the type of the first argument to a function of the type fun_type
 *
 * Returns NULL on failure. Eg not callable or no more args accepted.
 *
 * Returns the argument type on success.
 */
struct pike_type *get_first_arg_type(struct pike_type *fun_type,
				     INT32 flags)
{
  struct pike_type *res = NULL;
  struct pike_type *tmp;
  struct pike_type *tmp2;
 loop:
  /* Get rid of the array levels. */
  while(fun_type->type == PIKE_T_ARRAY) {
    fun_type = fun_type->car;
  }

  switch(fun_type->type) {
  case PIKE_T_SCOPE:
  case T_ASSIGN:
  case PIKE_T_NAME:
  case PIKE_T_ATTRIBUTE:
  case PIKE_T_RING:
    fun_type = fun_type->cdr;
    goto loop;

  case T_OR:
    if (!(res = get_first_arg_type(fun_type->car, flags))) {
      fun_type = fun_type->cdr;
      goto loop;
    }
    if (!(tmp = get_first_arg_type(fun_type->cdr, flags))) {
      break;
    }
    res = or_pike_types(tmp2 = res, tmp, 1);
    free_type(tmp);
    free_type(tmp2);
    break;
  case T_AND:
    if (!(res = get_first_arg_type(fun_type->car, flags))) {
      break;
    }
    if (!(tmp = get_first_arg_type(fun_type->cdr, flags))) {
      free_type(res);
      res = NULL;
      break;
    }
    /* NOTE: OR and not AND in some cases!
     *
     *   !function(!string:mixed)&function(string|int:string)
     *     ==>
     *   string | string|int
     *
     * This is however not true in the case where neither is inverted:
     *
     *   function(attribute(sprintf_args, mixed)...:string) &
     *   function(object|string:string)
     *     ==>
     *   attribute(sprintf_args, mixed) & object|string
     */
    if ((fun_type->car->type == T_NOT) == (fun_type->cdr->type == T_NOT)) {
      res = and_pike_types(tmp2 = res, tmp);
    } else {
      res = or_pike_types(tmp2 = res, tmp, 1);
    }
    free_type(tmp);
    free_type(tmp2);
    break;
  case T_NOT:
    if (!(res = get_first_arg_type(fun_type->car, flags))) {
      break;
    }
    if (res->type == T_NOT) {
      copy_pike_type(tmp, res->car);
      free_type(res);
      res = tmp;
    } else {
      type_stack_mark();
      push_finished_type(res);
      free_type(res);
      push_type(T_NOT);
      res = pop_unfinished_type();
    }
    break;
  case PIKE_T_PROGRAM:
    if ((fun_type = low_object_lfun_type(fun_type->car, LFUN_CREATE))) {
      /* No need to adjust the return type, since we're only
       * looking at the arguments.
       */
      goto loop;
    }
    /* FIXME: Multiple cases:
     *          Untyped object.			function(mixed...:object)
     *          Failed to lookup program id.	function(mixed...:object)
     *          Program does not have create().	function(:object)
     */
    /* No create() ==> no arguments. */
    copy_pike_type(res, mixed_type_string);
    break;
  case PIKE_T_OBJECT:
    fun_type = low_object_lfun_type(fun_type, LFUN_CALL);
    if (fun_type) {
      goto loop;
    }
    /* FIXME: Multiple cases:
     *          Untyped object.
     *          Failed to lookup program id.
     *          Program does not have the lfun `()().
     */

    /* FALL_THROUGH */
  case PIKE_T_MIXED:
    copy_pike_type(res, mixed_type_string);
    break;

  case PIKE_T_FUNCTION:
  case T_MANY:
    if ((res = fun_type->car)->type == T_VOID) {
      res = NULL;
      break;
    }
    res = low_get_first_arg_type(res, 0);
    break;

  default:
    /* Not a callable. */
    break;
  }

  return res;
}

/* NOTE: fun_type loses a reference. */
struct pike_type *new_check_call(struct pike_string *fun_name,
				 struct pike_type *fun_type,
				 node *args, INT32 *argno)
{
  struct pike_type *tmp = NULL;
  struct pike_type *res = NULL;
  struct svalue *sval = NULL;
  int flags = 0;

  debug_malloc_touch(fun_type);

  while (args && (args->token == F_ARG_LIST) && fun_type) {
    fun_type = new_check_call(fun_name, fun_type, CAR(args), argno);
    debug_malloc_touch(fun_type);
    args = CDR(args);
  }

  if (!args || !fun_type) {
    debug_malloc_touch(fun_type);
    return fun_type;
  }

  (*argno)++;

  if (args->token == F_CONSTANT) {
    sval = &args->u.sval;
  }

#ifdef PIKE_DEBUG
  if (l_flag>2) {
    fprintf(stderr, "  Checking argument #%d... ", *argno);
    simple_describe_type(args->type);
    if (sval) {
      fprintf(stderr, "\n  Constant of type %s", get_name_of_type(sval->type));
    }
    fprintf(stderr, "\n  fun_type: ");
    simple_describe_type(fun_type);
  }
#endif /* PIKE_DEBUG */

  if (TEST_COMPAT(7, 6)) {
    /* Attempt to reduce strictness to Pike 7.6 levels. */
    flags |= CALL_7_6;
  }

  if (args->token == F_PUSH_ARRAY) {
    struct pike_type *prev = fun_type;
    int cnt = 256;
    /* This token can expand to anything between zero and MAX_ARGS args. */

#ifdef PIKE_DEBUG
    if (l_flag>2) {
      fprintf(stderr, "\n  The argument is a splice operator.\n");
    }
#endif /* PIKE_DEBUG */

    copy_pike_type(res, fun_type);

    /* Loop until we get a stable fun_type, or it's an invalid argument. */
    while ((fun_type = low_new_check_call(debug_malloc_pass(prev),
					  debug_malloc_pass(args->type),
					  flags, sval)) &&
	   (fun_type != prev) && --cnt) {

#ifdef PIKE_DEBUG
      if (l_flag>4) {
	fprintf(stderr, "\n    sub_result_type: ");
	simple_describe_type(fun_type);
      }
#endif /* PIKE_DEBUG */

      res = dmalloc_touch(struct pike_type *,
			  or_pike_types(debug_malloc_pass(tmp = res),
					debug_malloc_pass(fun_type), 1));
#ifdef PIKE_DEBUG
      if (l_flag>4) {
	fprintf(stderr, "\n    joined_type: ");
	simple_describe_type(res);
      }
#endif /* PIKE_DEBUG */

      if ((res == tmp) || (res == fun_type)) {
	free_type(tmp);
	break;
      }
      free_type(tmp);

      free_type(prev);
      prev = fun_type;
    }
    free_type(prev);
    if (fun_type) {
      /* Max args reached or stable type. */
      free_type(fun_type);
    } else {
      /* The splice values are invalid for later arguments. */
      if (cnt == 256) {
	yywarning("In argument %d to %S: The @-operator argument must be an empty array.",
		  *argno, fun_name);
      } else if (lex.pragmas & ID_STRICT_TYPES) {
	yywarning("In argument %d to %S: The @-operator argument has a max length of %d.",
		  *argno, fun_name, 256-cnt);
      }
    }

#ifdef PIKE_DEBUG
    if (l_flag>2) {
      fprintf(stderr, "\n  result: ");
      simple_describe_type(res);
      fprintf(stderr, " OK.\n");
    }
#endif /* PIKE_DEBUG */

    return res;
  } else if ((res = low_new_check_call(fun_type, args->type, flags, sval))) {
    /* OK. */
#ifdef PIKE_DEBUG
    if (l_flag>2) {
      fprintf(stderr, " OK.\n");
    }
#endif /* PIKE_DEBUG */
    if (lex.pragmas & ID_STRICT_TYPES) {
      if (!(tmp = low_new_check_call(fun_type, args->type,
				     flags|CALL_STRICT, sval))) {
	yywarning("Type mismatch in argument %d to %S.",
		  *argno, fun_name);
	if ((tmp = get_first_arg_type(fun_type, 0))) {
	  yytype_error(NULL, tmp, args->type, YYTE_IS_WARNING);
	  free_type(tmp);
	} else {
	  yytype_error(NULL, NULL, args->type, YYTE_IS_WARNING);
	}
      } else {
	free_type(tmp);
      }
    }
    free_type(fun_type);
    return res;
  }

  if ((tmp = get_first_arg_type(fun_type, flags))) {
    struct pike_type *tmp2;

#ifdef PIKE_DEBUG
    if (l_flag>2) {
      fprintf(stderr, " Bad argument.\n");
    }
#endif /* PIKE_DEBUG */
    my_yyerror("Bad argument %d to %S.",
	       *argno, fun_name);
    yytype_error(NULL, tmp, args->type, 0);

    /* Try advancing with the suggested type, so that we can check
     * the rest of the arguments.
     */
    if ((tmp2 = low_new_check_call(fun_type, tmp, flags, NULL))) {
      /* Succeeded. */
      free_type(fun_type);
      free_type(tmp);
#ifdef PIKE_DEBUG
      if (l_flag>2) {
	fprintf(stderr, "  Created continuation type: ");
	simple_describe_type(tmp2);
	fprintf(stderr, " OK.\n");
      }
#endif /* PIKE_DEBUG */
      return tmp2;
    }
#ifdef PIKE_DEBUG
    if (l_flag>2) {
      fprintf(stderr, "\n  Failed to create continuation type.\n");
    }
#endif /* PIKE_DEBUG */
    free_type(tmp);
  } else {
#ifdef PIKE_DEBUG
    if (l_flag>2) {
      fprintf(stderr, " Too many arguments.\n");
    }
#endif /* PIKE_DEBUG */
    my_yyerror("Too many arguments to %S (expected %d arguments).",
	       fun_name, *argno - 1);
    yytype_error(NULL, NULL, args->type, 0);
  }
  free_type(fun_type);
  return NULL;
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

    case PIKE_T_ATTRIBUTE:
      {
	struct pike_type *res;
	if ((res = zzap_function_return(a->cdr, id))) {
	  type_stack_mark();
	  push_finished_type(res);
	  push_type_attribute((struct pike_string *)a->car);
	  free_type(res);
	  res = pop_unfinished_type();
	}
	return res;
      }

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
      struct array *a = s->u.array;

      type_stack_mark();
#if 0
      {
	struct pike_type *arg_type;
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
	push_finished_type(arg_type);
	free_type(arg_type);
      }
#else /* !0 */
      if (a->size) {
	push_type_field(a->type_field);
      }else
	push_finished_type(zero_type_string);
#endif /* 0 */
      push_type(T_ARRAY);
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

  case T_STRING:
    type_stack_mark();
    if (s->u.string->len) {
      /* FIXME: Could be extended to detect 7-bit strings, etc. */
      push_string_type(8<<s->u.string->size_shift);
    } else {
      push_string_type(0);
    }
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
    case PIKE_T_ATTRIBUTE:
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
      my_yyerror("Expected: %S", s1);
      my_yyerror("Got     : %S", s2);
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

static void low_make_pike_type(unsigned char *type_string,
			       unsigned char **cont);

static void low_make_function_type(unsigned char *type_string,
				   unsigned char **cont)
{
  if (*type_string == T_MANY) {
    low_make_pike_type(type_string+1, cont);
    low_make_pike_type(*cont, cont);
    push_reverse_type(T_MANY);
    return;
  }
  low_make_pike_type(type_string, cont);
  low_make_function_type(*cont, cont);
  push_reverse_type(T_FUNCTION);
}

static void low_make_pike_type(unsigned char *type_string,
			       unsigned char **cont)
{
  unsigned INT32 type;
  struct pike_type *tmp;

  switch(type = *type_string) {
  case T_SCOPE:
    Pike_fatal("Not supported yet.\n");
  case T_ASSIGN:
    if ((type_string[1] < '0') || (type_string[1] > '9')) {
      Pike_fatal("low_make_pike_type(): Bad marker: %d\n", type_string[1]);
    }
    low_make_pike_type(type_string+2, cont);
    push_assign_type(type_string[1]);
    break;
  case T_OR:
  case T_AND:
    /* Order independant */
    /* FALL_THROUGH */
    
  case T_MANY:
  case T_TUPLE:
  case T_MAPPING:
  case PIKE_T_RING:
    /* Order dependant */
    low_make_pike_type(type_string+1, cont);
    low_make_pike_type(*cont, cont);
    push_reverse_type(type);
    break;
  case T_FUNCTION:
    low_make_function_type(type_string+1, cont);
    break;
  case T_ARRAY:
  case T_MULTISET:
  case T_TYPE:
  case T_NOT:
  case T_PROGRAM:
    low_make_pike_type(type_string+1, cont);
    push_reverse_type(type);
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
    /* Marker type */
    /* FALL_THROUGH */
  case T_FLOAT:
  case T_MIXED:
  case T_VOID:
  case T_ZERO:
  case PIKE_T_UNKNOWN:
    /* Leaf type */
    *cont = type_string+1;
    push_type(type);
    break;

  case T_STRING:
    *cont = type_string + 1;
    push_string_type(32);
    break;

  case PIKE_T_NSTRING:
    *cont = type_string + 2;	/* 1 + 1 */
    /* FIXME: Add check for valid bitwidth (0..32). */
    push_string_type(type_string[1]);
    break;

  case T_INT:
    {
      INT32 min = extract_type_int((char *)type_string+1);
      INT32 max = extract_type_int((char *)type_string+5);
      
      *cont = type_string + 9;	/* 2*sizeof(INT32) + 1 */
      push_int_type(min, max);
      break;
    }

  case PIKE_T_INT_UNTYPED:
    *cont = type_string + 1;
    push_int_type((INT32)-0x80000000, 0x7fffffff);
    break;

  case T_OBJECT:
    *cont = type_string + 6;	/* 1 + sizeof(INT32) + 1 */
    push_object_type(type_string[1], extract_type_int((char *)type_string+2));
    break;

  case PIKE_T_NAME:
  case PIKE_T_ATTRIBUTE:
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
	bytes = strlen((char *)type_string+2);
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
      low_make_pike_type(type_string + 2 + bytes + (1<<size_shift), cont);
      if (type_string[0] == PIKE_T_NAME) {
	push_type_name(str = end_shared_string(str));
      } else {
	push_type_attribute(str = end_shared_string(str));
      }
      free_string(str);
      break;
    }
  default:
    Pike_fatal("compile_type_string(): Error in type string %d.\n", type);
    /* NOT_REACHED */
    break;
  }
}

/* Make a pike-type from a serialized (old-style) type. */
struct pike_type *debug_make_pike_type(const char *serialized_type)
{
  unsigned char *dummy;
  type_stack_mark();
  low_make_pike_type((unsigned char *)serialized_type, &dummy);
  return pop_unfinished_type();
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
    case PIKE_T_ATTRIBUTE:
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
    case PIKE_T_ZERO:
    case T_VOID:
      return 1;
  default:
    Pike_fatal("pike_type_allow_premature_toss: Unknown type (code: %d)\n",
	       type->type);
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
  case PIKE_T_ATTRIBUTE:
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

  case T_STRING:
    {
      INT32 width = (INT32)CAR_TO_INT(t);
      if (width == 32) {
	my_putchar(T_STRING);
      } else {
	my_putchar(PIKE_T_NSTRING);
	my_putchar(width & 0xff);
      }
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

#if 0
#ifdef DEBUG_MALLOC
static void gc_mark_external_types(struct callback *cb, void *a, void *b)
{
  GC_ENTER(pike_type_hash, PIKE_T_TYPE) {
    if (string0_type_string)
      gc_mark_external(string0_type_string, " as string0_type_string");
    if (string_type_string)
      gc_mark_external(string_type_string, " as string_type_string");
    if (int_type_string)
      gc_mark_external(int_type_string, " as int_type_string");
    if (object_type_string)
      gc_mark_external(object_type_string, " as object_type_string");
    if (program_type_string)
      gc_mark_external(program_type_string, " as program_type_string");
    if (float_type_string)
      gc_mark_external(float_type_string, " as float_type_string");
    if (mixed_type_string)
      gc_mark_external(mixed_type_string, " as mixed_type_string");
    if (array_type_string)
      gc_mark_external(array_type_string, " as array_type_string");
    if (multiset_type_string)
      gc_mark_external(multiset_type_string, " as multiset_type_string");
    if (mapping_type_string)
      gc_mark_external(mapping_type_string, " as mapping_type_string");
    if (function_type_string)
      gc_mark_external(function_type_string, " as function_type_string");
    if (type_type_string)
      gc_mark_external(type_type_string, " as type_type_string");
    if (void_type_string)
      gc_mark_external(void_type_string, " as void_type_string");
    if (zero_type_string)
      gc_mark_external(zero_type_string, " as zero_type_string");
    if (any_type_string)
      gc_mark_external(any_type_string, " as any_type_string");
    if (weak_type_string)
      gc_mark_external(weak_type_string, " as weak_type_string");
  } GC_LEAVE;
}

static struct callback *dmalloc_gc_callback = NULL;
#endif /* DEBUG_MALLOC */
#endif /* 0 */

static struct mapping *builtin_attributes = NULL;

void register_attribute_handler(struct pike_string *attr,
				struct svalue *handler)
{
  mapping_string_insert(builtin_attributes, attr, handler);
}

static void f___register_attribute_handler(INT32 args)
{
  if (args < 2) SIMPLE_TOO_FEW_ARGS_ERROR("__register_attribute_handler", 2);
  if (args > 2) {
    pop_n_elems(args-2);
    args = 2;
  }
  if (Pike_sp[-2].type != PIKE_T_STRING) {
    SIMPLE_BAD_ARG_ERROR("__register_attribute_handler", 1, "string");
  }
  mapping_insert(builtin_attributes, Pike_sp-2, Pike_sp-1);
  pop_n_elems(args);
}

static void f___handle_attribute_constant(INT32 args)
{
  struct svalue *sval;
  if (args < 4) SIMPLE_TOO_FEW_ARGS_ERROR("__handle_attribute_constant", 4);
  if (args > 4) {
    pop_n_elems(args-4);
    args = 4;
  }
  if (Pike_sp[-4].type != PIKE_T_STRING) {
    SIMPLE_BAD_ARG_ERROR("__handle_attribute_constant", 1, "string");
  }
  if ((sval = low_mapping_lookup(builtin_attributes, Pike_sp-4))) {
    apply_svalue(sval, 4);
  } else {
    pop_n_elems(args);
    push_undefined();
  }
}

void init_types(void)
{
  /* Initialize hashtable here. */
  pike_type_hash = (struct pike_type **)xalloc(sizeof(struct pike_type *) *
					       PIKE_TYPE_HASH_SIZE);
  MEMSET(pike_type_hash, 0, sizeof(struct pike_type *) * PIKE_TYPE_HASH_SIZE);
  pike_type_hash_size = PIKE_TYPE_HASH_SIZE;
  init_pike_type_blocks();

  string0_type_string = CONSTTYPE(tStr0);
  string_type_string = CONSTTYPE(tStr32);
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
  /* add_ref(weak_type_string);	*//* LEAK */

  builtin_attributes = allocate_mapping(20);

  ADD_EFUN("__register_attribute_handler", f___register_attribute_handler,
	   tFunc(tSetvar(0, tStr)
		 tFunc(tVar(0) tMix tType(tMix) tType(tMix), tType(tMix)),
		 tVoid), 0);

  ADD_EFUN("__handle_attribute_constant", f___handle_attribute_constant,
	   tFunc(tStr tMix tType(tMix) tType(tMix), tType(tMix)), 0);

#if 0
#ifdef DEBUG_MALLOC
  dmalloc_gc_callback = add_gc_callback(gc_mark_external_types, NULL, NULL);
#endif /* DEBUG_MALLOC */
#endif /* 0 */
}

void cleanup_pike_types(void)
{
#ifdef DO_PIKE_CLEANUP
  struct pike_type_location *t = all_pike_type_locations;

  while(t) {
    free_type(t->t);
    t = t->next;
  }
#endif /* DO_PIKE_CLEANUP */

  if (builtin_attributes) {
    free_mapping(builtin_attributes);
    builtin_attributes = NULL;
  }

  clear_markers();

  free_type(string0_type_string);
  string0_type_string = NULL;
  free_type(string_type_string);
  string_type_string = NULL;
  free_type(int_type_string);
  int_type_string = NULL;
  free_type(float_type_string);
  float_type_string = NULL;
  free_type(function_type_string);
  function_type_string = NULL;
  free_type(object_type_string);
  object_type_string = NULL;
  free_type(program_type_string);
  program_type_string = NULL;
  free_type(array_type_string);
  array_type_string = NULL;
  free_type(multiset_type_string);
  multiset_type_string = NULL;
  free_type(mapping_type_string);
  mapping_type_string = NULL;
  free_type(type_type_string);
  type_type_string = NULL;
  free_type(mixed_type_string);
  mixed_type_string = NULL;
  free_type(void_type_string);
  void_type_string = NULL;
  free_type(zero_type_string);
  zero_type_string = NULL;
  free_type(any_type_string);
  any_type_string = NULL;
  free_type(weak_type_string);
  weak_type_string = NULL;

#if 0
#ifdef DEBUG_MALLOC
  remove_callback(dmalloc_gc_callback);
#endif /* DEBUG_MALLOC */
#endif /* 0 */
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

#if 0
#ifdef PIKE_DEBUG

void gc_mark_type_as_referenced(struct pike_type *t)
{
  if (gc_mark(t)) {
    GC_ENTER(t, PIKE_T_TYPE) {
      switch(t->type) {
      case PIKE_T_SCOPE:
      case T_ASSIGN:
      case PIKE_T_NAME:
      case PIKE_T_ATTRIBUTE:
	if (t->cdr) gc_mark_type_as_referenced(t->cdr);
	break;
      case PIKE_T_FUNCTION:
      case T_MANY:
      case PIKE_T_RING:
      case PIKE_T_TUPLE:
      case PIKE_T_MAPPING:
      case T_OR:
      case T_AND:
	if (t->cdr) gc_mark_type_as_referenced(t->cdr);
      /* FALL_THOUGH */
      case PIKE_T_ARRAY:
      case PIKE_T_MULTISET:
      case T_NOT:
      case PIKE_T_TYPE:
      case PIKE_T_PROGRAM:
	if (t->car) gc_mark_type_as_referenced(t->car);
	break;
      }      
    } GC_LEAVE;
  }
}

unsigned gc_touch_all_types(void)
{
  size_t e;
  unsigned n = 0;
  if (!pike_type_hash) return 0;
  for(e=0;e<pike_type_hash_size;e++)
  {
    struct pike_type *t;
    for(t = pike_type_hash[e]; t; t=t->next) debug_gc_touch(t), n++;
  }
  return n;
}

void gc_mark_all_types(void)
{
  size_t e;
  if(!pike_type_hash) return;
  for(e=0;e<pike_type_hash_size;e++)
  {
    struct pike_type *t;
    for(t=pike_type_hash[e]; t; t=t->next) {
      if (gc_is_referenced(t)) {
	gc_mark_type_as_referenced(t);
      }
    }
  }
}

size_t gc_free_all_unreferenced_types(void)
{
  size_t unreferenced = 0;
  size_t e;
  for (e = 0; e < pike_type_hash_size; e++) {
    struct pike_type *t;
  loop:
    for (t = pike_type_hash[e]; t; t = t->next) {
      struct marker *m = find_marker(t);
      if (!m) continue;
      if ((m->flags & GC_GOT_EXTRA_REF) == GC_GOT_EXTRA_REF) {
	if (gc_do_free(t)) {
	  free_type(t);
	  unreferenced++;
	  /* The hash table may have been modified! */
	  goto loop;
	}
      }
    }
  }
  for (e = 0; e < pike_type_hash_size; e++) {
    struct pike_type *t;
    for (t = pike_type_hash[e]; t; t = t->next) {
      struct marker *m = find_marker(t);
      if (!m) continue;
      if ((m->flags & GC_GOT_EXTRA_REF) == GC_GOT_EXTRA_REF) {
	fprintf(stderr, "There's still an extra ref to ");
	simple_describe_type(t);
	fprintf(stderr, "\n");
      }
    }
  }
  return unreferenced;
}

void real_gc_cycle_check_type(struct pike_type *t, int weak)
{
  GC_CYCLE_ENTER(t, PIKE_T_TYPE, weak) {
    switch(t->type) {
      case PIKE_T_SCOPE:
      case T_ASSIGN:
      case PIKE_T_NAME:
      case PIKE_T_ATTRIBUTE:
	if (t->cdr) gc_cycle_check_type(t->cdr, 0);
	break;
      case PIKE_T_FUNCTION:
      case T_MANY:
      case PIKE_T_RING:
      case PIKE_T_TUPLE:
      case PIKE_T_MAPPING:
      case T_OR:
      case T_AND:
	if (t->cdr) gc_cycle_check_type(t->cdr, 0);
      /* FALL_THOUGH */
      case PIKE_T_ARRAY:
      case PIKE_T_MULTISET:
      case T_NOT:
      case PIKE_T_TYPE:
      case PIKE_T_PROGRAM:
	if (t->car) gc_cycle_check_type(t->car, 0);
	break;
    }
  } GC_CYCLE_LEAVE;
}

void gc_cycle_check_all_types(void)
{
#if 0
  size_t e;
  if(!pike_type_hash) return;
  for(e=0;e<pike_type_hash_size;e++)
  {
    struct pike_type *t;
    for(t=pike_type_hash[e]; t; t=t->next) {
      real_gc_cycle_check_type(t, 0);
      gc_cycle_run_queue();
    }
  }
#endif
}

#endif /* PIKE_DEBUG */
#endif /* 0 */
