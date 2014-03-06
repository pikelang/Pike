#pike __REAL_VERSION__
#pragma strict_types
#require constant(Crypto.Hash)

//! Keeps the state that is shared by all SSL-connections for one
//! server (or one port). It includes policy configuration, a server
//! certificate, the server's private key(s), etc. It also includes
//! the session cache.

#ifdef SSL3_DEBUG
#define SSL3_DEBUG_MSG(X ...)  werror(X)
#else /*! SSL3_DEBUG */
#define SSL3_DEBUG_MSG(X ...)
#endif /* SSL3_DEBUG */

import .Constants;

//! The server's default private key.
//!
//! Supported key types are currently:
//! @mixed
//!   @type Crypto.RSA
//!     Rivest-Shamir-Adelman.
//!   @type Crypto.DSA
//!     Digital Signing Algorithm.
//!   @type Crypto.ECC.Curve.ECDSA
//!     Elliptic Curve Digital Signing Algorithm.
//! @endmixed
//!
//! This key MUST match the public key in the first certificate
//! in @[certificates].
//! 
//! @note
//!   If SNI (Server Name Indication) is used and multiple keys are
//!   available, this key will not be used, instead the appropriate
//!   SNI key will be used (the default implementation stores these in
//!   @[sni_keys].
Crypto.Sign private_key;

//! Compatibility.
//! @deprecated private_key
__deprecated__ Crypto.RSA `rsa()
{
  return private_key && (private_key->name() == "RSA") &&
    [object(Crypto.RSA)]private_key;
}

//! Compatibility.
//! @deprecated private_key
__deprecated__ void `rsa=(Crypto.RSA k)
{
  private_key = k;
}

//! Should an SSL client include the Server Name extension?
//!
//! If so, then client_server_names should specify the values to send.
int client_use_sni = 0;

//! Host names to send to the server when using the Server Name
//! extension.
array(string(8bit)) client_server_names = ({});

/* For client authentication */

//! The client's private key (used with client certificate
//! authentication)
Crypto.Sign client_public_key;

//! Compatibility.
//! @deprecated client_public_key
__deprecated__ Crypto.RSA `client_rsa()
{
  return client_public_key && (client_public_key->name() == "RSA") &&
    [object(Crypto.RSA)]client_public_key;
}

//! Compatibility.
//! @deprecated client_public_key
__deprecated__ void `client_rsa=(Crypto.RSA k)
{
  client_public_key = k;
}

//! An array of certificate chains a client may present to a server
//! when client certificate authentication is requested.
array(array(string(8bit))) client_certificates = ({});

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

//! Temporary, non-certified, private keys, used with a
//! server_key_exchange message. The rules are as follows:
//!
//! If the negotiated cipher_suite has the "exportable" property, and
//! short_rsa is not zero, send a server_key_exchange message with the
//! (public part of) the short_rsa key.
//!
//! If the negotiated cipher_suite does not have the exportable
//! property, and long_rsa is not zero, send a server_key_exchange
//! message with the (public part of) the long_rsa key.
//!
//! Otherwise, dont send any server_key_exchange message.
Crypto.RSA long_rsa;
Crypto.RSA short_rsa;

//! Compatibility.
//! @deprecated private_key
__deprecated__ Crypto.DSA `dsa()
{
  return private_key && (private_key->name() == "DSA") && 
    [object(Crypto.DSA)]private_key;
}

//! Compatibility.
//! @deprecated private_key
__deprecated__ void `dsa=(Crypto.DSA k)
{
  private_key = k;
}

//! Parameters for dh keyexchange.
.Cipher.DHKeyExchange dh_ke;

//! Used to generate random cookies for the hello-message. If we use
//! the RSA keyexchange method, and this is a server, this random
//! number generator is not used for generating the master_secret. By
//! default set to @[Crypto.Random.random_string].
function(int:string(8bit)) random = Crypto.Random.random_string;

//! The server's certificate, or a chain of X509.v3 certificates, with
//! the server's certificate first and root certificate last.
array(string(8bit)) certificates;

//! A mapping containing certificate chains for use by SNI (Server
//! Name Indication). Each entry should consist of a key indicating
//! the server hostname and the value containing the certificate chain
//! for that hostname.
mapping(string:array(string(8bit))) sni_certificates = ([]);

//! A mapping containing private keys for use by SNI (Server Name
//! Indication). Each entry should consist of a key indicating the
//! server hostname and the value containing the private key object
//! for that hostname.
//!
//! @note
//!  keys objects may be generated from a decoded key string using
//!  @[Standards.PKCS.RSA.parse_private_key()].
mapping(string:object(Crypto.Sign)) sni_keys = ([]);

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

//! Add a certificate.
//!
//! @param key
//!   Private key matching the first certificate in @[certs].
//!
//! @param certs
//!  A chain of X509.v1 or X509.v3 certificates, with the local
//!  certificate first and root-most certificate last.
//!
//! @note
//!   Currently this replaces the default certificate,
//!   (if any) but in the future it may support having
//!   multiple certificates and certificate selection
//!   depending on parameters supplied by the peer.
//!
//! @fixme
//!   The function does currently NOT validate that the @[key]
//!   matches the first entry of @[certs], or that the list
//!   of @[certs] is valid, but may do so in the future.
void add_cert(Crypto.Sign key, array(string(8bit)) certs)
{
  if (!sizeof(certs)) {
    error("Empty list of certificates.\n");
  }
  private_key = key;
  certificates = certs;
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
    KE_dh:		6,
    KE_ecdh_rsa:	7,
    KE_ecdh_ecdsa:	8,
    KE_dhe_rsa:		9,
    KE_dhe_dss:		10,
    KE_ecdhe_rsa:	11,
    KE_ecdhe_ecdsa:	12,
  ])[info[0]];

  int auth_prio = ([
    KE_null:		0,
    KE_dh:		0,
    KE_dh_anon:		0,
    KE_ecdh_anon:	0,
    KE_fortezza:	1,
    KE_dms:		2,
    KE_rsa:		8,
    KE_ecdh_rsa:	8,
    KE_dhe_rsa:		8,
    KE_ecdhe_rsa:	8,
    KE_dhe_dss:		8,
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
//! @param sign
//!   Signature algorithm, typically @[SIGNATURE_rsa] or
//!   @[SIGNATURE_dsa].
//!
//! @param min_keylength
//!   Minimum supported effective keylength.
//!
//! @note
//!   Note that the effective keylength may differ from
//!   the actual keylength for old ciphers where there
//!   are known attacks.
array(int) get_suites(int sign, int min_keylength, int|void max_version)
{
  // Default to the unsigned key exchange methods.
  multiset(int) kes = (< KE_null, KE_dh, KE_dh_anon,
#if constant(Crypto.ECC.Curve)
			 KE_ecdh_anon,
#endif
  >);

  // Add the signature-dependent methods.
  switch(sign) {
  case SIGNATURE_rsa:
    kes |= (< KE_rsa, KE_dhe_rsa,
#if constant(Crypto.ECC.Curve)
	      KE_ecdh_rsa, KE_ecdhe_rsa,
#endif
    >);
    break;
  case SIGNATURE_dsa:
    kes |= (< KE_dhe_dss >);
    break;
#if constant(Crypto.ECC.Curve)
  case SIGNATURE_ecdsa:
    kes |= (< KE_ecdh_ecdsa, KE_ecdhe_ecdsa >);
    break;
#endif
  }

  // Filter unsupported key exchange methods.
  array(int) res =
    filter(indices(CIPHER_SUITES),
	   lambda(int suite) {
	     return kes[CIPHER_SUITES[suite][0]];
	   });

  // Filter short effective key lengths.
  if (min_keylength) {
    res = filter(res,
		 lambda(int suite, int min_keylength) {
		   return min_keylength <=
		     CIPHER_effective_keylengths[CIPHER_SUITES[suite][1]];
		 }, min_keylength);
  }

  if (!zero_type(max_version) && (max_version < PROTOCOL_TLS_1_2)) {
    // AEAD protocols are not supported prior to TLS 1.2.
    // Variant cipher-suite dependent prfs are not supported prior to TLS 1.2.
    res = filter(res,
		 lambda(int suite) {
		   return (sizeof(CIPHER_SUITES[suite]) < 4);
		 });
  }

#if !constant(Crypto.SHA384)
  // Filter suites needing SHA384 as our Nettle doesn't support it.
  res = filter(res,
	       lambda(int suite) {
		 return (CIPHER_SUITES[suite][2] != HASH_sha384);
	       });
#endif

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
void rsa_mode(int|void min_keylength, int|void max_version)
{
  SSL3_DEBUG_MSG("SSL.context: rsa_mode()\n");
  preferred_suites = get_suites(SIGNATURE_rsa, min_keylength, max_version);
}

//! Set @[preferred_suites] to DSS based methods.
//!
//! @param min_keylength
//!   Minimum acceptable key length in bits.
//!
//! @seealso
//!   @[rsa_mode()], @[ecdsa_mode()], @[filter_weak_suites()]
void dhe_dss_mode(int|void min_keylength, int|void max_version)
{
  SSL3_DEBUG_MSG("SSL.context: dhe_dss_mode()\n");
  preferred_suites = get_suites(SIGNATURE_dsa, min_keylength, max_version);
}

//! Set @[preferred_suites] to ECDSA based methods.
//!
//! @param min_keylength
//!   Minimum acceptable key length in bits.
//!
//! @seealso
//!   @[rsa_mode()], @[dhe_dss_mode()], @[filter_weak_suites()]
void ecdsa_mode(int|void min_keylength, int|void max_version)
{
  SSL3_DEBUG_MSG("SSL.context: ecdsa_mode()\n");
  preferred_suites = get_suites(SIGNATURE_ecdsa, min_keylength, max_version);
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
  rsa_mode(128);
}

// update the cached decoded authorities list
private void update_authorities()
{
  authorities_cache = ({});
  foreach(authorities, string a)
    authorities_cache += ({ Standards.X509.decode_certificate(a)->
                            subject->get_der() });
}

private array(string(8bit))
  internal_select_server_certificate(.context context,
				     array(string(8bit)) server_names)
{
  array(string(8bit)) certs;

  if(server_names && sizeof(server_names))
  {
    foreach(server_names;; string(8bit) sn)
    {
      if(context->sni_certificates && (certs = context->sni_certificates[lower_case(sn)]))
        return certs;
    }
  }

  return 0;
}

private Crypto.Sign internal_select_server_key(.context context,
                                               array(string) server_names)
{
  Crypto.Sign key;

  if(server_names && sizeof(server_names))
  {
    foreach(server_names;; string sn)
    {
      if(context->sni_keys && (key = context->sni_keys[lower_case(sn)]))
        return key;
    }
  }

  return 0;
}

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
