/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

    {
      struct program *p;
      struct reference *ref;
      struct pike_frame *new_frame;
      struct identifier *function;

#ifdef PIKE_DEBUG
      if (fun < 0)
        Pike_fatal ("Invalid function offset: %td.\n", fun);
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

#ifdef PIKE_DEBUG
      if(fun>=(int)p->num_identifier_references)
      {
	fprintf(stderr, "Function index out of range. %ld >= %d\n",
                (long)fun, (int)p->num_identifier_references);
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
      new_frame->children_base = Pike_interpreter.accounted_time;
      new_frame->start_time = get_cpu_time() - Pike_interpreter.unlocked_time;

      /* This is mostly for profiling, but
       * could also be used to find out the name of a function
       * in a destructed object. -hubbe
       *
       * Since it not used for anything but profiling yet, I will
       * put it here until someone needs it. -Hubbe
       */
      new_frame->ident = ref->identifier_offset;
      W_PROFILING_DEBUG("%p{: Push at %" PRINT_CPU_TIME
                        " %" PRINT_CPU_TIME "\n",
                        Pike_interpreter.thread_state, new_frame->start_time,
                        new_frame->children_base);
#endif
      debug_malloc_touch(new_frame);

      new_frame->next = Pike_fp;
      new_frame->current_object = o;
      new_frame->current_program = p;
      new_frame->context = INHERIT_FROM_PTR(p, ref);

      function = ID_FROM_PTR(p, ref);
      new_frame->fun = (unsigned INT16)fun;

      if(!(p->flags & (PROGRAM_PASS_1_DONE|PROGRAM_AVOID_CHECK)) &&
         ((function->identifier_flags &
           (IDENTIFIER_TYPE_MASK|IDENTIFIER_ALIAS)) != IDENTIFIER_CONSTANT)) {
        free_pike_frame(new_frame);

        PIKE_ERROR("__empty_program() -> function",
                   "Cannot call functions in unfinished objects.\n", Pike_sp, args);
      }


#ifdef PIKE_DEBUG
	if(Pike_interpreter.trace_level > 9)
	{
	  fprintf(stderr,"-- ref: inoff=%d idoff=%d flags=%d\n",
		  ref->inherit_offset,
		  ref->identifier_offset,
		  ref->id_flags);

	  fprintf(stderr,"-- context: prog->id=%d inlev=%d idlev=%d pi=%d po=%d so=%ld name=%s\n",
		  new_frame->context->prog->id,
		  new_frame->context->inherit_level,
		  new_frame->context->identifier_level,
		  new_frame->context->parent_identifier,
		  new_frame->context->parent_offset,
                  (long)new_frame->context->storage_offset,
		  new_frame->context->name ? new_frame->context->name->str  : "NULL");
	  if(Pike_interpreter.trace_level>19)
	  {
	    describe(new_frame->context->prog);
	  }
	}
#endif


      new_frame->locals = Pike_sp - args;
      new_frame->args = args;
      new_frame->pc = 0;
      new_frame->scope=scope;

      if(scope && (scope->current_object == o) &&
         (new_frame->fun == scope->fun)) {
	/* Continuing a previous function call. */

	/* Generators have two (optional) arguments:
	 *
	 *   * A resumption value.
	 *
	 *   * A resumption callback function.
	 */
	if (args < 2) {
	  if (!args) push_undefined();
	  push_undefined();
        } else if (UNLIKELY(args > 2)) {
	  pop_n_elems(args-2);
	}

	/* Switch to the shared locals. */
	new_frame->locals = scope->locals;
        if (!(scope->flags & PIKE_FRAME_MALLOCED_LOCALS)) {
          Pike_fatal("Generator scope not initialized yet.\n");
        }
	add_ref(scope->locals[-1].u.array);
	new_frame->flags |= PIKE_FRAME_MALLOCED_LOCALS;
	new_frame->args = scope->args;
        new_frame->num_locals = scope->locals[-1].u.array->size;
	args = scope->num_args;

	/* Switch to the parent scope. */
	new_frame->scope = scope->scope;
      }

      frame_set_save_sp(new_frame, save_sp);

      add_ref(new_frame->current_object);
      add_ref(new_frame->current_program);
      if(new_frame->scope) add_ref(new_frame->scope);
#ifdef PIKE_DEBUG
      if (Pike_fp) {

        if (!((new_frame->flags|Pike_fp->flags) &
              PIKE_FRAME_MALLOCED_LOCALS) &&
	    new_frame->locals < Pike_fp->locals) {
          Pike_fatal("New locals below old locals: %p < %p\n",
                     new_frame->locals, Pike_fp->locals);
	}

	if (d_flag > 1) {
	  /* Liberal use of variables for debugger convenience. */
	  size_t i;
	  struct svalue *l = frame_get_save_sp(Pike_fp);
	  for (i = 0; l + i < Pike_sp; i++)
	    if (TYPEOF(l[i]) != PIKE_T_FREE)
	      debug_check_svalue (l + i);
	}
      }
#endif /* PIKE_DEBUG */

      Pike_fp = new_frame;

      if(Pike_interpreter.trace_level)
      {
        do_trace_function_call(o, function, args);
      }
      if (PIKE_FN_START_ENABLED()) {
	/* DTrace enter probe
	   arg0: function name
	   arg1: object
	*/
	struct byte_buffer obj_name = BUFFER_INIT();
	struct svalue obj_sval;
	SET_SVAL(obj_sval, T_OBJECT, 0, object, o);
	safe_describe_svalue(&obj_name, &obj_sval, 0, NULL);
	PIKE_FN_START(function->name->size_shift == 0 ?
		      function->name->str : "[widestring fn name]",
		      buffer_get_string(&obj_name));
        buffer_free(&obj_name);
      }

#ifdef PROFILING
      function->num_calls++;
      function->recur_depth++;
#endif

      if(
#ifdef USE_VALGRIND
         function->identifier_flags & IDENTIFIER_PIKE_FUNCTION &&
#endif
         function->func.offset == -1) {
	new_frame->num_args = args;
	generic_error(NULL, Pike_sp, args,
                      "Calling undefined function %pS%s%pS().\n",
		      (ref->inherit_offset && new_frame->context->name)?
		      new_frame->context->name : empty_pike_string,
		      ref->inherit_offset? "::" : "",
		      function->name);
      }

      switch(function->identifier_flags & (IDENTIFIER_TYPE_MASK|IDENTIFIER_ALIAS))
      {
      case IDENTIFIER_C_FUNCTION:
	debug_malloc_touch(Pike_fp);
	Pike_fp->num_args=args;
	new_frame->current_storage = o->storage+new_frame->context->storage_offset;
	new_frame->num_locals=args;
	(*function->func.c_fun)(args);
	break;

      case IDENTIFIER_CONSTANT:
      {
	struct svalue *s=&(Pike_fp->context->prog->
			   constants[function->func.const_info.offset].sval);
	debug_malloc_touch(Pike_fp);
	if(TYPEOF(*s) == T_PROGRAM)
	{
	  struct object *tmp;
	  Pike_fp->num_args=args;
	  tmp=parent_clone_object(s->u.program,
				  o,
				  fun,
				  args);
	  push_object(tmp);
	  break;
	}
      }
      /* FALLTHRU */

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
	  memmove(Pike_sp-args,Pike_sp-args-1,sizeof(struct svalue)*args);
#ifdef DEBUG_MALLOC
	  if (args) {
	    int i;
	    /* Note: touch the dead svalue too. */
	    for (i=args+2; i > 0; i--) {
	      dmalloc_touch_svalue(Pike_sp-i);
	    }
	  }
#endif /* DEBUG_MALLOC */
	}else{
	  free_svalue(Pike_sp-args-1);
	}
	mark_free_svalue (Pike_sp - args - 1);
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
	if (PIKE_FN_POPFRAME_ENABLED()) {
	  /* DTrace adjust frame depth */
	  PIKE_FN_POPFRAME();
	}
	goto apply_svalue;
      }

      case IDENTIFIER_PIKE_FUNCTION:
      {
	PIKE_OPCODE_T *pc;

#ifdef PIKE_DEBUG
	if (Pike_in_gc > GC_PASS_PREPARE && Pike_in_gc < GC_PASS_FREE)
	  Pike_fatal("Pike code called within gc.\n");
    new_frame->num_args = 0;
    new_frame->num_locals = 0;
#endif

	debug_malloc_touch(Pike_fp);
	new_frame->current_storage = o->storage+new_frame->context->storage_offset;
	pc=new_frame->context->prog->program + function->func.offset;

	new_frame->save_mark_sp=Pike_mark_sp;
	new_frame->pc = pc
#ifdef ENTRY_PROLOGUE_SIZE
	  + ENTRY_PROLOGUE_SIZE
#endif /* ENTRY_PROLOGUE_SIZE */
	  ;

	return new_frame->pc;
      }

      default:
	if (IDENTIFIER_IS_ALIAS(function->identifier_flags)) {
	  POP_PIKE_FRAME();
	  do {
	    struct external_variable_context loc;
	    loc.o = o;
	    loc.inherit = INHERIT_FROM_INT(p, fun);
	    loc.parent_identifier = 0;
	    find_external_context(&loc, function->func.ext_ref.depth);
	    fun = function->func.ext_ref.id;
	    p = (o = loc.o)->prog;
	    function = ID_FROM_INT(p, fun);
	  } while (IDENTIFIER_IS_ALIAS(function->identifier_flags));
	  if (PIKE_FN_POPFRAME_ENABLED()) {
	    /* DTrace adjust frame depth */
	    PIKE_FN_POPFRAME();
	  }
	  goto apply_low;
	}
#ifdef PIKE_DEBUG
	Pike_fatal("Unknown identifier type.\n");
#endif
      }
      POP_PIKE_FRAME();
    }
