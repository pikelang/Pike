//!
//! Support for Universal Unique Identifiers (UUID) and
//! Globally Unique Identifiers (GUID).
//!
// $Id: UUID.pmod,v 1.2 2004/10/04 21:34:33 nilsson Exp $
//
// 2004-10-01 Henrik Grubbström
// 2004-10-04 Martin Nilsson

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


#if 0
  constant clk_offset = Calendar.ISO.Second(1582,10,15,0,0,0)->
    distance(Calendar.ISO.Second("unix",0))->how_many(Calendar.Second)
    * 10000000;
#else
  constant clk_offset = 12219297132 * 10000000;
#endif


//! Represents an UUID
class UUID {

  //! 60 bit value representing the time stamp.
  int timestamp; // 60 bit value

  //! Returns the posix time of the UUID.
  int posix_time() {
    return (timestamp-clk_offset)/10000000;
  }

  //! The version of the UUID.
  int version = 1; // 4 bits

  //! Returns a string representation of the version, e.g.
  //! @expr{"Name-based"@}.
  string str_version() {
    return ([ 1 : "Time-based",
	      2 : "DCE security",
	      3 : "Name-based",
	      4 : "Random" ])[version] || "Unknown";
  }

  //! The variant of the UUID, zero padded. I.e. any of 0, 4, 6 and 7.
  int var = 4; // 1-3 bits

  //! Returns a string representation of the variant, e.g.
  //! @expr{"IETF draft variant"@}.
  string str_variant() {
    switch(var) {
    case 0: return "Reserved, NCS backward compatibility";
    case 4: return "IETF draft variant";
    case 6: return "Reserved, Microsoft Corporation backward compatibility";
    case 7: return "Reserved for future definition";
    default: return "Illegal variant";
    }
  }

  //! The clock sequence. Should be 13 to 15 bits depending on UUID
  //! version.
  int clk_seq; // 13-15 bit

  //! The UUID node. Should be 48 bit.
  int node; // 48 bit

  //! Validates the current UUID.
  void validate() {
    if(!timestamp && !version && !var && !clk_seq && !node)
      return;

    if( !(< 0,4,6,7 >)[var] ) error("Illegal variant %O.\n", var);

    if(timestamp>=pow(2,60) || timestamp<0)
      error("Timestamp %O out of range.\n", timestamp);
    if( version<0 || version>15 )
      error("Version %O out of range.\n", version);

    int seq_limit = 15;
    if( (< 3, 4 >)[version] ) seq_limit = 14;
    if(clk_seq>=pow(2,seq_limit) || clk_seq<0)
      error("Clock sequence %O out of range.\n", clk_seq);

    if(node>=pow(2,48) || node<0)
      error("Node %012x out of range\n", node);
  }

  //! Returns the time_low field.
  int time_low() {
    return timestamp & 0xFFffFFff;
  }

  //! Returns the time_mid field.
  int time_mid() {
    return (timestamp >> 32) & 0xFFff;
  }

  //! Returns the time_hi_and_version field.
  int time_hi_and_version() {
    return ((timestamp >> 32+16) << 4) | version;
  }

  //! Encodes a binary representation of the UUID.
  string encode() {

    validate();
    string ret = "";

    int t = timestamp;
    ret += sprintf("%4c", t); // time_low
    t >>= 32;
    ret += sprintf("%2c", t); // time_mid
    t >>= 16;
    t <<= 4;
    t |= version;
    ret += sprintf("%2c", t);

    // FIXME: Hardcoded for variant 10.
    t = clk_seq>>8;
    t |= (var<<5);
    ret += sprintf("%c", t);
    ret += sprintf("%c", clk_seq&255);

    ret += sprintf("%6c", node);

    return ret;
  }

  //! Creates a string representation of the UUID.
  string str() {
    string ret = String.string2hex(encode());
    while( sizeof(ret)<32 ) ret = "0"+ret;
    return ret[..7]+"-"+ret[8..11]+"-"+ret[12..15]+"-"+ret[16..19]+"-"+ret[20..];
  }

  //! Creates a URN representation of the UUID.
  string urn() {
    return "urn:uuid:"+str();
  }

  //! Optionally created with a string or binary representation of a UUID.
  void create(void|string in) {
    if(!in) return;
    sscanf(in, "urn:%s", in);
    sscanf(in, "uuid:%s", in);
    switch(sizeof(in)) {
    case 36:
      in -= "-";
      if(sizeof(in)!=32) error("Illegal UUID.\n");
      // fallthrough
    case 32:
      in = String.hex2string(in);
      // fallthrough
    case 16:
      int time_low, time_mid, time_hi_and_version, clk_seq_res;
      sscanf(in, "%4c%2c%2c%2c%6c", time_low, time_mid, time_hi_and_version,
	     clk_seq_res, node);
      version = time_hi_and_version & 0b1111;
      time_hi_and_version >>= 4;
      timestamp = time_hi_and_version<<(6*8) | time_mid<<(4*8) | time_low;
      var = clk_seq_res >> 13;
      if( !(var & 4) ) {
	var = 0;
	clk_seq = clk_seq_res;
      }
      else if( !(var & 2) ) {
	var = 4;
	clk_seq = clk_seq_res & 0x7fff;
      }
      else
	clk_seq = clk_seq_res & 0x3fff;
      break;
    default:
      error("Illegal UUID.\n");
    }
    validate();
  }

  mixed cast(string to) {
    switch(to) {
    case "string": return str();
    case "mapping": return ([
      "time_low" : time_low(),
      "time_mid" : time_mid(),
      "time_hi_and_version" : time_hi_and_version(),
    ]);
    }
  }

}

static class ClkSeq {
  int clk_seq = random(pow(2,14));
  int last_time;

  System.Time clock = System.Time();

  int time() {
    return clock->usec_full*10 + clk_offset;
  }

  array(int) get() {

    int timestamp = time();

    if( timestamp <= last_time) {
      clk_seq++;
      clk_seq %= pow(2,14);
    }
    last_time = timestamp;

    return ({ timestamp, clk_seq });
  }

  array(int) get_state() {
    return ({ last_time, clk_seq });
  }

  void set_state(int _last_time, int _clk_seq) {
    last_time = _last_time;
    clk_seq = _clk_seq;
  }
}

static ClkSeq clk_seq = ClkSeq();

//! Returns the internal clock state. Can be used for persistent
//! storage when an application is terminated.
//! @seealso
//!   @[set_clock_state]
array(int) get_clock_state() {
  return clk_seq->get_state();
}

//! Sets the internal clock state.
//! @seealso
//!   @[get_clock_state]
void set_clock_state(int last_time, int seq) {
  clk_seq->set_state(last_time, seq);
}

//! Creates a null UUID object.
UUID make_null() {
  UUID u=UUID();
  u->version = 0;
  u->var = 0;
  u->clk_seq = 0;
  return u;
}

//! Creates a new version 1 UUID.
//! @param node
//!   Either the 48 bit mac address of the system or -1.
UUID make_version1(int node) {
  UUID u=UUID();
  [ u->timestamp, u->clk_seq ] = clk_seq->get();
  u->version = 1;
  if(node<0)
    u->node = random(pow(2,48)) & 1<<47;
  else
    u->node = node;
  return u;
}

#define H(X) String.hex2string((X)-"-")

//! Creates a DNS UUID with the given DNS name.
UUID make_dns(string name) {
  return make_version3(name, H("6ba7b810-9dad-11d1-80b4-00c04fd430c8"));
}

//! Creates a URL UUID with the given URL.
UUID make_url(string name) {
  return make_version3(name, H("6ba7b811-9dad-11d1-80b4-00c04fd430c8"));
}

//! Creates a OID UUID with the given OID.
UUID make_oid(string name) {
  return make_version3(name, H("6ba7b812-9dad-11d1-80b4-00c04fd430c8"));
}

//! Creates a X500 UUID with the gived X500 address.
UUID make_x500(string name) {
  return make_version3(name, H("6ba7b814-9dad-11d1-80b4-00c04fd430c8"));
}

//! Creates a version 3 UUID with a @[name] string and a binary
//! representation of a name space UUID.
UUID make_version3(string name, string namespace) {

  // step 2
  namespace = reverse(namespace[0..3]) + reverse(namespace[4..5]) +
    reverse(namespace[6..7]) + namespace[8..];

  // step 3
  string h = Crypto.MD5.hash(namespace+name);

  string ret = reverse(h[0..3]) + reverse(h[4..5]) +
    reverse(h[6..7]) + h[8..];

  // step 7
  int ver = ret[6];
  ver &= 0b00001111;
  ver |= 0b00110000;
  ret[6] = ver;

  // step 9
  int res = ret[8];
  res &= 0b00111111;
  res |= 0b10000000;
  ret[8] = res;

  return UUID(ret);
}

//! Creates a version 4 (random) UUID.
UUID make_version4() {
  UUID u=UUID();
  u->timestamp = random(pow(2,60));
  u->version = 4;
  u->clk_seq = random(pow(2,14));
  u->node = random(pow(2,48));
  return u;
}


// Grubbas implementation:

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
