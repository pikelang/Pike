/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include <math.h>
#include <ctype.h>
#include "global.h"
#include "interpret.h"
#include "svalue.h"
#include "array.h"
#include "stralloc.h"
#include "mapping.h"
#include "multiset.h"
#include "opcodes.h"
#include "object.h"
#include "error.h"
#include "pike_types.h"
#include "pike_memory.h"
#include "fd_control.h"
#include "cyclic.h"
#include "builtin_functions.h"

RCSID("$Id: opcodes.c,v 1.17 1998/04/06 04:18:33 hubbe Exp $");

void index_no_free(struct svalue *to,struct svalue *what,struct svalue *ind)
{
  INT32 i;
  switch(what->type)
  {
  case T_ARRAY:
    simple_array_index_no_free(to,what->u.array,ind);
    break;

  case T_MAPPING:
    mapping_index_no_free(to,what->u.mapping,ind);
    break;

  case T_OBJECT:
    object_index_no_free(to, what->u.object, ind);
    break;

  case T_MULTISET:
    i=multiset_member(what->u.multiset, ind);
    to->type=T_INT;
    to->subtype=i ? NUMBER_UNDEFINED : 0;
    to->u.integer=i;
    break;

  case T_STRING:
    if(ind->type==T_INT)
    {
      i=ind->u.integer;
      if(i<0)
	i+=what->u.string->len;
      if(i<0 || i>=what->u.string->len)
	error("Index out of range.\n");
      else
	i=EXTRACT_UCHAR(what->u.string->str + i);
      to->type=T_INT;
      to->subtype=NUMBER_NUMBER;
      to->u.integer=i;
      break;
    }else{
      error("Index is not an integer.\n");
    }

  default:
    error("Indexing a basic type.\n");
  }
}

void o_index(void)
{
  struct svalue s;
  index_no_free(&s,sp-2,sp-1);
  pop_n_elems(2);
  *sp=s;
  sp++;
}

void o_cast(struct pike_string *type, INT32 run_time_type)
{
  INT32 i;

  if(run_time_type != sp[-1].type)
  {
    if(run_time_type == T_MIXED)
      return;

    if(sp[-1].type == T_OBJECT)
    {
      struct pike_string *s;
      s=describe_type(type);
      push_string(s);
      if(!sp[-2].u.object->prog)
	error("Cast called on destructed object.\n");
      if(FIND_LFUN(sp[-2].u.object->prog,LFUN_CAST) == -1)
	error("No cast method in object.\n");
      apply_lfun(sp[-2].u.object, LFUN_CAST, 1);
      free_svalue(sp-2);
      sp[-2]=sp[-1];
      sp--;
      return;
    }

    switch(run_time_type)
    {
      case T_MIXED:
	return;
	
      case T_ARRAY:
	switch(sp[-1].type)
	{
	  case T_MAPPING:
	  {
	    struct array *a=mapping_to_array(sp[-1].u.mapping);
	    pop_stack();
	    push_array(a);
	    break;
	  }

	  case T_STRING:
	    f_values(1);
	    break;

	  case T_MULTISET:
	    f_indices(1);
	    break;

	  default:
	    error("Cannot cast to array.\n");
	      
	}
	break;
	
      case T_INT:
	switch(sp[-1].type)
	{
	  case T_FLOAT:
	    i=(int)(sp[-1].u.float_number);
	    break;
	    
	  case T_STRING:
	    i=STRTOL(sp[-1].u.string->str,0,0);
	    free_string(sp[-1].u.string);
	    break;
	    
	  default:
	    error("Cannot cast to int.\n");
	}
	
	sp[-1].type=T_INT;
	sp[-1].u.integer=i;
	break;
	
      case T_FLOAT:
      {
	FLOAT_TYPE f;
	switch(sp[-1].type)
	{
	  case T_INT:
	    f=(FLOAT_TYPE)(sp[-1].u.integer);
	    break;
	    
	  case T_STRING:
	    f=STRTOD(sp[-1].u.string->str,0);
	    free_string(sp[-1].u.string);
	    break;
	    
	  default:
	    error("Cannot cast to float.\n");
	    f=0.0;
	}
	
	sp[-1].type=T_FLOAT;
	sp[-1].u.float_number=f;
	break;
      }
      
      case T_STRING:
      {
	char buf[200];
	switch(sp[-1].type)
	{
	  case T_INT:
	    sprintf(buf,"%ld",(long)sp[-1].u.integer);
	    break;
	    
	  case T_FLOAT:
	    sprintf(buf,"%f",(double)sp[-1].u.float_number);
	    break;
	    
	  default:
	    error("Cannot cast to string.\n");
	}
	
	sp[-1].type=T_STRING;
	sp[-1].u.string=make_shared_string(buf);
	break;
      }
      
      case T_OBJECT:
	switch(sp[-1].type)
	{
	  case T_STRING:
	    if(fp->pc)
	    {
	      INT32 lineno;
	      push_text(get_line(fp->pc, fp->context.prog, &lineno));
	    }else{
	      push_int(0);
	    }
	    APPLY_MASTER("cast_to_object",2);
	    return;
	    
	  case T_FUNCTION:
	    sp[-1].type = T_OBJECT;
	    break;

	  default:
	    error("Cannot cast to object.\n");
	}
	break;
	
      case T_PROGRAM:
      switch(sp[-1].type)
      {
	case T_STRING:
	  if(fp->pc)
	  {
	    INT32 lineno;
	    push_text(get_line(fp->pc, fp->context.prog, &lineno));
	  }else{
	    push_int(0);
	  }
	  APPLY_MASTER("cast_to_program",2);
	  return;
	  
	case T_FUNCTION:
	{
	  struct program *p=program_from_function(sp-1);
	  if(p)
	  {
	    p->refs++;
	    pop_stack();
	    push_program(p);
	  }else{
	    pop_stack();
	    push_int(0);
	  }
	}
	return;

	default:
	  error("Cannot cast that to a program.\n");
      }

	
      case T_FUNCTION:
      {
	INT32 i;
	if(fp->current_object->prog)
	  error("Cast to function in destructed object.\n");
	i=find_shared_string_identifier(sp[-1].u.string,fp->current_object->prog);
	free_string(sp[-1].u.string);
	/* FIXME, check that it is a indeed a function */
	if(i==-1)
	{
	  sp[-1].type=T_FUNCTION;
	  sp[-1].subtype=i;
	  sp[-1].u.object=fp->current_object;
	  fp->current_object->refs++;
	}else{
	  sp[-1].type=T_INT;
	  sp[-1].subtype=NUMBER_UNDEFINED;
	  sp[-1].u.integer=0;
	}
	break;
      }
    }
  }

#ifdef DEBUG
  if(run_time_type != sp[-1].type)
    fatal("Internal error: Cast failed (run_time_type = %d, sp[-1].type = %d.)\n",run_time_type,sp[-1].type);
#endif

  switch(run_time_type)
  {
    case T_ARRAY:
    {
      struct pike_string *itype;
      INT32 run_time_itype;

      push_string(itype=index_type(sp[-2].u.string,0));
      run_time_itype=compile_type_to_runtime_type(itype);

      if(run_time_itype != T_MIXED)
      {
	struct array *a;
	struct array *tmp=sp[-2].u.array;
	DECLARE_CYCLIC();
	
	if((a=(struct array *)BEGIN_CYCLIC(tmp,0)))
	{
	  ref_push_array(a);
	}else{
	  INT32 e,i;
	  struct pike_string *s;
	  push_array(a=allocate_array(tmp->size));
	  SET_CYCLIC_RET(a);
	  
	  for(e=0;e<a->size;e++)
	  {
	    push_svalue(tmp->item+e);
	    o_cast(itype, run_time_itype);
	    array_set_index(a,e,sp-1);
	    pop_stack();
	  }
	  END_CYCLIC();
	}
	assign_svalue(sp-3,sp-1);
	pop_stack();
      }
      pop_stack();
    }
  }
}


void f_cast(void)
{
#ifdef DEBUG
  struct svalue *save_sp=sp;
#endif
  o_cast(sp[-2].u.string,
	 compile_type_to_runtime_type(sp[-2].u.string));
#ifdef DEBUG
  if(save_sp != sp)
    fatal("Internal error: o_cast() left droppings on stack.\n");
#endif
  free_svalue(sp-2);
  sp[-2]=sp[-1];
  sp--;
}


/*
  flags:
   *
  operators:
  %d
  %s
  %f
  %c
  %n
  %[
  %%
*/

static int read_set(char *match,int cnt,char *set,int match_len)
{
  int init;
  int last=0;
  int e;

  if(cnt>=match_len)
    error("Error in sscanf format string.\n");

  if(match[cnt]=='^')
  {
    for(e=0;e<256;e++) set[e]=1;
    init=0;
    cnt++;
    if(cnt>=match_len)
      error("Error in sscanf format string.\n");
  }else{
    for(e=0;e<256;e++) set[e]=0;
    init=1;
  }
  if(match[cnt]==']')
  {
    set[last=']']=init;
    cnt++;
    if(cnt>=match_len)
      error("Error in sscanf format string.\n");
  }

  for(;match[cnt]!=']';cnt++)
  {
    if(match[cnt]=='-')
    {
      cnt++;
      if(cnt>=match_len)
	error("Error in sscanf format string.\n");
      if(match[cnt]==']')
      {
	set['-']=init;
	break;
      }
      for(e=last;e<(int) EXTRACT_UCHAR(match+cnt);e++) set[e]=init;
    }
    set[last=EXTRACT_UCHAR(match+cnt)]=init;
  }
  return cnt;
}

static INT32 low_sscanf(INT32 num_arg)
{
  char *input;
  int input_len;
  char *match;
  int match_len;
  struct svalue sval;
  int e,cnt,matches,eye,arg;
  int no_assign,field_length;
  char set[256];
  struct svalue *argp;
  
  if(num_arg < 2) error("Too few arguments to sscanf.\n");
  argp=sp-num_arg;

  if(argp[0].type != T_STRING) error("Bad argument 1 to sscanf.\n");
  if(argp[1].type != T_STRING) error("Bad argument 2 to sscanf.\n");

  input=argp[0].u.string->str;
  input_len=argp[0].u.string->len;;

  match=argp[1].u.string->str;
  match_len=argp[1].u.string->len;

  arg=eye=matches=0;

  for(cnt=0;cnt<match_len;cnt++)
  {
    for(;cnt<match_len;cnt++)
    {
      if(match[cnt]=='%')
      {
        if(match[cnt+1]=='%')
        {
          cnt++;
        }else{
          break;
        }
      }
      if(eye>=input_len || input[eye]!=match[cnt])
	return matches;
      eye++;
    }
    if(cnt>=match_len) return matches;

#ifdef DEBUG
    if(match[cnt]!='%' || match[cnt+1]=='%')
    {
      fatal("Error in sscanf.\n");
    }
#endif

    no_assign=0;
    field_length=-1;

    cnt++;
    if(cnt>=match_len)
      error("Error in sscanf format string.\n");

    while(1)
    {
      switch(match[cnt])
      {
      case '*':
	no_assign=1;
	cnt++;
	if(cnt>=match_len)
	  error("Error in sscanf format string.\n");
	continue;

      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
      {
	char *t;
	field_length=STRTOL(match+cnt,&t,10);
	cnt=t-match;
	continue;
      }

      case 'c':
	if(field_length == -1) field_length = 1;
	if(eye+field_length > input_len) return matches;
	sval.type=T_INT;
	sval.subtype=NUMBER_NUMBER;
	sval.u.integer=0;
	while(--field_length >= 0)
	{
	  sval.u.integer<<=8;
	  sval.u.integer|=EXTRACT_UCHAR(input+eye);
	  eye++;
	}
	break;

      case 'd':
      {
	char * t;

	if(eye>=input_len) return matches;
	sval.u.integer=STRTOL(input+eye,&t,10);
	if(input + eye == t) return matches;
	eye=t-input;
	sval.type=T_INT;
	sval.subtype=NUMBER_NUMBER;
	break;
      }

      case 'x':
      {
	char * t;

	if(eye>=input_len) return matches;
	sval.u.integer=STRTOL(input+eye,&t,16);
	if(input + eye == t) return matches;
	eye=t-input;
	sval.type=T_INT;
	sval.subtype=NUMBER_NUMBER;
	break;
      }

      case 'o':
      {
	char * t;

	if(eye>=input_len) return matches;
	sval.u.integer=STRTOL(input+eye,&t,8);
	if(input + eye == t) return matches;
	eye=t-input;
	sval.type=T_INT;
	sval.subtype=NUMBER_NUMBER;
	break;
      }

      case 'D':
      case 'i':
      {
	char * t;

	if(eye>=input_len) return matches;
	sval.u.integer=STRTOL(input+eye,&t,0);
	if(input + eye == t) return matches;
	eye=t-input;
	sval.type=T_INT;
	sval.subtype=NUMBER_NUMBER;
	break;
      }

      case 'f':
      {
	char * t;

	if(eye>=input_len) return matches;
	sval.u.float_number=STRTOD(input+eye,&t);
	if(input + eye == t) return matches;
	eye=t-input;
	sval.type=T_FLOAT;
#ifdef __CHECKER__
	sval.subtype=0;
#endif
	break;
      }

      case 's':
	if(field_length != -1)
	{
	  if(input_len - eye < field_length)
	    return matches;

	  sval.type=T_STRING;
#ifdef __CHECKER__
	  sval.subtype=0;
#endif
	  sval.u.string=make_shared_binary_string(input+eye,field_length);
	  eye+=field_length;
	  break;
	}

	if(cnt+1>=match_len)
	{
	  sval.type=T_STRING;
#ifdef __CHECKER__
	  sval.subtype=0;
#endif
	  sval.u.string=make_shared_binary_string(input+eye,input_len-eye);
	  eye=input_len;
	  break;
	}else{
	  char *end_str_start;
	  char *end_str_end;
	  char *s=0;		/* make gcc happy */
	  char *p=0;		/* make gcc happy */
	  int start,contains_percent_percent, new_eye;

	  start=eye;
	  end_str_start=match+cnt+1;
          
	  s=match+cnt+1;
	test_again:
	  if(*s=='%')
	  {
	    s++;
	    if(*s=='*') s++;
	    switch(*s)
	    {
	    case 'n':
	      s++;
	      goto test_again;
	      
	    case 's':
	      error("Illegal to have two adjecent %%s.\n");
	      return 0;		/* make gcc happy */
	      
	      /* sscanf("foo-bar","%s%d",a,b) might not work as expected */
	    case 'd':
	      for(e=0;e<256;e++) set[e]=1;
	      for(e='0';e<='9';e++) set[e]=0;
	      set['-']=0;
	      goto match_set;

	    case 'o':
	      for(e=0;e<256;e++) set[e]=1;
	      for(e='0';e<='7';e++) set[e]=0;
	      goto match_set;

	    case 'x':
	      for(e=0;e<256;e++) set[e]=1;
	      for(e='0';e<='9';e++) set[e]=0;
	      for(e='a';e<='f';e++) set[e]=0;
	      goto match_set;

	    case 'D':
	      for(e=0;e<256;e++) set[e]=1;
	      for(e='0';e<='9';e++) set[e]=0;
	      set['-']=0;
	      set['x']=0;
	      goto match_set;

	    case 'f':
	      for(e=0;e<256;e++) set[e]=1;
	      for(e='0';e<='9';e++) set[e]=0;
	      set['.']=set['-']=0;
	      goto match_set;

	    case '[':		/* oh dear */
	      read_set(match,s-match+1,set,match_len);
	      for(e=0;e<256;e++) set[e]=!set[e];
	      goto match_set;
	    }
	  }

	  contains_percent_percent=0;

	  for(e=cnt;e<match_len;e++)
	  {
	    if(match[e]=='%')
	    {
	      if(match[e+1]=='%')
	      {
		contains_percent_percent=1;
		e++;
	      }else{
		break;
	      }
	    }
	  }
	   
	  end_str_end=match+e;

	  if(!contains_percent_percent)
	  {
	    s=my_memmem(end_str_start,
			end_str_end-end_str_start,
			input+eye,
			input_len-eye);
	    if(!s) return matches;
	    eye=s-input;
	    new_eye=eye+end_str_end-end_str_start;
	  }else{
	    for(;eye<input_len;eye++)
	    {
	      p=input+eye;
	      for(s=end_str_start;s<end_str_end;s++,p++)
	      {
		if(*s!=*p) break;
		if(*s=='%') s++;
	      }
	      if(s==end_str_end)
		break;
	    }
	    if(eye==input_len)
	      return matches;
	    new_eye=p-input;
	  }

	  sval.type=T_STRING;
#ifdef __CHECKER__
	  sval.subtype=0;
#endif
	  sval.u.string=make_shared_binary_string(input+start,eye-start);

	  cnt=end_str_end-match-1;
	  eye=new_eye;
	  break;
	}

      case '[':
	cnt=read_set(match,cnt+1,set,match_len);

      match_set:
	for(e=eye;eye<input_len && set[EXTRACT_UCHAR(input+eye)];eye++);
	sval.type=T_STRING;
#ifdef __CHECKER__
	sval.subtype=0;
#endif
	sval.u.string=make_shared_binary_string(input+e,eye-e);
	break;

      case 'n':
	sval.type=T_INT;
	sval.subtype=NUMBER_NUMBER;
	sval.u.integer=eye;
	break;
    
      default:
	error("Unknown sscanf token %%%c\n",match[cnt]);
      }
      break;
    }
    matches++;

    if(!no_assign)
    {
      arg++;
      if((num_arg-2)/2<arg)
      {
	free_svalue(&sval);
	error("Too few arguments for format to sscanf.\n");
      }
      assign_lvalue(argp+(2+(arg-1)*2), &sval);
    }
    free_svalue(&sval);
  }
  return matches;
}

void f_sscanf(INT32 args)
{
#ifdef DEBUG
  extern int t_flag;
#endif
  INT32 i;
  i=low_sscanf(args);
#ifdef DEBUG
  if(t_flag >2)
  {
    int nonblock;
    if((nonblock=query_nonblocking(2)))
      set_nonblocking(2,0);
    
    fprintf(stderr,"-    Matches: %ld\n",(long)i);
    if(nonblock)
      set_nonblocking(2,1);
  }
#endif  
  pop_n_elems(args);
  push_int(i);
}
