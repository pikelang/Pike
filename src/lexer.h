/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: lexer.h,v 1.48 2004/03/24 20:29:25 grubba Exp $
*/

/*
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

/* Generic */
#define GOBBLE(c) (LOOK()==c?(SKIP(),1):0)
#define SKIPSPACE() do { while(ISSPACE(LOOK()) && LOOK()!='\n') SKIP(); }while(0)
#define SKIPWHITE() do { while(ISSPACE(LOOK())) SKIP(); }while(0)
#define SKIPUPTO(X) do { while(LOOK()!=(X) && LOOK()) SKIP(); }while(0)

#if (SHIFT == 0)

#define LOOK() EXTRACT_UCHAR(lex.pos)
#define GETC() EXTRACT_UCHAR(lex.pos++)
#define SKIP() lex.pos++

#define READBUF(X) do {				\
  register int C;				\
  buf = lex.pos;				\
  while((C = LOOK()) && (X))			\
    lex.pos++;					\
  len = (size_t)(lex.pos - buf);		\
} while(0)

#define TWO_CHAR(X,Y) ((X)<<8)+(Y)
#define ISWORD(X) ((len == strlen(X)) && !MEMCMP(buf,X,strlen(X)))

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
#define SKIP() (lex.pos += (1<<SHIFT))
#define GETC() (SKIP(),INDEX_CHARP(lex.pos-(1<<SHIFT),0,SHIFT))

#define READBUF(X) do {				\
  register int C;				\
  buf = lex.pos;				\
  while((C = LOOK()) && (X))			\
    SKIP();					\
  len = (size_t)((lex.pos - buf) >> SHIFT);	\
} while(0)

#define TWO_CHAR(X,Y) ((X)<<8)+(Y)

#define ISWORD(X) ((len == strlen(X)) && low_isword(buf, X, strlen(X)))

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

static int low_isword(char *buf, char *X, size_t len)
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
  if(end) end[0]=(char *)foo.ptr;
  return ret;
}

static double lex_strtod(char *buf, char **end)
{
  PCHARP foo;
  double ret;
  ret=STRTOD_PCHARP(MKPCHARP(buf,SHIFT),&foo);
  if(end) end[0]=(char *)foo.ptr;
  return ret;
}

#endif /* SHIFT == 0 */


/*** Lexical analyzing ***/

/* String escape sequences
 *
 * Sequence		Character
 *   \\			backslash
 *   \[0-7]*		octal escape
 *   \a			alert (BEL)
 *   \b			backspace (BS)
 *   \d[0-9]		decimal escape
 *   \e			escape (ESC)
 *   \f			form-feed (FF)
 *   \n			newline (LF)
 *   \r			carriage-return (CR)
 *   \t			tab (HT)
 *   \v			vertical-tab (VT)
 *   \x[0-9a-fA-F]*	hexadecimal escape
 */
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
      
    case 'a': return 7;		/* BEL */
    case 'b': return 8;		/* BS */
    case 't': return 9;		/* HT */
    case 'n': return 10;	/* LF */
    case 'v': return 11;	/* VT */
    case 'f': return 12;	/* FF */
    case 'r': return 13;	/* CR */
    case 'e': return 27;	/* ESC */
      
    case '\n':
      lex.current_line++;
      return '\n';
      
    case 'x':
      c=0;
      while(1)
      {
	switch(LOOK())
	{
	  case '0': case '1': case '2': case '3':
	  case '4': case '5': case '6': case '7':
	  case '8': case '9':
	    c=c*16+GETC()-'0';
	    continue;
	    
	  case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
	    c=c*16+GETC()-'a'+10;
	    continue;
	    
	  case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
	    c=c*16+GETC()-'A'+10;
	    continue;
	}
	break;
      }
      break;

    case 'd':
      c=0;
      while(1)
      {
	switch(LOOK())
	{
	  case '0': case '1': case '2': case '3':
	  case '4': case '5': case '6': case '7':
	  case '8': case '9':
	    c=c*10+GETC()-'0';
	    continue;
	}
	break;
      }
      break;
  }
  return c;
}

static struct pike_string *readstring(void)
{
  int c;
  struct string_builder tmp;
#if (SHIFT != 0)
  PCHARP bufptr = { NULL, SHIFT };
#endif /* SHIFT != 0 */

  init_string_builder(&tmp,0);
  
  while(1)
  {
    char *buf;
    size_t len;

    READBUF(((C > '\\') || ((C != '"') && (C != '\\') && (C != '\n'))));
    if (len) {
#if (SHIFT == 0)
      string_builder_binary_strcat(&tmp, buf, len);
#else /* SHIFT != 0 */
      bufptr.ptr = (p_wchar0 *)buf;
      string_builder_append(&tmp, bufptr, len);
#endif /* SHIFT == 0 */
    }
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
#ifdef PIKE_DEBUG
      Pike_fatal("Default case in readstring() reached. c:%d\n", c);
#endif /* PIKE_DEBUG */
      break;
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
  INT32 c;
  size_t len;
  char *buf;

#ifdef __CHECKER__
  MEMSET((char *)yylval,0,sizeof(YYSTYPE));
#endif
#ifdef MALLOC_DEBUG
  check_sfltable();
#endif

  while(1)
  {
    c = GETC();

    if((c>'9') && lex_isidchar(c))
    {
      lex.pos -= (1<<SHIFT);
      READBUF(lex_isidchar(C));

      PIKE_MEM_WO_RANGE (yylval, sizeof (YYSTYPE));

      if(len>1 && len<10)
      {
	/* NOTE: TWO_CHAR() will generate false positives with wide strings,
	 * but that doesn't matter, since ISWORD() will fix it.
	 */
	switch(TWO_CHAR(INDEX_CHARP(buf, 0, SHIFT),
			INDEX_CHARP(buf, 1, SHIFT)))
	{
	case TWO_CHAR('a','r'):
	  if(ISWORD("array")) return TOK_ARRAY_ID;
	  break;
	case TWO_CHAR('b','r'):
	  if(ISWORD("break")) return TOK_BREAK;
	  break;
	case TWO_CHAR('c','a'):
	  if(ISWORD("case")) return TOK_CASE;
	  if(ISWORD("catch")) return TOK_CATCH;
	  break;
	case TWO_CHAR('c','l'):
	  if(ISWORD("class")) return TOK_CLASS;
	  break;
	case TWO_CHAR('c','o'):
	  if(ISWORD("constant")) return TOK_CONSTANT;
	  if(ISWORD("continue")) return TOK_CONTINUE;
	  break;
	case TWO_CHAR('d','e'):
	  if(ISWORD("default")) return TOK_DEFAULT;
	  break;
	case TWO_CHAR('d','o'):
	  if(ISWORD("do")) return TOK_DO;
	  break;
	case TWO_CHAR('e','l'):
	  if(ISWORD("else")) return TOK_ELSE;
	  break;
	case TWO_CHAR('e','n'):
	  if(ISWORD("enum")) return TOK_ENUM;
	  break;
	case TWO_CHAR('e','x'):
	  if(ISWORD("extern")) return TOK_EXTERN;
	  break;
	case TWO_CHAR('f','i'):
	  if(ISWORD("final")) return TOK_FINAL_ID;
	  break;
	case TWO_CHAR('f','l'):
	  if(ISWORD("float")) return TOK_FLOAT_ID;
	  break;
	case TWO_CHAR('f','o'):
	  if(ISWORD("for")) return TOK_FOR;
	  if(ISWORD("foreach")) return TOK_FOREACH;
	  break;
	case TWO_CHAR('f','u'):
	  if(ISWORD("function")) return TOK_FUNCTION_ID;
	  break;
	case TWO_CHAR('g','a'):
	  if(ISWORD("gauge")) return TOK_GAUGE;
	  break;
	case TWO_CHAR('g','l'):
	  if (ISWORD("global") && !TEST_COMPAT(7,2)) return TOK_GLOBAL;
	  break;
	case TWO_CHAR('i','f'):
	  if(ISWORD("if")) return TOK_IF;
	  break;
	case TWO_CHAR('i','m'):
	  if(ISWORD("import")) return TOK_IMPORT;
	  break;
	case TWO_CHAR('i','n'):
	  if(ISWORD("int")) return TOK_INT_ID;
	  if(ISWORD("inherit")) return TOK_INHERIT;
	  if(ISWORD("inline")) return TOK_INLINE;
	  break;
	case TWO_CHAR('l','a'):
	  if(ISWORD("lambda")) return TOK_LAMBDA;
	  break;
	case TWO_CHAR('l','o'):
	  if(ISWORD("local")) return TOK_LOCAL_ID;
	  break;
	case TWO_CHAR('m','a'):
	  if(ISWORD("mapping")) return TOK_MAPPING_ID;
	  break;
	case TWO_CHAR('m','i'):
	  if(ISWORD("mixed")) return TOK_MIXED_ID;
	  break;
	case TWO_CHAR('m','u'):
	  if(ISWORD("multiset")) return TOK_MULTISET_ID;
	  break;
	case TWO_CHAR('n','o'):
	  if(ISWORD("nomask")) return TOK_NO_MASK;
	  break;
	case TWO_CHAR('o','b'):
	  if(ISWORD("object")) return TOK_OBJECT_ID;
	  break;
	case TWO_CHAR('o','p'):
	  if(ISWORD("optional")) return TOK_OPTIONAL;
	  break;
	case TWO_CHAR('p','r'):
	  if(ISWORD("program")) return TOK_PROGRAM_ID;
	  if(ISWORD("predef")) return TOK_PREDEF;
	  if(ISWORD("private")) return TOK_PRIVATE;
	  if(ISWORD("protected")) return TOK_PROTECTED;
	  break;
	case TWO_CHAR('p','u'):
	  if(ISWORD("public")) return TOK_PUBLIC;
	  break;
	case TWO_CHAR('r','e'):
	  if(ISWORD("return")) return TOK_RETURN;
	  break;
	case TWO_CHAR('s','s'):
	  if(ISWORD("sscanf")) return TOK_SSCANF;
	  break;
	case TWO_CHAR('s','t'):
	  if(ISWORD("string")) return TOK_STRING_ID;
	  if(ISWORD("static")) return TOK_STATIC;
	  break;
	case TWO_CHAR('s','w'):
	  if(ISWORD("switch")) return TOK_SWITCH;
	  break;
	case TWO_CHAR('t','y'):
	  if(ISWORD("typedef")) return TOK_TYPEDEF;
	  if(ISWORD("typeof")) return TOK_TYPEOF;
	  break;
	case TWO_CHAR('v','a'):
	  if(ISWORD("variant")) return TOK_VARIANT;
	  break;
	case TWO_CHAR('v','o'):
	  if(ISWORD("void")) return TOK_VOID_ID;
	  break;
	case TWO_CHAR('w','h'):
	  if(ISWORD("while")) return TOK_WHILE;
	  break;
	}
      }
      {
#if (SHIFT == 0)
	struct pike_string *tmp = make_shared_binary_string(buf, len);
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
	return TOK_IDENTIFIER;
      }
    }

    /* Note that 0 <= c <= 255 at this point. */

    switch(c)
    {
    case 0:
      lex.pos -= (1<<SHIFT);
      if(lex.end != lex.pos)
	yyerror("Illegal character (NUL)");

#ifdef TOK_LEX_EOF
      return TOK_LEX_EOF;
#else /* !TOK_LEX_EOF */
      return 0;
#endif /* TOK_LEX_EOF */

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
      READBUF(C!='\n' && C!=' ' && C!='\t');

      switch(len>0?INDEX_CHARP(buf, 0, SHIFT):0)
      {
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
	if (Pike_compiler->compiler_pass == 1 &&
	    !Pike_compiler->new_program->num_linenumbers) {
	  /* A nested program will always get an entry right away in
	   * language.yacc. */
	  store_linenumber(0, lex.current_file);
#ifdef DEBUG_MALLOC
	  if(strcmp(lex.current_file->str,"-"))
	    debug_malloc_name(Pike_compiler->new_program, lex.current_file->str, 0);
#endif
	}
	break;

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
	  else if (ISWORD("strict_types"))
	  {
	    lex.pragmas |= ID_STRICT_TYPES;
	  }
	  else if (ISWORD("save_parent"))
	  {
	    lex.pragmas |= ID_SAVE_PARENT;
	  }
	  else if (ISWORD("dont_save_parent"))
	  {
	    lex.pragmas |= ID_DONT_SAVE_PARENT;
	  }
	  break;
	}

	if(ISWORD("pike"))
	{
	  int minor;
	  int major;
	  SKIPSPACE();
	  READBUF(C!='.' && C!='\n');
	  major=lex_atoi(buf);
	  if(!GOBBLE('.'))
	  {
	    yyerror("Missing '.' in #pike directive.");
	    minor=0;
	  }else{
	    READBUF(C!='\n' && C!='.');
	    minor=lex_atoi(buf);
	    if(GOBBLE('.'))
	      yyerror("Build numbers not supported in #pike directive.");

	    READBUF(C!='\n');
	    change_compiler_compatibility(major, minor);
	  }
	  break;
	}
	/* FALL_THROUGH */

      default:
	/* FIXME: This doesn't look all that safe...
	 * buf isn't NUL-terminated, and it won't work on wide strings.
	 * /grubba 1999-02-20
	 * It also modified a shared string... ie. this code was not
	 * good at all. Fixed.
	 * /grubba 2000-11-19 (in Versailles)
	 */
	if (len < 256) {
#if (SHIFT == 0)
	  struct pike_string *dir =
	    make_shared_binary_string(buf, len);
#else /* SHIFT != 0 */
#if (SHIFT == 1)
	  struct pike_string *dir =
	    make_shared_binary_string1((p_wchar1 *)buf, len);
#else /* SHIFT != 1 */
#if (SHIFT == 2)
	  struct pike_string *dir =
	    make_shared_binary_string2((p_wchar2 *)buf, len);
#else /* SHIFT != 2 */
#error Unsupported SHIFT.
#endif /* SHIFT == 2 */
#endif /* SHIFT == 1 */
	  if (!dir->size_shift) {
#endif /* SHIFT == 0 */
	    my_yyerror("Unknown preprocessor directive #%s.", dir->str);
#if (SHIFT != 0)
	  } else {
	    yyerror("Unknown preprocessor directive.");
	  }
#endif /* SHIFT != 0 */
	  free_string(dir);
	} else {
	  yyerror("Unknown preprocessor directive.");
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
	break;

      case '\'':
	yyerror("Zero-length character constant.");
	debug_malloc_pass( yylval->n=mkintnode(0) );
	return TOK_NUMBER;
      }
      if(!GOBBLE('\''))
	yyerror("Unterminated character constant.");
      debug_malloc_pass( yylval->n=mkintnode(c) );
      return TOK_NUMBER;
	
    case '"':
    {
      struct pike_string *s=readstring();
      yylval->n=mkstrnode(s);
      free_string(s);
      return TOK_STRING;
    }
  
    case ':':
      if(GOBBLE(':')) return TOK_COLON_COLON;
      return c;

    case '.':
      if(GOBBLE('.'))
      {
	if(GOBBLE('.')) return TOK_DOT_DOT_DOT;
	return TOK_DOT_DOT;
      }
      return c;
  
    case '0':
    {
      int base = 0;
      
      if(GOBBLE('b') || GOBBLE('B'))
      {
	base = 2;
	goto read_based_number;
      }
      else if(GOBBLE('x') || GOBBLE('X'))
      {
	struct svalue sval;
	base = 16;
      read_based_number:
	sval.type = PIKE_T_INT;
	sval.subtype = NUMBER_NUMBER;
	sval.u.integer = 0;
	wide_string_to_svalue_inumber(&sval,
				      lex.pos,
				      (void **)&lex.pos,
				      base,
				      0,
				      SHIFT);
	dmalloc_touch_svalue(&sval);
	yylval->n = mksvaluenode(&sval);
	free_svalue(&sval);
	return TOK_NUMBER;
      }
    }
  
    case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
    {
      char *p1, *p2;
      double f;
      long l = 0;
      struct svalue sval;

      lex.pos -= (1<<SHIFT);
      if(INDEX_CHARP(lex.pos, 0, SHIFT)=='0')
	for(l=1;INDEX_CHARP(lex.pos, l, SHIFT)<='9' &&
	      INDEX_CHARP(lex.pos, l, SHIFT)>='0';l++)
	  if(INDEX_CHARP(lex.pos, l, SHIFT)>='8')
	    my_yyerror("Illegal octal digit '%c'.",
		       INDEX_CHARP(lex.pos, l, SHIFT));

      f=lex_strtod(lex.pos, &p1);

      sval.type = PIKE_T_INT;
      sval.subtype = NUMBER_NUMBER;
      sval.u.integer = 0;      

      wide_string_to_svalue_inumber(&sval,
				    lex.pos,
				    &p2,
				    0,
				    0,
				    SHIFT);
      dmalloc_touch_svalue(&sval);
      yylval->n = mksvaluenode(&sval);
      free_svalue(&sval);

      if(p1>p2)
      {
	debug_malloc_touch(yylval->n);
	free_node(yylval->n);
	yylval->fnum=(FLOAT_TYPE)f;
#if 0
	fprintf(stderr, "LEX: \"%.8s\" => %f, %f\n",
		(char *)lex.pos, f, yylval->fnum);
#endif /* 0 */
	lex.pos=p1;
	if (lex_isidchar (LOOK())) {
	  my_yyerror ("Invalid char '%c' in constant.",
		      INDEX_CHARP (lex.pos, l, SHIFT));
	  do SKIP(); while (lex_isidchar (LOOK()));
	}
	return TOK_FLOAT;
      }else{
	debug_malloc_touch(yylval->n);
	lex.pos=p2;
	if (lex_isidchar (LOOK())) {
	  my_yyerror ("Invalid char '%c' in constant.",
		      INDEX_CHARP (lex.pos, l, SHIFT));
	  do SKIP(); while (lex_isidchar (LOOK()));
	}
	return TOK_NUMBER;
      }
    }

    case '-':
      if(GOBBLE('=')) return TOK_SUB_EQ;
      if(GOBBLE('>')) return TOK_ARROW;
      if(GOBBLE('-')) return TOK_DEC;
      return '-';
  
    case '+':
      if(GOBBLE('=')) return TOK_ADD_EQ;
      if(GOBBLE('+')) return TOK_INC;
      return '+';
  
    case '&':
      if(GOBBLE('=')) return TOK_AND_EQ;
      if(GOBBLE('&')) return TOK_LAND;
      return '&';
  
    case '|':
      if(GOBBLE('=')) return TOK_OR_EQ;
      if(GOBBLE('|')) return TOK_LOR;
      return '|';

    case '^':
      if(GOBBLE('=')) return TOK_XOR_EQ;
      return '^';
  
    case '*':
      if(GOBBLE('=')) return TOK_MULT_EQ;
      return '*';

    case '%':
      if(GOBBLE('=')) return TOK_MOD_EQ;
      return '%';
  
    case '/':
      if(GOBBLE('=')) return TOK_DIV_EQ;
      return '/';
  
    case '=':
      if(GOBBLE('=')) return TOK_EQ;
      return '=';
  
    case '<':
      if(GOBBLE('<'))
      {
	if(GOBBLE('=')) return TOK_LSH_EQ;
	return TOK_LSH;
      }
      if(GOBBLE('=')) return TOK_LE;
      return '<';
  
    case '>':
      if(GOBBLE(')')) return TOK_MULTISET_END;
      if(GOBBLE('=')) return TOK_GE;
      if(GOBBLE('>'))
      {
	if(GOBBLE('=')) return TOK_RSH_EQ;
	return TOK_RSH;
      }
      return '>';

    case '!':
      if(GOBBLE('=')) return TOK_NE;
      return TOK_NOT;

    case '(':
      if(GOBBLE('<')) return TOK_MULTISET_START;
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
      if(GOBBLE('`')) {
	offset--;
	if(GOBBLE('`')) {
	  offset--;
	}
      }
      
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
	tmp="```()";
	if(GOBBLE(')')) 
	{
	  break;
	}
	yyerror("Illegal ` identifier. Expected `().");
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
	tmp="```[]";
	if(GOBBLE(']'))
	{
	  if(GOBBLE('=')) tmp="```[]=";
	  break;
	}
	yyerror("Illegal ` identifier. Expected `[].");
	break;

      default:
	yyerror("Illegal ` identifier.");
	lex.pos -= (1<<SHIFT);
	tmp="```";
	break;
      }

      {
	struct pike_string *s=make_shared_string(tmp+offset);
	yylval->n=mkstrnode(s);
	free_string(s);
	return TOK_IDENTIFIER;
      }
    }

  
    default:
      {
	if (c > 31) {
	  my_yyerror("Illegal character (hex %02x) '%c'", c, c);
	} else {
	  my_yyerror("Illegal character (hex %02x)", c);
	}
	return ' ';
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
