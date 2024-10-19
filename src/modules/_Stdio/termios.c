/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "file_machine.h"

#if defined(HAVE_TERMIOS_H) || defined(HAVE_SYS_TERMIOS_H) || defined(__NT__)

#ifdef HAVE_TERMIOS_H
#include <termios.h>
#elif defined(HAVE_SYS_TERMIOS_H)
/* NB: Deprecated by <termios.h> above. */
#include <sys/termios.h>
#endif

#include <errno.h>

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#include "fdlib.h"
#include "interpret.h"
#include "mapping.h"
#include "svalue.h"
#include "stralloc.h"
#include "operators.h"

#include "module_support.h"

#include "file.h"

#define sp Pike_sp

/* Friendly BeOS fix */
#if defined(CS5) && defined(CS6) && CS6 == CS5
#undef CS5
#endif
#if defined(CS6) && defined(CS7) && CS7 == CS6
#undef CS6
#endif

/*! @module _Stdio
 */

/*! @class Fd
 */

/*! @decl mapping(string(7bit):int) tcgetattr()
 *! @decl int tcsetattr(mapping(string(7bit):int) attr)
 *! @decl int tcsetattr(mapping(string(7bit):int) attr, string(7bit) when)
 *!
 *! Gets/sets term attributes. The returned value/the @[attr] parameter
 *! is a mapping on the form
 *!
 *! @mapping
 *!   @member int(-1..) "ispeed"
 *!     In baud rate.
 *!   @member int(-1..) "ospeed"
 *!     Out baud rate.
 *!   @member int(-1..-1)|int(5..8) "csize"
 *!     Character size in bits.
 *!   @member int "rows"
 *!     Terminal rows.
 *!   @member int "columns"
 *!     Terminal columns.
 *!   @member int(0..1) flag_name
 *!     The value of a named flag. The flag name is
 *!     the string describing the termios flags (IGNBRK, BRKINT,
 *!     IGNPAR, PARMRK, INPCK, ISTRIP, INLCR, IGNCR, ICRNL, IUCLC,
 *!     IXON, IXANY, IXOFF, IMAXBEL, OPOST, OLCUC, ONLCR, OCRNL,
 *!     ONOCR, ONLRET, OFILL, OFDEL, OXTABS, ONOEOT, CSTOPB, CREAD,
 *!     PARENB, PARODD, HUPCL, CLOCAL, CRTSCTS, ISIG, ICANON, XCASE,
 *!     ECHO, ECHOE, ECHOK, ECHONL, ECHOCTL, ECHOPRT, ECHOKE, FLUSHO,
 *!     NOFLSH, TOSTOP, PENDIN). See the manpage for termios or
 *!     other documentation for more information. All flags are not
 *!     available on all platforms.
 *!   @member int(0..255) character_name
 *!     Sets the value of a control character (VINTR, VQUIT, VERASE,
 *!     VKILL, VEOF, VTIME, VMIN, VSWTC, VSTART, VSTOP, VSUSP, VEOL,
 *!     VREPRINT, VDISCARD, VWERASE, VLNEXT, VEOL2). All control
 *!     characters are not available on all platforms.
 *! @endmapping
 *!
 *! Negative values are not allowed as indata, but might appear in the
 *! result from @[tcgetattr] when the actual value is unknown. @[tcsetattr]
 *! returns 0 if failed.
 *!
 *! The argument @[when] to @[tcsetattr] describes when the
 *! changes are to take effect:
 *! @string
 *!   @value "TCSANOW"
 *!     The change occurs immediately (default).
 *!   @value "TCSADRAIN"
 *!     The change occurs after all output has been written.
 *!   @value "TCSAFLUSH"
 *!     The change occurs after all output has been written,
 *!     and empties input buffers.
 *! @endstring
 *!
 *! @example
 *!   // setting the terminal in raw mode:
 *!   Stdio.stdin->tcsetattr((["ECHO":0,"ICANON":0,"VMIN":0,"VTIME":0]));
 *!
 *! @note
 *!   Unknown flags are ignored by @[tcsetattr()]. @[tcsetattr] always
 *!   changes the attribute, so only include attributes that actually
 *!   should be altered in the attribute mapping.
 *!
 *! @bugs
 *!   Terminal rows and columns setting by @[tcsetattr()] is not
 *!   currently supported.
 *!
 *! @seealso
 *!   @[tcsetsize()]
 */



#undef THIS
#define THIS ((struct my_file *)(Pike_fp->current_storage))
#define FD (THIS->box.fd)
#define ERRNO (THIS->my_errno)

#ifdef HAVE_TCGETATTR
static int termios_bauds( int speed )
{
    switch (speed)
    {
#define TERMIOS_SPEED(B,V) case B: return V;
#include "termios_flags.h"
#undef TERMIOS_SPEED
    }
    return speed;
}


#define c_cflag 1
#define c_iflag 2
#define c_oflag 3
#define c_lflag 4
#define TERMIOS_FLAG(where,flag,sflag) { where, flag, sflag },
#define TERMIOS_CHAR(cc,scc)           { 0, cc, scc },
static const struct {
  int var;
  int val;
  const char *name;
} termiosflags[] = {
#include "termios_flags.h"
};
#undef TERMIOS_CHAR
#undef TERMIOS_FLAG
#undef c_cflag
#undef c_iflag
#undef c_oflag
#undef c_lflag

#endif /* HAVE_TCGETATTR */

void file_tcgetattr(INT32 args)
{
#ifdef HAVE_TCGETATTR
   struct termios ti;
#endif
   unsigned int n = 0;

   if(FD < 0)
      Pike_error("File not open.\n");

   pop_n_elems(args);

#ifdef HAVE_TCGETATTR
   if (tcgetattr(FD,&ti)) /* error */
   {
      ERRNO=errno;
      push_int(0);
      return;
   }

   for( n=0; n<NELEM(termiosflags); n++ )
   {
      push_text( termiosflags[n].name );
      switch( termiosflags[n].var )
      {
        case 0: push_int( ti.c_cc[termiosflags[n].val] );  break;
        case 1: push_int( (ti.c_cflag & termiosflags[n].val) ? 1 : 0 ); break;
        case 2: push_int( (ti.c_iflag & termiosflags[n].val) ? 1 : 0 ); break;
        case 3: push_int( (ti.c_oflag & termiosflags[n].val) ? 1 : 0 ); break;
        case 4: push_int( (ti.c_lflag & termiosflags[n].val) ? 1 : 0 ); break;
      }
   }

   push_static_text("ospeed");
   push_int(termios_bauds(cfgetospeed(&ti)));
   push_static_text("ispeed");
   push_int(termios_bauds(cfgetispeed(&ti)));
   n++;

#ifdef CSIZE
   push_static_text("csize");
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

#endif /* HAVE_TCGETATTR */

#ifdef TIOCGWINSZ
   {
      struct winsize winsize;
      if (!fd_ioctl(FD, TIOCGWINSZ, &winsize))
      {
	 push_static_text("rows");
	 push_int(winsize.ws_row);
	 n++;
	 push_static_text("columns");
	 push_int(winsize.ws_col);
	 n++;
      }
   }
#endif

   f_aggregate_mapping(n*2);
}


#ifdef HAVE_TCGETATTR
static int termios_speed( int speed )
{
    switch (speed)
    {
#define TERMIOS_SPEED(B,V) case V: return B;
#include "termios_flags.h"
#undef TERMIOS_SPEED
    }
    Pike_error("illegal argument 1 to tcsetattr, value of key %s is not a valid baud rate\n","ospeed");
}


void file_tcsetattr(INT32 args)
{
  struct termios ti;
  int optional_actions=TCSANOW;
  struct svalue *tmp;
  unsigned int i;

  if(FD < 0)
    Pike_error("File not open.\n");

  if (!args)
    SIMPLE_WRONG_NUM_ARGS_ERROR("tcsetattr", 1);

  if (args>1)
  {
    if (args>2)
      pop_n_elems(args-2);
    if (TYPEOF(sp[-1]) != T_STRING)
      SIMPLE_ARG_TYPE_ERROR("tcsetattr", 2, "string");

    if (!strcmp(sp[-1].u.string->str,"TCSANOW"))
      optional_actions=TCSANOW;
    else if (!strcmp(sp[-1].u.string->str,"TCSADRAIN"))
      optional_actions=TCSADRAIN;
    else if (!strcmp(sp[-1].u.string->str,"TCSAFLUSH"))
      optional_actions=TCSAFLUSH;
    else
      Pike_error("illegal argument 2 to tcsetattr\n");

    pop_stack();
  }

  if (TYPEOF(sp[-1]) != T_MAPPING)
    SIMPLE_ARG_TYPE_ERROR("tcsetattr", 1, "mapping");

  /* read attr to edit */
  if (tcgetattr(FD,&ti)) /* error */
  {
    ERRNO=errno;
    push_int(0);
    return;
  }

  for( i=0; i<NELEM(termiosflags); i++ )
  {
    if( (tmp=simple_mapping_string_lookup( sp[-1].u.mapping, termiosflags[i].name )) )
    {
      if( TYPEOF(*tmp) != PIKE_T_INT )
        Pike_error("illegal argument 1 to tcsetattr: key %s has illegal value\n",termiosflags[i].name);
      switch( termiosflags[i].var )
      {
        case 0:
          ti.c_cc[termiosflags[i].val] = tmp->u.integer;
          break;
        case 1:
          if( tmp->u.integer )
            ti.c_cflag |= termiosflags[i].val;
          else
            ti.c_cflag &= ~termiosflags[i].val;
          break;
        case 2:
          if( tmp->u.integer )
            ti.c_iflag |= termiosflags[i].val;
          else
            ti.c_iflag &= ~termiosflags[i].val;
          break;
        case 3:
          if( tmp->u.integer )
            ti.c_oflag |= termiosflags[i].val;
          else
            ti.c_oflag &= ~termiosflags[i].val;
          break;
        case 4:
          if( tmp->u.integer )
            ti.c_lflag |= termiosflags[i].val;
          else
            ti.c_lflag &= ~termiosflags[i].val;
          break;
      }
    }
  }

#ifdef CSIZE
  if ( (tmp = simple_mapping_string_lookup( sp[-1].u.mapping, "csize" )) )
  {
    if (TYPEOF(*tmp) != T_INT)
      Pike_error("illegal argument 1 to tcsetattr: key %s has illegal value\n","csize");

    switch (tmp->u.integer)
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
        Pike_error("illegal argument 1 to tcsetattr: value of key %s is not a valid char size\n","csize");
    }
  }
#endif


  if ( (tmp = simple_mapping_string_lookup( sp[-1].u.mapping, "ospeed" )) )
  {
    if (TYPEOF(*tmp) != T_INT)
      Pike_error("illegal argument 1 to tcsetattr: key %s has illegal value\n","ospeed");
    cfsetospeed(&ti,termios_speed(tmp->u.integer));
  }

  if ( (tmp = simple_mapping_string_lookup( sp[-1].u.mapping, "ispeed" )) )
  {
    if (TYPEOF(*tmp) != T_INT)
      Pike_error("illegal argument 1 to tcsetattr: key %s has illegal value\n","ospeed");
    cfsetispeed(&ti,termios_speed(tmp->u.integer));
  }
  pop_stack(); /* lose the mapping */
  push_int(!tcsetattr(FD,optional_actions,&ti));
}

/*! @decl int(0..1) tcflush(string(7bit)|void flush_direction)
 *!
 *! Flush queued terminal control messages.
 *!
 *! @param flush_direction
 *!   @string
 *!     @value "TCIFLUSH"
 *!       Flush received but not read.
 *!     @value "TCOFLUSH"
 *!       Flush written but not transmitted.
 *!     @value "TCIOFLUSH"
 *!       Flush both of the above. Default.
 *!   @endstring
 *!
 *! @returns
 *!   Returns @expr{1@} on success and @expr{0@} (zero) on failure.
 *!
 *! @seealso
 *!   @[tcdrain()]
 */
void file_tcflush(INT32 args)
{
  int action=TCIOFLUSH;

  if(args)
    {
      struct pike_string *a, *s_tciflush, *s_tcoflush, *s_tcioflush;
      MAKE_CONSTANT_SHARED_STRING( s_tciflush, "TCIFLUSH" );
      MAKE_CONSTANT_SHARED_STRING( s_tcoflush, "TCOFLUSH" );
      MAKE_CONSTANT_SHARED_STRING( s_tcioflush, "TCIOFLUSH" );
      get_all_args( NULL, args, "%n", &a );
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

/*! @decl int(0..1) tcdrain()
 *!
 *! Wait for transmission buffers to empty.
 *!
 *! @returns
 *!   Returns @expr{1@} on success and @expr{0@} (zero) on failure.
 *!
 *! @seealso
 *!   @[tcflush()]
 */
void file_tcdrain(INT32 PIKE_UNUSED(args))
{
  push_int(!tcdrain(FD));
}

/*! @decl int(0..1) tcsendbreak(int|void duration)
 *!
 *! Send a break signal.
 *!
 *! @param duration
 *!   Duration to send the signal for. @expr{0@} (zero) causes
 *!   a break signal to be sent for between 0.25 and 0.5 seconds.
 *!   Other values are operating system dependent:
 *!   @dl
 *!     @item SunOS
 *!       The number of joined break signals as above.
 *!     @item Linux, AIX, Digital Unix, Tru64
 *!       The time in milliseconds.
 *!     @item FreeBSD, NetBSD, HP-UX, MacOS
 *!       The value is ignored.
 *!     @item Solaris, Unixware
 *!       The behavior is changed to be similar to @[tcdrain()].
 *!   @enddl
 *!
 *! @returns
 *!   Returns @expr{1@} on success and @expr{0@} (zero) on failure.
 */
void file_tcsendbreak(INT32 args)
{
  INT_TYPE len=0;

  get_all_args(NULL, args, ".%i", &len);
  pop_stack();
  push_int(!tcsendbreak(FD, len));
}

#endif /* HAVE_TCGETATTR */

#ifdef TIOCSWINSZ
/*! @decl int(0..1) tcsetsize(int rows, int cols)
 *!
 *! Set the number of rows and columns for a terminal.
 *!
 *! @returns
 *!   Returns @expr{1@} on success and @expr{0@} (zero) on failure.
 *!
 *! @seealso
 *!   @[tcgetattr()], @[tcsetattr()]
 */
void file_tcsetsize(INT32 args)
{
  INT_TYPE rows;
  INT_TYPE cols;
  struct winsize winsize;

  get_all_args(NULL, args, "%i%i", &rows, &cols);

  winsize.ws_row = rows;
  winsize.ws_col = cols;

  push_int(!fd_ioctl(FD, TIOCSWINSZ, &winsize));
}
#endif

/*! @endclass
 */

/*! @endmodule
 */

/* end of termios stuff */
#endif
