/*
 * $Id: lexer.h,v 1.8 1999/10/23 06:51:28 hubbe Exp $
 *
 * Lexical analyzer template.
 * Based on lex.c 1.62
 *
 * Henrik Grubbström 1999-02-20
 */

#ifndef SHIFT
#error Internal error: SHIFT not defined
#endif

/*
 * Definitions
 */
#if (SHIFT == 0)

#define LOOK() EXTRACT_UCHAR(lex.pos)
#define GETC() EXTRACT_UCHAR(lex.pos++)
#define SKIP() lex.pos++
#define GOBBLE(c) (LOOK()==c?(lex.pos++,1):0)
#define SKIPSPACE() do { while(ISSPACE(LOOK()) && LOOK()!='\n') lex.pos++; }while(0)
#define SKIPWHITE() do { while(ISSPACE(LOOK())) lex.pos++; }while(0)
#define SKIPUPTO(X) do { while(LOOK()!=(X) && LOOK()) lex.pos++; }while(0)

#define READBUF(X) {				\
  register int C;				\
  buf=lex.pos;					\
  while((C=LOOK()) && (X)) lex.pos++;		\
  len=lex.pos - buf;				\
}

#define TWO_CHAR(X,Y) ((X)<<8)+(Y)
#define ISWORD(X) (len==(long)strlen(X) && !MEMCMP(buf,X,strlen(X)))

/*
 * Function renaming
 */

#define yylex yylex0
#define low_yylex low_yylex0
#define lex_atoi atoi
#define lex_strtol STRTOL
#define lex_strtod my_strtod
#define lex_isidchar isidchar

#else /* SHIFT != 0 */

#define LOOK() INDEX_CHARP(lex.pos,0,SHIFT)
#define GETC() ((lex.pos+=(1<<SHIFT)),INDEX_CHARP(lex.pos-(1<<SHIFT),0,SHIFT))
#define SKIP() (lex.pos += (1<<SHIFT))
#define GOBBLE(c) (LOOK()==c?((lex.pos+=(1<<SHIFT)),1):0)
#define SKIPSPACE() do { while(ISSPACE(LOOK()) && LOOK()!='\n') lex.pos += (1<<SHIFT); }while(0)
#define SKIPWHITE() do { while(ISSPACE(LOOK())) lex.pos += (1<<SHIFT); }while(0)
#define SKIPUPTO(X) do { while(LOOK()!=(X) && LOOK()) lex.pos += (1<<SHIFT); }while(0)

#define READBUF(X) {				\
  register int C;				\
  buf=lex.pos;					\
  while((C=LOOK()) && (X)) lex.pos+=(1<<SHIFT);	\
  len=(lex.pos - buf) >> SHIFT;			\
}

#define TWO_CHAR(X,Y) ((X)<<8)+(Y)

#define ISWORD(X) (len==(long)strlen(X) && low_isword(buf, X, strlen(X)))

/* Function renaming */
#if (SHIFT == 1)

#define low_isword low_isword1
#define char_const char_const1
#define readstring readstring1
#define yylex yylex1
#define low_yylex low_yylex1
#define lex_atoi lex_atoi1
#define lex_strtol lex_strtol1
#define lex_strtod lex_strtod1

#else /* SHIFT != 1 */

#define low_isword low_isword2
#define char_const char_const2
#define readstring readstring2
#define yylex yylex2
#define low_yylex low_yylex2
#define lex_atoi lex_atoi2
#define lex_strtol lex_strtol2
#define lex_strtod lex_strtod2

#endif /* SHIFT == 1 */

#define lex_isidchar(X) ((((unsigned) X)>=256) || isidchar(X))

static int low_isword(char *buf, char *X, int len)
{
  while(len--) {
    if (INDEX_CHARP(buf, len, SHIFT) != ((unsigned char *)X)[len]) {
      return 0;
    }
  }
  return 1;
}

static int lex_atoi(char *buf)
{
  /* NOTE: Cuts at 63 digits */
  char buff[64];
  int i=0;
  int c;

  while(((c = INDEX_CHARP(buf, i, SHIFT))>='0') && (c <= '9') && (i < 63)) {
    buff[i++] = c;
  }
  buff[i] = 0;
  return atoi(buff);
}

static long lex_strtol(char *buf, char **end, int base)
{
  PCHARP foo;
  long ret;
  ret=STRTOL_PCHARP(MKPCHARP(buf,SHIFT),&foo,base);
  if(end) end[0]=foo.ptr;
  return ret;
}

static double lex_strtod(char *buf, char **end)
{
  PCHARP foo;
  double ret;
  ret=STRTOD_PCHARP(MKPCHARP(buf,SHIFT),&foo);
  if(end) end[0]=foo.ptr;
  return ret;
}

#endif /* SHIFT == 0 */

/*** Lexical analyzing ***/

static int char_const(void)
{
  int c;
  switch(c=GETC())
  {
    case 0:
      lex.pos -= (1<<SHIFT);
      yyerror("Unexpected end of file\n");
      return 0;
      
    case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7':
      c-='0';
      while(LOOK()>='0' && LOOK()<='8')
	c=c*8+(GETC()-'0');
      return c;
      
    case 'r': return '\r';
    case 'n': return '\n';
    case 't': return '\t';
    case 'b': return '\b';
      
    case '\n':
      lex.current_line++;
      return '\n';
      
    case 'x':
      c=0;
      while(1)
      {
	switch(LOOK())
	{
	  default: return c;
	  case '0': case '1': case '2': case '3':
	  case '4': case '5': case '6': case '7':
	  case '8': case '9':
	    c=c*16+GETC()-'0';
	    break;
	    
	  case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
	    c=c*16+GETC()-'a'+10;
	    break;
	    
	  case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
	    c=c*16+GETC()-'A'+10;
	    break;
	}
      }

    case 'd':
      c=0;
      while(1)
      {
	switch(LOOK())
	{
	  default: return c;
	  case '0': case '1': case '2': case '3':
	  case '4': case '5': case '6': case '7':
	  case '8': case '9':
	    c=c*10+GETC()-'0';
	    break;
	}
      }
  }
  return c;
}

static struct pike_string *readstring(void)
{
  int c;
  struct string_builder tmp;
  init_string_builder(&tmp,0);
  
  while(1)
  {
    switch(c=GETC())
    {
    case 0:
      lex.pos -= (1<<SHIFT);
      yyerror("End of file in string.");
      break;
      
    case '\n':
      lex.current_line++;
      yyerror("Newline in string.");
      break;
      
    case '\\':
      string_builder_putchar(&tmp,char_const());
      continue;
      
    case '"':
      break;
      
    default:
      string_builder_putchar(&tmp,c);
      continue;
    }
    break;
  }
  return finish_string_builder(&tmp);
}

int yylex(YYSTYPE *yylval)
#if LEXDEBUG>4
{
  int t;
  int low_yylex(YYSTYPE *);
#if LEXDEBUG>8
  fprintf(stderr, "YYLEX:\n");
#endif /* LEXDEBUG>8 */

  t=low_yylex(yylval);
  if(t<256)
  {
    fprintf(stderr,"YYLEX: '%c' (%d) at %s:%d\n",t,t,lex.current_file->str,lex.current_line);
  }else{
    fprintf(stderr,"YYLEX: %s (%d) at %s:%d\n",low_get_f_name(t,0),t,lex.current_file->str,lex.current_line);
  }
  return t;
}

static int low_yylex(YYSTYPE *yylval)
#endif /* LEXDEBUG>4 */
{
  INT32 c,len;
  char *buf;

#ifdef __CHECKER__
  MEMSET((char *)yylval,0,sizeof(YYSTYPE));
#endif
#ifdef MALLOC_DEBUG
  check_sfltable();
#endif

  while(1)
  {
    switch(c = GETC())
    {
    case 0:
      lex.pos -= (1<<SHIFT);
#ifdef F_LEX_EOF
      return F_LEX_EOF;
#else /* !F_LEX_EOF */
      return 0;
#endif /* F_LEX_EOF */

    case '\n':
      lex.current_line++;
      continue;

    case 0x1b: case 0x9b:	/* ESC or CSI */
      /* Assume ANSI/DEC escape sequence.
       * Format supported:
       * 	<ESC>[\040-\077]+[\100-\177]
       * or
       * 	<CSI>[\040-\077]*[\100-\177]
       */
      while ((c = LOOK()) && (c == ((c & 0x1f)|0x20))) {
	SKIP();
      }
      if (c == ((c & 0x3f)|0x40)) {
	SKIP();
      } else {
	/* FIXME: Warning here? */
      }
      continue;

    case '#':
      SKIPSPACE();
      READBUF(C!=' ' && C!='\t' && C!='\n');

      switch(len>0?INDEX_CHARP(buf, 0, SHIFT):0)
      {
	char *p;
	
      case 'l':
	if(!ISWORD("line")) goto badhash;
	READBUF(C!=' ' && C!='\t' && C!='\n');
	
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
	lex.current_line=lex_atoi(buf)-1;
	SKIPSPACE();
	if(GOBBLE('"'))
	{
	  struct pike_string *tmp=readstring();
	  free_string(lex.current_file);
	  lex.current_file=tmp;
	}
	break;
	
      case 'e':
	if(ISWORD("error"))
	{
	  SKIPSPACE();
	  READBUF(C!='\n');
	  yyerror(buf);
	  break;
	}
	goto badhash;

      case 'p':
	if(ISWORD("pragma"))
	{
	  SKIPSPACE();
	  READBUF(C!='\n');
	  if (ISWORD("all_inline"))
	  {
	    lex.pragmas |= ID_INLINE;
	  }
	  else if (ISWORD("all_nomask"))
	  {
	    lex.pragmas |= ID_NOMASK;
	  }
	  break;
	}
	
      badhash:
	/* FIXME: This doesn't look all that safe...
	 * buf isn't NUL-terminated, and it won't work on wide strings.
	 * /grubba 1999-02-20
	 */
	if (strlen(buf) < 1024) {
	  my_yyerror("Unknown preprocessor directive #%s.",buf);
	} else {
	  my_yyerror("Unknown preprocessor directive.");
	}
	SKIPUPTO('\n');
	continue;
      }
      continue;

    case ' ':
    case '\t':
      continue;

    case '\'':
      switch(c=GETC())
      {
      case 0:
	lex.pos -= (1<<SHIFT);
	yyerror("Unexpected end of file\n");
	break;

	case '\\':
	  c = char_const();
      }
      if(!GOBBLE('\''))
	yyerror("Unterminated character constant.");
      debug_malloc_pass( yylval->n=mkintnode(c) );
      return F_NUMBER;
	
    case '"':
    {
      struct pike_string *s=readstring();
      yylval->n=mkstrnode(s);
      free_string(s);
      return F_STRING;
    }
  
    case ':':
      if(GOBBLE(':')) return F_COLON_COLON;
      return c;

    case '.':
      if(GOBBLE('.'))
      {
	if(GOBBLE('.')) return F_DOT_DOT_DOT;
	return F_DOT_DOT;
      }
      return c;
  
    case '0':
      if(GOBBLE('x') || GOBBLE('X'))
      {
	debug_malloc_pass( yylval->n=mkintnode(0) );
	wide_string_to_svalue_inumber(&yylval->n->u.sval,
				      lex.pos,
				      (void **)&lex.pos,
				      16,
				      0,
				      SHIFT);
	free_string(yylval->n->type);
	yylval->n->type=get_type_of_svalue(&yylval->n->u.sval);
	return F_NUMBER;
      }
  
    case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
    {
      char *p1, *p2;
      double f;
      long l;
      lex.pos -= (1<<SHIFT);
      if(INDEX_CHARP(lex.pos, 0, SHIFT)=='0')
	for(l=1;INDEX_CHARP(lex.pos, l, SHIFT)<='9' &&
	      INDEX_CHARP(lex.pos, l, SHIFT)>='0';l++)
	  if(INDEX_CHARP(lex.pos, l, SHIFT)>='8')
	    yyerror("Illegal octal number.");

      f=lex_strtod(lex.pos, &p1);

      debug_malloc_pass( yylval->n=mkintnode(0) );
      wide_string_to_svalue_inumber(&yylval->n->u.sval,
				    lex.pos,
				    (void **)&p2,
				    0,
				    0,
				    SHIFT);

      free_string(yylval->n->type);
      yylval->n->type=get_type_of_svalue(&yylval->n->u.sval);

      if(p1>p2)
      {
	debug_malloc_touch(yylval->n);
	free_node(yylval->n);
	lex.pos=p1;
	yylval->fnum=(FLOAT_TYPE)f;
	return F_FLOAT;
      }else{
	debug_malloc_touch(yylval->n);
	lex.pos=p2;
	return F_NUMBER;
      }
  
    case '-':
      if(GOBBLE('=')) return F_SUB_EQ;
      if(GOBBLE('>')) return F_ARROW;
      if(GOBBLE('-')) return F_DEC;
      return '-';
  
    case '+':
      if(GOBBLE('=')) return F_ADD_EQ;
      if(GOBBLE('+')) return F_INC;
      return '+';
  
    case '&':
      if(GOBBLE('=')) return F_AND_EQ;
      if(GOBBLE('&')) return F_LAND;
      return '&';
  
    case '|':
      if(GOBBLE('=')) return F_OR_EQ;
      if(GOBBLE('|')) return F_LOR;
      return '|';

    case '^':
      if(GOBBLE('=')) return F_XOR_EQ;
      return '^';
  
    case '*':
      if(GOBBLE('=')) return F_MULT_EQ;
      return '*';

    case '%':
      if(GOBBLE('=')) return F_MOD_EQ;
      return '%';
  
    case '/':
      if(GOBBLE('=')) return F_DIV_EQ;
      return '/';
  
    case '=':
      if(GOBBLE('=')) return F_EQ;
      return '=';
  
    case '<':
      if(GOBBLE('<'))
      {
	if(GOBBLE('=')) return F_LSH_EQ;
	return F_LSH;
      }
      if(GOBBLE('=')) return F_LE;
      return '<';
  
    case '>':
      if(GOBBLE(')')) return F_MULTISET_END;
      if(GOBBLE('=')) return F_GE;
      if(GOBBLE('>'))
      {
	if(GOBBLE('=')) return F_RSH_EQ;
	return F_RSH;
      }
      return '>';

    case '!':
      if(GOBBLE('=')) return F_NE;
      return F_NOT;

    case '(':
      if(GOBBLE('<')) return F_MULTISET_START;
      return '(';

    case ']':
    case '?':
    case ',':
    case '~':
    case '@':
    case ')':
    case '[':
    case '{':
    case ';':
    case '}': return c;

    case '`':
    {
      char *tmp;
      int offset=2;
      if(GOBBLE('`')) offset--;
      if(GOBBLE('`')) offset--;
      
      switch(GETC())
      {
      case '/': tmp="```/"; break;
      case '%': tmp="```%"; break;
      case '*': tmp="```*"; break;
      case '&': tmp="```&"; break;
      case '|': tmp="```|"; break;
      case '^': tmp="```^"; break;
      case '~': tmp="```~"; break;
      case '+':
	if(GOBBLE('=')) { tmp="```+="; break; }
	tmp="```+";
	break;
      case '<':
	if(GOBBLE('<')) { tmp="```<<"; break; }
	if(GOBBLE('=')) { tmp="```<="; break; }
	tmp="```<";
	break;

      case '>':
	if(GOBBLE('>')) { tmp="```>>"; break; }
	if(GOBBLE('=')) { tmp="```>="; break; }
	tmp="```>";
	break;

      case '!':
	if(GOBBLE('=')) { tmp="```!="; break; }
	tmp="```!";
	break;

      case '=':
	if(GOBBLE('=')) { tmp="```=="; break; }
	tmp="```=";
	break;

      case '(':
	if(GOBBLE(')')) 
	{
	  tmp="```()";
	  break;
	}
	yyerror("Illegal ` identifier.");
	tmp="``";
	break;
	
      case '-':
	if(GOBBLE('>'))
	{
	  tmp="```->";
	  if(GOBBLE('=')) tmp="```->=";
	}else{
	  tmp="```-";
	}
	break;

      case '[':
	if(GOBBLE(']'))
	{
	  tmp="```[]";
	  if(GOBBLE('=')) tmp="```[]=";
	  break;
	}

      default:
	yyerror("Illegal ` identifier.");
	lex.pos -= (1<<SHIFT);
	tmp="``";
	break;
      }

      {
	struct pike_string *s=make_shared_string(tmp+offset);
	yylval->n=mkstrnode(s);
	free_string(s);
	return F_IDENTIFIER;
      }
    }

  
    default:
      if(lex_isidchar(c))
      {
	struct pike_string *s;
	lex.pos -= (1<<SHIFT);
	READBUF(lex_isidchar(C));

	yylval->number=lex.current_line;

	if(len>1 && len<10)
	{
	  /* NOTE: TWO_CHAR() will generate false positives with wide strings,
	   * but that doesn't matter, since ISWORD() will fix it.
	   */
	  switch(TWO_CHAR(INDEX_CHARP(buf, 0, SHIFT),
			  INDEX_CHARP(buf, 1, SHIFT)))
	  {
	  case TWO_CHAR('a','r'):
	    if(ISWORD("array")) return F_ARRAY_ID;
	  break;
	  case TWO_CHAR('b','r'):
	    if(ISWORD("break")) return F_BREAK;
	  break;
	  case TWO_CHAR('c','a'):
	    if(ISWORD("case")) return F_CASE;
	    if(ISWORD("catch")) return F_CATCH;
	  break;
	  case TWO_CHAR('c','l'):
	    if(ISWORD("class")) return F_CLASS;
	  break;
	  case TWO_CHAR('c','o'):
	    if(ISWORD("constant")) return F_CONSTANT;
	    if(ISWORD("continue")) return F_CONTINUE;
	  break;
	  case TWO_CHAR('d','e'):
	    if(ISWORD("default")) return F_DEFAULT;
	  break;
	  case TWO_CHAR('d','o'):
	    if(ISWORD("do")) return F_DO;
	  break;
	  case TWO_CHAR('e','l'):
	    if(ISWORD("else")) return F_ELSE;
	  break;
	  case TWO_CHAR('f','i'):
	    if(ISWORD("final")) return F_FINAL_ID;
	  break;
	  case TWO_CHAR('f','l'):
	    if(ISWORD("float")) return F_FLOAT_ID;
	  break;
	  case TWO_CHAR('f','o'):
	    if(ISWORD("for")) return F_FOR;
	    if(ISWORD("foreach")) return F_FOREACH;
	  break;
	  case TWO_CHAR('f','u'):
	    if(ISWORD("function")) return F_FUNCTION_ID;
	  break;
	  case TWO_CHAR('g','a'):
	    if(ISWORD("gauge")) return F_GAUGE;
	  break;
	  case TWO_CHAR('i','f'):
	    if(ISWORD("if")) return F_IF;
	  break;
	  case TWO_CHAR('i','m'):
	    if(ISWORD("import")) return F_IMPORT;
	  break;
	  case TWO_CHAR('i','n'):
	    if(ISWORD("int")) return F_INT_ID;
	    if(ISWORD("inherit")) return F_INHERIT;
	    if(ISWORD("inline")) return F_INLINE;
	  break;
	  case TWO_CHAR('l','a'):
	    if(ISWORD("lambda")) return F_LAMBDA;
	  break;
	  case TWO_CHAR('l','o'):
	    if(ISWORD("local")) return F_LOCAL_ID;
	  break;
	  case TWO_CHAR('m','a'):
	    if(ISWORD("mapping")) return F_MAPPING_ID;
	  break;
	  case TWO_CHAR('m','i'):
	    if(ISWORD("mixed")) return F_MIXED_ID;
	  break;
	  case TWO_CHAR('m','u'):
	    if(ISWORD("multiset")) return F_MULTISET_ID;
	  break;
	  case TWO_CHAR('n','o'):
	    if(ISWORD("nomask")) return F_NO_MASK;
	  break;
	  case TWO_CHAR('o','b'):
	    if(ISWORD("object")) return F_OBJECT_ID;
	  break;
	  case TWO_CHAR('p','r'):
	    if(ISWORD("program")) return F_PROGRAM_ID;
	    if(ISWORD("predef")) return F_PREDEF;
	    if(ISWORD("private")) return F_PRIVATE;
	    if(ISWORD("protected")) return F_PROTECTED;
	    break;
	  break;
	  case TWO_CHAR('p','u'):
	    if(ISWORD("public")) return F_PUBLIC;
	  break;
	  case TWO_CHAR('r','e'):
	    if(ISWORD("return")) return F_RETURN;
	  break;
	  case TWO_CHAR('s','s'):
	    if(ISWORD("sscanf")) return F_SSCANF;
	  break;
	  case TWO_CHAR('s','t'):
	    if(ISWORD("string")) return F_STRING_ID;
	    if(ISWORD("static")) return F_STATIC;
	  break;
	  case TWO_CHAR('s','w'):
	    if(ISWORD("switch")) return F_SWITCH;
	  break;
	  case TWO_CHAR('t','y'):
	    if(ISWORD("typeof")) return F_TYPEOF;
	  break;
	  case TWO_CHAR('v','o'):
	    if(ISWORD("void")) return F_VOID_ID;
	  break;
	  case TWO_CHAR('w','h'):
	    if(ISWORD("while")) return F_WHILE;
	  break;
	  }
	}
	{
#if (SHIFT == 0)
	  struct pike_string *tmp = make_shared_binary_string(buf,len);
#else /* SHIFT != 0 */
#if (SHIFT == 1)
	  struct pike_string *tmp = make_shared_binary_string1((p_wchar1 *)buf,
							       len);
#else /* SHIFT != 1 */
	  struct pike_string *tmp = make_shared_binary_string2((p_wchar2 *)buf,
							       len);
#endif /* SHIFT == 1 */
#endif /* SHIFT == 0 */
	  yylval->n=mkstrnode(tmp);
	  free_string(tmp);
	  return F_IDENTIFIER;
	}
#if 0
      }else if (c == (c & 0x9f)) {
	/* Control character in one of the ranges \000-\037 or \200-\237 */
	/* FIXME: Warning here? */
	/* Ignore */
#endif /* 0 */
      }else{
	char buff[100];
	if ((c > 31) && (c < 256)) {
	  sprintf(buff, "Illegal character (hex %02x) '%c'", c, c);
	} else {
	  sprintf(buff, "Illegal character (hex %02x)", c);
	}
	yyerror(buff);
	return ' ';
      }
    }
    }
  }
}

/*
 * Clear the defines for the next pass
 */

#undef LOOK
#undef GETC
#undef SKIP
#undef GOBBLE
#undef SKIPSPACE
#undef SKIPWHITE
#undef SKIPUPTO
#undef READBUF
#undef TWO_CHAR
#undef ISWORD

#undef low_isword
#undef char_const
#undef readstring
#undef yylex
#undef low_yylex
#undef lex_atoi
#undef lex_strtol
#undef lex_strtod
#undef lex_isidchar
