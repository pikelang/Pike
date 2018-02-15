
#pike 7.5

//! @deprecated Crypto.DSA

#if constant(Gmp.mpz)

#define bignum object(Gmp.mpz)

// FIXME: Privatize these?
bignum p; /* Modulo */
bignum q; /* Group order */
bignum g; /* Generator */

bignum y; /* Public key */
bignum x; /* Private key */

function random;

Gmp.mpz get_p() { return p; }
Gmp.mpz get_q() { return q; }
Gmp.mpz get_g() { return g; }
Gmp.mpz get_y() { return y; }
Gmp.mpz get_x() { return x; }

object set_public_key(bignum p_, bignum q_, bignum g_, bignum y_)
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
  
  return this_object();
}

object set_private_key(bignum secret)
{
  x = secret;
  return this_object();
}

object set_random(function r)
{
  random = r;
  return this;
}

object use_random(function r)
{
  random = r;
  return this;
}

bignum hash2number(string digest)
{
  return Gmp.mpz(digest, 256) % q;
}

bignum dsa_hash(string msg)
{
  return hash2number(Crypto.SHA1->hash(msg));
}
  
// Generate a random number k, 0<=k<n
bignum random_number(bignum n)
{
  return Gmp.mpz(random( (q->size() + 10 / 8)), 256) % n;
}

bignum random_exponent()
{
  return random_number(q - 1) + 1;
}

array(bignum) raw_sign(bignum h)
{
  bignum k = random_exponent();
  
  bignum r = g->powm(k, p) % q;
  bignum s = (k->invert(q) * (h + x*r)) % q;

  return ({ r, s });
}

int raw_verify(bignum h, bignum r, bignum s)
{
  bignum w;
  if (catch
      {
	w = s->invert(q);
      })
    /* Non-invertible */
    return 0;

  /* The inner %q's are redundant, as g^q == y^q == 1 (mod p) */
  return r == (g->powm(w * h % q, p) * y->powm(w * r % q, p) % p) % q;
}

string sign_rsaref(string msg)
{
  [bignum r, bignum s] = raw_sign(dsa_hash(msg));

  return sprintf("%'\0'20s%'\0'20s", r->digits(256), s->digits(256));
}

int verify_rsaref(string msg, string s)
{
  if (sizeof(s) != 40)
    return 0;

  return raw_verify(dsa_hash(msg),
		    Gmp.mpz(s[..19], 256),
		    Gmp.mpz(s[20..], 256));
}

string sign_ssl(string msg)
{
  return Standards.ASN1.Types.asn1_sequence(
    Array.map(raw_sign(dsa_hash(msg)),
	      Standards.ASN1.Types.asn1_integer))->get_der();
}

int verify_ssl(string msg, string s)
{
  object a = Standards.ASN1.Decode.simple_der_decode(s);

  if (!a
      || (a->type_name != "SEQUENCE")
      || (sizeof(a->elements) != 2)
      || (sizeof(a->elements->type_name - ({ "INTEGER" }))))
    return 0;

  return raw_verify(dsa_hash(msg),
		    a->elements[0]->value,
		    a->elements[1]->value);
}

object set_public_test_key()
{
  return set_public_key(Gmp.mpz("cc61a8f5a4f94e31f5412d462791e7b493e8360a2ad6e5288e67a106927feb0b3338f2b9e3d19d0056127f6aa2062d48ae0f41185633a3fc1b22ee34a2161e5a1885d99be7ba5cfa09a0abc4becf8598ea4ec2c81316d9e2c6d28385a53f2e03",
				16),
			Gmp.mpz("f50473f33754ac2173968f96b50c24eb7d0a472d",
				16),
			Gmp.mpz("1209dae33ba50a8f9f6cb00d1b1274bf5cc94acdf3e8a78df7a13aca0640465fdf1bc3b66ae068ccd0845abdb73e622f90b633372c7fa439a65732e8cf88077c686cc679b1c463a979c02696f6d99af05eea07d974a5fc3da6fa34ff48b030b5",
				16),
			Gmp.mpz("7008517ecec3974b0a8813e5a39252b6b0051442f460bef29b4eb5c7f2972ac5f805c5383b6edcaa72596d50995b1e40f76b9a0b89ab4fb08f0458c38f2485740de6959e260e12e3052e1e28275ab84fdf3c7e9d347cb772792d4d38abcd3cfc",
				16));
}

object set_private_test_key()
{
  return set_private_key(Gmp.mpz("403a09fa0820287c84f2e8459a1fccf4c48c32e1",
				 16));
}

#define SEED_LENGTH 20

// The (slow) NIST method of generating DSA primes. Algorithm 4.56 of
// Handbook of Applied Cryptography.
string nist_hash(bignum x)
{
  string s = x->digits(256);

  return Crypto.SHA1->hash(s[sizeof(s) - SEED_LENGTH..]);
}

array(bignum) nist_primes(int l)
{
  if ( (l < 0) || (l > 8) )
    error( "Crypto.dsa->nist_primes: Unsupported key size.\n" );

  int L = 512 + 64 * l;

  int n = (L-1) / 160;
  int b = (L-1) % 160;

  for (;;)
  {
    /* Generate q */
    string seed = random(SEED_LENGTH);
    bignum s = Gmp.mpz(seed, 256);

    string h = nist_hash(s) ^ nist_hash(s + 1);

    h = sprintf("%c%s%c", h[0] | 0x80, h[1..sizeof(h) - 2], h[-1] | 1);

    bignum q = Gmp.mpz(h, 256);

    if (q->small_factor() || !q->probably_prime_p())
      continue;

    /* q is a prime, with overwelming probability. */

    int i, j;

    for (i = 0, j = 2; i < 4096; i++, j += n+1)
    {
      string buffer = "";
      int k;

      for (k = 0; k<= n; k++)
	buffer = nist_hash(s + j + k) + buffer;

      buffer = buffer[sizeof(buffer) - L/8 ..];
      buffer = sprintf("%c%s", buffer[0] | 0x80, buffer[1..]);

      bignum p = Gmp.mpz(buffer, 256);

      p -= p % (2 * q) - 1;

      if (!p->small_factor() && p->probably_prime_p())
      {
	/* Done */
	return ({ p, q });
      }
    }
  }
}

bignum find_generator(bignum p, bignum q)
{
  bignum e = (p - 1) / q;
  bignum g;

  do
  {
    /* A random number in { 2, 3, ... p - 2 } */
    g = (random_number(p-3) + 2)
      /* Exponentiate to get an element of order 1 or q */
      ->powm(e, p);
  }
  while (g == 1);

  return g;
}

object generate_parameters(int bits)
{
  if (bits % 64)
    error( "Crypto.dsa->generate_key: Unsupported key size.\n" );

  [p, q] = nist_primes(bits / 64 - 8);

  if (p % q != 1)
    error( "Crypto.dsa->generate_key: Internal error.\n" );

  if (q->size() != 160)
    error( "Crypto.dsa->generate_key: Internal error.\n" );

  g = find_generator(p, q);

  if ( (g == 1) || (g->powm(q, p) != 1))
    error( "Crypto.dsa->generate_key: Internal error.\n" );

  return this_object();
}

object generate_key()
{
  /* x in { 2, 3, ... q - 1 } */
  x = random_number(q - 2) + 2;
  y = g->powm(x, p);

  return this_object();
}

int public_key_equal (object dsa)
{
  return p == dsa->p && q == dsa->q && g == dsa->g && y == dsa->y;
}

#endif
