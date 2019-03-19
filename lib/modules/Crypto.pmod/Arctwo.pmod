#pike __REAL_VERSION__
#pragma strict_types
#require constant(Nettle.ARCTWO)

//! Arctwo is a block cipher, also known under the trade marked name
//! RC2.
//!
//! The cipher is quite weak, and should not be used for new software.

inherit Nettle.ARCTWO;
