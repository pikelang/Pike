#pike __REAL_VERSION__

/* 
 * $Id: X509.pmod,v 1.17 2004/02/25 15:02:33 nilsson Exp $
 *
 * Some random functions for creating RFC-2459 style X.509 certificates.
 *
 */

#if constant(Standards.ASN1.Types.asn1_sequence)

constant dont_dump_module = 1;

import Standards.ASN1.Types;
import Standards.PKCS;

// Note: Redump this module if you change X509_DEBUG
#ifdef X509_DEBUG
#define X509_WERR werror
#else
#define X509_WERR
#endif

object make_time(int t)
{
  Calendar.Second second = Calendar.Second(t)->set_timezone("UTC");

  if (second->year_no() >= 2050)
    error( "Tools.X509.make_time: "
	   "Times later than 2049 not supported yet\n" );

  return asn1_utc(sprintf("%02d%02d%02d%02d%02d%02dZ",
			  second->year_no() % 100,
			  second->month_no(),
			  second->month_day(),
			  second->hour_no(),
			  second->minute_no(),
			  second->second_no()));
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
    m->year += 100;
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

  /* NOTE: Allows for leap seconds */
  if ( (m->sec < 0) || (m->sec > 60))
    return 0;

  return m;
}

int time_compare(mapping t1, mapping t2)
{
  foreach( ({ "year", "mon", "mday", "hour", "min", "sec" }), string name)
    if (t1[name] < t2[name])
      return -1;
    else if (t1[name] > t2[name])
      return 1;
  return 0;
}

	 
object extension_sequence = meta_explicit(2, 3);
object version_integer = meta_explicit(2, 0);

object rsa_md2_algorithm = asn1_sequence( ({ Identifiers.rsa_md2_id,
					     asn1_null() }) );

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

string make_selfsigned_dsa_certificate(object dsa, int ttl, array name,
				       array|void extensions)
{
  object serial = asn1_integer(1); /* Hard coded serial number */
  int now = time();
  object validity = asn1_sequence( ({ make_time(now),
				      make_time(now + ttl) }) );

  object signature_algorithm = asn1_sequence( ({ Identifiers.dsa_sha_id }) );
  
  object keyinfo = asn1_sequence(
    ({ /* Use an identifier with parameters */
       DSA.algorithm_identifier(dsa),
       asn1_bit_string(DSA.public_key(dsa)) }) );

  object dn = Certificate.build_distinguished_name(@name);
  
  object tbs = make_tbs(dn, signature_algorithm,
			dn, keyinfo,
			serial, ttl, extensions);
  
  return asn1_sequence(
    ({ tbs,
       signature_algorithm,
       asn1_bit_string(dsa
		       ->sign_ssl(tbs->get_der())) }))->get_der();
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
       asn1_bit_string(RSA.public_key(rsa)) }) );

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
	else if (algorithm->get_der() == rsa_md2_algorithm->get_der())
	  return rsa_verify_digest(rsa, Identifiers.md2_id,
				   Crypto.md2()->update(msg)->digest(),
				   signature);
	else
	  return 0;
      }
    }
}

#if 0
/* FIXME: This is a little more difficult, as the dsa-parameters are
 * sometimes taken from the CA, and not present in the keyinfo. */
class dsa_verifier
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
  else if (keyinfo->elements[0]->elements[0]->get_der()
	   == Identifiers.dsa_sha_id->get_der())
  {
    /* FIXME: Not implemented */
    return 0;
  }
}

class TBSCertificate
{
  string der;
  
  int version;
  object serial;
  object algorithm;  /* Algorithm Identifier */
  object issuer;
  mapping not_after;
  mapping not_before;

  object subject;
  object public_key;

  /* Optional */
  object issuer_id;
  object subject_id;
  object extensions;

  object init(object asn1)
    {
      der = asn1->get_der();
      if (asn1->type_name != "SEQUENCE")
	return 0;

      array a = asn1->elements;
      X509_WERR("TBSCertificate: sizeof(a) = %d\n", sizeof(a));
      
      if (sizeof(a) < 6)
	return 0;

      if (sizeof(a) > 6)
      {
	/* The optional version field must be present */
	if (!a[0]->constructed
	    || (a[0]->get_combined_tag() != make_combined_tag(2, 0))
	    || (sizeof(a[0]->elements) != 1)
	    || (a[0]->elements[0]->type_name != "INTEGER"))
	  return 0;

	version = (int) a[0]->elements[0]->value + 1;
	if ( (version < 2) || (version > 3))
	  return 0;
	a = a[1..];
      } else
	version = 1;

      X509_WERR("TBSCertificate: version = %d\n", version);
      if (a[0]->type_name != "INTEGER")
	return 0;
      serial = a[0]->value;

      X509_WERR("TBSCertificate: serial = %s\n", (string) serial);
      
      if ((a[1]->type_name != "SEQUENCE")
	  || !sizeof(a[1]->elements )
	  || (a[1]->elements[0]->type_name != "OBJECT IDENTIFIER"))
	return 0;

      algorithm = a[1];

      X509_WERR("TBSCertificate: algorithm = %s\n", algorithm->debug_string());

      if (a[2]->type_name != "SEQUENCE")
	return 0;
      issuer = a[2];

      X509_WERR("TBSCertificate: issuer = %s\n", issuer->debug_string());

      if ((a[3]->type_name != "SEQUENCE")
	  || (sizeof(a[3]->elements) != 2))
	return 0;

      array validity = a[3]->elements;

      not_before = parse_time(validity[0]);
      if (!not_before)
	return 0;
      
      X509_WERR("TBSCertificate: not_before = %O\n", not_before);

      not_after = parse_time(validity[1]);
      if (!not_after)
	return 0;

      X509_WERR("TBSCertificate: not_after = %O\n", not_after);

      if (a[4]->type_name != "SEQUENCE")
	return 0;
      subject = a[4];

      X509_WERR("TBSCertificate: keyinfo = %s\n", a[5]->debug_string());
      
      public_key = make_verifier(a[5]);

      if (!public_key)
	return 0;

      X509_WERR("TBSCertificate: parsed public key. type = %s\n",
		public_key->type);

      int i = 6;
      if (i == sizeof(a))
	return this_object();

      if (version < 2)
	return 0;

      if (! a[i]->constructed
	  && (a[i]->combined_tag == make_combined_tag(2, 1)))
      {
	issuer_id = asn1_bit_string()->decode_primitive(a[i]->raw);
	i++;
	if (i == sizeof(a))
	  return this_object();
      }
      if (! a[i]->constructed
	  && (a[i]->combined_tag == make_combined_tag(2, 2)))
      {
	subject_id = asn1_bit_string()->decode_primitive(a[i]->raw);
	i++;
	if (i == sizeof(a))
	  return this_object();
      }
      if (a[i]->constructed
	  && (a[i]->combined_tag == make_combined_tag(2, 3)))
      {
	extensions = a[i];
	i++;
	if (i == sizeof(a))
	  return this_object();
      }
      /* Too many fields */
      return 0;
    }
}      

object decode_certificate(string|object cert)
{
  if (stringp (cert)) cert = Standards.ASN1.Decode.simple_der_decode(cert);

  if (!cert
      || (cert->type_name != "SEQUENCE")
      || (sizeof(cert->elements) != 3)
      || (cert->elements[0]->type_name != "SEQUENCE")
      || (cert->elements[1]->type_name != "SEQUENCE")
      || (!sizeof(cert->elements[1]->elements))
      || (cert->elements[1]->elements[0]->type_name != "OBJECT IDENTIFIER")
      || (cert->elements[2]->type_name != "BIT STRING")
      || cert->elements[2]->unused)
    return 0;

  object(TBSCertificate) tbs = TBSCertificate()->init(cert->elements[0]);

  if (!tbs || (cert->elements[1]->get_der() != tbs->algorithm->get_der()))
    return 0;

  return tbs;
}

/* Decodes a certificate, checks the signature. Returns the
 * TBSCertificate structure, or 0 if decoding or verification failes.
 *
 * Authorities is a mapping from (DER-encoded) names to a verifiers. */

/* NOTE: This function allows self-signed certificates, and it doesn't
 * check that names or extensions make sense. */

object verify_certificate(string s, mapping authorities)
{
  object cert = Standards.ASN1.Decode.simple_der_decode(s);

  object(TBSCertificate) tbs = decode_certificate(cert);
  if (!tbs) return 0;

  object v;
  
  if (tbs->issuer->get_der() == tbs->subject->get_der())
  {
    /* A self signed certificate */
    X509_WERR("Self signed certificate\n");
    v = tbs->public_key;
  }
  else
    v = authorities[tbs->issuer->get_der()];

  return v && v->verify(cert->elements[1],
			cert->elements[0]->get_der(),
			cert->elements[2]->value)
    && tbs;
}

/* Decodes a certificate chain, checks the signatures. Returns a mapping
 * with the following contents, depending on the verification of the
 * certificate chain:
 *
 * ([ "self_signed" : non-zero if the certificate is self-signed,
 *    "verified"    : non-zero if the certificate is verified,
 *    "authority"   : DER-encoded name of the authority that verified the chain,
 * ])
 *
 * authorities is a mapping from (DER-encoded) names to verifiers. */

mapping verify_certificate_chain(array(string) cert_chain, mapping authorities)
{
  mapping m = ([ ]);

  for(int i=0; i<sizeof(cert_chain); i++)
  {
    object cert = Standards.ASN1.Decode.simple_der_decode(cert_chain[i]);

    object(TBSCertificate) tbs = decode_certificate(cert);
    if (!tbs) return m;

    object v;

    if(i == sizeof(cert_chain)-1) // Last cert
      v = authorities[tbs->issuer->get_der()];
    if (!v && tbs->issuer->get_der() == tbs->subject->get_der())
    {
      /* A self signed certificate */
      m->self_signed = 1;
      X509_WERR("Self signed certificate\n");
      v = tbs->public_key;
    }
    else
      v = authorities[tbs->issuer->get_der()];

    if (v && v->verify(cert->elements[1],
		       cert->elements[0]->get_der(),
		       cert->elements[2]->value)
	&& tbs)
    {
      m->authority = tbs->issuer->get_der();
      if(v == authorities[tbs->issuer->get_der()])
	m->verified = 1;
    }
  }
  return m;
}

#endif
