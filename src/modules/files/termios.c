#include "global.h"
RCSID("$Id: termios.c,v 1.7 2000/02/07 08:02:17 hubbe Exp $");
#include "file_machine.h"

#if defined(HAVE_TERMIOS_H)

#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "fdlib.h"
#include "interpret.h"
#include "mapping.h"
#include "svalue.h"
#include "stralloc.h"
#include "operators.h"

#include "module_support.h"

#include "file.h"


/*
**! method mapping tcgetattr()
**! method int tcsetattr(mapping attr)
**! method int tcsetattr(mapping attr,string when)
**!	gets/sets term attributes
**!
**!	the returned value/the parameter is a mapping on the form
**!	<pre>
**!		"ispeed":    baud rate
**!		"ospeed":    baud rate
**!		"csize":     character size (5,6,7 or 8)
**!		"rows":      terminal rows 
**!		"columns":   terminal columns  
**!		flag:        0 or 1
**!		control char:value
**!		...
**!	</pre>
**!     where 'flag' is the string describing the termios 
**!     (input, output, control and local mode) flags 
**!     (see the manpage for termios or the tutorial).
**!
**!	Note that tcsetattr always _changes_ the attributes;
**!	correct use to set a flag would be something like:
**!	<tt>fd->tcsetattr((["myflag":1]));</tt>
**!
**!	the argument 'when' to tcsetattr describes when the 
**!	changes are to take effect:
**!	"TCSANOW": the change occurs immediately (default);
**!	"TCSADRAIN": the change occurs after all output has been written;
**!	"TCSAFLUSH": the change occurs after all output has been written,
**!	and empties input buffers.
**!
**!	Example for setting the terminal in raw mode:
**!	   Stdio.stdin->tcsetattr((["ECHO":0,"ICANON":0,"VMIN":0,"VTIME":0]));
**!
**!	Note: Unknown flags are ignored by tcsetattr().
**!	Terminal rows and columns settring by tcsetattr() is not
**!	currently supported.
**!
**!	returns 0 if failed.
*/

#undef THIS
#define THIS ((struct my_file *)(fp->current_storage))
#define FD (THIS->fd)
#define ERRNO (THIS->my_errno)

void file_tcgetattr(INT32 args)
{
   struct termios ti;
   int n;

   if(FD < 0)
      error("File not open.\n");

   pop_n_elems(args);

   if (tcgetattr(FD,&ti)) /* error */
   {
      ERRNO=errno;
      push_int(0);
      return;
   }

   n=0;

#define TERMIOS_FLAG(where,flag,sflag) \
   push_text(sflag); \
   push_int(!!(ti.where&flag)); \
   n++; 
#define TERMIOS_CHAR(c,sc) \
   push_text(sc); \
   push_int(ti.c_cc[c]); \
   n++; 

#include "termios_flags.h"

#undef TERMIOS_FLAG
#undef TERMIOS_CHAR

   push_text("ospeed");
   switch (cfgetospeed(&ti))
   {
#define TERMIOS_SPEED(B,V) case B: push_int(V); break;
#include "termios_flags.h"
      default: push_int(-1);
   }
   n++;

   push_text("ispeed");
   switch (cfgetispeed(&ti))
   {
#include "termios_flags.h"
#undef TERMIOS_SPEED
      default: push_int(-1);
   }
   n++;

#ifdef CSIZE
   push_text("csize");
   switch (ti.c_cflag&CSIZE)
   {
#ifdef CS8
      case CS8: push_int(8); break;
#endif
#ifdef CS7
      case CS7: push_int(7); break;
#endif
#ifdef CS6
      case CS6: push_int(6); break;
#endif
#ifdef CS5
      case CS5: push_int(5); break;
#endif
      default:
	 push_int(-1);
   }
   n++;
#endif

#ifdef TIOCGWINSZ
   {
      struct winsize winsize;
      if (!ioctl(FD,TIOCGWINSZ,&winsize))
      {
	 push_text("rows");
	 push_int(winsize.ws_row);
	 n++;
	 push_text("columns");
	 push_int(winsize.ws_col);
	 n++;
      }
   }
#endif


   f_aggregate_mapping(n*2);
}

void file_tcsetattr(INT32 args)
{
   struct termios ti;
   int optional_actions=TCSANOW;

   if(FD < 0)
      error("File not open.\n");

   if (!args)
      error("too few arguments to tcsetattr\n");


   if (args>=2)
   {
      if (sp[-1].type!=T_STRING)
	 error("illegal argument 2 to tcsetattr\n");
	  
      if (!strcmp(sp[-1].u.string->str,"TCSANOW"))
	 optional_actions=TCSANOW;
      else if (!strcmp(sp[-1].u.string->str,"TCSADRAIN"))
	 optional_actions=TCSADRAIN;
      else if (!strcmp(sp[-1].u.string->str,"TCSAFLUSH"))
	 optional_actions=TCSAFLUSH;
      else
	 error("illegal argument 2 to tcsetattr\n");

      pop_n_elems(args-1);
   }

   if (sp[-1].type!=T_MAPPING)
      error("illegal argument 1 to tcsetattr\n");

   /* read attr to edit */
   if (tcgetattr(FD,&ti)) /* error */
   {
      ERRNO=errno;
      push_int(0);
      return;
   }

#define TERMIOS_FLAG(where,flag,sflag) \
   stack_dup(); \
   push_text(sflag); \
   f_index(2); \
   if (!IS_UNDEFINED(sp-1)) \
   { \
      if (sp[-1].type!=T_INT)  \
         error("illegal argument 1 to tcsetattr: key %s has illegal value",sflag); \
      if (sp[-1].u.integer) ti.where|=flag; else ti.where&=~flag; \
   } \
   pop_stack();

#define TERMIOS_CHAR(cc,scc) \
   stack_dup(); \
   push_text(scc); \
   f_index(2); \
   if (!IS_UNDEFINED(sp-1)) \
   { \
      if (sp[-1].type!=T_INT)  \
         error("illegal argument 1 to tcsetattr: key %s has illegal value",scc); \
      ti.c_cc[cc]=(char)sp[-1].u.integer; \
   } \
   pop_stack();

#include "termios_flags.h"

#undef TERMIOS_FLAG
#undef TERMIOS_CHAR

#ifdef CSIZE
   stack_dup();
   push_text("csize");
   f_index(2);

   if (!IS_UNDEFINED(sp-1)) 
   {
      if (sp[-1].type!=T_INT)  
   	 error("illegal argument 1 to tcsetattr: key %s has illegal value","csize"); 

      switch (sp[-1].u.integer)
      {
#ifdef CS8
	 case 8: ti.c_cflag=(ti.c_cflag&~CSIZE)|CS8; break;
#endif
#ifdef CS7
	 case 7: ti.c_cflag=(ti.c_cflag&~CSIZE)|CS7; break;
#endif
#ifdef CS6
	 case 6: ti.c_cflag=(ti.c_cflag&~CSIZE)|CS6; break;
#endif
#ifdef CS5
	 case 5: ti.c_cflag=(ti.c_cflag&~CSIZE)|CS5; break;
#endif
	 default:
	    error("illegal argument 1 to tcsetattr: value of key %s is not a valid char size","csize"); 
      }
   }
   pop_stack();

#endif
   
   
   stack_dup(); 
   push_text("ospeed");
   f_index(2); 
   if (!IS_UNDEFINED(sp-1)) 
   {
      if (sp[-1].type!=T_INT)  
   	 error("illegal argument 1 to tcsetattr: key %s has illegal value","ospeed"); 
      switch (sp[-1].u.integer)
      {
   #define TERMIOS_SPEED(B,V) case V: push_int(B); break;
   #include "termios_flags.h"
   	 default:
   	    error("illegal argument 1 to tcsetattr, value of key %s is not a valid baud rate\n","ospeed");
      }
      cfsetospeed(&ti,sp[-1].u.integer);
      pop_stack();
   }
   pop_stack();

   stack_dup(); 
   push_text("ispeed");
   f_index(2); 
   if (!IS_UNDEFINED(sp-1)) 
   {
      if (sp[-1].type!=T_INT)  
   	 error("illegal argument 1 to tcsetattr: key %s has illegal value","ispeed"); 
      switch (sp[-1].u.integer)
      {
   #include "termios_flags.h"
   #undef TERMIOS_SPEED
   	 default:
   	    error("illegal argument 1 to tcsetattr, value of key %s is not a valid baud rate\n","ispeed");
      }
      cfsetispeed(&ti,sp[-1].u.integer);
      pop_stack();
   }
   pop_stack();

   pop_stack(); /* lose the mapping */
   
   push_int(!tcsetattr(FD,optional_actions,&ti));
}

void file_tcflush(INT32 args)                               
{                                                           
  int action=TCIOFLUSH;                                     

  if(args)                                                  
    {                                                         
      struct pike_string *a, *s_tciflush, *s_tcoflush, *s_tcioflush;
      MAKE_CONSTANT_SHARED_STRING( s_tciflush, "TCIFLUSH" );  
      MAKE_CONSTANT_SHARED_STRING( s_tcoflush, "TCOFLUSH" );  
      MAKE_CONSTANT_SHARED_STRING( s_tcioflush, "TCIOFLUSH" );
      get_all_args( "tcflush", args, "%S", &a );              
      if(a == s_tciflush )                                    
	action=TCIFLUSH;                                      
      else if(a == s_tcoflush )                               
	action=TCOFLUSH;                                      
      
#ifdef TCIOFLUSH                                            
      else if(a == s_tcioflush )                              
	action=TCIOFLUSH;                                     
#endif                                                      
      free_string( s_tcoflush );                              
      free_string( s_tciflush );                              
      free_string( s_tcioflush );                             
      pop_stack();
    }                                                         
  push_int(!tcflush(FD, action));                           
}                                                           

void file_tcsendbreak(INT32 args)
{
  int len=0;

  get_all_args("tcsendbreak", args, "%d", &len);
  pop_stack();
  push_int(!tcsendbreak(FD, len));
}
  

/* end of termios stuff */
#endif 
