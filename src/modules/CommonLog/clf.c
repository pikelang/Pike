/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: clf.c,v 1.15 2004/03/05 23:03:26 nilsson Exp $
*/

/* MUST BE FIRST */
#include "global.h"
RCSID("$Id: clf.c,v 1.15 2004/03/05 23:03:26 nilsson Exp $");
#include "fdlib.h"
#include "stralloc.h"
#include "program.h"
#include "interpret.h"
#include "builtin_functions.h"
#include "module_support.h"
#include "pike_error.h"
#include "bignum.h"
#include "security.h"

#include "threads.h"
#include <stdio.h>
#include <fcntl.h>


#define sp Pike_sp

/** Forward declarations of functions implementing Pike functions **/

static void f_read( INT32 args );


/** Global tables **/

#ifndef CHAR_BIT
#define CHAR_BIT	9	/* Should be safe for most hardware */
#endif /* !CHAR_BIT */

static char char_class[1<<CHAR_BIT];

#define CLS_WSPACE 0
#define CLS_CRLF   1
#define CLS_TOKEN  2
#define CLS_DIGIT  3
#define CLS_QUOTE  4
#define CLS_LBRACK 5
#define CLS_RBRACK 6
#define CLS_SLASH  7
#define CLS_COLON  8
#define CLS_HYPHEN 9
#define CLS_PLUS   10


/* How much to read each round in the loop. */
#define CLF_BLOCK_SIZE 4096

/** Externally available functions **/

/* Initialize and start module */

PIKE_MODULE_INIT
{
  int i;

  MEMSET(char_class, CLS_TOKEN, sizeof(char_class));
  for(i='\0'; i<=' '; i++)
    char_class[i] = CLS_WSPACE;
  for(i='0'; i<='9'; i++)
    char_class[i] = CLS_DIGIT;
  char_class['\n'] = CLS_CRLF;
  char_class['\r'] = CLS_CRLF;
  char_class['"'] = CLS_QUOTE;
  char_class['['] = CLS_LBRACK;
  char_class[']'] = CLS_RBRACK;
  char_class['/'] = CLS_SLASH;
  char_class[':'] = CLS_COLON;
  char_class['-'] = CLS_HYPHEN;
  char_class['+'] = CLS_PLUS;

  add_function_constant( "read", f_read,
			 "function(function(array(string|int),int|void:void),"
			 "string|object,int|void:int)", 0 );
}


/* Restore and exit module */

PIKE_MODULE_EXIT
{
}


/** Functions implementing Pike functions **/

/*! @module CommonLog
 *!
 *! The CommonLog module is used to parse the lines in a www server's logfile,
 *! which must be in "common log" format -- such as used by default for the
 *! access log by Roxen, Caudium, Apache et al.
 */

/*! @decl int read(function(array(int|string), int : void ) callback,@
 *!            Stdio.File|string logfile, void|int offset)
 *!
 *! Reads the log file and calls the callback function for every parsed line.
 *! For lines that fails to be parsed the callback is not called not is any
 *! error thrown. The number of bytes read are returned.
 *!
 *! @param callback
 *! The callbacks first argument is an array with the different parts of the
 *! log entry.
 *! @array
 *!   @elem string remote_host
 *!
 *!   @elem int(0..0)|string ident_user
 *!
 *!   @elem int(0..0)|string auth_user
 *!
 *!   @elem int year
 *!
 *!   @elem int month
 *!
 *!   @elem int day
 *!
 *!   @elem int hours
 *!
 *!   @elem int minutes
 *!
 *!   @elem int seconds
 *!
 *!   @elem int timezone
 *!
 *!   @elem int(0..0)|string method
 *!     One of "GET", "POST", "HEAD" etc.
 *!   @elem int(0..0)|string path
 *!
 *!   @elem string protocol
 *!     E.g. "HTTP/1.0"
 *!   @elem int reply_code
 *!     One of 200, 404 etc.
 *!   @elem int bytes
 *! @endarray
 *!
 *! The second callback argument is the current offset to the end of the
 *! current line.
 *!
 *! @param offset
 *! The position in the file where the parser should begin.
 */

static void f_read( INT32 args )
{
  char *read_buf;
  struct svalue *logfun, *file;
  FD f = -1;
  int cls, c, my_fd=1, state=0, tzs=0;
  char *char_pointer;
  INT32 v=0, yy=0, mm=0, dd=0, h=0, m=0, s=0, tz=0;
  ptrdiff_t offs0=0, len=0;
  struct svalue *old_sp;
  /* #define DYNAMIC_BUF */
#ifdef DYNAMIC_BUF
  dynamic_buffer buf;
#else
#define BUFSET(X) do { if(bufpos == bufsize) { bufsize *= 2; buf = realloc(buf, bufsize+1); } buf[bufpos++] = c; } while(0)
#define PUSHBUF() do { push_string( make_shared_binary_string( buf,bufpos ) ); bufpos=0; } while(0)
  char *buf;
  int bufsize=CLF_BLOCK_SIZE, bufpos=0;
#endif

  if(args>2 && sp[-1].type == T_INT) {
    offs0 = sp[-1].u.integer;
    pop_n_elems(1);
    --args;
  }
  old_sp = sp;

  get_all_args("CommonLog.read", args, "%*%*", &logfun, &file);
  if(logfun->type != T_FUNCTION)
    SIMPLE_BAD_ARG_ERROR("CommonLog.read", 1, "function");

  if(file->type == T_OBJECT)
  {
    f = fd_from_object(file->u.object);
    
    if(f == -1)
      Pike_error("CommonLog.read: File is not open.\n");
    my_fd = 0;
  } else if(file->type == T_STRING &&
	    file->u.string->size_shift == 0) {
#ifdef PIKE_SECURITY
      if(!CHECK_SECURITY(SECURITY_BIT_SECURITY))
      {
	if(!CHECK_SECURITY(SECURITY_BIT_CONDITIONAL_IO))
	  Pike_error("Permission denied.\n");
	push_text("read");
	push_int(0);
	ref_push_string(file->u.string);
	push_text("r");
	push_int(00666);

	safe_apply(OBJ2CREDS(CURRENT_CREDS)->user,"valid_open",5);
	switch(Pike_sp[-1].type)
	{
	case PIKE_T_INT:
	  switch(Pike_sp[-1].u.integer)
	  {
	  case 0: /* return 0 */
	    errno=EPERM;
	    Pike_error("CommonLog.read(): Failed to open file for reading (errno=%d).\n",
		       errno);

	  case 2: /* ok */
	    pop_stack();
	    break;

	  case 3: /* permission denied */
	    Pike_error("CommonLog.read: permission denied.\n");

	  default:
	    Pike_error("Error in user->valid_open, wrong return value.\n");
	  }
	  break;

	default:
	  Pike_error("Error in user->valid_open, wrong return type.\n");

	case PIKE_T_STRING:
	  /*	  if(Pike_sp[-1].u.string->shift_size) */
	  /*	    file=Pike_sp[-1]; */
	  pop_stack();
	}

      }
#endif
    do {
      THREADS_ALLOW();
      f=fd_open((char *)STR0(file->u.string), fd_RDONLY, 0);
      THREADS_DISALLOW();
      if (f >= 0 || errno != EINTR) break;
      check_threads_etc();
    } while (1);

    if(errno < 0)
      Pike_error("CommonLog.read(): Failed to open file for reading (errno=%d).\n",
	    errno);
  } else
    SIMPLE_BAD_ARG_ERROR("CommonLog.read", 2, "string|Stdio.File");

#ifdef HAVE_LSEEK64
  lseek64(f, offs0, SEEK_SET);
#else
  fd_lseek(f, offs0, SEEK_SET);
#endif
  read_buf = malloc(CLF_BLOCK_SIZE+1);
#ifndef DYNAMIC_BUF
  buf = malloc(bufsize);
#endif
  while(1) {
    do {
      THREADS_ALLOW();
      len = fd_read(f, read_buf, CLF_BLOCK_SIZE);
      THREADS_DISALLOW();
      if (len >= 0 || errno != EINTR) break;
      check_threads_etc();
    } while (1);
    if(len == 0)
      break; /* nothing more to read. */
    if(len < 0)
      break;
    char_pointer = read_buf;
    while(len--) {
      offs0++;
      c = char_pointer[0];
      char_pointer ++;
      cls = char_class[c];
#ifdef TRACE_DFA
      fprintf(stderr, "DFA(%d): '%c' ", state, (c<32? '.':c));
      switch(cls) {
      case CLS_WSPACE: fprintf(stderr, "CLS_WSPACE"); break;
      case CLS_CRLF: fprintf(stderr, "CLS_CRLF"); break;
      case CLS_TOKEN: fprintf(stderr, "CLS_TOKEN"); break;
      case CLS_DIGIT: fprintf(stderr, "CLS_DIGIT"); break;
      case CLS_QUOTE: fprintf(stderr, "CLS_QUOTE"); break;
      case CLS_LBRACK: fprintf(stderr, "CLS_LBRACK"); break;
      case CLS_RBRACK: fprintf(stderr, "CLS_RBRACK"); break;
      case CLS_SLASH: fprintf(stderr, "CLS_SLASH"); break;
      case CLS_COLON: fprintf(stderr, "CLS_COLON"); break;
      case CLS_HYPHEN: fprintf(stderr, "CLS_HYPHEN"); break;
      case CLS_PLUS: fprintf(stderr, "CLS_PLUS"); break;
      default: fprintf(stderr, "???");
      }
      fprintf(stderr, " %d items on stack\n", sp-old_sp);
#endif
      switch(state) {
      case 0:
	if(sp != old_sp) {
	  if(sp == old_sp+15) {
	    f_aggregate(15);
	    push_int64(offs0);
	    apply_svalue(logfun, 2);
	    pop_stack();
	  } else
	    pop_n_elems(sp-old_sp);
	}
	if(cls > CLS_CRLF) {
	  if(cls == CLS_HYPHEN) {
	    push_int(0);
	    state = 2;
	    break;
	  }
#ifdef DYNAMIC_BUF
	  buf.s.str = NULL;
	  initialize_buf( &buf );
	  low_my_putchar( c, &buf );
#else
	  bufpos = 0;
	  BUFSET(c);
#endif
	  state=1;
	}
	break;
      case 1:
	if(cls > CLS_CRLF) {
#ifdef DYNAMIC_BUF
	  low_my_putchar( c, &buf );
#else
	  BUFSET(c);
#endif
	  break;
	}
#ifdef DYNAMIC_BUF
	push_string( low_free_buf( &buf ) ); /* remotehost */
#else
	PUSHBUF();
#endif
	state = (cls == CLS_WSPACE? 2:0);
	break;
      case 2:
	if(cls > CLS_CRLF) {
	  if(cls == CLS_HYPHEN) {
	    push_int(0);
	    state = 4;
	    break;
	  }
#ifdef DYNAMIC_BUF
	  buf.s.str = NULL;
	  initialize_buf( &buf );
	  low_my_putchar( c, &buf );
#else
	  bufpos = 0;
	  BUFSET(c);
#endif

	  state=3;
	} else if(cls == CLS_CRLF)
	  state=0;
	break;
      case 3:
	if(cls > CLS_CRLF) {
#ifdef DYNAMIC_BUF
	  low_my_putchar( c, &buf );
#else
	  BUFSET(c);
#endif

	  break;
	}
#ifdef DYNAMIC_BUF
	push_string( low_free_buf( &buf ) ); /* rfc931 */
#else
	PUSHBUF(); /* rfc931 */
#endif
	state = (cls == CLS_WSPACE? 4:0);
	break;
      case 4:
	if(cls > CLS_CRLF) {
	  if(cls == CLS_HYPHEN) {
	    push_int(0);
	    state = 6;
	    break;
	  }
#ifdef DYNAMIC_BUF
	  buf.s.str = NULL;
	  initialize_buf( &buf );
	  low_my_putchar( c, &buf );
#else
	  bufpos = 0;
	  BUFSET(c);
#endif

	  state=5;
	} else if(cls == CLS_CRLF)
	  state=0;
	break;
      case 5:
	if(cls > CLS_CRLF) {
#ifdef DYNAMIC_BUF
	  low_my_putchar( c, &buf );
#else
	  BUFSET(c);
#endif

	  break;
	}
#ifdef DYNAMIC_BUF
	push_string( low_free_buf( &buf ) ); /* authuser */
#else
	PUSHBUF(); /* authuser */
#endif
	state = (cls == CLS_WSPACE? 6:0);
	break;
      case 6:
	if(cls == CLS_LBRACK)
	  state = 15;
	else if(cls == CLS_CRLF)
	  state = 0;
	else if(cls == CLS_HYPHEN) {
	  push_int(0);
	  push_int(0);
	  push_int(0);
	  state = 7;
	}
	break;
      case 7:
	if(cls == CLS_QUOTE) {
#ifdef DYNAMIC_BUF
	  buf.s.str = NULL;
	  initialize_buf( &buf );
#else
	  bufpos = 0;
#endif
	  state = 31;
	} else if(cls == CLS_CRLF)
	  state = 0;
	else if(cls == CLS_HYPHEN) {
	  push_int(0);
	  push_int(0);
	  push_int(0);
	  state = 10;
	}
	break;
      case 8:
	if(cls == CLS_QUOTE)
	  state = 9;
	else if(cls == CLS_CRLF) {
#ifdef DYNAMIC_BUF
	  push_string( low_free_buf( &buf ) );
#else
	  PUSHBUF();
#endif
	  state = 0;
	} else
#ifdef DYNAMIC_BUF
	  low_my_putchar( c, &buf );
#else
	  BUFSET(c);
#endif

	break;
      case 9:
	if(cls > CLS_CRLF) {
#ifdef DYNAMIC_BUF
	  low_my_putchar( '"', &buf);
	  low_my_putchar( c, &buf);
#else
	  BUFSET('"');
	  BUFSET(c);
#endif
	  state = 8;
	  break;
	}
#ifdef DYNAMIC_BUF
	push_string( low_free_buf( &buf ) ); /* protocol */
#else
	PUSHBUF(); /* protoocl */
#endif
	state = (cls == CLS_CRLF? 0 : 10);
	break;
      case 10:
	if(cls == CLS_DIGIT) {
	  v = c&0xf;
	  state = 11;
	} else if(cls == CLS_CRLF)
	  state = 0;
	else if(cls == CLS_HYPHEN) {
	  push_int(0);
	  state = 12;
	}
	break;
      case 11:
	if(cls == CLS_DIGIT)
	  v = v*10+(c&0xf);
	else if(cls == CLS_WSPACE) {
	  push_int(v); /* status */
	  state = 12;
	} else state = 0;
	break;
      case 12:
	if(cls == CLS_DIGIT) {
	  v = c&0xf;
	  state = 13;
	} else if(cls == CLS_CRLF)
	  state = 0;
	else if(cls == CLS_HYPHEN) {
	  push_int(0);
	  state = 14;
	}
	break;
      case 13:
	if(cls == CLS_DIGIT)
	  v = v*10+(c&0xf);
	else {
	  push_int(v); /* bytes */
	  state = (cls == CLS_CRLF? 0:14);
	}
	break;
      case 14:
	if(cls == CLS_CRLF)
	  state = 0;
	break;

      case 15:
	if(cls == CLS_DIGIT) {
	  dd = c&0xf;
	  state = 16;
	} else
	  state = (cls == CLS_CRLF? 0:14);
	break;
      case 16:
	/* getting day */
	if(cls == CLS_DIGIT)
	  dd = dd*10+(c&0xf);
	else if(cls == CLS_SLASH)
	  state = 17;
	else
	  state = (cls == CLS_CRLF? 0:14);
	break;
      case 17:
	if(cls == CLS_DIGIT) {
	  mm = c&0xf;
	  state = 18;
	} else if(cls == CLS_TOKEN) {
	  mm = c|0x20;
	  state = 21;
	} else
	  state = (cls == CLS_CRLF? 0:14);
	break;
      case 18:
	/* getting numeric month */
	if(cls == CLS_DIGIT)
	  mm = mm*10+(c&0xf);
	else if(cls == CLS_SLASH)
	  state = 19;
	else
	  state = (cls == CLS_CRLF? 0:14);
	break;
      case 19:
	if(cls == CLS_DIGIT) {
	  yy = c&0xf;
	  state = 20;
	} else
	  state = (cls == CLS_CRLF? 0:14);
	break;
      case 20:
	/* getting year */
	if(cls == CLS_DIGIT)
	  yy = yy*10+(c&0xf);
	else if(cls == CLS_COLON)
	  state = 22;
	else
	  state = (cls == CLS_CRLF? 0:14);
	break;
      case 21:
	/* getting textual month */
	if(cls == CLS_TOKEN)
	  mm = (mm<<8)|c|0x20;
	else if(cls == CLS_SLASH) {
	  state = 19;
	  switch(mm) {
	  case ('j'<<16)|('a'<<8)|'n': mm=1; break;
	  case ('f'<<16)|('e'<<8)|'b': mm=2; break;
	  case ('m'<<16)|('a'<<8)|'r': mm=3; break;
	  case ('a'<<16)|('p'<<8)|'r': mm=4; break;
	  case ('m'<<16)|('a'<<8)|'y': mm=5; break;
	  case ('j'<<16)|('u'<<8)|'n': mm=6; break;
	  case ('j'<<16)|('u'<<8)|'l': mm=7; break;
	  case ('a'<<16)|('u'<<8)|'g': mm=8; break;
	  case ('s'<<16)|('e'<<8)|'p': mm=9; break;
	  case ('o'<<16)|('c'<<8)|'t': mm=10; break;
	  case ('n'<<16)|('o'<<8)|'v': mm=11; break;
	  case ('d'<<16)|('e'<<8)|'c': mm=12; break;
	  default:
	    state = 14;
	  }
	}
	break;
      case 22:
	if(cls == CLS_DIGIT) {
	  h = c&0xf;
	  state = 23;
	} else
	  state = (cls == CLS_CRLF? 0:14);
	break;
      case 23:
	/* getting hour */
	if(cls == CLS_DIGIT)
	  h = h*10+(c&0xf);
	else if(cls == CLS_COLON)
	  state = 24;
	else
	  state = (cls == CLS_CRLF? 0:14);
	break;
      case 24:
	if(cls == CLS_DIGIT) {
	  m = c&0xf;
	  state = 25;
	} else
	  state = (cls == CLS_CRLF? 0:14);
	break;
      case 25:
	/* getting minute */
	if(cls == CLS_DIGIT)
	  m = m*10+(c&0xf);
	else if(cls == CLS_COLON)
	  state = 26;
	else
	  state = (cls == CLS_CRLF? 0:14);
	break;
      case 26:
	if(cls == CLS_DIGIT) {
	  s = c&0xf;
	  state = 27;
	} else
	  state = (cls == CLS_CRLF? 0:14);
	break;
      case 27:
	/* getting second */
	if(cls == CLS_DIGIT)
	  s = s*10+(c&0xf);
	else if(cls == CLS_WSPACE)
	  state = 28;
	else
	  state = (cls == CLS_CRLF? 0:14);
	break;
      case 28:
	if(cls>=CLS_HYPHEN) {
	  state = 29;
	  tzs = cls!=CLS_PLUS;
	  tz = 0;
	} else if(cls == CLS_DIGIT) {
	  state = 29;
	  tzs = 0;
	  tz = c&0xf;
	} else if(cls==CLS_CRLF)
	  state = 0;
	break;
      case 29:
	/* getting timezone */
	if(cls == CLS_DIGIT)
	  tz = tz*10+(c&0xf);
	else {
	  if(tzs)
	    tz = -tz;
	  push_int(yy);
	  push_int(mm);
	  push_int(dd);
	  push_int(h);
	  push_int(m);
	  push_int(s);
	  push_int(tz);
	  if(cls == CLS_RBRACK)
	    state = 7;
	  else
	    state = (cls == CLS_CRLF? 0 : 30);
	}
	break;
      case 30:
	if(cls == CLS_RBRACK)
	  state = 7;
	else if(cls == CLS_CRLF)
	  state = 0;
	break;
      case 31:
	if(cls == CLS_QUOTE) {
#ifdef DYNAMIC_BUF
	  push_string( low_free_buf( &buf ) );
#else
	  PUSHBUF();
#endif
	  push_int(0);
	  push_int(0);
	  state = 10;
	} else if(cls >= CLS_TOKEN)
#ifdef DYNAMIC_BUF
	  low_my_putchar( c, &buf );
#else
	  BUFSET(c);
#endif

	else {
#ifdef DYNAMIC_BUF
	  push_string( low_free_buf( &buf ) ); /* method */
#else
	  PUSHBUF(); /* method */
#endif
	  state = (cls == CLS_CRLF? 0 : 32);
	}
	break;
      case 32:
	if(cls == CLS_QUOTE) {
	  push_int(0);
	  push_int(0);
	  state = 10;
	} else if(cls >= CLS_TOKEN) {
#ifdef DYNAMIC_BUF
	  buf.s.str = NULL;
	  initialize_buf( &buf );
	  low_my_putchar( c, &buf );
#else
	  bufpos = 0;
	  BUFSET(c);
#endif

	  state = 33;
	} else
	  if(cls == CLS_CRLF)
	    state = 0;
	break;
      case 33:
	if(cls == CLS_QUOTE)
	  state = 34;
	else if(cls == CLS_CRLF) {
#ifdef DYNAMIC_BUF
	  push_string( low_free_buf( &buf ) ); 
#else
	  PUSHBUF(); 
#endif
	  state = 0;
	} else if(cls == CLS_WSPACE) {
#ifdef DYNAMIC_BUF
	  push_string( low_free_buf( &buf ) );  /* path */
#else
	  PUSHBUF();  /* path */
#endif
	  state = 35;
	} else	
#ifdef DYNAMIC_BUF
	  low_my_putchar( c, &buf );
#else
	  BUFSET(c);
#endif

	break;
      case 34:
	if(cls >= CLS_TOKEN) {
#ifdef DYNAMIC_BUF
	  low_my_putchar( '"', &buf );
	  low_my_putchar( c, &buf );
#else
	  BUFSET('"');
	  BUFSET(c);
#endif

	  state = 33;
	} else if(cls == CLS_CRLF) {
#ifdef DYNAMIC_BUF
	  push_string( low_free_buf( &buf ) ); 
#else
	  PUSHBUF(); 
#endif
	  state = 0;
	} else {
#ifdef DYNAMIC_BUF
	  push_string( low_free_buf( &buf ) ); 
#else
	  PUSHBUF(); 
#endif
	  push_text("HTTP/0.9");
	  state = 10;
	}
	break;
      case 35:
	if(cls == CLS_QUOTE) {
	  push_text("HTTP/0.9");
	  state = 10;
	} else if(cls >= CLS_TOKEN) {
#ifdef DYNAMIC_BUF
	  buf.s.str = NULL;
	  initialize_buf( &buf );
	  low_my_putchar( c, &buf );
#else
	  bufpos = 0;
	  BUFSET(c);
#endif

	  state = 8;
	} else
	  if(cls == CLS_CRLF)
	    state = 0;
	break;
      }
    }
  }
  if(state == 1 || state == 3 || state == 5 ||
     state == 8 || state == 9 ||
     state == 31 || state == 33 || state == 34) {
#ifdef DYNAMIC_BUF
    push_string( low_free_buf( &buf ) ); 
#else
    PUSHBUF(); 
#endif
  }
  if(sp == old_sp + 15) {
    f_aggregate(15);
    push_int64(offs0);
    apply_svalue(logfun, 2);
    pop_stack();
  }
  free(read_buf);
#ifndef DYNAMIC_BUF
  free(buf);
#endif
  if(my_fd)
    /* If my_fd == 0, the second argument was an object and thus we don't
     * want to free it.
     */
    fd_close(f);
  pop_n_elems(sp-old_sp+args);  
  push_int64(offs0);
}

/*! @endmodule
 */
