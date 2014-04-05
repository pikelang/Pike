#pike __REAL_VERSION__

/*
 * The SSL3 Protocol is specified in the following RFCs and drafts:
 *
 *   SSL 3.0			draft-freier-ssl-version3-02.txt
 *   SSL 3.0			RFC 6101
 *
 *   SSL 3.1/TLS 1.0		RFC 2246
 *   Kerberos for TLS 1.0	RFC 2712
 *   56-bit Export Cipher	draft-ietf-tls-56-bit-ciphersuites-01.txt
 *   AES Ciphers for TLS 1.0	RFC 3268
 *   Extensions for TLS 1.0	RFC 3546
 *   LZS Compression for TLS	RFC 3943
 *   Camellia Cipher for TLS	RFC 4132
 *   SEED Cipher for TLS 1.0	RFC 4162
 *   Pre-Shared Keys for TLS	RFC 4279
 *
 *   SSL 3.2/TLS 1.1		RFC 4346
 *   Extensions for TLS 1.1	RFC 4366
 *   ECC Ciphers for TLS 1.1	RFC 4492
 *   Session Resumption		RFC 4507
 *   TLS Handshake Message	RFC 4680
 *   User Mapping Extension	RFC 4681
 *   PSK with NULL for TLS 1.1	RFC 4785
 *   SRP with TLS 1.1		RFC 5054
 *   Session Resumption		RFC 5077
 *   OpenPGP Authentication	RFC 5081
 *   Authenticated Encryption	RFC 5116
 *
 *   DTLS over DCCP		RFC 5238
 *
 *   SSL 3.3/TLS 1.2		RFC 5246
 *   AES GCM Cipher for TLS	RFC 5288
 *   ECC with SHA256/384 & GCM	RFC 5289
 *   Suite B Profile for TLS	RFC 5430
 *   DES and IDEA for TLS	RFC 5469
 *   Pre-Shared Keys with GCM	RFC 5487
 *   ECDHA_PSK Cipher for TLS	RFC 5489
 *   Renegotiation Extension	RFC 5746
 *   Authorization Extensions	RFC 5878
 *   Camellia Cipher for TLS	RFC 5932
 *   KeyNote Auth for TLS	RFC 6042
 *   TLS Extension Defintions	RFC 6066
 *   OpenPGP Authentication	RFC 6091
 *   ARIA Cipher for TLS	RFC 6209
 *   Additional Master Secrets	RFC 6358
 *   Camellia Cipher for TLS	RFC 6367
 *   Suite B Profile for TLS	RFC 6460
 *   Heartbeat Extension	RFC 6520
 *   AES-CCM Cipher for TLS	RFC 6655
 *   Multiple Certificates	RFC 6961
 *   Certificate Transparency	RFC 6962
 *   ECC Brainpool Curves	RFC 7027
 *
 *   Next Protocol Negotiation  Google technical note: nextprotoneg
 *   Application Layer Protocol Negotiation  draft-ietf-tls-applayerprotoneg
 *
 * The SSL 2.0 protocol was specified in the following document:
 *
 *   SSL 2.0			draft-hickman-netscape-ssl-00.txt
 *
 * The TLS parameters registry:
 *   http://www.iana.org/assignments/tls-parameters/tls-parameters.xhtml
 *
 */

//! Protocol constants

//! Constants for specifying the versions of SSL to use.
//!
//! @seealso
//!   @[SSL.sslfile()->create()], @[SSL.handshake()->create()]
enum ProtocolVersion {
  PROTOCOL_SSL_3_0	= 0x300, //! SSL 3.0 - The original SSL3 draft version.
  PROTOCOL_SSL_3_1	= 0x301, //! SSL 3.1 - The RFC 2246 version of SSL.
  PROTOCOL_TLS_1_0	= 0x301, //! TLS 1.0 - The RFC 2246 version of TLS.
  PROTOCOL_SSL_3_2	= 0x302, //! SSL 3.2 - The RFC 4346 version of SSL.
  PROTOCOL_TLS_1_1	= 0x302, //! TLS 1.1 - The RFC 4346 version of TLS.
  PROTOCOL_SSL_3_3	= 0x303, //! SSL 3.3 - The RFC 5246 version of SSL.
  PROTOCOL_TLS_1_2	= 0x303, //! TLS 1.2 - The RFC 5246 version of TLS.
}

//! Max supported SSL version.
constant PROTOCOL_SSL_MAX = PROTOCOL_TLS_1_2;
constant PROTOCOL_TLS_MAX = PROTOCOL_TLS_1_2;

/* Packet types */
constant PACKET_change_cipher_spec = 20; // RFC 5246
constant PACKET_alert              = 21; // RFC 5246
constant PACKET_handshake          = 22; // RFC 5246
constant PACKET_application_data   = 23; // RFC 5246
constant PACKET_heartbeat          = 24; // RFC 6520
constant PACKET_types = (< PACKET_change_cipher_spec,
			   PACKET_alert,
			   PACKET_handshake,
			   PACKET_application_data >);
constant PACKET_V2 = -1; /* Backwards compatibility */

constant PACKET_MAX_SIZE = 0x4000;	// 2^14.

/* Cipher specification */
constant CIPHER_stream   = 0;
constant CIPHER_block    = 1;
constant CIPHER_aead     = 2;
constant CIPHER_types = (< CIPHER_stream, CIPHER_block, CIPHER_aead >);

constant CIPHER_null     = 0;
constant CIPHER_rc4_40   = 2;
constant CIPHER_rc2_40   = 3;
constant CIPHER_des40    = 6;
constant CIPHER_rc4      = 1;
constant CIPHER_des      = 4;
constant CIPHER_3des     = 5;
constant CIPHER_fortezza = 7;
constant CIPHER_idea	 = 8;
constant CIPHER_aes	 = 9;
constant CIPHER_aes256	 = 10;
constant CIPHER_camellia128 = 11;
constant CIPHER_camellia256 = 12;

//! Mapping from cipher algorithm to effective key length.
constant CIPHER_effective_keylengths = ([
  CIPHER_null:		0, 
  CIPHER_rc2_40:	16,	// A 64bit key in RC2 has strength ~34...
  CIPHER_rc4_40:	40,
  CIPHER_des40:		32,	// A 56bit key in DES has strength ~40...
  CIPHER_rc4:		128,
  CIPHER_des:		40,
  CIPHER_3des:		112,
  CIPHER_fortezza:	96,
  CIPHER_idea:		128,
  CIPHER_aes:		128,
  CIPHER_aes256:	256,
  CIPHER_camellia128:	128,
  CIPHER_camellia256:	256,
]);

//! Hash algorithms as per RFC 5246 7.4.1.4.1.
enum HashAlgorithm {
  HASH_none	= 0,
  HASH_md5	= 1,
  HASH_sha	= 2,
  HASH_sha224	= 3,
  HASH_sha256	= 4,
  HASH_sha384	= 5,
  HASH_sha512	= 6,
}

//! Cipher operation modes.
enum CipherModes {
  MODE_cbc	= 0,	//! CBC - Cipher Block Chaining mode.
  MODE_ccm_8	= 1,	//! CCM - Counter with 8 bit CBC-MAC mode.
  MODE_ccm	= 2,	//! CCM - Counter with CBC-MAC mode.
  MODE_gcm	= 3,	//! GCM - Galois Cipher Mode.
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
  HASH_sha:    Crypto.SHA1,
  HASH_md5:    Crypto.MD5,
]);

//! Signature algorithms from TLS 1.2.
enum SignatureAlgorithm {
  SIGNATURE_any         = -2,   //! All non-anonymous signatures (internal).
  SIGNATURE_invalid	= -1,	//! Signature not supported (internal).
  SIGNATURE_anonymous	= 0,	//! No signature.
  SIGNATURE_rsa		= 1,	//! RSASSA PKCS1 v1.5 signature.
  SIGNATURE_dsa		= 2,	//! DSS signature.
  SIGNATURE_ecdsa	= 3,	//! ECDSA signature.
}

//! Key exchange methods.
enum KeyExchangeType {
  KE_null	= 0,	//! None.
  KE_rsa	= 1,	//! Rivest-Shamir-Adelman
  KE_dh_dss	= 2,	//! Diffie-Hellman cert signed with DSS
  KE_dh_rsa	= 3,	//! Diffie-Hellman cert signed with RSA
  KE_dhe_dss	= 4,	//! Diffie-Hellman Ephemeral DSS
  KE_dhe_rsa	= 5,	//! Diffie-Hellman Ephemeral RSA
  KE_dh_anon	= 6,	//! Diffie-Hellman Anonymous
  KE_dms	= 7,
  KE_fortezza	= 8,
  // The following five are from RFC 4492.
  KE_ecdh_ecdsa = 9,	//! Elliptic Curve DH cert signed with ECDSA
  KE_ecdhe_ecdsa= 10,	//! Elliptic Curve DH Ephemeral with ECDSA
  KE_ecdh_rsa   = 11,	//! Elliptic Curve DH cert signed with RSA
  KE_ecdhe_rsa  = 12,	//! Elliptic Curve DH Ephemeral with RSA
  KE_ecdh_anon  = 13,	//! Elliptic Curve DH Anonymous
  // The following three are from RFC 4279.
  KE_psk	= 14,	//! Preshared Key
  KE_dhe_psk	= 15,	//! Preshared Key with DHE
  KE_rsa_psk	= 16,	//! Preshared Key signed with RSA
  // The following three are from RFC 5054.
  KE_srp_sha	= 17,	//! Secure Remote Password (SRP)
  KE_srp_sha_rsa= 18,	//! SRP signed with RSA
  KE_srp_sha_dss= 19,	//! SRP signed with DSS
}

//! Mapps from @[KeyExchangeType] to @[SignatureAlgorithm].
constant KE_TO_SA = ([
  KE_null:		SIGNATURE_anonymous,
  KE_rsa:		SIGNATURE_rsa,
  KE_dh_dss:		SIGNATURE_dsa,
  KE_dh_rsa:		SIGNATURE_dsa,
  KE_dhe_dss:		SIGNATURE_dsa,
  KE_dhe_rsa:		SIGNATURE_rsa,
  KE_dh_anon:		SIGNATURE_anonymous,
  KE_dms:		SIGNATURE_invalid,
  KE_fortezza:		SIGNATURE_invalid,
  KE_ecdh_ecdsa:	SIGNATURE_ecdsa,
  KE_ecdhe_ecdsa:	SIGNATURE_ecdsa,
  KE_ecdh_rsa:		SIGNATURE_ecdsa,
  KE_ecdhe_rsa:		SIGNATURE_rsa,
  KE_ecdh_anon:		SIGNATURE_anonymous,
]);

//! Compression methods.
enum CompressionType {
  COMPRESSION_null = 0,		//! No compression.
  COMPRESSION_deflate = 1,	//! Deflate compression. RFC 3749
  COMPRESSION_lzs = 64,		//! LZS compression. RFC 3943
}

/* Alert messages */
constant ALERT_warning			= 1;
constant ALERT_fatal			= 2;
constant ALERT_levels = (< ALERT_warning, ALERT_fatal >);

constant ALERT_close_notify			= 0;	// SSL 3.0
constant ALERT_unexpected_message		= 10;	// SSL 3.0
constant ALERT_bad_record_mac			= 20;	// SSL 3.0
constant ALERT_decryption_failed		= 21;	// TLS 1.0
constant ALERT_record_overflow			= 22;	// TLS 1.0
constant ALERT_decompression_failure		= 30;	// SSL 3.0
constant ALERT_handshake_failure		= 40;	// SSL 3.0
constant ALERT_no_certificate			= 41;	// SSL 3.0
constant ALERT_bad_certificate			= 42;	// SSL 3.0
constant ALERT_unsupported_certificate		= 43;	// SSL 3.0
constant ALERT_certificate_revoked		= 44;	// SSL 3.0
constant ALERT_certificate_expired		= 45;	// SSL 3.0
constant ALERT_certificate_unknown		= 46;	// SSL 3.0
constant ALERT_illegal_parameter		= 47;	// SSL 3.0
constant ALERT_unknown_ca			= 48;	// TLS 1.0
constant ALERT_access_denied			= 49;	// TLS 1.0
constant ALERT_decode_error			= 50;	// TLS 1.0
constant ALERT_decrypt_error			= 51;	// TLS 1.0
constant ALERT_export_restriction_RESERVED	= 60;	// TLS 1.0
constant ALERT_protocol_version			= 70;	// TLS 1.0
constant ALERT_insufficient_security		= 71;	// TLS 1.0
constant ALERT_internal_error			= 80;	// TLS 1.0
constant ALERT_user_canceled			= 90;	// TLS 1.0
constant ALERT_no_renegotiation			= 100;	// TLS 1.0
constant ALERT_unsupported_extension		= 110;	// RFC 3546
constant ALERT_certificate_unobtainable		= 111;	// RFC 3546
constant ALERT_unrecognized_name		= 112;	// RFC 3546
constant ALERT_bad_certificate_status_response	= 113;	// RFC 3546
constant ALERT_bad_certificate_hash_value	= 114;	// RFC 3546
constant ALERT_unknown_psk_identity		= 115;  // RFC 4279
constant ALERT_no_application_protocol          = 120;  // draft-ietf-tls-applayerprotoneg
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
  ALERT_export_restriction_RESERVED: "Export restrictions apply.",
  ALERT_protocol_version: "Unsupported protocol.",
  ALERT_insufficient_security: "Insufficient security.",
  ALERT_internal_error: "Internal error.",
  ALERT_user_canceled: "User canceled.",
  ALERT_no_renegotiation: "Renegotiation not allowed.",
  ALERT_unsupported_extension: "Unsolicitaded extension.",
  ALERT_certificate_unobtainable: "Failed to obtain certificate.",
  ALERT_unrecognized_name: "Unrecognized host name.",
  ALERT_bad_certificate_status_response: "Bad certificate status response.",
  ALERT_bad_certificate_hash_value: "Invalid certificate signature.",
  // ALERT_unknown_psk_identity
  ALERT_no_application_protocol : "No compatible application layer protocol.",
]);
 			      
constant CONNECTION_client 	= 0;
constant CONNECTION_server 	= 1;
constant CONNECTION_client_auth = 2;

/* Cipher suites */
constant SSL_null_with_null_null 		= 0x0000;	// SSL 3.0
constant SSL_rsa_with_null_md5			= 0x0001;	// SSL 3.0
constant SSL_rsa_with_null_sha			= 0x0002;	// SSL 3.0
constant SSL_rsa_export_with_rc4_40_md5		= 0x0003;	// SSL 3.0
constant SSL_rsa_export_with_rc2_cbc_40_md5	= 0x0006;	// SSL 3.0
constant SSL_rsa_export_with_des40_cbc_sha	= 0x0008;	// SSL 3.0
constant SSL_dh_dss_export_with_des40_cbc_sha	= 0x000b;	// SSL 3.0
constant SSL_dh_rsa_export_with_des40_cbc_sha	= 0x000e;	// SSL 3.0
constant SSL_dhe_dss_export_with_des40_cbc_sha	= 0x0011;	// SSL 3.0
constant SSL_dhe_rsa_export_with_des40_cbc_sha	= 0x0014;	// SSL 3.0
constant SSL_dh_anon_export_with_rc4_40_md5	= 0x0017;	// SSL 3.0
constant SSL_dh_anon_export_with_des40_cbc_sha	= 0x0019;	// SSL 3.0
constant TLS_krb5_with_des_cbc_40_sha           = 0x0026;	// RFC 2712
constant TLS_krb5_with_rc2_cbc_40_sha           = 0x0027;	// RFC 2712
constant TLS_krb5_with_rc4_40_sha               = 0x0028;	// RFC 2712
constant TLS_krb5_with_des_cbc_40_md5           = 0x0029;	// RFC 2712
constant TLS_krb5_with_rc2_cbc_40_md5           = 0x002a;	// RFC 2712
constant TLS_krb5_with_rc4_40_md5               = 0x002b;	// RFC 2712
constant TLS_psk_with_null_sha                  = 0x002c;	// RFC 4785
constant TLS_dhe_psk_with_null_sha              = 0x002d;	// RFC 4785
constant TLS_rsa_psk_with_null_sha              = 0x002e;	// RFC 4785
constant TLS_rsa_with_null_sha256               = 0x003b;	// TLS 1.2
constant SSL_rsa_with_rc4_128_md5		= 0x0004;	// SSL 3.0
constant SSL_rsa_with_rc4_128_sha		= 0x0005;	// SSL 3.0
constant SSL_rsa_with_idea_cbc_sha		= 0x0007;	// SSL 3.0
constant SSL_rsa_with_des_cbc_sha		= 0x0009;	// SSL 3.0
constant SSL_rsa_with_3des_ede_cbc_sha		= 0x000a;	// SSL 3.0
constant SSL_dh_dss_with_des_cbc_sha		= 0x000c;	// SSL 3.0
constant SSL_dh_dss_with_3des_ede_cbc_sha	= 0x000d;	// SSL 3.0
constant SSL_dh_rsa_with_des_cbc_sha		= 0x000f;	// SSL 3.0
constant SSL_dh_rsa_with_3des_ede_cbc_sha	= 0x0010;	// SSL 3.0
constant SSL_dhe_dss_with_des_cbc_sha		= 0x0012;	// SSL 3.0
constant SSL_dhe_dss_with_3des_ede_cbc_sha	= 0x0013;	// SSL 3.0
constant SSL_dhe_rsa_with_des_cbc_sha		= 0x0015;	// SSL 3.0
constant SSL_dhe_rsa_with_3des_ede_cbc_sha	= 0x0016;	// SSL 3.0
constant SSL_dh_anon_with_rc4_128_md5		= 0x0018;	// SSL 3.0
constant SSL_dh_anon_with_des_cbc_sha		= 0x001a;	// SSL 3.0
constant SSL_dh_anon_with_3des_ede_cbc_sha	= 0x001b;	// SSL 3.0

/* SSLv3/TLS conflict */
/* constant SSL_fortezza_dms_with_null_sha		= 0x001c; */
/* constant SSL_fortezza_dms_with_fortezza_cbc_sha	= 0x001d; */
/* constant SSL_fortezza_dms_with_rc4_128_sha	= 0x001e; */

constant TLS_krb5_with_des_cbc_sha              = 0x001e;	// RFC 2712
constant TLS_krb5_with_3des_ede_cbc_sha         = 0x001f;	// RFC 2712
constant TLS_krb5_with_rc4_128_sha              = 0x0020;	// RFC 2712
constant TLS_krb5_with_idea_cbc_sha             = 0x0021;	// RFC 2712
constant TLS_krb5_with_des_cbc_md5              = 0x0022;	// RFC 2712
constant TLS_krb5_with_3des_ede_cbc_md5         = 0x0023;	// RFC 2712
constant TLS_krb5_with_rc4_128_md5              = 0x0024;	// RFC 2712
constant TLS_krb5_with_idea_cbc_md5             = 0x0025;	// RFC 2712
constant TLS_rsa_with_aes_128_cbc_sha           = 0x002f;	// RFC 3268
constant TLS_dh_dss_with_aes_128_cbc_sha        = 0x0030;	// RFC 3268
constant TLS_dh_rsa_with_aes_128_cbc_sha        = 0x0031;	// RFC 3268
constant TLS_dhe_dss_with_aes_128_cbc_sha       = 0x0032;	// RFC 3268
constant TLS_dhe_rsa_with_aes_128_cbc_sha       = 0x0033;	// RFC 3268
constant TLS_dh_anon_with_aes_128_cbc_sha       = 0x0034;	// RFC 3268
constant TLS_rsa_with_aes_256_cbc_sha           = 0x0035;	// RFC 3268
constant TLS_dh_dss_with_aes_256_cbc_sha        = 0x0036;	// RFC 3268
constant TLS_dh_rsa_with_aes_256_cbc_sha        = 0x0037;	// RFC 3268
constant TLS_dhe_dss_with_aes_256_cbc_sha       = 0x0038;	// RFC 3268
constant TLS_dhe_rsa_with_aes_256_cbc_sha       = 0x0039;	// RFC 3268
constant TLS_dh_anon_with_aes_256_cbc_sha       = 0x003a;	// RFC 3268
constant TLS_rsa_with_aes_128_cbc_sha256        = 0x003c;	// TLS 1.2
constant TLS_rsa_with_aes_256_cbc_sha256        = 0x003d;	// TLS 1.2
constant TLS_dh_dss_with_aes_128_cbc_sha256     = 0x003e;	// TLS 1.2
constant TLS_dh_rsa_with_aes_128_cbc_sha256     = 0x003f;	// TLS 1.2
constant TLS_dhe_dss_with_aes_128_cbc_sha256    = 0x0040;	// TLS 1.2
constant TLS_rsa_with_camellia_128_cbc_sha      = 0x0041;	// RFC 4132
constant TLS_dh_dss_with_camellia_128_cbc_sha   = 0x0042;	// RFC 4132
constant TLS_dh_rsa_with_camellia_128_cbc_sha   = 0x0043;	// RFC 4132
constant TLS_dhe_dss_with_camellia_128_cbc_sha  = 0x0044;	// RFC 4132
constant TLS_dhe_rsa_with_camellia_128_cbc_sha  = 0x0045;	// RFC 4132
constant TLS_dh_anon_with_camellia_128_cbc_sha  = 0x0046;	// RFC 4132

// draft-ietf-tls-ecc-01.txt has ECDH_* suites in the range [0x0047, 0x005a].
// They were moved to 0xc001.. in RFC 4492.

// These suites from the 56-bit draft are apparently in use
// by some versions of MSIE.
constant TLS_rsa_export1024_with_des_cbc_sha	= 0x0062;	// 56bit draft
constant TLS_dhe_dss_export1024_with_des_cbc_sha= 0x0063;	// 56bit draft
constant TLS_rsa_export1024_with_rc4_56_sha	= 0x0064;	// 56bit draft
constant TLS_dhe_dss_export1024_with_rc4_56_sha	= 0x0065;	// 56bit draft
constant TLS_dhe_dss_with_rc4_128_sha		= 0x0066;	// 56bit draft

constant TLS_dhe_rsa_with_aes_128_cbc_sha256    = 0x0067;	// TLS 1.2
constant TLS_dh_dss_with_aes_256_cbc_sha256     = 0x0068;	// TLS 1.2
constant TLS_dh_rsa_with_aes_256_cbc_sha256     = 0x0069;	// TLS 1.2
constant TLS_dhe_dss_with_aes_256_cbc_sha256    = 0x006a;	// TLS 1.2
constant TLS_dhe_rsa_with_aes_256_cbc_sha256    = 0x006b;	// TLS 1.2
constant TLS_dh_anon_with_aes_128_cbc_sha256    = 0x006c;	// TLS 1.2
constant TLS_dh_anon_with_aes_256_cbc_sha256    = 0x006d;	// TLS 1.2

constant TLS_rsa_with_camellia_256_cbc_sha      = 0x0084;	// RFC 4132
constant TLS_dh_dss_with_camellia_256_cbc_sha   = 0x0085;	// RFC 4132
constant TLS_dh_rsa_with_camellia_256_cbc_sha   = 0x0086;	// RFC 4132
constant TLS_dhe_dss_with_camellia_256_cbc_sha  = 0x0087;	// RFC 4132
constant TLS_dhe_rsa_with_camellia_256_cbc_sha  = 0x0088;	// RFC 4132
constant TLS_dh_anon_with_camellia_256_cbc_sha  = 0x0089;	// RFC 4132
constant TLS_psk_with_rc4_128_sha               = 0x008a;	// RFC 4279
constant TLS_psk_with_3des_ede_cbc_sha          = 0x008b;	// RFC 4279
constant TLS_psk_with_aes_128_cbc_sha           = 0x008c;	// RFC 4279
constant TLS_psk_with_aes_256_cbc_sha           = 0x008d;	// RFC 4279
constant TLS_dhe_psk_with_rc4_128_sha           = 0x008e;	// RFC 4279
constant TLS_dhe_psk_with_3des_ede_cbc_sha      = 0x008f;	// RFC 4279
constant TLS_dhe_psk_with_aes_128_cbc_sha       = 0x0090;	// RFC 4279
constant TLS_dhe_psk_with_aes_256_cbc_sha       = 0x0091;	// RFC 4279
constant TLS_rsa_psk_with_rc4_128_sha           = 0x0092;	// RFC 4279
constant TLS_rsa_psk_with_3des_ede_cbc_sha      = 0x0093;	// RFC 4279
constant TLS_rsa_psk_with_aes_128_cbc_sha       = 0x0094;	// RFC 4279
constant TLS_rsa_psk_with_aes_256_cbc_sha       = 0x0095;	// RFC 4279
constant TLS_rsa_with_seed_cbc_sha              = 0x0096;	// RFC 4162
constant TLS_dh_dss_with_seed_cbc_sha           = 0x0097;	// RFC 4162
constant TLS_dh_rsa_with_seed_cbc_sha           = 0x0098;	// RFC 4162
constant TLS_dhe_dss_with_seed_cbc_sha          = 0x0099;	// RFC 4162
constant TLS_dhe_rsa_with_seed_cbc_sha          = 0x009a;	// RFC 4162
constant TLS_dh_anon_with_seed_cbc_sha          = 0x009b;	// RFC 4162
constant TLS_rsa_with_aes_128_gcm_sha256        = 0x009c;	// RFC 5288
constant TLS_rsa_with_aes_256_gcm_sha384        = 0x009d;	// RFC 5288
constant TLS_dhe_rsa_with_aes_128_gcm_sha256    = 0x009e;	// RFC 5288
constant TLS_dhe_rsa_with_aes_256_gcm_sha384    = 0x009f;	// RFC 5288
constant TLS_dh_rsa_with_aes_128_gcm_sha256     = 0x00a0;	// RFC 5288
constant TLS_dh_rsa_with_aes_256_gcm_sha384     = 0x00a1;	// RFC 5288
constant TLS_dhe_dss_with_aes_128_gcm_sha256    = 0x00a2;	// RFC 5288
constant TLS_dhe_dss_with_aes_256_gcm_sha384    = 0x00a3;	// RFC 5288
constant TLS_dh_dss_with_aes_128_gcm_sha256     = 0x00a4;	// RFC 5288
constant TLS_dh_dss_with_aes_256_gcm_sha384     = 0x00a5;	// RFC 5288
constant TLS_dh_anon_with_aes_128_gcm_sha256    = 0x00a6;	// RFC 5288
constant TLS_dh_anon_with_aes_256_gcm_sha384    = 0x00a7;	// RFC 5288
constant TLS_psk_with_aes_128_gcm_sha256        = 0x00a8;	// RFC 5487
constant TLS_psk_with_aes_256_gcm_sha384        = 0x00a9;	// RFC 5487
constant TLS_dhe_psk_with_aes_128_gcm_sha256    = 0x00aa;	// RFC 5487
constant TLS_dhe_psk_with_aes_256_gcm_sha384    = 0x00ab;	// RFC 5487
constant TLS_rsa_psk_with_aes_128_gcm_sha256    = 0x00ac;	// RFC 5487
constant TLS_rsa_psk_with_aes_256_gcm_sha384    = 0x00ad;	// RFC 5487
constant TLS_psk_with_aes_128_cbc_sha256        = 0x00ae;	// RFC 5487
constant TLS_psk_with_aes_256_cbc_sha384        = 0x00af;	// RFC 5487
constant TLS_psk_with_null_sha256               = 0x00b0;	// RFC 5487
constant TLS_psk_with_null_sha384               = 0x00b1;	// RFC 5487
constant TLS_dhe_psk_with_aes_128_cbc_sha256    = 0x00b2;	// RFC 5487
constant TLS_dhe_psk_with_aes_256_cbc_sha384    = 0x00b3;	// RFC 5487
constant TLS_dhe_psk_with_null_sha256           = 0x00b4;	// RFC 5487
constant TLS_dhe_psk_with_null_sha384           = 0x00b5;	// RFC 5487
constant TLS_rsa_psk_with_aes_128_cbc_sha256    = 0x00b6;	// RFC 5487
constant TLS_rsa_psk_with_aes_256_cbc_sha384    = 0x00b7;	// RFC 5487
constant TLS_rsa_psk_with_null_sha256           = 0x00b8;	// RFC 5487
constant TLS_rsa_psk_with_null_sha384           = 0x00b9;	// RFC 5487
constant TLS_rsa_with_camellia_128_cbc_sha256   = 0x00ba;	// RFC 5932
constant TLS_dh_dss_with_camellia_128_cbc_sha256= 0x00bb;	// RFC 5932
constant TLS_dh_rsa_with_camellia_128_cbc_sha256= 0x00bc;	// RFC 5932
constant TLS_dhe_dss_with_camellia_128_cbc_sha256= 0x00bd;	// RFC 5932
constant TLS_dhe_rsa_with_camellia_128_cbc_sha256= 0x00be;	// RFC 5932
constant TLS_dh_anon_with_camellia_128_cbc_sha256= 0x00bf;	// RFC 5932
constant TLS_rsa_with_camellia_256_cbc_sha256   = 0x00c0;	// RFC 5932
constant TLS_dh_dss_with_camellia_256_cbc_sha256= 0x00c1;	// RFC 5932
constant TLS_dh_rsa_with_camellia_256_cbc_sha256= 0x00c2;	// RFC 5932
constant TLS_dhe_dss_with_camellia_256_cbc_sha256= 0x00c3;	// RFC 5932
constant TLS_dhe_rsa_with_camellia_256_cbc_sha256= 0x00c4;	// RFC 5932
constant TLS_dh_anon_with_camellia_256_cbc_sha256= 0x00c5;	// RFC 5932

constant TLS_empty_renegotiation_info_scsv	= 0x00ff;	// RFC 5746

constant TLS_ecdh_ecdsa_with_null_sha           = 0xc001;	// RFC 4492
constant TLS_ecdh_ecdsa_with_rc4_128_sha        = 0xc002;	// RFC 4492
constant TLS_ecdh_ecdsa_with_3des_ede_cbc_sha   = 0xc003;	// RFC 4492
constant TLS_ecdh_ecdsa_with_aes_128_cbc_sha    = 0xc004;	// RFC 4492
constant TLS_ecdh_ecdsa_with_aes_256_cbc_sha    = 0xc005;	// RFC 4492
constant TLS_ecdhe_ecdsa_with_null_sha          = 0xc006;	// RFC 4492
constant TLS_ecdhe_ecdsa_with_rc4_128_sha       = 0xc007;	// RFC 4492
constant TLS_ecdhe_ecdsa_with_3des_ede_cbc_sha  = 0xc008;	// RFC 4492
constant TLS_ecdhe_ecdsa_with_aes_128_cbc_sha   = 0xc009;	// RFC 4492
constant TLS_ecdhe_ecdsa_with_aes_256_cbc_sha   = 0xc00a;	// RFC 4492
constant TLS_ecdh_rsa_with_null_sha             = 0xc00b;	// RFC 4492
constant TLS_ecdh_rsa_with_rc4_128_sha          = 0xc00c;	// RFC 4492
constant TLS_ecdh_rsa_with_3des_ede_cbc_sha     = 0xc00d;	// RFC 4492
constant TLS_ecdh_rsa_with_aes_128_cbc_sha      = 0xc00e;	// RFC 4492
constant TLS_ecdh_rsa_with_aes_256_cbc_sha      = 0xc00f;	// RFC 4492
constant TLS_ecdhe_rsa_with_null_sha            = 0xc010;	// RFC 4492
constant TLS_ecdhe_rsa_with_rc4_128_sha         = 0xc011;	// RFC 4492
constant TLS_ecdhe_rsa_with_3des_ede_cbc_sha    = 0xc012;	// RFC 4492
constant TLS_ecdhe_rsa_with_aes_128_cbc_sha     = 0xc013;	// RFC 4492
constant TLS_ecdhe_rsa_with_aes_256_cbc_sha     = 0xc014;	// RFC 4492
constant TLS_ecdh_anon_with_null_sha            = 0xc015;	// RFC 4492
constant TLS_ecdh_anon_with_rc4_128_sha         = 0xc016;	// RFC 4492
constant TLS_ecdh_anon_with_3des_ede_cbc_sha    = 0xc017;	// RFC 4492
constant TLS_ecdh_anon_with_aes_128_cbc_sha     = 0xc018;	// RFC 4492
constant TLS_ecdh_anon_with_aes_256_cbc_sha     = 0xc019;	// RFC 4492
constant TLS_srp_sha_with_3des_ede_cbc_sha      = 0xc01a;	// RFC 5054
constant TLS_srp_sha_rsa_with_3des_ede_cbc_sha  = 0xc01b;	// RFC 5054
constant TLS_srp_sha_dss_with_3des_ede_cbc_sha  = 0xc01c;	// RFC 5054
constant TLS_srp_sha_with_aes_128_cbc_sha       = 0xc01d;	// RFC 5054
constant TLS_srp_sha_rsa_with_aes_128_cbc_sha   = 0xc01e;	// RFC 5054
constant TLS_srp_sha_dss_with_aes_128_cbc_sha   = 0xc01f;	// RFC 5054
constant TLS_srp_sha_with_aes_256_cbc_sha       = 0xc020;	// RFC 5054
constant TLS_srp_sha_rsa_with_aes_256_cbc_sha   = 0xc021;	// RFC 5054
constant TLS_srp_sha_dss_with_aes_256_cbc_sha   = 0xc022;	// RFC 5054
constant TLS_ecdhe_ecdsa_with_aes_128_cbc_sha256= 0xc023;	// RFC 5289
constant TLS_ecdhe_ecdsa_with_aes_256_cbc_sha384= 0xc024;	// RFC 5289
constant TLS_ecdh_ecdsa_with_aes_128_cbc_sha256 = 0xc025;	// RFC 5289
constant TLS_ecdh_ecdsa_with_aes_256_cbc_sha384 = 0xc026;	// RFC 5289
constant TLS_ecdhe_rsa_with_aes_128_cbc_sha256  = 0xc027;	// RFC 5289
constant TLS_ecdhe_rsa_with_aes_256_cbc_sha384  = 0xc028;	// RFC 5289
constant TLS_ecdh_rsa_with_aes_128_cbc_sha256   = 0xc029;	// RFC 5289
constant TLS_ecdh_rsa_with_aes_256_cbc_sha384   = 0xc02a;	// RFC 5289
constant TLS_ecdhe_ecdsa_with_aes_128_gcm_sha256= 0xc02b;	// RFC 5289
constant TLS_ecdhe_ecdsa_with_aes_256_gcm_sha384= 0xc02c;	// RFC 5289
constant TLS_ecdh_ecdsa_with_aes_128_gcm_sha256 = 0xc02d;	// RFC 5289
constant TLS_ecdh_ecdsa_with_aes_256_gcm_sha384 = 0xc02e;	// RFC 5289
constant TLS_ecdhe_rsa_with_aes_128_gcm_sha256  = 0xc02f;	// RFC 5289
constant TLS_ecdhe_rsa_with_aes_256_gcm_sha384  = 0xc030;	// RFC 5289
constant TLS_ecdh_rsa_with_aes_128_gcm_sha256   = 0xc031;	// RFC 5289
constant TLS_ecdh_rsa_with_aes_256_gcm_sha384   = 0xc032;	// RFC 5289
constant TLS_ecdhe_psk_with_rc4_128_sha         = 0xc033;	// RFC 5489
constant TLS_ecdhe_psk_with_3des_ede_cbc_sha    = 0xc034;	// RFC 5489
constant TLS_ecdhe_psk_with_aes_128_cbc_sha     = 0xc035;	// RFC 5489
constant TLS_ecdhe_psk_with_aes_256_cbc_sha     = 0xc036;	// RFC 5489
constant TLS_ecdhe_psk_with_aes_128_cbc_sha256  = 0xc037;	// RFC 5489
constant TLS_ecdhe_psk_with_aes_256_cbc_sha384  = 0xc038;	// RFC 5489
constant TLS_ecdhe_psk_with_null_sha            = 0xc039;	// RFC 5489
constant TLS_ecdhe_psk_with_null_sha256         = 0xc03a;	// RFC 5489
constant TLS_ecdhe_psk_with_null_sha384         = 0xc03b;	// RFC 5489
constant TLS_rsa_with_aria_128_cbc_sha256       = 0xc03c;	// RFC 6209
constant TLS_rsa_with_aria_256_cbc_sha384       = 0xc03d;	// RFC 6209
constant TLS_dh_dss_with_aria_128_cbc_sha256    = 0xc03e;	// RFC 6209
constant TLS_dh_dss_with_aria_256_cbc_sha384    = 0xc03f;	// RFC 6209
constant TLS_dh_rsa_with_aria_128_cbc_sha256    = 0xc040;	// RFC 6209
constant TLS_dh_rsa_with_aria_256_cbc_sha384    = 0xc041;	// RFC 6209
constant TLS_dhe_dss_with_aria_128_cbc_sha256   = 0xc042;	// RFC 6209
constant TLS_dhe_dss_with_aria_256_cbc_sha384   = 0xc043;	// RFC 6209
constant TLS_dhe_rsa_with_aria_128_cbc_sha256   = 0xc044;	// RFC 6209
constant TLS_dhe_rsa_with_aria_256_cbc_sha384   = 0xc045;	// RFC 6209
constant TLS_dh_anon_with_aria_128_cbc_sha256   = 0xc046;	// RFC 6209
constant TLS_dh_anon_with_aria_256_cbc_sha384   = 0xc047;	// RFC 6209
constant TLS_ecdhe_ecdsa_with_aria_128_cbc_sha256= 0xc048;	// RFC 6209
constant TLS_ecdhe_ecdsa_with_aria_256_cbc_sha384= 0xc049;	// RFC 6209
constant TLS_ecdh_ecdsa_with_aria_128_cbc_sha256= 0xc04a;	// RFC 6209
constant TLS_ecdh_ecdsa_with_aria_256_cbc_sha384= 0xc04b;	// RFC 6209
constant TLS_ecdhe_rsa_with_aria_128_cbc_sha256 = 0xc04c;	// RFC 6209
constant TLS_ecdhe_rsa_with_aria_256_cbc_sha384 = 0xc04d;	// RFC 6209
constant TLS_ecdh_rsa_with_aria_128_cbc_sha256  = 0xc04e;	// RFC 6209
constant TLS_ecdh_rsa_with_aria_256_cbc_sha384  = 0xc04f;	// RFC 6209
constant TLS_rsa_with_aria_128_gcm_sha256       = 0xc050;	// RFC 6209
constant TLS_rsa_with_aria_256_gcm_sha384       = 0xc051;	// RFC 6209
constant TLS_dhe_rsa_with_aria_128_gcm_sha256   = 0xc052;	// RFC 6209
constant TLS_dhe_rsa_with_aria_256_gcm_sha384   = 0xc053;	// RFC 6209
constant TLS_dh_rsa_with_aria_128_gcm_sha256    = 0xc054;	// RFC 6209
constant TLS_dh_rsa_with_aria_256_gcm_sha384    = 0xc055;	// RFC 6209
constant TLS_dhe_dss_with_aria_128_gcm_sha256   = 0xc056;	// RFC 6209
constant TLS_dhe_dss_with_aria_256_gcm_sha384   = 0xc057;	// RFC 6209
constant TLS_dh_dss_with_aria_128_gcm_sha256    = 0xc058;	// RFC 6209
constant TLS_dh_dss_with_aria_256_gcm_sha384    = 0xc059;	// RFC 6209
constant TLS_dh_anon_with_aria_128_gcm_sha256   = 0xc05a;	// RFC 6209
constant TLS_dh_anon_with_aria_256_gcm_sha384   = 0xc05b;	// RFC 6209
constant TLS_ecdhe_ecdsa_with_aria_128_gcm_sha256= 0xc05c;	// RFC 6209
constant TLS_ecdhe_ecdsa_with_aria_256_gcm_sha384= 0xc05d;	// RFC 6209
constant TLS_ecdh_ecdsa_with_aria_128_gcm_sha256= 0xc05e;	// RFC 6209
constant TLS_ecdh_ecdsa_with_aria_256_gcm_sha384= 0xc05f;	// RFC 6209
constant TLS_ecdhe_rsa_with_aria_128_gcm_sha256 = 0xc060;	// RFC 6209
constant TLS_ecdhe_rsa_with_aria_256_gcm_sha384 = 0xc061;	// RFC 6209
constant TLS_ecdh_rsa_with_aria_128_gcm_sha256  = 0xc062;	// RFC 6209
constant TLS_ecdh_rsa_with_aria_256_gcm_sha384  = 0xc063;	// RFC 6209
constant TLS_psk_with_aria_128_cbc_sha256       = 0xc064;	// RFC 6209
constant TLS_psk_with_aria_256_cbc_sha384       = 0xc065;	// RFC 6209
constant TLS_dhe_psk_with_aria_128_cbc_sha256   = 0xc066;	// RFC 6209
constant TLS_dhe_psk_with_aria_256_cbc_sha384   = 0xc067;	// RFC 6209
constant TLS_rsa_psk_with_aria_128_cbc_sha256   = 0xc068;	// RFC 6209
constant TLS_rsa_psk_with_aria_256_cbc_sha384   = 0xc069;	// RFC 6209
constant TLS_psk_with_aria_128_gcm_sha256       = 0xc06a;	// RFC 6209
constant TLS_psk_with_aria_256_gcm_sha384       = 0xc06b;	// RFC 6209
constant TLS_dhe_psk_with_aria_128_gcm_sha256   = 0xc06c;	// RFC 6209
constant TLS_dhe_psk_with_aria_256_gcm_sha384   = 0xc06d;	// RFC 6209
constant TLS_rsa_psk_with_aria_128_gcm_sha256   = 0xc06e;	// RFC 6209
constant TLS_rsa_psk_with_aria_256_gcm_sha384   = 0xc06f;	// RFC 6209
constant TLS_ecdhe_psk_with_aria_128_cbc_sha256 = 0xc070;	// RFC 6209
constant TLS_ecdhe_psk_with_aria_256_cbc_sha384 = 0xc071;	// RFC 6209
constant TLS_ecdhe_ecdsa_with_camellia_128_cbc_sha256= 0xc072;	// RFC 6367
constant TLS_ecdhe_ecdsa_with_camellia_256_cbc_sha384= 0xc073;	// RFC 6367
constant TLS_ecdh_ecdsa_with_camellia_128_cbc_sha256 = 0xc074;	// RFC 6367
constant TLS_ecdh_ecdsa_with_camellia_256_cbc_sha384 = 0xc075;	// RFC 6367
constant TLS_ecdhe_rsa_with_camellia_128_cbc_sha256  = 0xc076;	// RFC 6367
constant TLS_ecdhe_rsa_with_camellia_256_cbc_sha384  = 0xc077;	// RFC 6367
constant TLS_ecdh_rsa_with_camellia_128_cbc_sha256   = 0xc078;	// RFC 6367
constant TLS_ecdh_rsa_with_camellia_256_cbc_sha384   = 0xc079;	// RFC 6367
constant TLS_rsa_with_camellia_128_gcm_sha256        = 0xc07a;	// RFC 6367
constant TLS_rsa_with_camellia_256_gcm_sha384        = 0xc07b;	// RFC 6367
constant TLS_dhe_rsa_with_camellia_128_gcm_sha256    = 0xc07c;	// RFC 6367
constant TLS_dhe_rsa_with_camellia_256_gcm_sha384    = 0xc07d;	// RFC 6367
constant TLS_dh_rsa_with_camellia_128_gcm_sha256     = 0xc07e;	// RFC 6367
constant TLS_dh_rsa_with_camellia_256_gcm_sha384     = 0xc07f;	// RFC 6367
constant TLS_dhe_dss_with_camellia_128_gcm_sha256    = 0xc080;	// RFC 6367
constant TLS_dhe_dss_with_camellia_256_gcm_sha384    = 0xc081;	// RFC 6367
constant TLS_dh_dss_with_camellia_128_gcm_sha256     = 0xc082;	// RFC 6367
constant TLS_dh_dss_with_camellia_256_gcm_sha384     = 0xc083;	// RFC 6367
constant TLS_dh_anon_with_camellia_128_gcm_sha256    = 0xc084;	// RFC 6367
constant TLS_dh_anon_with_camellia_256_gcm_sha384    = 0xc085;	// RFC 6367
constant TLS_ecdhe_ecdsa_with_camellia_128_gcm_sha256= 0xc086;	// RFC 6367
constant TLS_ecdhe_ecdsa_with_camellia_256_gcm_sha384= 0xc087;	// RFC 6367
constant TLS_ecdh_ecdsa_with_camellia_128_gcm_sha256 = 0xc088;	// RFC 6367
constant TLS_ecdh_ecdsa_with_camellia_256_gcm_sha384 = 0xc089;	// RFC 6367
constant TLS_ecdhe_rsa_with_camellia_128_gcm_sha256  = 0xc08a;	// RFC 6367
constant TLS_ecdhe_rsa_with_camellia_256_gcm_sha384  = 0xc08b;	// RFC 6367
constant TLS_ecdh_rsa_with_camellia_128_gcm_sha256   = 0xc08c;	// RFC 6367
constant TLS_ecdh_rsa_with_camellia_256_gcm_sha384   = 0xc08d;	// RFC 6367
constant TLS_psk_with_camellia_128_gcm_sha256        = 0xc08e;	// RFC 6367
constant TLS_psk_with_camellia_256_gcm_sha384        = 0xc08f;	// RFC 6367
constant TLS_dhe_psk_with_camellia_128_gcm_sha256    = 0xc090;	// RFC 6367
constant TLS_dhe_psk_with_camellia_256_gcm_sha384    = 0xc091;	// RFC 6367
constant TLS_rsa_psk_with_camellia_128_gcm_sha256    = 0xc092;	// RFC 6367
constant TLS_rsa_psk_with_camellia_256_gcm_sha384    = 0xc093;	// RFC 6367
constant TLS_psk_with_camellia_128_cbc_sha256        = 0xc094;	// RFC 6367
constant TLS_psk_with_camellia_256_cbc_sha384        = 0xc095;	// RFC 6367
constant TLS_dhe_psk_with_camellia_128_cbc_sha256    = 0xc096;	// RFC 6367
constant TLS_dhe_psk_with_camellia_256_cbc_sha384    = 0xc097;	// RFC 6367
constant TLS_rsa_psk_with_camellia_128_cbc_sha256    = 0xc098;	// RFC 6367
constant TLS_rsa_psk_with_camellia_256_cbc_sha384    = 0xc099;	// RFC 6367
constant TLS_ecdhe_psk_with_camellia_128_cbc_sha256  = 0xc09a;	// RFC 6367
constant TLS_ecdhe_psk_with_camellia_256_cbc_sha384  = 0xc09b;	// RFC 6367
constant TLS_rsa_with_aes_128_ccm		= 0xc09c;	// RFC 6655
constant TLS_rsa_with_aes_256_ccm		= 0xc09d;	// RFC 6655
constant TLS_dhe_rsa_with_aes_128_ccm		= 0xc09e;	// RFC 6655
constant TLS_dhe_rsa_with_aes_256_ccm		= 0xc09f;	// RFC 6655
constant TLS_rsa_with_aes_128_ccm_8		= 0xc0a0;	// RFC 6655
constant TLS_rsa_with_aes_256_ccm_8		= 0xc0a1;	// RFC 6655
constant TLS_dhe_rsa_with_aes_128_ccm_8		= 0xc0a2;	// RFC 6655
constant TLS_dhe_rsa_with_aes_256_ccm_8		= 0xc0a3;	// RFC 6655
constant TLS_psk_with_aes_128_ccm		= 0xc0a4;	// RFC 6655
constant TLS_psk_with_aes_256_ccm		= 0xc0a5;	// RFC 6655
constant TLS_dhe_psk_with_aes_128_ccm		= 0xc0a6;	// RFC 6655
constant TLS_dhe_psk_with_aes_256_ccm		= 0xc0a7;	// RFC 6655
constant TLS_psk_with_aes_128_ccm_8		= 0xc0a8;	// RFC 6655
constant TLS_psk_with_aes_256_ccm_8		= 0xc0a9;	// RFC 6655
constant TLS_psk_dhe_with_aes_128_ccm_8		= 0xc0aa;	// RFC 6655
constant TLS_psk_dhe_with_aes_256_ccm_8		= 0xc0ab;	// RFC 6655

// Constants from SSL 2.0.
// These may appear in HANDSHAKE_hello_v2 and
// are here for informational purposes.
constant SSL2_ck_rc4_128_with_md5		= 0x010080;
constant SSL2_ck_rc4_128_export40_with_md5	= 0x020080;
constant SSL2_ck_rc2_128_cbc_with_md5		= 0x030080;
constant SSL2_ck_rc2_128_cbc_export40_with_md5	= 0x040080;
constant SSL2_ck_idea_128_cbc_with_md5		= 0x050080;
constant SSL2_ck_des_64_cbc_with_md5		= 0x060040;
constant SSL2_ck_des_192_ede3_cbc_with_md5	= 0x0700c0;

string fmt_constant(string prefix, int c)
{
  if (!has_suffix(prefix, "_")) prefix += "_";
  foreach([array(string)]indices(this), string id)
    if (has_prefix(id, prefix) && (this[id] == c)) return id;
  return sprintf("%sunknown(%d)", prefix, c);
}

protected mapping(int:string) suite_to_symbol = ([]);

string fmt_cipher_suite(int suite)
{
  if (!sizeof(suite_to_symbol)) {
    foreach([array(string)]indices(this), string id)
      if( has_prefix(id, "SSL_") || has_prefix(id, "TLS_") ||
	  has_prefix(id, "SSL2_") ) {
	suite_to_symbol[this[id]] = id;
      }
  }
  string res = suite_to_symbol[suite];
  if (res) return res;
  return suite_to_symbol[suite] = sprintf("unknown(%d)", suite);
}

string fmt_cipher_suites(array(int) s)
{
  String.Buffer b = String.Buffer();
  foreach(s, int c)
    b->sprintf("   %-6d: %s\n", c, fmt_cipher_suite(c));
  return (string)b;
}

string fmt_signature_pairs(array(array(int)) pairs)
{
  String.Buffer b = String.Buffer();
  foreach(pairs, [int hash, int signature])
    b->sprintf("  <%s, %s>\n", fmt_constant("HASH",hash), fmt_constant("SIGNATURE",signature));
  return (string)b;
}

string fmt_version(ProtocolVersion version)
{
  if (version <= PROTOCOL_SSL_3_0) {
    return sprintf("SSL %d.%d", version>>8, version & 0xff);
  }
  version -= PROTOCOL_TLS_1_0 - 0x100;
  return sprintf("TLS %d.%d", version>>8, version & 0xff);
}

/* FIXME: Add SIGN-type element to table */
constant CIPHER_SUITES =
([
   // The following cipher suites are only intended for testing.
   SSL_null_with_null_null :    	({ 0, 0, 0 }),
   SSL_rsa_with_null_md5 :      	({ KE_rsa, 0, HASH_md5 }), 
   SSL_rsa_with_null_sha :      	({ KE_rsa, 0, HASH_sha }),
   TLS_rsa_with_null_sha256 :      	({ KE_rsa, 0, HASH_sha256 }),

   // NB: The export suites are obsolete in TLS 1.1 and later.
   //     The RC4/40 suite is required for Netscape 4.05 Intl.
#if constant(Crypto.Arctwo)
   SSL_rsa_export_with_rc2_cbc_40_md5 :	({ KE_rsa, CIPHER_rc2_40, HASH_md5 }),
#endif
   SSL_rsa_export_with_rc4_40_md5 :	({ KE_rsa, CIPHER_rc4_40, HASH_md5 }),
   SSL_dhe_dss_export_with_des40_cbc_sha :
      ({ KE_dhe_dss, CIPHER_des40, HASH_sha }),
   SSL_dhe_rsa_export_with_des40_cbc_sha :
      ({ KE_dhe_rsa, CIPHER_des40, HASH_sha }),
   SSL_dh_dss_export_with_des40_cbc_sha :
      ({ KE_dh_dss, CIPHER_des40, HASH_sha }),
   SSL_dh_rsa_export_with_des40_cbc_sha :
      ({ KE_dh_rsa, CIPHER_des40, HASH_sha }),
   SSL_rsa_export_with_des40_cbc_sha :  ({ KE_rsa, CIPHER_des40, HASH_sha }),

   // NB: The IDEA and DES suites are obsolete in TLS 1.2 and later.
#if constant(Crypto.IDEA)
   SSL_rsa_with_idea_cbc_sha :		({ KE_rsa, CIPHER_idea, HASH_sha }),
#endif
   SSL_rsa_with_des_cbc_sha :		({ KE_rsa, CIPHER_des, HASH_sha }),
   SSL_dhe_dss_with_des_cbc_sha :	({ KE_dhe_dss, CIPHER_des, HASH_sha }),
   SSL_dhe_rsa_with_des_cbc_sha :	({ KE_dhe_rsa, CIPHER_des, HASH_sha }),
   SSL_dh_dss_with_des_cbc_sha :	({ KE_dh_dss, CIPHER_des, HASH_sha }),
   SSL_dh_rsa_with_des_cbc_sha :	({ KE_dh_rsa, CIPHER_des, HASH_sha }),

   SSL_rsa_with_rc4_128_sha :		({ KE_rsa, CIPHER_rc4, HASH_sha }),
   SSL_rsa_with_rc4_128_md5 :		({ KE_rsa, CIPHER_rc4, HASH_md5 }),
   TLS_dhe_dss_with_rc4_128_sha :	({ KE_dhe_dss, CIPHER_rc4, HASH_sha }),

   // Some anonymous diffie-hellman variants.
   SSL_dh_anon_export_with_rc4_40_md5:	({ KE_dh_anon, CIPHER_rc4_40, HASH_md5 }),
   SSL_dh_anon_export_with_des40_cbc_sha: ({ KE_dh_anon, CIPHER_des40, HASH_sha }),
   SSL_dh_anon_with_rc4_128_md5:	({ KE_dh_anon, CIPHER_rc4, HASH_md5 }),
   SSL_dh_anon_with_des_cbc_sha:	({ KE_dh_anon, CIPHER_des, HASH_sha }),
   SSL_dh_anon_with_3des_ede_cbc_sha:	({ KE_dh_anon, CIPHER_3des, HASH_sha }),
   TLS_dh_anon_with_aes_128_cbc_sha:	({ KE_dh_anon, CIPHER_aes, HASH_sha }),
   TLS_dh_anon_with_aes_256_cbc_sha:	({ KE_dh_anon, CIPHER_aes256, HASH_sha }),
   TLS_dh_anon_with_aes_128_cbc_sha256: ({ KE_dh_anon, CIPHER_aes, HASH_sha256 }),
   TLS_dh_anon_with_aes_256_cbc_sha256: ({ KE_dh_anon, CIPHER_aes256, HASH_sha256 }),
   TLS_ecdh_anon_with_null_sha:		({ KE_ecdh_anon, 0, HASH_sha }),
   TLS_ecdh_anon_with_rc4_128_sha:	({ KE_ecdh_anon, CIPHER_rc4, HASH_sha }),
   TLS_ecdh_anon_with_3des_ede_cbc_sha:	({ KE_ecdh_anon, CIPHER_3des, HASH_sha }),
   TLS_ecdh_anon_with_aes_128_cbc_sha:	({ KE_ecdh_anon, CIPHER_aes, HASH_sha }),
   TLS_ecdh_anon_with_aes_256_cbc_sha:	({ KE_ecdh_anon, CIPHER_aes256, HASH_sha }),

   // Required by TLS 1.0 RFC 2246 9.
   SSL_dhe_dss_with_3des_ede_cbc_sha :	({ KE_dhe_dss, CIPHER_3des, HASH_sha }),

   // Required by TLS 1.1 RFC 4346 9.
   SSL_rsa_with_3des_ede_cbc_sha :	({ KE_rsa, CIPHER_3des, HASH_sha }),

   // Required by TLS 1.2 RFC 5246 9.
   TLS_rsa_with_aes_128_cbc_sha :	({ KE_rsa, CIPHER_aes, HASH_sha }),

   SSL_dhe_rsa_with_3des_ede_cbc_sha :	({ KE_dhe_rsa, CIPHER_3des, HASH_sha }),
   SSL_dh_dss_with_3des_ede_cbc_sha :	({ KE_dh_dss, CIPHER_3des, HASH_sha }),
   SSL_dh_rsa_with_3des_ede_cbc_sha :	({ KE_dh_rsa, CIPHER_3des, HASH_sha }),

   TLS_dhe_dss_with_aes_128_cbc_sha :	({ KE_dhe_dss, CIPHER_aes, HASH_sha }),
   TLS_dhe_rsa_with_aes_128_cbc_sha :	({ KE_dhe_rsa, CIPHER_aes, HASH_sha }),
   TLS_dh_dss_with_aes_128_cbc_sha :	({ KE_dh_dss, CIPHER_aes, HASH_sha }),
   TLS_dh_rsa_with_aes_128_cbc_sha :	({ KE_dh_rsa, CIPHER_aes, HASH_sha }),
   TLS_rsa_with_aes_256_cbc_sha :	({ KE_rsa, CIPHER_aes256, HASH_sha }),
   TLS_dhe_dss_with_aes_256_cbc_sha :	({ KE_dhe_dss, CIPHER_aes256, HASH_sha }),
   TLS_dhe_rsa_with_aes_256_cbc_sha :	({ KE_dhe_rsa, CIPHER_aes256, HASH_sha }),
   TLS_dh_dss_with_aes_256_cbc_sha :	({ KE_dh_dss, CIPHER_aes256, HASH_sha }),
   TLS_dh_rsa_with_aes_256_cbc_sha :	({ KE_dh_rsa, CIPHER_aes256, HASH_sha }),

   // Suites from RFC 4492 (TLSECC)
   TLS_ecdh_ecdsa_with_null_sha : ({ KE_ecdh_ecdsa, 0, HASH_sha }),
   TLS_ecdh_ecdsa_with_rc4_128_sha : ({ KE_ecdh_ecdsa, CIPHER_rc4, HASH_sha }),
   TLS_ecdh_ecdsa_with_3des_ede_cbc_sha : ({ KE_ecdh_ecdsa, CIPHER_3des, HASH_sha }),
   TLS_ecdh_ecdsa_with_aes_128_cbc_sha : ({ KE_ecdh_ecdsa, CIPHER_aes, HASH_sha }),
   TLS_ecdh_ecdsa_with_aes_256_cbc_sha : ({ KE_ecdh_ecdsa, CIPHER_aes256, HASH_sha }),

   TLS_ecdhe_ecdsa_with_null_sha :	({ KE_ecdhe_ecdsa, 0, HASH_sha }),
   TLS_ecdhe_ecdsa_with_rc4_128_sha :	({ KE_ecdhe_ecdsa, CIPHER_rc4, HASH_sha }),
   TLS_ecdhe_ecdsa_with_3des_ede_cbc_sha : ({ KE_ecdhe_ecdsa, CIPHER_3des, HASH_sha }),
   TLS_ecdhe_ecdsa_with_aes_128_cbc_sha : ({ KE_ecdhe_ecdsa, CIPHER_aes, HASH_sha }),
   TLS_ecdhe_ecdsa_with_aes_256_cbc_sha : ({ KE_ecdhe_ecdsa, CIPHER_aes256, HASH_sha }),

   TLS_ecdh_rsa_with_null_sha : ({ KE_ecdh_rsa, 0, HASH_sha }),
   TLS_ecdh_rsa_with_rc4_128_sha : ({ KE_ecdh_rsa, CIPHER_rc4, HASH_sha }),
   TLS_ecdh_rsa_with_3des_ede_cbc_sha : ({ KE_ecdh_rsa, CIPHER_3des, HASH_sha }),
   TLS_ecdh_rsa_with_aes_128_cbc_sha : ({ KE_ecdh_rsa, CIPHER_aes, HASH_sha }),
   TLS_ecdh_rsa_with_aes_256_cbc_sha : ({ KE_ecdh_rsa, CIPHER_aes256, HASH_sha }),

   TLS_ecdhe_rsa_with_null_sha :	({ KE_ecdhe_rsa, 0, HASH_sha }),
   TLS_ecdhe_rsa_with_rc4_128_sha :	({ KE_ecdhe_rsa, CIPHER_rc4, HASH_sha }),
   TLS_ecdhe_rsa_with_3des_ede_cbc_sha : ({ KE_ecdhe_rsa, CIPHER_3des, HASH_sha }),
   TLS_ecdhe_rsa_with_aes_128_cbc_sha :	({ KE_ecdhe_rsa, CIPHER_aes, HASH_sha }),
   TLS_ecdhe_rsa_with_aes_256_cbc_sha :	({ KE_ecdhe_rsa, CIPHER_aes256, HASH_sha }),


   // Suites from RFC 5246 (TLS 1.2)
   TLS_rsa_with_aes_128_cbc_sha256 :    ({ KE_rsa, CIPHER_aes, HASH_sha256 }),
   TLS_dhe_rsa_with_aes_128_cbc_sha256 : ({ KE_dhe_rsa, CIPHER_aes, HASH_sha256 }),
   TLS_dhe_dss_with_aes_128_cbc_sha256 : ({ KE_dhe_dss, CIPHER_aes, HASH_sha256 }),
   TLS_dh_rsa_with_aes_128_cbc_sha256 : ({ KE_dh_rsa, CIPHER_aes, HASH_sha256 }),
   TLS_dh_dss_with_aes_128_cbc_sha256 : ({ KE_dh_dss, CIPHER_aes, HASH_sha256 }),
   TLS_rsa_with_aes_256_cbc_sha256 :	({ KE_rsa, CIPHER_aes256, HASH_sha256 }),
   TLS_dhe_rsa_with_aes_256_cbc_sha256 : ({ KE_dhe_rsa, CIPHER_aes256, HASH_sha256 }),
   TLS_dhe_dss_with_aes_256_cbc_sha256 : ({ KE_dhe_dss, CIPHER_aes256, HASH_sha256 }),
   TLS_dh_rsa_with_aes_256_cbc_sha256 : ({ KE_dh_rsa, CIPHER_aes256, HASH_sha256 }),
   TLS_dh_dss_with_aes_256_cbc_sha256 : ({ KE_dh_dss, CIPHER_aes256, HASH_sha256 }),

   // Suites from RFC 5289
   // Note that these are not valid for TLS versions prior to TLS 1.2.
   TLS_ecdhe_ecdsa_with_aes_128_cbc_sha256 : ({ KE_ecdhe_ecdsa, CIPHER_aes, HASH_sha256, MODE_cbc }),
   TLS_ecdhe_ecdsa_with_aes_256_cbc_sha384 : ({ KE_ecdhe_ecdsa, CIPHER_aes256, HASH_sha384, MODE_cbc }),
   TLS_ecdh_ecdsa_with_aes_128_cbc_sha256 : ({ KE_ecdh_ecdsa, CIPHER_aes, HASH_sha256, MODE_cbc }),
   TLS_ecdh_ecdsa_with_aes_256_cbc_sha384 : ({ KE_ecdh_ecdsa, CIPHER_aes256, HASH_sha384, MODE_cbc }),
   TLS_ecdhe_rsa_with_aes_128_cbc_sha256 : ({ KE_ecdhe_rsa, CIPHER_aes, HASH_sha256, MODE_cbc }),
   TLS_ecdhe_rsa_with_aes_256_cbc_sha384 : ({ KE_ecdhe_rsa, CIPHER_aes256, HASH_sha384, MODE_cbc }),
   TLS_ecdh_rsa_with_aes_128_cbc_sha256 : ({ KE_ecdh_rsa, CIPHER_aes, HASH_sha256, MODE_cbc }),
   TLS_ecdh_rsa_with_aes_256_cbc_sha384 : ({ KE_ecdh_rsa, CIPHER_aes256, HASH_sha384, MODE_cbc }),

   // Suites from RFC 6655
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
   TLS_rsa_with_camellia_128_cbc_sha:	({ KE_rsa, CIPHER_camellia128, HASH_sha }),
   TLS_dhe_dss_with_camellia_128_cbc_sha: ({ KE_dhe_dss, CIPHER_camellia128, HASH_sha }),
   TLS_dhe_rsa_with_camellia_128_cbc_sha: ({ KE_dhe_rsa, CIPHER_camellia128, HASH_sha }),
   TLS_dh_dss_with_camellia_128_cbc_sha: ({ KE_dh_dss, CIPHER_camellia128, HASH_sha }),
   TLS_dh_rsa_with_camellia_128_cbc_sha: ({ KE_dh_rsa, CIPHER_camellia128, HASH_sha }),
   TLS_rsa_with_camellia_256_cbc_sha:	({ KE_rsa, CIPHER_camellia256, HASH_sha }),
   TLS_dhe_dss_with_camellia_256_cbc_sha: ({ KE_dhe_dss, CIPHER_camellia256, HASH_sha }),
   TLS_dhe_rsa_with_camellia_256_cbc_sha: ({ KE_dhe_rsa, CIPHER_camellia256, HASH_sha }),
   TLS_dh_dss_with_camellia_256_cbc_sha: ({ KE_dh_dss, CIPHER_camellia256, HASH_sha }),
   TLS_dh_rsa_with_camellia_256_cbc_sha: ({ KE_dh_rsa, CIPHER_camellia256, HASH_sha }),

   TLS_rsa_with_camellia_128_cbc_sha256:	({ KE_rsa, CIPHER_camellia128, HASH_sha256 }),
   TLS_dhe_dss_with_camellia_128_cbc_sha256: ({ KE_dhe_dss, CIPHER_camellia128, HASH_sha256 }),
   TLS_dhe_rsa_with_camellia_128_cbc_sha256: ({ KE_dhe_rsa, CIPHER_camellia128, HASH_sha256 }),
   TLS_dh_dss_with_camellia_128_cbc_sha256: ({ KE_dh_dss, CIPHER_camellia128, HASH_sha256 }),
   TLS_dh_rsa_with_camellia_128_cbc_sha256: ({ KE_dh_rsa, CIPHER_camellia128, HASH_sha256 }),
   TLS_rsa_with_camellia_256_cbc_sha256:	({ KE_rsa, CIPHER_camellia256, HASH_sha256 }),
   TLS_dhe_dss_with_camellia_256_cbc_sha256: ({ KE_dhe_dss, CIPHER_camellia256, HASH_sha256 }),
   TLS_dhe_rsa_with_camellia_256_cbc_sha256: ({ KE_dhe_rsa, CIPHER_camellia256, HASH_sha256 }),
   TLS_dh_dss_with_camellia_256_cbc_sha256: ({ KE_dh_dss, CIPHER_camellia256, HASH_sha256 }),
   TLS_dh_rsa_with_camellia_256_cbc_sha256: ({ KE_dh_rsa, CIPHER_camellia256, HASH_sha256 }),

   // Anonymous variants:
   TLS_dh_anon_with_camellia_128_cbc_sha: ({ KE_dh_anon, CIPHER_camellia128, HASH_sha }),
   TLS_dh_anon_with_camellia_256_cbc_sha: ({ KE_dh_anon, CIPHER_camellia256, HASH_sha }),
   TLS_dh_anon_with_camellia_128_cbc_sha256: ({ KE_dh_anon, CIPHER_camellia128, HASH_sha256 }),
   TLS_dh_anon_with_camellia_256_cbc_sha256: ({ KE_dh_anon, CIPHER_camellia256, HASH_sha256 }),

   // From RFC 6367
   // Note that this RFC explicitly allows use of these suites
   // with TLS versions prior to TLS 1.2 (RFC 6367 3.3).
   TLS_ecdh_ecdsa_with_camellia_128_cbc_sha256: ({ KE_ecdh_ecdsa, CIPHER_camellia128, HASH_sha256 }),
   TLS_ecdh_ecdsa_with_camellia_256_cbc_sha384: ({ KE_ecdh_ecdsa, CIPHER_camellia256, HASH_sha384 }),
   TLS_ecdh_rsa_with_camellia_128_cbc_sha256: ({ KE_ecdh_rsa, CIPHER_camellia128, HASH_sha256 }),
   TLS_ecdh_rsa_with_camellia_256_cbc_sha384: ({ KE_ecdh_rsa, CIPHER_camellia256, HASH_sha384 }),
   TLS_ecdhe_ecdsa_with_camellia_128_cbc_sha256: ({ KE_ecdhe_ecdsa, CIPHER_camellia128, HASH_sha256 }),
   TLS_ecdhe_ecdsa_with_camellia_256_cbc_sha384: ({ KE_ecdhe_ecdsa, CIPHER_camellia256, HASH_sha384 }),
   TLS_ecdhe_rsa_with_camellia_128_cbc_sha256: ({ KE_ecdhe_rsa, CIPHER_camellia128, HASH_sha256 }),
   TLS_ecdhe_rsa_with_camellia_256_cbc_sha384: ({ KE_ecdhe_rsa, CIPHER_camellia256, HASH_sha384 }),
#endif /* Crypto.Camellia */

#if constant(Crypto.GCM)
   // GCM Suites:
   TLS_rsa_with_aes_128_gcm_sha256:	({ KE_rsa, CIPHER_aes, HASH_sha256, MODE_gcm }),
   TLS_dhe_rsa_with_aes_128_gcm_sha256:	({ KE_dhe_rsa, CIPHER_aes, HASH_sha256, MODE_gcm }),
   TLS_dhe_dss_with_aes_128_gcm_sha256:	({ KE_dhe_dss, CIPHER_aes, HASH_sha256, MODE_gcm }),
   TLS_dh_rsa_with_aes_128_gcm_sha256:	({ KE_dh_rsa, CIPHER_aes, HASH_sha256, MODE_gcm }),
   TLS_dh_dss_with_aes_128_gcm_sha256:	({ KE_dh_dss, CIPHER_aes, HASH_sha256, MODE_gcm }),
   TLS_ecdhe_ecdsa_with_aes_128_gcm_sha256: ({ KE_ecdhe_ecdsa, CIPHER_aes, HASH_sha256, MODE_gcm }),
   TLS_ecdh_ecdsa_with_aes_128_gcm_sha256: ({ KE_ecdh_ecdsa, CIPHER_aes, HASH_sha256, MODE_gcm }),
   TLS_ecdhe_rsa_with_aes_128_gcm_sha256: ({ KE_ecdhe_rsa, CIPHER_aes, HASH_sha256, MODE_gcm }),
   TLS_ecdh_rsa_with_aes_128_gcm_sha256: ({ KE_ecdh_rsa, CIPHER_aes, HASH_sha256, MODE_gcm }),

   TLS_rsa_with_aes_256_gcm_sha384:	({ KE_rsa, CIPHER_aes256, HASH_sha384, MODE_gcm }),
   TLS_dhe_rsa_with_aes_256_gcm_sha384:	({ KE_dhe_rsa, CIPHER_aes256, HASH_sha384, MODE_gcm }),
   TLS_dhe_dss_with_aes_256_gcm_sha384:	({ KE_dhe_dss, CIPHER_aes256, HASH_sha384, MODE_gcm }),
   TLS_dh_rsa_with_aes_256_gcm_sha384:	({ KE_dh_rsa, CIPHER_aes256, HASH_sha384, MODE_gcm }),
   TLS_dh_dss_with_aes_256_gcm_sha384:	({ KE_dh_dss, CIPHER_aes256, HASH_sha384, MODE_gcm }),
   TLS_ecdhe_ecdsa_with_aes_256_gcm_sha384: ({ KE_ecdhe_ecdsa, CIPHER_aes256, HASH_sha384, MODE_gcm }),
   TLS_ecdh_ecdsa_with_aes_256_gcm_sha384: ({ KE_ecdh_ecdsa, CIPHER_aes256, HASH_sha384, MODE_gcm }),
   TLS_ecdhe_rsa_with_aes_256_gcm_sha384: ({ KE_ecdhe_rsa, CIPHER_aes256, HASH_sha384, MODE_gcm }),
   TLS_ecdh_rsa_with_aes_256_gcm_sha384: ({ KE_ecdh_rsa, CIPHER_aes256, HASH_sha384, MODE_gcm }),

   // Anonymous variants:
   TLS_dh_anon_with_aes_128_gcm_sha256: ({ KE_dh_anon, CIPHER_aes, HASH_sha256, MODE_gcm }),
   TLS_dh_anon_with_aes_256_gcm_sha384: ({ KE_dh_anon, CIPHER_aes256, HASH_sha384, MODE_gcm }),

#if constant(Crypto.Camellia)
   // Camellia and GCM.
   TLS_rsa_with_camellia_128_gcm_sha256:({ KE_rsa, CIPHER_camellia128, HASH_sha256, MODE_gcm }),
   TLS_rsa_with_camellia_256_gcm_sha384:({ KE_rsa, CIPHER_camellia256, HASH_sha384, MODE_gcm }),
   TLS_dhe_rsa_with_camellia_128_gcm_sha256:({ KE_dhe_rsa, CIPHER_camellia128, HASH_sha256, MODE_gcm }),
   TLS_dhe_rsa_with_camellia_256_gcm_sha384:({ KE_dhe_rsa, CIPHER_camellia256, HASH_sha384, MODE_gcm }),
   TLS_dhe_dss_with_camellia_128_gcm_sha256:({ KE_dhe_dss, CIPHER_camellia128, HASH_sha256, MODE_gcm }),
   TLS_dhe_dss_with_camellia_256_gcm_sha384:({ KE_dhe_dss, CIPHER_camellia256, HASH_sha384, MODE_gcm }),
   TLS_dh_rsa_with_camellia_128_gcm_sha256:({ KE_dh_rsa, CIPHER_camellia128, HASH_sha256, MODE_gcm }),
   TLS_dh_rsa_with_camellia_256_gcm_sha384:({ KE_dh_rsa, CIPHER_camellia256, HASH_sha384, MODE_gcm }),
   TLS_dh_dss_with_camellia_128_gcm_sha256:({ KE_dh_dss, CIPHER_camellia128, HASH_sha256, MODE_gcm }),
   TLS_dh_dss_with_camellia_256_gcm_sha384:({ KE_dh_dss, CIPHER_camellia256, HASH_sha384, MODE_gcm }),

   // Anonymous variants:
   TLS_dh_anon_with_camellia_128_gcm_sha256: ({ KE_dh_anon, CIPHER_camellia128, HASH_sha256, MODE_gcm }),
   TLS_dh_anon_with_camellia_256_gcm_sha384: ({ KE_dh_anon, CIPHER_camellia256, HASH_sha384, MODE_gcm }),

   // From RFC 6367
   TLS_ecdhe_ecdsa_with_camellia_128_gcm_sha256: ({ KE_ecdhe_ecdsa, CIPHER_camellia128, HASH_sha256, MODE_gcm }),
   TLS_ecdhe_ecdsa_with_camellia_256_gcm_sha384: ({ KE_ecdhe_ecdsa, CIPHER_camellia256, HASH_sha384, MODE_gcm }),
   TLS_ecdh_ecdsa_with_camellia_128_gcm_sha256: ({ KE_ecdh_ecdsa, CIPHER_camellia128, HASH_sha256, MODE_gcm }),
   TLS_ecdh_ecdsa_with_camellia_256_gcm_sha384: ({ KE_ecdh_ecdsa, CIPHER_camellia256, HASH_sha384, MODE_gcm }),
   TLS_ecdhe_rsa_with_camellia_128_gcm_sha256: ({ KE_ecdhe_rsa, CIPHER_camellia128, HASH_sha256, MODE_gcm }),
   TLS_ecdhe_rsa_with_camellia_256_gcm_sha384: ({ KE_ecdhe_rsa, CIPHER_camellia256, HASH_sha384, MODE_gcm }),
   TLS_ecdh_rsa_with_camellia_128_gcm_sha256: ({ KE_ecdh_rsa, CIPHER_camellia128, HASH_sha256, MODE_gcm }),
   TLS_ecdh_rsa_with_camellia_256_gcm_sha384: ({ KE_ecdh_rsa, CIPHER_camellia256, HASH_sha384, MODE_gcm }),
#endif /* Crypto.Camellia */
#endif /* Crypto.GCM */
]);

constant HANDSHAKE_hello_v2		= -1; /* Backwards compatibility */
constant HANDSHAKE_hello_request	= 0;  // RFC 5246
constant HANDSHAKE_client_hello		= 1;  // RFC 5246
constant HANDSHAKE_server_hello		= 2;  // RFC 5246
constant HANDSHAKE_hello_verify_request = 3;  // RFC 6347
constant HANDSHAKE_NewSessionTicket     = 4;  // RFC 4507
constant HANDSHAKE_certificate		= 11; // RFC 5246
constant HANDSHAKE_server_key_exchange	= 12; // RFC 5246
constant HANDSHAKE_certificate_request	= 13; // RFC 5246
constant HANDSHAKE_server_hello_done	= 14; // RFC 5246
constant HANDSHAKE_certificate_verify	= 15; // RFC 5246
constant HANDSHAKE_client_key_exchange	= 16; // RFC 5246
constant HANDSHAKE_finished		= 20; // RFC 5246
constant HANDSHAKE_cerificate_url       = 21; // RFC 6066
constant HANDSHAKE_certificate_status   = 22; // RFC 6066
constant HANDSHAKE_supplemental_data    = 23; // RFC 4680
constant HANDSHAKE_next_protocol	= 67;	// draft-agl-tls-nextprotoneg

constant AUTHLEVEL_none		= 1;
constant AUTHLEVEL_ask		= 2;
constant AUTHLEVEL_require	= 3;

/* FIXME: CERT_* would be better names for these constants */
constant AUTH_rsa_sign		= 1;	// SSL 3.0
constant AUTH_dss_sign		= 2;	// SSL 3.0
constant AUTH_rsa_fixed_dh	= 3;	// SSL 3.0
constant AUTH_dss_fixed_dh	= 4;	// SSL 3.0
constant AUTH_rsa_ephemeral_dh	= 5;	// SSL 3.0
constant AUTH_dss_ephemeral_dh	= 6;	// SSL 3.0
constant AUTH_fortezza_kea	= 20;	// SSL 3.0
constant AUTH_fortezza_dms	= 20;
constant AUTH_ecdsa_sign        = 64;	// RFC 4492
constant AUTH_rsa_fixed_ecdh    = 65;	// RFC 4492
constant AUTH_ecdsa_fixed_ecdh  = 66;	// RFC 4492

/* ECC curve types from RFC 4492 5.4. */
enum CurveType {
  CURVETYPE_explicit_prime	= 1,
  CURVETYPE_explicit_char2	= 2,
  CURVETYPE_named_curve		= 3,
}

/* ECBasis types from RFC 4492 5.4 errata. */
enum ECBasisType {
  ECBASIS_trinomial = 1,
  ECBASIS_pentanomial = 2,
}

/* ECC named curves from RFC 4492 5.1.1. */
enum NamedCurve {
  CURVE_sect163k1			= 1,	// RFC 4492
  CURVE_sect163r1			= 2,	// RFC 4492
  CURVE_sect163r2			= 3,	// RFC 4492
  CURVE_sect193r1			= 4,	// RFC 4492
  CURVE_sect193r2			= 5,	// RFC 4492
  CURVE_sect233k1			= 6,	// RFC 4492
  CURVE_sect233r1			= 7,	// RFC 4492
  CURVE_sect239k1			= 8,	// RFC 4492
  CURVE_sect283k1			= 9,	// RFC 4492
  CURVE_sect283r1			= 10,	// RFC 4492
  CURVE_sect409k1			= 11,	// RFC 4492
  CURVE_sect409r1			= 12,	// RFC 4492
  CURVE_sect571k1			= 13,	// RFC 4492
  CURVE_sect571r1			= 14,	// RFC 4492
  CURVE_secp160k1			= 15,	// RFC 4492
  CURVE_secp160r1			= 16,	// RFC 4492
  CURVE_secp160r2			= 17,	// RFC 4492
  CURVE_secp192k1			= 18,	// RFC 4492
  CURVE_secp192r1			= 19,	// RFC 4492
  CURVE_secp224k1			= 20,	// RFC 4492
  CURVE_secp224r1			= 21,	// RFC 4492
  CURVE_secp256k1			= 22,	// RFC 4492
  CURVE_secp256r1			= 23,	// RFC 4492
  CURVE_secp384r1			= 24,	// RFC 4492
  CURVE_secp521r1			= 25,	// RFC 4492

  CURVE_brainpoolP256r1			= 26,	// RFC 7027
  CURVE_brainpoolP384r1			= 27,	// RFC 7027
  CURVE_brainpoolP512r1			= 28,	// RFC 7027

  CURVE_arbitrary_explicit_prime_curves	= 0xFF01,
  CURVE_arbitrary_explicit_char2_curves	= 0xFF02,
}

//! Lookup for Pike ECC name to @[NamedCurve].
constant ECC_NAME_TO_CURVE = ([
  "SECP_192R1": CURVE_secp192r1,
  "SECP_224R1": CURVE_secp224r1,
  "SECP_256R1": CURVE_secp256r1,
  "SECP_384R1": CURVE_secp384r1,
  "SECP_521R1": CURVE_secp521r1,
]);

/* ECC point formats from RFC 4492 5.1.2. */
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

constant EXTENSION_server_name			= 0;		// RFC 6066
constant EXTENSION_max_fragment_length		= 1;		// RFC 6066
constant EXTENSION_client_certificate_url	= 2;		// RFC 6066
constant EXTENSION_trusted_ca_keys		= 3;		// RFC 6066
constant EXTENSION_truncated_hmac		= 4;		// RFC 6066
constant EXTENSION_status_request		= 5;		// RFC 6066
constant EXTENSION_user_mapping                 = 6;            // RFC 4681
constant EXTENSION_client_authz			= 7;		// RFC 5878
constant EXTENSION_server_authz			= 8;		// RFC 5878
constant EXTENSION_cert_type                    = 9;            // RFC 6091
constant EXTENSION_elliptic_curves              = 10;           // RFC 4492
constant EXTENSION_ec_point_formats             = 11;           // RFC 4492
constant EXTENSION_srp                          = 12;           // RFC 5054
constant EXTENSION_signature_algorithms		= 13;		// RFC 5246
constant EXTENSION_use_srtp                     = 14;           // RFC 5764
constant EXTENSION_heartbeat                    = 15;           // RFC 6520
constant EXTENSION_application_layer_protocol_negotiation = 16; // draft-ietf-tls-applayerprotoneg
constant EXTENSION_status_request_v2            = 17;           // RFC-ietf-tls-multiple-cert-status-extension-08
constant EXTENSION_signed_certificate_timestamp = 18;           // RFC 6962
constant EXTENSION_client_certificate_type      = 19;           // RFC-ietf-tls-oob-pubkey-11
constant EXTENSION_server_certificate_type      = 20;           // RFC-ietf-tls-oob-pubkey-11
constant EXTENSION_padding                      = 21;           // TEMPORARY draft-agl-tls-padding
constant EXTENSION_session_ticket_tls           = 35;           // RFC 4507 / RFC 5077
constant EXTENSION_next_protocol_negotiation	= 13172;	// draft-agl-tls-nextprotoneg
constant EXTENSION_renegotiation_info		= 0xff01;	// RFC 5746

constant ECC_CURVES = ([
#if constant(Crypto.ECC.Curve)
  CURVE_secp192r1: Crypto.ECC.SECP_192R1,
  CURVE_secp224r1: Crypto.ECC.SECP_224R1,
  CURVE_secp256r1: Crypto.ECC.SECP_256R1,
  CURVE_secp384r1: Crypto.ECC.SECP_384R1,
  CURVE_secp521r1: Crypto.ECC.SECP_521R1,
#endif
]);

enum HeartBeatModeType {
  HEARTBEAT_MODE_disabled = 0,
  HEARTBEAT_MODE_peer_allowed_to_send = 1,
  HEARTBEAT_MODE_peer_not_allowed_to_send = 1,
};

enum HeartBeatMessageType {
  HEARTBEAT_MESSAGE_request = 1,
  HEARTBEAT_MESSAGE_response = 2,
};

protected mapping(string(8bit):array(HashAlgorithm|SignatureAlgorithm))
  pkcs_der_to_sign_alg = ([
  // RSA
  Standards.PKCS.Identifiers.rsa_md5_id->get_der():
  ({ HASH_md5, SIGNATURE_rsa }),
  Standards.PKCS.Identifiers.rsa_sha1_id->get_der():
  ({ HASH_sha, SIGNATURE_rsa }),
  Standards.PKCS.Identifiers.rsa_sha256_id->get_der():
  ({ HASH_sha256, SIGNATURE_rsa }),
  Standards.PKCS.Identifiers.rsa_sha384_id->get_der():
  ({ HASH_sha384, SIGNATURE_rsa }),
  Standards.PKCS.Identifiers.rsa_sha512_id->get_der():
  ({ HASH_sha512, SIGNATURE_rsa }),

  // DSA
  Standards.PKCS.Identifiers.dsa_sha_id->get_der():
  ({ HASH_sha, SIGNATURE_dsa }),
  Standards.PKCS.Identifiers.dsa_sha224_id->get_der():
  ({ HASH_sha224, SIGNATURE_dsa }),
  Standards.PKCS.Identifiers.dsa_sha256_id->get_der():
  ({ HASH_sha256, SIGNATURE_dsa }),

  // ECDSA
  Standards.PKCS.Identifiers.ecdsa_sha1_id->get_der():
  ({ HASH_sha, SIGNATURE_ecdsa }),
  Standards.PKCS.Identifiers.ecdsa_sha224_id->get_der():
  ({ HASH_sha224, SIGNATURE_ecdsa }),
  Standards.PKCS.Identifiers.ecdsa_sha256_id->get_der():
  ({ HASH_sha256, SIGNATURE_ecdsa }),
  Standards.PKCS.Identifiers.ecdsa_sha384_id->get_der():
  ({ HASH_sha384, SIGNATURE_ecdsa }),
  Standards.PKCS.Identifiers.ecdsa_sha512_id->get_der():
  ({ HASH_sha512, SIGNATURE_ecdsa }),
]);

//! A chain of X509 certificates with corresponding private key.
//!
//! It also contains some derived metadata.
class CertificatePair
{
  //! Private key.
  Crypto.Sign key;

  //! Chain of certificates, root cert last.
  array(string(8bit)) certs;

  //! Array of DER for the issuers matching @[certs].
  array(string(8bit)) issuers;

  //! Array of commonName globs from the first certificate in @[certs].
  array(string(8bit)) globs;

  //! TLS 1.2-style hash and signature pairs matching the @[certs].
  array(array(HashAlgorithm|SignatureAlgorithm)) sign_algs;

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
  protected void create(Crypto.Sign key, array(string(8bit)) certs,
			array(string(8bit))|void extra_name_globs)
  {
    if (!sizeof(certs)) {
      error("Empty list of certificates.\n");
    }

    array(Standards.X509.TBSCertificate) tbss =
      map(certs, Standards.X509.decode_certificate);

    if (has_value(tbss, 0)) error("Invalid cert\n");

    // Validate that the key matches the cert.
    if (!key->public_key_equal(tbss[0]->public_key->pkc)) {
      error("Private key doesn't match certificate.\n");
    }

    this_program::key = key;
    this_program::certs = certs;

    issuers = tbss->issuer->get_der();

    sign_algs = map(map(tbss->algorithm, `[], 0)->get_der(),
		    pkcs_der_to_sign_alg);

    if (has_value(sign_algs, 0)) error("Unknown signature algorithm.\n");

    globs = ({});

    string cn;
    foreach(Standards.PKCS.Certificate.
	    decode_distinguished_name(tbss[0]->subject)->commonName, cn) {
      if (cn) {
	globs += ({ lower_case(cn) });
      }
    }

    if (extra_name_globs) globs += map(extra_name_globs, lower_case);

    if (!sizeof(globs)) error("No common name.\n");

    globs = Array.uniq(globs);

    // FIXME: Ought to check certificate extensions.
    //        cf RFC 5246 7.4.2.
    ke_mask = 0;
    ke_mask_invariant = 0;
    switch(sign_algs[0][1]) {
    case SIGNATURE_rsa:
      foreach(({ KE_rsa, KE_dhe_rsa, KE_ecdhe_rsa }), KeyExchangeType ke) {
	ke_mask |= 1<<ke;
      }
      ke_mask_invariant = ke_mask;
      break;
    case SIGNATURE_dsa:
      ke_mask |= 1<<KE_dhe_dss;
      if ((sizeof(sign_algs) == 1) || (sign_algs[1][1] == SIGNATURE_dsa)) {
	// RFC 4346 7.4.2: DH_DSS
	// Diffie-Hellman key. The algorithm used
	// to sign the certificate MUST be DSS.
	ke_mask |= 1<<KE_dh_dss;
      } else if (sign_algs[1][1] == SIGNATURE_rsa) {
	// RFC 4346 7.4.2: DH_RSA
	// Diffie-Hellman key. The algorithm used
	// to sign the certificate MUST be RSA.
	ke_mask |= 1<<KE_dh_rsa;
      }
      ke_mask_invariant = ke_mask | ((1<<KE_dh_dss) | (1<<KE_dh_rsa));
      break;
    case SIGNATURE_ecdsa:
      ke_mask |= 1<<KE_ecdhe_ecdsa;
      if ((sizeof(sign_algs) == 1) || (sign_algs[1][1] == SIGNATURE_ecdsa)) {
	// RFC 4492 2.1: ECDH_ECDSA
	// In ECDH_ECDSA, the server's certificate MUST contain
	// an ECDH-capable public key and be signed with ECDSA.
	ke_mask |= 1<<KE_ecdh_ecdsa;
      } else if (sign_algs[1][1] == SIGNATURE_rsa) {
	// RFC 4492 2.3: ECDH_RSA
	// This key exchange algorithm is the same as ECDH_ECDSA
	// except that the server's certificate MUST be signed
	// with RSA rather than ECDSA.
	ke_mask |= 1<<KE_ecdh_rsa;
      }
      ke_mask_invariant = ke_mask | ((1<<KE_ecdh_ecdsa) | (1<<KE_ecdh_rsa));
      break;
    }
    if (!ke_mask) error("Certificate not useful for TLS!\n");
  }

  protected string _sprintf(int c)
  {
    return sprintf("CertificatePair(({%{%O, %}}))", globs);
  }
}
