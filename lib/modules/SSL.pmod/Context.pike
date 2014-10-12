#pike __REAL_VERSION__
#pragma strict_types
#require constant(Crypto.Hash)

//! Keeps the state that is shared by all SSL-connections on a client,
//! or for one port on a server. It includes policy configuration,
//! the server or client certificate(s), the corresponding private key(s),
//! etc. It also includes the session cache.
//!
//! The defaults are usually suitable for a client, but for a server
//! some configuration is necessary.
//!
//! Typical use is to:
//! @ul
//!   @item
//!     Call @[add_cert()] with the certificates belonging to the server
//!     or client. Note that clients often don't have or need any
//!     certificates, and also that certificate-less server operation is
//!     possible, albeit discouraged and not enabled by default.
//!
//!     Suitable self-signed certificates can be created with
//!     @[Standards.X509.make_selfsigned_certificate()].
//!   @item
//!     Optionally call @[get_suites()] to get a set of cipher_suites
//!     to assign to @[preferred_suites]. This is only needed if the
//!     default set of suites from @expr{get_suites(128, 1)@} isn't
//!     satisfactory.
//! @endul
//!
//! The initialized @[Context] object is then passed to
//! @[File()->create()] or used as is embedded in @[Port].
//!
//! @seealso
//!   @[File], @[Port], @[Standards.X509]

#ifdef SSL3_DEBUG
#define SSL3_DEBUG_MSG(X ...)  werror(X)
#else /*! SSL3_DEBUG */
#define SSL3_DEBUG_MSG(X ...)
#endif /* SSL3_DEBUG */

import ".";
import Constants;

protected void create()
{
  SSL3_DEBUG_MSG("SSL.Context->create\n");

  /* Backwards compatibility */
  preferred_suites = get_suites(128, 1);
}

//! The minimum supported protocol version.
//!
//! Defaults to @[PROTOCOL_TLS_1_0].
//!
//! @note
//!   This value should not be greater than @[max_version].
ProtocolVersion min_version = PROTOCOL_TLS_1_0;

//! The maximum supported protocol version.
//!
//! Defaults to @[PROTOCOL_TLS_MAX].
//!
//! @note
//!   This value should not be less than @[min_version].
ProtocolVersion max_version = PROTOCOL_TLS_MAX;

//! List of advertised protocols using using TLS application level
//! protocol negotiation.
array(string(8bit)) advertised_protocols;

//! The maximum amount of data that is sent in each SSL packet by
//! @[File]. A value between 1 and @[Constants.PACKET_MAX_SIZE].
int packet_max_size = PACKET_MAX_SIZE;

//! Lists the supported compression algorithms in order of preference.
//!
//! Defaults to @expr{({ COMPRESSION_null, COMPRESSION_deflate })@}
//! (ie avoid compression unless the client requires it) due to
//! SSL attacks that target compression.
array(int) preferred_compressors = ({
  COMPRESSION_null,
#if constant(Gz)
  COMPRESSION_deflate,
#endif
 });

//! If set, the other peer will be probed for the heartbleed bug
//! during handshake. If heartbleed is found the connection is closed
//! with insufficient security fatal error.
int(0..1) heartbleed_probe = 0;

//! @decl Alert alert_factory(SSL.Connection con, int level, int description, @
//!			      ProtocolVersion version, @
//!			      string|void message, mixed|void trace)
//!
//! Alert factory.
//!
//! This function may be overloaded to eg obtain logging of
//! generated alerts.
//!
//! @param con
//!   Connection which caused the alert.
//!
//! @param level
//!   Level of alert.
//!
//! @param description
//!   Description code for the alert.
//!
//! @param message
//!   Optional log message for the alert.
//!
//! @note
//!   Not all alerts are fatal, and some (eg @[ALERT_close_notify]) are used
//!   during normal operation.
Alert alert_factory(object con,
		     int(1..2) level, int(8bit) description,
                     ProtocolVersion version, string|void message)
{
  return Alert(level, description, version, message);
}


//
// --- Cryptography
//

//! Temporary, non-certified, private keys, used for RSA key exchange
//! in export mode. They are used as follows:
//!
//! @[short_rsa] is a 512-bit RSA key used for the SSL 3.0 and TLS 1.0
//! export cipher suites.
//!
//! @[long_rsa] is a 1024-bit RSA key to be used for the RSA_EXPORT1024
//! suites from draft-ietf-tls-56-bit-ciphersuites-01.txt.
//!
//! They have associated counters @[short_rsa_counter] and @[long_rsa_counter],
//! which are decremented each time the keys are used.
//!
//! When the counters reach zero, the corresponding RSA key is cleared,
//! and a new generated on demand at which time the counter is reset.
Crypto.RSA.State long_rsa;
Crypto.RSA.State short_rsa;

//! Counters for export RSA keys.
int long_rsa_counter;
int short_rsa_counter;

//! Used to generate random cookies for the hello-message. If we use
//! the RSA keyexchange method, and this is a server, this random
//! number generator is not used for generating the master_secret. By
//! default set to @[Crypto.Random.random_string].
function(int(0..):string(8bit)) random = Crypto.Random.random_string;

//! Attempt to enable encrypt-then-mac mode.
int encrypt_then_mac = 1;

//! Cipher suites we want to support, in order of preference, best
//! first.
array(int) preferred_suites;

//! Supported elliptical curve cipher curves in order of preference.
array(int) ecc_curves = reverse(sort(indices(ECC_CURVES)));

//! Supported DH groups for DHE key exchanges, in order of preference.
//! Defaults to MODP Group 24 (2048/256 bits) from RFC 5114 section
//! 2.3.
array(Crypto.DH.Parameters) dh_groups = ({
  Crypto.DH.MODPGroup24, // MODP Group 24 (2048/256 bits).
});


//! The set of <hash, signature> combinations to use by us.
//!
//! Only used with TLS 1.2 and later.
//!
//! Defaults to all combinations supported by Pike except for MD5.
//!
//! This list is typically filtered by @[get_signature_algorithms()]
//! to get rid of combinations not supported by the runtime.
//!
//! @note
//!   According to RFC 5246 7.4.2 all certificates needs to be signed
//!   by any of the supported signature algorithms. To be forward
//!   compatible this list needs to be limited to the combinations
//!   that have existing PKCS identifiers.
//!
//! @seealso
//!   @[get_signature_algorithms()]
array(array(int)) signature_algorithms = ({
#if constant(Crypto.SHA512)
#if constant(Crypto.ECC.Curve)
  ({ HASH_sha512, SIGNATURE_ecdsa }),
#endif
  ({ HASH_sha512, SIGNATURE_rsa }),
#endif
#if constant(Crypto.SHA384)
#if constant(Crypto.ECC.Curve)
  ({ HASH_sha384, SIGNATURE_ecdsa }),
#endif
  ({ HASH_sha384, SIGNATURE_rsa }),
#endif
#if constant(Crypto.ECC.Curve)
  ({ HASH_sha256, SIGNATURE_ecdsa }),
#endif
  ({ HASH_sha256, SIGNATURE_dsa }),
  ({ HASH_sha256, SIGNATURE_rsa }),
#if constant(Crypto.SHA224)
#if constant(Crypto.ECC.Curve)
  ({ HASH_sha224, SIGNATURE_ecdsa }),
#endif
  ({ HASH_sha224, SIGNATURE_dsa }),
#endif
#if constant(Crypto.ECC.Curve)
  ({ HASH_sha, SIGNATURE_ecdsa }),
#endif
  ({ HASH_sha, SIGNATURE_dsa }),
  ({ HASH_sha, SIGNATURE_rsa }),
});

//! Get the (filtered) set of locally supported signature algorithms.
//!
//! @seealso
//!   @[signature_algorithms]
array(array(int)) get_signature_algorithms(array(array(int))|void signature_algorithms)
{
  if (!signature_algorithms) {
    signature_algorithms = this_program::signature_algorithms;
  }

#if constant(Crypto.ECC.Curve) && constant(Crypto.SHA512) && \
  constant(Crypto.SHA384) && constant(Crypto.SHA224)
  return signature_algorithms;
#else
  return [array(array(int))]
    filter(signature_algorithms,
		lambda(array(int) pair) {
		  [int hash, int sign] = pair;
#if !constant(Crypto.ECC.Curve)
		  if (sign == SIGNATURE_ecdsa) return 0;
#endif
		  if ((<
#if !constant(Crypto.SHA512)
			HASH_sha512,
#endif
#if !constant(Crypto.SHA384)
			HASH_sha384,
#endif
#if !constant(Crypto.SHA224)
			HASH_sha224,
#endif
		      >)[hash]) return 0;
		  return 1;
		});
#endif
}

// Generate a sort key for a cipher suite.
//
// The larger the value, the stronger the cipher suite.
protected int cipher_suite_sort_key(int suite)
{
  array(int) info = [array(int)] (CIPHER_SUITES[suite] || ({ 0, 0, 0 }));

  int keylength = CIPHER_effective_keylengths[info[1]];

  // NB: Currently the hash algorithms are allocated in a suitable order.
  int hash = info[2];

  if (sizeof(info) > 3) {
    hash |= info[3]<<5;
  }

  // NB: As are the cipher ids if you disregard the keylengths.
  int cipher = info[1];

  // FIXME: I'm not quite sure about the priorities here.
  int ke_prio = ([
    KE_null:		0,
    KE_dh_anon:		1,
    KE_ecdh_anon:	2,
    KE_fortezza:	3,
    KE_dms:		4,
    KE_dh_rsa:		5,
    KE_dh_dss:		6,
    KE_rsa:		7,
    KE_rsa_fips:	8,
    KE_ecdh_rsa:	9,
    KE_ecdh_ecdsa:	10,
    KE_dhe_rsa:		11,
    KE_dhe_dss:		12,
    KE_ecdhe_rsa:	13,
    KE_ecdhe_ecdsa:	14,
  ])[info[0]];

  int auth_prio = keylength && ([
    KE_null:		0,
    KE_dh_anon:		0,
    KE_ecdh_anon:	0,
    KE_fortezza:	1,
    KE_dms:		2,
    KE_rsa:		8,
    KE_rsa_fips:	8,
    KE_dhe_rsa:		8,
    KE_ecdhe_rsa:	8,
    KE_dh_dss:		8,
    KE_dh_rsa:		8,
    KE_dhe_dss:		8,
    KE_ecdh_rsa:	8,
    KE_ecdh_ecdsa:	8,
    KE_ecdhe_ecdsa:	8,
  ])[info[0]];

  // int not_anonymous = ke_prio >= 3;

  // Klugde to test GCM.
  // if (sizeof(info) > 3) keylength += 0x100;

  // NB: 8 bits for cipher.
  //     8 bits for hash + mode.
  //     8 bits for key exchange.
  //     12 bits for keylength.
  //     4 bits for auth.
  return cipher | hash << 8 | ke_prio << 16 | keylength << 24 | auth_prio << 36;
}

//! Sort a set of cipher suites according to our preferences.
//!
//! @returns
//!   Returns the array sorted with the most preferrable (aka "best")
//!   cipher suite first.
//!
//! @note
//!   The original array (@[suites]) is modified destructively,
//!   but is not the same array as the result.
array(int) sort_suites(array(int) suites)
{
  sort(map(suites, cipher_suite_sort_key), suites);
  return reverse(suites);
}

//! Get the prioritized list of supported cipher suites
//! that satisfy the requirements.
//!
//! @param min_keylength
//!   Minimum supported effective keylength in bits. Defaults to @expr{128@}.
//!   Specify @expr{-1@} to enable null ciphers.
//!
//! @param ke_mode
//!   Level of protection for the key exchange.
//!   @int
//!     @value 0
//!       Require forward secrecy (ephemeral keys).
//!     @value 1
//!       Also allow certificate based key exchanges.
//!     @value 2
//!       Allow anonymous server key exchange. Note that this
//!       allows for man in the middle attacks.
//!   @endint
//!
//! @param blacklisted_ciphers
//!   Multiset of ciphers that are NOT to be used.
//!
//! @param blacklisted_kes
//!   Multiset of key exchange methods that are NOT to be used.
//!
//! @param blacklisted_hashes
//!   Multiset of hash algoriths that are NOT to be used.
//!
//! @param blacklisted_ciphermodes
//!   Multiset of cipher modes that are NOT to be used.
//!
//! @note
//!   Note that the effective keylength may differ from
//!   the actual keylength for old ciphers where there
//!   are known attacks.
array(int) get_suites(int(-1..)|void min_keylength,
		      int(0..2)|void ke_flags,
		      multiset(int)|void blacklisted_ciphers,
		      multiset(KeyExchangeType)|void blacklisted_kes,
		      multiset(HashAlgorithm)|void blacklisted_hashes,
		      multiset(CipherModes)|void blacklisted_ciphermodes)
{
  if (!min_keylength) min_keylength = 128;	// Reasonable default.

  // Ephemeral key exchange methods.
  multiset(int) kes = (<
    KE_dhe_rsa, KE_dhe_dss,
    KE_ecdhe_rsa, KE_ecdhe_ecdsa,
  >);

  if (ke_flags) {
    // Static certificate based key exchange methods.
    kes |= (<
      KE_rsa, KE_rsa_fips,
      KE_dh_rsa, KE_dh_dss,
#if constant(Crypto.ECC.Curve)
      KE_ecdh_rsa,
      KE_ecdh_ecdsa,
#endif
    >);
    if (ke_flags == 2) {
      // Unsigned key exchange methods.
      kes |= (< KE_null, KE_dh_anon,
#if constant(Crypto.ECC.Curve)
		KE_ecdh_anon,
#endif
      >);
    }
  }

  if (blacklisted_kes) {
    kes -= blacklisted_kes;
  }

  // Filter unsupported key exchange methods.
  array(int) res =
    filter(indices(CIPHER_SUITES),
	   lambda(int suite) {
	     return kes[CIPHER_SUITES[suite][0]];
	   });

  // Filter short effective key lengths.
  if (min_keylength > 0) {
    res = filter(res,
		 lambda(int suite, int min_keylength) {
		   return min_keylength <=
		     CIPHER_effective_keylengths[CIPHER_SUITES[suite][1]];
		 }, min_keylength);
  }

  if (blacklisted_ciphers) {
    res = filter(res,
		 lambda(int suite, multiset(int) blacklisted_hashes) {
		   return !blacklisted_hashes[CIPHER_SUITES[suite][1]];
		 }, blacklisted_ciphers);
  }

#if !constant(Crypto.SHA384)
  // Filter suites needing SHA384 as our Nettle doesn't support it.
  if (!blacklisted_hashes)
    blacklisted_hashes = (< HASH_sha384 >);
  else
    blacklisted_hashes[HASH_sha384] = 1;
#endif
  if (blacklisted_hashes) {
    res = filter(res,
		 lambda(int suite, multiset(int) blacklisted_hashes) {
		   return !blacklisted_hashes[CIPHER_SUITES[suite][2]];
		 }, blacklisted_hashes);
  }

  if (blacklisted_ciphermodes) {
    res = filter(res,
		 lambda(int suite, multiset(int) blacklisted_ciphermodes) {
		   array(int) info = [array(int)]CIPHER_SUITES[suite];
		   int mode = (sizeof(info) > 3)?info[3]:MODE_cbc;
		   return !blacklisted_ciphermodes[mode];
		 }, blacklisted_ciphermodes);
  }

  // Sort and return.
  return sort_suites(res);
}

//! Filter cipher suites from @[preferred_suites] that don't have a
//! key with an effective length of at least @[min_keylength] bits.
void filter_weak_suites(int min_keylength)
{
  if (!preferred_suites || !min_keylength) return;
  preferred_suites =
    filter(preferred_suites,
	   lambda(int suite) {
	     array(int) def = [array(int)]CIPHER_SUITES[suite];
	     return def &&
	       (CIPHER_effective_keylengths[def[1]] >= min_keylength);
	   });
}

#if constant(Crypto.ECC.Curve) && constant(Crypto.AES.GCM) && constant(Crypto.SHA384)

//! Configure the context for Suite B compliant operation.
//!
//! This restricts the context to the cipher suites
//! specified by RFC 6460 in strict mode.
//!
//! Additional suites may be enabled, but they will only be
//! selected if a Suite B suite isn't available.
//!
//! @param min_keylength
//!   Minimum supported key length in bits. Either @expr{128@}
//!   or @expr{192@}.
//!
//! @param strictness_level
//!   Allow additional suites.
//!   @int
//!     @value 2..
//!       Strict mode.
//!
//!       Allow only the Suite B suites from RFC 6460 and TLS 1.2.
//!     @value 1
//!       Transitional mode.
//!
//!       Also allow the transitional suites from RFC 5430 for use
//!       with TLS 1.0 and 1.1.
//!     @value 0
//!       Permissive mode (default).
//!
//!       Also allow other suites that conform to the minimum key length.
//!   @endint
//!
//! @note
//!   This function is only present when Suite B compliant operation
//!   is possible (ie both elliptic curves and GCM are available).
//!
//! @note
//!   Note also that for Suite B server operation compliant certificates
//!   need to be added with @[add_cert()].
//!
//! @seealso
//!   @[get_suites()]
void configure_suite_b(int(128..)|void min_keylength,
		       int(0..)|void strictness_level)
{
  if (min_keylength < 128) min_keylength = 128;

  if (min_keylength > 128) {
    preferred_suites = ({
      TLS_ecdhe_ecdsa_with_aes_256_gcm_sha384,
    });
  } else {
    preferred_suites = ({
      TLS_ecdhe_ecdsa_with_aes_128_gcm_sha256,
      TLS_ecdhe_ecdsa_with_aes_256_gcm_sha384,
    });
  }

  max_version = PROTOCOL_TLS_MAX;
  min_version = PROTOCOL_TLS_1_2;

  if (strictness_level < 2) {
    // Transitional or permissive mode.

    // Allow TLS 1.0.
    min_version = PROTOCOL_TLS_1_0;

    // First add the transitional suites.
    if (min_keylength > 128) {
      // Transitional Suite B Combination 2
      preferred_suites += ({
	TLS_ecdhe_ecdsa_with_aes_256_cbc_sha,
      });
    } else {
      // Transitional Suite B Combination 1
      preferred_suites += ({
	TLS_ecdhe_ecdsa_with_aes_128_cbc_sha,
	TLS_ecdhe_ecdsa_with_aes_256_cbc_sha,
      });
    }

    if (strictness_level < 1) {
      // Permissive mode. Add the remaining suites of
      // the required strength.
      preferred_suites += get_suites(min_keylength) - preferred_suites;
    }
  }
}

#endif /* Crypto.ECC.Curve && Crypto.AES.GCM && Crypto.SHA384 */


//
// --- Certificates and authentication
//

// Unless connecting in anonymous mode the server has to have a set of
// CertificatePair certificate chains to sign its handshake with.
// These are stored in the cert_chains_domain mapping, where they are
// retrieved based on domain the client is connecting to.
//
// If the server sends a certificate request the client has to respond
// with a certificate matching the requested issuer der. These are
// stored in the cert_chains_issuer mapping.
//
// The client/server potentially has a set of trusted issuers
// certificate (root certificates) that are used to validate the
// server/client sent certificate. These are stored in a cache from
// subject der to Verifier object. FIXME: Should use key identifier.

//! Policy for client authentication. One of
//! @[SSL.Constants.AUTHLEVEL_none], @[SSL.Constants.AUTHLEVEL_ask]
//! and @[SSL.Constants.AUTHLEVEL_require].
int auth_level;

//! Array of authorities that are accepted for client certificates.
//! The server will only accept connections from clients whose
//! certificate is signed by one of these authorities. The string is a
//! DER-encoded certificate, which typically must be decoded using
//! @[MIME.decode_base64] or @[Standards.PEM.Messages] first.
//!
//! Note that it is presumed that the issuer will also be trusted by
//! the server. See @[trusted_issuers] for details on specifying
//! trusted issuers.
//!
//! If empty, the server will accept any client certificate whose
//! issuer is trusted by the server.
void set_authorities(array(string) a)
{
  authorities = a;
  update_authorities();
}

//! When set, require the chain to be known, even if the root is self
//! signed.
//! 
//! Note that if set, and certificates are set to be verified, trusted
//! issuers must be provided, or no connections will be accepted.
int require_trust=0;

//! Get the list of allowed authorities. See @[set_authorities]. 
array(string) get_authorities()
{
  return authorities;
}

protected array(string) authorities = ({});
array(string(8bit)) authorities_cache = ({});

//! Sets the list of trusted certificate issuers. 
//!
//! @param a
//!
//! An array of certificate chains whose root is self signed (ie a
//! root issuer), and whose final certificate is an issuer that we
//! trust. The root of the certificate should be first certificate in
//! the chain. The string is a DER-encoded certificate, which
//! typically must be decoded using @[MIME.decode_base64] or
//! @[Standards.PEM.Messages] first.
//! 
//! If this array is left empty, and the context is set to verify
//! certificates, a certificate chain must have a root that is self
//! signed.
void set_trusted_issuers(array(array(string))  i)
{
  trusted_issuers = i;
  update_trusted_issuers();
}

//! Get the list of trusted issuers. See @[set_trusted_issuers]. 
array(array(string)) get_trusted_issuers()
{
  return trusted_issuers;
}

protected array(array(string)) trusted_issuers = ({});
mapping(string:array(Standards.X509.Verifier)) trusted_issuers_cache = ([]);

//! Determines whether certificates presented by the peer are
//! verified, or just accepted as being valid.
int verify_certificates = 0;

//! For client authentication. Used only if auth_level is AUTH_ask or
//! AUTH_require.
array(int) preferred_auth_methods =
({ AUTH_rsa_sign });

// Lookup from issuer DER to an array of suitable @[CertificatePair]s,
// sorted in order of strength.
protected mapping(string(8bit):array(CertificatePair)) cert_chains_issuer = ([]);

// Lookup from DN/SNI domain name/glob to an array of suitable
// @[CertificatePair]s, sorted in order of strength.
protected mapping(string(8bit):array(CertificatePair)) cert_chains_domain = ([]);

//! Look up a suitable set of certificates for the specified issuer.
//! @[UNDEFIEND] if no certificate was found.
array(CertificatePair) find_cert_issuer(array(string) ders)
{
  // Return the first matching issuer. FIXME: Should we merge if
  // several matching issuers are found?
  foreach(ders, string der)
    if(cert_chains_issuer[der])
      return cert_chains_issuer[der];

  // We MAY return any certificate here. Let's not reveal any
  // certificates not specifically requested.
  return UNDEFINED;
}

//! Look up a suitable set of certificates for the specified domain.
//! @[UNDEFINED] if no certificate was found.
array(CertificatePair) find_cert_domain(string(8bit) domain)
{
  if( domain )
  {
    if( cert_chains_domain[domain] )
      return cert_chains_domain[domain];

    // Return first matching chain.
    foreach(cert_chains_domain; string g; array(CertificatePair) chains)
      if( glob(g, domain) )
        return chains;
  }

  return cert_chains_domain["*"];
}

//! Returns a list of all server certificates added with @[add_cert].
array(CertificatePair) get_certificates()
{
  mapping(CertificatePair:int) c = ([]);
  foreach(cert_chains_domain;; array(CertificatePair) chains)
    foreach(chains, CertificatePair p)
      c[p]++;
  return indices(c);
}

//! Add a certificate.
//!
//! This function is used on both servers and clients to add
//! a key and chain of certificates to the set of certificate
//! candidates to use in @[find_cert()].
//!
//! On a server these are used in the normal initial handshake,
//! while on a client they are only used if a server requests
//! client certificate authentication.
//!
//! @param key
//!   Private key matching the first certificate in @[certs].
//!
//!   Supported key types are currently:
//!   @mixed
//!     @type Crypto.RSA.State
//!       Rivest-Shamir-Adelman.
//!     @type Crypto.DSA.State
//!       Digital Signing Algorithm.
//!     @type Crypto.ECC.Curve.ECDSA
//!       Elliptic Curve Digital Signing Algorithm.
//!   @endmixed
//!
//!   This key MUST match the public key in the first certificate
//!   in @[certs].
//!
//! @param certs
//!   A chain of X509.v1 or X509.v3 certificates, with the local
//!   certificate first and root-most certificate last.
//!
//! @param extra_name_globs
//!   Further SNI globs (than the ones in the first certificate), that
//!   this certificate should be selected for. Typically used to set
//!   the default certificate(s) by specifying @expr{({ "*" })@}.
//!
//!   The SNI globs are only relevant for server-side certificates.
//!
//! @param cp
//!   An alternative is to send an initialized @[CertificatePair].
//!
//! @throws
//!   The function performs various validations of the @[key]
//!   and @[certs], and throws errors if the validation fails.
//!
//! @seealso
//!   @[find_cert()]
void add_cert(Crypto.Sign.State key, array(string(8bit)) certs,
	      array(string(8bit))|void extra_name_globs)
{
  CertificatePair cp = CertificatePair(key, certs, extra_name_globs);
  add_cert(cp);
}
variant void add_cert(string(8bit) key, array(string(8bit)) certs,
                      array(string(8bit))|void extra_name_globs)
{
  Crypto.Sign.State _key = Standards.PKCS.RSA.parse_private_key(key) ||
    Standards.PKCS.DSA.parse_private_key(key) ||
#if constant(Crypto.ECC.Curve)
    Standards.PKCS.ECDSA.parse_private_key(key) ||
#endif
    0;
  add_cert(_key, certs, extra_name_globs);
}
variant void add_cert(CertificatePair cp)
{
  void add(string what, mapping(string:array(CertificatePair)) to)
  {
    if( !to[what] )
      to[what] = ({cp});
    else
      to[what] = sort( to[what]+({cp}) );
  };

  // FIXME: Look at leaf flags to determine which mapping to store the
  // chains in.

  // Insert cp in cert_chains both under all DN/SNI names/globs and
  // under issuer DER. Keep lists sorted by strength.
  foreach( cp->globs, string id )
    add(id, cert_chains_domain);

  add(cp->issuers[0], cert_chains_issuer);
}

// update the cached decoded authorities list
private void update_authorities()
{
  authorities_cache = ({});
  foreach(authorities, string a)
    authorities_cache += ({ Standards.X509.decode_certificate(a)->
                            subject->get_der() });
}

// update the cached decoded issuers list
private void update_trusted_issuers()
{
  trusted_issuers_cache=([]);
  foreach(trusted_issuers, array(string) i)
  {
    // make sure the chain is valid and intact.
    mapping result = Standards.X509.verify_certificate_chain(i, ([]), 0);

    if(!result->verified)
      error("Broken trusted issuer chain!\n");

    // FIXME: The pathLenConstraint does not survive the cache.

    // The leaf of the trusted issuer is the root to validate
    // certificate chains against.
    Standards.X509.TBSCertificate cert =
      ([array(object(Standards.X509.TBSCertificate))]result->certificates)[-1];

    if( !cert->ext_basicConstraints_cA ||
        !(cert->ext_keyUsage & Standards.X509.KU_keyCertSign) )
      error("Trusted issuer not allowed to sign other certificates.\n");

    trusted_issuers_cache[cert->subject->get_der()] += ({ cert->public_key });
  }
}


//
// --- Sessions
//

//! Non-zero to enable caching of sessions
int use_cache = 1;

//! Sessions are removed from the cache when they are older than this
//! limit (in seconds). Sessions are also removed from the cache if a
//! connection using the session dies unexpectedly.
int session_lifetime = 600;

//! Maximum number of sessions to keep in the cache.
int max_sessions = 300;

/* Queue of pairs (time, id), in cronological order */
ADT.Queue active_sessions = ADT.Queue();

mapping(string:Session) session_cache = ([]);

int session_number; /* Incremented for each session, and used when
		     * constructing the session id */

// Remove sessions older than @[session_lifetime] from the session cache.
void forget_old_sessions()
{
  int t = time() - session_lifetime;
  array pair;
  while ( (pair = [array]active_sessions->peek())
	  && (pair[0] < t)) {
    SSL3_DEBUG_MSG("SSL.Context->forget_old_sessions: "
                   "garbing session %O due to session_lifetime limit\n",
                   pair[1]);
    m_delete (session_cache, [string]([array]active_sessions->get())[1]);
  }
}

//! Lookup a session identifier in the cache. Returns the
//! corresponding session, or zero if it is not found or caching is
//! disabled.
Session lookup_session(string id)
{
  if (use_cache)
  {
    return session_cache[id];
  }
  else
    return 0;
}

//! Create a new session.
Session new_session()
{
  Session s = Session();
  s->identity = (use_cache) ?
    sprintf("%4cPikeSSL3%4c", time(), session_number++) : "";
  return s;
}

//! Add a session to the cache (if caching is enabled).
void record_session(Session s)
{
  if (use_cache && s->identity)
  {
    while (sizeof (active_sessions) >= max_sessions) {
      array pair = [array] active_sessions->get();
      SSL3_DEBUG_MSG("SSL.Context->record_session: "
                     "garbing session %O due to max_sessions limit\n", pair[1]);
      m_delete (session_cache, [string]pair[1]);
    }
    forget_old_sessions();
    SSL3_DEBUG_MSG("SSL.Context->record_session: caching session %O\n",
                   s->identity);
    active_sessions->put( ({ time(1), s->identity }) );
    session_cache[s->identity] = s;
  }
}

//! Remove a session from the cache.
void purge_session(Session s)
{
  SSL3_DEBUG_MSG("SSL.Context->purge_session: %O\n", s->identity || "");
  if (s->identity)
    m_delete (session_cache, s->identity);
  /* There's no need to remove the id from the active_sessions queue */
}


//
// --- Compat code below
//

protected Crypto.RSA.State compat_rsa;
protected array(string(8bit)) compat_certificates;

//! The servers default private RSA key.
//! 
//! Compatibility, don't use.
//!
//! @deprecated find_cert
//!
//! @seealso
//!   @[`certificates], @[find_cert()]
__deprecated__ Crypto.RSA.State `rsa()
{
  return compat_rsa;
}

//! Set the servers default private RSA key.
//!
//! Compatibility, don't use.
//!
//! @deprecated add_cert
//!
//! @seealso
//!   @[`certificates=], @[add_cert()]
__deprecated__ void `rsa=(Crypto.RSA.State k)
{
  compat_rsa = k;
  if (k && compat_certificates) {
    catch {
      add_cert(k, compat_certificates);
    };
  }
}

//! The server's certificate, or a chain of X509.v3 certificates, with
//! the server's certificate first and root certificate last.
//!
//! Compatibility, don't use.
//!
//! @deprecated find_cert
//!
//! @seealso
//!   @[`rsa], @[find_cert()]
__deprecated__ array(string(8bit)) `certificates()
{
  return compat_certificates;
}

//! Set the servers certificate, or a chain of X509.v3 certificates, with
//! the servers certificate first and root certificate last.
//!
//! Compatibility, don't use.
//!
//! @deprecated add_cert
//!
//! @seealso
//!   @[`rsa=], @[add_cert()]
__deprecated__ void `certificates=(array(string(8bit)) certs)
{
  compat_certificates = certs;

  if (compat_rsa && certs) {
    catch {
      add_cert(compat_rsa, certs);
    };
  }
}

//! The clients RSA private key.
//!
//! Compatibility, don't use.
//!
//! @deprecated find_cert
//!
//! @seealso
//!   @[`certificates], @[find_cert()]
__deprecated__ Crypto.RSA.State `client_rsa()
{
  return compat_rsa;
}

//! Set the clients default private RSA key.
//!
//! Compatibility, don't use.
//!
//! @deprecated add_cert
//!
//! @seealso
//!   @[`client_certificates=], @[add_cert()]
__deprecated__ void `client_rsa=(Crypto.RSA.State k)
{
  compat_rsa = k;
  if (k && compat_certificates) {
    catch {
      add_cert(k, compat_certificates);
    };
  }
}

//! The client's certificate, or a chain of X509.v3 certificates, with
//! the client's certificate first and root certificate last.
//!
//! Compatibility, don't use.
//!
//! @deprecated find_cert
//!
//! @seealso
//!   @[`rsa], @[find_cert()]
__deprecated__ array(array(string(8bit))) `client_certificates()
{
  return compat_certificates && ({ compat_certificates });
}

//! Set the client's certificate, or a chain of X509.v3 certificates, with
//! the client's certificate first and root certificate last.
//!
//! Compatibility, don't use.
//!
//! @deprecated add_cert
//!
//! @seealso
//!   @[`rsa=], @[add_cert()]
__deprecated__ void `client_certificates=(array(array(string(8bit))) certs)
{
  compat_certificates = certs && (sizeof(certs)?certs[0]:({}));

  if (compat_rsa && certs) {
    foreach(certs, array(string(8bit)) chain) {
      catch {
	add_cert(compat_rsa, chain);
      };
    }
  }
}

//! Compatibility.
//! @deprecated find_cert
__deprecated__ Crypto.DSA.State `dsa()
{
  return UNDEFINED;
}

//! Compatibility.
//! @deprecated add_cert
__deprecated__ void `dsa=(Crypto.DSA.State k)
{
  error("The old DSA API is not supported anymore.\n");
}

//! Set @[preferred_suites] to RSA based methods.
//!
//! @param min_keylength
//!   Minimum acceptable key length in bits.
//!
//! @seealso
//!   @[dhe_dss_mode()], @[filter_weak_suites()]
//!
//! @deprecated get_suites
__deprecated__ void rsa_mode(int(0..)|void min_keylength)
{
  SSL3_DEBUG_MSG("SSL.Context: rsa_mode()\n");
  preferred_suites = get_suites(min_keylength, 1);
}

//! Set @[preferred_suites] to DSS based methods.
//!
//! @param min_keylength
//!   Minimum acceptable key length in bits.
//!
//! @seealso
//!   @[rsa_mode()], @[filter_weak_suites()]
//!
//! @deprecated get_suites
__deprecated__ void dhe_dss_mode(int(0..)|void min_keylength)
{
  SSL3_DEBUG_MSG("SSL.Context: dhe_dss_mode()\n");
  preferred_suites = get_suites(min_keylength, 1);
}
