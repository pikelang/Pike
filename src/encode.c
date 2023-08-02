/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "stralloc.h"
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "mapping.h"
#include "array.h"
#include "multiset.h"
#include "lex.h"
#include "buffer.h"
#include "pike_error.h"
#include "operators.h"
#include "builtin_functions.h"
#include "module_support.h"
#include "fsort.h"
#include "threads.h"
#include "version.h"
#include "bignum.h"
#include "pikecode.h"
#include "pike_types.h"
#include "opcodes.h"
#include "peep.h"
#include "pike_compiler.h"
#include "bitvector.h"
#include "pike_float.h"
#include "sprintf.h"
#include "modules/Gmp/my_gmp.h"

/* #define ENCODE_DEBUG */

/* Use the old encoding method for programs. */
/* #define OLD_PIKE_ENCODE_PROGRAM */

#ifdef ENCODE_DEBUG
/* Pass a nonzero integer as the third arg to encode_value,
 * encode_value_canonic and decode_value to activate this debug. It
 * both enables debug messages and also lessens the pickyness to
 * sort-of be able to decode programs with the wrong codec. */
#define EDB(N,X) do { debug_malloc_touch(data); if (data->debug>=N) {X;} } while (0)
#ifndef PIKE_DEBUG
#error ENCODE_DEBUG requires PIKE_DEBUG
#endif
#else
#define EDB(N,X) do { debug_malloc_touch(data); } while (0)
#endif

/* Use this macro to guard sections of code that should only
 * be run when a trace buffer is active. This typically means
 * code that uses the ENCODE_WERR(), ENCODE_WERR_COMMENT(),
 * DECODE_WERR() or DECODE_WERR_COMMENT() macros below.
 */
#define ETRACE(X)	do {                    \
    if (data->debug_buf) {                      \
      X;                                        \
    }                                           \
  } while(0)

#define ENCODE_WERR_COMMENT(COMMENT, ...) do {				\
    string_builder_sprintf_disassembly(data->debug_buf,			\
				       data->debug_pos,			\
				       (((char *)data->buf.end) -	\
					data->buf.length) +		\
				       data->debug_pos,			\
				       data->buf.dst,			\
                                       COMMENT, __VA_ARGS__);           \
    data->debug_pos = data->buf.length -				\
      (((char *)data->buf.end) - ((char *)data->buf.dst));		\
  } while(0)
#define ENCODE_WERR(...) ENCODE_WERR_COMMENT(NULL, __VA_ARGS__)
#define ENCODE_FLUSH() do {						\
    size_t end_pos = data->buf.length -					\
      (((char *)data->buf.end) - ((char *)data->buf.dst));		\
    if (data->debug_pos < end_pos) {					\
      string_builder_append_disassembly_data(data->debug_buf,		\
					     data->debug_pos,		\
					     (((char *)data->buf.end) - \
					      data->buf.length) +	\
					     data->debug_pos,		\
					     data->buf.dst,		\
					     NULL);			\
      data->debug_pos = end_pos;					\
    }									\
  } while(0)

#define DECODE_WERR_COMMENT(COMMENT, ...) do {				\
    string_builder_sprintf_disassembly(data->debug_buf,			\
				       data->debug_ptr,			\
				       data->data + data->debug_ptr,	\
				       data->data + data->ptr,		\
                                       COMMENT, __VA_ARGS__);           \
    data->debug_ptr = data->ptr;					\
  } while(0)
#define DECODE_WERR(...) DECODE_WERR_COMMENT(NULL, __VA_ARGS__)
#define DECODE_FLUSH() do {						\
    if (data->debug_ptr < data->ptr) {					\
      string_builder_append_disassembly_data(data->debug_buf,		\
					     data->debug_ptr,		\
					     data->data +		\
					     data->debug_ptr,		\
					     data->data + data->ptr,	\
					     NULL);			\
      data->debug_ptr = data->ptr;					\
    }									\
  } while(0)

#ifdef _AIX
#include <net/nh.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef PIKE_DEBUG
#define encode_value2 encode_value2_
#define decode_value2 decode_value2_
#endif


/* Tags used by encode value.
 *
 * Currently they differ from the old PIKE_T variants by
 *   TAG_FLOAT == OLD_PIKE_T_TYPE == 7
 * and
 *   TAG_TYPE == OLD_PIKE_T_FLOAT == 9
 *
 * The old PIKE_T variants in turn differ from the current for values
 * less than 16 (aka MAX_TYPE) by bit 3 (mask 0x0008 (aka MIN_REF_TYPE))
 * being inverted.
 *
 * These are NOT to be renumbered unless the file-format version is changed!
 */
/* Current encoding: \266ke0
 *
 * +---+-+-+-------+
 * |s z|s|n| t a g |
 * +---+-+-+-------+
 *  	sz	size/small int
 *  	s	small int indicator
 *  	n	negative (or rather inverted)
 *  	tag	TAG_type (4 bits)
 */
#define TAG_ARRAY 0
#define TAG_MAPPING 1
#define TAG_MULTISET 2
#define TAG_OBJECT 3
#define TAG_FUNCTION 4
#define TAG_PROGRAM 5
#define TAG_STRING 6
#define TAG_FLOAT 7
#define TAG_INT 8
#define TAG_TYPE 9           /* Not supported yet */

#define TAG_DELAYED 14		/* Note: Coincides with T_ZERO. */
#define TAG_AGAIN 15
#define TAG_MASK 15
#define TAG_NEG 16
#define TAG_SMALL 32
#define SIZE_SHIFT 6
#define MAX_SMALL (1<<(8-SIZE_SHIFT))
#define COUNTER_START (-MAX_SMALL)

/* Entries used to encode the identifier_references table. */
#define ID_ENTRY_TYPE_CONSTANT	-4
#define ID_ENTRY_EFUN_CONSTANT	-3
#define ID_ENTRY_RAW		-2
#define ID_ENTRY_EOT		-1
#define ID_ENTRY_VARIABLE	0
#define ID_ENTRY_FUNCTION	1
#define ID_ENTRY_CONSTANT	2
#define ID_ENTRY_INHERIT	3
#define ID_ENTRY_ALIAS		4

static const char * const DECODING_NEEDS_CODEC_ERROR = "Attempt to decode value requiring codec.\n";

static struct object *lookup_codec (struct pike_string *codec_name)
{
  struct object *m = get_master();
  if (!m) {
    /* Use a dummy if there's no master around yet. This will cause an
     * error in apply later, so we don't need to bother. */
    return clone_object (null_program, 0);
  }
  else {
    ref_push_object (m);
    ref_push_string (codec_name);
    o_index();
    if (UNSAFE_IS_ZERO (Pike_sp - 1)) {
      add_ref (m);
      return m;
    }
    else {
      apply_svalue (Pike_sp - 1, 0);
      if (TYPEOF(Pike_sp[-1]) != T_OBJECT)
        Pike_error ("master()->%pS() did not return an object. Got: %pO\n",
                    codec_name, Pike_sp - 1);
      m = (--Pike_sp)->u.object;
      pop_stack();
      return m;
    }
  }
}

struct encode_data
{
  int canonic;
  struct object *codec;
  struct svalue counter;
  struct mapping *encoded;
  /* The encoded mapping maps encoded things to their entry IDs. A
   * value less than COUNTER_START means that it's a forward reference
   * to a thing not yet encoded. */
  struct array *delayed;
  struct byte_buffer buf;
  struct string_builder *debug_buf;
  size_t debug_pos;
#ifdef ENCODE_DEBUG
  int debug, depth;
#endif
};

static struct object *encoder_codec (struct encode_data *data)
{
  struct pike_string *encoder_str;
  if (data->codec) return data->codec;
  MAKE_CONST_STRING (encoder_str, "Encoder");
  return data->codec = lookup_codec (encoder_str);
}

/* Convert to/from forward reference ID. */
#define CONVERT_ENTRY_ID(ID) (-((ID) - COUNTER_START) - (-COUNTER_START + 1))

static void encode_value2(struct svalue *val, struct encode_data *data, int force_encode);

#ifdef ENCODE_DEBUG
static void debug_dump_mem(size_t offset, const unsigned char *bytes,
			   size_t len)
{
  size_t i;
  for (i = 0; i < len; i += 8) {
    size_t j;
    fprintf(stderr, "  %08zx: ", offset + i);
    for (j = 0; (i+j < len) && (j < 8); j++) {
      fprintf(stderr, "%02x", bytes[i+j]);
    }
    fprintf(stderr, "   ");
    for (j = 0; (i+j < len) && (j < 8); j++) {
      int c = bytes[i+j];
      if (((c & 0177) < ' ') || (c == 0177)) {
	/* Control or DEL */
	c = '.';
      }
      if (c < 0177) {
	/* ASCII */
	fprintf(stderr, "%c", c);
      } else {
	/* Latin-1 ==> Encode to UTF-8. */
	fprintf(stderr, "%c%c", 0xc0 | (c>>6), 0x80 | (c & 0x3f));
      }
    }
    fprintf(stderr, "\n");
  }
}
static void low_addstr(struct byte_buffer *buf, const char *bytes,
		       size_t len, int edb)
{
  if (edb >= 6) {
    debug_dump_mem(buf->length - (((char *)buf->end) - ((char *)buf->dst)),
		   (const unsigned char *)bytes, len);
  }
  buffer_memcpy(buf, bytes, len);
}
static void low_addchar(struct byte_buffer *buf, int c, int edb)
{
  char byte = c;
  low_addstr(buf, &byte, 1, edb);
}
#define addstr(s, l) low_addstr(&(data->buf), (s), (l), data->debug)
#define addchar(t)   low_addchar(&(data->buf), (t), data->debug)
#define addchar_unsafe(t) addchar(t)
#else
#define addstr(s, l) buffer_memcpy(&(data->buf), (s), (l))
#define addchar(t)   buffer_add_char(&(data->buf), (char)(t))
#define addchar_unsafe(t)       buffer_add_char_unsafe(&(data->buf), t)
#endif

/* Code a pike string */

#if PIKE_BYTEORDER == 4321
#define ENCODE_DATA(S) \
   addstr( (S)->str, (S)->len << (S)->size_shift );
#else
#define ENCODE_DATA(S) do {                                             \
    buffer_ensure_space(&(data->buf), (S)->len << (S)->size_shift);     \
    if ((S)->size_shift == 1) {                                         \
        const p_wchar1 *src = STR1(S);                                  \
        ptrdiff_t len = (S)->len, q;                                    \
        for(q=0;q<len;q++) {                                            \
           buffer_add_be16_unsafe(&(data->buf), src[q]);                \
        }                                                               \
    } else { /* shift 2 */                                              \
        const p_wchar2 *src = STR2(S);                                  \
        ptrdiff_t len = (S)->len, q;                                    \
        for(q=0;q<len;q++) {                                            \
           buffer_add_be32_unsafe(&(data->buf), src[q]);                \
        }                                                               \
    }                                                                   \
} while(0)
#endif

#define adddata(S) do {                                 \
  const struct pike_string *__str = (S);                \
  if(__str->size_shift)                                 \
  {                                                     \
    int q;                                              \
    code_entry(TAG_STRING,-1, data);                    \
    code_entry(__str->size_shift, __str->len, data);    \
    ETRACE({						\
	ENCODE_WERR(".string  %td<<%d",			\
		    __str->len, __str->size_shift);	\
      });						\
    ENCODE_DATA(__str);                                 \
  }else{                                                \
    code_entry(TAG_STRING, __str->len, data);           \
    ETRACE({						\
	ENCODE_WERR(".string  %td", __str->len);	\
      });						\
    addstr((char *)(__str->str),__str->len);            \
  }                                                     \
  ETRACE({						\
      ENCODE_FLUSH();					\
    });							\
}while(0)

#ifdef ENCODE_DEBUG
/* NOTE: Fix when type encodings change. */
static int tag_to_type(int tag)
{
  if (tag == TAG_FLOAT) return T_FLOAT;
  if (tag == TAG_TYPE) return T_TYPE;
  if (tag <= MAX_TYPE) return tag ^ MIN_REF_TYPE;
  return tag;
}
#endif

/* Let's cram those bits... */
static void code_entry(int tag, INT64 num, struct encode_data *data)
{
  int t;
  /* this needs 5 bytes maximal */
  buffer_ensure_space(&data->buf, 5);
  EDB(5,
    fprintf(stderr,"%*sencode: code_entry(tag=%d (%s), num=%ld)\n",
	    data->depth, "", tag,
	    get_name_of_type(tag_to_type(tag)),
	    (long)num) );
  if(num<0)
  {
    tag |= TAG_NEG;
    num = ~num;
  }

  if(num < MAX_SMALL)
  {
    tag |= TAG_SMALL | (num << SIZE_SHIFT);
    addchar_unsafe((char)tag);
    return;
  }else{
    num -= MAX_SMALL;
  }

  /* NB: There's only space for two bits of length info. */
  for(t = 0; (size_t)t < 3; t++)
  {
    if(num >= (((INT64)256) << (t<<3)))
      num -= (((INT64)256) << (t<<3));
    else
      break;
  }

  tag |= t << SIZE_SHIFT;
  addchar_unsafe((char)tag);

  switch(t)
  {
  case 3: addchar_unsafe((char)((num >> 24)&0xff)); /* FALLTHRU */
  case 2: addchar_unsafe((char)((num >> 16)&0xff)); /* FALLTHRU */
  case 1: addchar_unsafe((char)((num >> 8)&0xff));  /* FALLTHRU */
  case 0: addchar_unsafe((char)(num&0xff));
  }
}

static void encode_number(ptrdiff_t num, struct encode_data *data,
                          const char *comment)
{
  EDB(5, fprintf(stderr, "%*sencode_number(%td)\n",
		 data->depth, "", num));
  code_entry(num & 15, num >> 4, data);
  if (comment) {
    ETRACE({
        ENCODE_WERR_COMMENT(comment,
                            ".number  %"PRINTPTRDIFFT"d", num);
      });
  }
}

/* NOTE: Take care to encode it exactly as the corresponing
 *       type string would have been encoded (cf T_FUNCTION, T_MANY,
 *       T_STRING, PIKE_T_NSTRING).
 */
static void encode_type(struct pike_type *t, struct encode_data *data)
{
 one_more_type:
  if (!t) {
    addchar(PIKE_T_UNKNOWN);
    ETRACE({
	ENCODE_WERR(".type    __unknown__");
      });
    return;
  }
  if (t->type == T_MANY) {
    addchar(T_FUNCTION ^ MIN_REF_TYPE);
    ETRACE({
	ENCODE_WERR(".type    function");
      });
    addchar(T_MANY);
    ETRACE({
	ENCODE_WERR(".type    many");
      });
  } else if (t->type == T_STRING) {
    if (t->car != int_pos_type_string) {
      /* Limited length string. */
      addchar(PIKE_T_LSTRING);
      ETRACE({
	  ENCODE_WERR(".type    lstring");
	});
      encode_type(t->car, data);
      encode_type(t->cdr, data);
    } else if (t->cdr == int_type_string) {
      addchar(T_STRING ^ MIN_REF_TYPE);
      ETRACE({
	  ENCODE_WERR(".type    string");
	});
    } else {
      /* Narrow string */
      addchar(PIKE_T_NSTRING);
      ETRACE({
	  ENCODE_WERR(".type    nstring");
	});
      encode_type(t->cdr, data);
    }
    return;
  } else if (t->type == T_ARRAY) {
    if (t->car != int_pos_type_string) {
      /* Limited length array. */
      addchar(PIKE_T_LARRAY);
      ETRACE({
	  ENCODE_WERR(".type    larray");
	});
      encode_type(t->car, data);
    } else {
      addchar(T_ARRAY ^ MIN_REF_TYPE);
      ETRACE({
	  ENCODE_WERR(".type    %s",
		      get_name_of_type(t->type));
	});
    }
    t = t->cdr;
    goto one_more_type;
  } else if (t->type <= MAX_TYPE) {
    if (t->type == T_FUNCTION) {
      struct pike_type *tmp = t;
      while (tmp->type == T_FUNCTION) tmp = tmp->cdr;
      if (tmp->type != T_MANY) {
	/* Function type needing tFuncArg. */
	while (t->type == T_FUNCTION) {
	  addchar(t->type);	/* Intentional !!!!! */
          ETRACE({
	      ENCODE_WERR(".type    function_arg");
	    });
	  encode_type(t->car, data);
	  t = t->cdr;
	}
	goto one_more_type;
      }
    }
    addchar(t->type ^ MIN_REF_TYPE);
    ETRACE({
	ENCODE_WERR(".type    %s",
		    get_name_of_type(t->type));
      });
  } else {
    addchar(t->type & PIKE_T_MASK);
    ETRACE({
	ENCODE_WERR(".type    %s",
		    get_name_of_type(t->type & 0xff));
      });
    if ((t->type & PIKE_T_MASK) == PIKE_T_OPERATOR) {
      addchar(t->type>>8);
      ETRACE({
	  ENCODE_WERR(".htype    %s",
		      get_name_of_type(t->type));
	});
    }
  }
  switch(t->type & PIKE_T_MASK) {
    default:
      Pike_fatal("error in type tree: 0x%04x.\n", t->type);
      UNREACHABLE(break);

    case PIKE_T_OPERATOR:
      if (t->type & 0x8000) {
	encode_type(t->cdr, data);
      } else if (t->type == PIKE_T_FIND_LFUN) {
	struct svalue sval;
	SET_SVAL(sval, PIKE_T_STRING, 0, string, (void *)t->cdr);
	encode_value2(&sval, data, 0);
      }
      t = t->car;
      goto one_more_type;

    case PIKE_T_ATTRIBUTE:	/* FIXME: Strip this in compat mode. */
    case PIKE_T_NAME:
      {
	struct svalue sval;
	SET_SVAL(sval, PIKE_T_STRING, 0, string, (void *)t->car);
	encode_value2(&sval, data, 0);
      }
      t=t->cdr;
      goto one_more_type;

    case T_ASSIGN:
      {
	ptrdiff_t marker = CAR_TO_INT(t);
	if ((marker < 0) || (marker > 9)) {
	  Pike_fatal("Bad assign marker: %ld\n",
		     (long)marker);
	}
	addchar('0' + marker);
        ETRACE({
	    ENCODE_WERR(".type    marker, %td", marker);
	  });
	t = t->cdr;
      }
      goto one_more_type;

    case T_FUNCTION:
      while(t->type == T_FUNCTION) {
	encode_type(t->car, data);
	t = t->cdr;
      }
      addchar(T_MANY);
      /* FALLTHRU */
    case T_MANY:
      encode_type(t->car, data);
      t = t->cdr;
      goto one_more_type;

    case T_SCOPE:
      {
	ptrdiff_t val = CAR_TO_INT(t);
	addchar(val & 0xff);
        ETRACE({
	    ENCODE_WERR(".type    scope, %td", val);
	  });
      }
      t = t->cdr;
      goto one_more_type;

    case T_MAPPING:
    case T_OR:
    case T_AND:
      encode_type(t->car, data);
      t = t->cdr;
      goto one_more_type;

    case T_TYPE:
    case T_PROGRAM:
    case T_MULTISET:
    case T_NOT:
      t = t->car;
      goto one_more_type;

    case T_INT:
      {
	ptrdiff_t val;

        buffer_add_be32(&data->buf, CAR_TO_INT(t));
        ETRACE({
	    ENCODE_WERR(".data    %d", (int)CAR_TO_INT(t));
	  });
        buffer_add_be32(&data->buf, CDR_TO_INT(t));
        ETRACE({
	    ENCODE_WERR(".data    %d", (int)CDR_TO_INT(t));
	  });
      }
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
    case T_MIXED:
    case T_ZERO:
    case T_VOID:
    case PIKE_T_UNKNOWN:
      break;

    case T_OBJECT:
    {
      addchar(CAR_TO_INT(t));
      ETRACE({
	  ENCODE_WERR(".type    %d", (int)CAR_TO_INT(t));
	});

      if(t->cdr)
      {
	ptrdiff_t id = CDR_TO_INT(t);
	if( id >= PROG_DYNAMIC_ID_START )
	{
	  struct program *p=id_to_program(id);
	  if(p)
	  {
	    ref_push_program(p);
	  }else{
	    push_int(0);
	  }
	} else
	  push_int( id );
      }else{
	push_int(0);
      }
      /* If it's a program that should be encoded recursively then we
       * must delay it. Consider:
       *
       *     class A {B b;}
       *     class B {inherit A;}
       *
       * We can't dump B when the type is encountered inside A, since
       * upon decode B won't have a complete A to inherit then.
       */
      encode_value2(Pike_sp-1, data, 0);
      pop_stack();
      break;
    }
  }
}

/* force_encode == 0: Maybe dump the thing later, and only a forward
 * reference here (applies to programs only).
 *
 * force_encode == 1: Dump the thing now.
 *
 * force_encode == 2: A forward reference has been encoded to this
 * thing. Now it's time to dump it. */

static void encode_value2(struct svalue *val, struct encode_data *data, int force_encode)

#ifdef PIKE_DEBUG
#undef encode_value2
#define encode_value2(X,Y,Z) do {			\
    struct svalue *_=Pike_sp;				\
    struct svalue *X_ = (X);				\
    encode_value2_(X_,Y,Z);				\
    if(Pike_sp != _) {					\
      fprintf(stderr, "Stack error when encoding:\n");	\
      print_svalue(stderr, X_);				\
      fprintf(stderr, "\n");				\
      if (TYPEOF(*X_) == T_PROGRAM) {			\
        dump_program_tables(X_->u.program, 2);		\
      }							\
      Pike_fatal("encode_value2() failed %p != %p!\n",	\
		 Pike_sp, _);				\
    }							\
  } while(0)
#endif

{
  static struct svalue dested = SVALUE_INIT (T_INT, NUMBER_DESTRUCTED, 0);
  INT32 i;
  struct svalue *tmp;
  struct svalue entry_id;

#ifdef ENCODE_DEBUG
  data->depth += 2;
#endif

  ETRACE({
      ENCODE_FLUSH();
    });
  ETRACE({
      ENCODE_WERR("# Encoding a %s.",
		  get_name_of_type(TYPEOF(*val)));
    });

  if((TYPEOF(*val) == T_OBJECT ||
      (TYPEOF(*val) == T_FUNCTION && SUBTYPEOF(*val) != FUNCTION_BUILTIN)) &&
     !val->u.object->prog)
    val = &dested;

  if((tmp=low_mapping_lookup(data->encoded, val)))
  {
    entry_id = *tmp;		/* It's always a small integer. */
    if (entry_id.u.integer < COUNTER_START)
      entry_id.u.integer = CONVERT_ENTRY_ID (entry_id.u.integer);
    if (force_encode && tmp->u.integer < COUNTER_START) {
      EDB(1,
	  fprintf(stderr, "%*sEncoding delayed thing to <%ld>: ",
		  data->depth, "", (long)entry_id.u.integer);
	  if(data->debug == 1)
	  {
	    fprintf(stderr,"TAG%d", TYPEOF(*val));
	  }else{
	    print_svalue(stderr, val);
	  }
	  fputc('\n', stderr););
      code_entry (TAG_DELAYED, entry_id.u.integer, data);
      tmp->u.integer = entry_id.u.integer;
      ETRACE({
	  ENCODE_WERR(".tag     delayed, %ld", (long)entry_id.u.integer);
	});
    }
    else {
      EDB(1,fprintf(stderr, "%*sEncoding TAG_AGAIN from <%ld>\n",
		    data->depth, "", (long)entry_id.u.integer));
      code_entry(TAG_AGAIN, entry_id.u.integer, data);
      ETRACE({
	  ENCODE_WERR(".tag     again, %ld", (long)entry_id.u.integer);
	});
      goto encode_done;
    }
  }else {
#ifdef PIKE_DEBUG
    if (force_encode == 2)
      Pike_fatal ("Didn't find old entry for delay encoded thing.\n");
#endif
    if (TYPEOF(*val) != T_TYPE) {
      entry_id = data->counter;	/* It's always a small integer. */
      EDB(1,fprintf(stderr, "%*sEncoding to <%ld>: ",
		    data->depth, "", (long)entry_id.u.integer);
	  if(data->debug == 1)
	  {
	    fprintf(stderr,"TAG%d", TYPEOF(*val));
	  }else{
	    print_svalue(stderr, val);
	  }
	  fputc('\n', stderr););
      ETRACE({
	  ENCODE_WERR("# Encoding to tag #%ld", (long)entry_id.u.integer);
	});
      if( (TYPEOF(*val) < MIN_REF_TYPE) || (val->u.dummy->refs > 1) ||
          ((TYPEOF(*val) == T_OBJECT) &&
           (QUICK_FIND_LFUN(val->u.object->prog, LFUN___HASH) != -1))) {
          mapping_insert(data->encoded, val, &entry_id);
      }
      data->counter.u.integer++;
    }
  }

  switch(TYPEOF(*val))
  {
    case T_INT:
      /* NOTE: Doesn't encode NUMBER_UNDEFINED et al. */
      {
        INT_TYPE i=val->u.integer;
#if SIZEOF_INT_TYPE > 4
        if (i != (INT32)i)
        {
          MP_INT tmp;

          code_entry(TAG_OBJECT, 2, data);
          ETRACE({
	      ENCODE_WERR(".bignum  %*s# %ld", 20, "", i);
	    });

          /* Note: conversion to base 36 could be done directly here
	   * using / and % instead.  Doing it like this is actually
	   * only slightly slower, however, and saves on code size.
	   *
	   * Note that we also must adjust the data->encoded mapping,
	   * to not confuse the decoder. This is easiest to do by
	   * performing a recursive call, which means that we need
	   * a proper pike_string anyway.
	   */
          mpz_init_set_si( &tmp, i );
	  push_string(low_get_mpz_digits(&tmp, 36));
	  mpz_clear( &tmp );
	  encode_value2(Pike_sp-1, data, force_encode);
	  pop_stack();
        }
        else
#endif
	{
          code_entry(TAG_INT, i, data);

          ETRACE({
	      ENCODE_WERR(".integer %ld", i);
	    });
	}
      }
      break;

    case T_STRING:
      adddata(val->u.string);
      break;

    case T_TYPE:
      /* NOTE: Types are added to the encoded mapping AFTER they have
       *       been encoded, to simplify decoding.
       */
      if (data->canonic)
	Pike_error("Canonical encoding of the type type not supported.\n");

      code_entry(TAG_TYPE, 0, data);	/* Type encoding #0 */
      ETRACE({
	  ENCODE_WERR(".entry   type");
	});

      encode_type(val->u.type, data);
      EDB(2,fprintf(stderr, "%*sEncoded type to <%ld>: ",
		    data->depth, "", (long)data->counter.u.integer);
	  print_svalue(stderr, val);
	  fputc('\n', stderr););
      mapping_insert(data->encoded, val, &data->counter);
      data->counter.u.integer++;
      break;

    case T_FLOAT:
    {
      double d = val->u.float_number;

#define Pike_FP_SNAN -4 /* Signal Not A Number */
#define Pike_FP_QNAN -3 /* Quiet Not A Number */
#define Pike_FP_NINF -2 /* Negative infinity */
#define Pike_FP_PINF -1 /* Positive infinity */
#define Pike_FP_ZERO  0 /* Backwards compatible zero */
#define Pike_FP_NZERO 1 /* Negative Zero */
#define Pike_FP_PZERO 0 /* Positive zero */
#define Pike_FP_UNKNOWN -4711 /* Positive zero */


#ifdef HAVE_FPCLASS
      switch(fpclass(d)) {
	case FP_SNAN:
	  code_entry(TAG_FLOAT,0,data);
	  code_entry(TAG_FLOAT,Pike_FP_SNAN,data);
          ETRACE({
	      ENCODE_WERR(".float   snan");
	    });
	  break;

	case FP_QNAN:
	  code_entry(TAG_FLOAT,0,data);
	  code_entry(TAG_FLOAT,Pike_FP_QNAN,data);
          ETRACE({
	      ENCODE_WERR(".float   qnan");
	    });
	  break;

	case FP_NINF:
	  code_entry(TAG_FLOAT,0,data);
	  code_entry(TAG_FLOAT,Pike_FP_NINF,data);
          ETRACE({
	      ENCODE_WERR(".float   -inf");
	    });
	  break;

	case FP_PINF:
	  code_entry(TAG_FLOAT,0,data);
	  code_entry(TAG_FLOAT,Pike_FP_PINF,data);
          ETRACE({
	      ENCODE_WERR(".float   inf");
	    });
	  break;

	case FP_NZERO:
	  code_entry(TAG_FLOAT,0,data);
	  code_entry(TAG_FLOAT,Pike_FP_NZERO,data);
          ETRACE({
	      ENCODE_WERR(".float   -0.0");
	    });
	  break;

	case FP_PZERO:
	  code_entry(TAG_FLOAT,0,data);
	  code_entry(TAG_FLOAT,Pike_FP_ZERO,data); /* normal zero */
          ETRACE({
	      ENCODE_WERR(".float   0.0");
	    });
	  break;

	  /* Ugly, but switch gobbles breaks -Hubbe */
	default:
	  goto encode_normal_float;
      }
      break;
    encode_normal_float:

#else
      {
	int pike_ftype=Pike_FP_UNKNOWN;
        if(PIKE_ISINF(d))
          pike_ftype=Pike_FP_PINF;
        else if(PIKE_ISNAN(d))
          pike_ftype=Pike_FP_SNAN;
#ifdef HAVE_ISZERO
        else if(iszero(d))
          pike_ftype=Pike_FP_PZERO;
#endif

	if(
#ifdef HAVE_SIGNBIT
	  signbit(d)
#else
	  d<0.0
#endif
	  ) {
	  switch(pike_ftype)
	  {
	    case Pike_FP_PINF:
	      pike_ftype=Pike_FP_NINF;
	      break;

#ifdef HAVE_ISZERO
	    case Pike_FP_PZERO:
	      pike_ftype=Pike_FP_NZERO;
	      break;
#endif
	  }
	}

	if(pike_ftype != Pike_FP_UNKNOWN)
	{
	  code_entry(TAG_FLOAT,0,data);
	  code_entry(TAG_FLOAT,pike_ftype,data);
          ETRACE({
	      ENCODE_WERR(".float   %d", pike_ftype);
	    });
	  break;
	}
      }
#endif

      if(d == 0.0)
      {
	code_entry(TAG_FLOAT,0,data);
	code_entry(TAG_FLOAT,0,data);
        ETRACE({
	    ENCODE_WERR(".float   0.0");
	  });
      }else{
	INT64 x;
	int y;
	double tmp;

	tmp = frexp(d, &y);
        x = (INT64)((((INT64)1)<<(sizeof(INT64)*8 - 2))*tmp);
	y -= sizeof(INT64)*8 - 2;

	EDB(2,fprintf(stderr,
		    "Encoding float... tmp: %10g, x: 0x%016llx, y: %d\n",
		      tmp, (long long)x, y));

        x >>= 32;
        y += 32;

        EDB(2,fprintf(stderr,
		      "Reducing float... x: 0x%08llx, y: %d\n",
		      (long long)x, y));
	code_entry(TAG_FLOAT,x,data);
	code_entry(TAG_FLOAT,y,data);
        ETRACE({
	    ENCODE_WERR(".float   %ld, %d", (long)x, y);
	  });
      }
      break;
    }

    case T_ARRAY:
      code_entry(TAG_ARRAY, val->u.array->size, data);
      ETRACE({
	  ENCODE_WERR(".entry   array, %d", val->u.array->size);
	});
      for(i=0; i<val->u.array->size; i++)
	encode_value2(ITEM(val->u.array)+i, data, 0);
      break;

    case T_MAPPING:
      check_stack(2);
      ref_push_mapping(val->u.mapping);
      f_indices(1);

      ref_push_mapping(val->u.mapping);
      f_values(1);

      if (data->canonic) {
	INT32 *order;
	if (val->u.mapping->data->ind_types & ~(BIT_BASIC & ~BIT_TYPE)) {
	  mapping_fix_type_field(val->u.mapping);
	  if (val->u.mapping->data->ind_types & ~(BIT_BASIC & ~BIT_TYPE))
	    /* This doesn't let bignums through. That's necessary as
	     * long as they aren't handled deterministically by the
	     * sort function. */
	    /* They should be handled deterministically now - Hubbe */
	    Pike_error("Canonical encoding requires basic types in indices.\n");
	}
	order = get_switch_order(Pike_sp[-2].u.array);
	order_array(Pike_sp[-2].u.array, order);
	order_array(Pike_sp[-1].u.array, order);
	free(order);
      }

      code_entry(TAG_MAPPING, Pike_sp[-2].u.array->size,data);
      ETRACE({
	  ENCODE_WERR(".entry   mapping, %d", Pike_sp[-2].u.array->size);
	});
      for(i=0; i<Pike_sp[-2].u.array->size; i++)
      {
	encode_value2(ITEM(Pike_sp[-2].u.array)+i, data, 0); /* indices */
	encode_value2(ITEM(Pike_sp[-1].u.array)+i, data, 0); /* values */
      }
      pop_n_elems(2);
      /* FIXME: What about flags? */
      break;

    case T_MULTISET: {
      struct multiset *l = val->u.multiset;

      if (TYPEOF(*multiset_get_cmp_less(l)) != T_INT)
	Pike_error ("FIXME: Encoding of multisets with "
		    "custom sort function not yet implemented.\n");
      else {
	/* Encode valueless multisets without compare functions in a
	 * compatible way. */
	code_entry(TAG_MULTISET, multiset_sizeof (l), data);
        ETRACE({
	    ENCODE_WERR(".entry   multiset, %d", multiset_sizeof(l));
	  });
	if (data->canonic) {
	  INT32 *order;
	  if (multiset_ind_types(l) & ~(BIT_BASIC & ~BIT_TYPE)) {
	    multiset_fix_type_field(l);
	    if (multiset_ind_types(l) & ~(BIT_BASIC & ~BIT_TYPE))
	      /* This doesn't let bignums through. That's necessary as
	       * long as they aren't handled deterministically by the
	       * sort function. */
	      Pike_error("Canonical encoding requires basic types in indices.\n");
	  }
	  check_stack(1);
	  push_array(multiset_indices(l));
	  order = get_switch_order(Pike_sp[-1].u.array);
	  order_array(Pike_sp[-1].u.array, order);
	  free(order);
	  for (i = 0; i < Pike_sp[-1].u.array->size; i++)
	    encode_value2(ITEM(Pike_sp[-1].u.array)+i, data, 0);
	  pop_stack();
	}
	else {
	  struct svalue ind;
	  union msnode *node = low_multiset_first (l->msd);
	  for (; node; node = low_multiset_next (node))
	    encode_value2 (low_use_multiset_index (node, ind), data, 0);
	}
      }
      break;
    }

    case T_OBJECT:
      check_stack(1);

      /* This could be implemented a lot more generic,
       * but that will have to wait until next time. /Hubbe
       */
      if(is_bignum_object(val->u.object))
      {
        MP_INT *i = (MP_INT*)val->u.object->storage;

	code_entry(TAG_OBJECT, 2, data);
        ETRACE({
	    ENCODE_WERR(".bignum");
	});
        push_string(low_get_mpz_digits(i, 36));
        encode_value2(Pike_sp-1, data, force_encode);
        pop_stack();
	break;
      }

      if (data->canonic)
	Pike_error("Canonical encoding of objects not supported.\n");
      push_svalue(val);
      apply(encoder_codec (data), "nameof", 1);
      EDB(5, fprintf(stderr, "%*s->nameof: ", data->depth, "");
	  print_svalue(stderr, Pike_sp-1);
	  fputc('\n', stderr););
      switch(TYPEOF(Pike_sp[-1]))
      {
	case T_INT:
	  if(SUBTYPEOF(Pike_sp[-1]) == NUMBER_UNDEFINED)
	  {
	    int to_change = buffer_content_length(&data->buf);
	    struct svalue tmp = entry_id;

	    EDB(5,fprintf(stderr, "%*s(UNDEFINED)\n", data->depth, ""));

	    if (SUBTYPEOF(*val)) {
	      /* Subtyped object.
	       *
	       * Encode the subtype, and then try encoding the plain object.
	       */
	      code_entry(TAG_OBJECT, 4, data);
              ETRACE({
		  ENCODE_WERR(".entry   object, 4");
		});
              encode_number(SUBTYPEOF(*val), data, "inherit");
	      pop_stack();
	      ref_push_object(val->u.object);
	      break;
	    }

	    /* We have to remove ourselves from the cache */
	    map_delete(data->encoded, val);

	    pop_stack();
	    push_svalue(val);
	    f_object_program(1);

	    /* Code the program */
	    code_entry(TAG_OBJECT, 3,data);
            ETRACE({
		ENCODE_WERR(".entry   object, 3");
	      });
	    encode_value2(Pike_sp-1, data, 1);
	    pop_stack();

	    push_svalue(val);

	    /* If we do not exist in cache, use backwards-
	     * compatible method, otherwise use newfangled
	     * style=3.  -Hubbe
	     */
	    if(!low_mapping_lookup(data->encoded, val))
	    {
	      int fun;
	      EDB(1,fprintf(stderr, "%*sZapping 3 -> 1 in TAG_OBJECT\n",
			    data->depth, ""));

	      /* This causes the code_entry above to
	       * become: code_entry(TAG_OBJECT, 1, data);
	       * -Hubbe
	       */
	      ((char*)buffer_ptr(&data->buf))[to_change] = 99;

	      fun = find_identifier("encode_object",
				    encoder_codec (data)->prog);
	      if (fun < 0)
		Pike_error("Cannot encode objects without an "
			   "\"encode_object\" function in the codec.\n");
	      apply_low(data->codec,fun,1);

	      /* Put value back in cache for future reference -Hubbe */
	      mapping_insert(data->encoded, val, &tmp);
	    }
	    break;
	  }
          /* FALLTHRU */

	default:
	  code_entry(TAG_OBJECT, 0,data);
          ETRACE({
	      ENCODE_WERR(".entry   object");
	    });
	  break;
      }
      encode_value2(Pike_sp-1, data, 0);
      pop_stack();
      break;

    case T_FUNCTION:
      /* FIXME: Ought to have special treatment of trampolines. */
      if (data->canonic)
	Pike_error("Canonical encoding of functions not supported.\n");
      check_stack(1);
      push_svalue(val);
      apply(encoder_codec (data),"nameof", 1);
      if(TYPEOF(Pike_sp[-1]) == T_INT &&
	 SUBTYPEOF(Pike_sp[-1]) == NUMBER_UNDEFINED)
      {
	if(SUBTYPEOF(*val) != FUNCTION_BUILTIN)
	{
	  if(really_low_find_shared_string_identifier(
	       ID_FROM_INT(val->u.object->prog, SUBTYPEOF(*val))->name,
	       val->u.object->prog,
	       SEE_PROTECTED|SEE_PRIVATE) == SUBTYPEOF(*val))
	  {
	    /* We have to remove ourself from the cache for now */
	    struct svalue tmp = entry_id;
	    map_delete(data->encoded, val);

	    code_entry(TAG_FUNCTION, 1, data);
            ETRACE({
		ENCODE_WERR(".entry   function, 1");
	      });
	    ref_push_object(val->u.object);
	    encode_value2(Pike_sp-1, data, 0);
	    ref_push_string(ID_FROM_INT(val->u.object->prog,
					SUBTYPEOF(*val))->name);
	    encode_value2(Pike_sp-1, data, 0);
	    pop_n_elems(3);

	    /* Put value back in cache */
	    mapping_insert(data->encoded, val, &tmp);
	    goto encode_done;
	  }
	  else {
	    /* FIXME: Encode the object, the inherit and the name. */
	    Pike_error("Cannot encode overloaded functions (yet).\n");
	  }
	}
	Pike_error("Cannot encode builtin functions.\n");
      }

      code_entry(TAG_FUNCTION, 0, data);
      ETRACE({
	  ENCODE_WERR(".entry   function");
	});
      encode_value2(Pike_sp-1, data, 0);
      pop_stack();
      break;


    case T_PROGRAM:
    {
      int d;
      if ((val->u.program->id < PROG_DYNAMIC_ID_START) &&
	  (val->u.program->id >= 0)) {
	code_entry(TAG_PROGRAM, 3, data);
        ETRACE({
	    ENCODE_WERR(".entry   program, 3");
	  });
	push_int(val->u.program->id);
	encode_value2(Pike_sp-1, data, 0);
	pop_stack();
	break;
      }
      if (data->canonic)
	Pike_error("Canonical encoding of programs not supported.\n");
      if (!(val->u.program->flags & PROGRAM_FIXED))
	Pike_error("Encoding of unfixated programs not supported.\n");
      check_stack(1);
      push_svalue(val);
      apply(encoder_codec (data),"nameof", 1);
      if(TYPEOF(Pike_sp[-1]) == TYPEOF(*val))
	Pike_error("Error in master()->nameof(), same type returned.\n");
      if(TYPEOF(Pike_sp[-1]) == T_INT &&
	 SUBTYPEOF(Pike_sp[-1]) == NUMBER_UNDEFINED)
      {
	struct program *p=val->u.program;
	debug_malloc_touch(p);
	pop_stack();
	if( (p->flags & PROGRAM_HAS_C_METHODS) || p->event_handler )
	{
	  int has_local_c_methods = 0;
	  for (d = 0; d < p->num_identifiers; d++) {
	    struct identifier *id = p->identifiers + d;
	    if (IDENTIFIER_IS_C_FUNCTION(id->identifier_flags) &&
		!low_is_variant_dispatcher(id)) {
	      has_local_c_methods = 1;
	      break;
	    }
	  }
	  if (has_local_c_methods) {
	    if(p->parent)
	    {
	      /* We have to remove ourselves from the cache for now */
	      struct svalue tmp = entry_id;
	      EDB(1, fprintf(stderr,
			     "%*sencode: encoding C program via parent.\n",
			     data->depth, ""));
	      map_delete(data->encoded, val);

	      code_entry(TAG_PROGRAM, 2, data);
              ETRACE({
		  ENCODE_WERR(".entry   program, 2");
		});
	      ref_push_program(p->parent);
	      encode_value2(Pike_sp-1, data, 0);

	      ref_push_program(p);
	      f_function_name(1);
	      encode_value2(Pike_sp-1, data, 0);

	      pop_n_elems(2);

	      /* Put value back in cache */
	      mapping_insert(data->encoded, val, &tmp);
	      goto encode_done;
	    }
	    if( p->event_handler )
	      Pike_error("Cannot encode programs with event handlers.\n");
	  } else {
	    EDB(1, fprintf(stderr,
			   "%*sencode: encoding program overloading a C program.\n",
			   data->depth, ""));
	  }
	}

	/* Portable encoding (4 and 5). */

	if (!force_encode) {
	  /* Encode later (5). */
	  EDB(1, fprintf(stderr, "%*sencode: delayed encoding of program\n",
			 data->depth, ""));
	  code_entry (TAG_PROGRAM, 5, data);
          ETRACE({
	      ENCODE_WERR(".entry   program, 5");
	    });
	  data->delayed = append_array (data->delayed, val);
	  tmp = low_mapping_lookup (data->encoded, val);
	  if (!tmp)
	    Pike_error("Internal error in delayed encoder of programs.\n");
	  tmp->u.integer = CONVERT_ENTRY_ID (tmp->u.integer);
	  goto encode_done;
	}

	EDB(1, fprintf(stderr, "%*sencode: encoding program in new style\n",
		       data->depth, ""));
	code_entry(TAG_PROGRAM, 4, data);
        ETRACE({
	    ENCODE_WERR(".entry   program, 4");
	  });

	/* Byte-order. */
        encode_number(PIKE_BYTEORDER, data, "byte-order");

	/* flags */
        encode_number(p->flags, data, "flags");

	/* version */
	push_compact_version();
        ETRACE({
	    ENCODE_WERR("# version");
	  });
	encode_value2(Pike_sp-1, data, 0);
	pop_stack();

	/* parent */
        ETRACE({
	    ENCODE_WERR("# parent");
	  });
	if (p->parent) {
	  ref_push_program(p->parent);
	} else {
	  push_int(0);
	}
	encode_value2(Pike_sp-1, data, 0);
	pop_stack();

	/* num_* */
#define FOO(NUMTYPE,TYPE,ARGTYPE,NAME)	do {				\
          encode_number( p->PIKE_CONCAT(num_,NAME), data, TOSTR(NAME)); \
	} while(0);

#include "program_areas.h"

	/* Byte-code method
	 */
        encode_number(PIKE_BYTECODE_PORTABLE, data, "byte-code");

	{
	  struct svalue str_sval;
	  SET_SVAL(str_sval, T_STRING, 0, string, NULL);
	  /* strings */
	  for(d=0;d<p->num_strings;d++) {
	    str_sval.u.string = p->strings[d];
	    encode_value2(&str_sval, data, 0);
	  }
	}

	EDB(5, dump_program_tables(p, data->depth));

	/* Encode the efun constants since they are needed by the optimizer. */
	{
	  struct svalue str_sval;
	  SET_SVAL(str_sval, T_STRING, 0, string, NULL);

          ETRACE({
	      ENCODE_WERR("# Constant table.");
	    });
	  /* constants */
	  for(d=0;d<p->num_constants;d++)
	  {
	    if ((TYPEOF(p->constants[d].sval) == T_FUNCTION) &&
		(SUBTYPEOF(p->constants[d].sval) == FUNCTION_BUILTIN)) {
              encode_number(ID_ENTRY_EFUN_CONSTANT, data, NULL);
              ETRACE({
		  ENCODE_WERR(".ident   efun_constant");
		});
	    } else if (TYPEOF(p->constants[d].sval) == T_TYPE) {
              encode_number(ID_ENTRY_TYPE_CONSTANT, data, NULL);
              ETRACE({
		  ENCODE_WERR(".ident   type_constant");
		});
	    } else {
	      continue;
	    }
            encode_number(d, data, "constant #");
	    /* value */
	    encode_value2(&p->constants[d].sval, data, 0);

	    /* name */
	      push_int(0);
	      encode_value2(Pike_sp-1, data, 0);
	      dmalloc_touch_svalue(Pike_sp-1);
	      Pike_sp--;
	  }
	}

	/* Dump the identifiers in a portable manner... */
	{
	  int inherit_num = 1;
	  struct svalue str_sval;
	  char *id_dumped = alloca(p->num_identifiers);
	  int d_min = 0;
	  memset(id_dumped,0,p->num_identifiers);
	  SET_SVAL(str_sval, T_STRING, 0, string, NULL);

	  EDB(2,
	      fprintf(stderr, "%*sencode: encoding references\n",
		      data->depth, ""));

#ifdef ENCODE_DEBUG
	  data->depth += 2;
#endif

          ETRACE({
	      ENCODE_WERR("# Identifier reference table.");
	    });
	  /* NOTE: d is incremented by hand inside the loop. */
	  for (d=0; d < p->num_identifier_references;)
	  {
	    int d_max = p->num_identifier_references;

	    /* Find insertion point of next inherit. */
	    if (inherit_num < p->num_inherits) {
	      d_max = p->inherits[inherit_num].identifier_ref_offset;
	    }

	    EDB (4, fprintf (stderr, "%*sencode: inherit_num: %d, d_max: %d\n",
			     data->depth, "", inherit_num, d_max););

	    /* Fix locally defined identifiers. */
	    for (; d < d_max; d++) {
	      struct reference *ref = p->identifier_references + d;
	      struct inherit *inh = INHERIT_FROM_PTR(p, ref);
	      struct identifier *id = ID_FROM_PTR(p, ref);

	      /* Skip identifiers that haven't been overloaded. */
	      if (ref->id_flags & ID_INHERITED) {
		if ((ref->id_flags & (ID_VARIANT|ID_HIDDEN)) == ID_VARIANT) {
		  /* Find the dispatcher. */
		  int i = really_low_find_shared_string_identifier(id->name, p,
						SEE_PROTECTED|SEE_PRIVATE);
		  /* NB: We use the id_dumped flag for the
		   *     dispatcher to mark whether we have
		   *     dumped the first variant of this
		   *     name in this program.
		   */
		  if ((i >= 0) && !is_variant_dispatcher(p, i) &&
		      !PTR_FROM_INT(p, i)->inherit_offset) {
		    /* Overloaded in this program.
		     *
		     * Make sure later variants don't clear this one.
		     */
		    id_dumped[PTR_FROM_INT(p, i)->identifier_offset] = 1;
		  }
		}
		continue;
	      }

	      /* Skip getter/setter variables; they get pulled in
	       * by their respective functions.
	       */
	      if (!IDENTIFIER_IS_ALIAS(id->identifier_flags) &&
		  IDENTIFIER_IS_VARIABLE(id->identifier_flags) &&
		  (id->run_time_type == PIKE_T_GET_SET))
		continue;

	      EDB(3,
		  fprintf(stderr,
			  "%*sencoding identifier ref %d: %4x \"%s\"\n",
			  data->depth, "", d,
			  id->identifier_flags,
			  id->name->str));

#ifdef ENCODE_DEBUG
	      data->depth += 2;
#endif

	      /* Variable, constant or function. */

	      if (ref->inherit_offset || ref->id_flags & ID_HIDDEN) {
		int ref_no = -1;
		/* Explicit reference to inherited symbol. */

		EDB(3,
		    fprintf(stderr, "%*sencode: encoding raw reference\n",
			    data->depth, ""));

                encode_number(ID_ENTRY_RAW, data, NULL);
                ETRACE({
		    ENCODE_WERR(".ident   raw");
		  });
                encode_number(ref->id_flags, data, "modifiers");

		/* inherit_offset */
                encode_number(ref->inherit_offset, data, "inherit_offset");

		/* identifier_offset */
		/* Find the corresponding identifier reference
		 * in the inherit. */
		{
		  struct program *p2 = p->inherits[ref->inherit_offset].prog;
		  int i;
		  debug_malloc_touch(p);
		  debug_malloc_touch(p2);
		  for (i=0; i < p2->num_identifier_references; i++) {
		    struct reference *ref2 = p2->identifier_references + i;
		    if (!(ref2->inherit_offset) &&
			!(ref2->id_flags & ID_HIDDEN) &&
			(ref2->identifier_offset == ref->identifier_offset)) {
		      ref_no = i;
		      break;
		    }
		  }
		}
		if (ref_no == -1) {
		  Pike_error("Failed to reverse explicit reference\n");
		}
                encode_number(ref_no, data, "ref_no");
	      } else {
		int gs_flags = -1;

		if (id_dumped[ref->identifier_offset]) {
		  EDB(3,
		      fprintf(stderr, "%*sencode: already encoded reference\n",
			      data->depth, ""));
		  goto next_identifier_ref;
		}
		id_dumped[ref->identifier_offset] = 1;

		if (id->name && (id->name->len>3) &&
		    (index_shared_string(id->name, 0) == '`') &&
		    (index_shared_string(id->name, 1) == '-') &&
		    (index_shared_string(id->name, 2) == '>')) {
		  /* Potential old-style getter/setter. */
		  struct pike_string *symbol = NULL;
		  if (index_shared_string(id->name, id->name->len-1) != '=') {
		    /* Getter callback. */
		    symbol = string_slice(id->name, 3, id->name->len - 3);
		  } else if (id->name->len > 4) {
		    /* Setter callback. */
		    symbol = string_slice(id->name, 3, id->name->len - 4);
		  }
		  if (symbol) {
		    int i = really_low_find_shared_string_identifier(symbol, p,
						  SEE_PROTECTED|SEE_PRIVATE);
		    if (i >= 0) {
		      /* Found the symbol. */
		      gs_flags = PTR_FROM_INT(p, i)->id_flags;
		    }
		    free_string(symbol);
		  }
		} else if (id->name && (id->name->len>1) &&
                           (index_shared_string(id->name, 0) == '`') &&
                           wide_isidchar(index_shared_string(id->name, 1))) {
                  /* New-style getter/setter. */
		  struct pike_string *symbol = NULL;
		  if (index_shared_string(id->name, id->name->len-1) != '=') {
		    /* Getter callback. */
		    symbol = string_slice(id->name, 1, id->name->len - 1);
		  } else if (id->name->len > 2) {
		    /* Setter callback. */
		    symbol = string_slice(id->name, 1, id->name->len - 2);
		  }
		  if (symbol) {
		    int i = really_low_find_shared_string_identifier(symbol, p,
						  SEE_PROTECTED|SEE_PRIVATE);
		    if (i >= 0) {
		      /* Found the symbol. */
		      gs_flags = PTR_FROM_INT(p, i)->id_flags;
		    }
		    free_string(symbol);
		  }
		} else if (ref->id_flags & ID_VARIANT) {
		  /* Find the dispatcher. */
		  int i = really_low_find_shared_string_identifier(id->name, p,
						SEE_PROTECTED|SEE_PRIVATE);
		  /* NB: We use the id_dumped flag for the
		   *     dispatcher to mark whether we have
		   *     dumped the first variant of this
		   *     name in this program.
		   */
		  if ((i < 0) || !is_variant_dispatcher(p, i)) {
		    Pike_error("Failed to find dispatcher for inherited "
                               "variant function: %pS\n", id->name);
		  }
		  if (PTR_FROM_INT(p, i)->inherit_offset) {
                    Pike_error("Dispatcher for variant function %pS "
                               "is inherited.\n", id->name);
		  }
		  gs_flags = ref->id_flags & PTR_FROM_INT(p, i)->id_flags;
		  if (id_dumped[PTR_FROM_INT(p, i)->identifier_offset]) {
		    /* Either already dumped, or the dispatcher is in
		     * front of us, which indicates that we are overloading
		     * an inherited function with a variant.
		     */
		    gs_flags |= ID_VARIANT;
		  } else {
		    /* First variant. */
		    id_dumped[PTR_FROM_INT(p, i)->identifier_offset] = 1;
		  }
		}

		if (IDENTIFIER_IS_ALIAS(id->identifier_flags)) {
		  if ((!id->func.ext_ref.depth) &&
   		      IDENTIFIER_IS_VARIABLE(id->identifier_flags)) {
		    struct identifier *other =
		      ID_FROM_INT(p, id->func.ext_ref.id);
		    if (other->name == id->name) {
		      /* Let define_variable() handle the decoding. */
		      EDB(3, fprintf(stderr,
				     "%*sencode: encoding aliased variable\n",
				     data->depth, ""));
		      goto encode_entry_variable;
		    }
    		  }
	 	  EDB(3, fprintf(stderr, "%*sencode: encoding alias\n",
				 data->depth, ""));

                  encode_number(ID_ENTRY_ALIAS, data, NULL);
                  ETRACE({
		      ENCODE_WERR(".ident   alias");
		    });

		  /* flags */
                  encode_number(ref->id_flags, data, "modifiers");

		  /* name */
		  str_sval.u.string = id->name;
		  encode_value2(&str_sval, data, 0);

		  /* type */
		  ref_push_type_value(id->type);
		  encode_value2(Pike_sp-1, data, 0);
		  pop_stack();

		  /* filename */
                  encode_number(id->filename_strno, data, "filename_strno");

		  /* linenumber */
                  encode_number(id->linenumber, data, "linenumber");

		  /* depth */
                  encode_number(id->func.ext_ref.depth, data, "depth");

		  /* refno */
                  encode_number(id->func.ext_ref.id, data, "refno");

		} else switch (id->identifier_flags & IDENTIFIER_TYPE_MASK) {
		case IDENTIFIER_CONSTANT:
		  EDB(3,
		      fprintf(stderr, "%*sencode: encoding constant\n",
			      data->depth, ""));

                  encode_number(ID_ENTRY_CONSTANT, data, NULL);
                  ETRACE({
		      ENCODE_WERR(".ident   constant");
		    });
		  if (gs_flags >= 0) {
                    encode_number(gs_flags, data, "modifiers");
		  } else {
                    encode_number(ref->id_flags, data, "modifiers");
		  }

		  /* name */
		  str_sval.u.string = id->name;
		  encode_value2(&str_sval, data, 0);

		  /* type */
		  ref_push_type_value(id->type);
		  encode_value2(Pike_sp-1, data, 0);
		  pop_stack();

		  /* filename */
                  encode_number(id->filename_strno, data, "filename_strno");

		  /* linenumber */
                  encode_number(id->linenumber, data, "linenumber");

		  /* offset */
                  encode_number(id->func.const_info.offset, data, "offset");

		  /* run-time type */
                  encode_number(id->run_time_type, data, "run_time_type");

		  /* opt flags */
                  encode_number(id->opt_flags, data, "opt_flags");
		  break;

		case IDENTIFIER_PIKE_FUNCTION:
		encode_pike_function:
		  EDB(3,
		      fprintf(stderr, "%*sencode: encoding function\n",
			      data->depth, ""));

                  encode_number(ID_ENTRY_FUNCTION, data, NULL);
                  ETRACE({
		      ENCODE_WERR(".ident   function");
		    });
		  if (gs_flags >= 0) {
                    encode_number(gs_flags, data, "modifiers");
		  } else {
                    encode_number(ref->id_flags, data, "modifiers");
		  }

		  /* name */
		  str_sval.u.string = id->name;
                  encode_value2(&str_sval, data, 0);

		  /* type */
		  ref_push_type_value(id->type);
                  encode_value2(Pike_sp-1, data, 0);
		  pop_stack();

		  /* filename */
                  encode_number(id->filename_strno, data, "filename_strno");

		  /* linenumber */
                  encode_number(id->linenumber, data, "linenumber");

		  /* func_flags (aka identifier_flags) */
                  encode_number(id->identifier_flags, data,
                                "identifier_flags");

		  /* func */
		  if (id->func.offset >= 0) {
		    /* Code the number of the string containing
		     * the raw bytecode.
		     */
                    encode_number(read_program_data(p->program + id->func.offset,
                                                    -1), data,
                                  "byte_code_strno");
		  } else {
		    /* Prototype */
                    encode_number(-1, data, "byte_code_strno");
		  }

		  /* opt_flags */
                  encode_number(id->opt_flags, data, "opt_flags");
		  break;

		case IDENTIFIER_C_FUNCTION:
		  if (is_variant_dispatcher(p, d)) {
		    int k;
		    int j;
		    struct identifier *other = NULL;
		    /* This is handled by end_first_pass() et al. */
		    /* NB: This can be reached even though id_dumped
		     *     for it gets set by the variant functions,
		     *     if it is overriding an old definition.
		     *
		     * Note that this means that even the first local
		     * function must have the variant modifier (since
		     * otherwise it would have overridden the old def
		     * and not the dispatcher). This is handled
		     * automatically by the use of id_dumped for the
		     * dispatcher as marker for whether the first
		     * variant has been added or not.
		     */
		    EDB(3,
		      fprintf(stderr, "%*sencode: encoding variant dispatcher\n",
			      data->depth, ""));
                    ETRACE({
			ENCODE_WERR("# Variant dispatcher.\n");
		      });

		    for(k = 0; k < p->num_identifiers; k++) {
		      other = p->identifiers + k;
		      if (other->name == id->name) break;
		    }
		    if (other >= id) {
		      /* variant before termination function. */
		      EDB(3, fprintf(stderr, "%*sVariant before termination function.\n",
				     data->depth, ""));
                      ETRACE({
			  ENCODE_WERR("# Before termination function (skipped).");
			});
		      goto next_identifier_ref;
		    }
		    if ((other->identifier_flags & IDENTIFIER_TYPE_MASK) ==
			IDENTIFIER_PIKE_FUNCTION) {
		      struct reference *r = NULL;
		      for (j = 0; j < p->num_identifier_references; j++) {
			r = p->identifier_references + j;
			if (!r->inherit_offset && (r->identifier_offset == k)) {
			  /* Found the override. */
			  break;
			}
		      }
		      if ((j != p->num_identifier_references) && (j > d)) {
			/* We have a termination function at identifier k,
			 * that was overridden by the variant dispatcher.
			 *
			 * We need to encode it here at its original place.
			 */
			id_dumped[k] = 1;
			gs_flags = r->id_flags & ref->id_flags;
			id = other;
			ref = r;
			EDB(3, fprintf(stderr, "%*sEncoding termination function.\n",
				       data->depth, ""));
                        ETRACE({
			    ENCODE_WERR("# Encoding termination function.");
			  });
			goto encode_pike_function;
		      }
		    }
		  }
		  /* Not supported. */
		  Pike_error("Cannot encode functions implemented in C "
                             "(identifier=%pq).\n",
			     id->name);
		  break;

		case IDENTIFIER_VARIABLE:
		  if (d < d_min) {
		    EDB(3,
			fprintf(stderr, "%*sencode: Skipping overloaded variable \"%s\"\n",
				data->depth, "",
				id->name->str));
		    /* We still want to dump it later... */
		    id_dumped[ref->identifier_offset] = 0;
		    goto next_identifier_ref;
		  }
		  EDB(3,
		      fprintf(stderr, "%*sencode: encoding variable\n",
			      data->depth, ""));
		encode_entry_variable:
                  encode_number(ID_ENTRY_VARIABLE, data, NULL);
                  ETRACE({
		      ENCODE_WERR(".ident   variable");
		    });
		  if (gs_flags >= 0) {
                    encode_number(gs_flags, data, "modifiers");
		  } else {
                    encode_number(ref->id_flags, data, "modifiers");
		  }

		  /* name */
		  str_sval.u.string = id->name;
		  encode_value2(&str_sval, data, 0);

		  /* type */
		  ref_push_type_value(id->type);
		  encode_value2(Pike_sp-1, data, 0);
		  pop_stack();

		  /* filename */
                  encode_number(id->filename_strno, data, "filename_strno");

		  /* linenumber */
                  encode_number(id->linenumber, data, "linenumber");
		  break;

#ifdef PIKE_DEBUG
		default:
		  Pike_fatal ("Unknown identifier type: 0x%04x for symbol \"%s\".\n",
			      id->identifier_flags & IDENTIFIER_TYPE_MASK,
			      id->name->str);
#endif
		}
	      }

	      /* Identifier reference number */
              encode_number(d, data, "ref_no");

	    next_identifier_ref:
	      ;		/* C requires a statement after lables. */
#ifdef ENCODE_DEBUG
	      data->depth -= 2;
#endif
	    }

	    /* Encode next inherit. */
	    if (inherit_num < p->num_inherits) {
	      /* Inherit */

	      /* Flags that have been set by/after the inherit. */
	      unsigned INT16 inherit_flags_set = 0;
	      /* Mask of flags that may have been affected by
	       * the inherit. */
	      unsigned INT16 inherit_flags_mask = ~(ID_HIDDEN|ID_INHERITED);
	      struct inherit *inh = p->inherits + inherit_num;
	      struct reference *ref = p->identifier_references + d;
	      int i;

	      /* The references from this inherit stop at this point. */
	      d_min = inh->identifier_level +
		inh->prog->num_identifier_references;

	      EDB(3,
		  fprintf(stderr, "%*sencode: encoding inherit\n",
			  data->depth, ""));

#ifdef ENCODE_DEBUG
	      data->depth += 2;
#endif

              encode_number(ID_ENTRY_INHERIT, data, NULL);
              ETRACE({
		  ENCODE_WERR(".ident   inherit");
		});

	      /* Calculate id_flags */
	      for (i = 0; i < inh->prog->num_identifier_references; i++) {
		if (ref[i].inherit_offset) {
		  unsigned INT16 id_flags = ref[i].id_flags;
		  unsigned INT16 inh_id_flags =
		    inh->prog->identifier_references[i].id_flags;
		  /* Ignore identifiers that have been hidden. */
		  if (!(id_flags & ID_HIDDEN)) {
		    inherit_flags_set |= id_flags & ~inh_id_flags;
		    if (inh_id_flags & ID_PUBLIC) {
		      /* Public symbols aren't affected by a
		       * private inherit. */
		      inherit_flags_mask &= id_flags | ID_PRIVATE;
		    } else {
		      inherit_flags_mask &= id_flags;
		    }
		  }
		} else {
		  /* If an inherited identifiers has been overloaded,
		   * it can not have been a local inherit. */
		  inherit_flags_mask &= ~ID_LOCAL;
		}
	      }
	      EDB(5,
		  fprintf(stderr, "%*sraw inherit_flags_set: %04x:%04x\n",
			  data->depth, "",
			  inherit_flags_set, inherit_flags_mask));
	      inherit_flags_set &= inherit_flags_mask;
              encode_number(inherit_flags_set, data, "modifiers");

	      EDB(5,
		  fprintf(stderr, "%*sinherit_flags: %04x\n",
			  data->depth, "", inherit_flags_set));

	      /* Identifier reference level at insertion. */
              encode_number(d_max, data, "max_ref_no");

	      /* name */
              if (!inh->name)
                  Pike_error("Cannot encode programs with unnamed inherits.\n");
	      str_sval.u.string = inh->name;
	      encode_value2(&str_sval, data, 0);

	      /* prog */
	      ref_push_program(inh->prog);
	      encode_value2(Pike_sp-1, data, 1);
	      pop_stack();

	      /* parent */
	      if (inh->parent) {
		ref_push_object(inh->parent);
	      } else {
		push_int(0);
	      }
	      encode_value2(Pike_sp-1, data, 0);
	      pop_stack();

	      /* parent_identifier */
              encode_number(inh->parent_identifier, data, "parent_id");

	      /* parent_offset */
              encode_number(inh->parent_offset, data, "parent_offset");

	      /* Number of identifier references. */
              encode_number(inh->prog->num_identifier_references, data,
                            "num_refs");

	      inherit_num += inh->prog->num_inherits;

#ifdef ENCODE_DEBUG
	      data->depth -= 2;
#endif
	    }
	  }
	  /* End-marker */
          encode_number(ID_ENTRY_EOT, data, NULL);
          ETRACE({
	      ENCODE_WERR_COMMENT("End of identifier table",
				  ".ident   eot");
	    });

#ifdef ENCODE_DEBUG
	  data->depth -= 2;
#endif
	}

	/* Encode the constant values table. */
	{
	  struct svalue str_sval;
	  SET_SVAL(str_sval, T_STRING, 0, string, NULL);

	  EDB(2,
	      fprintf(stderr, "%*sencode: encoding constants\n",
		      data->depth, ""));
          ETRACE({
	      ENCODE_WERR("# Constant table");
	    });

	  /* constants */
	  for(d=0;d<p->num_constants;d++)
	  {
	    EDB(5,
		fprintf(stderr, "%*sencode: encoding constant #%d\n",
			data->depth, "", d));

            ETRACE({
		ENCODE_WERR("# Constant #%d", d);
	      });

	    if (((TYPEOF(p->constants[d].sval) == T_FUNCTION) &&
		 (SUBTYPEOF(p->constants[d].sval) == FUNCTION_BUILTIN)) ||
		(TYPEOF(p->constants[d].sval) == T_TYPE)) {
	      /* Already encoded above. */
	      continue;
	    }

	    /* value */
	    encode_value2(&p->constants[d].sval, data, 0);

	    /* name */
	      push_int(0);
	      encode_value2(Pike_sp-1, data, 0);
	      dmalloc_touch_svalue(Pike_sp-1);
	      Pike_sp--;
	  }
	}
      }else{
	code_entry(TAG_PROGRAM, 0, data);
        ETRACE({
	    ENCODE_WERR(".entry   program");
	  });
	encode_value2(Pike_sp-1, data, 0);
	pop_stack();
      }
      break;
    }
  }

encode_done:;

#ifdef ENCODE_DEBUG
  data->depth -= 2;
#endif
  ETRACE({
      ENCODE_FLUSH();
    });
}

static void free_encode_data(struct encode_data *data)
{
  buffer_free(& data->buf);
  if (data->codec) free_object (data->codec);
  free_mapping(data->encoded);
  free_array(data->delayed);
}

/*! @decl string encode_value(mixed value, Codec|void codec)
 *! @decl string encode_value(mixed value, Codec|void codec, @
 *!                           String.Buffer trace)
 *! @decl string encode_value(mixed value, Codec|void codec, @
 *!                           int(0..) debug)
 *!
 *! Code a value into a string.
 *!
 *! @param value
 *!   Value to encode.
 *!
 *! @param codec
 *!   @[Codec] to use when encoding objects and programs.
 *!
 *! @param trace
 *!   @[String.Buffer] to contain a readable dump of the value.
 *!
 *! @param debug
 *!   Debug level. Only available when the runtime has been compiled
 *!   --with-rtl-debug.
 *!
 *! This function takes a value, and converts it to a string. This string
 *! can then be saved, sent to another Pike process, packed or used in
 *! any way you like. When you want your value back you simply send this
 *! string to @[decode_value()] and it will return the value you encoded.
 *!
 *! Almost any value can be coded, mappings, floats, arrays, circular
 *! structures etc.
 *!
 *! If @[codec] is specified, it's used as the codec for the encode.
 *! If none is specified, then one is instantiated through
 *! @expr{master()->Encoder()@}. As a compatibility fallback, the
 *! master itself is used if it has no @expr{Encoder@} class.
 *!
 *! If @expr{@[codec]->nameof(o)@} returns @tt{UNDEFINED@} for an
 *! object, @expr{val = o->encode_object(o)@} will be called. The
 *! returned value will be passed to @expr{o->decode_object(o, val)@}
 *! when the object is decoded.
 *!
 *! @note
 *!
 *! When only simple types like int, floats, strings, mappings,
 *! multisets and arrays are encoded, the produced string is very
 *! portable between pike versions. It can at least be read by any
 *! later version.
 *!
 *! The portability when objects, programs and functions are involved
 *! depends mostly on the codec. If the byte code is encoded, i.e.
 *! when Pike programs are actually dumped in full, then the string
 *! can probably only be read by the same pike version.
 *!
 *! @seealso
 *!   @[decode_value()], @[sprintf()], @[encode_value_canonic()]
 */
void f_encode_value(INT32 args)
{
  ONERROR tmp;
  struct encode_data d, *data;
  struct string_builder debug_buf;
  int i;
  data=&d;

  check_all_args("encode_value", args,
		 BIT_MIXED,
		 BIT_VOID | BIT_OBJECT | BIT_ZERO,
                 BIT_VOID | BIT_OBJECT
#ifdef ENCODE_DEBUG
		 /* This argument is only an internal debug helper.
		  * It's intentionally not part of the function
		  * prototype, to keep the argument position free for
		  * other uses in the future. */
                 | BIT_INT
#endif
                 , 0);

  buffer_init(&data->buf);
  data->canonic = 0;
  data->encoded=allocate_mapping(128);
  data->encoded->data->flags |= MAPPING_FLAG_NO_SHRINK;
  data->delayed = allocate_array (0);
  SET_SVAL(data->counter, T_INT, NUMBER_NUMBER, integer, COUNTER_START);

  data->debug_buf = NULL;
  data->debug_pos = 0;
#ifdef ENCODE_DEBUG
  data->debug = 0;
  data->depth = -2;
#endif
  if (args > 2) {
    if (TYPEOF(Pike_sp[2-args]) == PIKE_T_OBJECT) {
      struct object *sb = Pike_sp[2-args].u.object;
      if ((data->debug_buf = string_builder_from_string_buffer(sb))) {
#ifdef ENCODE_DEBUG
	data->debug = 1;
#endif
      }
#ifdef ENCODE_DEBUG
    } else {
      if ((data->debug = Pike_sp[2-args].u.integer)) {
	init_string_builder(&debug_buf, 0);
	data->debug_buf = &debug_buf;
      }
#endif
    }
  }

  if(args > 1 && TYPEOF(Pike_sp[1-args]) == T_OBJECT)
  {
    if (SUBTYPEOF(Pike_sp[1-args]))
      Pike_error("The codec may not be a subtyped object yet.\n");

    data->codec=Pike_sp[1-args].u.object;
    add_ref (data->codec);
  }else{
    data->codec=NULL;
  }

  SET_ONERROR(tmp, free_encode_data, data);
  addstr("\266ke0", 4);

  if (data->debug_buf) {
    string_builder_append_disassembly(data->debug_buf,
                                      data->debug_pos,
                                      ((char *)data->buf.end) -
                                      (data->buf.length - data->debug_pos),
                                      data->buf.dst,
                                      ".format  0",
                                      NULL,
                                      "Encoding #0.");
    data->debug_pos = data->buf.length -
      (((char *)data->buf.end) - ((char *)data->buf.dst));
  }

  encode_value2(Pike_sp-args, data, 1);

  for (i = 0; i < data->delayed->size; i++) {
    ETRACE( {
	string_builder_sprintf(data->debug_buf,
			       "0x%016zx  # Delayed item #%d\n",
			       data->debug_pos, i);
      });
    encode_value2 (ITEM(data->delayed) + i, data, 2);
  }

  UNSET_ONERROR(tmp);

  ETRACE({
      ENCODE_FLUSH();
    });

  if (data->codec) free_object (data->codec);
  free_mapping(data->encoded);
  free_array (data->delayed);

#ifdef ENCODE_DEBUG
  if (data->debug_buf == &debug_buf) {
    write_and_reset_string_builder(2, data->debug_buf);
    free_string_builder(data->debug_buf);
  }
#endif

  pop_n_elems(args);
  push_string(buffer_finish_pike_string(&data->buf));
}

/*! @decl string encode_value_canonic(mixed value, object|void codec)
 *! @decl string encode_value_canonic(mixed value, object|void codec, @
 *!                                   String.buffer trace)
 *! @decl string encode_value_canonic(mixed value, object|void codec, @
 *!                                   int(0..) debug)
 *!
 *! Code a value into a string on canonical form.
 *!
 *! @param value
 *!   Value to encode.
 *!
 *! @param codec
 *!   @[Codec] to use when encoding objects and programs.
 *!
 *! @param trace
 *!   @[String.Buffer] to contain a readable dump of the value.
 *!
 *! @param debug
 *!   Debug level. Only available when the runtime has been compiled
 *!   --with-rtl-debug.
 *!
 *! Takes a value and converts it to a string on canonical form, much like
 *! @[encode_value()]. The canonical form means that if an identical value is
 *! encoded, it will produce exactly the same string again, even if it's
 *! done at a later time and/or in another Pike process. The produced
 *! string is compatible with @[decode_value()].
 *!
 *! @note
 *!   Note that this function is more restrictive than @[encode_value()] with
 *!   respect to the types of values it can encode. It will throw an error
 *!   if it can't encode to a canonical form.
 *!
 *! @seealso
 *!   @[encode_value()], @[decode_value()]
 */
void f_encode_value_canonic(INT32 args)
{
  ONERROR tmp;
  struct encode_data d, *data;
#ifdef ENCODE_DEBUG
  struct string_builder debug_buf;
#endif
  int i;
  data=&d;

  check_all_args("encode_value_canonic", args,
		 BIT_MIXED,
		 BIT_VOID | BIT_OBJECT | BIT_ZERO,
                 BIT_VOID | BIT_OBJECT
#ifdef ENCODE_DEBUG
		 /* This argument is only an internal debug helper.
		  * It's intentionally not part of the function
		  * prototype, to keep the argument position free for
		  * other uses in the future. */
                 | BIT_INT
#endif
                 , 0);

  buffer_init(&data->buf);
  data->canonic = 1;
  data->encoded=allocate_mapping(128);
  data->delayed = allocate_array (0);
  SET_SVAL(data->counter, T_INT, NUMBER_NUMBER, integer, COUNTER_START);

  data->debug_buf = NULL;
  data->debug_pos = 0;
#ifdef ENCODE_DEBUG
  data->debug = 0;
  data->depth = -2;
#endif
  if (args > 2) {
    if (TYPEOF(Pike_sp[2-args]) == PIKE_T_OBJECT) {
      struct object *sb = Pike_sp[2-args].u.object;
      if ((data->debug_buf = string_builder_from_string_buffer(sb))) {
#ifdef ENCODE_DEBUG
	data->debug = 1;
#endif
      }
#ifdef ENCODE_DEBUG
    } else {
      if ((data->debug = Pike_sp[2-args].u.integer)) {
	init_string_builder(&debug_buf, 0);
	data->debug_buf = &debug_buf;
      }
#endif
    }
  }

  if(args > 1 && TYPEOF(Pike_sp[1-args]) == T_OBJECT)
  {
    if (SUBTYPEOF(Pike_sp[1-args]))
      Pike_error("The codec may not be a subtyped object yet.\n");

    data->codec=Pike_sp[1-args].u.object;
    add_ref (data->codec);
  }else{
    data->codec=NULL;
  }

  SET_ONERROR(tmp, free_encode_data, data);
  addstr("\266ke0", 4);

  ETRACE({
      string_builder_append_disassembly(data->debug_buf,
					data->debug_pos,
					((char *)data->buf.end) -
					(data->buf.length - data->debug_pos),
					data->buf.dst,
					".format  0",
					NULL,
					"Encoding #0.");
      data->debug_pos = data->buf.length -
	(((char *)data->buf.end) - ((char *)data->buf.dst));
    });

  encode_value2(Pike_sp-args, data, 1);

  for (i = 0; i < data->delayed->size; i++) {
    ETRACE({
	string_builder_sprintf(data->debug_buf,
			       "0x%016zx  # Delayed item #%d\n",
			       data->debug_pos, i);
      });
    encode_value2 (ITEM(data->delayed) + i, data, 2);
  }

  UNSET_ONERROR(tmp);

  ETRACE({
      ENCODE_FLUSH();
    });

  if (data->codec) free_object (data->codec);
  free_mapping(data->encoded);
  free_array (data->delayed);

#ifdef ENCODE_DEBUG
  if (data->debug_buf == &debug_buf) {
    write_and_reset_string_builder(2, data->debug_buf);
    free_string_builder(data->debug_buf);
  }
#endif

  pop_n_elems(args);
  push_string(buffer_finish_pike_string(&data->buf));
}


struct unfinished_prog_link
{
  struct unfinished_prog_link *next;
  struct program *prog;
};

struct unfinished_obj_link
{
  struct unfinished_obj_link *next;
  struct object *o;
  struct svalue decode_arg;
};

struct decode_data
{
  struct pike_string *data_str;
  unsigned char *data;
  ptrdiff_t len;
  ptrdiff_t ptr;
  struct mapping *decoded;
  struct unfinished_prog_link *unfinished_programs;
  struct unfinished_obj_link *unfinished_objects;
  struct unfinished_obj_link *unfinished_placeholders;
  struct svalue counter;
  struct object *codec;
  int explicit_codec;
  int pickyness;
  int pass;
  int delay_counter;
  int support_delay_counter;
  struct compilation *support_compilation;
  struct pike_string *raw;
  struct decode_data *next;
#ifdef PIKE_THREADS
  struct thread_state *thread_state;
  struct object *thread_obj;
#endif
  struct string_builder *debug_buf;
  ptrdiff_t debug_ptr;
#ifdef ENCODE_DEBUG
  int debug, depth;
#endif
};

static struct object *decoder_codec (struct decode_data *data)
{
  struct pike_string *decoder_str;
  if (data->codec) return data->codec;
  if (data->explicit_codec)
    Pike_fatal("Trying to load codec while explicitely opted out.\n");
  MAKE_CONST_STRING (decoder_str, "Decoder");
  return data->codec = lookup_codec (decoder_str);
}

static void decode_value2(struct decode_data *data);

static int my_extract_char(struct decode_data *data)
{
  if(data->ptr >= data->len)
    Pike_error("Not enough data in string.\n");

#ifdef ENCODE_DEBUG
  if (data->debug >= 6) {
    debug_dump_mem(data->ptr, data->data + data->ptr, 1);
  }
#endif /* PIKE_DEBUG */
  return data->data [ data->ptr++ ];
}

static DECLSPEC(noreturn) void decode_error (
  struct decode_data *data, struct svalue *decoding, const char *msg, ...)
     ATTRIBUTE((noinline,noreturn)) ATTRIBUTE((format (printf, 3, 4)));

static DECLSPEC(noreturn) void decode_error (
  struct decode_data *data, struct svalue *decoding, const char *msg, ...)
{
  int n = 0;
  va_list args;
  struct string_builder sb;
  struct object *o = fast_clone_object (decode_error_program);
  struct decode_error_struct *dec =
    (struct decode_error_struct *) (o->storage + decode_error_offset);
  struct generic_error_struct *gen = get_storage (o, generic_error_program);

  ASSERT_THREAD_SWAPPED_IN();

  copy_shared_string (dec->decode_string, data->data_str);

  if (decoding) {
    push_static_text ("Error while decoding "); n++;
    push_static_text ("%O");
    push_svalue (decoding);
    f_sprintf (2); n++;
    push_static_text (":\n"); n++;
  }
  else {
    push_static_text ("Decode error: "); n++;
  }

  init_string_builder (&sb, 0);
  va_start (args, msg);
  string_builder_vsprintf (&sb, msg, args);
  va_end (args);
  push_string (finish_string_builder (&sb)); n++;

  f_add (n);
  gen->error_message = (--Pike_sp)->u.string;

  generic_error_va (o, NULL, NULL, 0, NULL, NULL);
}

#define GETC() my_extract_char(data)

#define DECODE(Z) do {					\
  EDB(5,						\
    fprintf(stderr,"%*sdecode(%s) at %d:\n",		\
	    data->depth,"",(Z),__LINE__));		\
  what=GETC();						\
  e=what>>SIZE_SHIFT;					\
  if(what & TAG_SMALL) {				\
     num=e;						\
  } else {						\
    num = 0;						\
    while(e-->=0) num = ((UINT64)num<<8)                \
			    + (GETC()+1);		\
    num += MAX_SMALL - 1;				\
  }							\
  if(what & TAG_NEG) {					\
    num = ~num;						\
  }							\
  EDB(5,						\
      fprintf(stderr,"%*s==> type=%d (%s), num=%ld\n",	\
	      data->depth,"",(what & TAG_MASK),			\
	      get_name_of_type(tag_to_type(what & TAG_MASK)),	\
	    (long)num) ); 				\
} while (0)

#define decode_entry(X,Y,Z)					\
  do {								\
    INT32 what, e;						\
    INT64 num;							\
    DECODE("decode_entry");					\
    if((what & TAG_MASK) != (X))				\
      decode_error(data, NULL, "Wrong bits (%d).\n", what & TAG_MASK);	\
    (Y)=num;								\
  } while(0);

#if PIKE_BYTEORDER == 4321
#define BITFLIP(S)
#else
#define BITFLIP(S)						\
   switch(what)							\
   {								\
     case 1: for(e=0;e<num;e++) STR1(S)[e]=ntohs(STR1(S)[e]); break;	\
     case 2: for(e=0;e<num;e++) STR2(S)[e]=ntohl(STR2(S)[e]); break;    \
   }
#endif

#define get_string_data(STR,LEN, data) do {				    \
  if((LEN) == -1)							    \
  {									    \
    INT32 what, e;							\
    INT64 num;								\
    ptrdiff_t sz;							\
    DECODE("get_string_data");						    \
    what &= TAG_MASK;							    \
    if(what<0 || what>2)						    \
      decode_error (data, NULL, "Illegal size shift %d.\n", what);	\
    ETRACE({								\
	DECODE_WERR(".string  %ld<<%d", (long)num, what);		\
      });								\
    sz = (ptrdiff_t) num << what;					\
    if (sz < 0)								\
      decode_error (data, NULL, "Illegal negative size %td.\n", sz);	\
    if (sz > data->len - data->ptr)					\
      decode_error (data, NULL, "Too large size %td (max is %td).\n",	\
		    sz, data->len - data->ptr);				\
    STR=begin_wide_shared_string(num, what);				\
    memcpy(STR->str, data->data + data->ptr, sz);			\
    EDB(6, debug_dump_mem(data->ptr, data->data + data->ptr, sz));	\
    data->ptr += sz;							\
    BITFLIP(STR);							    \
    STR=end_shared_string(STR);                                             \
  }else{								    \
    ptrdiff_t sz = (LEN);						\
    if (sz < 0)								\
      decode_error (data, NULL, "Illegal negative size %td.\n", sz);	\
    if (sz > data->len - data->ptr)					\
      decode_error (data, NULL, "Too large size %td (max is %td).\n",	\
		    sz, data->len - data->ptr);				\
    ETRACE({								\
	DECODE_WERR(".string  %td", sz);				\
      });								\
    STR=make_shared_binary_string((char *)(data->data + data->ptr), sz); \
    EDB(6, debug_dump_mem(data->ptr, data->data + data->ptr, sz));	\
    data->ptr += sz;							\
  }									    \
  ETRACE({								\
      DECODE_FLUSH();							\
    });									\
}while(0)

static INT64 decode_number(struct decode_data *data, const char *comment)
{
  INT32 what, e;
  INT64 num;

  DECODE("decode_number");
  num = (what & TAG_MASK) | ((UINT64)num<<4);
  EDB(5, fprintf(stderr, "%*s  ==>%"PRINTINT64"d\n",
                 data->depth, "", num));
  if (comment) {
    ETRACE({
        DECODE_WERR_COMMENT(comment, ".number  "
                            "%"PRINTINT64"d", num);
      });
  }

  return num;
}

#define decode_number(X, data, comment) do {    \
    X = decode_number(data, comment);           \
  } while(0)

static void restore_type_stack(struct pike_type **old_stackp)
{
#ifdef PIKE_DEBUG
  if (old_stackp > Pike_compiler->type_stackp) {
    Pike_fatal("type stack out of sync!\n");
  }
#endif /* PIKE_DEBUG */
  while(Pike_compiler->type_stackp > old_stackp) {
    compiler_discard_top_type();
  }
}

static void restore_type_mark(struct pike_type ***old_type_mark_stackp)
{
#ifdef PIKE_DEBUG
  if (old_type_mark_stackp > Pike_compiler->pike_type_mark_stackp) {
    Pike_fatal("type Pike_interpreter.mark_stack out of sync!\n");
  }
#endif /* PIKE_DEBUG */
  Pike_compiler->pike_type_mark_stackp = old_type_mark_stackp;
}

static void low_decode_type(struct decode_data *data)
{
  /* FIXME: Probably ought to use the tag encodings too. */

  int tmp;
  ONERROR err1;
  ONERROR err2;

  SET_ONERROR(err1, restore_type_stack, Pike_compiler->type_stackp);
  SET_ONERROR(err2, restore_type_mark, Pike_compiler->pike_type_mark_stackp);

  tmp = GETC();
  if (tmp <= MAX_TYPE) tmp ^= MIN_REF_TYPE;
  ETRACE({
      DECODE_WERR(".type    %s", get_name_of_type(tmp));
    });
  switch(tmp)
  {
    default:
      decode_error(data, NULL, "Error in type string (%d).\n", tmp);
      UNREACHABLE(break);

    case T_ASSIGN:
      tmp = GETC();
      if ((tmp < '0') || (tmp > '9')) {
	decode_error(data, NULL, "Bad marker in type string (%d).\n", tmp);
      }
      low_decode_type(data);
      push_assign_type(tmp);	/* Actually reverse, but they're the same */
      break;

    case T_SCOPE:
      tmp = GETC();
      low_decode_type(data);
      push_scope_type(tmp);	/* Actually reverse, but they're the same */
      break;

    case PIKE_T_FUNCTION_ARG:
      low_decode_type(data);	/* Argument */
      low_decode_type(data);	/* Continuation */
      push_reverse_type(T_FUNCTION);
      break;

    case T_FUNCTION:
      {
	int narg = 0;

	while (GETC() != T_MANY) {
	  data->ptr--;
	  low_decode_type(data);
	  narg++;
	}
	low_decode_type(data);	/* Many */
	low_decode_type(data);	/* Return */
	push_reverse_type(T_MANY);
	while(narg-- > 0) {
	  push_reverse_type(T_FUNCTION);
	}
      }
      break;

    case T_MAPPING:
    case T_OR:
    case T_AND:
      low_decode_type(data);
      low_decode_type(data);
      push_reverse_type(tmp);
      break;

    case T_TYPE:
    case T_PROGRAM:
    case T_MULTISET:
    case T_NOT:
      low_decode_type(data);
      push_type(tmp);
      break;

    case T_INT:
      {
	INT32 min=0, max=0;
	if(data->ptr + 8 > data->len)
          decode_error(data, NULL, "Not enough data.\n");
	EDB(6, debug_dump_mem(data->ptr, data->data + data->ptr, 8));
	min = get_unaligned_be32(data->data + data->ptr);
	data->ptr += 4;
        ETRACE({
            DECODE_WERR(".data    %d", (int)min);
          });
	max = get_unaligned_be32(data->data + data->ptr);
	data->ptr += 4;
        ETRACE({
            DECODE_WERR(".data    %d", (int)max);
          });

        if (min > max)
          decode_error(data, NULL, "Error in int type (min (%d) > max (%d)).\n", min, max);

	push_int_type(min, max);
      }
      break;

    case T_STRING:
      /* Common case and compat */
      push_finished_type(int_type_string);
      push_unlimited_array_type(T_STRING);
      break;

    case PIKE_T_NSTRING:
      tmp = T_STRING;
      /* FALLTHRU */
    case T_ARRAY:
      {
	low_decode_type(data);
	push_unlimited_array_type(tmp);
      }
      break;

    case PIKE_T_LSTRING:
      low_decode_type(data);
      low_decode_type(data);
      push_reverse_type(PIKE_T_STRING);
      break;

    case PIKE_T_LARRAY:
      low_decode_type(data);
      low_decode_type(data);
      push_reverse_type(PIKE_T_ARRAY);
      break;

    case PIKE_T_OPERATOR:
      tmp |= GETC()<<8;
      ETRACE({
          DECODE_WERR(".htype    %s", get_name_of_type(tmp));
        });
      low_decode_type(data);
      if (tmp & 0x8000) {
        low_decode_type(data);
        push_reverse_type(tmp);
      } else {
        switch(tmp) {
        case PIKE_T_FIND_LFUN:
          {
            struct pike_string *str;
            int lfun = -1;
            int i;
            decode_value2(data);

            if (TYPEOF(Pike_sp[-1]) != PIKE_T_STRING) {
              decode_error(data, NULL, "Lfun name is not a string: %pO\n",
                           Pike_sp - 1);
            }
            str = Pike_sp[-1].u.string;
            for (i = 0; i < 256; i++) {
              if (lfun_strings[i] == str) {
                lfun = i;
                break;
              }
            }
            if (lfun == -1) {
              decode_error(data, NULL, "Unknown lfun: %pq\n", str);
            }
            pop_stack();
            push_type_operator(PIKE_T_FIND_LFUN, (struct pike_type *)(ptrdiff_t)lfun);
            break;
          }
        default:
          decode_error(data, NULL, "Error in type string (%d).\n", tmp);
          UNREACHABLE(break);
        }
      }
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
    case T_MIXED:
    case T_ZERO:
    case T_VOID:
    case PIKE_T_UNKNOWN:
      push_type(tmp);
      break;

    case PIKE_T_ATTRIBUTE:
      decode_value2(data);

      if (TYPEOF(Pike_sp[-1]) != PIKE_T_STRING) {
	decode_error(data, NULL, "Type attribute is not a string: %pO\n",
		     Pike_sp - 1);
      }
      low_decode_type(data);
      push_type_attribute(Pike_sp[-1].u.string);
      pop_stack();
      break;

    case PIKE_T_NAME:
      decode_value2(data);

      if (TYPEOF(Pike_sp[-1]) != PIKE_T_STRING) {
	decode_error(data, NULL, "Type name is not a string: %pO\n",
		     Pike_sp - 1);
      }
      low_decode_type(data);
      push_type_name(Pike_sp[-1].u.string);
      pop_stack();
      break;

    case T_OBJECT:
    {
      int flag = GETC();

      decode_value2(data);
      switch(TYPEOF(Pike_sp[-1]))
      {
	case T_INT:
	  push_object_type_backwards(flag, Pike_sp[-1].u.integer );
	  break;

	case T_PROGRAM:
	  push_object_type_backwards(flag, Pike_sp[-1].u.program->id);
	  break;

        case T_FUNCTION:
	  {
	    struct program *prog;
	    if (SUBTYPEOF(Pike_sp[-1]) == FUNCTION_BUILTIN) {
	      decode_error(data, NULL, "Failed to decode object type.\n");
	    }
	    prog = program_from_svalue(Pike_sp-1);
	    if (!prog) {
	      decode_error(data, NULL, "Failed to decode object type.\n");
	    }
	    debug_malloc_touch(prog);
	    push_object_type_backwards(flag, prog->id);
	  }
	  break;

	default:
	  decode_error(data, NULL, "Failed to decode type "
		       "(object(%s), expected object(zero|program)).\n",
		       get_name_of_type(TYPEOF(Pike_sp[-1])));
      }
      pop_stack();
    }
  }

  UNSET_ONERROR(err2);
  UNSET_ONERROR(err1);
}

#define SETUP_DECODE_MEMOBJ(TYPE, U, VAR, ALLOCATE,SCOUR) do {		\
  struct svalue *tmpptr;						\
  struct svalue tmp;							\
  if(data->pass > 1 &&							\
     (tmpptr=low_mapping_lookup(data->decoded, & entry_id)))		\
  {									\
    tmp=*tmpptr;							\
    VAR=tmp.u.U;							\
    SCOUR;                                                              \
  }else{								\
    SET_SVAL(tmp, TYPE, 0, U, (VAR = ALLOCATE));			\
    mapping_insert(data->decoded, & entry_id, &tmp);			\
  /* Since a reference to the object is stored in the mapping, we can	\
   * safely decrease this reference here. Thus it will be automatically	\
   * freed if something goes wrong.					\
   */									\
    sub_ref(VAR);							\
  }									\
}while(0)

/* This really needs to disable threads.... */
#define decode_type(X,data)  do {		\
  type_stack_mark();				\
  low_decode_type(data);			\
  (X)=pop_unfinished_type();			\
} while(0)

static void cleanup_new_program_decode (void *compilation)
{
  struct compilation *c = compilation;
  debug_malloc_touch(Pike_compiler->new_program);
  debug_malloc_touch(Pike_compiler->new_program->parent);
  /* The program is consistent enough to be freed... */
  Pike_compiler->new_program->flags &= ~PROGRAM_AVOID_CHECK;
  unlink_current_supporter(& c->supporter);
  end_first_pass(0);
}

static void restore_current_file(void *save_current_file)
{
  struct compilation *c = THIS_COMPILATION;
  free_string(c->lex.current_file);
  c->lex.current_file = save_current_file;
}

/* Decode bytecode string @[string_no].
 * Returns resulting offset in p->program.
 */
static INT32 decode_portable_bytecode(struct decode_data *data, INT32 string_no)
{
  struct program *p = Pike_compiler->new_program;
  struct pike_string *bytecode;
  struct pike_string *current_file=NULL;
  INT_TYPE current_line = 0;
  int e;
  ONERROR err;

  debug_malloc_touch(p);
  if ((string_no < 0) || (string_no >= p->num_strings)) {
    decode_error(data, NULL,
		 "Bad bytecode string number: %d (expected 0 - %d).\n",
		 string_no, p->num_strings-1);
  }

  bytecode = p->strings[string_no];

  if (bytecode->len % 3) {
    decode_error(data, NULL, "Bad bytecode string length: "
		 "%td (expected multiple of 3).\n", bytecode->len);
  }

  init_bytecode();

  SET_ONERROR(err, exit_bytecode, NULL);

  switch(bytecode->size_shift) {
#define SIGNED_CHAR(X)	X

#if SIZEOF_INT_TYPE > 4
#define DO_HIGH(X) (X)
#else
#define DO_HIGH(X) 0
#endif

    /* The EMIT_BYTECODE2 macro will generate the warning
     * "comparison is always false due to limited range of data type"
     * if used on STR0. Thus, the need to have two macros here.
     */
#define EMIT_BYTECODE2(STR)				\
      if (STR(bytecode)[e] == F_FILENAME) {		\
	INT32 strno = STR(bytecode)[e+1];		\
	if (SIGNED_CHAR(strno < 0) ||			\
	    (strno >= p->num_strings)) {		\
	  decode_error(data, NULL, "Bad filename directive number:"	\
		       " %d (expected 0 - %d).\n",			\
		       strno, p->num_strings);				\
	}						\
	current_file = p->strings[strno];		\
      } else if (STR(bytecode)[e] == F_LINE) {		\
	current_line =					\
	  ((unsigned INT32)STR(bytecode)[e+1]) |	\
	  DO_HIGH(((INT_TYPE)STR(bytecode)[e+2])<<32);  \
      } else if (!current_file) {			\
	decode_error(data, NULL, "Missing filename directive in "	\
		     "byte code.\n");			\
      } else if (!current_line) {			\
	decode_error(data, NULL, "Missing line directive in "		\
		     "byte code.\n");			\
      } else

#define EMIT_BYTECODE(STR, X) do {		\
    for (e = 0; e < bytecode->len; e += 3) {	\
      X(STR)					\
      {						\
	insert_opcode2(STR(bytecode)[e],	\
		       STR(bytecode)[e+1],	\
		       STR(bytecode)[e+2],	\
		       current_line,		\
		       current_file);		\
      }						\
    }						\
  } while(0)

  case 2:
    EMIT_BYTECODE(STR2, EMIT_BYTECODE2);
    break;
#undef SIGNED_CHAR
#define SIGNED_CHAR(X) 0
  case 1:
    EMIT_BYTECODE(STR1, EMIT_BYTECODE2);
    break;
  case 0:
#undef EMIT_BYTECODE2
#define EMIT_BYTECODE2(X)				\
    if (!current_file) {				\
      decode_error(data, NULL, "Missing filename directive in "		\
		   "byte code.\n");			\
    } else if (!current_line) {				\
      decode_error(data, NULL, "Missing line directive in "		\
		   "byte code.\n");			\
    } else

    EMIT_BYTECODE(STR0, EMIT_BYTECODE2);
    break;
#undef SIGNED_CHAR
#undef EMIT_BYTECODE
#undef EMIT_BYTECODE2
  }
  UNSET_ONERROR(err);
  return assemble(1);
}

static void low_do_decode (struct decode_data *data);
static void free_decode_data (struct decode_data *data, int delay,
			      int DEBUGUSED(free_after_error));

static int call_delayed_decode(struct Supporter *s, int finish)
{
  JMP_BUF recovery;
  struct decode_data *data = s->data;
  struct compilation *cc = (struct compilation *)s;
  volatile int ok = 0;

  if (!data)
    return 0;

  debug_malloc_touch(cc);
  debug_malloc_touch(cc->p);

  if (finish) {
#ifdef ENCODE_DEBUG
    struct string_builder buf;
#endif
    struct svalue *osp = Pike_sp;
    struct compilation *prevcc = data->support_compilation;
    data->support_compilation = cc;
    SET_SVAL(data->counter, T_INT, NUMBER_NUMBER, integer, COUNTER_START);
    data->ptr=4; /* Skip 182 'k' 'e' '0' */
#ifdef ENCODE_DEBUG
    data->debug_ptr = 0;
    if (data->debug && !data->debug_buf) {
      data->debug_buf = &buf;
      init_string_builder(data->debug_buf, 0);
    }
    data->depth = -2;
#endif

    if (SETJMP(recovery)) {
      push_svalue (&throw_value);
      SAFE_APPLY_MASTER ("describe_error", 1);
      pop_stack();
      UNSETJMP(recovery);
      free_svalue(&throw_value);
      mark_free_svalue (&throw_value);
    } else {
      int e;
      struct keypair *k;

      while(data->unfinished_programs)
	{
	  struct unfinished_prog_link *tmp=data->unfinished_programs;
	  data->unfinished_programs=tmp->next;
	  free(tmp);
	}

      while(data->unfinished_objects)
	{
	  struct unfinished_obj_link *tmp=data->unfinished_objects;
	  data->unfinished_objects=tmp->next;
	  free_svalue(&tmp->decode_arg);
	  free_object(tmp->o);
	  free(tmp);
	}

      while(data->unfinished_placeholders)
	{
	  struct unfinished_obj_link *tmp=data->unfinished_placeholders;
	  data->unfinished_placeholders=tmp->next;
	  free_object(tmp->o);
	  free(tmp);
	}

      data->pass=1;

      low_do_decode (data);

      UNSETJMP(recovery);
      ok = 1;
    }
#ifdef ENCODE_DEBUG
    if (data->debug_buf == &buf) {
      write_and_reset_string_builder(2, data->debug_buf);
      free_string_builder(data->debug_buf);
      data->debug_buf = NULL;
    }
#endif
    data->support_compilation = prevcc;
    pop_n_elems(Pike_sp-osp);
  }

  if(cc->p) {
    free_program(cc->p); /* later */
    cc->p = NULL;
  }

#ifdef PIKE_DEBUG
  verify_supporters();
#endif

  return ok;
}

static void exit_delayed_decode(struct Supporter *s)
{
  if (s->data != NULL) {
    struct decode_data *data = s->data;
    --data->support_delay_counter;
    free_decode_data(data, 0, 0);
  }
}

static void decode_value2(struct decode_data *data)

#ifdef PIKE_DEBUG
#undef decode_value2
#define decode_value2(X) do { struct svalue *_=Pike_sp; decode_value2_(X); if(Pike_sp!=_+1) Pike_fatal("decode_value2 failed!\n"); } while(0)
#endif


{
  INT32 what, e;
  INT64 num;
  struct svalue entry_id, *tmp2;
  struct svalue *delayed_enc_val;

#ifdef ENCODE_DEBUG
  data->depth += 2;
#endif

  check_c_stack(1024);

  DECODE("decode_value2");

  switch(what & TAG_MASK)
  {
    case TAG_DELAYED:
      EDB (2, fprintf(stderr, "%*sDecoding delay encoded from <%ld>\n",
		      data->depth, "", (long)num););
      ETRACE(DECODE_WERR(".tag     delayed, %ld", (long)num));
      SET_SVAL(entry_id, T_INT, NUMBER_NUMBER, integer, num);
      if (!(delayed_enc_val = low_mapping_lookup (data->decoded, &entry_id)))
	decode_error (data, NULL, "Failed to find previous record of "
		      "delay encoded entry <%ld>.\n", (long)num);
      if (TYPEOF(*delayed_enc_val) != T_PROGRAM ||
	  delayed_enc_val->u.program->flags != PROGRAM_VIRGIN) {
	decode_error (data, NULL, "Didn't get program embryo "
		      "for delay encoded program <%pO>: %pO\n",
		      &entry_id, delayed_enc_val);
      }
      DECODE ("decode_value2");
      break;

    case TAG_AGAIN:
      EDB (1, fprintf(stderr, "%*sDecoding TAG_AGAIN from <%ld>\n",
		      data->depth, "", (long)num););
      ETRACE(DECODE_WERR(".tag     again, %ld", (long)num));
      SET_SVAL(entry_id, T_INT, NUMBER_NUMBER, integer, num);
      if((tmp2=low_mapping_lookup(data->decoded, &entry_id)))
      {
	push_svalue(tmp2);
      }else{
	decode_error(data, NULL, "Failed to decode TAG_AGAIN entry <%ld>.\n",
		     (long)num);
      }
      goto decode_done;

    default:
      entry_id = data->counter;
      if ((what & TAG_MASK) != TAG_TYPE) {
        data->counter.u.integer++;
      }
      EDB (2, fprintf(stderr, "%*sDecoding to <%ld>: TAG%d (%ld)\n",
		      data->depth, "", entry_id.u.integer,
		      what & TAG_MASK, (long)num););
      ETRACE({
	  ptrdiff_t save_ptr = data->ptr;
	  data->ptr = data->debug_ptr;
	  DECODE_WERR("# Decoding to tag #%ld", entry_id.u.integer);
	  data->ptr = save_ptr;
	});
      /* Types are added to the encoded mapping AFTER they have been
       * encoded. */
      delayed_enc_val = NULL;
      break;
  }

  check_stack(1);

  switch(what & TAG_MASK)
  {
    case TAG_INT:
      push_int(num);

      ETRACE({
	  DECODE_WERR(".integer %ld", (long)num);
	});
      break;

    case TAG_STRING:
    {
      struct pike_string *str;
      get_string_data(str, num, data);
      push_string(str);
      break;
    }

    case TAG_FLOAT:
    {
      double res;

      EDB(2,fprintf(stderr, "Decoding float... num:0x%016" PRINTINT64 "x\n",
		    num));

      res = (double)num;

      EDB(2,fprintf(stderr, "Mantissa: %10g\n", res));

      DECODE("float");

      EDB(2,fprintf(stderr, "Exponent: %ld\n", (long)num));

      if(!res)
      {
        switch(num)
	{
	  case Pike_FP_SNAN: /* Signal Not A Number */
            push_float((FLOAT_TYPE)MAKE_NAN());
            ETRACE({
		DECODE_WERR(".float   snan");
	      });
	    break;

	  case Pike_FP_QNAN: /* Quiet Not A Number */
            push_float((FLOAT_TYPE)MAKE_NAN());
            ETRACE({
		DECODE_WERR(".float   qnan");
	      });
	    break;

	  case Pike_FP_NINF: /* Negative infinity */
            push_float(-(FLOAT_TYPE)MAKE_INF());
            ETRACE({
		DECODE_WERR(".float   -inf");
	      });
	    break;

	  case Pike_FP_PINF: /* Positive infinity */
            push_float((FLOAT_TYPE)MAKE_INF());
            ETRACE({
		DECODE_WERR(".float   inf");
	      });
	    break;

	  case Pike_FP_NZERO: /* Negative Zero */
	    push_float(-0.0); /* Does this do what we want? */
            ETRACE({
		DECODE_WERR(".float   -0.0");
	      });
	    break;

	  default:
            push_float((FLOAT_TYPE)ldexp(res, num));
            ETRACE({
		DECODE_WERR(".float   %ld, %ld", (long)res, (long)num);
	      });
	    break;
	}
	break;
      }

      push_float((FLOAT_TYPE)ldexp(res, num));
      ETRACE({
	  DECODE_WERR(".float   %ld, %ld", (long)res, (long)num);
	});
      break;
    }

    case TAG_TYPE:
    {
      struct pike_type *t;

      if (!data->codec && data->explicit_codec)
        Pike_error(DECODING_NEEDS_CODEC_ERROR);

      ETRACE({
	  DECODE_WERR(".entry   type, %ld", (long)num);
	});

      decode_type(t, data);
      push_type_value(t);

      entry_id = data->counter;
      data->counter.u.integer++;
    }
    break;

    case TAG_ARRAY:
    {
      struct array *a;
      TYPE_FIELD types;
      if(num < 0)
	decode_error(data, NULL,
		     "Failed to decode array (array size is negative).\n");

      /* Heruetical */
      if(num > data->len - data->ptr)
	decode_error(data, NULL, "Failed to decode array (not enough data).\n");

      EDB(2,fprintf(stderr, "%*sDecoding array of size %ld to <%ld>\n",
		  data->depth, "", (long)num, entry_id.u.integer));
      ETRACE({
	  DECODE_WERR(".entry   array, %ld", (long)num);
	});

      SETUP_DECODE_MEMOBJ(T_ARRAY, array, a, allocate_array(num),
			  free_svalues(ITEM(a), a->size, a->type_field));

      types = 0;
      for(e=0;e<num;e++)
      {
	decode_value2(data);
	stack_pop_to_no_free (ITEM(a) + e);
	types |= 1 << TYPEOF(ITEM(a)[e]);
      }
      a->type_field = types;
      ref_push_array(a);
      goto decode_done;
    }

    case TAG_MAPPING:
    {
      struct mapping *m;
      if(num<0)
	decode_error(data, NULL, "Failed to decode mapping "
		     "(mapping size is negative).\n");

      /* Heuristical */
      if(num > data->len - data->ptr)
	decode_error(data, NULL, "Failed to decode mapping "
		     "(not enough data).\n");

      EDB(2,fprintf(stderr, "%*sDecoding mapping of size %ld to <%ld>\n",
		  data->depth, "", (long)num, entry_id.u.integer));

      ETRACE({
	  DECODE_WERR(".entry   mapping, %ld", (long)num);
	});

      SETUP_DECODE_MEMOBJ(T_MAPPING, mapping, m, allocate_mapping(num), ; );

      for(e=0;e<num;e++)
      {
	decode_value2(data);
	decode_value2(data);
	mapping_insert(m, Pike_sp-2, Pike_sp-1);
	pop_n_elems(2);
      }
      ref_push_mapping(m);
      goto decode_done;
    }

    case TAG_MULTISET:
    {
      struct multiset *m;
      struct array *a;
      TYPE_FIELD types;
      if(num<0)
	decode_error(data, NULL, "Failed to decode multiset "
		     "(multiset size is negative).\n");

      /* Heruetical */
      if(num > data->len - data->ptr)
	decode_error(data, NULL, "Failed to decode multiset "
		     "(not enough data).\n");

      /* NOTE: This code knows stuff about the implementation of multisets...*/

      EDB(2,fprintf(stderr, "%*sDecoding multiset of size %ld to <%ld>\n",
		  data->depth, "", (long)num, entry_id.u.integer));
      ETRACE({
	  DECODE_WERR(".entry   multiset, %ld", (long)num);
	});
      SETUP_DECODE_MEMOBJ (T_MULTISET, multiset, m,
			   allocate_multiset (0, 0, NULL), ;);
      /* FIXME: This array could be avoided by building the multiset directly. */
      push_array(a = low_allocate_array (num, 0));

      types = 0;
      for(e=0;e<num;e++)
      {
	decode_value2(data);
	stack_pop_to_no_free (ITEM(a) + e);
	types |= 1 << TYPEOF(ITEM(a)[e]);
      }
      a->type_field = types;
      {
	struct multiset *l = mkmultiset (a);
	pop_stack();
	/* This special case is handled efficiently by merge_multisets. */
	merge_multisets (m, l, PIKE_MERGE_DESTR_A | PIKE_ARRAY_OP_ADD);
	free_multiset (l);
      }
      ref_push_multiset(m);
      goto decode_done;
    }

    case TAG_OBJECT:
    {
      int subtype = 0;

      if (!data->codec && data->explicit_codec)
        Pike_error(DECODING_NEEDS_CODEC_ERROR);

      ETRACE({
	  if (!num) {
	    DECODE_WERR(".entry   object");
	  } else if (num == 2) {
	    DECODE_WERR(".bignum");
	  } else {
	    DECODE_WERR(".entry   object, %ld", (long)num);
	  }
	});

      if (num == 4) {
        decode_number(subtype, data, "inherit");
      }
      decode_value2(data);

      switch(num)
      {
	case 0:
	  apply(decoder_codec (data),"objectof", 1);
	  break;

	case 1:
	  {
	    int fun;
	    /* decode_value_clone_object does not call __INIT, so
	     * we want to do that ourselves...
	     */
	    struct object *o=decode_value_clone_object(Pike_sp-1);

	    if (!o) {
	      if (data->pickyness)
		decode_error (data, NULL,
			      "Failed to decode program for object. Got: %pO\n",
			      Pike_sp - 1);
	      EDB(1,fprintf(stderr, "%*sDecoded a failed object to <%ld>: ",
			    data->depth, "", entry_id.u.integer);
		  print_svalue(stderr, Pike_sp-1);
		  fputc('\n', stderr););
	      decode_value2(data);
	      pop_n_elems(2);
	      push_undefined();
	      break;
	    }

	    debug_malloc_touch(o);
	    pop_stack();
	    push_object(o);

	    if(o->prog)
	    {
	      if(o->prog->flags & PROGRAM_FINISHED)
	      {
		int lfun = FIND_LFUN(o->prog, LFUN___INIT);
		if (lfun >= 0) {
		  apply_low(o, lfun, 0);
		  pop_stack();
		}
		/* FIXME: Should call LFUN_CREATE here in <= 7.2
		 * compatibility mode. */
	      }else{
		struct unfinished_obj_link *ol=ALLOC_STRUCT(unfinished_obj_link);
		EDB(2,fprintf(stderr,
			      "%*sDecoded an unfinished object to <%ld>: ",
			      data->depth, "", entry_id.u.integer);
		print_svalue(stderr, Pike_sp-1);
		fputc('\n', stderr););
		add_ref(ol->o = o);
		ol->next=data->unfinished_objects;
		SET_SVAL(ol->decode_arg, PIKE_T_INT, NUMBER_UNDEFINED,
			 integer, 0);
		data->unfinished_objects=ol;
		decode_value2(data);
		assign_svalue(&ol->decode_arg, Pike_sp-1);
		pop_stack();
		break;
	      }
	    }

	    EDB(2,fprintf(stderr, "%*sDecoded an object to <%ld>: ",
			data->depth, "", entry_id.u.integer);
		print_svalue(stderr, Pike_sp-1);
		fputc('\n', stderr););

	    ref_push_object(o);
	    decode_value2(data);

	    fun = find_identifier("decode_object", decoder_codec (data)->prog);
	    if (fun < 0)
	      decode_error(data, Pike_sp - 1,
			   "Cannot decode objects without a "
			   "\"decode_object\" function in the codec.\n");
	    apply_low(data->codec,fun,2);
	    if ((TYPEOF(Pike_sp[-1]) == T_ARRAY) && o->prog &&
		((fun = FIND_LFUN(o->prog, LFUN_CREATE)) != -1)) {
	      /* Call lfun::create(@args). */
	      INT32 args;
	      Pike_sp--;
	      args = Pike_sp->u.array->size;
	      if (args) {
		/* Note: Eats reference */
		push_array_items(Pike_sp->u.array);
	      } else {
		free_array(Pike_sp->u.array);
	      }
	      apply_low(o, fun, args);
	    }
	    pop_stack();
	  }

	  break;

	case 2:
	{
	  check_stack(2);
	  /* 256 would be better, but then negative numbers
	   * doesn't work... /Hubbe
	   */
	  push_int(36);
	  convert_stack_top_with_base_to_bignum();
	  reduce_stack_top_bignum();
	  break;
	}

	case 3:
	  pop_stack();
	  decode_value2(data);
	  break;

        case 4:
	  /* Subtyped object. */
	  if ((TYPEOF(Pike_sp[-1]) != T_OBJECT) || SUBTYPEOF(Pike_sp[-1]) ||
	      !Pike_sp[-1].u.object->prog) {
	    decode_error(data, NULL, "Expected plain object. Got: %pO\n",
			 Pike_sp-1);
	  }
	  if ((subtype < 0) ||
	      (subtype >= Pike_sp[-1].u.object->prog->num_inherits)) {
	    decode_error(data, NULL,
			 "Invalid subtype for object: %d (max: %d). "
			 "Object: %pO\n",
			 subtype, Pike_sp[-1].u.object->prog->num_inherits,
			 Pike_sp-1);
	  }
	  SET_SVAL_SUBTYPE(Pike_sp[-1], subtype);
	  break;

	default:
	  decode_error(data, NULL, "Object coding not compatible: %ld\n", (long)num);
	  break;
      }

      if((TYPEOF(Pike_sp[-1]) != T_OBJECT) && data->pickyness) {
	if (num != 2) {
	  decode_error(data, NULL, "Failed to decode object. Got: %pO\n",
		       Pike_sp - 1);
	} else if (TYPEOF(Pike_sp[-1]) != PIKE_T_INT) {
	  decode_error(data, NULL, "Failed to decode bignum. Got: %pO\n",
		       Pike_sp - 1);
	}
      }

      break;
    }

    case TAG_FUNCTION:
      if (!data->codec && data->explicit_codec)
        Pike_error(DECODING_NEEDS_CODEC_ERROR);

      ETRACE({
	  if (num) {
	    DECODE_WERR(".entry   function, %ld", (long)num);
	  } else {
	    DECODE_WERR(".entry   function");
	  }
	});

      decode_value2(data);
      stack_dup();	/* For diagnostic purposes... */

      switch(num)
      {
	case 0:
	  apply(decoder_codec (data),"functionof", 1);
	  break;

	case 1: {
	  struct program *p;
	  if(TYPEOF(Pike_sp[-1]) != T_OBJECT && data->pickyness)
	    decode_error(data, NULL,
			 "Failed to decode function object. Got: %pO\n",
			 Pike_sp - 1);

	  decode_value2(data);
	  if(TYPEOF(Pike_sp[-1]) != T_STRING && data->pickyness)
	    decode_error(data, NULL,
			 "Failed to decode function identifier. Got: %pO\n",
			 Pike_sp - 1);

	  if (TYPEOF(Pike_sp[-2]) == T_OBJECT &&
	      TYPEOF(Pike_sp[-1]) == T_STRING &&
	      (p = Pike_sp[-2].u.object->prog)) {
	    int f = really_low_find_shared_string_identifier(
	      Pike_sp[-1].u.string,
	      p->inherits[SUBTYPEOF(Pike_sp[-2])].prog,
	      SEE_PROTECTED|SEE_PRIVATE);
	    debug_malloc_touch(p);
	    if (f >= 0) {
	      struct svalue func;
	      low_object_index_no_free(&func, Pike_sp[-2].u.object, f);
	      debug_malloc_touch(p);
	      pop_n_elems(2);
	      *Pike_sp++ = func;
	      dmalloc_touch_svalue(Pike_sp-1);
	      break;
	    }
	    else if (data->pickyness) {
	      debug_malloc_touch(p);
	      if (Pike_sp[-1].u.string->size_shift)
		decode_error(data, NULL, "Couldn't find identifier in %pO.\n",
			     Pike_sp - 2);
	      else
		decode_error(data, NULL, "Couldn't find identifier %s in %pO.\n",
			     Pike_sp[-1].u.string->str, Pike_sp - 2);
	    }
	    debug_malloc_touch(p);
	  }
	  pop_stack();
	  break;
	}

	default:
	  decode_error(data, NULL, "Function coding not compatible: %ld\n", (long)num);
	  break;
      }

      if((TYPEOF(Pike_sp[-1]) != T_FUNCTION) &&
	 (TYPEOF(Pike_sp[-1]) != T_PROGRAM) &&
	 data->pickyness)
	decode_error(data, Pike_sp - 2,
		     "Failed to decode function. Got: %pO\n", Pike_sp - 1);

      stack_pop_keep_top();

      break;


    case TAG_PROGRAM:
      if (!data->codec && data->explicit_codec)
        Pike_error(DECODING_NEEDS_CODEC_ERROR);
      EDB(3,
	  fprintf(stderr, "%*s  TAG_PROGRAM(%ld)\n",
		  data->depth, "", (long)num));
      switch(num)
      {
	case 0:
	{
	  struct program *p;

          ETRACE({
	      DECODE_WERR(".entry   program");
	    });

	  decode_value2(data);
	  apply(decoder_codec (data),"programof", 1);

	  p = program_from_svalue(Pike_sp-1);

	  if (!p) {
	    if(data->pickyness)
	      decode_error(data, NULL, "Failed to decode program. Got: %pO\n",
			   Pike_sp - 1);
	    pop_stack();
	    push_undefined();
	    break;
	  }

	  if ((p->flags & PROGRAM_NEEDS_PARENT)) {
	    EDB(2, fprintf(stderr, "%*sKeeping %s to keep parent pointer.\n",
			   data->depth, "",
			   get_name_of_type(TYPEOF(Pike_sp[-1]))));
	    break;
	  }

	  add_ref(p);
	  pop_stack();
	  push_program(p);
	  break;
	}

	case 1:			/* Old-style encoding. */
	{
	  decode_error(data, NULL, "Failed to decode program. Old-style program encoding is not supported, anymore.\n");
	}

	case 2:
          ETRACE({
	      DECODE_WERR(".entry   program, 2");
	    });

	  decode_value2(data);
	  decode_value2(data);
	  if(TYPEOF(Pike_sp[-2]) == T_INT)
	  {
	    pop_stack();
	  }else{
	    f_arrow(2);
	  }
	  if(TYPEOF(Pike_sp[-1]) != T_PROGRAM && data->pickyness)
	    decode_error(data, NULL, "Failed to decode program. Got: %pO\n",
			 Pike_sp - 1);
	  break;

        case 3:
          ETRACE({
	      DECODE_WERR(".entry   program, 3");
	    });

	  decode_value2(data);
	  if ((TYPEOF(Pike_sp[-1]) == T_INT) &&
	      (Pike_sp[-1].u.integer < PROG_DYNAMIC_ID_START) &&
	      (Pike_sp[-1].u.integer > 0)) {
	    struct program *p = id_to_program(Pike_sp[-1].u.integer);
	    if (!p) {
	      decode_error(data, NULL, "Failed to get program from ID %pO.\n",
			   Pike_sp - 1);
	    }
	    pop_stack();
	    ref_push_program(p);
	  } else {
	    decode_error(data, NULL, "Failed to decode program by ID. "
			 "Expected integer, got: %pO\n", Pike_sp - 1);
	  }
	  break;

	case 5: {		/* Forward reference for new-style encoding. */
	  struct program *p = low_allocate_program();
          ETRACE({
	      DECODE_WERR(".entry   program, 5");
	    });

	  push_program (p);
	  EDB(2,
	      fprintf (stderr, "%*sInited an embryo for a delay encoded program "
		       "to <%ld>: ",
		       data->depth, "", entry_id.u.integer);
	      print_svalue (stderr, Pike_sp - 1);
	      fputc ('\n', stderr););

	  data->delay_counter++;

	  break;
	}

	case 4:			/* New-style encoding. */
	{
	  struct program *p;
	  ONERROR err;
	  ONERROR err2;
	  int byteorder;
	  int bytecode_method;
	  int entry_type;
	  unsigned INT16 id_flags;
	  INT16 p_flags;
	  ptrdiff_t old_pragmas;
	  struct compilation *c;
	  struct pike_string *save_current_file;
	  struct object *placeholder = NULL;
	  INT_TYPE save_current_line;
          int delay;
#define FOO(NUMTYPE,Y,ARGTYPE,NAME) \
          NUMTYPE PIKE_CONCAT(local_num_, NAME) = 0;
#include "program_areas.h"

#ifdef ENCODE_DEBUG
	  data->depth += 2;
#endif
          ETRACE({
	      DECODE_WERR(".entry   program, 4");
	    });

	  /* Decode byte-order. */
          decode_number(byteorder, data, "byte-order");

	  EDB(4,
	      fprintf(stderr, "%*sbyte order:%d\n",
		      data->depth, "", byteorder));

	  if ((byteorder != PIKE_BYTEORDER)
#if (PIKE_BYTEORDER == 1234)
	      && (byteorder != 4321)
#else
#if (PIKE_BYTEORDER == 4321)
	      && (byteorder != 1234)
#endif
#endif
	      ) {
	    decode_error (data, NULL, "Unsupported byte-order. "
			  "Native:%d Encoded:%d\n", PIKE_BYTEORDER, byteorder);
	  }

	  /* Decode flags. */
          decode_number(p_flags, data, "flags");
	  p_flags &= ~(PROGRAM_FINISHED | PROGRAM_OPTIMIZED |
		       PROGRAM_FIXED | PROGRAM_PASS_1_DONE);
	  p_flags |= PROGRAM_AVOID_CHECK;

	  if (delayed_enc_val) {
	    EDB(2,fprintf(stderr, "%*sdecoding a delay encoded program: ",
			  data->depth, "");
		print_svalue(stderr, delayed_enc_val);
		fputc('\n', stderr););
	    /* No new ref here; low_start_new_program will add one for
	     * Pike_compiler->new_program and we want ride on that one
	     * just like when it's created there. */
	    p = delayed_enc_val->u.program;
	    debug_malloc_touch(p);
	  }
	  else if (data->support_compilation) {
	    struct svalue *val = low_mapping_lookup (data->decoded, &entry_id);
	    if (val == NULL)
	      p = NULL;
	    else {
	      if (TYPEOF(*val) != T_PROGRAM ||
		  (val->u.program->flags &
		   (PROGRAM_OPTIMIZED | PROGRAM_FIXED |
		    PROGRAM_FINISHED | PROGRAM_PASS_1_DONE)) !=
		  PROGRAM_PASS_1_DONE) {
		decode_error (data, NULL, "Didn't get pass 1 program "
			      "for supporter delay encoded program <%pO>: %pO\n",
			      &entry_id, val);
	      }
	      p = val->u.program;
	      if (p != data->support_compilation->supporter.prog)
		p = NULL;
	      else {
		debug_malloc_touch(p);
		/* low_start_new_program will restore the program to
		   a pristine state as long as p->program is NULL,
		   so free it here */
		if (p->program) {
#ifdef PIKE_USE_MACHINE_CODE
		  mexec_free(p->program);
#else
		  free(p->program);
#endif
		  p->program = NULL;
		  p->num_program = 0;
		}
	      }
	    }
	  } else
	    p = NULL;

	  enter_compiler(NULL, 0);

	  c = THIS_COMPILATION;

	  /* We don't want to be affected by #pragma save_parent or
	   * __pragma_save_parent__.
	   */
	  old_pragmas = c->lex.pragmas;
	  c->lex.pragmas = (old_pragmas & ~ID_SAVE_PARENT)|ID_DONT_SAVE_PARENT;
	  /* We also don't want to generate deprecation warnings. */
	  c->lex.pragmas |= ID_NO_DEPRECATION_WARNINGS;

	  /* Start the new program. */
	  low_start_new_program(p, COMPILER_PASS_FIRST, NULL, 0, NULL);
	  p = Pike_compiler->new_program;
	  p->flags = p_flags;

	  /* Kludge to get end_first_pass() to free the program. */
	  Pike_compiler->num_parse_error++;

	  init_supporter(& c->supporter,
			 (supporter_callback *) call_delayed_decode, NULL);
	  c->supporter.exit_fun = exit_delayed_decode;
	  c->supporter.prog = p;
	  SET_ONERROR(err, cleanup_new_program_decode, c);

	  if (data->support_compilation &&
	      data->support_compilation->supporter.prog == p &&
	      data->support_compilation->placeholder != NULL)
	    add_ref(c->placeholder = data->support_compilation->placeholder);
	  else
	  {
	    int fun = find_identifier("__register_new_program",
				      decoder_codec (data)->prog);

	    if (fun >= 0) {
	      ref_push_program(p);
	      apply_low(data->codec, fun, 1);

	      /* Returned a placeholder */
	      if(TYPEOF(Pike_sp[-1]) == T_OBJECT)
	      {
		add_ref(c->placeholder=Pike_sp[-1].u.object);
		if(c->placeholder->prog != null_program) {
		  decode_error(data, NULL, "Placeholder object is not "
			       "a __null_program clone.\n");
		}
	      } else if (TYPEOF(Pike_sp[-1]) != T_INT ||
			 Pike_sp[-1].u.integer) {
		decode_error (data, NULL, "Expected placeholder object or zero "
			      "from __register_new_program.\n");
	      }
	      pop_stack();
	    }
	  }

	  copy_shared_string(save_current_file, c->lex.current_file);
	  save_current_line = c->lex.current_line;

	  SET_ONERROR(err2, restore_current_file, save_current_file);

	  if (!delayed_enc_val) {
	    struct svalue prog;
	    SET_SVAL(prog, T_PROGRAM, 0, program, p);
	    EDB(2,fprintf(stderr, "%*sDecoding a program to <%ld>: ",
			  data->depth, "", entry_id.u.integer);
		print_svalue(stderr, &prog);
		fputc('\n', stderr););
	    mapping_insert(data->decoded, &entry_id, &prog);
	    debug_malloc_touch(p);
	  } else {
	    data->delay_counter--;
	  }

	  debug_malloc_touch(p);

	  /* Check the version. */
          ETRACE({
	      DECODE_WERR("# version");
	    });
	  decode_value2(data);
	  push_compact_version();
	  if(!is_eq(Pike_sp-1,Pike_sp-2)
#ifdef ENCODE_DEBUG
	     && !data->debug
#endif
	    )
	    decode_error(data, NULL, "Cannot decode programs encoded with "
			 "other pike version %pO.\n", Pike_sp - 2);
	  pop_n_elems(2);

	  debug_malloc_touch(p);

#ifdef ENCODE_DEBUG
	  if (!data->debug)
#endif
	    data->pickyness++;

	  /* parent */
          ETRACE({
	      DECODE_WERR("# parent");
	    });
	  decode_value2(data);
	  if (TYPEOF(Pike_sp[-1]) == T_PROGRAM) {
	    p->parent = Pike_sp[-1].u.program;
	    debug_malloc_touch(p->parent);
	  } else if ((TYPEOF(Pike_sp[-1]) == T_INT) &&
		     (!Pike_sp[-1].u.integer)) {
	    p->parent = NULL;
	  } else {
	    decode_error (data, NULL, "Bad type for parent program (%s).\n",
			  get_name_of_type(TYPEOF(Pike_sp[-1])));
	  }
	  dmalloc_touch_svalue(Pike_sp-1);
	  Pike_sp--;

	  /* Decode lengths. */
#define FOO(NUMTYPE,TYPE,ARGTYPE,NAME) do {				\
            decode_number(PIKE_CONCAT(local_num_, NAME), data,          \
                          TOSTR(NAME));                                 \
	  } while(0);
#include "program_areas.h"

	  /* Byte-code method */
          decode_number(bytecode_method, data, "byte-code");
	  if (bytecode_method != PIKE_BYTECODE_PORTABLE) {
	    decode_error(data, NULL, "Unsupported byte-code method: %d\n",
			 bytecode_method);
	  }

	  /* Decode strings */
	  for (e=0; e<local_num_strings; e++) {
	    decode_value2(data);
	    if (TYPEOF(Pike_sp[-1]) != T_STRING) {
	      ref_push_program (p);
	      decode_error(data, Pike_sp - 1,
			   "Nonstrings in string table: %pO\n", Pike_sp - 2);
	    }
	    add_to_strings(Pike_sp[-1].u.string);
	    dmalloc_touch_svalue(Pike_sp-1);
	    Pike_sp--;
	  }

	  /* First pass constants.
	   *
	   * These will be replaced later on.
	   */
	  {
	    struct program_constant constant;
	    SET_SVAL(constant.sval, T_INT, NUMBER_UNDEFINED, integer, 0);
	    constant.offset = -1;

	    for(e=0;e<local_num_constants;e++) {
	      add_to_constants(constant);
	    }
	  }

          ETRACE({
	      DECODE_WERR("# Constant table.");
	    });
	  /* Decode identifier_references, inherits and identifiers. */
          decode_number(entry_type, data, NULL);
	  EDB(4,
	      fprintf(stderr, "%*sDecoding identifier references.\n",
		      data->depth, ""));
#ifdef ENCODE_DEBUG
	  data->depth+=2;
#endif
	  while ((entry_type == ID_ENTRY_EFUN_CONSTANT) ||
		 (entry_type == ID_ENTRY_TYPE_CONSTANT)) {
	    INT32 efun_no;
	    struct program_constant *constant;
            ETRACE({
		if (entry_type == ID_ENTRY_EFUN_CONSTANT) {
		  DECODE_WERR(".ident   efun_constant");
		} else {
		  DECODE_WERR(".ident   type_constant");
		}
	      });
            decode_number(efun_no, data, "constant #");
	    EDB(2,
		fprintf(stderr, "%*sDecoding efun/type constant #%d.\n",
			data->depth, "", efun_no));
	    if ((efun_no < 0) || (efun_no >= local_num_constants)) {
	      ref_push_program (p);
	      decode_error(data, Pike_sp - 1,
			   "Bad efun/type number: %d (expected 0 - %d).\n",
			   efun_no, local_num_constants-1);
	    }
	    constant = p->constants+efun_no;
	    /* value */
	    decode_value2(data);
	    switch(entry_type) {
	    case ID_ENTRY_EFUN_CONSTANT:
	      if (((TYPEOF(Pike_sp[-1]) != T_FUNCTION) ||
		   (SUBTYPEOF(Pike_sp[-1]) != FUNCTION_BUILTIN)) &&
		  data->pickyness) {
		ref_push_program (p);
		decode_error(data, Pike_sp - 1,
			     "Expected efun constant: %pO\n", Pike_sp - 2);
	      }
	      break;
	    case ID_ENTRY_TYPE_CONSTANT:
	      if (TYPEOF(Pike_sp[-1]) != T_TYPE && data->pickyness) {
		ref_push_program (p);
		decode_error(data, Pike_sp - 1,
			     "Expected type constant: %pO\n", Pike_sp - 2);
	      }
	      break;
	    default:
	      if (data->pickyness)
		decode_error(data, NULL, "Internal error: "
			     "Unsupported early constant (%d).\n",
			     entry_type);
	      break;
	    }
	    /* name */
	    decode_value2(data);
	    constant->offset = -1;
	    pop_stack();
	    constant->sval = Pike_sp[-1];
	    dmalloc_touch_svalue(Pike_sp-1);
	    Pike_sp -= 1;
            decode_number(entry_type, data, NULL);
	  }

	  while (entry_type != ID_ENTRY_EOT) {
            ETRACE({
		switch(entry_type) {
		case ID_ENTRY_TYPE_CONSTANT:
		  DECODE_WERR(".ident   type_constant");
		  break;
		case ID_ENTRY_EFUN_CONSTANT:
		  DECODE_WERR(".ident   efun_constant");
		  break;
		case ID_ENTRY_RAW:
		  DECODE_WERR(".ident   raw");
		  break;
		case ID_ENTRY_EOT:
		  /* NOT reached. Here for completion. */
		  DECODE_WERR(".ident   eot");
		  break;
		case ID_ENTRY_VARIABLE:
		  DECODE_WERR(".ident   variable");
		  break;
		case ID_ENTRY_FUNCTION:
		  DECODE_WERR(".ident   function");
		  break;
		case ID_ENTRY_CONSTANT:
		  DECODE_WERR(".ident   constant");
		  break;
		case ID_ENTRY_INHERIT:
		  DECODE_WERR(".ident   inherit");
		  break;
		case ID_ENTRY_ALIAS:
		  DECODE_WERR(".ident   alias");
		  break;
		default:
		  DECODE_WERR_COMMENT("Unknown", ".ident   %d", entry_type);
		  break;
		}
	      });
            decode_number(id_flags, data, "modifiers");
	    if ((entry_type != ID_ENTRY_RAW) &&
		(entry_type != ID_ENTRY_INHERIT)) {
	      /* Common identifier fields. */

	      unsigned INT32 filename_strno;

	      /* name */
	      decode_value2(data);
	      if (TYPEOF(Pike_sp[-1]) != T_STRING) {
		ref_push_program (p);
		decode_error(data, Pike_sp - 1,
			     "Bad identifier name (not a string): %pO\n",
			     Pike_sp - 2);
	      }

	      /* type */
	      decode_value2(data);
	      if (TYPEOF(Pike_sp[-1]) != T_TYPE) {
		ref_push_program (p);
		decode_error(data, Pike_sp - 1,
			     "Bad identifier type (not a type): %pO\n",
			     Pike_sp - 2);
	      }

	      /* filename */
              decode_number(filename_strno, data, "filename_strno");
	      if (filename_strno >= p->num_strings) {
		ref_push_program(p);
		decode_error(data, NULL,
			     "String number out of range: %u >= %u",
			     filename_strno, p->num_strings);
	      }
	      free_string(c->lex.current_file);
	      copy_shared_string(c->lex.current_file,
				 p->strings[filename_strno]);

	      /* linenumber */
              decode_number(c->lex.current_line, data, "linenumber");

	      /* Identifier name and type on the pike stack.
	       * Definition location in c->lex.
	       */
	    }
	    switch(entry_type) {
	    case ID_ENTRY_RAW:
	      {
		int no;
		int ref_no;
		struct reference ref;

		/* id_flags */
		ref.id_flags = id_flags;

		/* inherit_offset */
                decode_number(ref.inherit_offset, data, "inherit_offset");

		/* identifier_offset */
		/* Actually the id ref number from the inherited program */
                decode_number(ref_no, data, "ref_no");

                if (ref.inherit_offset >= p->num_inherits)
                    decode_error(data, NULL, "Inherit offset out of range %u vs %u.\n",
                                 ref.inherit_offset, p->num_inherits);
                if (ref_no < 0 || ref_no >= p->inherits[ref.inherit_offset].prog->num_identifier_references)
                    decode_error(data, NULL, "Identifier reference out of range %u vs %u.\n",
                    ref_no, p->inherits[ref.inherit_offset].prog->num_identifier_references);

		ref.identifier_offset = p->inherits[ref.inherit_offset].prog->
		  identifier_references[ref_no].identifier_offset;

		ref.run_time_type = PIKE_T_UNKNOWN;
		ref.func.offset = 0;

		/* Expected identifier reference number */
                decode_number(no, data, "ref_no");

		if (no < 0 || no > p->num_identifier_references) {
		  EDB (3, dump_program_tables (p, data->depth));
		  ref_push_program (p);
		  decode_error(data, Pike_sp - 1,
			       "Bad identifier reference offset: %d != %d\n",
			       no,
			       Pike_compiler->new_program->
			       num_identifier_references);
		} else if (no == p->num_identifier_references) {
		  add_to_identifier_references(ref);
		}
		else {
		  p->identifier_references[no] = ref;
		}
	      }
	      break;

	    case ID_ENTRY_VARIABLE:
	      {
		int no, n;

		/* Expected identifier offset */
                decode_number(no, data, "ref_no");

		EDB(5,
		    fprintf(stderr,
			    "%*sdefine_variable(\"%s\", X, 0x%04x)\n",
			    data->depth, "",
			    Pike_sp[-2].u.string->str, id_flags));

		/* Alters
		 *
		 * storage, variable_index, identifiers and
		 * identifier_references
		 */
		n = define_variable(Pike_sp[-2].u.string,
				    Pike_sp[-1].u.type,
				    id_flags);
		if (no != n) {
		  ref_push_program (p);
		  decode_error(data, Pike_sp - 1,
			       "Bad variable identifier offset "
			       "(got %d, expected %d).\n", n, no);
		}

		pop_n_elems(2);
	      }
	      break;
	    case ID_ENTRY_FUNCTION:
	      {
		union idptr func;
		unsigned INT8 func_flags;
		unsigned INT16 opt_flags;
		int no;
		int n;

		/* func_flags (aka identifier_flags) */
                decode_number(func_flags, data, "identifier_flags");

		/* func */
                decode_number(func.offset, data, "byte_code_strno");
		if (func.offset != -1) {
#ifdef ENCODE_DEBUG
		  int old_a_flag = a_flag;
#endif
		  EDB(2,
		  {
		    fprintf(stderr, "%*sDecoding portable bytecode.\n",
			    data->depth, "");
		    a_flag = (a_flag > (data->debug-1))?a_flag:(data->debug-1);
		  });
		  func.offset = decode_portable_bytecode(data, func.offset);
		  EDB(2, a_flag = old_a_flag);
		}

		/* opt_flags */
                decode_number(opt_flags, data, "opt_flags");

		/* FIXME:
		 *   Verify validity of func_flags, func.offset & opt_flags
		 */

		/* Expected identifier offset */
                decode_number(no, data, "ref_no");

		EDB(5, {
		  INT_TYPE line;
		  struct pike_string *file =
		    get_line(func.offset + p->program, p, &line);
		  fprintf(stderr,
			  "%*sdefine_function(\"%s\", X, 0x%04x, 0x%04x,\n"
			  "%*s                0x%04tx, 0x%04x)\n"
			  "%*s    @ %s:%ld\n",
			  data->depth, "",
			  Pike_sp[-2].u.string->str, id_flags, func_flags,
			  data->depth, "",
			  func.offset, opt_flags,
			  data->depth, "",
			  file->str, (long)line);
		});

		/* Alters
		 *
		 * identifiers, identifier_references
		 */
		n = define_function(Pike_sp[-2].u.string,
				    Pike_sp[-1].u.type,
				    id_flags, func_flags,
				    &func, opt_flags);
		if ((no < 0 || no >= p->num_identifier_references) ||
		    (no != n &&
		     (p->identifier_references[no].id_flags != id_flags ||
		      p->identifier_references[no].identifier_offset !=
		      p->identifier_references[n].identifier_offset ||
		      p->identifier_references[no].inherit_offset != 0))) {
#ifdef PIKE_DEBUG
		  dump_program_tables(Pike_compiler->new_program, 0);
#endif
		  ref_push_program (p);
		  decode_error(data, Pike_sp - 1,
			       "Bad function identifier offset for %pS:%pT: %d != %d\n",
			       Pike_sp[-3].u.string, Pike_sp[-2].u.type, n, no);
		}

		pop_n_elems(2);
	      }
	      break;
	    case ID_ENTRY_CONSTANT:
	      {
		struct identifier id;
		struct reference ref;
		int no;
		int n;

		id.filename_strno = store_prog_string(c->lex.current_file);
		id.linenumber = c->lex.current_line;

		id.name = Pike_sp[-2].u.string;
		id.type = Pike_sp[-1].u.type;

		/* identifier_flags */
		id.identifier_flags = IDENTIFIER_CONSTANT;

		/* offset */
                decode_number(id.func.const_info.offset, data, "offset");

		/* FIXME:
		 *   Verify validity of func.const_info.offset
		 */

		/* run_time_type */
                decode_number(id.run_time_type, data, "run_time_type");

		/* opt_flags */
                decode_number(id.opt_flags, data, "opt_flags");

		ref.run_time_type = PIKE_T_UNKNOWN;
		ref.func.offset = 0;

		/* Expected identifier number. */
                decode_number(no, data, "ref_no");

		n = isidentifier(id.name);

#ifdef PROFILING
		id.self_time=0;
		id.num_calls=0;
		id.recur_depth=0;
		id.total_time=0;
#endif

		/* id_flags */
		ref.id_flags = id_flags;

		EDB(5,
		    fprintf(stderr,
			    "%*sdefining constant(\"%s\", X, 0x%04x)\n",
			    data->depth, "",
			    Pike_sp[-2].u.string->str, id_flags));

		/* identifier_offset */
		ref.identifier_offset =
		  Pike_compiler->new_program->num_identifiers;
		add_to_identifiers(id);

		/* References now held by the new program identifier. */
		dmalloc_touch_svalue(Pike_sp-1);
		dmalloc_touch_svalue(Pike_sp-2);
		Pike_sp -= 2;

		/* ref.inherit_offset */
		ref.inherit_offset = 0;

		/* Alters
		 *
		 * identifiers, identifier_references
		 */

		if (n < 0 || (n = override_identifier (&ref, id.name, 0)) < 0) {
		  n = p->num_identifier_references;
		  add_to_identifier_references(ref);
		}

		if (no != n) {
		  ref_push_program (p);
		  decode_error(data, Pike_sp - 1,
			       "Bad function identifier offset "
			       "(expected %d, got %d) for %pS.\n",
			       no, n, id.name);
		}

	      }
	      break;
	    case ID_ENTRY_ALIAS:
	      {
		int depth;
		int refno;
		int no;
		int n;

		/* depth */
                decode_number(depth, data, "depth");

		/* refno */
                decode_number(refno, data, "refno");

		/* FIXME:
		 *   Verify validity of depth and refno.
		 */

		/* Expected identifier number. */
                decode_number(no, data, "ref_no");

		EDB(5,
		    fprintf(stderr,
			    "%*slow_define_alias(\"%s\", X, 0x%04x)\n",
			    data->depth, "",
			    Pike_sp[-2].u.string->str, id_flags));

		/* Alters
		 *
		 * variable_index, identifiers and
		 * identifier_references
		 */
		n = low_define_alias(Pike_sp[-2].u.string,
				     Pike_sp[-1].u.type, id_flags,
				     depth, refno);

		if (no != n) {
		  ref_push_program (p);
		  decode_error(data, Pike_sp - 1,
			       "Bad alias identifier offset "
			       "(expected %d, got %d) for %pO.\n",
			       no, n, Pike_sp - 3);
		}

		pop_n_elems(2);
	      }
	      break;
	    case ID_ENTRY_INHERIT:
	      {
		struct program *prog;
		struct object *parent = NULL;
		int parent_identifier;
		int parent_offset;
		struct pike_string *name = NULL;
		int no;
		int save_compiler_flags;

                decode_number(no, data, "ref_no");
		if (no !=
		    Pike_compiler->new_program->num_identifier_references) {
		  ref_push_program (p);
		  decode_error(data, Pike_sp - 1,
			       "Bad inherit identifier offset: %d\n", no);
		}

		/* name */
		decode_value2(data);
		if (TYPEOF(Pike_sp[-1]) == T_STRING) {
		  name = Pike_sp[-1].u.string;
		} else if ((TYPEOF(Pike_sp[-1]) != T_INT) ||
			   Pike_sp[-1].u.integer) {
		  ref_push_program (p);
		  decode_error(data, Pike_sp - 1,
			       "Bad inherit name (not a string): %pO\n",
			       Pike_sp - 2);
		}

		/* prog */
		SET_FORCE_RESOLVE(save_compiler_flags);
		decode_value2(data);
		UNSET_FORCE_RESOLVE(save_compiler_flags);
		if (!(prog = program_from_svalue(Pike_sp-1))) {
		  ref_push_program (p);
		  decode_error(data, Pike_sp - 1,
			       "Bad inherit: Expected program, got: %pO\n",
			       Pike_sp - 2);
		}
		if (prog == placeholder_program) {
		  ref_push_program (p);
		  decode_error (data, Pike_sp - 1,
				"Trying to inherit placeholder program "
				"(resolver or codec problem).\n");
		}
		if(!(prog->flags & (PROGRAM_FINISHED | PROGRAM_PASS_1_DONE))) {
		  ref_push_program (p);
		  decode_error (data, Pike_sp - 1,
				"Cannot inherit a program which is not "
				"fully compiled yet (resolver or codec "
				"problem): %pO\n", Pike_sp - 2);
		}

		/* parent */
		decode_value2(data);
		if (TYPEOF(Pike_sp[-1]) == T_OBJECT) {
		  parent = Pike_sp[-1].u.object;
		} else if ((TYPEOF(Pike_sp[-1]) != T_INT) ||
			   Pike_sp[-1].u.integer) {
		  ref_push_program (p);
		  decode_error(data, Pike_sp - 1,
			       "Bad inherit: Parent isn't an object: %pO\n",
			       Pike_sp - 2);
		}

		/* parent_identifier */
                decode_number(parent_identifier, data, "parent_id");

		/* parent_offset */
                decode_number(parent_offset, data, "parent_offset");

		/* Expected number of identifier references. */
                decode_number(no, data, "ref_no");

		if (prog->num_identifier_references != no) {
		  ref_push_program (p);
		  decode_error(data, Pike_sp - 1, "Bad number of identifiers "
			       "in inherit (%d != %d).\n",
			       no, prog->num_identifier_references);
		}

		EDB(5,
		    fprintf(stderr,
			    "%*slower_inherit(..., \"%s\")\n",
			    data->depth, "",
			    name?name->str:"NULL"));

		/* Alters
		 *
		 * storage, inherits and identifier_references
		 */
		lower_inherit(prog, parent, parent_identifier,
			      parent_offset + 42, id_flags, name);

		pop_n_elems(3);
	      }
	      break;
	    default:
	      decode_error(data, NULL, "Unsupported id entry type: %d\n",
			   entry_type);
	    }
            ETRACE({
		DECODE_FLUSH();
	      });

            decode_number(entry_type, data, NULL);
	  }
          ETRACE({
	      DECODE_WERR(".ident   eot");
	    });

	  /* Restore c->lex. */
	  CALL_AND_UNSET_ONERROR(err2);
	  c->lex.current_line = save_current_line;

#ifdef ENCODE_DEBUG
	  data->depth-=2;
#endif

          delay = unlink_current_supporter(& c->supporter);

	  if (delay) {
	    data->support_delay_counter++;
	    c->supporter.data = data;
	  }

          UNSET_ONERROR(err);

	  /* De-kludge to get end_first_pass() to free the program. */
	  Pike_compiler->num_parse_error--;

	  p->flags |= PROGRAM_PASS_1_DONE;

	  /* Fixate & optimize
	   *
	   * lfuns and identifier_index
	   */
	  ref_push_program (p);
	  if (delay && (p->flags & PROGRAM_USES_PARENT)) {
	    /* Kludge for end_first_pass(0) not allocating
	     * parent_info_storage (intentionally).
	     */
	    p->parent_info_storage =
	      add_xstorage(sizeof(struct parent_info),
			   ALIGNOF(struct parent_info),
			   0);
	  }

	  if (!(p = end_first_pass(delay? 0 : 2)) ||
	      !call_dependants(&c->supporter, !!p)) {
	    decode_error(data, Pike_sp - 1, "Failed to decode program.\n");
	  }

	  pop_stack();
	  push_program(p);

	  if (c->placeholder && !delay) {
	    push_object(placeholder = c->placeholder);
	    c->placeholder = NULL;
	  }

	  exit_compiler();

	  EDB(5, dump_program_tables(p, data->depth));
#ifdef PIKE_DEBUG
	  check_program (p);
#endif

          /* We've regenerated p->program, so these may be off. */
          local_num_program = p->num_program;
          local_num_relocations = p->num_relocations;
          local_num_linenumbers = p->num_linenumbers;

	  /* Verify... */
#define FOO(NUMTYPE,TYPE,ARGTYPE,NAME)					\
	  if (PIKE_CONCAT(local_num_, NAME) != p->PIKE_CONCAT(num_,NAME)) { \
	    ref_push_program (p);					\
	    decode_error(data, Pike_sp - 1,				\
			 "Value mismatch for num_" TOSTR(NAME) ": "	\
			 "%zd != %zd (bytecode method: %d)\n",		\
			 (size_t) PIKE_CONCAT(local_num_, NAME),	\
			 (size_t) p->PIKE_CONCAT(num_, NAME),		\
			 bytecode_method);				\
          }
	  if (!delay) {
#include "program_areas.h"
	  }

	  if (placeholder) {
            struct unfinished_obj_link *up;

	    if (placeholder->prog != null_program) {
	      decode_error(data, NULL,
			   "Placeholder has been zapped during decoding.\n");
	    }
	    debug_malloc_touch(placeholder);
	    free_program(placeholder->prog);
	    add_ref(placeholder->prog = p);
	    placeholder->storage = p->storage_needed ?
	      (char *)xcalloc(p->storage_needed, 1) :
	      (char *)NULL;
	    call_c_initializers(placeholder);
	    /* It's not safe to call __INIT() or create() yet */

            up = ALLOC_STRUCT(unfinished_obj_link);
	    up->next = data->unfinished_placeholders;
	    data->unfinished_placeholders = up;
	    add_ref(up->o = placeholder);
	    pop_stack();
	  }

	  /* Decode the actual constants
	   *
	   * This must be done after the program has been ended.
	   */
	  for (e=0; e<local_num_constants; e++) {
	    struct program_constant *constant = p->constants+e;
	    if ((TYPEOF(constant->sval) != T_INT) ||
		(SUBTYPEOF(constant->sval) != NUMBER_UNDEFINED)) {
	      /* Already initialized. */
	      EDB(5,
		  fprintf(stderr, "%*sskipping constant %d\n",
			  data->depth, "", e));
	      continue;
	    }
	    /* value */
	    decode_value2(data);
	    /* name */
	    decode_value2(data);
	    constant->offset = -1;
	    pop_stack();
	    constant->sval = Pike_sp[-1];
	    dmalloc_touch_svalue(Pike_sp-1);
	    Pike_sp -= 1;
	    EDB(5,
		fprintf(stderr, "%*sDecoded constant %d to a %s\n",
			data->depth, "",
			e, get_name_of_type(TYPEOF(constant->sval))));
	  }

#ifdef ENCODE_DEBUG
	  if (!data->debug)
#endif
	    data->pickyness--;

	  /* The program should be consistent now. */
	  p->flags &= ~PROGRAM_AVOID_CHECK;

	  EDB(5, fprintf(stderr, "%*sProgram flags: 0x%04x\n",
			 data->depth, "", p->flags));

	  if (!data->delay_counter) {
	    /* Call the Pike initializers for the delayed placeholders. */
	    struct unfinished_obj_link *up;

	    while ((up = data->unfinished_placeholders)) {
	      struct object *o;
	      data->unfinished_placeholders = up->next;
	      push_object(o = up->o);
	      free(up);
	      call_pike_initializers(o, 0);
	      pop_stack();
	    }
	  }

#ifdef ENCODE_DEBUG
	  data->depth -= 2;
#endif
	  goto decode_done;
	}

	default:
	  decode_error(data, NULL,
		       "Cannot decode program encoding type %ld\n",(long)num);
      }
      break;

  default:
    decode_error(data, NULL, "Failed to restore string (illegal type %d).\n",
		 what & TAG_MASK);
  }

  mapping_insert(data->decoded, &entry_id, Pike_sp-1);

decode_done:;
  EDB(2,fprintf(stderr, "%*sDecoded to <%ld>: ", data->depth, "", entry_id.u.integer);
      print_svalue(stderr, Pike_sp-1);
      fputc('\n', stderr););
#ifdef ENCODE_DEBUG
  data->depth -= 2;
#endif
}

static struct decode_data *current_decode = NULL;

static void free_decode_data (struct decode_data *data, int delay,
			      int DEBUGUSED(free_after_error))
{
#ifdef PIKE_DEBUG
  int e;
  struct keypair *k;
#endif

  debug_malloc_touch(data);

  if (current_decode == data) {
    current_decode = data->next;
  } else {
    struct decode_data *d;
    for (d = current_decode; d; d=d->next) {
      if (d->next == data) {
	d->next = d->next->next;
	break;
      }
    }
  }

  if(delay || data->support_delay_counter)
  {
    debug_malloc_touch(data);
    /* We have been delayed */
    return;
  }

#ifdef PIKE_DEBUG
  if (!free_after_error) {
    if(data->unfinished_programs)
      Pike_fatal("We have unfinished programs left in decode()!\n");
    if(data->unfinished_objects)
      Pike_fatal("We have unfinished objects left in decode()!\n");
    if(data->unfinished_placeholders)
      Pike_fatal("We have unfinished placeholders left in decode()!\n");
  }
#endif

  if (data->codec) free_object (data->codec);

  while(data->unfinished_programs)
  {
    struct unfinished_prog_link *tmp=data->unfinished_programs;
    data->unfinished_programs=tmp->next;
    free(tmp);
  }

  while(data->unfinished_objects)
  {
    struct unfinished_obj_link *tmp=data->unfinished_objects;
    data->unfinished_objects=tmp->next;
    free_svalue(&tmp->decode_arg);
    free_object(tmp->o);
    free(tmp);
  }

  while(data->unfinished_placeholders)
  {
    struct unfinished_obj_link *tmp=data->unfinished_placeholders;
    data->unfinished_placeholders=tmp->next;
    free_object(tmp->o);
    free(tmp);
  }

#ifdef PIKE_THREADS
  data->thread_state = NULL;
  free_object (data->thread_obj);
#endif

#ifdef PIKE_DEBUG
  if (!free_after_error) {
    NEW_MAPPING_LOOP (data->decoded->data) {
      if (TYPEOF(k->val) == T_PROGRAM &&
	  !(k->val.u.program->flags & PROGRAM_PASS_1_DONE)) {
	ONERROR err;
	/* Move some references to the stack so that they
	 * will be freed when decode_error() throws, but
	 * still be available to decode_error().
	 */
	push_string(data->data_str);
	push_mapping(data->decoded);
	SET_ONERROR(err, free, data);

	decode_error (data, NULL,
		      "Got unfinished program <%pO> after decode: %pO\n",
		      &k->ind, &k->val);
	UNSET_ONERROR(err);
      }
    }
  }
#endif
  free_string(data->data_str);
  free_mapping(data->decoded);
  free( (char *) data);
}

static void low_do_decode (struct decode_data *data)
{
  current_decode = data;

  decode_value2(data);

  while (data->ptr < data->len) {
    decode_value2 (data);
    pop_stack();
  }
}

static void error_free_decode_data (struct decode_data *data)
{
  int delay;
  debug_malloc_touch (data);
  delay = 0;
  if (data->debug_buf) {
    free_string_builder(data->debug_buf);
    data->debug_buf = NULL;
  }
  free_decode_data (data, delay, 1);
}

#ifdef ENCODE_DEBUG
static void error_debug_free_decode_data (struct decode_data *data)
{
  write_and_reset_string_builder(2, data->debug_buf);
  error_free_decode_data (data);
}
#endif

static INT32 my_decode(struct pike_string *tmp,
                       struct object *codec, int explicit_codec,
                       int debug, struct string_builder *debug_buf)
{
  struct decode_data *data;
  ONERROR err;
#ifdef ENCODE_DEBUG
  struct string_builder buf;
#endif

  /* FIXME: Why not use CYCLIC? */
  /* Attempt to avoid infinite recursion on circular structures. */
  for (data = current_decode; data; data=data->next) {
    if (data->raw == tmp &&
	(codec ? data->codec == codec : !data->explicit_codec)
#ifdef PIKE_THREADS
	&& data->thread_state == Pike_interpreter.thread_state
#endif
       ) {
      struct svalue *res;
      struct svalue val = SVALUE_INIT_INT (COUNTER_START);
      if ((res = low_mapping_lookup(data->decoded, &val))) {
	push_svalue(res);
	return 1;
      }
      /* Possible recursion detected. */
      /* return 0; */
    }
  }

  data=ALLOC_STRUCT(decode_data);
  SET_SVAL(data->counter, T_INT, NUMBER_NUMBER, integer, COUNTER_START);
  data->data_str = tmp;
  data->data=(unsigned char *)tmp->str;
  data->len=tmp->len;
  data->ptr=0;
  data->codec=codec;
  data->explicit_codec = explicit_codec;
  data->pickyness=0;
  data->pass=1;
  data->unfinished_programs=0;
  data->unfinished_objects=0;
  data->unfinished_placeholders = NULL;
  data->delay_counter = 0;
  data->support_delay_counter = 0;
  data->support_compilation = NULL;
  data->raw = tmp;
  data->next = current_decode;
  data->debug_buf = debug_buf;
  data->debug_ptr = 0;
#ifdef PIKE_THREADS
  data->thread_state = Pike_interpreter.thread_state;
  data->thread_obj = Pike_interpreter.thread_state->thread_obj;
#endif
#ifdef ENCODE_DEBUG
  data->debug = debug;
  if (data->debug && !debug_buf) {
    data->debug_buf = debug_buf = &buf;
    init_string_builder(debug_buf, 0);
  }
  data->depth = -2;
#endif

  if (tmp->size_shift ||
      data->len < 5 ||
      GETC() != 182 ||
      GETC() != 'k' ||
      GETC() != 'e' ||
      GETC() != '0')
  {
    free( (char *) data);
    return 0;
  }

  ETRACE({
      string_builder_append_disassembly(data->debug_buf,
					data->debug_ptr,
					data->data + data->debug_ptr,
					data->data + data->ptr,
					".format  0",
					NULL,
					"Encoding #0.");
      data->debug_ptr = data->ptr;
    });

  data->decoded=allocate_mapping(128);

  add_ref (data->data_str);
  if (data->codec) add_ref (data->codec);
#ifdef PIKE_THREADS
  add_ref (data->thread_obj);
#endif
#ifdef ENCODE_DEBUG
  if (debug_buf == &buf)
    SET_ONERROR(err, error_debug_free_decode_data, data);
  else
#endif
    SET_ONERROR(err, error_free_decode_data, data);

  low_do_decode (data);

  UNSET_ONERROR(err);

  ETRACE({
      DECODE_FLUSH();
    });

#ifdef ENCODE_DEBUG
  if (debug_buf == &buf) {
    write_and_reset_string_builder(2, debug_buf);
    free_string_builder(debug_buf);
    data->debug_buf = NULL;
  }
#endif

  {
    int delay;
    delay = 0;
    free_decode_data (data, delay, 0);
  }

  return 1;
}

/*! @class MasterObject
 */

/*! @decl program Encoder;
 *!
 *! This program in the master is cloned and used as codec by
 *! @[encode_value] if it wasn't given any codec. An instance is only
 *! created on-demand the first time @[encode_value] encounters
 *! something for which it needs a codec, i.e. an object, program, or
 *! function.
 *!
 *! @seealso
 *! @[Encoder], @[Pike.Encoder]
 */

/*! @decl program Decoder;
 *!
 *! This program in the master is cloned and used as codec by
 *! @[decode_value] if it wasn't given any codec. An instance is only
 *! created on-demand the first time @[decode_value] encounters
 *! something for which it needs a codec, i.e. the result of a call to
 *! @[Pike.Encoder.nameof].
 *!
 *! @seealso
 *! @[Decoder], @[Pike.Decoder]
 */

/*! @endclass
 */

/*! @class Encoder
 *!
 *!   Codec used by @[encode_value()] to encode objects, functions and
 *!   programs. Its purpose is to look up some kind of identifier for
 *!   them, so they can be mapped back to the corresponding instance
 *!   by @[decode_value()], rather than creating a new copy.
 */

/*! @decl mixed nameof(object|function|program x)
 *!
 *!   Called by @[encode_value()] to encode objects, functions and programs.
 *!
 *! @returns
 *!   Returns something encodable on success, typically a string.
 *!   The returned value will be passed to the corresponding
 *!   @[objectof()], @[functionof()] or @[programof()] by
 *!   @[decode_value()].
 *!
 *!   If it returns @[UNDEFINED] then @[encode_value] starts to encode
 *!   the thing recursively, so that @[decode_value] later will
 *!   rebuild a copy.
 *!
 *! @note
 *!   @[encode_value()] has fallbacks for some classes of objects,
 *!   functions and programs.
 *!
 *! @seealso
 *!   @[Decoder.objectof()], @[Decoder.functionof()],
 *!   @[Decoder.objectof()]
 */

/*! @endclass
 */

/*! @class Decoder
 *!
 *!   Codec used by @[decode_value()] to decode objects, functions and
 *!   programs which have been encoded by @[Encoder.nameof] in the
 *!   corresponding @[Encoder] object.
 */

/*! @decl object objectof(string data)
 *!
 *!   Decode object encoded in @[data].
 *!
 *!   This function is called by @[decode_value()] when it encounters
 *!   encoded objects.
 *!
 *! @param data
 *!   Encoding of some object as returned by @[Encoder.nameof()].
 *!
 *! @returns
 *!   Returns the decoded object.
 *!
 *! @seealso
 *!   @[functionof()], @[programof()]
 */

/*! @decl function functionof(string data)
 *!
 *!   Decode function encoded in @[data].
 *!
 *!   This function is called by @[decode_value()] when it encounters
 *!   encoded functions.
 *!
 *! @param data
 *!   Encoding of some function as returned by @[Encoder.nameof()].
 *!
 *! @returns
 *!   Returns the decoded function.
 *!
 *! @seealso
 *!   @[objectof()], @[programof()]
 */

/*! @decl program programof(string data)
 *!
 *!   Decode program encoded in @[data].
 *!
 *!   This function is called by @[decode_value()] when it encounters
 *!   encoded programs.
 *!
 *! @param data
 *!   Encoding of some program as returned by @[Encoder.nameof()].
 *!
 *! @returns
 *!   Returns the decoded program.
 *!
 *! @seealso
 *!   @[functionof()], @[objectof()]
 */

/*! @decl object __register_new_program(program p)
 *!
 *!   Called to register the program that is being decoded. Might get
 *!   called repeatedly with several other programs that are being
 *!   decoded recursively. The only safe assumption is that when the
 *!   top level thing being decoded is a program, then the first call
 *!   will be with the unfinished embryo that will later become that
 *!   program.
 *!
 *! @returns
 *!   Returns either zero or a placeholder object. A placeholder
 *!   object must be a clone of @[__null_program]. When the program is
 *!   finished, the placeholder object will be converted to a clone of
 *!   it. This is used for pike module objects.
 */

/*! @endclass
 */

/*! @class Codec
 *!
 *! An @[Encoder] and a @[Decoder] lumped into a single instance which
 *! can be used for both encoding and decoding.
 */

/*! @decl inherit Encoder;
 */

/*! @decl inherit Decoder;
 */

/*! @endclass
 */

/* Compatibility decoder */

static unsigned char extract_char(char **v, ptrdiff_t *l)
{
  if(!*l) decode_error(current_decode, NULL, "Not enough place for char.\n");
  else (*l)--;
  (*v)++;
  return ((unsigned char *)(*v))[-1];
}

static ptrdiff_t extract_int(char **v, ptrdiff_t *l)
{
  INT32 j;
  ptrdiff_t i;

  j=extract_char(v,l);
  if(j & 0x80) return (j & 0x7f);

  if((j & ~8) > 4)
    decode_error(current_decode, NULL, "Invalid integer.\n");
  i=0;
  while(j & 7) { i=(i<<8) | extract_char(v,l); j--; }
  if(j & 8) return -i;
  return i;
}

static void rec_restore_value(char **v, ptrdiff_t *l, int no_codec)
{
  ptrdiff_t t, i;

  i = extract_int(v,l);
  t = extract_int(v,l);
  switch(i)
  {
  case TAG_INT:
    push_int(t);
    return;

  case TAG_FLOAT:
    if(sizeof(ptrdiff_t) < sizeof(FLOAT_TYPE))  /* FIXME FIXME FIXME FIXME */
      decode_error(current_decode, NULL, "Float architecture not supported.\n");
    push_int(t);
    SET_SVAL_TYPE(Pike_sp[-1], T_FLOAT);
    return;

  case TAG_TYPE:
    {
      decode_error(current_decode, NULL, "TAG_TYPE not supported yet.\n");
    }
    return;

  case TAG_STRING:
    if(t<0) decode_error(current_decode, NULL,
			 "length of string is negative.\n");
    if(*l < t) decode_error(current_decode, NULL, "string too short\n");
    push_string(make_shared_binary_string(*v, t));
    (*l)-= t;
    (*v)+= t;
    return;

  case TAG_ARRAY:
    if(t<0) decode_error(current_decode, NULL,
			 "length of array is negative.\n");
    check_stack(t);
    for(i=0;i<t;i++) rec_restore_value(v,l,no_codec);
    f_aggregate(t); /* FIXME: Unbounded stack consumption. */
    return;

  case TAG_MULTISET:
    if(t<0) decode_error(current_decode, NULL,
			 "length of multiset is negative.\n");
    check_stack(t);
    for(i=0;i<t;i++) rec_restore_value(v,l,no_codec);
    f_aggregate_multiset(t); /* FIXME: Unbounded stack consumption. */
    return;

  case TAG_MAPPING:
    if(t<0) decode_error(current_decode, NULL,
			 "length of mapping is negative.\n");
    check_stack(t*2);
    for(i=0;i<t;i++)
    {
      rec_restore_value(v,l,no_codec);
      rec_restore_value(v,l,no_codec);
    }
    f_aggregate_mapping(t*2); /* FIXME: Unbounded stack consumption. */
    return;

  case TAG_OBJECT:
    if (no_codec) Pike_error(DECODING_NEEDS_CODEC_ERROR);
    if(t<0) decode_error(current_decode, NULL,
			 "length of object is negative.\n");
    if(*l < t) decode_error(current_decode, NULL, "string too short\n");
    push_string(make_shared_binary_string(*v, t));
    (*l) -= t; (*v) += t;
    APPLY_MASTER("objectof", 1);
    return;

  case TAG_FUNCTION:
    if (no_codec) Pike_error(DECODING_NEEDS_CODEC_ERROR);
    if(t<0) decode_error(current_decode, NULL,
			 "length of function is negative.\n");
    if(*l < t) decode_error(current_decode, NULL, "string too short\n");
    push_string(make_shared_binary_string(*v, t));
    (*l) -= t; (*v) += t;
    APPLY_MASTER("functionof", 1);
    return;

  case TAG_PROGRAM:
    if (no_codec) Pike_error(DECODING_NEEDS_CODEC_ERROR);
    if(t<0) decode_error(current_decode, NULL,
			 "length of program is negative.\n");
    if(*l < t) decode_error(current_decode, NULL, "string too short\n");
    push_string(make_shared_binary_string(*v, t));
    (*l) -= t; (*v) += t;
    APPLY_MASTER("programof", 1);
    return;

  default:
    decode_error(current_decode, NULL, "Unknown type tag %ld:%ld\n",
                 (long)i, (long)t);
  }
}

static void restore_current_decode (struct decode_data *old_data)
{
  current_decode = old_data;
}

/* Defined in builtin.cmod. */
extern struct program *MasterCodec_program;

/*! @decl mixed decode_value(string coded_value, void|Codec|int(-1..-1) codec)
 *!
 *! Decode a value from the string @[coded_value].
 *!
 *! @param coded_value
 *!   Value to decode.
 *!
 *! @param codec
 *!   @[Codec] to use when encoding objects and programs.
 *!
 *! @param trace
 *!   @[String.Buffer] to contain a readable dump of the value.
 *!
 *! @param debug
 *!   Debug level. Only available when the runtime has been compiled
 *!   --with-rtl-debug.
 *!
 *! This function takes a string created with @[encode_value()] or
 *! @[encode_value_canonic()] and converts it back to the value that was
 *! coded.
 *!
 *! If @[codec] is specified, it's used as the codec for the decode.
 *! If none is specified, then one is instantiated through
 *! @expr{master()->Decoder()@}. As a compatibility fallback, the
 *! master itself is used if it has no @expr{Decoder@} class.
 *! If @[codec] is the special value @expr{-1@}, then decoding of
 *! types, functions, programs and objects is disabled.
 *!
 *! @note
 *!   Decoding a @[coded_value] that you have not generated yourself
 *!   is a @b{security risk@} that can lead to execution of arbitrary
 *!   code, unless @[codec] is specified as @expr{-1@}.
 *!
 *! @seealso
 *!   @[encode_value()], @[encode_value_canonic()]
 */
void f_decode_value(INT32 args)
{
  struct pike_string *s;
  struct object *codec = NULL;
  int explicit_codec = 0;
  int debug = 0;
  struct string_builder *debug_buf = NULL;

  check_all_args("decode_value", args,
		 BIT_STRING,
		 BIT_VOID | BIT_INT | BIT_OBJECT | BIT_ZERO,
                 BIT_VOID | BIT_OBJECT
#ifdef ENCODE_DEBUG
		 /* This argument is only an internal debug helper.
		  * It's intentionally not part of the function
		  * prototype, to keep the argument position free for
		  * other uses in the future. */
                 | BIT_INT
#endif
                 , 0);

  s = Pike_sp[-args].u.string;

  switch (args) {
    default:
      if (TYPEOF(Pike_sp[2-args]) == PIKE_T_OBJECT) {
	struct object *sb = Pike_sp[2-args].u.object;
	if ((debug_buf = string_builder_from_string_buffer(sb))) {
#ifdef ENCODE_DEBUG
	  debug = 1;
#endif
	}
#ifdef ENCODE_DEBUG
      } else {
	debug = Pike_sp[2-args].u.integer;
#endif
      }
      /* Fall through. */
    case 2:
      if (TYPEOF(Pike_sp[1-args]) == T_OBJECT) {
	if (SUBTYPEOF(Pike_sp[1-args])) {
	  struct decode_data data;
	  memset (&data, 0, sizeof (data));
	  data.data_str = s;	/* Not refcounted. */
	  decode_error(&data, NULL,
		       "The codec may not be a subtyped object yet.\n");
	}
	codec = Pike_sp[1-args].u.object;
        explicit_codec = 1;
	break;
      }
      if (TYPEOF(Pike_sp[1-args]) == PIKE_T_INT)
      {
        if (Pike_sp[1-args].u.integer == -1)
        {
          explicit_codec = 1; // the NULL-codec, in this case
          break;
        }
        else if (Pike_sp[1-args].u.integer)
        {
          SIMPLE_ARG_TYPE_ERROR("decode_value", 2, "int(-1..0) | object");
        }
      }
      /* Fall through. */
    case 1:
      if (!get_master()) {
	/* The codec used for decoding the master program. */
	push_object (clone_object (MasterCodec_program, 0));
	args++;
	codec = Pike_sp[-1].u.object;
        explicit_codec = 1;
      }
  }

  if(!my_decode(s, codec, explicit_codec, debug, debug_buf))
  {
    char *v=s->str;
    ptrdiff_t l=s->len;
    struct decode_data data;
    ONERROR uwp;
    memset (&data, 0, sizeof (data));
    data.data_str = s;		/* Not refcounted. */
    SET_ONERROR (uwp, restore_current_decode, current_decode);
    current_decode = &data;
    rec_restore_value(&v, &l, !codec && explicit_codec);
    CALL_AND_UNSET_ONERROR (uwp);
  }
  assign_svalue(Pike_sp-args-1, Pike_sp-1);
  pop_n_elems(args);
}
