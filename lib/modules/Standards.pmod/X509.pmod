#pike __REAL_VERSION__
#require constant(Crypto.Hash)
//#pragma strict_types

//! Functions to generate and validate @rfc{2459@} style X.509 v3
//! certificates.

constant dont_dump_module = 1;

import Standards.ASN1.Types;

#ifdef X509_DEBUG
#define DBG(X ...) werror(X)
#define X509_VALIDATION_DEBUG
#else
#define DBG(X ...)
#endif

#ifdef X509_VALIDATION_DEBUG
#define NULL(X ...) (werror(X), 0)
#else
#define NULL(X ...) 0
#endif

enum CertFailure
{
  //!
  CERT_TOO_OLD = 1<<0,

  //!
  CERT_TOO_NEW = 1<<1,

  //!
  CERT_INVALID = 1<<2,

  //!
  CERT_CHAIN_BROKEN = 1<<3,

  //!
  CERT_ROOT_UNTRUSTED = 1<<4,

  //!
  CERT_BAD_SIGNATURE = 1<<5,

  //! A CA certificate is not allowed by basic constraints to sign
  //! another certificate.
  CERT_UNAUTHORIZED_CA = 1<<6,

  //! The certificate is not allowed by its key usage to sign data.
  CERT_UNAUTHORIZED_SIGNING = 1<<7,

  //! The certificate chain is longer than allowed by a certificate in
  //! the chain.
  CERT_EXCEEDED_PATH_LENGTH = 1<<8,
}


// Bit 0 is the first bit in the BitString.
enum keyUsage {
  KU_digitalSignature  = 1<<0,
  KU_nonRepudiation    = 1<<1, // contentCommitment
  KU_keyEncipherment   = 1<<2,
  KU_dataEncipherment  = 1<<3,
  KU_keyAgreement      = 1<<4,
  KU_keyCertSign       = 1<<5,
  KU_cRLSign           = 1<<6,
  KU_encipherOnly      = 1<<7,
  KU_decipherOnly      = 1<<8,
  KU_last_keyUsage     = 1<<9, // end marker
};

// Generates the reverse int for keyUsage.
protected BitString build_keyUsage(keyUsage i)
{
  string v = "";
  int pos=7, char;

  while(i)
  {
    if(i&1)
      char |= 1<<pos;
    if( --pos < 0 )
    {
      pos = 7;
      v += sprintf("%c", char);
      char = 0;
    }
    i >>= 1;
  }
  if( char )
    v += sprintf("%c", char);

  BitString b = BitString(v);
  b->unused = pos==7 ? 0 : pos+1;
  return b;
}

#pragma strict_types

//! Unique identifier for the certificate issuer.
//!
//! X.509v2 (deprecated).
class IssuerId {
  inherit BitString;
  int(0..3) cls = 2;
  int(1..) tag = 1;
}

//! Unique identifier for the certificate subject.
//!
//! X.509v2 (deprecated).
class SubjectId {
  inherit BitString;
  int(0..3) cls = 2;
  int(1..) tag = 2;
}

#pragma no_strict_types

protected {
  MetaExplicit extension_sequence = MetaExplicit(2, 3);
  MetaExplicit version_integer = MetaExplicit(2, 0);

  mapping algorithms = ([
#if constant(Crypto.MD2)
    .PKCS.Identifiers.rsa_md2_id : Crypto.MD2,
#endif
    .PKCS.Identifiers.rsa_md5_id : Crypto.MD5,
    .PKCS.Identifiers.rsa_sha1_id : Crypto.SHA1,
    .PKCS.Identifiers.rsa_sha256_id : Crypto.SHA256,
#if constant(Crypto.SHA384)
    .PKCS.Identifiers.rsa_sha384_id : Crypto.SHA384,
#endif
#if constant(Crypto.SHA512)
    .PKCS.Identifiers.rsa_sha512_id : Crypto.SHA512,
#endif

    .PKCS.Identifiers.dsa_sha_id : Crypto.SHA1,
#if constant(Crypto.SHA224)
    .PKCS.Identifiers.dsa_sha224_id : Crypto.SHA224,
#endif
    .PKCS.Identifiers.dsa_sha256_id : Crypto.SHA256,

    .PKCS.Identifiers.ecdsa_sha1_id : Crypto.SHA1,
#if constant(Crypto.SHA224)
    .PKCS.Identifiers.ecdsa_sha224_id : Crypto.SHA224,
#endif
    .PKCS.Identifiers.ecdsa_sha256_id : Crypto.SHA256,
#if constant(Crypto.SHA384)
    .PKCS.Identifiers.ecdsa_sha384_id : Crypto.SHA384,
#endif
#if constant(Crypto.SHA512)
    .PKCS.Identifiers.ecdsa_sha512_id : Crypto.SHA512,
#endif
  ]);
}

//! Returns the mapping of signature algorithm to hash algorithm
//! supported by @[Verifier] and thus @[verify_ca_certificate()],
//! @[verify_certificate()], and @[verify_certificate_chain()].
mapping(Identifier:Crypto.Hash) get_algorithms()
{
  return algorithms + ([ ]);
}

class Verifier {
  constant type = [string(7bit)](mixed)"none";
  Crypto.Sign.State pkc;

  //! Verifies the @[signature] of the certificate @[msg] using the
  //! indicated hash @[algorithm], choosing from @[verifier_algorithms].
  //!
  //! @seealso
  //!    @[get_algorithms()]
  int(0..1) verify(Sequence algorithm, string(8bit) msg,
                   string(8bit) signature,
                   mapping(Identifier:Crypto.Hash) verifier_algorithms =
		   algorithms)
  {
    DBG("Verify hash %O\n", algorithm[0]);
    Crypto.Hash hash = verifier_algorithms[algorithm[0]];
    if (!hash) return 0;
    return pkc && pkc->pkcs_verify(msg, hash, signature);
  }

  protected int(0..1) `==(mixed o)
  {
    return objectp(o) && o->pkc->?name && pkc->name()==o->pkc->name() &&
      pkc->public_key_equal(o->pkc);
  }

  protected string _sprintf(int t)
  {
    return t=='O' && sprintf("%O(%O)", this_program, pkc);
  }
}

protected class RSAVerifier
{
  inherit Verifier;

  @Pike.Annotations.Implements(Verifier);

  constant type = "rsa";

  protected void create(string key) {
    pkc = .PKCS.RSA.parse_public_key(key);
  }
}

protected class DSAVerifier
{
  inherit Verifier;

  @Pike.Annotations.Implements(Verifier);

  constant type = "dsa";

  protected void create(string key, Gmp.mpz p, Gmp.mpz q, Gmp.mpz g)
  {
    pkc = .PKCS.DSA.parse_public_key(key, p, q, g);
  }
}

#if constant(Crypto.ECC.Curve)
protected class ECDSAVerifier
{
  inherit Verifier;
  constant type = "ecdsa";

  protected void create(string(8bit) key, Identifier curve_id)
  {
    Crypto.ECC.Curve curve;
    foreach(values(Crypto.ECC), mixed c) {
      if (objectp(c) && c->pkcs_named_curve_id &&
	  (c->pkcs_named_curve_id() == curve_id)) {
	curve = [object(Crypto.ECC.Curve)]c;
	break;
      }
    }
    if(!curve) return;
    DBG("ECC Curve: %O (%O)\n", curve, curve_id);
    pkc = curve->ECDSA()->set_public_key(key);
  }
}
#endif

#if constant(Crypto.ECC.Curve25519)
protected class EdDSAVerifier
{
  inherit Verifier;
  constant type = "eddsa";

  protected void create(string(8bit) key, Identifier curve_id)
  {
    Crypto.ECC.Curve curve;
    foreach(values(Crypto.ECC), mixed c) {
      if (objectp(c) && c->pkcs_eddsa_id && (c->pkcs_eddsa_id() == curve_id)) {
	curve = [object(Crypto.ECC.Curve)]c;
	break;
      }
    }
    if(!curve) return;
    DBG("ECC Edwards Curve: %O (%O)\n", curve, curve_id);
    pkc = curve->EdDSA()->set_public_key(key);
  }
}
#endif

protected Verifier make_verifier(Object _keyinfo)
{
  if( _keyinfo->type_name != "SEQUENCE" )
    return NULL("keyinfo isn't a SEQUENCE.\n");
  Sequence keyinfo = [object(Sequence)]_keyinfo;

  if ( (keyinfo->type_name != "SEQUENCE")
       || (sizeof(keyinfo) != 2)
       || (keyinfo[0]->type_name != "SEQUENCE")
       || !sizeof( [object(Sequence)]keyinfo[0] )
       || (keyinfo[1]->type_name != "BIT STRING")
       || keyinfo[1]->unused)
    return NULL("Illegal keyinfo ASN.1\n");
  Sequence seq = [object(Sequence)]keyinfo[0];
  String str = [object(String)]keyinfo[1];

  if(sizeof(seq)==0) return NULL("Empty keyinfo algorithm identifier.\n");

  if (seq[0]->get_der() == .PKCS.Identifiers.rsa_id->get_der())
  {
    if ( (sizeof(seq) > 2) ||
         // Strictly there should always be a Null parameter member
         // here, but there has been a lot of confusion about 1
         // element sequence vs. 2 element sequence with Null. Allow
         // both for compatibility.
         (sizeof(seq)==2 && seq[1]->get_der() != Null()->get_der()) )
      return NULL("Illegal RSA ASN.1\n");

    return RSAVerifier(str->value);
  }

  if(seq[0]->get_der() == .PKCS.Identifiers.dsa_id->get_der())
  {
    if( sizeof(seq)!=2 || seq[1]->type_name!="SEQUENCE" ||
        sizeof(seq[1])!=3 || seq[1][0]->type_name!="INTEGER" ||
        seq[1][1]->type_name!="INTEGER" || seq[1][2]->type_name!="INTEGER" )
      return NULL("Illegal DSA ASN.1\n");

    Sequence params = seq[1];
    return DSAVerifier(str->value, params[0]->value,
                       params[1]->value, params[2]->value);
  }

#if constant(Crypto.ECC.Curve)
  if(seq[0]->get_der() == .PKCS.Identifiers.ec_id->get_der())
  {
    if( sizeof(seq)!=2 || seq[1]->type_name!="OBJECT IDENTIFIER" )
      return NULL("Illegal ECDSA ASN.1\n");

    Identifier params = seq[1];
    Verifier ver = ECDSAVerifier(str->value, params);
    if(ver->pkc) return ver;
    return NULL("make_verifier: Unsupported ECDSA curve.\n");
  }
#endif

#if constant(Crypto.ECC.Curve25519)
  if((< .PKCS.Identifiers.eddsa25519_id->get_der(),
	.PKCS.Identifiers.eddsa448_id->get_der() >)[seq[0]->get_der()])
  {
    if( sizeof(seq)!=1 )
      return NULL("Illegal EdDSA ASN.1\n");

    Verifier ver = EdDSAVerifier(str->value, seq[0]);
    if(ver->pkc) return ver;
    return NULL("make_verifier: Unsupported EdDSA curve.\n");
  }
#endif

  return NULL("make_verifier: Unknown algorithm identifier: %O\n", seq[0]);
}

protected mapping(int:program(Object)) x509_types = ([
    make_combined_tag(2, 1):IssuerId,
    make_combined_tag(2, 2):SubjectId,
  ]);

//! Represents a TBSCertificate.
//!
//! @note
//!   Was not compatible with @[Standards.ASN1.Types.Sequence]
//!   prior to Pike 8.0.
class TBSCertificate
{
  inherit Sequence;

  void _decode(array(int|array(Object)) x)
  {
    ::_decode(x);
    init(this);
  }

  protected string internal_der;

  //!
  void `der=(string asn1)
  {
    internal_der = UNDEFINED;
    if (init(Standards.ASN1.Decode.secure_der_decode(asn1, x509_types))) {
      internal_der = asn1;
    }
  }
  string `der()
  {
    if (internal_der) return internal_der;
    return internal_der = build_der(get_der_content());
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

  protected Sequence low_get_sequence(int index)
  {
    return [object(Sequence)]low_get(index);
  }

  protected Integer low_get_integer(int index)
  {
    return [object(Integer)]low_get(index);
  }

  //!
  void `serial=(Gmp.mpz|int s)
  {
    low_set(0, Integer(s));
  }
  Gmp.mpz `serial()
  {
    return low_get_integer(0)->value;
  }

  //! Algorithm Identifier.
  void `algorithm=(Sequence a)
  {
    low_set(1, a);
  }
  Sequence `algorithm()
  {
    return low_get_sequence(1);
  }

  //! Algorithm hash if known and supported.
  //! Otherwise @[UNDEFINED].
  Crypto.Hash `hash()
  {
    Sequence a = low_get_sequence(1);
    if (sizeof(a) < 1) return UNDEFINED;
    return algorithms[a[0]];
  }

  //! Certificate issuer.
  void `issuer=(Sequence i)
  {
    low_set(2, i);
  }
  Sequence `issuer()
  {
    return low_get_sequence(2);
  }

  //! Return the issuer of the certificate as a human readable string.
  //! Mainly useful for debug.
  string issuer_str()
  {
    return dn_str(issuer);
  }

  //!
  void `validity=(Sequence v)
  {
    // FIXME: Validate?
    low_set(3, v);
  }
  Sequence `validity()
  {
    return low_get_sequence(3);
  }

  protected UTC time_object(int t)
  {
    if( !t )
    {
      // This is the "no well-known expiration date".
      return GeneralizedTime("99991231235959Z");
    }
    Calendar.ISO_UTC.Second s = Calendar.ISO_UTC.Second(t);
    if( s->year_no()<1950 || s->year_no()>=2050 )
      return GeneralizedTime(s);
    return UTC(s);
  }

  //!
  void `not_before=(int t)
  {
    Sequence validity = low_get_sequence(3);
    validity->elements[0] = time_object(t);
    internal_der = UNDEFINED;
  }
  int `not_before()
  {
    Sequence validity = low_get_sequence(3);
    return validity[0]->get_posix();
  }

  //!
  void `not_after=(int t)
  {
    Sequence validity = low_get_sequence(3);
    validity->elements[1] = time_object(t);
    internal_der = UNDEFINED;
  }
  int `not_after()
  {
    Sequence validity = low_get_sequence(3);
    return validity[1]->get_posix();
  }

  //!
  void `subject=(Sequence s)
  {
    low_set(4, s);
  }
  Sequence `subject()
  {
    return low_get_sequence(4);
  }

  //! Attempt to create a presentable string from the subject DER.
  string subject_str()
  {
    return dn_str(subject);
  }

  //! Try to extract a readable name from @[dn].  This is one of
  //! commonName, organizationName or organizationUnitName.  The first
  //! that is found is returned. Suitable for subjects and issuer
  //! sequences.
  string dn_str(Sequence dn)
  {
    mapping ids = ([]);
    foreach(dn->elements, Compound pair)
    {
      if(pair->type_name!="SET" || !sizeof(pair)) continue;
      pair = pair[0];
      if(pair->type_name!="SEQUENCE" || sizeof(pair)!=2)
        continue;
      if(pair[0]->type_name=="OBJECT IDENTIFIER" &&
         pair[1]->value && !ids[pair[0]])
        ids[pair[0]] = pair[1]->value;
    }

    string res =  ids[.PKCS.Identifiers.at_ids.commonName] ||
      ids[.PKCS.Identifiers.at_ids.organizationName] ||
      ids[.PKCS.Identifiers.at_ids.organizationUnitName];

    return res;
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
    return low_get_sequence(5);
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

  protected mapping extension_types = ([
    .PKCS.Identifiers.ce_ids.authorityKeyIdentifier :
    ([
      make_combined_tag(2,0) : OctetString, // keyIdentifier
      make_combined_tag(2,2) : Integer,     // certSerialNo
    ]),
    .PKCS.Identifiers.ce_ids.subjectAltName :
    ([
      make_combined_tag(2,1) : IA5String,   // rfc822Name
      make_combined_tag(2,2) : IA5String,   // dNSName
      make_combined_tag(2,6) : IA5String,   // URI
      make_combined_tag(2,7) : OctetString, // iPAddress
      make_combined_tag(2,8) : Identifier,  // registeredID
    ]),
  ]);

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
    mapping(Identifier:Object) extensions = ([]);
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
	DBG("TBSCertificate: Bad extensions ASN1.\n");
        return;
      }

      Sequence ext = [object(Sequence)]_ext;
      if( ext[0]->type_name != "OBJECT IDENTIFIER" ||
	  ext[-1]->type_name != "OCTET STRING" )
      {
	DBG("TBSCertificate: Bad extensions structure.\n");
        return;
      }

      DBG("TBSCertificate: Extension: %O\n", ext[0]);
      Identifier id = ext[0];

      if( extensions[id] )
      {
	DBG("TBSCertificate: Extension %O sent twice.\n");
        return;
      }

      extensions[ id ] =
        Standards.ASN1.Decode.secure_der_decode(ext->elements[-1]->value,
                                                extension_types[id]);
      if(sizeof(ext)==3)
      {
	if( ext[1]->type_name != "BOOLEAN" )
        {
          DBG("Illegal extension critical ASN.1\n");
          return;
        }
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
  protected mapping(Identifier:Object) internal_extensions = ([]);
  mapping(Identifier:Object) `extensions()
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
      Sequence(({ UTC(-0x8000000),
		  UTC(0x7fffffff) })),	                // validity
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
      return UNDEFINED;
      break;
    }
  }

  protected array fmt_asn1(object asn)
  {
    array i = ({});
    mapping|zero m = ([]);

    foreach(asn->elements;; object o)
    {
      o = o[0];
      string id = .PKCS.Identifiers.reverse_name_ids[o[0]] ||
        .PKCS.Identifiers.reverse_attribute_ids[o[0]] ||
      (array(string))o[0]->id*".";

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

  string fmt_extensions()
  {
    foreach(extensions; Identifier i; Object o)
      write("%O : %O\n", .PKCS.Identifiers.reverse_ce_ids[i]||i, o);
  }

  protected string _sprintf(int t)
  {
    if( t!='O' ) return UNDEFINED;
    mapping m = cast("mapping");
    catch {
      m->serial = serial;
      m->issuer = fmt_asn1(m->issuer);
      m->subject = fmt_asn1(m->subject);
      foreach(extensions; Identifier i; Object o) {
	m[.PKCS.Identifiers.reverse_ce_ids[i]||i] = fmt_asn1(o);
      }
    };
    return sprintf("%O(%O)", this_program, m);
  }

  //! Populates the object from a certificate decoded into an ASN.1
  //! Object. Returns the object on success, otherwise @expr{0@}. You
  //! probably want to call @[decode_certificate] or even
  //! @[verify_certificate].
  this_program init(array|Object asn1)
  {
    if (!objectp(asn1) || asn1->type_name != "SEQUENCE")
      return NULL("init: Argument not an Sequence object. %O\n", asn1);

    array(Object) a = ([object(Sequence)]asn1)->elements;
    if (sizeof(a) < 6)
      return NULL("init: Incorrect ASN.1. %O\n", a);

    int version = 1;

    if (sizeof(a) > 6)
    {
      /* The optional version field must be present */
      if (!a[0]->constructed
	  || (a[0]->get_combined_tag() != make_combined_tag(2, 0))
	  || (sizeof(a[0]) != 1)
	  || (a[0][0]->type_name != "INTEGER"))
	return NULL("init: Incorrect ASN.1. %O\n", a);

      version = (int) a[0][0]->value + 1;
      if ( (version < 2) || (version > 3))
	return NULL("init: Unsupported version %O.\n", version);
      a = a[1..];
    }
    this::version = version;

    if (a[0]->type_name != "INTEGER")
      return NULL("init: Illegal serial ASN.1. %O\n", a[0]);
    serial = a[0]->value;

    if ((a[1]->type_name != "SEQUENCE")
	|| !sizeof(a[1])
	|| (a[1][0]->type_name != "OBJECT IDENTIFIER"))
      return NULL("init: Illegal algorithm ASN.1. %O\n", a[1]);

    algorithm = a[1];

    // FIXME: Verify this more strictly.
    if (a[2]->type_name != "SEQUENCE")
      return NULL("init: Illegal issuer ASN.1. %O\n", a[2]);
    issuer = a[2];

    if ((a[3]->type_name != "SEQUENCE")
	|| (sizeof(a[3]) != 2))
      return NULL("init: Illegal validity ASN.1. %O\n", a[3]);
    array validity = a[3]->elements;

    if (mixed err = catch {
	not_before = validity[0]->get_posix();
      })
      return NULL("init: Failed to decode not_before. %O\n%s\n",
                  validity[0], describe_backtrace(err));

    if (mixed err = catch {
	not_after = validity[1]->get_posix();
      })
      return NULL("init: Failed to decode not_after. %O\n%s\n",
                  validity[1], describe_backtrace(err));

    // FIXME: Verify this more strictly.
    if (a[4]->type_name != "SEQUENCE")
      return NULL("init: Illegal subject ASN.1. %O\n", a[4]);
    subject = a[4];

    Verifier key = make_verifier(a[5]);
    if (!key)
      return NULL("init: Failed to decode public key. %O\n", a[5]);
    public_key = key;

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

#define EXT(X) do { \
          Object o = internal_extensions[.PKCS.Identifiers.ce_ids.##X]; \
          if(o && !parse_##X(o)) \
            DBG("TBSCertificate: Failed to parse extension %O.\n", #X); \
        } while (0)
        EXT(basicConstraints);       // 2.5.29.19
        EXT(authorityKeyIdentifier); // 2.5.29.35
        EXT(subjectKeyIdentifier);   // 2.5.29.14
        EXT(keyUsage);               // 2.5.29.15
        EXT(extKeyUsage);            // 2.5.29.37
        EXT(subjectAltName);         // 2.5.29.17
#undef EXT
      }
    }
    internal_der = asn1->get_der();
    if (i == sizeof(a))
      return this;

    return NULL("init: Too many fields. %O\n", sizeof(a));
  }

  //
  // --- Extension code
  //

  //! Set if the certificate contains a valid basicConstraints
  //! extension. @rfc{3280:4.2.1.10@}.
  int(0..1) ext_basicConstraints;

  //! If set, the certificate may be used as a CA certificate, i.e.
  //! sign other certificates.
  int(0..1) ext_basicConstraints_cA;

  //! The maximum number of certificates that may follow this
  //! certificate in a certificate chain. @expr{0@} in case no limit
  //! is imposed. Note that this variable is off by one compared to
  //! the @rfc{3280@} definition, which only counts intermediate
  //! certificates (i.e. 0 intermediates means this variable would be
  //! 1, as in one following certificate).
  int ext_basicConstraints_pathLenConstraint;

  protected int(0..1) parse_basicConstraints(Object o)
  {
    // FIXME: This extension must be critical if certificate contains
    // public keys use usage is to validate signatures on
    // certificates.

    if( o->type_name!="SEQUENCE" )
      return 0;
    Sequence s = [object(Sequence)]o;
    if( sizeof(s)==0 )
    {
      ext_basicConstraints = 1;
      ext_basicConstraints_cA = 0;
      return 1;
    }
    if( sizeof(s)>2 || s[0]->type_name!="BOOLEAN" )
      return 0;

    if( sizeof(s)==2 )
    {
      if( s[1]->type_name!="INTEGER" || s[0]->value==0 || s[1]->value<0 )
        return 0;
      ext_basicConstraints_pathLenConstraint = s[1]->value + 1;
      // FIXME: pathLenConstraint is not permitted if keyCertSign
      // isn't set in key usage. We need to check that at a higher
      // level though.
    }
    else
      ext_basicConstraints_pathLenConstraint = 0;

    ext_basicConstraints = 1;
    ext_basicConstraints_cA = s[0]->value;
    return 1;
  }

  //! Set if the certificate contains a valid authorityKeyIdentifier
  //! extension. @rfc{3280:4.2.1.1@}.
  int(0..1) ext_authorityKeyIdentifier;

  //! Set to the KeyIdentifier, if set in the extension.
  string ext_authorityKeyIdentifier_keyIdentifier;

  //! Set to the CertificateSerialNumber, if set in the extension.
  Gmp.mpz ext_authorityKeyIdentifier_authorityCertSerialNumber;

  protected int(0..1) parse_authorityKeyIdentifier(Object o)
  {
    if( o->type_name!="SEQUENCE" )
      return 0;
    Sequence s = [object(Sequence)]o;

    // Let's assume you can only have one unique identifier of each
    // kind.
    array list = filter(s->elements, lambda(Object o) { return o->cls==2; });
    if( sizeof(list) != sizeof(Array.uniq(list->tag)) )
      return 0;

    foreach(list, Object o)
    {
      switch(o->tag)
      {
      case 0:
        ext_authorityKeyIdentifier_keyIdentifier = o->value;
        break;
      case 2:
        ext_authorityKeyIdentifier_authorityCertSerialNumber = o->value;
        break;
      }}

    // FIXME: We don't parse authorityCertIssuer yet.

    ext_authorityKeyIdentifier = 1;
    return 1;
  }

  //! Set to the value of the SubjectKeyIdentifier if the certificate
  //! contains the subjectKeyIdentifier
  //! extension. @rfc{3280:4.2.1.2@}.
  string ext_subjectKeyIdentifier;

  protected int(0..1) parse_subjectKeyIdentifier(Object o)
  {
    if( o->type_name!="OCTET STRING" )
      return 0;
    ext_subjectKeyIdentifier = o->value;
    return 1;
  }

  //! Set to the value of the KeyUsage if the certificate contains the
  //! keyUsage extension. @rfc{3280:4.2.1.3@}.
  keyUsage ext_keyUsage;

  protected int(0..1) parse_keyUsage(Object o)
  {
    if( o->type_name!="BIT STRING" )
      return 0;

    int pos;
    foreach(o->value;; int char)
      for(int i; i<8; i++)
      {
        int bit = !!(char & 0x80);
        ext_keyUsage |= (bit << pos);
        pos++;
        char <<= 1;
      }

    return 1;
  }

  //! Set to the list of extended key usages from anyExtendedKeyUsage,
  //! if the certificate contains the extKeyUsage extensions. These
  //! Identifier objects are typically found in
  //! @[.PKCS.Identifiers.reverse_kp_ids]. @rfc{3280:4.2.1.13@}.
  array(Identifier) ext_extKeyUsage;

  protected int(0..1) parse_extKeyUsage(Object o)
  {
    if( o->type_name!="SEQUENCE" )
      return 0;
    Sequence s = [object(Sequence)]o;

    ext_extKeyUsage = s->elements;
    return 1;
  }

  array(string) ext_subjectAltName_rfc822Name;

  array(string) ext_subjectAltName_dNSName;

  array(string) ext_subjectAltName_uniformResourceIdentifier;

  array(string) ext_subjectAltName_iPAddress;

  array(Identifier) ext_subjectAltName_registeredID;


  protected int(0..1) parse_subjectAltName(Object o)
  {
    if( o->type_name!="SEQUENCE" )
      return 0;
    Sequence s = [object(Sequence)]o;

    foreach(s->elements, Object o)
    {
      if( o->cls!=2 ) continue;
#define CASE(X) do { if(!ext_subjectAltName_##X) ext_subjectAltName_##X=({}); \
        ext_subjectAltName_##X += ({ o->value }); } while(0)

      switch(o->tag)
      {
      case 1:
        CASE(rfc822Name);
        break;
      case 2:
        CASE(dNSName);
        break;
      case 6:
        CASE(uniformResourceIdentifier);
        break;
      case 7:
        CASE(iPAddress);
        break;
      case 8:
        CASE(registeredID);
        break;
      }
    }

    // FIXME: otherName, x400Address, directoryName and ediPartyName
    // not supported.

    return 1;
  }

}

//! Creates the ASN.1 TBSCertificate sequence (see @rfc{2459@} section
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

//! Creates the ASN.1 TBSCertificate sequence (see @rfc{2459@} section
//! 4.1) to be signed (TBS) by the CA. version is explicitly set to
//! v3, validity is calculated based on time and @[ttl], and
//! @[extensions] is optionally added to the sequence.  issuerUniqueID
//! and subjectUniqueID are not supported.
//!
//! @note
//!   Prior to Pike 8.0 this function returned a plain @[Sequence] object.
variant TBSCertificate make_tbs(Sequence issuer, Sequence algorithm,
				Sequence subject, Sequence keyinfo,
				Integer serial, int ttl,
				array|int(0..0)|void extensions)
{
  int now = time();
  Sequence validity = Sequence( ({ UTC(now),
                                   UTC(now + ttl) }) );

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
		  Crypto.Sign.State sign, Crypto.Hash hash)
{
  return Sequence(({ tbs,
		     sign->pkcs_signature_algorithm_id(hash),
		     BitString(sign->pkcs_sign(tbs->get_der(), hash)),
		  }));
}

//! Low-level function for creating a signed certificate.
//!
//! @param issuer
//!   Distinguished name for the issuer.
//!   See @[Standards.PKCS.Certificate.build_distinguished_name].
//!
//! @param c
//!   RSA, DSA or ECDSA parameters for the subject. Only the public
//!   key needs to be set. See @[Crypto.RSA], @[Crypto.DSA] and
//!   @[Crypto.ECC.Curve.ECDSA].
//!
//! @param ca
//!   RSA, DSA or ECDSA parameters for the issuer. Only the private
//!   key needs to be set. See @[Crypto.RSA], @[Crypto.DSA] and
//!   @[Crypto.ECC.Curve.ECDSA].
//!
//! @param h
//!   The hash function to use for the certificate. Must be one of the
//!   standardized PKCS hashes to be used with the given Crypto.
//!
//! @param subject
//!   Distinguished name for the subject.
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
string sign_key(Sequence issuer, Crypto.Sign.State c, Crypto.Sign.State ca,
		Crypto.Hash h, Sequence subject, int serial, int ttl,
		array|mapping|void extensions)
{
  Sequence algorithm_id = c->pkcs_signature_algorithm_id(h);
  if(!algorithm_id) error("Can't use %O for %O.\n", h, c);
  if(serial<=0) error("Conforming CA serial number needs to be >0.\n");
  if(serial>1<<142) error("Serial needs to be less than 20 bytes encoded.\n");

  if( mappingp(extensions) )
  {
    mapping(Identifier:Sequence) m = [mapping]extensions;
    array(Sequence) a = ({});
    foreach( sort(indices(m)), Identifier i )
      a += ({ m[i] });
    extensions = a;
  }

  return sign_tbs(make_tbs(issuer, algorithm_id,
			   subject, c->pkcs_public_key(),
			   Integer(serial), ttl, extensions),
		  ca, h)->get_der();
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

protected int make_key_usage_flags(Crypto.Sign.State c)
{
  int flags = KU_digitalSignature|KU_keyEncipherment;

  // ECDSA certificates can be used for ECDH exchanges, which requires
  // keyAgreement. Potentially we should make a nicer API than name
  // prefix.
  if( has_prefix(c->name(), "ECDSA") )
    flags |= KU_keyAgreement;

  return flags;
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
//!   Mapping with extensions as ASN.1 structures, as produced by
//!   @[make_extension]. The extensions subjectKeyIdentifier, keyUsage
//!   (flagged critical) and basicConstraints (flagged critical) will
//!   automatically be added if not present.
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
string make_selfsigned_certificate(Crypto.Sign.State c, int ttl,
                                   mapping|array name,
                                   mapping(Identifier:Sequence) extensions =
				   ([]),
                                   Crypto.Hash h = Crypto.SHA256,
				   int serial =
				   (int)Gmp.mpz(Standards.UUID.
						make_version1(-1)->encode(),
						256))
{
  Sequence dn = .PKCS.Certificate.build_distinguished_name(name);

  void add(string name, Object data, void|int critical)
  {
    Identifier id = .PKCS.Identifiers.ce_ids[name];
    if(!extensions[id])
      extensions[id] = make_extension(id, data, critical);
  };

  // While RFC 3280 section 4.2.1.2 suggest to only hash the BIT
  // STRING part of the subjectPublicKey, it is only a suggestion.
  add("subjectKeyIdentifier",
      OctetString( Crypto.SHA1.hash(c->pkcs_public_key()->get_der()) ));

  add("keyUsage", build_keyUsage(make_key_usage_flags(c)), 1);

  add("basicConstraints", Sequence(({})), 1);

  return sign_key(dn, c, c, h, dn, serial, ttl, extensions);
}

string make_site_certificate(TBSCertificate ca, Crypto.Sign.State ca_key,
                             Crypto.Sign.State c, int ttl, mapping|array name,
                             mapping extensions = ([]),
                             Crypto.Hash h = Crypto.SHA256,
			     int serial =
			     (int)Gmp.mpz(Standards.UUID.
					  make_version1(-1)->encode(), 256))
{
  Sequence dn = .PKCS.Certificate.build_distinguished_name(name);

  void add(string name, Object data, void|int critical)
  {
    Identifier id = .PKCS.Identifiers.ce_ids[name];
    if(!extensions[id])
      extensions[id] = make_extension(id, data, critical);
  };

  // FIXME: authorityKeyIdentifier
  add("keyUsage", build_keyUsage(make_key_usage_flags(c)), 1);

  add("basicConstraints", Sequence(({})), 1);
  return sign_key(ca->subject, c, ca_key, h, dn, serial, ttl, extensions);
}

string make_root_certificate(Crypto.Sign.State c, int ttl, mapping|array name,
			     mapping(Identifier:Sequence) extensions = ([]),
			     Crypto.Hash h = Crypto.SHA256,
			     int serial =
			     (int)Gmp.mpz(Standards.UUID.
					  make_version1(-1)->encode(), 256))
{
  Sequence dn = .PKCS.Certificate.build_distinguished_name(name);

  void add(string name, Object data, void|int critical)
  {
    Identifier id = .PKCS.Identifiers.ce_ids[name];
    if(!extensions[id])
      extensions[id] = make_extension(id, data, critical);
  };

  // While RFC 3280 section 4.2.1.2 suggest to only hash the BIT
  // STRING part of the subjectPublicKey, it is only a suggestion.
  // FIXME: authorityKeyIdentifier
  add("subjectKeyIdentifier",
      OctetString( Crypto.SHA1.hash(c->pkcs_public_key()->get_der()) ));
  add("keyUsage", build_keyUsage(KU_keyCertSign|KU_cRLSign), 1);
  add("basicConstraints", Sequence(({Boolean(1)})), 1);

  return sign_key(dn, c, c, h, dn, serial, ttl, extensions);
}

//! Decodes a certificate and verifies that it is structually sound.
//! Returns a @[TBSCertificate] object if ok, otherwise @expr{0@}.
TBSCertificate|zero decode_certificate(string|.PKCS.Signature.Signed cert)
{
  if (stringp (cert))
  {
    cert = .PKCS.Signature.decode_signed(cert, x509_types);
    if(!cert)
      return NULL("ASN.1 had trailing data.\n");
  }

  return TBSCertificate([object(.PKCS.Signature.Signed)]cert->tbs);
}

//! Decodes a certificate, checks the signature. Returns the
//! TBSCertificate structure, or 0 if decoding or verification fails.
//! The valid time range for the certificate is not checked.
//!
//! @param authorities
//!   A mapping from (DER-encoded) names to a verifiers.
//!
//! @param options
//!   @mapping
//!     @member mapping(Standards.ASN1.Types.Identifier:Crypto.Hash) "verifier_algorithms"
//!       A mapping of verifier algorithm identifier to hash algorithm
//!       implementation.
//!   @endmapping
//!
//! @note
//!   This function allows self-signed certificates, and it doesn't
//!   check that names or extensions make sense.
object(TBSCertificate)|zero verify_certificate(string s,
				  mapping(string:Verifier|array(Verifier)) authorities,
				  mapping(Standards.ASN1.Types.Identifier:Crypto.Hash)|void options)
{
  .PKCS.Signature.Signed cert = .PKCS.Signature.decode_signed(s, x509_types);

  TBSCertificate tbs = decode_certificate(cert);
  if (!tbs) return 0;

  if(tbs->algorithm->get_der() != cert->algorithm->get_der())
    return NULL("Mismatching algorithm identifiers.\n");

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
    if (v->verify(cert[1], cert[0]->get_der(), cert[2]->value,
                  options->?verifier_algorithms))
      return tbs;
  }
  return 0;
}

//! Verifies that all extensions mandated for certificate signing
//! certificates are present and valid.
object(TBSCertificate)|zero verify_ca_certificate(string|TBSCertificate tbs)
{
  if(stringp(tbs)) tbs = decode_certificate(tbs);
  if(!tbs) return 0;

  int t = time();
  if( tbs->not_after < t ) return NULL("verify ca: not after check failed\n");
  if( tbs->not_before > t ) return NULL("verify ca: not before check failed\n");

  multiset crit = tbs->critical + (<>);
  int self_signed = (tbs->issuer->get_der() == tbs->subject->get_der());

  // id-ce-basicConstraints is required for certificates with public
  // key used to validate certificate signatures.
  crit[.PKCS.Identifiers.ce_ids.basicConstraints]=0;
  if( tbs->ext_basicConstraints )
  {
    if( !tbs->ext_basicConstraints_cA )
      return NULL("vertify ca: id-ce-basicContraints-cA is false.\n");
  }
  else
    return NULL("verify ca: Bad or missing id-ce-basicConstraints.\n");

  // id-ce-authorityKeyIdentifier is required by RFC 5759, unless self
  // signed. Defined in RFC 3280 4.2.1.1, but there only as
  // recommended.
  crit[.PKCS.Identifiers.ce_ids.authorityKeyIdentifier]=0;
  if( !tbs->ext_authorityKeyIdentifier && !self_signed )
    return NULL("verify ca: Missing id-ce-authorityKeyIdentifier.\n");

  // id-ce-keyUsage is required.
  crit[.PKCS.Identifiers.ce_ids.keyUsage]=0;
  if( !(tbs->ext_keyUsage & KU_keyCertSign) )
    return NULL("verify ca: id-ce-keyUsage doesn't allow keyCertSign.\n");

  // FIXME: RFC 5759 also requires CRLSign set.
  if( tbs->ext_keyUsage &
      (~(KU_keyCertSign | KU_cRLSign | KU_digitalSignature |
         KU_nonRepudiation)&(KU_last_keyUsage-1)) )
    return NULL("verify ca: illegal CA uses in id-ce-keyUsage.\n");

  // FIXME: In addition RFC 5759 requires policyMappings,
  // policyConstraints and inhibitAnyPolicy to be processed in
  // accordance with RFC 5280.

  // One or more critical extensions have not been processed.
  if( sizeof(crit) )
    return NULL("verify ca: Critical unknown extensions %O.\n", crit);

  return tbs;
}

protected mapping(string:mapping(string:array(Verifier)))
  authorities_cache = ([]);
protected mapping(string:int)
  authorities_cache_expire = ([]);

//! Convenience function for loading known root certificates.
//!
//! @param root_cert_dirs
//!   Directory/directories containing the PEM-encoded root certificates
//!   to load. Defaults to a rather long list of directories, including
//!   @expr{"/etc/ssl/certs"@}, @expr{"/etc/pki/tls/certs"@} and
//!   @expr{"/System/Library/OpenSSL/certs"@}, which seem to be the most
//!   common locations.
//!
//! @param cache
//!   A flag to control if the answer should be given from an internal
//!   cache or always scan the directories. If a cache is used, it
//!   will refresh when any certificate expires (which typically is
//!   measured in years) or when asked for in unchached mode.
//!
//! @returns
//!   Returns a mapping from DER-encoded issuer to @[Verifier]s
//!   compatible with eg @[verify_certificate()]
//!
//! @note
//!   If a certificate directory contains a file named
//!   @expr{"ca-certificates.crt"@}, @expr{"ca-bundle.crt"@} or
//!   @expr{"ca-bundle.trust.crt"@}, it is assumed to
//!   contain a concatenation of all the certificates
//!   in the directory.
//!
//! @seealso
//!   @[verify_certificate()], @[verify_certificate_chain()]
mapping(string:array(Verifier))
  load_authorities(string|array(string)|void root_cert_dirs,
                   int(0..1)|void cache)
{
  string key = "";
  if(stringp(root_cert_dirs))
    key = root_cert_dirs;
  else if(arrayp(root_cert_dirs))
    key = root_cert_dirs*":";
  if( cache && sizeof(authorities_cache) &&
      authorities_cache_expire[key] > time() )
    return authorities_cache[key];

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
    // Redhat Enterprise 7
    // Redhat Fedora Core 4, OpenSSL 0.9.7
    // Redhat Fedora Core 5 / 6, OpenSSL 0.9.8

    "/System/Library/OpenSSL/certs",
    // Mac OS X 10.1.2, OpenSSL 0.9.6b

    "/Library/Keychains",
    // Mac OS X.

    "/System/Library/Keychains",
    // Mac OS X 11.4.2

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
  int expire;
  void update_expire(TBSCertificate tbs)
  {
    if(!expire) expire=tbs->not_after;
    expire = min(expire, tbs->not_after);
    if(tbs->not_before > time(1))
      expire = min(expire, tbs->not_before);
  };
  int(0..1) add_cert(TBSCertificate|string(8bit) cert)
  {
    TBSCertificate tbs = verify_ca_certificate(cert);
    if (!tbs) return 0;
    string subj = tbs->subject->get_der();
    if( !res[subj] || !has_value(res[subj], tbs->public_key ) )
    {
      update_expire(tbs);
      res[subj] += ({ tbs->public_key });
    }
    return 1;
  };
  int(0..1) add_pem(string(8bit) pem)
  {
    int(0..1) found;
    Standards.PEM.Messages messages = Standards.PEM.Messages(pem);
    foreach(messages->get_certificates(), string m) {
      found |= add_cert(m);
    }
    return found;
  };

  foreach(root_cert_dirs, string dir) {
    if (!Stdio.is_dir(dir)) continue;

    int found;

    // Try the merged certificate files first.
    foreach(({ "ca-certificates.crt", "ca-bundle.crt", "ca-bundle.trust.crt" }),
	    string fname) {
      string(8bit) pem;
      catch { pem = Stdio.read_bytes(combine_path(dir, fname)); };
      if (pem) {
	found |= add_pem(pem);
      }
    }
    if (found) continue;

    // Then try the Apple KeyChain files.
    foreach(({
	      // Mostly TLS Root CAs:
	      "SystemRootCertificates.keychain",

	      // Certificates for certifying identities and email,
	      // many of which are expired.
	      "SystemCACertificates.keychain",

	      // Old name for SystemRootCertificates.keychain.
	      "X509Anchors",

	      // Old name for SystemCACertificates.keychain.
	      "X509Certificates",
	    }), string fname) {
      string keychain;
      catch { keychain = Stdio.read_bytes(combine_path(dir, fname)); };
      if (keychain) {
	Apple.Keychain chain = Apple.Keychain(keychain);
	foreach(chain->certs, TBSCertificate tbs) {
	  found |= add_cert(tbs);
	}
      }
    }
    if (found) continue;

    // Fall back to trying every file.
    foreach(get_dir(dir) || ({}), string fname) {
      if (has_suffix(fname, ".0")) {
	// Skip OpenSSL hash files for now (as they are duplicates).
	continue;
      }
      fname = combine_path(dir, fname);
      if (!Stdio.is_file(fname)) continue;
      string(8bit) pem;
      catch { pem = Stdio.read_bytes(fname); };
      if (!pem) continue;
      add_pem(pem);
    }
  }

  if( authorities_cache_expire[key] || cache )
  {
    authorities_cache[key] = res;
    authorities_cache_expire[key] = expire;
  }
  return res;
}

//! Read and decode certificate chain for the given @[host] from the
//! letsencrypt directory (/etc/letsencrypt/live/{host}/). Note that
//! the process has to have read privileges for the directory.
//!
//! The resulting array can be used directly as input to the
//! @[SSL.Context()->add_cert()] method.
//!
//! @returns
//!   @array
//!     @elem Crypto.Sign.State 0
//!       A signing object using the private key.
//!     @elem array(string) 1
//!       An array of DER encoded certificates representing the
//!       certificate chain.
//!   @endarray
//!
array get_letsencrypt_cert(string host)
{
  if( has_value(host, "/") ) {
    error("Host %O contains '/'.", host);
  }

  string cert, der;
  cert = Stdio.read_file("/etc/letsencrypt/live/"+host+"/fullchain.pem");
  der = Stdio.read_file("/etc/letsencrypt/live/"+host+"/privkey.pem");

  if( !cert ) {
    error("Could not load certificate chain file.\n");
  }
  if( !der ) {
    error("Could not load private key file.\n");
  }

  object certs = Standards.PEM.Messages(cert);
  der = Standards.PEM.simple_decode(der);
  object asn = Standards.ASN1.Decode.simple_der_decode(der);
  Crypto.RSA rsa = Standards.PKCS.parse_private_key(asn);
  return ({ rsa, certs->get_certificates() });
}


//! Decodes a certificate chain, ordered from leaf to root, and
//! checks the signatures. Verifies that the chain can be decoded
//! correctly, is unbroken, and that all certificates are in effect
//! (time-wise.) and allowed to sign its child certificate.
//!
//! No verifications are done on the leaf certificate to determine
//! what it can and can not be used for.
//!
//! Returns a mapping with the following contents, depending
//! on the verification of the certificate chain:
//!
//! @mapping
//!   @member int "error_code"
//!     Error describing type of verification failures, if
//!     verification failed. May be one of the following, OR:ed
//!     together: @[CERT_TOO_NEW], @[CERT_TOO_OLD],
//!     @[CERT_ROOT_UNTRUSTED], @[CERT_BAD_SIGNATURE],
//!     @[CERT_INVALID], @[CERT_CHAIN_BROKEN], @[CERT_UNAUTHORIZED_CA]
//!     or @[CERT_EXCEEDED_PATH_LENGTH].
//!   @member int "error_cert"
//!     Index number of the certificate that caused the verification failure.
//!   @member int(0..1) "self_signed"
//!     Non-zero if the certificate is self-signed.
//!   @member int(0..1) "verified"
//!     Non-zero if the certificate is verified.
//!   @member Standards.ASN1.Sequence "authority"
//!     The authority RDN that verified the chain.
//!   @member Standards.ASN1.Sequence "cn"
//!     The common name RDN of the leaf certificate.
//!   @member array(TBSCertificate) "certificates"
//!     An array with the decoded certificates, ordered from root to leaf.
//! @endmapping
//!
//! @param cert_chain
//!   An array of certificates, with the relative-root last. Each
//!   certificate should be a DER-encoded certificate, or decoded as a
//!   @[Standards.PKCS.Signature.Signed] object.
//! @param authorities
//!   A mapping from (DER-encoded) names to verifiers.
//! @param require_trust
//!   Require that the certificate be traced to an authority, even if
//!   it is self signed.
//! @param strict
//!   By default this function only requires that the certificates are
//!   in order, it ignores extra certificates we didn't need to verify
//!   the leaf certificate.
//!
//!   If you specify @[strict], this will change, each certificate has
//!   to be signed by the next in the chain.
//!
//!   Some https-servers send extraneous intermediate certificates
//!   that aren't used to validate the leaf certificate. So strict
//!   mode will be incompatible with such srevers.
//! @param options
//!   @mapping
//!     @member mapping(Standards.ASN1.Types.Identifier:Crypto.Hash) "verifier_algorithms"
//!       A mapping of verifier algorithm identifier to hash algorithm
//!       implementation.
//!     @member int "strict"
//!       See @[strict] above.
//!   @endmapping
//!
//! @seealso
//!   @[get_algorithms()]
//!
//! See @[Standards.PKCS.Certificate.get_dn_string] for converting the
//! RDN to an X500 style string.
mapping verify_certificate_chain(array(string|.PKCS.Signature.Signed) cert_chain,
				 mapping(string:Verifier|array(Verifier)) authorities,
                                 int|void require_trust,
                                 mapping(string:mixed)|bool|void options)
{
  mapping m = ([ ]);
  int strict = mappingp(options) ? options->strict : options;

#define ERROR(X) do {                                     \
    DBG("Error " #X "\n");                                \
    m->verified=0; m->error_code|=(X); m->error_cert=idx; \
    } while(0)
#define FATAL(X) do { ERROR(X); return m; } while(0)

  // Decode all certificates in the chain. Leaf is first and root is
  // last.

  int len = sizeof(cert_chain);
  array chain_obj = ({});
  array chain_cert = ({});

  string last_issuer;
  foreach(cert_chain; int idx; string|.PKCS.Signature.Signed c)
  {
     .PKCS.Signature.Signed cert;
     cert = .PKCS.Signature.decode_signed(c, x509_types);
     TBSCertificate tbs = decode_certificate(cert);
     if(!tbs)
        FATAL(CERT_INVALID);

     string subject = tbs->subject->get_der();
     if (!strict && last_issuer && subject != last_issuer)
     {
        // This doesn't look relevant for validating the previous (closer to
        // the leaf) certificate.
        DBG("Skipping %q\n", tbs->subject_str());
        continue;
     }
     DBG("Adding %q to chain\n", tbs->subject_str());

     // This is the leaf or is needed to validate the previous certificate.
     last_issuer = tbs->issuer->get_der();
     chain_cert = ({ cert }) + chain_cert;
     chain_obj = ({ tbs }) + chain_obj;

     if (!strict && has_index(authorities, last_issuer))
     {
        // We've reached a certificate signed by a root we trust, end here.
        DBG("Ending at %q - signed by root %q\n", tbs->subject_str(), tbs->issuer_str());
        break;
     }
  }

  // Update length since we've filtered the certificate chain a bit.
  len = sizeof(chain_obj);
  m->certificates = chain_obj;

  // Chain is now reversed so root is first and leaf is last.

  int my_time = time();
  foreach(chain_obj; int idx; TBSCertificate tbs)
  {
    array(Verifier)|Verifier verifiers;

    // Check not_before. We want the current time to be later.
    if(my_time < tbs->not_before)
      ERROR(CERT_TOO_NEW);

    // Check not_after. We want the current time to be earlier.
    if(my_time > tbs->not_after)
      ERROR(CERT_TOO_OLD);

    if(idx != len-1) // Not the leaf
    {
      // id-ce-basicConstraints is required for certificates with
      // public key used to validate certificate signatures.

      if( !tbs->ext_basicConstraints )
        ERROR(CERT_INVALID);

      if( !tbs->ext_basicConstraints_cA )
        ERROR(CERT_UNAUTHORIZED_CA);

      if( tbs->ext_basicConstraints_pathLenConstraint )
      {
        // len-1-idx is the number of following certificates.
        if( len-1-idx > tbs->ext_basicConstraints_pathLenConstraint )
        {
          // The error was later in the chain though, so maybe a
          // different error should be sent.
          ERROR(CERT_EXCEEDED_PATH_LENGTH);
        }
      }

      if( !(tbs->ext_keyUsage & KU_keyCertSign) )
        ERROR(CERT_UNAUTHORIZED_CA);
    }

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
      }

      if (objectp(verifiers))
	verifiers = ({ verifiers });
    }

    else // otherwise, we make sure the chain is unbroken.
    {
      // FIXME: We should use identifiers instead of issuer/subject.

      // is the issuer of this certificate the subject of the previous
      // (more rootward) certificate?
      if(tbs->issuer->get_der() != chain_obj[idx-1]->subject->get_der())
        ERROR(CERT_CHAIN_BROKEN);

      // the verifier for this certificate should be the public key of
      // the previous certificate in the chain.
      verifiers = ({ chain_obj[idx-1]->public_key });
    }

    m->verified = 0;
    int verified;
    foreach(verifiers || ({}), Verifier v) {
      if( v->verify(chain_cert[idx][1],
                    chain_cert[idx][0]->get_der(),
                    chain_cert[idx][2]->value,
                    mappingp(options) && options->verifier_algorithms)
          && tbs)
      {
        DBG("signature is verified..\n");
        verified = 1;

        // if we're the root of the chain and we've verified, this is
        // the authority.
        if(idx == 0)
          m->authority = tbs->issuer;

        if(idx == sizeof(chain_cert)-1) m->cn = tbs->subject;
        break;
      }
    }
    if (!verified)
    {
      ERROR(CERT_BAD_SIGNATURE);
      return m;
    }
    else if (zero_type(m->error_code)) // only if no error occured...
      m->verified = verified;
  }
  return m;

#undef ERROR
#undef FATAL
}

//! DWIM-parse the ASN.1-sequence for a private key.
Crypto.Sign.State parse_private_key(Sequence seq)
{
  switch(sizeof(seq)) {
  case 5:
    return Standards.PKCS.DSA.parse_private_key(seq);
  case 9:
    return Standards.PKCS.RSA.parse_private_key(seq);
  case 2:
    // ECDSA, implicit curve. Not supported yet.
    return UNDEFINED;
  case 3:
  case 4:
    // Either PKCS#8 or ECDSA.
    Crypto.Sign.State res = Standards.PKCS.parse_private_key(seq);
    if (res) return res;
#if constant(Nettle.ECC_Curve)
    // EDCSA
    return Standards.PKCS.ECDSA.parse_private_key(seq);
#endif
  }
  return UNDEFINED;
}

//! DWIM-parse the DER-sequence for a private key.
variant Crypto.Sign.State parse_private_key(string(8bit) private_key)
{
  Object seq = Standards.ASN1.Decode.secure_der_decode(private_key);
  if (!seq || (seq->type_name != "SEQUENCE")) return UNDEFINED;
  return parse_private_key([object(Sequence)]seq);
}
