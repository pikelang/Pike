/* hmac.pike
 *
 * HMAC, defined by RFC-2104
 */

function H;  /* Constructor for hash object */

/* B is the size of one compression block, in octets. */
int B;

void create(function h, int|void b)
{
  // werror("hmac.pike->create()\n");
  H = h;
  
  /* Block size is 64 octets for md5 and sha */
  B = b || 64;
}

string raw_hash(string s)
{
  return H()->update(s)->digest();
}

string pkcs_digest(string s)
{
  return Standards.PKCS.Signature.build_digestinfo(s, H());
}

class `()
{
  string ikey; /* ipad XOR:ed with the key */
  string okey; /* opad XOR:ed with the key */

  void create(string passwd)
    {
      if (strlen(passwd) < B)
	passwd = passwd + "\0" * (B - strlen(passwd));

      ikey = passwd ^ ("6" * B);
      okey = passwd ^ ("\\" * B);
    }
      
  string `()(string text)
    {
      return raw_hash(okey + raw_hash(ikey + text));
    }

  string digest_info(string text)
    {
      return pkcs_digest(okey + raw_hash(ikey + text));
    }
}

