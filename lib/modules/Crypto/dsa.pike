/* dsa.pike
 *
 * The Digital Signature Algorithm (aka DSS, Digital Signature Standard).
 */

#define bignum object(Gmp.mpz)

bignum p; /* Modulo */
bignum q; /* Group order */
bignum g; /* Generator */

bignum y; /* Public key */
bignum x; /* Private key */

object set_public_key(bignum p_, bignum q_, bignum g_, bignum y_)
{
  p = p_; q = q_; g = g_; y = y_;
  return this_object();
}

object set_private_key(bignum secret)
{
  x = secret;
  return this_object();
}

bignum hash2number(string digest)
{
  return Gmp.mpz(digest, 256) % q;
}

bignum dsa_hash(string msg)
{
  return hash2number(Crypto.sha()->update(msg)->digest());
}
  
/* Generate a random number k, 0<k<q */
bignum random_exponent(function random)
{
  return Gmp.mpz(random( (q->size() + 10 / 8)), 256) % (q - 1) + 1;
}

array(bignum) raw_sign(bignum h, function random)
{
  bignum k = random_exponent(random);
  
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

string sign_rsaref(string msg, function random)
{
  [bignum r, bignum s] = raw_sign(dsa_hash(msg), random);

  return sprintf("%'\0'20s%'\0'20s", r->digits(256), s->digits(256));
}

int verify_rsaref(string msg, string s)
{
  if (strlen(s) != 40)
    return 0;

  return raw_verify(dsa_hash(msg),
		    Gmp.mpz(s[..19], 256),
		    Gmp.mpz(s[20..], 256));
}

string sign_ssl(string msg, function random)
{
  return Standards.ASN1.Types.asn1_sequence(
    Array.map(raw_sign(dsa_hash(msg), random),
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
