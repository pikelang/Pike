#undef GET_ARG
#undef GET_ARG2

#ifdef PIKE_DEBUG

#define GET_ARG() (backlog[backlogp].arg=(\
  instr=prefix,\
  prefix=0,\
  instr+=EXTRACT_UCHAR(pc++),\
  (t_flag>3 ? sprintf(trace_buffer,"-    Arg = %ld\n",(long)instr),write_to_stderr(trace_buffer,strlen(trace_buffer)) : 0),\
  instr))
#define GET_ARG2() (\
  instr=EXTRACT_UCHAR(pc++),\
  (t_flag>3 ? sprintf(trace_buffer,"-    Arg2= %ld\n",(long)instr),write_to_stderr(trace_buffer,strlen(trace_buffer)) : 0),\
  instr)

#else
#define GET_ARG() (instr=prefix,prefix=0,instr+EXTRACT_UCHAR(pc++))
#define GET_ARG2() EXTRACT_UCHAR(pc++)
#endif

static int eval_instruction(unsigned char *pc)
{
  unsigned INT32 accumulator=0,instr, prefix=0;
  debug_malloc_touch(fp);
  while(1)
  {
    fp->pc = pc;
    instr=EXTRACT_UCHAR(pc++);

#ifdef PIKE_DEBUG
    if(d_flag)
    {
#if defined(_REENTRANT) && !defined(__NT__)
      if(!mt_trylock(& interpreter_lock))
	fatal("Interpreter running unlocked!\n");
#endif
      sp[0].type=99; /* an invalid type */
      sp[1].type=99;
      sp[2].type=99;
      sp[3].type=99;
      
      if(sp<evaluator_stack || mark_sp < mark_stack || fp->locals>sp)
	fatal("Stack error (generic).\n");
      
      if(sp > evaluator_stack+stack_size)
	fatal("Stack error (overflow).\n");
      
      if(fp->fun>=0 && fp->current_object->prog &&
	 fp->locals+fp->num_locals > sp)
	fatal("Stack error (stupid!).\n");

      if(recoveries && sp-evaluator_stack < recoveries->sp)
	fatal("Stack error (underflow).\n");

      if(mark_sp > mark_stack && mark_sp[-1] > sp)
	fatal("Stack error (underflow?)\n");
      
      if(d_flag > 9) do_debug();

      backlogp++;
      if(backlogp >= BACKLOG) backlogp=0;

      if(backlog[backlogp].program)
	free_program(backlog[backlogp].program);

      backlog[backlogp].program=fp->context.prog;
      add_ref(fp->context.prog);
      backlog[backlogp].instruction=instr;
      backlog[backlogp].arg=0;
      backlog[backlogp].pc=pc;
    }

    if(t_flag > 2)
    {
      char *file, *f;
      INT32 linep;

      file=get_line(pc-1,fp->context.prog,&linep);
      while((f=STRCHR(file,'/'))) file=f+1;
      fprintf(stderr,"- %s:%4ld:(%lx): %-25s %4ld %4ld\n",
	      file,(long)linep,
	      (long)(pc-fp->context.prog->program-1),
	      get_f_name(instr + F_OFFSET),
	      (long)(sp-evaluator_stack),
	      (long)(mark_sp-mark_stack));
    }

    if(instr + F_OFFSET < F_MAX_OPCODE) 
      ADD_RUNNED(instr + F_OFFSET);
#endif

    switch(instr)
    {
      /* Support to allow large arguments */
      CASE(F_PREFIX_256); prefix+=256; break;
      CASE(F_PREFIX_512); prefix+=512; break;
      CASE(F_PREFIX_768); prefix+=768; break;
      CASE(F_PREFIX_1024); prefix+=1024; break;
      CASE(F_PREFIX_24BITX256);
      prefix+=EXTRACT_UCHAR(pc++)<<24;
      CASE(F_PREFIX_WORDX256);
      prefix+=EXTRACT_UCHAR(pc++)<<16;
      CASE(F_PREFIX_CHARX256);
      prefix+=EXTRACT_UCHAR(pc++)<<8;
      break;

      CASE(F_LDA); accumulator=GET_ARG(); break;

      /* Push number */
      CASE(F_CONST0); push_int(0); break;
      CASE(F_CONST1); push_int(1); break;
      CASE(F_CONST_1); push_int(-1); break;
      CASE(F_BIGNUM); push_int(0x7fffffff); break;
      CASE(F_NUMBER); push_int(GET_ARG()); break;
      CASE(F_NEG_NUMBER); push_int(-GET_ARG()); break;

      /* The rest of the basic 'push value' instructions */	

      CASE(F_MARK_AND_STRING); *(mark_sp++)=sp;
      CASE(F_STRING);
      copy_shared_string(sp->u.string,fp->context.prog->strings[GET_ARG()]);
      sp->type=T_STRING;
      sp->subtype=0;
      sp++;
      print_return_value();
      break;

      CASE(F_ARROW_STRING);
      copy_shared_string(sp->u.string,fp->context.prog->strings[GET_ARG()]);
      sp->type=T_STRING;
      sp->subtype=1; /* Magic */
      sp++;
      print_return_value();
      break;

      CASE(F_CONSTANT);
      assign_svalue_no_free(sp++,fp->context.prog->constants+GET_ARG());
      print_return_value();
      break;

      CASE(F_FLOAT);
      sp->type=T_FLOAT;
      MEMCPY((void *)&sp->u.float_number, pc, sizeof(FLOAT_TYPE));
      pc+=sizeof(FLOAT_TYPE);
      sp++;
      break;

      CASE(F_LFUN);
      sp->u.object=fp->current_object;
      add_ref(fp->current_object);
      sp->subtype=GET_ARG()+fp->context.identifier_level;
      sp->type=T_FUNCTION;
      sp++;
      print_return_value();
      break;

      CASE(F_TRAMPOLINE);
      {
	struct object *o=low_clone(pike_trampoline_program);
	add_ref( ((struct pike_trampoline *)(o->storage))->frame=fp );
	((struct pike_trampoline *)(o->storage))->func=GET_ARG()+fp->context.identifier_level;
	push_object(o);
	print_return_value();
	break;
      }


      /* The not so basic 'push value' instructions */
      CASE(F_GLOBAL);
      low_object_index_no_free(sp,
			       fp->current_object,
			       GET_ARG() + fp->context.identifier_level);
      sp++;
      print_return_value();
      break;

      CASE(F_EXTERNAL);
      {
	struct inherit *inherit;
	struct program *p;
	struct object *o;
	INT32 i,id=GET_ARG();

	inherit=&fp->context;
	
	o=fp->current_object;

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
	    accumulator+=inherit->parent_offset-1;
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
	  
	  inherit=INHERIT_FROM_INT(p, i);
	  
	  if(!accumulator) break;
	  --accumulator;
	}

	low_object_index_no_free(sp,
				 o,
				 id + inherit->identifier_level);
	sp++;
	print_return_value();
	break;
      }

      CASE(F_EXTERNAL_LVALUE);
      {
	struct inherit *inherit;
	struct program *p;
	struct object *o;
	INT32 i,id=GET_ARG();

	inherit=&fp->context;
	o=fp->current_object;
	
	if(!o)
	  error("Current object is destructed\n");

	while(1)
	{
	  if(inherit->parent_offset)
	  {
	    i=o->parent_identifier;
	    o=o->parent;
	    accumulator+=inherit->parent_offset-1;
	  }else{
	    i=inherit->parent_identifier;
	    o=inherit->parent;
	  }

	  if(!o)
	    error("Parent no longer exists\n");

	  if(!(p=o->prog))
	    error("Attempting to access variable in destructed object\n");

	  inherit=INHERIT_FROM_INT(p, i);

	  if(!accumulator) break;
	  accumulator--;
	}

	ref_push_object(o);
	sp->type=T_LVALUE;
	sp->u.integer=id + inherit->identifier_level;
	sp++;
	break;
      }


      CASE(F_MARK_AND_LOCAL); *(mark_sp++)=sp;
      CASE(F_LOCAL);
      assign_svalue_no_free(sp++,fp->locals+GET_ARG());
      print_return_value();
      break;

      CASE(F_2_LOCALS);
      assign_svalue_no_free(sp++,fp->locals+GET_ARG());
      print_return_value();
      assign_svalue_no_free(sp++,fp->locals+GET_ARG2());
      print_return_value();
      break;

      CASE(F_LOCAL_2_LOCAL);
      {
	int tmp=GET_ARG();
	assign_svalue(fp->locals+tmp, fp->locals+GET_ARG2());
	break;
      }

      CASE(F_LOCAL_2_GLOBAL);
      {
	INT32 tmp=GET_ARG() + fp->context.identifier_level;
	struct identifier *i;

	if(!fp->current_object->prog)
	  error("Cannot access global variables in destructed object.\n");

	i=ID_FROM_INT(fp->current_object->prog, tmp);
	if(!IDENTIFIER_IS_VARIABLE(i->identifier_flags))
	  error("Cannot assign functions or constants.\n");
	if(i->run_time_type == T_MIXED)
	{
	  assign_svalue((struct svalue *)GLOBAL_FROM_INT(tmp), fp->locals + GET_ARG2());
	}else{
	  assign_to_short_svalue((union anything *)GLOBAL_FROM_INT(tmp),
				 i->run_time_type,
				 fp->locals + GET_ARG2());
	}
	break;
      }

      CASE(F_GLOBAL_2_LOCAL);
      {
	INT32 tmp=GET_ARG() + fp->context.identifier_level;
	INT32 tmp2=GET_ARG2();
	free_svalue(fp->locals + tmp2);
	low_object_index_no_free(fp->locals + tmp2,
				 fp->current_object,
				 tmp);
	break;
      }

      CASE(F_LOCAL_LVALUE);
      sp[0].type=T_LVALUE;
      sp[0].u.lval=fp->locals+GET_ARG();
      sp[1].type=T_VOID;
      sp+=2;
      break;

      CASE(F_LEXICAL_LOCAL);
      {
	struct pike_frame *f=fp;
	while(accumulator--)
	{
	  f=f->scope;
	  if(!f) error("Lexical scope error.\n");
	}
	push_svalue(f->locals + GET_ARG());
	print_return_value();
	break;
      }

      CASE(F_LEXICAL_LOCAL_LVALUE);
      {
	struct pike_frame *f=fp;
	while(accumulator--)
	{
	  f=f->scope;
	  if(!f) error("Lexical scope error.\n");
	}
	sp[0].type=T_LVALUE;
	sp[0].u.lval=f->locals+GET_ARG();
	sp[1].type=T_VOID;
	sp+=2;
	break;
      }

      CASE(F_ARRAY_LVALUE);
      f_aggregate(GET_ARG()*2);
      sp[-1].u.array->flags |= ARRAY_LVALUE;
      sp[-1].u.array->type_field |= BIT_UNFINISHED | BIT_MIXED;
      sp[0]=sp[-1];
      sp[-1].type=T_ARRAY_LVALUE;
      sp++;
      break;

      CASE(F_CLEAR_2_LOCAL);
      instr=GET_ARG();
      free_svalues(fp->locals + instr, 2, -1);
      fp->locals[instr].type=T_INT;
      fp->locals[instr].subtype=0;
      fp->locals[instr].u.integer=0;
      fp->locals[instr+1].type=T_INT;
      fp->locals[instr+1].subtype=0;
      fp->locals[instr+1].u.integer=0;
      break;

      CASE(F_CLEAR_4_LOCAL);
      {
	int e;
	instr=GET_ARG();
	free_svalues(fp->locals + instr, 4, -1);
	for(e=0;e<4;e++)
	{
	  fp->locals[instr+e].type=T_INT;
	  fp->locals[instr+e].subtype=0;
	  fp->locals[instr+e].u.integer=0;
	}
	break;
      }

      CASE(F_CLEAR_LOCAL);
      instr=GET_ARG();
      free_svalue(fp->locals + instr);
      fp->locals[instr].type=T_INT;
      fp->locals[instr].subtype=0;
      fp->locals[instr].u.integer=0;
      break;

      CASE(F_INC_LOCAL);
      instr=GET_ARG();
      if(fp->locals[instr].type == T_INT)
      {
        fp->locals[instr].u.integer++;
	assign_svalue_no_free(sp++,fp->locals+instr);
      }else{
	assign_svalue_no_free(sp++,fp->locals+instr);
	push_int(1);
	f_add(2);
	assign_svalue(fp->locals+instr,sp-1);
      }
      break;

      CASE(F_POST_INC_LOCAL);
      instr=GET_ARG();
      assign_svalue_no_free(sp++,fp->locals+instr);
      goto inc_local_and_pop;

      CASE(F_INC_LOCAL_AND_POP);
      instr=GET_ARG();
    inc_local_and_pop:
      if(fp->locals[instr].type == T_INT)
      {
	fp->locals[instr].u.integer++;
      }else{
	assign_svalue_no_free(sp++,fp->locals+instr);
	push_int(1);
	f_add(2);
	assign_svalue(fp->locals+instr,sp-1);
	pop_stack();
      }
      break;

      CASE(F_DEC_LOCAL);
      instr=GET_ARG();
      if(fp->locals[instr].type == T_INT)
      {
	fp->locals[instr].u.integer--;
	assign_svalue_no_free(sp++,fp->locals+instr);
      }else{
	assign_svalue_no_free(sp++,fp->locals+instr);
	push_int(1);
	o_subtract();
	assign_svalue(fp->locals+instr,sp-1);
      }
      break;

      CASE(F_POST_DEC_LOCAL);
      instr=GET_ARG();
      assign_svalue_no_free(sp++,fp->locals+instr);
      goto dec_local_and_pop;
      /* fp->locals[instr].u.integer--; */
      break;

      CASE(F_DEC_LOCAL_AND_POP);
      instr=GET_ARG();
    dec_local_and_pop:
      if(fp->locals[instr].type == T_INT)
      {
	fp->locals[instr].u.integer--;
      }else{
	assign_svalue_no_free(sp++,fp->locals+instr);
	push_int(1);
	o_subtract();
	assign_svalue(fp->locals+instr,sp-1);
	pop_stack();
      }
      break;

      CASE(F_LTOSVAL);
      lvalue_to_svalue_no_free(sp,sp-2);
      sp++;
      break;

      CASE(F_LTOSVAL2);
      sp[0]=sp[-1];
      lvalue_to_svalue_no_free(sp-1,sp-3);

      /* this is so that foo+=bar (and similar things) will be faster, this
       * is done by freeing the old reference to foo after it has been pushed
       * on the stack. That way foo can have only 1 reference if we are lucky,
       * and then the low array/multiset/mapping manipulation routines can be
       * destructive if they like
       */
      if( (1 << sp[-1].type) & ( BIT_ARRAY | BIT_MULTISET | BIT_MAPPING | BIT_STRING ))
      {
	struct svalue s;
	s.type=T_INT;
	s.subtype=0;
	s.u.integer=0;
	assign_lvalue(sp-3,&s);
      }
      sp++;
      break;


      CASE(F_ADD_TO_AND_POP);
      sp[0]=sp[-1];
      lvalue_to_svalue_no_free(sp-1,sp-3);

      /* this is so that foo+=bar (and similar things) will be faster, this
       * is done by freeing the old reference to foo after it has been pushed
       * on the stack. That way foo can have only 1 reference if we are lucky,
       * and then the low array/multiset/mapping manipulation routines can be
       * destructive if they like
       */
      if( (1 << sp[-1].type) & ( BIT_ARRAY | BIT_MULTISET | BIT_MAPPING | BIT_STRING ))
      {
	struct svalue s;
	s.type=T_INT;
	s.subtype=0;
	s.u.integer=0;
	assign_lvalue(sp-3,&s);
      }
      sp++;
      f_add(2);
      assign_lvalue(sp-3,sp-1);
      pop_n_elems(3);
      break;

      CASE(F_GLOBAL_LVALUE);
      {
	struct identifier *i;
	INT32 tmp=GET_ARG() + fp->context.identifier_level;

	if(!fp->current_object->prog)
	  error("Cannot access global variables in destructed object.\n");

	i=ID_FROM_INT(fp->current_object->prog, tmp);

	if(!IDENTIFIER_IS_VARIABLE(i->identifier_flags))
	  error("Cannot re-assign functions or constants.\n");

	if(i->run_time_type == T_MIXED)
	{
	  sp[0].type=T_LVALUE;
	  sp[0].u.lval=(struct svalue *)GLOBAL_FROM_INT(tmp);
	}else{
	  sp[0].type=T_SHORT_LVALUE;
	  sp[0].u.short_lval= (union anything *)GLOBAL_FROM_INT(tmp);
	  sp[0].subtype=i->run_time_type;
	}
	sp[1].type=T_VOID;
	sp+=2;
	break;
      }
      
      CASE(F_INC);
      {
	union anything *u=get_pointer_if_this_type(sp-2, T_INT);
	if(u)
	{
	  instr=++ u->integer;
	  pop_n_elems(2);
	  push_int(u->integer);
	}else{
	  lvalue_to_svalue_no_free(sp, sp-2); sp++;
	  push_int(1);
	  f_add(2);
	  assign_lvalue(sp-3, sp-1);
	  assign_svalue(sp-3, sp-1);
	  pop_n_elems(2);
	}
	break;
      }

      CASE(F_DEC);
      {
	union anything *u=get_pointer_if_this_type(sp-2, T_INT);
	if(u)
	{
	  instr=-- u->integer;
	  pop_n_elems(2);
	  push_int(u->integer);
	}else{
	  lvalue_to_svalue_no_free(sp, sp-2); sp++;
	  push_int(1);
	  o_subtract();
	  assign_lvalue(sp-3, sp-1);
	  assign_svalue(sp-3, sp-1);
	  pop_n_elems(2);
	}
	break;
      }

      CASE(F_DEC_AND_POP);
      {
	union anything *u=get_pointer_if_this_type(sp-2, T_INT);
	if(u)
	{
	  instr=-- u->integer;
	  pop_n_elems(2);
	}else{
	  lvalue_to_svalue_no_free(sp, sp-2); sp++;
	  push_int(1);
	  o_subtract();
	  assign_lvalue(sp-3, sp-1);
	  pop_n_elems(3);
	}
	break;
      }

      CASE(F_INC_AND_POP);
      {
	union anything *u=get_pointer_if_this_type(sp-2, T_INT);
	if(u)
	{
	  instr=++ u->integer;
	  pop_n_elems(2);
	}else{
	  lvalue_to_svalue_no_free(sp, sp-2); sp++;
	  push_int(1);
	  f_add(2);
	  assign_lvalue(sp-3, sp-1);
	  pop_n_elems(3);
	}
	break;
      }

      CASE(F_POST_INC);
      {
	union anything *u=get_pointer_if_this_type(sp-2, T_INT);
	if(u)
	{
	  instr=u->integer ++;
	  pop_n_elems(2);
	  push_int(instr);
	}else{
	  lvalue_to_svalue_no_free(sp, sp-2); sp++;
	  assign_svalue_no_free(sp,sp-1); sp++;
	  push_int(1);
	  f_add(2);
	  assign_lvalue(sp-4, sp-1);
	  assign_svalue(sp-4, sp-2);
	  pop_n_elems(3);
	}
	break;
      }

      CASE(F_POST_DEC);
      {
	union anything *u=get_pointer_if_this_type(sp-2, T_INT);
	if(u)
	{
	  instr=u->integer --;
	  pop_n_elems(2);
	  push_int(instr);
	}else{
	  lvalue_to_svalue_no_free(sp, sp-2); sp++;
	  assign_svalue_no_free(sp,sp-1); sp++;
	  push_int(1);
	  o_subtract();
	  assign_lvalue(sp-4, sp-1);
	  assign_svalue(sp-4, sp-2);
	  pop_n_elems(3);
	}
	break;
      }

      CASE(F_ASSIGN);
      assign_lvalue(sp-3,sp-1);
      free_svalue(sp-3);
      free_svalue(sp-2);
      sp[-3]=sp[-1];
      sp-=2;
      break;

      CASE(F_ASSIGN_AND_POP);
      assign_lvalue(sp-3,sp-1);
      pop_n_elems(3);
      break;

    CASE(F_APPLY_ASSIGN_LOCAL);
      strict_apply_svalue(fp->context.prog->constants + GET_ARG(), sp - *--mark_sp );
      /* Fall through */

      CASE(F_ASSIGN_LOCAL);
      assign_svalue(fp->locals+GET_ARG(),sp-1);
      break;

    CASE(F_APPLY_ASSIGN_LOCAL_AND_POP);
      strict_apply_svalue(fp->context.prog->constants + GET_ARG(), sp - *--mark_sp );
      /* Fall through */

      CASE(F_ASSIGN_LOCAL_AND_POP);
      instr=GET_ARG();
      free_svalue(fp->locals+instr);
      fp->locals[instr]=sp[-1];
      sp--;
      break;

      CASE(F_ASSIGN_GLOBAL)
      {
	struct identifier *i;
	INT32 tmp=GET_ARG() + fp->context.identifier_level;
	if(!fp->current_object->prog)
	  error("Cannot access global variables in destructed object.\n");

	i=ID_FROM_INT(fp->current_object->prog, tmp);
	if(!IDENTIFIER_IS_VARIABLE(i->identifier_flags))
	  error("Cannot assign functions or constants.\n");
	if(i->run_time_type == T_MIXED)
	{
	  assign_svalue((struct svalue *)GLOBAL_FROM_INT(tmp), sp-1);
	}else{
	  assign_to_short_svalue((union anything *)GLOBAL_FROM_INT(tmp),
				 i->run_time_type,
				 sp-1);
	}
      }
      break;

      CASE(F_ASSIGN_GLOBAL_AND_POP)
      {
	struct identifier *i;
	INT32 tmp=GET_ARG() + fp->context.identifier_level;
	if(!fp->current_object->prog)
	  error("Cannot access global variables in destructed object.\n");

	i=ID_FROM_INT(fp->current_object->prog, tmp);
	if(!IDENTIFIER_IS_VARIABLE(i->identifier_flags))
	  error("Cannot assign functions or constants.\n");

	if(i->run_time_type == T_MIXED)
	{
	  struct svalue *s=(struct svalue *)GLOBAL_FROM_INT(tmp);
	  free_svalue(s);
	  sp--;
	  *s=*sp;
	}else{
	  assign_to_short_svalue((union anything *)GLOBAL_FROM_INT(tmp),
				 i->run_time_type,
				 sp-1);
	  pop_stack();
	}
      }
      break;

      /* Stack machine stuff */
      CASE(F_POP_VALUE); pop_stack(); break;
      CASE(F_POP_N_ELEMS); pop_n_elems(GET_ARG()); break;
      CASE(F_MARK2); *(mark_sp++)=sp;
      CASE(F_MARK); *(mark_sp++)=sp; break;
      CASE(F_MARK_X); *(mark_sp++)=sp-GET_ARG(); break;
      CASE(F_POP_MARK); --mark_sp; break;

      CASE(F_CLEAR_STRING_SUBTYPE);
      if(sp[-1].type==T_STRING) sp[-1].subtype=0;
      break;

      /* Jumps */
      CASE(F_BRANCH);
      DOJUMP();
      break;

      CASE(F_BRANCH_IF_NOT_LOCAL_ARROW);
      {
	struct svalue tmp;
	tmp.type=T_STRING;
	tmp.u.string=fp->context.prog->strings[GET_ARG()];
	tmp.subtype=1;
	sp->type=T_INT;	
	sp++;
	index_no_free(sp-1,fp->locals+GET_ARG2() , &tmp);
	print_return_value();
      }

      /* Fall through */

      CASE(F_BRANCH_WHEN_ZERO);
      if(!IS_ZERO(sp-1))
      {
	pc+=sizeof(INT32);
      }else{
	DOJUMP();
      }
      pop_stack();
      break;
      
      CASE(F_BRANCH_WHEN_NON_ZERO);
      if(IS_ZERO(sp-1))
      {
	pc+=sizeof(INT32);
      }else{
	DOJUMP();
      }
      pop_stack();
      break;

      CASE(F_BRANCH_IF_LOCAL);
      instr=GET_ARG();
      if(IS_ZERO(fp->locals + instr))
      {
	pc+=sizeof(INT32);
      }else{
	DOJUMP();
      }
      break;

      CASE(F_BRANCH_IF_NOT_LOCAL);
      instr=GET_ARG();
      if(!IS_ZERO(fp->locals + instr))
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
      if(!IS_ZERO(sp-1))
      {
	pc+=sizeof(INT32);
      }else{
	DOJUMP();
	pop_stack();
      }
      break;

      CASE(F_BRANCH_AND_POP_WHEN_NON_ZERO);
      if(IS_ZERO(sp-1))
      {
	pc+=sizeof(INT32);
      }else{
	DOJUMP();
	pop_stack();
      }
      break;

      CASE(F_LAND);
      if(!IS_ZERO(sp-1))
      {
	pc+=sizeof(INT32);
	pop_stack();
      }else{
	DOJUMP();
      }
      break;

      CASE(F_LOR);
      if(IS_ZERO(sp-1))
      {
	pc+=sizeof(INT32);
	pop_stack();
      }else{
	DOJUMP();
      }
      break;

      CASE(F_EQ_OR);
      if(!is_eq(sp-2,sp-1))
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
      if(is_eq(sp-2,sp-1))
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

      CASE(F_THROW_ZERO);
      push_int(0);
      f_throw(1);
      break;

      CASE(F_SWITCH)
      {
	INT32 tmp;
	tmp=switch_lookup(fp->context.prog->
			  constants[GET_ARG()].u.array,sp-1);
	pc=(unsigned char *)DO_ALIGN(pc,sizeof(INT32));
	pc+=(tmp>=0 ? 1+tmp*2 : 2*~tmp) * sizeof(INT32);
	if(*(INT32*)pc < 0) fast_check_threads_etc(7);
	pc+=*(INT32*)pc;
	pop_stack();
	break;
      }
      
      LOOP(F_INC_LOOP, ++, <, f_add(2), is_lt);
      LOOP(F_DEC_LOOP, --, >, o_subtract(), is_gt);
      LOOP(F_INC_NEQ_LOOP, ++, !=, f_add(2), !is_eq);
      LOOP(F_DEC_NEQ_LOOP, --, !=, o_subtract(), !is_eq);

      CASE(F_FOREACH) /* array, lvalue, X, i */
      {
	if(sp[-4].type != T_ARRAY)
	  PIKE_ERROR("foreach", "Bad argument 1.\n", sp-3, 1);
	if(sp[-1].u.integer < sp[-4].u.array->size)
	{
	  fast_check_threads_etc(10);
	  index_no_free(sp,sp-4,sp-1);
	  sp++;
	  assign_lvalue(sp-4, sp-1);
	  free_svalue(sp-1);
	  sp--;
	  pc+=EXTRACT_INT(pc);
	  sp[-1].u.integer++;
	}else{
	  pc+=sizeof(INT32);
	}
	break;
      }

      CASE(F_APPLY_AND_RETURN);
      {
	INT32 args=sp - *--mark_sp;
/*	fprintf(stderr,"%p >= %p\n",fp->expendible,sp-args); */
	if(fp->expendible >= sp-args)
	{
/*	  fprintf(stderr,"NOT EXPENDIBLE!\n"); */
	  MEMMOVE(sp-args+1,sp-args,args*sizeof(struct svalue));
	  sp++;
	  sp[-args-1].type=T_INT;
	}
	/* We sabotage the stack here */
	assign_svalue(sp-args-1,fp->context.prog->constants+GET_ARG());
	return args+1;
      }

      CASE(F_CALL_LFUN_AND_RETURN);
      {
	INT32 args=sp - *--mark_sp;
	if(fp->expendible >= sp-args)
	{
	  MEMMOVE(sp-args+1,sp-args,args*sizeof(struct svalue));
	  sp++;
	  sp[-args-1].type=T_INT;
	}else{
	  free_svalue(sp-args-1);
	}
	/* More stack sabotage */
	sp[-args-1].u.object=fp->current_object;
	sp[-args-1].subtype=GET_ARG()+fp->context.identifier_level;
#ifdef PIKE_DEBUG
	if(t_flag > 9)
	  fprintf(stderr,"-    IDENTIFIER_LEVEL: %d\n",fp->context.identifier_level);
#endif
	sp[-args-1].type=T_FUNCTION;
	add_ref(fp->current_object);

	return args+1;
      }

      CASE(F_RETURN_LOCAL);
      instr=GET_ARG();
      if(fp->expendible <= fp->locals+instr)
      {
	pop_n_elems(sp-1 - (fp->locals+instr));
      }else{
	push_svalue(fp->locals+instr);
      }
      print_return_value();
      goto do_return;

      CASE(F_RETURN_1);
      push_int(1);
      goto do_return;

      CASE(F_RETURN_0);
      push_int(0);
      goto do_return;

      CASE(F_RETURN);
    do_return:
#if defined(PIKE_DEBUG) && defined(GC2)
      if(d_flag>2) do_gc();
      if(d_flag>3) do_debug();
      check_threads_etc();
#endif

      /* fall through */

      CASE(F_DUMB_RETURN);
      return -1;

      CASE(F_NEGATE); 
      if(sp[-1].type == T_INT)
      {
	sp[-1].u.integer =- sp[-1].u.integer;
      }else if(sp[-1].type == T_FLOAT)
      {
	sp[-1].u.float_number =- sp[-1].u.float_number;
      }else{
	o_negate();
      }
      break;

      CASE(F_COMPL); o_compl(); break;

      CASE(F_NOT);
      switch(sp[-1].type)
      {
      case T_INT:
	sp[-1].u.integer =! sp[-1].u.integer;
	break;

      case T_FUNCTION:
      case T_OBJECT:
	if(IS_ZERO(sp-1))
	{
	  pop_stack();
	  push_int(1);
	}else{
	  pop_stack();
	  push_int(0);
	}
	break;

      default:
	free_svalue(sp-1);
	sp[-1].type=T_INT;
	sp[-1].u.integer=0;
      }
      break;

      CASE(F_LSH); o_lsh(); break;
      CASE(F_RSH); o_rsh(); break;

      COMPARISMENT(F_EQ, is_eq(sp-2,sp-1));
      COMPARISMENT(F_NE,!is_eq(sp-2,sp-1));
      COMPARISMENT(F_GT, is_gt(sp-2,sp-1));
      COMPARISMENT(F_GE,!is_lt(sp-2,sp-1));
      COMPARISMENT(F_LT, is_lt(sp-2,sp-1));
      COMPARISMENT(F_LE,!is_gt(sp-2,sp-1));

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

      CASE(F_PUSH_ARRAY);
      switch(sp[-1].type)
      {
	default:
	  PIKE_ERROR("@", "Bad argument.\n", sp, 1);

	case T_OBJECT:
	  if(!sp[-1].u.object->prog || FIND_LFUN(sp[-1].u.object->prog,LFUN__VALUES) == -1)
	    PIKE_ERROR("@", "Bad argument.\n", sp, 1);

	  apply_lfun(sp[-1].u.object, LFUN__VALUES, 0);
	  if(sp[-1].type != T_ARRAY)
	    error("Bad return type from o->_values() in @\n");
	  free_svalue(sp-2);
	  sp[-2]=sp[-1];
	  sp--;
	  break;

	case T_ARRAY: break;
      }
      sp--;
      push_array_items(sp->u.array);
      break;

      CASE(F_LOCAL_LOCAL_INDEX);
      {
	struct svalue *s=fp->locals+GET_ARG();
	if(s->type == T_STRING) s->subtype=0;
	sp++->type=T_INT;
	index_no_free(sp-1,fp->locals+GET_ARG2(),s);
	break;
      }

      CASE(F_LOCAL_INDEX);
      {
	struct svalue tmp,*s=fp->locals+GET_ARG();
	if(s->type == T_STRING) s->subtype=0;
	index_no_free(&tmp,sp-1,s);
	free_svalue(sp-1);
	sp[-1]=tmp;
	break;
      }

      CASE(F_GLOBAL_LOCAL_INDEX);
      {
	struct svalue tmp,*s;
	low_object_index_no_free(sp,
				 fp->current_object,
				 GET_ARG() + fp->context.identifier_level);
	sp++;
	s=fp->locals+GET_ARG2();
	if(s->type == T_STRING) s->subtype=0;
	index_no_free(&tmp,sp-1,s);
	free_svalue(sp-1);
	sp[-1]=tmp;
	break;
      }

      CASE(F_LOCAL_ARROW);
      {
	struct svalue tmp;
	tmp.type=T_STRING;
	tmp.u.string=fp->context.prog->strings[GET_ARG()];
	tmp.subtype=1;
	sp->type=T_INT;	
	sp++;
	index_no_free(sp-1,fp->locals+GET_ARG2() , &tmp);
	print_return_value();
	break;
      }

      CASE(F_ARROW);
      {
	struct svalue tmp,tmp2;
	tmp.type=T_STRING;
	tmp.u.string=fp->context.prog->strings[GET_ARG()];
	tmp.subtype=1;
	index_no_free(&tmp2, sp-1, &tmp);
	free_svalue(sp-1);
	sp[-1]=tmp2;
	print_return_value();
	break;
      }

      CASE(F_STRING_INDEX);
      {
	struct svalue tmp,tmp2;
	tmp.type=T_STRING;
	tmp.u.string=fp->context.prog->strings[GET_ARG()];
	tmp.subtype=0;
	index_no_free(&tmp2, sp-1, &tmp);
	free_svalue(sp-1);
	sp[-1]=tmp2;
	print_return_value();
	break;
      }

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
	index_no_free(&s,sp-2,sp-1);
	pop_n_elems(2);
	*sp=s;
	sp++;
      }
      print_return_value();
      break;

      CASE(F_MAGIC_INDEX);
      push_magic_index(magic_index_program, accumulator, GET_ARG());
      break;

      CASE(F_MAGIC_SET_INDEX);
      push_magic_index(magic_set_index_program, accumulator, GET_ARG());
      break;

      CASE(F_CAST); f_cast(); break;

      CASE(F_RANGE); o_range(); break;
      CASE(F_COPY_VALUE);
      {
	struct svalue tmp;
	copy_svalues_recursively_no_free(&tmp,sp-1,1,0);
	free_svalue(sp-1);
	sp[-1]=tmp;
      }
      break;

      CASE(F_INDIRECT);
      {
	struct svalue s;
	lvalue_to_svalue_no_free(&s,sp-2);
	if(s.type != T_STRING)
	{
	  pop_n_elems(2);
	  *sp=s;
	  sp++;
	}else{
	  struct object *o;
	  o=low_clone(string_assignment_program);
	  ((struct string_assignment_storage *)o->storage)->lval[0]=sp[-2];
	  ((struct string_assignment_storage *)o->storage)->lval[1]=sp[-1];
	  ((struct string_assignment_storage *)o->storage)->s=s.u.string;
	  sp-=2;
	  push_object(o);
	}
      }
      print_return_value();
      break;
      

      CASE(F_SIZEOF);
      instr=pike_sizeof(sp-1);
      pop_stack();
      push_int(instr);
      break;

      CASE(F_SIZEOF_LOCAL);
      push_int(pike_sizeof(fp->locals+GET_ARG()));
      break;

      CASE(F_SSCANF); o_sscanf(GET_ARG()); break;

      CASE(F_CALL_LFUN);
      apply_low(fp->current_object,
		GET_ARG()+fp->context.identifier_level,
		sp - *--mark_sp);
      break;

      CASE(F_CALL_LFUN_AND_POP);
      apply_low(fp->current_object,
		GET_ARG()+fp->context.identifier_level,
		sp - *--mark_sp);
      pop_stack();
      break;

    CASE(F_MARK_APPLY);
      strict_apply_svalue(fp->context.prog->constants + GET_ARG(), 0);
      break;

    CASE(F_MARK_APPLY_POP);
      strict_apply_svalue(fp->context.prog->constants + GET_ARG(), 0);
      pop_stack();
      break;

    CASE(F_APPLY);
      strict_apply_svalue(fp->context.prog->constants + GET_ARG(), sp - *--mark_sp );
      break;

    CASE(F_APPLY_AND_POP);
      strict_apply_svalue(fp->context.prog->constants + GET_ARG(), sp - *--mark_sp );
      pop_stack();
      break;

    CASE(F_CALL_FUNCTION);
    mega_apply(APPLY_STACK,sp - *--mark_sp,0,0);
    break;

    CASE(F_CALL_FUNCTION_AND_RETURN);
    {
      INT32 args=sp - *--mark_sp;
      if(!args)
	PIKE_ERROR("`()", "Too few arguments.\n", sp, 0);
      switch(sp[-args].type)
      {
	case T_INT:
	  if (!sp[-args].u.integer) {
	    PIKE_ERROR("`()", "Attempt to call the NULL-value\n", sp, args);
	  }
	case T_STRING:
	case T_FLOAT:
	case T_MAPPING:
	case T_MULTISET:
	  PIKE_ERROR("`()", "Attempt to call a non-function value.\n", sp, args);
      }
      return args;
    }

    default:
      fatal("Strange instruction %ld\n",(long)instr);
    }

  }
}
