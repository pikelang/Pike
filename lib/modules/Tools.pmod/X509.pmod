/* X509.pmod
 *
 * Some random functions for creating RFC-2459 style X.509 certificates.
 *
 */

import Standards.ASN1.Types;
import Standards.PKCS;

object make_time(int t)
{
  mapping m = gmtime(t);

  if (m->year >= 150)
    throw( ({ "Tools.X509.make_time: "
	      "Times later than 2049 not supported yet\n",
	      backtrace() }) );

  return asn1_utc(sprintf("%02d%02d%02d%02d%02d%02dZ",
			  m->year % 100,
			  m->mon + 1,
			  m->mday,
			  m->hour,
			  m->min,
			  m->sec));
}

/* Returns a mapping similar to that returned by gmtime */
mapping parse_time(object asn1)
{
  if ((asn1->type_name != "UTCTime")
      || (strlen(asn1->value) != 13))
    return 0;

  sscanf(asn1->value, "%[0-9]s%c", string s, int c);
  if ( (strlen(s) != 12) && (c != 'Z') )
    return 0;

  /* NOTE: This relies on pike-0.7 not interpreting leading zeros as
   * an octal prefix. */
  mapping m = mkmapping( ({ "year", "mon", "mday", "hour", "min", "sec" }),
			 (array(int)) (s/2));

  if (m->year < 50)
    m->year += 50;
  if ( (m->mon <= 0 ) || (m->mon > 12) )
    return 0;
  m->mon--;
  
  if ( (m->mday <= 0) || (m->mday > Calendar.ISO.Year(m->year + 1900)
			  ->month(m->mon + 1)->number_of_days()))
    return 0;

  if ( (m->hour < 0) || (m->hour > 23))
    return 0;

  if ( (m->min < 0) || (m->min > 59))
    return 0;

  /* NOTE: Allows for lead seconds */
  if ( (m->sec < 0) || (m->min > 60))
    return 0;

  return m;
}

int time_compare(mapping t1, mapping t2)
{
  foreach( ({ "year", "mon", "mday", "hour", "min", "sec" }), string name)
    {
      if (t1->name < t2->name)
	return -1;
      if (t1->name > t2->name)
	return 1;
    }
  return 0;
}

	 
object extension_sequence = meta_explicit(3);
object version_integer = meta_explicit(0);

object rsa_md5_algorithm = asn1_sequence( ({ Identifiers.rsa_md5_id,
					     asn1_null() }) );

object rsa_sha1_algorithm = asn1_sequence( ({ Identifiers.rsa_sha1_id,
					      asn1_null() }) );


object make_tbs(object issuer, object algorithm,
		object subject, object keyinfo,
		object serial, int ttl,
		array extensions)
{
  int now = time();
  object validity = asn1_sequence( ({ make_time(now),
				      make_time(now + ttl) }) );

  return (extensions
	  ? asn1_sequence( ({ version_integer(asn1_integer(2)), /* Version 3 */
			      serial,
			      algorithm,
			      issuer,
			      validity,
			      subject,
			      keyinfo,
			      extension_sequence(extensions) }) )
	  : asn1_sequence( ({ serial,
			      algorithm,
			      issuer,
			      validity,
			      subject,
			      keyinfo }) ));
}

string rsa_sign_digest(object rsa, object digest_id, string digest)
{
  object digest_info = asn1_sequence( ({ asn1_sequence( ({ digest_id,
							   asn1_null() }) ),
					 asn1_octet_string(digest) }) );
  return rsa->raw_sign(digest_info->get_der())->digits(256);
}

int rsa_verify_digest(object rsa, object digest_id, string digest, string s)
{
  object digest_info = asn1_sequence( ({ asn1_sequence( ({ digest_id,
							   asn1_null() }) ),
					 asn1_octet_string(digest) }) );

  return rsa->raw_verify(digest_info->get_der(), Gmp.mpz(s, 256));
}

string make_selfsigned_rsa_certificate(object rsa, int ttl, array name,
				       array|void extensions)
{
  object serial = asn1_integer(1); /* Hard coded serial number */

  int now = time();
  object validity = asn1_sequence( ({ make_time(now),
				      make_time(now + ttl) }) );

  object signature_algorithm = asn1_sequence( ({ Identifiers.rsa_sha1_id,
					         asn1_null() }) );

  object keyinfo = asn1_sequence(
    ({ asn1_sequence( ({ Identifiers.rsa_id,
			 asn1_null() }) ),
       asn1_bit_string(RSA.rsa_public_key(rsa)) }) );

  object dn = Certificate.build_distinguished_name(@name);
  
  object tbs = make_tbs(dn, rsa_sha1_algorithm,
			dn, keyinfo,
			serial, ttl, extensions);
  
  return asn1_sequence(
    ({ tbs,
       rsa_sha1_algorithm,
       asn1_bit_string(rsa_sign_digest(rsa, Identifiers.sha1_id,
				       Crypto.sha()->update(tbs->get_der())
				       ->digest())) }) )->get_der();
}

class rsa_verifier
{
  object rsa;

  constant type = "rsa";

  object init(string key)
    {
      rsa = RSA.parse_public_key(key);
      return rsa && this_object();
    }
  
  int verify(object algorithm, string msg, string signature)
    {
      {
	if (algorithm->get_der() == rsa_md5_algorithm->get_der())
	  return rsa_verify_digest(rsa, Identifiers.md5_id,
				   Crypto.md5()->update(msg)->digest(),
				   signature);
	else if (algorithm->get_der() == rsa_sha1_algorithm->get_der())
	  return rsa_verify_digest(rsa, Identifiers.sha1_id,
				   Crypto.sha()->update(msg)->digest(),
				   signature);
	else
	  return 0;
      }
    }
}

#if 0
/* FIXME: This is a little more difficult, as the dsa-parameters are
 * sometimes taken from the CA, and not present in the keyinfo. */
class dsa_verifyer
{
  object dsa;

  constant type = "dsa";

  object init(string key)
    {
    }
}
#endif

object make_verifier(object keyinfo)
{
  if ( (keyinfo->type_name != "SEQUENCE")
       || (sizeof(keyinfo->elements) != 2)
       || (keyinfo->elements[0]->type_name != "SEQUENCE")
       || !sizeof(keyinfo->elements[0]->elements)
       || (keyinfo->elements[1]->type_name != "BIT STRING")
       || keyinfo->elements[1]->unused)
    return 0;
  
  if (keyinfo->elements[0]->elements[0]->get_der()
      == Identifiers.rsa_id->get_der())
  {
    if ( (sizeof(keyinfo->elements[0]->elements) != 2)
	 || (keyinfo->elements[0]->elements[1]->get_der()
	     != asn1_null()->get_der()))
      return 0;
    
    return rsa_verifier()->init(keyinfo->elements[1]->value);
  }
  else return 0;
}
