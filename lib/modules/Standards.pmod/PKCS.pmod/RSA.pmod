/* rsa.pmod
 *
 * rsa operations and types as described in PKCS-1 */

#if 0
#define WERROR werror
#else
#define WERROR(x)
#endif

import asn1.encode;

/* Create a DER-coded RSAPrivateKey structure */
string rsa_private_key(object rsa)
{
  return asn1_sequence(@ Array.map(
    ({ 0, rsa->n, rsa->e, rsa->d, rsa->p, rsa->q,
       rsa->d % (rsa->p - 1), rsa->d, (rsa->q -1),
       rsa->q->invert(rsa->p) % rsa->p
    }),
    asn1_integer))->der();
}

/* Decode a coded RSAPrivateKey structure */
object parse_private_key(string key)
{
  WERROR(sprintf("rsa->parse_private_key: '%s'\n", key));
  array a = asn1.decode(key)->get_asn1();

  WERROR(sprintf("rsa->parse_private_key: asn1 = %O\n", a));
  if (!a
      || (a[0] != "SEQUENCE")
      || (sizeof(a[1]) != 10)
      || (sizeof(column(a[1], 0) - ({ "INTEGER" })))
      || a[1][0][1])
    return 0;
  
  object rsa = Crypto.rsa();
  rsa->set_public_key(a[1][1][1], a[1][2][1]);
  rsa->set_private_key(a[1][3][1], column(a[1][4..], 1));
  return rsa;
}

object build_rsa_public_key(object rsa)
{
  return asn1_sequence(
    asn1_sequence(
      identifiers.rsa_id, asn1_null()),
    asn1_bitstring(asn1_sequence(
      asn1_integer(rsa->n), asn1_integer(rsa->e))->der()));
}

  
		       
    
