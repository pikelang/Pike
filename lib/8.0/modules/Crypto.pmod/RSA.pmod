#pike 8.1
#require constant(Crypto.Hash)

inherit Crypto.RSA : pre;

class State
{
  inherit pre::State;

#if 0 // This doesn't work correctly
  //! Compatibility with Pike 7.8.
  this_program generate_key(int(128..) bits,
                            void|int|function(int(0..):string(8bit)) rnd)
  {
    if( intp(rnd) ) return ::generate_key(bits, rnd);
    function(int(0..):string(8bit)) old_rnd = random;
    random = rnd;
    this_program res;
    mixed err = catch {
	res = ::generate_key(bits);
      };
    random = old_rnd;
    if (err) throw(err);
    return res;
  }
#endif

  //! Returns the RSA modulo (n) as a binary string.
  string(8bit) cooked_get_n()
  {
    return n->digits(256);
  }

  //! Returns the RSA public exponent (e) as a binary string.
  string(8bit) cooked_get_e()
  {
    return e->digits(256);
  }

  //! Returns the RSA private exponent (d) as a binary string, if known.
  string(8bit) cooked_get_d()
  {
    return d->digits(256);
  }

  //! Returns the first RSA prime (p) as a binary string, if known.
  string(8bit) cooked_get_p()
  {
    return p->digits(256);
  }

  //! Returns the second RSA prime (q) as a binary string, if known.
  string(8bit) cooked_get_q()
  {
    return q->digits(256);
  }

  //! Signs @[digest] as @[raw_sign] and returns the signature as a byte
  //! string.
  string(8bit) cooked_sign(string(8bit) digest)
  {
    return raw_sign(digest)->digits(256);
  }

  int query_blocksize() {
    return block_size();
  }

  int(0..) rsa_size() { return [int(0..)](size*8); }


  // Broken implementation of RSA/MD5 SIG RFC 2537. The 0x00 01 FF* 00
  // prefix is missing.

  // (RSA/SHA-1 SIG is in RFC 3110)

#if constant(Crypto.MD5)

  string(8bit) md5_sign(string(8bit) message, mixed|void r)
  {
    string(8bit) s = Crypto.MD5->hash(message);
    s = "0 0\14\6\10*\x86H\x86\xf7\15\2\5\5\0\4\20"+s;
    return raw_sign(s)->digits(256);r;
  }

  int md5_verify(string(8bit) message, string(8bit) signature)
  {
    string(8bit) s = Crypto.MD5->hash(message);
    s = "0 0\14\6\10*\x86H\x86\xf7\15\2\5\5\0\4\20"+s;
    return raw_verify(s, Gmp.mpz(signature, 256));
  }

#endif

  string(8bit) sha_sign(string(8bit) message, mixed|void r)
  {
    string(8bit) s = sprintf("%c%s%1H", 4, "sha1", Crypto.SHA1->hash(message));
    return raw_sign(s)->digits(256);r;
  }

  int sha_verify(string(8bit) message, string(8bit) signature)
  {
    string(8bit) s = sprintf("%c%s%1H", 4, "sha1", Crypto.SHA1->hash(message));
    return raw_verify(s, Gmp.mpz(signature, 256));
  }

  Gmp.mpz sign(string(8bit) message, Crypto.Hash h)
  {
    return raw_sign(Standards.PKCS.Signature.build_digestinfo(message, h));
  }

  int(0..1) verify(string(8bit) message, Crypto.Hash h, Gmp.mpz sign)
  {
    return raw_verify(Standards.PKCS.Signature.build_digestinfo(message, h),
		      sign);
  }
}