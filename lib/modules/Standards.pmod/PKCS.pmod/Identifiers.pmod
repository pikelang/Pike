//
// $Id: Identifiers.pmod,v 1.15 2004/04/14 20:19:26 nilsson Exp $
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

#if constant(Standards.ASN1.Types)

import Standards.ASN1.Types;

Identifier pkcs_id = Identifier(1, 2, 840, 113549, 1);
Identifier pkcs_1_id = pkcs_id->append(1);
Identifier pkcs_9_id = pkcs_id->append(9);

/* For public key */
Identifier rsa_id = pkcs_1_id->append(1);

/* Signature algorithms */
Identifier rsa_md2_id = pkcs_1_id->append(2);
Identifier rsa_md5_id = pkcs_1_id->append(4);
Identifier rsa_sha1_id = pkcs_1_id->append(5);

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

Identifier md2_id = Identifier(1, 2, 840, 113549, 2, 2);
Identifier md5_id = Identifier(1, 2, 840, 113549, 2, 5);
Identifier sha1_id = Identifier(1, 3, 14, 3, 2, 26);

/*      dhpublicnumber OBJECT IDENTIFIER ::= { iso(1) member-body(2)
                  us(840) ansi-x942(10046) number-type(2) 1 } */

Identifier dh_id = Identifier(1, 2, 840, 10046, 2, 1);

/* Object Identifiers used in X509 distinguished names */

Identifier at_id = Identifier(2, 5, 4);

mapping(Identifier:string) short_name_ids = 
([
  at_id->append(3) : "CN",       /* printable string */
  at_id->append(6) : "C",       /* printable string */
  at_id->append(7) : "L",      /* printable string */
  at_id->append(8) : "ST", /* printable string */
  at_id->append(10) : "O", /* printable string */
  at_id->append(11) : "OU",  /* printable string */
  Identifier(1, 2, 840, 113549, 1, 9, 1) : "E" /* printable string */
]);

mapping(string:Identifier) name_ids =
([  
  /* layman.asc says "commonUnitName". Typo? */
  "commonName" : at_id->append(3),        /* printable string */
  "countryName" : at_id->append(6),       /* printable string */
  "localityName" : at_id->append(7),      /* printable string */
  "stateOrProvinceName" : at_id->append(8), /* printable string */
  "organizationName" : at_id->append(10), /* printable string */
  "organizationUnitName" : at_id->append(11)  /* printable string */
  ]);

mapping(string:Identifier) attribute_ids =
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

/* From RFC 2459 */

mapping(string:Identifier) at_ids =
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
  "generationQualifier" : at_id->append(43),
  /* What does this attribute mean? */
  "dnQualifier" : at_id->append(46),
  /* Obsolete, not recommended. */
  "emailAddress" : pkcs_9_id->append(1)            /* IA5String */  
]);

Identifier ce_id = Identifier(2, 5, 29);
Identifier pkix_id = Identifier(1, 3, 6, 1, 5, 5, 7);


mapping(string:Identifier) ce_ids =
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
   "extKeyUsage"		: ce_id->append(37)
 ]);

/* Policy qualifiers */
Identifier qt_id = pkix_id->append(2);

mapping(string:Identifier) qt_ids =
([ "cps" : qt_id->append(1),
   "unotice" : qt_id->append(2) ]);
  
/* Key purposes */

Identifier kp_id = pkix_id->append(3);

mapping(string:Identifier) kp_ids =
([ "serverAuth" : kp_id->append(1),
   "clientAuth" : kp_id->append(2),
   "codeSigning" : kp_id->append(3),
   "emailProtection" : kp_id->append(4),
   "timeStamping" : kp_id->append(8) ]);

/* Private extensions */
Identifier pe_id = pkix_id->append(1);

/* Access descriptions */

Identifier ad_id = pkix_id->append(48);

mapping(string:Identifier) ad_ids =
([ "caIssuers" : ad_id->append(2) ]);

#else
constant this_program_does_not_exist=1;
#endif
