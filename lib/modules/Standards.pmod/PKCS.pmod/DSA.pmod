/* DSA.pmod
 *
 * DSA operations as defined in RFC-2459.
 *
 */

/* NOTE: Unlike the functions in RSA.pmod, this function returns
 * an object rather than a string. */

#if constant(Gmp.mpz)

constant asn1_sequence = Standards.ASN1.Types.asn1_sequence;
constant asn1_integer = Standards.ASN1.Types.asn1_integer;

object algorithm_identifier(object|void dsa)
{
  return
    dsa ? asn1_sequence( ({ .Identifiers.dsa_id,
			    asn1_sequence( ({ asn1_integer(dsa->p),
					      asn1_integer(dsa->q),
					      asn1_integer(dsa->g) }) ) }) )
    : asn1_sequence( ({ .Identifiers.dsa_id }) );
}

string public_key(object dsa)
{
  return asn1_integer(dsa->y)->get_der();
}

/* I don't know if this format interoperates with anything else */
string private_key(object dsa)
{
  return asn1_sequence(Array.map(
    ({ dsa->p, dsa->q, dsa->g, dsa->y, dsa->x }),
    asn1_integer))->get_der();
}

object parse_private_key(string key)
{
    object a = Standards.ASN1.Decode.simple_der_decode(key);

    if (!a
	|| (a->type_name != "SEQUENCE")
	|| (sizeof(a->elements) != 5)
	|| (sizeof(a->elements->type_name - ({ "INTEGER" }))) )
      return 0;

    object dsa = Crypto.dsa();
    dsa->set_public_key(@ a->elements[..3]->value);
    dsa->set_private_key(a->elements[4]->value);

    return dsa;
}

#endif
