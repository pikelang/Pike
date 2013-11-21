#pike __REAL_VERSION__
//#pragma strict_types

//! Functions to generate and validate RFC2459 style X.509 v3
//! certificates.

constant dont_dump_module = 1;

#if constant(Crypto.Hash)

import Standards.ASN1.Types;
import Standards.PKCS;

#ifdef X509_DEBUG
#define DBG(X ...) werror(X)
#else
#define DBG(X ...)
#endif

//!
constant CERT_TOO_OLD = 1;

//!
constant CERT_TOO_NEW = 2;

//!
constant CERT_INVALID = 3;

//!
constant CERT_CHAIN_BROKEN = 4;

//!
constant CERT_ROOT_UNTRUSTED = 5;

//!
constant CERT_BAD_SIGNATURE = 6;

//!
constant CERT_UNAUTHORIZED_CA = 7;

//! Creates a @[Standards.ASN1.Types.UTC] object from the posix
//! time @[t].
UTC make_time(int t)
{
  Calendar.ISO.Second second = Calendar.ISO.Second(t)->set_timezone("UTC");

  if (second->year_no() >= 2050)
    error( "Times later than 2049 not supported yet\n" );

  return UTC(sprintf("%02d%02d%02d%02d%02d%02dZ",
		     second->year_no() % 100,
		     second->month_no(),
		     second->month_day(),
		     second->hour_no(),
		     second->minute_no(),
		     second->second_no()));
}

//! Returns a mapping similar to that returned by gmtime. Returns
//! @expr{0@} on invalid dates.
//!
//! @returns
//!   @mapping
//!     @member int "year"
//!     @member int "mon"
//!     @member int "mday"
//!     @member int "hour"
//!     @member int "min"
//!     @member int "sec"
//!   @endmapping
mapping(string:int) parse_time(UTC asn1)
{
  if ((asn1->type_name != "UTCTime")
      || (sizeof(asn1->value) != 13))
    return 0;

  sscanf(asn1->value, "%[0-9]s%c", string s, int c);
  if ( (sizeof(s) != 12) && (c != 'Z') )
    return 0;

  mapping(string:int) m = mkmapping( ({ "year", "mon", "mday",
					"hour", "min", "sec" }),
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

//! Comparision function between two "date" mappings of the
//! kind that @[parse_time] returns.
int(-1..1) time_compare(mapping(string:int) t1, mapping(string:int) t2)
{
  foreach( ({ "year", "mon", "mday", "hour", "min", "sec" }), string name)
    if (t1[name] < t2[name])
      return -1;
    else if (t1[name] > t2[name])
      return 1;
  return 0;
}

protected {
  MetaExplicit extension_sequence = MetaExplicit(2, 3);
  MetaExplicit version_integer = MetaExplicit(2, 0);

  // FIXME: These should probably move into PKCS.
  Sequence rsa_md2_algorithm = Sequence( ({ Identifiers.rsa_md2_id, Null() }) );

  Sequence rsa_md5_algorithm = Sequence( ({ Identifiers.rsa_md5_id, Null() }) );

  Sequence rsa_sha1_algorithm = Sequence( ({ Identifiers.rsa_sha1_id,
                                             Null() }) );

  Sequence rsa_sha256_algorithm = Sequence( ({ Identifiers.rsa_sha256_id,
                                               Null() }) );

  Sequence rsa_sha384_algorithm = Sequence( ({ Identifiers.rsa_sha384_id,
                                               Null() }) );

  Sequence rsa_sha512_algorithm = Sequence( ({ Identifiers.rsa_sha512_id,
                                               Null() }) );

  Sequence dsa_sha1_algorithm = Sequence( ({ Identifiers.dsa_sha_id }) );
  Sequence dsa_sha224_algorithm = Sequence( ({ Identifiers.dsa_sha224_id }) );
  Sequence dsa_sha256_algorithm = Sequence( ({ Identifiers.dsa_sha256_id }) );

  mapping algorithms = ([
#if constant(Crypto.MD2)
    rsa_md2_algorithm->get_der() : Crypto.MD2,
#endif
    rsa_md5_algorithm->get_der() : Crypto.MD5,
    rsa_sha1_algorithm->get_der() : Crypto.SHA1,
    rsa_sha256_algorithm->get_der() : Crypto.SHA256,
#if constant(Crypto.SHA384)
    rsa_sha384_algorithm->get_der() : Crypto.SHA384,
#endif
#if constant(Crypto.SHA512)
    rsa_sha512_algorithm->get_der() : Crypto.SHA512,
#endif

    dsa_sha1_algorithm->get_der() : Crypto.SHA1,
    dsa_sha224_algorithm->get_der() : Crypto.SHA224,
    dsa_sha256_algorithm->get_der() : Crypto.SHA256,
  ]);
}

//! Creates the ASN.1 TBSCertificate sequence (see RFC2459 section
//! 4.1) to be signed (TBS) by the CA. version is explicitly set to
//! v3, validity is calculated based on time and @[ttl], and
//! @[extensions] is optionally added to the sequence. issuerUniqueID
//! and subjectUniqueID are not supported.
Sequence make_tbs(Sequence issuer, Sequence algorithm,
                  Sequence subject, Sequence keyinfo,
                  Integer serial, int ttl,
                  array extensions)
{
  int now = time();
  Sequence validity = Sequence( ({ make_time(now), make_time(now + ttl) }) );

  return (extensions
	  ? Sequence( ({ version_integer(Integer(2)), /* Version 3 */
			 serial,
			 algorithm,
			 issuer,
			 validity,
			 subject,
			 keyinfo,
			 extension_sequence(extensions) }) )
	  : Sequence( ({ serial,
			 algorithm,
			 issuer,
			 validity,
			 subject,
			 keyinfo }) ));
}

//!
//! @param issuer
//!   Distinguished name for the issuer.
//!   See @[Standards.PKCS.Certificate.build_distinguished_name].
//!
//! @param c
//!   RSA or DSA parameters for the issuer.
//!   See @[Crypto.RSA] and @[Crypto.DSA].
//!
//! @param subject
//!   Distinguished name for the issuer.
//!   See @[Standards.PKCS.Certificate.build_distinguished_name].
//!
//! @param public_key
//!   DER-encoded RSAPublicKey structure.
//!   See @[Standards.PKCS.RSA.public_key()].
//!
//! @param serial
//!   Serial number for this key and subject.
//!
//! @param ttl
//!   Validity time in seconds for this signature to be valid.
//!
//! @param extensions
//!   Set of extensions.
//!
//! @returns
//!   Returns a DER-encoded certificate.
string sign_key(Sequence issuer, Crypto.RSA|Crypto.DSA c, Sequence subject,
                int serial, int ttl, array|void extensions)
{
  Sequence tbs = make_tbs(issuer, c->pkcs_algorithm_id(Crypto.SHA1),
                          subject, c->pkcs_public_key(),
                          Integer(serial), ttl, extensions);

  return Sequence(({ tbs, c->pkcs_algorithm_id(Crypto.SHA1),
                     BitString(c->pkcs_sign(tbs->get_der(), Crypto.SHA1))
                  }))->get_der();
}

//! Creates a selfsigned certificate, i.e. where issuer and subject
//! are the same entity. This entity is derived from the list of pairs
//! in @[name], which is encoded into an distinguished_name by
//! @[Standards.PKCS.Certificate.build_distinguished_name].
//!
//! @param c
//!   The public key cipher used for the certificate, @[Crypto.RSA] or
//!   @[Crypto.DSA]. The object should be initialized with (at least)
//!   public keys.
//!
//! @param ttl
//!   The validity of the certificate, in seconds, starting from
//!   creation date.
//!
//! @param name
//!   List of properties to create distinguished name from.
//!
//! @param extensions
//!   List of extensions as ASN.1 structures.
//!
//! @param serial
//!   Serial number of the certificate. Defaults to generating a UUID
//!   version1 value with random node. Some browsers will refuse
//!   different certificates from the same signer with the same serial
//!   number.
string make_selfsigned_certificate(Crypto.RSA|Crypto.DSA c, int ttl,
                                   mapping|array name, array|void extensions,
                                   void|int serial)
{
  if(!serial)
    serial = (int)Gmp.mpz(Standards.UUID.make_version1(-1)->encode(), 256);
  Sequence dn = Certificate.build_distinguished_name(name);
  return sign_key(dn, c, dn, serial, ttl, extensions);
}

class Verifier {
  constant type = "none";
  int(0..1) verify(object,string,string);
  optional Crypto.RSA rsa;
  optional Crypto.DSA dsa;
}

protected class RSAVerifier
{
  inherit Verifier;
  Crypto.RSA rsa;

  constant type = "rsa";

  protected void create(string key) {
    rsa = RSA.parse_public_key(key);
  }

  //!
  int(0..1) verify(Sequence algorithm, string msg, string signature)
  {
    if (!rsa) return 0;
    Crypto.Hash hash = algorithms[algorithm->get_der()];
    if (!hash) return 0;
    return rsa->pkcs_verify(msg, hash, signature);
  }
}

protected class DSAVerifier
{
  inherit Verifier;
  Crypto.DSA dsa;

  constant type = "dsa";

  protected void create(string key, Gmp.mpz p, Gmp.mpz q, Gmp.mpz g)
  {
    dsa = DSA.parse_public_key(key, p, q, g);
  }

  //! Verifies the @[signature] of the certificate @[msg] using the
  //! indicated hash @[algorithm]. The signature is the DER-encoded
  //! ASN.1 sequence Dss-Sig-Value with the two integers r and s. See
  //! RFC 3279 section 2.2.2.
  int(0..1) verify(Sequence algorithm, string msg, string signature)
  {
    if (!dsa) return 0;
    Crypto.Hash hash = algorithms[algorithm->get_der()];
    if (!hash) return 0;
    return dsa->pkcs_verify(msg, hash, signature);
  }
}

protected Verifier make_verifier(Object _keyinfo)
{
  if( _keyinfo->type_name != "SEQUENCE" )
    return 0;
  Sequence keyinfo = [object(Sequence)]_keyinfo;

  if ( (keyinfo->type_name != "SEQUENCE")
       || (sizeof(keyinfo) != 2)
       || (keyinfo[0]->type_name != "SEQUENCE")
       || !sizeof( [object(Sequence)]keyinfo[0] )
       || (keyinfo[1]->type_name != "BIT STRING")
       || keyinfo[1]->unused)
    return 0;
  Sequence seq = [object(Sequence)]keyinfo[0];
  String str = [object(String)]keyinfo[1];

  if(sizeof(seq)==0) return 0;

  if (seq[0]->get_der() == Identifiers.rsa_id->get_der())
  {
    if ( (sizeof(seq) != 2)
	 || (seq[1]->get_der() != Null()->get_der()) )
      return 0;

    return RSAVerifier(str->value);
  }

  if(seq[0]->get_der() == Identifiers.dsa_id->get_der())
  {
    if( sizeof(seq)!=2 || seq[1]->type_name!="SEQUENCE" ||
        sizeof(seq[1])!=3 || seq[1][0]->type_name!="INTEGER" ||
        seq[1][1]->type_name!="INTEGER" || seq[1][2]->type_name!="INTEGER" )
      return 0;

    Sequence params = seq[1];
    return DSAVerifier(str->value, params[0]->value,
                       params[1]->value, params[2]->value);
  }
}

//! Represents a TBSCertificate.
class TBSCertificate
{
  //!
  string der;

  //!  
  int version;

  //!
  Gmp.mpz serial;
  
  //!
  Sequence algorithm;  /* Algorithm Identifier */

  //!
  Sequence issuer;

  //!
  mapping not_after;

  //!
  mapping not_before;

  //!
  Sequence subject;

  //!
  Verifier public_key;

  /* Optional */

  //! @note
  //! optional
  BitString issuer_id;
  
  //! @note
  //! optional
  BitString subject_id;

  //! @note
  //! optional
  object extensions;

  protected mixed cast(string to)
  {
    switch(to)
    {
    case "mapping":
      return ([ "version" : version,
                "algorithm" : algorithm,
                "issuer" : issuer,
                "subject" : subject,
      ]);
      break;
    default:
      error("Can't case %O to %O\n", this_program, to);
      break;
    }
  }

  protected string get_id(object asn)
  {
    foreach(.PKCS.Identifiers.name_ids; string name; object id)
      if( asn==id ) return name;
    return (array(string))asn->id*".";
  }

  protected array fmt_asn1(object asn)
  {
    array i = ({});
    mapping m = ([]);

    foreach(asn->elements;; object o)
    {
      o = o[0];
      string id = get_id(o[0]);
      i += ({ ([ id : o[1]->value]) });
      if( m )
      {
        if(m[id])
        {
          m = 0;
          continue;
        }
        m[id] = o[1]->value;
      }
    }

    return m || i;
  }

  protected string _sprintf(int t)
  {
    if( t!='O' ) return UNDEFINED;
    mapping m = cast("mapping");
    catch {
      m->issuer = fmt_asn1(m->issuer);
      m->subject = fmt_asn1(m->subject);
    };
    return sprintf("%O(%O)", this_program, m);
  }

  //! Populates the object from a certificate decoded into an ASN.1
  //! Object. Returns the object on success, otherwise @expr{0@}. You
  //! probably want to call @[decode_certificate] or even
  //! @[verify_certificate].
  this_program init(Object asn1)
  {
    der = asn1->get_der();
    if (asn1->type_name != "SEQUENCE")
      return 0;

    array(Object) a = ([object(Sequence)]asn1)->elements;
    DBG("TBSCertificate: sizeof(a) = %d\n", sizeof(a));
      
    if (sizeof(a) < 6)
      return 0;

    if (sizeof(a) > 6)
    {
      /* The optional version field must be present */
      if (!a[0]->constructed
	  || (a[0]->get_combined_tag() != make_combined_tag(2, 0))
	  || (sizeof(a[0]) != 1)
	  || (a[0][0]->type_name != "INTEGER"))
	return 0;

      version = (int) a[0][0]->value + 1;
      if ( (version < 2) || (version > 3))
	return 0;
      a = a[1..];
    } else
      version = 1;
    DBG("TBSCertificate: version = %d\n", version);

    if (a[0]->type_name != "INTEGER")
      return 0;
    serial = a[0]->value;
    DBG("TBSCertificate: serial = %s\n", (string) serial);
      
    if ((a[1]->type_name != "SEQUENCE")
	|| !sizeof(a[1])
	|| (a[1][0]->type_name != "OBJECT IDENTIFIER"))
      return 0;

    algorithm = a[1];
    DBG("TBSCertificate: algorithm = %O\n", algorithm);

    if (a[2]->type_name != "SEQUENCE")
      return 0;
    issuer = a[2];
    DBG("TBSCertificate: issuer = %O\n", issuer);

    if ((a[3]->type_name != "SEQUENCE")
	|| (sizeof(a[3]) != 2))
      return 0;
    array validity = a[3]->elements;

    not_before = parse_time(validity[0]);
    if (!not_before)
      return 0;
    DBG("TBSCertificate: not_before = %O\n", not_before);

    not_after = parse_time(validity[1]);
    if (!not_after)
      return 0;
    DBG("TBSCertificate: not_after = %O\n", not_after);

    if (a[4]->type_name != "SEQUENCE")
      return 0;
    subject = a[4];

    DBG("TBSCertificate: keyinfo = %O\n", a[5]);
    public_key = make_verifier(a[5]);

    if (!public_key)
      return 0;

    DBG("TBSCertificate: parsed public key. type = %s\n",
        public_key->type);

    int i = 6;
    if (i == sizeof(a))
      return this;

    if (version < 2)
      return 0;

    if (! a[i]->constructed
	&& (a[i]->combined_tag == make_combined_tag(2, 1)))
    {
      issuer_id = BitString()->decode_primitive(a[i]->raw);
      i++;
      if (i == sizeof(a))
	return this;
    }
    if (! a[i]->constructed
	&& (a[i]->combined_tag == make_combined_tag(2, 2)))
    {
      subject_id = BitString()->decode_primitive(a[i]->raw);
      i++;
      if (i == sizeof(a))
	return this;
    }
    if (a[i]->constructed
	&& (a[i]->combined_tag == make_combined_tag(2, 3)))
    {
      extensions = a[i];
      i++;
      if (i == sizeof(a))
	return this;
    }
    /* Too many fields */
    return 0;
  }
}

//! Decodes a certificate and verifies that it is structually sound.
//! Returns a @[TBSCertificate] object if ok, otherwise @expr{0@}.
TBSCertificate decode_certificate(string|object cert)
{
  if (stringp (cert)) cert = Standards.ASN1.Decode.simple_der_decode(cert);

  if (!cert
      || (cert->type_name != "SEQUENCE")
      || (sizeof(cert) != 3)
      || (cert[0]->type_name != "SEQUENCE")
      || (cert[1]->type_name != "SEQUENCE")
      || (!sizeof(cert[1]))
      || (cert[1][0]->type_name != "OBJECT IDENTIFIER")
      || (cert[2]->type_name != "BIT STRING")
      || cert[2]->unused)
    return 0;

  TBSCertificate tbs = TBSCertificate()->init(cert[0]);

  if (!tbs || (cert[1]->get_der() != tbs->algorithm->get_der()))
    return 0;

  return tbs;
}

//! Decodes a certificate, checks the signature. Returns the
//! TBSCertificate structure, or 0 if decoding or verification failes.
//! The valid time range for the certificate is not checked.
//!
//! Authorities is a mapping from (DER-encoded) names to a verifiers.
//!
//! @note
//!   This function allows self-signed certificates, and it doesn't
//!   check that names or extensions make sense.
TBSCertificate verify_certificate(string s, mapping authorities)
{
  object cert = Standards.ASN1.Decode.simple_der_decode(s);

  TBSCertificate tbs = decode_certificate(cert);
  if (!tbs) return 0;

  object v;
  
  if (tbs->issuer->get_der() == tbs->subject->get_der())
  {
    /* A self signed certificate */
    DBG("Self signed certificate\n");
    v = tbs->public_key;
  }
  else
    v = authorities[tbs->issuer->get_der()];

  return v && v->verify(cert[1],
			cert[0]->get_der(),
			cert[2]->value)
    && tbs;
}

//! Decodes a certificate chain, checks the signatures. Verifies that the
//! chain is unbroken, and that all certificates are in effect  
//! (time-wise.) 
//!
//! Returns a mapping with the following contents, depending 
//! on the verification of the certificate chain:
//!
//! @mapping
//!   @member int "error_code"
//!     Error describing type of verification failure, if verification failed.
//!     May be one of the following: @[CERT_TOO_NEW], @[CERT_TOO_OLD],
//!       @[CERT_ROOT_UNTRUSTED], @[CERT_BAD_SIGNATURE], @[CERT_INVALID],
//!       @[CERT_UNAUTHORIZED_CA] or @[CERT_CHAIN_BROKEN]
//!   @member int "error_cert"
//!     Index number of the certificate that caused the verification failure.
//!   @member int(0..1) "self_signed"
//!     Non-zero if the certificate is self-signed.
//!   @member int(0..1) "verified"
//!     Non-zero if the certificate is verified.
//!   @member string "authority"
//!     @[Standards.ASN1.Sequence] of the authority RDN that verified
//!     the chain.
//!   @member string "cn"
//!     @[Standards.ASN1.Sequence] of the common name RDN of the leaf
//!     certificate.
//! @endmapping
//!
//! @param cert_chain
//!   An array of certificates, with the relative-root last. Each
//!   certificate should be a DER-encoded certificate.
//! @param authorities
//!   A mapping from (DER-encoded) names to verifiers.
//! @param require_trust
//!   Require that the certificate be traced to an authority, even if
//!   it is self signed.
//!
//! See @[Standards.PKCS.Certificate.get_dn_string] for converting the
//! RDN to an X500 style string.
mapping verify_certificate_chain(array(string) cert_chain,
				 mapping authorities, int|void require_trust)
{

  mapping m = ([ ]);

  int len = sizeof(cert_chain);
  array chain_obj = allocate(len);
  array chain_cert = allocate(len);

  foreach(cert_chain; int idx; string c)
  {
     object cert = Standards.ASN1.Decode.simple_der_decode(c);
     TBSCertificate tbs = decode_certificate(cert);
     if(!tbs)
     { 
       m->error_code = CERT_INVALID;
       m->error_cert = idx;
       return m;
     }

     int idx = len-idx-1;
     chain_cert[idx] = cert;
     chain_obj[idx] = tbs;
  }

  foreach(chain_obj; int idx; TBSCertificate tbs)
  {
    object v;

#if 0
    // NOTE: disabled due to unreliable presence of cA constraint.
    //
    // if we are a CA certificate (we don't care about the end cert)
    // make sure the CA constraint is set.
    // 
    // should we be considering self signed certificates?
    if(idx != (sizeof(chain_obj)-1))
    {
        int caok = 0;

        if(tbs->extensions && sizeof(tbs->extensions))
        {
            werror("have extensions.\n");
            foreach(tbs->extensions[0]->elements, Sequence c)
            {
               werror("checking each element...\n");
               if(c[0] == Identifiers.ce_id->append(19))
               {
                 werror("have a basic constraints element.\n");
                 foreach(c->elements[1..], Sequence v)
                 {
                   werror("checking for boolean: " + v->type_name + " " + v->value + "\n");
                   if(v->type_name == "BOOLEAN" && v->value == 1)
                     caok = 1;
                 }
               }
            }
        }
        
        if(! caok)
        {
          DBG("a CA certificate does not have the CA basic constraint.\n");
          m->error_code = CERT_UNAUTHORIZED_CA;
          m->error_cert = idx;
          return m;
        }
    }
#endif  /* 0 */

    if(idx == 0) // The root cert
    {
      v = authorities[tbs->issuer->get_der()];

      // if we don't know the issuer of the root certificate, and we
      // require trust, we're done.
      if(!v && require_trust)
      {
         DBG("we require trust, but haven't got it.\n");
        m->error_code = CERT_ROOT_UNTRUSTED;
        m->error_cert = idx;
        return m;
      }

      // is the root self signed?
      if (tbs->issuer->get_der() == tbs->subject->get_der())
      {
        /* A self signed certificate */
        m->self_signed = 1;
        DBG("Self signed certificate\n");

        // always trust our own authority first, even if it is self signed.
        if(!v) 
          v = tbs->public_key;
      }
    }

    else // otherwise, we make sure the chain is unbroken.
    {
      // is the certificate in effect (time-wise)?
      int my_time = time();

      // first check not_before. we want the current time to be later.
      if(my_time < mktime(tbs->not_before))
      {
        m->verified = 0;
        m->error_code = CERT_TOO_NEW;
        m->error_cert = idx;
        return m;
      }

      // first check not_after. we want the current time to be earlier.
      if(my_time > mktime(tbs->not_after))
      {
        m->verified = 0;
        m->error_code = CERT_TOO_OLD;
        m->error_cert = idx;
        return m;
      }

      // is the issuer of this certificate the subject of the previous
      // (more rootward) certificate?
      if(tbs->issuer->get_der() != chain_obj[idx-1]->subject->get_der())
      {
        DBG("issuer chain is broken!\n");
        m->verified = 0;
        m->error_code = CERT_CHAIN_BROKEN;
        m->error_cert = idx;
        return m;
      }
      // the verifier for this certificate should be the public key of
      // the previous certificate in the chain.
      v = chain_obj[idx-1]->public_key;
    }

    if (v)
    {
      if( v->verify(chain_cert[idx][1],
                    chain_cert[idx][0]->get_der(),
                    chain_cert[idx][2]->value)
          && tbs)
      {
        DBG("signature is verified..\n");
        m->verified = 1;

        // if we're the root of the chain and we've verified, this is
        // the authority.
        if(idx == 0)
          m->authority = tbs->issuer;
 
        if(idx == sizeof(chain_cert)-1) m->cn = tbs->subject;
      }
      else
      {
        DBG("signature _not_ verified...\n");
        m->error_code = CERT_BAD_SIGNATURE;
        m->error_cert = idx;
        m->verified = 0;
        return m;
      }
    }
  }
  return m;
}

#endif
