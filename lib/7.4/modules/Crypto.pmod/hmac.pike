
#pike 7.5

//! @deprecated Crypto.HMAC

inherit Crypto.HMAC;

protected class Wrapper {
  protected object h;
  function(void:mixed) asn1_id;
  void create(object _h) {
    h = _h;
    if (h()->identifier)
      asn1_id = lambda() {
		  return Standards.ASN1.Types.Identifier()->
		    decode_primitive(h()->identifier());
		};
  }
  string hash(string m) { return h()->update(m)->digest(); }
  int digest_size() { return sizeof(hash("")); }
  class HMAC
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

      ikey = (passwd ^ ("6" * B));
      okey = (passwd ^ ("\\" * B));
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
  };
}

void create(object h, void|int b) {
  ::create(Wrapper(h), b||64);
}
