/* rsa.pmod
 *
 * rsa operations and types as described in PKCS-1 */

#if 0
#define WERROR werror
#else
#define WERROR(x)
#endif

#if __VERSION__ >= 0.6
import ".";
#endif /* __VERSION__ >= 0.6 */

import Standards.ASN1.Types;

/* Create a DER-coded RSAPublicKey structure */
string rsa_public_key(object rsa)
{
  return asn1_sequence(Array.map(
    ({ rsa->n, rsa->e }),
    asn1_integer))->get_der();
}

/* Create a DER-coded RSAPrivateKey structure */
string rsa_private_key(object rsa)
{
  return asn1_sequence(Array.map(
    ({ 0, rsa->n, rsa->e, rsa->d,
       rsa->p, rsa->q,
       rsa->d % (rsa->p - 1), rsa->d % (rsa->q -1),
       rsa->q->invert(rsa->p) % rsa->p
    }),
    asn1_integer))->get_der();
}

/* Decode a coded RSAPublicKey structure */
object parse_public_key(string key)
{
  // WERROR(sprintf("rsa->parse_public_key: '%s'\n", key));
  object a = Standards.ASN1.Decode.simple_der_decode(key);

  // WERROR(sprintf("rsa->parse_public_key: asn1 = %O\n", a));
  if (!a
      || (a->type_name != "SEQUENCE")
      || (sizeof(a->elements) != 2)
      || (sizeof(a->elements->type_name - ({ "INTEGER" }))) )
  {
    //  WERROR("Not a Valid Key!\n");
    return 0;
  }
  object rsa = Crypto.rsa();
  rsa->set_public_key(a->elements[0]->value, a->elements[1]->value);
  return rsa;
}

/* Decode a coded RSAPrivateKey structure */
object parse_private_key(string key)
{
  WERROR(sprintf("rsa->parse_private_key: '%s'\n", key));
  object a = Standards.ASN1.Decode.simple_der_decode(key);
  
  WERROR(sprintf("rsa->parse_private_key: asn1 = %O\n", a));
  if (!a
      || (a->type_name != "SEQUENCE")
      || (sizeof(a->elements) != 9)
      || (sizeof(a->elements->type_name - ({ "INTEGER" })))
      || a->elements[0]->value)
    return 0;
  
  object rsa = Crypto.rsa();
  rsa->set_public_key(a->elements[1]->value, a->elements[2]->value);
  rsa->set_private_key(a->elements[3]->value, a->elements[4..]->value);
  return rsa;
}

object build_rsa_public_key(object rsa)
{
  return asn1_sequence( ({
    asn1_sequence(
      ({ Identifiers.rsa_id, asn1_null() }) ),
    asn1_bit_string(asn1_sequence(
      ({ asn1_integer(rsa->n), asn1_integer(rsa->e) }) )->get_der()) }) );
}
      
int check_rsa_public_key (string public_key, object rsa)
{
  object a = Standards.ASN1.Decode.simple_der_decode (public_key);
  if (a->type_name != "SEQUENCE" ||
      sizeof (a->elements) != 2 ||
      sizeof (a->elements->type_name - ({"INTEGER"})))
    return 0;
  return a->elements[0]->value == rsa->n && a->elements[1]->value == rsa->e;
}
