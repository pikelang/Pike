// $Id: System.pmod,v 1.5 2002/02/23 00:41:57 nilsson Exp $
// this module is to allow the system module to be called system.

inherit _system;

constant Timer = __builtin.Timer;
constant Time  = __builtin.Time;

//! @class Timer

//! @decl static void create( int|void fast )
//!   Create a new timer object. The timer keeps track of relative time
//!   with sub-second precision.
//!   
//!   If fast is specified, the timer will not do system calls to get
//!   the current time but instead use the one maintained by pike. This
//!   will result in faster but somewhat more inexact timekeeping. 
//!   Also, if your program never utilizes the pike event loop the pike
//!   maintained current time never change.

//! @decl float get()
//!   Return the time in seconds since the last time get was called.
//!   The first time this method is called the time since the object
//!   was created is returned instead.

//! @decl float peek()
//!   Return the time in seconds since the last time @[get] was called.

//! @endclass



//! @class Time
//!
//! The current time as a structure containing a sec and a usec
//! member.

//! @decl static void create( int fast );
//!
//! If fast is true, do not request a new time from the system,
//! instead use the global current time variable.
//!
//! This will only work in callbacks, but can save significant amounts
//! of CPU.

//! @decl int sec;
//! @decl int usec;
//!
//!   The number of seconds and microseconds since the epoch and the
//!   last whole second, respectively. (See also @[efun::time()])
//!
//!   Please note that these variables will continually update when
//!   they are requested, there is no need to create new Time()
//!   objects.

//! @decl int usec_full;
//!
//!   The number of microseconds since the epoch. Please note that
//!   pike has to have been compiled with bignum support for this
//!   variable to contain sensible values.

//! @endclass

