/*
 * $Id: preprocessor.h,v 1.6 1999/03/01 20:59:46 grubba Exp $
 *
 * Preprocessor template.
 * Based on cpp.c 1.45
 *
 * 1999-02-27 Henrik Grubbström
 */

#ifndef SHIFT
#error Internal error: SHIFT not defined
#endif

/*
 * Definitions
 */

#if (SHIFT == 0)

#define WCHAR	p_wchar0

#define WC_ISSPACE	isspace
#define WC_ISIDCHAR	isidchar

/*
 * Function renaming
 */

#define lower_cpp		lower_cpp0
#define find_end_parenthesis	find_end_parenthesis0
#define PUSH_STRING		PUSH_STRING0
#define WC_BINARY_FINDSTRING	binary_findstring

#define calc	calc_0
#define calc1	calc1_0
#define calc2	calc2_0
#define calc3	calc3_0
#define calc4	calc4_0
#define calc5	calc5_0
#define calc6	calc6_0
#define calc7	calc7_0
#define calc7b	calc7b_0
#define calc8	calc8_0
#define calc9	calc9_0
#define calcA	calcA_0
#define calcB	calcB_0
#define calcC	calcC_0

#else /* SHIFT != 0 */

#define WC_ISSPACE(C)	(((C) < 256)?isspace(C):0)
#define WC_ISIDCHAR(C)	(((C) < 256)?isidchar(C):1)

#if (SHIFT == 1)

#define WCHAR	p_wchar1


/*
 * Function renaming
 */

#define lower_cpp		lower_cpp1
#define find_end_parenthesis	find_end_parenthesis1
#define PUSH_STRING		PUSH_STRING1
#define WC_BINARY_FINDSTRING	binary_findstring1

#define calc	calc_1
#define calc1	calc1_1
#define calc2	calc2_1
#define calc3	calc3_1
#define calc4	calc4_1
#define calc5	calc5_1
#define calc6	calc6_1
#define calc7	calc7_1
#define calc7b	calc7b_1
#define calc8	calc8_1
#define calc9	calc9_1
#define calcA	calcA_1
#define calcB	calcB_1
#define calcC	calcC_1

#else /* SHIFT != 1 */

#define WCHAR	p_wchar2

/*
 * Function renaming
 */

#define lower_cpp		lower_cpp2
#define find_end_parenthesis	find_end_parenthesis2
#define PUSH_STRING		PUSH_STRING2
#define WC_BINARY_FINDSTRING	binary_findstring2

#define calc	calc_2
#define calc1	calc1_2
#define calc2	calc2_2
#define calc3	calc3_2
#define calc4	calc4_2
#define calc5	calc5_2
#define calc6	calc6_2
#define calc7	calc7_2
#define calc7b	calc7b_2
#define calc8	calc8_2
#define calc9	calc9_2
#define calcA	calcA_2
#define calcB	calcB_2
#define calcC	calcC_2

#endif /* SHIFT == 1 */


static struct pike_string *WC_BINARY_FINDSTRING(WCHAR *str, INT32 len)
{
  struct pike_string *s;
#if (SHIFT == 1)
  s = make_shared_binary_string1(str, len);
#else /* SHIFT != 1 */
#if (SHIFT == 2)
  s = make_shared_binary_string2(str, len);
#else /* SHIFT != 2 */
#error Bad SHIFT
#endif /* SHIFT == 2 */
#endif /* SHIFT == 1 */

  if (s->refs == 1) {
    free_string(s);
    return NULL;
  }
  free_string(s);
  return(s);
}

#endif /* SHIFT == 0 */

/*
 * Generic macros
 */
#define STRCAT(STR,LEN) do {				\
  INT32 x_,len_=(LEN);					\
  char *str_=(STR);					\
  if(OUTP())						\
    string_builder_binary_strcat(&this->buf, str_, len_);	\
  else							\
    for(x_=0;x_<len_;x_++)				\
      if(str_[x_]=='\n')				\
        string_builder_putchar(&this->buf, '\n');	\
}while(0)

#define WC_STRCAT(STR,LEN) do {				\
  INT32 x_,len_=(LEN);					\
  WCHAR *str_=(STR);					\
  if(OUTP())						\
    string_builder_append(&this->buf, MKPCHARP(str_, SHIFT), len_);	\
  else							\
    for(x_=0;x_<len_;x_++)				\
      if(str_[x_]=='\n')				\
        string_builder_putchar(&this->buf, '\n');	\
}while(0)

#if 0	/* OBSOLETE */
#define CHECKWORD(X) \
 (!strncmp(X,data+pos,strlen(X)) && !WC_ISIDCHAR(data[pos+strlen(X)]))
#define WGOBBLE(X) (CHECKWORD(X) ? (pos+=strlen(X)),1 : 0)
#define GOBBLEOP(X) \
 ((!strncmp(X,data+pos,strlen(X))) ? (pos+=strlen(X)),1 : 0)
#endif /* 0 */

#define CHECKWORD2(X,LEN) \
 (!MEMCMP(X,data+pos,LEN<<SHIFT) && !WC_ISIDCHAR(data[pos+LEN]))
#define WGOBBLE2(X) (CHECKWORD2(X,NELEM(X)) ? (pos+=NELEM(X)),1 : 0)
#define GOBBLEOP2(X) \
 ((!MEMCMP(X,data+pos,sizeof(X))) ? (pos += NELEM(X)),1 : 0)

/*
 * Some prototypes
 */

static void PUSH_STRING0(p_wchar0 *, INT32, struct string_builder *);
static void PUSH_STRING1(p_wchar1 *, INT32, struct string_builder *);
static void PUSH_STRING2(p_wchar2 *, INT32, struct string_builder *);

static INT32 calc_0(struct cpp *,p_wchar0 *,INT32,INT32);
static INT32 calc_1(struct cpp *,p_wchar1 *,INT32,INT32);
static INT32 calc_2(struct cpp *,p_wchar2 *,INT32,INT32);

static INT32 calc1(struct cpp *,WCHAR *,INT32,INT32);

/*
 * Functions
 */

#if (SHIFT == 0)
static void PUSH_STRING_SHIFT(void *str, INT32 len, int shift,
			      struct string_builder *buf)
{
  switch(shift) {
  case 0:
    PUSH_STRING0((p_wchar0 *)str, len, buf);
    break;
  case 1:
    PUSH_STRING1((p_wchar1 *)str, len, buf);
    break;
  case 2:
    PUSH_STRING2((p_wchar2 *)str, len, buf);
    break;
  default:
    fatal("cpp(): Bad string shift detected in PUSH_STRING_SHIFT(): %d\n",
	  shift);
    break;
  }
}
#endif /* SHIFT == 0 */

static void PUSH_STRING(WCHAR *str,
			INT32 len,
			struct string_builder *buf)
{
  INT32 p2;
  string_builder_putchar(buf, '"');
  for(p2=0; p2<len; p2++)
  {
    switch(str[p2])
    {
    case '\n':
      string_builder_putchar(buf, '\\');
      string_builder_putchar(buf, 'n');
      break;
      
    case '\t':
      string_builder_putchar(buf, '\\');
      string_builder_putchar(buf, 't');
      break;
      
    case '\r':
      string_builder_putchar(buf, '\\');
      string_builder_putchar(buf, 'r');
      break;
      
    case '\b':
      string_builder_putchar(buf, '\\');
      string_builder_putchar(buf, 'b');
      break;
      
    case '\\':
    case '"':
      string_builder_putchar(buf, '\\');
      string_builder_putchar(buf, str[p2]);
      break;
      
    default:
      if(isprint(EXTRACT_UCHAR(str+p2)))
      {
	string_builder_putchar(buf, str[p2]);
      }
      else
      {
	int c = str[p2];
#if (SHIFT != 0)
	if (c < 256) {
#endif /* SHIFT != 0 */
	  string_builder_putchar(buf, '\\');
	  string_builder_putchar(buf, ((c>>6)&7)+'0');
	  string_builder_putchar(buf, ((c>>3)&7)+'0');
	  string_builder_putchar(buf, (c&7)+'0');
	  if(EXTRACT_UCHAR(str+p2+1)>='0' && EXTRACT_UCHAR(str+p2+1)<='7')
	  {
	    string_builder_putchar(buf, '"');
	    string_builder_putchar(buf, '"');
	  }
#if (SHIFT != 0)
	} else {
	  string_builder_putchar(buf, c);
	}
#endif /* SHIFT != 0 */
      }
      break;
    }
  }
  string_builder_putchar(buf, '"');
}

static INLINE int find_end_parenthesis(struct cpp *this,
				       WCHAR *data,
				       INT32 len,
				       INT32 pos) /* position of first " */
{
  while(1)
  {
    if(pos+1>=len)
    {
      cpp_error(this, "End of file while looking for end parenthesis.");
      return pos;
    }

    switch(data[pos++])
    {
    case '\n': PUTNL(); this->current_line++; break;
    case '\'': FIND_END_OF_CHAR();  break;
    case '"':  FIND_END_OF_STRING();  break;
    case '(':  pos=find_end_parenthesis(this, data, len, pos); break;
    case ')':  return pos;
    }
  }
}

static INT32 calcC(struct cpp *this,WCHAR *data,INT32 len,INT32 pos)
{
  FINDTOK();

  /* DUMPPOS("calcC"); */

  switch(data[pos])
  {
  case '(':
    pos=calc1(this,data,len,pos+1);
    FINDTOK();
    if(!GOBBLE(')'))
      error("Missing ')'\n");
    break;
    
  case '0':
    if(data[pos+1]=='x' || data[pos+1]=='X')
    {
      PCHARP p;
      push_int(STRTOL_PCHARP(MKPCHARP(data+pos+2, SHIFT), &p, 16));
      pos = ((WCHAR *)p.ptr) - data;
      break;
    }
    
  case '1': case '2': case '3': case '4':
  case '5': case '6': case '7': case '8': case '9':
  {
    PCHARP p1,p2;
    PCHARP p;
    double f;
    long l;
    
    p = MKPCHARP(data+pos, SHIFT);
    f = STRTOD_PCHARP(p, &p1);
    l = STRTOL_PCHARP(p, &p2, 0);
    if(COMPARE_PCHARP(p1,>,p2))
    {
      push_float(f);
      pos = ((WCHAR *)p1.ptr) - data;
    }else{
      push_int(l);
      pos = ((WCHAR *)p2.ptr) - data;
    }
    break;
  }

  case '\'':
  {
    int tmp;
    READCHAR(tmp);
    pos++;
    if(!GOBBLE('\''))
      error("Missing end quote in character constant.\n");
    push_int(tmp);
    break;
  }

  case '"':
  {
    struct string_builder s;
    init_string_builder(&s, 0);
    READSTRING(s);
    push_string(finish_string_builder(&s));
    break;
  }
  
  default:
    fprintf(stderr, "Bad char %c (%d)\n", data[pos], data[pos]);
#ifdef PIKE_DEBUG
    if(WC_ISIDCHAR(data[pos]))
      error("Syntax error in #if (should not happen)\n");
#endif

    error("Syntax error in #if.\n");
  }
  

  FINDTOK();

  while(GOBBLE('['))
  {
    /* DUMPPOS("inside calcC"); */
    pos=calc1(this,data,len,pos);
    f_index(2);

    FINDTOK();
    if(!GOBBLE(']'))
      error("Missing ']'");
  }
  D/* UMPPOS("after calcC"); */
  return pos;
}

static INT32 calcB(struct cpp *this,WCHAR *data,INT32 len,INT32 pos)
{
  /* DUMPPOS("before calcB"); */

  FINDTOK();
  switch(data[pos])
  {
    case '-': pos++; pos=calcB(this,data,len,pos); o_negate(); break;
    case '!': pos++; pos=calcB(this,data,len,pos); o_not(); break;
    case '~': pos++; pos=calcB(this,data,len,pos); o_compl(); break;
    default: pos=calcC(this,data,len,pos);
  }
  /* DUMPPOS("after calcB"); */
  return pos;
}

static INT32 calcA(struct cpp *this,WCHAR *data,INT32 len,INT32 pos)
{
  /* DUMPPOS("before calcA"); */

  pos=calcB(this,data,len,pos);
  while(1)
  {
    /* DUMPPOS("inside calcA"); */
    FINDTOK();
    switch(data[pos])
    {
      case '/':
	if(data[1]=='/' || data[1]=='*') return pos;
	pos++;
	pos=calcB(this,data,len,pos);
	o_divide();
	continue;

      case '*':
	pos++;
	pos=calcB(this,data,len,pos);
	o_multiply();
	continue;

      case '%':
	pos++;
	pos=calcB(this,data,len,pos);
	o_mod();
	continue;
    }
    break;
  }
  /* DUMPPOS("after calcA"); */
  return pos;
}

static INT32 calc9(struct cpp *this,WCHAR *data,INT32 len,INT32 pos)
{
  /* DUMPPOS("before calc9"); */

  pos=calcA(this,data,len,pos);

  while(1)
  {
    /* DUMPPOS("inside calc9"); */
    FINDTOK();
    switch(data[pos])
    {
      case '+':
	pos++;
	pos=calcA(this,data,len,pos);
	f_add(2);
	continue;

      case '-':
	pos++;
	pos=calcA(this,data,len,pos);
	o_subtract();
	continue;
    }
    break;
  }

  /* DUMPPOS("after calc9"); */
  return pos;
}

static INT32 calc8(struct cpp *this,WCHAR *data,INT32 len,INT32 pos)
{
  /* DUMPPOS("before calc8"); */

  pos=calc9(this,data,len,pos);

  while(1)
  {
    static WCHAR lsh_[] = { '<', '<' };
    static WCHAR rsh_[] = { '>', '>' };

    /* DUMPPOS("inside calc8"); */
    FINDTOK();
    if(GOBBLEOP2(lsh_))
    {
      /* DUMPPOS("Found <<"); */
      pos=calc9(this,data,len,pos);
      o_lsh();
      break;
    }

    if(GOBBLEOP2(rsh_))
    {
      /* DUMPPOS("Found >>"); */
      pos=calc9(this,data,len,pos);
      o_rsh();
      break;
    }

    break;
  }
  return pos;
}

static INT32 calc7b(struct cpp *this,WCHAR *data,INT32 len,INT32 pos)
{
  /* DUMPPOS("before calc7b"); */

  pos=calc8(this,data,len,pos);

  while(1)
  {
    /* DUMPPOS("inside calc7b"); */

    FINDTOK();
    
    switch(data[pos])
    {
      case '<':
	if(data[pos]+1 == '<') break;
	pos++;
	if(GOBBLE('='))
	{
	   pos=calc8(this,data,len,pos);
	   f_le(2);
	}else{
	   pos=calc8(this,data,len,pos);
	   f_lt(2);
	}
	continue;

      case '>':
	if(data[pos]+1 == '>') break;
	pos++;
	if(GOBBLE('='))
	{
	   pos=calc8(this,data,len,pos);
	   f_ge(2);
	}else{
	   pos=calc8(this,data,len,pos);
	   f_gt(2);
	}
	continue;
    }
    break;
  }
  return pos;
}

static INT32 calc7(struct cpp *this,WCHAR *data,INT32 len,INT32 pos)
{
  /* DUMPPOS("before calc7"); */

  pos=calc7b(this,data,len,pos);

  while(1)
  {
    static WCHAR eq_[] = { '=', '=' };
    static WCHAR ne_[] = { '!', '=' };

    /* DUMPPOS("inside calc7"); */

    FINDTOK();
    if(GOBBLEOP2(eq_))
    {
      pos=calc7b(this,data,len,pos);
      f_eq(2);
      continue;
    }

    if(GOBBLEOP2(ne_))
    {
      pos=calc7b(this,data,len,pos);
      f_ne(2);
      continue;
    }

    break;
  }
  return pos;
}

static INT32 calc6(struct cpp *this,WCHAR *data,INT32 len,INT32 pos)
{
  /* DUMPPOS("before calc6"); */

  pos=calc7(this,data,len,pos);

  FINDTOK();
  while(data[pos] == '&' && data[pos+1]!='&')
  {
    /* DUMPPOS("inside calc6"); */

    pos++;
    pos=calc7(this,data,len,pos);
    o_and();
  }
  return pos;
}

static INT32 calc5(struct cpp *this,WCHAR *data,INT32 len,INT32 pos)
{
  /* DUMPPOS("before calc5"); */

  pos=calc6(this,data,len,pos);

  FINDTOK();
  while(GOBBLE('^'))
  {
    /* DUMPPOS("inside calc5"); */

    pos=calc6(this,data,len,pos);
    o_xor();
  }
  return pos;
}

static INT32 calc4(struct cpp *this,WCHAR *data,INT32 len,INT32 pos)
{
  /* DUMPPOS("before calc4"); */

  pos=calc5(this,data,len,pos);

  FINDTOK();
  while(data[pos] == '|' && data[pos+1]!='|')
  {
    /* DUMPPOS("inside calc4"); */
    pos++;
    pos=calc5(this,data,len,pos);
    o_or();
  }
  return pos;
}

static INT32 calc3(struct cpp *this,WCHAR *data,INT32 len,INT32 pos)
{
  static WCHAR land_[] = { '&', '&' };

  /* DUMPPOS("before calc3"); */

  pos=calc4(this,data,len,pos);

  FINDTOK();
  while(GOBBLEOP2(land_))
  {
    /* DUMPPOS("inside calc3"); */

    check_destructed(sp-1);
    if(IS_ZERO(sp-1))
    {
      pos=calc4(this,data,len,pos);
      pop_stack();
    }else{
      pop_stack();
      pos=calc4(this,data,len,pos);
    }
  }
  return pos;
}

static INT32 calc2(struct cpp *this,WCHAR *data,INT32 len,INT32 pos)
{
  static WCHAR lor_[] = { '|', '|' };

  /* DUMPPOS("before calc2"); */

  pos=calc3(this,data,len,pos);

  FINDTOK();
  while(GOBBLEOP2(lor_))
  {
    /* DUMPPOS("inside calc2"); */

    check_destructed(sp-1);
    if(!IS_ZERO(sp-1))
    {
      pos=calc3(this,data,len,pos);
      pop_stack();
    }else{
      pop_stack();
      pos=calc3(this,data,len,pos);
    }
  }
  return pos;
}

static INT32 calc1(struct cpp *this, WCHAR *data, INT32 len, INT32 pos)
{
  /* DUMPPOS("before calc1"); */

  pos=calc2(this,data,len,pos);

  FINDTOK();

  if(GOBBLE('?'))
  {
    pos=calc1(this,data,len,pos);
    if(!GOBBLE(':'))
      error("Colon expected");
    pos=calc1(this,data,len,pos);

    check_destructed(sp-3);
    assign_svalue(sp-3,IS_ZERO(sp-3)?sp-1:sp-2);
    pop_n_elems(2);
  }
  return pos;
}

static int calc(struct cpp *this, WCHAR *data, INT32 len, INT32 tmp)
{
  JMP_BUF recovery;
  INT32 pos;

  /* DUMPPOS("Calculating"); */

  if (SETJMP(recovery))
  {
    pos=tmp;
    if(throw_value.type == T_ARRAY && throw_value.u.array->size)
    {
      union anything *a;
      a=low_array_get_item_ptr(throw_value.u.array, 0, T_STRING);
      if(a)
      {
	cpp_error(this, a->string->str);
      }else{
	cpp_error(this, "Nonstandard error format.");
      }
    }else{
      cpp_error(this, "Nonstandard error format.");
    }
    FIND_EOL();
    push_int(0);
  }else{
    pos=calc1(this,data,len,tmp);
    check_destructed(sp-1);
  }
  UNSETJMP(recovery);

  /* DUMPPOS("Done"); */

  return pos;
}

static INT32 lower_cpp(struct cpp *this,
		       WCHAR *data,
		       INT32 len,
		       int flags)
{
  INT32 pos, tmp, e, tmp2;
  
  for(pos=0; pos<len;)
  {
/*    fprintf(stderr,"%c",data[pos]);
    fflush(stderr); */

    switch(data[pos++])
    {
    case '\n':
      if(flags & CPP_END_AT_NEWLINE) return pos-1;

/*      fprintf(stderr,"CURRENT LINE: %d\n",this->current_line); */
      this->current_line++;
      PUTNL();
      goto do_skipwhite;

    case '\t':
    case ' ':
    case '\r':
      PUTC(' ');
      
    do_skipwhite:
      while(data[pos]==' ' || data[pos]=='\r' || data[pos]=='\t')
	pos++;
      break;

      /* Minor optimization */
      case '<':
	if(data[pos]=='<'  &&
	   data[pos+1]=='<'  &&
	   data[pos+2]=='<'  &&
	   data[pos+3]=='<'  &&
	   data[pos+4]=='<'  &&
	   data[pos+5]=='<')
	  cpp_error(this, "CVS conflict detected");
	
      case '!': case '@': case '$': case '%': case '^': case '&':
      case '*': case '(': case ')': case '-': case '=': case '+':
      case '{': case '}': case ':': case '?': case '`': case ';':
      case '>': case ',': case '.': case '~': case '[':
      case ']': case '|':
	PUTC(data[pos-1]);
      break;
      
    default:
      if(OUTP() && WC_ISIDCHAR(data[pos-1]))
      {
	struct pike_string *s=0;
	struct define *d=0;
	tmp=pos-1;
	while(WC_ISIDCHAR(data[pos])) pos++;

	if(flags & CPP_DO_IF)
	{
	  static WCHAR defined_[] = { 'd','e','f','i','n','e','d' };
	  static WCHAR efun_[] = { 'e','f','u','n' };
	  static WCHAR constant_[] = { 'c','o','n','s','t','a','n','t' };

	  if(pos-tmp == 7 && !MEMCMP(defined_, data+tmp, 7<<SHIFT))
	  {
	    d = defined_macro;
	  } else if((pos-tmp == 4 && !MEMCMP(efun_, data+tmp, 4<<SHIFT)) ||
		    (pos-tmp == 8 && !MEMCMP(constant_, data+tmp, 8<<SHIFT)))
	  {
	    d = constant_macro;
	  } else {
	    goto do_find_define;
	  }
	}else{
	do_find_define:
	  if((s=WC_BINARY_FINDSTRING(data+tmp, pos-tmp)))
	  {
	    d=find_define(s);
	  }
	}
	  
	if(d && !d->inside)
	{
	  int arg=0;
	  struct string_builder tmp;
	  struct define_argument arguments [MAX_ARGS];
	  
	  if(s) add_ref(s);
	  
	  if(d->args>=0)
	  {
	    SKIPWHITE();
	    
	    if(!GOBBLE('('))
	    {
	      cpp_error(this, "Missing '(' in macro call.");
	      break;
	    }
	    
	    for(arg=0;arg<d->args;arg++)
	    {
	      if(arg && data[pos]==',')
	      {
		pos++;
		SKIPWHITE();
	      }else{
		SKIPWHITE();
		if(data[pos]==')')
		{
		  char buffer[1024];
		  sprintf(buffer,
			  "Too few arguments to macro %950s, expected %d.",
			  d->link.s->str, d->args);
		  cpp_error(this, buffer);
		  break;
		}
	      }
	      arguments[arg].arg = MKPCHARP(data + pos, SHIFT);

	      
	      while(1)
	      {
		if(pos+1>len)
		{
		  cpp_error(this, "End of file in macro call.");
		  break;
		}
		
		switch(data[pos++])
		{
		case '\n':
		  this->current_line++;
		  PUTNL();
		default: continue;
		  
		case '"':
		  FIND_END_OF_STRING();
		  continue;
		  
		case '\'':
		  FIND_END_OF_CHAR();
		  continue;
		  
		case '(':
		  pos=find_end_parenthesis(this, data, len, pos);
		  continue;
		  
		case ')': 
		case ',': pos--;
		  break;
		}
		break;
	      }
	      arguments[arg].len = data + pos -
		((WCHAR *)(arguments[arg].arg.ptr));
	    }
	    SKIPWHITE();
	    if(!GOBBLE(')'))
	      cpp_error(this, "Missing ) in macro call.");
	  }
	  
	  if(d->args >= 0 && arg != d->args)
	    cpp_error(this, "Wrong number of arguments to macro.");
	  
	  init_string_builder(&tmp, SHIFT);
	  if(d->magic)
	  {
	    d->magic(this, d, arguments, &tmp);
	  }else{
	    string_builder_binary_strcat(&tmp, d->first->str, d->first->len);
	    for(e=0;e<d->num_parts;e++)
	    {
	      WCHAR *a;
	      INT32 l;
	      
	      if((d->parts[e].argument & DEF_ARG_MASK) < 0 || 
		 (d->parts[e].argument & DEF_ARG_MASK) >= arg)
	      {
		cpp_error(this, "Macro not expanded correctly.");
		continue;
	      }
	      
#ifdef PIKE_DEBUG
	      if (arguments[d->parts[e].argument&DEF_ARG_MASK].arg.shift !=
		  SHIFT) {
		fatal("cpp(): Bad argument shift %d != %d\n", SHIFT,
		      arguments[d->parts[e].argument&DEF_ARG_MASK].arg.shift);
	      }
#endif /* PIKE_DEBUG */
	      a = (WCHAR *)
		(arguments[d->parts[e].argument&DEF_ARG_MASK].arg.ptr);
	      l = arguments[d->parts[e].argument&DEF_ARG_MASK].len;
	      
	      if(!(d->parts[e].argument & DEF_ARG_NOPRESPACE))
		string_builder_putchar(&tmp, ' ');
	      
	      if(d->parts[e].argument & DEF_ARG_STRINGIFY)
	      {
		PUSH_STRING(a,l,&tmp);
	      }else{
		if(DEF_ARG_NOPRESPACE)
		  while(l && WC_ISSPACE(*a))
		    a++,l--;
		
		if(DEF_ARG_NOPOSTSPACE)
		  while(l && WC_ISSPACE(a[l-1]))
		    l--;

		if(d->parts[e].argument & (DEF_ARG_NOPRESPACE | DEF_ARG_NOPOSTSPACE))
		{
		  string_builder_append(&tmp, MKPCHARP(a, SHIFT), l);
		}else{
		  struct string_builder save;
		  INT32 line=this->current_line;
		  save=this->buf;
		  this->buf=tmp;
		  lower_cpp(this, a, l,
			    flags & ~(CPP_EXPECT_ENDIF | CPP_EXPECT_ELSE));
		  tmp=this->buf;
		  this->buf=save;
		  this->current_line=line;
		}
	      }
	      
	      if(!(d->parts[e].argument & DEF_ARG_NOPOSTSPACE))
		string_builder_putchar(&tmp, ' ');
	      
	      string_builder_binary_strcat(&tmp, d->parts[e].postfix->str,
					   d->parts[e].postfix->len);
	    }
	  }
	  
	  /* FIXME */
	  for(e=0;e<(long)tmp.s->len;e++)
	    if(tmp.s->str[e]=='\n')
	      tmp.s->str[e]=' ';

	  if(s) d->inside=1;
	  
	  string_builder_putchar(&tmp, 0);
	  tmp.s->len--;
#ifdef PIKE_DEBUG
	  if (tmp.s->size_shift != SHIFT) {
	    fatal("cpp(): Bad shift in #if: %d != %d\n",
		  tmp.s->size_shift, SHIFT);
	  }
#endif /* PIKE_DEBUG */
	  lower_cpp(this, (WCHAR *)tmp.s->str, tmp.s->len, 
		    flags & ~(CPP_EXPECT_ENDIF | CPP_EXPECT_ELSE));
	  
	  if(s)
	  {
	    if((d=find_define(s)))
	      d->inside=0;
	    
	    free_string(s);
	  }

	  free_string_builder(&tmp);
	  break;
	}else{
	  if(flags & CPP_DO_IF)
	  {
	    STRCAT(" 0 ", 3);
	  }else{
	    WC_STRCAT(data+tmp, pos-tmp);
	  }
	}
      }else{
	PUTC(data[pos-1]);
      }
      break;
      
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      PUTC(data[pos-1]);
      while(data[pos]>='0' && data[pos]<='9') PUTC(data[pos++]);
      break;

    case '"':
      FIXSTRING(this->buf,OUTP());
      break;

    case '\'':
      tmp=pos-1;
      FIND_END_OF_CHAR();
      WC_STRCAT(data+tmp, pos-tmp);
      break;

    case '/':
      if(data[pos]=='/')
      {
	FIND_EOL();
	break;
      }

      if(data[pos]=='*')
      {
	PUTC(' ');
	SKIPCOMMENT();
	break;
      }

      PUTC(data[pos-1]);
      break;

  case '#':
    if(GOBBLE('!'))
    {
      FIND_EOL();
      break;
    }
    SKIPSPACE();

    switch(data[pos])
    {
    case 'l':
      {
	static WCHAR line_[] = { 'l', 'i', 'n', 'e' };

	if(WGOBBLE2(line_))
	{
	  /* FIXME: Why not use SKIPSPACE()? */
	  while(data[pos]==' ' || data[pos]=='\t') pos++;
	}else{
	  goto unknown_preprocessor_directive;
	}
      /* Fall through */
      }
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
    {
      PCHARP foo = MKPCHARP(data+pos, SHIFT);
      this->current_line=STRTOL_PCHARP(foo, &foo, 10)-1;
      string_builder_putchar(&this->buf, '#');
      string_builder_append(&this->buf, MKPCHARP(data+pos, SHIFT),
			    ((WCHAR *)foo.ptr) - (data+pos));
      pos = ((WCHAR *)foo.ptr) - data;
      SKIPSPACE();
      
      if(data[pos]=='"')
      {
	struct string_builder nf;
	init_string_builder(&nf, 0);
	
	READSTRING(nf);

	free_string(this->current_file);
	this->current_file = finish_string_builder(&nf);

	string_builder_putchar(&this->buf, ' ');
	switch (this->current_file->size_shift) {
	case 0:
	  PUSH_STRING0((p_wchar0 *)this->current_file->str,
		       this->current_file->len,&this->buf);
	  break;
	case 1:
	  PUSH_STRING1((p_wchar1 *)this->current_file->str,
		       this->current_file->len,&this->buf);
	  break;
	case 2:
	  PUSH_STRING2((p_wchar2 *)this->current_file->str,
		       this->current_file->len,&this->buf);
	  break;
	default:
	  fatal("cpp(): Filename has bad shift %d\n",
		this->current_file->size_shift);
	  break;
	}
      }
      
      FIND_EOL();
      break;
    }

      case '"':
      {
	struct string_builder nf;
	init_string_builder(&nf, 0);
	
	READSTRING2(nf);
	if(OUTP())
	{
	  PUSH_STRING_SHIFT(nf.s->str, nf.s->len,nf.s->size_shift, &this->buf);
	}
	free_string_builder(&nf);
	break;
      }

      case 's':
	{
	  WCHAR string_[] = { 's', 't', 'r', 'i', 'n', 'g' };

	  if(WGOBBLE2(string_))
	  {
	    tmp2=1;
	    goto do_include;
	  }
	}
      goto unknown_preprocessor_directive;

    case 'i': /* include, if, ifdef */
      {
	static WCHAR include_[] = { 'i', 'n', 'c', 'l', 'u', 'd', 'e' };
	static WCHAR if_[] = { 'i', 'f' };
	static WCHAR ifdef_[] = { 'i', 'f', 'd', 'e', 'f' };
	static WCHAR ifndef_[] = { 'i', 'f', 'n', 'd', 'e', 'f' };

      if(WGOBBLE2(include_))
      {
	tmp2=0;
      do_include:
	{
	  struct svalue *save_sp=sp;
	  SKIPSPACE();
	  
	  check_stack(3);
	  
	  switch(data[pos++])
	  {
	    case '"':
	      {
		struct string_builder nf;
		init_string_builder(&nf, 0);
		pos--;
		READSTRING(nf);
		push_string(finish_string_builder(&nf));
		ref_push_string(this->current_file);
		push_int(1);
		break;
	      }

	    case '<':
	      {
		INT32 tmp=pos;
		while(data[pos]!='>')
		{
		  if(data[pos]=='\n')
		  {
		    cpp_error(this, "Expecting '>' in include.");
		    break;
		  }
		      
		  pos++;
		}
#if (SHIFT == 0)
		push_string(make_shared_binary_string(data+tmp, pos-tmp));
#else /* SHIFT != 0 */
#if (SHIFT == 1)
		push_string(make_shared_binary_string1(data+tmp, pos-tmp));
#else /* SHIFT != 1 */
		push_string(make_shared_binary_string2(data+tmp, pos-tmp));
#endif /* SHIFT == 1 */
#endif /* SHIFT == 0 */
		ref_push_string(this->current_file);
		pos++;
		push_int(0);
		break;
	      }

	    default:
	      cpp_error(this, "Expected file to include.");
	      break;
	    }

	  if(sp==save_sp) break;

	  if(OUTP())
	  {
	    struct pike_string *new_file;

	    SAFE_APPLY_MASTER("handle_include",3);
	  
	    if(sp[-1].type != T_STRING)
	    {
	      cpp_error(this, "Couldn't include file.");
	      pop_n_elems(sp-save_sp);
	      break;
	    }
	    
	    new_file=sp[-1].u.string;
	    
	    assign_svalue_no_free(sp,sp-1);
	    sp++;
	    
	    SAFE_APPLY_MASTER("read_include",1);
	    
	    if(sp[-1].type != T_STRING)
	    {
	      cpp_error(this, "Couldn't read include file.");
	      pop_n_elems(sp-save_sp);
	      break;
	    }
	    
	    {
	      char buffer[47];
	      struct pike_string *save_current_file;
	      INT32 save_current_line;

	      save_current_file=this->current_file;
	      save_current_line=this->current_line;
	      copy_shared_string(this->current_file,new_file);
	      this->current_line=1;
	      
	      string_builder_binary_strcat(&this->buf, "# 1 ", 4);
	      PUSH_STRING_SHIFT(new_file->str, new_file->len,
				new_file->size_shift, &this->buf);
	      string_builder_putchar(&this->buf, '\n');
	      if(tmp2)
	      {
		struct pike_string *str = sp[-1].u.string;
		PUSH_STRING_SHIFT(str->str, str->len, str->size_shift,
				  &this->buf);
	      }else{
		low_cpp(this,
			sp[-1].u.string->str,
			sp[-1].u.string->len,
			sp[-1].u.string->size_shift,
			flags&~(CPP_EXPECT_ENDIF | CPP_EXPECT_ELSE));
	      }
	      
	      free_string(this->current_file);
	      this->current_file=save_current_file;
	      this->current_line=save_current_line;
	      
	      sprintf(buffer,"# %d ",this->current_line);
	      string_builder_binary_strcat(&this->buf, buffer, strlen(buffer));
	      PUSH_STRING_SHIFT(this->current_file->str,
				this->current_file->len,
				this->current_file->size_shift,
				&this->buf);
	      string_builder_putchar(&this->buf, '\n');
	    }
	  }

	  pop_n_elems(sp-save_sp);
	  
	  break;
	}
      }

      if(WGOBBLE2(if_))
      {
	struct string_builder save,tmp;
	INT32 nflags=CPP_EXPECT_ELSE | CPP_EXPECT_ENDIF;
#ifdef PIKE_DEBUG
	int skip;
#endif /* PIKE_DEBUG */
	
	if(!OUTP())
	  nflags|=CPP_REALLY_NO_OUTPUT;
	
	save=this->buf;
	init_string_builder(&this->buf, SHIFT);
#ifdef PIKE_DEBUG
	/* DUMPPOS("#if before lower_cpp()"); */
	skip = lower_cpp(this, data+pos, len-pos, CPP_END_AT_NEWLINE|CPP_DO_IF);
	/* fprintf(stderr, "shift:%d, pos:%d, len:%d, skip:%d\n",
	   SHIFT, pos, len, skip); */

	pos += skip;
	/* DUMPPOS("#if after lower_cpp()"); */
	/* fprintf(stderr, "tmp (%d:%d): \"",
	   this->buf.s->len, this->buf.s->size_shift); */
	/* fflush(stderr); */
	/* write(2, this->buf.s->str,
	   this->buf.s->len << this->buf.s->size_shift); */
	/* fprintf(stderr, "\"\n"); */
	/* fflush(stderr); */
#else /* !PIKE_DEBUG */
	pos+=lower_cpp(this, data+pos, len-pos, CPP_END_AT_NEWLINE|CPP_DO_IF);
#endif /* PIKE_DEBUG */
	tmp=this->buf;
	this->buf=save;
	
	string_builder_putchar(&tmp, 0);
	tmp.s->len--;
	
	switch(tmp.s->size_shift) {
	case 0:
	  calc_0(this, (p_wchar0 *)tmp.s->str, tmp.s->len, 0);
	  break;
	case 1:
	  calc_1(this, (p_wchar1 *)tmp.s->str, tmp.s->len, 0);
	  break;
	case 2:
	  calc_2(this, (p_wchar2 *)tmp.s->str, tmp.s->len, 0);
	  break;
	default:
	  fatal("cpp(): Bad shift: %d\n", tmp.s->size_shift);
	  break;
	}
	free_string_builder(&tmp);
	if(IS_ZERO(sp-1)) nflags|=CPP_NO_OUTPUT;
	pop_stack();
	pos += lower_cpp(this, data+pos, len-pos, nflags);
	break;
      }

      if(WGOBBLE2(ifdef_))
	{
	  INT32 namestart,nflags;
	  struct pike_string *s;
	  SKIPSPACE();

	  if(!WC_ISIDCHAR(data[pos]))
	    cpp_error(this, "#ifdef what?\n");

	  namestart=pos;
	  while(WC_ISIDCHAR(data[pos])) pos++;
	  nflags=CPP_EXPECT_ELSE | CPP_EXPECT_ENDIF | CPP_NO_OUTPUT;

	  if(!OUTP())
	    nflags|=CPP_REALLY_NO_OUTPUT;

	  if((s=WC_BINARY_FINDSTRING(data+namestart, pos-namestart)))
	    if(find_define(s))
	      nflags&=~CPP_NO_OUTPUT;

	  pos += lower_cpp(this, data+pos, len-pos, nflags);
	  break;
	}

      if(WGOBBLE2(ifndef_))
	{
	  INT32 namestart,nflags;
	  struct pike_string *s;
	  SKIPSPACE();

	  if(!WC_ISIDCHAR(data[pos]))
	    cpp_error(this, "#ifndef what?");

	  namestart=pos;
	  while(WC_ISIDCHAR(data[pos])) pos++;
	  nflags=CPP_EXPECT_ELSE | CPP_EXPECT_ENDIF;

	  if(!OUTP())
	    nflags|=CPP_REALLY_NO_OUTPUT;

	  if((s = WC_BINARY_FINDSTRING(data+namestart, pos-namestart)))
	    if(find_define(s))
	      nflags|=CPP_NO_OUTPUT;

	  pos += lower_cpp(this, data+pos, len-pos, nflags);
	  break;
	}

      goto unknown_preprocessor_directive;
      }
    case 'e': /* endif, else, elif, error */
      {
	static WCHAR endif_[] = { 'e', 'n', 'd', 'i', 'f' };
	static WCHAR else_[] = { 'e', 'l', 's', 'e' };
	static WCHAR elseif_[] = { 'e', 'l', 's', 'e', 'i', 'f' };
	static WCHAR elif_[] = { 'e', 'l', 'i', 'f' };
	static WCHAR error_[] = { 'e', 'r', 'r', 'o', 'r' };

      if(WGOBBLE2(endif_))
      {
	if(!(flags & CPP_EXPECT_ENDIF))
	  cpp_error(this, "Unmatched #endif");

	return pos;
      }

      if(WGOBBLE2(else_))
	{
	  if(!(flags & CPP_EXPECT_ELSE))
	    cpp_error(this, "Unmatched #else");

	  flags&=~CPP_EXPECT_ELSE;
	  flags|=CPP_EXPECT_ENDIF;

	  if(flags & CPP_NO_OUTPUT)
	    flags&=~CPP_NO_OUTPUT;
	  else
	    flags|=CPP_NO_OUTPUT;

	  break;
	}

      if(WGOBBLE2(elif_) || WGOBBLE2(elseif_))
      {
	if(!(flags & CPP_EXPECT_ELSE))
	  cpp_error(this, "Unmatched #elif");
	
	flags|=CPP_EXPECT_ENDIF;
	
	if(flags & CPP_NO_OUTPUT)
	{
	  struct string_builder save,tmp;
	  save=this->buf;
	  init_string_builder(&this->buf, 0);
	  pos += lower_cpp(this, data+pos, len-pos,
			   CPP_END_AT_NEWLINE|CPP_DO_IF);
	  tmp=this->buf;
	  this->buf=save;
	  
	  string_builder_putchar(&tmp, 0);
	  tmp.s->len--;
	  
	  switch(tmp.s->size_shift) {
	  case 0:
	    calc_0(this, (p_wchar0 *)tmp.s->str, tmp.s->len,0);
	    break;
	  case 1:
	    calc_1(this, (p_wchar1 *)tmp.s->str, tmp.s->len,0);
	    break;
	  case 2:
	    calc_2(this, (p_wchar2 *)tmp.s->str, tmp.s->len,0);
	    break;
	  default:
	    fatal("cpp(): Bad shift: %d\n", tmp.s->size_shift);
	    break;
	  }
	  free_string_builder(&tmp);
	  if(!IS_ZERO(sp-1)) flags&=~CPP_NO_OUTPUT;
	  pop_stack();
	}else{
	  FIND_EOL();
	  flags|= CPP_NO_OUTPUT | CPP_REALLY_NO_OUTPUT;
	}
	break;
      }

      if(WGOBBLE2(error_))
	{
          INT32 foo;
          SKIPSPACE();
          foo=pos;
          FIND_EOL();
	  pos++;
	  if(OUTP())
	  {
#if (SHIFT == 0)
	    push_string(make_shared_binary_string(data+foo, pos-foo));
#else /* SHIFT != 0 */
#if (SHIFT == 1)
	    push_string(make_shared_binary_string1(data+foo, pos-foo));
#else /* SHIFT != 1 */
	    push_string(make_shared_binary_string2(data+foo, pos-foo));
#endif /* SHIFT == 1 */
#endif /* SHIFT == 0 */
	    cpp_error(this, sp[-1].u.string->str);
	  }
	  break;
	}
      }
      goto unknown_preprocessor_directive;

    case 'd': /* define */
      {
	static WCHAR define_[] = { 'd', 'e', 'f', 'i', 'n', 'e' };

      if(WGOBBLE2(define_))
	{
	  struct string_builder str;
	  INT32 namestart, tmp3, nameend, argno=-1;
	  struct define *def;
	  struct svalue *partbase,*argbase=sp;

	  SKIPSPACE();

	  namestart=pos;
	  if(!WC_ISIDCHAR(data[pos])) {
	    cpp_error(this, "Define what?");
	    /* Scan to EOL */
	    while (data[pos] && (data[pos] != '\n')) {
	      pos++;
	    }
	    break;
	  }

	  while(WC_ISIDCHAR(data[pos])) pos++;
	  nameend=pos;

	  if(GOBBLE('('))
	    {
	      argno=0;
	      SKIPWHITE();

	      while(data[pos]!=')')
		{
		  INT32 tmp2;
		  if(argno)
		    {
		      if(!GOBBLE(','))
			cpp_error(this,
				  "Expecting comma in macro definition.");
		      SKIPWHITE();
		    }
		  tmp2=pos;

		  if(!WC_ISIDCHAR(data[pos]))
		  {
		    cpp_error(this, "Expected argument for macro.");
		    break;
		  }

		  while(WC_ISIDCHAR(data[pos])) pos++;
		  check_stack(1);

#if (SHIFT == 0)
		  push_string(make_shared_binary_string(data+tmp2, pos-tmp2));
#else /* SHIFT != 0 */
#if (SHIFT == 1)
		  push_string(make_shared_binary_string1(data+tmp2, pos-tmp2));
#else /* SHIFT != 1 */
		  push_string(make_shared_binary_string2(data+tmp2, pos-tmp2));
#endif /* SHIFT == 1 */
#endif /* SHIFT == 0 */

		  SKIPWHITE();
		  argno++;
		  if(argno>=MAX_ARGS)
		  {
		    cpp_error(this, "Too many arguments in macro definition.");
		    pop_stack();
		    argno--;
		  }
		}

	      if(!GOBBLE(')'))
		cpp_error(this, "Missing ) in macro definition.");
	    }

	  SKIPSPACE();

	  partbase=sp;
	  init_string_builder(&str, 0);
	  
	  while(1)
	  {
	    INT32 extra=0;

/*	    fprintf(stderr,"%c",data[pos]);
	    fflush(stderr); */

	    switch(data[pos++])
	    {
	    case '/':
	      if(data[pos]=='/')
	      {
		string_builder_putchar(&str, ' ');
		FIND_EOL();
		continue;
	      }
	      
	      if(data[pos]=='*')
	      {
		PUTC(' ');
		SKIPCOMMENT();
		continue;
	      }
	      
	      string_builder_putchar(&str, '/');
	      continue;
	      
	    case '0': case '1': case '2': case '3':	case '4':
	    case '5': case '6': case '7': case '8':	case '9':
	      string_builder_putchar(&str, data[pos-1]);
	      while(data[pos]>='0' && data[pos]<='9')
		string_builder_putchar(&str, data[pos++]);
	      continue;
	      
	    case '#':
	      if(GOBBLE('#'))
	      {
		extra=DEF_ARG_NOPRESPACE;
		/* FIXME: Wide strings? */
		while(str.s->len && isspace(((unsigned char *)str.s->str)[str.s->len-1]))
		  str.s->len--;
		if(!str.s->len && sp-partbase>1)
		{
#ifdef PIKE_DEBUG
		  if(sp[-1].type != T_INT)
		    fatal("Internal error in CPP\n");
#endif
		  sp[-1].u.integer|=DEF_ARG_NOPOSTSPACE;
		}
	      }else{
		extra=DEF_ARG_STRINGIFY;
	      }
	      SKIPSPACE();
	      pos++;
	      /* fall through */
	      
	    default:
	      if(WC_ISIDCHAR(data[pos-1]))
	      {
		struct pike_string *s;
		tmp3=pos-1;
		while(WC_ISIDCHAR(data[pos])) pos++;
		if(argno>0)
		{
		  if((s=WC_BINARY_FINDSTRING(data+tmp3, pos-tmp3)))
		  {
		    for(e=0;e<argno;e++)
		    {
		      if(argbase[e].u.string == s)
		      {
			check_stack(2);
			push_string(finish_string_builder(&str));
			init_string_builder(&str, 0);
			push_int(e | extra);
			extra=0;
			break;
		      }
		    }
		    if(e!=argno) continue;
		  }
		}
		string_builder_append(&str, MKPCHARP(data+tmp3, SHIFT),
				      pos-tmp3);
	      }else{
		string_builder_putchar(&str, data[pos-1]);
	      }
	      extra=0;
	      continue;
	      
	    case '"':
	      FIXSTRING(str, 1);
	      continue;
	      
	    case '\'':
	      tmp3=pos-1;
	      FIND_END_OF_CHAR();
	      string_builder_append(&str, MKPCHARP(data+tmp3, SHIFT),
				    pos-tmp3);
	      continue;
	      
	    case '\\':
	      if(GOBBLE('\n'))
	      { 
		this->current_line++;
		PUTNL();
	      }
	      continue;
	      
	    case '\n':
		PUTNL();
		this->current_line++;
	    case 0:
		break;
	    }
	    push_string(finish_string_builder(&str));
	    break;
	  }

	  if(OUTP())
	  {

#if (SHIFT == 0)
	    def=alloc_empty_define(
		  make_shared_binary_string(data+namestart,
					    nameend-namestart),
		  (sp-partbase)/2);
#else /* SHIFT != 0 */
#if (SHIFT == 1)
	    def=alloc_empty_define(
		  make_shared_binary_string1(data+namestart,
					     nameend-namestart),
		  (sp-partbase)/2);
#else /* SHIFT != 1 */
	    def=alloc_empty_define(
		  make_shared_binary_string2(data+namestart,
					     nameend-namestart),
		  (sp-partbase)/2);
#endif /* SHIFT == 1 */
#endif /* SHIFT == 0 */
	    copy_shared_string(def->first, partbase->u.string);
	    def->args=argno;
	    
	    for(e=0;e<def->num_parts;e++)
	    {
#if 1
	      if(partbase[e*2+1].type != T_INT)
		fatal("Cpp internal error, expected integer!\n");
	      
	      if(partbase[e*2+2].type != T_STRING)
		fatal("Cpp internal error, expected string!\n");
#endif
	      def->parts[e].argument=partbase[e*2+1].u.integer;
	      copy_shared_string(def->parts[e].postfix,
				 partbase[e*2+2].u.string);
	    }
	    
#ifdef PIKE_DEBUG
	    if(def->num_parts==1 &&
	       (def->parts[0].argument & DEF_ARG_MASK) > MAX_ARGS)
	      fatal("Internal error in define\n");
#endif	  
	    
	    this->defines=hash_insert(this->defines, & def->link);
	    
	  }
	  pop_n_elems(sp-argbase);
	  break;
	}
      
      goto unknown_preprocessor_directive;
      }
    case 'u': /* undefine */
      {
	static WCHAR undef_[] = { 'u', 'n', 'd', 'e', 'f' };
	static WCHAR undefine_[] = { 'u', 'n', 'd', 'e', 'f', 'i', 'n', 'e' };

	/* NOTE: Reuses undefine_ for undef_ */
      if(WGOBBLE2(undefine_) || WGOBBLE2(undef_))
	{
	  INT32 tmp;
	  struct pike_string *s;

	  SKIPSPACE();

	  tmp=pos;
	  if(!WC_ISIDCHAR(data[pos]))
	    cpp_error(this, "Undefine what?");

	  while(WC_ISIDCHAR(data[pos])) pos++;

	  /* #undef some_long_identifier
	   *        ^                   ^
	   *        tmp               pos
	   */

	  if(OUTP())
	  {
	    if((s = WC_BINARY_FINDSTRING(data+tmp, pos-tmp)))
	      undefine(this,s);
	  }

	  break;
	}

      goto unknown_preprocessor_directive;
      }
    case 'c': /* charset */
      {
	static WCHAR charset_[] = { 'c', 'h', 'a', 'r', 's', 'e', 't' };

      if (WGOBBLE2(charset_)) {
	INT32 p;
	struct pike_string *s;

	if (flags & (CPP_EXPECT_ENDIF | CPP_EXPECT_ELSE)) {
	  /* Only allowed at the top-level */
	  cpp_error(this, "#charset directive inside #if/#endif.\n");
	  /* Skip to end of line */
	  while (data[pos] && data[pos] != '\n') {
	    pos++;
	  }
	  break;
	}

	/* FIXME: Should probably only be allowed in 8bit strings.
	 */

	SKIPSPACE();

	p = pos;
	while(data[pos] && !isspace(data[pos])) {
	  pos++;
	}

	if (pos != p) {
	  /* The rest of the string. */
	  s = begin_shared_string(len - pos);
	  MEMCPY(s->str, data + pos, len - pos);
	  push_string(end_shared_string(s));

	  /* The charset name */
	  s = begin_shared_string(pos - p);
	  MEMCPY(s->str, data + p, pos - p);
	  push_string(end_shared_string(s));

	  SAFE_APPLY_MASTER("decode_charset", 2);

	  if (sp[-1].type != T_STRING) {
	    pop_stack();
	    cpp_error(this, "Unknown charset.");
	  }

	  low_cpp(this, sp[-1].u.string->str, sp[-1].u.string->len,
		  sp[-1].u.string->size_shift, flags);
	  pop_stack();

	  /* FIXME: Is this the correct thing to return? */
	  return len;
	} else {
	  cpp_error(this, "What charset?");
	}
	
	break;
      }
      goto unknown_preprocessor_directive;
      }
    case 'p': /* pragma */
      {
	static WCHAR pragma_[] = { 'p', 'r', 'a', 'g', 'm', 'a' };

	if(WGOBBLE2(pragma_))
	{
	  if(OUTP())
	    STRCAT("#pragma", 7);
	  else
	    FIND_EOL();
	  break;
	}
      }
    default:
    unknown_preprocessor_directive:
      {
	WCHAR buffer[180];
	int i;
	for(i=0; i<180-1; i++)
	{
	  if(!WC_ISIDCHAR(data[pos])) break;
	  buffer[i]=data[pos++];
	}
	buffer[i]=0;
	
	cpp_error(this, "Unknown preprocessor directive.");
      }
    }
    }
  }

  if(flags & CPP_EXPECT_ENDIF)
    error("End of file while searching for #endif\n");

  return pos;
}

/*
 * Clear the defines for the next pass
 */

#undef WCHAR
#undef WC_ISSPACE
#undef WC_ISIDCHAR
#undef WC_BINARY_FINDSTRING
#undef PUSH_STRING
#undef STRCAT
#undef WC_STRCAT

#undef lower_cpp
#undef find_end_parenthesis

#undef calc	calc_2
#undef calc1	calc1_2
#undef calc2	calc2_2
#undef calc3	calc3_2
#undef calc4	calc4_2
#undef calc5	calc5_2
#undef calc6	calc6_2
#undef calc7	calc7_2
#undef calc7b	calc7b_2
#undef calc8	calc8_2
#undef calc9	calc9_2
#undef calcA	calcA_2
#undef calcB	calcB_2
#undef calcC	calcC_2

