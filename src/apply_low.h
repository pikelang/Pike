/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: apply_low.h,v 1.27 2004/04/03 21:53:48 mast Exp $
*/

    {
      struct program *p;
      struct reference *ref;
      struct pike_frame *new_frame;
      struct identifier *function;

#if 0
      /* This kind of fault tolerance is braindamaged. /mast */
      if(fun<0)
      {
	pop_n_elems(Pike_sp-save_sp);
	push_undefined();
	return 0;
      }
#else
#ifdef PIKE_DEBUG
      if (fun < 0)
	Pike_fatal ("Invalid function offset.\n");
#endif
#endif

      check_stack(256);
      check_mark_stack(256);

#ifdef PIKE_DEBUG
      {
	static int counter;
	switch(d_flag)
	{
	  case 0:
	  case 1:
	  case 2:
	    break;
	  case 3:
	    if(!((counter++ & 7)))
	      do_debug();
	    break;
	  case 4:
	  default:
	    do_debug();
	}
      }
#endif

      p=o->prog;
      if(!p)
	PIKE_ERROR("destructed object->function",
	      "Cannot call functions in destructed objects.\n", Pike_sp, args);

      if(!(p->flags & PROGRAM_PASS_1_DONE) || (p->flags & PROGRAM_AVOID_CHECK))
	PIKE_ERROR("__empty_program() -> function",
	      "Cannot call functions in unfinished objects.\n", Pike_sp, args);
	

#ifdef PIKE_SECURITY
      CHECK_DATA_SECURITY_OR_ERROR(o, SECURITY_BIT_CALL,
				   ("Function call permission denied.\n"));

      if(!CHECK_DATA_SECURITY(o, SECURITY_BIT_NOT_SETUID))
	SET_CURRENT_CREDS(o->prot);
#endif


#ifdef PIKE_DEBUG
      if(fun>=(int)p->num_identifier_references)
      {
	fprintf(stderr, "Function index out of range. %ld >= %d\n",
		DO_NOT_WARN((long)fun),
		(int)p->num_identifier_references);
	fprintf(stderr,"########Program is:\n");
	describe(p);
	fprintf(stderr,"########Object is:\n");
	describe(o);
	Pike_fatal("Function index out of range.\n");
      }
#endif

      ref = p->identifier_references + fun;
#ifdef PIKE_DEBUG
      if(ref->inherit_offset>=p->num_inherits)
	Pike_fatal("Inherit offset out of range in program.\n");
#endif

      /* init a new evaluation pike_frame */
      new_frame=alloc_pike_frame();
#ifdef PROFILING
#ifdef HAVE_GETHRTIME
      new_frame->children_base = Pike_interpreter.accounted_time;
      new_frame->start_time = gethrtime() - Pike_interpreter.time_base;

#endif

      /* This is mostly for profiling, but
       * could also be used to find out the name of a function
       * in a destructed object. -hubbe
       *
       * Since it not used for anything but profiling yet, I will
       * put it here until someone needs it. -Hubbe
       */
      new_frame->ident = ref->identifier_offset;
#endif
      debug_malloc_touch(new_frame);

      new_frame->next = Pike_fp;
      new_frame->current_object = o;
      new_frame->context = p->inherits[ ref->inherit_offset ];

      function = new_frame->context.prog->identifiers + ref->identifier_offset;
      new_frame->fun = DO_NOT_WARN((unsigned INT16)fun);

      
#ifdef PIKE_DEBUG
	if(Pike_interpreter.trace_level > 9)
	{
	  fprintf(stderr,"-- ref: inoff=%d idoff=%d flags=%d\n",
		  ref->inherit_offset,
		  ref->identifier_offset,
		  ref->id_flags);

	  fprintf(stderr,"-- context: prog->id=%d inlev=%d idlev=%d pi=%d po=%d so=%ld name=%s\n",
		  new_frame->context.prog->id,
		  new_frame->context.inherit_level,
		  new_frame->context.identifier_level,
		  new_frame->context.parent_identifier,
		  new_frame->context.parent_offset,
		  DO_NOT_WARN((long)new_frame->context.storage_offset),
		  new_frame->context.name ? new_frame->context.name->str  : "NULL");
	  if(Pike_interpreter.trace_level>19)
	  {
	    describe(new_frame->context.prog);
	  }
	}
#endif


      new_frame->expendible =new_frame->locals = Pike_sp - args;
      new_frame->args = args;
      new_frame->pc = 0;
#ifdef SCOPE
      new_frame->scope=scope;
#ifdef PIKE_DEBUG
      if(new_frame->fun == scope->fun)
      {
	Pike_fatal("Que? A function cannot be parented by itself!\n");
      }
#endif
#else
      new_frame->scope=0;
#endif
      new_frame->save_sp=save_sp;
      
      add_ref(new_frame->current_object);
      add_ref(new_frame->context.prog);
      if(new_frame->context.parent) add_ref(new_frame->context.parent);
#ifdef SCOPE
      if(new_frame->scope) add_ref(new_frame->scope);
#endif

      if(Pike_interpreter.trace_level)
      {
	dynamic_buffer save_buf;
	char buf[50];

	init_buf(&save_buf);
	sprintf(buf, "%lx->", DO_NOT_WARN((long) PTR_TO_INT (o)));
	my_strcat(buf);
	if (function->name->size_shift)
	  my_strcat ("[widestring function name]");
	else
	  my_strcat(function->name->str);
	do_trace_call(args, &save_buf);
      }

#ifdef PIKE_DEBUG      
      if (Pike_fp && (new_frame->locals < Pike_fp->locals)) {
	fatal("New locals below old locals: %p < %p\n",
	      new_frame->locals, Pike_fp->locals);
      }
#endif /* PIKE_DEBUG */

      Pike_fp = new_frame;
      
#ifdef PROFILING
      function->num_calls++;
#endif
  
      if(function->func.offset == -1) {
	new_frame->num_args = args;
	generic_error(NULL, Pike_sp, args,
		      "Calling undefined function.\n");
      }
      
#ifdef PROFILING
#ifdef HAVE_GETHRTIME
      new_frame->self_time_base=function->total_time;
#endif
#endif

      switch(function->identifier_flags & IDENTIFIER_TYPE_MASK)
      {
      case IDENTIFIER_C_FUNCTION:
	debug_malloc_touch(Pike_fp);
	Pike_fp->num_args=args;
	new_frame->current_storage = o->storage+new_frame->context.storage_offset;
	new_frame->num_locals=args;
	check_threads_etc();
	(*function->func.c_fun)(args);
	break;
	
      case IDENTIFIER_CONSTANT:
      {
	struct svalue *s=&(Pike_fp->context.prog->
			   constants[function->func.offset].sval);
	debug_malloc_touch(Pike_fp);
	if(s->type == T_PROGRAM)
	{
	  struct object *tmp;
	  check_threads_etc();
	  Pike_fp->num_args=args;
	  tmp=parent_clone_object(s->u.program,
				  o,
				  fun,
				  args);
	  push_object(tmp);
	  break;
	}
	/* Fall through */
      }

      case IDENTIFIER_VARIABLE:
      {
	/* FIXME:
	 * Use new-style tail-recursion instead
	 */
	debug_malloc_touch(Pike_fp);
	debug_malloc_touch(o);
	if(Pike_sp-save_sp-args<=0)
	{
	  /* Create an extra svalue for tail recursion style call */
	  Pike_sp++;
	  MEMMOVE(Pike_sp-args,Pike_sp-args-1,sizeof(struct svalue)*args);
#ifdef DEBUG_MALLOC
	  if (args) {
	    int i;
	    /* Note: touch the dead svalue too. */
	    for (i=args+2; i > 0; i--) {
	      dmalloc_touch_svalue(Pike_sp-i);
	    }
	  }
#endif /* DEBUG_MALLOC */	      
	  Pike_sp[-args-1].type=T_INT;
	}else{
	  free_svalue(Pike_sp-args-1);
	  Pike_sp[-args-1].type=T_INT;
	}
	low_object_index_no_free(Pike_sp-args-1,o,fun);

	/* No profiling code for calling variables - Hubbe */
	POP_PIKE_FRAME();

	if(Pike_sp-save_sp-args > (args<<2) + 32)
	{
	  /* The test above assures these two areas
	   * are not overlapping
	   */
	  assign_svalues(save_sp, Pike_sp-args, args, BIT_MIXED);
	  pop_n_elems(Pike_sp-save_sp-args);
	}
	arg1=(void *)(Pike_sp-args-1);
	goto apply_svalue;
      }

      case IDENTIFIER_PIKE_FUNCTION:
      {
	int num_args;
	int num_locals;
	PIKE_OPCODE_T *pc;
	fast_check_threads_etc(6);

#ifdef PIKE_DEBUG
	if (Pike_in_gc > GC_PASS_PREPARE && Pike_in_gc < GC_PASS_FREE)
	  Pike_fatal("Pike code called within gc.\n");
#endif

	debug_malloc_touch(Pike_fp);
	pc=new_frame->context.prog->program + function->func.offset;

	/*
	 * FIXME: The following stack stuff could probably
	 *        be moved to an opcode.
	 */

	num_locals = READ_INCR_BYTE(pc);

	if(function->identifier_flags & IDENTIFIER_SCOPE_USED)
	  new_frame->expendible+=num_locals;
	
	num_args = READ_INCR_BYTE(pc);

#ifdef PIKE_DEBUG
	if(num_locals < num_args)
	  Pike_fatal("Wrong number of arguments or locals in function def.\n"
		"num_locals: %d < num_args: %d\n",
		num_locals, num_args);
#endif

	
	if(function->identifier_flags & IDENTIFIER_VARARGS)
	{
	  /* adjust arguments on stack */
	  if(args < num_args) /* push zeros */
	  {
	    push_undefines(num_args-args);
	    f_aggregate(0);
	  }else{
	    f_aggregate(args - num_args); /* make array */
	  }
	  args = num_args+1;
	}else{
	  /* adjust arguments on stack */
	  if(args < num_args) /* push zeros */
	    push_undefines(num_args-args);
	  else
	    pop_n_elems(args - num_args);
	  args=num_args;
	}

	if(num_locals > args)
	  push_zeroes(num_locals-args);

	new_frame->num_locals=num_locals;
	new_frame->num_args=num_args;
	new_frame->save_mark_sp=new_frame->mark_sp_base=Pike_mark_sp;
	new_frame->pc = pc
#ifdef ENTRY_PROLOGUE_SIZE
	  + ENTRY_PROLOGUE_SIZE
#endif /* ENTRY_PROLOGUE_SIZE */
	  ;
	return 1;
      }

      default:;
#ifdef PIKE_DEBUG
	Pike_fatal("Unknown identifier type.\n");
#endif
      }
#ifdef PROFILING
#ifdef HAVE_GETHRTIME
  {
    long long time_passed, time_in_children, self_time;
    time_in_children=  Pike_interpreter.accounted_time - Pike_fp->children_base;
    time_passed = gethrtime() - Pike_interpreter.time_base - Pike_fp->start_time;
    self_time=time_passed - time_in_children;
    Pike_interpreter.accounted_time+=self_time;
    function->total_time=Pike_fp->self_time_base + (INT32)(time_passed /1000);
    function->self_time+=(INT32)( self_time /1000);
  }
#endif
#endif

#if 0
#ifdef PIKE_DEBUG
      if(Pike_fp!=new_frame)
	Pike_fatal("Frame stack out of whack!\n");
#endif
#endif
      
      POP_PIKE_FRAME();
    }
