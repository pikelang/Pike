/* $Id: rsa.pike,v 1.2 2003/03/23 23:08:24 nilsson Exp $
 *
 * Follow the PKCS#1 standard for padding and encryption.
 */

#pike __REAL_VERSION__

#if constant(Gmp.mpz)

static Gmp.mpz n;  /* modulo */
static Gmp.mpz e;  /* public exponent */
static Gmp.mpz d;  /* private exponent (if known) */
int(12..) size;

/* Extra info associated with a private key. Not currently used. */
   
static Gmp.mpz p;
static Gmp.mpz q;

Gmp.mpz get_n()
{
  return n;
}

Gmp.mpz get_e()
{
  return e;
}

Gmp.mpz get_d()
{
  return d;
}

Gmp.mpz get_p()
{
  return p;
}

Gmp.mpz get_q()
{
  return q;
}

string cooked_get_n()
{
  return n->digits(256);
}

string cooked_get_e()
{
  return e->digits(256);
}

string cooked_get_d()
{
  return d->digits(256);
}

string cooked_get_p()
{
  return p->digits(256);
}

string cooked_get_q()
{
  return q->digits(256);
}

this_program set_public_key(Gmp.mpz|int modulo, Gmp.mpz|int pub)
{
  n = Gmp.mpz(modulo);
  e = Gmp.mpz(pub);
  size = n->size(256);
  if (size < 12)
    error( "Too small modulo.\n" );
  return this_object();
}

this_program set_private_key(Gmp.mpz|int priv, array(Gmp.mpz|int)|void extra)
{
  d = Gmp.mpz(priv);
  if (extra)
  {
    p = Gmp.mpz(extra[0]);
    q = Gmp.mpz(extra[1]);
    n = p*q;
    size = n->size(256);
  }
  return this_object();
}

int(9..) query_blocksize() { return size - 3; }

Gmp.mpz rsa_pad(string message, int type, mixed|void random)
{
  string cookie;
  int len;

  len = size - 3 - sizeof(message);
  /*  write(sprintf("%d, %d, %d, %s", len, size, sizeof(message), message)); */
  if (len < 8)
    error( "Block too large. (%d,%d)\n", sizeof(message), size-3 );

  switch(type)
  {
  case 1:
    cookie = sprintf("%@c", replace(allocate(len), 0, 0xff));
    break;
  case 2:
    if (random)
      cookie = replace(random(len), "\0", "\1");
    else
      cookie = sprintf("%@c", map(allocate(len), lambda(int dummy)
					{
					  return predef::random(255) + 1;
					} ));
    break;
  default:
    error( "Unknown type.\n" );
  }
  return Gmp.mpz(sprintf("%c", type) + cookie + "\0" + message, 256);
}

string rsa_unpad(Gmp.mpz block, int type)
{
  string s = block->digits(256);
  int(-1..) i = search(s, "\0");

  if ((i < 9) || (sizeof(s) != (size - 1)) || (s[0] != type))
    return 0;
  return s[i+1..];
}

Gmp.mpz raw_sign(string digest)
{
  return rsa_pad(digest, 1, 0)->powm(d, n);
}

string cooked_sign(string digest)
{
  return raw_sign(digest)->digits(256);
}

int(0..1) raw_verify(string digest, Gmp.mpz s)
{
  return s->powm(e, n) == rsa_pad(digest, 1, 0);
}

string encrypt(string s, mixed|void r)
{
  return rsa_pad(s, 2, r)->powm(e, n)->digits(256);
}

string decrypt(string s)
{
  return rsa_unpad(Gmp.mpz(s, 256)->powm(d, n), 2);
}

int(0..) rsa_size() { return n->size(); }

int(0..1) public_key_equal(this_program rsa)
{
  return n == rsa->get_n() && e == rsa->get_e();
}

// end of _rsa

//! @fixme
//!   Document this function.
Gmp.mpz sign(string message, program h)
{
  return raw_sign(Standards.PKCS.Signature.build_digestinfo(message, h()));
}

//! @fixme
//!   Document this function.
int(0..1) verify(string msg, program h, object sign)
{
  string s = Standards.PKCS.Signature.build_digestinfo(msg, h());
  return raw_verify(s, sign);
}

//! @fixme
//!   Document this function.
string sha_sign(string message, mixed|void r)
{
  Crypto.sha hash = Crypto.sha();
  string s;

  hash->update(message);
  s = hash->digest();
  s = sprintf("%c%s%c%s", 4, "sha1", sizeof(s), s);
  return cooked_sign(s);
}
  
//! @fixme
//!   Document this function.
int sha_verify(string message, string signature)
{
  Crypto.sha hash = Crypto.sha();
  string s;
  
  hash->update(message);
  s = hash->digest();
  s = sprintf("%c%s%c%s", 4, "sha1", sizeof(s), s);

  return raw_verify(s, Gmp.mpz(signature, 256));
}

//! @fixme
//!   Document this function.
string md5_sign(string message, mixed|void r)
{
  Crypto.md5 hash = Crypto.md5();
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
  Crypto.md5 hash = Crypto.md5();
  string s;
  
  hash->update(message);
  s = hash->digest();
  s = "0 0\14\6\10*\x86H\x86\xf7\15\2\5\5\0\4\20"+s;

  return raw_verify(s, Gmp.mpz(signature, 256));
}

//! @fixme
//!   Document this function.
Gmp.mpz get_prime(int bits, function r)
{
  int len = (bits + 7) / 8;
  int bit_to_set = 1 << ( (bits - 1) % 8);

  object p;
  
  do {
    string s = r(len);
    p = Gmp.mpz(sprintf("%c%s", (s[0] & (bit_to_set - 1))
			      | bit_to_set, s[1..]),
		      256)->next_prime();
  } while (p->size() > bits);
  
  return p;
}

//! @fixme
//!   Document this function.
this_program generate_key(int bits, function|void r)
{
  if (!r)
    r = Crypto.randomness.really_random()->read;
  if (bits < 128)
    error( "Ridicously small key.\n" );

  int s1 = bits / 2; /* Size of the first prime */
  int s2 = bits - s1;
  
  string msg = "This is a valid RSA key pair\n";
  
  do
  {
    object p = get_prime(s1, r);
    object q = get_prime(s2, r);
    Gmp.mpz phi = Gmp.mpz(p-1)*Gmp.mpz(q-1);

    array gs; /* gcd(pub, phi), and pub^-1 mod phi */
    Gmp.mpz pub = Gmp.mpz(
#ifdef SSL3_32BIT_PUBLIC_EXPONENT
			 random(1 << 30) |
#endif /* SSL3_32BIT_PUBLIC_EXPONENT */
			 0x10001);

    while ((gs = pub->gcdext2(phi))[0] != 1)
      pub += 1;

    if (gs[1] < 0)
      gs[1] += phi;
    
    set_public_key(p * q, pub);
    set_private_key(gs[1], ({ p, q }));

  } while (!sha_verify(msg, sha_sign(msg, r)));
  return this_object();
}

/*
 * Block cipher compatibility.
 */

int encrypt_mode; /* For block cipher compatible functions */

//! @fixme
//!   Document this function.
this_program set_encrypt_key(array(Gmp.mpz) key)
{
  set_public_key(key[0], key[1]);
  encrypt_mode = 1;
  return this_object();
}

//! @fixme
//!   Document this function.
this_program set_decrypt_key(array(Gmp.mpz) key)
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
