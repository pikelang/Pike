#pike __REAL_VERSION__

/* $Id$
 *
 */

//! Protocol constants
//! @deprecated SSL.Constants

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
#endif /* !WEAK_CRYPTO_40BIT (magic comment) */
			     >);

constant HASH_md5      = 1;
constant HASH_sha      = 2;
constant HASH_hashes = (< HASH_md5, HASH_sha >);

/* Key exchange */
constant KE_rsa	= 1;
/* We ignore the distinction between dh_dss and dh_rsa for now. */
constant KE_dh	= 2;
constant KE_dhe_dss = 3;
constant KE_dhe_rsa = 4;
constant KE_dh_anon = 5;
constant KE_dms	= 6;

/* Compression methods */
constant COMPRESSION_null = 0;

/* Alert messages */
constant ALERT_warning			= 1;
constant ALERT_fatal			= 2;
constant ALERT_levels = (< ALERT_warning, ALERT_fatal >);

constant ALERT_close_notify             = 0;
constant ALERT_unexpected_message       = 10;
constant ALERT_bad_record_mac           = 20;
constant ALERT_decompression_failure    = 30;
constant ALERT_handshake_failure        = 40;
constant ALERT_no_certificate           = 41;
constant ALERT_bad_certificate          = 42;
constant ALERT_unsupported_certificate  = 43;
constant ALERT_certificate_revoked      = 44;
constant ALERT_certificate_expired      = 45;
constant ALERT_certificate_unknown      = 46;
constant ALERT_illegal_parameter        = 47;
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
 				 ALERT_illegal_parameter >);
 			      
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
constant SSL_fortezza_dms_with_null_sha		= 0x001c;
constant SSL_fortezza_dms_with_fortezza_cbc_sha	= 0x001d;
constant SSL_fortezza_dms_with_rc4_128_sha	= 0x001e;
#endif /* !WEAK_CRYPTO_40BIT (magic comment) */

#if 0
/* Methods for signing any server_key_exchange message */
constant SIGN_anon = 0;
constant SIGN_rsa = 1;
constant SIGN_dsa = 2;

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
#endif /* !WEAK_CRYPTO_40BIT (magic comment) */
]);

constant HANDSHAKE_hello_v2		= -1; /* Backwards compatibility */
constant HANDSHAKE_hello_request	= 0;
constant HANDSHAKE_client_hello		= 1;
constant HANDSHAKE_server_hello		= 2;
constant HANDSHAKE_certificate		= 11;
constant HANDSHAKE_server_key_exchange	= 12;
constant HANDSHAKE_certificate_request	= 13;
constant HANDSHAKE_server_hello_done	= 14;
constant HANDSHAKE_certificate_verify	= 15;
constant HANDSHAKE_client_key_exchange	= 16;
constant HANDSHAKE_finished		= 20;

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




