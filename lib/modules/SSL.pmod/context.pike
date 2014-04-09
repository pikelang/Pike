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
//! The initialized @[context] object is then passed to
//! @[sslfile()->create()] or used as is embedded in @[sslport].
//!
//! @seealso
//!   @[sslfile], @[sslport], @[Standards.X509]

#ifdef SSL3_DEBUG
#define SSL3_DEBUG_MSG(X ...)  werror(X)
#else /*! SSL3_DEBUG */
#define SSL3_DEBUG_MSG(X ...)
#endif /* SSL3_DEBUG */

import .Constants;

//! The minimum supported protocol version.
//!
//! Defaults to @[PROTOCOL_SSL_3_0].
//!
//! @note
//!   This value should not be greater than @[max_version].
ProtocolVersion min_version = PROTOCOL_SSL_3_0;

//! The maximum supported protocol version.
//!
//! Defaults to @[PROTOCOL_TLS_MAX].
//!
//! @note
//!   This value should not be less than @[min_version].
ProtocolVersion max_version = PROTOCOL_TLS_MAX;

protected Crypto.RSA compat_rsa;
protected array(string(8bit)) compat_certificates;

//! The servers default private RSA key.
//! 
//! Compatibility, don't use.
//!
//! @deprecated find_cert
//!
//! @seealso
//!   @[`certificates], @[find_cert()]
__deprecated__ Crypto.RSA `rsa()
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
__deprecated__ void `rsa=(Crypto.RSA k)
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


//! Should an SSL client include the Server Name extension?
//!
//! If so, then client_server_names should specify the values to send.
int client_use_sni = 0;

//! Host names to send to the server when using the Server Name
//! extension.
array(string(8bit)) client_server_names = ({});

/* For client authentication */

//! The clients RSA private key.
//!
//! Compatibility, don't use.
//!
//! @deprecated find_cert
//!
//! @seealso
//!   @[`certificates], @[find_cert()]
__deprecated__ Crypto.RSA `client_rsa()
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
__deprecated__ void `client_rsa=(Crypto.RSA k)
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

//! A mapping containing certificate chains for use by SNI (Server
//! Name Indication). Each entry should consist of a key indicating
//! the server hostname and the value containing the certificate chain
//! for that hostname.
//!
//! @deprecated add_cert
//!
//! @seealso
//!   @[add_cert()]
__deprecated__ void `sni_certificates=(mapping(string:array(string(8bit))) sni)
{
  error("The old SNI API is not supported anymore.\n");
}

__deprecated__ mapping(string:array(string(8bit))) `sni_certificates()
{
  return ([]);
}

#if 0

//! A function which will select an acceptable client certificate for
//! presentation to a remote server. This function will receive the
//! SSL context, an array of acceptable certificate types, and a list
//! of DNs of acceptable certificate authorities. This function should
//! return an array of strings containing a certificate chain, with
//! the client certificate first, (and the root certificate last, if
//! applicable.)
function(.context,array(int),array(string(8bit)):array(string(8bit)))
  client_certificate_selector = internal_select_client_certificate;

//! A function which will select an acceptable server certificate for
//! presentation to a client. This function will receive the SSL
//! context, and an array of server names, if provided by the client.
//! This function should return an array of strings containing a
//! certificate chain, with the client certificate first, (and the
//! root certificate last, if applicable.)
//!
//! The default implementation will select a certificate chain for a
//! given server based on values contained in @[sni_certificates].
function (.context,array(string(8bit)):array(string(8bit)))
  select_server_certificate_func = internal_select_server_certificate;

//! A function which will select an acceptable server key for
//! presentation to a client. This function will receive the SSL
//! context, and an array of server names, if provided by the client.
//! This function should return an object matching the certificate for
//! the server hostname.
//!
//! The default implementation will select the key for a given server
//! based on values contained in @[sni_keys].
function (.context,array(string):object(Crypto.Sign)) select_server_key_func
  = internal_select_server_key;

private array(string(8bit))
  internal_select_client_certificate(.context context,
				     array(int) acceptable_types,
				     array(string) acceptable_authority_dns)
{
  if( !context->client_certificates ||
      !sizeof(context->client_certificates) )
    return ({});

  // FIXME: Create a cache for the certificate objects.
  array(mapping(string:mixed)) c = ({});
  foreach(context->client_certificates; int i; array(string) chain)
  {
    if(sizeof(chain))
      c += ({ (["cert":Standards.X509.decode_certificate(chain[0]),
                "chain":i ]) });
  }

  string wantedtype;
  mapping(int:string) cert_types = ([
    AUTH_rsa_sign : "rsa",
    AUTH_dss_sign : "dss",
    AUTH_ecdsa_sign : "ecdsa",
  ]);

  foreach(acceptable_types, int t)
  {
    wantedtype = cert_types[t];

    foreach(c, mapping(string:mixed) cert)
    {
      Standards.X509.TBSCertificate crt =
        [object(Standards.X509.TBSCertificate)]cert->cert;
      if(crt->public_key->type == wantedtype)
        return context->client_certificates[[int]cert->chain];
    }
  }

  // FIXME: Check acceptable_authority_dns.
  acceptable_authority_dns;
  return ({});
}

#endif /* 0 */

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
mapping(string:Standards.X509.Verifier) trusted_issuers_cache = ([]);

//! Determines whether certificates presented by the peer are
//! verified, or just accepted as being valid.
int verify_certificates = 0;

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
Crypto.RSA long_rsa;
Crypto.RSA short_rsa;

//! Counters for export RSA keys.
int long_rsa_counter;
int short_rsa_counter;

//! Compatibility.
//! @deprecated get_private_key
__deprecated__ Crypto.DSA `dsa()
{
  return UNDEFINED;
}

//! Compatibility.
//! @deprecated set_private_key
__deprecated__ void `dsa=(Crypto.DSA k)
{
  error("The old DSA API is not supported anymore.\n");
}

#if 0
//! Parameters for dh keyexchange.
.Cipher.DHKeyExchange dh_ke;
#endif


//! Used to generate random cookies for the hello-message. If we use
//! the RSA keyexchange method, and this is a server, this random
//! number generator is not used for generating the master_secret. By
//! default set to @[Crypto.Random.random_string].
function(int(0..):string(8bit)) random = Crypto.Random.random_string;

//! Certificates and their corresponding keys.
array(CertificatePair) cert_pairs = ({});

//! Lookup from SNI (Server Name Indication) (server), or Issuer DER
//! (client) to an array of suitable @[CertificatePair]s.
//!
//! Generated on demand from @[cert_pairs].
mapping(string(8bit):array(CertificatePair)) cert_cache = ([]);

//! For client authentication. Used only if auth_level is AUTH_ask or
//! AUTH_require.
array(int) preferred_auth_methods =
({ AUTH_rsa_sign });

//! Cipher suites we want to support, in order of preference, best
//! first.
array(int) preferred_suites;

//! Supported elliptical curve cipher curves in order of preference.
array(int) ecc_curves = reverse(sort(indices(ECC_CURVES)));

//! List of advertised protocols using using TLS next protocol
//! negotiation.
array(string(8bit)) advertised_protocols;

//! Protocols to advertise during handshake using the next protocol
//! negotiation extension. Currently only used together with spdy.
void advertise_protocols(string(8bit) ... protos)
{
    advertised_protocols = protos;
}

//! The maximum amount of data that is sent in each SSL packet by
//! @[sslfile]. A value between 1 and
//! @[SSL.Constants.PACKET_MAX_SIZE].
int packet_max_size = SSL.Constants.PACKET_MAX_SIZE;

//! Look up a suitable set of certificates for the specified SNI (server)
//! or issuer (client).
//!
//! @param is_issuer
//!   Indicates whether to @[glob]-match against the common name (server),
//!   or against the DER for the issuer (client).
array(CertificatePair) find_cert(array(string)|void sni_or_issuer,
				 int(0..1)|void is_issuer)
{
  mapping(string(8bit):array(CertificatePair)) certs = ([]);
  array(string(8bit)) maybes = ({});

  if (!is_issuer) {
    sni_or_issuer = [array(string(8bit))]
      map(sni_or_issuer || ({ "" }), lower_case);
  }

  foreach(sni_or_issuer, string name) {
    array(CertificatePair) res;
    if (res = cert_cache[name]) {
      certs[name] = res;
      continue;
    }
    if (zero_type(res)) {
      // Not known bad.
      certs[name] = ({});
      maybes += ({ name });
    }
  }

  if (sizeof(maybes)) {
    // There were some unknown names. Check them.
    if (is_issuer) {
      foreach(cert_pairs, CertificatePair cp) {
	foreach(maybes, string(8bit) i) {
	  if (has_value(cp->issuers, i)) {
	    certs[i] += ({ cp });
	  }
	}
      }
    } else {
      foreach(cert_pairs, CertificatePair cp) {
	foreach(cp->globs, string(8bit) g) {
	  foreach(glob(g, maybes), string name) {
	    certs[name] += ({ cp });
	  }
	}
      }
    }

    if (sizeof(cert_cache) > (sizeof(cert_pairs) * 10 + 10)) {
      // It seems the cache has been poisoned. Clean it.
      cert_cache = ([]);
    }

    // Update the cache.
    foreach(maybes, string name) {
      cert_cache[name] = certs[name];
    }
  }

  // No certificate found.
  if (!sizeof(certs)) return UNDEFINED;

  if (sizeof(certs) == 1) {
    // Just a single matching name.
    return values(certs)[0];
  }

  return values(certs) * ({});
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
//!     @type Crypto.RSA
//!       Rivest-Shamir-Adelman.
//!     @type Crypto.DSA
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
//!   The function performs various validation of the @[key]
//!   and @[certs], and throws errors if the validation fails.
//!
//! @seealso
//!   @[find_cert()]
void add_cert(Crypto.Sign key, array(string(8bit)) certs,
	      array(string(8bit))|void extra_name_globs)
{
  CertificatePair cp = CertificatePair(key, certs, extra_name_globs);

  cert_pairs += ({ cp });

  cert_cache = ([]);
}
variant void add_cert(CertificatePair cp)
{
  cert_pairs += ({ cp });

  cert_cache = ([]);
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
    hash |= info[3]<<6;
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
    KE_rsa:		5,
    KE_dh_rsa:		6,
    KE_dh_dss:		7,
    KE_ecdh_rsa:	8,
    KE_ecdh_ecdsa:	9,
    KE_dhe_rsa:		10,
    KE_dhe_dss:		11,
    KE_ecdhe_rsa:	12,
    KE_ecdhe_ecdsa:	13,
  ])[info[0]];

  int auth_prio = keylength && ([
    KE_null:		0,
    KE_dh_anon:		0,
    KE_ecdh_anon:	0,
    KE_fortezza:	1,
    KE_dms:		2,
    KE_rsa:		8,
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
  //     8 bits for hash.
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
      KE_rsa,
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

//! Set @[preferred_suites] to RSA based methods.
//!
//! @param min_keylength
//!   Minimum acceptable key length in bits.
//!
//! @seealso
//!   @[dhe_dss_mode()], @[ecdsa_mode()], @[filter_weak_suites()]
//!
//! @deprecated get_suites
__deprecated__ void rsa_mode(int(0..)|void min_keylength, int|void max_version)
{
  SSL3_DEBUG_MSG("SSL.context: rsa_mode()\n");
  preferred_suites = get_suites(min_keylength, 1);
}

//! Set @[preferred_suites] to DSS based methods.
//!
//! @param min_keylength
//!   Minimum acceptable key length in bits.
//!
//! @seealso
//!   @[rsa_mode()], @[ecdsa_mode()], @[filter_weak_suites()]
//!
//! @deprecated get_suites
__deprecated__ void dhe_dss_mode(int(0..)|void min_keylength, int|void max_version)
{
  SSL3_DEBUG_MSG("SSL.context: dhe_dss_mode()\n");
  preferred_suites = get_suites(min_keylength, 1);
}

//! Lists the supported compression algorithms in order of preference.
//!
//! Defaults to @expr{({ COMPRESSION_null, COMPRESSION_deflate })@}
//! (ie avoid compression unless the client requires it) due to
//! SSL attacks that target compression.
array(int) preferred_compressors =
  ({ COMPRESSION_null, COMPRESSION_deflate });

//! Non-zero to enable cahing of sessions
int use_cache = 1;

//! Sessions are removed from the cache when they are older than this
//! limit (in seconds). Sessions are also removed from the cache if a
//! connection using the session dies unexpectedly.
int session_lifetime = 600;

//! Maximum number of sessions to keep in the cache.
int max_sessions = 300;

/* Queue of pairs (time, id), in cronological order */
ADT.Queue active_sessions = ADT.Queue();

mapping(string:.session) session_cache = ([]);

int session_number; /* Incremented for each session, and used when
		     * constructing the session id */

// Remove sessions older than @[session_lifetime] from the session cache.
void forget_old_sessions()
{
  int t = time() - session_lifetime;
  array pair;
  while ( (pair = [array]active_sessions->peek())
	  && (pair[0] < t)) {
    SSL3_DEBUG_MSG("SSL.context->forget_old_sessions: "
                   "garbing session %O due to session_lifetime limit\n",
                   pair[1]);
    m_delete (session_cache, [string]([array]active_sessions->get())[1]);
  }
}

//! Lookup a session identifier in the cache. Returns the
//! corresponding session, or zero if it is not found or caching is
//! disabled.
.session lookup_session(string id)
{
  if (use_cache)
  {
    return session_cache[id];
  }
  else
    return 0;
}

//! Create a new session.
.session new_session()
{
  .session s = .session();
  s->identity = (use_cache) ?
    sprintf("%4cPikeSSL3%4c", time(), session_number++) : "";
  return s;
}

//! Add a session to the cache (if caching is enabled).
void record_session(.session s)
{
  if (use_cache && s->identity)
  {
    while (sizeof (active_sessions) >= max_sessions) {
      array pair = [array] active_sessions->get();
      SSL3_DEBUG_MSG("SSL.context->record_session: "
                     "garbing session %O due to max_sessions limit\n", pair[1]);
      m_delete (session_cache, [string]pair[1]);
    }
    forget_old_sessions();
    SSL3_DEBUG_MSG("SSL.context->record_session: caching session %O\n",
                   s->identity);
    active_sessions->put( ({ time(1), s->identity }) );
    session_cache[s->identity] = s;
  }
}

//! Remove a session from the cache.
void purge_session(.session s)
{
  SSL3_DEBUG_MSG("SSL.context->purge_session: %O\n", s->identity || "");
  if (s->identity)
    m_delete (session_cache, s->identity);
  /* There's no need to remove the id from the active_sessions queue */
}

protected void create()
{
  SSL3_DEBUG_MSG("SSL.context->create\n");

  /* Backwards compatibility */
  preferred_suites = get_suites(128, 1);
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

    Standards.X509.TBSCertificate cert =
      Standards.X509.decode_certificate(i[-1]);

    trusted_issuers_cache[cert->subject->get_der()] = cert->public_key;
  }
}
