/* $Id: RSA.pike,v 1.7 2004/02/28 14:57:10 nilsson Exp $
 *
 * Follow the PKCS#1 standard for padding and encryption.
 */

#pike __REAL_VERSION__
#pragma strict_types

#if constant(Gmp.mpz) && constant(Crypto.Hash)

static Gmp.mpz n;  /* modulo */
static Gmp.mpz e;  /* public exponent */
static Gmp.mpz d;  /* private exponent (if known) */
static int size;

/* Extra info associated with a private key. Not currently used. */
   
static Gmp.mpz p;
static Gmp.mpz q;

//! Returns the RSA modulo.
Gmp.mpz get_n()
{
  return n;
}

//! Returns the RSA public exponent.
Gmp.mpz get_e()
{
  return e;
}

//! Returns the RSA private exponent (if known).
Gmp.mpz get_d()
{
  return d;
}

//! Returns the first RSA prime (if known).
Gmp.mpz get_p()
{
  return p;
}

//! Returns the second RSA prime (if known).
Gmp.mpz get_q()
{
  return q;
}

//! Returns the RSA modulo as a binary string.
string cooked_get_n()
{
  return n->digits(256);
}

//! Returns the RSA public exponent as a binary string.
string cooked_get_e()
{
  return e->digits(256);
}

//! Returns the RSA private exponent (if known) as a binary string.
string cooked_get_d()
{
  return d->digits(256);
}

//! Returns the first RSA prime (if known) as a binary string.
string cooked_get_p()
{
  return p->digits(256);
}

//! Returns the second RSA prime (if known) as a binary string.
string cooked_get_q()
{
  return q->digits(256);
}

//! Sets the public key.
this_program set_public_key(Gmp.mpz|int modulo, Gmp.mpz|int pub)
{
  n = Gmp.mpz(modulo);
  e = Gmp.mpz(pub);
  size = n->size(256);
  if (size < 12)
    error( "Too small modulo.\n" );
  return this;
}

//! Sets the private key.
//! @param extra
//!   @array
//!     @elem Gmp.mpz|int 0
//!       The first prime.
//!     @elem Gmp.mpz|int 1
//!       The second prime.
//!   @endarray
this_program set_private_key(Gmp.mpz|int priv, array(Gmp.mpz|int)|void extra)
{
  d = Gmp.mpz(priv);
  if (extra)
  {
    p = Gmp.mpz(extra[0]);
    q = Gmp.mpz(extra[1]);
    n = [object(Gmp.mpz)](p*q);
    size = n->size(256);
  }
  return this;
}

//! Returns the crypto block size, or zero if not yet set.
int query_blocksize() {
  if(!size) return 0;
  return size - 3;
}

//! Pads the @[message] to the current block size with method @[type]
//! and returns the result as an integer.
//! @param type
//!   @int
//!     @value 1
//!       The message is padded with @expr{0xff@} bytes.
//!     @value 2
//!       The message is padded with random data, using the @[random]
//!       function if provided. Otherwise
//!       @[Crypto.Random.random_string] will be used.
//!   @endint
Gmp.mpz rsa_pad(string message, int(1..2) type,
		function(int:string)|void random)
{
  string cookie;
  int len;

  len = size - 3 - sizeof(message);
  if (len < 8)
    error( "Block too large. (%d,%d)\n", sizeof(message), size-3 );

  switch(type)
  {
  case 1:
    cookie = sprintf("%@c", allocate(len, 0xff));
    break;
  case 2:
    if (random)
      cookie = replace(random(len), "\0", "\1");
    else
      cookie = replace(.Random.random_string(len), "\0", "\1");
    break;
  default:
    error( "Unknown type.\n" );
  }
  return Gmp.mpz(sprintf("%c", type) + cookie + "\0" + message, 256);
}

//! Reverse the effect of @[rsa_pad].
string rsa_unpad(Gmp.mpz block, int type)
{
  string s = block->digits(256);
  int i = search(s, "\0");

  if ((i < 9) || (sizeof(s) != (size - 1)) || (s[0] != type))
    return 0;
  return s[i+1..];
}

//! Pads the @[digest] with @[rsa_pad] type 1 and signs it.
Gmp.mpz raw_sign(string digest)
{
  return rsa_pad(digest, 1, 0)->powm(d, n);
}

//! Signs @[digest] as @[raw_sign] and returns the signature as a byte
//! string.
string cooked_sign(string digest)
{
  return raw_sign(digest)->digits(256);
}

//! Verifies the @[digest] against the signature @[s], assuming pad
//! type 1.
//! @seealso
//!   @[rsa_pad], @[raw_sign]
int(0..1) raw_verify(string digest, Gmp.mpz s)
{
  return s->powm(e, n) == rsa_pad(digest, 1, 0);
}

//! Pads the message @[s] with @[rsa_pad] type 2, signs it and returns
//! the signature as a byte string.
//! @param r
//!   Optional random function to be passed down to @[rsa_pad].
string encrypt(string s, function(int:string)|void r)
{
  return rsa_pad(s, 2, r)->powm(e, n)->digits(256);
}

//! Decrypt a message encrypted with @[encrypt].
string decrypt(string s)
{
  return rsa_unpad(Gmp.mpz(s, 256)->powm(d, n), 2);
}

//! Returns the size of the key in terms of number of bits.
int(0..) rsa_size() { return [int(0..)](size*8); }

//! Compares the public key of this RSA object with another RSA
//! object.
int(0..1) public_key_equal(this_program rsa)
{
  return n == rsa->get_n() && e == rsa->get_e();
}

// end of _rsa

//! Signs the @[message] with a PKCS-1 signature using hash algorithm
//! @[h].
Gmp.mpz sign(string message, .Hash h)
{
  return raw_sign(Standards.PKCS.Signature.build_digestinfo(message, h));
}

//! Verify PKCS-1 signature @[sign] of message @[msg] using hash
//! algorithm @[h].
int(0..1) verify(string msg, .Hash h, Gmp.mpz sign)
{
  string s = Standards.PKCS.Signature.build_digestinfo(msg, h);
  return raw_verify(s, sign);
}

//! @fixme
//!   Document this function.
string sha_sign(string message, mixed|void r)
{
  string s = Crypto.SHA1->hash(message);
  s = sprintf("%c%s%c%s", 4, "sha1", sizeof(s), s);
  return cooked_sign(s);
}
  
//! @fixme
//!   Document this function.
int sha_verify(string message, string signature)
{
  string s = Crypto.SHA1->hash(message);
  s = sprintf("%c%s%c%s", 4, "sha1", sizeof(s), s);
  return raw_verify(s, Gmp.mpz(signature, 256));
}

//! @fixme
//!   Document this function.
string md5_sign(string message, mixed|void r)
{
  string s = Crypto.MD5->hash(message);
  s = "0 0\14\6\10*\x86H\x86\xf7\15\2\5\5\0\4\20"+s;
  return cooked_sign(s);
}

//! @fixme
//!   Document this function.
int md5_verify(string message, string signature)
{
  string s = Crypto.MD5->hash(message);
  s = "0 0\14\6\10*\x86H\x86\xf7\15\2\5\5\0\4\20"+s;
  return raw_verify(s, Gmp.mpz(signature, 256));
}

//! Generate a prime with @[bits] number of bits using random function
//! @[r].
Gmp.mpz get_prime(int bits, function(int:string) r)
{
  int len = (bits + 7) / 8;
  int bit_to_set = 1 << ( (bits - 1) % 8);

  Gmp.mpz p;
  
  do {
    string s = r(len);
    p = Gmp.mpz(sprintf("%c%s", (s[0] & (bit_to_set - 1))
			      | bit_to_set, s[1..]),
		      256)->next_prime();
  } while (p->size() > bits);
  
  return p;
}

//! Generate a valid RSA key pair with the size @[bits]. A random
//! function may be provided as arguemnt @[r], otherwise
//! @[Crypto.Random.random_string] will be used. Keys must be at least
//! 128 bits.
this_program generate_key(int(128..) bits, function(int:string)|void r)
{
  if (!r)
    r = Crypto.Random.random_string;
  if (bits < 128)
    error( "Ridiculously small key.\n" );

  int s1 = bits / 2; /* Size of the first prime */
  int s2 = bits - s1;
  
  string msg = "This is a valid RSA key pair\n";

  do
  {
    Gmp.mpz p = get_prime(s1, r);
    Gmp.mpz q = get_prime(s2, r);
    Gmp.mpz phi = [object(Gmp.mpz)](Gmp.mpz([object(Gmp.mpz)](p-1))*
				    Gmp.mpz([object(Gmp.mpz)](q-1)));

    array(Gmp.mpz) gs; /* gcd(pub, phi), and pub^-1 mod phi */
    Gmp.mpz pub = Gmp.mpz(
#ifdef SSL3_32BIT_PUBLIC_EXPONENT
			 random(1 << 30) |
#endif /* SSL3_32BIT_PUBLIC_EXPONENT */
			 0x10001);

    while ((gs = pub->gcdext2(phi))[0] != 1)
      pub += 1;

    if (gs[1] < 0)
      gs[1] += phi;
    
    set_public_key( [object(Gmp.mpz)](p * q), pub);
    set_private_key(gs[1], ({ p, q }));

  } while (!sha_verify(msg, sha_sign(msg, r)));
  return this;
}

/*
 * Block cipher compatibility.
 */

static int encrypt_mode; // For block cipher compatible functions

//! Sets the public key to @[key] and the mode to encryption.
//! @seealso
//!   @[set_decrypt_key], @[crypt]
this_program set_encrypt_key(array(Gmp.mpz) key)
{
  set_public_key(key[0], key[1]);
  encrypt_mode = 1;
  return this;
}

//! Sets the public key to @[key]and the mod to decryption.
//! @seealso
//!   @[set_encrypt_key], @[crypt]
this_program set_decrypt_key(array(Gmp.mpz) key)
{
  set_public_key(key[0], key[1]);
  set_private_key(key[2]);
  encrypt_mode = 0;
  return this;
}

//! Encrypt or decrypt depending on set mode.
//! @seealso
//!   @[set_encrypt_key], @[set_decrypt_key]
string crypt(string s)
{
  return (encrypt_mode ? encrypt(s) : decrypt(s));
}

//! Returns the string @expr{"RSA"@}.
string name() {
  return "RSA";
}

#endif
