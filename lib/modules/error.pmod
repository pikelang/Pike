#pike __REAL_VERSION__

//! @decl void error(string message)
//! @appears error
//!
//! Identical to @tt{throw(({message,backtrace()}))@}.

// Moahahahah!
// $Id: error.pmod,v 1.6 2001/12/20 15:04:12 nilsson Exp $
void `()(string f, mixed ... args)
{
  array(array) b = backtrace();
  if (sizeof(args)) f = sprintf(f, @args);
  throw( ({ f, b[..sizeof(b)-2] }) );
}
