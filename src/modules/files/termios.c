#include "global.h"
RCSID("$Id: termios.c,v 1.1 1998/12/17 00:48:30 mirar Exp $");
#include "file_machine.h"

#if defined(WITH_TERMIOS) && defined(HAVE_TERMIOS_H)

#include <termios.h>
#include <unistd.h>
#include <errno.h>

#include "fdlib.h"
#include "interpret.h"
#include "mapping.h"
#include "svalue.h"
#include "stralloc.h"
#include "operators.h"

#include "file.h"


/*
**! method mapping tcgetattr()
**! method int tcsetattr(mapping attr)
**! method int tcsetattr(mapping attr,string when)
**!	gets/sets term attributes
**!
**!	the returned value/the parameter is a mapping on the form
**!	<pre>
**!		"ispeed":baud rate
**!		"ospeed":baud rate
**!		flag:0|1
**!		...
**!	</pre>
**!     where 'flag' is the string describing the termios 
**!     (input, output, control and local mode) flags 
**!     (see the manpage for termios or the tutorial).
**!
**!	Note that tcsetattr always will set the attributes
**!	in the mapping; correct use would be something like:
**!	<tt>fd->tcsetattr(fd->tcgetattr()|(["myflag":1]));</tt>
**!
**!	the argument 'when' to tcsetattr describes when the 
**!	changes are to take effect:
**!	"TCSANOW": the change occurs immediately (default);
**!	"TCSADRAIN": the change occurs after all output has been written;
**!	"TCSAFLUSH": the change occurs after all output has been written,
**!	and empties input buffers.
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

#include "termios_flags.h"

#undef TERMIOS_FLAG

   push_text("ospeed");
   switch (cfgetospeed(&ti))
   {
#define TERMIOS_SPEED(B,V) case B: push_int(V); break;
#include "termios_speeds.h"
      default: push_int(-1);
   }
   n++;

   push_text("ispeed");
   switch (cfgetispeed(&ti))
   {
#include "termios_speeds.h"
#undef TERMIOS_SPEED
      default: push_int(-1);
   }
   n++;

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

   /* just zero these */
   ti.c_iflag=0;
   ti.c_oflag=0;
   ti.c_cflag=0;
   ti.c_lflag=0;

#define TERMIOS_FLAG(where,flag,sflag) \
   stack_dup(); \
   push_text(sflag); \
   f_index(2); \
   if (sp[-1].type!=T_INT)  \
      error("illegal argument 1 to tcsetattr: key %s has illegal value",sflag); \
   if (sp[-1].u.integer) ti.where|=flag; \
   pop_stack();

#include "termios_flags.h"

#undef TERMIOS_FLAG

   stack_dup(); 
   push_text("ospeed");
   f_index(2); 
   if (sp[-1].type!=T_INT)  
      error("illegal argument 1 to tcsetattr: key %s has illegal value","ospeed"); 
   switch (sp[-1].u.integer)
   {
#define TERMIOS_SPEED(B,V) case V: push_int(B); break;
#include "termios_speeds.h"
      default:
	 error("illegal argument 1 to tcsetattr, value of key %s in not a valid baud rate\n","ospeed");
   }
   cfsetospeed(&ti,sp[-1].u.integer);
   pop_stack();
   pop_stack();

   stack_dup(); 
   push_text("ispeed");
   f_index(2); 
   if (sp[-1].type!=T_INT)  
      error("illegal argument 1 to tcsetattr: key %s has illegal value","ospeed"); 
   switch (sp[-1].u.integer)
   {
#include "termios_speeds.h"
#undef TERMIOS_SPEED
      default:
	 error("illegal argument 1 to tcsetattr, value of key %s in not a valid baud rate\n","ispeed");
   }
   cfsetispeed(&ti,sp[-1].u.integer);
   pop_stack();
   pop_stack();

   pop_stack(); /* lose the mapping */
   
   push_int(!tcsetattr(FD,optional_actions,&ti));
}


/* end of termios stuff */
#endif 
