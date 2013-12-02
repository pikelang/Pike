
//! The Digital Signature Algorithm DSA is part of the NIST Digital
//! Signature Standard DSS, FIPS-186 (1993).


#pike __REAL_VERSION__
#pragma strict_types

#if constant(Crypto.Hash)

//
// --- Variables and accessors
//

protected Gmp.mpz p; // Modulo
protected Gmp.mpz q; // Group order
protected Gmp.mpz g; // Generator

protected Gmp.mpz y; // Public key
protected Gmp.mpz x; // Private key

protected function(int:string) random = .Random.random_string;

Gmp.mpz get_p() { return p; } //! Returns the DSA modulo (p).
Gmp.mpz get_q() { return q; } //! Returns the DSA group order (q).
Gmp.mpz get_g() { return g; } //! Returns the DSA generator (g).
Gmp.mpz get_y() { return y; } //! Returns the DSA public key (y).
Gmp.mpz get_x() { return x; } //! Returns the DSA private key (x).

//! Sets the random function, used to generate keys and parameters, to
//! the function @[r]. Default is @[Crypto.Random.random_string].
this_program set_random(function(int:string) r)
{
  random = r;
  return this;
}

//! Returns the string @expr{"DSA"@}.
string name() { return "DSA"; }

//
// --- Key methods
//

//! Sets the public key in this DSA object.
//! @param modulo
//!   This is the p parameter.
//! @param order
//!   This is the group order q parameter.
//! @param generator
//!   This is the g parameter.
//! @param kye
//!   This is the public key y parameter.
this_program set_public_key(Gmp.mpz modulo, Gmp.mpz order,
                            Gmp.mpz generator, Gmp.mpz key)
{
  p = modulo;
  q = order;
  g = generator;
  y = key;
  return this;
}

//! Compares the public key in this object with that in the provided
//! DSA object.
int(0..1) public_key_equal(this_program dsa)
{
  return (p == dsa->get_p()) && (q == dsa->get_q()) &&
    (g == dsa->get_g()) && (y == dsa->get_y());
}

//! Sets the private key, the x parameter, in this DSA object.
this_program set_private_key(Gmp.mpz secret)
{
  x = secret;
  return this;
}

//
// --- Key generation
//

#if !constant(Nettle.dsa_generate_keypair)

#define SEED_LENGTH 20
protected string nist_hash(Gmp.mpz x)
{
  string s = x->digits(256);
  return .SHA1.hash(s[sizeof(s) - SEED_LENGTH..]);
}

// The (slow) NIST method of generating a DSA prime pair. Algorithm
// 4.56 of Handbook of Applied Cryptography.
protected array(Gmp.mpz) nist_primes(int l)
{
  if ( (l < 0) || (l > 8) )
    error( "Unsupported key size.\n" );

  int L = 512 + 64 * l;

  int n = (L-1) / 160;
  //  int b = (L-1) % 160;

  for (;;)
  {
    /* Generate q */
    string seed = random(SEED_LENGTH);
    Gmp.mpz s = Gmp.mpz(seed, 256);

    string h = nist_hash(s) ^ nist_hash( [object(Gmp.mpz)](s + 1) );

    h = sprintf("%c%s%c", h[0] | 0x80, h[1..<1], h[-1] | 1);

    Gmp.mpz q = Gmp.mpz(h, 256);

    if (q->small_factor() || !q->probably_prime_p())
      continue;

    /* q is a prime, with overwelming probability. */

    int i, j;

    for (i = 0, j = 2; i < 4096; i++, j += n+1)
    {
      string buffer = "";
      int k;

      for (k = 0; k<= n; k++)
	buffer = nist_hash( [object(Gmp.mpz)](s + j + k) ) + buffer;

      buffer = buffer[sizeof(buffer) - L/8 ..];
      buffer[0] = buffer[0] | 0x80;

      Gmp.mpz p = Gmp.mpz(buffer, 256);

      p -= p % (2 * q) - 1;

      if (!p->small_factor() && p->probably_prime_p())
      {
	/* Done */
	return ({ p, q });
      }
    }
  }
}

protected Gmp.mpz find_generator(Gmp.mpz p, Gmp.mpz q)
{
  Gmp.mpz e = [object(Gmp.mpz)]((p - 1) / q);
  Gmp.mpz g;

  do
  {
    /* A random number in { 2, 3, ... p - 2 } */
    g = ([object(Gmp.mpz)](random_number( [object(Gmp.mpz)](p-3) ) + 2))
      /* Exponentiate to get an element of order 1 or q */
      ->powm(e, p);
  }
  while (g == 1);

  return g;
}

// Generate key parameters (p, q and g) using the NIST DSA prime pair
// generation algorithm. @[bits] must be multiple of 64.
protected void generate_parameters(int bits)
{
  if (!bits || bits % 64)
    error( "Unsupported key size.\n" );

  [p, q] = nist_primes(bits / 64 - 8);

  if (p % q != 1)
    error( "Internal error.\n" );

  if (q->size() != 160)
    error( "Internal error.\n" );

  g = find_generator(p, q);

  if ( (g == 1) || (g->powm(q, p) != 1))
    error( "Internal error.\n" );
}

variant this_program generate_key(int p_bits, int q_bits)
{
  if(q_bits!=160)
    error("Only 1024/160 supported with Nettle version < 2.0\n");
  generate_parameters(1024);
  return generate_key();
}

#else // !constant(Nettle.dsa_generate_keypair)

//! Generates DSA parameters (p, q, g) and key (x, y). Depending on
//! Nettle version @[q_bits] can be 160, 224 and 256 bits. 160 works
//! for all versions.
variant this_program generate_key(int p_bits, int q_bits)
{
  [ p, q, g, y, x ] = Nettle.dsa_generate_keypair(p_bits, q_bits, random);
  return this;
}

#endif

//! Generates a public/private key pair. Needs the public parameters
//! p, q and g set, either through @[set_public_key] or
//! @[generate_key(int,int)].
variant this_program generate_key()
{
  /* x in { 2, 3, ... q - 1 } */
  if(!p || !q || !g) error("Public parameters not set..\n");
  x = [object(Gmp.mpz)](random_number( [object(Gmp.mpz)](q-2) ) + 2);
  y = g->powm(x, p);

  return this;
}


//
// --- PKCS methods
//

#define Sequence Standards.ASN1.Types.Sequence

//! Calls @[Standards.PKCS.DSA.signatue_algorithm_id] with the
//! provided @[hash].
Sequence pkcs_algorithm_id(.Hash hash)
{
  return [object(Sequence)]Standards.PKCS.DSA->signature_algorithm_id(hash);
}

//! Calls @[Standards.PKCS.DSA.build_public_key] with this object as
//! argument.
Sequence pkcs_public_key()
{
  return [object(Sequence)]Standards.PKCS.DSA->build_public_key(this);
}

#undef Sequence

//! Signs the @[message] with a PKCS-1 signature using hash algorithm
//! @[h].
string pkcs_sign(string message, .Hash h)
{
  array sign = map(raw_sign(hash(message, h)), Standards.ASN1.Types.Integer);
  return Standards.ASN1.Types.Sequence(sign)->get_der();
}

#define Object Standards.ASN1.Types.Object

//! Verify PKCS-1 signature @[sign] of message @[message] using hash
//! algorithm @[h].
int(0..1) pkcs_verify(string message, .Hash h, string sign)
{
  Object a = Standards.ASN1.Decode.simple_der_decode(sign);

  if (!a
      || (a->type_name != "SEQUENCE")
      || (sizeof([array]a->elements) != 2)
      || (sizeof( ([array(object(Object))]a->elements)->type_name -
		  ({ "INTEGER" }))))
    return 0;

  return raw_verify(hash(message, h),
		    [object(Gmp.mpz)]([array(object(Object))]a->elements)[0]->
		      value,
		    [object(Gmp.mpz)]([array(object(Object))]a->elements)[1]->
		      value);
}

#undef Object

//
//  --- Below are methods for RSA applied in other standards.
//


//! Makes a DSA hash of the messge @[msg].
Gmp.mpz hash(string msg, .Hash h)
{
  string digest = h->hash(msg)[..q->size()/8-1];
  return [object(Gmp.mpz)](Gmp.mpz(digest, 256) % q);
}
  
protected Gmp.mpz random_number(Gmp.mpz n)
{
  return [object(Gmp.mpz)](Gmp.mpz(random( (q->size() + 10 / 8)), 256) % n);
}

protected Gmp.mpz random_exponent()
{
  return [object(Gmp.mpz)](random_number([object(Gmp.mpz)](q - 1)) + 1);
}

//! Sign the message @[h]. Returns the signature as two @[Gmp.mpz]
//! objects.
array(Gmp.mpz) raw_sign(Gmp.mpz h, void|Gmp.mpz k)
{
  if(!k) k = random_exponent();
  
  Gmp.mpz r = [object(Gmp.mpz)](g->powm(k, p) % q);
  Gmp.mpz s = [object(Gmp.mpz)]((k->invert(q) * (h + x*r)) % q);

  return ({ r, s });
}

//! Verify the signature @[r],@[s] against the message @[h].
int(0..1) raw_verify(Gmp.mpz h, Gmp.mpz r, Gmp.mpz s)
{
  Gmp.mpz w;
  if (catch
      {
	w = s->invert(q);
      })
    /* Non-invertible */
    return 0;

  /* The inner %q's are redundant, as g^q == y^q == 1 (mod p) */
  return r == (g->powm( [object(Gmp.mpz)](w * h % q), p) *
	       y->powm( [object(Gmp.mpz)](w * r % q), p) % p) % q;
}


//
// --- Deprecated stuff
//

//! Make a RSA ref signature of message @[msg].
__deprecated__ string sign_rsaref(string msg)
{
  [Gmp.mpz r, Gmp.mpz s] = raw_sign(hash(msg, .SHA1));

  return sprintf("%'\0'20s%'\0'20s", r->digits(256), s->digits(256));
}

//! Verify a RSA ref signature @[s] of message @[msg].
__deprecated__ int(0..1) verify_rsaref(string msg, string s)
{
  if (sizeof(s) != 40)
    return 0;

  return raw_verify(hash(msg, .SHA1),
		    Gmp.mpz(s[..19], 256),
		    Gmp.mpz(s[20..], 256));
}

//! Make an SSL signature of message @[msg].
__deprecated__ string sign_ssl(string msg)
{
  return pkcs_sign(msg, .SHA1);
}

//! Verify an SSL signature @[s] of message @[msg].
__deprecated__ int(0..1) verify_ssl(string msg, string s)
{
  return pkcs_verify(msg, .SHA1, s);
}

#else
constant this_program_does_not_exist=1;
#endif
