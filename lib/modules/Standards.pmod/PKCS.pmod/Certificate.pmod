
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

import Standards.ASN1.Types;
import .Identifiers;

protected object X509 = master()->resolv("Standards.X509");

class AttributeValueAssertion
{
  inherit Sequence;
  void create(mapping(string:object) types,
	      string type,
	      object value)
    {
      if (!objectp(types[type]))
	error("Unknown attribute type '%s':%O\n", type, types[type]);
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
variant Sequence build_distinguished_name(mapping args)
{
  // Turn mapping into array of pairs
  array a = ({});
  foreach(sort(indices(args)), string name)
    a += ({ ([name : args[name]]) });

  return build_distinguished_name(a);
}

//! Creates an ASN.1 @[Sequence] with the distinguished name of the
//! list of pairs given in @[args]. Supported identifiers are
//!
//! @dl
//!   @item commonName
//!   @item surname
//!   @item countryName
//!   @item localityName
//!   @item stateOrProvinceName
//!   @item organizationName
//!   @item organizationUnitName
//!   @item title
//!   @item name
//!   @item givenName
//!   @item initials
//!   @item generationQualifier
//!   @item dnQualifier
//!   @item emailAddress
//! @enddl
//!
//! @param args
//!   Either a mapping that lists from id string to string or ASN.1
//!   object, or an array with mappings, each containing one pair. No
//!   type validation is performed.
variant Sequence build_distinguished_name(array args)
{
  args += ({});
  // DWIM support. Turn string values into PrintableString.
  foreach(args; int i; mapping m) {
    foreach(m; string name; mixed value)
      if( stringp(value) )
      {
        args[i] = ([ name : PrintableString(value) ]);
        break;
      }
  }

  return Sequence(map(args, lambda(mapping rdn) {
			      return attribute_set(
				.Identifiers.at_ids, rdn);
			    } ));
}

//! Perform the reverse operation of @[build_distinguished_name()].
//!
//! @seealso
//!   @[build_distinguished_name()]
array(mapping(string(7bit):string)) decode_distinguished_name(Sequence dn)
{
  array(mapping(string(7bit):string)) ret =
    allocate(sizeof(dn->elements), aggregate_mapping)();
  foreach(dn->elements; int i; Set attr) {
    foreach(attr->elements, Sequence val) {
      string(7bit) name = reverse_at_ids[val[0]];
      if (!name) {
	// Unknown identifier.
	name = ((array(string))val[0]->id) * ".";
      }
      ret[i][name] = val[1]->value;
    }
  }
  return ret;
}

//! Return the certificate issuer RDN from a certificate string.
//!
//! @param cert
//! A string containing an X509 certificate.
//!
//! Note that the certificate usually must be decoded using
//! @[Standards.PEM.simple_decode()].
//!
//! @returns
//!  An Standards.ASN1.Sequence object containing the certificate issuer
//!  Distinguished Name (DN).
//!
//! @deprecated Standards.X509.decode_certificate
__deprecated__ Sequence get_certificate_issuer(string cert)
{
  return X509.decode_certificate(cert)->issuer;
}

//! Converts an RDN (relative distinguished name) Seqeunce object to a
//! human readable string in X500 format.
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
      string t = short_name_ids[val[0]];
      string v = val[1]->value;

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
//! Note that the certificate usually must be decoded using
//! @[PEM.simpe_decode()].
//!
//! @returns
//!  An Standards.ASN1.Sequence object containing the certificate subject
//!  Distinguished Name (DN).
//!
//! @deprecated Standards.X509.decode_certificate
__deprecated__ Sequence get_certificate_subject(string cert)
{
  return X509.decode_certificate(cert)->subject;
}

class Attribute
{
  inherit Sequence;

  protected void create(mapping(string:object) types, string type,
			array(object) v)
  {
    if (!types[type])
      error( "Unknown attribute type '%s'\n", type);
    ::create( ({ types[type], Set(v) }) );
  }
  protected variant void create(array(Object) elements)
  {
    if (sizeof(elements) != 2)
      error("Invalid attribute encoding.\n");
    ::create(elements);
  }
}

class Attributes
{
  inherit Set;

  protected void create(mapping(string:object) types,
			mapping(string:array(object)) m)
  {
    ::create(map(indices(m),
		 lambda(string field, mapping m, mapping t) {
		   return Attribute(t, field, m[field]);
		 }, m, types));
  }
  protected variant void create(array(Object) elements)
  {
    ::create(map(elements,
		 lambda(object e) {
		   return Attribute(e->elements);
		 }));
  }
}
