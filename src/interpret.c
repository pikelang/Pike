/*\
||| This file a part of uLPC, and is copyright by Fredrik Hubinette
||| uLPC is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
#include "interpret.h"
#include "object.h"
#include "program.h"
#include "svalue.h"
#include "array.h"
#include "mapping.h"
#include "error.h"
#include "language.h"
#include "stralloc.h"
#include "add_efun.h"
#include "macros.h"
#include "list.h"
#include "backend.h"
#include "operators.h"
#include "opcodes.h"
#include "main.h"
#include "lex.h"
#include "builtin_efuns.h"
#include "lpc_signal.h"

#define TRACE_LEN 256
struct svalue evaluator_stack[EVALUATOR_STACK_SIZE];
struct svalue *mark_stack[EVALUATOR_STACK_SIZE];
struct frame *fp; /* frame pointer */

/* sp points to first unused value on stack
 * (much simpler than letting it point at the last used value.)
 */
struct svalue *sp=evaluator_stack;

/* mark stack, used to store markers into the normal stack */
struct svalue **mark_sp=mark_stack;

static void eval_instruction(unsigned char *pc);


/*
 * lvalues are stored in two svalues in one of these formats:
 * array[index]   : { array, index } 
 * mapping[index] : { mapping, index } 
 * list[index] : { list, index } 
 * local variable : { svalue_pointer, nothing } 
 * global variable : { svalue_pointer/short_svalue_pointer, nothing } 
 */

void lvalue_to_svalue_no_free(struct svalue *to,struct svalue *lval)
{
  switch(lval->type)
  {
  case T_LVALUE:
    assign_svalue_no_free(to, lval->u.lval);
    break;

  case T_SHORT_LVALUE:
    assign_from_short_svalue_no_free(to, lval->u.short_lval, lval->subtype);
    break;

  case T_OBJECT:
    object_index_no_free(to, lval->u.object, lval+1);
    break;

  case T_ARRAY:
    simple_array_index_no_free(to, lval->u.array, lval+1);
    break;

  case T_MAPPING:
    mapping_index_no_free(to, lval->u.mapping, lval+1);
    break;

  case T_LIST:
    to->type=T_INT;
    if(list_member(lval->u.list,lval+1))
    {
      to->u.integer=0;
      to->subtype=NUMBER_UNDEFINED;
    }else{
      to->u.integer=0;
      to->subtype=NUMBER_NUMBER;
    }
    break;
    
  default:
    error("Indexing a basic type.\n");
  }
}

void assign_lvalue(struct svalue *lval,struct svalue *from)
{
  switch(lval->type)
  {
  case T_LVALUE:
    assign_svalue(lval->u.lval,from);
    break;

  case T_SHORT_LVALUE:
    assign_to_short_svalue(lval->u.short_lval, lval->subtype, from);
    break;

  case T_OBJECT:
    object_set_index(lval->u.object, lval+1, from);
    break;

  case T_ARRAY:
    simple_set_index(lval->u.array, lval+1, from);
    break;

  case T_MAPPING:
    mapping_insert(lval->u.mapping, lval+1, from);
    break;

  case T_LIST:
    if(IS_ZERO(from))
      list_delete(lval->u.list, lval+1);
    else
      list_insert(lval->u.list, lval+1);
    break;
    
  default:
    error("Indexing a basic type.\n");
  }
}

union anything *get_pointer_if_this_type(struct svalue *lval, TYPE_T t)
{
  switch(lval->type)
  {
  case T_LVALUE:
    if(lval->u.lval->type == t) return & ( lval->u.lval->u );
    return 0;

  case T_SHORT_LVALUE:
    if(lval->subtype == t) return lval->u.short_lval;
    return 0;

  case T_OBJECT:
    return object_get_item_ptr(lval->u.object,lval+1,t);

  case T_ARRAY:
    return array_get_item_ptr(lval->u.array,lval+1,t);

  case T_MAPPING:
    return mapping_get_item_ptr(lval->u.mapping,lval+1,t);

  case T_LIST: return 0;

  default:
    error("Indexing a basic type.\n");
    return 0;
  }
}

void pop_n_elems(INT32 x)
{
#ifdef DEBUG
  if(sp - &(evaluator_stack[0]) < x)
    fatal("Popping out of stack.\n");

  if(x < 0) fatal("Popping negative number of args....\n");
#endif
  free_svalues(sp-x,x);
  sp-=x;
}

#ifdef DEBUG
static char trace_buffer[100];
#define GET_ARG() (backlog[backlogp].arg=(\
  instr=prefix,\
  prefix=0,\
  instr+=EXTRACT_UCHAR(pc++),\
  (t_flag>3 ? sprintf(trace_buffer,"-    Arg = %ld\n",(long)instr),write_to_stderr(trace_buffer,strlen(trace_buffer)) : 0),\
  instr))

#else
#define GET_ARG() (instr=prefix,prefix=0,instr+EXTRACT_UCHAR(pc++))
#endif

#define CASE(X) case (X)-F_OFFSET:

#define COMPARISMENT(ID,EXPR) \
CASE(ID); \
instr=EXPR; \
pop_n_elems(2); \
sp->type=T_INT; \
sp->u.integer=instr; \
sp++; \
break;

#define INC_OR_DEC(ID,EXPR) \
CASE(ID) \
{ \
  union anything *u=get_pointer_if_this_type(sp-2, T_INT); \
  if(!u) error("++ or -- on non-integer.\n"); \
  instr=EXPR; \
  pop_n_elems(2); \
  sp->type=T_INT; \
  sp->u.integer=instr; \
  sp++; \
  break; \
}

#define INC_OR_DEC_AND_POP(ID,EXPR) \
CASE(ID) \
{ \
  union anything *u=get_pointer_if_this_type(sp-2, T_INT); \
  if(!u) error("++ or -- on non-integer.\n"); \
  EXPR; \
  pop_n_elems(2); \
  break; \
}

#define LOOP(ID, OP1, OP2) \
CASE(ID) \
{ \
  union anything *i=get_pointer_if_this_type(sp-2, T_INT); \
  if(!i) error("Lvalue not usable in loop.\n"); \
  OP1 ( i->integer ); \
  if( i->integer OP2 sp[-3].u.integer) \
  { \
    pc+=EXTRACT_INT(pc); \
    check_signals(); \
  }else{ \
    pc+=sizeof(INT32); \
    pop_n_elems(3); \
  } \
  break; \
}

/*
 * reset the stack machine.
 */
void reset_evaluator()
{
  fp=0;
  pop_n_elems(sp - evaluator_stack);
}

/* Put catch outside of eval_instruction, so
 * the setjmp won't affect the optimization of
 * eval_instruction
 */
void f_catch(unsigned char *pc)
{
  JMP_BUF tmp;
  if(SETJMP(tmp))
  {
    *sp=throw_value;
    throw_value.type=T_INT;
    sp++;
  }else{
    eval_instruction(pc);
    push_int(0);
  }
  UNSETJMP(tmp);
}

#ifdef DEBUG
#define BACKLOG 512
struct backlog
{
  INT32 instruction;
  INT32 arg;
  struct program *program;
  unsigned char *pc;
};

struct backlog backlog[BACKLOG];
int backlogp=BACKLOG-1;

void dump_backlog()
{
  int e;
  if(!d_flag || backlogp<0 || backlogp>=BACKLOG)
    return;

  e=backlogp;
  do
  {
    e++;
    if(e>=BACKLOG) e=0;

    if(backlog[e].program)
    {
      char *file;
      INT32 line;

      file=get_line(backlog[e].pc-1,backlog[e].program, &line);
      fprintf(stderr,"%s:%ld: %s(%ld)\n",
	      file,
	      (long)line,
	      low_get_f_name(backlog[e].instruction + F_OFFSET, backlog[e].program),
	      (long)backlog[e].arg);
    }
  }while(e!=backlogp);
}

#endif

static void eval_instruction(unsigned char *pc)
{
  unsigned INT32 instr, prefix=0;
  while(1)
  {
    fp->pc = pc;
    instr=EXTRACT_UCHAR(pc++);

  again:
#ifdef DEBUG
    sp[0].type=99; /* an invalid type */
    sp[1].type=99;
    sp[2].type=99;
    sp[3].type=99;

    if(sp<evaluator_stack || mark_sp < mark_stack || fp->locals>sp)
      fatal("Stack error.\n");

    if(fp->fun>=0 && fp->current_object->prog &&
	fp->locals+fp->num_locals > sp)
      fatal("Stack error.\n");

    if(d_flag)
    {
      if(d_flag > 9) check_signals();

      backlogp++;
      if(backlogp >= BACKLOG) backlogp=0;

      if(backlog[backlogp].program)
	free_program(backlog[backlogp].program);

      backlog[backlogp].program=fp->context.prog;
      fp->context.prog->refs++;
      backlog[backlogp].instruction=instr;
      backlog[backlogp].arg=0;
      backlog[backlogp].pc=pc;
    }

    if(t_flag > 2)
    {
      char *file, *f;
      INT32 linep, nonblock;
      if((nonblock=query_nonblocking(2)))
	set_nonblocking(2,0);

      file=get_line(pc-1,fp->context.prog,&linep);
      while((f=strchr(file,'/'))) file=f+1;
      fprintf(stderr,"- %s:%4ld:(%lx): %-25s %4ld %4ld\n",
	      file,(long)linep,
	      (long)(pc-fp->context.prog->program-1),
	      get_f_name(instr + F_OFFSET),
	      (long)(sp-evaluator_stack),
	      (long)(mark_sp-mark_stack));
      if(nonblock)
	set_nonblocking(2,1);
    }

#endif
    switch(instr)
    {
      /* Support for large instructions */
      CASE(F_ADD_256); instr=EXTRACT_UCHAR(pc++)+256; goto again;
      CASE(F_ADD_512); instr=EXTRACT_UCHAR(pc++)+512; goto again;
      CASE(F_ADD_768); instr=EXTRACT_UCHAR(pc++)+768; goto again;
      CASE(F_ADD_1024);instr=EXTRACT_UCHAR(pc++)+1024;goto again;
      CASE(F_ADD_256X); instr=EXTRACT_UWORD(pc); pc+=sizeof(INT16); goto again;

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
      /* Push number */
      CASE(F_CONST0); sp->type=T_INT; sp->u.integer=0;  sp++; break;
      CASE(F_CONST1); sp->type=T_INT; sp->u.integer=1;  sp++; break;
      CASE(F_CONST_1);sp->type=T_INT; sp->u.integer=-1; sp++; break;
      CASE(F_NUMBER); sp->type=T_INT; sp->u.integer=GET_ARG(); sp++; break;
      CASE(F_NEG_NUMBER);
      sp->type=T_INT;
      sp->u.integer=-GET_ARG();
      sp++;
      break;

      /* The rest of the basic 'push value' instructions */	
      CASE(F_STRING);
      copy_shared_string(sp->u.string,fp->context.prog->strings[GET_ARG()]);
      sp->type=T_STRING;
      sp++;
      break;

      CASE(F_CONSTANT);
      assign_svalue_no_free(sp++,fp->context.prog->constants+GET_ARG());
      break;

      CASE(F_FLOAT);
      sp->type=T_FLOAT;
      MEMCPY((void *)&sp->u.float_number, pc, sizeof(FLOAT_TYPE));
      pc+=sizeof(FLOAT_TYPE);
      sp++;
      break;

      CASE(F_LFUN);
      sp->u.object=fp->current_object;
      fp->current_object->refs++;
      sp->subtype=GET_ARG()+fp->context.identifier_level;
      sp->type=T_FUNCTION;
      sp++;
      break;

      /* The not so basic 'push value' instructions */
      CASE(F_GLOBAL)
      {
	struct identifier *i;
	INT32 tmp=GET_ARG() + fp->context.identifier_level;

	if(!fp->current_object->prog)
	  error("Cannot access global variables in destructed object.\n");

	i=ID_FROM_INT(fp->current_object->prog, tmp);
	if(i->run_time_type == T_MIXED)
	{
	  struct svalue *s;
	  s=(struct svalue *)GLOBAL_FROM_INT(tmp);
	  check_destructed(s);
	  assign_svalue_no_free(sp,s);
	}else{
	  union anything *u;
	  u=(union anything *)GLOBAL_FROM_INT(tmp);
	  check_short_destructed(u,i->run_time_type);
	  
	  assign_from_short_svalue_no_free(sp,u, i->run_time_type);
	}
	sp++;
	break;
      }

      CASE(F_LOCAL);
      assign_svalue_no_free(sp++,fp->locals+GET_ARG());
      break;

      CASE(F_LOCAL_LVALUE);
      sp[0].type=T_LVALUE;
      sp[0].u.lval=fp->locals+GET_ARG();
      sp[1].type=T_VOID;
      sp+=2;
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
       * and then the low array/list/mapping manipulation routines can be
       * destructive if they like
       */
      if( (1 << sp[-1].type) & ( BIT_ARRAY | BIT_LIST | BIT_MAPPING ))
      {
	struct svalue s;
	s.type=T_INT;
	s.u.integer=0;
	assign_lvalue(sp-3,&s);
      }
      sp++;
      break;

      CASE(F_GLOBAL_LVALUE)
      {
	struct identifier *i;
	INT32 tmp=GET_ARG() + fp->context.identifier_level;

	if(!fp->current_object->prog)
	  error("Cannot access global variables in destructed object.\n");

	i=ID_FROM_INT(fp->current_object->prog, tmp);
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
      
      INC_OR_DEC(F_INC,++u->integer);
      INC_OR_DEC(F_POST_INC,u->integer++);
      INC_OR_DEC(F_DEC,--u->integer);
      INC_OR_DEC(F_POST_DEC,u->integer--);
      INC_OR_DEC_AND_POP(F_INC_AND_POP,++u->integer);
      INC_OR_DEC_AND_POP(F_DEC_AND_POP,--u->integer);

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

      CASE(F_ASSIGN_LOCAL);
      assign_svalue(fp->locals+GET_ARG(),sp-1);
      break;

      CASE(F_ASSIGN_LOCAL_AND_POP);
      sp--;
      instr=GET_ARG();
      free_svalue(fp->locals+instr);
      fp->locals[instr]=*sp;
      break;

      CASE(F_ASSIGN_GLOBAL)
      {
	struct identifier *i;
	INT32 tmp=GET_ARG() + fp->context.identifier_level;
	if(!fp->current_object->prog)
	  error("Cannot access global variables in destructed object.\n");

	i=ID_FROM_INT(fp->current_object->prog, tmp);
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
      CASE(F_MARK); *(mark_sp++)=sp; break;

      /* Jumps */
      CASE(F_BRANCH);
      check_signals();
      pc+=EXTRACT_INT(pc);
      break;

      CASE(F_BRANCH_WHEN_ZERO);
      check_destructed(sp-1);
      if(!IS_ZERO(sp-1))
      {
	pc+=sizeof(INT32);
      }else{
	check_signals();
	pc+=EXTRACT_INT(pc);
      }
      pop_stack();
      break;
      
      CASE(F_BRANCH_WHEN_NON_ZERO);
      check_destructed(sp-1);
      if(IS_ZERO(sp-1))
      {
	pc+=sizeof(INT32);
      }else{
	check_signals();
	pc+=EXTRACT_INT(pc);
      }
      pop_stack();
      break;

      CASE(F_LAND);
      check_destructed(sp-1);
      if(!IS_ZERO(sp-1))
      {
	pc+=sizeof(INT32);
	pop_stack();
      }else{
	check_signals();
	pc+=EXTRACT_INT(pc);
      }
      break;

      CASE(F_LOR);
      check_destructed(sp-1);
      if(IS_ZERO(sp-1))
      {
	pc+=sizeof(INT32);
	pop_stack();
      }else{
	check_signals();
	pc+=EXTRACT_INT(pc);
      }
      break;

      CASE(F_CATCH);
      f_catch(pc+sizeof(INT32));
      pc+=EXTRACT_INT(pc);
      break;

      CASE(F_SWITCH)
      {
	INT32 tmp;
	tmp=switch_lookup(fp->context.prog->
			  constants[EXTRACT_UWORD(pc)].u.array,sp-1);
	pc+=sizeof(INT16);
	pc=(unsigned char *)MY_ALIGN(pc);
	if(tmp >= 0)
	  pc+=((INT32 *)pc)[1+tmp*2];
	else
	  pc+=((INT32 *)pc)[2*~tmp];
	pop_stack();
	break;
      }
      
      LOOP(F_INC_LOOP, ++, <);
      LOOP(F_DEC_LOOP, --, <);
      LOOP(F_INC_NEQ_LOOP, ++, !=);
      LOOP(F_DEC_NEQ_LOOP, --, !=);

      CASE(F_FOREACH) /* lvalue, array, i */
      {
	if(sp[-4].type != T_ARRAY) error("Bad argument 1 to foreach()\n");
	if(sp[-1].u.integer < sp[-4].u.array->size)
	{
	  check_signals();
	  index_no_free(sp,sp-4,sp-1);
	  assign_lvalue(sp-3, sp);
	  free_svalue(sp);
	  pc+=EXTRACT_INT(pc);
	  sp[-1].u.integer++;
	}else{
	  pc+=sizeof(INT32);
	  pop_n_elems(4);
	}
	break;
      }

      CASE(F_RETURN_0);
      pop_n_elems(sp-fp->locals);
      check_signals();
      return;

      CASE(F_RETURN);
      if(fp->locals != sp-1)
      {
	assign_svalue(fp->locals, sp-1);
	pop_n_elems(sp - fp->locals - 1);
      }
      /* fall through */

      CASE(F_DUMB_RETURN);
      check_signals();
      return;

      CASE(F_NEGATE); 
      if(sp[-1].type == T_INT)
      {
	sp[-1].u.integer =- sp[-1].u.integer;
      }else if(sp[-1].type == T_FLOAT)
      {
	sp[-1].u.float_number =- sp[-1].u.float_number;
      }else{
	error("Bad argument to unary minus.\n");
      }
      break;

      CASE(F_COMPL);
      if(sp[-1].type != T_INT) error("Bad argument to ~.\n");
      sp[-1].u.integer = ~ sp[-1].u.integer;
      break;

      CASE(F_NOT);
      switch(sp[-1].type)
      {
      case T_INT:
	sp[-1].u.integer =! sp[-1].u.integer;
	break;

      case T_FUNCTION:
      case T_OBJECT:
	check_destructed(sp-1);
	if(sp[-1].type == T_INT)
	{
	  sp[-1].u.integer=1;
	  break;
	}

      default:
	free_svalue(sp-1);
	sp[-1].type=T_INT;
	sp[-1].u.integer=0;
      }
      break;

      CASE(F_LSH);
      if(sp[-2].type != T_INT) error("Bad argument 1 to <<\n");
      if(sp[-1].type != T_INT) error("Bad argument 2 to <<\n");
      sp--;
      sp[-1].u.integer = sp[-1].u.integer << sp->u.integer;
      break;

      CASE(F_RSH);
      if(sp[-2].type != T_INT) error("Bad argument 1 to >>\n");
      if(sp[-1].type != T_INT) error("Bad argument 2 to >>\n");
      sp--;
      sp[-1].u.integer = sp[-1].u.integer >> sp->u.integer;
      break;

      COMPARISMENT(F_EQ, is_eq(sp-2,sp-1));
      COMPARISMENT(F_NE,!is_eq(sp-2,sp-1));
      COMPARISMENT(F_GT, is_gt(sp-2,sp-1));
      COMPARISMENT(F_GE,!is_lt(sp-2,sp-1));
      COMPARISMENT(F_LT, is_lt(sp-2,sp-1));
      COMPARISMENT(F_LE,!is_gt(sp-2,sp-1));

      CASE(F_ADD);      f_sum(2);     break;
      CASE(F_SUBTRACT); f_subtract(); break;
      CASE(F_AND);      f_and();      break;
      CASE(F_OR);       f_or();       break;
      CASE(F_XOR);      f_xor();      break;
      CASE(F_MULTIPLY); f_multiply(); break;
      CASE(F_DIVIDE);   f_divide();   break;
      CASE(F_MOD);      f_mod();      break;

      CASE(F_PUSH_ARRAY);
      if(sp[-1].type!=T_ARRAY) error("Bad argument to @\n");
      sp--;
      push_array_items(sp->u.array);
      break;

      CASE(F_INDEX); f_index(); break;
      CASE(F_CAST); f_cast(); break;

      CASE(F_RANGE); f_range(); break;
      CASE(F_COPY_VALUE);
      copy_svalues_recursively_no_free(sp,sp-1,1,0);
      free_svalue(sp-1);
      sp[-1]=*sp;
      break;

      CASE(F_SSCANF); f_sscanf(GET_ARG()); break;

      CASE(F_CALL_LFUN);
      apply_low(fp->current_object,
		GET_ARG()+fp->context.identifier_level,
		sp - *--mark_sp);
      break;

    default:
      instr -= F_MAX_OPCODE - F_OFFSET;
#ifdef DEBUG
      if(instr >= fp->context.prog->num_constants)
      {
	instr += F_MAX_OPCODE;
	fatal("Strange instruction %ld\n",(long)instr);
      }
#endif      
      strict_apply_svalue(fp->context.prog->constants + instr, sp - *--mark_sp );
    }
  }
}

int apply_low_safe_and_stupid(struct object *o, INT32 offset)
{
  JMP_BUF tmp;
  struct frame new_frame;
  int ret;

  new_frame.parent_frame = fp;
  new_frame.current_object = o;
  new_frame.context=o->prog->inherits[0];
  new_frame.locals = evaluator_stack;
  new_frame.args = 0;
  new_frame.fun = -1;
  new_frame.pc = 0;
  new_frame.current_storage=o->storage;
  fp = & new_frame;

  new_frame.current_object->refs++;
  new_frame.context.prog->refs++;

  if(SETJMP(tmp))
  {
    ret=1;
  }else{
    eval_instruction(o->prog->program + offset);
#ifdef DEBUG
    if(sp<evaluator_stack)
      fatal("Stack error.\n");
#endif
    ret=0;
  }
  UNSETJMP(tmp);

  free_object(new_frame.current_object);
  free_program(new_frame.context.prog);

  fp = new_frame.parent_frame;
  return ret;
}

void apply_low(struct object *o, int fun, int args)
{
  struct program *p;
  struct reference *ref;
  struct frame new_frame;
  struct identifier *function;

  check_signals();
  if(fun<0)
  {
    pop_n_elems(args);
    push_int(0);
    return;
  }

  if(evaluator_stack+EVALUATOR_STACK_SIZE-sp < 256)
    error("Stack overflow.\n");

  p=o->prog;
  if(!p)
    error("Cannot call functions in destructed objects.\n");
#ifdef DEBUG
  if(fun>=p->num_identifier_references)
    fatal("Function index out of range.\n");
#endif

  ref = p->identifier_references + fun;
#ifdef DEBUG
  if(ref->inherit_offset>=p->num_inherits)
    fatal("Inherit offset out of range in program.\n");
#endif

  /* init a new evaluation frame */
  new_frame.parent_frame = fp;
  new_frame.current_object = o;
  new_frame.context = p->inherits[ ref->inherit_offset ];
  function = new_frame.context.prog->identifiers + ref->identifier_offset;
  
  new_frame.locals = sp - args;
  new_frame.args = args;
  new_frame.fun = fun;
  new_frame.current_storage = o->storage+new_frame.context.storage_offset;
  new_frame.pc = 0;

  new_frame.current_object->refs++;
  new_frame.context.prog->refs++;

#ifdef DEBUG
  if(t_flag)
  {
    char *file, *f;
    INT32 linep,e,nonblock;
    char buf[50],*s;

    if((nonblock=query_nonblocking(2)))
      set_nonblocking(2,0);

    if(fp && fp->pc)
    {
      file=get_line(fp->pc,fp->context.prog,&linep);
      while((f=strchr(file,'/'))) file=f+1;
    }else{
      linep=0;
      file="-";
    }

    init_buf();
    sprintf(buf,"%lx->",(long)o);
    my_strcat(buf);
    my_strcat(function->name->str);
    my_strcat("(");
    for(e=0;e<args;e++)
    {
      if(e) my_strcat(",");
      describe_svalue(sp-args+e,0,0);
    }
    my_strcat(")"); 
    s=simple_free_buf();
    if(strlen(s) > TRACE_LEN)
    {
      s[TRACE_LEN]=0;
      s[TRACE_LEN-1]='.';
      s[TRACE_LEN-2]='.';
      s[TRACE_LEN-2]='.';
    }
    fprintf(stderr,"- %s:%4ld: %s\n",file,(long)linep,s);
    free(s);

    if(nonblock)
      set_nonblocking(2,1);
  }
#endif

  fp = &new_frame;

  if(function->func.offset == -1)
    error("Calling undefined function '%s'.\n",function->name->str);

  if(function->flags & IDENTIFIER_C_FUNCTION)
  {
#if 0
    function->func.c_fun(args);
#else
    (*function->func.c_fun)(args);
#endif
  }else{
    int num_args;
    int num_locals;
    unsigned char *pc;
    pc=new_frame.context.prog->program + function->func.offset;

    num_locals=EXTRACT_UCHAR(pc++);
    num_args=EXTRACT_UCHAR(pc++);

    /* adjust arguments on stack */
    if(args < num_args) /* push zeros */
    {
      clear_svalues(sp, num_args-args);
      sp += num_args-args;
      args += num_args-args;
    }

    if(function->flags & IDENTIFIER_VARARGS)
    {
      f_aggregate(args - num_args); /* make array */
      args = num_args+1;
    }else{
      if(args > num_args)
      {
	/* pop excessive */
	pop_n_elems(args - num_args);
	args=num_args;
      }
    }

    clear_svalues(sp, num_locals - args);
    sp += num_locals - args;
#ifdef DEBUG
    fp->num_locals=num_locals;
    fp->num_args=num_args;
#endif
    eval_instruction(pc);
#ifdef DEBUG
    if(sp<evaluator_stack)
      fatal("Stack error.\n");
#endif
  }

  if(sp - new_frame.locals > 1)
  {
    pop_n_elems(sp - new_frame.locals -1);
  }else if(sp - new_frame.locals < 1){
#ifdef DEBUG
    if(sp - new_frame.locals<0) fatal("Frame underflow.\n");
#endif
    sp->u.integer = 0;
    sp->subtype=NUMBER_NUMBER;
    sp->type = T_INT;
    sp++;
  }

  free_object(new_frame.current_object);
  free_program(new_frame.context.prog);

  fp = new_frame.parent_frame;

#ifdef DEBUG
  if(t_flag)
  {
    char *s;
    int nonblock;

    if((nonblock=query_nonblocking(2)))
      set_nonblocking(2,0);

    init_buf();
    my_strcat("Return: ");
    describe_svalue(sp-1,0,0);
    s=simple_free_buf();
    if(strlen(s) > TRACE_LEN)
    {
      s[TRACE_LEN]=0;
      s[TRACE_LEN-1]='.';
      s[TRACE_LEN-2]='.';
      s[TRACE_LEN-2]='.';
    }
    fprintf(stderr,"%-*s%s\n",4,"-",s);
    free(s);

    if(nonblock)
      set_nonblocking(2,1);
  }
#endif
}

void safe_apply_low(struct object *o,int fun,int args)
{
  JMP_BUF recovery;

  sp-=args;
  if(SETJMP(recovery))
  {
    /* Should probably generate an error message here. */
    sp->u.integer = 0;
    sp->subtype=NUMBER_NUMBER;
    sp->type = T_INT;
    sp++;
  }else{
    struct svalue *expected_sp;
    expected_sp=sp+1;
    sp+=args;
    apply_low(o,fun,args);
    if(sp > expected_sp)
      pop_n_elems(sp-expected_sp);
    if(sp < expected_sp)
    {
      sp->u.integer = 0;
      sp->subtype=NUMBER_NUMBER;
      sp->type = T_INT;
      sp++;
    }
  }
  UNSETJMP(recovery);

}

void apply_shared(struct object *o,
		  struct lpc_string *fun,
		  int args)
{
  int fun_number;
  fun_number = find_shared_string_identifier(fun, o->prog);
  if(fun_number < 0)
  {
    pop_n_elems(args);
    sp++;
    sp->u.integer=0;
    sp->type=T_INT;
  }else{
    apply_low(o, fun_number, args);
  }
}

void apply(struct object *o, char *fun, int args)
{
  int fun_number;

  fun_number = find_identifier(fun, o->prog);
  if(fun_number < 0)
  {
    pop_n_elems(args);
    sp->u.integer=0;
    sp->type=T_INT;
    sp++;
  }else{
    apply_low(o, fun_number, args);
  }
}

void strict_apply_svalue(struct svalue *s, INT32 args)
{
#ifdef DEBUG
  struct svalue *save_sp;
  save_sp=sp-args;
  if(t_flag>1)
  {
    char *file, *f;
    INT32 linep,e,nonblock;
    char *st;

    if((nonblock=query_nonblocking(2)))
      set_nonblocking(2,0);

    if(fp && fp->pc)
    {
      file=get_line(fp->pc,fp->context.prog,&linep);
      while((f=strchr(file,'/'))) file=f+1;
    }else{
      linep=0;
      file="-";
    }

    init_buf();
    describe_svalue(s,0,0);
    my_strcat("(");
    for(e=0;e<args;e++)
    {
      if(e) my_strcat(",");
      describe_svalue(sp-args+e,0,0);
    }
    my_strcat(")"); 
    st=simple_free_buf();
    if(strlen(st) > TRACE_LEN)
    {
      st[TRACE_LEN]=0;
      st[TRACE_LEN-1]='.';
      st[TRACE_LEN-2]='.';
      st[TRACE_LEN-2]='.';
    }
    fprintf(stderr,"- %s:%4ld: %s\n",file,(long)linep,st);
    free(st);

    if(nonblock)
      set_nonblocking(2,1);
  }
#endif

  switch(s->type)
  {
  case T_FUNCTION:
    if(s->subtype == -1)
    {
      (*(s->u.efun->function))(args);
    }else{
      apply_low(s->u.object, s->subtype, args);
    }
    break;

  case T_ARRAY:
    apply_array(s->u.array,args);
    break;

  default:
    error("Call to non-function value.\n");
  }

#ifdef DEBUG
  if(t_flag>1 && sp>save_sp)
  {
    char *s;
    int nonblock;
    if((nonblock=query_nonblocking(2)))
      set_nonblocking(2,0);

    init_buf();
    my_strcat("Return: ");
    describe_svalue(sp-1,0,0);
    s=simple_free_buf();
    if(strlen(s) > TRACE_LEN)
    {
      s[TRACE_LEN]=0;
      s[TRACE_LEN-1]='.';
      s[TRACE_LEN-2]='.';
      s[TRACE_LEN-2]='.';
    }
    fprintf(stderr,"%-*s%s\n",4,"-",s);
    free(s);

    if(nonblock)
      set_nonblocking(2,1);
  }
#endif
}

void apply_svalue(struct svalue *s, INT32 args)
{
  if(s->type==T_INT)
  {
    pop_n_elems(args);
    push_int(0);
  }else{
    struct svalue *expected_sp=sp-args+1;
    strict_apply_svalue(s,args);
    if(sp > expected_sp)
    {
      pop_n_elems(sp-expected_sp);
    }
    else if(sp < expected_sp)
    {
      push_int(0);
    }
#ifdef DEBUG
    if(sp < expected_sp) fatal("Stack underflow!\n");
#endif
  }
}

#ifdef DEBUG
void slow_check_stack()
{
  struct svalue *s,**m;
  struct frame *f;

  check_stack();

  if(sp > &(evaluator_stack[EVALUATOR_STACK_SIZE]))
    fatal("Stack overflow\n");

  if(mark_sp < mark_stack)
    fatal("Mark stack underflow.\n");

  if(mark_sp > &(mark_stack[EVALUATOR_STACK_SIZE]))
    fatal("Mark stack underflow.\n");

  for(s=evaluator_stack;s<sp;s++) check_svalue(s);

  s=evaluator_stack;
  for(m=mark_stack;m<mark_sp;m++)
  {
    if(*m < s)
      fatal("Mark stack failiure.\n");

    s=*m;
  }

  if(s > &(evaluator_stack[EVALUATOR_STACK_SIZE]))
    fatal("Mark stack exceeds svalue stack\n");

  for(f=fp;f;f=f->parent_frame)
  {
    if(f->locals < evaluator_stack ||
       f->locals > &(evaluator_stack[EVALUATOR_STACK_SIZE]))
      fatal("Local variable pointer points to Finspång.\n");

    if(f->args < 0 || f->args > EVALUATOR_STACK_SIZE)
      fatal("FEL FEL FEL! HELP!! (corrupted frame)\n");
  }
}
#endif

void cleanup_interpret()
{
#ifdef DEBUG
  int e;
#endif

  while(fp)
  {
    free_object(fp->current_object);
    free_program(fp->context.prog);
    
    fp = fp->parent_frame;
  }

#ifdef DEBUG
  for(e=0;e<BACKLOG;e++)
  {
    if(backlog[e].program)
    {
      free_program(backlog[e].program);
      backlog[e].program=0;
    }
  }
#endif
  reset_evaluator();
}
