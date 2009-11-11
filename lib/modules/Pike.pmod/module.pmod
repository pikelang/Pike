#pike __REAL_VERSION__
#pragma strict_types

// Pike core things that don't belong anywhere else.
//
// $Id: module.pmod,v 1.23 2009/11/11 20:05:06 mast Exp $

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

constant __Backend = __builtin.Backend;

//! The class of the @[DefaultBackend].
//!
//! Typically something that has inherited @[__Backend].
//!
//! @seealso
//!   @[__Backend], @[DefaultBackend]
constant Backend = __builtin.DefaultBackendClass;

#if constant(__builtin.PollDeviceBackend)
constant PollDeviceBackend = __builtin.PollDeviceBackend;
#endif

#if constant(__builtin.PollBackend)
constant PollBackend = __builtin.PollBackend;
#endif

#if constant(__builtin.PollBackend)
constant SmallBackend = __builtin.PollBackend;
#elif constant(__builtin.PollDeviceBackend)
constant SmallBackend = __builtin.PollDeviceBackend;
#else
constant SmallBackend = __builtin.SelectBackend;
#endif

//! @decl program(Pike.Backend) SmallBackend
//!
//! This is the most suitable backend implementation if you only want
//! to monitor a small number of @[Stdio.File] objects.

constant SelectBackend = __builtin.SelectBackend;

constant DefaultBackend = __builtin.__backend;

constant gc_parameters = __builtin.gc_parameters;
constant implicit_gc_real_time = __builtin.implicit_gc_real_time;
constant count_memory = __builtin.count_memory;

constant get_runtime_info = __builtin.get_runtime_info;

// Type-checking:
constant low_check_call = predef::__low_check_call;
constant get_return_type = predef::__get_return_type;
constant get_first_arg_type = predef::__get_first_arg_type;

program Encoder = [program] master()->Encoder;
program Decoder = [program] master()->Decoder;
program Codec = [program] master()->Codec;

#if 0
protected constant TYPE = typeof(typeof([mixed]0));

TYPE check_call(TYPE fun_type, TYPE ... arg_types)
{
  array(TYPE) conts = allocate(sizeof(arg_types) + 1);
  conts[0] = fun_type;
  foreach(arg_types; int i; TYPE arg_type) {
    if (!(conts[i+1] = low_check_call(conts[i], arg_type,
				      (i == (sizeof(arg_types)-1))?2:0))) {
      werror("Error: Bad argument %d to function, got %O, expected %O.\n",
	     i+1, arg_type, get_first_arg_type(conts[i]));
      break;
    }
  }
  if (!conts[sizeof(arg_types)]) {
    int i;
    for(i = 0; (i < sizeof(arg_types)) && conts[i+1]; i++) {
      TYPE param_type = get_first_arg_type(conts[i]);
      if (arg_types[i] <= param_type) continue;
      werror("Warning: Potentially bad argument %d to function, got %O, expected %O.\n",
	     i+1, arg_types[i], param_type);
    }
    return 0;
  }
  TYPE ret = get_return_type(conts[-1]);
  if (!ret) {
    werror("Error: Too few arguments.\n");
  }
  return ret;
}
#endif /* 0 */
