
//! HMAC, defined by RFC-2104

#pike __REAL_VERSION__
#pragma strict_types

#if constant(Crypto.Hash)

protected .Hash H;  // hash object

// B is the size of one compression block, in octets.
protected int B;

//! @param h
//!   The hash object on which the HMAC object should base its
//!   operations. Typical input is @[Crypto.MD5].
//! @param b
//!   The block size of one compression block, in octets. Defaults to
//!   block_size() of @[h].
protected void create(.Hash h, int|void b)
{
  H = h;
  B = b || H->block_size();

  if(H->digest_size()>B)
    error("Block size is less than hash digest size.\n");
}

//! Calls the hash function given to @[create] and returns the hash
//! value of @[s].
string(8bit) raw_hash(string(8bit) s)
{
  return H->hash(s);
}

//! Makes a PKCS-1 digestinfo block with the message @[s].
//! @seealso
//!   @[Standards.PKCS.Signature.build_digestinfo]
string(8bit) pkcs_digest(string(8bit) s)
{
  return Standards.PKCS.Signature.build_digestinfo(s, H);
}

//! Calling the HMAC object with a password returns a new object that
//! can perform the actual HMAC hashing. E.g. doing a HMAC hash with
//! MD5 and the password @expr{"bar"@} of the string @expr{"foo"@}
//! would require the code @expr{Crypto.HMAC(Crypto.MD5)("bar")("foo")@}.
Crypto.Hash.HMAC `()(string(8bit) passwd)
{
  return H->HMAC(passwd, B);
}

#else
constant this_program_does_not_exist=1;
#endif
