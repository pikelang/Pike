//! $Id: Object.pmod,v 1.1 2005/02/09 16:35:50 mast Exp $

#pike __REAL_VERSION__
#pragma strict_types

constant DESTRUCT_EXPLICIT = __builtin.DESTRUCT_EXPLICIT;
constant DESTRUCT_NO_REFS = __builtin.DESTRUCT_NO_REFS;
constant DESTRUCT_GC = __builtin.DESTRUCT_GC;
constant DESTRUCT_CLEANUP = __builtin.DESTRUCT_CLEANUP;
//! Flags passed to @[lfun::destroy].
//!
//! @note
//! @[Object.DESTRUCT_EXPLICIT] is @expr{0@} and
//! @[Object.DESTRUCT_CLEANUP] is @expr{1@} for compatibility.
