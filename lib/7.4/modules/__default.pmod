// Compatibility namespace
// $Id: __default.pmod,v 1.1 2002/12/30 15:05:38 grubba Exp $

#pike 7.5

//! Pike 7.4 compatibility.
//!
//! The symbols in this namespace will appear in
//! programs that use @tt{#pike 7.4@} or lower.

//! @decl inherit predef::

mapping(string:mixed) all_constants()
{
  mapping(string:mixed) ret=predef::all_constants()+([]);
  // No differences (yet).
  return ret;
}

