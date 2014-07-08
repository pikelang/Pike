//
// Object identifiers

#pike __REAL_VERSION__
#pragma strict_types

//! Various ASN.1 identifiers used by PKCS.

/* Attributes (from http://leangen.uninett.no:29659/~hta/ietf/oid/2.5.4.html):
   (by 1999-01-25, a better URL is http://www.alvestrand.no/objectid/top.html)

   2.5.4.0 - id-at-objectClass 
      2.5.4.1 - id-at-aliasedEntryName 
      2.5.4.2 - id-at-knowldgeinformation 
      2.5.4.3 - id-at-commonName 
      2.5.4.4 - id-at-surname 
      2.5.4.5 - id-at-serialNumber 
      2.5.4.6 - id-at-countryName 
      2.5.4.7 - id-at-localityName (1 more) 
      2.5.4.8 - id-at-stateOrProvinceName (1 more) 
      2.5.4.9 - id-at-streetAddress (1 more) 
      2.5.4.10 - id-at-organizationName (1 more) 
      2.5.4.11 - id-at-organizationalUnitName (1 more) 
      2.5.4.12 - id-at-title 
      2.5.4.13 - id-at-description 
      2.5.4.14 - id-at-searchGuide 
      2.5.4.15 - id-at-businessCategory 
      2.5.4.16 - id-at-postalAddress (1 more) 
      2.5.4.17 - id-at-postalCode (1 more) 
      2.5.4.18 - id-at-postOfficeBox (1 more) 
      2.5.4.19 - id-at-physicalDeliveryOfficeName (1 more) 
      2.5.4.20 - id-at-telephoneNumber (1 more) 
      2.5.4.21 - id-at-telexNumber (1 more) 
      2.5.4.22 - id-at-teletexTerminalIdentifier (1 more) 
      2.5.4.23 - id-at-facsimileTelephoneNumber (1 more) 
      2.5.4.24 - id-at-x121Address 
      2.5.4.25 - id-at-internationalISDNNumber (1 more) 
      2.5.4.26 - id-at-registeredAddress 
      2.5.4.27 - id-at-destinationIndicator 
      2.5.4.28 - id-at-preferredDeliveryMethod 
      2.5.4.29 - id-at-presentationAddress 
      2.5.4.30 - id-at-supportedApplicationContext 
      2.5.4.31 - id-at-member 
      2.5.4.32 - id-at-owner 
      2.5.4.33 - id-at-roleOccupant 
      2.5.4.34 - id-at-seeAlso 
      2.5.4.35 - id-at-userPassword 
      2.5.4.36 - id-at-userCertificate 
      2.5.4.37 - id-at-cACertificate

      2.5.4.38 - id-at-authorityRevocationList 
      2.5.4.39 - id-at-certificateRevocationList 
      2.5.4.40 - id-at-crossCertificatePair 
      2.5.4.41 - id-at-name 
      2.5.4.42 - id-at-givenName 
      2.5.4.43 - id-at-initials 
      2.5.4.44 - id-at-generationQualifier 
      2.5.4.45 - id-at-uniqueIdentifier 
      2.5.4.46 - id-at-dnQualifier 
      2.5.4.47 - id-at-enhancedSearchGuide 
      2.5.4.48 - id-at-protocolInformation 
      2.5.4.49 - id-at-distinguishedName 
      2.5.4.50 - id-at-uniqueMember 
      2.5.4.51 - id-at-houseIdentifier 
      2.5.4.52 - id-at-supportedAlgorithms 
      2.5.4.53 - id-at-deltaRevocationList
*/

import Standards.ASN1.Types;

Identifier pkcs_id = Identifier(1, 2, 840, 113549, 1);
Identifier pkcs_1_id = pkcs_id->append(1);
Identifier pkcs_9_id = pkcs_id->append(9);

/* For public key */
Identifier rsa_id = pkcs_1_id->append(1);

/* Signature algorithms */
Identifier rsa_md2_id = pkcs_1_id->append(2);
// Identifier rsa_md4_id = pkcs_1_id->append(3);
Identifier rsa_md5_id = pkcs_1_id->append(4);
Identifier rsa_sha1_id = pkcs_1_id->append(5);
Identifier rsa_sha256_id = pkcs_1_id->append(11);
Identifier rsa_sha384_id = pkcs_1_id->append(12);
Identifier rsa_sha512_id = pkcs_1_id->append(13);

Identifier ecdsa_sha1_id   = Identifier(1, 2, 840, 10045, 4, 1);
Identifier ecdsa_sha224_id = Identifier(1, 2, 840, 10045, 4, 3, 1);
Identifier ecdsa_sha256_id = Identifier(1, 2, 840, 10045, 4, 3, 2);
Identifier ecdsa_sha384_id = Identifier(1, 2, 840, 10045, 4, 3, 3);
Identifier ecdsa_sha512_id = Identifier(1, 2, 840, 10045, 4, 3, 4);

/* For public key (unrestricted) from RFC 5480. */
Identifier ec_id = Identifier(1, 2, 840, 10045, 2, 1);
Identifier ec_public_key_id = ec_id;

Identifier ec_dh_id = Identifier(1, 3, 132, 1, 12);
Identifier ec_mqw_id = Identifier(1, 3, 132, 1, 13);

/* Elliptic Curves from RFC 5480 */
Identifier ecc_secp192r1_id = Identifier(1, 2, 840, 10045, 3, 1, 1);
Identifier ecc_sect163k1_id = Identifier(1, 3, 132, 0, 1);
Identifier ecc_sect163r2_id = Identifier(1, 3, 132, 0, 15);
Identifier ecc_secp224r1_id = Identifier(1, 3, 132, 0, 33);
Identifier ecc_sect233k1_id = Identifier(1, 3, 132, 0, 26);
Identifier ecc_sect233r1_id = Identifier(1, 3, 132, 0, 27);
Identifier ecc_secp256r1_id = Identifier(1, 2, 840, 10045, 3, 1, 7);
Identifier ecc_sect283k1_id = Identifier(1, 3, 132, 0, 16);
Identifier ecc_sect283r1_id = Identifier(1, 3, 132, 0, 17);
Identifier ecc_secp384r1_id = Identifier(1, 3, 132, 0, 34);
Identifier ecc_sect409k1_id = Identifier(1, 3, 132, 0, 36);
Identifier ecc_sect409r1_id = Identifier(1, 3, 132, 0, 37);
Identifier ecc_secp521r1_id = Identifier(1, 3, 132, 0, 35);
Identifier ecc_sect571k1_id = Identifier(1, 3, 132, 0, 38);
Identifier ecc_sect571r1_id = Identifier(1, 3, 132, 0, 39);

/* Brainpool curves from RFC 5639. */
protected Identifier ec_sign_id = Identifier(1, 3, 36, 3, 3, 2);
Identifier ec_std_curves_and_generation_id = ec_sign_id->append(8);
protected Identifier ec_curve_id = ec_std_curves_and_generation_id->append(1);
protected Identifier ec_curve_ver1_id = ec_curve_id->append(1);
Identifier brainpool_p160r1 = ec_curve_ver1_id->append(1);
Identifier brainpool_p160t1 = ec_curve_ver1_id->append(2);
Identifier brainpool_p192r1 = ec_curve_ver1_id->append(3);
Identifier brainpool_p192t1 = ec_curve_ver1_id->append(4);
Identifier brainpool_p224r1 = ec_curve_ver1_id->append(5);
Identifier brainpool_p224t1 = ec_curve_ver1_id->append(6);
Identifier brainpool_p256r1 = ec_curve_ver1_id->append(7);
Identifier brainpool_p256t1 = ec_curve_ver1_id->append(8);
Identifier brainpool_p320r1 = ec_curve_ver1_id->append(9);
Identifier brainpool_p320t1 = ec_curve_ver1_id->append(10);
Identifier brainpool_p384r1 = ec_curve_ver1_id->append(11);
Identifier brainpool_p384t1 = ec_curve_ver1_id->append(12);
Identifier brainpool_p521r1 = ec_curve_ver1_id->append(13);
Identifier brainpool_p521t1 = ec_curve_ver1_id->append(14);

/* For public key 
        id-dsa ID ::= { iso(1) member-body(2) us(840) x9-57(10040)
                  x9cm(4) 1 }
*/
Identifier dsa_id = Identifier(1, 2, 840, 10040, 4, 1);

/* Signature algorithm 
           id-dsa-with-sha1 ID  ::=  {
                   iso(1) member-body(2) us(840) x9-57 (10040)
                   x9cm(4) 3 }
*/
Identifier dsa_sha_id = Identifier(1, 2, 840, 10040, 4, 3);
Identifier dsa_sha224_id = Identifier(2, 16, 840, 1, 101, 3, 4, 3, 1);
Identifier dsa_sha256_id = Identifier(2, 16, 840, 1, 101, 3, 4, 3, 2);

Identifier md2_id = Identifier(1, 2, 840, 113549, 2, 2);
Identifier md4_id = Identifier(1, 2, 840, 113549, 2, 4);
Identifier md5_id = Identifier(1, 2, 840, 113549, 2, 5);
Identifier sha1_id = Identifier(1, 3, 14, 3, 2, 26);
Identifier sha256_id = Identifier(2, 16, 840, 1, 101, 3, 4, 2, 1);
Identifier sha384_id = Identifier(2, 16, 840, 1, 101, 3, 4, 2, 2);
Identifier sha512_id = Identifier(2, 16, 840, 1, 101, 3, 4, 2, 3);

/*      dhpublicnumber OBJECT IDENTIFIER ::= { iso(1) member-body(2)
                  us(840) ansi-x942(10046) number-type(2) 1 } */

Identifier dh_id = Identifier(1, 2, 840, 10046, 2, 1);

/* Object Identifiers used in X509 distinguished names */

Identifier at_id = Identifier(2, 5, 4);

mapping(Identifier:string(7bit)) short_name_ids =
([
  at_id->append(3) : "CN",       /* printable string */
  at_id->append(6) : "C",       /* printable string */
  at_id->append(7) : "L",      /* printable string */
  at_id->append(8) : "ST", /* printable string */
  at_id->append(10) : "O", /* printable string */
  at_id->append(11) : "OU",  /* printable string */
  Identifier(1, 2, 840, 113549, 1, 9, 1) : "E" /* printable string */
]);

#define REVERSE(X) mkmapping(values(X),indices(X))

mapping(string(7bit):Identifier) name_ids =
([  
  /* layman.asc says "commonUnitName". Typo? */
  "commonName" : at_id->append(3),        /* printable string */
  "countryName" : at_id->append(6),       /* printable string */
  "localityName" : at_id->append(7),      /* printable string */
  "stateOrProvinceName" : at_id->append(8), /* printable string */
  "streetAddress" : at_id->append(9),     /* printable string */
  "organizationName" : at_id->append(10), /* printable string */
  "organizationUnitName" : at_id->append(11),  /* printable string */
  "postalCode" : at_id->append(17),       /* printable string */
  ]);

mapping(Identifier:string(7bit)) reverse_name_ids = REVERSE(name_ids);

mapping(string(7bit):Identifier) attribute_ids =
([
  "emailAddress" : pkcs_9_id->append(1),            /* IA5String */
  "unstructuredName" : pkcs_9_id->append(2),        /* IA5String */
  "contentType" : pkcs_9_id->append(3),             /* Identifier */
  "messageDigest" : pkcs_9_id->append(4),           /* Octet string */
  "signingTime" : pkcs_9_id->append(5),             /* UTCTime */
  "countersignature" : pkcs_9_id->append(6),        /* SignerInfo */
  "challengePassword" : pkcs_9_id->append(7),       /* Printable | T61
						       | Universal */
  "unstructuredAddress" : pkcs_9_id->append(8),     /* Printable | T61 */
  "extendedCertificateAttributes" : pkcs_9_id->append(9), /* Attributes */
  "friendlyName" : pkcs_9_id->append(20),           /* BMPString */
  "localKeyID" : pkcs_9_id->append(21)              /* OCTET STRING */
]);

mapping(Identifier:string(7bit)) reverse_attribute_ids =
  REVERSE(attribute_ids);

/* From RFC 2459 */

mapping(string(7bit):Identifier) at_ids =
([ /* All attribute values are a CHOICE of most string types,
    * including PrintableString, TeletexString (which in practice
    * means latin1) and UTF8String. */
  "commonName" : at_id->append(3),       
  "surname" : at_id->append(4),
  "countryName" : at_id->append(6),
  "localityName" : at_id->append(7),
  "stateOrProvinceName" : at_id->append(8),
  "organizationName" : at_id->append(10),
  "organizationUnitName" : at_id->append(11),
  "title" : at_id->append(12),
  "name" : at_id->append(41),
  "givenName" : at_id->append(42),
  "initials" : at_id->append(43),
  "generationQualifier" : at_id->append(44),
  /* What does this attribute mean? */
  "dnQualifier" : at_id->append(46),
  /* Obsolete, not recommended. */
  "emailAddress" : pkcs_9_id->append(1)            /* IA5String */  
]);

mapping(Identifier:string(7bit)) reverse_at_ids = REVERSE(at_ids);

Identifier ce_id = Identifier(2, 5, 29);
// RFC 3280
Identifier pkix_id = Identifier(1, 3, 6, 1, 5, 5, 7);

Identifier id_pe = pkix_id->append(1);

mapping(string(7bit):Identifier) ce_ids =
([
   "subjectDirectoryAttributes"	: ce_id->append(9),
   "subjectKeyIdentifier"	: ce_id->append(14),
   "keyUsage"			: ce_id->append(15),
   "privateKeyUsagePeriod"	: ce_id->append(16),
   "subjectAltName"		: ce_id->append(17),
   "issuerAltName"		: ce_id->append(18),
   "basicConstraints"		: ce_id->append(19),
   "nameConstraints"		: ce_id->append(30),
   "cRLDistributionPoints"	: ce_id->append(31),
   "certificatePolicies"	: ce_id->append(32),
   "policyMappings"		: ce_id->append(33),
   "authorityKeyIdentifier"	: ce_id->append(35),
   "policyConstraints"		: ce_id->append(36),
   "extKeyUsage"		: ce_id->append(37),

   // Private extensions (IANA security-related objects)
   "authorityInfoAccess"        : id_pe->append(1),
   "biometricInfo"              : id_pe->append(2),
   "qcStatements"               : id_pe->append(3),
   "logotype"                   : id_pe->append(12),

   // 2.16.840.1.113730.1.1 Netscape Certificate type
 ]);

mapping(Identifier:string(7bit)) reverse_ce_ids = REVERSE(ce_ids);

/* Policy qualifiers */
Identifier qt_id = pkix_id->append(2);

mapping(string(7bit):Identifier) qt_ids =
([ "cps" : qt_id->append(1),
   "unotice" : qt_id->append(2) ]);

mapping(Identifier:string(7bit)) reverse_qt_ids = REVERSE(qt_ids);

/* Key purposes */

Identifier kp_id = pkix_id->append(3);

mapping(string(7bit):Identifier) kp_ids =
([ "serverAuth"      : kp_id->append(1),
   "clientAuth"      : kp_id->append(2),
   "codeSigning"     : kp_id->append(3),
   "emailProtection" : kp_id->append(4),
   "timeStamping"    : kp_id->append(8),
   "OCSPSigning"     : kp_id->append(9),
]);

mapping(Identifier:string(7bit)) reverse_kp_ids = REVERSE(kp_ids);

/* Private extensions */
Identifier pe_id = pkix_id->append(1);

/* RFC 4985 - Subject Alternative Name for Expression of Service Name */
Identifier on_id = pkix_id->append(8);
Identifier on_dnsSRV_id = on_id->append(7);

/* Access descriptions */

Identifier ad_id = pkix_id->append(48);

mapping(string(7bit):Identifier) ad_ids =
([ "caIssuers" : ad_id->append(2) ]);

mapping(Identifier:string(7bit)) reverse_ad_ids = REVERSE(ad_ids);
