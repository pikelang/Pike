#pike __REAL_VERSION__
#pragma strict_types

// Pike core things that don't belong anywhere else.
//
// $Id: module.pmod,v 1.11 2004/07/04 12:38:50 mast Exp $

constant WEAK_INDICES = __builtin.PIKE_WEAK_INDICES;
constant WEAK_VALUES = __builtin.PIKE_WEAK_VALUES;
constant WEAK = WEAK_INDICES|WEAK_VALUES;
//! Flags for use together with @[set_weak_flag] and @[get_weak_flag].
//! See @[set_weak_flag] for details.

constant BacktraceFrame = __builtin.backtrace_frame;

constant Backend = __builtin.Backend;
constant DefaultBackend = __builtin.__backend;

constant gc_parameters = __builtin.gc_parameters;

program Encoder = [program] master()->Encoder;
program Decoder = [program] master()->Decoder;
program Codec = [program] master()->Codec;
