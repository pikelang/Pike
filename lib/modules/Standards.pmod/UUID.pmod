//!
//! Support for Universal Unique Identifiers (UUID) and
//! Globally Unique Identifiers (GUID).
//!
// $Id: UUID.pmod,v 1.1 2004/10/04 10:49:03 grubba Exp $
//
// 2004-10-01 Henrik Grubbström

// Specifications:
//
// draft-leach-uuids-guids-01.txt (Expired internet draft, Microsoft)
// CDE 1.1: Remote Procedure Call: Universal Unique Identifier (Open Group)

// UUID format:
//
//   All fields are in network (bigendian) byte order.
//
//	Size	Field
//	  4	time_low
//	  2	time_mid
//	  2	time_hi_and_version
//	  1	clock_seq_hi_and_reserved
//	  1	clock_seq_low
//	  6	node
//
//   The UUID version is specified in the 4 most significant bits
//   of time_hi_and_version. The remaining 60 bits of time_* are
//   a time stamp.
//
//   The UUID variant is encoded in the most significant bit(s) of
//   clock_seq_hi_and_reserved as follows:
//
//	Bits	Variant
//	 0--	NCS backward compat, reserved.
//	 10-	DCE.
//	 110	Microsoft GUID, reserved.
//	 111	Reserved for future use.

// UUID version 1 format:
//
//   The time stamp is specified as the number of 100 ns intervals
//   since 1582-10-15 00:00:00.00 GMT.

// UUID version 1 DCE format:
//
//   The clock_seq_* field contains a 6 bit sequence number to
//   avoid duplicate timestamps.
//
//   The node field contains the IEEE 802 (aka MAC) address for the
//   node generating the UUID.
//
//   For systems without an IEEE 802 address, a random value
//   with the multicast bit set may be used.

static int last_hrtime = gethrtime(1)/100;
static int clock_sequence = random(0x4000);
static string mac_address =
  Crypto.Random.random_string(6)|"\1\0\0\0\0\0"; // Multicast bit.

//! Return a new binary UUID.
string new()
{
  int now = gethrtime(1)/100;
  if (now != last_hrtime) {
    clock_sequence = random(0x4000);
    last_hrtime = now;
  }
  int seq = clock_sequence++;
  // FIXME: Check if clock_sequence has wrapped during this @[now].

  // Adjust @[now] with the number of 100ns intervals between
  // 1582-10-15 00:00:00.00 GMT and 1970-01-01 00:00:00.00 GMT.
#if 0
  now -= Calendar.parse("%Y-%M-%D %h:%m:%s.%f %z",
			"1582-10-15 00:00:00.00 GMT")->unix_time() * 10000000;
#else /* !0 */
  now += 0x01b21dd213814000;	// Same as above.
#endif /* 0 */
  now &= 0x0fffffffffffffff;
  now |= 0x1000000000000000;	// DCE version 1.
  clock_sequence &= 0x3fff;
  clock_sequence |= 0x8000;	// DCE variant of UUIDs.

  return sprintf("%4c%2c%2c%2c%6s",
		 now & 0xffffffff,
		 (now >> 32) & 0xffff,
		 (now >> 48) & 0xffff,
		 clock_sequence,
		 mac_address);
}

//! Returns the string representation of the binary UUID @[uuid].
string format_uuid(string uuid)
{
  if (sizeof(uuid) != 16) {
    error("Bad binary UUID: %O\n", uuid);
  }
  return sprintf("%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-"
		 "%02x%02x%02x%02x%02x%02x", @values(uuid));
}

//! Returns the binary representation of the UUID @[uuid].
string parse_uuid(string uuid)
{
  int time_low, time_mid, time_hi_and_version;
  int clock_seq_hi_and_reserved, clock_seq_low;
  int node;
  array(string) segs = uuid/"-";
  if ((sizeof(uuid) != 36) ||
      (sscanf(uuid, "%8x-%4x-%4x-%4x-%12x",
	      time_low, time_mid, time_hi_and_version,
	      clock_seq_hi_and_reserved, clock_seq_low,
	      node) != 6)) {
    error("Bad UID string: %O\n", uuid);
  }
  return sprintf("%4c%2c%2c%2c%6c",
		 time_low, time_mid, time_hi_and_version,
		 clock_seq_hi_and_reserved, clock_seq_low,
		 node);
}

//! Return a new UUID string.
string new_string()
{
  return format_uuid(new());
}

// Some UUIDs:

//! The Nil UUID.
constant Nil_UUID = "00000000-0000-0000-0000-000000000000";

//! Name space UUID for DNS.
constant NameSpace_DNS = "6ba7b810-9dad-11d1-80b4-00c04fd430c8";

//! Name space UUID for URL.
constant NameSpace_URL = "6ba7b811-9dad-11d1-80b4-00c04fd430c8";

//! Name space UUID for OID.
constant NameSpace_OID = "6ba7b812-9dad-11d1-80b4-00c04fd430c8";

//! Name space UUID for X500.
constant NameSpace_X500 = "6ba7b814-9dad-11d1-80b4-00c04fd430c8";
