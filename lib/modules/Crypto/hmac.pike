
//! HMAC, defined by RFC-2104

#pike __REAL_VERSION__

function H;  /* Constructor for hash object */

/* B is the size of one compression block, in octets. */
int B;

//! @fixme
//!   Document this function.
void create(function h, int|void b)
{
  // werror("hmac.pike->create()\n");
  H = h;
  
  /* Block size is 64 octets for md5 and sha */
  B = b || 64;
}

//! @fixme
//!   Document this function.
string raw_hash(string s)
{
  return H()->update(s)->digest();
}

//! @fixme
//!   Document this function.
string pkcs_digest(string s)
{
  return Standards.PKCS.Signature.build_digestinfo(s, H());
}

//! @fixme
//!   Document this function.
class `()
{
  string ikey; /* ipad XOR:ed with the key */
  string okey; /* opad XOR:ed with the key */

  //! @fixme
  //!   Document this function.
  void create(string passwd)
    {
      if (sizeof(passwd) < B)
	passwd = passwd + "\0" * (B - sizeof(passwd));

      ikey = passwd ^ ("6" * B);
      okey = passwd ^ ("\\" * B);
    }

  //! @fixme
  //!   Document this function.
  string `()(string text)
    {
      return raw_hash(okey + raw_hash(ikey + text));
    }

  //! @fixme
  //!   Document this function.
  string digest_info(string text)
    {
      return pkcs_digest(okey + raw_hash(ikey + text));
    }
}

