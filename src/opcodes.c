/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
#include <math.h>
#include <ctype.h>
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
#include "module_support.h"
#include "security.h"

RCSID("$Id: opcodes.c,v 1.42 1999/07/27 16:41:37 mirar Exp $");

void index_no_free(struct svalue *to,struct svalue *what,struct svalue *ind)
{
  INT32 i;

#ifdef PIKE_SECURITY
  if(what->type <= MAX_COMPLEX)
    if(!CHECK_DATA_SECURITY(what->u.array, SECURITY_BIT_INDEX))
      error("Index permission denied.\n");
#endif

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
	error("Index %d is out of range 0 - %d.\n", i, what->u.string->len-1);
      else
	i=index_shared_string(what->u.string,i);
      to->type=T_INT;
      to->subtype=NUMBER_NUMBER;
      to->u.integer=i;
      break;
    }else{
      error("Index is not an integer.\n");
    }

  case T_PROGRAM:
    program_index_no_free(to, what->u.program, ind);
    break;

  case T_FUNCTION:
    {
      struct program *p = program_from_svalue(what);
      if (p) {
	program_index_no_free(to, p, ind);
	break;
      }
    }
    /* FALL THROUGH */

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
    }else

    switch(run_time_type)
    {
      default:
	error("Cannot perform cast to that type.\n");
	
      case T_MIXED:
	return;

      case T_MAPPING:
	switch(sp[-1].type)
	{
	  case T_ARRAY:
	  {
	    f_transpose(1);
	    sp--;
	    push_array_items(sp->u.array);
	    f_mkmapping(2);
	    break;
	  }

	  default:
	    error("Cannot cast %s to mapping.\n",get_name_of_type(sp[-1].type));
	}
	break;
	
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
	    error("Cannot cast %s to array.\n",get_name_of_type(sp[-1].type));
	      
	}
	break;
	
      case T_INT:
	switch(sp[-1].type)
	{
	  case T_FLOAT:
	    i=(int)(sp[-1].u.float_number);
	    break;
	    
	  case T_STRING:
	    i=STRTOL(sp[-1].u.string->str,0,10);
	    free_string(sp[-1].u.string);
	    break;
	    
	  default:
	    error("Cannot cast %s to int.\n",get_name_of_type(sp[-1].type));
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
	    error("Cannot cast %s to float.\n",get_name_of_type(sp[-1].type));
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

	  case T_ARRAY:
	    {
	      int i;
	      struct array *a = sp[-1].u.array;
	      struct pike_string *s;
	      int shift = 0;

	      for(i = a->size; i--; ) {
		unsigned INT32 val;
		if (a->item[i].type != T_INT) {
		  error("cast: Item %d is not an integer.\n", i);
		}
		val = (unsigned INT32)a->item[i].u.integer;
		if (val > 0xff) {
		  shift = 1;
		  if (val > 0xffff) {
		    shift = 2;
		    break;
		  }
		  while(i--) {
		    if (a->item[i].type != T_INT) {
		      error("cast: Item %d is not an integer.\n", i);
		    }
		    val = (unsigned INT32)a->item[i].u.integer;
		    if (val > 0xffff) {
		      shift = 2;
		      break;
		    }
		  }
		  break;
		}
	      }
	      s = begin_wide_shared_string(a->size, shift);
	      switch(shift) {
	      case 0:
		for(i = a->size; i--; ) {
		  s->str[i] = a->item[i].u.integer;
		}
		break;
	      case 1:
		{
		  p_wchar1 *str1 = STR1(s);
		  for(i = a->size; i--; ) {
		    str1[i] = a->item[i].u.integer;
		  }
		}
		break;
	      case 2:
		{
		  p_wchar2 *str2 = STR2(s);
		  for(i = a->size; i--; ) {
		    str2[i] = a->item[i].u.integer;
		  }
		}
		break;
	      default:
		free_string(end_shared_string(s));
		fatal("cast: Bad shift: %d.\n", shift);
		break;
	      }
	      s = end_shared_string(s);
	      pop_stack();
	      push_string(s);
	      return;
	    }
	    break;
	    
	  default:
	    error("Cannot cast %s to string.\n",get_name_of_type(sp[-1].type));
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
	    error("Cannot cast %s to object.\n",get_name_of_type(sp[-1].type));
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
	    add_ref(p);
	    pop_stack();
	    push_program(p);
	  }else{
	    pop_stack();
	    push_int(0);
	  }
	}
	return;

	default:
	  error("Cannot cast %s to a program.\n",get_name_of_type(sp[-1].type));
      }
    }
  }

#ifdef PIKE_DEBUG
  if(run_time_type != sp[-1].type)
    fatal("Internal error: Cast failed (run_time_type = %d, sp[-1].type = %d.)\n",run_time_type,sp[-1].type);
#endif

  if (!type) return;

  switch(run_time_type)
  {
    case T_ARRAY:
    {
      struct pike_string *itype;
      INT32 run_time_itype;

      push_string(itype=index_type(type,0));
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
#ifdef PIKE_DEBUG
	  struct svalue *save_sp=sp+1;
#endif
	  push_array(a=allocate_array(tmp->size));
	  SET_CYCLIC_RET(a);
	  
	  for(e=0;e<a->size;e++)
	  {
	    push_svalue(tmp->item+e);
	    o_cast(itype, run_time_itype);
	    array_set_index(a,e,sp-1);
	    pop_stack();
	  }
#ifdef PIKE_DEBUG
	  if(save_sp!=sp)
	    fatal("o_cast left stack droppings.\n");
#endif
	  END_CYCLIC();
	}
	assign_svalue(sp-3,sp-1);
	pop_stack();
      }
      pop_stack();
    }
    break;

    case T_MULTISET:
    {
      struct pike_string *itype;
      INT32 run_time_itype;

      push_string(itype=key_type(type,0));
      run_time_itype=compile_type_to_runtime_type(itype);

      if(run_time_itype != T_MIXED)
      {
	struct multiset *m;
	struct array *tmp=sp[-2].u.multiset->ind;
	DECLARE_CYCLIC();
	
	if((m=(struct multiset *)BEGIN_CYCLIC(tmp,0)))
	{
	  ref_push_multiset(m);
	}else{
	  INT32 e,i;
	  struct array *a;
#ifdef PIKE_DEBUG
	  struct svalue *save_sp=sp+1;
#endif
	  push_multiset(m=allocate_multiset(a=allocate_array(tmp->size)));
	  
	  SET_CYCLIC_RET(m);
	  
	  for(e=0;e<a->size;e++)
	  {
	    push_svalue(tmp->item+e);
	    o_cast(itype, run_time_itype);
	    array_set_index(a,e,sp-1);
	    pop_stack();
	  }
#ifdef PIKE_DEBUG
	  if(save_sp!=sp)
	    fatal("o_cast left stack droppings.\n");
#endif
	  order_multiset(m);
	  END_CYCLIC();
	}
	assign_svalue(sp-3,sp-1);
	pop_stack();
      }
      pop_stack();
    }
    break;

    case T_MAPPING:
    {
      struct pike_string *itype,*vtype;
      INT32 run_time_itype;
      INT32 run_time_vtype;

      push_string(itype=key_type(type,0));
      run_time_itype=compile_type_to_runtime_type(itype);

      push_string(vtype=index_type(type,0));
      run_time_vtype=compile_type_to_runtime_type(vtype);

      if(run_time_itype != T_MIXED ||
	 run_time_vtype != T_MIXED)
      {
	struct mapping *m;
	struct mapping *tmp=sp[-3].u.mapping;
	DECLARE_CYCLIC();
	
	if((m=(struct mapping *)BEGIN_CYCLIC(tmp,0)))
	{
	  ref_push_mapping(m);
	}else{
	  INT32 e,i;
	  struct keypair *k;
#ifdef PIKE_DEBUG
	  struct svalue *save_sp=sp+1;
#endif
	  push_mapping(m=allocate_mapping(tmp->size));
	  
	  SET_CYCLIC_RET(m);
	  
	  MAPPING_LOOP(tmp)
	  {
	    push_svalue(& k->ind);
	    o_cast(itype, run_time_itype);
	    push_svalue(& k->val);
	    o_cast(vtype, run_time_vtype);
	    mapping_insert(m,sp-2,sp-1);
	    pop_n_elems(2);
	  }
#ifdef PIKE_DEBUG
	  if(save_sp!=sp)
	    fatal("o_cast left stack droppings.\n");
#endif
	  END_CYCLIC();
	}
	assign_svalue(sp-4,sp-1);
	pop_stack();
      }
      pop_n_elems(2);
    }
  }
}


void f_cast(void)
{
#ifdef PIKE_DEBUG
  struct svalue *save_sp=sp;
  if(sp[-2].type != T_STRING)
    fatal("Cast expression destroyed stack or left droppings!\n");
#endif
  o_cast(sp[-2].u.string,
	 compile_type_to_runtime_type(sp[-2].u.string));
#ifdef PIKE_DEBUG
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

static int read_set(unsigned char *match,int cnt,char *set,int match_len)
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
  if(match[cnt]==']' || match[cnt]=='-')
  {
    set[last=match[cnt]]=init;
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



/* Parse binary IEEE strings on a machine which uses a different kind
   of floating point internally */

#ifndef FLOAT_IS_IEEE_BIG
#ifndef FLOAT_IS_IEEE_LITTLE
#define NEED_CUSTOM_IEEE
#endif
#endif
#ifndef NEED_CUSTOM_IEEE
#ifndef DOUBLE_IS_IEEE_BIG
#ifndef DOUBLE_IS_IEEE_LITTLE
#define NEED_CUSTOM_IEEE
#endif
#endif
#endif

#ifdef NEED_CUSTOM_IEEE

#if HAVE_LDEXP
#define LDEXP ldexp
#else
extern double LDEXP(double x, int exp); /* defined in encode.c */
#endif

static INLINE float low_parse_IEEE_float(char *b, int sz)
{
  unsigned INT32 f, extra_f;
  int s, e;
  unsigned char x[4];
  double r;

  x[0] = EXTRACT_UCHAR(b);
  x[1] = EXTRACT_UCHAR(b+1);
  x[2] = EXTRACT_UCHAR(b+2);
  x[3] = EXTRACT_UCHAR(b+3);
  s = ((x[0]&0x80)? 1 : 0);

  if(sz==4) {
    e = (((int)(x[0]&0x7f))<<1)|((x[1]&0x80)>>7);
    f = (((unsigned INT32)(x[1]&0x7f))<<16)|(((unsigned INT32)x[2])<<8)|x[3];
    extra_f = 0;
    if(e==255)
      e = 9999;
    else if(e>0) {
      f |= 0x00800000;
      e -= 127+23;
    } else
      e -= 126+23;
  } else {
    e = (((int)(x[0]&0x7f))<<4)|((x[1]&0xf0)>>4);
    f = (((unsigned INT32)(x[1]&0x0f))<<16)|(((unsigned INT32)x[2])<<8)|x[3];
    extra_f = (((unsigned INT32)EXTRACT_UCHAR(b+4))<<24)|
      (((unsigned INT32)EXTRACT_UCHAR(b+5))<<16)|
      (((unsigned INT32)EXTRACT_UCHAR(b+6))<<8)|
      ((unsigned INT32)EXTRACT_UCHAR(b+7));
    if(e==2047)
      e = 9999;
    else if(e>0) {
      f |= 0x00100000;
      e -= 1023+20;
    } else
      e -= 1022+20;
  }
  if(e>=9999)
    if(f||extra_f) {
      /* NAN */
      
      /* Hmm...  No idea how to generate NaN in a portable way. */
      /* Let's turn it into a 0 for now... */
      return (float)0.0;
    } else {
      /* +/- Infinity */
#ifdef HUGE_VAL
      return (float)(s? -HUGE_VAL:HUGE_VAL);
#else
      /* This number is infinite enough...  :) */
      e = 1024;
      f = 1;
      extra_f = 0;
#endif
    }

  r = (double)f;
  if(extra_f)
    r += ((double)extra_f)/4294967296.0;
  return (float)(s? -LDEXP(r, e):LDEXP(r, e));
}

#endif

static INT32 really_low_sscanf(char *input,
			       long input_len,
			       char *match,
			       long match_len,
			       long *chars_matched,
			       int *success)
{
  struct svalue sval;
  int e,cnt,matches,eye,arg;
  int no_assign,field_length,minus_flag;
  char set[256];
  struct svalue *argp;
  
  success[0]=0;

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
      {
	chars_matched[0]=eye;
	return matches;
      }
      eye++;
    }
    if(cnt>=match_len)
    {
      chars_matched[0]=eye;
      return matches;
    }

#ifdef PIKE_DEBUG
    if(match[cnt]!='%' || match[cnt+1]=='%')
    {
      fatal("Error in sscanf.\n");
    }
#endif

    no_assign=0;
    field_length=-1;
    minus_flag=0;

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

        case '-':
	  minus_flag=1;
	  cnt++;
	  continue;

	case '{':
	{
	  ONERROR err;
	  long tmp;
	  for(e=cnt+1,tmp=1;tmp;e++)
	  {
	    if(!match[e])
	    {
	      error("Missing %%} in format string.\n");
	      break;		/* UNREACHED */
	    }
	    if(match[e]=='%')
	    {
	      switch(match[e+1])
	      {
		case '%': e++; break;
		case '}': tmp--; break;
		case '{': tmp++; break;
	      }
	    }
	  }
	  sval.type=T_ARRAY;
	  sval.u.array=allocate_array(0);
	  SET_ONERROR(err, do_free_array, sval.u.array);

	  while(input_len-eye)
	  {
	    int yes;
	    struct svalue *save_sp=sp;
	    really_low_sscanf(input+eye,
			      input_len-eye,
			      match+cnt+1,
			      e-cnt-2,
			      &tmp,
			      &yes);
	    if(yes && tmp)
	    {
	      f_aggregate(sp-save_sp);
	      sval.u.array=append_array(sval.u.array,sp-1);
	      pop_stack();
	      eye+=tmp;
	    }else{
	      pop_n_elems(sp-save_sp);
	      break;
	    }
	  }
	  cnt=e;
	  UNSET_ONERROR(err);
	  break;
	}

	case 'c':
	  if(field_length == -1) field_length = 1;
	  if(eye+field_length > input_len)
	  {
	    chars_matched[0]=eye;
	    return matches;
	  }
	  sval.type=T_INT;
	  sval.subtype=NUMBER_NUMBER;
	  sval.u.integer=0;
	  if (minus_flag)
	  {
	     int pos=0;
	     while(--field_length >= 0)
	     {
		sval.u.integer|=(EXTRACT_UCHAR(input+eye))<<pos;
		pos+=8;
		eye++;
	     }
	  }
	  else
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

	  if(eye>=input_len)
	  {
	    chars_matched[0]=eye;
	    return matches;
	  }
	  if(field_length != -1 && eye+field_length < input_len)
	  {
	    char save=input[eye+field_length];
	    input[eye+field_length]=0; /* DANGEROUS */
	    sval.u.integer=STRTOL(input+eye,&t,10);
	    input[eye+field_length]=save;
	  }else
	    sval.u.integer=STRTOL(input+eye,&t,10);

	  if(input + eye == t)
	  {
	    chars_matched[0]=eye;
	    return matches;
	  }
	  eye=t-input;
	  sval.type=T_INT;
	  sval.subtype=NUMBER_NUMBER;
	  break;
	}

	case 'x':
	{
	  char * t;

	  if(eye>=input_len)
	  {
	    chars_matched[0]=eye;
	    return matches;
	  }
	  if(field_length != -1 && eye+field_length < input_len)
	  {
	    char save=input[eye+field_length];
	    input[eye+field_length]=0; /* DANGEROUS */
	    sval.u.integer=STRTOL(input+eye,&t,16);
	    input[eye+field_length]=save;
	  }else
	    sval.u.integer=STRTOL(input+eye,&t,16);
	  if(input + eye == t)
	  {
	    chars_matched[0]=eye;
	    return matches;
	  }
	  eye=t-input;
	  sval.type=T_INT;
	  sval.subtype=NUMBER_NUMBER;
	  break;
	}

	case 'o':
	{
	  char * t;

	  if(eye>=input_len)
	  {
	    chars_matched[0]=eye;
	    return matches;
	  }
	  if(field_length != -1 && eye+field_length < input_len)
	  {
	    char save=input[eye+field_length];
	    input[eye+field_length]=0; /* DANGEROUS */
	    sval.u.integer=STRTOL(input+eye,&t,8);
	    input[eye+field_length]=save;
	  }else
	    sval.u.integer=STRTOL(input+eye,&t,8);
	  if(input + eye == t)
	  {
	    chars_matched[0]=eye;
	    return matches;
	  }
	  eye=t-input;
	  sval.type=T_INT;
	  sval.subtype=NUMBER_NUMBER;
	  break;
	}

	case 'D':
	case 'i':
	{
	  char * t;

	  if(eye>=input_len)
	  {
	    chars_matched[0]=eye;
	    return matches;
	  }
	  if(field_length != -1 && eye+field_length < input_len)
	  {
	    char save=input[eye+field_length];
	    input[eye+field_length]=0; /* DANGEROUS */
	    sval.u.integer=STRTOL(input+eye,&t,0);
	    input[eye+field_length]=save;
	  }else
	    sval.u.integer=STRTOL(input+eye,&t,0);
	  if(input + eye == t)
	  {
	    chars_matched[0]=eye;
	    return matches;
	  }
	  eye=t-input;
	  sval.type=T_INT;
	  sval.subtype=NUMBER_NUMBER;
	  break;
	}

	case 'f':
	{
	  char * t;

	  if(eye>=input_len)
	  {
	    chars_matched[0]=eye;
	    return matches;
	  }
	  sval.u.float_number=STRTOD(input+eye,&t);
	  if(input + eye == t)
	  {
	    chars_matched[0]=eye;
	    return matches;
	  }
	  eye=t-input;
	  sval.type=T_FLOAT;
#ifdef __CHECKER__
	  sval.subtype=0;
#endif
	  break;
	}

	case 'F':
	  if(field_length == -1) field_length = 4;
	  if(field_length != 4 && field_length != 8)
	    error("Invalid IEEE width %d in sscanf format string.\n",
		  field_length);
	  if(eye+field_length > input_len)
	  {
	    chars_matched[0]=eye;
	    return matches;
	  }
	  sval.type=T_FLOAT;
#ifdef __CHECKER__
	  sval.subtype=0;
#endif
	  switch(field_length) {
	    case 4:
#ifdef FLOAT_IS_IEEE_BIG
	    {
	      float f;
	      ((char *)&f)[0] = *(input+eye);
	      ((char *)&f)[1] = *(input+eye+1);
	      ((char *)&f)[2] = *(input+eye+2);
	      ((char *)&f)[3] = *(input+eye+3);
	      sval.u.float_number = f;
	    }
#else
#ifdef FLOAT_IS_IEEE_LITTLE
	    {
	      float f;
	      ((char *)&f)[3] = *(input+eye);
	      ((char *)&f)[2] = *(input+eye+1);
	      ((char *)&f)[1] = *(input+eye+2);
	      ((char *)&f)[0] = *(input+eye+3);
	      sval.u.float_number = f;
	    }
#else
	    sval.u.float_number = low_parse_IEEE_float(input+eye, 4);
#endif
#endif
	    eye += 4;
	    break;
	    case 8:
#ifdef DOUBLE_IS_IEEE_BIG
	    {
	      double d;
	      ((char *)&d)[0] = *(input+eye);
	      ((char *)&d)[1] = *(input+eye+1);
	      ((char *)&d)[2] = *(input+eye+2);
	      ((char *)&d)[3] = *(input+eye+3);
	      ((char *)&d)[4] = *(input+eye+4);
	      ((char *)&d)[5] = *(input+eye+5);
	      ((char *)&d)[6] = *(input+eye+6);
	      ((char *)&d)[7] = *(input+eye+7);
	      sval.u.float_number = (float)d;
	    }
#else
#ifdef DOUBLE_IS_IEEE_LITTLE
	    {
	      double d;
	      ((char *)&d)[7] = *(input+eye);
	      ((char *)&d)[6] = *(input+eye+1);
	      ((char *)&d)[5] = *(input+eye+2);
	      ((char *)&d)[4] = *(input+eye+3);
	      ((char *)&d)[3] = *(input+eye+4);
	      ((char *)&d)[2] = *(input+eye+5);
	      ((char *)&d)[1] = *(input+eye+6);
	      ((char *)&d)[0] = *(input+eye+7);
	      sval.u.float_number = (float)d;
	    }
#else
	    sval.u.float_number = low_parse_IEEE_float(input+eye, 8);
#endif
#endif
	    eye += 8;
	    break;
	  }
	  break;

	case 's':
	  if(field_length != -1)
	  {
	    if(input_len - eye < field_length)
	    {
	      chars_matched[0]=eye;
	      return matches;
	    }

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
		  read_set((unsigned char *)match,s-match+1,set,match_len);
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
	      if(!s)
	      {
		chars_matched[0]=eye;
		return matches;
	      }
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
	      {
		chars_matched[0]=eye;
		return matches;
	      }
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
	  cnt=read_set((unsigned char *)match,cnt+1,set,match_len);

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

    if(no_assign)
    {
      free_svalue(&sval);
    }else{
      check_stack(1);
      *sp++=sval;
#ifdef PIKE_DEBUG
      sval.type=99;
#endif
    }
  }
  chars_matched[0]=eye;
  success[0]=1;
  return matches;
}

void o_sscanf(INT32 args)
{
#ifdef PIKE_DEBUG
  extern int t_flag;
#endif
  INT32 e,i;
  int x;
  long matched_chars;
  struct svalue *save_sp=sp;

  if(sp[-args].type != T_STRING)
    error("Bad argument 1 to sscanf().\n");

  if(sp[1-args].type != T_STRING)
    error("Bad argument 1 to sscanf().\n");

  i=really_low_sscanf(sp[-args].u.string->str,
		      sp[-args].u.string->len,
		      sp[1-args].u.string->str,
		      sp[1-args].u.string->len,
		      &matched_chars,
		      &x);

  if(sp-save_sp > args/2-1)
    error("Too few arguments for sscanf format.\n");

  for(x=0;x<sp-save_sp;x++)
    assign_lvalue(save_sp-args+2+x*2,save_sp+x);
  pop_n_elems(sp-save_sp +args);

#ifdef PIKE_DEBUG
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
  push_int(i);
}

void f_sscanf(INT32 args)
{
#ifdef PIKE_DEBUG
  extern int t_flag;
#endif
  INT32 e,i;
  int x;
  long matched_chars;
  struct svalue *save_sp=sp;
  struct array *a;

  check_all_args("array_sscanf",args,BIT_STRING, BIT_STRING,0);

  i=really_low_sscanf(sp[-args].u.string->str,
		      sp[-args].u.string->len,
		      sp[1-args].u.string->str,
		      sp[1-args].u.string->len,
		      &matched_chars,
		      &x);

  a=aggregate_array(sp-save_sp);
  pop_n_elems(args);
  push_array(a);
}

