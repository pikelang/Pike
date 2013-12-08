/*
 * Follow the PKCS#1 standard for padding and encryption.
 */

#pike __REAL_VERSION__
#pragma strict_types

#if constant(Crypto.Hash)

//
// --- Variables and accessors
//

protected Gmp.mpz n;  /* modulo */
protected Gmp.mpz e;  /* public exponent */
protected Gmp.mpz d;  /* private exponent (if known) */
protected int size;

/* Extra info associated with a private key. Not currently used. */
   
protected Gmp.mpz p;
protected Gmp.mpz q;

protected function(int:string(0..255)) random = .Random.random_string;

Gmp.mpz get_n() { return n; } //! Returns the RSA modulo (n).
Gmp.mpz get_e() { return e; } //! Returns the RSA public exponent (e).

//! Returns the RSA private exponent (d), if known.
Gmp.mpz get_d() { return d; }

Gmp.mpz get_p() { return p; } //! Returns the first RSA prime (p), if known.
Gmp.mpz get_q() { return q; } //! Returns the second RSA prime (q), if known.

//! Sets the random function, used to generate keys and parameters, to
//! the function @[r]. Default is @[Crypto.Random.random_string].
this_program set_random(function(int:string(0..255)) r)
{
  random = r;
  return this;
}

//! Returns the string @expr{"RSA"@}.
string(0..255) name() { return "RSA"; }

//
// --- Key methods
//

//! Can be initialized with a mapping with the elements n, e, d, p and
//! q.
protected void create(mapping(string(0..255):Gmp.mpz|int)|void params)
{
  if(!params) return;
  if( params->n && params->e )
    set_public_key(params->n, params->e);
  if( params->d )
    set_private_key(params->d, ({ params->p, params->q, params->n }));
}

//! Sets the public key.
//! @param modulo
//!   The RSA modulo, often called n. This value needs to be >=12.
//! @param pub
//!   The public RSA exponent, often called e.
this_program set_public_key(Gmp.mpz|int modulo, Gmp.mpz|int pub)
{
  n = Gmp.mpz(modulo);
  e = Gmp.mpz(pub);
  size = n->size(256);
  if (size < 12)
    error( "Too small modulo.\n" );
  return this;
}

//! Compares the public key of this RSA object with another RSA
//! object.
int(0..1) public_key_equal(this_program rsa)
{
  return n == rsa->get_n() && e == rsa->get_e();
}

//! Sets the private key.
//! @param priv
//!   The private RSA exponent, often called d.
//! @param extra
//!   @array
//!     @elem Gmp.mpz|int 0
//!       The first prime, often called p.
//!     @elem Gmp.mpz|int 1
//!       The second prime, often called q.
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

//
// --- Key generation
//

#if constant(Nettle.rsa_generate_keypair)

this_program generate_key(int bits, void|int e)
{
  // While a smaller e is possible, and more efficient, using 0x10001
  // has become standard and is the only value supported by several
  // TLS implementations.
  if(!e)
    e = 0x10001;
  else
  {
    if(!(e&1)) error("e needs to be odd.\n");
    if(e<3) error("e is too small.\n");
    if(e->size()>bits) error("e has to be smaller in size than the key.\n");
  }

  if(bits<89) error("Too small key length.\n");

  array(Gmp.mpz) key = Nettle.rsa_generate_keypair(bits, e,
                                                   random);
  if(!key) error("Error generating key.\n");
  [ n, d, p, q ] = key;
  this_program::e = Gmp.mpz(e);
  size = n->size(256);
  return this;
}

#else

// Generate a prime with @[bits] number of bits using random function
// @[r].
protected Gmp.mpz get_prime(int bits, function(int:string(0..255)) r)
{
  int len = (bits + 7) / 8;
  int bit_to_set = 1 << ( (bits - 1) % 8);

  Gmp.mpz p;
  
  do {
    string(0..255) s = r(len);
    p = Gmp.mpz(sprintf("%c%s", (s[0] & (bit_to_set - 1))
			      | bit_to_set, s[1..]),
		      256)->next_prime();
  } while (p->size() > bits);
  
  return p;
}

//! Generate a valid RSA key pair with the size @[bits] using the
//! random function set with @[set_random()]. The public exponent @[e]
//! will be used, which defaults to 65537. Keys must be at least 89
//! bits.
this_program generate_key(int(128..) bits, void|int e)
{
  if (bits < 128)
    error( "Ridiculously small key.\n" );
  if( e )
  {
    if(!(e&1)) error("e needs to be odd.\n");
    if(e<3) error("e is too small.\n");
    if(e->size()>bits) error("e has to be smaller in size than the key.\n");
  }

  /* NB: When multiplying two n-bit integers,
   *     you're most likely to get an (2n - 1)-bit result.
   *     We therefore add an extra bit to s2.
   *
   * cf [bug 6620].
   */

  int s1 = bits / 2; /* Size of the first prime */
  int s2 = 1 + bits - s1;
  
  string(0..255) msg = "A" * (bits/8-3-8);

  do
  {
    Gmp.mpz p;
    Gmp.mpz q;
    Gmp.mpz mod;
    do {
      p = get_prime(s1, random);
      q = get_prime(s2, random);
      mod = [object(Gmp.mpz)](p * q);
    } while (mod->size() != bits);
    Gmp.mpz phi = [object(Gmp.mpz)](Gmp.mpz([object(Gmp.mpz)](p-1))*
				    Gmp.mpz([object(Gmp.mpz)](q-1)));

    array(Gmp.mpz) gs; /* gcd(pub, phi), and pub^-1 mod phi */

    // For a while it was thought that small exponents were a security
    // problem, but turned out was a padding problem. The exponent
    // 0x10001 has however become common practice, although a smaller
    // value would be more efficient.
    Gmp.mpz pub = Gmp.mpz(e || 0x10001);

    // For security reason we need to ensure no common denominator
    // between n and phi. We could create a different exponent, but
    // some Crypto packages are hard coded for 0x10001, so instead
    // we'll just start over.
    if ((gs = pub->gcdext2(phi))[0] != 1)
      continue;

    if (gs[1] < 0)
      gs[1] += phi;
    
    set_public_key(mod, pub);
    set_private_key(gs[1], ({ p, q }));

  } while (!raw_verify(msg, raw_sign(msg)));
  return this;
}

#endif

//! Compatibility with Pike 7.8.
variant __deprecated__ this_program generate_key(int(128..) bits,
						 function(int:string(0..255)) rnd)
{
  function(int:string(0..255)) old_rnd = random;
  random = rnd;
  this_program res = generate_key(bits);
  random = old_rnd;
  return res;
}

//
// --- PKCS methods
//

#define Sequence Standards.ASN1.Types.Sequence

//! Calls @[Standards.PKCS.RSA.signatue_algorithm_id] with the
//! provided @[hash].
Sequence pkcs_algorithm_id(.Hash hash)
{
  return [object(Sequence)]Standards.PKCS.RSA->signature_algorithm_id(hash);
}

//! Calls @[Standards.PKCS.RSA.build_public_key] with this object as
//! argument.
Sequence pkcs_public_key()
{
  return [object(Sequence)]Standards.PKCS.RSA->build_public_key(this);
}

#undef Sequence

//! Signs the @[message] with a PKCS-1 signature using hash algorithm
//! @[h].
string(0..255) pkcs_sign(string(0..255) message, .Hash h)
{
  string(0..255) di = Standards.PKCS.Signature.build_digestinfo(message, h);
  return raw_sign(di)->digits(256);
}

//! Verify PKCS-1 signature @[sign] of message @[message] using hash
//! algorithm @[h].
int(0..1) pkcs_verify(string(0..255) message, .Hash h, string(0..255) sign)
{
  string(0..255) s = Standards.PKCS.Signature.build_digestinfo(message, h);
  return raw_verify(s, Gmp.mpz(sign, 256));
}

//
// --- Encryption/decryption
//

//! Pads the message @[s] with @[rsa_pad] type 2, signs it and returns
//! the signature as a byte string.
//! @param r
//!   Optional random function to be passed down to @[rsa_pad].
string(0..255) encrypt(string(0..255) s, function(int:string(0..255))|void r)
{
  return rsa_pad(s, 2, r)->powm(e, n)->digits(256);
}

//! Decrypt a message encrypted with @[encrypt].
string(0..255) decrypt(string(0..255) s)
{
  return rsa_unpad(Gmp.mpz(s, 256)->powm(d, n), 2);
}

//
// --- Block cipher compatibility.
//

protected int encrypt_mode; // For block cipher compatible functions

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
string(0..255) crypt(string(0..255) s)
{
  return (encrypt_mode ? encrypt(s) : decrypt(s));
}

//! Returns the crypto block size, or zero if not yet set.
int block_size()
{
  if(!size) return 0;
  // FIXME: This can be both zero and negative...
  return size - 3;
}

//! Returns the size of the key in terms of number of bits.
int(0..) key_size() { return [int(0..)](size*8); }


//
//  --- Below are methods for RSA applied in other standards.
//


//! Pads the @[message] to the current block size with method @[type]
//! and returns the result as an integer. This is equvivalent to
//! OS2IP(EME-PKCS1-V1_5-ENCODE(message)) in PKCS-1.
//! @param type
//!   @int
//!     @value 1
//!       The message is padded with @expr{0xff@} bytes.
//!     @value 2
//!       The message is padded with random data, using the @[random]
//!       function if provided. Otherwise the default random function
//!       set in the object will be used.
//!   @endint
Gmp.mpz rsa_pad(string(0..255) message, int(1..2) type,
		function(int:string(0..255))|void random)
{
  string(0..255) cookie = "";
  int len;

  len = size - 3 - sizeof(message);
  if (len < 8)
    error( "Block too large. (%d,%d)\n", sizeof(message), size-3 );

  switch(type)
  {
  case 1:
    cookie = [string(0..255)]sprintf("%@c", allocate(len, 0xff));
    break;
  case 2:
    if( !random ) random = this_program::random;
    do {
      cookie += random(len-sizeof(cookie)) - "\0";
    }  while( sizeof(cookie)<len );
    break;
  default:
    error( "Unknown type.\n" );
  }
  return Gmp.mpz(sprintf("%c", type) + cookie + "\0" + message, 256);
}

//! Reverse the effect of @[rsa_pad].
string(0..255) rsa_unpad(Gmp.mpz block, int type)
{
  string(0..255) s = block->digits(256);
  int i = search(s, "\0");

  if ((i < 9) || (sizeof(s) != (size - 1)) || (s[0] != type))
    return 0;
  return s[i+1..];
}

//! Pads the @[digest] with @[rsa_pad] type 1 and signs it.
Gmp.mpz raw_sign(string(0..255) digest)
{
  return rsa_pad(digest, 1, 0)->powm(d, n);
}

//! Verifies the @[digest] against the signature @[s], assuming pad
//! type 1.
//! @seealso
//!   @[rsa_pad], @[raw_sign]
int(0..1) raw_verify(string(0..255) digest, Gmp.mpz s)
{
  return s->powm(e, n) == rsa_pad(digest, 1, 0);
}

//
// --- Deprecated stuff
//

//! Returns the RSA modulo (n) as a binary string.
__deprecated__ string(0..255) cooked_get_n()
{
  return n->digits(256);
}

//! Returns the RSA public exponent (e) as a binary string.
__deprecated__ string(0..255) cooked_get_e()
{
  return e->digits(256);
}

//! Returns the RSA private exponent (d) as a binary string, if known.
__deprecated__ string(0..255) cooked_get_d()
{
  return d->digits(256);
}

//! Returns the first RSA prime (p) as a binary string, if known.
__deprecated__ string(0..255) cooked_get_p()
{
  return p->digits(256);
}

//! Returns the second RSA prime (q) as a binary string, if known.
__deprecated__ string(0..255) cooked_get_q()
{
  return q->digits(256);
}

//! Signs @[digest] as @[raw_sign] and returns the signature as a byte
//! string.
__deprecated__ string(0..255) cooked_sign(string(0..255) digest)
{
  return raw_sign(digest)->digits(256);
}

__deprecated__ int query_blocksize() {
  return block_size();
}

__deprecated__ int(0..) rsa_size() { return [int(0..)](size*8); }


// Broken implementation of RSA/MD5 SIG RFC 2537. The 0x00 01 FF* 00
// prefix is missing.

// (RSA/SHA-1 SIG is in RFC 3110)

#if constant(Crypto.MD5)

__deprecated__ string(0..255) md5_sign(string(0..255) message, mixed|void r)
{
  string(0..255) s = Crypto.MD5->hash(message);
  s = "0 0\14\6\10*\x86H\x86\xf7\15\2\5\5\0\4\20"+s;
  return raw_sign(s)->digits(256);r;
}

__deprecated__ int md5_verify(string(0..255) message, string(0..255) signature)
{
  string(0..255) s = Crypto.MD5->hash(message);
  s = "0 0\14\6\10*\x86H\x86\xf7\15\2\5\5\0\4\20"+s;
  return raw_verify(s, Gmp.mpz(signature, 256));
}

#endif

__deprecated__ string(0..255) sha_sign(string(0..255) message, mixed|void r)
{
  string(0..255) s = [string(0..255)]sprintf("%c%s%1H", 4, "sha1",
					     Crypto.SHA1->hash(message));
  return raw_sign(s)->digits(256);r;
}

__deprecated__ int sha_verify(string(0..255) message, string(0..255) signature)
{
  string(0..255) s = [string(0..255)]sprintf("%c%s%1H", 4, "sha1",
					     Crypto.SHA1->hash(message));
  return raw_verify(s, Gmp.mpz(signature, 256));
}

__deprecated__ Gmp.mpz sign(string(0..255) message, .Hash h)
{
  return raw_sign(Standards.PKCS.Signature.build_digestinfo(message, h));
}

__deprecated__ int(0..1) verify(string(0..255) message, .Hash h, Gmp.mpz sign)
{
  return raw_verify(Standards.PKCS.Signature.build_digestinfo(message, h), sign);
}

#else
constant this_program_does_not_exist=1;
#endif
