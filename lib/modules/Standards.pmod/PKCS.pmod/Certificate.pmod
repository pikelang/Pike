/* certificate.pmod
 *
 * Handle pkcs-6 and pkcs-10 certificates and certificate requests.
 *
 */

/* ASN.1 structures:

#pike __REAL_VERSION__

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

#if __VERSION__ >= 0.6
import ".";
#endif /* __VERSION__ >= 0.6 */

#if constant(Standards.ASN1.Types.asn1_sequence)

import Standards.ASN1.Types;
import Identifiers;

class AttributeValueAssertion
{
  import Standards.ASN1.Types;
  inherit asn1_sequence;
  void create(mapping(string:object) types,
	      string type,
	      object value)
    {
      if (!types[type])
	error( "AttributeValueAssertion: "
	       "Unknown attribute type '%s'\n", type );
      ::create( ({ types[type], value }) );
    }
}

/* RelativeDistinguishedName */
class attribute_set
{
  import Standards.ASN1.Types;
  inherit asn1_set;

  void create(mapping(string:object) types, mapping(string:object) pairs)
    {
      ::create(Array.map(indices(pairs),
			 lambda(string s, mapping m, mapping t)
			 {
			   return AttributeValueAssertion(t, s, m[s]);
			 },
			   pairs, types));
    }
}

object build_distinguished_name(mapping(string:object) ... args)
{
  return asn1_sequence(Array.map(args, lambda(mapping rdn)
				 {
				   return attribute_set(
				     Identifiers.at_ids, rdn);
				 } ));
}

class Attribute
{
  import Standards.ASN1.Types;
  inherit asn1_sequence;

  void create(mapping(string:object) types, string type,
	      array(object) v)
    {
      if (!types[type])
	error( "Attribute: Unknown attribute type '%s'\n", type);
      ::create( ({ types[type], asn1_set(v) }) );
    }
}

class Attributes
{
  import Standards.ASN1.Types;
  inherit asn1_set;

  void create(mapping(string:object) types, mapping(string:array(object)) m)
    {
      ::create(Array.map(indices(m),
			 lambda(string field, mapping m, mapping t)
			 {
			   return Attribute(t, field, m[field]);
			 }, m, types));
    }
}
      
#endif
