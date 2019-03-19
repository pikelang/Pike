#pike __REAL_VERSION__
#pragma strict_types
#require constant(Nettle.SERPENT)

//! SERPENT is one of the AES finalists, designed by Ross Anderson,
//! Eli Biham and Lars Knudsen. Thus, the interface and properties are
//! similar to @[AES]'. One peculiarity is that it is quite pointless to
//! use it with anything but the maximum key size, smaller keys are
//! just padded to larger ones.

inherit Nettle.SERPENT;
