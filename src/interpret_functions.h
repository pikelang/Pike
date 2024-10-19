/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

/*
 * Opcode definitions for the interpreter.
 */

#include "global.h"

#undef CJUMP
#undef LOOP
#undef COMPARISON
#undef MKAPPLY
#undef DO_CALL_BUILTIN

#ifdef GEN_PROTOS
/* Used to generate the interpret_protos.h file. */
#define OPCODE0(A, B, F, C)		OPCODE0(A, B, F) --- C
#define OPCODE1(A, B, F, C)		OPCODE1(A, B, F) --- C
#define OPCODE2(A, B, F, C)		OPCODE2(A, B, F) --- C
#define OPCODE0_TAIL(A, B, F, C)	OPCODE0_TAIL(A, B, F) --- C
#define OPCODE1_TAIL(A, B, F, C)	OPCODE1_TAIL(A, B, F) --- C
#define OPCODE2_TAIL(A, B, F, C)	OPCODE2_TAIL(A, B, F) --- C
#define OPCODE0_JUMP(A, B, F, C)	OPCODE0_JUMP(A, B, F) --- C
#define OPCODE1_JUMP(A, B, F, C)	OPCODE1_JUMP(A, B, F) --- C
#define OPCODE2_JUMP(A, B, F, C)	OPCODE2_JUMP(A, B, F) --- C
#define OPCODE0_TAILJUMP(A, B, F, C)	OPCODE0_TAILJUMP(A, B, F) --- C
#define OPCODE1_TAILJUMP(A, B, F, C)	OPCODE1_TAILJUMP(A, B, F) --- C
#define OPCODE2_TAILJUMP(A, B, F, C)	OPCODE2_TAILJUMP(A, B, F) --- C
#define OPCODE0_PTRJUMP(A, B, F, C)	OPCODE0_PTRJUMP(A, B, F) --- C
#define OPCODE1_PTRJUMP(A, B, F, C)	OPCODE1_PTRJUMP(A, B, F) --- C
#define OPCODE2_PTRJUMP(A, B, F, C)	OPCODE2_PTRJUMP(A, B, F) --- C
#define OPCODE0_TAILPTRJUMP(A, B, F, C)	OPCODE0_TAILPTRJUMP(A, B, F) --- C
#define OPCODE1_TAILPTRJUMP(A, B, F, C)	OPCODE1_TAILPTRJUMP(A, B, F) --- C
#define OPCODE2_TAILPTRJUMP(A, B, F, C)	OPCODE2_TAILPTRJUMP(A, B, F) --- C
#define OPCODE0_RETURN(A, B, F, C)	OPCODE0_RETURN(A, B, F) --- C
#define OPCODE1_RETURN(A, B, F, C)	OPCODE1_RETURN(A, B, F) --- C
#define OPCODE2_RETURN(A, B, F, C)	OPCODE2_RETURN(A, B, F) --- C
#define OPCODE0_TAILRETURN(A, B, F, C)	OPCODE0_TAILRETURN(A, B, F) --- C
#define OPCODE1_TAILRETURN(A, B, F, C)	OPCODE1_TAILRETURN(A, B, F) --- C
#define OPCODE2_TAILRETURN(A, B, F, C)	OPCODE2_TAILRETURN(A, B, F) --- C
#define OPCODE0_BRANCH(A, B, F, C)	OPCODE0_BRANCH(A, B, F) --- C
#define OPCODE1_BRANCH(A, B, F, C)	OPCODE1_BRANCH(A, B, F) --- C
#define OPCODE2_BRANCH(A, B, F, C)	OPCODE2_BRANCH(A, B, F) --- C
#define OPCODE0_TAILBRANCH(A, B, F, C)	OPCODE0_TAILBRANCH(A, B, F) --- C
#define OPCODE1_TAILBRANCH(A, B, F, C)	OPCODE1_TAILBRANCH(A, B, F) --- C
#define OPCODE2_TAILBRANCH(A, B, F, C)	OPCODE2_TAILBRANCH(A, B, F) --- C
#define OPCODE0_ALIAS(A, B, F, C)	OPCODE0_ALIAS(A, B, F, C) --- FOO
#define OPCODE1_ALIAS(A, B, F, C)	OPCODE1_ALIAS(A, B, F, C) --- FOO
#define OPCODE2_ALIAS(A, B, F, C)	OPCODE2_ALIAS(A, B, F, C) --- FOO
#endif /* GEN_PROTOS */

#ifndef OPCODE0_ALIAS
#define OPCODE0_ALIAS(A,B,C,D)	OPCODE0(A,B,C,{D();})
#endif /* !OPCODE0_ALIAS */
#ifndef OPCODE1_ALIAS
#define OPCODE1_ALIAS(A,B,C,D)	OPCODE1(A,B,C,{D();})
#endif /* !OPCODE1_ALIAS */
#ifndef OPCODE2_ALIAS
#define OPCODE2_ALIAS(A,B,C,D)	OPCODE2(A,B,C,{D();})
#endif /* !OPCODE2_ALIAS */


/*
#ifndef PROG_COUNTER
#define PROG_COUNTER pc
#endif
*/

#ifndef INTER_RETURN
#define INTER_RETURN return -1
#endif

/* BRANCH opcodes use these two to indicate whether the
 * branch should be taken or not.
 */
#ifndef DO_BRANCH
#define DO_BRANCH	DOJUMP
#endif
#ifndef DONT_BRANCH
#define DONT_BRANCH	SKIPJUMP
#endif

#ifndef MACHINE_CODE_FORCE_FP
#define MACHINE_CODE_FORCE_FP()	0
#endif

#ifndef OVERRIDE_JUMPS

#undef GET_JUMP
#undef SKIPJUMP
#undef DOJUMP

#ifdef PIKE_DEBUG

#define GET_JUMP() (backlog[backlogp].arg=(			\
  (Pike_interpreter.trace_level>3 ?				\
     sprintf(trace_buffer, "-    Target = %+ld\n",		\
             (long)LOW_GET_JUMP()),				\
     write_to_stderr(trace_buffer,strlen(trace_buffer)) : 0),	\
  LOW_GET_JUMP()))

#define SKIPJUMP() (GET_JUMP(), LOW_SKIPJUMP())

#else /* !PIKE_DEBUG */

#define GET_JUMP() (LOW_GET_JUMP())
#define SKIPJUMP() (LOW_SKIPJUMP())

#endif /* PIKE_DEBUG */

#define DOJUMP() do { \
    PIKE_OPCODE_T *addr;						\
    INT32 tmp;                                                          \
    JUMP_SET_TO_PC_AT_NEXT (addr);					\
    tmp = GET_JUMP();                                                   \
    SET_PROG_COUNTER(addr + tmp);                                       \
    FETCH;                                                              \
    if(tmp < 0)                                                         \
      FAST_CHECK_THREADS_ON_BRANCH();					\
  } while(0)

#endif /* OVERRIDE_JUMPS */


/* WARNING:
 * The surgeon general has stated that define code blocks
 * without do{}while() can be hazardous to your health.
 * However, in these cases it is required to handle break
 * properly. -Hubbe
 */
#undef DO_JUMP_TO
#define DO_JUMP_TO(NEWPC)	{	\
  SET_PROG_COUNTER(NEWPC);		\
  FETCH;				\
  JUMP_DONE;				\
}

#undef DO_DUMB_RETURN
#define DO_DUMB_RETURN {				\
  if(Pike_fp -> flags & PIKE_FRAME_RETURN_INTERNAL)	\
  {							\
    int f=Pike_fp->flags;				\
    if(f & PIKE_FRAME_RETURN_POP)			\
       low_return_pop();				\
     else						\
       low_return();					\
							\
    DO_IF_DEBUG(if (Pike_interpreter.trace_level > 5)	\
      fprintf(stderr, "Returning to 0x%p\n",		\
	      Pike_fp->return_addr));			\
    DO_JUMP_TO(Pike_fp->return_addr);			\
  }							\
  DO_IF_DEBUG(if (Pike_interpreter.trace_level > 5)	\
    fprintf(stderr, "Inter return\n"));			\
  INTER_RETURN;						\
}

#undef DO_RETURN
#ifndef PIKE_DEBUG
#define DO_RETURN DO_DUMB_RETURN
#else
#define DO_RETURN {				\
  if(d_flag>3) do_gc(0);			\
  if(d_flag>4) do_debug();			\
  DO_DUMB_RETURN;				\
}
#endif

#ifdef OPCODE_RETURN_JUMPADDR
#define DO_JUMP_TO_NEXT do {						\
    PIKE_OPCODE_T *next_addr;						\
    JUMP_SET_TO_PC_AT_NEXT (next_addr);					\
    SET_PROG_COUNTER (next_addr);					\
    FETCH;								\
    JUMP_DONE;								\
  } while (0)
#else  /* !OPCODE_RETURN_JUMPADDR */
#define JUMP_SET_TO_PC_AT_NEXT(PC) ((PC) = PROG_COUNTER)
#define DO_JUMP_TO_NEXT JUMP_DONE
#endif	/* !OPCODE_RETURN_JUMPADDR */

#undef DO_INDEX
#define DO_INDEX do {				\
    struct svalue tmp;		                \
    index_no_free(&tmp,Pike_sp-2,Pike_sp-1);	\
    pop_2_elems();				\
    move_svalue (Pike_sp, &tmp);		\
    Pike_sp++;					\
    print_return_value();			\
  }while(0)


OPCODE0(F_UNDEFINED, "push UNDEFINED", I_UPDATE_SP, {
  push_undefined();
});

OPCODE0(F_CONST0, "push 0", I_UPDATE_SP, {
  push_int(0);
});

OPCODE0(F_CONST1, "push 1", I_UPDATE_SP, {
  push_int(1);
});


OPCODE0(F_MARK_AND_CONST0, "mark & 0", I_UPDATE_SP|I_UPDATE_M_SP, {
  *(Pike_mark_sp++)=Pike_sp;
  push_int(0);
});

OPCODE0(F_MARK_AND_CONST1, "mark & 1", I_UPDATE_SP|I_UPDATE_M_SP, {
  *(Pike_mark_sp++)=Pike_sp;
  push_int(1);
});

OPCODE0(F_CONST_1,"push -1", I_UPDATE_SP, {
  push_int(-1);
});

OPCODE0(F_BIGNUM, "push 0x7fffffff", I_UPDATE_SP, {
  push_int(0x7fffffff);
});

OPCODE1(F_NUMBER, "push int", I_UPDATE_SP, {
  push_int(arg1);
});

/* always need to declare this opcode to make working dists */
#if SIZEOF_INT_TYPE > 4
OPCODE2(F_NUMBER64, "push 64-bit int", I_UPDATE_SP, {
   push_int( (INT_TYPE)
	     (( ((unsigned INT_TYPE)arg1) << 32)
	      | ((unsigned INT32)arg2)) );
});
#else
OPCODE2(F_NUMBER64, "push 64-bit int", I_UPDATE_SP, {
  Pike_error("F_NUMBER64: this opcode should never be used in your system\n");
});
#endif

OPCODE1(F_NEG_NUMBER, "push -int", I_UPDATE_SP, {
  if (!INT32_NEG_OVERFLOW(arg1)) {
    push_int(-arg1);
  } else {
    push_int(arg1);
    o_negate();
  }
});

OPCODE1(F_CONSTANT, "constant", I_UPDATE_SP|I_ARG_T_CONST, {
  push_svalue(& Pike_fp->context->prog->constants[arg1].sval);
  print_return_value();
});


/* Generic swap instruction:
 * swaps the arg1 top values with the arg2 values beneath
 */
OPCODE2(F_REARRANGE,"rearrange",0,{
  check_stack(arg2);
  memcpy(Pike_sp,Pike_sp-arg1-arg2,sizeof(struct svalue)*arg2);
  memmove(Pike_sp-arg1-arg2,Pike_sp-arg1,sizeof(struct svalue)*arg1);
  memcpy(Pike_sp-arg2,Pike_sp,sizeof(struct svalue)*arg2);
});

/* The rest of the basic 'push value' instructions */

OPCODE1_TAIL(F_MARK_AND_STRING, "mark & string",
             I_UPDATE_SP|I_UPDATE_M_SP|I_ARG_T_STRING, {
  *(Pike_mark_sp++)=Pike_sp;

  ADVERTISE_FALLTHROUGH;
  OPCODE1(F_STRING, "string", I_UPDATE_SP|I_ARG_T_STRING, {
    ref_push_string(Pike_fp->context->prog->strings[arg1]);
    print_return_value();
  });
});


OPCODE1(F_ARROW_STRING, "->string", I_UPDATE_SP|I_ARG_T_STRING, {
  ref_push_string(Pike_fp->context->prog->strings[arg1]);
  SET_SVAL_SUBTYPE(Pike_sp[-1], 1); /* Magic */
  print_return_value();
});

OPCODE1(F_LOOKUP_LFUN, "->lfun", I_ARG_T_GLOBAL, {
  struct object *o;
  struct svalue tmp;
  struct program *p;

  if ((TYPEOF(Pike_sp[-1]) == T_OBJECT) &&
      (p = (o = Pike_sp[-1].u.object)->prog) &&
      (FIND_LFUN(p = p->inherits[SUBTYPEOF(Pike_sp[-1])].prog,
		 LFUN_ARROW) == -1)) {
    int id = FIND_LFUN(p, arg1);
    if ((id != -1) &&
	(!(p->identifier_references[id].id_flags &
	   (ID_PROTECTED|ID_PRIVATE|ID_HIDDEN)))) {
      id += o->prog->inherits[SUBTYPEOF(Pike_sp[-1])].identifier_level;
      low_object_index_no_free(&tmp, o, id);
    } else {
      /* Not found. */
      SET_SVAL(tmp, T_INT, NUMBER_UNDEFINED, integer, 0);
    }
  } else {
    struct svalue tmp2;
    SET_SVAL(tmp2, PIKE_T_STRING, 1, string, lfun_strings[arg1]);
    index_no_free(&tmp, Pike_sp-1, &tmp2);
  }
  free_svalue(Pike_sp-1);
  move_svalue (Pike_sp - 1, &tmp);
  print_return_value();
});

OPCODE1(F_LFUN, "local function", I_UPDATE_SP|I_ARG_T_GLOBAL, {
  ref_push_function (Pike_fp->current_object,
		     arg1+Pike_fp->context->identifier_level);
  print_return_value();
});

OPCODE2(F_TRAMPOLINE, "trampoline", I_UPDATE_SP|I_ARG_T_GLOBAL, {
  struct pike_frame *f=Pike_fp;
  DO_IF_DEBUG(INT32 arg2_ = arg2;)
  struct object *o;
  o = low_clone(pike_trampoline_program);

  while(arg2--) {
    DO_IF_DEBUG({
      if (!f->scope) {
	Pike_fatal("F_TRAMPOLINE %d, %d: Missing %d levels of scope!\n",
	      arg1, arg2_, arg2+1);
      }
    });
    f=f->scope;
  }
  add_ref( ((struct pike_trampoline *)(o->storage))->frame=f );
  ((struct pike_trampoline *)(o->storage))->func=arg1+Pike_fp->context->identifier_level;
  push_function(o, FIND_LFUN(pike_trampoline_program, LFUN_CALL));
  print_return_value();
});

/* The not so basic 'push value' instructions */

OPCODE1_TAIL(F_MARK_AND_GLOBAL, "mark & global",
             I_UPDATE_SP|I_UPDATE_M_SP|I_ARG_T_GLOBAL, {
  *(Pike_mark_sp++)=Pike_sp;

  ADVERTISE_FALLTHROUGH;
  OPCODE1(F_GLOBAL, "global", I_UPDATE_SP|I_ARG_T_GLOBAL, {
    low_index_current_object_no_free(Pike_sp, arg1);
    Pike_sp++;
    print_return_value();
  });
});

/* NB: arg1 is raw storage offset. */
OPCODE1(F_PRIVATE_GLOBAL, "global <private>", I_UPDATE_SP, {
    struct svalue *sp;
    struct object *co = Pike_fp->current_object;
    if(!co->prog) /* note: generate an error. */
      object_low_set_index(co,0,0);
    sp = (struct svalue *)(Pike_fp->current_storage + arg1);
    push_svalue( sp );
    print_return_value();
});

/* NB: arg1 is raw storage offset. */
OPCODE2(F_PRIVATE_IF_DIRECT_GLOBAL, "global <private if direct>",
        I_UPDATE_SP|I_ARG2_T_GLOBAL, {
    struct object *co = Pike_fp->current_object;
    struct inherit *cx = Pike_fp->context;
    if(!co->prog) /* note: generate an error. */
      object_low_set_index(co,0,0);
    if( co->prog != cx->prog )
    {
      low_index_current_object_no_free(Pike_sp, arg2);
      Pike_sp++;
      print_return_value();
    }
    else
    {
      struct svalue *sp;
      sp = (struct svalue *)(Pike_fp->current_storage + arg1);
      push_svalue( sp );
      print_return_value();
    }
});

/* NB: arg1 is raw storage offset. */
OPCODE2(F_ASSIGN_PRIVATE_IF_DIRECT_GLOBAL,
        "assign global <private if direct>", I_ARG2_T_GLOBAL,
   {
     struct object *co = Pike_fp->current_object;
     struct inherit *cx = Pike_fp->context;

     if( co->prog != cx->prog )
     {
       object_low_set_index(co, arg2 + cx->identifier_level, Pike_sp-1);
     }
     else
     {
       struct svalue *tmp;
       tmp = (struct svalue *)(Pike_fp->current_storage + arg1);
       assign_svalue(tmp,Pike_sp-1);
      }
   });

/* NB: arg1 is raw storage offset. */
OPCODE2(F_ASSIGN_PRIVATE_TYPED_GLOBAL_AND_POP,
        "assign global <private,typed> and pop", I_UPDATE_SP|I_ARG2_T_RTYPE, {
   /* lazy mode. */
  union anything  *tmp_s;
  struct object *o;
  o = Pike_fp->current_object;
  if(!o->prog) /* note: generate an error. */
    object_low_set_index(o,0,0);
  tmp_s = (union anything *)(Pike_fp->current_storage + arg1);
  assign_to_short_svalue( tmp_s, arg2, Pike_sp-1 );
  pop_stack();
});

OPCODE2(F_ASSIGN_PRIVATE_TYPED_GLOBAL, "assign global <private,typed>",
        I_ARG2_T_RTYPE, {
  union anything  *tmp;
  struct object *o;
  o = Pike_fp->current_object;
  if(!o->prog) /* note: generate an error. */
    object_low_set_index(o,0,0);
  tmp = (union anything *)(Pike_fp->current_storage + arg1);
  assign_to_short_svalue( tmp, arg2, Pike_sp-1);
});


#if SIZEOF_FLOAT_TYPE != SIZEOF_INT_TYPE
#define DO_IF_ELSE_SIZEOF_FLOAT_INT(EQ, NEQ)	do { NEQ; } while(0)
#else
#define DO_IF_ELSE_SIZEOF_FLOAT_INT(EQ, NEQ)	do { EQ; } while(0)
#endif

OPCODE2(F_PRIVATE_TYPED_GLOBAL, "global <private,typed>",
        I_UPDATE_SP|I_ARG2_T_RTYPE, {
    void *ptr;
    struct object *o;

    o = Pike_fp->current_object;
    ptr = (void *)(Pike_fp->current_storage + arg1);
    if( arg2 < MIN_REF_TYPE )
    {
      DO_IF_ELSE_SIZEOF_FLOAT_INT(
	SET_SVAL_TYPE_SUBTYPE(Pike_sp[0], arg2, 0);
	Pike_sp[0].u.integer = *(INT_TYPE*)ptr;
	Pike_sp++;
      ,
	if( UNLIKELY(arg2)==PIKE_T_INT )
	  push_int( *(INT_TYPE*)ptr );
	else
	  push_float( *(FLOAT_TYPE*)ptr )
      );
    }
    else
    {
      INT32 *refs = *(INT32**)ptr;
      if( !refs )
	push_undefined();
      else
      {
	struct svalue tmp;
	SET_SVAL_TYPE_SUBTYPE(tmp,arg2,0);
	tmp.u.refs = refs;
	push_svalue(&tmp);
      }
    }

    print_return_value();
});

OPCODE2_TAIL(F_MARK_AND_EXTERNAL, "mark & external", I_UPDATE_SP|I_UPDATE_M_SP, {
  *(Pike_mark_sp++)=Pike_sp;

  ADVERTISE_FALLTHROUGH;
  OPCODE2(F_EXTERNAL,"external", I_UPDATE_SP, {
    struct external_variable_context loc;

    loc.o=Pike_fp->current_object;
    loc.parent_identifier=Pike_fp->fun;
    loc.inherit=Pike_fp->context;
    find_external_context(&loc, arg2);

    DO_IF_DEBUG({
      TRACE((5,"-   Identifier=%d Offset=%d\n",
	     arg1,
	     loc.inherit->identifier_level));
    });

    if (arg1 == IDREF_MAGIC_THIS)
      /* Special treatment to allow doing Foo::this on destructed
       * parent objects. */
      ref_push_object (loc.o);
    else {
      low_object_index_no_free(Pike_sp,
			       loc.o,
			       arg1 + loc.inherit->identifier_level);
      Pike_sp++;
    }
    print_return_value();
  });
});

OPCODE2(F_EXTERNAL_LVALUE, "& external", I_UPDATE_SP, {
  struct external_variable_context loc;

  loc.o=Pike_fp->current_object;
  loc.parent_identifier=Pike_fp->fun;
  loc.inherit=Pike_fp->context;
  find_external_context(&loc, arg2);

  if (!loc.o->prog)
    Pike_error ("Cannot access variable in destructed parent object.\n");

  DO_IF_DEBUG({
    TRACE((5,"-   Identifier=%d Offset=%d\n",
	   arg1,
	   loc.inherit->identifier_level));
  });

  ref_push_object(loc.o);
  push_obj_index(arg1 + loc.inherit->identifier_level);
});

OPCODE1(F_MARK_AND_LOCAL, "mark & local",
        I_UPDATE_SP|I_UPDATE_M_SP|I_ARG_T_LOCAL, {
  *(Pike_mark_sp++) = Pike_sp;
  push_svalue( Pike_fp->locals + arg1);
  print_return_value();
});

OPCODE1(F_LOCAL, "local", I_UPDATE_SP|I_ARG_T_LOCAL, {
  push_svalue( Pike_fp->locals + arg1);
  print_return_value();
});

OPCODE2(F_2_LOCALS, "2 locals", I_UPDATE_SP|I_ARG_T_LOCAL|I_ARG2_T_LOCAL, {
  push_svalue( Pike_fp->locals + arg1);
  print_return_value();
  push_svalue( Pike_fp->locals + arg2);
  print_return_value();
});

OPCODE2(F_LOCAL_2_LOCAL, "local = local", I_ARG_T_LOCAL|I_ARG2_T_LOCAL, {
  assign_svalue(Pike_fp->locals + arg1, Pike_fp->locals + arg2);
});

OPCODE2(F_LOCAL_2_GLOBAL, "global = local",I_ARG_T_GLOBAL|I_ARG2_T_LOCAL, {
  object_low_set_index(Pike_fp->current_object,
		       arg1 + Pike_fp->context->identifier_level,
		       Pike_fp->locals + arg2);
});

OPCODE2(F_GLOBAL_2_LOCAL, "local = global", I_ARG_T_LOCAL|I_ARG2_T_GLOBAL, {
  free_svalue(Pike_fp->locals + arg2);
  mark_free_svalue (Pike_fp->locals + arg2);
  low_index_current_object_no_free(Pike_fp->locals + arg2, arg1);
});

OPCODE1(F_LOCAL_LVALUE, "& local", I_UPDATE_SP|I_ARG_T_LOCAL, {
  SET_SVAL(Pike_sp[0], T_SVALUE_PTR, 0, lval, Pike_fp->locals + arg1);
  SET_SVAL_TYPE(Pike_sp[1], T_VOID);
  Pike_sp += 2;
});

OPCODE2(F_LEXICAL_LOCAL, "lexical local", I_UPDATE_SP|I_ARG_T_LOCAL, {
  struct pike_frame *f=Pike_fp;
  while(arg2-- > 0)
  {
    f=f->scope;
    if(!f) Pike_error("Lexical scope error.\n");
  }
  push_svalue(f->locals + arg1);
  print_return_value();
});

OPCODE2(F_LEXICAL_LOCAL_LVALUE, "&lexical local", I_UPDATE_SP|I_ARG_T_LOCAL, {
  struct pike_frame *f=Pike_fp;
  while(arg2-- > 0)
  {
    f=f->scope;
    if(!f) Pike_error("Lexical scope error.\n");
  }
  SET_SVAL(Pike_sp[0], T_SVALUE_PTR, 0, lval, f->locals+arg1);
  SET_SVAL_TYPE(Pike_sp[1], T_VOID);
  Pike_sp+=2;
});

OPCODE1(F_ARRAY_LVALUE, "[ lvalues ]", I_UPDATE_SP, {
  f_aggregate(arg1);
  Pike_sp[-1].u.array->flags |= ARRAY_LVALUE;
  Pike_sp[-1].u.array->type_field |= BIT_UNFINISHED | BIT_MIXED;
  /* FIXME: Shouldn't a ref be added here?
   *
   * No - T_ARRAY_LVALUE's are not reference-counted!
   */
  move_svalue (Pike_sp, Pike_sp - 1);
  SET_SVAL_TYPE(Pike_sp[-1], T_ARRAY_LVALUE);
  Pike_sp++;
});

OPCODE2(F_CLEAR_N_LOCAL, "clear n local", I_ARG_T_LOCAL, {
  struct svalue *locals = Pike_fp->locals;
  int e;
  free_mixed_svalues(locals + arg1, arg2);
  for(e = 0; e < arg2; e++)
  {
    SET_SVAL(locals[arg1+e], PIKE_T_INT, NUMBER_NUMBER, integer, 0);
  }
});

OPCODE1(F_CLEAR_LOCAL, "clear local", I_ARG_T_LOCAL, {
  free_svalue(Pike_fp->locals + arg1);
  SET_SVAL(Pike_fp->locals[arg1], PIKE_T_INT, NUMBER_NUMBER, integer, 0);
});

OPCODE2(F_ADD_LOCALS_AND_POP, "local += local", I_ARG_T_LOCAL|I_ARG2_T_LOCAL,
{
  struct svalue *dst = Pike_fp->locals+arg1;
  struct svalue *src = Pike_fp->locals+arg2;
  /* NB: The following test only works because PIKE_T_INT == 0! */
  if( (TYPEOF(*dst)|TYPEOF(*src)) == PIKE_T_INT
      && !INT_TYPE_ADD_OVERFLOW(src->u.integer,dst->u.integer) )
  {
    SET_SVAL_SUBTYPE(*dst,NUMBER_NUMBER);
    dst->u.integer += src->u.integer;
  }
  else if(TYPEOF(*dst) == TYPEOF(*src) && TYPEOF(*dst) == PIKE_T_STRING )
  {
      struct pike_string *srcs = src->u.string;
      struct pike_string *dsts = dst->u.string;
      if( dsts->len && srcs->len )
      {
          size_t tmp = dsts->len;
          size_t tmp2 = srcs->len;
          /*
           * in case srcs==dsts
           *  pike_string_cpy(MKPCHARP_STR_OFF(dsts,tmp), srcs);
           * does bad stuff
           */
          dsts = new_realloc_shared_string( dsts, tmp+srcs->len, MAXIMUM(srcs->size_shift,dsts->size_shift) );
          update_flags_for_add( dsts, srcs );
          generic_memcpy(MKPCHARP_STR_OFF(dsts,tmp), MKPCHARP_STR(srcs), tmp2);
          dst->u.string = low_end_shared_string( dsts );
      }
      else if( !dsts->len )
      {
          free_string( dsts );
          dst->u.string = srcs;
          srcs->refs++;
      }
  }
  else
  {
    *Pike_sp++ = *dst;
    SET_SVAL_TYPE(*dst,PIKE_T_INT);
    push_svalue( src );
    f_add(2);
    *dst = *--Pike_sp;
  }
});

OPCODE2(F_ADD_LOCAL_INT_AND_POP, "local += number", I_ARG_T_LOCAL,{
  struct svalue *dst = Pike_fp->locals+arg1;
  if( TYPEOF(*dst) == PIKE_T_INT
      && !INT_TYPE_ADD_OVERFLOW(dst->u.integer,arg2) )
  {
    SET_SVAL_SUBTYPE(*dst,NUMBER_NUMBER);
    dst->u.integer += arg2;
  }
  else
  {
    *Pike_sp++ = *dst;
    SET_SVAL_TYPE(*dst,PIKE_T_INT);
    push_int( arg2 );
    f_add(2);
    *dst = *--Pike_sp;
  }
});

OPCODE2(F_ADD_LOCAL_INT, "local += number local", I_ARG_T_LOCAL,{
  struct svalue *dst = Pike_fp->locals+arg1;
  if( TYPEOF(*dst) == PIKE_T_INT
      && !INT_TYPE_ADD_OVERFLOW(dst->u.integer,arg2) )
  {
    SET_SVAL_SUBTYPE(*dst,NUMBER_NUMBER);
    dst->u.integer += arg2;
    push_int( dst->u.integer );
  }
  else
  {
    *Pike_sp++ = *dst;
    SET_SVAL_TYPE(*dst,PIKE_T_INT);
    push_int( arg2 );
    f_add(2);
    *dst = *--Pike_sp;
  }
});

OPCODE1(F_INC_LOCAL, "++local", I_UPDATE_SP|I_ARG_T_LOCAL, {
  struct svalue *dst = Pike_fp->locals+arg1;
  if( (TYPEOF(*dst) == PIKE_T_INT)
      && !INT_TYPE_ADD_OVERFLOW(dst->u.integer, 1) )
  {
    push_int(++dst->u.integer);
    SET_SVAL_SUBTYPE(*dst, NUMBER_NUMBER); /* Could have UNDEFINED there before. */
  } else {
    *Pike_sp++ = *dst;
    SET_SVAL_TYPE(*dst,PIKE_T_INT);
    push_int(1);
    f_add(2);
    assign_svalue(dst, Pike_sp-1);
  }
});

OPCODE1(F_POST_INC_LOCAL, "local++", I_UPDATE_SP|I_ARG_T_LOCAL, {
  struct svalue *dst = Pike_fp->locals+arg1;
  if( (TYPEOF(*dst) == PIKE_T_INT)
      && !INT_TYPE_ADD_OVERFLOW(dst->u.integer, 1) )
  {
    push_int( dst->u.integer++ );
    SET_SVAL_SUBTYPE(*dst, NUMBER_NUMBER); /* Could have UNDEFINED there before. */
  } else {
    push_svalue( dst );
    push_svalue( dst );
    push_int(1);
    f_add(2);
    stack_pop_to(dst);
  }
});

OPCODE1(F_INC_LOCAL_AND_POP, "++local and pop", I_ARG_T_LOCAL, {
  struct svalue *dst = Pike_fp->locals+arg1;
  if( (TYPEOF(*dst) == PIKE_T_INT)
      && !INT_TYPE_ADD_OVERFLOW(dst->u.integer, 1) )
  {
    dst->u.integer++;
    SET_SVAL_SUBTYPE(*dst, NUMBER_NUMBER); /* Could have UNDEFINED there before. */
  } else {
    *Pike_sp++ = *dst;
    SET_SVAL_TYPE(*dst,PIKE_T_INT);
    push_int(1);
    f_add(2);
    *dst = *--Pike_sp;
  }
});

OPCODE1(F_DEC_LOCAL, "--local", I_UPDATE_SP|I_ARG_T_LOCAL, {
  struct svalue *dst = Pike_fp->locals+arg1;
  if( (TYPEOF(*dst) == PIKE_T_INT)
      && !INT_TYPE_SUB_OVERFLOW(dst->u.integer, 1) )
  {
    push_int(--(dst->u.integer));
    SET_SVAL_SUBTYPE(*dst, NUMBER_NUMBER); /* Could have UNDEFINED there before. */
  } else {
    *Pike_sp++ = *dst;
    SET_SVAL_TYPE(*dst,PIKE_T_INT);
    push_int(1);
    o_subtract();
    assign_svalue(dst,Pike_sp-1);
  }
});

OPCODE1(F_POST_DEC_LOCAL, "local--", I_UPDATE_SP|I_ARG_T_LOCAL, {
  push_svalue( Pike_fp->locals + arg1);

  if( (TYPEOF(Pike_fp->locals[arg1]) == PIKE_T_INT)
      && !INT_TYPE_SUB_OVERFLOW(Pike_fp->locals[arg1].u.integer, 1) )
  {
    Pike_fp->locals[arg1].u.integer--;
    SET_SVAL_SUBTYPE(Pike_fp->locals[arg1], NUMBER_NUMBER); /* Could have UNDEFINED there before. */
  } else {
    push_svalue(Pike_fp->locals + arg1);
    push_int(1);
    o_subtract();
    stack_pop_to(Pike_fp->locals + arg1);
  }
});

OPCODE1(F_DEC_LOCAL_AND_POP, "--local and pop", I_ARG_T_LOCAL, {
  struct svalue *dst = Pike_fp->locals+arg1;
  if( (TYPEOF(*dst) == PIKE_T_INT)
      && !INT_TYPE_SUB_OVERFLOW(dst->u.integer, 1) )
  {
    --dst->u.integer;
    SET_SVAL_SUBTYPE(*dst, NUMBER_NUMBER); /* Could have UNDEFINED there before. */
  } else {
    *Pike_sp++ = *dst;
    SET_SVAL_TYPE(*dst,PIKE_T_INT);
    push_int(1);
    o_subtract();
    *dst = *--Pike_sp;
  }
});

/* lval[0], lval[1], *Pike_sp
 * ->
 * lval[0], lval[1], result, *Pike_sp
 */
OPCODE0(F_LTOSVAL, "lvalue to svalue", I_UPDATE_SP, {
  dmalloc_touch_svalue(Pike_sp-2);
  dmalloc_touch_svalue(Pike_sp-1);
  lvalue_to_svalue_no_free(Pike_sp, Pike_sp-2);
  Pike_sp++;
  print_return_value();
});

/* The F_LTOSVAL*_AND_FREE opcodes are used to optimize foo+=bar and
 * similar things. The optimization is to free the old reference to
 * foo after it has been pushed on the stack. That way we make it
 * possible for foo to have only 1 reference, and then the low
 * array/multiset/mapping manipulation routines can be destructive if
 * they like.
 *
 * Warning: We must not release the interpreter lock while foo is
 * zeroed, or else other threads might read the zero in cases where
 * there's supposed to be none.
 *
 * FIXME: The next opcode must not throw, because then the zeroing
 * becomes permanent and can cause lasting side effects if it's a
 * global variable. F_ADD and most other opcodes currently break this.
 *
 * (Another way to handle both problems above is to restrict this
 * optimization to local variables.)
 */

/* lval[0], lval[1], x, *Pike_sp
 * ->
 * lval[0], lval[1], result, x, *Pike_sp
 */
OPCODE0(F_LTOSVAL2_AND_FREE, "ltosval2 and free", I_UPDATE_SP, {
  dmalloc_touch_svalue(Pike_sp-3);
  dmalloc_touch_svalue(Pike_sp-2);
  dmalloc_touch_svalue(Pike_sp-1);

  move_svalue (Pike_sp, Pike_sp - 1);
  mark_free_svalue (Pike_sp - 1);
  Pike_sp++;
  lvalue_to_svalue_no_free(Pike_sp-2, Pike_sp-4);

  if( (1 << TYPEOF(Pike_sp[-2])) &
      (BIT_ARRAY | BIT_MULTISET | BIT_MAPPING | BIT_STRING) )
  {
    struct svalue tmp;
    SET_SVAL(tmp, PIKE_T_INT, NUMBER_NUMBER, integer, 0);
    assign_lvalue(Pike_sp-4, &tmp);
  }
});

/* lval[0], lval[1], x, y, *Pike_sp
 * ->
 * lval[0], lval[1], result, x, y, *Pike_sp
 */
OPCODE0(F_LTOSVAL3_AND_FREE, "ltosval3 and free", I_UPDATE_SP, {
  dmalloc_touch_svalue(Pike_sp-4);
  dmalloc_touch_svalue(Pike_sp-3);
  dmalloc_touch_svalue(Pike_sp-2);
  dmalloc_touch_svalue(Pike_sp-1);

  move_svalue (Pike_sp, Pike_sp - 1);
  move_svalue (Pike_sp - 1, Pike_sp - 2);
  mark_free_svalue (Pike_sp - 2);
  Pike_sp++;
  lvalue_to_svalue_no_free(Pike_sp-3, Pike_sp-5);

  /* This is so that foo=foo[x..y] (and similar things) will be faster.
   * It's done by freeing the old reference to foo after it has been
   * pushed on the stack. That way foo can have only 1 reference if we
   * are lucky, and then the low array/multiset/mapping manipulation
   * routines can be destructive if they like.
   */
  if( (1 << TYPEOF(Pike_sp[-3])) &
      (BIT_ARRAY | BIT_MULTISET | BIT_MAPPING | BIT_STRING) )
  {
    struct svalue tmp;
    SET_SVAL(tmp, PIKE_T_INT, NUMBER_NUMBER, integer, 0);
    assign_lvalue(Pike_sp-5, &tmp);
  }
});

/* lval[0], lval[1], *Pike_sp
 * ->
 * lval[0], lval[1], result, *Pike_sp
 */
OPCODE0(F_LTOSVAL_AND_FREE, "ltosval and free", I_UPDATE_SP, {
  dmalloc_touch_svalue(Pike_sp-2);
  dmalloc_touch_svalue(Pike_sp-1);

  lvalue_to_svalue_no_free(Pike_sp, Pike_sp-2);
  Pike_sp++;

  /* See ltosval3. This opcode is used e.g. in foo = foo[..] where no
   * bound arguments are pushed on the stack. */
  if( (1 << TYPEOF(Pike_sp[-1])) &
      (BIT_ARRAY | BIT_MULTISET | BIT_MAPPING | BIT_STRING) )
  {
    struct svalue tmp;
    SET_SVAL(tmp, PIKE_T_INT, NUMBER_NUMBER, integer, 0);
    assign_lvalue(Pike_sp-3, &tmp);
  }
});

OPCODE0(F_ADD_TO, "+=", I_UPDATE_SP, {
  ONERROR uwp;
  move_svalue (Pike_sp, Pike_sp - 1);
  mark_free_svalue (Pike_sp - 1);
  Pike_sp++;
  lvalue_to_svalue_no_free(Pike_sp-2,Pike_sp-4);

  if( TYPEOF(Pike_sp[-1]) == PIKE_T_INT &&
      TYPEOF(Pike_sp[-2]) == PIKE_T_INT  )
  {
    if(!INT_TYPE_ADD_OVERFLOW(Pike_sp[-1].u.integer, Pike_sp[-2].u.integer))
    {
      /* Optimization for a rather common case. Makes it 30% faster. */
      INT_TYPE val = (Pike_sp[-1].u.integer += Pike_sp[-2].u.integer);
      SET_SVAL_SUBTYPE(Pike_sp[-1], NUMBER_NUMBER); /* Could have UNDEFINED there before. */
      assign_lvalue(Pike_sp-4,Pike_sp-1);
      Pike_sp-=2;
      pop_2_elems();
      push_int(val);
      goto add_to_done;
    }
  }
  /* This is so that foo+=bar (and similar things) will be faster.
   * It's done by freeing the old reference to foo after it has been
   * pushed on the stack. That way foo can have only 1 reference if we
   * are lucky, and then the low array/multiset/mapping manipulation
   * routines can be destructive if they like.
   */
  if( (1 << TYPEOF(Pike_sp[-2])) &
      (BIT_ARRAY | BIT_MULTISET | BIT_MAPPING | BIT_STRING) )
  {
    struct svalue tmp;
    SET_SVAL(tmp, PIKE_T_INT, NUMBER_NUMBER, integer, 0);
    assign_lvalue(Pike_sp-4, &tmp);
  } else if (TYPEOF(Pike_sp[-2]) == T_OBJECT) {
    /* One ref in the lvalue, and one on the stack. */
    int i;
    struct object *o;
    struct program *p;
    if((o = Pike_sp[-2].u.object)->refs <= 2 &&
       (p = o->prog) &&
       (i = FIND_LFUN(p->inherits[SUBTYPEOF(Pike_sp[-2])].prog,
		      LFUN_ADD_EQ)) != -1)
    {
      apply_low(o, i + p->inherits[SUBTYPEOF(Pike_sp[-2])].identifier_level, 1);
      /* NB: The lvalue already contains the object, so
       *     no need to reassign it.
       */
      pop_stack();
      stack_pop_2_elems_keep_top();
      goto add_to_done;
    }
  }
  /* NOTE: Pike_sp-4 is the lvalue, Pike_sp-2 is the original value.
   *       If an error gets thrown, the original value will thus be restored.
   *       If f_add() succeeds, Pike_sp-2 will hold the result.
   */
  SET_ONERROR(uwp, o_assign_lvalue, Pike_sp-4);
  f_add(2);
  CALL_AND_UNSET_ONERROR(uwp);	/* assign_lvalue(Pike_sp-3,Pike_sp-1); */
  stack_pop_2_elems_keep_top();
 add_to_done:
   ; /* make gcc happy */
});

OPCODE0(F_ADD_TO_AND_POP, "+= and pop", I_UPDATE_SP, {
  ONERROR uwp;
  move_svalue (Pike_sp, Pike_sp - 1);
  mark_free_svalue (Pike_sp - 1);
  Pike_sp++;
  lvalue_to_svalue_no_free(Pike_sp-2,Pike_sp-4);

  if( TYPEOF(Pike_sp[-1]) == PIKE_T_INT &&
      TYPEOF(Pike_sp[-2]) == PIKE_T_INT  )
  {
    if(!INT_TYPE_ADD_OVERFLOW(Pike_sp[-1].u.integer, Pike_sp[-2].u.integer))
    {
      /* Optimization for a rather common case. Makes it 30% faster. */
      Pike_sp[-1].u.integer += Pike_sp[-2].u.integer;
      SET_SVAL_SUBTYPE(Pike_sp[-1], NUMBER_NUMBER); /* Could have UNDEFINED there before. */
      assign_lvalue(Pike_sp-4,Pike_sp-1);
      Pike_sp-=2;
      pop_2_elems();
      goto add_to_and_pop_done;
    }
  }
  /* This is so that foo+=bar (and similar things) will be faster.
   * It's done by freeing the old reference to foo after it has been
   * pushed on the stack. That way foo can have only 1 reference if we
   * are lucky, and then the low array/multiset/mapping manipulation
   * routines can be destructive if they like.
   */
  if( (1 << TYPEOF(Pike_sp[-2])) &
      (BIT_ARRAY | BIT_MULTISET | BIT_MAPPING | BIT_STRING) )
  {
    struct svalue tmp;
    SET_SVAL(tmp, PIKE_T_INT, NUMBER_NUMBER, integer, 0);
    assign_lvalue(Pike_sp-4, &tmp);
  } else if (TYPEOF(Pike_sp[-2]) == PIKE_T_OBJECT) {
    /* One ref in the lvalue, and one on the stack. */
    int i;
    struct object *o;
    struct program *p;
    if((o = Pike_sp[-2].u.object)->refs <= 2 &&
       (p = o->prog) &&
       (i = FIND_LFUN(p->inherits[SUBTYPEOF(Pike_sp[-2])].prog,
		      LFUN_ADD_EQ)) != -1)
    {
      apply_low(o, i + p->inherits[SUBTYPEOF(Pike_sp[-2])].identifier_level, 1);
      /* NB: The lvalue already contains the object, so
       *     no need to reassign it.
       */
      pop_n_elems(4);
      goto add_to_and_pop_done;
    }
  }
  /* NOTE: Pike_sp-4 is the lvalue, Pike_sp-2 is the original value.
   *       If an error gets thrown, the original value will thus be restored.
   *       If f_add() succeeds, Pike_sp-2 will hold the result.
   */
  SET_ONERROR(uwp, o_assign_lvalue, Pike_sp-4);
  f_add(2);
  CALL_AND_UNSET_ONERROR(uwp);	/* assign_lvalue(Pike_sp-3,Pike_sp-1); */
  pop_n_elems(3);
 add_to_and_pop_done:
   ; /* make gcc happy */
});

OPCODE1(F_GLOBAL_LVALUE, "& global", I_UPDATE_SP|I_ARG_T_GLOBAL, {
  ref_push_object(Pike_fp->current_object);
  push_obj_index(arg1 + Pike_fp->context->identifier_level);
});

OPCODE0(F_INC, "++x", I_UPDATE_SP, {
  union anything *u=get_pointer_if_this_type(Pike_sp-2, PIKE_T_INT);
  /* NOTE: if u->integer is 0, the lvalue could be UNDEFINED.
   * we use the slow path to make sure it becomes a proper integer */
  if(u && u->integer && !INT_TYPE_ADD_OVERFLOW(u->integer, 1))
  {
    INT_TYPE val = ++u->integer;
    pop_2_elems();
    push_int(val);
  } else {
    lvalue_to_svalue_no_free(Pike_sp, Pike_sp-2); Pike_sp++;
    push_int(1);
    f_add(2);
    assign_lvalue(Pike_sp-3, Pike_sp-1);
    stack_pop_2_elems_keep_top();
  }
});

OPCODE0(F_DEC, "--x", I_UPDATE_SP, {
  union anything *u=get_pointer_if_this_type(Pike_sp-2, PIKE_T_INT);
  /* NOTE: if u->integer is 0, the lvalue could be UNDEFINED.
   * we use the slow path to make sure it becomes a proper integer */
  if(u && u->integer && !INT_TYPE_SUB_OVERFLOW(u->integer, 1))
  {
    INT_TYPE val = --u->integer;
    pop_2_elems();
    push_int(val);
  } else {
    lvalue_to_svalue_no_free(Pike_sp, Pike_sp-2); Pike_sp++;
    push_int(1);
    o_subtract();
    assign_lvalue(Pike_sp-3, Pike_sp-1);
    stack_pop_2_elems_keep_top();
  }
});

OPCODE0(F_DEC_AND_POP, "x-- and pop", I_UPDATE_SP, {
  union anything *u=get_pointer_if_this_type(Pike_sp-2, PIKE_T_INT);
  /* NOTE: if u->integer is 0, the lvalue could be UNDEFINED.
   * we use the slow path to make sure it becomes a proper integer */
  if(u && u->integer && !INT_TYPE_SUB_OVERFLOW(u->integer, 1))
  {
    --u->integer;
    pop_2_elems();
  }else{
    lvalue_to_svalue_no_free(Pike_sp, Pike_sp-2); Pike_sp++;
    push_int(1);
    o_subtract();
    assign_lvalue(Pike_sp-3, Pike_sp-1);
    pop_n_elems(3);
  }
});

OPCODE0(F_INC_AND_POP, "x++ and pop", I_UPDATE_SP, {
  union anything *u=get_pointer_if_this_type(Pike_sp-2, PIKE_T_INT);
  /* NOTE: if u->integer is 0, the lvalue could be UNDEFINED.
   * we use the slow path to make sure it becomes a proper integer */
  if(u && u->integer && !INT_TYPE_ADD_OVERFLOW(u->integer, 1))
  {
    ++u->integer;
    pop_2_elems();
  } else {
    lvalue_to_svalue_no_free(Pike_sp, Pike_sp-2); Pike_sp++;
    push_int(1);
    f_add(2);
    assign_lvalue(Pike_sp-3, Pike_sp-1);
    pop_n_elems(3);
  }
});

OPCODE0(F_POST_INC, "x++", I_UPDATE_SP, {
  union anything *u=get_pointer_if_this_type(Pike_sp-2, PIKE_T_INT);
  /* NOTE: if u->integer is 0, the lvalue could be UNDEFINED.
   * we use the slow path to make sure it becomes a proper integer */
  if(u && u->integer && !INT_TYPE_ADD_OVERFLOW(u->integer, 1))
  {
    INT_TYPE val = u->integer++;
    pop_2_elems();
    push_int(val);
  } else {
    lvalue_to_svalue_no_free(Pike_sp, Pike_sp-2); Pike_sp++;
    stack_dup();
    push_int(1);
    f_add(2);
    assign_lvalue(Pike_sp-4, Pike_sp-1);
    pop_stack();
    stack_pop_2_elems_keep_top();
    print_return_value();
  }
});

OPCODE0(F_POST_DEC, "x--", I_UPDATE_SP, {
  union anything *u=get_pointer_if_this_type(Pike_sp-2, PIKE_T_INT);
  /* NOTE: if u->integer is 0, the lvalue could be UNDEFINED.
   * we use the slow path to make sure it becomes a proper integer */
  if(u && u->integer && !INT_TYPE_SUB_OVERFLOW(u->integer, 1))
  {
    INT_TYPE val = u->integer--;
    pop_2_elems();
    push_int(val);
  } else {
    lvalue_to_svalue_no_free(Pike_sp, Pike_sp-2); Pike_sp++;
    stack_dup();
    push_int(1);
    o_subtract();
    assign_lvalue(Pike_sp-4, Pike_sp-1);
    pop_stack();
    stack_pop_2_elems_keep_top();
    print_return_value();
  }
});

OPCODE1(F_ASSIGN_LOCAL, "assign local", I_ARG_T_LOCAL, {
  assign_svalue(Pike_fp->locals+arg1,Pike_sp-1);
});

OPCODE0(F_ASSIGN, "assign", I_UPDATE_SP, {
  assign_lvalue(Pike_sp-3,Pike_sp-1);
  free_svalue(Pike_sp-3);
  free_svalue(Pike_sp-2);
  move_svalue (Pike_sp - 3, Pike_sp - 1);
  Pike_sp-=2;
});

OPCODE1(F_ASSIGN_INDICES, "assign[]", I_UPDATE_SP, {
  if(TYPEOF(Pike_sp[-2]) != PIKE_T_ARRAY )
      PIKE_ERROR("[*]=", "Destination is not an array.\n", Pike_sp, 1);
  if(TYPEOF(Pike_sp[-1]) != PIKE_T_ARRAY )
      PIKE_ERROR("[*]=", "Source is not an array.\n", Pike_sp-1, 1);
  assign_array_level( Pike_sp[-2].u.array, Pike_sp[-1].u.array, arg1 );
  pop_stack(); /* leaves arr on stack. */
});

OPCODE1(F_ASSIGN_ALL_INDICES, "assign[*]", I_UPDATE_SP, {
  if(TYPEOF(Pike_sp[-2]) != PIKE_T_ARRAY )
      PIKE_ERROR("[*]=", "Destination is not an array.\n", Pike_sp, 1);
  assign_array_level_value( Pike_sp[-2].u.array, Pike_sp-1, arg1 );
  pop_stack(); /* leaves arr on stack. */
});

OPCODE2(F_APPLY_ASSIGN_LOCAL_AND_POP,
        "apply, assign local and pop",
        I_UPDATE_SP|I_UPDATE_M_SP|I_ARG_T_CONST|I_ARG2_T_LOCAL, {
  apply_svalue(&((Pike_fp->context->prog->constants + arg1)->sval),
               (INT32)(Pike_sp - *--Pike_mark_sp));
  free_svalue(Pike_fp->locals+arg2);
  move_svalue (Pike_fp->locals + arg2, Pike_sp - 1);
  Pike_sp--;
});

OPCODE2(F_APPLY_ASSIGN_LOCAL, "apply, assign local",
        I_UPDATE_ALL|I_ARG_T_CONST|I_ARG2_T_LOCAL, {
  apply_svalue(&((Pike_fp->context->prog->constants + arg1)->sval),
               (INT32)(Pike_sp - *--Pike_mark_sp));
  assign_svalue(Pike_fp->locals+arg2, Pike_sp-1);
});

OPCODE0(F_ASSIGN_AND_POP, "assign and pop", I_UPDATE_SP, {
  assign_lvalue(Pike_sp-3, Pike_sp-1);
  pop_n_elems(3);
});

OPCODE1(F_ASSIGN_LOCAL_AND_POP, "assign local and pop",
        I_UPDATE_SP|I_ARG_T_LOCAL, {
  free_svalue(Pike_fp->locals + arg1);
  move_svalue (Pike_fp->locals + arg1, Pike_sp - 1);
  Pike_sp--;
});

OPCODE2(F_ASSIGN_LOCAL_NUMBER_AND_POP, "assign local number and pop",
        I_ARG_T_LOCAL, {
  free_svalue(Pike_fp->locals + arg1);
  SET_SVAL(Pike_fp->locals[arg1], PIKE_T_INT, 0, integer, arg2);
});

OPCODE1(F_ASSIGN_GLOBAL, "assign global", I_ARG_T_GLOBAL, {
  object_low_set_index(Pike_fp->current_object,
		       arg1 + Pike_fp->context->identifier_level,
		       Pike_sp-1);
});

OPCODE1(F_ASSIGN_GLOBAL_AND_POP, "assign global and pop",
        I_UPDATE_SP|I_ARG_T_GLOBAL, {
  object_low_set_index(Pike_fp->current_object,
		       arg1 + Pike_fp->context->identifier_level,
		       Pike_sp-1);
  pop_stack();
});

/* NB: Argument is raw storage offset. */
OPCODE1(F_ASSIGN_PRIVATE_GLOBAL_AND_POP,
        "assign private global and pop", I_UPDATE_SP, {
  struct svalue *tmp;
  struct object *co;
  co = Pike_fp->current_object;
  if(!co->prog) /* note: generate an error. */
    object_low_set_index(co,0,0);
  tmp = (struct svalue *)(Pike_fp->current_storage + arg1);
  free_svalue(tmp);
  *tmp = *--Pike_sp;
});

/* NB: Argument is raw storage offset. */
OPCODE1(F_ASSIGN_PRIVATE_GLOBAL, "assign private global", I_UPDATE_SP, {
  struct svalue *tmp;
  struct object *co;
  co = Pike_fp->current_object;
  if(!co->prog) /* note: generate an error. */
    object_low_set_index(co,0,0);
  tmp = (struct svalue *)(Pike_fp->current_storage + arg1);
  assign_svalue( tmp, Pike_sp-1 );
});

OPCODE2(F_ASSIGN_GLOBAL_NUMBER_AND_POP, "assign global number and pop",
        I_ARG_T_GLOBAL, {
  struct svalue tmp;
  SET_SVAL(tmp,PIKE_T_INT,0,integer,arg2);
  object_low_set_index(Pike_fp->current_object,
		       arg1 + Pike_fp->context->identifier_level,
		       &tmp);
});


/* Stack machine stuff */

OPCODE0(F_POP_VALUE, "pop", I_UPDATE_SP, {
  pop_stack();
});

OPCODE1(F_POP_N_ELEMS, "pop_n_elems", I_UPDATE_SP, {
  pop_n_elems(arg1);
});

OPCODE0_TAIL(F_MARK2, "mark mark", I_UPDATE_M_SP, {
  *(Pike_mark_sp++)=Pike_sp;

/* This opcode is only used when running with -d. Identical to F_MARK,
 * but with a different name to make the debug printouts more clear. */
  ADVERTISE_FALLTHROUGH;
  OPCODE0_TAIL(F_SYNCH_MARK, "synch mark", I_UPDATE_M_SP, {

    OPCODE0(F_MARK, "mark", I_UPDATE_M_SP, {
      *(Pike_mark_sp++)=Pike_sp;
    });
  });
});

OPCODE1(F_MARK_X, "mark Pike_sp-X", I_UPDATE_M_SP, {
  *(Pike_mark_sp++)=Pike_sp-arg1;
});

OPCODE0(F_POP_MARK, "pop mark", I_UPDATE_M_SP, {
  --Pike_mark_sp;
});

OPCODE0(F_POP_TO_MARK, "pop to mark", I_UPDATE_SP|I_UPDATE_M_SP, {
  pop_n_elems(Pike_sp - *--Pike_mark_sp);
});

/* These opcodes are only used when running with -d. The reason for
 * the two aliases is mainly to keep the indentation in asm debug
 * output. */
OPCODE0_TAIL(F_CLEANUP_SYNCH_MARK, "cleanup synch mark", I_UPDATE_SP|I_UPDATE_M_SP, {
  OPCODE0(F_POP_SYNCH_MARK, "pop synch mark", I_UPDATE_SP|I_UPDATE_M_SP, {
    if (d_flag) {
      if (Pike_mark_sp <= Pike_interpreter.mark_stack) {
	Pike_fatal("Mark stack out of synch - %p <= %p.\n",
		   Pike_mark_sp, Pike_interpreter.mark_stack);
      } else if (*--Pike_mark_sp != Pike_sp) {
	ptrdiff_t should = *Pike_mark_sp - Pike_interpreter.evaluator_stack;
	ptrdiff_t is = Pike_sp - Pike_interpreter.evaluator_stack;
	if (Pike_sp - *Pike_mark_sp > 0) /* not always same as Pike_sp > *Pike_mark_sp */
	/* Some attempt to recover, just to be able to report the backtrace. */
	  pop_n_elems(Pike_sp - *Pike_mark_sp);
	Pike_fatal("Stack out of synch - "
		   "should be %"PRINTPTRDIFFT"d, is %"PRINTPTRDIFFT"d.\n",
		   should, is);
      }
    }
  });
});

OPCODE0(F_CLEAR_STRING_SUBTYPE, "clear string subtype", 0, {
  if(TYPEOF(Pike_sp[-1]) == PIKE_T_STRING) SET_SVAL_SUBTYPE(Pike_sp[-1], 0);
});

      /* Jumps */
OPCODE0_BRANCH(F_BRANCH, "branch", 0, {
  MACHINE_CODE_FORCE_FP();
  DO_BRANCH();
});

OPCODE2_BRANCH(F_BRANCH_IF_NOT_LOCAL_ARROW, "branch if !local->x",
               I_ARG_T_STRING|I_ARG2_T_LOCAL, {
  struct svalue tmp;
  SET_SVAL(tmp, PIKE_T_STRING, 1, string,
	   Pike_fp->context->prog->strings[arg1]);
  mark_free_svalue (Pike_sp);
  Pike_sp++;
  index_no_free(Pike_sp-1,Pike_fp->locals+arg2, &tmp);
  print_return_value();

  /* Fall through */

  OPCODE0_TAILBRANCH(F_BRANCH_WHEN_ZERO, "branch if zero", I_UPDATE_SP, {
    if(!UNSAFE_IS_ZERO(Pike_sp-1))
    {
      MACHINE_CODE_FORCE_FP();
      DONT_BRANCH();
    }else{
      DO_BRANCH();
    }
    pop_stack();
  });
});

OPCODE0_BRANCH(F_QUICK_BRANCH_WHEN_ZERO, "(Q) branch if zero", I_UPDATE_SP, {
    if(Pike_sp[-1].u.integer)
    {
      DONT_BRANCH();
    }else{
      DO_BRANCH();
    }
    pop_stack();
  });

OPCODE0_BRANCH(F_QUICK_BRANCH_WHEN_NON_ZERO, "(Q) branch if not zero", I_UPDATE_SP, {
  if(Pike_sp[-1].u.integer)
  {
    DO_BRANCH();
  }else{
    DONT_BRANCH();
  }
  pop_stack();
});

OPCODE0_BRANCH(F_BRANCH_WHEN_NON_ZERO, "branch if not zero", I_UPDATE_SP, {
  if(UNSAFE_IS_ZERO(Pike_sp-1))
  {
    DONT_BRANCH();
  }else{
    DO_BRANCH();
  }
  pop_stack();
});

OPCODE1_BRANCH(F_BRANCH_IF_TYPE_IS_NOT, "branch if type is !=",
               I_UPDATE_SP|I_ARG_T_RTYPE, {
/*  fprintf(stderr,"******BRANCH IF TYPE IS NOT***** %s\n",get_name_of_type(arg1)); */
  struct object *o;
  if(TYPEOF(Pike_sp[-1]) == T_OBJECT &&
     (o = Pike_sp[-1].u.object)->prog)
  {
    int fun = FIND_LFUN(o->prog->inherits[SUBTYPEOF(Pike_sp[-1])].prog,
			LFUN__IS_TYPE);
    if(fun != -1)
    {
/*      fprintf(stderr,"******OBJECT OVERLOAD IN TYPEP***** %s\n",get_name_of_type(arg1)); */
      push_text(get_name_of_type(arg1));
      apply_low(o, fun +
		o->prog->inherits[SUBTYPEOF(Pike_sp[-2])].identifier_level, 1);
      arg1=UNSAFE_IS_ZERO(Pike_sp-1) ? T_FLOAT : T_OBJECT ;
      pop_stack();
    }
  }
  if(TYPEOF(Pike_sp[-1]) == arg1)
  {
    DONT_BRANCH();
  }else{
    DO_BRANCH();
  }
  pop_stack();
});

OPCODE1_BRANCH(F_BRANCH_IF_LOCAL, "branch if local", I_ARG_T_LOCAL, {
  if(UNSAFE_IS_ZERO(Pike_fp->locals + arg1))
  {
    DONT_BRANCH();
  }else{
    DO_BRANCH();
  }
});

OPCODE1_BRANCH(F_BRANCH_IF_NOT_LOCAL, "branch if !local", I_ARG_T_LOCAL, {
  if(!UNSAFE_IS_ZERO(Pike_fp->locals + arg1))
  {
    DONT_BRANCH();
  }else{
    DO_BRANCH();
  }
});

#define CJUMP(X, DESC, Y) \
  OPCODE0_BRANCH(X, DESC, I_UPDATE_SP, { \
    if(Y(Pike_sp-2,Pike_sp-1)) { \
      DO_BRANCH(); \
    }else{ \
      DONT_BRANCH(); \
    } \
    pop_2_elems(); \
  })

CJUMP(F_BRANCH_WHEN_EQ, "branch if ==", is_eq);
CJUMP(F_BRANCH_WHEN_NE, "branch if !=", !is_eq);
CJUMP(F_BRANCH_WHEN_LT, "branch if <", is_lt);
CJUMP(F_BRANCH_WHEN_LE, "branch if <=", is_le);
CJUMP(F_BRANCH_WHEN_GT, "branch if >", is_gt);
CJUMP(F_BRANCH_WHEN_GE, "branch if >=", is_ge);

OPCODE0_BRANCH(F_BRANCH_AND_POP_WHEN_ZERO, "branch & pop if zero", 0, {
  if(!UNSAFE_IS_ZERO(Pike_sp-1))
  {
    DONT_BRANCH();
  }else{
    DO_BRANCH();
    pop_stack();
  }
});

OPCODE0_BRANCH(F_BRANCH_AND_POP_WHEN_NON_ZERO, "branch & pop if !zero", 0, {
  if(UNSAFE_IS_ZERO(Pike_sp-1))
  {
    DONT_BRANCH();
  }else{
    DO_BRANCH();
    pop_stack();
  }
});

OPCODE0_BRANCH(F_LAND, "&&", I_UPDATE_SP, {
  if(!UNSAFE_IS_ZERO(Pike_sp-1))
  {
    DONT_BRANCH();
    pop_stack();
  }else{
    DO_BRANCH();
  }
});

OPCODE0_BRANCH(F_LOR, "||", I_UPDATE_SP, {
  if(UNSAFE_IS_ZERO(Pike_sp-1))
  {
    DONT_BRANCH();
    pop_stack();
  }else{
    DO_BRANCH();
  }
});

OPCODE0_BRANCH(F_EQ_OR, "==||", I_UPDATE_SP, {
  if(!is_eq(Pike_sp-2,Pike_sp-1))
  {
    DONT_BRANCH();
    pop_2_elems();
  }else{
    DO_BRANCH();
    pop_2_elems();
    push_int(1);
  }
});

OPCODE0_BRANCH(F_EQ_AND, "==&&", I_UPDATE_SP, {
  if(is_eq(Pike_sp-2,Pike_sp-1))
  {
    DONT_BRANCH();
    pop_2_elems();
  }else{
    DO_BRANCH();
    pop_2_elems();
    push_int(0);
  }
});

#ifndef ENTRY_PROLOGUE_SIZE
#define ENTRY_PROLOGUE_SIZE 0
#endif

/* Ideally this ought to be an OPCODE0_PTRRETURN but I don't fancy
 * adding that variety to this macro hell. At the end of the day there
 * wouldn't be any difference anyway afaics. /mast */
OPCODE0_PTRJUMP(F_CATCH, "catch", I_UPDATE_ALL|I_RETURN, {
  PIKE_OPCODE_T *addr;

  {
    struct catch_context *new_catch_ctx = alloc_catch_context();
    DO_IF_REAL_DEBUG (
      new_catch_ctx->frame = Pike_fp;
      init_recovery (&new_catch_ctx->recovery, 0, 0, PERR_LOCATION());
    );
    DO_IF_NOT_REAL_DEBUG (
      init_recovery (&new_catch_ctx->recovery, 0);
    );
    JUMP_SET_TO_PC_AT_NEXT (addr);
    new_catch_ctx->continue_reladdr = GET_JUMP()
      /* We need to run the entry prologue... */
      - ENTRY_PROLOGUE_SIZE;

    new_catch_ctx->next_addr = addr;
    new_catch_ctx->prev = Pike_interpreter.catch_ctx;
    Pike_interpreter.catch_ctx = new_catch_ctx;
    DO_IF_DEBUG({
	TRACE((3,"-   Pushed catch context %p\n", new_catch_ctx));
      });
  }

  /* Need to adjust next_addr by sizeof(INT32) to skip past the jump
   * address to the continue position after the catch block. */
  addr = (PIKE_OPCODE_T *) ((INT32 *) addr + 1);

  if (Pike_interpreter.catching_eval_jmpbuf) {
    /* There's already a catching_eval_instruction around our
     * eval_instruction, so we can just continue. */
    debug_malloc_touch_named (Pike_interpreter.catch_ctx, "(1)");
    /* Skip past the entry prologue... */
    addr += ENTRY_PROLOGUE_SIZE;
    SET_PROG_COUNTER(addr);
    FETCH;
    DO_IF_DEBUG({
	TRACE((3,"-   In active catch; continuing at %p\n", addr));
      });
    JUMP_DONE;
  }

  else {
    debug_malloc_touch_named (Pike_interpreter.catch_ctx, "(2)");

    while (1) {
      /* Loop here every time an exception is caught. Once we've
       * gotten here and set things up to run eval_instruction from
       * inside catching_eval_instruction, we keep doing it until it's
       * time to return. */

      int res;

      DO_IF_DEBUG({
	  TRACE((3,"-   Activating catch; calling %p in context %p\n",
		 addr, Pike_interpreter.catch_ctx));
	});

      res = catching_eval_instruction (addr);

      DO_IF_DEBUG({
	  TRACE((3,"-   catching_eval_instruction(%p) returned %d\n",
		 addr, res));
	});

      if (res != -3) {
	/* There was an inter return inside the evaluated code. Just
	 * propagate it. */
	DO_IF_DEBUG ({
	    TRACE((3,"-   Returning from catch.\n"));
	    if (res != -1) Pike_fatal ("Unexpected return value from "
				       "catching_eval_instruction: %d\n", res);
	  });
	break;
      }

      else {
	/* Caught an exception. */
	struct catch_context *cc = Pike_interpreter.catch_ctx;

	DO_IF_DEBUG ({
	    TRACE((3,"-   Caught exception. catch context: %p\n", cc));
	    if (!cc) Pike_fatal ("Catch context dropoff.\n");
	    if (cc->frame != Pike_fp)
	      Pike_fatal ("Catch context doesn't belong to this frame.\n");
	  });

	debug_malloc_touch_named (cc, "(3)");
	UNSETJMP (cc->recovery);
	move_svalue (Pike_sp++, &throw_value);
	mark_free_svalue (&throw_value);
	low_destruct_objects_to_destruct();

	if (cc->continue_reladdr < 0)
	  FAST_CHECK_THREADS_ON_BRANCH();
	addr = cc->next_addr + cc->continue_reladdr;

	DO_IF_DEBUG({
	    TRACE((3,"-   Popping catch context %p ==> %p\n",
		   cc, cc->prev));
	    if (!addr) Pike_fatal ("Unexpected null continue addr.\n");
	  });

	Pike_interpreter.catch_ctx = cc->prev;
	really_free_catch_context (cc);
      }
    }

    INTER_RETURN;
  }
});

OPCODE0(F_ESCAPE_CATCH, "escape catch", 0, {
  POP_CATCH_CONTEXT;
});

OPCODE0(F_EXIT_CATCH, "exit catch", I_UPDATE_SP, {
  push_undefined();
  POP_CATCH_CONTEXT;
});

OPCODE1_JUMP(F_SWITCH, "switch", I_UPDATE_ALL|I_ARG_T_CONST, {
  INT32 tmp;
  PIKE_OPCODE_T *addr;
  JUMP_SET_TO_PC_AT_NEXT (addr);
  tmp = switch_lookup(&Pike_fp->context->prog->constants[arg1].sval, Pike_sp-1);
  addr = (PIKE_OPCODE_T *) DO_ALIGN(PTR_TO_INT(addr), ((ptrdiff_t)sizeof(INT32)));
  addr = (PIKE_OPCODE_T *)(((INT32 *)addr) + (tmp>=0 ? 1+tmp*2 : 2*~tmp));
  if(*(INT32*)addr < 0) FAST_CHECK_THREADS_ON_BRANCH();
  pop_stack();
  DO_JUMP_TO(addr + *(INT32*)addr);
});

OPCODE1_JUMP(F_SWITCH_ON_INDEX, "switch on index", I_UPDATE_ALL|I_ARG_T_CONST, {
  INT32 tmp;
  PIKE_OPCODE_T *addr;
  struct svalue tmp2;
  JUMP_SET_TO_PC_AT_NEXT (addr);
  index_no_free(&tmp2, Pike_sp-2, Pike_sp-1);
  move_svalue (Pike_sp++, &tmp2);

  tmp = switch_lookup(&Pike_fp->context->prog->constants[arg1].sval, Pike_sp-1);
  pop_n_elems(3);
  addr = (PIKE_OPCODE_T *) DO_ALIGN(PTR_TO_INT(addr), ((ptrdiff_t)sizeof(INT32)));
  addr = (PIKE_OPCODE_T *)(((INT32 *)addr) + (tmp>=0 ? 1+tmp*2 : 2*~tmp));
  if(*(INT32*)addr < 0) FAST_CHECK_THREADS_ON_BRANCH();
  DO_JUMP_TO(addr + *(INT32*)addr);
});

OPCODE2_JUMP(F_SWITCH_ON_LOCAL, "switch on local",
             I_ARG_T_LOCAL|I_ARG2_T_CONST, {
  INT32 tmp;
  PIKE_OPCODE_T *addr;
  JUMP_SET_TO_PC_AT_NEXT (addr);
  tmp = switch_lookup(&Pike_fp->context->prog->constants[arg2].sval,
                      Pike_fp->locals + arg1);
  addr = (PIKE_OPCODE_T *) DO_ALIGN(PTR_TO_INT(addr), ((ptrdiff_t)sizeof(INT32)));
  addr = (PIKE_OPCODE_T *)(((INT32 *)addr) + (tmp>=0 ? 1+tmp*2 : 2*~tmp));
  if(*(INT32*)addr < 0) FAST_CHECK_THREADS_ON_BRANCH();
  DO_JUMP_TO(addr + *(INT32*)addr);
});


      /* LOOP(OPCODE, INCREMENT, OPERATOR, IS_OPERATOR) */
#define LOOP(ID, DESC, INC, OP2, OP4)					\
  OPCODE0_BRANCH(ID, DESC, 0, {						\
    union anything *i=get_pointer_if_this_type(Pike_sp-2, T_INT);	\
    if(i && !INT_TYPE_ADD_OVERFLOW(i->integer,INC) &&			\
       TYPEOF(Pike_sp[-3]) == T_INT)					\
    {									\
      i->integer += INC;						\
      if(i->integer OP2 Pike_sp[-3].u.integer)				\
      {									\
  	DO_BRANCH();							\
      }else{								\
  	DONT_BRANCH();							\
      }									\
    }else{								\
      lvalue_to_svalue_no_free(Pike_sp,Pike_sp-2); Pike_sp++;		\
      push_int(INC);							\
      f_add(2);								\
      assign_lvalue(Pike_sp-3,Pike_sp-1);				\
      if(OP4 ( Pike_sp-1, Pike_sp-4 ))					\
      {									\
  	DO_BRANCH();							\
      }else{								\
  	DONT_BRANCH();							\
      }									\
      pop_stack();							\
    }									\
  })

LOOP(F_INC_LOOP, "++Loop", 1, <, is_lt);
LOOP(F_DEC_LOOP, "--Loop", -1, >, is_gt);
LOOP(F_INC_NEQ_LOOP, "++Loop!=", 1, !=, !is_eq);
LOOP(F_DEC_NEQ_LOOP, "--Loop!=", -1, !=, !is_eq);

/* Use like:
 *
 * push(loopcnt)
 * branch(l2)
 * l1:
 *   sync_mark
 *     code
 *   pop_sync_mark
 * l2:
 * loop(l1)
 */
OPCODE0_BRANCH(F_LOOP, "loop", I_UPDATE_SP, { /* loopcnt */
  /* Use >= and 1 to be able to reuse the 1 for the subtraction. */
  push_int(1);
  if (!is_lt(Pike_sp-2, Pike_sp-1)) {
    o_subtract();
    DO_BRANCH();
  } else {
    DONT_BRANCH();
    pop_2_elems();
  }
});

OPCODE0_BRANCH(F_FOREACH, "foreach", 0, { /* array, lvalue, i */
  if(TYPEOF(Pike_sp[-4]) != PIKE_T_ARRAY)
    PIKE_ERROR("foreach", "Bad argument 1.\n", Pike_sp-3, 1);
  if(Pike_sp[-1].u.integer < Pike_sp[-4].u.array->size)
  {
    DO_IF_DEBUG(if(Pike_sp[-1].u.integer < 0)
      /* Isn't this an internal compiler error? /mast */
                  Pike_error("Foreach loop variable is negative!\n"));
    assign_lvalue(Pike_sp-3, Pike_sp[-4].u.array->item + Pike_sp[-1].u.integer);
    DO_BRANCH();
    Pike_sp[-1].u.integer++;
    DO_IF_DEBUG (
      if (SUBTYPEOF(Pike_sp[-1]))
	Pike_fatal ("Got unexpected subtype in loop variable.\n");
    );
  }else{
    DONT_BRANCH();
  }
});

OPCODE0(F_MAKE_ITERATOR, "get_iterator", 0, {
  f_get_iterator(1);
});

/* Stack is: iterator, index lvalue, value lvalue. */
OPCODE0_BRANCH (F_FOREACH_START, "foreach start", 0, {
    /* Branch on iterator failure. */
  DO_IF_DEBUG (
    if(TYPEOF(Pike_sp[-5]) != PIKE_T_OBJECT)
      Pike_fatal ("Iterator gone from stack.\n");
  );
  /* FIXME: object subtype. */
  if (foreach_iterate (Pike_sp[-5].u.object))
    DONT_BRANCH();
  else {
    DO_BRANCH();
  }
});

/* Stack is: iterator, index lvalue, value lvalue. */
OPCODE0_BRANCH(F_FOREACH_LOOP, "foreach loop", 0, {
    /* Branch on iterator success. */
  DO_IF_DEBUG (
    if(TYPEOF(Pike_sp[-5]) != PIKE_T_OBJECT)
      Pike_fatal ("Iterator gone from stack.\n");
  );
  /* FIXME: object subtype. */
  if(foreach_iterate(Pike_sp[-5].u.object))
  {
    DO_BRANCH();
  }else{
    DONT_BRANCH();
  }
});


OPCODE1_RETURN(F_RETURN_LOCAL,"return local",
               I_UPDATE_SP|I_UPDATE_FP|I_ARG_T_LOCAL, {
  DO_IF_DEBUG(
    /* special case! Pike_interpreter.mark_stack may be invalid at the time we
     * call return -1, so we must call the callbacks here to
     * prevent false alarms! /Hubbe
     */
    if(d_flag>3) do_gc(0);
    if(d_flag>4) do_debug();
    );
  if (!(Pike_fp->flags & (PIKE_FRAME_MALLOCED_LOCALS|PIKE_FRAME_SAVE_LOCALS))) {
    pop_n_elems(Pike_sp-1 - (Pike_fp->locals + arg1));
  } else {
    push_svalue(Pike_fp->locals + arg1);
  }
  DO_DUMB_RETURN;
});


OPCODE0_RETURN(F_RETURN_IF_TRUE,"return if true", I_UPDATE_SP|I_UPDATE_FP, {
  if(!UNSAFE_IS_ZERO(Pike_sp-1)) DO_RETURN;
  pop_stack();
  DO_JUMP_TO_NEXT;
});

OPCODE0_RETURN(F_RETURN_1,"return 1", I_UPDATE_SP|I_UPDATE_FP, {
  push_int(1);
  DO_RETURN;
});

OPCODE0_RETURN(F_RETURN_0,"return 0", I_UPDATE_SP|I_UPDATE_FP, {
  push_int(0);
  DO_RETURN;
});

OPCODE0_RETURN(F_RETURN, "return", I_UPDATE_FP, {
  DO_RETURN;
});

OPCODE0_RETURN(F_DUMB_RETURN,"dumb return", I_UPDATE_FP, {
  DO_DUMB_RETURN;
});

OPCODE0(F_NEGATE, "unary minus", 0, {
  if(TYPEOF(Pike_sp[-1]) == PIKE_T_INT)
  {
    if(INT_TYPE_NEG_OVERFLOW(Pike_sp[-1].u.integer))
    {
      convert_stack_top_to_bignum();
      o_negate();
    }
    else
    {
      Pike_sp[-1].u.integer =- Pike_sp[-1].u.integer;
      SET_SVAL_SUBTYPE(Pike_sp[-1], NUMBER_NUMBER); /* Could have UNDEFINED there before. */
    }
  }
  else if(TYPEOF(Pike_sp[-1]) == PIKE_T_FLOAT)
  {
    Pike_sp[-1].u.float_number =- Pike_sp[-1].u.float_number;
  }else{
    o_negate();
  }
});

OPCODE0_ALIAS(F_COMPL, "~", 0, o_compl);

OPCODE0(F_NOT, "!", 0, {
  switch(TYPEOF(Pike_sp[-1]))
  {
  case PIKE_T_INT:
    Pike_sp[-1].u.integer =! Pike_sp[-1].u.integer;
    SET_SVAL_SUBTYPE(Pike_sp[-1], NUMBER_NUMBER); /* Could have UNDEFINED there before. */
    break;

  case PIKE_T_FUNCTION:
  case PIKE_T_OBJECT:
    if(UNSAFE_IS_ZERO(Pike_sp-1))
    {
      pop_stack();
      push_int(1);
    }else{
      pop_stack();
      push_int(0);
    }
    break;

  default:
    free_svalue(Pike_sp-1);
    SET_SVAL(Pike_sp[-1], PIKE_T_INT, NUMBER_NUMBER, integer, 0);
  }
});

/* Used with F_LTOSVAL*_AND_FREE - must not release interpreter lock. */
OPCODE0_ALIAS(F_LSH, "<<", I_UPDATE_SP, o_lsh);
OPCODE0_ALIAS(F_RSH, ">>", I_UPDATE_SP, o_rsh);

#define COMPARISON(ID,DESC,EXPR)	\
  OPCODE0(ID, DESC, I_UPDATE_SP, {	\
    INT32 val = EXPR;			\
    pop_2_elems();			\
    push_int(val);			\
  })

COMPARISON(F_EQ, "==", is_eq(Pike_sp-2,Pike_sp-1));
COMPARISON(F_NE, "!=", !is_eq(Pike_sp-2,Pike_sp-1));
COMPARISON(F_GT, ">", is_gt(Pike_sp-2,Pike_sp-1));
COMPARISON(F_GE, ">=", is_ge(Pike_sp-2,Pike_sp-1));
COMPARISON(F_LT, "<", is_lt(Pike_sp-2,Pike_sp-1));
COMPARISON(F_LE, "<=", is_le(Pike_sp-2,Pike_sp-1));

/* Used with F_LTOSVAL*_AND_FREE - must not release interpreter lock. */
OPCODE0(F_ADD, "+", I_UPDATE_SP, {
  f_add(2);
});

/* Used with F_LTOSVAL*_AND_FREE - must not release interpreter lock. */
OPCODE0(F_ADD_INTS, "int+int", I_UPDATE_SP, {
  if(TYPEOF(Pike_sp[-1]) == T_INT && TYPEOF(Pike_sp[-2]) == T_INT
     && !INT_TYPE_ADD_OVERFLOW(Pike_sp[-1].u.integer, Pike_sp[-2].u.integer))
  {
    Pike_sp[-2].u.integer+=Pike_sp[-1].u.integer;
    SET_SVAL_SUBTYPE(Pike_sp[-2], NUMBER_NUMBER); /* Could have UNDEFINED there before. */
    dmalloc_touch_svalue(Pike_sp-1);
    Pike_sp--;
  }else{
    f_add(2);
  }
});

/* Used with F_LTOSVAL*_AND_FREE - must not release interpreter lock. */
OPCODE0(F_ADD_FLOATS, "float+float", I_UPDATE_SP, {
  if(TYPEOF(Pike_sp[-1]) == T_FLOAT && TYPEOF(Pike_sp[-2]) == T_FLOAT)
  {
    Pike_sp[-2].u.float_number+=Pike_sp[-1].u.float_number;
    dmalloc_touch_svalue(Pike_sp-1);
    Pike_sp--;
  }else{
    f_add(2);
  }
});

/* Used with F_LTOSVAL*_AND_FREE - must not release interpreter lock. */
OPCODE0_ALIAS(F_SUBTRACT, "-", I_UPDATE_SP, o_subtract);
OPCODE0_ALIAS(F_AND, "&", I_UPDATE_SP, o_and);
OPCODE0_ALIAS(F_OR, "|", I_UPDATE_SP, o_or);
OPCODE0_ALIAS(F_XOR, "^", I_UPDATE_SP, o_xor);
OPCODE0_ALIAS(F_MULTIPLY, "*", I_UPDATE_SP, o_multiply);
OPCODE0_ALIAS(F_DIVIDE, "/", I_UPDATE_SP, o_divide);
OPCODE0_ALIAS(F_MOD, "%", I_UPDATE_SP, o_mod);

OPCODE1(F_SUBTRACT_INT, "- int", 0, {
    push_int( arg1 );
    o_subtract();
});

OPCODE1(F_AND_INT, "& int", 0, {
    push_int( arg1 );
    o_and();
});

OPCODE1(F_OR_INT, "| int", 0, {
    push_int( arg1 );
    o_or();
});

OPCODE1(F_XOR_INT, "^ int", 0, {
    push_int( arg1 );
    o_xor();
});

OPCODE1(F_MULTIPLY_INT, "* int", 0, {
    push_int( arg1 );
    o_multiply();
});

OPCODE1(F_DIVIDE_INT, "/ int", 0, {
    push_int( arg1 );
    o_divide();
});

OPCODE1(F_MOD_INT, "% int", 0, {
    push_int( arg1 );
    o_mod();
});

OPCODE1(F_RSH_INT, ">> int", 0, {
    push_int( arg1 );
    o_rsh();
});

OPCODE1(F_LSH_INT, "<< int", 0, {
    push_int( arg1 );
    o_lsh();
});

OPCODE1(F_ADD_INT, "add integer", 0, {
  if(TYPEOF(Pike_sp[-1]) == T_INT
     && !INT_TYPE_ADD_OVERFLOW(Pike_sp[-1].u.integer, arg1))
  {
    Pike_sp[-1].u.integer+=arg1;
    SET_SVAL_SUBTYPE(Pike_sp[-1], NUMBER_NUMBER); /* Could have UNDEFINED there before. */
  }else{
    push_int(arg1);
    f_add(2);
  }
});

OPCODE1(F_ADD_NEG_INT, "add -integer", 0, {
  if(TYPEOF(Pike_sp[-1]) == T_INT
     && !INT_TYPE_ADD_OVERFLOW(Pike_sp[-1].u.integer, -arg1))
  {
    Pike_sp[-1].u.integer-=arg1;
    SET_SVAL_SUBTYPE(Pike_sp[-1], NUMBER_NUMBER); /* Could have UNDEFINED there before. */
  }else{
    push_int(-arg1);
    f_add(2);
  }
});

OPCODE0(F_PUSH_ARRAY, "@", I_UPDATE_SP, {
  struct object *o;
  struct program *p;

  switch(TYPEOF(Pike_sp[-1]))
  {
  default:
    PIKE_ERROR("@", "Bad argument.\n", Pike_sp, 1);

  case PIKE_T_OBJECT:
    {
    int i;
    if(!(p = (o = Pike_sp[-1].u.object)->prog) ||
       (i = FIND_LFUN(p->inherits[SUBTYPEOF(Pike_sp[-1])].prog,
		      LFUN__VALUES)) == -1)
      PIKE_ERROR("@", "Bad argument.\n", Pike_sp, 1);

    apply_low(o, i + p->inherits[SUBTYPEOF(Pike_sp[-1])].identifier_level, 0);
    if(TYPEOF(Pike_sp[-1]) != PIKE_T_ARRAY)
      Pike_error("Bad return type from o->_values() in @\n");
    free_svalue(Pike_sp-2);
    move_svalue (Pike_sp - 2, Pike_sp - 1);
    Pike_sp--;
    }
    break;

  case PIKE_T_ARRAY: break;
  }
  dmalloc_touch_svalue(Pike_sp-1);
  Pike_sp--;
  push_array_items(Pike_sp->u.array);
});

OPCODE0(F_APPEND_ARRAY, "append array", I_UPDATE_SP|I_UPDATE_M_SP, {
    o_append_array(Pike_sp - *(--Pike_mark_sp));
  });

OPCODE0(F_APPEND_MAPPING, "append mapping", I_UPDATE_SP|I_UPDATE_M_SP, {
    o_append_mapping(Pike_sp - *(--Pike_mark_sp));
  });

OPCODE2(F_LOCAL_LOCAL_INDEX, "local[local]",
        I_UPDATE_SP|I_ARG_T_LOCAL|I_ARG2_T_LOCAL, {
  struct svalue *s;
  s = Pike_fp->locals + arg1;
  if(TYPEOF(*s) == PIKE_T_STRING) SET_SVAL_SUBTYPE(*s, 0);
  mark_free_svalue (Pike_sp++);
  index_no_free(Pike_sp-1,Pike_fp->locals+arg2,s);
});

OPCODE1(F_LOCAL_INDEX, "local index", I_ARG_T_LOCAL, {
  struct svalue *s;
  struct svalue tmp;
  s = Pike_fp->locals + arg1;
  if(TYPEOF(*s) == PIKE_T_STRING) SET_SVAL_SUBTYPE(*s, 0);
  index_no_free(&tmp,Pike_sp-1,s);
  free_svalue(Pike_sp-1);
  move_svalue (Pike_sp - 1, &tmp);
});

OPCODE2(F_GLOBAL_LOCAL_INDEX, "global[local]",
        I_UPDATE_SP|I_ARG_T_GLOBAL|I_ARG2_T_LOCAL, {
  struct svalue *s;
  struct svalue tmp;
  low_index_current_object_no_free(Pike_sp, arg1);
  Pike_sp++;
  s=Pike_fp->locals+arg2;
  if(TYPEOF(*s) == PIKE_T_STRING) SET_SVAL_SUBTYPE(*s, 0);
  index_no_free(&tmp,Pike_sp-1,s);
  free_svalue(Pike_sp-1);
  move_svalue (Pike_sp - 1, &tmp);
});

OPCODE2(F_LOCAL_ARROW, "local->x", I_UPDATE_SP|I_ARG_T_STRING|I_ARG2_T_LOCAL, {
  struct pike_frame *fp = Pike_fp;
  struct svalue tmp;
  SET_SVAL(tmp, PIKE_T_STRING, 1, string,
	   fp->context->prog->strings[arg1]);
  mark_free_svalue (Pike_sp++);
  index_no_free(Pike_sp-1,fp->locals+arg2, &tmp);
  print_return_value();
});

OPCODE1(F_ARROW, "->x", I_ARG_T_STRING, {
  struct svalue tmp;
  struct svalue tmp2;
  SET_SVAL(tmp, PIKE_T_STRING, 1, string,
	   Pike_fp->context->prog->strings[arg1]);
  index_no_free(&tmp2, Pike_sp-1, &tmp);
  free_svalue(Pike_sp-1);
  move_svalue (Pike_sp - 1, &tmp2);
  print_return_value();
});

OPCODE1(F_STRING_INDEX, "string index", I_ARG_T_STRING, {
  struct svalue tmp;
  struct svalue tmp2;
  SET_SVAL(tmp, PIKE_T_STRING, 0, string,
	   Pike_fp->context->prog->strings[arg1]);
  index_no_free(&tmp2, Pike_sp-1, &tmp);
  free_svalue(Pike_sp-1);
  move_svalue (Pike_sp - 1, &tmp2);
  print_return_value();
});

OPCODE1(F_POS_INT_INDEX, "int index", 0, {
    push_int((ptrdiff_t)(int)arg1);
  print_return_value();
  DO_INDEX;
});

OPCODE1(F_NEG_INT_INDEX, "-int index", 0, {
  push_int(-(ptrdiff_t)arg1);
  print_return_value();
  DO_INDEX;
});

OPCODE0(F_INDEX, "index", I_UPDATE_SP, {
  DO_INDEX;
});

OPCODE2(F_MAGIC_INDEX, "::`[]", I_UPDATE_SP, {
  push_magic_index(magic_index_program, arg2, arg1);
});

OPCODE2(F_MAGIC_SET_INDEX, "::`[]=", I_UPDATE_SP, {
  push_magic_index(magic_set_index_program, arg2, arg1);
});

OPCODE2(F_MAGIC_INDICES, "::_indices", I_UPDATE_SP, {
  push_magic_index(magic_indices_program, arg2, arg1);
});

OPCODE2(F_MAGIC_VALUES, "::_values", I_UPDATE_SP, {
  push_magic_index(magic_values_program, arg2, arg1);
});

OPCODE0_ALIAS(F_CAST, "cast", I_UPDATE_SP, f_cast);
OPCODE0_ALIAS(F_CAST_TO_INT, "cast_to_int", 0, o_cast_to_int);
OPCODE0_ALIAS(F_CAST_TO_STRING, "cast_to_string", 0, o_cast_to_string);

OPCODE0(F_SOFT_CAST, "soft cast", I_UPDATE_SP, {
  /* Stack: type_string, value */
  DO_IF_DEBUG({
    if (TYPEOF(Pike_sp[-2]) != T_TYPE) {
      Pike_fatal("Argument 1 to soft_cast isn't a type!\n");
    }
  });
  if (runtime_options & RUNTIME_CHECK_TYPES) {
    o_check_soft_cast(Pike_sp-1, Pike_sp[-2].u.type);
    DO_IF_DEBUG({
      if (d_flag > 2) {
	struct pike_string *t = describe_type(Pike_sp[-2].u.type);
	fprintf(stderr, "Soft cast to %s\n", t->str);
	free_string(t);
      }
    });
  }
  stack_swap();
  pop_stack();
});

/* Used with F_LTOSVAL*_AND_FREE - must not release interpreter lock. */
OPCODE1_ALIAS(F_RANGE, "range", I_UPDATE_SP, o_range2);

OPCODE0(F_COPY_VALUE, "copy_value", 0, {
  struct svalue tmp;
  copy_svalues_recursively_no_free(&tmp,Pike_sp-1,1,0);
  free_svalue(Pike_sp-1);
  move_svalue (Pike_sp - 1, &tmp);
  print_return_value();
});

OPCODE0(F_INDIRECT, "indirect", I_UPDATE_SP, {
  struct svalue tmp;
  lvalue_to_svalue_no_free(&tmp, Pike_sp-2);
  if(TYPEOF(tmp) != PIKE_T_STRING)
  {
    pop_2_elems();
    move_svalue (Pike_sp, &tmp);
    Pike_sp++;
  }else{
    struct string_assignment_storage *s;
    struct object *o;
    o=low_clone(string_assignment_program);
    s = (struct string_assignment_storage *)o->storage;
    move_svalue (s->lval, Pike_sp - 2);
    move_svalue (s->lval + 1, Pike_sp - 1);
    s->s=tmp.u.string;
    Pike_sp-=2;
    push_object(o);
  }
  print_return_value();
});

OPCODE0(F_SIZEOF, "sizeof", 0, {
  INT_TYPE val = pike_sizeof(Pike_sp-1);
  pop_stack();
  push_int(val);
});

OPCODE0(F_SIZEOF_STRING, "sizeof(string)", 0, {
  INT_TYPE val = pike_sizeof(Pike_sp-1);
  pop_stack();
  push_int(val);
});

OPCODE1(F_SIZEOF_LOCAL, "sizeof local", I_UPDATE_SP|I_ARG_T_LOCAL, {
  push_int(pike_sizeof(Pike_fp->locals+arg1));
});

OPCODE1(F_SIZEOF_LOCAL_STRING, "sizeof local string", I_UPDATE_SP|I_ARG_T_LOCAL, {
  push_int(pike_sizeof(Pike_fp->locals+arg1));
});

OPCODE1_ALIAS(F_SSCANF, "sscanf", I_UPDATE_SP, o_sscanf);

OPCODE1_ALIAS(F_SSCANF_80, "sscanf_80", I_UPDATE_SP, o_sscanf_80);

#define MKAPPLY(OP,OPCODE,NAME,TYPE,  ARG2, ARG3)                       \
PIKE_CONCAT(OP,_JUMP)(PIKE_CONCAT(F_,OPCODE),NAME,I_UPDATE_ALL,         \
{                                                                       \
  PIKE_OPCODE_T *addr;                                                  \
  JUMP_SET_TO_PC_AT_NEXT (Pike_fp->return_addr);                        \
  if((addr=low_mega_apply(TYPE,(INT32)(Pike_sp - *--Pike_mark_sp),      \
                          ARG2, ARG3)))                                 \
  {                                                                     \
    Pike_fp->flags |= PIKE_FRAME_RETURN_INTERNAL;                       \
    DO_JUMP_TO(addr);                                                   \
  }                                                                     \
  else {                                                                \
    DO_JUMP_TO_NEXT;							\
  }									\
 });                                                                    \
                                                                        \
PIKE_CONCAT(OP,_JUMP)(PIKE_CONCAT3(F_,OPCODE,_AND_POP),NAME " & pop",   \
                      I_UPDATE_ALL,                                     \
{                                                                       \
  PIKE_OPCODE_T *addr;                                                  \
  JUMP_SET_TO_PC_AT_NEXT (Pike_fp->return_addr);			\
  if((addr=low_mega_apply(TYPE, (INT32)(Pike_sp - *--Pike_mark_sp),     \
                          ARG2, ARG3)))                                 \
  {                                                                     \
    Pike_fp->flags |= PIKE_FRAME_RETURN_INTERNAL | PIKE_FRAME_RETURN_POP; \
    DO_JUMP_TO(addr);                                                   \
  }                                                                     \
  else {                                                                \
    pop_stack();                                                        \
    DO_JUMP_TO_NEXT;                                                    \
  }                                                                     \
});								        \
                                                                        \
PIKE_CONCAT(OP,_RETURN)(PIKE_CONCAT3(F_,OPCODE,_AND_RETURN),		\
                        NAME " & return",I_UPDATE_ALL,                  \
{                                                                       \
  PIKE_OPCODE_T *addr;                                                  \
  if((addr = low_mega_apply(TYPE, (INT32)(Pike_sp - *--Pike_mark_sp),   \
                            ARG2,ARG3)))                                \
  {                                                                     \
    DO_IF_DEBUG(Pike_fp->next->pc=0);                                   \
    unlink_previous_frame();                                            \
    DO_JUMP_TO(addr);                                                   \
  }                                                                     \
  else {                                                                \
    DO_DUMB_RETURN;                                                     \
  }                                                                     \
})


#define MKAPPLY2(OP,OPCODE,NAME,TYPE,  ARG2, ARG3)			\
                                                                        \
MKAPPLY(OP,OPCODE,NAME,TYPE,  ARG2, ARG3);			        \
                                                                        \
PIKE_CONCAT(OP,_JUMP)(PIKE_CONCAT(F_MARK_,OPCODE),"mark, " NAME,	\
                      I_UPDATE_ALL,                                     \
{                                                                       \
  PIKE_OPCODE_T *addr;                                                  \
  JUMP_SET_TO_PC_AT_NEXT (Pike_fp->return_addr);                        \
  if((addr=low_mega_apply(TYPE, 0, ARG2, ARG3)))                        \
  {								        \
    Pike_fp->flags |= PIKE_FRAME_RETURN_INTERNAL;                       \
    DO_JUMP_TO(addr);                                                   \
  }                                                                     \
  else {                                                                \
    DO_JUMP_TO_NEXT;                                                    \
  }                                                                     \
});                                                                     \
                                                                        \
PIKE_CONCAT(OP,_JUMP)(PIKE_CONCAT3(F_MARK_,OPCODE,_AND_POP),		\
                      "mark, " NAME " & pop",I_UPDATE_ALL,              \
{                                                                       \
  PIKE_OPCODE_T *addr;                                                  \
  JUMP_SET_TO_PC_AT_NEXT (Pike_fp->return_addr);			\
  if((addr=low_mega_apply(TYPE, 0, ARG2, ARG3)))                        \
  {									\
    Pike_fp->flags |= PIKE_FRAME_RETURN_INTERNAL | PIKE_FRAME_RETURN_POP; \
    DO_JUMP_TO(addr);                                                   \
  }                                                                     \
  else {                                                                \
    pop_stack();                                                        \
    DO_JUMP_TO_NEXT;                                                    \
  }                                                                     \
});                                                                     \
                                                                        \
PIKE_CONCAT(OP,_RETURN)(PIKE_CONCAT3(F_MARK_,OPCODE,_AND_RETURN),	\
                        "mark, " NAME " & return",I_UPDATE_ALL,         \
{                                                                       \
  PIKE_OPCODE_T *addr;                                                  \
  if((addr=low_mega_apply(TYPE, 0, ARG2, ARG3)))                        \
  {									\
    DO_IF_DEBUG(Pike_fp->next->pc=0);                                   \
    unlink_previous_frame();                                            \
    DO_JUMP_TO(addr);                                                   \
  }                                                                     \
  else {                                                                \
    DO_DUMB_RETURN;                                                     \
  }                                                                     \
})


OPCODE1_JUMP(F_CALL_LFUN , "call lfun", I_UPDATE_ALL|I_ARG_T_GLOBAL, {
        PIKE_OPCODE_T *addr;
        JUMP_SET_TO_PC_AT_NEXT (Pike_fp->return_addr);
        if((addr = lower_mega_apply((INT32)(Pike_sp - *--Pike_mark_sp),
                            Pike_fp->current_object,
                                    (arg1+Pike_fp->context->identifier_level) )))
        {
            Pike_fp->flags |= PIKE_FRAME_RETURN_INTERNAL;
            DO_JUMP_TO(addr);
        }
        else
        {
            DO_JUMP_TO_NEXT;
        }
    });

OPCODE2_JUMP(F_CALL_LFUN_N , "call lfun <n>", I_UPDATE_ALL|I_ARG_T_GLOBAL, {
        PIKE_OPCODE_T *addr;
        JUMP_SET_TO_PC_AT_NEXT (Pike_fp->return_addr);
        if((addr = lower_mega_apply(arg2,
                                    Pike_fp->current_object,
                                    (arg1+Pike_fp->context->identifier_level) )))
        {
            Pike_fp->flags |= PIKE_FRAME_RETURN_INTERNAL;
            DO_JUMP_TO(addr);
        }
        else
        {
            DO_JUMP_TO_NEXT;
        }
    });

OPCODE1_JUMP(F_CALL_LFUN_AND_POP, "call lfun & pop",
             I_UPDATE_ALL|I_ARG_T_GLOBAL, {
        PIKE_OPCODE_T *addr;
        JUMP_SET_TO_PC_AT_NEXT (Pike_fp->return_addr);
        if((addr = lower_mega_apply((INT32)(Pike_sp - *--Pike_mark_sp),
                                         Pike_fp->current_object,
                                    (arg1+Pike_fp->context->identifier_level))))
        {
            Pike_fp->flags |= PIKE_FRAME_RETURN_INTERNAL | PIKE_FRAME_RETURN_POP;
            DO_JUMP_TO(addr);
        }
        else
        {
            pop_stack();
            DO_JUMP_TO_NEXT;
        }
    });

OPCODE1_RETURN(F_CALL_LFUN_AND_RETURN , "call lfun & return",
               I_UPDATE_ALL|I_ARG_T_GLOBAL, {
        PIKE_OPCODE_T *addr;
        if((addr = lower_mega_apply((INT32)(Pike_sp - *--Pike_mark_sp),
                            Pike_fp->current_object,
                                    (arg1+Pike_fp->context->identifier_level))))
        {
            DO_IF_DEBUG(Pike_fp->next->pc=0);
            unlink_previous_frame();
            DO_JUMP_TO(addr);
        }else{
            DO_DUMB_RETURN;
        }
  });

OPCODE1_JUMP(F_MARK_CALL_LFUN, "mark, call lfun" ,
             I_UPDATE_ALL|I_ARG_T_GLOBAL, {
    PIKE_OPCODE_T *addr;
    JUMP_SET_TO_PC_AT_NEXT (Pike_fp->return_addr);
    if((addr = lower_mega_apply(0, Pike_fp->current_object,
                             (arg1+Pike_fp->context->identifier_level)))) {
      Pike_fp->flags |= PIKE_FRAME_RETURN_INTERNAL;
      DO_JUMP_TO(addr);
    } else {
      DO_JUMP_TO_NEXT;
    }
  });

OPCODE1_JUMP( F_MARK_CALL_LFUN_AND_POP , "mark, call lfun & pop",
              I_UPDATE_ALL|I_ARG_T_GLOBAL, {
    PIKE_OPCODE_T *addr;
    JUMP_SET_TO_PC_AT_NEXT (Pike_fp->return_addr);
    if((addr = lower_mega_apply(0, Pike_fp->current_object,
                             (arg1+Pike_fp->context->identifier_level) )))
    {
      Pike_fp->flags |= PIKE_FRAME_RETURN_INTERNAL | PIKE_FRAME_RETURN_POP;
      DO_JUMP_TO(addr);
    }
    else
    {
      pop_stack();
      DO_JUMP_TO_NEXT;
    }
  });

OPCODE1_RETURN(F_MARK_CALL_LFUN_AND_RETURN , "mark, call lfun & return",
               I_UPDATE_ALL|I_ARG_T_GLOBAL, {
    PIKE_OPCODE_T *addr;
    if((addr = lower_mega_apply(0, Pike_fp->current_object,
                                (arg1+Pike_fp->context->identifier_level))))
    {
      DO_IF_DEBUG(Pike_fp->next->pc=0);
      unlink_previous_frame();
      DO_JUMP_TO(addr);
    }
    else
    {
        DO_DUMB_RETURN;
    }
  });

MKAPPLY2(OPCODE1,APPLY,"apply",APPLY_SVALUE_STRICT,
	 &((Pike_fp->context->prog->constants + arg1)->sval),0);

MKAPPLY(OPCODE0,CALL_FUNCTION,"call function",APPLY_STACK, 0,0);

OPCODE1_JUMP(F_CALL_OTHER,"call other", I_UPDATE_ALL|I_ARG_T_STRING, {
  INT32 args=(INT32)(Pike_sp - *--Pike_mark_sp);
  struct svalue *s;
  s = Pike_sp-args;
  JUMP_SET_TO_PC_AT_NEXT (Pike_fp->return_addr);
  if(TYPEOF(*s) == T_OBJECT)
  {
    struct object *o;
    struct program *p;
    o = s->u.object;
    if((p=o->prog))
    {
      p = p->inherits[SUBTYPEOF(*s)].prog;
      if(FIND_LFUN(p, LFUN_ARROW) == -1)
      {
        PIKE_OPCODE_T *addr;
	int fun;
	fun=find_shared_string_identifier(Pike_fp->context->prog->strings[arg1],
					  p);
	if(fun >= 0)
	{
	  fun += o->prog->inherits[SUBTYPEOF(*s)].identifier_level;
	  if((addr = lower_mega_apply(args-1, o, fun)))
	  {
	    Pike_fp->save_sp--;
	    Pike_fp->flags |= PIKE_FRAME_RETURN_INTERNAL;
	    DO_JUMP_TO(addr);
	  }
	  stack_pop_keep_top();
	  DO_JUMP_TO_NEXT;
	}
      }
    }
  }

  {
    struct svalue tmp;
    struct svalue tmp2;
    PIKE_OPCODE_T *addr;
    SET_SVAL(tmp, PIKE_T_STRING, 1, string,
	     Pike_fp->context->prog->strings[arg1]);

    index_no_free(&tmp2, s, &tmp);
    free_svalue(s);
    move_svalue (s, &tmp2);
    print_return_value();

    if((addr = low_mega_apply(APPLY_STACK, args, 0, 0)))
    {
      Pike_fp->flags |= PIKE_FRAME_RETURN_INTERNAL;
      DO_JUMP_TO(addr);
    }
    else {
      DO_JUMP_TO_NEXT;
    }
  }
});

OPCODE1_JUMP(F_CALL_OTHER_AND_POP,"call other & pop",
             I_UPDATE_ALL|I_ARG_T_STRING, {
  INT32 args=(INT32)(Pike_sp - *--Pike_mark_sp);
  struct svalue *s;
  s = Pike_sp-args;
  JUMP_SET_TO_PC_AT_NEXT (Pike_fp->return_addr);
  if(TYPEOF(*s) == T_OBJECT)
  {
    struct object *o;
    struct program *p;
    o = s->u.object;
    if((p=o->prog))
    {
      p = p->inherits[SUBTYPEOF(*s)].prog;
      if(FIND_LFUN(p, LFUN_ARROW) == -1)
      {
	int fun;
        PIKE_OPCODE_T *addr;
	fun=find_shared_string_identifier(Pike_fp->context->prog->strings[arg1],
					  p);
	if(fun >= 0)
	{
	  fun += o->prog->inherits[SUBTYPEOF(*s)].identifier_level;
	  if((addr = lower_mega_apply(args-1, o, fun)))
	  {
	    Pike_fp->save_sp--;
	    Pike_fp->flags |=
	      PIKE_FRAME_RETURN_INTERNAL |
	      PIKE_FRAME_RETURN_POP;
	    DO_JUMP_TO(addr);
	  }
	  pop_2_elems();
	  DO_JUMP_TO_NEXT;
	}
      }
    }
  }

  {
    struct svalue tmp;
    struct svalue tmp2;
    PIKE_OPCODE_T *addr;

    SET_SVAL(tmp, PIKE_T_STRING, 1, string,
	     Pike_fp->context->prog->strings[arg1]);

    index_no_free(&tmp2, s, &tmp);
    free_svalue(s);
    move_svalue (s, &tmp2);
    print_return_value();

    if((addr = low_mega_apply(APPLY_STACK, args, 0, 0)))
    {
      Pike_fp->flags |= PIKE_FRAME_RETURN_INTERNAL | PIKE_FRAME_RETURN_POP;
      DO_JUMP_TO(addr);
    }
    else {
      pop_stack();
      DO_JUMP_TO_NEXT;
    }
  }
});

OPCODE1_RETURN(F_CALL_OTHER_AND_RETURN,"call other & return",
               I_UPDATE_ALL|I_ARG_T_STRING, {
  INT32 args=(INT32)(Pike_sp - *--Pike_mark_sp);
  struct svalue *s;
  s = Pike_sp - args;
  if(TYPEOF(*s) == T_OBJECT)
  {
    struct object *o;
    struct program *p;
    o = s->u.object;
    if((p=o->prog))
    {
      p = p->inherits[SUBTYPEOF(*s)].prog;
      if(FIND_LFUN(p, LFUN_ARROW) == -1)
      {
	int fun;
        PIKE_OPCODE_T *addr;
	fun=find_shared_string_identifier(Pike_fp->context->prog->strings[arg1],
					  p);
	if(fun >= 0)
	{
	  fun += o->prog->inherits[SUBTYPEOF(*s)].identifier_level;
	  if((addr = lower_mega_apply(args-1, o, fun)))
	  {
	    Pike_fp->save_sp--;
	    DO_IF_DEBUG(Pike_fp->next->pc=0);
	    unlink_previous_frame();
	    DO_JUMP_TO(addr);
	  }
	  stack_pop_keep_top();
	  DO_DUMB_RETURN;
	}
      }
    }
  }

  {
    struct svalue tmp;
    struct svalue tmp2;
    PIKE_OPCODE_T *addr;
    SET_SVAL(tmp, PIKE_T_STRING, 1, string,
	     Pike_fp->context->prog->strings[arg1]);

    index_no_free(&tmp2, s, &tmp);
    free_svalue(s);
    move_svalue (s, &tmp2);
    print_return_value();

    if((addr = low_mega_apply(APPLY_STACK, args, 0, 0)))
    {
      DO_IF_DEBUG(Pike_fp->next->pc=0);
      unlink_previous_frame();
      DO_JUMP_TO(addr);
    }
    DO_DUMB_RETURN;
  }
});

#undef DO_CALL_BUILTIN
#ifdef PIKE_DEBUG
#define DO_CALL_BUILTIN(ARGS) do {					 \
  int args_=(ARGS);							 \
  struct svalue *expected_stack=Pike_sp-args_;				 \
  struct svalue *s;						         \
  s = &Pike_fp->context->prog->constants[arg1].sval;			 \
  if(Pike_interpreter.trace_level)					 \
  {									 \
    do_trace_efun_call(s, args_);                                        \
  }									 \
  if (PIKE_FN_START_ENABLED()) {					 \
    /* DTrace enter probe						 \
       arg0: function name						 \
       arg1: object							 \
    */									 \
    PIKE_FN_START(s->u.efun->name->size_shift == 0 ?			 \
		  s->u.efun->name->str : "[widestring fn name]",	 \
		  "");							 \
  }									 \
  (*(s->u.efun->function))(args_);					 \
  DO_IF_PROFILING (s->u.efun->runs++);					 \
  if(Pike_sp != expected_stack + !s->u.efun->may_return_void)		 \
  {									 \
    if(Pike_sp < expected_stack)					 \
      Pike_fatal("Function popped too many arguments: %s\n",		 \
	    s->u.efun->name->str);					 \
    if(Pike_sp>expected_stack+1)					 \
      Pike_fatal("Function left %"PRINTPTRDIFFT"d droppings on stack: %s\n", \
           Pike_sp-(expected_stack+1),					 \
	    s->u.efun->name->str);					 \
    if(Pike_sp == expected_stack && !s->u.efun->may_return_void)	 \
      Pike_fatal("Non-void function returned without return value "	 \
	    "on stack: %s %d\n",					 \
	    s->u.efun->name->str,s->u.efun->may_return_void);		 \
    if(Pike_sp==expected_stack+1 && s->u.efun->may_return_void)		 \
      Pike_fatal("Void function returned with a value on the stack: %s %d\n", \
	    s->u.efun->name->str, s->u.efun->may_return_void);		 \
  }									 \
  if(Pike_interpreter.trace_level>1) {					 \
    do_trace_efun_return(s, Pike_sp>expected_stack);                     \
  }									 \
  if (PIKE_FN_DONE_ENABLED()) {						 \
    /* DTrace leave probe						 \
       arg0: function name						 \
    */									 \
    PIKE_FN_DONE(s->u.efun->name->size_shift == 0 ?			 \
		 s->u.efun->name->str : "[widestring fn name]");	 \
  }									 \
}while(0)
#else
#define DO_CALL_BUILTIN(ARGS) do {					\
    (*(Pike_fp->context->prog->constants[arg1].sval.u.efun->function))(ARGS); \
  } while (0)
#endif

OPCODE1(F_CALL_BUILTIN, "call builtin", I_UPDATE_ALL|I_ARG_T_CONST, {
  FAST_CHECK_THREADS_ON_CALL();
  DO_CALL_BUILTIN((INT32)(Pike_sp - *--Pike_mark_sp));
});

OPCODE1(F_CALL_BUILTIN_AND_POP,"call builtin & pop",
        I_UPDATE_ALL|I_ARG_T_CONST, {
  FAST_CHECK_THREADS_ON_CALL();
  DO_CALL_BUILTIN((INT32)(Pike_sp - *--Pike_mark_sp));
  pop_stack();
});

OPCODE1_RETURN(F_CALL_BUILTIN_AND_RETURN,"call builtin & return",
               I_UPDATE_ALL|I_ARG_T_CONST, {
  FAST_CHECK_THREADS_ON_CALL();
  DO_CALL_BUILTIN((INT32)(Pike_sp - *--Pike_mark_sp));
  DO_DUMB_RETURN;
});


OPCODE1(F_MARK_CALL_BUILTIN, "mark, call builtin",
        I_UPDATE_ALL|I_ARG_T_CONST, {
  FAST_CHECK_THREADS_ON_CALL();
  DO_CALL_BUILTIN(0);
});

OPCODE1(F_MARK_CALL_BUILTIN_AND_POP, "mark, call builtin & pop",
        I_ARG_T_CONST, {
  FAST_CHECK_THREADS_ON_CALL();
  DO_CALL_BUILTIN(0);
  pop_stack();
});

OPCODE1_RETURN(F_MARK_CALL_BUILTIN_AND_RETURN, "mark, call builtin & return",
               I_UPDATE_ALL|I_ARG_T_CONST, {
  FAST_CHECK_THREADS_ON_CALL();
  DO_CALL_BUILTIN(0);
  DO_DUMB_RETURN;
});


OPCODE1(F_CALL_BUILTIN1, "call builtin 1", I_UPDATE_ALL|I_ARG_T_CONST, {
  FAST_CHECK_THREADS_ON_CALL();
  DO_CALL_BUILTIN(1);
});

OPCODE2(F_CALL_BUILTIN_N, "call builtin N", I_UPDATE_ALL|I_ARG_T_CONST, {
  FAST_CHECK_THREADS_ON_CALL();
  DO_CALL_BUILTIN(arg2);
});

OPCODE2( F_APPLY_N, "apply N", I_UPDATE_ALL|I_ARG_T_CONST, {
   mega_apply( APPLY_SVALUE_STRICT, arg2, &((Pike_fp->context->prog->constants + arg1)->sval), 0 );
});

OPCODE1(F_CALL_BUILTIN1_AND_POP, "call builtin1 & pop",
        I_UPDATE_ALL|I_ARG_T_CONST, {
  FAST_CHECK_THREADS_ON_CALL();
  DO_CALL_BUILTIN(1);
  pop_stack();
});

OPCODE1(F_LTOSVAL_CALL_BUILTIN_AND_ASSIGN, "ltosval, call builtin & assign",
        I_UPDATE_ALL|I_ARG_T_CONST, {
  INT32 args = (INT32)(Pike_sp - *--Pike_mark_sp);
  ONERROR uwp;

  /* Give other threads a chance to run now, before we temporarily
   * clear the svalue, in case another thread looks at it. */
  FAST_CHECK_THREADS_ON_CALL();

  /* FIXME: Assert that args > 0 */

  STACK_LEVEL_START(args+2);

  free_svalue(Pike_sp-args);
  mark_free_svalue (Pike_sp - args);
  lvalue_to_svalue_no_free(Pike_sp-args, Pike_sp-args-2);
  /* This is so that foo = efun(foo,...) (and similar things) will be faster.
   * It's done by freeing the old reference to foo after it has been
   * pushed on the stack. That way foo can have only 1 reference if we
   * are lucky, and then the low array/multiset/mapping manipulation
   * routines can be destructive if they like.
   */
  if( (1 << TYPEOF(Pike_sp[-args])) &
      (BIT_ARRAY | BIT_MULTISET | BIT_MAPPING | BIT_STRING) )
  {
    struct svalue tmp;
    SET_SVAL(tmp, PIKE_T_INT, NUMBER_NUMBER, integer, 0);
    assign_lvalue(Pike_sp-args-2, &tmp);
  }
  /* NOTE: Pike_sp-args-2 is the lvalue, Pike_sp-args is the original value.
   *       If an error gets thrown, the original value will thus be restored.
   *       If the efun succeeds, Pike_sp-args will hold the result.
   */
  SET_ONERROR(uwp, o_assign_lvalue, Pike_sp-args-2);
  DO_CALL_BUILTIN(args);
  STACK_LEVEL_CHECK(3);
  CALL_AND_UNSET_ONERROR(uwp);

  STACK_LEVEL_CHECK(3);
  free_svalue(Pike_sp-3);
  free_svalue(Pike_sp-2);
  move_svalue(Pike_sp - 3, Pike_sp - 1);
  Pike_sp-=2;
  STACK_LEVEL_DONE(1);
});

OPCODE1(F_LTOSVAL_CALL_BUILTIN_AND_ASSIGN_POP,
        "ltosval, call builtin, assign & pop", I_UPDATE_ALL|I_ARG_T_CONST, {
  INT32 args = (INT32)(Pike_sp - *--Pike_mark_sp);
  ONERROR uwp;

  /* Give other threads a chance to run now, before we temporarily
   * clear the svalue, in case another thread looks at it. */
  FAST_CHECK_THREADS_ON_CALL();

  /* FIXME: Assert that args > 0 */

  STACK_LEVEL_START(args+2);

  free_svalue(Pike_sp-args);
  mark_free_svalue (Pike_sp - args);
  lvalue_to_svalue_no_free(Pike_sp-args, Pike_sp-args-2);
  /* This is so that foo = efun(foo,...) (and similar things) will be faster.
   * It's done by freeing the old reference to foo after it has been
   * pushed on the stack. That way foo can have only 1 reference if we
   * are lucky, and then the low array/multiset/mapping manipulation
   * routines can be destructive if they like.
   */
  if( (1 << TYPEOF(Pike_sp[-args])) &
      (BIT_ARRAY | BIT_MULTISET | BIT_MAPPING | BIT_STRING) )
  {
   struct svalue tmp;
    SET_SVAL(tmp, PIKE_T_INT, NUMBER_NUMBER, integer, 0);
    assign_lvalue(Pike_sp-args-2, &tmp);
  }
  /* NOTE: Pike_sp-args-2 is the lvalue, Pike_sp-args is the original value.
   *       If an error gets thrown, the original value will thus be restored.
   *       If the efun succeeds, Pike_sp-args will hold the result.
   */
  SET_ONERROR(uwp, o_assign_lvalue, Pike_sp-args-2);
  DO_CALL_BUILTIN(args);
  STACK_LEVEL_CHECK(3);
  CALL_AND_UNSET_ONERROR(uwp);

  pop_n_elems (3);
  STACK_LEVEL_DONE (0);
});

#ifndef ENTRY_PROLOGUE_SIZE
#define ENTRY_PROLOGUE_SIZE 0
#endif /* !ENTRY_PROLOGUE_SIZE */

#define DO_RECUR(XFLAGS) do{						   \
  PIKE_OPCODE_T *addr;							   \
  struct pike_frame *new_frame;                                            \
  INT32 args = (INT32)(Pike_sp - *--Pike_mark_sp);                         \
									   \
  FAST_CHECK_THREADS_ON_CALL();						   \
  check_stack(256);							   \
									   \
  new_frame=alloc_pike_frame();						   \
  new_frame->next=Pike_fp;						   \
									   \
  JUMP_SET_TO_PC_AT_NEXT (addr);                                           \
  Pike_fp->return_addr = (PIKE_OPCODE_T *)(((INT32 *) addr) + 1);          \
  addr += GET_JUMP();                                                      \
									   \
  addr += ENTRY_PROLOGUE_SIZE;						   \
                                                                           \
  new_frame->args = args;                                                  \
  new_frame->save_sp = new_frame->locals = Pike_sp - args;		   \
  new_frame->save_mark_sp = Pike_mark_sp;	                           \
  DO_IF_DEBUG(new_frame->num_args=0;new_frame->num_locals=0;);             \
  SET_PROG_COUNTER(addr);						   \
  new_frame->fun=Pike_fp->fun;						   \
  DO_IF_PROFILING( new_frame->ident=Pike_fp->ident );			   \
  new_frame->current_storage=Pike_fp->current_storage;                     \
  if(Pike_fp->scope) add_ref(new_frame->scope=Pike_fp->scope);		   \
  add_ref(new_frame->current_object = Pike_fp->current_object);		   \
  add_ref(new_frame->current_program = Pike_fp->current_program);	   \
  new_frame->context = Pike_fp->context;				   \
									   \
  DO_IF_PROFILING({							   \
      struct identifier *func;						   \
      new_frame->start_time =						   \
	get_cpu_time() - Pike_interpreter.unlocked_time;		   \
      new_frame->ident = Pike_fp->ident;				   \
      new_frame->children_base = Pike_interpreter.accounted_time;	   \
      func = new_frame->context->identifiers_prog->identifiers +           \
        new_frame->context->identifiers_offset + new_frame->ident;         \
      func->num_calls++;						   \
      func->recur_depth++;						   \
      W_PROFILING_DEBUG("%p{: Push at %" PRINT_CPU_TIME                    \
                        " %" PRINT_CPU_TIME "\n",                          \
                        Pike_interpreter.thread_state,                     \
                        new_frame->start_time,                             \
                        new_frame->children_base);                         \
    });									   \
									   \
  Pike_fp=new_frame;							   \
  new_frame->flags=PIKE_FRAME_RETURN_INTERNAL | XFLAGS;			   \
									   \
  if (UNLIKELY(Pike_interpreter.trace_level > 3)) {                        \
    fprintf(stderr, "-    Addr = 0x%lx\n", (unsigned long) addr);	   \
  }									   \
									   \
  FETCH;								   \
  JUMP_DONE;								   \
}while(0)

/* Assume that the number of arguments is correct */
OPCODE1_PTRJUMP(F_COND_RECUR, "recur if not overloaded",
                I_UPDATE_ALL|I_ARG_T_GLOBAL, {
  PIKE_OPCODE_T *addr;
  struct program *p;
  p = Pike_fp->current_program;
  JUMP_SET_TO_PC_AT_NEXT (addr);
  Pike_fp->return_addr = (PIKE_OPCODE_T *)(((INT32 *)addr) + 1);

  /* Test if the function is overloaded.
   *
   * Note: The second part of the test is sufficient, but
   *       since the first case is much simpler to test and
   *       is common, it should offer a speed improvement.
   *
   *	/grubba 2002-11-14
   *
   * Also test if the function uses scoping. DO_RECUR() doesn't
   * adjust fp->expendible which will make eg RETURN_LOCAL fail.
   *
   *	/grubba 2003-03-25
   */
  if(((p != Pike_fp->context->prog) ||
      (Pike_fp->context !=
       &p->inherits[p->identifier_references[Pike_fp->context->identifier_level +
					     arg1].inherit_offset])) ||
     (ID_FROM_INT(p, arg1+Pike_fp->context->identifier_level)->
      identifier_flags & IDENTIFIER_SCOPE_USED))
  {
    PIKE_OPCODE_T *faddr;
    PIKE_OPCODE_T *addr2;
    INT32 args = (INT32)(Pike_sp - *--Pike_mark_sp);
    JUMP_SET_TO_PC_AT_NEXT (faddr);
    faddr += GET_JUMP();

    if((addr2 = lower_mega_apply(args,
                               Pike_fp->current_object,
                                 (arg1+Pike_fp->context->identifier_level))))
    {
      Pike_fp->flags |= PIKE_FRAME_RETURN_INTERNAL;
      addr = addr2;
    }
    DO_JUMP_TO(addr);
  }

  /* FALLTHRU */

  /* Assume that the number of arguments is correct */

  OPCODE0_TAILPTRJUMP(F_RECUR, "recur", I_UPDATE_ALL, {
    DO_RECUR(0);
  });
});

/* Ugly code duplication */
OPCODE0_PTRJUMP(F_RECUR_AND_POP, "recur & pop", I_UPDATE_ALL, {
  DO_RECUR(PIKE_FRAME_RETURN_POP);
});


/* Assume that the number of arguments is correct */
/* FIXME: adjust Pike_mark_sp */
OPCODE0_PTRJUMP(F_TAIL_RECUR, "tail recursion", I_UPDATE_ALL, {
  PIKE_OPCODE_T *addr;
  INT32 args = (INT32)(Pike_sp - *--Pike_mark_sp);

  FAST_CHECK_THREADS_ON_CALL();

  JUMP_SET_TO_PC_AT_NEXT (addr);
  addr += GET_JUMP();
  addr += ENTRY_PROLOGUE_SIZE;
  SET_PROG_COUNTER(addr);

  /* FIXME: What about MALLOCED_LOCALS? */
  if(Pike_sp-args != Pike_fp->locals)
  {
    DO_IF_DEBUG({
      if (Pike_sp < Pike_fp->locals + args)
	Pike_fatal("Pike_sp (%p) < Pike_fp->locals (%p) + args (%d)\n",
	      Pike_sp, Pike_fp->locals, args);
    });
    assign_svalues(Pike_fp->locals, Pike_sp-args, args, BIT_MIXED);
    pop_n_elems(Pike_sp - (Pike_fp->locals + args));
  }

  FETCH;
  JUMP_DONE;
});

OPCODE1(F_THIS_OBJECT, "this_object", I_UPDATE_SP, {
    int level;
    struct object *o;
    o = Pike_fp->current_object;
    for (level = 0; level < arg1; level++) {
      struct program *p;
      p = o->prog;
      if (!p)
	Pike_error ("Object %d level(s) up is destructed - cannot get the parent.\n",
		    level);
      if (!(p->flags & PROGRAM_USES_PARENT))
	/* FIXME: Ought to write out the object here. */
	Pike_error ("Object %d level(s) up lacks parent reference.\n", level);
      o = PARENT_INFO(o)->parent;
    }
    ref_push_object(o);
  });

OPCODE0(F_UNDEFINEDP,"undefinedp",0, {
    int undef;
    if(TYPEOF(Pike_sp[-1]) != T_INT)
    {
      pop_stack();
      push_int(0);
    }
    else
    {
      undef = SUBTYPEOF(Pike_sp[-1]) == NUMBER_UNDEFINED;
      SET_SVAL(Pike_sp[-1], T_INT, NUMBER_NUMBER, integer,
	       undef);
    }
});

OPCODE0(F_DESTRUCTEDP,"destructedp",0, {
  if(TYPEOF(Pike_sp[-1]) != T_INT)
  {
    if(IS_DESTRUCTED(Pike_sp-1))
    {
      pop_stack();
      push_int(1);
    }else{
      pop_stack();
      push_int(0);
    }
  }else{
    SET_SVAL(Pike_sp[-1], T_INT, NUMBER_NUMBER, integer,
	     NUMBER_DESTRUCTED == SUBTYPEOF(Pike_sp[-1]));
  }
});

OPCODE0(F_ZERO_TYPE, "zero_type", 0, {
  if(TYPEOF(Pike_sp[-1]) != T_INT)
  {
    if(IS_DESTRUCTED(Pike_sp-1))
    {
      pop_stack();
      push_int(NUMBER_DESTRUCTED);
    }else{
      pop_stack();
      push_int(0);
    }
  }else{
    SET_SVAL(Pike_sp[-1], T_INT, NUMBER_NUMBER, integer,
	     SUBTYPEOF(Pike_sp[-1]));
  }
});

/* Swap the stack top with the element arg1 positions below. */
OPCODE1(F_SWAP, "swap", 0, {
  if (!arg1) {
    stack_swap();
  } else {
    swap_svalues(Pike_sp - (arg1 + 2), Pike_sp - 1);
  }
});

/* Duplicate the element arg1 positions down on the stack. */
OPCODE1(F_DUP, "dup", I_UPDATE_SP, {
  push_svalue(Pike_sp - (arg1 + 1));
});

OPCODE2(F_THIS, "this", I_UPDATE_SP, {
    struct external_variable_context loc;

    loc.o = Pike_fp->current_object;
    loc.parent_identifier = Pike_fp->fun;
    loc.inherit = Pike_fp->context;
    find_external_context(&loc, arg1);

    DO_IF_DEBUG({
      TRACE((5,"-   Identifier=%d Offset=%d\n",
	     arg1,
	     loc.inherit->identifier_level));
    });
    if ((arg2 < 0) || !loc.o->prog) {
      ref_push_object(loc.o);
    } else {
      ref_push_object_inherit(loc.o,
			      (loc.inherit - loc.o->prog->inherits) + arg2);
    }
    print_return_value();
});

OPCODE2(F_MAGIC_TYPES, "::_types", I_UPDATE_SP, {
  push_magic_index(magic_types_program, arg2, arg1);
});

OPCODE2(F_INIT_FRAME, "init_frame", 0, {
    Pike_fp->num_args = arg1;
    Pike_fp->num_locals = arg2;
  });

/* Save local variables according to bitmask. The high 16 bits of arg1
   is an offset, the low 16 bits is a bitmask for that offset. See doc
   for pike_frame.save_locals_bitmask. */
OPCODE1(F_SAVE_LOCALS, "save_locals", 0, {
    unsigned INT16 offset = ((arg1 & 0xFFFF0000) >> 16);
    unsigned INT16 mask = arg1 & 0xFFFF;
    if (!(Pike_fp->flags & PIKE_FRAME_SAVE_LOCALS)) {
      size_t num_ints = (Pike_fp->num_locals >> 4) + 1;
      size_t num_bytes = num_ints * sizeof(unsigned INT16);
      Pike_fp->save_locals_bitmask = (unsigned INT16*)xalloc(num_bytes);
      memset(Pike_fp->save_locals_bitmask, 0, num_bytes);
      Pike_fp->flags |= PIKE_FRAME_SAVE_LOCALS;
    }
    *(Pike_fp->save_locals_bitmask + (ptrdiff_t)offset) = mask;
  });

OPCODE2(F_FILL_STACK, "fill_stack", I_UPDATE_SP, {
    if (!(Pike_fp->flags & PIKE_FRAME_MALLOCED_LOCALS)) {
      INT32 tmp = (Pike_fp->locals + arg1) - Pike_sp;
      if (tmp > 0) {
	if (arg2) {
	  push_undefines(tmp);
	} else {
	  push_zeroes(tmp);
	}
      }
    }
  });

OPCODE1(F_MARK_AT, "mark_at", I_UPDATE_SP, {
    /* FIXME: What about MALLOCED_LOCALS? */
    if (Pike_fp->flags & PIKE_FRAME_MALLOCED_LOCALS) {
      *(Pike_mark_sp++) = frame_get_save_sp(Pike_fp) + arg1;
    } else {
      *(Pike_mark_sp++) = Pike_fp->locals + arg1;
    }
  });

OPCODE2(F_MAGIC_ANNOTATIONS, "::_annotations", I_UPDATE_SP, {
  push_magic_index(magic_annotations_program, arg2, arg1);
});

/* Swap the top stack element and a local variable. */
OPCODE1(F_SWAP_STACK_LOCAL, "swap_stack_local", I_ARG_T_LOCAL, {
    struct svalue tmp = Pike_fp->locals[arg1];
    Pike_fp->locals[arg1] = Pike_sp[-1];
    Pike_sp[-1] = tmp;
  });

/* Assign the first stack value to the local if it is equal
 * to the second atomically.
 */
OPCODE1(F_TEST_AND_SET_LOCAL, "test_and_set_local", I_ARG_T_LOCAL|I_UPDATE_SP, {
    if (is_eq(Pike_sp-1, Pike_fp->locals + arg1)) {
      assign_svalue(Pike_fp->locals + arg1, Pike_sp-2);
    }
    pop_stack();
    pop_stack();
  });

OPCODE1(F_GENERATOR, "generator", I_ARG_T_GLOBAL, {
    save_locals(Pike_fp);
    Pike_fp->fun = arg1 + Pike_fp->context->identifier_level;
  });

OPCODE1(F_RESTORE_STACK_FROM_LOCAL, "restore_stack",
        I_UPDATE_SP|I_ARG_T_LOCAL, {
    struct array *a;

    DO_IF_DEBUG(
                if (!(Pike_fp->flags & PIKE_FRAME_MALLOCED_LOCALS)) {
                  Pike_fatal("Invalid frame. Lost context?\n");
                }
                );

    if(UNLIKELY(TYPEOF(Pike_fp->locals[arg1]) != PIKE_T_ARRAY))
    {
      free_svalue(&Pike_fp->locals[arg1]);
      SET_SVAL(Pike_fp->locals[arg1], PIKE_T_ARRAY, 0, array, &empty_array);
      add_ref(&empty_array);
    } else {
      a = Pike_fp->locals[arg1].u.array;
      if (a->size) {
	struct svalue *save_sp = Pike_sp;
	struct svalue *s = ITEM(a);
	memcpy(Pike_sp, s, sizeof(struct svalue) * a->size);
	DO_IF_DEBUG(memset(s, 0, sizeof(struct svalue) * a->size));
	Pike_sp += a->size;
	a->size = 0;

	/* The last element from the array is the marks (if any). */
	if (TYPEOF(Pike_sp[-1]) == PIKE_T_ARRAY) {
	  ptrdiff_t i;
	  struct array *mark_a = Pike_sp[-1].u.array;
	  struct svalue *s = ITEM(mark_a);
	  for (i = 0; i < mark_a->size; i++) {
	    *(Pike_mark_sp++) = save_sp + s[i].u.integer;
	  }
	}
	pop_stack();
      }
    }
  });

OPCODE2(F_SAVE_STACK_TO_LOCAL, "save_stack", I_ARG_T_LOCAL, {
    struct svalue *save_sp = frame_get_save_sp(Pike_fp);
    ptrdiff_t len = Pike_sp - save_sp - arg2;
    struct svalue **save_mark_sp = Pike_fp->save_mark_sp;
    ptrdiff_t mark_len = Pike_mark_sp - save_mark_sp;

    if (mark_len || (len > 0)) {
      struct array *a;
      struct svalue *s;
      ptrdiff_t i;

      if (mark_len) {
	a = allocate_array(mark_len);
	s = ITEM(a);
	for (i = 0; i < mark_len; i++) {
	  SET_SVAL(s[i], PIKE_T_INT, NUMBER_NUMBER, integer,
		   (save_mark_sp[i] - save_sp));
	}
	push_array(a);
      } else {
	push_undefined();
      }
      len++;

      if ((TYPEOF(Pike_fp->locals[arg1]) == PIKE_T_ARRAY) &&
	  !(a = Pike_fp->locals[arg1].u.array)->size &&
	  (a->malloced_size >= len)) {
	/* Reuse the already allocated (and emptied) array. */
	a->size = len;
	a->type_field = BIT_MIXED | BIT_UNFINISHED;
	a->item = a->u.real_item;
      } else {
	/* Allocate a new array. */
	a = allocate_array(len);
	free_svalue(Pike_fp->locals + arg1);
	SET_SVAL(Pike_fp->locals[arg1], PIKE_T_ARRAY, 0, array, a);
      }

      s = ITEM(a);
      for(i = 0; i < len-1; i++) {
	assign_svalue_no_free(s + i, save_sp + i);
      }
      assign_svalue_no_free(s + i, Pike_sp-1);

      pop_stack();
    }
  });

OPCODE1(F_PUSH_CATCHES, "push_catches", I_UPDATE_SP, {
    struct catch_context *cc = Pike_interpreter.catch_ctx;
    while (arg1 > 0) {
      if (!cc) {
	Pike_error("PUSH_CATCHES: Lost track of catch.\n");
      }
      push_int(Pike_interpreter.evaluator_stack + cc->recovery.stack_pointer -
	       frame_get_save_sp(Pike_fp));
      push_int(Pike_interpreter.mark_stack + cc->recovery.mark_sp -
	       Pike_fp->save_mark_sp);
      cc = cc->prev;
      arg1--;
    }
  });

/* Ideally this ought to be an OPCODE0_PTRRETURN but I don't fancy
 * adding that variety to this macro hell. At the end of the day there
 * wouldn't be any difference anyway afaics. /mast */
OPCODE0_PTRJUMP(F_CATCH_AT, "catch_at", I_UPDATE_ALL|I_RETURN, {
  PIKE_OPCODE_T *addr;

  {
    struct catch_context *new_catch_ctx;

    if ((TYPEOF(Pike_sp[-1]) != PIKE_T_INT) ||
	(TYPEOF(Pike_sp[-2]) != PIKE_T_INT)) {
      Pike_error("CATCH_AT: Invalid arguments.\n");
    }

    new_catch_ctx = alloc_catch_context();
    DO_IF_REAL_DEBUG (
      new_catch_ctx->frame = Pike_fp;
      init_recovery (&new_catch_ctx->recovery, 0, 0, PERR_LOCATION());
    );
    DO_IF_NOT_REAL_DEBUG (
      init_recovery (&new_catch_ctx->recovery, 0);
    );

    new_catch_ctx->recovery.stack_pointer =
      frame_get_save_sp(Pike_fp) + Pike_sp[-2].u.integer -
      Pike_interpreter.evaluator_stack;
    new_catch_ctx->recovery.mark_sp =
      Pike_fp->save_mark_sp + Pike_sp[-1].u.integer -
      Pike_interpreter.mark_stack;
    Pike_sp -= 2;

    JUMP_SET_TO_PC_AT_NEXT (addr);
    new_catch_ctx->continue_reladdr = GET_JUMP()
      /* We need to run the entry prologue... */
      - ENTRY_PROLOGUE_SIZE;

    new_catch_ctx->next_addr = addr;
    new_catch_ctx->prev = Pike_interpreter.catch_ctx;
    Pike_interpreter.catch_ctx = new_catch_ctx;
    DO_IF_DEBUG({
	TRACE((3,"-   Pushed catch context %p\n", new_catch_ctx));
      });
  }

  /* Need to adjust next_addr by sizeof(INT32) to skip past the jump
   * address to the continue position after the catch block. */
  addr = (PIKE_OPCODE_T *) ((INT32 *) addr + 1);

  if (Pike_interpreter.catching_eval_jmpbuf) {
    /* There's already a catching_eval_instruction around our
     * eval_instruction, so we can just continue. */
    debug_malloc_touch_named (Pike_interpreter.catch_ctx, "(1)");
    /* Skip past the entry prologue... */
    addr += ENTRY_PROLOGUE_SIZE;
    SET_PROG_COUNTER(addr);
    FETCH;
    DO_IF_DEBUG({
	TRACE((3,"-   In active catch; continuing at %p\n", addr));
      });
    JUMP_DONE;
  }

  else {
    debug_malloc_touch_named (Pike_interpreter.catch_ctx, "(2)");

    while (1) {
      /* Loop here every time an exception is caught. Once we've
       * gotten here and set things up to run eval_instruction from
       * inside catching_eval_instruction, we keep doing it until it's
       * time to return. */

      int res;

      DO_IF_DEBUG({
	  TRACE((3,"-   Activating catch; calling %p in context %p\n",
		 addr, Pike_interpreter.catch_ctx));
	});

      res = catching_eval_instruction (addr);

      DO_IF_DEBUG({
	  TRACE((3,"-   catching_eval_instruction(%p) returned %d\n",
		 addr, res));
	});

      if (res != -3) {
	/* There was an inter return inside the evaluated code. Just
	 * propagate it. */
	DO_IF_DEBUG ({
	    TRACE((3,"-   Returning from catch.\n"));
	    if (res != -1) Pike_fatal ("Unexpected return value from "
				       "catching_eval_instruction: %d\n", res);
	  });
	break;
      }

      else {
	/* Caught an exception. */
	struct catch_context *cc = Pike_interpreter.catch_ctx;

	DO_IF_DEBUG ({
	    TRACE((3,"-   Caught exception. catch context: %p\n", cc));
	    if (!cc) Pike_fatal ("Catch context dropoff.\n");
	    if (cc->frame != Pike_fp)
	      Pike_fatal ("Catch context doesn't belong to this frame.\n");
	  });

	debug_malloc_touch_named (cc, "(3)");
	UNSETJMP (cc->recovery);
	move_svalue (Pike_sp++, &throw_value);
	mark_free_svalue (&throw_value);
	low_destruct_objects_to_destruct();

	if (cc->continue_reladdr < 0)
	  FAST_CHECK_THREADS_ON_BRANCH();
	addr = cc->next_addr + cc->continue_reladdr;

	DO_IF_DEBUG({
	    TRACE((3,"-   Popping catch context %p ==> %p\n",
		   cc, cc->prev));
	    if (!addr) Pike_fatal ("Unexpected null continue addr.\n");
	  });

	Pike_interpreter.catch_ctx = cc->prev;
	really_free_catch_context (cc);
      }
    }

    INTER_RETURN;
  }
});

OPCODE0(F_ATOMIC_GET_SET, "?=", I_UPDATE_SP, {
  atomic_get_set_lvalue(Pike_sp-3, Pike_sp-1);
  free_svalue(Pike_sp-3);
  free_svalue(Pike_sp-2);
  move_svalue(Pike_sp - 3, Pike_sp - 1);
  Pike_sp-=2;
});

/* The actual raw get/set function is already on the stack,
 * so this opcode just fills the second svalue of the lvalue.
 */
OPCODE0(F_GET_SET_LVALUE, "&get/set", I_UPDATE_SP, {
    SET_SVAL_TYPE(Pike_sp[0], T_VOID);
    Pike_sp++;
  });

/*
#undef PROG_COUNTER
*/
