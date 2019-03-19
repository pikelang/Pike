#pike __REAL_VERSION__
#pragma strict_types

//! @decl int(0..) crc32(string(8bit) data, void|int(0..) offset)
//!
//!   This function calculates the standard ISO3309 Cyclic Redundancy
//!   Check.

//! @decl int(0..) adler32(string(8bit) data, void|int(0..) offset)
//!
//!   This function calculates the Adler-32 Cyclic Redundancy Check.


#if constant(Gz.crc32)
constant crc32 = Gz.crc32;
constant adler32 = Gz.adler32;
#endif

//! @decl int(0..) crc32c(string(8bit) data)
//!
//!   This function calculates the Castagnoli CRC, CRC32C. Hardware
//!   optimized on Intel CPUs with SSE 4.2.

#if constant(Nettle.crc32c)
constant crc32c = Nettle.crc32c;
#endif
