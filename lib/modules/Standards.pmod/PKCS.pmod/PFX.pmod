/* PFX.pmod
 *
 * M$ Personal Exchange Syntax and Protocol Standard, aka PKCS#12
 *
 * Subsets of PKCS#12 and PKCS#7 needed to import keys and
 * certificates into Netscape and IE.
 *
 */

import Standards.ASN1

pkcs_7_id = .Identifiers.pkcs_id->append(7);
data_id = pkcs_7_id->append(1);
signed_data_id = pkcs_7_id->append(2);
enveloped_data_id = pkcs_7_id->append(3);
signed_and_enveloped_data_id = pkcs_7_id->append(4);
digested_data_id = pkcs_7_id->append(5);
encrypted_data_id = pkcs_7_id->append(7);

pkcs_12_id = .Identifiers.pkcs_id->append(12);
pkcs_12_pbe_id = pkcs_12_id->append(1);
pbe_sha_rc4 = pkcs_12_pbe_id->append(1);
pbe_sha_rc4_weak = pkcs_12_pbe_id->append(2);
pbe_sha_3tripledes = pkcs_12_pbe_id->append(3);
pbe_sha_2triple_des = pkcs_12_pbe_id->append(4);
pbe_sha_rc2= pkcs_12_pbe_id->append(5);
pbe_sha_rc2_weak = pkcs_12_pbe_id->append(6);

pkcs_12_version1_id = pkcs_12_id->append(10);
pkcs_12_bag_id = pkcs_12_version1_id->append(1);
keybag_id = pkcs_12_bag_id->append(1);
pkcs_8_keybag_id = pkcs_12_bag_id->append(2);
certbag_id = pkcs_12_bag_id->append(3);
crlbag_id = pkcs_12_bag_id->append(4);
secretbag_id = pkcs_12_bag_id->append(5);
safebag_id = pkcs_12_bag_id->append(6);

/* Perhaps ContentInfo should be moved into a separate module, with
   other PKCS#7 stuff? */

class ContentInfo_meta
{
  /* Maps DER-encoded identifiers to corresponding content types
   * (including explicit tags) */
  mapping(string:function) content_types;

  class `()
    {
      inherit asn1_sequence;
  
      mapping element_types(int i, mapping t)
	{
	  switch(i)
	  {
	  case 0:
	    return ([ 6 : asn1_identifier ]);
	  case 1:
	    return ([ 0 : content_types[elements[0]->get_der()] ]);
	  default:
	    error("ContentInfo->element_types: Bad index\n");
	  }
	}
      
      object set_types(mapping(string:function) types)
	{
	  content_types = types;
	  return this_object();
	}

      object end_decode_constructed(int length)
	{
	  if (length > 2)
	    error("ContentInfo->end_decode_constructed: Bad index\n");
	  return this_object();
	}
  
#if 0
      object decode_constructed(array contents, string raw)
	{
	  asn1_sequence::decode_constructed(contents, raw);
	  if (!sizeof(elements)
	      || (elements[0] != "Identifier")
	      || (sizeof(elements) > 2))
	    return 0;

	  function p = content_types[elements[0]->get_der()];
	  if (sizeof(elements) == 1)
	  {
	    /* No contents */
	    if (p)
	      /* No content expected */
	      return 0;
	  } else {
	    /* Decode contents */
	    if (!p)
	      return 0;
	    elements[1] = p(elements[1]);
	  }
	  return this_object();
	}
#endif

      object init(object type, object contents)
	{
	  /* Neglects the valid_types field of meta_explicit */
	  return ::init( ({ type, meta_explicit(0, 0)()->init(contents) }) );
	}
    }
  void create(mapping|void types)
    {
      content_types = types;
    }
}

/* PFX definition, taken from the preview of PKCS#12 version 3
   
   PFX ::= SEQUENCE {
   	       version         Version    -- V3(3) for this version.  This
                                          -- field is not optional.
   	       authSafes       ContentInfo,    -- from PKCS #7 v1.5
   			       -- SignedData in public-key integrity mode
   			       -- Data in password integrity mode
   	       macData         MacData OPTIONAL }
   
   
   MacData ::= SEQUENCE {
   	       mac      DigestInfo,    -- from PKCS #7 v1.5
   	       macSalt  OCTET STRING,
   	       macIterationCount  INTEGER DEFAULT 1 }
   	       -- if you want to be compatible with a certain release from
   	       -- Microsoft, you should use the value 1 and not place the
   	       -- macIterationCount field's encoding in the PDU's
   	       -- BER-encoding.  Unfortunately, using a value of 1 here
   	       -- means that there's no point in having a value other
   	       -- than 1 for any password-based encryption in the PDU that
   	       -- uses the same password as is used for password-based
   	       -- authentication
   
   AuthenticatedSafes ::= 	SEQUENCE OF ContentInfo    -- from PKCS #7 v1.5
   			   -- Data if unencrypted
   			   -- EncryptedData if password-encrypted
   			   -- EnvelopedData if public-key-encrypted
   
   
   pkcs-12PbeParams ::= SEQUENCE {
   	       salt           OCTET STRING,
   	       iterationCount INTEGER }
   
   
   pkcs-12PbeIds OBJECT IDENTIFIER ::= { pkcs-12 1 }
   pbeWithSHA1And128BitRC4 OBJECT IDENTIFIER ::= { pkcs-12PbeIds 1 }
   pbeWithSHA1And40BitRC4 OBJECT IDENTIFIER ::= { pkcs-12PbeIds 2 }
   -- both triple-DES pbe OIDs do EDE (encrypt-decrypt-encrypt)
   pbeWithSHA1And3-KeyTripleDES-CBC OBJECT IDENTIFIER ::=
     { pkcs-12PbeIds 3 }
   pbeWithSHA1And2-KeyTripleDES-CBC OBJECT IDENTIFIER ::=
     { pkcs-12PbeIds 4 }
   -- pbeWithSHA1And128BitRC2-CBC uses an effective keyspace search
   -- size of 128 bits, as well as a 128-bit key
   pbeWithSHA1And128BitRC2-CBC OBJECT IDENTIFIER ::=
     { pkcs-12PbeIds 5 }
   -- pbeWithSHA1And40BitRC2-CBC uses an effective keyspace search
   -- size of 40 bits, as well as a 40-bit key
   pbeWithSHA1And40BitRC2-CBC OBJECT IDENTIFIER ::=
     { pkcs-12PbeIds 6 }
   
   
   SafeContents ::= SEQUENCE OF SafeBag
   
   SafeBag ::= SEQUENCE {
   	       bagType       OBJECT IDENTIFIER,
   	       bagContent    [0] EXPLICIT ANY DEFINED BY bagType,
   	       bagAttributes Attributes OPTIONAL }
   
   
   Attributes ::= SET OF Attribute    -- from X.501
   -- in pre-1994 ASN.1, Attribute looks like:
   -- Attribute ::= SEQUENCE {
   --      type OBJECT IDENTIFIER,
   --      values SET OF ANY DEFINED BY type }
   
   
   FriendlyName ::= BMPString    -- a friendlyName has a single attr. value
   LocalKeyID   ::= OCTET STRING    -- a localKeyID has a single attr.
   value
   
   
   friendlyName OBJECT IDENTIFIER ::= { PKCS-9 20 }
   localKeyID   OBJECT IDENTIFIER ::= { PKCS-9 21 }
   
   
   KeyBag ::= PrivateKeyInfo    -- from PKCS #8 v1.2
   
   PKCS-8ShroudedKeyBag ::= EncryptedPrivateKeyInfo    -- from PKCS #8 v1.2
   
   CertBag   ::= SEQUENCE {
   	     certType    OBJECT IDENTIFIER,
   	     cert        EXPLICIT [0] ANY DEFINED BY certType }
   
   CRLBag    ::= SEQUENCE {
   	     crlType     OBJECT IDENTIFIER,
   	     crl         EXPLICIT [0] ANY DEFINED BY crlType }
   
   SecretBag ::= SEQUENCE {
   	     secretType  OBJECT IDENTIFIER,
   	     secret      EXPLICIT [0] ANY DEFINED BY secretType }
   
   SafeContentsBag ::= SafeContents
   
   
   pkcs-12Version1      OBJECT IDENTIFIER ::= { pkcs-12 10 }
   pkcs-12BagIds        OBJECT IDENTIFIER ::= { pkcs-12Version1 1}
   keyBag               OBJECT IDENTIFIER ::= { pkcs-12BagIds 1 }
   pkcs-8ShroudedKeyBag OBJECT IDENTIFIER ::= { pkcs-12BagIds 2 }
   certBag              OBJECT IDENTIFIER ::= { pkcs-12BagIds 3 }
   crlBag               OBJECT IDENTIFIER ::= { pkcs-12BagIds 4 }
   secretBag            OBJECT IDENTIFIER ::= { pkcs-12BagIds 5 }
   safeContentsBag      OBJECT IDENTIFIER ::= { pkcs-12BagIds 6 }
   
   
   certTypes       OBJECT IDENTIFIER ::= { PKCS-9 22 }
   X509Certificate                   ::= OCTET STRING
   SDSICertificate                   ::= IA5String
   x509Certificate OBJECT IDENTIFIER ::= { certTypes 1 }
   sdsiCertificate OBJECT IDENTIFIER ::= { certTypes 2 }
   
   
   crlTypes OBJECT IDENTIFIER ::= { PKCS-9 23 }
   X509Crl                    ::= OCTET STRING
   x509Crl  OBJECT IDENTIFIER ::= { crlTypes 1 }
*/

/* Same as PKCS#8 PrivateKeyInfo */
class KeyBag
{
  inherit asn1_sequence;

  object init(object algorithm_id, string|object key,
	      void|object attr)
    {
      if (stringp(key))
	key = asn1_octet_string(key);

      array a = ({ asn1_integer(0), algorithm_id, key });
      if (attr)
	a += ({ attr });

      return ::init(a);
    }
}

class pfx
{
  inherit asn1_sequence;

  object data;
  string passwd; /* Assumed to be latin1 */
  
  object init(object s)
    {
      data = s;
      return this_object();
    }

  object set_signature_key(object key)
    {
      error("pfx->sign: Not implemented\n");
    }

  string latin1_to_bmp(string s)
    {
      /* String of 16 bit characters in big-endian order, terminated
       * by a null character */
      return "\0" + (s/"") * "\0" + "\0\0";
    }
  
  /* passwd is assumed to be latin 1 */
  object set_passwd(string passwd)
    {
      hmac = Crypto.hmac(Crypto.sha)(latin1_to_bmp(passwd));
    }

  der_encode()
    {
      if (hmac)
      {	/* Password-integrity mode */
	
  
		    
  
  
}

object simple_make_pfx(
      
  
