/*
 * $Id: interpret_functions.h,v 1.12 2000/04/20 11:49:50 grubba Exp $
 *
 * Opcode definitions for the interpreter.
 */

OPCODE0(F_UNDEFINED,"push UNDEFINED")
  push_int(0);
  Pike_sp[-1].subtype=NUMBER_UNDEFINED;
BREAK;

OPCODE0(F_CONST0, "push 0")
  push_int(0);
BREAK;

OPCODE0(F_CONST1, "push 1")
  push_int(1);
BREAK;

OPCODE0(F_CONST_1,"push -1")
  push_int(-1);
BREAK;

OPCODE0(F_BIGNUM, "push 0x7fffffff")
  push_int(0x7fffffff);
BREAK;

OPCODE1(F_NUMBER, "push int")
  push_int(arg1);
BREAK;

OPCODE1(F_NEG_NUMBER,"push -int")
  push_int(-arg1);
BREAK;

OPCODE1(F_CONSTANT,"constant")
  assign_svalue_no_free(Pike_sp++,& Pike_fp->context.prog->constants[arg1].sval);
  print_return_value();
BREAK;

/* The rest of the basic 'push value' instructions */	

OPCODE1_TAIL(F_MARK_AND_STRING,"mark & string")
  *(Pike_mark_sp++)=Pike_sp;

OPCODE1(F_STRING,"string")
  copy_shared_string(Pike_sp->u.string,Pike_fp->context.prog->strings[arg1]);
  Pike_sp->type=PIKE_T_STRING;
  Pike_sp->subtype=0;
  Pike_sp++;
  print_return_value();
BREAK;


OPCODE1(F_ARROW_STRING,"->string")
  copy_shared_string(Pike_sp->u.string,Pike_fp->context.prog->strings[arg1]);
  Pike_sp->type=PIKE_T_STRING;
  Pike_sp->subtype=1; /* Magic */
  Pike_sp++;
  print_return_value();
BREAK;

OPCODE0(F_FLOAT,"push float")
  /* FIXME, this opcode uses 'pc' which is not allowed.. */
  Pike_sp->type=PIKE_T_FLOAT;
  MEMCPY((void *)&Pike_sp->u.float_number, pc, sizeof(FLOAT_TYPE));
  pc+=sizeof(FLOAT_TYPE);
  Pike_sp++;
BREAK;

OPCODE1(F_LFUN, "local function")
  Pike_sp->u.object=Pike_fp->current_object;
  add_ref(Pike_fp->current_object);
  Pike_sp->subtype=arg1+Pike_fp->context.identifier_level;
  Pike_sp->type=PIKE_T_FUNCTION;
  Pike_sp++;
  print_return_value();
BREAK;


OPCODE1(F_TRAMPOLINE, "trampoline")
{
  struct object *o=low_clone(pike_trampoline_program);
  add_ref( ((struct pike_trampoline *)(o->storage))->frame=Pike_fp );
  ((struct pike_trampoline *)(o->storage))->func=arg1+Pike_fp->context.identifier_level;
  push_object(o);
  /* Make it look like a function. */
  Pike_sp[-1].subtype = pike_trampoline_program->lfuns[LFUN_CALL];
  Pike_sp[-1].type = T_FUNCTION;
  print_return_value();
}
BREAK;

/* The not so basic 'push value' instructions */

OPCODE1(F_GLOBAL,"global")
  low_object_index_no_free(Pike_sp,
			   Pike_fp->current_object,
			   arg1 + Pike_fp->context.identifier_level);
  Pike_sp++;
  print_return_value();
BREAK;


OPCODE2(F_EXTERNAL,"external")
{
  struct inherit *inherit;
  struct program *p;
  struct object *o;
  INT32 i;
  
  inherit=&Pike_fp->context;
  
  o=Pike_fp->current_object;
  
  if(!o)
    error("Current object is destructed\n");
  
  while(1)
  {
    if(inherit->parent_offset)
    {
#ifdef PIKE_DEBUG
      if(t_flag>4)
      {
	sprintf(trace_buffer,"-   Following o->parent (accumulator+=%d)\n",inherit->parent_offset-1);
	write_to_stderr(trace_buffer,strlen(trace_buffer));
      }
#endif
      
      i=o->parent_identifier;
      o=o->parent;
      arg2+=inherit->parent_offset-1;
    }else{
#ifdef PIKE_DEBUG
      if(t_flag>4)
      {
	sprintf(trace_buffer,"-   Following inherit->parent (accumulator+=%d)\n",inherit->parent_offset-1);
	write_to_stderr(trace_buffer,strlen(trace_buffer));
      }
#endif
      i=inherit->parent_identifier;
      o=inherit->parent;
    }
    
    if(!o)
      error("Parent was lost during cloning.\n");
    
    if(!(p=o->prog))
      error("Attempting to access variable in destructed object\n");
    
#ifdef DEBUG_MALLOC
    if (o->refs == 0x55555555) {
      fprintf(stderr, "The object %p has been zapped!\n", o);
      describe(p);
      fatal("Object zapping detected.\n");
    }
    if (p->refs == 0x55555555) {
      fprintf(stderr, "The program %p has been zapped!\n", p);
      describe(p);
      fprintf(stderr, "Which taken from the object %p\n", o);
      describe(o);
      fatal("Looks like the program %p has been zapped!\n", p);
    }
#endif /* DEBUG_MALLOC */
    
#ifdef PIKE_DEBUG
    if(i < 0 || i > p->num_identifier_references)
      fatal("Identifier out of range!\n");
#endif
    
    inherit=INHERIT_FROM_INT(p, i);
    
#ifdef DEBUG_MALLOC
    if (inherit->storage_offset == 0x55555555) {
      fprintf(stderr, "The inherit %p has been zapped!\n", inherit);
      debug_malloc_dump_references(inherit,0,2,0);
      fprintf(stderr, "It was extracted from the program %p %d\n", p, i);
      describe(p);
      fprintf(stderr, "Which was in turn taken from the object %p\n", o);
      describe(o);
      fatal("Looks like the program %p has been zapped!\n", p);
    }
#endif /* DEBUG_MALLOC */
    
    if(!arg2) break;
    --arg2;
  }
  
  low_object_index_no_free(Pike_sp,
			   o,
			   arg1 + inherit->identifier_level);
  Pike_sp++;
  print_return_value();
  break;
}
BREAK;

OPCODE2(F_EXTERNAL_LVALUE,"& external")
{
  struct inherit *inherit;
  struct program *p;
  struct object *o;
  INT32 i,id=arg1;
  
  inherit=&Pike_fp->context;
  o=Pike_fp->current_object;
  
  if(!o)
    error("Current object is destructed\n");
  
  while(1)
  {
    if(inherit->parent_offset)
    {
      i=o->parent_identifier;
      o=o->parent;
      arg2+=inherit->parent_offset-1;
    }else{
      i=inherit->parent_identifier;
      o=inherit->parent;
    }
    
    if(!o)
      error("Parent no longer exists\n");
    
    if(!(p=o->prog))
      error("Attempting to access variable in destructed object\n");
    
    inherit=INHERIT_FROM_INT(p, i);
    
    if(!arg2) break;
    arg2--;
  }
  
  ref_push_object(o);
  Pike_sp->type=T_LVALUE;
  Pike_sp->u.integer=id + inherit->identifier_level;
  Pike_sp++;
  break;
}

BREAK;


      CASE(F_MARK_AND_LOCAL); *(Pike_mark_sp++)=Pike_sp;
      CASE(F_LOCAL);
      assign_svalue_no_free(Pike_sp++,Pike_fp->locals+GET_ARG());
      print_return_value();
      break;

OPCODE2(F_2_LOCALS, "2 locals")
  assign_svalue_no_free(Pike_sp++, Pike_fp->locals + arg1);
  print_return_value();
  assign_svalue_no_free(Pike_sp++, Pike_fp->locals + arg2);
  print_return_value();
BREAK;

OPCODE2(F_LOCAL_2_LOCAL, "local = local")
  assign_svalue(Pike_fp->locals + arg1, Pike_fp->locals + arg2);
BREAK;

OPCODE2(F_LOCAL_2_GLOBAL, "global = local")
{
  INT32 tmp = arg1 + Pike_fp->context.identifier_level;
  struct identifier *i;

  if(!Pike_fp->current_object->prog)
    error("Cannot access global variables in destructed object.\n");

  i = ID_FROM_INT(Pike_fp->current_object->prog, tmp);
  if(!IDENTIFIER_IS_VARIABLE(i->identifier_flags))
    error("Cannot assign functions or constants.\n");
  if(i->run_time_type == T_MIXED)
  {
    assign_svalue((struct svalue *)GLOBAL_FROM_INT(tmp),
		  Pike_fp->locals + arg2);
  }else{
    assign_to_short_svalue((union anything *)GLOBAL_FROM_INT(tmp),
			   i->run_time_type,
			   Pike_fp->locals + arg2);
  }
}
BREAK;

OPCODE2(F_GLOBAL_2_LOCAL,"local = global")
{
  INT32 tmp = arg1 + Pike_fp->context.identifier_level;
  free_svalue(Pike_fp->locals + arg2);
  low_object_index_no_free(Pike_fp->locals + arg2,
			   Pike_fp->current_object,
			   tmp);
}
BREAK;

OPCODE1(F_LOCAL_LVALUE, "& local")
  Pike_sp[0].type = T_LVALUE;
  Pike_sp[0].u.lval = Pike_fp->locals + arg1;
  Pike_sp[1].type = T_VOID;
  Pike_sp += 2;
BREAK;

OPCODE2(F_LEXICAL_LOCAL,"lexical local")
{
  struct pike_frame *f=Pike_fp;
  while(arg2--)
  {
    f=f->scope;
    if(!f) error("Lexical scope error.\n");
  }
  push_svalue(f->locals + arg1);
  print_return_value();
  break;
}
BREAK;


OPCODE2(F_LEXICAL_LOCAL_LVALUE,"&lexical local")
{
  struct pike_frame *f=Pike_fp;
  while(arg2--)
  {
    f=f->scope;
    if(!f) error("Lexical scope error.\n");
  }
  Pike_sp[0].type=T_LVALUE;
  Pike_sp[0].u.lval=f->locals+arg1;
  Pike_sp[1].type=T_VOID;
  Pike_sp+=2;
  break;
}
BREAK;

OPCODE1(F_ARRAY_LVALUE, "[ lvalues ]")
  f_aggregate(arg1*2);
  Pike_sp[-1].u.array->flags |= ARRAY_LVALUE;
  Pike_sp[-1].u.array->type_field |= BIT_UNFINISHED | BIT_MIXED;
  /* FIXME: Shouldn't a ref be added here? */
  Pike_sp[0] = Pike_sp[-1];
  Pike_sp[-1].type = T_ARRAY_LVALUE;
  dmalloc_touch_svalue(Pike_sp);
  Pike_sp++;
BREAK;

OPCODE1(F_CLEAR_2_LOCAL, "clear 2 local")
  instr = arg1;
  free_svalues(Pike_fp->locals + instr, 2, -1);
  Pike_fp->locals[instr].type = PIKE_T_INT;
  Pike_fp->locals[instr].subtype = 0;
  Pike_fp->locals[instr].u.integer = 0;
  Pike_fp->locals[instr+1].type = PIKE_T_INT;
  Pike_fp->locals[instr+1].subtype = 0;
  Pike_fp->locals[instr+1].u.integer = 0;
BREAK;

OPCODE1(F_CLEAR_4_LOCAL, "clear 4 local")
{
  int e;
  instr = arg1;
  free_svalues(Pike_fp->locals + instr, 4, -1);
  for(e = 0; e < 4; e++)
  {
    Pike_fp->locals[instr+e].type = PIKE_T_INT;
    Pike_fp->locals[instr+e].subtype = 0;
    Pike_fp->locals[instr+e].u.integer = 0;
  }
}
BREAK;

OPCODE1(F_CLEAR_LOCAL, "clear local")
  instr = arg1;
  free_svalue(Pike_fp->locals + instr);
  Pike_fp->locals[instr].type = PIKE_T_INT;
  Pike_fp->locals[instr].subtype = 0;
  Pike_fp->locals[instr].u.integer = 0;
BREAK;

OPCODE1(F_INC_LOCAL, "++local")
  instr = arg1;
  if( (Pike_fp->locals[instr].type == PIKE_T_INT)
#ifdef AUTO_BIGNUM
      && (!INT_TYPE_ADD_OVERFLOW(Pike_fp->locals[instr].u.integer, 1))
#endif /* AUTO_BIGNUM */
      )
  {
    Pike_fp->locals[instr].u.integer++;
    assign_svalue_no_free(Pike_sp++,Pike_fp->locals+instr);
  } else {
    assign_svalue_no_free(Pike_sp++,Pike_fp->locals+instr);
    push_int(1);
    f_add(2);
    assign_svalue(Pike_fp->locals+instr,Pike_sp-1);
  }
BREAK;

      CASE(F_POST_INC_LOCAL);
      instr=GET_ARG();
      assign_svalue_no_free(Pike_sp++,Pike_fp->locals+instr);
      goto inc_local_and_pop;

      CASE(F_INC_LOCAL_AND_POP);
      instr=GET_ARG();
    inc_local_and_pop:
#ifdef AUTO_BIGNUM
      if(Pike_fp->locals[instr].type == PIKE_T_INT &&
         !INT_TYPE_ADD_OVERFLOW(Pike_fp->locals[instr].u.integer, 1))
#else
      if(Pike_fp->locals[instr].type == PIKE_T_INT)
#endif /* AUTO_BIGNUM */
      {
	Pike_fp->locals[instr].u.integer++;
      }else{
	assign_svalue_no_free(Pike_sp++,Pike_fp->locals+instr);
	push_int(1);
	f_add(2);
	assign_svalue(Pike_fp->locals+instr,Pike_sp-1);
	pop_stack();
      }
      break;

OPCODE1(F_DEC_LOCAL, "--local")
  instr = arg1;
  if( (Pike_fp->locals[instr].type == PIKE_T_INT)
#ifdef AUTO_BIGNUM
      && (!INT_TYPE_SUB_OVERFLOW(Pike_fp->locals[instr].u.integer, 1))
#endif /* AUTO_BIGNUM */
      )
  {
    Pike_fp->locals[instr].u.integer--;
    assign_svalue_no_free(Pike_sp++,Pike_fp->locals+instr);
  } else {
    assign_svalue_no_free(Pike_sp++,Pike_fp->locals+instr);
    push_int(1);
    o_subtract();
    assign_svalue(Pike_fp->locals+instr,Pike_sp-1);
  }
BREAK;

      CASE(F_POST_DEC_LOCAL);
      instr=GET_ARG();
      assign_svalue_no_free(Pike_sp++,Pike_fp->locals+instr);
      goto dec_local_and_pop;
      /* Pike_fp->locals[instr].u.integer--; */
      break;

      CASE(F_DEC_LOCAL_AND_POP);
      instr=GET_ARG();
    dec_local_and_pop:
#ifdef AUTO_BIGNUM
      if(Pike_fp->locals[instr].type == PIKE_T_INT &&
         !INT_TYPE_SUB_OVERFLOW(Pike_fp->locals[instr].u.integer, 1))
#else
      if(Pike_fp->locals[instr].type == PIKE_T_INT)
#endif /* AUTO_BIGNUM */
      {
	Pike_fp->locals[instr].u.integer--;
      }else{
	assign_svalue_no_free(Pike_sp++,Pike_fp->locals+instr);
	push_int(1);
	o_subtract();
	assign_svalue(Pike_fp->locals+instr,Pike_sp-1);
	pop_stack();
      }
      break;

OPCODE0(F_LTOSVAL, "lvalue to svalue")
  lvalue_to_svalue_no_free(Pike_sp, Pike_sp-2);
  Pike_sp++;
BREAK;

OPCODE0(F_LTOSVAL2, "ltosval2")
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
BREAK;


OPCODE0(F_ADD_TO_AND_POP, "+= and pop")
  Pike_sp[0]=Pike_sp[-1];
  Pike_sp[-1].type=PIKE_T_INT;
  Pike_sp++;
  lvalue_to_svalue_no_free(Pike_sp-2,Pike_sp-4);

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
BREAK;

OPCODE1(F_GLOBAL_LVALUE, "& global")
{
  struct identifier *i;
  INT32 tmp=arg1 + Pike_fp->context.identifier_level;
  if(!Pike_fp->current_object->prog)
    error("Cannot access global variables in destructed object.\n");
  i=ID_FROM_INT(Pike_fp->current_object->prog, tmp);

  if(!IDENTIFIER_IS_VARIABLE(i->identifier_flags))
    error("Cannot re-assign functions or constants.\n");

  if(i->run_time_type == T_MIXED)
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
}
BREAK;

      
OPCODE0(F_INC, "++x")
{
  union anything *u=get_pointer_if_this_type(Pike_sp-2, PIKE_T_INT);
  if(u
#ifdef AUTO_BIGNUM
     && !INT_TYPE_ADD_OVERFLOW(u->integer, 1)
#endif
     )
  {
    instr=++ u->integer;
    pop_n_elems(2);
    push_int(u->integer);
  } else {
    lvalue_to_svalue_no_free(Pike_sp, Pike_sp-2); Pike_sp++;
    push_int(1);
    f_add(2);
    assign_lvalue(Pike_sp-3, Pike_sp-1);
    assign_svalue(Pike_sp-3, Pike_sp-1);
    pop_n_elems(2);
  }
}
BREAK;

OPCODE0(F_DEC, "--x")
{
  union anything *u=get_pointer_if_this_type(Pike_sp-2, PIKE_T_INT);
  if(u
#ifdef AUTO_BIGNUM
     && !INT_TYPE_SUB_OVERFLOW(u->integer, 1)
#endif
     )
  {
    instr=-- u->integer;
    pop_n_elems(2);
    push_int(u->integer);
  } else {
    lvalue_to_svalue_no_free(Pike_sp, Pike_sp-2); Pike_sp++;
    push_int(1);
    o_subtract();
    assign_lvalue(Pike_sp-3, Pike_sp-1);
    assign_svalue(Pike_sp-3, Pike_sp-1);
    pop_n_elems(2);
  }
}
BREAK;

OPCODE0(F_DEC_AND_POP, "x-- and pop")
{
  union anything *u=get_pointer_if_this_type(Pike_sp-2, PIKE_T_INT);
  if(u
#ifdef AUTO_BIGNUM
     && !INT_TYPE_SUB_OVERFLOW(u->integer, 1)
#endif
)
  {
    instr=-- u->integer;
    pop_n_elems(2);
  }else{
    lvalue_to_svalue_no_free(Pike_sp, Pike_sp-2); Pike_sp++;
    push_int(1);
    o_subtract();
    assign_lvalue(Pike_sp-3, Pike_sp-1);
    pop_n_elems(3);
  }
}
BREAK;

OPCODE0(F_INC_AND_POP, "x++ and pop")
{
  union anything *u=get_pointer_if_this_type(Pike_sp-2, PIKE_T_INT);
  if(u
#ifdef AUTO_BIGNUM
     && !INT_TYPE_ADD_OVERFLOW(u->integer, 1)
#endif
     )
  {
    instr=++ u->integer;
    pop_n_elems(2);
  } else {
    lvalue_to_svalue_no_free(Pike_sp, Pike_sp-2); Pike_sp++;
    push_int(1);
    f_add(2);
    assign_lvalue(Pike_sp-3, Pike_sp-1);
    pop_n_elems(3);
  }
}
BREAK;

OPCODE0(F_POST_INC, "x++")
{
  union anything *u=get_pointer_if_this_type(Pike_sp-2, PIKE_T_INT);
  if(u
#ifdef AUTO_BIGNUM
     && !INT_TYPE_ADD_OVERFLOW(u->integer, 1)
#endif
     )
  {
    instr=u->integer ++;
    pop_n_elems(2);
    push_int(instr);
  } else {
    lvalue_to_svalue_no_free(Pike_sp, Pike_sp-2); Pike_sp++;
    assign_svalue_no_free(Pike_sp,Pike_sp-1); Pike_sp++;
    push_int(1);
    f_add(2);
    assign_lvalue(Pike_sp-4, Pike_sp-1);
    assign_svalue(Pike_sp-4, Pike_sp-2);
    pop_n_elems(3);
  }
}
BREAK;


OPCODE0(F_POST_DEC, "x--")
{
  union anything *u=get_pointer_if_this_type(Pike_sp-2, PIKE_T_INT);
  if(u
#ifdef AUTO_BIGNUM
     && !INT_TYPE_SUB_OVERFLOW(u->integer, 1)
#endif
     )
  {
    instr=u->integer --;
    pop_n_elems(2);
    push_int(instr);
  } else {
    lvalue_to_svalue_no_free(Pike_sp, Pike_sp-2); Pike_sp++;
    assign_svalue_no_free(Pike_sp,Pike_sp-1); Pike_sp++;
    push_int(1);
    o_subtract();
    assign_lvalue(Pike_sp-4, Pike_sp-1);
    assign_svalue(Pike_sp-4, Pike_sp-2);
    pop_n_elems(3);
  }
}
BREAK;

OPCODE1(F_ASSIGN_LOCAL,"assign local")
  assign_svalue(Pike_fp->locals+arg1,Pike_sp-1);
BREAK;

OPCODE0(F_ASSIGN, "assign")
  assign_lvalue(Pike_sp-3,Pike_sp-1);
  free_svalue(Pike_sp-3);
  free_svalue(Pike_sp-2);
  Pike_sp[-3]=Pike_sp[-1];
  Pike_sp-=2;
BREAK;

OPCODE2(F_APPLY_ASSIGN_LOCAL_AND_POP,"apply, assign local and pop")
  strict_apply_svalue(Pike_fp->context.prog->constants + arg1, Pike_sp - *--Pike_mark_sp );
  free_svalue(Pike_fp->locals+arg2);
  Pike_fp->locals[arg2]=Pike_sp[-1];
  Pike_sp--;
BREAK;

OPCODE2(F_APPLY_ASSIGN_LOCAL,"apply, assign local")
  strict_apply_svalue(Pike_fp->context.prog->constants + arg1, Pike_sp - *--Pike_mark_sp );
  assign_svalue(Pike_fp->locals+arg2,Pike_sp-1);
BREAK;

OPCODE0(F_ASSIGN_AND_POP, "assign and pop")
  assign_lvalue(Pike_sp-3, Pike_sp-1);
  pop_n_elems(3);
BREAK;


      CASE(F_ASSIGN_LOCAL_AND_POP);
      instr=GET_ARG();
      free_svalue(Pike_fp->locals+instr);
      Pike_fp->locals[instr]=Pike_sp[-1];
      Pike_sp--;
      break;

OPCODE1(F_ASSIGN_GLOBAL, "assign global")
{
  struct identifier *i;
  INT32 tmp=arg1 + Pike_fp->context.identifier_level;
  if(!Pike_fp->current_object->prog)
    error("Cannot access global variables in destructed object.\n");

  i=ID_FROM_INT(Pike_fp->current_object->prog, tmp);
  if(!IDENTIFIER_IS_VARIABLE(i->identifier_flags))
    error("Cannot assign functions or constants.\n");
  if(i->run_time_type == T_MIXED)
  {
    assign_svalue((struct svalue *)GLOBAL_FROM_INT(tmp), Pike_sp-1);
  }else{
    assign_to_short_svalue((union anything *)GLOBAL_FROM_INT(tmp),
			   i->run_time_type,
			   Pike_sp-1);
  }
}
BREAK;

OPCODE1(F_ASSIGN_GLOBAL_AND_POP, "assign global and pop")
{
  struct identifier *i;
  INT32 tmp=arg1 + Pike_fp->context.identifier_level;
  if(!Pike_fp->current_object->prog)
    error("Cannot access global variables in destructed object.\n");

  i=ID_FROM_INT(Pike_fp->current_object->prog, tmp);
  if(!IDENTIFIER_IS_VARIABLE(i->identifier_flags))
    error("Cannot assign functions or constants.\n");

  if(i->run_time_type == T_MIXED)
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
}
BREAK;


/* Stack machine stuff */

OPCODE0(F_POP_VALUE, "pop")
  pop_stack();
BREAK;

OPCODE1(F_POP_N_ELEMS, "pop_n_elems")
  pop_n_elems(arg1);
BREAK;

      CASE(F_MARK2); *(Pike_mark_sp++)=Pike_sp;
      CASE(F_MARK); *(Pike_mark_sp++)=Pike_sp; break;

OPCODE1(F_MARK_X, "mark sp-X")
  *(Pike_mark_sp++)=Pike_sp-arg1;
BREAK;

OPCODE0(F_POP_MARK, "pop mark")
  --Pike_mark_sp;
BREAK;

OPCODE0(F_CLEAR_STRING_SUBTYPE, "clear string subtype")
  if(Pike_sp[-1].type==PIKE_T_STRING) Pike_sp[-1].subtype=0;
BREAK;

      /* Jumps */
      CASE(F_BRANCH);
      DOJUMP();
      break;

      CASE(F_BRANCH_IF_NOT_LOCAL_ARROW);
      {
	struct svalue tmp;
	tmp.type=PIKE_T_STRING;
	tmp.u.string=Pike_fp->context.prog->strings[GET_ARG()];
	tmp.subtype=1;
	Pike_sp->type=PIKE_T_INT;	
	Pike_sp++;
	index_no_free(Pike_sp-1,Pike_fp->locals+GET_ARG2() , &tmp);
	print_return_value();
      }

      /* Fall through */

      CASE(F_BRANCH_WHEN_ZERO);
      if(!IS_ZERO(Pike_sp-1))
      {
	pc+=sizeof(INT32);
      }else{
	DOJUMP();
      }
      pop_stack();
      break;

      
      CASE(F_BRANCH_WHEN_NON_ZERO);
      if(IS_ZERO(Pike_sp-1))
      {
	pc+=sizeof(INT32);
      }else{
	DOJUMP();
      }
      pop_stack();
      break;

      CASE(F_BRANCH_IF_LOCAL);
      instr=GET_ARG();
      if(IS_ZERO(Pike_fp->locals + instr))
      {
	pc+=sizeof(INT32);
      }else{
	DOJUMP();
      }
      break;

      CASE(F_BRANCH_IF_NOT_LOCAL);
      instr=GET_ARG();
      if(!IS_ZERO(Pike_fp->locals + instr))
      {
	pc+=sizeof(INT32);
      }else{
	DOJUMP();
      }
      break;

      CJUMP(F_BRANCH_WHEN_EQ, is_eq);
      CJUMP(F_BRANCH_WHEN_NE,!is_eq);
      CJUMP(F_BRANCH_WHEN_LT, is_lt);
      CJUMP(F_BRANCH_WHEN_LE,!is_gt);
      CJUMP(F_BRANCH_WHEN_GT, is_gt);
      CJUMP(F_BRANCH_WHEN_GE,!is_lt);

      CASE(F_BRANCH_AND_POP_WHEN_ZERO);
      if(!IS_ZERO(Pike_sp-1))
      {
	pc+=sizeof(INT32);
      }else{
	DOJUMP();
	pop_stack();
      }
      break;

      CASE(F_BRANCH_AND_POP_WHEN_NON_ZERO);
      if(IS_ZERO(Pike_sp-1))
      {
	pc+=sizeof(INT32);
      }else{
	DOJUMP();
	pop_stack();
      }
      break;

      CASE(F_LAND);
      if(!IS_ZERO(Pike_sp-1))
      {
	pc+=sizeof(INT32);
	pop_stack();
      }else{
	DOJUMP();
      }
      break;

      CASE(F_LOR);
      if(IS_ZERO(Pike_sp-1))
      {
	pc+=sizeof(INT32);
	pop_stack();
      }else{
	DOJUMP();
      }
      break;

      CASE(F_EQ_OR);
      if(!is_eq(Pike_sp-2,Pike_sp-1))
      {
	pop_n_elems(2);
	pc+=sizeof(INT32);
      }else{
	pop_n_elems(2);
	push_int(1);
	DOJUMP();
      }
      break;

      CASE(F_EQ_AND);
      if(is_eq(Pike_sp-2,Pike_sp-1))
      {
	pop_n_elems(2);
	pc+=sizeof(INT32);
      }else{
	pop_n_elems(2);
	push_int(0);
	DOJUMP();
      }
      break;

      CASE(F_CATCH);
      if(o_catch(pc+sizeof(INT32)))
	return -1; /* There was a return inside the evaluated code */
      else
	pc+=EXTRACT_INT(pc);
      break;

OPCODE0(F_THROW_ZERO, "throw(0)")
  push_int(0);
  f_throw(1);
BREAK;

OPCODE1(F_SWITCH, "switch")
{
  INT32 tmp;
  tmp=switch_lookup(Pike_fp->context.prog->
		    constants[arg1].sval.u.array,Pike_sp-1);
  pc=(unsigned char *)DO_ALIGN(pc,sizeof(INT32));
  pc+=(tmp>=0 ? 1+tmp*2 : 2*~tmp) * sizeof(INT32);
  if(*(INT32*)pc < 0) fast_check_threads_etc(7);
  pc+=*(INT32*)pc;
  pop_stack();
}
BREAK;

      /* FIXME: Does this need bignum tests? /Fixed - Hubbe */
      LOOP(F_INC_LOOP, 1, <, is_lt);
      LOOP(F_DEC_LOOP, -1, >, is_gt);
      LOOP(F_INC_NEQ_LOOP, 1, !=, !is_eq);
      LOOP(F_DEC_NEQ_LOOP, -1, !=, !is_eq);

      CASE(F_FOREACH) /* array, lvalue, X, i */
      {
	if(Pike_sp[-4].type != PIKE_T_ARRAY)
	  PIKE_ERROR("foreach", "Bad argument 1.\n", Pike_sp-3, 1);
	if(Pike_sp[-1].u.integer < Pike_sp[-4].u.array->size)
	{
	  fast_check_threads_etc(10);
	  index_no_free(Pike_sp,Pike_sp-4,Pike_sp-1);
	  Pike_sp++;
	  assign_lvalue(Pike_sp-4, Pike_sp-1);
	  free_svalue(Pike_sp-1);
	  Pike_sp--;
	  pc+=EXTRACT_INT(pc);
	  Pike_sp[-1].u.integer++;
	}else{
	  pc+=sizeof(INT32);
	}
	break;
      }

      CASE(F_APPLY_AND_RETURN);
      {
	INT32 args=Pike_sp - *--Pike_mark_sp;
/*	fprintf(stderr,"%p >= %p\n",Pike_fp->expendible,Pike_sp-args); */
	if(Pike_fp->expendible >= Pike_sp-args)
	{
/*	  fprintf(stderr,"NOT EXPENDIBLE!\n"); */
	  MEMMOVE(Pike_sp-args+1,Pike_sp-args,args*sizeof(struct svalue));
	  Pike_sp++;
	  Pike_sp[-args-1].type=PIKE_T_INT;
	}
	/* We sabotage the stack here */
	assign_svalue(Pike_sp-args-1,&Pike_fp->context.prog->constants[GET_ARG()].sval);
	return args+1;
      }

      CASE(F_CALL_LFUN_AND_RETURN);
      {
	INT32 args=Pike_sp - *--Pike_mark_sp;
	if(Pike_fp->expendible >= Pike_sp-args)
	{
	  MEMMOVE(Pike_sp-args+1,Pike_sp-args,args*sizeof(struct svalue));
	  Pike_sp++;
	  Pike_sp[-args-1].type=PIKE_T_INT;
	}else{
	  free_svalue(Pike_sp-args-1);
	}
	/* More stack sabotage */
	Pike_sp[-args-1].u.object=Pike_fp->current_object;
	Pike_sp[-args-1].subtype=GET_ARG()+Pike_fp->context.identifier_level;
#ifdef PIKE_DEBUG
	if(t_flag > 9)
	  fprintf(stderr,"-    IDENTIFIER_LEVEL: %d\n",Pike_fp->context.identifier_level);
#endif
	Pike_sp[-args-1].type=PIKE_T_FUNCTION;
	add_ref(Pike_fp->current_object);

	return args+1;
      }

      CASE(F_RETURN_LOCAL);
      instr=GET_ARG();
      if(Pike_fp->expendible <= Pike_fp->locals+instr)
      {
	pop_n_elems(Pike_sp-1 - (Pike_fp->locals+instr));
      }else{
	push_svalue(Pike_fp->locals+instr);
      }
      print_return_value();
      goto do_return;

      CASE(F_RETURN_IF_TRUE);
      if(!IS_ZERO(Pike_sp-1))
	goto do_return;
      pop_stack();
      break;

      CASE(F_RETURN_1);
      push_int(1);
      goto do_return;

      CASE(F_RETURN_0);
      push_int(0);
      goto do_return;

      CASE(F_RETURN);
    do_return:
#if defined(PIKE_DEBUG) && defined(GC2)
      if(d_flag>3) do_gc();
      if(d_flag>4) do_debug();
      check_threads_etc();
#endif

      /* fall through */

      CASE(F_DUMB_RETURN);
      return -1;

OPCODE0(F_NEGATE, "unary minus")
  if(Pike_sp[-1].type == PIKE_T_INT)
  {
#ifdef AUTO_BIGNUM
    if(INT_TYPE_NEG_OVERFLOW(Pike_sp[-1].u.integer))
    {
      convert_stack_top_to_bignum();
      o_negate();
    }
    else
#endif /* AUTO_BIGNUM */
      Pike_sp[-1].u.integer =- Pike_sp[-1].u.integer;
  }
  else if(Pike_sp[-1].type == PIKE_T_FLOAT)
  {
    Pike_sp[-1].u.float_number =- Pike_sp[-1].u.float_number;
  }else{
    o_negate();
  }
BREAK;

OPCODE0(F_COMPL, "~")
  o_compl();
BREAK;

OPCODE0(F_NOT, "!")
  switch(Pike_sp[-1].type)
  {
  case PIKE_T_INT:
    Pike_sp[-1].u.integer =! Pike_sp[-1].u.integer;
    break;

  case PIKE_T_FUNCTION:
  case PIKE_T_OBJECT:
    if(IS_ZERO(Pike_sp-1))
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
    Pike_sp[-1].u.integer=0;
  }
BREAK;

OPCODE0(F_LSH, "<<")
  o_lsh();
BREAK;

OPCODE0(F_RSH, ">>")
  o_rsh();
BREAK;

      COMPARISMENT(F_EQ, is_eq(Pike_sp-2,Pike_sp-1));
      COMPARISMENT(F_NE,!is_eq(Pike_sp-2,Pike_sp-1));
      COMPARISMENT(F_GT, is_gt(Pike_sp-2,Pike_sp-1));
      COMPARISMENT(F_GE,!is_lt(Pike_sp-2,Pike_sp-1));
      COMPARISMENT(F_LT, is_lt(Pike_sp-2,Pike_sp-1));
      COMPARISMENT(F_LE,!is_gt(Pike_sp-2,Pike_sp-1));

      CASE(F_ADD);      f_add(2);     break;
      CASE(F_SUBTRACT); o_subtract(); break;
      CASE(F_AND);      o_and();      break;
      CASE(F_OR);       o_or();       break;
      CASE(F_XOR);      o_xor();      break;
      CASE(F_MULTIPLY); o_multiply(); break;
      CASE(F_DIVIDE);   o_divide();   break;
      CASE(F_MOD);      o_mod();      break;

      CASE(F_ADD_INT); push_int(GET_ARG()); f_add(2); break;
      CASE(F_ADD_NEG_INT); push_int(-GET_ARG()); f_add(2); break;

OPCODE0(F_PUSH_ARRAY, "@")
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
      error("Bad return type from o->_values() in @\n");
    free_svalue(Pike_sp-2);
    Pike_sp[-2]=Pike_sp[-1];
    Pike_sp--;
    break;

  case PIKE_T_ARRAY: break;
  }
  Pike_sp--;
  push_array_items(Pike_sp->u.array);
BREAK;

OPCODE2(F_LOCAL_LOCAL_INDEX, "local local index")
{
  struct svalue *s=Pike_fp->locals+arg1;
  if(s->type == PIKE_T_STRING) s->subtype=0;
  Pike_sp++->type=PIKE_T_INT;
  index_no_free(Pike_sp-1,Pike_fp->locals+arg2,s);
}
BREAK;

OPCODE1(F_LOCAL_INDEX, "local index")
{
  struct svalue tmp,*s=Pike_fp->locals+arg1;
  if(s->type == PIKE_T_STRING) s->subtype=0;
  index_no_free(&tmp,Pike_sp-1,s);
  free_svalue(Pike_sp-1);
  Pike_sp[-1]=tmp;
}
BREAK;

OPCODE2(F_GLOBAL_LOCAL_INDEX, "global[local]")
{
  struct svalue tmp,*s;
  low_object_index_no_free(Pike_sp,
			   Pike_fp->current_object,
			   arg1 + Pike_fp->context.identifier_level);
  Pike_sp++;
  s=Pike_fp->locals+arg2;
  if(s->type == PIKE_T_STRING) s->subtype=0;
  index_no_free(&tmp,Pike_sp-1,s);
  free_svalue(Pike_sp-1);
  Pike_sp[-1]=tmp;
}
BREAK;

OPCODE2(F_LOCAL_ARROW, "local->x")
{
  struct svalue tmp;
  tmp.type=PIKE_T_STRING;
  tmp.u.string=Pike_fp->context.prog->strings[arg1];
  tmp.subtype=1;
  Pike_sp->type=PIKE_T_INT;	
  Pike_sp++;
  index_no_free(Pike_sp-1,Pike_fp->locals+arg2, &tmp);
  print_return_value();
}
BREAK;

OPCODE1(F_ARROW, "->x")
{
  struct svalue tmp,tmp2;
  tmp.type=PIKE_T_STRING;
  tmp.u.string=Pike_fp->context.prog->strings[arg1];
  tmp.subtype=1;
  index_no_free(&tmp2, Pike_sp-1, &tmp);
  free_svalue(Pike_sp-1);
  Pike_sp[-1]=tmp2;
  print_return_value();
}
BREAK;

OPCODE1(F_STRING_INDEX, "string index")
{
  struct svalue tmp,tmp2;
  tmp.type=PIKE_T_STRING;
  tmp.u.string=Pike_fp->context.prog->strings[arg1];
  tmp.subtype=0;
  index_no_free(&tmp2, Pike_sp-1, &tmp);
  free_svalue(Pike_sp-1);
  Pike_sp[-1]=tmp2;
  print_return_value();
}
BREAK;

      CASE(F_POS_INT_INDEX);
      push_int(GET_ARG());
      print_return_value();
      goto do_index;

      CASE(F_NEG_INT_INDEX);
      push_int(-GET_ARG());
      print_return_value();

      CASE(F_INDEX);
    do_index:
      {
	struct svalue s;
	index_no_free(&s,Pike_sp-2,Pike_sp-1);
	pop_n_elems(2);
	*Pike_sp=s;
	Pike_sp++;
      }
      print_return_value();
      break;

OPCODE2(F_MAGIC_INDEX, "::`[]")
  push_magic_index(magic_index_program, arg2, arg1);
BREAK;

OPCODE2(F_MAGIC_SET_INDEX, "::`[]=")
  push_magic_index(magic_set_index_program, arg2, arg1);
BREAK;

OPCODE0(F_CAST, "cast")
  f_cast();
BREAK;

OPCODE0(F_SOFT_CAST, "soft cast")
  /* Stack: type_string, value */
#ifdef PIKE_DEBUG
  if (Pike_sp[-2].type != T_STRING) {
    /* FIXME: The type should really be T_TYPE... */
    fatal("Argument 1 to soft_cast isn't a string!\n");
  }
#endif /* PIKE_DEBUG */
  if (runtime_options & RUNTIME_CHECK_TYPES) {
    struct pike_string *sval_type = get_type_of_svalue(Pike_sp-1);
    if (!pike_types_le(sval_type, Pike_sp[-2].u.string)) {
      /* get_type_from_svalue() doesn't return a fully specified type
       * for array, mapping and multiset, so we perform a more lenient
       * check for them.
       */
      if (!pike_types_le(sval_type, weak_type_string) ||
	  !match_types(sval_type, Pike_sp[-2].u.string)) {
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

	t1 = describe_type(Pike_sp[-2].u.string);
	SET_ONERROR(tmp1, do_free_string, t1);
	  
	t2 = describe_type(sval_type);
	SET_ONERROR(tmp2, do_free_string, t2);
	  
	free_string(sval_type);

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
    free_string(sval_type);
#ifdef PIKE_DEBUG
    if (d_flag > 2) {
      struct pike_string *t = describe_type(Pike_sp[-2].u.string);
      fprintf(stderr, "Soft cast to %s\n", t->str);
      free_string(t);
    }
#endif /* PIKE_DEBUG */
  }
  stack_swap();
  pop_stack();
BREAK;

OPCODE0(F_RANGE, "range")
  o_range();
BREAK;

OPCODE0(F_COPY_VALUE, "copy_value")
{
  struct svalue tmp;
  copy_svalues_recursively_no_free(&tmp,Pike_sp-1,1,0);
  free_svalue(Pike_sp-1);
  Pike_sp[-1]=tmp;
}
BREAK;

OPCODE0(F_INDIRECT, "indirect")
{
  struct svalue s;
  lvalue_to_svalue_no_free(&s,Pike_sp-2);
  if(s.type != PIKE_T_STRING)
  {
    pop_n_elems(2);
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
}
print_return_value();
BREAK;
      
OPCODE0(F_SIZEOF, "sizeof")
  instr=pike_sizeof(Pike_sp-1);
  pop_stack();
  push_int(instr);
BREAK;

OPCODE1(F_SIZEOF_LOCAL, "sizeof local")
  push_int(pike_sizeof(Pike_fp->locals+arg1));
BREAK;

OPCODE1(F_SSCANF, "sscanf")
  o_sscanf(arg1);
BREAK;

      CASE(F_CALL_LFUN);
      apply_low(Pike_fp->current_object,
		GET_ARG()+Pike_fp->context.identifier_level,
		Pike_sp - *--Pike_mark_sp);
      break;

      CASE(F_CALL_LFUN_AND_POP);
      apply_low(Pike_fp->current_object,
		GET_ARG()+Pike_fp->context.identifier_level,
		Pike_sp - *--Pike_mark_sp);
      pop_stack();
      break;

    CASE(F_MARK_APPLY);
      strict_apply_svalue(Pike_fp->context.prog->constants + GET_ARG(), 0);
      break;

    CASE(F_MARK_APPLY_POP);
      strict_apply_svalue(Pike_fp->context.prog->constants + GET_ARG(), 0);
      pop_stack();
      break;

    CASE(F_APPLY);
      strict_apply_svalue(Pike_fp->context.prog->constants + GET_ARG(), Pike_sp - *--Pike_mark_sp );
      break;

    CASE(F_APPLY_AND_POP);
      strict_apply_svalue(Pike_fp->context.prog->constants + GET_ARG(), Pike_sp - *--Pike_mark_sp );
      pop_stack();
      break;

    CASE(F_CALL_FUNCTION);
    mega_apply(APPLY_STACK,Pike_sp - *--Pike_mark_sp,0,0);
    break;

    CASE(F_CALL_FUNCTION_AND_RETURN);
    {
      INT32 args=Pike_sp - *--Pike_mark_sp;
      if(!args)
	PIKE_ERROR("`()", "Too few arguments.\n", Pike_sp, 0);
      switch(Pike_sp[-args].type)
      {
	case PIKE_T_INT:
	  if (!Pike_sp[-args].u.integer) {
	    PIKE_ERROR("`()", "Attempt to call the NULL-value\n", Pike_sp, args);
	  }
	case PIKE_T_STRING:
	case PIKE_T_FLOAT:
	case PIKE_T_MAPPING:
	case PIKE_T_MULTISET:
	  PIKE_ERROR("`()", "Attempt to call a non-function value.\n", Pike_sp, args);
      }
      return args;
    }

