// $Id: Certificate.pmod,v 1.18 2004/04/14 20:19:26 nilsson Exp $

//! Handle PKCS-6 and PKCS-10 certificates and certificate requests.

#pike __REAL_VERSION__

/* ASN.1 structures:

CertificationRequestInfo ::= SEQUENCE {
  version Version,
  subject Name,
  subjectPublicKeyInfo SubjectPublicKeyInfo,
  attributes [0] IMPLICIT Attributes }

Version ::= INTEGER

Attributes ::= SET OF Attribute

-- From the last section of PKCS-9.
Attribute ::= SEQUENCE {
  attribyteType ::= OBJECT IDENTIFIER,
  attributeValue ::= SET OF ANY }

CertificationRequest ::= SEQUENCE {
  certificationRequestInfo CertificationRequestInfo,
  signatureAlgorithm SignatureAlgorithmIdentifier,
  signature Signature }

SignatureAlgorithmIdentifier ::= AlgorithmIdentifier

Signature ::= BIT STRING

CertificationRequest ::= SEQUENCE {
  certificationRequestInfo CertificationRequestInfo,
  signatureAlgorithm SignatureAlgorithmIdentifier,
  signature Signature }

SignatureAlgorithmIdentifier ::= AlgorithmIdentifier

Signature ::= BIT STRING

ExtendedCertificateOrCertificate ::= CHOICE {
  certificate Certificate, -- X.509
  extendedCertificate [0] IMPLICIT ExtendedCertificate

ExtendedCertificateInfo ::= SEQUENCE {
  version Version,
  certificate Certificate,
  attributes Attributes }

Version ::= INTEGER

ExtendedCertificate ::= SEQUENCE {
  extendedCertificateInfo ExtendedCertificateInfo,
  signatureAlgorithm SignatureAlgorithmIdentifier,
  signature Signature }

SignatureAlgorithmIdentifier ::= AlgorithmIdentifier

Signature ::= BIT STRING

CertificateInfo ::= SEQUENCE {
  version [0] Version DEFAULT v1988,
  serialNumber CertificateSerialNumber,
  signature AlgorithmIdentifier,
  issuer Name,
  validity Validity,
  subject Name,
  subjectPublicKeyInfo SubjectPublicKeyInfo }

Version ::= INTEGER { v1988(0) }

CertificateSerialNumber ::= INTEGER

Validity ::= SEQUENCE {
  notBefore UTCTime,
  notAfter UTCTime }

SubjectPublicKeyInfo ::= SEQUENCE {
  algorithm AlgorithmIdentifier,
  subjectPublicKey BIT STRING }

AlgorithmIdentifier ::= SEQUENCE {
  algorithm OBJECT IDENTIFIER,
  parameters ANY DEFINED BY ALGORITHM OPTIONAL }

Certificate ::= SEQUENCE {
  certificateInfo CertificateInfo,
  signatureAlgorithm AlgorithmIdentifier,
  signature BIT STRING }

Name ::= CHOICE {
  RDNSequence }

RDNSequence ::= SEQUENCE OF RelativeDistinguishedName

RelativeDistinguishedName ::=
  SET OF AttributeValueAssertion

AttributeValueAssertion ::= SEQUENCE {
   AttributeType,
   AttributeValue }

AttributeType ::= OBJECT IDENTIFIER

AttributeValue ::= ANY

RSAPublicKey ::= SEQUENCE {
  modulus INTEGER, -- n
  publicExponent INTEGER -- e }

RSAPrivateKey ::= SEQUENCE {
  version Version,
  modulus INTEGER, -- n
  publicExponent INTEGER, -- e
  privateExponent INTEGER, -- d
  prime1 INTEGER, -- p
  prime2 INTEGER, -- q
  exponent1 INTEGER, -- d mod (p-1)
  exponent2 INTEGER, -- d mod (q-1)
  coefficient INTEGER -- (inverse of q) mod p }

Version ::= INTEGER

*/

#if constant(Standards.ASN1.Types)

import Standards.ASN1.Types;
import .Identifiers;

class AttributeValueAssertion
{
  inherit Sequence;
  void create(mapping(string:object) types,
	      string type,
	      object value)
    {
      if (!types[type])
	error( "Unknown attribute type '%s'\n", type );
      ::create( ({ types[type], value }) );
    }
}

/* RelativeDistinguishedName */
class attribute_set
{
  inherit Set;

  void create(mapping(string:object) types, mapping(string:object) pairs)
  {
    ::create(map(indices(pairs),
		 lambda(string s, mapping m, mapping t) {
		   return AttributeValueAssertion(t, s, m[s]);
		 },
		 pairs, types));
  }
}

//!
Sequence build_distinguished_name(mapping(string:object) ... args)
{
  return Sequence(map(args, lambda(mapping rdn) {
			      return attribute_set(
				.Identifiers.at_ids, rdn);
			    } ));
}

Sequence decode_pem_certificate(string cert)
{

}

//! Return the certificate issuer RDN from a certificate string.
//!
//! @param cert
//! A string containing an X509 certificate.
//!
//! Note that the certificate normally must be decoded using
//! @[MIME.decode_base64].
//!
//! @returns
//!  An Standards.ASN1.Sequence object containing the certificate issuer
//!  Distinguished Name (DN).
Sequence get_certificate_issuer(string cert)
{
  return Standards.ASN1.Decode.simple_der_decode(cert)->elements[0]->elements[3];
}

//! Converts an RDN (relative distinguished name) Seqeunce object to a
//! human readable string in X500 format.
//!
//! @param cert
//! A string containing an X509 certificate.
//!
//! Note that the certificate normally must be decoded using
//! @[MIME.decode_base64].
//!
//! @returns
//!  A string containing the certificate issuer
//!  Distinguished Name (DN) in human readable X500 format.
//!
//! @note
//!  We don't currently handle attributes with multiple values, not
//!  all attribute types are understood.
string get_dn_string(Sequence dnsequence)
{
  string dn="";
  array rdns=({});

  foreach(reverse(dnsequence->elements), Set att)
  {
    foreach(att->elements, Sequence val)
    {
      array value = ({});
      string t = short_name_ids[val->elements[0]];
      string v = val->elements[1]->value;

      // we must escape characters now.
      v = replace(v, 
          ({",", "+", "\"", "\\", "<", ">", ";"}), 
          ({"\\,", "\\+", "\\\"", "\\\\", "\\<", "\\>", "\\;"}) );

      if(v[0..0] == " ")
         v="\\" + v;
      else if(v[0..0] == "#")
         v="\\" + v;

      if(v[(sizeof(v)-1)..(sizeof(v)-1)] == " ")
         v=v[0..(sizeof(v)-2)] + "\\ ";

      rdns += ({ (t + "=" + v) });
    }        
  }
  
  dn = rdns * ",";
  return dn;  
}

//! Return the certificate subject RDN from a certificate string.
//!
//! @param cert
//! A string containing an X509 certificate.
//!
//! Note that the certificate normally must be decoded using
//! @[MIME.decode_base64].
//!
//! @returns
//!  An Standards.ASN1.Sequence object containing the certificate subject
//!  Distinguished Name (DN).
Sequence get_certificate_subject(string cert)
{
  return Standards.ASN1.Decode.simple_der_decode(cert)->elements[0]->elements[5];
}

class Attribute
{
  inherit Sequence;

  void create(mapping(string:object) types, string type,
	      array(object) v)
  {
    if (!types[type])
      error( "Unknown attribute type '%s'\n", type);
    ::create( ({ types[type], Set(v) }) );
  }
}

class Attributes
{
  inherit Set;

  void create(mapping(string:object) types, mapping(string:array(object)) m)
  {
    ::create(map(indices(m),
		 lambda(string field, mapping m, mapping t) {
		   return Attribute(t, field, m[field]);
		 }, m, types));
  }
}

#else
constant this_program_does_not_exist=1;
#endif
