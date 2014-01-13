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

#if 0
// A CA certificate does not have the CA basic constraint.
constant CERT_UNAUTHORIZED_CA = 7;
#endif

protected {
  MetaExplicit extension_sequence = MetaExplicit(2, 3);
  MetaExplicit version_integer = MetaExplicit(2, 0);

  mapping algorithms = ([
#if constant(Crypto.MD2)
    Identifiers.rsa_md2_id->get_der() : Crypto.MD2,
#endif
    Identifiers.rsa_md5_id->get_der() : Crypto.MD5,
    Identifiers.rsa_sha1_id->get_der() : Crypto.SHA1,
    Identifiers.rsa_sha256_id->get_der() : Crypto.SHA256,
#if constant(Crypto.SHA384)
    Identifiers.rsa_sha384_id->get_der() : Crypto.SHA384,
#endif
#if constant(Crypto.SHA512)
    Identifiers.rsa_sha512_id->get_der() : Crypto.SHA512,
#endif

    Identifiers.dsa_sha_id->get_der() : Crypto.SHA1,
#if constant(Crypto.SHA224)
    Identifiers.dsa_sha224_id->get_der() : Crypto.SHA224,
#endif
    Identifiers.dsa_sha256_id->get_der() : Crypto.SHA256,

#if constant(Crypto.SHA224)
    Identifiers.ecdsa_sha224_id->get_der() : Crypto.SHA224,
#endif
    Identifiers.ecdsa_sha256_id->get_der() : Crypto.SHA256,
#if constant(Crypto.SHA384)
    Identifiers.ecdsa_sha384_id->get_der() : Crypto.SHA384,
#endif
#if constant(Crypto.SHA512)
    Identifiers.ecdsa_sha512_id->get_der() : Crypto.SHA512,
#endif
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
  Sequence validity = Sequence( ({ UTC()->set_posix(now),
                                   UTC()->set_posix(now + ttl) }) );

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
//!   RSA, DSA or ECDSA parameters for the issuer.
//!   See @[Crypto.RSA], @[Crypto.DSA] and @[Crypto.ECC.Curve.ECDSA].
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
string sign_key(Sequence issuer, Crypto.Sign c, Crypto.Hash h,
                Sequence subject, int serial, int ttl, array|void extensions)
{
  Sequence algorithm_id = c->pkcs_signature_algorithm_id(h);
  if(!algorithm_id) error("Can't use %O for %O.\n", h, c);
  Sequence tbs = make_tbs(issuer, algorithm_id,
                          subject, c->pkcs_public_key(),
                          Integer(serial), ttl, extensions);

  return Sequence(({ tbs, c->pkcs_signature_algorithm_id(h),
                     BitString(c->pkcs_sign(tbs->get_der(), h))
                  }))->get_der();
}

//! Creates a selfsigned certificate, i.e. where issuer and subject
//! are the same entity. This entity is derived from the list of pairs
//! in @[name], which is encoded into an distinguished_name by
//! @[Standards.PKCS.Certificate.build_distinguished_name].
//!
//! @param c
//!   The public key cipher used for the certificate, @[Crypto.RSA],
//!   @[Crypto.DSA] or @[Crypto.ECC.Curve.ECDSA]. The object should be
//!   initialized with (at least) public keys.
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
//! @param h
//!   The hash function to use for the certificate. Must be one of the
//!   standardized PKCS hashes to be used with the given Crypto. By
//!   default @[Crypto.SHA256] is selected for both RSA and DSA.
//!
//! @param serial
//!   Serial number of the certificate. Defaults to generating a UUID
//!   version1 value with random node. Some browsers will refuse
//!   different certificates from the same signer with the same serial
//!   number.
string make_selfsigned_certificate(Crypto.Sign c, int ttl,
                                   mapping|array name, array|void extensions,
                                   void|Crypto.Hash h, void|int serial)
{
  if(!serial)
    serial = (int)Gmp.mpz(Standards.UUID.make_version1(-1)->encode(), 256);
  Sequence dn = Certificate.build_distinguished_name(name);
  return sign_key(dn, c, h||Crypto.SHA256, dn, serial, ttl, extensions);
}

class Verifier {
  constant type = "none";
  Crypto.Sign pkc;
  optional Crypto.RSA rsa;
  optional Crypto.DSA dsa;
#if constant(Crypto.ECC.Curve)
  optional Crypto.ECC.SECP_521R1.ECDSA ecdsa;
#endif

  //! Verifies the @[signature] of the certificate @[msg] using the
  //! indicated hash @[algorithm].
  int(0..1) verify(Sequence algorithm, string msg, string signature)
  {
    Crypto.Hash hash = algorithms[algorithm[0]->get_der()];
    if (!hash) return 0;
    return pkc && pkc->pkcs_verify(msg, hash, signature);
  }
}

protected class RSAVerifier
{
  inherit Verifier;
  constant type = "rsa";

  protected void create(string key) {
    pkc = RSA.parse_public_key(key);
  }

  Crypto.RSA `rsa() { return [object(Crypto.RSA)]pkc; }
}

protected class DSAVerifier
{
  inherit Verifier;
  constant type = "dsa";

  protected void create(string key, Gmp.mpz p, Gmp.mpz q, Gmp.mpz g)
  {
    pkc = DSA.parse_public_key(key, p, q, g);
  }

  Crypto.DSA `dsa() { return [object(Crypto.DSA)]pkc; }
}

#if constant(Crypto.ECC.Curve)
protected class ECDSAVerifier
{
  inherit Verifier;
  constant type = "ecdsa";

  protected void create(string(8bit) key, string(8bit) curve_der)
  {
    Crypto.ECC.Curve curve;
    foreach(values(Crypto.ECC), mixed c) {
      if (objectp(c) && c->pkcs_named_curve_id &&
	  (c->pkcs_named_curve_id()->get_der() == curve_der)) {
	curve = [object(Crypto.ECC.Curve)]c;
	break;
      }
    }
    DBG("ECC Curve: %O (DER: %O)\n", curve, curve_der);
    pkc = curve->ECDSA()->set_public_key(key);
  }

  Crypto.ECC.SECP_521R1.ECDSA `ecdsa()
  {
    return [object(Crypto.ECC.SECP_521R1.ECDSA)]pkc;
  }
}
#endif

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

#if constant(Crypto.ECC.Curve)
  if(seq[0]->get_der() == Identifiers.ec_id->get_der())
  {
    if( sizeof(seq)!=2 || seq[1]->type_name!="SEQUENCE" ||
        sizeof(seq[1])!=1 || seq[1][0]->type_name!="OBJECT IDENTIFIER" )
      return 0;

    Sequence params = seq[1];
    return ECDSAVerifier(str->value, params[0]->get_der());
  }
#endif

  DBG("make_verifier: Unknown algorithm identifier: %O\n", seq[0]);
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
  int not_after;

  //!
  int not_before;

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
  mapping(string:Object) extensions = ([]);

  //! @note
  //! optional
  multiset critical = (<>);

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

    catch {
      not_before = validity[0]->get_posix();
    };
    if (!not_before)
      return 0;
    DBG("TBSCertificate: not_before = %O\n", not_before);

    catch {
      not_after = validity[1]->get_posix();
    };
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
      DBG("TBSCertificate: issuer_id = %O\n", issuer_id);
      i++;
      if (i == sizeof(a))
	return this;
    }
    if (! a[i]->constructed
	&& (a[i]->combined_tag == make_combined_tag(2, 2)))
    {
      subject_id = BitString()->decode_primitive(a[i]->raw);
      DBG("TBSCertificate: subject_id = %O\n", subject_id);
      i++;
      if (i == sizeof(a))
	return this;
    }
    if (a[i]->constructed
	&& (a[i]->combined_tag == make_combined_tag(2, 3))
        && sizeof(a[i])==1
        && a[i][0]->type_name == "SEQUENCE")
    {
      extensions = ([]);
      foreach(a[i][0]->elements, Object _ext)
      {
        if( _ext->type_name != "SEQUENCE" ||
            sizeof(_ext)<2 || sizeof(_ext)>3 )
        {
          DBG("TBSCertificate: Bad extensions structure.\n");
          return 0;
        }
        Sequence ext = [object(Sequence)]_ext;
        if( ext[0]->type_name != "OBJECT IDENTIFIER" ||
            ext[-1]->type_name != "OCTET STRING" )
        {
          DBG("TBSCertificate: Bad extensions structure.\n");
          return 0;
        }
        DBG("TBSCertificate: extension: %O\n", ext[0]);
        string id = ext[0]->get_der();

        if( extensions[id] )
        {
          DBG("TBSCertificate: extension %O sent twice.\n");
          return 0;
        }

        extensions[ id ] =
          Standards.ASN1.Decode.simple_der_decode(ext->elements[-1]->value);
        if(sizeof(ext)==3)
        {
          if( ext[1]->type_name != "BOOLEAN" ) return 0;
          if( ext[1]->value ) critical[id]=1;
        }
      }

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
  {
    DBG("Certificate has the wrong ASN.1 structure.\n");
    return 0;
  }

  TBSCertificate tbs = TBSCertificate()->init(cert[0]);

  if (!tbs || (cert[1]->get_der() != tbs->algorithm->get_der()))
  {
    DBG("Failed to generate TBSCertificate.\n");
    return 0;
  }

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
TBSCertificate verify_certificate(string s, mapping(string:Verifier) authorities)
{
  object cert = Standards.ASN1.Decode.simple_der_decode(s);

  TBSCertificate tbs = decode_certificate(cert);
  if (!tbs) return 0;

  object v;
  
  if (tbs->issuer->get_der() == tbs->subject->get_der())
  {
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

//! Decodes a root certificate using @[decode_certificate] and
//! verifies that all extensions mandated for root certificates are
//! present and valid.
TBSCertificate verify_root_certificate(string s)
{
  TBSCertificate tbs = decode_certificate(s);
  if(!tbs) return 0;

  multiset crit = tbs->critical + (<>);

  Object lookup(int num)
  {
    string id = Identifiers.ce_id->append(num)->get_der();
    crit[id]=0;
    return tbs->extensions[id];
  };

  // id-ce-basicConstraints is required for certificates with public
  // key used to validate certificate signatures. RFC 3280, 4.2.1.10.
  Object c = lookup(19);
  if( !c || c->type_name!="SEQUENCE" || sizeof(c)<1 || sizeof(c)>2 ||
      c[0]->type_name!="BOOLEAN" ||
      !c[0]->value )
  {
    DBG("verify root: Bad or missing id-ce-basicConstraints.\n");
    return 0;
  }

  // id-ce-authorityKeyIdentifier is required, unless self signed. RFC
  // 3280 4.2.1.1
  if( !lookup(35) && tbs->issuer->get_der() != tbs->subject->get_der() )
  {
    DBG("verify root: Missing id-ce-authorityKeyIdentifier.\n");
    return 0;
  }

  // id-ce-subjectKeyIdentifier is required. RFC 3280 4.2.1.2
  if( !lookup(14) )
  {
    DBG("verify root: Missing id-ce-subjectKeyIdentifier.\n");
    return 0;
  }

  // id-ce-keyUsage is required. RFC 3280 4.2.1.3
  if( !lookup(15) ) // FIXME: Look at usage bits
  {
    DBG("verify root: Missing id-ce-keyUsage.\n");
    return 0;
  }

  // One or more critical extensions have not been processed.
  if( sizeof(crit) )
  {
    DBG("verify root: Critical unknown extensions %O.\n", crit);
    return 0;
  }

  return tbs;
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
//!       @[CERT_ROOT_UNTRUSTED], @[CERT_BAD_SIGNATURE], @[CERT_INVALID]
//!       or @[CERT_CHAIN_BROKEN]
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
				 mapping(string:Verifier) authorities,
                                 int|void require_trust)
{
  mapping m = ([ ]);
#define ERROR(X) do { \
    DBG("Error " #X "\n"); \
    m->verified=0; m->error_code=(X); m->error_cert=idx; \
    return m; \
    } while(0)

  int len = sizeof(cert_chain);
  array chain_obj = allocate(len);
  array chain_cert = allocate(len);

  foreach(cert_chain; int idx; string c)
  {
     object cert = Standards.ASN1.Decode.simple_der_decode(c);
     TBSCertificate tbs = decode_certificate(cert);
     if(!tbs)
       ERROR(CERT_INVALID);

     int idx = len-idx-1;
     chain_cert[idx] = cert;
     chain_obj[idx] = tbs;
  }

  foreach(chain_obj; int idx; TBSCertificate tbs)
  {
    Verifier v;

    if(idx == 0) // The root cert
    {
      v = authorities[tbs->issuer->get_der()];

      // if we don't know the issuer of the root certificate, and we
      // require trust, we're done.
      if(!v && require_trust)
        ERROR(CERT_ROOT_UNTRUSTED);

      // Is the root self signed?
      if (tbs->issuer->get_der() == tbs->subject->get_der())
      {
        DBG("Self signed certificate\n");
        m->self_signed = 1;

        // always trust our own authority first, even if it is self signed.
        if(!v) 
          v = tbs->public_key;
      }
    }

    else // otherwise, we make sure the chain is unbroken.
    {
      // is the certificate in effect (time-wise)?
      int my_time = time();

      // Check not_before. We want the current time to be later.
      if(my_time < tbs->not_before)
        ERROR(CERT_TOO_NEW);

      // Check not_after. We want the current time to be earlier.
      if(my_time > tbs->not_after)
        ERROR(CERT_TOO_OLD);

      // is the issuer of this certificate the subject of the previous
      // (more rootward) certificate?
      if(tbs->issuer->get_der() != chain_obj[idx-1]->subject->get_der())
        ERROR(CERT_CHAIN_BROKEN);

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
        ERROR(CERT_BAD_SIGNATURE);
    }
  }
  return m;

#undef ERROR
}

#endif
