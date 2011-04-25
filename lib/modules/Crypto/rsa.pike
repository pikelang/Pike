/* $Id$
 *
 * Follow the PKCS#1 standard for padding and encryption.
 */

#pike __REAL_VERSION__

#if constant(Gmp.mpz)

#define bignum object(Gmp.mpz)
#define BIGNUM (Gmp.mpz)

#ifdef USE_RSA_WRAPPER

//!
static class rsa_wrapper
{
  object _rsa_c;
  object _rsa_pike;

  string n = "n is not in the API";
  string e = "e is not in the API";
  string d = "d is not in the API";
  string size = "size is not in the API";

  string p = "p is not in the API";
  string q = "q is not in the API";

  //! @appears Crypto.rsaset_public_key
  //! @fixme
  //!   Document this function.
  object set_public_key(bignum modulo, bignum pub)
  {
    _rsa_pike->set_public_key(modulo, pub);
    _rsa_c->set_public_key(modulo, pub);

    return this_object();
  }

  object set_private_key(bignum priv, array(bignum)|void extra)
  {
    _rsa_pike->set_private_key(priv, extra);
    _rsa_c->set_private_key(priv, extra);

    return this_object();
  }

  //! @appears Crypto.rsa.query_blocksize
  //! @fixme
  //!   Document this function.
  int query_blocksize()
  {
    int res1 = _rsa_pike->query_blocksize();
    int res2 = _rsa_c->query_blocksize();

    if (res1 != res2) {
      werror("RSA: query_blocksize() failed!\n");
      error(sprintf("res1:%O != res2:%O\n", res1, res2));
    }
    return res1;
  }

  //! @appears Crypto.rsa.rsa_pad
  //! @fixme
  //!   Document this function.
  bignum rsa_pad(string message, int type, mixed|void random)
  {
    bignum res1 = _rsa_pike->rsa_pad(message, type, random);
    bignum res2 = _rsa_c->rsa_pad(message, type, random);

    if (res1 != res2) {
      werror("RSA: rsa_pad() failed!\n");
      error(sprintf("res1:%O != res2:%O\n", res1, res2));
    }
    return res1;
  }

  //! @appears Crypto.rsa.rsa_unpad
  //! @fixme
  //!   Document this function.
  string rsa_unpad(bignum block, int type)
  {
    string res1 = _rsa_pike->rsa_unpad(block, type);
    string res2 = _rsa_c->rsa_unpad(block, type);

    if (res1 != res2) {
      werror("RSA: rsa_unpad() failed!\n");
      error(sprintf("res1:%O != res2:%O\n", res1, res2));
    }
    return res1;
  }

  bignum raw_sign(string digest)
  {
    bignum res1 = _rsa_pike->raw_sign(digest);
    bignum res2 = Gmp.mpz(_rsa_c->cooked_sign(digest), 256);

    if (res1 != res2) {
      werror("RSA: rsa_sign() failed!\n");
      error(sprintf("res1:%O != res2:%O\n", res1, res2));
    }
    return res1;
  }

  //! @appears Crypto.rsa.cooked_sign
  //! @fixme
  //!   Document this function.
  string cooked_sign(string digest)
  {
    string res1 = _rsa_pike->cooked_sign(digest);
    string res2 = _rsa_c->cooked_sign(digest);

    if (res1 != res2) {
      werror("RSA: cooked_sign() failed!\n");
      error(sprintf("res1:%O != res2:%O\n", res1, res2));
    }
    return res1;
  }

  //! @appears Crypto.rsa.raw_verify
  //! @fixme
  //!   Document this function.
  int raw_verify(string digest, bignum s)
  {
    int res1 = _rsa_pike->cooked_sign(digest, s);
    int res2 = _rsa_c->cooked_sign(digest, s);

    if (res1 != res2) {
      werror("RSA: rsa_verify() failed!\n");
      error(sprintf("res1:%O != res2:%O\n", res1, res2));
    }
    return res1;
  }

  //! @appears Crypto.rsa.encrypt
  //! @fixme
  //!   Document this function.
  string encrypt(string s, mixed|void r)
  {
    string res1 = _rsa_pike->encrypt(s, r);
    string res2 = _rsa_c->encrypt(s, r);

    if (res1 != res2) {
      werror("RSA: encrypt() failed!\n");
      error(sprintf("res1:%O != res2:%O\n", res1, res2));
    }
    return res1;
  }

  //! @appears Crypto.rsa.decrypt
  //! @fixme
  //!   Document this function.
  string decrypt(string s)
  {
    string res1 = _rsa_pike->decrypt(s);
    string res2 = _rsa_c->decrypt(s);

    if (res1 != res2) {
      werror("RSA: decrypt() failed!\n");
      error(sprintf("res1:%O != res2:%O\n", res1, res2));
    }
    return res1;
  }

  //! @appears Crypto.rsa.rsa_size
  //! @fixme
  //!   Document this function.
  int rsa_size()
  {
    int res1 = _rsa_pike->rsa_size();
    int res2 = _rsa_c->rsa_size();

    if (res1 != res2) {
      werror("RSA: rsa_size() failed!\n");
      error(sprintf("res1:%O != res2:%O\n", res1, res2));
    }
    return res1;
  }

  //! @appears Crypto.rsa.public_key_equal
  //! @fixme
  //!   Document this function.
  int public_key_equal(object rsa_wrapper)
  {
    int res1 = _rsa_pike->public_key_equal(rsa_wrapper->_rsa_pike);
    int res2 = _rsa_c->public_key_equal(rsa_wrapper->_rsa_c);

    if (res1 != res2) {
      werror("RSA: public_key_equal() failed!\n");
      error(sprintf("res1:%O != res2:%O\n", res1, res2));
    }
    return res1;
  }

  object get_n()
  {
    object res1 = _rsa_pike->get_n();
    object res2 = BIGNUM(_rsa_c->cooked_get_n(), 256);

    if (res1 != res2) {
      werror("RSA: get_n() failed!\n");
      error(sprintf("res1:%O != res2:%O\n", res1, res2));
    }
    return res1;
  }

  object get_e()
  {
    object res1 = _rsa_pike->get_e();
    object res2 = BIGNUM(_rsa_c->cooked_get_e(), 256);

    if (res1 != res2) {
      werror("RSA: get_e() failed!\n");
      error(sprintf("res1:%O != res2:%O\n", res1, res2));
    }
    return res1;
  }

  object get_d()
  {
    object res1 = _rsa_pike->get_d();
    object res2 = BIGNUM(_rsa_c->cooked_get_d(), 256);

    if (res1 != res2) {
      werror("RSA: get_e() failed!\n");
      error(sprintf("res1:%O != res2:%O\n", res1, res2));
    }
    return res1;
  }

  object get_p()
  {
    object res1 = _rsa_pike->get_p();
    object res2 = BIGNUM(_rsa_c->cooked_get_p(), 256);

    if (res1 != res2) {
      werror("RSA: get_p() failed!\n");
      error(sprintf("res1:%O != res2:%O\n", res1, res2));
    }
    return res1;
  }

  object get_q()
  {
    object res1 = _rsa_pike->get_q();
    object res2 = BIGNUM(_rsa_c->cooked_get_q(), 256);

    if (res1 != res2) {
      werror("RSA: get_q() failed!\n");
      error(sprintf("res1:%O != res2:%O\n", res1, res2));
    }
    return res1;
  }

  static void create()
  {
    werror("RSA: Using rsa_wrapper\n");
    _rsa_c = _Crypto._rsa();
    _rsa_pike = ((program)"_rsa.pike")();
  }
}

//! @ignore
inherit rsa_wrapper;
#else /* !USE_RSA_WRAPPER */
#ifdef USE_PIKE_RSA
inherit "_rsa.pike";
#else /* !USE_PIKE_RSA */
inherit Crypto._rsa;
#endif /* USE_PIKE_RSA */
#endif /* !USE_RSA_WRAPPER */
//! @endignore

#if !defined(USE_PIKE_RSA) && !defined(USE_RSA_WRAPPER) && constant(_Crypto._rsa)
// Only the cooked variant is implemented in C. */

//! @fixme
//!   Document this function.
bignum raw_sign(string digest)
{
  return BIGNUM(cooked_sign(digest), 256);
}

//! @fixme
//!   Document this function.
bignum get_n()
{
  return BIGNUM(cooked_get_n(), 256);
}

//! @fixme
//!   Document this function.
bignum get_e()
{
  return BIGNUM(cooked_get_e(), 256);
}

//! @fixme
//!   Document this function.
bignum get_d()
{
  return BIGNUM(cooked_get_d(), 256);
}

/* Handle p and q by hand.
 * They are only used for key-generation.
 */
static bignum p;
static bignum q;

//! @fixme
//!   Document this function.
bignum get_p()
{
  return p;
}

//! @fixme
//!   Document this function.
bignum get_q()
{
  return q;
}

//! @fixme
//!   Document this function.
string cooked_get_p()
{
  return p->digits(256);
}

//! @fixme
//!   Document this function.
string cooked_get_q()
{
  return q->digits(256);
}

//! @fixme
//!   Document this function.
object set_private_key(bignum priv, array(bignum)|void extra)
{
  if (extra) {
    p = extra[0];
    q = extra[1];
  }
  return ::set_private_key(priv);
}
#endif /* !USE_PIKE_RSA && !USE_RSA_WRAPPER && constant(_Crypto._rsa) */

//! @fixme
//!   Document this function.
object sign(string message, program h)
{
  return raw_sign(Standards.PKCS.Signature.build_digestinfo(message, h()));
}

//! @fixme
//!   Document this function.
int verify(string msg, program h, object sign)
{
  // werror(sprintf("msg: '%s'\n", Crypto.string_to_hex(msg)));
  string s = Standards.PKCS.Signature.build_digestinfo(msg, h());
  // werror(sprintf("rsa: s = '%s'\n", s));
  return raw_verify(s, sign);
}

//! @fixme
//!   Document this function.
string sha_sign(string message, mixed|void r)
{
  object hash = Crypto.sha();
  string s;

  hash->update(message);
  s = hash->digest();
  s = sprintf("%c%s%c%s", 4, "sha1", strlen(s), s);
  return cooked_sign(s);
}
  
//! @fixme
//!   Document this function.
int sha_verify(string message, string signature)
{
  object hash = Crypto.sha();
  string s;
  
  hash->update(message);
  s = hash->digest();
  s = sprintf("%c%s%c%s", 4, "sha1", strlen(s), s);

  return raw_verify(s, BIGNUM(signature, 256));
}

//! @fixme
//!   Document this function.
string md5_sign(string message, mixed|void r)
{
  object hash = Crypto.md5();
  string s;
  
  hash->update(message);
  s = hash->digest();
  s = "0 0\14\6\10*\x86H\x86\xf7\15\2\5\5\0\4\20"+s;
  return cooked_sign(s);
}

//! @fixme
//!   Document this function.
int md5_verify(string message, string signature)
{
  object hash = Crypto.md5();
  string s;
  
  hash->update(message);
  s = hash->digest();
  s = "0 0\14\6\10*\x86H\x86\xf7\15\2\5\5\0\4\20"+s;

  return raw_verify(s, BIGNUM(signature, 256));
}

//! @fixme
//!   Document this function.
bignum get_prime(int bits, function r)
{
  int len = (bits + 7) / 8;
  int bit_to_set = 1 << ( (bits - 1) % 8);

  object p;
  
  do {
    string s = r(len);
    p = BIGNUM(sprintf("%c%s", (s[0] & (bit_to_set - 1))
			      | bit_to_set, s[1..]),
		      256)->next_prime();
  } while (p->size() > bits);
  
  return p;
}

//! @fixme
//!   Document this function.
object generate_key(int bits, function|void r)
{
  if (!r)
    r = Crypto.randomness.really_random()->read;
  if (bits < 128)
    error( "Crypto.rsa->generate_key: ridicously small key.\n" );

  int s1 = bits / 2; /* Size of the first prime */
  int s2 = bits - s1;
  
  string msg = "This is a valid RSA key pair\n";
  
  do
  {
    object p = get_prime(s1, r);
    object q = get_prime(s2, r);
    bignum phi = Gmp.mpz(p-1)*Gmp.mpz(q-1);

    array gs; /* gcd(pub, phi), and pub^-1 mod phi */
    bignum pub = Gmp.mpz(
#ifdef SSL3_32BIT_PUBLIC_EXPONENT
			 random(1 << 30) |
#endif /* SSL3_32BIT_PUBLIC_EXPONENT */
			 0x10001);

    while ((gs = pub->gcdext2(phi))[0] != 1)
      pub += 1;

//    werror(sprintf("p = %s\nq = %s\ne = %s\nd = %s\n",
//		   p->digits(), q->digits(), pub->digits(), gs[1]->digits()));
    if (gs[1] < 0)
      gs[1] += phi;
    
    set_public_key(p * q, pub);
    set_private_key(gs[1], ({ p, q }));

//    werror(sprintf("p = %s\nq = %s\ne = %s\nd = %s\n",
//		   p->digits(), q->digits(), pub->digits(), gs[1]->digits()));
  } while (!sha_verify(msg, sha_sign(msg, r)));
  return this_object();
}

/*
 * Block cipher compatibility.
 */

int encrypt_mode; /* For block cipher compatible functions */

//! @fixme
//!   Document this function.
object set_encrypt_key(array(bignum) key)
{
  set_public_key(key[0], key[1]);
  encrypt_mode = 1;
  return this_object();
}

//! @fixme
//!   Document this function.
object set_decrypt_key(array(bignum) key)
{
  set_public_key(key[0], key[1]);
  set_private_key(key[2]);
  encrypt_mode = 0;
  return this_object();
}

//! @fixme
//!   Document this function.
string crypt_block(string s)
{
  return (encrypt_mode ? encrypt(s) : decrypt(s));
}

#endif
