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

object extension_sequence = meta_explicit(3);

object make_tbs(object issuer, object algorithm,
		object subject, object keyinfo,
		object serial, int ttl,
		array extensions)
{
  int now = time();
  object validity = asn1_sequence( ({ make_time(now),
				      make_time(now + ttl) }) );

  return (extensions
	  ? asn1_sequence( ({ 2, /* Version 3 */
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

#if 0
string make_selfsigned_dss_certificate(object dss, int ttl, object name,
				       array|void extensions)
{
  object serial = asn1_integer(1); /* Hard coded serial number */
  int now = time();
  object validity = asn1_sequence( ({ make_time(now),
				      make_time(now + ttl) }) );

  object signature_algorithm = Identifiers.dsa_sha_id;
  
  object keyinfo = asn1_sequence(
    ({ /* Use an identifier with parameters */
       DSA.dsa_algorithm_identifier(dsa),
       asn1_bit_string(DSA.dsa_public_key(dsa)) }) );

  object tbs = make_tbs(name, signature_algorithm,
			name, keyinfo,
			serial, ttl, extensions);
  
  return asn1_sequence(
    ({ tbs,
       asn1_sequence( ({ signature_algorithm }) );
       asn1_bit_string(dsa
		       ->sign_ssl(tbs->get_der())) }))->get_der();
}
#endif

string rsa_sign_digest(object rsa, object digest_id, string digest)
{
  object digest_info = asn1_sequence( ({ asn1_sequence( ({ digest_id,
							   asn1_null() }) ),
					 asn1_octet_string(digest) }) );
  return rsa->raw_sign(digest_info->get_der())->digits(256);
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
  
  object tbs = make_tbs(dn, signature_algorithm,
			dn, keyinfo,
			serial, ttl, extensions);
  
  return asn1_sequence(
    ({ tbs,
       signature_algorithm,
       asn1_bit_string(rsa_sign_digest(rsa, Identifiers.sha1_id,
				       Crypto.sha()->update(tbs->get_der())
				       ->digest())) }) )->get_der();
}

