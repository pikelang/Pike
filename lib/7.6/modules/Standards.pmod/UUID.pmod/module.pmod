/*
 * Compatibility with Pike 7.6 Standards.UUID.
 *
 * $Id$
 *
 * Henrik Grubbström 2005-01-06
 */

#pike 7.7

//! Return a new binary UUID.
string new()
{
  return Standards.UUID.make_version1(-1)->encode();
}

//! Return a new UUID string.
string new_string()
{
  return Standards.UUID.make_version1(-1)->str();
}

#if constant(Crypto.MD5.hash)

//! Creates a version 3 UUID with a @[name] string and a binary
//! representation of a name space UUID.
//!
//! @note
//!   This implementation differs from
//!   @[predef::Standards.UUID.make_version3()] in that
//!   it does not attempt to normalize the @[namespace],
//!   and thus accepts malformed "UUIDs".
Standards.UUID.UUID make_version3(string name, string namespace) {

  // FIXME: I don't see why the following reversals are needed;
  //        the namespace should already be in network byte order.
  //  /grubba 2004-10-05
  //    Otherwise we get a result that differs from the example
  //    execution of the reference implementation in the Internet
  //    Draft.
  //  /nilsson 2004-10-05
  //    The implemenetaion in the Internet Draft is bugged.
  //    It contains code like the following:
  //        htonl(net_nsid.time_low);
  //    where it is supposed to say
  //        net_nsid.time_low = htonl(net_nsid.time_low);
  //    Therefore it gives incorrect results on little endian
  //    machines.  The code without reverse gives the same
  //    result as the reference implementation _when run on a
  //    big endian system_, and also the same as e.g. the Python
  //    UUID module.
  //  /marcus 2007-08-05

  // step 2
#if 0
  namespace = reverse(namespace[0..3]) + reverse(namespace[4..5]) +
    reverse(namespace[6..7]) + namespace[8..];
#endif

  // step 3
  string ret = Crypto.MD5.hash(namespace+name);

#if 0
  ret = reverse(ret[0..3]) + reverse(ret[4..5]) +
    reverse(ret[6..7]) + ret[8..];
#endif

  ret &=
    "\xff\xff\xff\xff"		// time_low
    "\xff\xff"			// time_mid
    "\x0f\xff"			// time_hi_and_version
    "\x3f\xff"			// clock_seq_and_variant
    "\xff\xff\xff\xff\xff\xff";	// node

  ret |=
    "\x00\x00\x00\x00"		// time_low
    "\x00\x00"			// time_mid
    "\x30\x00"			// time_hi_and_version
    "\x80\x00"			// clock_seq_and_veriant
    "\x00\x00\x00\x00\x00\x00";	// node

  return Standards.UUID.UUID(ret);
}

#endif

