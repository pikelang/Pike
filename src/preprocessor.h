/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: preprocessor.h,v 1.81 2004/11/14 18:35:24 mast Exp $
*/

/*
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

#define gobble_identifier	gobble_identifier0
#define lower_cpp		lower_cpp0
#define parse_esc_seq		parse_esc_seq0
#define find_end_parenthesis	find_end_parenthesis0
#define find_end_brace		find_end_brace0
#define PUSH_STRING		PUSH_STRING0
#define WC_BINARY_FINDSTRING(X, Y)	binary_findstring((char *)X, Y)
#define MAKE_BINARY_STRING	make_shared_binary_string0

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

#define WC_ISSPACE	WIDE_ISSPACE
#define WC_ISIDCHAR	WIDE_ISIDCHAR

#if (SHIFT == 1)

#define WCHAR	p_wchar1

/*
 * Function renaming
 */

#define gobble_identifier	gobble_identifier1
#define lower_cpp		lower_cpp1
#define parse_esc_seq		parse_esc_seq1
#define find_end_parenthesis	find_end_parenthesis1
#define find_end_brace		find_end_brace1
#define PUSH_STRING		PUSH_STRING1
#define WC_BINARY_FINDSTRING	binary_findstring1
#define MAKE_BINARY_STRING	make_shared_binary_string1

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

#define gobble_identifier	gobble_identifier2
#define lower_cpp		lower_cpp2
#define parse_esc_seq		parse_esc_seq2
#define find_end_parenthesis	find_end_parenthesis2
#define find_end_brace		find_end_brace2
#define PUSH_STRING		PUSH_STRING2
#define WC_BINARY_FINDSTRING	binary_findstring2
#define MAKE_BINARY_STRING	make_shared_binary_string2

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

static struct pike_string *WC_BINARY_FINDSTRING(WCHAR *str, ptrdiff_t len)
{
  struct pike_string *s = MAKE_BINARY_STRING(str, len);

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

#define STRCAT(X,Y) _STRCAT(X,Y,flags,this)
#define CHECKWORD2(X,LEN) \
 (!MEMCMP(X,data+pos,LEN<<SHIFT) && !WC_ISIDCHAR(data[pos+LEN]))
#define WGOBBLE2(X) (CHECKWORD2(X,NELEM(X)) ? (pos+=NELEM(X)),1 : 0)
#define GOBBLEOP2(X) \
 ((!MEMCMP(X,data+pos,sizeof(X))) ? (pos += NELEM(X)),1 : 0)

/*
 * Some prototypes
 */

static void PUSH_STRING0(p_wchar0 *, ptrdiff_t, struct string_builder *);
static void PUSH_STRING1(p_wchar1 *, ptrdiff_t, struct string_builder *);
static void PUSH_STRING2(p_wchar2 *, ptrdiff_t, struct string_builder *);

static ptrdiff_t calc_0(struct cpp *,p_wchar0 *,ptrdiff_t,ptrdiff_t, int);
static ptrdiff_t calc_1(struct cpp *,p_wchar1 *,ptrdiff_t,ptrdiff_t, int);
static ptrdiff_t calc_2(struct cpp *,p_wchar2 *,ptrdiff_t,ptrdiff_t, int);

static ptrdiff_t calc1(struct cpp *,WCHAR *,ptrdiff_t,ptrdiff_t, int);

/*
 * Functions
 */

static struct pike_string *gobble_identifier (WCHAR *data, ptrdiff_t *pos,
					      struct cpp *this)
{
  ptrdiff_t p = *pos;
  struct string_builder sb;
  init_string_builder (&sb, SHIFT);

  while (1) {
    ptrdiff_t start = p;
    while (WC_ISIDCHAR (data[p])) p++;
    if (p != start)
      PIKE_XCONCAT (string_builder_binary_strcat, SHIFT) (
	&sb, data + start, p - start);

    if (data[p] != '\\') goto past_identifier;

    switch (data[p + 1]) {
      case '\r':
	if (data[p + 2] != '\n')
	  goto past_identifier;
	p++;
	/* Fall through */
      case '\n':
	this->current_line++;
	PUTNL();
	p += 2;
	break;

      case 'u':
      case 'U': {
	/* Note: Code dup in parse_esc_seq in lexer.h. */
	unsigned INT32 c = 0;
	ptrdiff_t stop, q;
	if (data[p + 1] == 'u') {
	  if (data[p + 2] == 'u')
	    /* A quoted \u escape means we got "\uxxxx" dequoted here,
	     * and that can't be part of an identifier. */
	    goto past_identifier;
	  stop = p + 6;
	}
	else {
	  if (data[p + 2] == 'U')
	    goto past_identifier;
	  stop = p + 10;
	}
	for (q = p + 2; q < stop; q++)
	  switch (data[q]) {
	    case '0': case '1': case '2': case '3': case '4':
	    case '5': case '6': case '7': case '8': case '9':
	      c = 16 * c + data[q] - '0';
	      break;
	    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
	      c = 16 * c + data[q] - 'a' + 10;
	      break;
	    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
	      c = 16 * c + data[q] - 'A' + 10;
	      break;
	    default:
	      cpp_error_sprintf (this, "Too few hex digits in \\%c escape.",
				 data[p + 1]);
	      goto past_identifier;
	  }
	if (!WIDE_ISIDCHAR (c)) goto past_identifier;
	string_builder_putchar (&sb, c);
	p = q;
	break;
      }

      default:
	goto past_identifier;
    }
  }

past_identifier:
  if (p != *pos) {
    *pos = p;
    return finish_string_builder (&sb);
  }
  else {
    free_string_builder (&sb);
    return NULL;
  }
}

/* Gobble an identifier at the current position. */
#define GOBBLE_IDENTIFIER()						\
  (WC_ISIDCHAR (data[pos]) || data[pos] == '\\' ?			\
   dmalloc_touch (struct pike_string *, gobble_identifier (data, &pos, this)) : NULL)

#if (SHIFT == 0)
void _STRCAT(char *str, int len, int flags,struct cpp *this)
{
     ptrdiff_t x;
     if(OUTP())
       string_builder_binary_strcat(&this->buf, str, len);
     else
       for(x=0;x<len;x++)
	 if(str[x]=='\n')
	   string_builder_putchar(&this->buf, '\n');
}

static void PUSH_STRING_SHIFT(void *str, ptrdiff_t len, int shift,
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
#ifdef PIKE_DEBUG
  default:
    Pike_fatal("cpp(): Bad string shift detected in PUSH_STRING_SHIFT(): %d\n",
	  shift);
    break;
#endif
  }
}
#endif /* SHIFT == 0 */

static void PUSH_STRING(WCHAR *str,
			ptrdiff_t len,
			struct string_builder *buf)
{
  ptrdiff_t p2;
  string_builder_putchar(buf, '"');
  for(p2=0; p2<len; p2++)
  {
    unsigned int c = str[p2];

    switch(c)
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
      string_builder_putchar(buf, c);
      break;

    default:
#if (SHIFT != 0)
      if (c < 256) {
#endif /* SHIFT != 0 */
	if(isprint(c)) {
	  string_builder_putchar(buf, c);
	}
	else
	{
	  string_builder_putchar(buf, '\\');
	  string_builder_putchar(buf, ((c>>6)&7)+'0');
	  string_builder_putchar(buf, ((c>>3)&7)+'0');
	  string_builder_putchar(buf, (c&7)+'0');
	  if((str[p2+1] >= '0') && (str[p2+1] <= '9'))
	  {
	    string_builder_putchar(buf, '"');
	    string_builder_putchar(buf, '"');
	  }
	}
#if (SHIFT != 0)
      } else if (c == 0xfeff) {
	/* Keep it safe from filter_bom()... */
	string_builder_strcat(buf, "\\xfeff\"\"");
      } else {
	/* No need to encode these */
	string_builder_putchar(buf, c);
      }
#endif /* SHIFT != 0 */
      break;
    }
  }
  string_builder_putchar(buf, '"');
}

static INLINE ptrdiff_t find_end_parenthesis(struct cpp *this,
					     WCHAR *data,
					     ptrdiff_t len,
					     ptrdiff_t pos)
/* pos is after the open paren. Returns the position after the close paren. */
{
  INT32 start_line = this->current_line;
  while(1)
  {
    if(pos+1>=len)
    {
      INT32 save_line = this->current_line;
      this->current_line = start_line;
      cpp_error(this, "End of file while looking for end parenthesis.");
      this->current_line = save_line;
      return pos;
    }

    switch(data[pos++])
    {
    case '\n': PUTNL(); this->current_line++; break;
    case '\'': FIND_END_OF_CHAR();  break;
    case '"':  FIND_END_OF_STRING();  break;
    case '(':  pos=find_end_parenthesis(this, data, len, pos); break;
    case ')':  return pos;
    case '/':
      if (data[pos] == '*') {
	pos++;
	SKIPCOMMENT();
      } else if (data[pos] == '/') {
	pos++;
	FIND_EOL();
      }
    }
  }
}

static INLINE ptrdiff_t find_end_brace(struct cpp *this,
				       WCHAR *data,
				       ptrdiff_t len,
				       ptrdiff_t pos)
/* pos is after the open brace. Returns the position after the close brace. */
{
  INT32 start_line = this->current_line;
  while(1)
  {
    if(pos+1>=len)
    {
      INT32 save_line = this->current_line;
      this->current_line = start_line;
      cpp_error(this, "End of file while looking for end brace.");
      this->current_line = save_line;
      return pos;
    }

    switch(data[pos++])
    {
    case '\n': PUTNL(); this->current_line++; break;
    case '\'': FIND_END_OF_CHAR();  break;
    case '"':  FIND_END_OF_STRING();  break;
    case '{':  pos=find_end_brace(this, data, len, pos); break;
    case '}':  return pos;
    case '/':
      if (data[pos] == '*') {
	SKIPCOMMENT();
      } else if (data[pos] == '/') {
	FIND_EOL();
      }
    }
  }
}

static ptrdiff_t calcC(struct cpp *this, WCHAR *data, ptrdiff_t len,
		       ptrdiff_t pos, int flags)
{
  SKIPWHITE();

  CALC_DUMPPOS("calcC");

  switch(data[pos])
  {
  case '(':
    pos=calc1(this,data,len,pos+1,flags);
    SKIPWHITE();
    if(!GOBBLE(')'))
      cpp_error(this, "Missing ')'");
    break;
    
  case '0':
    if(data[pos+1]=='x' || data[pos+1]=='X')
    {
      PCHARP p;
      long val = STRTOL_PCHARP(MKPCHARP(data+pos+2, SHIFT), &p, 16);
      if(OUTP())
	push_int(val);
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
      if(OUTP())
	push_float(DO_NOT_WARN((FLOAT_TYPE)f));
      pos = ((WCHAR *)p1.ptr) - data;
    }else{
      if(OUTP())
	push_int(l);
      pos = ((WCHAR *)p2.ptr) - data;
    }
    break;
  }

  case '\'':
  {
    int tmp = data[++pos];
    if (tmp == '\\') READCHAR(tmp);
    pos++;
    if(!GOBBLE('\''))
      cpp_error(this, "Missing end quote in character constant.");
    if(OUTP())
      push_int(tmp);
    break;
  }

  case '"':
  {
    struct string_builder s;
    init_string_builder(&s, 0);
    READSTRING(s);
    if(OUTP())
      push_string(finish_string_builder(&s));
    else
      free_string_builder(&s);
    break;
  }
  
  default: {
    struct pike_string *func_name = GOBBLE_IDENTIFIER();
    if (func_name) {
      /* NOTE: defined() can not be handled here, 
       *       since the argument must not be expanded.
       */
      void (*cpp_func)(struct cpp *this, INT32 args) = NULL;

      if (func_name == efun_str || func_name == constant_str)
      {
	cpp_func = cpp_func_constant;
      }
      
      SKIPWHITE();

      if (cpp_func)
      {
	int start, end;
	int arg = 0;
	int start_line;

#ifdef PIKE_DEBUG
	if (func_name->size_shift)
	  Pike_fatal ("Didn't expect wide string name for meta function.\n");
#endif

	if(!GOBBLE('('))
	{
	  if(OUTP())
	    push_int(0);
	  break;
	}

	start_line = this->current_line;

	SKIPWHITE();

	start = end = pos;
	while (data[pos] != ')') {
	  switch(data[pos++]) {
	  case '(':
	    pos = find_end_parenthesis(this, data, len, pos);
	    break;
	  case ',':
	    push_string(MAKE_BINARY_STRING(data+start, end-start));
	    arg++;
	    start = pos;
	    break;
	  case '\0':
	    if (pos > len) {
	      int old_line = this->current_line;
	      this->current_line = start_line;
	      cpp_error_sprintf(this, "Missing ) in the meta function %s().",
				func_name->str);
	      this->current_line = old_line;
	      return pos-1;
	    }
	    /* FALL_THROUGH */
	  default:
	    if (WC_ISSPACE(data[pos-1])) {
	      SKIPWHITE();
	      continue;
	    }
	    break;
	  }
	  end = pos;
	}
	
	if (start != end) {
	  push_string(MAKE_BINARY_STRING(data+start, end-start));
	  arg++;
	}

	if(!GOBBLE(')')) {
	  int old_line = this->current_line;
	  this->current_line = start_line;
	  cpp_error_sprintf(this, "Missing ) in the meta function %s().",
			    func_name->str);
	  this->current_line = old_line;
	}
	/* NOTE: cpp_func MUST protect against errors. */
	if(OUTP())
	  cpp_func(this, arg);
	else
	  pop_n_elems(arg);
      } else {
	if(OUTP())
	  push_int(0);
      }
      free_string (func_name);
      break;
    }

    cpp_error_sprintf(this, "Syntax error in #if bad character %c (%d).",
		      data[pos], data[pos]);
    break;
  }
  }

  SKIPWHITE();

  while(GOBBLE('['))
  {
    CALC_DUMPPOS("inside calcC");
    pos=calc1(this,data,len,pos,flags);
    if(OUTP())
      f_index(2);

    SKIPWHITE();
    if(!GOBBLE(']'))
      cpp_error(this, "Missing ']'.");
  }
  CALC_DUMPPOS("after calcC");
  return pos;
}

static ptrdiff_t calcB(struct cpp *this, WCHAR *data, ptrdiff_t len,
		       ptrdiff_t pos, int flags)
{
  CALC_DUMPPOS("before calcB");

  SKIPWHITE();
  switch(data[pos])
  {
    case '-': pos++; pos=calcB(this,data,len,pos,flags);
      if(OUTP()) o_negate(); break;
    case '!': pos++; pos=calcB(this,data,len,pos,flags);
      if(OUTP()) o_not(); break;
    case '~': pos++; pos=calcB(this,data,len,pos,flags);
      if(OUTP()) o_compl(); break;
    default: pos=calcC(this,data,len,pos,flags);
  }
  CALC_DUMPPOS("after calcB");
  return pos;
}

static ptrdiff_t calcA(struct cpp *this, WCHAR *data, ptrdiff_t len,
		       ptrdiff_t pos, int flags)
{
  CALC_DUMPPOS("before calcA");

  pos=calcB(this,data,len,pos,flags);
  while(1)
  {
    CALC_DUMPPOS("inside calcA");
    SKIPWHITE();
    switch(data[pos])
    {
      case '/':
	if(data[1]=='/' || data[1]=='*') return pos;
	pos++;
	pos=calcB(this,data,len,pos,flags);
	if(OUTP())
	  o_divide();
	continue;

      case '*':
	pos++;
	pos=calcB(this,data,len,pos,flags);
	if(OUTP())
	  o_multiply();
	continue;

      case '%':
	pos++;
	pos=calcB(this,data,len,pos,flags);
	if(OUTP())
	  o_mod();
	continue;
    }
    break;
  }
  CALC_DUMPPOS("after calcA");
  return pos;
}

static ptrdiff_t calc9(struct cpp *this, WCHAR *data, ptrdiff_t len,
		       ptrdiff_t pos, int flags)
{
  CALC_DUMPPOS("before calc9");

  pos=calcA(this,data,len,pos,flags);

  while(1)
  {
    CALC_DUMPPOS("inside calc9");
    SKIPWHITE();
    switch(data[pos])
    {
      case '+':
	pos++;
	pos=calcA(this,data,len,pos,flags);
	if(OUTP())
	  f_add(2);
	continue;

      case '-':
	pos++;
	pos=calcA(this,data,len,pos,flags);
	if(OUTP())
	  o_subtract();
	continue;
    }
    break;
  }

  CALC_DUMPPOS("after calc9");
  return pos;
}

static ptrdiff_t calc8(struct cpp *this, WCHAR *data, ptrdiff_t len,
		       ptrdiff_t pos, int flags)
{
  CALC_DUMPPOS("before calc8");

  pos=calc9(this,data,len,pos,flags);

  while(1)
  {
    static const WCHAR lsh_[] = { '<', '<' };
    static const WCHAR rsh_[] = { '>', '>' };

    CALC_DUMPPOS("inside calc8");
    SKIPWHITE();
    if(GOBBLEOP2(lsh_))
    {
      CALC_DUMPPOS("Found <<");
      pos=calc9(this,data,len,pos,flags);
      if(OUTP())
	o_lsh();
      break;
    }

    if(GOBBLEOP2(rsh_))
    {
      CALC_DUMPPOS("Found >>");
      pos=calc9(this,data,len,pos,flags);
      if(OUTP())
	o_rsh();
      break;
    }

    break;
  }
  return pos;
}

static ptrdiff_t calc7b(struct cpp *this, WCHAR *data, ptrdiff_t len,
			ptrdiff_t pos, int flags)
{
  CALC_DUMPPOS("before calc7b");

  pos=calc8(this,data,len,pos,flags);

  while(1)
  {
    CALC_DUMPPOS("inside calc7b");

    SKIPWHITE();
    
    switch(data[pos])
    {
      case '<':
	if(data[pos]+1 == '<') break;
	pos++;
	if(GOBBLE('='))
	{
	   pos=calc8(this,data,len,pos,flags);
	   if(OUTP())
	     f_le(2);
	}else{
	   pos=calc8(this,data,len,pos,flags);
	   if(OUTP())
	     f_lt(2);
	}
	continue;

      case '>':
	if(data[pos]+1 == '>') break;
	pos++;
	if(GOBBLE('='))
	{
	   pos=calc8(this,data,len,pos,flags);
	   if(OUTP())
	     f_ge(2);
	}else{
	   pos=calc8(this,data,len,pos,flags);
	   if(OUTP())
	     f_gt(2);
	}
	continue;
    }
    break;
  }
  return pos;
}

static ptrdiff_t calc7(struct cpp *this, WCHAR *data, ptrdiff_t len,
		       ptrdiff_t pos, int flags)
{
  CALC_DUMPPOS("before calc7");

  pos=calc7b(this,data,len,pos,flags);

  while(1)
  {
    static const WCHAR eq_[] = { '=', '=' };
    static const WCHAR ne_[] = { '!', '=' };

    CALC_DUMPPOS("inside calc7");

    SKIPWHITE();
    if(GOBBLEOP2(eq_))
    {
      pos=calc7b(this,data,len,pos,flags);
      if(OUTP())
	f_eq(2);
      continue;
    }

    if(GOBBLEOP2(ne_))
    {
      pos=calc7b(this,data,len,pos,flags);
      if(OUTP())
	f_ne(2);
      continue;
    }

    break;
  }
  return pos;
}

static ptrdiff_t calc6(struct cpp *this, WCHAR *data, ptrdiff_t len,
		       ptrdiff_t pos, int flags)
{
  CALC_DUMPPOS("before calc6");

  pos=calc7(this,data,len,pos,flags);

  SKIPWHITE();
  while(data[pos] == '&' && data[pos+1]!='&')
  {
    CALC_DUMPPOS("inside calc6");

    pos++;
    pos=calc7(this,data,len,pos,flags);
    if(OUTP())
      o_and();
  }
  return pos;
}

static ptrdiff_t calc5(struct cpp *this, WCHAR *data, ptrdiff_t len,
		       ptrdiff_t pos, int flags)
{
  CALC_DUMPPOS("before calc5");

  pos=calc6(this,data,len,pos,flags);

  SKIPWHITE();
  while(GOBBLE('^'))
  {
    CALC_DUMPPOS("inside calc5");

    pos=calc6(this,data,len,pos,flags);
    if(OUTP())
      o_xor();
  }
  return pos;
}

static ptrdiff_t calc4(struct cpp *this, WCHAR *data, ptrdiff_t len,
		       ptrdiff_t pos, int flags)
{
  CALC_DUMPPOS("before calc4");

  pos=calc5(this,data,len,pos,flags);

  SKIPWHITE();
  while(data[pos] == '|' && data[pos+1]!='|')
  {
    CALC_DUMPPOS("inside calc4");
    pos++;
    pos=calc5(this,data,len,pos,flags);
    if(OUTP())
      o_or();
  }
  return pos;
}

static ptrdiff_t calc3(struct cpp *this, WCHAR *data, ptrdiff_t len,
		       ptrdiff_t pos, int flags)
{
  static const WCHAR land_[] = { '&', '&' };

  CALC_DUMPPOS("before calc3");

  pos=calc4(this,data,len,pos,flags);

  SKIPWHITE();
  while(GOBBLEOP2(land_))
  {
    CALC_DUMPPOS("inside calc3");

    if(OUTP()) {
      check_destructed(Pike_sp-1);
      if(UNSAFE_IS_ZERO(Pike_sp-1))
      {
	pos=calc4(this,data,len,pos,flags|CPP_REALLY_NO_OUTPUT);
      }else{
	pop_stack();
	pos=calc4(this,data,len,pos,flags);
      }
    } else
      pos=calc4(this,data,len,pos,flags);
  }
  return pos;
}

static ptrdiff_t calc2(struct cpp *this, WCHAR *data, ptrdiff_t len,
		       ptrdiff_t pos, int flags)
{
  static const WCHAR lor_[] = { '|', '|' };

  CALC_DUMPPOS("before calc2");

  pos=calc3(this,data,len,pos,flags);

  SKIPWHITE();
  while(GOBBLEOP2(lor_))
  {
    CALC_DUMPPOS("inside calc2");

    if(OUTP()) {
      check_destructed(Pike_sp-1);
      if(!UNSAFE_IS_ZERO(Pike_sp-1))
      {
	pos=calc3(this,data,len,pos,flags|CPP_REALLY_NO_OUTPUT);
      }else{
	pop_stack();
	pos=calc3(this,data,len,pos,flags);
      }
    } else
      pos=calc3(this,data,len,pos,flags);
  }
  return pos;
}

static ptrdiff_t calc1(struct cpp *this, WCHAR *data, ptrdiff_t len,
		       ptrdiff_t pos, int flags)
{
  CALC_DUMPPOS("before calc1");

  pos=calc2(this,data,len,pos,flags);

  SKIPWHITE();

  if(GOBBLE('?'))
  {
    int select = -1;
    if(OUTP()) {
      check_destructed(Pike_sp-1);
      select = (UNSAFE_IS_ZERO(Pike_sp-1)?0:1);
      pop_stack();
    }
    pos=calc1(this,data,len,pos,(select == 1? flags:(flags|CPP_REALLY_NO_OUTPUT)));
    if(!GOBBLE(':'))
      cpp_error(this, "Colon expected.");
    pos=calc1(this,data,len,pos,(select == 0? flags:(flags|CPP_REALLY_NO_OUTPUT)));
  }
  return pos;
}

static ptrdiff_t calc(struct cpp *this, WCHAR *data, ptrdiff_t len,
		      ptrdiff_t tmp, int flags)
{
  JMP_BUF recovery;
  ptrdiff_t pos;

  CALC_DUMPPOS("Calculating");

  if (SETJMP(recovery))
  {
    cpp_handle_exception (this, "Error evaluating expression.");
    pos=tmp;
    FIND_EOL();
    push_int(0);
  }else{
    pos=calc1(this,data,len,tmp,flags);
    check_destructed(Pike_sp-1);
  }
  UNSETJMP(recovery);

  CALC_DUMPPOS("Done");

  return pos;
}

static ptrdiff_t lower_cpp(struct cpp *this,
			   WCHAR *data,
			   ptrdiff_t len,
			   int flags,
			   int auto_convert,
			   struct pike_string *charset)
{
  ptrdiff_t pos, tmp, e;
  int tmp2;
  INT32 first_line = this->current_line;
  /* FIXME: What about this->current_file? */
  
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

    case 0x1b: case 0x9b:	/* ESC or CSI */
      /* Assume ANSI/DEC escape sequence.
       * Format supported:
       * 	<ESC>[\040-\077]+[\100-\177]
       * or
       * 	<CSI>[\040-\077]*[\100-\177]
       */
      /* FIXME: This place is far from enough to make these things
       * behave as whitespace. /mast */
      while ((tmp = data[pos]) && (tmp == ((tmp & 0x1f)|0x20))) {
	pos++;
      }
      if (tmp == ((tmp & 0x3f)|0x40)) {
	pos++;
      } else {
	/* FIXME: Warning here? */
      }

      PUTC(' ');
      break;

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
	  cpp_error(this, "CVS conflict detected.");
	
      case '!': case '@': case '$': case '%': case '^': case '&':
      case '*': case '(': case ')': case '-': case '=': case '+':
      case '{': case '}': case ':': case '?': case '`': case ';':
      case '>': case ',': case '.': case '~': case '[':
      case ']': case '|':
	PUTC(data[pos-1]);
      break;

    case '\\':
      if(data[pos]=='\n') {
	pos++;
	this->current_line++;
	PUTNL();
	goto do_skipwhite;
      }
      else if ((data[pos] == '\r') && (data[pos+1] == '\n')) {
	pos += 2;
	this->current_line++;
	PUTNL();
	goto do_skipwhite;
      }
      /* Fall through - identifiers might begin with \uxxxx. */

    default:
      if(OUTP())
      {
	struct pike_string *s;
	struct define *d=0;

	pos--;
	s = GOBBLE_IDENTIFIER();
	if (!s) {
	  PUTC (data[pos++]);
	  break;
	}

	if(flags & CPP_DO_IF && s == defined_str)
	{
	  /* NOTE: defined() must be handled here, since its argument
	   *       must not be macro expanded.
	   */
	  d = defined_macro;
	}else{
	  d=find_define(s);
	}
	  
	if(d && !(d->inside & 1))
	{
	  int arg=0;
	  INT32 start_line = this->current_line;
	  struct string_builder tmp;
	  struct define_argument arguments [MAX_ARGS];
	  short inside = d->inside;

	  if (d == defined_macro) {
	    free_string (s);
	    s = NULL;
	  }

	  if(d->args>=0)
	  {
	    SKIPWHITE();

	    if(!GOBBLE('('))
	    {
	      if (s) {
	        string_builder_shared_strcat(&this->buf,s);
		free_string(s);
	      }
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
		  if((d->varargs && arg + 1 == d->args) ||
		     (!arg && (d->args == 1))) {
		    /* Allow varargs to be left out.
		     *
		     * Allow a single argument to be left out.
		     */
		    arguments[arg].arg = MKPCHARP(data + pos, SHIFT);
		    arguments[arg].len=0;
		    continue;
		  }else{
		    cpp_error_sprintf(this,
				      "Too few arguments to macro %S, expected %d.",
				      d->link.s, d->args);
		    break;
		  }
		}
	      }
	      arguments[arg].arg = MKPCHARP(data + pos, SHIFT);

	      
	      while(1)
	      {
		if(pos+1>len)
		{
		  INT32 save_line = this->current_line;
		  this->current_line = start_line;
		  cpp_error(this, "End of file in macro call.");
		  this->current_line = save_line;
		  break;
		}
		
		switch(data[pos++])
		{
		case '\n':
		  this->current_line++;
		  PUTNL();
		default: continue;
		  
		case '"':
		  /* Note: Strings may contain \-escaped newlines.
		   *       They must be removed on insertion to
		   *       avoid being counted twice.
		   */
		  if(data[pos-2]!='#') {
		    FIND_END_OF_STRING();
		  }else{
		    FIND_END_OF_STRING2();  /* Newlines allowed */
		  }
		  continue;

		case '\'':
		  FIND_END_OF_CHAR();
		  continue;
		  
		case '(':
		  pos=find_end_parenthesis(this, data, len, pos);
		  continue;

		case '{':
		  pos=find_end_brace(this, data, len, pos);
		  continue;
		  
		case ',':
		  if(d->varargs && arg+1 == d->args) continue;

		case ')': 
		  pos--;
		  break;
		}
		break;
	      }
	      arguments[arg].len = data + pos -
		((WCHAR *)(arguments[arg].arg.ptr));
	    }
	    SKIPWHITE();
	    if(!GOBBLE(')')) {
	      this->current_line = start_line;
	      cpp_error_sprintf(this, "Missing ) in the macro %S.",
				d->link.s);
	    }
	  }
	  
	  if(d->args >= 0 && arg != d->args)
	    cpp_error(this, "Wrong number of arguments to macro.");
	  
	  init_string_builder(&tmp, SHIFT);
	  if(d->magic)
	  {
	    d->magic(this, d, arguments, &tmp);
	  }else{
	    string_builder_shared_strcat(&tmp, d->first);
	    for(e=0;e<d->num_parts;e++)
	    {
	      WCHAR *a;
	      ptrdiff_t l;
	      
	      if((d->parts[e].argument & DEF_ARG_MASK) < 0 || 
		 (d->parts[e].argument & DEF_ARG_MASK) >= arg)
	      {
		cpp_error(this, "Macro not expanded correctly.");
		continue;
	      }
	      
#ifdef PIKE_DEBUG
	      if (arguments[d->parts[e].argument&DEF_ARG_MASK].arg.shift !=
		  SHIFT) {
		Pike_fatal("cpp(): Bad argument shift %d != %d\n", SHIFT,
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
		/* NOTE: At entry a[0] is non white-space. */
		int e = 0;
		string_builder_putchar(&tmp, '"');
		for(e=0; e<l;) {
		  if (WC_ISSPACE(a[e]) || a[e]=='"' || a[e]=='\\') {
		    if (e) {
		      PIKE_XCONCAT (string_builder_binary_strcat, SHIFT) (&tmp, a, e);
		    }
		    if (a[e] == '"' || a[e]=='\\') {
		      /* String or quote. */
		      string_builder_putchar(&tmp, '\\');
		      string_builder_putchar(&tmp, a[e]);
		      if (a[e] == '"') {
			for (e++; (e < l) && (a[e] != '"'); e++) {
			  string_builder_putchar(&tmp, a[e]);
			  if (a[e] == '\\') {
			    string_builder_putchar(&tmp, '\\');
			    string_builder_putchar(&tmp, a[++e]);
			    if (a[e] == '\\') {
			      string_builder_putchar(&tmp, '\\');
			    }
			  }
			}
			string_builder_putchar(&tmp, '\\');
			string_builder_putchar(&tmp, '"');
			e++;
		      }
		    } else {
		      /* White space. */
		      while ((e < l) && WC_ISSPACE(a[e])) {
			e++;
		      }
		      if (e != l) {
			string_builder_putchar(&tmp, ' ');
		      }
		    }
		    a += e;
		    l -= e;
		    e = 0;
		  } else {
		    e++;
		  }
		}
		if (l) {
		  PIKE_XCONCAT (string_builder_binary_strcat, SHIFT) (&tmp, a, l);
		}
		string_builder_putchar(&tmp, '"');
	      }else{
		if(DEF_ARG_NOPRESPACE)
		  while(l && WC_ISSPACE(*a))
		    a++,l--;
		
		if(DEF_ARG_NOPOSTSPACE)
		  while(l && WC_ISSPACE(a[l-1]))
		    l--;

		if(d->parts[e].argument & (DEF_ARG_NOPRESPACE | DEF_ARG_NOPOSTSPACE))
		{
		  PIKE_XCONCAT (string_builder_binary_strcat, SHIFT) (&tmp, a, l);
		}else{
		  struct string_builder save;
		  INT32 line=this->current_line;
		  /* FIXME: Shouldn't we save current_file too? */
		  save=this->buf;
		  this->buf=tmp;
		  d->inside = 2;
		  lower_cpp(this, a, l,
			    flags & ~(CPP_EXPECT_ENDIF | CPP_EXPECT_ELSE),
			    auto_convert, charset);
		  d->inside = inside;
		  tmp=this->buf;
		  this->buf=save;
		  this->current_line=line;
		}
	      }
	      
	      if(!(d->parts[e].argument & DEF_ARG_NOPOSTSPACE))
		string_builder_putchar(&tmp, ' ');
	      
	      string_builder_shared_strcat(&tmp, d->parts[e].postfix);
	    }
	  }
	  
	  /* Remove any newlines from the completed expression. */
	  switch (tmp.s->size_shift) {
	  case 0:
	    for(e=0; e< (ptrdiff_t)tmp.s->len; e++)
	      if(STR0(tmp.s)[e]=='\n')
		STR0(tmp.s)[e]=' ';
	    break;
	  case 1:
	    for(e=0; e< (ptrdiff_t)tmp.s->len; e++)
	      if(STR1(tmp.s)[e]=='\n')
		STR1(tmp.s)[e]=' ';
	    break;
	  case 2:
	    for(e=0; e< (ptrdiff_t)tmp.s->len; e++)
	      if(STR2(tmp.s)[e]=='\n')
		STR2(tmp.s)[e]=' ';
	    break;
	  }

	  if(s) d->inside=1;
	  
	  string_builder_putchar(&tmp, 0);
	  tmp.s->len--;
	  low_cpp(this, tmp.s->str, tmp.s->len, tmp.s->size_shift,
		  flags & ~(CPP_EXPECT_ENDIF | CPP_EXPECT_ELSE),
		  auto_convert, charset);
	  
	  if(s)
	  {
	    if((d=find_define(s)))
	      d->inside = inside;
	    
	    free_string(s);
	  }

	  free_string_builder(&tmp);
	}else{
	  if (OUTP())
	    string_builder_shared_strcat (&this->buf, s);
	  free_string (s);
	}
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
      if(OUTP())
	PIKE_XCONCAT (string_builder_binary_strcat, SHIFT) (
	  &this->buf, data + tmp, pos - tmp);
      else
	for (; tmp < pos; tmp++)
	  if (data[tmp] == '\n')
	    string_builder_putchar (&this->buf, '\n');
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
	static const WCHAR line_[] = { 'l', 'i', 'n', 'e' };

	if(WGOBBLE2(line_))
	{
	  /* FIXME: Why not use SKIPSPACE()? */
	  /* Because SKIPSPACE skips newlines? - Hubbe */
	  while(data[pos]==' ' || data[pos]=='\t') pos++;
	}else{
	  goto unknown_preprocessor_directive;
	}
      /* Fall through */
      }
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
    {
      INT32 new_lineno;
      PCHARP foo = MKPCHARP(data+pos, SHIFT);
      new_lineno=STRTOL_PCHARP(foo, &foo, 10)-1;
      if(OUTP())
      {
	string_builder_putchar(&this->buf, '#');
	PIKE_XCONCAT (string_builder_binary_strcat, SHIFT) (
	  &this->buf, data + pos, ((WCHAR *)foo.ptr) - (data+pos));
      }
      pos = ((WCHAR *)foo.ptr) - data;
      SKIPSPACE();
      
      if(data[pos]=='"')
      {
	struct string_builder nf;
	init_string_builder(&nf, 0);
	
	READSTRING(nf);

	if(OUTP())
	{
	  free_string(this->current_file);
	  this->current_file = finish_string_builder(&nf);

	  string_builder_putchar(&this->buf, ' ');
	  PUSH_STRING_SHIFT (this->current_file->str, this->current_file->len,
			     this->current_file->size_shift, &this->buf);
	}else{
	  free_string_builder(&nf);
	}
      }

      if (OUTP())
	this->current_line = new_lineno;

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
	  static const WCHAR string_[] = { 's', 't', 'r', 'i', 'n', 'g' };

	  if(WGOBBLE2(string_))
	  {
	    tmp2=1;
	    goto do_include;
	  }
	}
      goto unknown_preprocessor_directive;

    case 'i': /* include, if, ifdef */
      {
	static const WCHAR include_[] = { 'i', 'n', 'c', 'l', 'u', 'd', 'e' };
	static const WCHAR if_[] = { 'i', 'f' };
	static const WCHAR ifdef_[] = { 'i', 'f', 'd', 'e', 'f' };
	static const WCHAR ifndef_[] = { 'i', 'f', 'n', 'd', 'e', 'f' };

      if(WGOBBLE2(include_))
      {
	tmp2=0;
      do_include:
	{
	  struct svalue *save_sp=Pike_sp;
	  SKIPSPACE();
	  
	  check_stack(3);

	  /* FIXME: Ought to macro-expand (Bug 2440). */
	  
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
		ptrdiff_t tmp = pos;
		while(data[pos]!='>')
		{
		  if(data[pos]=='\n')
		  {
		    cpp_error(this, "Expecting '>' in include.");
		    break;
		  }
		      
		  pos++;
		}
		push_string(MAKE_BINARY_STRING(data+tmp, pos-tmp));
		ref_push_string(this->current_file);
		pos++;
		push_int(0);
		break;
	      }

	    default:
	      cpp_error(this, "Expected file to include.");
	      break;
	    }

	  if(Pike_sp==save_sp) break;

	  if(OUTP())
	  {
	    struct pike_string *new_file;

	    if (!safe_apply_handler ("handle_include",
				     this->handler, this->compat_handler,
				     3, BIT_STRING) ||
		!(new_file=Pike_sp[-1].u.string) ) {
	      cpp_handle_exception (this, "Couldn't include file.");
	      pop_n_elems(Pike_sp-save_sp);
	      break;
	    }

	    /* Why not just use ref_push_string(new_file)? */
	    assign_svalue_no_free(Pike_sp,Pike_sp-1);
	    Pike_sp++;
	    dmalloc_touch_svalue(Pike_sp-1);

	    if (!safe_apply_handler ("read_include",
				     this->handler, this->compat_handler,
				     1, BIT_STRING)) {
	      cpp_handle_exception (this, "Couldn't read include file.");
	      pop_n_elems(Pike_sp-save_sp);
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
		/* #string */
		struct pike_string *str = Pike_sp[-1].u.string;
		PUSH_STRING_SHIFT(str->str, str->len, str->size_shift,
				  &this->buf);
	      }else{
		/* #include */
		if (auto_convert) {
		  struct pike_string *new_str =
		    recode_string(this, Pike_sp[-1].u.string);
		  free_string(Pike_sp[-1].u.string);
		  Pike_sp[-1].u.string = new_str;
		} else if (charset) {
		  ref_push_string(charset);
		  if (!safe_apply_handler ("decode_charset",
					   this->handler, this->compat_handler,
					   2, BIT_STRING)) {
		    cpp_handle_exception (this,
					  "Charset decoding failed for included file.");
		    pop_n_elems(Pike_sp - save_sp);
		    break;
		  }
		}
		if (Pike_sp[-1].u.string->size_shift) {
		  /* Get rid of any byte order marks (0xfeff) */
		  struct pike_string *new_str = filter_bom(Pike_sp[-1].u.string);
		  free_string(Pike_sp[-1].u.string);
		  Pike_sp[-1].u.string = new_str;
		}
		low_cpp(this,
			Pike_sp[-1].u.string->str,
			Pike_sp[-1].u.string->len,
			Pike_sp[-1].u.string->size_shift,
			flags&~(CPP_EXPECT_ENDIF | CPP_EXPECT_ELSE),
			auto_convert, charset);
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

	  pop_n_elems(Pike_sp-save_sp);
	  
	  break;
	}
      }

      if(WGOBBLE2(if_))
      {
	struct string_builder save, tmp;
	INT32 nflags = 0;
#ifdef PIKE_DEBUG
	ptrdiff_t skip;
#endif /* PIKE_DEBUG */
	
	if(!OUTP())
	  nflags = CPP_REALLY_NO_OUTPUT;

	save=this->buf;
	init_string_builder(&this->buf, SHIFT);
#ifdef PIKE_DEBUG
	/* CALC_DUMPPOS("#if before lower_cpp()"); */
	skip = lower_cpp(this, data+pos, len-pos,
			 nflags | CPP_END_AT_NEWLINE | CPP_DO_IF,
			 auto_convert, charset);
	/* fprintf(stderr, "shift:%d, pos:%d, len:%d, skip:%d\n",
	   SHIFT, pos, len, skip); */

	pos += skip;
	/* CALC_DUMPPOS("#if after lower_cpp()"); */
	/* fprintf(stderr, "tmp (%d:%d): \"",
	   this->buf.s->len, this->buf.s->size_shift); */
	/* fflush(stderr); */
	/* write(2, this->buf.s->str,
	   this->buf.s->len << this->buf.s->size_shift); */
	/* fprintf(stderr, "\"\n"); */
	/* fflush(stderr); */
#else /* !PIKE_DEBUG */
	pos += lower_cpp(this, data+pos, len-pos,
			 nflags | CPP_END_AT_NEWLINE | CPP_DO_IF,
			 auto_convert, charset);
#endif /* PIKE_DEBUG */
	tmp=this->buf;
	this->buf=save;
	
	string_builder_putchar(&tmp, 0);
	tmp.s->len--;

	if (!nflags) {
	  switch(tmp.s->size_shift) {
	  case 0:
	    calc_0(this, (p_wchar0 *)tmp.s->str, tmp.s->len, 0, 0);
	    break;
	  case 1:
	    calc_1(this, (p_wchar1 *)tmp.s->str, tmp.s->len, 0, 0);
	    break;
	  case 2:
	    calc_2(this, (p_wchar2 *)tmp.s->str, tmp.s->len, 0, 0);
	    break;
	  default:
	    Pike_fatal("cpp(): Bad shift: %d\n", tmp.s->size_shift);
	    break;
	  }
	  if(SAFE_IS_ZERO(Pike_sp-1)) nflags|=CPP_NO_OUTPUT;
	  pop_stack();
	}
	free_string_builder(&tmp);
	pos += lower_cpp(this, data+pos, len-pos,
			 nflags | CPP_EXPECT_ELSE | CPP_EXPECT_ENDIF,
			 auto_convert, charset);
	break;
      }

      if(WGOBBLE2(ifdef_))
	{
	  INT32 nflags;
	  struct pike_string *s;
	  SKIPSPACE();

	  s = GOBBLE_IDENTIFIER();
	  if(!s)
	    cpp_error(this, "#ifdef what?");

	  nflags = CPP_EXPECT_ELSE | CPP_EXPECT_ENDIF | CPP_NO_OUTPUT;
	  if(!OUTP())
	    nflags|=CPP_REALLY_NO_OUTPUT;
	  if(find_define(s))
	    nflags&=~CPP_NO_OUTPUT;
	  free_string (s);

	  pos += lower_cpp(this, data+pos, len-pos, nflags,
			   auto_convert, charset);
	  break;
	}

      if(WGOBBLE2(ifndef_))
	{
	  INT32 nflags;
	  struct pike_string *s;
	  SKIPSPACE();

	  s = GOBBLE_IDENTIFIER();
	  if(!s)
	    cpp_error(this, "#ifndef what?");

	  nflags=CPP_EXPECT_ELSE | CPP_EXPECT_ENDIF;
	  if(!OUTP())
	    nflags|=CPP_REALLY_NO_OUTPUT;
	  if(find_define(s))
	    nflags|=CPP_NO_OUTPUT;
	  free_string (s);

	  pos += lower_cpp(this, data+pos, len-pos, nflags,
			   auto_convert, charset);
	  break;
	}

      goto unknown_preprocessor_directive;
      }
    case 'e': /* endif, else, elif, error */
      {
	static const WCHAR endif_[] = { 'e', 'n', 'd', 'i', 'f' };
	static const WCHAR else_[] = { 'e', 'l', 's', 'e' };
	static const WCHAR elseif_[] = { 'e', 'l', 's', 'e', 'i', 'f' };
	static const WCHAR elif_[] = { 'e', 'l', 'i', 'f' };
	static const WCHAR error_[] = { 'e', 'r', 'r', 'o', 'r' };

      if(WGOBBLE2(endif_))
      {
	if(!(flags & CPP_EXPECT_ENDIF))
	  cpp_error(this, "Unmatched #endif.");

	return pos;
      }

      if(WGOBBLE2(else_))
	{
	  if(!(flags & CPP_EXPECT_ELSE))
	    cpp_error(this, "Unmatched #else.");

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
	  cpp_error(this, "Unmatched #elif.");
	
	flags|=CPP_EXPECT_ENDIF;
	
	if((flags & (CPP_NO_OUTPUT | CPP_REALLY_NO_OUTPUT)) == CPP_NO_OUTPUT)
	{
	  struct string_builder save,tmp;
	  save=this->buf;
	  init_string_builder(&this->buf, 0);
	  pos += lower_cpp(this, data+pos, len-pos,
			   CPP_END_AT_NEWLINE | CPP_DO_IF,
			   auto_convert, charset);
	  tmp=this->buf;
	  this->buf=save;

	  string_builder_putchar(&tmp, 0);
	  tmp.s->len--;
	  
	  switch(tmp.s->size_shift) {
	  case 0:
	    calc_0(this, (p_wchar0 *)tmp.s->str, tmp.s->len,0,0);
	    break;
	  case 1:
	    calc_1(this, (p_wchar1 *)tmp.s->str, tmp.s->len,0,0);
	    break;
	  case 2:
	    calc_2(this, (p_wchar2 *)tmp.s->str, tmp.s->len,0,0);
	    break;
	  default:
	    Pike_fatal("cpp(): Bad shift: %d\n", tmp.s->size_shift);
	    break;
	  }
	  free_string_builder(&tmp);
	  if(!SAFE_IS_ZERO(Pike_sp-1)) flags&=~CPP_NO_OUTPUT;
	  pop_stack();
	} else {
	  FIND_EOL();
	  flags |= CPP_NO_OUTPUT | CPP_REALLY_NO_OUTPUT;
	}
	break;
      }

      if(WGOBBLE2(error_))
	{
          ptrdiff_t foo;

          SKIPSPACE();
          foo=pos;
          FIND_EOL();
	  if(OUTP())
	  {
	    push_string(MAKE_BINARY_STRING(data+foo, pos-foo));
	    cpp_error_sprintf(this, "%O", Pike_sp[-1]);
	  }
	  break;
	}
      }
      goto unknown_preprocessor_directive;

    case 'd': /* define */
      {
	static const WCHAR define_[] = { 'd', 'e', 'f', 'i', 'n', 'e' };

      if(WGOBBLE2(define_))
	{
	  struct string_builder str;
	  INT32 argno=-1;
	  ptrdiff_t tmp3;
	  struct pike_string *def_name;
	  struct define *def;
	  struct svalue *partbase,*argbase=Pike_sp;
	  int varargs=0;

	  SKIPSPACE();

	  def_name = GOBBLE_IDENTIFIER();
	  if(!def_name) {
	    cpp_error(this, "Define what?");
	    FIND_EOL();
	    break;
	  }

	  if(GOBBLE('('))
	    {
	      argno=0;
	      SKIPWHITE();

	      while(data[pos]!=')')
		{
		  struct pike_string *arg_name;
		  if(argno)
		    {
		      if(!GOBBLE(','))
			cpp_error(this,
				  "Expecting comma in macro definition.");
		      SKIPWHITE();
		    }
		  if(varargs)
		    cpp_error(this,"Expected ) after ...");

		  arg_name = GOBBLE_IDENTIFIER();
		  if(!arg_name)
		  {
		    cpp_error(this, "Expected argument for macro.");
		    break;
		  }

		  check_stack(1);
		  push_string(arg_name);

		  SKIPWHITE();
		  argno++;
		  if(argno>=MAX_ARGS)
		  {
		    cpp_error(this, "Too many arguments in macro definition.");
		    pop_stack();
		    argno--;
		  }

		  if(data[pos]=='.' && data[pos+1]=='.' && data[pos+2]=='.')
		  {
		    varargs=1;
		    pos+=3;
		    SKIPWHITE();
		  }
		}

	      if(!GOBBLE(')'))
		cpp_error(this, "Missing ) in macro definition.");
	    }

	  SKIPSPACE();

	  partbase=Pike_sp;
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
	      
	    case '\\':
	      if(GOBBLE('\n'))
	      { 
		this->current_line++;
		PUTNL();
		continue;
	      }
	      if (data[pos] == '\r' && data[pos+1] == '\n') {
		pos += 2;
		this->current_line++;
		PUTNL();
		continue;
	      }
	      /* Identifiers might start with \uxxxx, so try to parse
	       * an identifier now. */
	      goto gobble_identifier_in_define;

	    case '#':
	      if(GOBBLE('#'))
	      {
		extra=DEF_ARG_NOPRESPACE;
		/* FIXME: Wide strings? */
		while(str.s->len && isspace(((unsigned char *)str.s->str)[str.s->len-1]))
		  str.s->len--;
		if(!str.s->len && Pike_sp-partbase>1)
		{
#ifdef PIKE_DEBUG
		  if(Pike_sp[-1].type != PIKE_T_INT)
		    Pike_fatal("Internal error in CPP\n");
#endif
		  Pike_sp[-1].u.integer|=DEF_ARG_NOPOSTSPACE;
		}
	      }else{
		extra=DEF_ARG_STRINGIFY;
	      }
	      SKIPSPACE();
	      pos++;
	      /* fall through */

	    gobble_identifier_in_define:
	    default: {
	      struct pike_string *s;
	      pos--;
	      s = GOBBLE_IDENTIFIER();
	      if (s)
	      {
		tmp3=pos-1;
		if(argno>0)
		{
		  if (s->refs > 1)
		  {
		    for(e=0;e<argno;e++)
		    {
		      if(argbase[e].u.string == s)
		      {
			check_stack(2);
			push_string(finish_string_builder(&str));
			init_string_builder(&str, 0);
			push_int(DO_NOT_WARN(e | extra));
			extra=0;
			break;
		      }
		    }
		    if(e!=argno) {
		      free_string (s);
		      continue;
		    }
		  }
		}
		string_builder_shared_strcat (&str, s);
		free_string (s);
	      }else{
		string_builder_putchar(&str, data[pos++]);
	      }
	      extra=0;
	      continue;
	    }
	      
	    case '"':
	      FIXSTRING(str, 1);
	      continue;
	      
	    case '\'':
	      tmp3=pos-1;
	      FIND_END_OF_CHAR();
	      PIKE_XCONCAT (string_builder_binary_strcat, SHIFT) (
		&str, data + tmp3, pos - tmp3);
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
	    def = alloc_empty_define(def_name, (Pike_sp-partbase)/2);
	    copy_shared_string(def->first, partbase->u.string);
	    def->args=argno;
	    def->varargs=varargs;
	    
	    for(e=0;e<def->num_parts;e++)
	    {
#ifdef PIKE_DEBUG
	      if(partbase[e*2+1].type != PIKE_T_INT)
		Pike_fatal("Cpp internal error, expected integer!\n");
	      
	      if(partbase[e*2+2].type != PIKE_T_STRING)
		Pike_fatal("Cpp internal error, expected string!\n");
#endif
	      def->parts[e].argument=partbase[e*2+1].u.integer;
	      copy_shared_string(def->parts[e].postfix,
				 partbase[e*2+2].u.string);
	    }
	    
#ifdef PIKE_DEBUG
	    if(def->num_parts==1 &&
	       (def->parts[0].argument & DEF_ARG_MASK) > MAX_ARGS)
	      Pike_fatal("Internal error in define\n");
#endif	  
	    {
	      struct define *d;
	      if ((d = find_define(def->link.s)) && (d->inside)) {
		cpp_error(this,
			  "Illegal to redefine a macro during its expansion.");
		free_one_define(&(def->link));
	      } else {
		this->defines=hash_insert(this->defines, & def->link);
	      }
	    }
	  }
	  else
	    free_string (def_name);
	  pop_n_elems(Pike_sp-argbase);
	  break;
	}
      
      goto unknown_preprocessor_directive;
      }
    case 'u': /* undefine */
      {
	static const WCHAR undef_[] = { 'u', 'n', 'd', 'e', 'f' };
	static const WCHAR undefine_[] = { 'u', 'n', 'd', 'e', 'f', 'i', 'n', 'e' };

	/* NOTE: Reuses undefine_ for undef_ */
	if(WGOBBLE2(undefine_) || WGOBBLE2(undef_))
	{
	  struct pike_string *s;
	  SKIPSPACE();
	  s = GOBBLE_IDENTIFIER();
	  if(!s) {
	    cpp_error(this, "Undefine what?");
	    break;
	  }
	  if(OUTP())
	  {
	    undefine(this,s);
	  }
	  free_string (s);
	  break;
	}

      goto unknown_preprocessor_directive;
      }
    case 'c': /* charset */
      {
	static const WCHAR charset_[] = { 'c', 'h', 'a', 'r', 's', 'e', 't' };

      if (WGOBBLE2(charset_)) {
	ptrdiff_t p;
	struct pike_string *s;

	if (flags & (CPP_EXPECT_ENDIF | CPP_EXPECT_ELSE)) {
	  /* Only allowed at the top-level */
	  cpp_error(this, "#charset directive inside #if/#endif.");
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

	  if (!safe_apply_handler ("decode_charset", this->handler, this->compat_handler,
				   2, BIT_STRING)) {
	    cpp_handle_exception (this, NULL);
	  } else {
	    low_cpp(this, Pike_sp[-1].u.string->str, Pike_sp[-1].u.string->len,
		    Pike_sp[-1].u.string->size_shift, flags,
		    auto_convert, charset);
	    pop_stack();
	  }
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
	static const WCHAR pragma_[] = { 'p', 'r', 'a', 'g', 'm', 'a' };
	static const WCHAR pike_[] = { 'p', 'i', 'k', 'e' };

	if(WGOBBLE2(pragma_))
	{
	  if(OUTP())
	    STRCAT("#pragma", 7);
	  else
	    FIND_EOL();
	  break;
	}
	if(WGOBBLE2(pike_))
	{
	  
	  if(OUTP())
	  {
	    int major, minor;
	    ptrdiff_t tmp;
	    PCHARP ptr;

	    STRCAT("#pike", 5);
	    tmp= this->buf.s->len;
	    pos +=  lower_cpp(this, data+pos, len-pos,
			      CPP_END_AT_NEWLINE | CPP_DO_IF,
			      auto_convert, charset);

	    ptr=MKPCHARP(this->buf.s->str, this->buf.s->size_shift);
	    INC_PCHARP(ptr, tmp);

	    major=STRTOL_PCHARP(ptr, &ptr, 10);
	    if(INDEX_PCHARP(ptr,0) == '.')
	    {
	      INC_PCHARP(ptr, 1);
	      minor=STRTOL_PCHARP(ptr, &ptr, 10);
	      cpp_change_compat(this, major, minor);
	    }else{
	      cpp_error(this, "Missing '.' in #pike.");
	      this->compat_minor=0;
	    }
	  }
	  else
	    FIND_EOL();
	  break;
	}
      }
    case 'w': /* warning */
      {
	static const WCHAR warning_[] = { 'w', 'a', 'r', 'n', 'i', 'n', 'g' };

	if(WGOBBLE2(warning_))
	{
	  ptrdiff_t foo;

          SKIPSPACE();
          foo=pos;
          FIND_EOL();
	  if(OUTP())
	  {
	    push_string(MAKE_BINARY_STRING(data+foo, pos-foo));
	    cpp_warning(this, "%O", Pike_sp[-1]);
	  }
	  break;
	}
	goto unknown_preprocessor_directive;
      }
    default:
      if(!OUTP() && !this->picky_cpp) break;
    unknown_preprocessor_directive:
      {
	struct pike_string *unknown = GOBBLE_IDENTIFIER();
	cpp_error_sprintf(this, "Unknown preprocessor directive %S.", unknown);
	free_string (unknown);
      }
    }
    }
  }

  if(flags & CPP_EXPECT_ENDIF) {
    INT32 saved_line = this->current_line;
    this->current_line = first_line;
    cpp_error(this, "End of file while searching for #endif.");
    this->current_line = saved_line;
  }

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
#undef MAKE_BINARY_STRING

#undef gobble_identifier
#undef lower_cpp
#undef parse_esc_seq
#undef find_end_parenthesis
#undef find_end_brace

#undef calc
#undef calc1
#undef calc2
#undef calc3
#undef calc4
#undef calc5
#undef calc6
#undef calc7
#undef calc7b
#undef calc8
#undef calc9
#undef calcA
#undef calcB
#undef calcC
