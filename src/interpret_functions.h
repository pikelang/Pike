/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: interpret_functions.h,v 1.130 2004/10/06 18:35:02 mast Exp $
*/

/*
 * Opcode definitions for the interpreter.
 */

#include "global.h"

#undef CJUMP
#undef AUTO_BIGNUM_LOOP_TEST
#undef LOOP
#undef COMPARISON
#undef MKAPPLY
#undef DO_CALL_BUILTIN

#undef DO_IF_BIGNUM
#ifdef AUTO_BIGNUM
#define DO_IF_BIGNUM(CODE)	CODE
#else /* !AUTO_BIGNUM */
#define DO_IF_BIGNUM(CODE)
#endif /* AUTO_BIGNUM */

#undef DO_IF_ELSE_COMPUTED_GOTO
#ifdef HAVE_COMPUTED_GOTO
#define DO_IF_ELSE_COMPUTED_GOTO(A, B)	(A)
#else /* !HAVE_COMPUTED_GOTO */
#define DO_IF_ELSE_COMPUTED_GOTO(A, B)	(B)
#endif /* HAVE_COMPUTED_GOTO */

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

#ifndef INTER_ESCAPE_CATCH
#define INTER_ESCAPE_CATCH return -2
#endif

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

#ifndef OVERRIDE_JUMPS

#undef GET_JUMP
#undef SKIPJUMP
#undef DOJUMP

#ifdef PIKE_DEBUG

#define GET_JUMP() (backlog[backlogp].arg=(\
  (t_flag>3 ? sprintf(trace_buffer, "-    Target = %+ld\n", \
                      (long)LOW_GET_JUMP()), \
              write_to_stderr(trace_buffer,strlen(trace_buffer)) : 0), \
  LOW_GET_JUMP()))

#define SKIPJUMP() (GET_JUMP(), LOW_SKIPJUMP())

#else /* !PIKE_DEBUG */

#define GET_JUMP() LOW_GET_JUMP()
#define SKIPJUMP() LOW_SKIPJUMP()

#endif /* PIKE_DEBUG */

#define DOJUMP() do { \
    INT32 tmp; \
    tmp = GET_JUMP(); \
    SET_PROG_COUNTER(PROG_COUNTER + tmp); \
    FETCH; \
    if(tmp < 0) \
      fast_check_threads_etc(6); \
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
  DONE;					\
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
    DO_IF_DEBUG(if (t_flag > 5)				\
      fprintf(stderr, "Returning to 0x%p\n",		\
	      Pike_fp->return_addr));			\
    DO_JUMP_TO(Pike_fp->return_addr);			\
  }							\
  DO_IF_DEBUG(if (t_flag > 5)				\
    fprintf(stderr, "Inter return\n"));			\
  INTER_RETURN;						\
}

#undef DO_RETURN
#ifndef PIKE_DEBUG
#define DO_RETURN DO_DUMB_RETURN
#else
#define DO_RETURN {				\
  if(d_flag>3) do_gc();				\
  if(d_flag>4) do_debug();			\
  DO_DUMB_RETURN;				\
}
#endif

#undef DO_INDEX
#define DO_INDEX do {				\
  struct svalue s;				\
  index_no_free(&s,Pike_sp-2,Pike_sp-1);	\
  pop_2_elems();				\
  *Pike_sp=s;					\
  Pike_sp++;					\
  print_return_value();				\
}while(0)


OPCODE0(F_UNDEFINED, "push UNDEFINED", 0, {
  push_int(0);
  Pike_sp[-1].subtype=NUMBER_UNDEFINED;
});

OPCODE0(F_CONST0, "push 0", 0, {
  push_int(0);
});

OPCODE0(F_CONST1, "push 1", 0, {
  push_int(1);
});


OPCODE0(F_MARK_AND_CONST0, "mark & 0", 0, {
  *(Pike_mark_sp++)=Pike_sp;
  push_int(0);
});

OPCODE0(F_MARK_AND_CONST1, "mark & 1", 0, {
  *(Pike_mark_sp++)=Pike_sp;
  push_int(1);
});

OPCODE0(F_CONST_1,"push -1", 0, {
  push_int(-1);
});

OPCODE0(F_BIGNUM, "push 0x7fffffff", 0, {
  push_int(0x7fffffff);
});

OPCODE1(F_NUMBER, "push int", 0, {
  push_int(arg1);
});

OPCODE1(F_NEG_NUMBER, "push -int", 0, {
  push_int(-arg1);
});

OPCODE1(F_CONSTANT, "constant", 0, {
  push_svalue(& Pike_fp->context.prog->constants[arg1].sval);
  print_return_value();
});


/* Generic swap instruction:
 * swaps the arg1 top values with the arg2 values beneath
 */
OPCODE2(F_REARRANGE,"rearrange",0,{
  check_stack(arg2);
  MEMCPY(Pike_sp,Pike_sp-arg1-arg2,sizeof(struct svalue)*arg2);
  MEMMOVE(Pike_sp-arg1-arg2,Pike_sp-arg1,sizeof(struct svalue)*arg1);
  MEMCPY(Pike_sp-arg2,Pike_sp,sizeof(struct svalue)*arg2);
});

/* The rest of the basic 'push value' instructions */	

OPCODE1_TAIL(F_MARK_AND_STRING, "mark & string", 0, {
  *(Pike_mark_sp++)=Pike_sp;

  OPCODE1(F_STRING, "string", 0, {
    copy_shared_string(Pike_sp->u.string,Pike_fp->context.prog->strings[arg1]);
    Pike_sp->type=PIKE_T_STRING;
    Pike_sp->subtype=0;
    Pike_sp++;
    print_return_value();
  });
});


OPCODE1(F_ARROW_STRING, "->string", 0, {
  copy_shared_string(Pike_sp->u.string,Pike_fp->context.prog->strings[arg1]);
  Pike_sp->type=PIKE_T_STRING;
  Pike_sp->subtype=1; /* Magic */
  Pike_sp++;
  print_return_value();
});

OPCODE1(F_LOOKUP_LFUN, "->lfun", 0, {
  struct svalue tmp;
  struct object *o;

  if ((Pike_sp[-1].type == T_OBJECT) && ((o = Pike_sp[-1].u.object)->prog) &&
      (FIND_LFUN(o->prog, LFUN_ARROW) == -1)) {
    int id = FIND_LFUN(o->prog, arg1);
    if ((id != -1) &&
	(!(o->prog->identifier_references[id].id_flags &
	   (ID_STATIC|ID_PRIVATE|ID_HIDDEN)))) {
      low_object_index_no_free(&tmp, o, id);
    } else {
      /* Not found. */
      tmp.type = T_INT;
      tmp.subtype = 1;
      tmp.u.integer = 0;
    }
  } else {
    struct svalue tmp2;
    tmp2.type = PIKE_T_STRING;
    tmp2.u.string = lfun_strings[arg1];
    tmp2.subtype = 1;
    index_no_free(&tmp, Pike_sp-1, &tmp2);
  }
  free_svalue(Pike_sp-1);
  Pike_sp[-1] = tmp;
  print_return_value();
});

OPCODE0(F_FLOAT, "push float", 0, {
  /* FIXME, this opcode uses 'PROG_COUNTER' which is not allowed.. */
  MEMCPY((void *)&Pike_sp->u.float_number, PROG_COUNTER, sizeof(FLOAT_TYPE));
  Pike_sp->type=PIKE_T_FLOAT;
  Pike_sp++;
  DO_JUMP_TO((PIKE_OPCODE_T *)(((FLOAT_TYPE *)PROG_COUNTER) + 1));
});

OPCODE1(F_LFUN, "local function", 0, {
  Pike_sp->u.object=Pike_fp->current_object;
  add_ref(Pike_fp->current_object);
  Pike_sp->subtype=arg1+Pike_fp->context.identifier_level;
  Pike_sp->type=PIKE_T_FUNCTION;
  Pike_sp++;
  print_return_value();
});

OPCODE2(F_TRAMPOLINE, "trampoline", 0, {
  struct object *o=low_clone(pike_trampoline_program);
  struct pike_frame *f=Pike_fp;
  DO_IF_DEBUG(INT32 arg2_ = arg2);

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
  ((struct pike_trampoline *)(o->storage))->func=arg1+Pike_fp->context.identifier_level;
  push_object(o);
  /* Make it look like a function. */
  Pike_sp[-1].subtype = pike_trampoline_program->lfuns[LFUN_CALL];
  Pike_sp[-1].type = T_FUNCTION;
  print_return_value();
});

/* The not so basic 'push value' instructions */

OPCODE1_TAIL(F_MARK_AND_GLOBAL, "mark & global", 0, {
  *(Pike_mark_sp++)=Pike_sp;

  OPCODE1(F_GLOBAL, "global", 0, {
    low_object_index_no_free(Pike_sp,
			     Pike_fp->current_object,
			     arg1 + Pike_fp->context.identifier_level);
    Pike_sp++;
    print_return_value();
  });
});

OPCODE2_TAIL(F_MARK_AND_EXTERNAL, "mark & external", 0, {
  *(Pike_mark_sp++)=Pike_sp;

  OPCODE2(F_EXTERNAL,"external", 0, {
    struct external_variable_context loc;

    loc.o=Pike_fp->current_object;
    loc.parent_identifier=Pike_fp->fun;
    if (loc.o->prog)
      loc.inherit=INHERIT_FROM_INT(loc.o->prog, loc.parent_identifier);
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


OPCODE2(F_EXTERNAL_LVALUE, "& external", 0, {
  struct external_variable_context loc;

  loc.o=Pike_fp->current_object;
  loc.parent_identifier=Pike_fp->fun;
  if (loc.o->prog)
    loc.inherit=INHERIT_FROM_INT(loc.o->prog, loc.parent_identifier);
  find_external_context(&loc, arg2);

  if (!loc.o->prog)
    Pike_error ("Cannot access variable in destructed parent object.\n");

  DO_IF_DEBUG({
    TRACE((5,"-   Identifier=%d Offset=%d\n",
	   arg1,
	   loc.inherit->identifier_level));
  });

  ref_push_object(loc.o);
  Pike_sp->type=T_LVALUE;
  Pike_sp->u.integer=arg1 + loc.inherit->identifier_level;
  Pike_sp++;
});

OPCODE1(F_MARK_AND_LOCAL, "mark & local", 0, {
  *(Pike_mark_sp++) = Pike_sp;
  push_svalue( Pike_fp->locals + arg1);
  print_return_value();
});

OPCODE1(F_LOCAL, "local", 0, {
  push_svalue( Pike_fp->locals + arg1);
  print_return_value();
});

OPCODE2(F_2_LOCALS, "2 locals", 0, {
  push_svalue( Pike_fp->locals + arg1);
  print_return_value();
  push_svalue( Pike_fp->locals + arg2);
  print_return_value();
});

OPCODE2(F_LOCAL_2_LOCAL, "local = local", 0, {
  assign_svalue(Pike_fp->locals + arg1, Pike_fp->locals + arg2);
});

OPCODE2(F_LOCAL_2_GLOBAL, "global = local", 0, {
  INT32 tmp = arg1 + Pike_fp->context.identifier_level;
  struct identifier *i;

  if(!Pike_fp->current_object->prog)
    Pike_error("Cannot access global variables in destructed object.\n");

  i = ID_FROM_INT(Pike_fp->current_object->prog, tmp);
  if(!IDENTIFIER_IS_VARIABLE(i->identifier_flags))
    Pike_error("Cannot assign functions or constants.\n");
  if(i->run_time_type == PIKE_T_MIXED)
  {
    assign_svalue((struct svalue *)GLOBAL_FROM_INT(tmp),
		  Pike_fp->locals + arg2);
  }else{
    assign_to_short_svalue((union anything *)GLOBAL_FROM_INT(tmp),
			   i->run_time_type,
			   Pike_fp->locals + arg2);
  }
});

OPCODE2(F_GLOBAL_2_LOCAL, "local = global", 0, {
  INT32 tmp = arg1 + Pike_fp->context.identifier_level;
  free_svalue(Pike_fp->locals + arg2);
  low_object_index_no_free(Pike_fp->locals + arg2,
			   Pike_fp->current_object,
			   tmp);
});

OPCODE1(F_LOCAL_LVALUE, "& local", 0, {
  Pike_sp[0].type = T_LVALUE;
  Pike_sp[0].u.lval = Pike_fp->locals + arg1;
  Pike_sp[1].type = T_VOID;
  Pike_sp += 2;
});

OPCODE2(F_LEXICAL_LOCAL, "lexical local", 0, {
  struct pike_frame *f=Pike_fp;
  while(arg2--)
  {
    f=f->scope;
    if(!f) Pike_error("Lexical scope error.\n");
  }
  push_svalue(f->locals + arg1);
  print_return_value();
});

OPCODE2(F_LEXICAL_LOCAL_LVALUE, "&lexical local", 0, {
  struct pike_frame *f=Pike_fp;
  while(arg2--)
  {
    f=f->scope;
    if(!f) Pike_error("Lexical scope error.\n");
  }
  Pike_sp[0].type=T_LVALUE;
  Pike_sp[0].u.lval=f->locals+arg1;
  Pike_sp[1].type=T_VOID;
  Pike_sp+=2;
});

OPCODE1(F_ARRAY_LVALUE, "[ lvalues ]", 0, {
  f_aggregate(arg1*2);
  Pike_sp[-1].u.array->flags |= ARRAY_LVALUE;
  Pike_sp[-1].u.array->type_field |= BIT_UNFINISHED | BIT_MIXED;
  /* FIXME: Shouldn't a ref be added here? */
  Pike_sp[0] = Pike_sp[-1];
  Pike_sp[-1].type = T_ARRAY_LVALUE;
  dmalloc_touch_svalue(Pike_sp);
  Pike_sp++;
});

OPCODE1(F_CLEAR_2_LOCAL, "clear 2 local", 0, {
  free_mixed_svalues(Pike_fp->locals + arg1, 2);
  Pike_fp->locals[arg1].type = PIKE_T_INT;
  Pike_fp->locals[arg1].subtype = 0;
  Pike_fp->locals[arg1].u.integer = 0;
  Pike_fp->locals[arg1+1].type = PIKE_T_INT;
  Pike_fp->locals[arg1+1].subtype = 0;
  Pike_fp->locals[arg1+1].u.integer = 0;
});

OPCODE1(F_CLEAR_4_LOCAL, "clear 4 local", 0, {
  int e;
  free_mixed_svalues(Pike_fp->locals + arg1, 4);
  for(e = 0; e < 4; e++)
  {
    Pike_fp->locals[arg1+e].type = PIKE_T_INT;
    Pike_fp->locals[arg1+e].subtype = 0;
    Pike_fp->locals[arg1+e].u.integer = 0;
  }
});

OPCODE1(F_CLEAR_LOCAL, "clear local", 0, {
  free_svalue(Pike_fp->locals + arg1);
  Pike_fp->locals[arg1].type = PIKE_T_INT;
  Pike_fp->locals[arg1].subtype = 0;
  Pike_fp->locals[arg1].u.integer = 0;
});

OPCODE1(F_INC_LOCAL, "++local", 0, {
  if( (Pike_fp->locals[arg1].type == PIKE_T_INT)
      DO_IF_BIGNUM(
      && (!INT_TYPE_ADD_OVERFLOW(Pike_fp->locals[arg1].u.integer, 1))
      )
      )
  {
    push_int(++(Pike_fp->locals[arg1].u.integer));
    Pike_fp->locals[arg1].subtype = NUMBER_NUMBER; /* Could have UNDEFINED there before. */
  } else {
    push_svalue(Pike_fp->locals+arg1);
    push_int(1);
    f_add(2);
    assign_svalue(Pike_fp->locals+arg1,Pike_sp-1);
  }
});

OPCODE1(F_POST_INC_LOCAL, "local++", 0, {
  push_svalue( Pike_fp->locals + arg1);

  if( (Pike_fp->locals[arg1].type == PIKE_T_INT)
      DO_IF_BIGNUM(
      && (!INT_TYPE_ADD_OVERFLOW(Pike_fp->locals[arg1].u.integer, 1))
      )
      )
  {
    Pike_fp->locals[arg1].u.integer++;
    Pike_fp->locals[arg1].subtype = NUMBER_NUMBER; /* Could have UNDEFINED there before. */
  } else {
    push_svalue(Pike_fp->locals + arg1);
    push_int(1);
    f_add(2);
    stack_pop_to(Pike_fp->locals + arg1);
  }
});

OPCODE1(F_INC_LOCAL_AND_POP, "++local and pop", 0, {
  if( (Pike_fp->locals[arg1].type == PIKE_T_INT)
      DO_IF_BIGNUM(
      && (!INT_TYPE_ADD_OVERFLOW(Pike_fp->locals[arg1].u.integer, 1))
      )
      )
  {
    Pike_fp->locals[arg1].u.integer++;
    Pike_fp->locals[arg1].subtype = NUMBER_NUMBER; /* Could have UNDEFINED there before. */
  } else {
    push_svalue( Pike_fp->locals + arg1);
    push_int(1);
    f_add(2);
    stack_pop_to(Pike_fp->locals + arg1);
  }
});

OPCODE1(F_DEC_LOCAL, "--local", 0, {
  if( (Pike_fp->locals[arg1].type == PIKE_T_INT)
      DO_IF_BIGNUM(
      && (!INT_TYPE_SUB_OVERFLOW(Pike_fp->locals[arg1].u.integer, 1))
      )
      )
  {
    push_int(--(Pike_fp->locals[arg1].u.integer));
    Pike_fp->locals[arg1].subtype = NUMBER_NUMBER; /* Could have UNDEFINED there before. */
  } else {
    push_svalue(Pike_fp->locals+arg1);
    push_int(1);
    o_subtract();
    assign_svalue(Pike_fp->locals+arg1,Pike_sp-1);
  }
});

OPCODE1(F_POST_DEC_LOCAL, "local--", 0, {
  push_svalue( Pike_fp->locals + arg1);

  if( (Pike_fp->locals[arg1].type == PIKE_T_INT)
      DO_IF_BIGNUM(
      && (!INT_TYPE_SUB_OVERFLOW(Pike_fp->locals[arg1].u.integer, 1))
      )
      )
  {
    Pike_fp->locals[arg1].u.integer--;
    Pike_fp->locals[arg1].subtype = NUMBER_NUMBER; /* Could have UNDEFINED there before. */
  } else {
    push_svalue(Pike_fp->locals + arg1);
    push_int(1);
    o_subtract();
    stack_pop_to(Pike_fp->locals + arg1);
  }
  /* Pike_fp->locals[instr].u.integer--; */
});

OPCODE1(F_DEC_LOCAL_AND_POP, "--local and pop", 0, {
  if( (Pike_fp->locals[arg1].type == PIKE_T_INT)
      DO_IF_BIGNUM(
      && (!INT_TYPE_SUB_OVERFLOW(Pike_fp->locals[arg1].u.integer, 1))
      )
      )
  {
    Pike_fp->locals[arg1].u.integer--;
    Pike_fp->locals[arg1].subtype = NUMBER_NUMBER; /* Could have UNDEFINED there before. */
  } else {
    push_svalue(Pike_fp->locals + arg1);
    push_int(1);
    o_subtract();
    stack_pop_to(Pike_fp->locals + arg1);
  }
});

OPCODE0(F_LTOSVAL, "lvalue to svalue", 0, {
  dmalloc_touch_svalue(Pike_sp-2);
  dmalloc_touch_svalue(Pike_sp-1);
  lvalue_to_svalue_no_free(Pike_sp, Pike_sp-2);
  Pike_sp++;
  print_return_value();
});

OPCODE0(F_LTOSVAL2, "ltosval2", 0, {
  dmalloc_touch_svalue(Pike_sp-3);
  dmalloc_touch_svalue(Pike_sp-2);
  dmalloc_touch_svalue(Pike_sp-1);
  Pike_sp[0] = Pike_sp[-1];
  Pike_sp[-1].type = PIKE_T_INT;
  Pike_sp++;
  lvalue_to_svalue_no_free(Pike_sp-2, Pike_sp-4);

  /* this is so that foo+=bar (and similar things) will be faster, this
   * is done by freeing the old reference to foo after it has been pushed
   * on the stack. That way foo can have only 1 reference if we are lucky,
   * and then the low array/multiset/mapping manipulation routines can be
   * destructive if they like
   */
  if( (1 << Pike_sp[-2].type) &
      (BIT_ARRAY | BIT_MULTISET | BIT_MAPPING | BIT_STRING) )
  {
    struct svalue s;
    s.type = PIKE_T_INT;
    s.subtype = 0;
    s.u.integer = 0;
    assign_lvalue(Pike_sp-4, &s);
  }
});

OPCODE0(F_LTOSVAL3, "ltosval3", 0, {
  Pike_sp[0] = Pike_sp[-1];
  Pike_sp[-1] = Pike_sp[-2];
  Pike_sp[-2].type = PIKE_T_INT;
  Pike_sp++;
  lvalue_to_svalue_no_free(Pike_sp-3, Pike_sp-5);

  /* this is so that foo=foo[x..y] (and similar things) will be faster, this
   * is done by freeing the old reference to foo after it has been pushed
   * on the stack. That way foo can have only 1 reference if we are lucky,
   * and then the low array/multiset/mapping manipulation routines can be
   * destructive if they like
   */
  if( (1 << Pike_sp[-3].type) &
      (BIT_ARRAY | BIT_MULTISET | BIT_MAPPING | BIT_STRING) )
  {
    struct svalue s;
    s.type = PIKE_T_INT;
    s.subtype = 0;
    s.u.integer = 0;
    assign_lvalue(Pike_sp-5, &s);
  }
});

OPCODE0(F_ADD_TO_AND_POP, "+= and pop", 0, {
  Pike_sp[0]=Pike_sp[-1];
  Pike_sp[-1].type=PIKE_T_INT;
  Pike_sp++;
  lvalue_to_svalue_no_free(Pike_sp-2,Pike_sp-4);

  if( Pike_sp[-1].type == PIKE_T_INT &&
      Pike_sp[-2].type == PIKE_T_INT  )
  {
    DO_IF_BIGNUM(
    if(!INT_TYPE_ADD_OVERFLOW(Pike_sp[-1].u.integer, Pike_sp[-2].u.integer))
    )
    {
      /* Optimization for a rather common case. Makes it 30% faster. */
      Pike_sp[-1].u.integer += Pike_sp[-2].u.integer;
      Pike_sp[-1].subtype = NUMBER_NUMBER; /* Could have UNDEFINED there before. */
      assign_lvalue(Pike_sp-4,Pike_sp-1);
      Pike_sp-=2;
      pop_2_elems();
      goto add_and_pop_done;
    }
  }
  /* this is so that foo+=bar (and similar things) will be faster, this
   * is done by freeing the old reference to foo after it has been pushed
   * on the stack. That way foo can have only 1 reference if we are lucky,
   * and then the low array/multiset/mapping manipulation routines can be
   * destructive if they like
   */
  if( (1 << Pike_sp[-2].type) &
      (BIT_ARRAY | BIT_MULTISET | BIT_MAPPING | BIT_STRING) )
  {
    struct svalue s;
    s.type=PIKE_T_INT;
    s.subtype=0;
    s.u.integer=0;
    assign_lvalue(Pike_sp-4,&s);
  }
  f_add(2);
  assign_lvalue(Pike_sp-3,Pike_sp-1);
  pop_n_elems(3);
 add_and_pop_done:
   ; /* make gcc happy */
});

OPCODE1(F_GLOBAL_LVALUE, "& global", 0, {
  struct identifier *i;
  INT32 tmp=arg1 + Pike_fp->context.identifier_level;
  if(!Pike_fp->current_object->prog)
    Pike_error("Cannot access global variables in destructed object.\n");
  i=ID_FROM_INT(Pike_fp->current_object->prog, tmp);

  if(!IDENTIFIER_IS_VARIABLE(i->identifier_flags))
    Pike_error("Cannot re-assign functions or constants.\n");

  if(i->run_time_type == PIKE_T_MIXED)
  {
    Pike_sp[0].type=T_LVALUE;
    Pike_sp[0].u.lval=(struct svalue *)GLOBAL_FROM_INT(tmp);
  }else{
    Pike_sp[0].type=T_SHORT_LVALUE;
    Pike_sp[0].u.short_lval= (union anything *)GLOBAL_FROM_INT(tmp);
    Pike_sp[0].subtype=i->run_time_type;
  }
  Pike_sp[1].type=T_VOID;
  Pike_sp+=2;
});

OPCODE0(F_INC, "++x", 0, {
  union anything *u=get_pointer_if_this_type(Pike_sp-2, PIKE_T_INT);
  if(u
     DO_IF_BIGNUM(
     && !INT_TYPE_ADD_OVERFLOW(u->integer, 1)
     )
     )
  {
    INT32 val = ++u->integer;
    pop_2_elems();
    push_int(val);
  } else {
    lvalue_to_svalue_no_free(Pike_sp, Pike_sp-2); Pike_sp++;
    push_int(1);
    f_add(2);
    assign_lvalue(Pike_sp-3, Pike_sp-1);
    stack_unlink(2);
  }
});

OPCODE0(F_DEC, "--x", 0, {
  union anything *u=get_pointer_if_this_type(Pike_sp-2, PIKE_T_INT);
  if(u
     DO_IF_BIGNUM(
     && !INT_TYPE_SUB_OVERFLOW(u->integer, 1)
     )
     )
  {
    INT32 val = --u->integer;
    pop_2_elems();
    push_int(val);
  } else {
    lvalue_to_svalue_no_free(Pike_sp, Pike_sp-2); Pike_sp++;
    push_int(1);
    o_subtract();
    assign_lvalue(Pike_sp-3, Pike_sp-1);
    stack_unlink(2);
  }
});

OPCODE0(F_DEC_AND_POP, "x-- and pop", 0, {
  union anything *u=get_pointer_if_this_type(Pike_sp-2, PIKE_T_INT);
  if(u
     DO_IF_BIGNUM(
     && !INT_TYPE_SUB_OVERFLOW(u->integer, 1)
     )
)
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

OPCODE0(F_INC_AND_POP, "x++ and pop", 0, {
  union anything *u=get_pointer_if_this_type(Pike_sp-2, PIKE_T_INT);
  if(u
     DO_IF_BIGNUM(
     && !INT_TYPE_ADD_OVERFLOW(u->integer, 1)
     )
     )
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

OPCODE0(F_POST_INC, "x++", 0, {
  union anything *u=get_pointer_if_this_type(Pike_sp-2, PIKE_T_INT);
  if(u
     DO_IF_BIGNUM(
     && !INT_TYPE_ADD_OVERFLOW(u->integer, 1)
     )
     )
  {
    INT32 val = u->integer++;
    pop_2_elems();
    push_int(val);
  } else {
    lvalue_to_svalue_no_free(Pike_sp, Pike_sp-2); Pike_sp++;
    stack_dup();
    push_int(1);
    f_add(2);
    assign_lvalue(Pike_sp-4, Pike_sp-1);
    pop_stack();
    stack_unlink(2);
    print_return_value();
  }
});

OPCODE0(F_POST_DEC, "x--", 0, {
  union anything *u=get_pointer_if_this_type(Pike_sp-2, PIKE_T_INT);
  if(u
     DO_IF_BIGNUM(
     && !INT_TYPE_SUB_OVERFLOW(u->integer, 1)
     )
     )
  {
    INT32 val = u->integer--;
    pop_2_elems();
    push_int(val);
  } else {
    lvalue_to_svalue_no_free(Pike_sp, Pike_sp-2); Pike_sp++;
    stack_dup();
    push_int(1);
    o_subtract();
    assign_lvalue(Pike_sp-4, Pike_sp-1);
    pop_stack();
    stack_unlink(2);
    print_return_value();
  }
});

OPCODE1(F_ASSIGN_LOCAL, "assign local", 0, {
  assign_svalue(Pike_fp->locals+arg1,Pike_sp-1);
});

OPCODE0(F_ASSIGN, "assign", 0, {
  assign_lvalue(Pike_sp-3,Pike_sp-1);
  free_svalue(Pike_sp-3);
  free_svalue(Pike_sp-2);
  Pike_sp[-3]=Pike_sp[-1];
  Pike_sp-=2;
});

OPCODE2(F_APPLY_ASSIGN_LOCAL_AND_POP, "apply, assign local and pop", 0, {
  apply_svalue(&((Pike_fp->context.prog->constants + arg1)->sval),
	       DO_NOT_WARN((INT32)(Pike_sp - *--Pike_mark_sp)));
  free_svalue(Pike_fp->locals+arg2);
  Pike_fp->locals[arg2]=Pike_sp[-1];
  Pike_sp--;
});

OPCODE2(F_APPLY_ASSIGN_LOCAL, "apply, assign local", 0, {
  apply_svalue(&((Pike_fp->context.prog->constants + arg1)->sval),
	       DO_NOT_WARN((INT32)(Pike_sp - *--Pike_mark_sp)));
  assign_svalue(Pike_fp->locals+arg2, Pike_sp-1);
});

OPCODE0(F_ASSIGN_AND_POP, "assign and pop", 0, {
  assign_lvalue(Pike_sp-3, Pike_sp-1);
  pop_n_elems(3);
});

OPCODE1(F_ASSIGN_LOCAL_AND_POP, "assign local and pop", 0, {
  free_svalue(Pike_fp->locals + arg1);
  Pike_fp->locals[arg1] = Pike_sp[-1];
  Pike_sp--;
});

OPCODE1(F_ASSIGN_GLOBAL, "assign global", 0, {
  struct identifier *i;
  INT32 tmp=arg1 + Pike_fp->context.identifier_level;
  if(!Pike_fp->current_object->prog)
    Pike_error("Cannot access global variables in destructed object.\n");

  i=ID_FROM_INT(Pike_fp->current_object->prog, tmp);
  if(!IDENTIFIER_IS_VARIABLE(i->identifier_flags))
    Pike_error("Cannot assign functions or constants.\n");
  if(i->run_time_type == PIKE_T_MIXED)
  {
    assign_svalue((struct svalue *)GLOBAL_FROM_INT(tmp), Pike_sp-1);
  }else{
    assign_to_short_svalue((union anything *)GLOBAL_FROM_INT(tmp),
			   i->run_time_type,
			   Pike_sp-1);
  }
});

OPCODE1(F_ASSIGN_GLOBAL_AND_POP, "assign global and pop", 0, {
  struct identifier *i;
  INT32 tmp=arg1 + Pike_fp->context.identifier_level;
  if(!Pike_fp->current_object->prog)
    Pike_error("Cannot access global variables in destructed object.\n");

  i=ID_FROM_INT(Pike_fp->current_object->prog, tmp);
  if(!IDENTIFIER_IS_VARIABLE(i->identifier_flags))
    Pike_error("Cannot assign functions or constants.\n");

  if(i->run_time_type == PIKE_T_MIXED)
  {
    struct svalue *s=(struct svalue *)GLOBAL_FROM_INT(tmp);
    free_svalue(s);
    Pike_sp--;
    *s=*Pike_sp;
  }else{
    assign_to_short_svalue((union anything *)GLOBAL_FROM_INT(tmp),
			   i->run_time_type,
			   Pike_sp-1);
    pop_stack();
  }
});


/* Stack machine stuff */

OPCODE0(F_POP_VALUE, "pop", 0, {
  pop_stack();
});

OPCODE1(F_POP_N_ELEMS, "pop_n_elems", 0, {
  pop_n_elems(arg1);
});

OPCODE0_TAIL(F_MARK2, "mark mark", 0, {
  *(Pike_mark_sp++)=Pike_sp;

/* This opcode is only used when running with -d. Identical to F_MARK,
 * but with a different name to make the debug printouts more clear. */
  OPCODE0_TAIL(F_SYNCH_MARK, "synch mark", 0, {

    OPCODE0(F_MARK, "mark", 0, {
      *(Pike_mark_sp++)=Pike_sp;
    });
  });
});

OPCODE1(F_MARK_X, "mark Pike_sp-X", 0, {
  *(Pike_mark_sp++)=Pike_sp-arg1;
});

OPCODE0(F_POP_MARK, "pop mark", 0, {
  --Pike_mark_sp;
});

OPCODE0(F_POP_TO_MARK, "pop to mark", 0, {
  pop_n_elems(Pike_sp - *--Pike_mark_sp);
});

/* These opcodes are only used when running with -d. The reason for
 * the two aliases is mainly to keep the indentation in asm debug
 * output. */
OPCODE0_TAIL(F_CLEANUP_SYNCH_MARK, "cleanup synch mark", 0, {
  OPCODE0(F_POP_SYNCH_MARK, "pop synch mark", 0, {
    if (d_flag) {
      if (Pike_mark_sp <= Pike_interpreter.mark_stack) {
	Pike_fatal("Mark stack out of synch - 0x%08lx <= 0x%08lx.\n",
	      DO_NOT_WARN((unsigned long)Pike_mark_sp),
	      DO_NOT_WARN((unsigned long)Pike_interpreter.mark_stack));
      } else if (*--Pike_mark_sp != Pike_sp) {
	ptrdiff_t should = *Pike_mark_sp - Pike_interpreter.evaluator_stack;
	ptrdiff_t is = Pike_sp - Pike_interpreter.evaluator_stack;
	if (Pike_sp - *Pike_mark_sp > 0) /* not always same as Pike_sp > *Pike_mark_sp */
	/* Some attempt to recover, just to be able to report the backtrace. */
	  pop_n_elems(Pike_sp - *Pike_mark_sp);
	Pike_fatal("Stack out of synch - should be %ld, is %ld.\n",
	      DO_NOT_WARN((long)should), DO_NOT_WARN((long)is));
      }
    }
  });
});

OPCODE0(F_CLEAR_STRING_SUBTYPE, "clear string subtype", 0, {
  if(Pike_sp[-1].type==PIKE_T_STRING) Pike_sp[-1].subtype=0;
});

      /* Jumps */
OPCODE0_BRANCH(F_BRANCH, "branch", 0, {
  DO_BRANCH();
});

OPCODE2_BRANCH(F_BRANCH_IF_NOT_LOCAL_ARROW, "branch if !local->x", 0, {
  struct svalue tmp;
  tmp.type=PIKE_T_STRING;
  tmp.u.string=Pike_fp->context.prog->strings[arg1];
  tmp.subtype=1;
  Pike_sp->type=PIKE_T_INT;	
  Pike_sp++;
  index_no_free(Pike_sp-1,Pike_fp->locals+arg2, &tmp);
  print_return_value();

  /* Fall through */

  OPCODE0_TAILBRANCH(F_BRANCH_WHEN_ZERO, "branch if zero", 0, {
    if(!UNSAFE_IS_ZERO(Pike_sp-1))
    {
      DONT_BRANCH();
    }else{
      DO_BRANCH();
    }
    pop_stack();
  });
});

      
OPCODE0_BRANCH(F_BRANCH_WHEN_NON_ZERO, "branch if not zero", 0, {
  if(UNSAFE_IS_ZERO(Pike_sp-1))
  {
    DONT_BRANCH();
  }else{
    DO_BRANCH();
  }
  pop_stack();
});

OPCODE1_BRANCH(F_BRANCH_IF_TYPE_IS_NOT, "branch if type is !=", 0, {
/*  fprintf(stderr,"******BRANCH IF TYPE IS NOT***** %s\n",get_name_of_type(arg1)); */
  if(Pike_sp[-1].type == T_OBJECT &&
     Pike_sp[-1].u.object->prog)
  {
    int fun=FIND_LFUN(Pike_sp[-1].u.object->prog, LFUN__IS_TYPE);
    if(fun != -1)
    {
/*      fprintf(stderr,"******OBJECT OVERLOAD IN TYPEP***** %s\n",get_name_of_type(arg1)); */
      push_text(get_name_of_type(arg1));
      apply_low(Pike_sp[-2].u.object, fun, 1);
      arg1=UNSAFE_IS_ZERO(Pike_sp-1) ? T_FLOAT : T_OBJECT ;
      pop_stack();
    }
  }
  if(Pike_sp[-1].type == arg1)
  {
    DONT_BRANCH();
  }else{
    DO_BRANCH();
  }
  pop_stack();
});

OPCODE1_BRANCH(F_BRANCH_IF_LOCAL, "branch if local", 0, {
  if(UNSAFE_IS_ZERO(Pike_fp->locals + arg1))
  {
    DONT_BRANCH();
  }else{
    DO_BRANCH();
  }
});

OPCODE1_BRANCH(F_BRANCH_IF_NOT_LOCAL, "branch if !local", 0, {
  if(!UNSAFE_IS_ZERO(Pike_fp->locals + arg1))
  {
    DONT_BRANCH();
  }else{
    DO_BRANCH();
  }
});

#define CJUMP(X, DESC, Y) \
  OPCODE0_BRANCH(X, DESC, 0, { \
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
CJUMP(F_BRANCH_WHEN_LE, "branch if <=", !is_gt);
CJUMP(F_BRANCH_WHEN_GT, "branch if >", is_gt);
CJUMP(F_BRANCH_WHEN_GE, "branch if >=", !is_lt);

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

OPCODE0_BRANCH(F_LAND, "&&", 0, {
  if(!UNSAFE_IS_ZERO(Pike_sp-1))
  {
    DONT_BRANCH();
    pop_stack();
  }else{
    DO_BRANCH();
  }
});

OPCODE0_BRANCH(F_LOR, "||", 0, {
  if(UNSAFE_IS_ZERO(Pike_sp-1))
  {
    DONT_BRANCH();
    pop_stack();
  }else{
    DO_BRANCH();
  }
});

OPCODE0_BRANCH(F_EQ_OR, "==||", 0, {
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

OPCODE0_BRANCH(F_EQ_AND, "==&&", 0, {
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

/* This instruction can't currently be a branch, since
 * it has more than two continuation paths.
 */
OPCODE0_JUMP(F_CATCH, "catch", 0, {
  check_c_stack(8192);
  switch (o_catch((PIKE_OPCODE_T *)(((INT32 *)PROG_COUNTER)+1)))
  {
  case 1:
    /* There was a return inside the evaluated code */
    DO_DUMB_RETURN;
  case 2:
    /* Escape catch, continue after the escape instruction. */
    DO_JUMP_TO(Pike_fp->return_addr);
    break;
  default:
    DOJUMP();
  }
  /* NOT_REACHED in byte-code and computed goto cases. */
});

OPCODE0_RETURN(F_ESCAPE_CATCH, "escape catch", 0, {
  Pike_fp->return_addr = PROG_COUNTER;
  INTER_ESCAPE_CATCH;
});

OPCODE0_RETURN(F_EXIT_CATCH, "exit catch", 0, {
  push_undefined();
  Pike_fp->return_addr = PROG_COUNTER;
  INTER_ESCAPE_CATCH;
});

OPCODE1(F_SWITCH, "switch", 0, {
  INT32 tmp;
  PIKE_OPCODE_T *addr = PROG_COUNTER;
  tmp=switch_lookup(Pike_fp->context.prog->
		    constants[arg1].sval.u.array,Pike_sp-1);
  addr = DO_IF_ELSE_COMPUTED_GOTO(addr, (PIKE_OPCODE_T *)
				  DO_ALIGN(addr,((ptrdiff_t)sizeof(INT32))));
  addr = (PIKE_OPCODE_T *)(((INT32 *)addr) + (tmp>=0 ? 1+tmp*2 : 2*~tmp));
  if(*(INT32*)addr < 0) fast_check_threads_etc(7);
  pop_stack();
  DO_JUMP_TO(addr + *(INT32*)addr);
});

OPCODE1(F_SWITCH_ON_INDEX, "switch on index", 0, {
  INT32 tmp;
  struct svalue s;
  PIKE_OPCODE_T *addr = PROG_COUNTER;
  index_no_free(&s,Pike_sp-2,Pike_sp-1);
  Pike_sp++[0]=s;

  tmp=switch_lookup(Pike_fp->context.prog->
		    constants[arg1].sval.u.array,Pike_sp-1);
  pop_n_elems(3);
  addr = DO_IF_ELSE_COMPUTED_GOTO(addr, (PIKE_OPCODE_T *)
				  DO_ALIGN(addr,((ptrdiff_t)sizeof(INT32))));
  addr = (PIKE_OPCODE_T *)(((INT32 *)addr) + (tmp>=0 ? 1+tmp*2 : 2*~tmp));
  if(*(INT32*)addr < 0) fast_check_threads_etc(7);
  DO_JUMP_TO(addr + *(INT32*)addr);
});

OPCODE2(F_SWITCH_ON_LOCAL, "switch on local", 0, {
  INT32 tmp;
  PIKE_OPCODE_T *addr = PROG_COUNTER;
  tmp=switch_lookup(Pike_fp->context.prog->
		    constants[arg2].sval.u.array,Pike_fp->locals + arg1);
  addr = DO_IF_ELSE_COMPUTED_GOTO(addr, (PIKE_OPCODE_T *)
				  DO_ALIGN(addr,((ptrdiff_t)sizeof(INT32))));
  addr = (PIKE_OPCODE_T *)(((INT32 *)addr) + (tmp>=0 ? 1+tmp*2 : 2*~tmp));
  if(*(INT32*)addr < 0) fast_check_threads_etc(7);
  DO_JUMP_TO(addr + *(INT32*)addr);
});


#ifdef AUTO_BIGNUM
#define AUTO_BIGNUM_LOOP_TEST(X,Y) INT_TYPE_ADD_OVERFLOW(X,Y)
#else
#define AUTO_BIGNUM_LOOP_TEST(X,Y) 0
#endif

      /* FIXME: Does this need bignum tests? /Fixed - Hubbe */
      /* LOOP(OPCODE, INCREMENT, OPERATOR, IS_OPERATOR) */
#define LOOP(ID, DESC, INC, OP2, OP4)					\
  OPCODE0_BRANCH(ID, DESC, 0, {						\
    union anything *i=get_pointer_if_this_type(Pike_sp-2, T_INT);	\
    if(i && !AUTO_BIGNUM_LOOP_TEST(i->integer,INC) &&			\
       Pike_sp[-3].type == T_INT)					\
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
OPCODE0_BRANCH(F_LOOP, "loop", 0, { /* loopcnt */
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
  if(Pike_sp[-4].type != PIKE_T_ARRAY)
    PIKE_ERROR("foreach", "Bad argument 1.\n", Pike_sp-3, 1);
  if(Pike_sp[-1].u.integer < Pike_sp[-4].u.array->size)
  {
    if(Pike_sp[-1].u.integer < 0)
      /* Isn't this an internal compiler error? /mast */
      Pike_error("Foreach loop variable is negative!\n");
    assign_lvalue(Pike_sp-3, Pike_sp[-4].u.array->item + Pike_sp[-1].u.integer);
    DO_BRANCH();
    Pike_sp[-1].u.integer++;
    DO_IF_DEBUG (
      if (Pike_sp[-1].subtype)
	Pike_fatal ("Got unexpected subtype in loop variable.\n");
    );
  }else{
    DONT_BRANCH();
  }
});

OPCODE0(F_MAKE_ITERATOR, "Iterator", 0, {
  extern void f_Iterator(INT32);
  f_Iterator(1);
});

/* Stack is: iterator, index lvalue, value lvalue. */
OPCODE0_BRANCH (F_FOREACH_START, "foreach start", 0, {
  extern int foreach_iterate(struct object *o, int do_step);
  DO_IF_DEBUG (
    if(Pike_sp[-5].type != PIKE_T_OBJECT)
      Pike_fatal ("Iterator gone from stack.\n");
  );
  if (foreach_iterate (Pike_sp[-5].u.object, 0))
    DONT_BRANCH();
  else {
    DO_BRANCH();
  }
});

/* Stack is: iterator, index lvalue, value lvalue. */
OPCODE0_BRANCH(F_FOREACH_LOOP, "foreach loop", 0, {
  extern int foreach_iterate(struct object *o, int do_step);
  DO_IF_DEBUG (
    if(Pike_sp[-5].type != PIKE_T_OBJECT)
      Pike_fatal ("Iterator gone from stack.\n");
  );
  if(foreach_iterate(Pike_sp[-5].u.object, 1))
  {
    DO_BRANCH();
  }else{
    DONT_BRANCH();
  }
});


OPCODE1_RETURN(F_RETURN_LOCAL,"return local",0,{
  DO_IF_DEBUG(
    /* special case! Pike_interpreter.mark_stack may be invalid at the time we
     * call return -1, so we must call the callbacks here to
     * prevent false alarms! /Hubbe
     */
    if(d_flag>3) do_gc();
    if(d_flag>4) do_debug();
    );
  if(Pike_fp->expendible <= Pike_fp->locals + arg1)
  {
    pop_n_elems(Pike_sp-1 - (Pike_fp->locals + arg1));
  }else{
    push_svalue(Pike_fp->locals + arg1);
  }
  DO_DUMB_RETURN;
});


OPCODE0_RETURN(F_RETURN_IF_TRUE,"return if true",0,{
  if(!UNSAFE_IS_ZERO(Pike_sp-1)) DO_RETURN;
  pop_stack();
});

OPCODE0_RETURN(F_RETURN_1,"return 1",0,{
  push_int(1);
  DO_RETURN;
});

OPCODE0_RETURN(F_RETURN_0,"return 0",0,{
  push_int(0);
  DO_RETURN;
});

OPCODE0_RETURN(F_RETURN, "return", 0, {
  DO_RETURN;
});

OPCODE0_RETURN(F_DUMB_RETURN,"dumb return", 0, {
  DO_DUMB_RETURN;
});

OPCODE0(F_NEGATE, "unary minus", 0, {
  if(Pike_sp[-1].type == PIKE_T_INT)
  {
    DO_IF_BIGNUM(
      if(INT_TYPE_NEG_OVERFLOW(Pike_sp[-1].u.integer))
      {
	convert_stack_top_to_bignum();
	o_negate();
      }
      else
    )
    {
      Pike_sp[-1].u.integer =- Pike_sp[-1].u.integer;
      Pike_sp[-1].subtype = NUMBER_NUMBER; /* Could have UNDEFINED there before. */
    }
  }
  else if(Pike_sp[-1].type == PIKE_T_FLOAT)
  {
    Pike_sp[-1].u.float_number =- Pike_sp[-1].u.float_number;
  }else{
    o_negate();
  }
});

OPCODE0_ALIAS(F_COMPL, "~", 0, o_compl);

OPCODE0(F_NOT, "!", 0, {
  switch(Pike_sp[-1].type)
  {
  case PIKE_T_INT:
    Pike_sp[-1].u.integer =! Pike_sp[-1].u.integer;
    Pike_sp[-1].subtype = NUMBER_NUMBER; /* Could have UNDEFINED there before. */
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
    Pike_sp[-1].type=PIKE_T_INT;
    Pike_sp[-1].subtype = NUMBER_NUMBER;
    Pike_sp[-1].u.integer=0;
  }
});

OPCODE0_ALIAS(F_LSH, "<<", 0, o_lsh);
OPCODE0_ALIAS(F_RSH, ">>", 0, o_rsh);

#define COMPARISON(ID,DESC,EXPR)	\
  OPCODE0(ID, DESC, 0, {		\
    INT32 val = EXPR;			\
    pop_2_elems();			\
    push_int(val);			\
  })

COMPARISON(F_EQ, "==", is_eq(Pike_sp-2,Pike_sp-1));
COMPARISON(F_NE, "!=", !is_eq(Pike_sp-2,Pike_sp-1));
COMPARISON(F_GT, ">", is_gt(Pike_sp-2,Pike_sp-1));
COMPARISON(F_GE, ">=", !is_lt(Pike_sp-2,Pike_sp-1));
COMPARISON(F_LT, "<", is_lt(Pike_sp-2,Pike_sp-1));
COMPARISON(F_LE, "<=", !is_gt(Pike_sp-2,Pike_sp-1));

OPCODE0(F_ADD, "+", 0, {
  f_add(2);
});

OPCODE0(F_ADD_INTS, "int+int", 0, {
  if(Pike_sp[-1].type == T_INT && Pike_sp[-2].type == T_INT 
     DO_IF_BIGNUM(
      && (!INT_TYPE_ADD_OVERFLOW(Pike_sp[-1].u.integer, Pike_sp[-2].u.integer))
      )
    )
  {
    Pike_sp[-2].u.integer+=Pike_sp[-1].u.integer;
    Pike_sp[-2].subtype = NUMBER_NUMBER; /* Could have UNDEFINED there before. */
    Pike_sp--;
  }else{
    f_add(2);
  }
});

OPCODE0(F_ADD_FLOATS, "float+float", 0, {
  if(Pike_sp[-1].type == T_FLOAT && Pike_sp[-2].type == T_FLOAT)
  {
    Pike_sp[-2].u.float_number+=Pike_sp[-1].u.float_number;
    Pike_sp--;
  }else{
    f_add(2);
  }
});

OPCODE0_ALIAS(F_SUBTRACT, "-", 0, o_subtract);
OPCODE0_ALIAS(F_AND, "&", 0, o_and);
OPCODE0_ALIAS(F_OR, "|", 0, o_or);
OPCODE0_ALIAS(F_XOR, "^", 0, o_xor);
OPCODE0_ALIAS(F_MULTIPLY, "*", 0, o_multiply);
OPCODE0_ALIAS(F_DIVIDE, "/", 0, o_divide);
OPCODE0_ALIAS(F_MOD, "%", 0, o_mod);

OPCODE1(F_ADD_INT, "add integer", 0, {
  if(Pike_sp[-1].type == T_INT
     DO_IF_BIGNUM(
      && (!INT_TYPE_ADD_OVERFLOW(Pike_sp[-1].u.integer, arg1))
      )
     )
  {
    Pike_sp[-1].u.integer+=arg1;
    Pike_sp[-1].subtype = NUMBER_NUMBER; /* Could have UNDEFINED there before. */
  }else{
    push_int(arg1);
    f_add(2);
  }
});

OPCODE1(F_ADD_NEG_INT, "add -integer", 0, {
  if(Pike_sp[-1].type == T_INT
     DO_IF_BIGNUM(
      && (!INT_TYPE_ADD_OVERFLOW(Pike_sp[-1].u.integer, -arg1))
      )
     )
  {
    Pike_sp[-1].u.integer-=arg1;
    Pike_sp[-1].subtype = NUMBER_NUMBER; /* Could have UNDEFINED there before. */
  }else{
    push_int(-arg1);
    f_add(2);
  }
});

OPCODE0(F_PUSH_ARRAY, "@", 0, {
  switch(Pike_sp[-1].type)
  {
  default:
    PIKE_ERROR("@", "Bad argument.\n", Pike_sp, 1);
    
  case PIKE_T_OBJECT:
    if(!Pike_sp[-1].u.object->prog ||
       FIND_LFUN(Pike_sp[-1].u.object->prog,LFUN__VALUES) == -1)
      PIKE_ERROR("@", "Bad argument.\n", Pike_sp, 1);

    apply_lfun(Pike_sp[-1].u.object, LFUN__VALUES, 0);
    if(Pike_sp[-1].type != PIKE_T_ARRAY)
      Pike_error("Bad return type from o->_values() in @\n");
    free_svalue(Pike_sp-2);
    Pike_sp[-2]=Pike_sp[-1];
    Pike_sp--;
    break;

  case PIKE_T_ARRAY: break;
  }
  Pike_sp--;
  push_array_items(Pike_sp->u.array);
});

OPCODE2(F_LOCAL_LOCAL_INDEX, "local[local]", 0, {
  struct svalue *s=Pike_fp->locals+arg1;
  if(s->type == PIKE_T_STRING) s->subtype=0;
  Pike_sp++->type=PIKE_T_INT;
  index_no_free(Pike_sp-1,Pike_fp->locals+arg2,s);
});

OPCODE1(F_LOCAL_INDEX, "local index", 0, {
  struct svalue tmp;
  struct svalue *s = Pike_fp->locals+arg1;
  if(s->type == PIKE_T_STRING) s->subtype=0;
  index_no_free(&tmp,Pike_sp-1,s);
  free_svalue(Pike_sp-1);
  Pike_sp[-1]=tmp;
});

OPCODE2(F_GLOBAL_LOCAL_INDEX, "global[local]", 0, {
  struct svalue tmp;
  struct svalue *s;
  low_object_index_no_free(Pike_sp,
			   Pike_fp->current_object,
			   arg1 + Pike_fp->context.identifier_level);
  Pike_sp++;
  s=Pike_fp->locals+arg2;
  if(s->type == PIKE_T_STRING) s->subtype=0;
  index_no_free(&tmp,Pike_sp-1,s);
  free_svalue(Pike_sp-1);
  Pike_sp[-1]=tmp;
});

OPCODE2(F_LOCAL_ARROW, "local->x", 0, {
  struct svalue tmp;
  tmp.type=PIKE_T_STRING;
  tmp.u.string=Pike_fp->context.prog->strings[arg1];
  tmp.subtype=1;
  Pike_sp->type=PIKE_T_INT;	
  Pike_sp++;
  index_no_free(Pike_sp-1,Pike_fp->locals+arg2, &tmp);
  print_return_value();
});

OPCODE1(F_ARROW, "->x", 0, {
  struct svalue tmp;
  struct svalue tmp2;
  tmp.type=PIKE_T_STRING;
  tmp.u.string=Pike_fp->context.prog->strings[arg1];
  tmp.subtype=1;
  index_no_free(&tmp2, Pike_sp-1, &tmp);
  free_svalue(Pike_sp-1);
  Pike_sp[-1]=tmp2;
  print_return_value();
});

OPCODE1(F_STRING_INDEX, "string index", 0, {
  struct svalue tmp;
  struct svalue tmp2;
  tmp.type=PIKE_T_STRING;
  tmp.u.string=Pike_fp->context.prog->strings[arg1];
  tmp.subtype=0;
  index_no_free(&tmp2, Pike_sp-1, &tmp);
  free_svalue(Pike_sp-1);
  Pike_sp[-1]=tmp2;
  print_return_value();
});

OPCODE1(F_POS_INT_INDEX, "int index", 0, {
  push_int(arg1);
  print_return_value();
  DO_INDEX;
});

OPCODE1(F_NEG_INT_INDEX, "-int index", 0, {
  push_int(-(ptrdiff_t)arg1);
  print_return_value();
  DO_INDEX;
});

OPCODE0(F_INDEX, "index", 0, {
  DO_INDEX;
});

OPCODE2(F_MAGIC_INDEX, "::`[]", 0, {
  push_magic_index(magic_index_program, arg2, arg1);
});

OPCODE2(F_MAGIC_SET_INDEX, "::`[]=", 0, {
  push_magic_index(magic_set_index_program, arg2, arg1);
});

OPCODE2(F_MAGIC_INDICES, "::_indices", 0, {
  push_magic_index(magic_indices_program, arg2, arg1);
});

OPCODE2(F_MAGIC_VALUES, "::_values", 0, {
  push_magic_index(magic_values_program, arg2, arg1);
});

OPCODE0_ALIAS(F_CAST, "cast", 0, f_cast);
OPCODE0_ALIAS(F_CAST_TO_INT, "cast_to_int", 0, o_cast_to_int);
OPCODE0_ALIAS(F_CAST_TO_STRING, "cast_to_string", 0, o_cast_to_string);

OPCODE0(F_SOFT_CAST, "soft cast", 0, {
  /* Stack: type_string, value */
  DO_IF_DEBUG({
    if (Pike_sp[-2].type != T_TYPE) {
      Pike_fatal("Argument 1 to soft_cast isn't a type!\n");
    }
  });
  if (runtime_options & RUNTIME_CHECK_TYPES) {
    struct pike_type *sval_type = get_type_of_svalue(Pike_sp-1);
    if (!pike_types_le(sval_type, Pike_sp[-2].u.type)) {
      /* get_type_from_svalue() doesn't return a fully specified type
       * for array, mapping and multiset, so we perform a more lenient
       * check for them.
       */
      if (!pike_types_le(sval_type, weak_type_string) ||
	  !match_types(sval_type, Pike_sp[-2].u.type)) {
	struct pike_string *t1;
	struct pike_string *t2;
	char *fname = "__soft-cast";
	ONERROR tmp1;
	ONERROR tmp2;

	if (Pike_fp->current_object && Pike_fp->context.prog &&
	    Pike_fp->current_object->prog) {
	  /* Look up the function-name */
	  struct pike_string *name =
	    ID_FROM_INT(Pike_fp->current_object->prog, Pike_fp->fun)->name;
	  if ((!name->size_shift) && (name->len < 100))
	    fname = name->str;
	}

	t1 = describe_type(Pike_sp[-2].u.type);
	SET_ONERROR(tmp1, do_free_string, t1);
	  
	t2 = describe_type(sval_type);
	SET_ONERROR(tmp2, do_free_string, t2);
	  
	free_type(sval_type);

	bad_arg_error(NULL, Pike_sp-1, 1, 1, t1->str, Pike_sp-1,
		      "%s(): Soft cast failed. Expected %s, got %s\n",
		      fname, t1->str, t2->str);
	/* NOT_REACHED */
	UNSET_ONERROR(tmp2);
	UNSET_ONERROR(tmp1);
	free_string(t2);
	free_string(t1);
      }
    }
    free_type(sval_type);

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

OPCODE0_ALIAS(F_RANGE, "range", 0, o_range);

OPCODE0(F_COPY_VALUE, "copy_value", 0, {
  struct svalue tmp;
  copy_svalues_recursively_no_free(&tmp,Pike_sp-1,1,0);
  free_svalue(Pike_sp-1);
  Pike_sp[-1]=tmp;
});

OPCODE0(F_INDIRECT, "indirect", 0, {
  struct svalue s;
  lvalue_to_svalue_no_free(&s,Pike_sp-2);
  if(s.type != PIKE_T_STRING)
  {
    pop_2_elems();
    *Pike_sp=s;
    Pike_sp++;
  }else{
    struct object *o;
    o=low_clone(string_assignment_program);
    ((struct string_assignment_storage *)o->storage)->lval[0]=Pike_sp[-2];
    ((struct string_assignment_storage *)o->storage)->lval[1]=Pike_sp[-1];
    ((struct string_assignment_storage *)o->storage)->s=s.u.string;
    Pike_sp-=2;
    push_object(o);
  }
  print_return_value();
});
      
OPCODE0(F_SIZEOF, "sizeof", 0, {
  INT32 val = pike_sizeof(Pike_sp-1);
  pop_stack();
  push_int(val);
});

OPCODE1(F_SIZEOF_LOCAL, "sizeof local", 0, {
  push_int(pike_sizeof(Pike_fp->locals+arg1));
});

OPCODE1_ALIAS(F_SSCANF, "sscanf", 0, o_sscanf);

#define MKAPPLY(OP,OPCODE,NAME,TYPE,  ARG2, ARG3)			   \
OP(PIKE_CONCAT(F_,OPCODE),NAME, 0, {					   \
Pike_fp->return_addr=PROG_COUNTER;					   \
if(low_mega_apply(TYPE,DO_NOT_WARN((INT32)(Pike_sp - *--Pike_mark_sp)),	   \
		  ARG2, ARG3))						   \
{									   \
  Pike_fp->flags |= PIKE_FRAME_RETURN_INTERNAL;				   \
  DO_JUMP_TO(Pike_fp->pc);						   \
}									   \
});									   \
									   \
OP(PIKE_CONCAT3(F_,OPCODE,_AND_POP),NAME " & pop", 0, {			   \
  Pike_fp->return_addr=PROG_COUNTER;					   \
  if(low_mega_apply(TYPE, DO_NOT_WARN((INT32)(Pike_sp - *--Pike_mark_sp)), \
		    ARG2, ARG3))					   \
  {									   \
    Pike_fp->flags |= PIKE_FRAME_RETURN_INTERNAL | PIKE_FRAME_RETURN_POP;  \
    DO_JUMP_TO(Pike_fp->pc);						   \
  }else{								   \
    pop_stack();							   \
  }									   \
});									   \
									   \
PIKE_CONCAT(OP,_RETURN)(PIKE_CONCAT3(F_,OPCODE,_AND_RETURN),		   \
			NAME " & return", 0, {				   \
  if(low_mega_apply(TYPE,DO_NOT_WARN((INT32)(Pike_sp - *--Pike_mark_sp)),  \
		    ARG2,ARG3))						   \
  {									   \
    PIKE_OPCODE_T *addr = Pike_fp->pc;					   \
    DO_IF_DEBUG(Pike_fp->next->pc=0);					   \
    unlink_previous_frame();						   \
    DO_JUMP_TO(addr);							   \
  }else{								   \
    DO_DUMB_RETURN;							   \
  }									   \
});									   \


#define MKAPPLY2(OP,OPCODE,NAME,TYPE,  ARG2, ARG3)			   \
									   \
MKAPPLY(OP,OPCODE,NAME,TYPE,  ARG2, ARG3)			           \
									   \
OP(PIKE_CONCAT(F_MARK_,OPCODE),"mark, " NAME, 0, {			   \
  Pike_fp->return_addr=PROG_COUNTER;					   \
  if(low_mega_apply(TYPE,0,						   \
		    ARG2, ARG3))					   \
  {									   \
    Pike_fp->flags |= PIKE_FRAME_RETURN_INTERNAL;			   \
    DO_JUMP_TO(Pike_fp->pc);						   \
  }									   \
});									   \
									   \
OP(PIKE_CONCAT3(F_MARK_,OPCODE,_AND_POP),"mark, " NAME " & pop", 0, {	\
  Pike_fp->return_addr=PROG_COUNTER;					   \
  if(low_mega_apply(TYPE, 0,						   \
		    ARG2, ARG3))					   \
  {									   \
    Pike_fp->flags |= PIKE_FRAME_RETURN_INTERNAL | PIKE_FRAME_RETURN_POP;  \
    DO_JUMP_TO(Pike_fp->pc);						   \
  }else{								   \
    pop_stack();							   \
  }									   \
});									   \
									   \
PIKE_CONCAT(OP,_RETURN)(PIKE_CONCAT3(F_MARK_,OPCODE,_AND_RETURN),	   \
			"mark, " NAME " & return", 0, {			   \
  if(low_mega_apply(TYPE,0,						   \
		    ARG2,ARG3))						   \
  {									   \
    PIKE_OPCODE_T *addr = Pike_fp->pc;					   \
    DO_IF_DEBUG(Pike_fp->next->pc=0);					   \
    unlink_previous_frame();						   \
    DO_JUMP_TO(addr);							   \
  }else{								   \
    DO_DUMB_RETURN;							   \
  }									   \
})


MKAPPLY2(OPCODE1,CALL_LFUN,"call lfun",APPLY_LOW,
	 Pike_fp->current_object,
	 (void *)(ptrdiff_t)(arg1+Pike_fp->context.identifier_level));

MKAPPLY2(OPCODE1,APPLY,"apply",APPLY_SVALUE_STRICT,
	 &((Pike_fp->context.prog->constants + arg1)->sval),0);

MKAPPLY(OPCODE0,CALL_FUNCTION,"call function",APPLY_STACK, 0,0);

OPCODE1(F_CALL_OTHER,"call other", 0, {
  INT32 args=DO_NOT_WARN((INT32)(Pike_sp - *--Pike_mark_sp));
  struct svalue *s=Pike_sp-args;
  Pike_fp->return_addr=PROG_COUNTER;
  if(s->type == T_OBJECT)
  {
    struct object *o=s->u.object;
    struct program *p;
    if((p=o->prog))
    {
      if(FIND_LFUN(p, LFUN_ARROW) == -1)
      {
	int fun;
	fun=find_shared_string_identifier(Pike_fp->context.prog->strings[arg1],
					  p);
	if(fun >= 0)
	{
	  if(low_mega_apply(APPLY_LOW, args-1, o, (void *)(ptrdiff_t)fun))
	  {
	    Pike_fp->save_sp--;
	    Pike_fp->flags |= PIKE_FRAME_RETURN_INTERNAL;
	    DO_JUMP_TO(Pike_fp->pc);
	  }
	  stack_unlink(1);
	  DONE;
	}
      }
    }
  }

  {
    struct svalue tmp;
    struct svalue tmp2;

    tmp.type=PIKE_T_STRING;
    tmp.u.string=Pike_fp->context.prog->strings[arg1];
    tmp.subtype=1;

    index_no_free(&tmp2, s, &tmp);
    free_svalue(s);
    *s=tmp2;
    print_return_value();

    if(low_mega_apply(APPLY_STACK, args, 0, 0))
    {
      Pike_fp->flags |= PIKE_FRAME_RETURN_INTERNAL;
      DO_JUMP_TO(Pike_fp->pc);
    }
    DONE;
  }
});

OPCODE1(F_CALL_OTHER_AND_POP,"call other & pop", 0, {
  INT32 args=DO_NOT_WARN((INT32)(Pike_sp - *--Pike_mark_sp));
  struct svalue *s=Pike_sp-args;
  Pike_fp->return_addr=PROG_COUNTER;
  if(s->type == T_OBJECT)
  {
    struct object *o=s->u.object;
    struct program *p;
    if((p=o->prog))
    {
      if(FIND_LFUN(p, LFUN_ARROW) == -1)
      {
	int fun;
	fun=find_shared_string_identifier(Pike_fp->context.prog->strings[arg1],
					  p);
	if(fun >= 0)
	{
	  if(low_mega_apply(APPLY_LOW, args-1, o, (void *)(ptrdiff_t)fun))
	  {
	    Pike_fp->save_sp--;
	    Pike_fp->flags |=
	      PIKE_FRAME_RETURN_INTERNAL |
	      PIKE_FRAME_RETURN_POP;
	    DO_JUMP_TO(Pike_fp->pc);
	  }
	  pop_2_elems();
	  DONE;
	}
      }
    }
  }

  {
    struct svalue tmp;
    struct svalue tmp2;

    tmp.type=PIKE_T_STRING;
    tmp.u.string=Pike_fp->context.prog->strings[arg1];
    tmp.subtype=1;

    index_no_free(&tmp2, s, &tmp);
    free_svalue(s);
    *s=tmp2;
    print_return_value();

    if(low_mega_apply(APPLY_STACK, args, 0, 0))
    {
      Pike_fp->flags |= PIKE_FRAME_RETURN_INTERNAL | PIKE_FRAME_RETURN_POP;
      DO_JUMP_TO(Pike_fp->pc);
    }
    pop_stack();
  }
});

OPCODE1(F_CALL_OTHER_AND_RETURN,"call other & return", 0, {
  INT32 args=DO_NOT_WARN((INT32)(Pike_sp - *--Pike_mark_sp));
  struct svalue *s=Pike_sp-args;
  if(s->type == T_OBJECT)
  {
    struct object *o=s->u.object;
    struct program *p;
    if((p=o->prog))
    {
      if(FIND_LFUN(p, LFUN_ARROW) == -1)
      {
	int fun;
	fun=find_shared_string_identifier(Pike_fp->context.prog->strings[arg1],
					  p);
	if(fun >= 0)
	{
	  if(low_mega_apply(APPLY_LOW, args-1, o, (void *)(ptrdiff_t)fun))
	  {
	    PIKE_OPCODE_T *addr = Pike_fp->pc;
	    Pike_fp->save_sp--;
	    DO_IF_DEBUG(Pike_fp->next->pc=0);
	    unlink_previous_frame();
	    DO_JUMP_TO(addr);
	  }
	  stack_unlink(1);
	  DO_DUMB_RETURN;
	}
      }
    }
  }

  {
    struct svalue tmp;
    struct svalue tmp2;

    tmp.type=PIKE_T_STRING;
    tmp.u.string=Pike_fp->context.prog->strings[arg1];
    tmp.subtype=1;

    index_no_free(&tmp2, s, &tmp);
    free_svalue(s);
    *s=tmp2;
    print_return_value();

    if(low_mega_apply(APPLY_STACK, args, 0, 0))
    {
      PIKE_OPCODE_T *addr = Pike_fp->pc;
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
  int args=(ARGS);							 \
  struct svalue *expected_stack=Pike_sp-args;				 \
    struct svalue *s=&Pike_fp->context.prog->constants[arg1].sval;	 \
  if(t_flag>1)								 \
  {									 \
    init_buf();								 \
    describe_svalue(s, 0,0);						 \
    do_trace_call(args);						 \
  }									 \
  (*(s->u.efun->function))(args);					 \
  s->u.efun->runs++;                                                     \
  if(Pike_sp != expected_stack + !s->u.efun->may_return_void)		 \
  {									 \
    if(Pike_sp < expected_stack)					 \
      Pike_fatal("Function popped too many arguments: %s\n",			 \
	    s->u.efun->name->str);					 \
    if(Pike_sp>expected_stack+1)					 \
      Pike_fatal("Function left %d droppings on stack: %s\n",		 \
           Pike_sp-(expected_stack+1),					 \
	    s->u.efun->name->str);					 \
    if(Pike_sp == expected_stack && !s->u.efun->may_return_void)	 \
      Pike_fatal("Non-void function returned without return value "		 \
	    "on stack: %s %d\n",					 \
	    s->u.efun->name->str,s->u.efun->may_return_void);		 \
    if(Pike_sp==expected_stack+1 && s->u.efun->may_return_void)		 \
      Pike_fatal("Void function returned with a value on the stack: %s %d\n", \
	    s->u.efun->name->str, s->u.efun->may_return_void);		 \
  }									 \
  if(t_flag>1 && Pike_sp>expected_stack) trace_return_value();		 \
}while(0)
#else
#define DO_CALL_BUILTIN(ARGS) \
(*(Pike_fp->context.prog->constants[arg1].sval.u.efun->function))(ARGS)
#endif

OPCODE1(F_CALL_BUILTIN, "call builtin", 0, {
  DO_CALL_BUILTIN(DO_NOT_WARN((INT32)(Pike_sp - *--Pike_mark_sp)));
});

OPCODE1(F_CALL_BUILTIN_AND_POP,"call builtin & pop", 0, {
  DO_CALL_BUILTIN(DO_NOT_WARN((INT32)(Pike_sp - *--Pike_mark_sp)));
  pop_stack();
});

OPCODE1_RETURN(F_CALL_BUILTIN_AND_RETURN,"call builtin & return", 0, {
  DO_CALL_BUILTIN(DO_NOT_WARN((INT32)(Pike_sp - *--Pike_mark_sp)));
  DO_DUMB_RETURN;
});


OPCODE1(F_MARK_CALL_BUILTIN, "mark, call builtin", 0, {
  DO_CALL_BUILTIN(0);
});

OPCODE1(F_MARK_CALL_BUILTIN_AND_POP, "mark, call builtin & pop", 0, {
  DO_CALL_BUILTIN(0);
  pop_stack();
});

OPCODE1_RETURN(F_MARK_CALL_BUILTIN_AND_RETURN, "mark, call builtin & return", 0, {
  DO_CALL_BUILTIN(0);
  DO_DUMB_RETURN;
});


OPCODE1(F_CALL_BUILTIN1, "call builtin 1", 0, {
  DO_CALL_BUILTIN(1);
});

OPCODE1(F_CALL_BUILTIN1_AND_POP, "call builtin1 & pop", 0, {
  DO_CALL_BUILTIN(1);
  pop_stack();
});

#ifndef ENTRY_PROLOGUE_SIZE
#define ENTRY_PROLOGUE_SIZE 0
#endif /* !ENTRY_PROLOGUE_SIZE */

#define DO_RECUR(XFLAGS) do{						   \
  PIKE_OPCODE_T *addr;							   \
  register struct pike_frame *new_frame;				   \
  ptrdiff_t args;							   \
									   \
  fast_check_threads_etc(6);						   \
  check_stack(256);							   \
									   \
  new_frame=alloc_pike_frame();						   \
									   \
  new_frame->refs=1;							   \
  new_frame->next=Pike_fp;						   \
									   \
  Pike_fp->return_addr = (PIKE_OPCODE_T *)(((INT32 *)PROG_COUNTER) + 1);   \
  addr = PROG_COUNTER+GET_JUMP();					   \
									   \
  new_frame->num_locals = READ_INCR_BYTE(addr);				   \
  args = READ_INCR_BYTE(addr);						   \
  addr += ENTRY_PROLOGUE_SIZE;						   \
									   \
  new_frame->num_args = new_frame->args = args;				   \
  new_frame->locals=new_frame->save_sp=new_frame->expendible=Pike_sp-args; \
  new_frame->save_mark_sp = new_frame->mark_sp_base = Pike_mark_sp;	   \
                                                                           \
  push_zeroes(new_frame->num_locals - args);				   \
                                                                           \
  DO_IF_DEBUG({								   \
    if(t_flag > 3)							   \
      fprintf(stderr,"-    Allocating %d extra locals.\n",		   \
	      new_frame->num_locals - new_frame->num_args);		   \
  });									   \
									   \
                                                                           \
  SET_PROG_COUNTER(addr);						   \
  new_frame->fun=Pike_fp->fun;						   \
  DO_IF_PROFILING( new_frame->ident=Pike_fp->ident );			   \
  new_frame->current_storage=Pike_fp->current_storage;                     \
  if(Pike_fp->scope) add_ref(new_frame->scope=Pike_fp->scope);		   \
  add_ref(new_frame->current_object=Pike_fp->current_object);		   \
  new_frame->context=Pike_fp->context;                                     \
  add_ref(new_frame->context.prog);					   \
  if(new_frame->context.parent)						   \
    add_ref(new_frame->context.parent);					   \
  Pike_fp=new_frame;							   \
  new_frame->flags=PIKE_FRAME_RETURN_INTERNAL | XFLAGS;			   \
									   \
  FETCH;								   \
  DONE;									   \
}while(0)

/* Assume that the number of arguments is correct */
OPCODE1_JUMP(F_COND_RECUR, "recur if not overloaded", 0, {
  struct program *p = Pike_fp->current_object->prog;
  PIKE_OPCODE_T *addr = (PIKE_OPCODE_T *)(((INT32 *)PROG_COUNTER) + 1);
  Pike_fp->return_addr=addr;

  /* Test if the function is overloaded.
   *
   * Note: The second part of the test is sufficient, but
   *       the since first case is much simpler to test and
   *       is common, it should offer a speed improvement.
   *	/grubba 2002-11-14
   *
   * Also test if the function uses scoping. DO_RECUR() doesn't
   * adjust fp->expendible which will make eg RETURN_LOCAL fail.
   *
   *	/grubba 2003-03-25
   */
  if(((p != Pike_fp->context.prog) &&
      (p->inherits[p->identifier_references[Pike_fp->context.identifier_level +
					    arg1].inherit_offset].prog !=
       Pike_fp->context.prog)) ||
     (ID_FROM_INT(p, arg1+Pike_fp->context.identifier_level)->
      identifier_flags & IDENTIFIER_SCOPE_USED))
  {
    PIKE_OPCODE_T *faddr = PROG_COUNTER+GET_JUMP();
    ptrdiff_t num_locals = READ_INCR_BYTE(faddr);	/* ignored */
    ptrdiff_t args = READ_INCR_BYTE(faddr);

    if(low_mega_apply(APPLY_LOW,
		      args,
		      Pike_fp->current_object,
		      (void *)(ptrdiff_t)(arg1+
					  Pike_fp->context.identifier_level)))
    {
      Pike_fp->flags |= PIKE_FRAME_RETURN_INTERNAL;
      addr = Pike_fp->pc;
    }
    DO_JUMP_TO(addr);
  }

  /* FALL THROUGH */

  /* Assume that the number of arguments is correct */

  OPCODE0_TAILJUMP(F_RECUR, "recur", 0, {
    DO_RECUR(0);
  });
});

/* Ugly code duplication */
OPCODE0_JUMP(F_RECUR_AND_POP, "recur & pop", 0, {
  DO_RECUR(PIKE_FRAME_RETURN_POP);
});


/* Assume that the number of arguments is correct */
/* FIXME: adjust Pike_mark_sp */
OPCODE0_JUMP(F_TAIL_RECUR, "tail recursion", 0, {
  INT32 num_locals;
  PIKE_OPCODE_T *addr;
  INT32 args;

  fast_check_threads_etc(6);

  addr = PROG_COUNTER+GET_JUMP();
  num_locals = READ_INCR_BYTE(addr);
  args = READ_INCR_BYTE(addr);
  addr += ENTRY_PROLOGUE_SIZE;
  SET_PROG_COUNTER(addr);

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

  push_zeroes(num_locals - args);

  DO_IF_DEBUG({
    if(Pike_sp != Pike_fp->locals + Pike_fp->num_locals)
      Pike_fatal("Sp whacked!\n");
  });

  FETCH;
  DONE;
});

OPCODE0(F_BREAKPOINT, "breakpoint", 0, {
  extern void o_breakpoint(void);
  o_breakpoint();
  DO_JUMP_TO(PROG_COUNTER-1);
});

OPCODE1(F_THIS_OBJECT, "this_object", 0, {
  if(Pike_fp)
  {
    struct object *o = Pike_fp->current_object;
    int level;
    for (level = 0; level < arg1; level++) {
      struct program *p = o->prog;
      if (!p)
	Pike_error ("Object %d level(s) up is destructed - cannot get the parent.\n",
		    level);
      if (!(p->flags & PROGRAM_USES_PARENT))
	/* FIXME: Ought to write out the object here. */
	Pike_error ("Object %d level(s) up lacks parent reference.\n", level);
      o = PARENT_INFO(o)->parent;
    }
    ref_push_object(o);
  }else{
    /* Shouldn't this generate an error? /mast */
    push_int(0);
  }
});

OPCODE0(F_ZERO_TYPE, "zero_type", 0, {
  if(Pike_sp[-1].type != T_INT)
  {
    if((Pike_sp[-1].type==T_OBJECT || Pike_sp[-1].type==T_FUNCTION)
       && !Pike_sp[-1].u.object->prog)
    {
      pop_stack();
      push_int(NUMBER_DESTRUCTED);
    }else{
      pop_stack();
      push_int(0);
    }
  }else{
    Pike_sp[-1].u.integer=Pike_sp[-1].subtype;
    Pike_sp[-1].subtype=NUMBER_NUMBER;
  }
});

/*
#undef PROG_COUNTER
*/
