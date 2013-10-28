#pike __REAL_VERSION__
#pragma strict_types

//! Arctwo is a block cipher, also known under the trade marked name
//! RC2.
//!
//! The cipher is quite weak, and should not be used for new software.

#if constant(Nettle) && constant(Nettle.ARCTWO)

inherit Nettle.ARCTWO;

#else
constant this_program_does_not_exist=1;
#endif
