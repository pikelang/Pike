#pike __REAL_VERSION__
#pragma strict_types

//! @decl int crc32(string(8bit) data, void|int(0..) start_value)
//!
//!   This function calculates the standard ISO3309 Cyclic Redundancy
//!   Check.

//! @decl int adler32(string(8bit) data, void|int(0..) start_value)
//!
//!   This function calculates the Adler-32 Cyclic Redundancy Check.


#if constant(Gz.crc32)
constant crc32 = Gz.crc32;
constant adler32 = Gz.adler32;
#endif
