
//! The Digital Signature Algorithm (aka DSS, Digital Signature Standard).

#pike __REAL_VERSION__
#pragma strict_types

#if constant(Gmp.mpz)

static Gmp.mpz p; // Modulo
static Gmp.mpz q; // Group order
static Gmp.mpz g; // Generator

static Gmp.mpz y; // Public key
static Gmp.mpz x; // Private key

function(int:string) random = .Random.random_string;


// Accessors

Gmp.mpz get_p() { return p; } //! Returns the modulo.
Gmp.mpz get_q() { return p; } //! Returns the group order.
Gmp.mpz get_g() { return p; } //! Returns the generator.
Gmp.mpz get_y() { return p; } //! Returns the public key.
Gmp.mpz get_x() { return p; } //! Returns the private key.


//! Sets the public key in this DSA object.
this_program set_public_key(Gmp.mpz p_, Gmp.mpz q_, Gmp.mpz g_, Gmp.mpz y_)
{
  p = p_; q = q_; g = g_; y = y_;

#if 0
#define D(x) ((x) ? (x)->digits() : "NULL")
  werror("dsa->set_public_key\n"
	 "  p = %s,\n"
	 "  q = %s,\n"
	 "  g = %s,\n"
	 "  y = %s,\n",
	 D(p), D(q), D(g), D(y));
#endif
  
  return this;
}

//! Sets the private key in this DSA object.
this_program set_private_key(Gmp.mpz secret)
{
  x = secret;
  return this;
}

//! @fixme
//!   Document this function.
this_program use_random(function(int:string) r)
{
  random = r;
  return this;
}

//! @fixme
//!   Document this function.
Gmp.mpz dsa_hash(string msg)
{
  return [object(Gmp.mpz)](Gmp.mpz(.SHA1.hash(msg), 256) % q);
}
  
static Gmp.mpz random_number(Gmp.mpz n)
{
  return [object(Gmp.mpz)](Gmp.mpz(random( (q->size() + 10 / 8)), 256) % n);
}

static Gmp.mpz random_exponent()
{
  return [object(Gmp.mpz)](random_number([object(Gmp.mpz)](q - 1)) + 1);
}

//! @fixme
//!   Document this function.
array(Gmp.mpz) raw_sign(Gmp.mpz h)
{
  Gmp.mpz k = random_exponent();
  
  Gmp.mpz r = [object(Gmp.mpz)](g->powm(k, p) % q);
  Gmp.mpz s = [object(Gmp.mpz)]((k->invert(q) * (h + x*r)) % q);

  return ({ r, s });
}

//! @fixme
//!   Document this function.
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

//! @fixme
//!   Document this function.
string sign_rsaref(string msg)
{
  [Gmp.mpz r, Gmp.mpz s] = raw_sign(dsa_hash(msg));

  return sprintf("%'\0'20s%'\0'20s", r->digits(256), s->digits(256));
}

//! @fixme
//!   Document this function.
int(0..1) verify_rsaref(string msg, string s)
{
  if (sizeof(s) != 40)
    return 0;

  return raw_verify(dsa_hash(msg),
		    Gmp.mpz(s[..19], 256),
		    Gmp.mpz(s[20..], 256));
}

//! @fixme
//!   Document this function.
string sign_ssl(string msg)
{
  return Standards.ASN1.Types.asn1_sequence(
    Array.map(raw_sign(dsa_hash(msg)),
	      Standards.ASN1.Types.asn1_integer))->get_der();
}

//! @fixme
//!   Document this function.
int(0..1) verify_ssl(string msg, string s)
{
#define Object Standards.ASN1.Types.Object
  Object a = Standards.ASN1.Decode.simple_der_decode(s);

  if (!a
      || (a->type_name != "SEQUENCE")
      || (sizeof([array]a->elements) != 2)
      || (sizeof( ([array(object(Object))]a->elements)->type_name -
		  ({ "INTEGER" }))))
    return 0;

  return raw_verify(dsa_hash(msg),
		    [object(Gmp.mpz)]([array(object(Object))]a->elements)[0]->
		      value,
		    [object(Gmp.mpz)]([array(object(Object))]a->elements)[1]->
		      value);
}


#define SEED_LENGTH 20

static string nist_hash(Gmp.mpz x)
{
  string s = x->digits(256);
  return .SHA1.hash(s[sizeof(s) - SEED_LENGTH..]);
}

//! The (slow) NIST method of generating a DSA prime pair. Algorithm
//! 4.56 of Handbook of Applied Cryptography.
array(Gmp.mpz) nist_primes(int l)
{
  if ( (l < 0) || (l > 8) )
    error( "Unsupported key size.\n" );

  int L = 512 + 64 * l;

  int n = (L-1) / 160;
  int b = (L-1) % 160;

  for (;;)
  {
    /* Generate q */
    string seed = random(SEED_LENGTH);
    Gmp.mpz s = Gmp.mpz(seed, 256);

    string h = nist_hash(s) ^ nist_hash( [object(Gmp.mpz)](s + 1) );

    h = sprintf("%c%s%c", h[0] | 0x80, h[1..sizeof(h) - 2], h[-1] | 1);

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
      buffer = sprintf("%c%s", buffer[0] | 0x80, buffer[1..]);

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

static Gmp.mpz find_generator(Gmp.mpz p, Gmp.mpz q)
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

//! Generate key parameters using @[nist_primes].
this_program generate_parameters(int bits)
{
  if (bits % 64)
    error( "Unsupported key size.\n" );

  [p, q] = nist_primes(bits / 64 - 8);

  if (p % q != 1)
    error( "Internal error.\n" );

  if (q->size() != 160)
    error( "Internal error.\n" );

  g = find_generator(p, q);

  if ( (g == 1) || (g->powm(q, p) != 1))
    error( "Internal error.\n" );

  return this;
}

//! Generates a public/private key pair. Needs the public parameters
//! p, q and g set, either through @[set_public_key] or
//! @[generate_parameters].
this_program generate_key()
{
  /* x in { 2, 3, ... q - 1 } */
  if(!p || !q || !g) error("Public parameters not set..\n");
  x = [object(Gmp.mpz)](random_number( [object(Gmp.mpz)](q-2) ) + 2);
  y = g->powm(x, p);

  return this;
}

//! Compares the public key in this object with that in the provided
//! @[DSA] object.
int(0..1) public_key_equal (.DSA dsa)
{
  return (p == dsa->get_p()) && (q == dsa->get_q()) &&
    (g == dsa->get_g()) && (y == dsa->get_y());
}

#endif
