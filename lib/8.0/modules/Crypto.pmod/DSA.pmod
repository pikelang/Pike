#pike 8.1
#require constant(Crypto.Hash)

inherit Crypto.DSA : pre;

class State
{
  inherit pre::State;

  //! Make a RSA ref signature of message @[msg].
  string(8bit) sign_rsaref(string(8bit) msg)
  {
    [Gmp.mpz r, Gmp.mpz s] = raw_sign(hash(msg, Crypto.SHA1));

    return sprintf("%'\0'20s%'\0'20s", r->digits(256), s->digits(256));
  }

  //! Verify a RSA ref signature @[s] of message @[msg].
  int(0..1) verify_rsaref(string(8bit) msg, string(8bit) s)
  {
    if (sizeof(s) != 40)
      return 0;

    return raw_verify(hash(msg, Crypto.SHA1),
		      Gmp.mpz(s[..19], 256),
		      Gmp.mpz(s[20..], 256));
  }

  //! Make an SSL signature of message @[msg].
  string(8bit) sign_ssl(string(8bit) msg)
  {
    return pkcs_sign(msg, Crypto.SHA1);
  }

  //! Verify an SSL signature @[s] of message @[msg].
  int(0..1) verify_ssl(string(8bit) msg, string(8bit) s)
  {
    return pkcs_verify(msg, Crypto.SHA1, s);
  }
}