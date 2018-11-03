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

#include "tls.h"

import ".";
import Constants;

protected void create()
{
  SSL3_DEBUG_MSG("SSL.Context->create\n");

  /* Backwards compatibility */
  multiset(int) blocked = (< CIPHER_rc4 >);
  preferred_suites = get_suites(128, 1, blocked);
}

//! List of supported versions, in order of preference. Defaults to
//! @[PROTOCOL_TLS_1_2], @[PROTOCOL_TLS_1_1] and @[PROTOCOL_TLS_1_0].
array(ProtocolVersion) supported_versions = ({
  PROTOCOL_TLS_1_2,
  PROTOCOL_TLS_1_1,
  PROTOCOL_TLS_1_0,
});

//! Returns a list of possible versions to use, given the version in
//! the client hello header.
array(ProtocolVersion) get_versions(ProtocolVersion client)
{
  // Do we support exactly the version the client advertise?
  int pos = search(supported_versions, client);
  if(pos!=-1) return supported_versions[pos..];

  // Client is TLS 1.2 and we have something later supported? Then we
  // expect supported_versions extension.
  int high = max(@supported_versions);
  if( client==PROTOCOL_TLS_1_2 && high > PROTOCOL_TLS_1_2 )
    return ({ PROTOCOL_IN_EXTENSION });

  // Return all versions lower than the given client version.
  return filter(supported_versions, lambda(ProtocolVersion v)
    {
      return v<client;
    });
}

//! List of advertised protocols using using TLS application level
//! protocol negotiation.
array(string(8bit)) advertised_protocols;

//! The maximum amount of data that is sent in each SSL packet by
//! @[File]. A value between 1 and @[Constants.PACKET_MAX_SIZE].
int packet_max_size = PACKET_MAX_SIZE;

//! Lists the supported compression algorithms in order of preference.
//!
//! Defaults to @expr{({ COMPRESSION_null })@} due to SSL attacks that
//! target compression.
array(int) preferred_compressors = ({ COMPRESSION_null });

//! If set enable SSL/TLS protocol renegotiation.
//!
//! Defaults to @expr{1@} (enabled).
//!
//! @note
//!   @rfc{7540:9.2.1@} requires this to be turned off after
//!   @[Protocols.HTTP2] communication has started.
int(0..1) enable_renegotiation = 1;

//! If set, the other peer will be probed for the heartbleed bug
//! during handshake. If heartbleed is found the connection is closed
//! with insufficient security fatal error. Requires
//! @expr{Constants.EXTENSION_heartbeat@} to be set in @[extensions].
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

//! A list of all extensions that will be considered in the handshake
//! process. Extensions not listed will not be sent, and will be
//! ignored if received.
//!
//! The following values are included by default.
//! @int
//!   @value Constants.EXTENSION_renegotiation_info
//!     Protection against renegotiation attack (@rfc{5746@}).
//!   @value Constants.EXTENSION_max_fragment_length
//!     Allows negotiation of the maximum fragment size (@rfc{6066:4@}).
//!   @value Constants.EXTENSION_encrypt_then_mac
//!     Attempts to address attacks against block
//!     ciphers (@rfc{7366@}).
//!   @value Constants.EXTENSION_application_layer_protocol_negotiation
//!     Required to support more than one protocol on the same TLS
//!     port (@rfc{7639@}).
//!   @value Constants.EXTENSION_signature_algorithms
//!     Required to select which out of several certificates to use
//!     (@rfc{5246:7.4.1.4.1@}).
//!   @value Constants.EXTENSION_ec_point_formats
//!     Required for elliptic curve key exchange (@rfc{4492:5.1.2@}).
//!   @value Constants.EXTENSION_elliptic_curves
//!     Required for elliptic curve key exchange (@rfc{4492:5.1.1@}).
//!   @value Constants.EXTENSION_server_name
//!     Allows the client to select which of several domains hosted on
//!     the same server it wants to connect to. Required by many
//!     websites (@rfc{6066:3@}).
//!   @value Constants.EXTENSION_session_ticket
//!     Support session resumption without server-side state
//!     (@rfc{4507@} and @rfc{5077@}).
//!   @value Constants.EXTENSION_next_protocol_negotiation
//!     Not supported by Pike. The server side will just check that
//!     the client packets are correctly formatted.
//!   @value Constants.EXTENSION_signed_certificate_timestamp
//!     Not supported by Pike. The server side will just check that
//!     the client packets are correctly formatted.
//!   @value Constants.EXTENSION_early_data
//!     Needed for TLS 1.3 0-RTT handshake. EXPERIMENTAL.
//!   @value Constants.EXTENSION_padding
//!     This extension is required to avoid a bug in some f5 SSL
//!     terminators for certain sizes of client handshake messages.
//! @endint
//!
//! The following supported values are not included by default.
//! @int
//!   @value Constants.EXTENSION_truncated_hmac
//!     This extension allows for the HMAC to be truncated for a small
//!     win in payload size. Not widely implemented and may be a
//!     security risk (@rfc{6066:7@}).
//!   @value Constants.EXTENSION_heartbeat
//!     This extension allows the client and server to send heartbeats
//!     over the connection. Intended to keep TCP connections
//!     alive. Required to be set to use @[heartbleed_probe]
//!     (@rfc{6520@}).
//!   @value Constants.EXTENSION_extended_master_secret
//!     Binds the master secret to important session parameters to
//!     protect against man in the middle attacks (@rfc{7627@}).
//! @endint
//!
//! @seealso
//!   @rfc{6066@}
multiset(int) extensions = (<
  EXTENSION_renegotiation_info,
  EXTENSION_max_fragment_length,
  EXTENSION_ec_point_formats,
  EXTENSION_encrypt_then_mac,
  EXTENSION_application_layer_protocol_negotiation,
  EXTENSION_signature_algorithms,
  EXTENSION_elliptic_curves,
  EXTENSION_server_name,
  EXTENSION_session_ticket,
  EXTENSION_next_protocol_negotiation,
  EXTENSION_signed_certificate_timestamp,
  EXTENSION_early_data,
  EXTENSION_padding,
>);

//
// --- Cryptography
//

//! Used to generate random cookies for the hello-message. If we use
//! the RSA keyexchange method, and this is a server, this random
//! number generator is not used for generating the master_secret. By
//! default set to @[random_string].
function(int(0..):string(8bit)) random = random_string;

//! Cipher suites we want to support, in order of preference, best
//! first. By default set to all suites with at least 128 bits cipher
//! key length, excluding RC4, and ephemeral and non-ephemeral
//! certificate based key exchange.
array(int) preferred_suites;

//! Supported elliptical curve cipher curves in order of
//! preference. Defaults to all supported curves, ordered with the
//! largest curves first.
array(int) ecc_curves = reverse(sort(indices(ECC_CURVES)));

//! Supported FFDHE groups for DHE key exchanges, in order of preference,
//! most preferred first.
//!
//! Defaults to the full set of supported FFDHE groups from the FFDHE
//! draft, in order of size with the smallest group (2048 bits) first.
//!
//! Server-side the first group in the list that satisfies the NIST guide
//! lines for key strength (NIST SP800-57 5.6.1) (if any) for the selected
//! cipher suite will be selected, and otherwise the largest group.
//!
//! Client-side the list will be reversed (as a precaution if the server
//! actually follows the clients preferences).
array(int) ffdhe_groups = sort(indices(FFDHE_GROUPS));

//! DHE parameter lookup for the FFDHE private range.
//!
//! Add any custom FFDHE-groups here.
//!
//! Defaults to the empty mapping.
//!
//! @note
//!   If you add any groups here, you will also need to update
//!   @[ffdhe_groups] accordingly.
mapping(int(508..511):Crypto.DH.Parameters) private_ffdhe_groups = ([]);

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
//!   According to @rfc{5246:7.4.2@} all certificates needs to be
//!   signed by any of the supported signature algorithms. To be
//!   forward compatible this list needs to be limited to the
//!   combinations that have existing PKCS identifiers.
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
  ({ HASH_sha1, SIGNATURE_ecdsa }),
#endif
  ({ HASH_sha1, SIGNATURE_dsa }),
  ({ HASH_sha1, SIGNATURE_rsa }),
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

  // Adjust for the cipher mode.
  if (sizeof(info) > 3) {
    hash |= info[3]<<5;
    if (info[3] == MODE_cbc) {
      // CBC.
      keylength >>= 1;
    }
  } else {
    // Old suite; CBC or RC4.
    // This adjustment is to make some browsers (like eg Chrome)
    // stop complaining, by preferring AES128/GCM to AES256/CBC.
    keylength >>= 1;
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
    KE_rsa_export:	5,
    KE_dh_rsa:		6,
    KE_dh_dss:		7,
    KE_rsa:		8,
    KE_rsa_fips:	9,
    KE_ecdh_rsa:	10,
    KE_ecdh_ecdsa:	11,
    KE_dhe_rsa:		12,
    KE_dhe_dss:		13,
    KE_ecdhe_rsa:	14,
    KE_ecdhe_ecdsa:	15,
  ])[info[0]];

  int auth_prio = keylength && ([
    KE_null:		0,
    KE_dh_anon:		0,
    KE_ecdh_anon:	0,
    KE_fortezza:	1,
    KE_dms:		2,
    KE_rsa_export:	4,	// cf FREAK-attack.
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
//!       Also allow anonymous server key exchange. Note that this
//!       allows for man in the middle attacks.
//!   @endint
//!
//! @param blacklisted_ciphers
//!   Multiset of ciphers that are NOT to be used. By default RC4, DES
//!   and export ciphers are blacklisted. An empty multiset needs to
//!   be given to unlock these.
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
//!   The list of suites is also filtered on the current settings of
//!   @[supported_versions].
//!
//! @note
//!   Note that the effective keylength may differ from
//!   the actual keylength for old ciphers where there
//!   are known attacks.
array(int) get_suites(int(-1..)|void min_keylength,
                      int(0..2)|void ke_mode,
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

  if (ke_mode) {
    // Static certificate based key exchange methods.
    kes |= (<
      KE_rsa, KE_rsa_export, KE_rsa_fips,
      KE_dh_rsa, KE_dh_dss,
#if constant(Crypto.ECC.Curve)
      KE_ecdh_rsa,
      KE_ecdh_ecdsa,
#endif
    >);
    if (ke_mode == 2) {
      // Unsigned key exchange methods.
      kes |= (< KE_null, KE_dh_anon,
#if constant(Crypto.ECC.Curve)
		KE_ecdh_anon,
#endif
      >);
    }
  }

#if constant(Crypto.ECC.Curve)
  if (!sizeof(ecc_curves)) {
    // No ECC curves available ==> No support for ECC.
    kes -= (<
      KE_ecdhe_rsa, KE_ecdhe_ecdsa,
      KE_ecdh_rsa, KE_ecdh_ecdsa,
      KE_ecdh_anon,
    >);
  }
#endif

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

  if( !blacklisted_ciphers || (max_version >= PROTOCOL_TLS_1_3))
  {
    // Block export ciphers and DES by default because they are
    // demonstrably broken.
    //
    // Block RC4 because it probably is (RFC 7465).
    //
    // TLS 1.3 prohibits RC4.
    if (!blacklisted_ciphers) blacklisted_ciphers = (<>);
    blacklisted_ciphers |= (< CIPHER_rc4, CIPHER_des, CIPHER_rc4_40,
			      CIPHER_rc2_40, CIPHER_des40 >);
  }
  if( sizeof(blacklisted_ciphers) )
      res = filter(res,
                   lambda(int suite, multiset(int) blacklisted_hashes) {
                     return !blacklisted_hashes[CIPHER_SUITES[suite][1]];
                   }, blacklisted_ciphers);

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

  switch(min(@supported_versions)) {
  case PROTOCOL_TLS_1_3:
    res = filter(res,
		 lambda(int suite) {
		   array(int) info = [array(int)]CIPHER_SUITES[suite];
		   // Non-AEAD suites were obsoleted in TLS 1.3.
		   if (sizeof(info) < 4) return 0;
		   if (info[3] == MODE_cbc) return 0;
		   return 1;
		 });
    break;
  case PROTOCOL_TLS_1_2:
    res = filter(res,
		 lambda(int suite) {
		   array(int) info = [array(int)]CIPHER_SUITES[suite];
		   switch(info[1]) {
		   // Export cipher suites were removed in TLS 1.1.
		   case 0:
		   case CIPHER_rc2_40:
		   case CIPHER_rc4_40:
		   case CIPHER_des40:
		   // IDEA and DES suites were removed in TLS 1.2.
		   case CIPHER_idea:
		   case CIPHER_des:
		     return 0;
		   }
		   return 1;
		 });
    break;
  case PROTOCOL_TLS_1_1:
    res = filter(res,
		 lambda(int suite) {
		   array(int) info = [array(int)]CIPHER_SUITES[suite];
		   // Export cipher suites were removed in TLS 1.1.
		   switch(info[1]) {
		   case 0:
		   case CIPHER_rc2_40:
		   case CIPHER_rc4_40:
		   case CIPHER_des40:
		     return 0;
		   }
		   return 1;
		 });
    break;
  }

  switch(max(@supported_versions)) {
  case PROTOCOL_TLS_1_1:
  case PROTOCOL_TLS_1_0:
  case PROTOCOL_SSL_3_0:
    res = filter(res,
		 lambda(int suite) {
		   array(int) info = [array(int)]CIPHER_SUITES[suite];
		   // AEAD suites are not supported in TLS versions
		   // prior to TLS 1.2.
		   // Hashes other than md5 or sha1 are not supported
		   // prior to TLS 1.2.
		   return (sizeof(info) < 4) && (info[2] <= HASH_sha1);
		 });
    break;
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
//! specified by @rfc{6460@} in strict mode.
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
//!       Allow only the Suite B suites from @rfc{6460@} and TLS 1.2.
//!     @value 1
//!       Transitional mode.
//!
//!       Also allow the transitional suites from @rfc{5430@} for use
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
  if( !min_keylength ) min_keylength = 256;
  if (min_keylength!=256)
    error("Only keylength 256 supported.\n");

  preferred_suites = ({
    TLS_ecdhe_ecdsa_with_aes_256_gcm_sha384,
  });

  supported_versions = ({ PROTOCOL_TLS_1_2 });

  if (strictness_level < 2) {
    // Transitional or permissive mode.

    // Allow TLS 1.0.
    supported_versions = ({
      PROTOCOL_TLS_1_2,
      PROTOCOL_TLS_1_1,
      PROTOCOL_TLS_1_0,
    });

    // First add the transitional suites.
    preferred_suites += ({
      TLS_ecdhe_ecdsa_with_aes_256_cbc_sha,
    });

    if (strictness_level < 1) {
      // Permissive mode. Add the remaining suites of
      // the required strength.
      preferred_suites += get_suites(min_keylength) - preferred_suites;
    }
  }
}

#endif /* Crypto.ECC.Curve && Crypto.AES.GCM && Crypto.SHA384 */

//! Called by the KeyExchangeExportRSA during KE_rsa_export key
//! exchanges to get the weak RSA key. By default a new 512 bit key is
//! generated for each key exchange. This method can be overloaded to
//! provide caching or alternative means to generate keys.
Crypto.RSA get_export_rsa_key()
{
  return Crypto.RSA()->generate_key(512);
}

// --- PSK API

// In addition to implementing get_psk, get_psk_id if you are a client
// and optionally get_psk_hint if you are a server, the context object
// also needs to ensure the apprioriate PSK cipher suites are in the
// preferred_suites array. If the server is only accepting these PSK
// connections, simply setting the array to a single member is best.
// The client must only include PSK suites when talking to a servers
// known to support it, or risk getting MITM attacks.

//! A context created for server side PSK use can optionally implement
//! get_psk_hint to return a hint string to be sent to the client. If
//! not implemented, or returning 0, no PSK hint will be sent.
optional string(8bit) get_psk_hint();

//! A context created for client side PSK use must implement a
//! get_psk_id method, which will be called with the server provided
//! hint, or 0 if no hint was sent. Note that while there is an API
//! difference between no hint and a zero length hint, some PSK modes
//! are unable to send no hints.
//!
//! The method should return a key id
//! for the PSK, which will be sent to the server. If the hint is not
//! valid, 0 should be returned.
optional string(8bit) get_psk_id(string(8bit) hint);

//! A context created for PSK use must implement a get_psk method,
//! which will be called with the key id, and should return the key to
//! be used for the connection. If the id is not valid, 0 should be
//! returned.
optional string(8bit) get_psk(string(8bit) id);

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
// FIXME: Currently only one client certificate per der issuer is
// supported. If multiple are added a random one will be selected,
// which later may fail when verified against supported certificate
// types, hash/signature algorithms.
//
// FIXME: There is no need to allow the same context object to be used
// both for client and server side, so we could join
// cert_chains_domain and cert_chains_issuer into one system.
//
// The client/server potentially has a set of trusted issuers
// certificates (root certificates) that are used to validate the
// server/client sent certificate. These are stored in trusted_issuers
// and in a cache from subject der to Verifier object. FIXME: Should
// use key identifier.

//! Policy for client authentication. One of
//! @[SSL.Constants.AUTHLEVEL_none],
//! @[SSL.Constants.AUTHLEVEL_verify], @[SSL.Constants.AUTHLEVEL_ask]
//! and @[SSL.Constants.AUTHLEVEL_require].
//!
//! Defaults to SSL.Constants.AUTHLEVEL_none.
int auth_level = AUTHLEVEL_none;

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

//! Get the list of allowed authorities. See @[set_authorities].
array(string) get_authorities()
{
  return authorities;
}

protected array(string) authorities = ({});
array(string(8bit)) authorities_cache = ({});

//! Sets the list of trusted certificate issuers.
//!
//! @param issuers
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
void set_trusted_issuers(array(array(string(8bit))) issuers)
{
  trusted_issuers = issuers;
  update_trusted_issuers();
}

//! Get the list of trusted issuers. See @[set_trusted_issuers].
array(array(string(8bit))) get_trusted_issuers()
{
  return trusted_issuers;
}

protected array(array(string(8bit))) trusted_issuers = ({});

//! Mapping from DER-encoded issuer to @[Standards.X509.Verifier]s
//! compatible with eg @[Standards.X509.verify_certificate()] and
//! @[Standards.X509.load_authorities()].
//!
//! @seealso
//!   @[get_trusted_issuers()], @[set_trusted_issuers()]
mapping(string(8bit):array(Standards.X509.Verifier)) trusted_issuers_cache = ([]);

//! The possible client authentication methods. Used only if
//! auth_level is AUTH_ask or AUTH_require. Generated by
//! @[set_authorities].
array(int) client_auth_methods = ({});

// Lookup from issuer DER to an array of suitable @[CertificatePair]s,
// sorted in order of strength.
protected mapping(string(8bit):array(CertificatePair)) cert_chains_issuer = ([]);

// Lookup from DN/SNI domain name/glob to an array of suitable
// @[CertificatePair]s, sorted in order of strength.
protected mapping(string(8bit):array(CertificatePair)) cert_chains_domain = ([]);

//! Look up a suitable set of certificates for the specified issuer.
//! @[UNDEFIEND] if no certificate was found. Called only by the
//! ClientConnection as a response to a certificate request.
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
//! @[UNDEFINED] if no certificate was found. Called only by the
//! Server.
array(CertificatePair) find_cert_domain(string(8bit) domain)
{
  if( domain )
  {
    if( cert_chains_domain[domain] )
      return cert_chains_domain[domain];

    // Return first matching chain that isn't a fallback certificate.
    foreach(cert_chains_domain; string g; array(CertificatePair) chains)
      if( (g != "*") && glob(g, domain) )
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
  mapping(int:int) cert_types = ([]);
  foreach(authorities, string a)
  {
    Standards.X509.TBSCertificate tbs = Standards.X509.decode_certificate(a);
    Standards.ASN1.Types.Identifier id = [object(Standards.ASN1.Types.Identifier)]tbs->algorithm[0];

    // --- START Duplicated code from CertificatePair
    array(HashAlgorithm|SignatureAlgorithm) sign_alg;
    sign_alg = [array(HashAlgorithm|SignatureAlgorithm)]pkcs_der_to_sign_alg[id->get_der()];
    if (!sign_alg) error("Unknown signature algorithm.\n");

    int cert_type = ([
      SIGNATURE_rsa: AUTH_rsa_sign,
      SIGNATURE_dsa: AUTH_dss_sign,
      SIGNATURE_ecdsa: AUTH_ecdsa_sign,
    ])[sign_alg[1]];
    // --- END Duplicated code from CertificatePair

    cert_types[cert_type]++;
    authorities_cache += ({ tbs->subject->get_der() });
  }
  client_auth_methods = indices(cert_types);
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

//! Sessions are removed from the cache when they have been inactive
//! more than this number of seconds. Sessions are also removed from
//! the cache if a connection using the session dies unexpectedly.
int session_lifetime = 600;

//! Maximum number of sessions to keep in the cache.
int max_sessions = 300;

mapping(string:Session) session_cache = ([]);

// Remove sessions older than @[session_lifetime] from the session cache.
void forget_old_sessions()
{
  int t = time() - session_lifetime;
  foreach(session_cache; string id; Session session)
  {
    if(session->last_activity < t)
    {
      SSL3_DEBUG_MSG("SSL.Context->forget_old_sessions: "
                     "garbing session %O due to session_lifetime limit\n",
                     id);
      m_delete (session_cache, id);
    }
  }
}

//! Lookup a session identifier in the cache. Returns the
//! corresponding session, or zero if it is not found or caching is
//! disabled.
Session lookup_session(string id)
{
  if (use_cache)
    return session_cache[id];
  else
    return 0;
}

//! Decode a session ticket and return the corresponding session
//! if valid or zero if invalid.
//!
//! @note
//!   The default implementation just calls @[lookup_session()].
//!
//!   Override this function (and @[encode_ticket()]) to implement
//!   server-side state-less session resumption.
//!
//! @seealso
//!   @[encode_ticket()], @[lookup_session()]
Session decode_ticket(string(8bit) ticket)
{
  return lookup_session(ticket);
}

//! Generate a session ticket for a session.
//!
//! @note
//!   The default implementation just generates a random ticket
//!   and calls @[record_session()] to store it.
//!
//!   Over-ride this function (and @[decode_ticket()]) to implement
//!   server-side state-less session resumption.
//!
//! @returns
//!   Returns @expr{0@} (zero) on failure (ie cache disabled), and
//!   an array on success:
//!   @array
//!     @elem string(8bit) 0
//!       Non-empty string with the ticket.
//!     @elem int 1
//!       Lifetime hint for the ticket.
//!   @endarray
//!
//! @seealso
//!   @[decode_ticket()], @[record_session()], @rfc{4507:3.3@}
array(string(8bit)|int) encode_ticket(Session session)
{
  if (!use_cache) return 0;
  string(8bit) ticket = session->ticket;
  if (!sizeof(ticket||"")) {
    do {
      ticket = random(32);
    } while(session_cache[ticket]);
    // FIXME: Should we update the fields here?
    //        Consider moving this to the caller.
    session->ticket = ticket;
    session->ticket_expiry_time = time(1) + 3600;
  }
  string(8bit) orig_id = session->identity;
  session->identity = ticket;
  record_session(session);
  session->identity = orig_id;
  // FIXME: Calculate the lifetime from the ticket_expiry_time field?
  return ({ ticket, 3600 });
}

//! Create a new session.
Session new_session()
{
  string(8bit) id = "";
  if(use_cache)
    do {
      id = random(32);
    } while( session_cache[id] );

  Session s = Session(id);
  s->ffdhe_groups = ffdhe_groups;

  return s;
}

//! Add a session to the cache (if caching is enabled).
void record_session(Session s)
{
  if (use_cache && sizeof(s->identity||""))
  {
    if( sizeof(session_cache) > max_sessions )
    {
      forget_old_sessions();
      int to_delete = sizeof(session_cache)-max_sessions;
      foreach(session_cache; string id;)
      {
        // Randomly delete sessions to keep within the limit.
        if( to_delete-- < 0 ) break;
        SSL3_DEBUG_MSG("SSL.Context->record_session: "
                       "garbing session %O due to max_sessions limit\n", id);
        m_delete (session_cache, id);
      }
    }
    SSL3_DEBUG_MSG("SSL.Context->record_session: caching session %O\n",
                   s->identity);
    session_cache[s->identity] = s;
  }
}

//! Invalidate a session for resumption and remove it from the cache.
void purge_session(Session s)
{
  SSL3_DEBUG_MSG("SSL.Context->purge_session: %O\n", s->identity || "");
  if (s->identity)
    m_delete (session_cache, s->identity);
  /* RFC 4346 7.2:
   *   In this case [fatal alert], other connections corresponding to
   *   the session may continue, but the session identifier MUST be
   *   invalidated, preventing the failed session from being used to
   *   establish new connections.
   */
  s->identity = 0;
  if (s->version > PROTOCOL_TLS_1_2) {
    // In TLS 1.2 and earlier the master_secret may be shared
    // between multiple concurrent connections (cf eg above),
    // so we can't scratch the master secret.
    s->master_secret = 0;
  }
}


//
// --- Compatibility code
//

//! @decl int verify_certificates
//!
//! Determines whether certificates presented by the peer are
//! verified, or just accepted as being valid.
//!
//! @deprecated auth_level

__deprecated__ void `verify_certificates=(int i)
{
  if(!i)
    auth_level = AUTHLEVEL_none;
  else if(auth_level < AUTHLEVEL_verify)
    auth_level = AUTHLEVEL_verify;
}

__deprecated__ int `verify_certificates()
{
  return auth_level >= AUTHLEVEL_verify;
}

//! @decl int require_trust
//!
//! When set, require the chain to be known, even if the root is self
//! signed.
//!
//! Note that if set, and certificates are set to be verified, trusted
//! issuers must be provided, or no connections will be accepted.
//!
//! @deprecated auth_level

__deprecated__ void `require_trust=(int i)
{
  if(i)
    auth_level = AUTHLEVEL_require;
  else if(auth_level > AUTHLEVEL_verify)
    auth_level = AUTHLEVEL_verify;
}

__deprecated__ int `require_trust()
{
  return auth_level >= AUTHLEVEL_require;
}

//! @decl int(0..1) encrypt_then_mac
//!
//! Attempt to enable encrypt-then-mac mode. Defaults to @expr{1@}.
//!
//! @deprecated extensions

__deprecated__ void `encrypt_then_mac=(int(0..1) i)
{
  extensions[EXTENSION_encrypt_then_mac] = !!i;
}

__deprecated__ int(0..1) `encrypt_then_mac()
{
  return !!extensions[EXTENSION_encrypt_then_mac];
}

//! @decl int min_version
//! @decl int max_version
//!
//! The accepted range of versions for the client/server. List
//! specific versions in @[supported_versions] instead.
//!
//! @deprecated supported_versions

ProtocolVersion `min_version()
{
  return min(@supported_versions);
}

ProtocolVersion `max_version()
{
  return max(@supported_versions);
}

protected void generate_versions(ProtocolVersion min, ProtocolVersion max)
{
  supported_versions = [array(int(16bit))]reverse(enumerate([int(16bit)](max-min+1), 1, min));
}

void `min_version=(ProtocolVersion version)
{
  ProtocolVersion m = max_version;
  if( version > m )
    supported_versions = ({ version });
  else
    generate_versions(version, m);
}

void `max_version=(ProtocolVersion version)
{
  ProtocolVersion m = min_version;
  if( version < m )
    supported_versions = ({ version });
  else
    generate_versions(m, version);
}
