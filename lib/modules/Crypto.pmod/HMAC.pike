
//! HMAC, defined by RFC-2104

#pike __REAL_VERSION__

static .Hash H;  // hash object

// B is the size of one compression block, in octets.
static int B;

//! @param h
//!   The hash object on which the HMAC object should base its
//!   operations. Typical input is @[Crypto.MD5].
//! @param b
//!   The block size of one compression block, in octets. Defaults to
//!   64.
void create(.Hash h, int|void b)
{
  H = h;
  
  // Block size is 64 octets for md5 and sha
  B = b || 64;
  if(H->digest_size()>B)
    error("Block size is less than hash digest size.\n");
}

//! Calls the hash function given to @[create] and returns the hash
//! value of @[s].
string raw_hash(string s)
{
  return H->hash(s);
}

//! Makes a PKCS-1 digestinfo block with the message @[s].
//! @seealso
//!   @[Standards.PKCS.Signature.build_digestinfo]
string pkcs_digest(string s)
{
  return Standards.PKCS.Signature.build_digestinfo(s, H);
}

//! Calling the HMAC object with a password returns a new object that
//! can perform the actual HMAC hashing. E.g. doing a HMAC hash with
//! MD5 and the password @expr{"bar"@} of the string @expr{"foo"@}
//! would require the code @expr{Crypto.HMAC(Crypto.MD5)("bar")("foo")@}.
class `()
{
  string ikey; /* ipad XOR:ed with the key */
  string okey; /* opad XOR:ed with the key */

  //! @param passwd
  //!   The secret password (K).
  void create(string passwd)
    {
      if (sizeof(passwd) > B)
	passwd = raw_hash(passwd);
      if (sizeof(passwd) < B)
	passwd = passwd + "\0" * (B - sizeof(passwd));

      ikey = passwd ^ ("6" * B);
      okey = passwd ^ ("\\" * B);
    }

  //! Hashes the @[text] according to the HMAC algorithm and returns
  //! the hash value.
  string `()(string text)
    {
      return raw_hash(okey + raw_hash(ikey + text));
    }

  //! Hashes the @[text] according to the HMAC algorithm and returns
  //! the hash value as a PKCS-1 digestinfo block.
  string digest_info(string text)
    {
      return pkcs_digest(okey + raw_hash(ikey + text));
    }
}

