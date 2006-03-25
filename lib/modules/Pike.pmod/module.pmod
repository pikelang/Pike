#pike __REAL_VERSION__
#pragma strict_types

// Pike core things that don't belong anywhere else.
//
// $Id: module.pmod,v 1.13 2006/03/25 20:43:20 grubba Exp $

constant WEAK_INDICES = __builtin.PIKE_WEAK_INDICES;
constant WEAK_VALUES = __builtin.PIKE_WEAK_VALUES;
constant WEAK = WEAK_INDICES|WEAK_VALUES;
//! Flags for use together with @[set_weak_flag] and @[get_weak_flag].
//! See @[set_weak_flag] for details.

constant INDEX_FROM_BEG = __builtin.INDEX_FROM_BEG;
constant INDEX_FROM_END = __builtin.INDEX_FROM_END;
constant OPEN_BOUND = __builtin.OPEN_BOUND;
//! Used with @[predef::`[..]] and @[lfun::`[..]] to specify how the
//! corresponding index maps to an upper or lower range bound:
//!
//! @dl
//!   @item INDEX_FROM_BEG
//!     The index is relative to the beginning of the string or array
//!     (or any other sequence implemented through an object).
//!     Sequences typically start at zero.
//!
//!   @item INDEX_FROM_END
//!     The index is relative to the end of the sequence. In strings
//!     and arrays, the last element is at zero, the one before that
//!     at one, etc.
//!
//!   @item OPEN_BOUND
//!     The range is open in the corresponding direction. The index is
//!     irrelevant in this case.
//! @enddl

constant BacktraceFrame = __builtin.backtrace_frame;

constant Backend = __builtin.Backend;
constant DefaultBackend = __builtin.__backend;

constant gc_parameters = __builtin.gc_parameters;

constant get_runtime_info = __builtin.get_runtime_info;

program Encoder = [program] master()->Encoder;
program Decoder = [program] master()->Decoder;
program Codec = [program] master()->Codec;
