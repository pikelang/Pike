
#pike __REAL_VERSION__

// Enable SSL3_EXPERIMENTAL to get cipher suites that have not been
// validated against other implementations.
//#define SSL3_EXPERIMENTAL

//! Protocol constants

//! Constants for specifying the versions of SSL/TLS to use.
//!
//! @seealso
//!   @[Context]
enum ProtocolVersion {
  PROTOCOL_SSL_3_0	= 0x300, //! SSL 3.0 - The original SSL3 draft version.
  PROTOCOL_TLS_1_0	= 0x301, //! TLS 1.0 - The @rfc{2246@} version of TLS.
  PROTOCOL_TLS_1_1	= 0x302, //! TLS 1.1 - The @rfc{4346@} version of TLS.
  PROTOCOL_TLS_1_2	= 0x303, //! TLS 1.2 - The @rfc{5246@} version of TLS.
  PROTOCOL_TLS_1_3      = 0x304, //! TLS 1.3 - The @rfc{8446@} version of TLS.

  PROTOCOL_DTLS_1_0	= 0xfeff, //! DTLS 1.0 - The @rfc{4347@}
				  //! version of DTLS.  This is
				  //! essentially TLS 1.1 over UDP.

  PROTOCOL_DTLS_1_2	= 0xfefd, //! DTLS 1.2 - The @rfc{6347@}
				  //! version of DTLS.  This is
                                  //! essentially TLS 1.2 over UDP.
  PROTOCOL_IN_EXTENSION = 0xffff, //! Pike internal marker
}

//! Max supported TLS version.
constant PROTOCOL_TLS_MAX = PROTOCOL_TLS_1_2;

//! Packet types.
enum PacketType {
  PACKET_change_cipher_spec = 20, //! @rfc{5246@}
  PACKET_alert              = 21, //! @rfc{5246@}
  PACKET_handshake          = 22, //! @rfc{5246@}
  PACKET_application_data   = 23, //! @rfc{5246@}
  PACKET_heartbeat          = 24, //! @rfc{6520@}
}

constant PACKET_types = (< PACKET_change_cipher_spec,
			   PACKET_alert,
			   PACKET_handshake,
			   PACKET_application_data,
			   PACKET_heartbeat,
>);

//! Maximum SSL packet size (@expr{0x4000@}) in bytes.
constant PACKET_MAX_SIZE = 0x4000;	// 2^14.

//! Handshake states.
enum HandshakeState {
  STATE_wait_for_hello		= 0,
  STATE_wait_for_peer		= 2,
  STATE_wait_for_verify		= 3,
  STATE_wait_for_ticket		= 4,
  STATE_wait_for_finish		= 5,
  STATE_handshake_finished	= 6,
}

//! Connection states.
//!
//! These are the states that a [Connection] may have.
//!
//! Queueing of more application data is only
//! allowed in the states @[CONNECTION_ready] and
//! @[CONNECTION_handshaking].
enum ConnectionState {
  CONNECTION_ready		= 0x0000,	//! Connection is ready for use.

  // Handshaking.
  CONNECTION_handshaking	= 0x0100,	//! Handshaking not done.

  // Peer.
  CONNECTION_peer_closed	= 0x0001,	//! Peer has closed the connection.
  CONNECTION_peer_fatal		= 0x0002,	//! Peer has issued a fatal alert.

  // Local.
  CONNECTION_local_closed	= 0x0010,	//! Local close packet sent.
  CONNECTION_local_fatal	= 0x0020,	//! Fatal alert sent.
  CONNECTION_local_closing	= 0x0040,	//! Local close packet pending.
  CONNECTION_local_failing	= 0x0080,	//! Fatal alert pending.

  // Some composite values.
  CONNECTION_closed		= 0x0011,	//! Closed at both ends.
  CONNECTION_closing		= 0x0051,	//! Connection closing mask.

  CONNECTION_peer_down		= 0x000f,	//! Peer mask.
  CONNECTION_local_down		= 0x00f0,	//! Local mask.

  CONNECTION_failing		= 0x00a2,	//! Connection failing mask.
};

/* Cipher specification */
//! Cipher types.
enum CipherType {
  CIPHER_stream   = 0,
  CIPHER_block    = 1,
  CIPHER_aead     = 2,
}

constant CIPHER_types = (< CIPHER_stream, CIPHER_block, CIPHER_aead >);

//! Cipher algorithms.
enum CipherAlgorithm {
  CIPHER_null		= 0,
  CIPHER_rc4_40		= 2,
  CIPHER_rc2_40		= 3,
  CIPHER_des40		= 6,
  CIPHER_rc4		= 1,
  CIPHER_des		= 4,
  CIPHER_3des		= 5,
  CIPHER_fortezza	= 7,
  CIPHER_idea		= 8,
  CIPHER_aes		= 9,
  CIPHER_aes256		= 10,
  CIPHER_camellia128	= 11,
  CIPHER_camellia256	= 12,
  CIPHER_chacha20	= 13,
}

//! Mapping from @[CipherAlgorithm] to effective key length.
constant CIPHER_effective_keylengths = ([
  CIPHER_null:		0,
  CIPHER_rc2_40:	16,	// A 64bit key in RC2 has strength ~34...
  CIPHER_rc4_40:	24,	// Estimated from plain rc4.
  CIPHER_des40:		32,	// A 56bit key in DES has strength ~40...
  CIPHER_rc4:		38,	// @rfc{7465@}: 13*2^30 encryptions.
  CIPHER_des:		40,
  CIPHER_3des:		112,
  CIPHER_fortezza:	96,
  CIPHER_idea:		128,	// 126.1 bits with bicliques attack.
  CIPHER_aes:		128,	// 126.1 bits with bicliques attack.
  CIPHER_aes256:	256,	// 254.4 bits with bicliques attack.
  CIPHER_camellia128:	128,
  CIPHER_camellia256:	256,
  CIPHER_chacha20:	256,
]);

//! Hash algorithms as per @rfc{5246:7.4.1.4.1@}.
enum HashAlgorithm {
  HASH_none	= 0x0000,
  HASH_md5	= 0x0100,
  HASH_sha1	= 0x0200,
  HASH_sha224	= 0x0300,
  HASH_sha256	= 0x0400,
  HASH_sha384	= 0x0500,
  HASH_sha512	= 0x0600,
  HASH_intrinsic= 0x0800,

  HASH_MASK	= 0xff00,
}

// Compatibility with Pike 8.0
constant HASH_sha = HASH_sha1;

//! Cipher operation modes.
enum CipherMode {
  MODE_cbc	= 0,	//! CBC - Cipher Block Chaining mode.
  MODE_ccm_8	= 1,	//! CCM - Counter with 8 bit CBC-MAC mode.
  MODE_ccm	= 2,	//! CCM - Counter with CBC-MAC mode.
  MODE_gcm	= 3,	//! GCM - Galois Cipher Mode.
  MODE_poly1305	= 4,	//! Poly1305 - Used only with ChaCha20.
}

//! Lookup from @[HashAlgorithm] to corresponding @[Crypto.Hash].
constant HASH_lookup = ([
#if constant(Crypto.SHA512)
  HASH_sha512: Crypto.SHA512,
#endif
#if constant(Crypto.SHA384)
  HASH_sha384: Crypto.SHA384,
#endif
  HASH_sha256: Crypto.SHA256,
#if constant(Crypto.SHA224)
  HASH_sha224: Crypto.SHA224,
#endif
  HASH_sha1:   Crypto.SHA1,
  HASH_md5:    Crypto.MD5,
]);

//! Signature algorithms from TLS 1.2.
enum SignatureAlgorithm {
  SIGNATURE_anonymous	= 0,	//! No signature.
  SIGNATURE_rsa		= 1,	//! RSASSA PKCS1 v1.5 signature.
  SIGNATURE_dsa		= 2,	//! DSS signature.
  SIGNATURE_ecdsa	= 3,	//! ECDSA signature.

  SIGNATURE_rsa_pss_256	= 4,	//! RSA PSS signature with 256 bit hash.
  SIGNATURE_rsa_pss_384	= 5,	//! RSA PSS signature with 384 bit hash.
  SIGNATURE_rsa_pss_512	= 6,	//! RSA PSS signature with 512 bit hash.

  SIGNATURE_ed25519	= 7,	//! EdDSA 25519 signature.
  SIGNATURE_ed448	= 8,	//! EdDSA 448 signature.

  SIGNATURE_MASK	= 0x00ff,
}

//! Signature algorithms from TLS 1.3.
enum SignatureScheme {
  //! RSASSA-PKCS1-v1_5 algorithms.
  SIGNATURE_rsa_pkcs1_sha256 = 0x0401,
  SIGNATURE_rsa_pkcs1_sha384 = 0x0501,
  SIGNATURE_rsa_pkcs1_sha512 = 0x0601,

  //! ECDSA algorithms.
  SIGNATURE_ecdsa_secp256r1_sha256 = 0x0403,
  SIGNATURE_ecdsa_secp384r1_sha384 = 0x0503,
  SIGNATURE_ecdsa_secp521r1_sha512 = 0x0603,

  //! RSASSA-PSS algorithms.
  SIGNATURE_rsa_pss_sha256 = 0x0804,
  SIGNATURE_rsa_pss_sha384 = 0x0805,
  SIGNATURE_rsa_pss_sha512 = 0x0806,

  //! EdDSA algorithms.
  SIGNATURE_ed25519_intrinsic = 0x0807,
  SIGNATURE_ed448_intrinsic = 0x0808,

  //! Legacy algorithms.
  SIGNATURE_rsa_pkcs1_sha1 = 0x0201,
  SIGNATURE_ecdsa_sha1 = 0x0203,

  //! Reserved: @expr{0xfe00@}..@expr{0xffff@}.
  SIGNATURE_reserved_start = 0xfe00,
  SIGNATURE_reserved_end = 0xffff,
}

//! Key exchange methods.
enum KeyExchangeType {
  KE_null	= 0,	//! None.
  KE_rsa	= 1,	//! Rivest-Shamir-Adelman
  KE_rsa_export	= 2,	//! Rivest-Shamir-Adelman (EXPORT)
  KE_dh_dss	= 3,	//! Diffie-Hellman cert signed with DSS
  KE_dh_rsa	= 4,	//! Diffie-Hellman cert signed with RSA
  KE_dhe_dss	= 5,	//! Diffie-Hellman Ephemeral DSS
  KE_dhe_rsa	= 6,	//! Diffie-Hellman Ephemeral RSA
  KE_dh_anon	= 7,	//! Diffie-Hellman Anonymous
  KE_dms	= 8,
  KE_fortezza	= 9,

  // The following five are from RFC 4492.
  KE_ecdh_ecdsa = 10,	//! Elliptic Curve DH cert signed with ECDSA (@rfc{4492@}).
  KE_ecdhe_ecdsa= 11,	//! Elliptic Curve DH Ephemeral with ECDSA (@rfc{4492@}).
  KE_ecdh_rsa   = 12,	//! Elliptic Curve DH cert signed with RSA (@rfc{4492@}).
  KE_ecdhe_rsa  = 13,	//! Elliptic Curve DH Ephemeral with RSA (@rfc{4492@}).
  KE_ecdh_anon  = 14,	//! Elliptic Curve DH Anonymous (@rfc{4492@}).

  // The following three are from RFC 4279.
  KE_psk	= 15,	//! Pre-shared Key (@rfc{4279@}).
  KE_dhe_psk	= 16,	//! Pre-shared Key with DHE (@rfc{4279@}).
  KE_rsa_psk	= 17,	//! Pre-shared Key signed with RSA (@rfc{4279@}).

  // This is from RFC 5489.
  KE_ecdhe_psk  = 18,   //! Pre-shared Key with ECDHE (@rfc{5489@}).

  // The following three are from RFC 5054.
  KE_srp_sha	= 19,	//! Secure Remote Password (SRP) (@rfc{5054@}).
  KE_srp_sha_rsa= 20,	//! SRP signed with RSA (@rfc{5054@}).
  KE_srp_sha_dss= 21,	//! SRP signed with DSS (@rfc{5054@}).

  // This was used during SSL 3.0 to test TLS 1.0.
  KE_rsa_fips	= 22,	//! Rivest-Shamir-Adelman with FIPS keys.
}

constant KE_ecc_mask = (1<<KE_ecdh_ecdsa)|(1<<KE_ecdhe_ecdsa)|
  (1<<KE_ecdh_rsa)|(1<<KE_ecdhe_rsa)|(1<<KE_ecdh_anon);

//! Lists @[KeyExchangeType] that doesn't require certificates.
constant KE_Anonymous = (<
  KE_null,
  KE_dh_anon,
  KE_ecdh_anon,
  KE_psk,
  KE_dhe_psk,
  KE_ecdhe_psk,
>);

//! Compression methods.
enum CompressionType {
  COMPRESSION_null = 0,		//! No compression.
  COMPRESSION_deflate = 1,	//! Deflate compression. @rfc{3749@}
  COMPRESSION_lzs = 64,		//! LZS compression. @rfc{3943@}
}

/* Signature context strings. */
constant SIGN_server_certificate_verify =
  " "*64 + "TLS 1.3, server CertificateVerify\0";
constant SIGN_client_certificate_verify =
  " "*64 + "TLS 1.3, client CertificateVerify\0";

/* Alert messages */
//! Alert message levels.
enum AlertLevel {
  ALERT_warning	= 1,	//! Warning alert.
  ALERT_fatal	= 2,	//! Error alert.
}

constant ALERT_levels = (< ALERT_warning, ALERT_fatal >);

//! Alert messages
enum AlertMessage {
  ALERT_close_notify			= 0,	//! SSL 3.0
  ALERT_unexpected_message		= 10,	//! SSL 3.0
  ALERT_bad_record_mac			= 20,	//! SSL 3.0
  ALERT_decryption_failed		= 21,	//! TLS 1.0
  ALERT_record_overflow			= 22,	//! TLS 1.0
  ALERT_decompression_failure		= 30,	//! SSL 3.0
  ALERT_handshake_failure		= 40,	//! SSL 3.0
  ALERT_no_certificate			= 41,	//! SSL 3.0
  ALERT_bad_certificate			= 42,	//! SSL 3.0
  ALERT_unsupported_certificate		= 43,	//! SSL 3.0
  ALERT_certificate_revoked		= 44,	//! SSL 3.0
  ALERT_certificate_expired		= 45,	//! SSL 3.0
  ALERT_certificate_unknown		= 46,	//! SSL 3.0
  ALERT_illegal_parameter		= 47,	//! SSL 3.0
  ALERT_unknown_ca			= 48,	//! TLS 1.0
  ALERT_access_denied			= 49,	//! TLS 1.0
  ALERT_decode_error			= 50,	//! TLS 1.0
  ALERT_decrypt_error			= 51,	//! TLS 1.0
  ALERT_export_restriction	        = 60,	//! TLS 1.0
  ALERT_protocol_version		= 70,	//! TLS 1.0
  ALERT_insufficient_security		= 71,	//! TLS 1.0
  ALERT_internal_error			= 80,	//! TLS 1.0
  ALERT_inappropriate_fallback		= 86,	//! @rfc{7507@}
  ALERT_user_canceled			= 90,	//! TLS 1.0
  ALERT_no_renegotiation		= 100,	//! TLS 1.0
  ALERT_missing_extension		= 109,	//! TLS 1.3 draft.
  ALERT_unsupported_extension		= 110,	//! @rfc{3546@}
  ALERT_certificate_unobtainable	= 111,	//! @rfc{3546@}
  ALERT_unrecognized_name		= 112,	//! @rfc{3546@}
  ALERT_bad_certificate_status_response	= 113,	//! @rfc{3546@}
  ALERT_bad_certificate_hash_value	= 114,	//! @rfc{3546@}
  ALERT_unknown_psk_identity		= 115,  //! @rfc{4279@}
  ALERT_no_application_protocol		= 120,	//! @rfc{7301@}
}

//! Mapping from @[AlertMessage] to a descriptive string.
constant ALERT_descriptions = ([
  ALERT_close_notify: "Connection closed.",
  ALERT_unexpected_message: "An inappropriate message was received.",
  ALERT_bad_record_mac: "Incorrect MAC.",
  ALERT_decryption_failed: "Decryption failure.",
  ALERT_record_overflow: "Record overflow.",
  ALERT_decompression_failure: "Decompression failure.",
  ALERT_handshake_failure: "Handshake failure.",
  ALERT_no_certificate: "Certificate required.",
  ALERT_bad_certificate: "Bad certificate.",
  ALERT_unsupported_certificate: "Unsupported certificate.",
  ALERT_certificate_revoked: "Certificate revoked.",
  ALERT_certificate_expired: "Certificate expired.",
  ALERT_certificate_unknown: "Unknown certificate problem.",
  ALERT_illegal_parameter: "Illegal parameter.",
  ALERT_unknown_ca: "Unknown certification authority.",
  ALERT_access_denied: "Access denied.",
  ALERT_decode_error: "Decoding error.",
  ALERT_decrypt_error: "Decryption error.",
  ALERT_export_restriction: "Export restrictions apply.",
  ALERT_protocol_version: "Unsupported protocol.",
  ALERT_insufficient_security: "Insufficient security.",
  ALERT_internal_error: "Internal error.",
  ALERT_inappropriate_fallback: "Inappropriate fallback.",
  ALERT_user_canceled: "User canceled.",
  ALERT_no_renegotiation: "Renegotiation not allowed.",
  ALERT_missing_extension: "Missing extension.",
  ALERT_unsupported_extension: "Unsolicitaded extension.",
  ALERT_certificate_unobtainable: "Failed to obtain certificate.",
  ALERT_unrecognized_name: "Unrecognized host name.",
  ALERT_bad_certificate_status_response: "Bad certificate status response.",
  ALERT_bad_certificate_hash_value: "Invalid certificate signature.",
  ALERT_unknown_psk_identity: "Unknown PSK identity.",
  ALERT_no_application_protocol: "No compatible application layer protocol.",
]);

//! Mapping from @[AlertMessage] to @[ProtocolVersion] where
//! the alert was deprecated.
constant ALERT_deprecated = ([
  ALERT_decryption_failed: PROTOCOL_TLS_1_2,
  ALERT_decompression_failure: PROTOCOL_TLS_1_3,
  ALERT_no_certificate: PROTOCOL_TLS_1_1,
  ALERT_export_restriction: PROTOCOL_TLS_1_1,
]);

constant CONNECTION_client 	= 0;
constant CONNECTION_server 	= 1;
constant CONNECTION_client_auth = 2;

//! Cipher suites.
enum CipherSuite {
  SSL_invalid_suite				= -1,
  SSL_null_with_null_null 			= 0x0000,	//! SSL 3.0
  SSL_rsa_with_null_md5				= 0x0001,	//! SSL 3.0
  SSL_rsa_with_null_sha				= 0x0002,	//! SSL 3.0
  SSL_rsa_export_with_rc4_40_md5		= 0x0003,	//! SSL 3.0
  SSL_rsa_with_rc4_128_md5			= 0x0004,	//! SSL 3.0
  SSL_rsa_with_rc4_128_sha			= 0x0005,	//! SSL 3.0
  SSL_rsa_export_with_rc2_cbc_40_md5		= 0x0006,	//! SSL 3.0
  SSL_rsa_with_idea_cbc_sha			= 0x0007,	//! SSL 3.0
  TLS_rsa_with_idea_cbc_sha			= 0x0007,       //! @rfc{5469@}
  SSL_rsa_export_with_des40_cbc_sha		= 0x0008,	//! SSL 3.0
  SSL_rsa_with_des_cbc_sha			= 0x0009,	//! SSL 3.0
  TLS_rsa_with_des_cbc_sha			= 0x0009,       //! @rfc{5469@}
  SSL_rsa_with_3des_ede_cbc_sha			= 0x000a,	//! SSL 3.0
  SSL_dh_dss_export_with_des40_cbc_sha		= 0x000b,	//! SSL 3.0
  SSL_dh_dss_with_des_cbc_sha			= 0x000c,	//! SSL 3.0
  TLS_dh_dss_with_des_cbc_sha			= 0x000c,       //! @rfc{5469@}
  SSL_dh_dss_with_3des_ede_cbc_sha		= 0x000d,	//! SSL 3.0
  SSL_dh_rsa_export_with_des40_cbc_sha		= 0x000e,	//! SSL 3.0
  SSL_dh_rsa_with_des_cbc_sha			= 0x000f,	//! SSL 3.0
  TLS_dh_rsa_with_des_cbc_sha			= 0x000f,       //! @rfc{5469@}
  SSL_dh_rsa_with_3des_ede_cbc_sha		= 0x0010,	//! SSL 3.0
  SSL_dhe_dss_export_with_des40_cbc_sha		= 0x0011,	//! SSL 3.0
  SSL_dhe_dss_with_des_cbc_sha			= 0x0012,	//! SSL 3.0
  TLS_dhe_dss_with_des_cbc_sha			= 0x0012,       //! @rfc{5469@}
  SSL_dhe_dss_with_3des_ede_cbc_sha		= 0x0013,	//! SSL 3.0
  SSL_dhe_rsa_export_with_des40_cbc_sha		= 0x0014,	//! SSL 3.0
  SSL_dhe_rsa_with_des_cbc_sha			= 0x0015,	//! SSL 3.0
  TLS_dhe_rsa_with_des_cbc_sha			= 0x0015,       //! @rfc{5469@}
  SSL_dhe_rsa_with_3des_ede_cbc_sha		= 0x0016,	//! SSL 3.0
  SSL_dh_anon_export_with_rc4_40_md5		= 0x0017,	//! SSL 3.0
  SSL_dh_anon_with_rc4_128_md5			= 0x0018,	//! SSL 3.0
  SSL_dh_anon_export_with_des40_cbc_sha		= 0x0019,	//! SSL 3.0
  SSL_dh_anon_with_des_cbc_sha			= 0x001a,	//! SSL 3.0
  TLS_dh_anon_with_des_cbc_sha			= 0x001a,       //! @rfc{5469@}
  SSL_dh_anon_with_3des_ede_cbc_sha		= 0x001b,	//! SSL 3.0

/* SSLv3/TLS conflict */
/*   SSL_fortezza_dms_with_null_sha		= 0x001c, */
/*   SSL_fortezza_dms_with_fortezza_cbc_sha	= 0x001d, */
/*   SSL_fortezza_dms_with_rc4_128_sha		= 0x001e, */

  TLS_krb5_with_des_cbc_sha			= 0x001e,	//! @rfc{2712@}
  TLS_krb5_with_3des_ede_cbc_sha		= 0x001f,	//! @rfc{2712@}
  TLS_krb5_with_rc4_128_sha			= 0x0020,	//! @rfc{2712@}
  TLS_krb5_with_idea_cbc_sha			= 0x0021,	//! @rfc{2712@}
  TLS_krb5_with_des_cbc_md5			= 0x0022,	//! @rfc{2712@}
  TLS_krb5_with_3des_ede_cbc_md5		= 0x0023,	//! @rfc{2712@}
  TLS_krb5_with_rc4_128_md5			= 0x0024,	//! @rfc{2712@}
  TLS_krb5_with_idea_cbc_md5			= 0x0025,	//! @rfc{2712@}
  TLS_krb5_export_with_des_cbc_40_sha		= 0x0026,	//! @rfc{2712@}
  TLS_krb5_export_with_rc2_cbc_40_sha		= 0x0027,	//! @rfc{2712@}
  TLS_krb5_export_with_rc4_40_sha		= 0x0028,	//! @rfc{2712@}
  TLS_krb5_export_with_des_cbc_40_md5		= 0x0029,	//! @rfc{2712@}
  TLS_krb5_export_with_rc2_cbc_40_md5		= 0x002a,	//! @rfc{2712@}
  TLS_krb5_export_with_rc4_40_md5		= 0x002b,	//! @rfc{2712@}
  TLS_psk_with_null_sha				= 0x002c,	//! @rfc{4785@}
  TLS_dhe_psk_with_null_sha			= 0x002d,	//! @rfc{4785@}
  TLS_rsa_psk_with_null_sha			= 0x002e,	//! @rfc{4785@}
  TLS_rsa_with_aes_128_cbc_sha			= 0x002f,	//! @rfc{3268@}
  TLS_dh_dss_with_aes_128_cbc_sha		= 0x0030,	//! @rfc{3268@}
  TLS_dh_rsa_with_aes_128_cbc_sha		= 0x0031,	//! @rfc{3268@}
  TLS_dhe_dss_with_aes_128_cbc_sha		= 0x0032,	//! @rfc{3268@}
  TLS_dhe_rsa_with_aes_128_cbc_sha		= 0x0033,	//! @rfc{3268@}
  TLS_dh_anon_with_aes_128_cbc_sha		= 0x0034,	//! @rfc{3268@}
  TLS_rsa_with_aes_256_cbc_sha			= 0x0035,	//! @rfc{3268@}
  TLS_dh_dss_with_aes_256_cbc_sha		= 0x0036,	//! @rfc{3268@}
  TLS_dh_rsa_with_aes_256_cbc_sha		= 0x0037,	//! @rfc{3268@}
  TLS_dhe_dss_with_aes_256_cbc_sha		= 0x0038,	//! @rfc{3268@}
  TLS_dhe_rsa_with_aes_256_cbc_sha		= 0x0039,	//! @rfc{3268@}
  TLS_dh_anon_with_aes_256_cbc_sha		= 0x003a,	//! @rfc{3268@}
  TLS_rsa_with_null_sha256			= 0x003b,	//! TLS 1.2
  TLS_rsa_with_aes_128_cbc_sha256		= 0x003c,	//! TLS 1.2
  TLS_rsa_with_aes_256_cbc_sha256		= 0x003d,	//! TLS 1.2
  TLS_dh_dss_with_aes_128_cbc_sha256		= 0x003e,	//! TLS 1.2
  TLS_dh_rsa_with_aes_128_cbc_sha256		= 0x003f,	//! TLS 1.2
  TLS_dhe_dss_with_aes_128_cbc_sha256		= 0x0040,	//! TLS 1.2
  TLS_rsa_with_camellia_128_cbc_sha		= 0x0041,	//! @rfc{4132@}
  TLS_dh_dss_with_camellia_128_cbc_sha		= 0x0042,	//! @rfc{4132@}
  TLS_dh_rsa_with_camellia_128_cbc_sha		= 0x0043,	//! @rfc{4132@}
  TLS_dhe_dss_with_camellia_128_cbc_sha		= 0x0044,	//! @rfc{4132@}
  TLS_dhe_rsa_with_camellia_128_cbc_sha		= 0x0045,	//! @rfc{4132@}
  TLS_dh_anon_with_camellia_128_cbc_sha		= 0x0046,	//! @rfc{4132@}

// draft-ietf-tls-ecc-01.txt has ECDH_* suites in the range [0x0047, 0x005a].
// They were moved to 0xc001.. in @rfc{4492@}.

// These suites from the 56-bit draft are apparently in use
// by some versions of MSIE.
  TLS_rsa_export1024_with_rc4_56_md5		= 0x0060,	//! 56bit draft
  TLS_rsa_export1024_with_rc2_cbc_56_md5	= 0x0061,	//! 56bit draft
  TLS_rsa_export1024_with_des_cbc_sha		= 0x0062,	//! 56bit draft
  TLS_dhe_dss_export1024_with_des_cbc_sha	= 0x0063,	//! 56bit draft
  TLS_rsa_export1024_with_rc4_56_sha		= 0x0064,	//! 56bit draft
  TLS_dhe_dss_export1024_with_rc4_56_sha	= 0x0065,	//! 56bit draft
  TLS_dhe_dss_with_rc4_128_sha			= 0x0066,	//! 56bit draft

  TLS_dhe_rsa_with_aes_128_cbc_sha256		= 0x0067,	//! TLS 1.2
  TLS_dh_dss_with_aes_256_cbc_sha256		= 0x0068,	//! TLS 1.2
  TLS_dh_rsa_with_aes_256_cbc_sha256		= 0x0069,	//! TLS 1.2
  TLS_dhe_dss_with_aes_256_cbc_sha256		= 0x006a,	//! TLS 1.2
  TLS_dhe_rsa_with_aes_256_cbc_sha256		= 0x006b,	//! TLS 1.2
  TLS_dh_anon_with_aes_128_cbc_sha256		= 0x006c,	//! TLS 1.2
  TLS_dh_anon_with_aes_256_cbc_sha256		= 0x006d,	//! TLS 1.2

  TLS_rsa_with_camellia_256_cbc_sha		= 0x0084,	//! @rfc{4132@}
  TLS_dh_dss_with_camellia_256_cbc_sha		= 0x0085,	//! @rfc{4132@}
  TLS_dh_rsa_with_camellia_256_cbc_sha		= 0x0086,	//! @rfc{4132@}
  TLS_dhe_dss_with_camellia_256_cbc_sha		= 0x0087,	//! @rfc{4132@}
  TLS_dhe_rsa_with_camellia_256_cbc_sha		= 0x0088,	//! @rfc{4132@}
  TLS_dh_anon_with_camellia_256_cbc_sha		= 0x0089,	//! @rfc{4132@}
  TLS_psk_with_rc4_128_sha			= 0x008a,	//! @rfc{4279@}
  TLS_psk_with_3des_ede_cbc_sha			= 0x008b,	//! @rfc{4279@}
  TLS_psk_with_aes_128_cbc_sha			= 0x008c,	//! @rfc{4279@}
  TLS_psk_with_aes_256_cbc_sha			= 0x008d,	//! @rfc{4279@}
  TLS_dhe_psk_with_rc4_128_sha			= 0x008e,	//! @rfc{4279@}
  TLS_dhe_psk_with_3des_ede_cbc_sha		= 0x008f,	//! @rfc{4279@}
  TLS_dhe_psk_with_aes_128_cbc_sha		= 0x0090,	//! @rfc{4279@}
  TLS_dhe_psk_with_aes_256_cbc_sha		= 0x0091,	//! @rfc{4279@}
  TLS_rsa_psk_with_rc4_128_sha			= 0x0092,	//! @rfc{4279@}
  TLS_rsa_psk_with_3des_ede_cbc_sha		= 0x0093,	//! @rfc{4279@}
  TLS_rsa_psk_with_aes_128_cbc_sha		= 0x0094,	//! @rfc{4279@}
  TLS_rsa_psk_with_aes_256_cbc_sha		= 0x0095,	//! @rfc{4279@}
  TLS_rsa_with_seed_cbc_sha			= 0x0096,	//! @rfc{4162@}
  TLS_dh_dss_with_seed_cbc_sha			= 0x0097,	//! @rfc{4162@}
  TLS_dh_rsa_with_seed_cbc_sha			= 0x0098,	//! @rfc{4162@}
  TLS_dhe_dss_with_seed_cbc_sha			= 0x0099,	//! @rfc{4162@}
  TLS_dhe_rsa_with_seed_cbc_sha			= 0x009a,	//! @rfc{4162@}
  TLS_dh_anon_with_seed_cbc_sha			= 0x009b,	//! @rfc{4162@}
  TLS_rsa_with_aes_128_gcm_sha256		= 0x009c,	//! @rfc{5288@}
  TLS_rsa_with_aes_256_gcm_sha384		= 0x009d,	//! @rfc{5288@}
  TLS_dhe_rsa_with_aes_128_gcm_sha256		= 0x009e,	//! @rfc{5288@}
  TLS_dhe_rsa_with_aes_256_gcm_sha384		= 0x009f,	//! @rfc{5288@}
  TLS_dh_rsa_with_aes_128_gcm_sha256		= 0x00a0,	//! @rfc{5288@}
  TLS_dh_rsa_with_aes_256_gcm_sha384		= 0x00a1,	//! @rfc{5288@}
  TLS_dhe_dss_with_aes_128_gcm_sha256		= 0x00a2,	//! @rfc{5288@}
  TLS_dhe_dss_with_aes_256_gcm_sha384		= 0x00a3,	//! @rfc{5288@}
  TLS_dh_dss_with_aes_128_gcm_sha256		= 0x00a4,	//! @rfc{5288@}
  TLS_dh_dss_with_aes_256_gcm_sha384		= 0x00a5,	//! @rfc{5288@}
  TLS_dh_anon_with_aes_128_gcm_sha256		= 0x00a6,	//! @rfc{5288@}
  TLS_dh_anon_with_aes_256_gcm_sha384		= 0x00a7,	//! @rfc{5288@}
  TLS_psk_with_aes_128_gcm_sha256		= 0x00a8,	//! @rfc{5487@}
  TLS_psk_with_aes_256_gcm_sha384		= 0x00a9,	//! @rfc{5487@}
  TLS_dhe_psk_with_aes_128_gcm_sha256		= 0x00aa,	//! @rfc{5487@}
  TLS_dhe_psk_with_aes_256_gcm_sha384		= 0x00ab,	//! @rfc{5487@}
  TLS_rsa_psk_with_aes_128_gcm_sha256		= 0x00ac,	//! @rfc{5487@}
  TLS_rsa_psk_with_aes_256_gcm_sha384		= 0x00ad,	//! @rfc{5487@}
  TLS_psk_with_aes_128_cbc_sha256		= 0x00ae,	//! @rfc{5487@}
  TLS_psk_with_aes_256_cbc_sha384		= 0x00af,	//! @rfc{5487@}
  TLS_psk_with_null_sha256			= 0x00b0,	//! @rfc{5487@}
  TLS_psk_with_null_sha384			= 0x00b1,	//! @rfc{5487@}
  TLS_dhe_psk_with_aes_128_cbc_sha256		= 0x00b2,	//! @rfc{5487@}
  TLS_dhe_psk_with_aes_256_cbc_sha384		= 0x00b3,	//! @rfc{5487@}
  TLS_dhe_psk_with_null_sha256			= 0x00b4,	//! @rfc{5487@}
  TLS_dhe_psk_with_null_sha384			= 0x00b5,	//! @rfc{5487@}
  TLS_rsa_psk_with_aes_128_cbc_sha256		= 0x00b6,	//! @rfc{5487@}
  TLS_rsa_psk_with_aes_256_cbc_sha384		= 0x00b7,	//! @rfc{5487@}
  TLS_rsa_psk_with_null_sha256			= 0x00b8,	//! @rfc{5487@}
  TLS_rsa_psk_with_null_sha384			= 0x00b9,	//! @rfc{5487@}
  TLS_rsa_with_camellia_128_cbc_sha256		= 0x00ba,	//! @rfc{5932@}
  TLS_dh_dss_with_camellia_128_cbc_sha256	= 0x00bb,	//! @rfc{5932@}
  TLS_dh_rsa_with_camellia_128_cbc_sha256	= 0x00bc,	//! @rfc{5932@}
  TLS_dhe_dss_with_camellia_128_cbc_sha256	= 0x00bd,	//! @rfc{5932@}
  TLS_dhe_rsa_with_camellia_128_cbc_sha256	= 0x00be,	//! @rfc{5932@}
  TLS_dh_anon_with_camellia_128_cbc_sha256	= 0x00bf,	//! @rfc{5932@}
  TLS_rsa_with_camellia_256_cbc_sha256		= 0x00c0,	//! @rfc{5932@}
  TLS_dh_dss_with_camellia_256_cbc_sha256	= 0x00c1,	//! @rfc{5932@}
  TLS_dh_rsa_with_camellia_256_cbc_sha256	= 0x00c2,	//! @rfc{5932@}
  TLS_dhe_dss_with_camellia_256_cbc_sha256	= 0x00c3,	//! @rfc{5932@}
  TLS_dhe_rsa_with_camellia_256_cbc_sha256	= 0x00c4,	//! @rfc{5932@}
  TLS_dh_anon_with_camellia_256_cbc_sha256	= 0x00c5,	//! @rfc{5932@}
  TLS_sm4_gcm_sm3                               = 0x00c6,       //! @rfc{8998@}
  TLS_sm4_ccm_sm3                               = 0x00c7,       //! @rfc{8998@}

  TLS_empty_renegotiation_info_scsv		= 0x00ff,	//! @rfc{5746@}

  // 0x0a0a Reserved in @rfc{8701@}

  // These are TLS 1.3 only.
  TLS_aes_128_gcm_sha256                        = 0x1301,       //! @rfc{8446@}
  TLS_aes_256_gcm_sha384                        = 0x1302,       //! @rfc{8446@}
  TLS_chacha20_poly1305_sha256                  = 0x1303,       //! @rfc{8446@}
  TLS_aes_128_ccm_sha256                        = 0x1304,       //! @rfc{8446@}
  TLS_aes_128_ccm_8_sha256                      = 0x1305,       //! @rfc{8446@}

  // 0x1a1a Reserved in @rfc{8701@}
  // 0x2a2a Reserved in @rfc{8701@}
  // 0x3a3a Reserved in @rfc{8701@}
  // 0x4a4a Reserved in @rfc{8701@}

  TLS_fallback_scsv				= 0x5600,	//! @rfc{7507@}

  // 0x5a5a Reserved in @rfc{8701@}
  // 0x6a6a Reserved in @rfc{8701@}
  // 0x7a7a Reserved in @rfc{8701@}
  // 0x8a8a Reserved in @rfc{8701@}
  // 0x9a9a Reserved in @rfc{8701@}
  // 0xaaaa Reserved in @rfc{8701@}
  // 0xbaba Reserved in @rfc{8701@}

  TLS_ecdh_ecdsa_with_null_sha			= 0xc001,	//! @rfc{4492@}
  TLS_ecdh_ecdsa_with_rc4_128_sha		= 0xc002,	//! @rfc{4492@}
  TLS_ecdh_ecdsa_with_3des_ede_cbc_sha		= 0xc003,	//! @rfc{4492@}
  TLS_ecdh_ecdsa_with_aes_128_cbc_sha		= 0xc004,	//! @rfc{4492@}
  TLS_ecdh_ecdsa_with_aes_256_cbc_sha		= 0xc005,	//! @rfc{4492@}
  TLS_ecdhe_ecdsa_with_null_sha			= 0xc006,	//! @rfc{4492@}
  TLS_ecdhe_ecdsa_with_rc4_128_sha		= 0xc007,	//! @rfc{4492@}
  TLS_ecdhe_ecdsa_with_3des_ede_cbc_sha		= 0xc008,	//! @rfc{4492@}
  TLS_ecdhe_ecdsa_with_aes_128_cbc_sha		= 0xc009,	//! @rfc{4492@}
  TLS_ecdhe_ecdsa_with_aes_256_cbc_sha		= 0xc00a,	//! @rfc{4492@}
  TLS_ecdh_rsa_with_null_sha			= 0xc00b,	//! @rfc{4492@}
  TLS_ecdh_rsa_with_rc4_128_sha			= 0xc00c,	//! @rfc{4492@}
  TLS_ecdh_rsa_with_3des_ede_cbc_sha		= 0xc00d,	//! @rfc{4492@}
  TLS_ecdh_rsa_with_aes_128_cbc_sha		= 0xc00e,	//! @rfc{4492@}
  TLS_ecdh_rsa_with_aes_256_cbc_sha		= 0xc00f,	//! @rfc{4492@}
  TLS_ecdhe_rsa_with_null_sha			= 0xc010,	//! @rfc{4492@}
  TLS_ecdhe_rsa_with_rc4_128_sha		= 0xc011,	//! @rfc{4492@}
  TLS_ecdhe_rsa_with_3des_ede_cbc_sha		= 0xc012,	//! @rfc{4492@}
  TLS_ecdhe_rsa_with_aes_128_cbc_sha		= 0xc013,	//! @rfc{4492@}
  TLS_ecdhe_rsa_with_aes_256_cbc_sha		= 0xc014,	//! @rfc{4492@}
  TLS_ecdh_anon_with_null_sha			= 0xc015,	//! @rfc{4492@}
  TLS_ecdh_anon_with_rc4_128_sha		= 0xc016,	//! @rfc{4492@}
  TLS_ecdh_anon_with_3des_ede_cbc_sha		= 0xc017,	//! @rfc{4492@}
  TLS_ecdh_anon_with_aes_128_cbc_sha		= 0xc018,	//! @rfc{4492@}
  TLS_ecdh_anon_with_aes_256_cbc_sha		= 0xc019,	//! @rfc{4492@}
  TLS_srp_sha_with_3des_ede_cbc_sha		= 0xc01a,	//! @rfc{5054@}
  TLS_srp_sha_rsa_with_3des_ede_cbc_sha		= 0xc01b,	//! @rfc{5054@}
  TLS_srp_sha_dss_with_3des_ede_cbc_sha		= 0xc01c,	//! @rfc{5054@}
  TLS_srp_sha_with_aes_128_cbc_sha		= 0xc01d,	//! @rfc{5054@}
  TLS_srp_sha_rsa_with_aes_128_cbc_sha		= 0xc01e,	//! @rfc{5054@}
  TLS_srp_sha_dss_with_aes_128_cbc_sha		= 0xc01f,	//! @rfc{5054@}
  TLS_srp_sha_with_aes_256_cbc_sha		= 0xc020,	//! @rfc{5054@}
  TLS_srp_sha_rsa_with_aes_256_cbc_sha		= 0xc021,	//! @rfc{5054@}
  TLS_srp_sha_dss_with_aes_256_cbc_sha		= 0xc022,	//! @rfc{5054@}
  TLS_ecdhe_ecdsa_with_aes_128_cbc_sha256	= 0xc023,	//! @rfc{5289@}
  TLS_ecdhe_ecdsa_with_aes_256_cbc_sha384	= 0xc024,	//! @rfc{5289@}
  TLS_ecdh_ecdsa_with_aes_128_cbc_sha256	= 0xc025,	//! @rfc{5289@}
  TLS_ecdh_ecdsa_with_aes_256_cbc_sha384	= 0xc026,	//! @rfc{5289@}
  TLS_ecdhe_rsa_with_aes_128_cbc_sha256		= 0xc027,	//! @rfc{5289@}
  TLS_ecdhe_rsa_with_aes_256_cbc_sha384		= 0xc028,	//! @rfc{5289@}
  TLS_ecdh_rsa_with_aes_128_cbc_sha256		= 0xc029,	//! @rfc{5289@}
  TLS_ecdh_rsa_with_aes_256_cbc_sha384		= 0xc02a,	//! @rfc{5289@}
  TLS_ecdhe_ecdsa_with_aes_128_gcm_sha256	= 0xc02b,	//! @rfc{5289@}
  TLS_ecdhe_ecdsa_with_aes_256_gcm_sha384	= 0xc02c,	//! @rfc{5289@}
  TLS_ecdh_ecdsa_with_aes_128_gcm_sha256	= 0xc02d,	//! @rfc{5289@}
  TLS_ecdh_ecdsa_with_aes_256_gcm_sha384	= 0xc02e,	//! @rfc{5289@}
  TLS_ecdhe_rsa_with_aes_128_gcm_sha256		= 0xc02f,	//! @rfc{5289@}
  TLS_ecdhe_rsa_with_aes_256_gcm_sha384		= 0xc030,	//! @rfc{5289@}
  TLS_ecdh_rsa_with_aes_128_gcm_sha256		= 0xc031,	//! @rfc{5289@}
  TLS_ecdh_rsa_with_aes_256_gcm_sha384		= 0xc032,	//! @rfc{5289@}
  TLS_ecdhe_psk_with_rc4_128_sha		= 0xc033,	//! @rfc{5489@}
  TLS_ecdhe_psk_with_3des_ede_cbc_sha		= 0xc034,	//! @rfc{5489@}
  TLS_ecdhe_psk_with_aes_128_cbc_sha		= 0xc035,	//! @rfc{5489@}
  TLS_ecdhe_psk_with_aes_256_cbc_sha		= 0xc036,	//! @rfc{5489@}
  TLS_ecdhe_psk_with_aes_128_cbc_sha256		= 0xc037,	//! @rfc{5489@}
  TLS_ecdhe_psk_with_aes_256_cbc_sha384		= 0xc038,	//! @rfc{5489@}
  TLS_ecdhe_psk_with_null_sha			= 0xc039,	//! @rfc{5489@}
  TLS_ecdhe_psk_with_null_sha256		= 0xc03a,	//! @rfc{5489@}
  TLS_ecdhe_psk_with_null_sha384		= 0xc03b,	//! @rfc{5489@}
  TLS_rsa_with_aria_128_cbc_sha256		= 0xc03c,	//! @rfc{6209@}
  TLS_rsa_with_aria_256_cbc_sha384		= 0xc03d,	//! @rfc{6209@}
  TLS_dh_dss_with_aria_128_cbc_sha256		= 0xc03e,	//! @rfc{6209@}
  TLS_dh_dss_with_aria_256_cbc_sha384		= 0xc03f,	//! @rfc{6209@}
  TLS_dh_rsa_with_aria_128_cbc_sha256		= 0xc040,	//! @rfc{6209@}
  TLS_dh_rsa_with_aria_256_cbc_sha384		= 0xc041,	//! @rfc{6209@}
  TLS_dhe_dss_with_aria_128_cbc_sha256		= 0xc042,	//! @rfc{6209@}
  TLS_dhe_dss_with_aria_256_cbc_sha384		= 0xc043,	//! @rfc{6209@}
  TLS_dhe_rsa_with_aria_128_cbc_sha256		= 0xc044,	//! @rfc{6209@}
  TLS_dhe_rsa_with_aria_256_cbc_sha384		= 0xc045,	//! @rfc{6209@}
  TLS_dh_anon_with_aria_128_cbc_sha256		= 0xc046,	//! @rfc{6209@}
  TLS_dh_anon_with_aria_256_cbc_sha384		= 0xc047,	//! @rfc{6209@}
  TLS_ecdhe_ecdsa_with_aria_128_cbc_sha256	= 0xc048,	//! @rfc{6209@}
  TLS_ecdhe_ecdsa_with_aria_256_cbc_sha384	= 0xc049,	//! @rfc{6209@}
  TLS_ecdh_ecdsa_with_aria_128_cbc_sha256	= 0xc04a,	//! @rfc{6209@}
  TLS_ecdh_ecdsa_with_aria_256_cbc_sha384	= 0xc04b,	//! @rfc{6209@}
  TLS_ecdhe_rsa_with_aria_128_cbc_sha256	= 0xc04c,	//! @rfc{6209@}
  TLS_ecdhe_rsa_with_aria_256_cbc_sha384	= 0xc04d,	//! @rfc{6209@}
  TLS_ecdh_rsa_with_aria_128_cbc_sha256		= 0xc04e,	//! @rfc{6209@}
  TLS_ecdh_rsa_with_aria_256_cbc_sha384		= 0xc04f,	//! @rfc{6209@}
  TLS_rsa_with_aria_128_gcm_sha256		= 0xc050,	//! @rfc{6209@}
  TLS_rsa_with_aria_256_gcm_sha384		= 0xc051,	//! @rfc{6209@}
  TLS_dhe_rsa_with_aria_128_gcm_sha256		= 0xc052,	//! @rfc{6209@}
  TLS_dhe_rsa_with_aria_256_gcm_sha384		= 0xc053,	//! @rfc{6209@}
  TLS_dh_rsa_with_aria_128_gcm_sha256		= 0xc054,	//! @rfc{6209@}
  TLS_dh_rsa_with_aria_256_gcm_sha384		= 0xc055,	//! @rfc{6209@}
  TLS_dhe_dss_with_aria_128_gcm_sha256		= 0xc056,	//! @rfc{6209@}
  TLS_dhe_dss_with_aria_256_gcm_sha384		= 0xc057,	//! @rfc{6209@}
  TLS_dh_dss_with_aria_128_gcm_sha256		= 0xc058,	//! @rfc{6209@}
  TLS_dh_dss_with_aria_256_gcm_sha384		= 0xc059,	//! @rfc{6209@}
  TLS_dh_anon_with_aria_128_gcm_sha256		= 0xc05a,	//! @rfc{6209@}
  TLS_dh_anon_with_aria_256_gcm_sha384		= 0xc05b,	//! @rfc{6209@}
  TLS_ecdhe_ecdsa_with_aria_128_gcm_sha256	= 0xc05c,	//! @rfc{6209@}
  TLS_ecdhe_ecdsa_with_aria_256_gcm_sha384	= 0xc05d,	//! @rfc{6209@}
  TLS_ecdh_ecdsa_with_aria_128_gcm_sha256	= 0xc05e,	//! @rfc{6209@}
  TLS_ecdh_ecdsa_with_aria_256_gcm_sha384	= 0xc05f,	//! @rfc{6209@}
  TLS_ecdhe_rsa_with_aria_128_gcm_sha256	= 0xc060,	//! @rfc{6209@}
  TLS_ecdhe_rsa_with_aria_256_gcm_sha384	= 0xc061,	//! @rfc{6209@}
  TLS_ecdh_rsa_with_aria_128_gcm_sha256		= 0xc062,	//! @rfc{6209@}
  TLS_ecdh_rsa_with_aria_256_gcm_sha384		= 0xc063,	//! @rfc{6209@}
  TLS_psk_with_aria_128_cbc_sha256		= 0xc064,	//! @rfc{6209@}
  TLS_psk_with_aria_256_cbc_sha384		= 0xc065,	//! @rfc{6209@}
  TLS_dhe_psk_with_aria_128_cbc_sha256		= 0xc066,	//! @rfc{6209@}
  TLS_dhe_psk_with_aria_256_cbc_sha384		= 0xc067,	//! @rfc{6209@}
  TLS_rsa_psk_with_aria_128_cbc_sha256		= 0xc068,	//! @rfc{6209@}
  TLS_rsa_psk_with_aria_256_cbc_sha384		= 0xc069,	//! @rfc{6209@}
  TLS_psk_with_aria_128_gcm_sha256		= 0xc06a,	//! @rfc{6209@}
  TLS_psk_with_aria_256_gcm_sha384		= 0xc06b,	//! @rfc{6209@}
  TLS_dhe_psk_with_aria_128_gcm_sha256		= 0xc06c,	//! @rfc{6209@}
  TLS_dhe_psk_with_aria_256_gcm_sha384		= 0xc06d,	//! @rfc{6209@}
  TLS_rsa_psk_with_aria_128_gcm_sha256		= 0xc06e,	//! @rfc{6209@}
  TLS_rsa_psk_with_aria_256_gcm_sha384		= 0xc06f,	//! @rfc{6209@}
  TLS_ecdhe_psk_with_aria_128_cbc_sha256	= 0xc070,	//! @rfc{6209@}
  TLS_ecdhe_psk_with_aria_256_cbc_sha384	= 0xc071,	//! @rfc{6209@}
  TLS_ecdhe_ecdsa_with_camellia_128_cbc_sha256	= 0xc072,	//! @rfc{6367@}
  TLS_ecdhe_ecdsa_with_camellia_256_cbc_sha384	= 0xc073,	//! @rfc{6367@}
  TLS_ecdh_ecdsa_with_camellia_128_cbc_sha256	= 0xc074,	//! @rfc{6367@}
  TLS_ecdh_ecdsa_with_camellia_256_cbc_sha384	= 0xc075,	//! @rfc{6367@}
  TLS_ecdhe_rsa_with_camellia_128_cbc_sha256	= 0xc076,	//! @rfc{6367@}
  TLS_ecdhe_rsa_with_camellia_256_cbc_sha384	= 0xc077,	//! @rfc{6367@}
  TLS_ecdh_rsa_with_camellia_128_cbc_sha256	= 0xc078,	//! @rfc{6367@}
  TLS_ecdh_rsa_with_camellia_256_cbc_sha384	= 0xc079,	//! @rfc{6367@}
  TLS_rsa_with_camellia_128_gcm_sha256		= 0xc07a,	//! @rfc{6367@}
  TLS_rsa_with_camellia_256_gcm_sha384		= 0xc07b,	//! @rfc{6367@}
  TLS_dhe_rsa_with_camellia_128_gcm_sha256	= 0xc07c,	//! @rfc{6367@}
  TLS_dhe_rsa_with_camellia_256_gcm_sha384	= 0xc07d,	//! @rfc{6367@}
  TLS_dh_rsa_with_camellia_128_gcm_sha256	= 0xc07e,	//! @rfc{6367@}
  TLS_dh_rsa_with_camellia_256_gcm_sha384	= 0xc07f,	//! @rfc{6367@}
  TLS_dhe_dss_with_camellia_128_gcm_sha256	= 0xc080,	//! @rfc{6367@}
  TLS_dhe_dss_with_camellia_256_gcm_sha384	= 0xc081,	//! @rfc{6367@}
  TLS_dh_dss_with_camellia_128_gcm_sha256	= 0xc082,	//! @rfc{6367@}
  TLS_dh_dss_with_camellia_256_gcm_sha384	= 0xc083,	//! @rfc{6367@}
  TLS_dh_anon_with_camellia_128_gcm_sha256	= 0xc084,	//! @rfc{6367@}
  TLS_dh_anon_with_camellia_256_gcm_sha384	= 0xc085,	//! @rfc{6367@}
  TLS_ecdhe_ecdsa_with_camellia_128_gcm_sha256	= 0xc086,	//! @rfc{6367@}
  TLS_ecdhe_ecdsa_with_camellia_256_gcm_sha384	= 0xc087,	//! @rfc{6367@}
  TLS_ecdh_ecdsa_with_camellia_128_gcm_sha256	= 0xc088,	//! @rfc{6367@}
  TLS_ecdh_ecdsa_with_camellia_256_gcm_sha384	= 0xc089,	//! @rfc{6367@}
  TLS_ecdhe_rsa_with_camellia_128_gcm_sha256	= 0xc08a,	//! @rfc{6367@}
  TLS_ecdhe_rsa_with_camellia_256_gcm_sha384	= 0xc08b,	//! @rfc{6367@}
  TLS_ecdh_rsa_with_camellia_128_gcm_sha256	= 0xc08c,	//! @rfc{6367@}
  TLS_ecdh_rsa_with_camellia_256_gcm_sha384	= 0xc08d,	//! @rfc{6367@}
  TLS_psk_with_camellia_128_gcm_sha256		= 0xc08e,	//! @rfc{6367@}
  TLS_psk_with_camellia_256_gcm_sha384		= 0xc08f,	//! @rfc{6367@}
  TLS_dhe_psk_with_camellia_128_gcm_sha256	= 0xc090,	//! @rfc{6367@}
  TLS_dhe_psk_with_camellia_256_gcm_sha384	= 0xc091,	//! @rfc{6367@}
  TLS_rsa_psk_with_camellia_128_gcm_sha256	= 0xc092,	//! @rfc{6367@}
  TLS_rsa_psk_with_camellia_256_gcm_sha384	= 0xc093,	//! @rfc{6367@}
  TLS_psk_with_camellia_128_cbc_sha256		= 0xc094,	//! @rfc{6367@}
  TLS_psk_with_camellia_256_cbc_sha384		= 0xc095,	//! @rfc{6367@}
  TLS_dhe_psk_with_camellia_128_cbc_sha256	= 0xc096,	//! @rfc{6367@}
  TLS_dhe_psk_with_camellia_256_cbc_sha384	= 0xc097,	//! @rfc{6367@}
  TLS_rsa_psk_with_camellia_128_cbc_sha256	= 0xc098,	//! @rfc{6367@}
  TLS_rsa_psk_with_camellia_256_cbc_sha384	= 0xc099,	//! @rfc{6367@}
  TLS_ecdhe_psk_with_camellia_128_cbc_sha256	= 0xc09a,	//! @rfc{6367@}
  TLS_ecdhe_psk_with_camellia_256_cbc_sha384	= 0xc09b,	//! @rfc{6367@}
  TLS_rsa_with_aes_128_ccm			= 0xc09c,	//! @rfc{6655@}
  TLS_rsa_with_aes_256_ccm			= 0xc09d,	//! @rfc{6655@}
  TLS_dhe_rsa_with_aes_128_ccm			= 0xc09e,	//! @rfc{6655@}
  TLS_dhe_rsa_with_aes_256_ccm			= 0xc09f,	//! @rfc{6655@}
  TLS_rsa_with_aes_128_ccm_8			= 0xc0a0,	//! @rfc{6655@}
  TLS_rsa_with_aes_256_ccm_8			= 0xc0a1,	//! @rfc{6655@}
  TLS_dhe_rsa_with_aes_128_ccm_8		= 0xc0a2,	//! @rfc{6655@}
  TLS_dhe_rsa_with_aes_256_ccm_8		= 0xc0a3,	//! @rfc{6655@}
  TLS_psk_with_aes_128_ccm			= 0xc0a4,	//! @rfc{6655@}
  TLS_psk_with_aes_256_ccm			= 0xc0a5,	//! @rfc{6655@}
  TLS_dhe_psk_with_aes_128_ccm			= 0xc0a6,	//! @rfc{6655@}
  TLS_dhe_psk_with_aes_256_ccm			= 0xc0a7,	//! @rfc{6655@}
  TLS_psk_with_aes_128_ccm_8			= 0xc0a8,	//! @rfc{6655@}
  TLS_psk_with_aes_256_ccm_8			= 0xc0a9,	//! @rfc{6655@}
  TLS_psk_dhe_with_aes_128_ccm_8		= 0xc0aa,	//! @rfc{6655@}
  TLS_psk_dhe_with_aes_256_ccm_8		= 0xc0ab,	//! @rfc{6655@}
  TLS_ecdhe_ecdsa_with_aes_128_ccm		= 0xc0ac,	//! @rfc{7251@}
  TLS_ecdhe_ecdsa_with_aes_256_ccm		= 0xc0ad,	//! @rfc{7251@}
  TLS_ecdhe_ecdsa_with_aes_128_ccm_8		= 0xc0ae,	//! @rfc{7251@}
  TLS_ecdhe_ecdsa_with_aes_256_ccm_8		= 0xc0af,	//! @rfc{7251@}
  TLS_eccpwd_with_aes_128_gcm_sha256            = 0xc0b0,       //! @rfc{8492@}
  TLS_eccpwd_with_aes_256_gcm_sha384            = 0xc0b1,       //! @rfc{8492@}
  TLS_eccpwd_with_aes_128_ccm_sha256            = 0xc0b2,       //! @rfc{8492@}
  TLS_eccpwd_with_aes_256_ccm_sha384            = 0xc0b3,       //! @rfc{8492@}
  // TLS_sha256_sha256  camwinget-tls-ts13-macciphersuites-12
  // TLS_sha384_sha384  camwinget-tls-ts13-macciphersuites-12

  // TLS_gostr341112_256_with_kuznyechik_ctr_omac
  // TLS_gostr341112_256_with_magma_ctr_omac
  // TLS_gostr341112_256_with_28147_cnr_imit
  // TLS_gostr341112_256_with_kuznyechik_mgm_l
  // TLS_gostr341112_256_with_magma_mgm_l
  // TLS_gostr341112_256_with_kuznyechik_mgm_s
  // TLS_gostr341112_256_with_magma_mgm_s

  // TLS_ecdhe_psk_with_aes_128_gcm_sha256	= 0xcafe,	// BoringSSL

  // 0xcaca Reserved in @rfc{8701@}

  TLS_ecdhe_rsa_with_oldchacha20_poly1305_sha256 = 0xcc13,  //! draft-agl-tls-chacha20poly1305-02
  TLS_ecdhe_ecdsa_with_oldchacha20_poly1305_sha256 = 0xcc14,//! draft-agl-tls-chacha20poly1305-02
  TLS_dhe_rsa_with_oldchacha20_poly1305_sha256	= 0xcc15,  //! draft-agl-tls-chacha20poly1305-02

  TLS_ecdhe_rsa_with_chacha20_poly1305_sha256	= 0xcca8,	//! @rfc{7905@}
  TLS_ecdhe_ecdsa_with_chacha20_poly1305_sha256	= 0xcca9,	//! @rfc{7905@}
  TLS_dhe_rsa_with_chacha20_poly1305_sha256	= 0xccaa,	//! @rfc{7905@}
  TLS_psk_with_chacha20_poly1305_sha256		= 0xccab,	//! @rfc{7905@}
  TLS_ecdhe_psk_with_chacha20_poly1305_sha256	= 0xccac,	//! @rfc{7905@}
  TLS_dhe_psk_with_chacha20_poly1305_sha256	= 0xccad,	//! @rfc{7905@}
  TLS_rsa_psk_with_chacha20_poly1305_sha256	= 0xccae,	//! @rfc{7905@}

  TLS_ecdhe_psk_with_aes_128_gcm_sha256         = 0xd001,       //! @rfc{8442@}
  TLS_ecdhe_psk_with_aes_256_gcm_sha384         = 0xd002,       //! @rfc{8442@}
  TLS_ecdhe_psk_with_aes_128_ccm_8_sha256       = 0xd003,       //! @rfc{8442@}

  TLS_ecdhe_psk_with_aes_128_ccm_sha256         = 0xd005,       //! @rfc{8442@}

  // 0xdada Reserved in @rfc{8701@}
  // 0xeaea Reserved in @rfc{8701@}
  // 0xfafa Reserved in @rfc{8701@}

// These were introduced by Netscape while developing SSL 3.1 after
// feedback from NIST. Eventually the feedback led to TLS 1.0.
  SSL_rsa_fips_with_des_cbc_sha			= 0xfefe,	//! Netscape.
  SSL_rsa_fips_with_3des_ede_cbc_sha		= 0xfeff,	//! Netscape.

// 0xFF00 and up are reserved for Private use.
  SSL_rsa_oldfips_with_des_cbc_sha		= 0xffe1,	//! experimental
  SSL_rsa_oldfips_with_3des_ede_cbc_sha		= 0xffe0,	//! experimental

  SSL_rsa_with_rc2_cbc_md5			= 0xff80,
  SSL_rsa_with_idea_cbc_md5			= 0xff81,
  SSL_rsa_with_des_cbc_md5			= 0xff82,
  SSL_rsa_with_3des_ede_cbc_md5			= 0xff83,
}

//! Return a descriptive name for a constant value.
//!
//! @param c
//!   Value to format.
//!
//! @param prefix
//!   Constant name prefix. Eg @expr{"CONNECTION"@}.
string fmt_constant(int c, string prefix)
{
  if (!has_suffix(prefix, "_")) prefix += "_";
  foreach([array(string)]indices(this), string id)
    if (has_prefix(id, prefix) && (this[id] == c)) return id;
  return sprintf("%sunknown(%d)", prefix, c);
}

protected mapping(int:string) suite_to_symbol = ([]);

//! Return a descriptive name for a cipher suite.
//!
//! @param suite
//!   Cipher suite to format.
string fmt_cipher_suite(int suite)
{
  if (!sizeof(suite_to_symbol)) {
    foreach([array(string)]indices(this), string id)
      if( has_prefix(id, "SSL_") || has_prefix(id, "TLS_") ) {
	suite_to_symbol[this[id]] = id;
      }
  }
  string res = suite_to_symbol[suite];
  if (res) return res;
  return suite_to_symbol[suite] = sprintf("unknown(%d)", suite);
}

//! Pretty-print an array of cipher suites.
//!
//! @param s
//!   Array of cipher suites to format.
string fmt_cipher_suites(array(int) s)
{
  String.Buffer b = String.Buffer();
  foreach(s, int c)
    b->sprintf("   %-6d: %s\n", c, fmt_cipher_suite(c));
  return (string)b;
}

//! Pretty-print an array of @[SignatureScheme]s.
//!
//! @param pairs
//!   Array of signature pairs to format.
string fmt_signature_pairs(array(int) pairs)
{
  String.Buffer b = String.Buffer();
  foreach(pairs, int signature_scheme)
    b->sprintf("  <%s, %s>\n",
	       fmt_constant(signature_scheme & HASH_MASK, "HASH"),
	       fmt_constant(signature_scheme & SIGNATURE_MASK, "SIGNATURE"));
  return (string)b;
}

//! Pretty-print a @[ProtocolVersion].
//!
//! @param version
//!   @[ProtocolVersion] to format.
string fmt_version(ProtocolVersion version)
{
  if (version <= PROTOCOL_SSL_3_0) {
    return sprintf("SSL %d.%d", version>>8, version & 0xff);
  }
  version -= PROTOCOL_TLS_1_0 - 0x100;
  return sprintf("TLS %d.%d", version>>8, version & 0xff);
}

/* FIXME: Add SIGN-type element to table */

//! A mapping from cipher suite identifier to an array defining the
//! algorithms to be used in that suite.
//!
//! @array
//!   @elem KeyExchangeType 0
//!     The key exchange algorithm to be used for this suite, or 0.
//!     E.g. @[KE_rsa].
//!   @elem int 1
//!     The cipher algorithm to be used for this suite, or 0. E.g.
//!     @[CIPHER_aes].
//!   @elem HashAlgorithm 2
//!     The hash algorithm to be used for this suite, or 0. E.g.
//!     @[HASH_sha1].
//!   @elem CipherMode 3
//!     Optionally for TLS 1.2 and later cipher suites the mode of
//!     operation. E.g. @[MODE_cbc].
//! @endarray
constant CIPHER_SUITES =
([
   // The following cipher suites are only intended for testing.
   // NB: TLS_rsa_with_null_sha256 has an explicit MODE_cbc in
   //     order to make it invalid for TLS 1.1 and earlier.
   SSL_null_with_null_null :    	({ 0, 0, 0 }),
   SSL_rsa_with_null_md5 :      	({ KE_rsa_export, 0, HASH_md5 }),
   SSL_rsa_with_null_sha :      	({ KE_rsa_export, 0, HASH_sha1 }),
   TLS_rsa_with_null_sha256 :      	({ KE_rsa_export, 0, HASH_sha256, MODE_cbc }),

   // NB: The export suites are obsolete in TLS 1.1 and later.
   //     The RC4/40 suite is required for Netscape 4.05 Intl.
#if constant(Crypto.Arctwo)
   SSL_rsa_export_with_rc2_cbc_40_md5 :
      ({ KE_rsa_export, CIPHER_rc2_40, HASH_md5 }),
#endif
   SSL_rsa_export_with_rc4_40_md5 :
      ({ KE_rsa_export, CIPHER_rc4_40, HASH_md5 }),
   SSL_dhe_dss_export_with_des40_cbc_sha :
      ({ KE_dhe_dss, CIPHER_des40, HASH_sha1 }),
   SSL_dhe_rsa_export_with_des40_cbc_sha :
      ({ KE_dhe_rsa, CIPHER_des40, HASH_sha1 }),
   SSL_dh_dss_export_with_des40_cbc_sha :
      ({ KE_dh_dss, CIPHER_des40, HASH_sha1 }),
   SSL_dh_rsa_export_with_des40_cbc_sha :
      ({ KE_dh_rsa, CIPHER_des40, HASH_sha1 }),
   SSL_rsa_export_with_des40_cbc_sha :
      ({ KE_rsa_export, CIPHER_des40, HASH_sha1 }),

   // NB: The IDEA and DES suites are obsolete in TLS 1.2 and later.
#if constant(Crypto.IDEA)
   SSL_rsa_with_idea_cbc_sha :		({ KE_rsa, CIPHER_idea, HASH_sha1 }),
   TLS_rsa_with_idea_cbc_sha :          ({ KE_rsa, CIPHER_idea, HASH_sha1 }),
   SSL_rsa_with_idea_cbc_md5 :		({ KE_rsa, CIPHER_idea, HASH_md5 }),
#endif
   SSL_rsa_with_des_cbc_sha :		({ KE_rsa, CIPHER_des, HASH_sha1 }),
   TLS_rsa_with_des_cbc_sha :           ({ KE_rsa, CIPHER_des, HASH_sha1 }),
   SSL_rsa_with_des_cbc_md5 :		({ KE_rsa, CIPHER_des, HASH_md5 }),
   SSL_dhe_dss_with_des_cbc_sha :	({ KE_dhe_dss, CIPHER_des, HASH_sha1 }),
   TLS_dhe_dss_with_des_cbc_sha :       ({ KE_dhe_dss, CIPHER_des, HASH_sha1 }),
   SSL_dhe_rsa_with_des_cbc_sha :	({ KE_dhe_rsa, CIPHER_des, HASH_sha1 }),
   TLS_dhe_rsa_with_des_cbc_sha :       ({ KE_dhe_rsa, CIPHER_des, HASH_sha1 }),
   SSL_dh_dss_with_des_cbc_sha :	({ KE_dh_dss, CIPHER_des, HASH_sha1 }),
   TLS_dh_dss_with_des_cbc_sha :        ({ KE_dh_dss, CIPHER_des, HASH_sha1 }),
   SSL_dh_rsa_with_des_cbc_sha :	({ KE_dh_rsa, CIPHER_des, HASH_sha1 }),
   TLS_dh_rsa_with_des_cbc_sha :        ({ KE_dh_rsa, CIPHER_des, HASH_sha1 }),

   SSL_rsa_with_rc4_128_sha :		({ KE_rsa, CIPHER_rc4, HASH_sha1 }),
   SSL_rsa_with_rc4_128_md5 :		({ KE_rsa, CIPHER_rc4, HASH_md5 }),
   TLS_dhe_dss_with_rc4_128_sha :	({ KE_dhe_dss, CIPHER_rc4, HASH_sha1 }),

   // These suites were used to test the TLS 1.0 key derivation
   // before TLS 1.0 was released.
   SSL_rsa_fips_with_des_cbc_sha :	({ KE_rsa_fips, CIPHER_des, HASH_sha1 }),
   SSL_rsa_fips_with_3des_ede_cbc_sha :	({ KE_rsa_fips, CIPHER_3des, HASH_sha1 }),
   SSL_rsa_oldfips_with_des_cbc_sha :	({ KE_rsa_fips, CIPHER_des, HASH_sha1 }),
   SSL_rsa_oldfips_with_3des_ede_cbc_sha :	({ KE_rsa_fips, CIPHER_3des, HASH_sha1 }),

   // Some anonymous diffie-hellman variants.
   SSL_dh_anon_export_with_rc4_40_md5:	({ KE_dh_anon, CIPHER_rc4_40, HASH_md5 }),
   SSL_dh_anon_export_with_des40_cbc_sha: ({ KE_dh_anon, CIPHER_des40, HASH_sha1 }),
   SSL_dh_anon_with_rc4_128_md5:	({ KE_dh_anon, CIPHER_rc4, HASH_md5 }),
   SSL_dh_anon_with_des_cbc_sha:	({ KE_dh_anon, CIPHER_des, HASH_sha1 }),
   TLS_dh_anon_with_des_cbc_sha:        ({ KE_dh_anon, CIPHER_des, HASH_sha1 }),
   SSL_dh_anon_with_3des_ede_cbc_sha:	({ KE_dh_anon, CIPHER_3des, HASH_sha1 }),
   TLS_dh_anon_with_aes_128_cbc_sha:	({ KE_dh_anon, CIPHER_aes, HASH_sha1 }),
   TLS_dh_anon_with_aes_256_cbc_sha:	({ KE_dh_anon, CIPHER_aes256, HASH_sha1 }),
   TLS_dh_anon_with_aes_128_cbc_sha256: ({ KE_dh_anon, CIPHER_aes, HASH_sha256, MODE_cbc }),
   TLS_dh_anon_with_aes_256_cbc_sha256: ({ KE_dh_anon, CIPHER_aes256, HASH_sha256, MODE_cbc }),
#if constant(Crypto.ECC.Curve)
   TLS_ecdh_anon_with_null_sha:		({ KE_ecdh_anon, 0, HASH_sha1 }),
   TLS_ecdh_anon_with_rc4_128_sha:	({ KE_ecdh_anon, CIPHER_rc4, HASH_sha1 }),
   TLS_ecdh_anon_with_3des_ede_cbc_sha:	({ KE_ecdh_anon, CIPHER_3des, HASH_sha1 }),
   TLS_ecdh_anon_with_aes_128_cbc_sha:	({ KE_ecdh_anon, CIPHER_aes, HASH_sha1 }),
   TLS_ecdh_anon_with_aes_256_cbc_sha:	({ KE_ecdh_anon, CIPHER_aes256, HASH_sha1 }),
#endif /* Crypto.ECC.Curve */

   // Required by TLS 1.0 @rfc{2246@} 9.
   SSL_dhe_dss_with_3des_ede_cbc_sha :	({ KE_dhe_dss, CIPHER_3des, HASH_sha1 }),

   // Required by TLS 1.1 @rfc{4346@} 9.
   SSL_rsa_with_3des_ede_cbc_sha :	({ KE_rsa, CIPHER_3des, HASH_sha1 }),

   // Required by TLS 1.2 @rfc{5246@} 9.
   TLS_rsa_with_aes_128_cbc_sha :	({ KE_rsa, CIPHER_aes, HASH_sha1 }),

   SSL_rsa_with_3des_ede_cbc_md5 :	({ KE_rsa, CIPHER_3des, HASH_md5 }),
   SSL_dhe_rsa_with_3des_ede_cbc_sha :	({ KE_dhe_rsa, CIPHER_3des, HASH_sha1 }),
   SSL_dh_dss_with_3des_ede_cbc_sha :	({ KE_dh_dss, CIPHER_3des, HASH_sha1 }),
   SSL_dh_rsa_with_3des_ede_cbc_sha :	({ KE_dh_rsa, CIPHER_3des, HASH_sha1 }),

   TLS_dhe_dss_with_aes_128_cbc_sha :	({ KE_dhe_dss, CIPHER_aes, HASH_sha1 }),
   TLS_dhe_rsa_with_aes_128_cbc_sha :	({ KE_dhe_rsa, CIPHER_aes, HASH_sha1 }),
   TLS_dh_dss_with_aes_128_cbc_sha :	({ KE_dh_dss, CIPHER_aes, HASH_sha1 }),
   TLS_dh_rsa_with_aes_128_cbc_sha :	({ KE_dh_rsa, CIPHER_aes, HASH_sha1 }),
   TLS_rsa_with_aes_256_cbc_sha :	({ KE_rsa, CIPHER_aes256, HASH_sha1 }),
   TLS_dhe_dss_with_aes_256_cbc_sha :	({ KE_dhe_dss, CIPHER_aes256, HASH_sha1 }),
   TLS_dhe_rsa_with_aes_256_cbc_sha :	({ KE_dhe_rsa, CIPHER_aes256, HASH_sha1 }),
   TLS_dh_dss_with_aes_256_cbc_sha :	({ KE_dh_dss, CIPHER_aes256, HASH_sha1 }),
   TLS_dh_rsa_with_aes_256_cbc_sha :	({ KE_dh_rsa, CIPHER_aes256, HASH_sha1 }),

#if constant(Crypto.ECC.Curve)
   // Suites from @rfc{4492@} (TLSECC)
   TLS_ecdh_ecdsa_with_null_sha : ({ KE_ecdh_ecdsa, 0, HASH_sha1 }),
   TLS_ecdh_ecdsa_with_rc4_128_sha : ({ KE_ecdh_ecdsa, CIPHER_rc4, HASH_sha1 }),
   TLS_ecdh_ecdsa_with_3des_ede_cbc_sha : ({ KE_ecdh_ecdsa, CIPHER_3des, HASH_sha1 }),
   TLS_ecdh_ecdsa_with_aes_128_cbc_sha : ({ KE_ecdh_ecdsa, CIPHER_aes, HASH_sha1 }),
   TLS_ecdh_ecdsa_with_aes_256_cbc_sha : ({ KE_ecdh_ecdsa, CIPHER_aes256, HASH_sha1 }),

   TLS_ecdhe_ecdsa_with_null_sha :	({ KE_ecdhe_ecdsa, 0, HASH_sha1 }),
   TLS_ecdhe_ecdsa_with_rc4_128_sha :	({ KE_ecdhe_ecdsa, CIPHER_rc4, HASH_sha1 }),
   TLS_ecdhe_ecdsa_with_3des_ede_cbc_sha : ({ KE_ecdhe_ecdsa, CIPHER_3des, HASH_sha1 }),
   TLS_ecdhe_ecdsa_with_aes_128_cbc_sha : ({ KE_ecdhe_ecdsa, CIPHER_aes, HASH_sha1 }),
   TLS_ecdhe_ecdsa_with_aes_256_cbc_sha : ({ KE_ecdhe_ecdsa, CIPHER_aes256, HASH_sha1 }),

   TLS_ecdh_rsa_with_null_sha : ({ KE_ecdh_rsa, 0, HASH_sha1 }),
   TLS_ecdh_rsa_with_rc4_128_sha : ({ KE_ecdh_rsa, CIPHER_rc4, HASH_sha1 }),
   TLS_ecdh_rsa_with_3des_ede_cbc_sha : ({ KE_ecdh_rsa, CIPHER_3des, HASH_sha1 }),
   TLS_ecdh_rsa_with_aes_128_cbc_sha : ({ KE_ecdh_rsa, CIPHER_aes, HASH_sha1 }),
   TLS_ecdh_rsa_with_aes_256_cbc_sha : ({ KE_ecdh_rsa, CIPHER_aes256, HASH_sha1 }),

   TLS_ecdhe_rsa_with_null_sha :	({ KE_ecdhe_rsa, 0, HASH_sha1 }),
   TLS_ecdhe_rsa_with_rc4_128_sha :	({ KE_ecdhe_rsa, CIPHER_rc4, HASH_sha1 }),
   TLS_ecdhe_rsa_with_3des_ede_cbc_sha : ({ KE_ecdhe_rsa, CIPHER_3des, HASH_sha1 }),
   TLS_ecdhe_rsa_with_aes_128_cbc_sha :	({ KE_ecdhe_rsa, CIPHER_aes, HASH_sha1 }),
   TLS_ecdhe_rsa_with_aes_256_cbc_sha :	({ KE_ecdhe_rsa, CIPHER_aes256, HASH_sha1 }),

   // Suites from @rfc{7251@}
   // These are AEAD suites, and thus not valid for TLS prior to TLS 1.2.
   TLS_ecdhe_ecdsa_with_aes_128_ccm :	({ KE_ecdhe_ecdsa, CIPHER_aes, HASH_sha256, MODE_ccm }),
TLS_ecdhe_ecdsa_with_aes_256_ccm :	({ KE_ecdhe_ecdsa, CIPHER_aes256, HASH_sha256, MODE_ccm }),
TLS_ecdhe_ecdsa_with_aes_128_ccm_8 :	({ KE_ecdhe_ecdsa, CIPHER_aes, HASH_sha256, MODE_ccm_8 }),
TLS_ecdhe_ecdsa_with_aes_256_ccm_8 :	({ KE_ecdhe_ecdsa, CIPHER_aes256, HASH_sha256, MODE_ccm_8 }),

#endif /* Crypto.ECC.Curve */


   // Suites from @rfc{5246@} (TLS 1.2)
   TLS_rsa_with_aes_128_cbc_sha256 :    ({ KE_rsa, CIPHER_aes, HASH_sha256, MODE_cbc }),
   TLS_dhe_rsa_with_aes_128_cbc_sha256 : ({ KE_dhe_rsa, CIPHER_aes, HASH_sha256, MODE_cbc }),
   TLS_dhe_dss_with_aes_128_cbc_sha256 : ({ KE_dhe_dss, CIPHER_aes, HASH_sha256, MODE_cbc }),
   TLS_dh_rsa_with_aes_128_cbc_sha256 : ({ KE_dh_rsa, CIPHER_aes, HASH_sha256, MODE_cbc }),
   TLS_dh_dss_with_aes_128_cbc_sha256 : ({ KE_dh_dss, CIPHER_aes, HASH_sha256, MODE_cbc }),
   TLS_rsa_with_aes_256_cbc_sha256 :	({ KE_rsa, CIPHER_aes256, HASH_sha256, MODE_cbc }),
   TLS_dhe_rsa_with_aes_256_cbc_sha256 : ({ KE_dhe_rsa, CIPHER_aes256, HASH_sha256, MODE_cbc }),
   TLS_dhe_dss_with_aes_256_cbc_sha256 : ({ KE_dhe_dss, CIPHER_aes256, HASH_sha256, MODE_cbc }),
   TLS_dh_rsa_with_aes_256_cbc_sha256 : ({ KE_dh_rsa, CIPHER_aes256, HASH_sha256, MODE_cbc }),
   TLS_dh_dss_with_aes_256_cbc_sha256 : ({ KE_dh_dss, CIPHER_aes256, HASH_sha256, MODE_cbc }),

#if constant(Crypto.ECC.Curve)
   // Suites from @rfc{5289@}
   // Note that these are not valid for TLS versions prior to TLS 1.2.
   TLS_ecdhe_ecdsa_with_aes_128_cbc_sha256 : ({ KE_ecdhe_ecdsa, CIPHER_aes, HASH_sha256, MODE_cbc }),
   TLS_ecdh_ecdsa_with_aes_128_cbc_sha256 : ({ KE_ecdh_ecdsa, CIPHER_aes, HASH_sha256, MODE_cbc }),
   TLS_ecdhe_rsa_with_aes_128_cbc_sha256 : ({ KE_ecdhe_rsa, CIPHER_aes, HASH_sha256, MODE_cbc }),
   TLS_ecdh_rsa_with_aes_128_cbc_sha256 : ({ KE_ecdh_rsa, CIPHER_aes, HASH_sha256, MODE_cbc }),
#if constant(Crypto.SHA384)
   TLS_ecdhe_ecdsa_with_aes_256_cbc_sha384 : ({ KE_ecdhe_ecdsa, CIPHER_aes256, HASH_sha384, MODE_cbc }),
   TLS_ecdh_ecdsa_with_aes_256_cbc_sha384 : ({ KE_ecdh_ecdsa, CIPHER_aes256, HASH_sha384, MODE_cbc }),
   TLS_ecdhe_rsa_with_aes_256_cbc_sha384 : ({ KE_ecdhe_rsa, CIPHER_aes256, HASH_sha384, MODE_cbc }),
   TLS_ecdh_rsa_with_aes_256_cbc_sha384 : ({ KE_ecdh_rsa, CIPHER_aes256, HASH_sha384, MODE_cbc }),
#endif /* Crypto.SHA384 */
#endif /* Crypto.ECC.Curve */

   // Suites from @rfc{6655@}
   // These are AEAD suites, and thus not valid for TLS prior to TLS 1.2.
   TLS_rsa_with_aes_128_ccm: ({ KE_rsa, CIPHER_aes, HASH_sha256, MODE_ccm }),
   TLS_rsa_with_aes_256_ccm: ({ KE_rsa, CIPHER_aes256, HASH_sha256, MODE_ccm }),
   TLS_dhe_rsa_with_aes_128_ccm: ({ KE_dhe_rsa, CIPHER_aes, HASH_sha256, MODE_ccm }),
   TLS_dhe_rsa_with_aes_256_ccm: ({ KE_dhe_rsa, CIPHER_aes256, HASH_sha256, MODE_ccm }),
   TLS_rsa_with_aes_128_ccm_8: ({ KE_rsa, CIPHER_aes, HASH_sha256, MODE_ccm_8 }),
   TLS_rsa_with_aes_256_ccm_8: ({ KE_rsa, CIPHER_aes256, HASH_sha256, MODE_ccm_8 }),
   TLS_dhe_rsa_with_aes_128_ccm_8: ({ KE_dhe_rsa, CIPHER_aes, HASH_sha256, MODE_ccm_8 }),
   TLS_dhe_rsa_with_aes_256_ccm_8: ({ KE_dhe_rsa, CIPHER_aes256, HASH_sha256, MODE_ccm_8 }),

#if constant(Crypto.Camellia)
   // Camellia Suites:
   TLS_rsa_with_camellia_128_cbc_sha:	({ KE_rsa, CIPHER_camellia128, HASH_sha1 }),
   TLS_dhe_dss_with_camellia_128_cbc_sha: ({ KE_dhe_dss, CIPHER_camellia128, HASH_sha1 }),
   TLS_dhe_rsa_with_camellia_128_cbc_sha: ({ KE_dhe_rsa, CIPHER_camellia128, HASH_sha1 }),
   TLS_dh_dss_with_camellia_128_cbc_sha: ({ KE_dh_dss, CIPHER_camellia128, HASH_sha1 }),
   TLS_dh_rsa_with_camellia_128_cbc_sha: ({ KE_dh_rsa, CIPHER_camellia128, HASH_sha1 }),
   TLS_rsa_with_camellia_256_cbc_sha:	({ KE_rsa, CIPHER_camellia256, HASH_sha1 }),
   TLS_dhe_dss_with_camellia_256_cbc_sha: ({ KE_dhe_dss, CIPHER_camellia256, HASH_sha1 }),
   TLS_dhe_rsa_with_camellia_256_cbc_sha: ({ KE_dhe_rsa, CIPHER_camellia256, HASH_sha1 }),
   TLS_dh_dss_with_camellia_256_cbc_sha: ({ KE_dh_dss, CIPHER_camellia256, HASH_sha1 }),
   TLS_dh_rsa_with_camellia_256_cbc_sha: ({ KE_dh_rsa, CIPHER_camellia256, HASH_sha1 }),

   TLS_rsa_with_camellia_128_cbc_sha256:	({ KE_rsa, CIPHER_camellia128, HASH_sha256, MODE_cbc }),
   TLS_dhe_dss_with_camellia_128_cbc_sha256: ({ KE_dhe_dss, CIPHER_camellia128, HASH_sha256, MODE_cbc }),
   TLS_dhe_rsa_with_camellia_128_cbc_sha256: ({ KE_dhe_rsa, CIPHER_camellia128, HASH_sha256, MODE_cbc }),
   TLS_dh_dss_with_camellia_128_cbc_sha256: ({ KE_dh_dss, CIPHER_camellia128, HASH_sha256, MODE_cbc }),
   TLS_dh_rsa_with_camellia_128_cbc_sha256: ({ KE_dh_rsa, CIPHER_camellia128, HASH_sha256, MODE_cbc }),
   TLS_rsa_with_camellia_256_cbc_sha256:	({ KE_rsa, CIPHER_camellia256, HASH_sha256, MODE_cbc }),
   TLS_dhe_dss_with_camellia_256_cbc_sha256: ({ KE_dhe_dss, CIPHER_camellia256, HASH_sha256, MODE_cbc }),
   TLS_dhe_rsa_with_camellia_256_cbc_sha256: ({ KE_dhe_rsa, CIPHER_camellia256, HASH_sha256, MODE_cbc }),
   TLS_dh_dss_with_camellia_256_cbc_sha256: ({ KE_dh_dss, CIPHER_camellia256, HASH_sha256, MODE_cbc }),
   TLS_dh_rsa_with_camellia_256_cbc_sha256: ({ KE_dh_rsa, CIPHER_camellia256, HASH_sha256, MODE_cbc }),

   // Anonymous variants:
   TLS_dh_anon_with_camellia_128_cbc_sha: ({ KE_dh_anon, CIPHER_camellia128, HASH_sha1 }),
   TLS_dh_anon_with_camellia_256_cbc_sha: ({ KE_dh_anon, CIPHER_camellia256, HASH_sha1 }),
   TLS_dh_anon_with_camellia_128_cbc_sha256: ({ KE_dh_anon, CIPHER_camellia128, HASH_sha256, MODE_cbc }),
   TLS_dh_anon_with_camellia_256_cbc_sha256: ({ KE_dh_anon, CIPHER_camellia256, HASH_sha256, MODE_cbc }),

#if constant(Crypto.ECC.Curve)
   // From @rfc{6367@}
   // Note that this @rfc{explicitly@} allows use of these suites
   // with TLS versions prior to TLS 1.2 (@rfc{6367@} 3.3).
   TLS_ecdh_ecdsa_with_camellia_128_cbc_sha256: ({ KE_ecdh_ecdsa, CIPHER_camellia128, HASH_sha256 }),
   TLS_ecdh_rsa_with_camellia_128_cbc_sha256: ({ KE_ecdh_rsa, CIPHER_camellia128, HASH_sha256 }),
   TLS_ecdhe_ecdsa_with_camellia_128_cbc_sha256: ({ KE_ecdhe_ecdsa, CIPHER_camellia128, HASH_sha256 }),
   TLS_ecdhe_rsa_with_camellia_128_cbc_sha256: ({ KE_ecdhe_rsa, CIPHER_camellia128, HASH_sha256 }),
#if constant(Crypto.SHA384)
   TLS_ecdh_ecdsa_with_camellia_256_cbc_sha384: ({ KE_ecdh_ecdsa, CIPHER_camellia256, HASH_sha384 }),
   TLS_ecdh_rsa_with_camellia_256_cbc_sha384: ({ KE_ecdh_rsa, CIPHER_camellia256, HASH_sha384 }),
   TLS_ecdhe_ecdsa_with_camellia_256_cbc_sha384: ({ KE_ecdhe_ecdsa, CIPHER_camellia256, HASH_sha384 }),
   TLS_ecdhe_rsa_with_camellia_256_cbc_sha384: ({ KE_ecdhe_rsa, CIPHER_camellia256, HASH_sha384 }),
#endif /* Crypto.SHA384 */
#endif /* Crypto.ECC.Curve */
#endif /* Crypto.Camellia */

#if constant(Crypto.AES.GCM)
   // GCM Suites:
   TLS_rsa_with_aes_128_gcm_sha256:	({ KE_rsa, CIPHER_aes, HASH_sha256, MODE_gcm }),
   TLS_dhe_rsa_with_aes_128_gcm_sha256:	({ KE_dhe_rsa, CIPHER_aes, HASH_sha256, MODE_gcm }),
   TLS_dhe_dss_with_aes_128_gcm_sha256:	({ KE_dhe_dss, CIPHER_aes, HASH_sha256, MODE_gcm }),
   TLS_dh_rsa_with_aes_128_gcm_sha256:	({ KE_dh_rsa, CIPHER_aes, HASH_sha256, MODE_gcm }),
   TLS_dh_dss_with_aes_128_gcm_sha256:	({ KE_dh_dss, CIPHER_aes, HASH_sha256, MODE_gcm }),

#if constant(Crypto.SHA384)
   TLS_rsa_with_aes_256_gcm_sha384:	({ KE_rsa, CIPHER_aes256, HASH_sha384, MODE_gcm }),
   TLS_dhe_rsa_with_aes_256_gcm_sha384:	({ KE_dhe_rsa, CIPHER_aes256, HASH_sha384, MODE_gcm }),
   TLS_dhe_dss_with_aes_256_gcm_sha384:	({ KE_dhe_dss, CIPHER_aes256, HASH_sha384, MODE_gcm }),
   TLS_dh_rsa_with_aes_256_gcm_sha384:	({ KE_dh_rsa, CIPHER_aes256, HASH_sha384, MODE_gcm }),
   TLS_dh_dss_with_aes_256_gcm_sha384:	({ KE_dh_dss, CIPHER_aes256, HASH_sha384, MODE_gcm }),
#endif /* Crypto.SHA384 */

#if constant(Crypto.ECC.Curve)
   TLS_ecdhe_ecdsa_with_aes_128_gcm_sha256: ({ KE_ecdhe_ecdsa, CIPHER_aes, HASH_sha256, MODE_gcm }),
   TLS_ecdh_ecdsa_with_aes_128_gcm_sha256: ({ KE_ecdh_ecdsa, CIPHER_aes, HASH_sha256, MODE_gcm }),
   TLS_ecdhe_rsa_with_aes_128_gcm_sha256: ({ KE_ecdhe_rsa, CIPHER_aes, HASH_sha256, MODE_gcm }),
   TLS_ecdh_rsa_with_aes_128_gcm_sha256: ({ KE_ecdh_rsa, CIPHER_aes, HASH_sha256, MODE_gcm }),

#if constant(Crypto.SHA384)
   TLS_ecdhe_ecdsa_with_aes_256_gcm_sha384: ({ KE_ecdhe_ecdsa, CIPHER_aes256, HASH_sha384, MODE_gcm }),
   TLS_ecdh_ecdsa_with_aes_256_gcm_sha384: ({ KE_ecdh_ecdsa, CIPHER_aes256, HASH_sha384, MODE_gcm }),
   TLS_ecdhe_rsa_with_aes_256_gcm_sha384: ({ KE_ecdhe_rsa, CIPHER_aes256, HASH_sha384, MODE_gcm }),
   TLS_ecdh_rsa_with_aes_256_gcm_sha384: ({ KE_ecdh_rsa, CIPHER_aes256, HASH_sha384, MODE_gcm }),
#endif /* Crypto.SHA384 */
#endif /* Crypto.ECC.Curve */

   // Anonymous variants:
   TLS_dh_anon_with_aes_128_gcm_sha256: ({ KE_dh_anon, CIPHER_aes, HASH_sha256, MODE_gcm }),
#if constant(Crypto.SHA384)
   TLS_dh_anon_with_aes_256_gcm_sha384: ({ KE_dh_anon, CIPHER_aes256, HASH_sha384, MODE_gcm }),
#endif /* Crypto.SHA384 */

#if constant(Crypto.Camellia)
   // Camellia and GCM.
   TLS_rsa_with_camellia_128_gcm_sha256:({ KE_rsa, CIPHER_camellia128, HASH_sha256, MODE_gcm }),
   TLS_dhe_rsa_with_camellia_128_gcm_sha256:({ KE_dhe_rsa, CIPHER_camellia128, HASH_sha256, MODE_gcm }),
   TLS_dhe_dss_with_camellia_128_gcm_sha256:({ KE_dhe_dss, CIPHER_camellia128, HASH_sha256, MODE_gcm }),
   TLS_dh_rsa_with_camellia_128_gcm_sha256:({ KE_dh_rsa, CIPHER_camellia128, HASH_sha256, MODE_gcm }),
   TLS_dh_dss_with_camellia_128_gcm_sha256:({ KE_dh_dss, CIPHER_camellia128, HASH_sha256, MODE_gcm }),
#if constant(Crypto.SHA384)
   TLS_rsa_with_camellia_256_gcm_sha384:({ KE_rsa, CIPHER_camellia256, HASH_sha384, MODE_gcm }),
   TLS_dhe_rsa_with_camellia_256_gcm_sha384:({ KE_dhe_rsa, CIPHER_camellia256, HASH_sha384, MODE_gcm }),
   TLS_dhe_dss_with_camellia_256_gcm_sha384:({ KE_dhe_dss, CIPHER_camellia256, HASH_sha384, MODE_gcm }),
   TLS_dh_rsa_with_camellia_256_gcm_sha384:({ KE_dh_rsa, CIPHER_camellia256, HASH_sha384, MODE_gcm }),
   TLS_dh_dss_with_camellia_256_gcm_sha384:({ KE_dh_dss, CIPHER_camellia256, HASH_sha384, MODE_gcm }),
#endif /* Crypto.SHA384 */

   // Anonymous variants:
   TLS_dh_anon_with_camellia_128_gcm_sha256: ({ KE_dh_anon, CIPHER_camellia128, HASH_sha256, MODE_gcm }),
#if constant(Crypto.SHA384)
   TLS_dh_anon_with_camellia_256_gcm_sha384: ({ KE_dh_anon, CIPHER_camellia256, HASH_sha384, MODE_gcm }),
#endif /* Crypto.SHA384 */

#if constant(Crypto.ECC.Curve)
   // From @rfc{6367@}
   TLS_ecdhe_ecdsa_with_camellia_128_gcm_sha256: ({ KE_ecdhe_ecdsa, CIPHER_camellia128, HASH_sha256, MODE_gcm }),
   TLS_ecdh_ecdsa_with_camellia_128_gcm_sha256: ({ KE_ecdh_ecdsa, CIPHER_camellia128, HASH_sha256, MODE_gcm }),
   TLS_ecdhe_rsa_with_camellia_128_gcm_sha256: ({ KE_ecdhe_rsa, CIPHER_camellia128, HASH_sha256, MODE_gcm }),
   TLS_ecdh_rsa_with_camellia_128_gcm_sha256: ({ KE_ecdh_rsa, CIPHER_camellia128, HASH_sha256, MODE_gcm }),
#if constant(Crypto.SHA384)
   TLS_ecdhe_ecdsa_with_camellia_256_gcm_sha384: ({ KE_ecdhe_ecdsa, CIPHER_camellia256, HASH_sha384, MODE_gcm }),
   TLS_ecdh_ecdsa_with_camellia_256_gcm_sha384: ({ KE_ecdh_ecdsa, CIPHER_camellia256, HASH_sha384, MODE_gcm }),
   TLS_ecdhe_rsa_with_camellia_256_gcm_sha384: ({ KE_ecdhe_rsa, CIPHER_camellia256, HASH_sha384, MODE_gcm }),
   TLS_ecdh_rsa_with_camellia_256_gcm_sha384: ({ KE_ecdh_rsa, CIPHER_camellia256, HASH_sha384, MODE_gcm }),
#endif /* Crypto.SHA384 */
#endif /* Crypto.ECC.Curve */
#endif /* Crypto.Camellia */
#endif /* Crypto.AES.GCM */

#if constant(Crypto.ChaCha20.POLY1305) && defined(NOT_BROKEN)
#if constant(Crypto.ECC.Curve)
   // Draft.
   TLS_ecdhe_rsa_with_chacha20_poly1305_sha256: ({ KE_ecdhe_rsa, CIPHER_chacha20, HASH_sha256, MODE_poly1305 }),
   TLS_ecdhe_ecdsa_with_chacha20_poly1305_sha256: ({ KE_ecdhe_ecdsa, CIPHER_chacha20, HASH_sha256, MODE_poly1305 }),
#endif /* Crypto.ECC.Curve */
   TLS_dhe_rsa_with_chacha20_poly1305_sha256: ({ KE_dhe_rsa, CIPHER_chacha20, HASH_sha256, MODE_poly1305 }),
#endif /* Crypto.ChaCha20.POLY1305 */

   // PSK without any KE
   TLS_psk_with_null_sha : ({ KE_psk, 0, HASH_sha1 }),
   TLS_psk_with_rc4_128_sha : ({ KE_psk, CIPHER_rc4, HASH_sha1 }),
   TLS_psk_with_3des_ede_cbc_sha : ({ KE_psk, CIPHER_3des, HASH_sha1 }),
   TLS_psk_with_aes_128_cbc_sha : ({ KE_psk, CIPHER_aes, HASH_sha1 }),
   TLS_psk_with_aes_256_cbc_sha : ({ KE_psk, CIPHER_aes256, HASH_sha1 }),
#if constant(Crypto.AES.GCM)
   TLS_psk_with_aes_128_gcm_sha256 : ({ KE_psk, CIPHER_aes, HASH_sha256, MODE_gcm }),
#if constant(Crypto.SHA384)
   TLS_psk_with_aes_256_gcm_sha384 : ({ KE_psk, CIPHER_aes256, HASH_sha384, MODE_gcm }),
#endif /* Crypto.SHA384 */
#endif /* Crypto.AES.GCM */
   TLS_psk_with_aes_128_cbc_sha256 : ({ KE_psk, CIPHER_aes, HASH_sha256, MODE_cbc }),
   TLS_psk_with_aes_256_cbc_sha384 : ({ KE_psk, CIPHER_aes256, HASH_sha384, MODE_cbc }),
   TLS_psk_with_null_sha256 : ({ KE_psk, 0, HASH_sha256, MODE_cbc }),
#if constant(Crypto.SHA384)
   TLS_psk_with_null_sha384 : ({ KE_psk, 0, HASH_sha384, MODE_cbc }),
#endif /* Crypto.SHA384 */
#if constant(Crypto.Camellia)
#if constant(Crypto.Camellia.GCM)
   TLS_psk_with_camellia_128_gcm_sha256 : ({ KE_psk, CIPHER_camellia128, HASH_sha256, MODE_gcm }),
#if constant(Crypto.SHA384)
   TLS_psk_with_camellia_256_gcm_sha384 : ({ KE_psk, CIPHER_camellia256, HASH_sha384, MODE_gcm }),
#endif /* Crypto.SHA384 */
#endif /* Crypto.Camellia.GCM */
   TLS_psk_with_camellia_128_cbc_sha256 : ({ KE_psk, CIPHER_camellia128, HASH_sha256, MODE_cbc }),
#if constant(Crypto.SHA384)
   TLS_psk_with_camellia_256_cbc_sha384 : ({ KE_psk, CIPHER_camellia256, HASH_sha384, MODE_cbc }),
#endif /* Crypto.SHA384 */
#endif /* Crypto.Camellia */
   TLS_psk_with_aes_128_ccm : ({ KE_psk, CIPHER_aes, HASH_sha256, MODE_ccm }),
   TLS_psk_with_aes_256_ccm : ({ KE_psk, CIPHER_aes256, HASH_sha256, MODE_ccm }),
   TLS_psk_with_aes_128_ccm_8 : ({ KE_psk, CIPHER_aes, HASH_sha256, MODE_ccm_8 }),
   TLS_psk_with_aes_256_ccm_8 : ({ KE_psk, CIPHER_aes256, HASH_sha256, MODE_ccm_8 }),

   // PSK with DHE
   TLS_dhe_psk_with_null_sha : ({ KE_dhe_psk, 0, HASH_sha1 }),
   TLS_dhe_psk_with_rc4_128_sha : ({ KE_dhe_psk, CIPHER_rc4, HASH_sha1 }),
   TLS_dhe_psk_with_3des_ede_cbc_sha : ({ KE_dhe_psk, CIPHER_3des, HASH_sha1 }),
   TLS_dhe_psk_with_aes_128_cbc_sha : ({ KE_dhe_psk, CIPHER_aes, HASH_sha1 }),
   TLS_dhe_psk_with_aes_256_cbc_sha : ({ KE_dhe_psk, CIPHER_aes256, HASH_sha1 }),
#if constant(Crypto.AES.GCM)
   TLS_dhe_psk_with_aes_128_gcm_sha256 : ({ KE_dhe_psk, CIPHER_aes, HASH_sha256, MODE_gcm }),
#if constant(Crypto.SHA384)
   TLS_dhe_psk_with_aes_256_gcm_sha384 : ({ KE_dhe_psk, CIPHER_aes256, HASH_sha384, MODE_gcm }),
#endif /* Crypto.SHA384 */
#endif /* Crypto.AES.GCM */
   TLS_dhe_psk_with_aes_128_cbc_sha256 : ({ KE_dhe_psk, CIPHER_aes, HASH_sha256, MODE_cbc }),
#if constant(Crypto.SHA384)
   TLS_dhe_psk_with_aes_256_cbc_sha384 : ({ KE_dhe_psk, CIPHER_aes256, HASH_sha384, MODE_cbc }),
#endif /* Crypto.SHA384 */
   TLS_dhe_psk_with_null_sha256 : ({ KE_dhe_psk, 0, HASH_sha256, MODE_cbc }),
#if constant(Crypto.SHA384)
   TLS_dhe_psk_with_null_sha384 : ({ KE_dhe_psk, 0, HASH_sha384, MODE_cbc }),
#endif /* Crypto.SHA384 */
#if constant(Crypto.Camellia)
#if constant(Crypto.Camellia.GCM)
   TLS_dhe_psk_with_camellia_128_gcm_sha256 : ({ KE_dhe_psk, CIPHER_camellia128, HASH_sha256, MODE_gcm }),
#if constant(Crypto.SHA384)
   TLS_dhe_psk_with_camellia_256_gcm_sha384 : ({ KE_dhe_psk, CIPHER_camellia256, HASH_sha384, MODE_gcm }),
#endif /* Crypto.SHA384 */
#endif /* Crypto.Camellia.GCM */
   TLS_dhe_psk_with_camellia_128_cbc_sha256 : ({ KE_dhe_psk, CIPHER_camellia128, HASH_sha256, MODE_cbc }),
#if constant(Crypto.SHA384)
   TLS_dhe_psk_with_camellia_256_cbc_sha384 : ({ KE_dhe_psk, CIPHER_camellia256, HASH_sha384, MODE_cbc }),
#endif /* Crypto.SHA384 */
#endif /* Crypto.Camellia */
   TLS_dhe_psk_with_aes_128_ccm : ({ KE_dhe_psk, CIPHER_aes, HASH_sha256, MODE_ccm }),
   TLS_dhe_psk_with_aes_256_ccm : ({ KE_dhe_psk, CIPHER_aes256, HASH_sha256, MODE_ccm }),
   TLS_psk_dhe_with_aes_128_ccm_8 : ({ KE_dhe_psk, CIPHER_aes, HASH_sha256, MODE_ccm_8 }),
   TLS_psk_dhe_with_aes_256_ccm_8 : ({ KE_dhe_psk, CIPHER_aes256, HASH_sha256, MODE_ccm_8 }),

   // PSK with RSA
   TLS_rsa_psk_with_null_sha : ({ KE_rsa_psk, 0, HASH_sha1 }),
   TLS_rsa_psk_with_rc4_128_sha : ({ KE_rsa_psk, CIPHER_rc4, HASH_sha1 }),
   TLS_rsa_psk_with_3des_ede_cbc_sha : ({ KE_rsa_psk, CIPHER_3des, HASH_sha1 }),
   TLS_rsa_psk_with_aes_128_cbc_sha : ({ KE_rsa_psk, CIPHER_aes, HASH_sha1 }),
   TLS_rsa_psk_with_aes_256_cbc_sha : ({ KE_rsa_psk, CIPHER_aes256, HASH_sha1 }),
#if constant(Crypto.AES.GCM)
   TLS_rsa_psk_with_aes_128_gcm_sha256 : ({ KE_rsa_psk, CIPHER_aes, HASH_sha256, MODE_gcm }),
#if constant(Crypto.SHA384)
   TLS_rsa_psk_with_aes_256_gcm_sha384 : ({ KE_rsa_psk, CIPHER_aes256, HASH_sha384, MODE_gcm }),
#endif /* Crypto.SHA384 */
#endif /* Crypto.AES.GCM */
   TLS_rsa_psk_with_aes_128_cbc_sha256 : ({ KE_rsa_psk, CIPHER_aes, HASH_sha256, MODE_cbc }),
#if constant(Crypto.SHA384)
   TLS_rsa_psk_with_aes_256_cbc_sha384 : ({ KE_rsa_psk, CIPHER_aes256, HASH_sha384, MODE_cbc }),
#endif /* Crypto.SHA384 */
   TLS_rsa_psk_with_null_sha256 : ({ KE_rsa_psk, 0, HASH_sha256, MODE_cbc }),
#if constant(Crypto.SHA384)
   TLS_rsa_psk_with_null_sha384 : ({ KE_rsa_psk, 0, HASH_sha384, MODE_cbc }),
#endif /* Crypto.SHA384 */
#if constant(Crypto.Camellia)
#if constant(Crypto.Camellia.GCM)
   TLS_rsa_psk_with_camellia_128_gcm_sha256 : ({ KE_rsa_psk, CIPHER_camellia128, HASH_sha256, MODE_gcm }),
#if constant(Crypto.SHA384)
   TLS_rsa_psk_with_camellia_256_gcm_sha384 : ({ KE_rsa_psk, CIPHER_camellia256, HASH_sha384, MODE_gcm }),
#endif /* Crypto.SHA384 */
#endif /* Crypto.Camellia.GCM */
   TLS_rsa_psk_with_camellia_128_cbc_sha256 : ({ KE_rsa_psk, CIPHER_camellia128, HASH_sha256, MODE_cbc }),
#if constant(Crypto.SHA384)
   TLS_rsa_psk_with_camellia_256_cbc_sha384 : ({ KE_rsa_psk, CIPHER_camellia256, HASH_sha384, MODE_cbc }),
#endif /* Crypto.SHA384 */
#endif /* Crypto.Camellia */

   // PSK with ECDHE
#if constant(Crypto.ECC.Curve)
   TLS_ecdhe_psk_with_null_sha : ({ KE_ecdhe_psk, 0, HASH_sha1 }),
   TLS_ecdhe_psk_with_null_sha256 : ({ KE_ecdhe_psk, 0, HASH_sha256, MODE_cbc }),
#if constant(Crypto.SHA384)
   TLS_ecdhe_psk_with_null_sha384 : ({ KE_ecdhe_psk, 0, HASH_sha384, MODE_cbc }),
#endif /* Crypto.SHA384 */
   TLS_ecdhe_psk_with_rc4_128_sha : ({ KE_ecdhe_psk, CIPHER_rc4, HASH_sha1 }),
   TLS_ecdhe_psk_with_3des_ede_cbc_sha : ({ KE_ecdhe_psk, CIPHER_3des, HASH_sha1 }),
   TLS_ecdhe_psk_with_aes_128_cbc_sha : ({ KE_ecdhe_psk, CIPHER_aes, HASH_sha1 }),
   TLS_ecdhe_psk_with_aes_256_cbc_sha : ({ KE_ecdhe_psk, CIPHER_aes256, HASH_sha1 }),
   TLS_ecdhe_psk_with_aes_128_cbc_sha256 : ({ KE_ecdhe_psk, CIPHER_aes, HASH_sha256, MODE_cbc }),
#if constant(Crypto.SHA384)
   TLS_ecdhe_psk_with_aes_256_cbc_sha384 : ({ KE_ecdhe_psk, CIPHER_aes256, HASH_sha384, MODE_cbc }),
#endif /* Crypto.SHA384 */
#if constant(Crypto.Camellia)
   TLS_ecdhe_psk_with_camellia_128_cbc_sha256 : ({ KE_ecdhe_psk, CIPHER_camellia128, HASH_sha256, MODE_cbc }),
#if constant(Crypto.SHA384)
   TLS_ecdhe_psk_with_camellia_256_cbc_sha384 : ({ KE_ecdhe_psk, CIPHER_camellia256, HASH_sha384, MODE_cbc }),
#endif /* Crypto.SHA384 */
#endif /* Crypto.Camellia */
   TLS_ecdhe_psk_with_aes_128_ccm_sha256 : ({ KE_ecdhe_psk, CIPHER_aes, HASH_sha256, MODE_ccm }),
   TLS_ecdhe_psk_with_aes_128_ccm_8_sha256 : ({ KE_ecdhe_psk, CIPHER_aes, HASH_sha256, MODE_ccm_8 }),
#if constant(Crypto.AES.GCM)
   TLS_ecdhe_psk_with_aes_128_gcm_sha256 : ({ KE_ecdhe_psk, CIPHER_aes, HASH_sha256, MODE_gcm }),
#if constant(Crypto.SHA384)
   TLS_ecdhe_psk_with_aes_256_gcm_sha384 : ({ KE_ecdhe_psk, CIPHER_aes256, HASH_sha384, MODE_gcm }),
#endif /* Crypto.SHA384 */
#endif /* Crypto.AES.GCM */
#endif /* Crypto.ECC.Curve */
]);

//! Handshake message types.
enum HandshakeMessageType {
  HANDSHAKE_hello_request	 = 0,	//! @rfc{5246@}
  HANDSHAKE_client_hello	 = 1,	//! @rfc{5246@}
  HANDSHAKE_server_hello	 = 2,	//! @rfc{5246@}
  HANDSHAKE_hello_verify_request = 3,	//! @rfc{6347@}
  HANDSHAKE_new_session_ticket	 = 4,	//! @rfc{4507@} / @rfc{5077@}
  HANDSHAKE_end_of_early_data	 = 5,	//! TLS 1.3 draft.
  HANDSHAKE_hello_retry_request	 = 6,	//! TLS 1.3 draft.
  HANDSHAKE_encrypted_extensions = 8,	//! TLS 1.3 draft.
  HANDSHAKE_certificate		 = 11,	//! @rfc{5246@}
  HANDSHAKE_server_key_exchange	 = 12,	//! @rfc{5246@}
  HANDSHAKE_certificate_request	 = 13,	//! @rfc{5246@}
  HANDSHAKE_server_hello_done	 = 14,	//! @rfc{5246@}
  HANDSHAKE_certificate_verify	 = 15,	//! @rfc{5246@}
  HANDSHAKE_client_key_exchange	 = 16,	//! @rfc{5246@}
  HANDSHAKE_finished		 = 20,	//! @rfc{5246@}
  HANDSHAKE_certificate_url	 = 21,	//! @rfc{6066@}
  HANDSHAKE_certificate_status   = 22,	//! @rfc{6066@}
  HANDSHAKE_supplemental_data    = 23,	//! @rfc{4680@}
  HANDSHAKE_key_update           = 24,	//! TLS 1.3 draft.
  HANDSHAKE_next_protocol	 = 67,	//! draft-agl-tls-nextprotoneg
  HANDSHAKE_message_hash         = 254, //! TLS 1.3 draft.
}

//! Don't request nor check any certificate.
constant AUTHLEVEL_none		= 0;

//! Don't request, but verify any certificate.
constant AUTHLEVEL_verify       = 1;

//! As a server, request a certificate, but don't require a response.
//! This AUTHLEVEL is not relevant for clients.
constant AUTHLEVEL_ask		= 2;

//! Require other party to send a valid certificate.
constant AUTHLEVEL_require	= 3;


//! Certificate Type.
enum CertType {
  /* FIXME: CERT_* would be better names for these constants */
  AUTH_rsa_sign		= 1,	//! SSL 3.0
  AUTH_dss_sign		= 2,	//! SSL 3.0
  AUTH_rsa_fixed_dh	= 3,	//! SSL 3.0
  AUTH_dss_fixed_dh	= 4,	//! SSL 3.0
  AUTH_rsa_ephemeral_dh	= 5,	//! SSL 3.0
  AUTH_dss_ephemeral_dh	= 6,	//! SSL 3.0
  AUTH_fortezza_kea	= 20,	//! SSL 3.0
  AUTH_fortezza_dms	= 20,
  AUTH_ecdsa_sign       = 64,	//! @rfc{4492@}.
  AUTH_rsa_fixed_ecdh   = 65,	//! @rfc{4492@}. Deprecated @rfc{8422:5.5@}.
  AUTH_ecdsa_fixed_ecdh = 66,	//! @rfc{4492@}. Deprecated @rfc{8422:5.5@}.
}

//! ECC curve types from @rfc{4492:5.4@} (ECCurveType).
enum CurveType {
  CURVETYPE_explicit_prime	= 1,	//! Deprecated @rfc{8422:5.4@}
  CURVETYPE_explicit_char2	= 2,	//! Deprecated @rfc{8422:5.4@}
  CURVETYPE_named_curve		= 3,	//! Curve or group from @[NamedGroup].
}

//! ECBasis types from @rfc{4492:5.4@} errata.
enum ECBasisType {
  ECBASIS_trinomial = 1,
  ECBASIS_pentanomial = 2,
}

//! Groups used for elliptic curves DHE (ECDHE) and finite field DH (FFDHE).
//!
//! @seealso
//!   @rfc{4492:5.1.1@} (NamedCurve) / TLS 1.3 7.4.2.5.2. */
enum NamedGroup {
  GROUP_sect163k1			= 1,	//! @rfc{4492@}
  GROUP_sect163r1			= 2,	//! @rfc{4492@}
  GROUP_sect163r2			= 3,	//! @rfc{4492@}
  GROUP_sect193r1			= 4,	//! @rfc{4492@}
  GROUP_sect193r2			= 5,	//! @rfc{4492@}
  GROUP_sect233k1			= 6,	//! @rfc{4492@}
  GROUP_sect233r1			= 7,	//! @rfc{4492@}
  GROUP_sect239k1			= 8,	//! @rfc{4492@}
  GROUP_sect283k1			= 9,	//! @rfc{4492@}
  GROUP_sect283r1			= 10,	//! @rfc{4492@}
  GROUP_sect409k1			= 11,	//! @rfc{4492@}
  GROUP_sect409r1			= 12,	//! @rfc{4492@}
  GROUP_sect571k1			= 13,	//! @rfc{4492@}
  GROUP_sect571r1			= 14,	//! @rfc{4492@}
  GROUP_secp160k1			= 15,	//! @rfc{4492@}
  GROUP_secp160r1			= 16,	//! @rfc{4492@}
  GROUP_secp160r2			= 17,	//! @rfc{4492@}
  GROUP_secp192k1			= 18,	//! @rfc{4492@}
  GROUP_secp192r1			= 19,	//! @rfc{4492@}
  GROUP_secp224k1			= 20,	//! @rfc{4492@}
  GROUP_secp224r1			= 21,	//! @rfc{4492@}
  GROUP_secp256k1			= 22,	//! @rfc{4492@}
  GROUP_secp256r1			= 23,	//! @rfc{4492@}
  GROUP_secp384r1			= 24,	//! @rfc{4492@}
  GROUP_secp521r1			= 25,	//! @rfc{4492@}

  GROUP_brainpoolP256r1			= 26,	//! @rfc{7027@}
  GROUP_brainpoolP384r1			= 27,	//! @rfc{7027@}
  GROUP_brainpoolP512r1			= 28,	//! @rfc{7027@}

  GROUP_x25519				= 29,	//! @rfc{8422@}
  GROUP_x448				= 30,	//! @rfc{8422@}

  GROUP_ffdhe2048			= 256,	//! @rfc{7919@}
  GROUP_ffdhe3072			= 257,	//! @rfc{7919@}
  GROUP_ffdhe4096			= 258,	//! @rfc{7919@}
  GROUP_ffdhe6144			= 259,	//! @rfc{7919@}
  GROUP_ffdhe8192			= 260,	//! @rfc{7919@}

  GROUP_ffdhe_private0			= 508,	//! @rfc{7919@}
  GROUP_ffdhe_private1			= 509,	//! @rfc{7919@}
  GROUP_ffdhe_private2			= 510,	//! @rfc{7919@}
  GROUP_ffdhe_private3			= 511,	//! @rfc{7919@}

  GROUP_arbitrary_explicit_prime_curves	= 0xFF01,	//! Deprecated @rfc{8422:5.1.1@}
  GROUP_arbitrary_explicit_char2_curves	= 0xFF02,	//! Deprecated @rfc{8422:5.1.1@}
}

//! Lookup for Pike ECC name to @[NamedGroup].
constant ECC_NAME_TO_CURVE = ([
  "SECP_192R1": GROUP_secp192r1,
  "SECP_224R1": GROUP_secp224r1,
  "SECP_256R1": GROUP_secp256r1,
  "SECP_384R1": GROUP_secp384r1,
  "SECP_521R1": GROUP_secp521r1,
  "Curve25519": GROUP_x25519,
  "Curve448": GROUP_x448,
]);

//! ECC point formats from @rfc{4492:5.1.2@} (ECPointFormat).
enum PointFormat {
  POINT_uncompressed = 0,
  POINT_ansiX962_compressed_prime = 1,
  POINT_ansiX962_compressed_char2 = 2,
}

//! Fragment lengths for @[EXTENSION_max_fragment_length].
enum FragmentLength {
  FRAGMENT_512	= 1,
  FRAGMENT_1024	= 2,
  FRAGMENT_2048	= 3,
  FRAGMENT_4096	= 4,
}

//! Certificate format types as per @rfc{6091@} and @rfc{7250@}.
enum CertificateType {
  CERTTYPE_x509 = 0,		// @rfc{6091@}
  CERTTYPE_openpgp = 1,		// @rfc{6091@}
  CERTTYPE_raw_public_key = 2,	// @rfc{7250@}
};

//! Values used for @tt{supp_data_type@} in @tt{SupplementalDataEntry@}
//! (cf @rfc{4681:3@}).
enum SupplementalDataType {
  SDT_user_mapping_data = 0,	// @rfc{4681@}
};

//! @rfc{4681:6@}.
enum UserMappingType {
  UMT_upn_domain_hint = 64,	// @rfc{4681@}
};

//! Client Hello extensions.
enum Extension {
  EXTENSION_server_name				= 0,	//! @rfc{6066@}
  EXTENSION_max_fragment_length			= 1,	//! @rfc{6066@}
  EXTENSION_client_certificate_url		= 2,	//! @rfc{6066@}
  EXTENSION_trusted_ca_keys			= 3,	//! @rfc{6066@}
  EXTENSION_truncated_hmac			= 4,	//! @rfc{6066@}
  EXTENSION_status_request			= 5,	//! @rfc{6066@}
  EXTENSION_user_mapping			= 6,	//! @rfc{4681@}
  EXTENSION_client_authz			= 7,	//! @rfc{5878@}
  EXTENSION_server_authz			= 8,	//! @rfc{5878@}
  EXTENSION_cert_type				= 9,	//! @rfc{6091@}
  EXTENSION_elliptic_curves			= 10,	//! @rfc{4492@}
  EXTENSION_ec_point_formats			= 11,	//! @rfc{4492@}
  EXTENSION_srp					= 12,	//! @rfc{5054@}
  EXTENSION_signature_algorithms		= 13,	//! @rfc{5246@}
  EXTENSION_use_srtp				= 14,	//! @rfc{5764@}
  EXTENSION_heartbeat				= 15,	//! @rfc{6520@}
  EXTENSION_application_layer_protocol_negotiation = 16,//! @rfc{7301@}
  EXTENSION_status_request_v2			= 17,	//! @rfc{6961@}
  EXTENSION_signed_certificate_timestamp	= 18,	//! @rfc{6962@}
  EXTENSION_client_certificate_type		= 19,	//! @rfc{7250@} (Only in registry!)
  EXTENSION_server_certificate_type		= 20,	//! @rfc{7250@} (Only in registry!)
  EXTENSION_padding				= 21,	//! @rfc{7685@}
  EXTENSION_encrypt_then_mac			= 22,	//! @rfc{7366@}
  EXTENSION_extended_master_secret		= 23,	//! @rfc{7627@}
  EXTENSION_session_ticket			= 35,	//! @rfc{4507@} / @rfc{5077@}
  EXTENSION_key_share			        = 40,	//! TLS 1.3 draft.
  EXTENSION_pre_shared_key                      = 41,   //! TLS 1.3 draft.
  EXTENSION_early_data				= 42,	//! TLS 1.3 draft.
  EXTENSION_supported_versions                  = 43,   //! TLS 1.3 draft.
  EXTENSION_cookie                              = 44,   //! TLS 1.3 draft.
  EXTENSION_psk_key_exchange_modes              = 45,   //! TLS 1.3 draft.
  EXTENSION_certificate_authorities             = 47,   //! TLS 1.3 draft.
  EXTENSION_oid_filters                         = 48,   //! TLS 1.3 draft.
  EXTENSION_post_handshake_auth                 = 49,   //! TLS 1.3 draft.
  EXTENSION_next_protocol_negotiation		= 13172,//! draft-agl-tls-nextprotoneg
  EXTENSION_origin_bound_certificates		= 13175,
  EXTENSION_encrypted_client_certificates	= 13180,
  EXTENSION_channel_id				= 30031,
  EXTENSION_channel_id_new			= 30032,
  EXTENSION_old_padding				= 35655,
  EXTENSION_renegotiation_info			= 0xff01,//! @rfc{5746@}
  EXTENSION_draft_version			= 0xff02,//! @url{https://github.com/tlswg/tls13-spec/wiki/Implementations@}
};

//! Lookup from @[NamedGroup] to Pike module implementing @[Crypto.ECC.Curve].
//!
//! @seealso
//!   @[FFDHE_GROUPS]
constant ECC_CURVES = ([
#if constant(Crypto.ECC.SECP_192R1)
  GROUP_secp192r1: Crypto.ECC.SECP_192R1,
#endif
#if constant(Crypto.ECC.SECP_224R1)
  GROUP_secp224r1: Crypto.ECC.SECP_224R1,
#endif
#if constant(Crypto.ECC.SECP_256R1)
  GROUP_secp256r1: Crypto.ECC.SECP_256R1,
#endif
#if constant(Crypto.ECC.SECP_384R1)
  GROUP_secp384r1: Crypto.ECC.SECP_384R1,
#endif
#if constant(Crypto.ECC.SECP_521R1)
  GROUP_secp521r1: Crypto.ECC.SECP_521R1,
#endif
#if constant(Crypto.ECC.Curve25519)
  GROUP_x25519: Crypto.ECC.Curve25519,
#endif
#if constant(Crypto.ECC.Curve448) && defined(SSL3_EXPERIMENTAL)
  GROUP_x448: Crypto.ECC.Curve448,
#endif
]);

//! Lookup from @[NamedGroup] to @[Crypto.DH.Parameters] object.
//!
//! @seealso
//!   @[ECC_CURVES], @[MODP_GROUPS]
constant FFDHE_GROUPS = ([
  GROUP_ffdhe2048: Crypto.DH.FFDHE2048,
  GROUP_ffdhe3072: Crypto.DH.FFDHE3072,
  GROUP_ffdhe4096: Crypto.DH.FFDHE4096,
  GROUP_ffdhe6144: Crypto.DH.FFDHE6144,
  GROUP_ffdhe8192: Crypto.DH.FFDHE8192,
]);

//! Lookup from @[NamedGroup] to @[Crypto.DH.Parameters] object.
//!
//! These groups have equivalent strength to the @[FFDHE_GROUPS],
//! but don't have codepoints of their own. As they are
//! popular groups to use for DHE, we also allow them.
//!
//! @seealso
//!   @[FFDHE_GROUPS], @[ECC_CURVES]
constant MODP_GROUPS = ([
  GROUP_ffdhe3072: Crypto.DH.MODPGroup15,
  GROUP_ffdhe4096: Crypto.DH.MODPGroup16,
  GROUP_ffdhe6144: Crypto.DH.MODPGroup17,
  GROUP_ffdhe8192: Crypto.DH.MODPGroup18,
]);

enum HeartBeatModeType {
  HEARTBEAT_MODE_disabled = 0,
  HEARTBEAT_MODE_peer_allowed_to_send = 1,
  HEARTBEAT_MODE_peer_not_allowed_to_send = 2,
};

enum HeartBeatMessageType {
  HEARTBEAT_MESSAGE_request = 1,
  HEARTBEAT_MESSAGE_response = 2,
};

//! Application Level Protocol Negotiation protocol identifiers.
//!
//! @seealso
//!   @[EXTENSION_application_layer_protocol_negotiation]
enum ALPNProtocol {
  ALPN_http_1_1		= "http/1.1",		//! @rfc{7301@}
  ALPN_spdy_1		= "spdy/1",		//! @rfc{7301@}
  ALPN_spdy_2		= "spdy/2",		//! @rfc{7301@}
  ALPN_spdy_3		= "spdy/3",		//! @rfc{7301@}
  ALPN_turn		= "stun.turn",		//! @rfc{7443@}
  ALPN_stun		= "stun.nat-discovery",	//! @rfc{7443@}
  ALPN_http_2		= "h2",			//! @rfc{7540@}
  ALPN_http_2_reserved	= "h2c",		//! @rfc{7540@}
};

//! Authorization data formats (@rfc{5878:2.3@}).
//!
//! @seealso
//!   @rfc{5755@}.
enum AuthzDataFormat {
  ADF_x509_attr_cert = 0,
  ADF_saml_assertion = 1,
  ADF_x509_attr_cert_url = 2,
  ADF_saml_assertion_url = 3,
};

//! Lookup from PKCS DER encoded string to @[SignatureScheme].
mapping(string(8bit):SignatureScheme)
  pkcs_der_to_sign_alg = ([
  // RSA
  Standards.PKCS.Identifiers.rsa_md5_id->get_der():
  HASH_md5 | SIGNATURE_rsa,
  Standards.PKCS.Identifiers.rsa_sha1_id->get_der():
  SIGNATURE_rsa_pkcs1_sha1,
  Standards.PKCS.Identifiers.rsa_sha256_id->get_der():
  SIGNATURE_rsa_pkcs1_sha256,
  Standards.PKCS.Identifiers.rsa_sha384_id->get_der():
  SIGNATURE_rsa_pkcs1_sha384,
  Standards.PKCS.Identifiers.rsa_sha512_id->get_der():
  SIGNATURE_rsa_pkcs1_sha256,

  // DSA
  Standards.PKCS.Identifiers.dsa_sha_id->get_der():
  HASH_sha1 | SIGNATURE_dsa,
  Standards.PKCS.Identifiers.dsa_sha224_id->get_der():
  HASH_sha224 | SIGNATURE_dsa,
  Standards.PKCS.Identifiers.dsa_sha256_id->get_der():
  HASH_sha256 | SIGNATURE_dsa,

  // ECDSA
  Standards.PKCS.Identifiers.ecdsa_sha1_id->get_der():
  SIGNATURE_ecdsa_sha1,
  Standards.PKCS.Identifiers.ecdsa_sha224_id->get_der():
  HASH_sha224 | SIGNATURE_ecdsa,
  Standards.PKCS.Identifiers.ecdsa_sha256_id->get_der():
  SIGNATURE_ecdsa_secp256r1_sha256,
  Standards.PKCS.Identifiers.ecdsa_sha384_id->get_der():
  SIGNATURE_ecdsa_secp384r1_sha384,
  Standards.PKCS.Identifiers.ecdsa_sha512_id->get_der():
  SIGNATURE_ecdsa_secp521r1_sha512,

  // EdDSA
  Standards.PKCS.Identifiers.eddsa25519_id->get_der():
  SIGNATURE_ed25519_intrinsic,
  Standards.PKCS.Identifiers.eddsa448_id->get_der():
  SIGNATURE_ed448_intrinsic,
]);

//! A chain of X509 certificates with corresponding private key.
//!
//! It also contains some derived metadata.
class CertificatePair
{
  //! Cerificate type for the leaf cert.
  //!
  //! One of the @[CertType] constants.
  CertType cert_type;

  //! Private key.
  Crypto.Sign.State key;

  //! Chain of certificates, root cert last.
  array(string(8bit)) certs;

  //! Array of DER for the issuers matching @[certs].
  array(string(8bit)) issuers;

  //! Array of commonName globs from the first certificate in @[certs].
  array(string(8bit)) globs;

  //! TLS 1.2-style hash and signature pairs matching the @[certs].
  array(SignatureScheme) sign_algs;

  //! Bitmask of the key exchange algorithms supported by the main certificate.
  //! This is used for TLS 1.1 and earlier.
  //! @seealso
  //!   @[ke_mask_invariant]
  int(0..) ke_mask;

  //! Bitmask of the key exchange algorithms supported by the main certificate.
  //! This is the same as @[ke_mask], but unified with respect to
  //! @[KE_dh_dss]/@[KE_dh_rsa] and @[KE_ecdh_ecdsa]/@[KE_ecdh_rsa],
  //! as supported by TLS 1.2 and later.
  int(0..) ke_mask_invariant;

  // Returns the comparable strength of the leaf certificate in bits.
  protected int bit_strength(int bits, int sign)
  {
    // Adjust the bits to be comparable for the different algorithms.
    switch(sign) {
    case SIGNATURE_rsa:
      // The normative size.
      break;
    case SIGNATURE_dsa:
      // The consensus seems to be that DSA keys are about
      // the same strength as the corresponding RSA length.
      break;
    case SIGNATURE_ecdsa:
      // ECDSA size:	NIST says:		Our approximation:
      //   160 bits	~1024 bits RSA		960 bits RSA
      //   224 bits	~2048 bits RSA		2240 bits RSA
      //   256 bits	~4096 bits RSA		3072 bits RSA
      //   384 bits	~7680 bits RSA		7680 bits RSA
      //   521 bits	~15360 bits RSA		14881 bits RSA
      bits = (bits * (bits - 64))>>4;
      if (bits < 0) bits = 128;
      break;
    }
    return bits;
  }

  // Comparison operator that sorts the CertificatePairs according to
  // their relative strength.
  protected int(0..1) `<(mixed o)
  {
    if(!objectp(o)) return this < o;
    if( !o->key || !o->sign_algs ) return this < o;

    int s = sign_algs[0] & SIGNATURE_MASK;
    int os = o->sign_algs[0] & SIGNATURE_MASK;

    // FIXME: Let hash bits influence strength. The signature bits
    // doesn't overshadow hash completely.

    // FIXME: We only look at leaf certificate. We could look at
    // weakest link in the chain.

    // These tests are reversed to reverse-sort the certificates
    // (Strongest first).
    int bs = bit_strength(key->key_size(), s);
    int obs = bit_strength(o->key->key_size(), os);
    if( bs < obs ) return 0;
    if( bs > obs ) return 1;

    int h = sign_algs[0] & HASH_MASK;
    int oh = o->sign_algs[0] & HASH_MASK;
    if( h < oh ) return 0;
    if( h > oh ) return 1;

    if( s < os ) return 0;
    return 1;
  }

  // Set the globs array based on certificate common name and subject
  // alternative name extension.
  protected void set_globs(Standards.X509.TBSCertificate tbs,
                           array(string(8bit))|void extra)
  {
    globs = Standards.PKCS.Certificate.
      decode_distinguished_name(tbs->subject)->commonName - ({ 0 });

    if( tbs->ext_subjectAltName_dNSName )
      globs += tbs->ext_subjectAltName_dNSName;

    if (extra) globs += extra;

    if (!sizeof(globs)) error("No common name.\n");

    globs = Array.uniq( map(globs, lower_case) );
  }

  //! Initializa a new @[CertificatePair].
  //!
  //! @param key
  //!   Private key.
  //!
  //! @param certs
  //!   Chain of certificates, root cert last.
  //!
  //! @param extra_globs
  //!   The set of @[globs] from the first certificate
  //!   is optionally extended with these.
  //!
  //! @note
  //!   Performs various validation checks.
  protected void create(Crypto.Sign.State key, array(string(8bit)) certs,
			array(string(8bit))|void extra_name_globs)
  {
    if (!sizeof(certs)) {
      error("Empty list of certificates.\n");
    }

    array(Standards.X509.TBSCertificate|zero) tbss =
      map(certs, Standards.X509.decode_certificate);

    if (has_value(tbss, 0)) error("Invalid cert\n");

    // Validate that the key matches the cert.
    if (!key || !key->public_key_equal(tbss[0]->public_key->pkc)) {
      if(sizeof(tbss) > 1 && key &&
         key->public_key_equal(tbss[-1]->public_key->pkc)) {
        tbss = reverse(tbss);
        certs = reverse(certs);
      }
      else
        error("Private key doesn't match certificate.\n");
    }

    this::key = key;
    this::certs = certs;

    issuers = tbss->issuer->get_der();

    sign_algs = map(map(tbss->algorithm, `[], 0)->get_der(),
		    pkcs_der_to_sign_alg);

    if (has_value(sign_algs, 0)) error("Unknown signature algorithm.\n");

    set_globs(tbss[0], extra_name_globs);

    // FIXME: Ought to check certificate extensions.
    //        cf RFC 5246 7.4.2.
    ke_mask = 0;
    ke_mask_invariant = 0;
    if ((sign_algs[0] & HASH_MASK) == HASH_intrinsic) {
      this::cert_type = ([
	SIGNATURE_ed25519_intrinsic: AUTH_ecdsa_sign,
	SIGNATURE_ed448_intrinsic: AUTH_ecdsa_sign,
      ])[sign_algs[0]];
      if (this::cert_type) {
	ke_mask |= 1<<KE_ecdhe_ecdsa;
	if ((sizeof(sign_algs) == 1) ||
	    ((sign_algs[1] & SIGNATURE_MASK) == SIGNATURE_ecdsa) ||
	    (< SIGNATURE_ed25519_intrinsic,
	       SIGNATURE_ed448_intrinsic >)[sign_algs[1]]) {
	  // RFC 4492 2.1: ECDH_ECDSA
	  // In ECDH_ECDSA, the server's certificate MUST contain
	  // an ECDH-capable public key and be signed with ECDSA.
	  ke_mask |= 1<<KE_ecdh_ecdsa;
	} else if ((sign_algs[1] & SIGNATURE_MASK) == SIGNATURE_rsa) {
	  // RFC 4492 2.3: ECDH_RSA
	  // This key exchange algorithm is the same as ECDH_ECDSA
	  // except that the server's certificate MUST be signed
	  // with RSA rather than ECDSA.
	  ke_mask |= 1<<KE_ecdh_rsa;
	}
	ke_mask_invariant = ke_mask | ((1<<KE_ecdh_ecdsa) | (1<<KE_ecdh_rsa));
      }
    } else {
      // FIXME: This probably needs to look at the leaf cert extensions!
      this::cert_type = ([
	SIGNATURE_rsa: AUTH_rsa_sign,
	SIGNATURE_dsa: AUTH_dss_sign,
	SIGNATURE_ecdsa: AUTH_ecdsa_sign,
      ])[sign_algs[0] & SIGNATURE_MASK];

      switch(sign_algs[0] & SIGNATURE_MASK) {
      case SIGNATURE_rsa:
	foreach(({ KE_rsa, KE_rsa_fips, KE_dhe_rsa, KE_ecdhe_rsa, KE_rsa_psk,
		   KE_rsa_export,
		}),
		KeyExchangeType ke) {
	  ke_mask |= 1<<ke;
	}
	ke_mask_invariant = ke_mask;
	break;
      case SIGNATURE_dsa:
	ke_mask |= 1<<KE_dhe_dss;
	if ((sizeof(sign_algs) == 1) ||
	    ((sign_algs[1] & SIGNATURE_MASK) == SIGNATURE_dsa)) {
	  // RFC 4346 7.4.2: DH_DSS
	  // Diffie-Hellman key. The algorithm used
	  // to sign the certificate MUST be DSS.
	  ke_mask |= 1<<KE_dh_dss;
	} else if ((sign_algs[1] & SIGNATURE_MASK) == SIGNATURE_rsa) {
	  // RFC 4346 7.4.2: DH_RSA
	  // Diffie-Hellman key. The algorithm used
	  // to sign the certificate MUST be RSA.
	  ke_mask |= 1<<KE_dh_rsa;
	}
	ke_mask_invariant = ke_mask | ((1<<KE_dh_dss) | (1<<KE_dh_rsa));
	break;
      case SIGNATURE_ecdsa:
	ke_mask |= 1<<KE_ecdhe_ecdsa;
	if ((sizeof(sign_algs) == 1) ||
	    ((sign_algs[1] & SIGNATURE_MASK) == SIGNATURE_ecdsa)) {
	  // RFC 4492 2.1: ECDH_ECDSA
	  // In ECDH_ECDSA, the server's certificate MUST contain
	  // an ECDH-capable public key and be signed with ECDSA.
	  ke_mask |= 1<<KE_ecdh_ecdsa;
	} else if ((sign_algs[1] & SIGNATURE_MASK) == SIGNATURE_rsa) {
	  // RFC 4492 2.3: ECDH_RSA
	  // This key exchange algorithm is the same as ECDH_ECDSA
	  // except that the server's certificate MUST be signed
	  // with RSA rather than ECDSA.
	  ke_mask |= 1<<KE_ecdh_rsa;
	}
	ke_mask_invariant = ke_mask | ((1<<KE_ecdh_ecdsa) | (1<<KE_ecdh_rsa));
	break;
      }
    }

    if (!ke_mask) error("Certificate not useful for TLS!\n"
			"sign_algs: %O\n", sign_algs);
  }

  protected string _sprintf(int c)
  {
    string k = sprintf("%O", key);
    sscanf(k, "Crypto.%s", k);
    string h = fmt_constant(sign_algs[0] & HASH_MASK, "HASH");
    sscanf(h, "HASH_%s", h);
    return sprintf("CertificatePair(%s, %s, ({%{%O, %}}))", k, h, globs);
  }
}
