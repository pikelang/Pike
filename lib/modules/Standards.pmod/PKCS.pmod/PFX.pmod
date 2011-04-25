//
// $Id$
//

#if 0 // This code is broken. (Missing mac identifier for sha)

/*
 * M$ Personal Exchange Syntax and Protocol Standard, aka PKCS#12
 *
 * Subsets of PKCS#12 and PKCS#7 needed to import keys and
 * certificates into Netscape and IE.
 *
 */

import Standards.ASN1.Types;

#pike __REAL_VERSION__

object pkcs_7_id = .Identifiers.pkcs_id->append(7);
object data_id = pkcs_7_id->append(1);
object signed_data_id = pkcs_7_id->append(2);
object enveloped_data_id = pkcs_7_id->append(3);
object signed_and_enveloped_data_id = pkcs_7_id->append(4);
object digested_data_id = pkcs_7_id->append(5);
object encrypted_data_id = pkcs_7_id->append(7);

object pkcs_12_id = .Identifiers.pkcs_id->append(12);
object pkcs_12_pbe_id = pkcs_12_id->append(1);
object pbe_sha_rc4 = pkcs_12_pbe_id->append(1);
object pbe_sha_rc4_weak = pkcs_12_pbe_id->append(2);
object pbe_sha_3tripledes = pkcs_12_pbe_id->append(3);
object pbe_sha_2triple_des = pkcs_12_pbe_id->append(4);
object pbe_sha_rc2= pkcs_12_pbe_id->append(5);
object pbe_sha_rc2_weak = pkcs_12_pbe_id->append(6);

object pkcs_12_version1_id = pkcs_12_id->append(10);
object pkcs_12_bag_id = pkcs_12_version1_id->append(1);
object keybag_id = pkcs_12_bag_id->append(1);
object pkcs_8_shroudedkeybag_id = pkcs_12_bag_id->append(2);
object certbag_id = pkcs_12_bag_id->append(3);
object crlbag_id = pkcs_12_bag_id->append(4);
object secretbag_id = pkcs_12_bag_id->append(5);
object safebag_id = pkcs_12_bag_id->append(6);

object pkcs_9_id = .Identifiers.pkcs_id->append(9);

object certTypes_id = pkcs_9_id->append(22);
object x509Certificate_id = certTypes_id->append(1);

/* Perhaps ContentInfo should be moved into a separate module, with
   other PKCS#7 stuff? */

class ContentInfo_meta
{
  /* Maps DER-encoded identifiers to corresponding content types
   * (including explicit tags) */
  mapping(string:function) content_types;

  class `()
    {
      inherit Standards.ASN1.Types.Sequence;
  
      mapping element_types(int i, mapping t)
	{
	  switch(i)
	  {
	  case 0:
	    return ([ 6 : Identifier ]);
	  case 1:
	    return ([ 0 : content_types[elements[0]->get_der()] ]);
	  default:
	    error("Bad index\n");
	  }
	}
      
      this_program set_types(mapping(string:function) types)
	{
	  content_types = types;
	  return this;
	}

      this_program end_decode_constructed(int length)
	{
	  if (length > 2)
	    error("ContentInfo->end_decode_constructed: Bad index\n");
	  return this;
	}
  
#if 0
      object decode_constructed(array contents, string raw)
	{
	  Sequence::decode_constructed(contents, raw);
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
	  return this;
	}
#endif

      object init(object type, object contents)
	{
	  /* Neglects the valid_types field of meta_explicit */
	  return ::init( ({ type, meta_explicit(0, 0)()->init(contents) }) );
	}

#if 0
      string get_data_der()
	{
	  return elements[1]->get_der();
	}
#endif

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
  inherit Sequence;

  object init(object algorithm_id, string|object key,
	      void|object attr)
    {
      if (stringp(key))
	key = OctetString(key);

      array a = ({ Integer(0), algorithm_id, key });
      if (attr)
	a += ({ attr });

      return ::init(a);
    }
}

/* Defaults for generated MAC:s */
   
#define SALT_SIZE 17
#define MAC_COUNT 1

class PFX
{
  inherit Sequence;

  object safes;
  string passwd; /* Assumed to be latin1 */

  this_program init(object s) {
    safes = s;
    return this;
  }

  object set_signature_key(object key) {
    error("Not implemented\n");
  }

  string latin1_to_bmp(string s) {
    /* String of 16 bit characters in big-endian order, terminated
     * by a null character */
    return "\0" + (s/"") * "\0" + "\0\0";
  }
  
  /* passwd is assumed to be latin 1 */
  void set_passwd(string s) {
    passwd = latin1_to_bmp(passwd);
  }

  string string_pad(string d, int block_size) {
    int s = sizeof(d);

    if (s)
    { /* Extend to a multiple of the block soze */
      int n = (s + 63) / block_size; // number of blocks
      int k = (n+s-1) / s;
      d = (d * k) [..n*block_size-1];
    }
    return d;
  }
  
  string generate_key(string salt, int id, int count, int needed)
    { /* Supports only SHA-1 */
      string D = sprintf("%c", id) * 64;

      string I = string_pad(salt, 64) + string_pad(passwd, 64);

      string A = D+I;

      for(int i; i<count; i++)
	A = Crypto.sha()->update(A)->digest;

      if (sizeof(A)<needed)
	error("PFX: Step 6c) of section 6.1 not implemented.\n");

      return A[..ndeded-1];
    }

  string get_hmac(string salt, int count)
    {
      string key = generate_key(salt, 3, count, 20);

      return Crypto.hmac(Crypto.sha)(key)
	// Extract value from the data field
	(elements[1]->elements[1]->value);
    }
  
  string der_encode()
    {
      elements = allocate(2 + !!passwd);
      elements[0] = Integer(3); // version
      elements[1] = safes;
      if (passwd)
      {	/* Password-integrity mode */
	salt = Crypto.Random.random_string(SALT_SIZE);

	elements[2] = Sequence(
	  ({ Sequence(
	    ({ Identifiers.sha_id,
	       OctetString(get_hmac(salt, MAC_COUNT)) }) ),
	     OctetString(salt)
	     /* , optional count, default = 1 */
	  }) );
	
      } else {
	error("Only passwd authentication supported\n");
      }
    }

  int uses_passwd_integrity()
    {
      return elements[1]->elements[0] == data_id;
    }
  
  int verify_passwd()
    {
      if (elements[2]->elements[0]->elements[0] != Identifiers.sha1_id)
	error("Unexpected hash algorithm\n");
      string salt = elements[2]->elements[1]->value;
      int count = (sizeof(elements[2]->elements) == 3)
	? (int) elements[2]->elements[2]->value : 1;
      if (count < 1)
	error("Bad count\n");

      return (elements[2]->elements[0]->elements[1]->value
	      == get_hmac(salt, count));
    }
}

/* Note that the ASN.1 type is a sequence of the structure this
 * function creates */
Sequence make_key_info(object id, string key)
{
  return Sequence( ({ Integer(0), id, OctetString(key) }) );
}

Sequence make_safe_bag(object id, object contents, object|void attributes)
{
  return Sequence( ({ id, contents })
		   + (attributes ? ({ attributes }) : ({ }) ));
}
  
/* A SafeBag instance, with type of KeyBag */
Sequence make_key_bag(array keys, object|void attributes)
{
  return make_safe_bag(keybag_id, Sequence(keys), attributes);
}

/* A safe bag of type certBag, containing a certBag of type x509Certificate */
Sequence make_x509_cert_bag(string cert, object|void attributes)
{
  return Sequence(certbag_id,
		  Sequence( ({ x509Certificate_id, cert }) ),
		  attributes);
}

/* Makes a PFX of unencrypted bags */
PFX simple_make_pfx(array bags, string passwd)
{
  Sequence safe_contents = Sequence(bags);
  
  PFX pfx = PFX(ContentInfo_meta()(data_id, String(safes->get_der())));
  pfx->set_passwd(passwd);
  return pfx;
}

#endif
