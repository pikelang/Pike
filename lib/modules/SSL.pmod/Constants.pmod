#pike __REAL_VERSION__

/* $Id$
 *
 * The SSL3 Protocol is specified in the following RFCs and drafts:
 *
 *   SSL 3.0			draft-freier-ssl-version3-02.txt
 *
 *   SSL 3.1/TLS 1.0		RFC 2246
 *   AES Ciphers for TLS 1.0	RFC 3268
 *   Extensions for TLS 1.0	RFC 3546
 *
 *   SSL 3.2/TLS 1.1		RFC 4346
 *   Extensions for TLS 1.1	RFC 4366
 *   ECC Ciphers for TLS 1.1	RFC 4492
 *
 *   SSL 3.3/TLS 1.2		RFC 5246
 *   Renegotiation Extension	RFC 5746
 *   Authorization Extensions	RFC 5878
 *
 */

//! Protocol constants

/* Packet types */
constant PACKET_change_cipher_spec = 20;
constant PACKET_alert              = 21;
constant PACKET_handshake          = 22;
constant PACKET_application_data   = 23;
constant PACKET_types = (< PACKET_change_cipher_spec,
			   PACKET_alert,
			   PACKET_handshake,
			   PACKET_application_data >);
constant PACKET_V2 = -1; /* Backwards compatibility */

constant PACKET_MAX_SIZE = 0x4000;

/* Cipher specification */
constant CIPHER_stream   = 0;
constant CIPHER_block    = 1;
constant CIPHER_types = (< CIPHER_stream, CIPHER_block >);

constant CIPHER_null     = 0;
constant CIPHER_rc4_40   = 2;
constant CIPHER_rc2      = 3;
constant CIPHER_des40    = 6;
#ifndef WEAK_CRYPTO_40BIT
constant CIPHER_rc4      = 1;
constant CIPHER_des      = 4;
constant CIPHER_3des     = 5;
constant CIPHER_fortezza = 7;
constant CIPHER_idea	 = 8;
constant CIPHER_aes	 = 9;
constant CIPHER_aes256	 = 10;
#endif /* !WEAK_CRYPTO_40BIT (magic comment) */
constant CIPHER_algorithms = (< CIPHER_null, 
				CIPHER_rc4_40,
				CIPHER_rc2,
				CIPHER_des40,
#ifndef WEAK_CRYPTO_40BIT
				CIPHER_rc4,
				CIPHER_des,
				CIPHER_3des,
				CIPHER_fortezza,
				CIPHER_idea,
				CIPHER_aes,
				CIPHER_aes256,
#endif /* !WEAK_CRYPTO_40BIT (magic comment) */
			     >);
/* Hash algorithms as per RFC 5246 7.4.1.4.1. */
constant HASH_none     = 0;
constant HASH_md5      = 1;
constant HASH_sha      = 2;
constant HASH_sha224   = 3;
constant HASH_sha256   = 4;
constant HASH_sha384   = 5;
constant HASH_sha512   = 6;
constant HASH_hashes = (< HASH_md5, HASH_sha >);

//! Key exchange methods.
enum KeyExchangeType {
  KE_rsa	= 1,	//! Rivest-Shamir-Adelman
  /* We ignore the distinction between dh_dss and dh_rsa for now. */
  KE_dh		= 2,	//! Diffie-Hellman
  KE_dhe_dss	= 3,
  KE_dhe_rsa	= 4,
  KE_dh_anon	= 5,
  KE_dms	= 6,
};

//! Compression methods.
enum CompressionType {
  COMPRESSION_null = 0,	//! No compression.
  COMPRESSION_deflate = 1,
  COMPRESSION_lzs = 64,
};

/* Alert messages */
constant ALERT_warning			= 1;
constant ALERT_fatal			= 2;
constant ALERT_levels = (< ALERT_warning, ALERT_fatal >);

constant ALERT_close_notify             = 0;
constant ALERT_unexpected_message       = 10;
constant ALERT_bad_record_mac           = 20;
constant ALERT_decryption_failed        = 21;
constant ALERT_record_overflow          = 22;
constant ALERT_decompression_failure    = 30;
constant ALERT_handshake_failure        = 40;
constant ALERT_no_certificate           = 41;
constant ALERT_bad_certificate          = 42;
constant ALERT_unsupported_certificate  = 43;
constant ALERT_certificate_revoked      = 44;
constant ALERT_certificate_expired      = 45;
constant ALERT_certificate_unknown      = 46;
constant ALERT_illegal_parameter        = 47;
constant ALERT_unknown_ca               = 48;
constant ALERT_access_denied            = 49;
constant ALERT_decode_error             = 50;
constant ALERT_decrypt_error            = 51;
constant ALERT_export_restriction_RESERVED = 60;
constant ALERT_protocol_version         = 70;
constant ALERT_insufficient_security    = 71;
constant ALERT_internal_error           = 80;
constant ALERT_user_canceled            = 90;
constant ALERT_no_renegotiation         = 100;
constant ALERT_unsupported_extension    = 110;
constant ALERT_certificate_unobtainable = 111;
constant ALERT_unregonized_name         = 112;
constant ALERT_bad_certificate_status_response = 113;
constant ALERT_bad_certificate_hash_value = 114;
constant ALERT_unknown_psk_identity     = 115;
constant ALERT_descriptions = (< ALERT_close_notify,
 				 ALERT_unexpected_message,
 				 ALERT_bad_record_mac,
 				 ALERT_decompression_failure,
 				 ALERT_handshake_failure,
 				 ALERT_no_certificate,
 				 ALERT_bad_certificate,
 				 ALERT_unsupported_certificate,
 				 ALERT_certificate_revoked,
 				 ALERT_certificate_expired,
 				 ALERT_certificate_unknown,
				 ALERT_illegal_parameter,
				 ALERT_no_renegotiation,
			      >);
 			      
constant CONNECTION_client 	= 0;
constant CONNECTION_server 	= 1;
constant CONNECTION_client_auth = 2;

/* Cipher suites */
constant SSL_null_with_null_null 		= 0x0000;
constant SSL_rsa_with_null_md5			= 0x0001;
constant SSL_rsa_with_null_sha			= 0x0002;
constant SSL_rsa_export_with_rc4_40_md5		= 0x0003;
constant SSL_rsa_export_with_rc2_cbc_40_md5	= 0x0006;
constant SSL_rsa_export_with_des40_cbc_sha	= 0x0008;
constant SSL_dh_dss_export_with_des40_cbc_sha	= 0x000b;
constant SSL_dh_rsa_export_with_des40_cbc_sha	= 0x000e;
constant SSL_dhe_dss_export_with_des40_cbc_sha	= 0x0011;
constant SSL_dhe_rsa_export_with_des40_cbc_sha	= 0x0014;
constant SSL_dh_anon_export_with_rc4_40_md5	= 0x0017;
constant SSL_dh_anon_export_with_des40_cbc_sha	= 0x0019;
constant TLS_krb5_with_des_cbc_40_sha           = 0x0026;
constant TLS_krb5_with_rc2_cbc_40_sha           = 0x0027;
constant TLS_krb5_with_rc4_40_sha               = 0x0028;
constant TLS_krb5_with_des_cbc_40_md5           = 0x0029;
constant TLS_krb5_with_rc2_cbc_40_md5           = 0x002a;
constant TLS_krb5_with_rc4_40_md5               = 0x002b;
#ifndef WEAK_CRYPTO_40BIT
constant SSL_rsa_with_rc4_128_md5		= 0x0004;
constant SSL_rsa_with_rc4_128_sha		= 0x0005;
constant SSL_rsa_with_idea_cbc_sha		= 0x0007;
constant SSL_rsa_with_des_cbc_sha		= 0x0009;
constant SSL_rsa_with_3des_ede_cbc_sha		= 0x000a; 
constant SSL_dh_dss_with_des_cbc_sha		= 0x000c;
constant SSL_dh_dss_with_3des_ede_cbc_sha	= 0x000d;
constant SSL_dh_rsa_with_des_cbc_sha		= 0x000f;
constant SSL_dh_rsa_with_3des_ede_cbc_sha	= 0x0010;
constant SSL_dhe_dss_with_des_cbc_sha		= 0x0012;
constant SSL_dhe_dss_with_3des_ede_cbc_sha	= 0x0013;
constant SSL_dhe_rsa_with_des_cbc_sha		= 0x0015;
constant SSL_dhe_rsa_with_3des_ede_cbc_sha	= 0x0016; 
constant SSL_dh_anon_with_rc4_128_md5		= 0x0018;
constant SSL_dh_anon_with_des_cbc_sha		= 0x001a;
constant SSL_dh_anon_with_3des_ede_cbc_sha	= 0x001b; 

/* SSLv3/TLS conflict */
/* constant SSL_fortezza_dms_with_null_sha		= 0x001c; */
/* constant SSL_fortezza_dms_with_fortezza_cbc_sha	= 0x001d; */
/* constant SSL_fortezza_dms_with_rc4_128_sha	= 0x001e; */

constant TLS_krb5_with_des_cbc_sha              = 0x001e;
constant TLS_krb5_with_3des_ede_cbc_sha         = 0x001f;
constant TLS_krb5_with_rc4_128_sha              = 0x0020;
constant TLS_krb5_with_idea_cbc_sha             = 0x0021;
constant TLS_krb5_with_des_cbc_md5              = 0x0022;
constant TLS_krb5_with_3des_ede_cbc_md5         = 0x0023;
constant TLS_krb5_with_rc4_128_md5              = 0x0024;
constant TLS_krb5_with_idea_cbc_md5             = 0x0025;
constant TLS_psk_with_null_sha                  = 0x002c;
constant TLS_dhe_psk_with_null_sha              = 0x002d;
constant TLS_rsa_psk_with_null_sha              = 0x002e;
constant TLS_rsa_with_aes_128_cbc_sha           = 0x002f;
constant TLS_dh_dss_with_aes_128_cbc_sha        = 0x0030;
constant TLS_dh_rsa_with_aes_128_cbc_sha        = 0x0031;
constant TLS_dhe_dss_with_aes_128_cbc_sha       = 0x0032;
constant TLS_dhe_rsa_with_aes_128_cbc_sha       = 0x0033;
constant TLS_dh_anon_with_aes_128_cbc_sha       = 0x0034;
constant TLS_rsa_with_aes_256_cbc_sha           = 0x0035;
constant TLS_dh_dss_with_aes_256_cbc_sha        = 0x0036;
constant TLS_dh_rsa_with_aes_256_cbc_sha        = 0x0037;
constant TLS_dhe_dss_with_aes_256_cbc_sha       = 0x0038;
constant TLS_dhe_rsa_with_aes_256_cbc_sha       = 0x0039;
constant TLS_dh_anon_with_aes_256_cbc_sha       = 0x003a;
constant TLS_rsa_with_null_sha256               = 0x003b;
constant TLS_rsa_with_aes_128_cbc_sha256        = 0x003c;
constant TLS_rsa_with_aes_256_cbc_sha256        = 0x003d;
constant TLS_dh_dss_with_aes_128_cbc_sha256     = 0x003e;
constant TLS_dh_rsa_with_aes_128_cbc_sha256     = 0x003f;
constant TLS_dhe_dss_with_aes_128_cbc_sha256    = 0x0040;
constant TLS_rsa_with_camellia_128_cbc_sha      = 0x0041;
constant TLS_dh_dss_with_camellia_128_cbc_sha   = 0x0042;
constant TLS_dh_rsa_with_camellia_128_cbc_sha   = 0x0043;
constant TLS_dhe_dss_with_camellia_128_cbc_sha  = 0x0044;
constant TLS_dhe_rsa_with_camellia_128_cbc_sha  = 0x0045;
constant TLS_dh_anon_with_camellia_128_cbc_sha  = 0x0046;

constant TLS_dhe_rsa_with_aes_128_cbc_sha256    = 0x0067;
constant TLS_dh_dss_with_aes_256_cbc_sha256     = 0x0068;
constant TLS_dh_rsa_with_aes_256_cbc_sha256     = 0x0069;
constant TLS_dhe_dss_with_aes_256_cbc_sha256    = 0x006a;
constant TLS_dhe_rsa_with_aes_256_cbc_sha256    = 0x006b;
constant TLS_dh_anon_with_aes_128_cbc_sha256    = 0x006c;
constant TLS_dh_anon_with_aes_256_cbc_sha256    = 0x006d;
constant TLS_rsa_with_camellia_256_cbc_sha      = 0x0084;
constant TLS_dh_dss_with_camellia_256_cbc_sha   = 0x0085;
constant TLS_dh_rsa_with_camellia_256_cbc_sha   = 0x0086;
constant TLS_dhe_dss_with_camellia_256_cbc_sha  = 0x0087;
constant TLS_dhe_rsa_with_camellia_256_cbc_sha  = 0x0088;
constant TLS_dh_anon_with_camellia_256_cbc_sha  = 0x0089;
constant TLS_psk_with_rc4_128_sha               = 0x008a;
constant TLS_psk_with_3des_ede_cbc_sha          = 0x008b;
constant TLS_psk_with_aes_128_cbc_sha           = 0x008c;
constant TLS_psk_with_aes_256_cbc_sha           = 0x008d;
constant TLS_dhe_psk_with_rc4_128_sha           = 0x008e;
constant TLS_dhe_psk_with_3des_ede_cbc_sha      = 0x008f;
constant TLS_dhe_psk_with_aes_128_cbc_sha       = 0x0090;
constant TLS_dhe_psk_with_aes_256_cbc_sha       = 0x0091;
constant TLS_rsa_psk_with_rc4_128_sha           = 0x0092;
constant TLS_rsa_psk_with_3des_ede_cbc_sha      = 0x0093;
constant TLS_rsa_psk_with_aes_128_cbc_sha       = 0x0094;
constant TLS_rsa_psk_with_aes_256_cbc_sha       = 0x0095;
constant TLS_rsa_with_seed_cbc_sha              = 0x0096;
constant TLS_dh_dss_with_seed_cbc_sha           = 0x0097;
constant TLS_dh_rsa_with_seed_cbc_sha           = 0x0098;
constant TLS_dhe_dss_with_seed_cbc_sha          = 0x0099;
constant TLS_dhe_rsa_with_seed_cbc_sha          = 0x009a;
constant TLS_dh_anon_with_seed_cbc_sha          = 0x009b;
constant TLS_rsa_with_aes_128_gcm_sha256        = 0x009c;
constant TLS_rsa_with_aes_256_gcm_sha384        = 0x009d;
constant TLS_dhe_rsa_with_aes_128_gcm_sha256    = 0x009e;
constant TLS_dhe_rsa_with_aes_256_gcm_sha384    = 0x009f;
constant TLS_dh_rsa_with_aes_128_gcm_sha256     = 0x00a0;
constant TLS_dh_rsa_with_aes_256_gcm_sha384     = 0x00a1;
constant TLS_dhe_dss_with_aes_128_gcm_sha256    = 0x00a2;
constant TLS_dhe_dss_with_aes_256_gcm_sha384    = 0x00a3;
constant TLS_dh_dss_with_aes_128_gcm_sha256     = 0x00a4;
constant TLS_dh_dss_with_aes_256_gcm_sha384     = 0x00a5;
constant TLS_dh_anon_with_aes_128_gcm_sha256    = 0x00a6;
constant TLS_dh_anon_with_aes_256_gcm_sha384    = 0x00a7;
constant TLS_psk_with_aes_128_gcm_sha256        = 0x00a8;
constant TLS_psk_with_aes_256_gcm_sha384        = 0x00a9;
constant TLS_dhe_psk_with_aes_128_gcm_sha256    = 0x00aa;
constant TLS_dhe_psk_with_aes_256_gcm_sha384    = 0x00ab;
constant TLS_rsa_psk_with_aes_128_gcm_sha256    = 0x00ac;
constant TLS_rsa_psk_with_aes_256_gcm_sha384    = 0x00ad;
constant TLS_psk_with_aes_128_cbc_sha256        = 0x00ae;
constant TLS_psk_with_aes_256_cbc_sha384        = 0x00af;
constant TLS_psk_with_null_sha256               = 0x00b0;
constant TLS_psk_with_null_sha384               = 0x00b1;
constant TLS_dhe_psk_with_aes_128_cbc_sha256    = 0x00b2;
constant TLS_dhe_psk_with_aes_256_cbc_sha384    = 0x00b3;
constant TLS_dhe_psk_with_null_sha256           = 0x00b4;
constant TLS_dhe_psk_with_null_sha384           = 0x00b5;
constant TLS_rsa_psk_with_aes_128_cbc_sha256    = 0x00b6;
constant TLS_rsa_psk_with_aes_256_cbc_sha384    = 0x00b7;
constant TLS_rsa_psk_with_null_sha256           = 0x00b8;
constant TLS_rsa_psk_with_null_sha384           = 0x00b9;
constant TLS_empty_renegotiation_info_scsv	= 0x00ff;	// RFC 5746
constant TLS_ecdh_ecdsa_with_null_sha           = 0xc001;
constant TLS_ecdh_ecdsa_with_rc4_128_sha        = 0xc002;
constant TLS_ecdh_ecdsa_with_3des_ede_cbc_sha   = 0xc003;
constant TLS_ecdh_ecdsa_with_aes_128_cbc_sha    = 0xc004;
constant TLS_ecdh_ecdsa_with_aes_256_cbc_sha    = 0xc005;
constant TLS_ecdhe_ecdsa_with_null_sha          = 0xc006;
constant TLS_ecdhe_ecdsa_with_rc4_128_sha       = 0xc007;
constant TLS_ecdhe_ecdsa_with_3des_ede_cbc_sha  = 0xc008;
constant TLS_ecdhe_ecdsa_with_aes_128_cbc_sha   = 0xc009;
constant TLS_ecdhe_ecdsa_with_aes_256_cbc_sha   = 0xc00a;
constant TLS_ecdh_rsa_with_null_sha             = 0xc00b;
constant TLS_ecdh_rsa_with_rc4_128_sha          = 0xc00c;
constant TLS_ecdh_rsa_with_3des_ede_cbc_sha     = 0xc00d;
constant TLS_ecdh_rsa_with_aes_128_cbc_sha      = 0xc00e;
constant TLS_ecdh_rsa_with_aes_256_cbc_sha      = 0xc00f;
constant TLS_ecdhe_rsa_with_null_sha            = 0xc010;
constant TLS_ecdhe_rsa_with_rc4_128_sha         = 0xc011;
constant TLS_ecdhe_rsa_with_3des_ede_cbc_sha    = 0xc012;
constant TLS_ecdhe_rsa_with_aes_128_cbc_sha     = 0xc013;
constant TLS_ecdhe_rsa_with_aes_256_cbc_sha     = 0xc014;
constant TLS_ecdh_anon_with_null_sha            = 0xc015;
constant TLS_ecdh_anon_with_rc4_128_sha         = 0xc016;
constant TLS_ecdh_anon_with_3des_ede_cbc_sha    = 0xc017;
constant TLS_ecdh_anon_with_aes_128_cbc_sha     = 0xc018;
constant TLS_ecdh_anon_with_aes_256_cbc_sha     = 0xc019;
constant TLS_srp_sha_with_3des_ede_cbc_sha      = 0xc01a;
constant TLS_srp_sha_rsa_with_3des_ede_cbc_sha  = 0xc01b;
constant TLS_srp_sha_dss_with_3des_ede_cbc_sha  = 0xc01c;
constant TLS_srp_sha_with_aes_128_cbc_sha       = 0xc01d;
constant TLS_srp_sha_rsa_with_aes_128_cbc_sha   = 0xc01e;
constant TLS_srp_sha_dss_with_aes_128_cbc_sha   = 0xc01f;
constant TLS_srp_sha_with_aes_256_cbc_sha       = 0xc020;
constant TLS_srp_sha_rsa_with_aes_256_cbc_sha   = 0xc021;
constant TLS_srp_sha_dss_with_aes_256_cbc_sha   = 0xc022;
constant TLS_ecdhe_ecdsa_with_aes_128_cbc_sha256= 0xc023;
constant TLS_ecdhe_ecdsa_with_aes_256_cbc_sha384= 0xc024;
constant TLS_ecdh_ecdsa_with_aes_128_cbc_sha256 = 0xc025;
constant TLS_ecdh_ecdsa_with_aes_256_cbc_sha384 = 0xc026;
constant TLS_ecdhe_rsa_with_aes_128_cbc_sha256  = 0xc027;
constant TLS_ecdhe_rsa_with_aes_256_cbc_sha384  = 0xc028;
constant TLS_ecdh_rsa_with_aes_128_cbc_sha256   = 0xc029;
constant TLS_ecdh_rsa_with_aes_256_cbc_sha384   = 0xc02a;
constant TLS_ecdhe_ecdsa_with_aes_128_gcm_sha256= 0xc02b;
constant TLS_ecdhe_ecdsa_with_aes_256_gcm_sha384= 0xc02c;
constant TLS_ecdh_ecdsa_with_aes_128_gcm_sha256 = 0xc02d;
constant TLS_ecdh_ecdsa_with_aes_256_gcm_sha384 = 0xc02e;
constant TLS_ecdhe_rsa_with_aes_128_gcm_sha256  = 0xc02f;
constant TLS_ecdhe_rsa_with_aes_256_gcm_sha384  = 0xc030;
constant TLS_ecdh_rsa_with_aes_128_gcm_sha256   = 0xc031;
constant TLS_ecdh_rsa_with_aes_256_gcm_sha384   = 0xc032;
constant TLS_ecdhe_psk_with_rc4_128_sha         = 0xc033;
constant TLS_ecdhe_psk_with_3des_ede_cbc_sha    = 0xc034;
constant TLS_ecdhe_psk_with_aes_128_cbc_sha     = 0xc035;
constant TLS_ecdhe_psk_with_aes_256_cbc_sha     = 0xc036;
constant TLS_ecdhe_psk_with_aes_128_cbc_sha256  = 0xc037;
constant TLS_ecdhe_psk_with_aes_256_cbc_sha384  = 0xc038;
constant TLS_ecdhe_psk_with_null_sha            = 0xc039;
constant TLS_ecdhe_psk_with_null_sha256         = 0xc03a;
constant TLS_ecdhe_psk_with_null_sha384         = 0xc03b;
#endif /* !WEAK_CRYPTO_40BIT (magic comment) */

#if 0
/* Methods for signing any server_key_exchange message (RFC 5246 7.4.1.4.1) */
constant SIGN_anon = 0;
constant SIGN_rsa = 1;
constant SIGN_dsa = 2;
constant SIGN_ecdsa = 3;

/* FIXME: Add SIGN-type element to table */
#endif

constant CIPHER_SUITES =
([ SSL_null_with_null_null :    	({ 0, 0, 0 }),
   SSL_rsa_with_null_md5 :      	({ KE_rsa, 0, HASH_md5 }), 
   SSL_rsa_with_null_sha :      	({ KE_rsa, 0, HASH_sha }),
   SSL_rsa_export_with_rc4_40_md5 :	({ KE_rsa, CIPHER_rc4_40, HASH_md5 }),
   SSL_dhe_dss_export_with_des40_cbc_sha :
      ({ KE_dhe_dss, CIPHER_des40, HASH_sha }),
#ifndef WEAK_CRYPTO_40BIT
   SSL_rsa_with_rc4_128_sha :		({ KE_rsa, CIPHER_rc4, HASH_sha }),
   SSL_rsa_with_rc4_128_md5 :		({ KE_rsa, CIPHER_rc4, HASH_md5 }),
   SSL_rsa_with_idea_cbc_sha :		({ KE_rsa, CIPHER_idea, HASH_sha }),
   SSL_rsa_with_des_cbc_sha :		({ KE_rsa, CIPHER_des, HASH_sha }),
   SSL_rsa_with_3des_ede_cbc_sha :	({ KE_rsa, CIPHER_3des, HASH_sha }),
   SSL_dhe_dss_with_des_cbc_sha :	({ KE_dhe_dss, CIPHER_des, HASH_sha }),
   SSL_dhe_dss_with_3des_ede_cbc_sha :	({ KE_dhe_dss, CIPHER_3des, HASH_sha }),
   TLS_rsa_with_aes_128_cbc_sha :	({ KE_rsa, CIPHER_aes, HASH_sha }),
   TLS_dhe_dss_with_aes_128_cbc_sha :	({ KE_dhe_dss, CIPHER_aes, HASH_sha }),
   TLS_rsa_with_aes_256_cbc_sha :	({ KE_rsa, CIPHER_aes256, HASH_sha }),
   TLS_dhe_dss_with_aes_256_cbc_sha :	({ KE_dhe_dss, CIPHER_aes256, HASH_sha }),
#endif /* !WEAK_CRYPTO_40BIT (magic comment) */
]);

constant HANDSHAKE_hello_v2		= -1; /* Backwards compatibility */
constant HANDSHAKE_hello_request	= 0;
constant HANDSHAKE_client_hello		= 1;
constant HANDSHAKE_server_hello		= 2;
constant HANDSHAKE_hello_verify_request = 3;
constant HANDSHAKE_NewSessionTicket     = 4;
constant HANDSHAKE_certificate		= 11;
constant HANDSHAKE_server_key_exchange	= 12;
constant HANDSHAKE_certificate_request	= 13;
constant HANDSHAKE_server_hello_done	= 14;
constant HANDSHAKE_certificate_verify	= 15;
constant HANDSHAKE_client_key_exchange	= 16;
constant HANDSHAKE_finished		= 20;
constant HANDSHAKE_cerificate_url       = 21;
constant HANDSHAKE_certificate_status   = 22;
constant HANDSHAKE_supplemental_data    = 23;

constant AUTHLEVEL_none		= 1;
constant AUTHLEVEL_ask		= 2;
constant AUTHLEVEL_require	= 3;

/* FIXME: CERT_* would be better names for these constants */
constant AUTH_rsa_sign		= 1;
constant AUTH_dss_sign		= 2;
constant AUTH_rsa_fixed_dh	= 3;
constant AUTH_dss_fixed_dh	= 4;
constant AUTH_rsa_ephemeral_dh	= 5;
constant AUTH_dss_ephemeral_dh	= 6;
constant AUTH_fortezza_dms	= 20;
constant AUTH_ecdsa_sign        = 64;
constant AUTH_rsa_fixed_ecdh    = 65;
constant AUTH_ecdsa_fixed_ecdh  = 66;

constant EXTENSION_server_name			= 0;		// RFC 4366.
constant EXTENSION_max_fragment_length		= 1;		// RFC 4366.
constant EXTENSION_client_certificate_url	= 2;		// RFC 4366.
constant EXTENSION_trusted_ca_keys		= 3;		// RFC 4366.
constant EXTENSION_truncated_hmac		= 4;		// RFC 4366.
constant EXTENSION_status_request		= 5;		// RFC 4366.
constant EXTENSION_client_authz			= 7;		// RFC 5878.
constant EXTENSION_server_authz			= 8;		// RFC 5878.
constant EXTENSION_signature_algorithms		= 13;		// RFC 5246.
constant EXTENSION_renegotiation_info		= 0xff01;	// RFC 5746.
