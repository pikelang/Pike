// Compatibility module
// $Id: __default.pmod,v 1.6 2002/11/28 23:21:05 grubba Exp $

#pike 7.3

//! Pike 7.2 efun compatibility module.
//!
//! The methods in this module will appear as efuns in
//! programs that use @tt{#pike 7.2@} or lower.

string dirname(string x)
{
  array(string) tmp=explode_path(x);
  return tmp[..sizeof(tmp)-2]*"/";
}

//!   High-resolution sleep (Pike 7.2 compatibility).
//!
//!   Sleep @[t] seconds.
//!
//! @note
//!   This function will busy-wait if the sleep-interval is short.
//!
//! @obsolete
//!   This function was renamed to @[delay()] in Pike 7.3.
//!
//! @seealso
//!   @[predef::sleep()], @[delay()]
void sleep(float|int t, void|int abort)
{
  delay(t, abort);
}

//!   Get the default YP domain (Pike 7.2 compatibility).
//!
//! @obsolete
//!   This function was removed in Pike 7.3, use
//!   @[YP.default_domain()] instead.
//!
//! @seealso
//!   @[YP.default_domain()]
string default_yp_domain() {
  return Yp.default_domain();
}

//!   Instantiate a program (Pike 7.2 compatibility).
//!
//!   A new instance of the class @[prog] will be created.
//!   All global variables in the new object be initialized, and
//!   then @[lfun::create()] will be called with @[args] as arguments.
//!
//! @obsolete
//!   This function was removed in Pike 7.3, use
//!   @code{((program)@[prog])(@@@[args])@}
//!   instead.
//!
//! @seealso
//!   @[destruct()], @[compile_string()], @[compile_file()], @[clone()]
//!
object new(string|program prog, mixed ... args)
{
  if(stringp(prog))
  {
    if(program p=(program)(prog, backtrace()[-2][0]))
      return p(@args);
    else
      error("Failed to find program %s.\n", prog);
  }
  return prog(@args);
}

//! @decl object clone(string|program prog, mixed ... args)
//!
//!   Alternate name for the function @[new()] (Pike 7.2 compatibility).
//!
//! @obsolete
//!   This function was removed in Pike 7.3, use
//!   @code{((program)@[prog])(@@@[args])@}
//!   instead.
//!
//! @seealso
//!   @[destruct()], @[compile_string()], @[compile_file()], @[new()]

function(string|program, mixed ... : object) clone = new;

mapping(string:mixed) all_constants()
{
  mapping(string:mixed) ret=predef::all_constants()+([]);
  ret->all_constants=all_constants;
  ret->dirname=dirname;
  ret->default_yp_domain=default_yp_domain;
  ret->new=new;
  ret->clone=clone;
  return ret;
}
