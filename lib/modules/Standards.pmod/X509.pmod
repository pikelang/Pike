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

// Bit 0 is the first bit in the BitString.
protected enum keyUsage {
  digitalSignature  = (1<<(7-0)),
  nonRepudiation    = (1<<(7-1)),
  keyEncipherment   = (1<<(7-2)),
  dataEncipherment  = (1<<(7-3)),
  keyAgreement      = (1<<(7-4)),
  keyCertSign       = (1<<(7-5)),
  cRLSign           = (1<<(7-6)),
};


//! Unique identifier for the certificate issuer.
//!
//! X.509v2 (deprecated).
class IssuerId {
  inherit BitString;
  constant cls = 2;
  constant tag = 1;
}

//! Unique identifier for the certificate subject.
//!
//! X.509v2 (deprecated).
class SubjectId {
  inherit BitString;
  constant cls = 2;
  constant tag = 2;
}

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

    Identifiers.ecdsa_sha1_id->get_der() : Crypto.SHA1,
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

class Verifier {
  constant type = "none";
  Crypto.Sign pkc;
  optional __deprecated__(Crypto.RSA) rsa;
  optional __deprecated__(Crypto.DSA) dsa;

  //! Verifies the @[signature] of the certificate @[msg] using the
  //! indicated hash @[algorithm].
  int(0..1) verify(Sequence algorithm, string msg, string signature)
  {
    DBG("Verify hash %O\n", algorithm[0]);
    Crypto.Hash hash = algorithms[algorithm[0]->get_der()];
    if (!hash) return 0;
    return pkc && pkc->pkcs_verify(msg, hash, signature);
  }

  protected string _sprintf(int t)
  {
    return t=='O' && sprintf("%O(%O)", this_program, pkc);
  }
}

protected class RSAVerifier
{
  inherit Verifier;
  constant type = "rsa";

  protected void create(string key) {
    pkc = RSA.parse_public_key(key);
  }

  __deprecated__ Crypto.RSA `rsa() { return [object(Crypto.RSA)]pkc; }
}

protected class DSAVerifier
{
  inherit Verifier;
  constant type = "dsa";

  protected void create(string key, Gmp.mpz p, Gmp.mpz q, Gmp.mpz g)
  {
    pkc = DSA.parse_public_key(key, p, q, g);
  }

  __deprecated__ Crypto.DSA `dsa() { return [object(Crypto.DSA)]pkc; }
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
    if( sizeof(seq)!=2 || seq[1]->type_name!="OBJECT IDENTIFIER" )
      return 0;

    Sequence params = seq[1];
    return ECDSAVerifier(str->value, params->get_der());
  }
#endif

  DBG("make_verifier: Unknown algorithm identifier: %O\n", seq[0]);
}

protected mapping(int:program(Object)) x509_types = ([
    make_combined_tag(2, 1):IssuerId,
    make_combined_tag(2, 2):SubjectId,
  ]);

//! Represents a TBSCertificate.
//!
//! @note
//!   Was not compatible with @[Standards.ASN1.Types.Sequence]
//!   Prior to Pike 8.0.
class TBSCertificate
{
  inherit Sequence;

  protected string internal_der;

  //!
  void `der=(string asn1)
  {
    internal_der = UNDEFINED;
    if (init(Standards.ASN1.Decode.simple_der_decode(asn1, x509_types))) {
      internal_der = asn1;
    }
  }
  string `der()
  {
    if (internal_der) return internal_der;
    return internal_der = der_encode();
  }

  //!
  void `version=(int v)
  {
    internal_der = UNDEFINED;
    if (v == 1) {
      if (sizeof(elements) > 6) {
	DBG("Reducing version to %d\n", v);
	elements = elements[1..6];
	issuer_pos = subject_pos = extensions_pos = 0;
	internal_extensions = ([]);
	internal_critical = (<>);
      }
    } else if (sizeof(elements) == 6) {
      DBG("Bumping version to %d\n", v);
      elements = ({ version_integer(Integer(v-1)) }) + elements;
    } else {
      if ((v < 3) && extensions_pos) {
	DBG("Reducing version to %d\n", v);
	elements = elements[..extensions_pos-1];
	extensions_pos = 0;
	internal_extensions = ([]);
	internal_critical = (<>);
      } else {
	DBG("Bumping version to %d\n", v);
      }
      elements[0] = version_integer(Integer(v-1));
    }
  }
  int `version()
  {
    if (sizeof(elements) == 6) return 1;
    return (int)elements[0][0]->value + 1;
  }

  //! @param index
  //!   Index in a v1 certificate.
  //!
  //! @param val
  //!   New value for index.
  protected void low_set(int index, Sequence|Integer val)
  {
    internal_der = UNDEFINED;
    if (sizeof(elements) > 6) index++;
    elements[index] = val;
  }

  protected Sequence|Integer low_get(int index)
  {
    if (sizeof(elements) > 6) index++;
    return elements[index];
  }

  //!
  void `serial=(Gmp.mpz|int s)
  {
    low_set(0, Integer(s));
  }
  Gmp.mpz `serial()
  {
    Integer s = low_get(0);
    return s->value;
  }

  //! Algorithm Identifier.
  void `algorithm=(Sequence a)
  {
    low_set(1, a);
  }
  Sequence `algorithm()
  {
    return low_get(1);
  }

  //! Certificate issuer.
  void `issuer=(Sequence i)
  {
    low_set(2, i);
  }
  Sequence `issuer()
  {
    return low_get(2);
  }

  //!
  void `validity=(Sequence v)
  {
    // FIXME: Validate?
    low_set(3, v);
  }
  Sequence `validity()
  {
    return low_get(3);
  }

  //!
  void `not_before=(int t)
  {
    Sequence validity = low_get(3);
    validity->elements[0] = UTC()->set_posix(t);
    internal_der = UNDEFINED;
  }
  int `not_before()
  {
    Sequence validity = low_get(3);
    return validity[0]->get_posix();
  }

  //!
  void `not_after=(int t)
  {
    Sequence validity = low_get(3);
    validity->elements[1] = UTC()->set_posix(t);
    internal_der = UNDEFINED;
  }
  int `not_after()
  {
    Sequence validity = low_get(3);
    return validity[1]->get_posix();
  }

  //!
  void `subject=(Sequence s)
  {
    low_set(4, s);
  }
  Sequence `subject()
  {
    return low_get(4);
  }

  protected Verifier internal_public_key;

  //!
  void `keyinfo=(Sequence ki)
  {
    internal_public_key = make_verifier(ki);
    low_set(5, ki);
  }
  Sequence `keyinfo()
  {
    return low_get(5);
  }

  //!
  void `public_key=(Verifier v)
  {
    internal_public_key = v;
    low_set(5, v->pkc->pkcs_public_key());
  }
  Verifier `public_key()
  {
    return internal_public_key;
  }

  /* Optional */

  protected int issuer_pos;
  protected int subject_pos;
  protected int extensions_pos;

  //! @note
  //! optional
  void `issuer_id=(IssuerId|BitString i)
  {
    internal_der = UNDEFINED;
    if (!i) {
      if (!issuer_pos) return;
      elements = elements[..6] + elements[8..];
      issuer_pos = 0;
      if (subject_pos) subject_pos--;
      if (extensions_pos) extensions_pos--;
      return;
    }
    if (i->cls != 2) {
      // Convert BitString to IssuerId.
      i = IssuerId(i->value);
    } else if (i->raw) {
      // Convert Primitive to IssuerId.
      (i = IssuerId())->decode_primitive(i->raw);
    }
    if (!issuer_pos) {
      if (version < 2) version = 2;
      issuer_pos = 7;
      elements = elements[..6] + ({ i }) + elements[7..];
      if (subject_pos) subject_pos++;
      if (extensions_pos) extensions_pos++;
      return;
    }
    elements[issuer_pos] = i;
  }
  IssuerId `issuer_id()
  {
    if (issuer_pos) return elements[issuer_pos];
    return UNDEFINED;
  }

  //! @note
  //! optional
  void `subject_id=(SubjectId|BitString s)
  {
    internal_der = UNDEFINED;
    if (!s) {
      if (!subject_pos) return;
      elements = elements[..subject_pos -1] + elements[subject_pos+1..];
      subject_pos = 0;
      if (extensions_pos) extensions_pos--;
      return;
    }
    if (s->cls != 2) {
      // Convert BitString to SubjectId.
      s = SubjectId()->decode_primitive(s->raw);
    } else if (s->raw) {
      // Convert Primitive to SubjectId.
      (s = SubjectId())->decode_primitive(s->raw);
    }
    if (!subject_pos) {
      if (version < 2) version = 2;
      subject_pos = (issuer_pos || 6) + 1;
      elements = elements[..subject_pos-1] + ({ s }) +
	elements[subject_pos..];
      if (extensions_pos) extensions_pos++;
      return;
    }
    elements[subject_pos] = s;
  }
  SubjectId `subject_id()
  {
    if (subject_pos) return elements[subject_pos];
    return UNDEFINED;
  }

  //! The raw ASN.1 objects from which @[extensions] and @[critical]
  //! have been generated.
  //!
  //! @note
  //! optional
  void `raw_extensions=(Sequence r)
  {
    internal_der = UNDEFINED;
    internal_extensions = ([]);
    internal_critical = (<>);
    mapping(string:Object) extensions = ([]);
    multiset critical = (<>);

    if (!r) {
      if (!extensions_pos) return;
      elements = elements[..extensions_pos-1];
      extensions_pos = 0;
      return;
    }

    foreach(r->elements, Object _ext)
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

    if (!extensions_pos) {
      if (version < 3) version = 3;
      extensions_pos = sizeof(elements);
      elements = elements + ({ TaggedType3(r) });
    } else {
      elements[extensions_pos] = TaggedType3(r);
    }

    internal_extensions = extensions;
    internal_critical = critical;
  }
  Sequence `raw_extensions()
  {
    if (extensions_pos) return elements[extensions_pos][0];
    return UNDEFINED;
  }

  //! @note
  //! optional
  protected mapping(string:Object) internal_extensions = ([]);
  mapping(string:Object) `extensions()
  {
    return internal_extensions;
  }

  //! @note
  //! optional
  protected multiset internal_critical = (<>);
  multiset `critical()
  {
    return internal_critical;
  }

  protected void create(Sequence|void asn1)
  {
    // Initialize to defaults.
    elements = ({
      Integer(0),					// serialNumber
      Null,						// signature
      Sequence(({})),					// issuer
      Sequence(({ UTC()->set_posix(-0x8000000),
		  UTC()->set_posix(0x7fffffff) })),	// validity
      Sequence(({})),					// subject
      Null,						// subjectPublicKeyInfo
    });

    if (asn1) {
      if (!init(asn1)) {
	error("Invalid ASN.1 structure for a TBSCertificate.\n");
      }
    }
  }

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
  this_program init(array(Object)|Object asn1)
  {
    array(Object) a;
    if (objectp(asn1)) {
      if (asn1->type_name != "SEQUENCE")
	return 0;

      a = ([object(Sequence)]asn1)->elements;
    } else {
      a = [array(Object)]asn1;
    }
    DBG("TBSCertificate: sizeof(a) = %d\n", sizeof(a));
      
    if (sizeof(a) < 6)
      return 0;

    int version = 1;

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
    }
    DBG("TBSCertificate: version = %d\n", version);

    this_program::version = version;

    if (a[0]->type_name != "INTEGER")
      return 0;
    serial = a[0]->value;
    if(serial<0)
      return 0;
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

    if (catch {
	not_before = validity[0]->get_posix();
      })
      return 0;
    DBG("TBSCertificate: not_before = %O\n", not_before);

    if (catch {
	not_after = validity[1]->get_posix();
      })
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

    if (version >= 2)
    {
      if ((i < sizeof(a)) && !a[i]->constructed &&
	  (a[i]->get_combined_tag() == make_combined_tag(2, 1)))
      {
	if (a[i]->raw) {
	  (issuer_id = IssuerId())->decode_primitive(a[i]->raw);
	} else {
	  issuer_id = a[i];
	}
	DBG("TBSCertificate: issuer_id = %O\n", issuer_id);
	i++;
      }
      if ((i < sizeof(a)) && !a[i]->constructed &&
	  (a[i]->get_combined_tag() == make_combined_tag(2, 2)))
      {
	if (a[i]->raw) {
	  (subject_id = SubjectId())->decode_primitive(a[i]->raw);
	} else {
	  subject_id = a[i];
	}
	DBG("TBSCertificate: subject_id = %O\n", subject_id);
	i++;
      }
    }
    if (version >= 3) {
      if ((i < sizeof(a)) && a[i]->constructed &&
	  (a[i]->combined_tag == make_combined_tag(2, 3)) &&
	  sizeof(a[i])==1 &&
	  a[i][0]->type_name == "SEQUENCE") {
	raw_extensions = a[i][0];
	i++;
      }
    }
    internal_der = asn1->get_der();
    if (i == sizeof(a))
      return this;
    /* Too many fields */
    return 0;
  }
}

//! Creates the ASN.1 TBSCertificate sequence (see RFC2459 section
//! 4.1) to be signed (TBS) by the CA. version is explicitly set to
//! v3, and @[extensions] is optionally added to the sequence.
//! issuerUniqueID and subjectUniqueID are not supported.
TBSCertificate make_tbs(Sequence issuer, Sequence algorithm,
			Sequence subject, Sequence keyinfo,
			Integer serial, Sequence validity,
			array|int(0..0)|void extensions)
{
  TBSCertificate tbs = TBSCertificate();
  tbs->serial = serial->value;
  tbs->algorithm = algorithm;
  tbs->issuer = issuer;
  tbs->validity = validity;
  tbs->subject = subject;
  tbs->keyinfo = keyinfo;
  tbs->raw_extensions = extensions && Sequence(extensions);
  return tbs;
}

//! Creates the ASN.1 TBSCertificate sequence (see RFC2459 section
//! 4.1) to be signed (TBS) by the CA. version is explicitly set to
//! v3, validity is calculated based on time and @[ttl], and
//! @[extensions] is optionally added to the sequence.
//! issuerUniqueID and subjectUniqueID are not supported.
//!
//! @note
//!   Prior to Pike 8.0 this function returned a plain @[Sequence] object.
variant TBSCertificate make_tbs(Sequence issuer, Sequence algorithm,
				Sequence subject, Sequence keyinfo,
				Integer serial, int ttl,
				array|int(0..0)|void extensions)
{
  int now = time();
  Sequence validity = Sequence( ({ UTC()->set_posix(now),
                                   UTC()->set_posix(now + ttl) }) );

  return make_tbs(issuer, algorithm, subject, keyinfo,
		  serial, validity, extensions);
}

//! Sign the provided TBSCertificate.
//!
//! @param tbs
//!   A @[TBSCertificate] as returned by @[decode_certificate()]
//!   or @[make_tbs()].
//!
//! @param sign
//!   RSA, DSA or ECDSA parameters for the issuer.
//!   See @[Crypto.RSA], @[Crypto.DSA] and @[Crypto.ECC.Curve.ECDSA].
//!   Must be initialized with the private key.
//!
//! @param hash
//!   The hash function to use for the certificate. Must be one of the
//!   standardized PKCS hashes to be used with the given Crypto.
//!
//! @seealso
//!   @[decode_certificate()], @[make_tbs()]
Sequence sign_tbs(TBSCertificate tbs,
		  Crypto.Sign sign, Crypto.Hash hash)
{
  return Sequence(({ [object(Sequence)]tbs,
		     sign->pkcs_signature_algorithm_id(hash),
		     BitString(sign->pkcs_sign(tbs->get_der(), hash)),
		  }));
}

//! Low-level function for creating a self-signed certificate.
//!
//! @param issuer
//!   Distinguished name for the issuer.
//!   See @[Standards.PKCS.Certificate.build_distinguished_name].
//!
//! @param c
//!   RSA, DSA or ECDSA parameters for the issuer.
//!   Both the public and the private keys need to be set.
//!   See @[Crypto.RSA], @[Crypto.DSA] and @[Crypto.ECC.Curve.ECDSA].
//!
//! @param h
//!   The hash function to use for the certificate. Must be one of the
//!   standardized PKCS hashes to be used with the given Crypto.
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
//!
//! @seealso
//!   @[make_selfsigned_certificate()], @[make_tbs()], @[sign_tbs()]
string sign_key(Sequence issuer, Crypto.Sign c, Crypto.Hash h,
                Sequence subject, int serial, int ttl, array|void extensions)
{
  Sequence algorithm_id = c->pkcs_signature_algorithm_id(h);
  if(!algorithm_id) error("Can't use %O for %O.\n", h, c);
  if(serial<0) error("Serial number needs to be >=0.\n");
  return sign_tbs(make_tbs(issuer, algorithm_id,
			   subject, c->pkcs_public_key(),
			   Integer(serial), ttl, extensions),
		  c, h)->get_der();
}

//! Creates a certificate extension with the @[id] as identifier and
//! @[ext] as the extension payload. If the @[critical] flag is set
//! the extension will be marked as critical.
Sequence make_extension(Identifier id, Object ext, void|int critical)
{
  array seq = ({ id });
  if( critical )
    seq += ({ Boolean(1) });
  return Sequence( seq+({ OctetString(ext->get_der()) }) );
}

//! Creates a selfsigned certificate, i.e. where issuer and subject
//! are the same entity. This entity is derived from the list of pairs
//! in @[name], which is encoded into an distinguished_name by
//! @[Standards.PKCS.Certificate.build_distinguished_name].
//!
//! @param c
//!   The public key cipher used for the certificate, @[Crypto.RSA],
//!   @[Crypto.DSA] or @[Crypto.ECC.Curve.ECDSA]. The object should be
//!   initialized with both public and private keys.
//!
//! @param ttl
//!   The validity of the certificate, in seconds, starting from
//!   creation date.
//!
//! @param name
//!   List of properties to create distinguished name from.
//!
//! @param extensions
//!   List of extensions as ASN.1 structures. The extensions
//!   subjectKeyIdentifier, keyUsage (flagged critical) and
//!   basicConstraints (flagged critical) will automatically be added.
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
//!
//! @seealso
//!   @[sign_key()], @[sign_tbs()]
string make_selfsigned_certificate(Crypto.Sign c, int ttl,
                                   mapping|array name, array|void extensions,
                                   void|Crypto.Hash h, void|int serial)
{
  if(!serial)
    serial = (int)Gmp.mpz(Standards.UUID.make_version1(-1)->encode(), 256);

  Sequence dn = Certificate.build_distinguished_name(name);

  // Extensions mandated for Suite B Self-Signed CA Certificates, RFC
  // 5759 4.5.1.
#define ADD(X,Y,Z) extensions+=({ make_extension(Identifiers.ce_ids->X,Y,Z) })

  if(!extensions) extensions = ({});

  // While RFC 3280 section 4.2.1.2 suggest to only hash the BIT
  // STRING part of the subjectPublicKey, it is only a suggestion.
  ADD(subjectKeyIdentifier,
      OctetString( Crypto.SHA1.hash(c->pkcs_public_key()->get_der()) ),
      0);
  ADD(keyUsage,
      BitString(Gmp.mpz(keyCertSign|cRLSign|digitalSignature)->digits(256)),
      1);
  ADD(basicConstraints,
      Sequence(({Boolean(1)})),
      1);

#undef ADD

  return sign_key(dn, c, h||Crypto.SHA256, dn, serial, ttl, extensions);
}

//! Decodes a certificate and verifies that it is structually sound.
//! Returns a @[TBSCertificate] object if ok, otherwise @expr{0@}.
TBSCertificate decode_certificate(string|object cert)
{
  if (stringp (cert)) {
    cert = Standards.ASN1.Decode.simple_der_decode(cert, x509_types);
  }

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
TBSCertificate verify_certificate(string s,
				  mapping(string:Verifier|array(Verifier)) authorities)
{
  object cert = Standards.ASN1.Decode.simple_der_decode(s);

  TBSCertificate tbs = decode_certificate(cert);
  if (!tbs) return 0;

  array(Verifier)|Verifier verifiers;
  
  if (tbs->issuer->get_der() == tbs->subject->get_der())
  {
    DBG("Self signed certificate: %O\n", tbs->public_key);
    verifiers = ({ tbs->public_key });
  }
  else {
    verifiers = authorities[tbs->issuer->get_der()];
    if (objectp(verifiers)) verifiers = ({ verifiers });
  }

  foreach(verifiers || ({}), Verifier v) {
    if (v->verify(cert[1], cert[0]->get_der(), cert[2]->value))
      return tbs;
  }
  return 0;
}


//! Verifies that all extensions mandated for certificate signing
//! certificates are present and valid.
TBSCertificate verify_root_certificate(TBSCertificate tbs)
{
  if(!tbs) return 0;

  multiset crit = tbs->critical + (<>);
  int self_signed = (tbs->issuer->get_der() == tbs->subject->get_der());

  Object lookup(string id)
  {
    id = Identifiers.ce_ids[id]->get_der();
    crit[id]=0;
    return tbs->extensions[id];
  };

  // FIXME: Move extension parsing into tbs.

  // id-ce-basicConstraints is required for certificates with public
  // key used to validate certificate signatures. RFC 3280, 4.2.1.10.
  Object c = lookup("basicConstraints");
  if( !c || c->type_name!="SEQUENCE" || sizeof(c)<1 || sizeof(c)>2 ||
      c[0]->type_name!="BOOLEAN" ||
      !c[0]->value )
  {
    DBG("verify root: Bad or missing id-ce-basicConstraints.\n");
    return 0;
  }
  Sequence s = [object(Sequence)]c;
  if( sizeof(s)==2 && ( s[1]->type_name!="INTEGER" || 0 > s[1]->value ) )
  {
    // FIXME: Compare with actual path length and not 0.
    DBG("verify root: Path length longer than pathLenConstraint.\n");
    return 0;
  }

  // id-ce-authorityKeyIdentifier is required, unless self signed. RFC
  // 3280 4.2.1.1
  if( !lookup("authorityKeyIdentifier") && !self_signed )
  {
    DBG("verify root: Missing id-ce-authorityKeyIdentifier.\n");
    return 0;
  }

  // id-ce-subjectKeyIdentifier is required. RFC 3280 4.2.1.2
  if( !lookup("subjectKeyIdentifier") )
  {
    DBG("verify root: Missing id-ce-subjectKeyIdentifier.\n");
    return 0;
  }

  // id-ce-keyUsage is required. RFC 3280 4.2.1.3
  c = lookup("keyUsage");
  if( !c || c->type_name!="BIT STRING" )
  {
    DBG("verify root: Missing id-ce-keyUsage.\n");
    return 0;
  }
  keyUsage usage = (int)c;
  if( !( usage & digitalSignature ) )
  {
    DBG("verify root: id-ce-keyUsage doesn't allow digitalSignature.\n");
    return 0;
  }
  if( !( usage & keyCertSign ) ) // RFC 5759
  {
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

//! Convenience function for loading known root certificates.
//!
//! @param root_cert_dirs
//!   Directory/directories containing the PEM-encoded root certificates
//!   to load. Defaults to a rather long list of directories, including
//!   @expr{"/etc/ssl/certs"@}, @expr{"/etc/pki/tls/certs"@} and
//!   @expr{"/System/Library/OpenSSL/certs"@}, which seem to be the most
//!   common locations.
//!
//! @returns
//!   Returns a mapping from DER-encoded issuer to @[Verifier]s
//!   compatible with eg @[verify_certificate()]
//!
//! @note
//!   If a certificate directory contains a file named
//!   @expr{"ca-certificates.crt"@}, it is assumed to
//!   contain a concatenation of all the certificates
//!   in the directory.
//!
//! @seealso
//!   @[verify_certificate()], @[verify_certificate_chain()]
mapping(string:array(Verifier)) load_authorities(string|array(string)|void root_cert_dirs)
{
  root_cert_dirs = root_cert_dirs || ({
    // These directories have with some minor modifications
    // been taken from the list at
    // http://gagravarr.org/writing/openssl-certs/others.shtml

    "/etc/ssl/certs",
    // This seems to be the default for most current installations
    // with the exception of RHEL and MacOS X.
    //
    // Debian Woody (3.0), OpenSSL 0.9.6
    // Debian Sarge (3.1), OpenSSL 0.9.7
    // Debian Etch (4.0), OpenSSL 0.9.8
    // Debian Lenny (5.0), OpenSSL 0.9.8
    // Debian Squeeze (6.0), OpenSSL 0.9.8o
    // FreeBSD, OpenSSL 0.9.8
    // Gentoo, OpenSSL 0.9.7
    // Nokia N900 Maemo 5, OpenSSL 0.9.8n
    // OpenBSD, OpenSSL 0.9.x
    // Slackware, OpenSSL 0.9.6
    // SuSE 8.1 / 8.2, OpenSSL 0.9.6
    // Ubuntu Maverick (10.10), OpenSSL 0.9.8o
    // Ubuntu Precise (12.04), OpenSSL 1.0.1

    "/etc/pki/tls/certs",
    // Redhat Enterprise 6, OpenSSL 1.0.0
    // Redhat Fedora Core 4, OpenSSL 0.9.7
    // Redhat Fedora Core 5 / 6, OpenSSL 0.9.8

    "/System/Library/OpenSSL/certs",
    // Mac OS X 10.1.2, OpenSSL 0.9.6b

    "/etc/openssl/certs",
    // NetBSD, OpenSSL 0.9.x

    // From this point on the operation systems start getting
    // a bit old.

    "/usr/share/ssl/certs",
    // Centos 3 / 4, OpenSSL 0.9.7
    // Redhat 6.2 / 7.x / 8.0 / 9, OpenSSL 0.9.6
    // Redhat Enterprise 3 / 4, OpenSSL 0.9.7
    // Redhat Fedora Core 2 / 3, OpenSSL 0.9.7
    // SuSE 7.3 / 8.0, OpenSSL 0.9.6

    "/usr/lib/ssl/certs",
    // Gentoo, OpenSSL 0.9.6
    // Mandrake 7.1 -> 8.2, OpenSSL 0.9.6

    "/var/ssl/certs",
    // AIX, OpenSSL 0.9.6 (from OpenSSH support packages)

    "/usr/ssl/certs",
    // Cygwin, OpenSSL 0.9.6

    "/usr/local/openssl/certs",
    // FreeBSD, OpenSSL 0.9.x (custom complile)

    "/usr/local/ssl/certs",
    // Normal OpenSSL Tarball Build, OpenSSL 0.9.6

    "/opt/local/ssl/certs",
    // Common alternative to /usr/local/.
  });
  if (!arrayp(root_cert_dirs)) {
    root_cert_dirs = ({ root_cert_dirs });
  }
  mapping(string:array(Verifier)) res = ([]);

  foreach(root_cert_dirs, string dir) {
    string pem = Stdio.read_bytes(combine_path(dir, "ca-certificates.crt"));
    if (pem) {
      Standards.PEM.Messages messages = Standards.PEM.Messages(pem);
      foreach(messages->fragments, string|Standards.PEM.Message m) {
	if (!objectp(m)) continue;
	TBSCertificate tbs = decode_certificate(m->body);
	if (!tbs) continue;
	res[tbs->subject->get_der()] += ({ tbs->public_key });
      }
      continue;
    }
    foreach(get_dir(dir) || ({}), string fname) {
      if (has_suffix(fname, ".0")) {
	// Skip OpenSSL hash files for now (as they are duplicates).
	continue;
      }
      fname = combine_path(dir, fname);
      if (!Stdio.is_file(fname)) continue;
      pem = Stdio.read_bytes(fname);
      if (!pem) continue;
      string cert = Standards.PEM.simple_decode(pem);
      if (!cert) continue;
      TBSCertificate tbs = decode_certificate(cert);
      if (!tbs) continue;
      res[tbs->subject->get_der()] += ({ tbs->public_key });
    }
  }
  return res;
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
				 mapping(string:Verifier|array(Verifier)) authorities,
                                 int|void require_trust)
{
  mapping m = ([ ]);
#define ERROR(X) do { \
    DBG("Error " #X "\n"); \
    m->verified=0; m->error_code=(X); m->error_cert=idx; \
    return m; \
    } while(0)

  // Decode all certificates in the chain. Leaf is first and root is
  // last.

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

  // Chain is now reversed so root is first and leaf is last.

  foreach(chain_obj; int idx; TBSCertificate tbs)
  {
    array(Verifier)|Verifier verifiers;

    if(idx == 0) // The root cert
    {
      verifiers = authorities[tbs->issuer->get_der()];

      // if we don't know the issuer of the root certificate, and we
      // require trust, we're done.
      if(!verifiers && require_trust)
        ERROR(CERT_ROOT_UNTRUSTED);

      // Is the root self signed?
      if (tbs->issuer->get_der() == tbs->subject->get_der())
      {
        DBG("Self signed certificate\n");
        m->self_signed = 1;

        // always trust our own authority first, even if it is self signed.
        if(!verifiers) 
          verifiers = ({ tbs->public_key });
      } else if (objectp(verifiers)) {
	verifiers = ({ verifiers });
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
      verifiers = ({ chain_obj[idx-1]->public_key });
    }

    int verified;
    foreach(verifiers || ({}), Verifier v) {
      if( v->verify(chain_cert[idx][1],
                    chain_cert[idx][0]->get_der(),
                    chain_cert[idx][2]->value)
          && tbs)
      {
        DBG("signature is verified..\n");
        m->verified = verified = 1;

        // if we're the root of the chain and we've verified, this is
        // the authority.
        if(idx == 0)
          m->authority = tbs->issuer;
 
        if(idx == sizeof(chain_cert)-1) m->cn = tbs->subject;
        break;
      }
    }
    if (!verified)
      ERROR(CERT_BAD_SIGNATURE);
  }
  return m;

#undef ERROR
}

#endif
