#pike __REAL_VERSION__
#pragma strict_types
#require constant(SSL.Cipher)

//! Server-side connection state.

#ifdef SSL3_DEBUG
#define SSL3_DEBUG_MSG(X ...)  werror(X)
#else
#define SSL3_DEBUG_MSG(X ...)
#endif

import ".";
import Constants;
inherit Connection;

// ALPN
int has_application_layer_protocol_negotiation;
string(8bit) application_protocol;

multiset(int) remote_extensions = (<>);

protected string _sprintf(int t)
{
  if (t == 'O') return sprintf("SSL.ServerConnection(%s)", describe_state());
}

Packet hello_request()
{
  return handshake_packet(HANDSHAKE_hello_request, "");
}

Packet finished_packet(string(8bit) sender)
{
  SSL3_DEBUG_MSG("Sending finished_packet, with sender=\""+sender+"\"\n" );
  // We're the server.
  server_verify_data = hash_messages(sender);
  return handshake_packet(HANDSHAKE_finished, server_verify_data);
}

Packet server_hello_packet()
{
  Buffer struct = Buffer();
  /* Build server_hello message */
  struct->add_int(version, 2); /* version */
  SSL3_DEBUG_MSG("Writing server hello, with version: %s\n",
                 fmt_version(version));
  struct->add(server_random);
  struct->add_hstring(session->identity, 1);
  struct->add_int(session->cipher_suite, 2);
  struct->add_int(session->compression_algorithm, 1);

  Buffer extensions = Buffer();
  Alert fail;

  void ext(int id, int condition, function(void:Buffer) code)
  {
    if(condition)
    {
      extensions->add_int(id, 2);
      extensions->add_hstring(code(), 2);
    }
  };

  ext (EXTENSION_renegotiation_info, secure_renegotiation) {
    // RFC 5746 3.7:
    // The server MUST include a "renegotiation_info" extension
    // containing the saved client_verify_data and server_verify_data in
    // the ServerHello.
    return Buffer()->add_hstring(client_verify_data + server_verify_data, 1);
  };

  ext (EXTENSION_max_fragment_length, session->max_packet_size != PACKET_MAX_SIZE) {
    // RFC 3546 3.2.
    Buffer extension = Buffer();
    switch(session->max_packet_size) {
    case 512:  extension->add_int(FRAGMENT_512,  1); break;
    case 1024: extension->add_int(FRAGMENT_1024, 1); break;
    case 2048: extension->add_int(FRAGMENT_2048, 1); break;
    case 4096: extension->add_int(FRAGMENT_4096, 1); break;
    default:
      fail = alert(ALERT_fatal, ALERT_illegal_parameter,
                   "Invalid fragment size.\n");
    }
    return extension;
  };

  ext (EXTENSION_ec_point_formats, sizeof(session->ecc_curves) &&
       remote_extensions[EXTENSION_ec_point_formats]) {
    // RFC 4492 5.2:
    // The Supported Point Formats Extension is included in a
    // ServerHello message in response to a ClientHello message
    // containing the Supported Point Formats Extension when
    // negotiating an ECC cipher suite.
    Buffer point = Buffer();
    point->add_int(POINT_uncompressed, 1);
    return Buffer()->add_hstring(point, 1);
  };

  ext (EXTENSION_truncated_hmac, session->truncated_hmac) {
    // RFC 3546 3.5 "Truncated HMAC"
    return Buffer();
  };

  ext (EXTENSION_heartbeat, session->heartbeat_mode) {
    // RFC 6520
    return Buffer()->add_int(HEARTBEAT_MODE_peer_allowed_to_send, 1);
  };

  ext (EXTENSION_encrypt_then_mac, session->encrypt_then_mac) {
    // draft-ietf-tls-encrypt-then-mac
    return Buffer();
  };

  ext (EXTENSION_application_layer_protocol_negotiation,
       application_protocol && has_application_layer_protocol_negotiation)
  {
    return Buffer()->add_string_array(({application_protocol}), 1, 2);
  };

  if (fail) return fail;

  // NB: Assume that the client understands extensions
  //     if it has sent extensions...
  if (sizeof(extensions))
      struct->add_hstring(extensions, 2);

  return handshake_packet(HANDSHAKE_server_hello, struct);
}

Packet server_key_exchange_packet()
{
  if (ke) error("KE!\n");
  ke = session->cipher_spec->ke_factory(context, session, this, client_version);
  string(8bit) data = ke->server_key_exchange_packet(client_random, server_random);
  return data && handshake_packet(HANDSHAKE_server_key_exchange, data);
}

Packet certificate_request_packet(Context context)
{
    /* Send a CertificateRequest message */
    Buffer struct = Buffer();
    struct->add_int_array(context->preferred_auth_methods, 1, 1);
    if (version >= PROTOCOL_TLS_1_2) {
      // TLS 1.2 has var_uint_array of hash and sign pairs here.
      struct->add_hstring(get_signature_algorithms(), 2);
    }
    struct->add_hstring([string(8bit)]
                        sprintf("%{%2H%}", context->authorities_cache), 2);
    return handshake_packet(HANDSHAKE_certificate_request, struct);
}

//! Renegotiate the connection (server initiated).
//!
//! Sends a @[hello_request] to force a new round of handshaking.
void send_renegotiate()
{
  send_packet(hello_request(), PRI_application);
}

int(0..1) not_ecc_suite(int cipher_suite)
{
  array(int) suite = [array(int)]CIPHER_SUITES[cipher_suite];
  return suite &&
    !(< KE_ecdh_ecdsa, KE_ecdhe_ecdsa, KE_ecdh_rsa, KE_ecdhe_rsa >)[suite[0]];
}

int(-1..0) reply_new_session(array(int) cipher_suites,
			     array(int) compression_methods)
{
  send_packet(server_hello_packet());

  // Don't send any certificate in anonymous mode.
  if (session->cipher_spec->signature_alg != SIGNATURE_anonymous) {
    // NB: session->certificate_chain is set by
    // session->select_cipher_suite() above.
    if (session->certificate_chain)
    {
      SSL3_DEBUG_MSG("Sending Certificate.\n");
      send_packet(certificate_packet(session->certificate_chain));
    } else {
      // Otherwise the server will just silently send an invalid
      // ServerHello sequence.
      error ("Certificate(s) missing.\n");
    }
  }

  Packet key_exchange = server_key_exchange_packet();

  if (key_exchange) {
    SSL3_DEBUG_MSG("Sending ServerKeyExchange.\n");
    send_packet(key_exchange);
  }
  if (context->auth_level >= AUTHLEVEL_ask)
  {
    // we can send a certificate request packet, even if we don't have
    // any authorized issuers.
    SSL3_DEBUG_MSG("Sending CertificateRequest.\n");
    send_packet(certificate_request_packet(context));
    certificate_state = CERT_requested;
  }
  send_packet(handshake_packet(HANDSHAKE_server_hello_done, ""));
  return 0;
}

string(8bit) server_derive_master_secret(string(8bit) data)
{
  string(8bit)|int(8bit) res =
    ke->server_derive_master_secret(data, client_random, server_random, version);
  if (stringp(res)) return [string]res;
  send_packet(alert(ALERT_fatal, [int(8bit)]res,
		    "Failed to derive master secret.\n"));
  return 0;
}

protected void create(Context ctx)
{
  ::create(ctx);
  handshake_state = STATE_wait_for_hello;
}

//! Do handshake processing. Type is one of HANDSHAKE_*, data is the
//! contents of the packet, and raw is the raw packet received (needed
//! for supporting SSLv2 hello messages).
//!
//! This function returns 0 if handshake is in progress, 1 if handshake
//! is finished, and -1 if a fatal error occurred. It uses the
//! send_packet() function to transmit packets.
int(-1..1) handle_handshake(int type, string(8bit) data, string(8bit) raw)
{
  Buffer input = Buffer(data);
#ifdef SSL3_PROFILING
  addRecord(type,0);
#endif
#ifdef SSL3_DEBUG_HANDSHAKE_STATE
  werror("SSL.ServerConnection: state %s, type %s\n",
	 fmt_constant(handshake_state, "STATE"),
         fmt_constant(type, "HANDSHAKE"));
  werror("sizeof(data)="+sizeof(data)+"\n");
#endif

  switch(handshake_state)
  {
  default:
    error( "Internal error\n" );
  case STATE_wait_for_hello:
   {
     array(int) cipher_suites;

     /* Reset all extra state variables */
     expect_change_cipher = certificate_state = 0;
     ke = 0;

     handshake_messages = raw;


     // The first four bytes of the client_random is specified to be
     // the timestamp on the client side. This is to guard against bad
     // random generators, where a client could produce the same
     // random numbers if the seed is reused. This argument is flawed,
     // since a broken random generator will make the connection
     // insecure anyways. The standard explicitly allows these bytes
     // to not be correct, so sending random data instead is safer and
     // reduces client fingerprinting.
     server_random = context->random(32);

     switch(type)
     {
     default:
       send_packet(alert(ALERT_fatal, ALERT_unexpected_message,
			 "Expected client_hello.\n"));
       return -1;
     case HANDSHAKE_client_hello:
      {
	string id;
	int cipher_len;
	array(int) cipher_suites;
	array(int) compression_methods;

	SSL3_DEBUG_MSG("SSL.ServerConnection: CLIENT_HELLO\n");

	if (
	  catch{
	    client_version =
	      [int(0x300..0x300)|ProtocolVersion]input->read_int(2);
	  client_random = input->read(32);
	  id = input->read_hstring(1);
	  cipher_len = input->read_int(2);
	  cipher_suites = input->read_ints(cipher_len/2, 2);
	  compression_methods = input->read_int_array(1, 1);
	  SSL3_DEBUG_MSG("STATE_wait_for_hello: received hello\n"
			 "version = %s\n"
			 "id=%O\n"
			 "cipher suites:\n%s\n"
			 "compression methods: %O\n",
			 fmt_version(client_version),
			 id, fmt_cipher_suites(cipher_suites),
                         compression_methods);

	}
	  || (cipher_len & 1))
	{
	  send_packet(alert(ALERT_fatal, ALERT_unexpected_message,
			    "Invalid client_hello.\n"));
	  return -1;
	}
	if (((client_version & ~0xff) != PROTOCOL_SSL_3_0) ||
	    (client_version < context->min_version)) {
	  SSL3_DEBUG_MSG("Unsupported version of SSL: %s.\n",
			 fmt_version(client_version));
	  send_packet(alert(ALERT_fatal, ALERT_protocol_version,
			    "Unsupported version.\n"));
	  return -1;
	}
	if (client_version > version) {
	  SSL3_DEBUG_MSG("Falling back client from %s to %s.\n",
			 fmt_version(client_version),
			 fmt_version(version));
	} else if (version > client_version) {
	  SSL3_DEBUG_MSG("Falling back server from %s to %s.\n",
			 fmt_version(version),
			 fmt_version(client_version));
	  version = client_version;
	}

	Stdio.Buffer extensions;
	if (sizeof(input))
	  extensions = input->read_hbuffer(2);

#ifdef SSL3_DEBUG
	if (sizeof(input))
	  werror("SSL.ServerConnection->handle_handshake: "
		 "extra data in hello message ignored\n");
#endif

	session = context->new_session();

	int missing_secure_renegotiation = secure_renegotiation;
	if (extensions) {
	  int maybe_safari_10_8 = 1;
	  while (sizeof(extensions)) {
	    int extension_type = extensions->read_int(2);
	    string(8bit) raw = extensions->read_hstring(2);
	    Buffer extension_data = Buffer(raw);
	    SSL3_DEBUG_MSG("SSL.ServerConnection->handle_handshake: "
			   "Got extension %s.\n",
			   fmt_constant(extension_type, "EXTENSION"));
            if( remote_extensions[extension_type] )
            {
              send_packet(alert(ALERT_fatal, ALERT_decode_error,
                                "Same extension sent twice.\n"));
              return -1;
            }
            else
              remote_extensions[extension_type] = 1;

          extensions:
	    switch(extension_type) {
	    case EXTENSION_signature_algorithms:
	      if (!remote_extensions[EXTENSION_ec_point_formats] ||
		  (raw != "\0\12\5\1\4\1\2\1\4\3\2\3") ||
		  (client_version != PROTOCOL_TLS_1_2)) {
		maybe_safari_10_8 = 0;
	      }
	      // RFC 5246
	      string bytes = extension_data->read_hstring(2);
	      // Pairs of <hash_alg, signature_alg>.
	      session->signature_algorithms = ((array(int))bytes)/2;
	      SSL3_DEBUG_MSG("New signature_algorithms:\n"+
			     fmt_signature_pairs(session->signature_algorithms));
	      break;
	    case EXTENSION_elliptic_curves:
	      if (!remote_extensions[EXTENSION_server_name] ||
		  (raw != "\0\6\0\x17\0\x18\0\x19")) {
		maybe_safari_10_8 = 0;
	      }
	      session->ecc_curves =
		filter(reverse(sort(extension_data->read_int_array(2, 2))),
		       ECC_CURVES);
	      SSL3_DEBUG_MSG("Elliptic curves: %O\n",
			     map(session->ecc_curves, fmt_constant, "CURVE"));
	      break;
	    case EXTENSION_ec_point_formats:
	      if (!remote_extensions[EXTENSION_elliptic_curves] ||
		  (raw != "\1\0")) {
		maybe_safari_10_8 = 0;
	      }
	      array(int) ecc_point_formats =
                extension_data->read_int_array(1, 1);
	      // NB: We only support the uncompressed point format for now.
	      if (has_value(ecc_point_formats, POINT_uncompressed)) {
		session->ecc_point_format = POINT_uncompressed;
	      } else {
		// Not a supported point format.
		session->ecc_point_format = -1;
	      }
	      SSL3_DEBUG_MSG("Elliptic point format: %O\n",
			     session->ecc_point_format);
	      break;
	    case EXTENSION_server_name:
	      if (sizeof(remote_extensions) != 1) {
		maybe_safari_10_8 = 0;
	      }
	      // RFC 6066 3.1 "Server Name Indication"
	      session->server_name = 0;
	      while (sizeof(extension_data)) {
		Stdio.Buffer server_name = extension_data->read_hbuffer(2);
		switch(server_name->read_int(1)) {	// name_type
		case 0:	// host_name
                  if( session->server_name )
                  {
                    send_packet(alert(ALERT_fatal, ALERT_unrecognized_name,
                                      "Multiple names given.\n"));
                    return -1;
                  }
		  session->server_name = server_name->read_hstring(2);
		  break;
		default:
		  // No other NameTypes defined yet.
		  break;
		}
	      }
              SSL3_DEBUG_MSG("SNI extension: %O\n", session->server_name);
	      break;
	    case EXTENSION_max_fragment_length:
	      // RFC 3546 3.2 "Maximum Fragment Length Negotiation"
	      int mfsz = sizeof(extension_data) &&
		extension_data->read_int(1);
	      if (sizeof(extension_data)) mfsz = 0;
	      switch(mfsz) {
	      case FRAGMENT_512:  session->max_packet_size = 512; break;
	      case FRAGMENT_1024: session->max_packet_size = 1024; break;
	      case FRAGMENT_2048: session->max_packet_size = 2048; break;
	      case FRAGMENT_4096: session->max_packet_size = 4096; break;
	      default:
		send_packet(alert(ALERT_fatal, ALERT_illegal_parameter,
				  "Invalid fragment size.\n"));
		return -1;
	      }
              SSL3_DEBUG_MSG("Maximum frame size %O.\n", session->max_packet_size);
	      break;
	    case EXTENSION_truncated_hmac:
	      // RFC 3546 3.5 "Truncated HMAC"
	      if (sizeof(extension_data)) {
		send_packet(alert(ALERT_fatal, ALERT_illegal_parameter,
				  "Invalid trusted HMAC extension.\n"));
                return -1;
	      }
	      session->truncated_hmac = 1;
              SSL3_DEBUG_MSG("Trucated HMAC\n");
	      break;

	    case EXTENSION_renegotiation_info:
	      string renegotiated_connection =
		extension_data->read_hstring(1);
	      if ((renegotiated_connection != client_verify_data) ||
		  (!(state & CONNECTION_handshaking) && !secure_renegotiation)) {
		// RFC 5746 3.7: (secure_renegotiation)
		// The server MUST verify that the value of the
		// "renegotiated_connection" field is equal to the saved
		// client_verify_data value; if it is not, the server MUST
		// abort the handshake.
		//
		// RFC 5746 4.4: (!secure_renegotiation)
		// The server MUST verify that the "renegotiation_info"
		// extension is not present; if it is, the server MUST
		// abort the handshake.
		send_packet(alert(ALERT_fatal, ALERT_handshake_failure,
				  "Invalid renegotiation data.\n"));
		return -1;
	      }
	      secure_renegotiation = 1;
	      missing_secure_renegotiation = 0;
              SSL3_DEBUG_MSG("Renego extension: %O\n", renegotiated_connection);
	      break;

            case EXTENSION_application_layer_protocol_negotiation:
              {
                has_application_layer_protocol_negotiation = 1;
                if( !context->advertised_protocols )
                  break;
                multiset(string) protocols = (<>);
                if( extension_data->read_int(2) != sizeof(extension_data) )
                {
                  send_packet(alert(ALERT_fatal, ALERT_handshake_failure,
				    "ALPN: Length mismatch.\n"));
                  return -1;
                }
                while (sizeof(extension_data)) {
                  string server_name = extension_data->read_hstring(1);
                  if( sizeof(server_name)==0 )
                  {
                    send_packet(alert(ALERT_fatal, ALERT_handshake_failure,
				      "ALPN: Empty protocol.\n"));
                    return -1;
                  }
                  protocols[ server_name ] = 1;
                }

                if( !sizeof(protocols) )
                {
                  // FIXME: What does an empty list mean? Ignore, no
                  // protocol failure or handshake failure? Currently
                  // it will hit the no compatible protocol fatal
                  // alert below.
                }

                // Although the protocol list is sent in client
                // preference order, it is the server preference that
                // wins.
                application_protocol = 0;
                foreach(context->advertised_protocols;; string(8bit) prot)
                  if( protocols[prot] )
                  {
                    application_protocol = prot;
                    break;
                  }
                if( !application_protocol )
                {
                  send_packet(alert(ALERT_fatal, ALERT_no_application_protocol,
				    "ALPN: No compatible protocol.\n"));
                  return -1;
                }
                SSL3_DEBUG_MSG("ALPN extension: %O %O\n",
			       protocols, application_protocol);
              }
              break;

	    case EXTENSION_heartbeat:
	      {
		int hb_mode;
		if (!sizeof(extension_data) ||
		    !(hb_mode = extension_data->read_int(1)) ||
		    sizeof(extension_data) ||
		    ((hb_mode != HEARTBEAT_MODE_peer_allowed_to_send) &&
		     (hb_mode != HEARTBEAT_MODE_peer_not_allowed_to_send))) {
		  // RFC 6520 2:
		  // Upon reception of an unknown mode, an error Alert
		  // message using illegal_parameter as its
		  // AlertDescription MUST be sent in response.
		  send_packet(alert(ALERT_fatal, ALERT_illegal_parameter,
				    "Heartbeat: Invalid extension.\n"));
                  return -1;
		}
		SSL3_DEBUG_MSG("heartbeat extension: %s\n",
			       fmt_constant(hb_mode, "HEARTBEAT_MODE"));
		session->heartbeat_mode = [int(0..1)]hb_mode;
	      }
	      break;

	    case EXTENSION_encrypt_then_mac:
	      {
		if (sizeof(extension_data)) {
		  send_packet(alert(ALERT_fatal, ALERT_illegal_parameter,
				    "Encrypt-then-MAC: Invalid extension.\n"));
                  return -1;
		}
		if (context->encrypt_then_mac) {
		  SSL3_DEBUG_MSG("Encrypt-then-MAC: Tentatively enabled.\n");
		  session->encrypt_then_mac = 1;
		} else {
		  SSL3_DEBUG_MSG("Encrypt-then-MAC: Rejected.\n");
		}
	      }
	      break;

            case EXTENSION_padding:
              if( !equal(String.range((string)extension_data), ({0,0})) )
              {
                send_packet(alert(ALERT_fatal, ALERT_illegal_parameter,
                                  "Possible covert side channel in padding.\n"
                                  ));
                return -1;
              }
              break;

	    default:
              SSL3_DEBUG_MSG("Unhandled extension %O (%d bytes)\n",
                             (string)extension_data,
                             sizeof(extension_data));
	      break;
	    }
	  }
	  if (maybe_safari_10_8) {
	    // According to OpenSSL (ssl/t1_lib.c:ssl_check_for_safari()),
	    // the Safari browser versions 10.8.0..10.8.3 have
	    // broken support for ECDHE_ECDSA, but advertise such
	    // suites anyway. We attempt to fingerprint Safari 10.8
	    // above by the set of extensions and the order it
	    // sends them in.
	    SSL3_DEBUG_MSG("Client version: %s\n"
			   "Number of extensions: %d\n",
			   fmt_version(client_version),
			   sizeof(remote_extensions));
	    if (((client_version == PROTOCOL_TLS_1_2) &&
		 ((sizeof(remote_extensions) != 4) ||
		  !remote_extensions[EXTENSION_signature_algorithms])) ||
		((client_version < PROTOCOL_TLS_1_2) &&
		 ((sizeof(remote_extensions) != 3) ||
		  !remote_extensions[EXTENSION_ec_point_formats]))) {
	      maybe_safari_10_8 = 0;
	    }
	    if (maybe_safari_10_8) {
	      SSL3_DEBUG_MSG("Safari 10.8 (or similar) detected.\n");
	      cipher_suites = filter(cipher_suites,
				     lambda(int suite) {
				       return CIPHER_SUITES[suite] &&
					 (CIPHER_SUITES[suite][0] !=
					  KE_ecdhe_ecdsa);
				     });
	      SSL3_DEBUG_MSG("Remaining cipher suites:\n"
			     "%s\n",
			     fmt_cipher_suites(cipher_suites));
	    }
	  }
	}
	if (missing_secure_renegotiation) {
	  // RFC 5746 3.7: (secure_renegotiation)
	  // The server MUST verify that the "renegotiation_info" extension is
	  // present; if it is not, the server MUST abort the handshake.
	  send_packet(alert(ALERT_fatal, ALERT_handshake_failure,
			    "Missing secure renegotiation extension.\n"));
	  return -1;
	}
	if (has_value(cipher_suites, TLS_empty_renegotiation_info_scsv)) {
	  if (secure_renegotiation || !(state & CONNECTION_handshaking)) {
	    // RFC 5746 3.7: (secure_renegotiation)
	    // When a ClientHello is received, the server MUST verify that it
	    // does not contain the TLS_EMPTY_RENEGOTIATION_INFO_SCSV SCSV.  If
	    // the SCSV is present, the server MUST abort the handshake.
	    //
	    // RFC 5746 4.4: (!secure_renegotiation)
	    // When a ClientHello is received, the server MUST verify
	    // that it does not contain the
	    // TLS_EMPTY_RENEGOTIATION_INFO_SCSV SCSV.  If the SCSV is
	    // present, the server MUST abort the handshake.
	    send_packet(alert(ALERT_fatal, ALERT_handshake_failure,
			      "SCSV is present.\n"));
	    return -1;
	  } else {
	    // RFC 5746 3.6:
	    // When a ClientHello is received, the server MUST check if it
	    // includes the TLS_EMPTY_RENEGOTIATION_INFO_SCSV SCSV.  If it
	    // does, set the secure_renegotiation flag to TRUE.
	    secure_renegotiation = 1;
	  }
	}
	if (has_value(cipher_suites, TLS_fallback_scsv)) {
	  // draft-ietf-tls-downgrade-scsv 3:
	  // If TLS_FALLBACK_SCSV appears in ClientHello.cipher_suites and the
	  // highest protocol version supported by the server is higher than
	  // the version indicated in ClientHello.client_version, the server
	  // MUST respond with an inappropriate_fallback alert.
	  if (client_version < context->max_version) {
	    send_packet(alert(ALERT_fatal, ALERT_inappropriate_fallback,
			      "Too low client version.\n"));
	    return -1;
	  }
	}

#ifdef SSL3_DEBUG
	if (sizeof(input))
	  werror("SSL.ServerConnection->handle_handshake: "
		 "extra data in hello message ignored\n");
#endif

	SSL3_DEBUG_MSG("ciphers: me:\n%s, client:\n%s",
		       fmt_cipher_suites(context->preferred_suites),
		       fmt_cipher_suites(cipher_suites));
	cipher_suites = context->preferred_suites & cipher_suites;
	SSL3_DEBUG_MSG("intersection:\n%s\n",
		       fmt_cipher_suites((array(int))cipher_suites));

	if (!sizeof(session->ecc_curves) || (session->ecc_point_format == -1)) {
	  // No overlapping support for ecc.
	  // Filter the ECC suites from the set.
	  SSL3_DEBUG_MSG("ECC not supported.\n");
	  cipher_suites = filter(cipher_suites, not_ecc_suite);
	}

	if (sizeof(cipher_suites)) {
	  array(CertificatePair) certs =
	    context->find_cert_domain(session->server_name);

	  ProtocolVersion orig_version = version;

	  while (!session->select_cipher_suite(certs, cipher_suites, version)) {
	    if (version > context->min_version) {
	      // Try falling back to an older version of SSL/TLS.
	      version--;
	    } else {
#if 0
	      werror("FAIL: %s (%s, client: %s)\n"
		     "Client suites:\n"
		     "%s\n",
		     fmt_version(version), fmt_version(orig_version),
		     fmt_version(client_version),
		     fmt_cipher_suites(cipher_suites));
#endif
	      // No compatible suite.
	      version = orig_version;
	      cipher_suites = ({});
	      break;
	    }
	  }

	  if (version != orig_version) {
	    SSL3_DEBUG_MSG("Fallback from %s to %s to select suite %s.\n",
			   fmt_version(orig_version), fmt_version(version),
			   fmt_cipher_suite(session->cipher_suite));
	  }
	}

	if (!sizeof(cipher_suites)) {
	  // No overlapping cipher suites, or obsolete cipher suite selected,
	  // or incompatible certificates.
	  send_packet(alert(ALERT_fatal, ALERT_handshake_failure,
			    "No common suites!\n"));
	  return -1;
	}

        if( version >= PROTOCOL_TLS_1_3 )
          compression_methods = ({ COMPRESSION_null }) & compression_methods;
        else
          compression_methods =
            context->preferred_compressors & compression_methods;
	if (sizeof(compression_methods))
	  session->set_compression_method(compression_methods[0]);
	else
	{
	  send_packet(alert(ALERT_fatal, ALERT_handshake_failure,
			    "Unsupported compression method.\n"));
	  return -1;
	}

#ifdef SSL3_DEBUG
	if (sizeof(id))
	  werror("SSL.ServerConnection: Looking up session %O\n", id);
#endif
	Session old_session = sizeof(id) && context->lookup_session(id);
	if (old_session && old_session->reusable_as(session))
        {
	  SSL3_DEBUG_MSG("SSL.ServerConnection: Reusing session %O\n", id);

	  /* Reuse session */
	  session = old_session;
	  send_packet(server_hello_packet());

	  array(State) res;
	  mixed err;
          if( err = catch(res = session->new_server_states(this,
							   client_random,
							   server_random,
							   version)) )
          {
            // DES/DES3 throws an exception if a weak key is used. We
            // could possibly send ALERT_insufficient_security instead.
            send_packet(alert(ALERT_fatal, ALERT_internal_error,
			      "Internal error.\n"));
            return -1;
          }
	  pending_read_state = res[0];
	  pending_write_state = res[1];
	  send_packet(change_cipher_packet());
	  if(version == PROTOCOL_SSL_3_0)
	    send_packet(finished_packet("SRVR"));
	  else if(version >= PROTOCOL_TLS_1_0)
	    send_packet(finished_packet("server finished"));

	  // NB: The client direction hash will be calculated
	  //     when we've received the client finished packet.

	  if (context->heartbleed_probe &&
              session->heartbeat_mode == HEARTBEAT_MODE_peer_allowed_to_send) {
	    // Probe for the Heartbleed vulnerability (CVE-2014-0160).
	    send_packet(heartbleed_packet());
	  }

	  expect_change_cipher = 1;
	  reuse = 1;

	  handshake_state = STATE_wait_for_finish;
	} else {
	  /* New session, do full handshake. */

	  int(-1..0) err = reply_new_session(cipher_suites,
					     compression_methods);
	  if (err)
	    return err;
	  handshake_state = STATE_wait_for_peer;
	}
	break;
      }
     }
     break;
   }
  case STATE_wait_for_finish:
    switch(type)
    {
    default:
      send_packet(alert(ALERT_fatal, ALERT_unexpected_message,
			"Expected finished.\n"));
      return -1;
    case HANDSHAKE_finished:
     {
       string(8bit) my_digest;
       string(8bit) digest;

       SSL3_DEBUG_MSG("SSL.ServerConnection: FINISHED\n");

       if(version == PROTOCOL_SSL_3_0) {
	 my_digest=hash_messages("CLNT");
	 if (catch {
	   digest = input->read(36);
           } || sizeof(input))
	   {
	     send_packet(alert(ALERT_fatal, ALERT_unexpected_message,
			       "Invalid handshake finished message.\n"));
	     return -1;
	   }
       } else if(version >= PROTOCOL_TLS_1_0) {
	 my_digest=hash_messages("client finished");
	 if (catch {
	   digest = input->read(12);
           } || sizeof(input))
	   {
	     send_packet(alert(ALERT_fatal, ALERT_unexpected_message,
			       "Invalid handshake finished message.\n"));
	     return -1;
	   }


       }

       if ((ke && ke->message_was_bad)	/* Error delayed until now */
	   || (my_digest != digest))
       {
	 if(ke && ke->message_was_bad)
	   SSL3_DEBUG_MSG("message_was_bad\n");
	 if(my_digest != digest)
	   SSL3_DEBUG_MSG("digests differ\n");
	 send_packet(alert(ALERT_fatal, ALERT_unexpected_message,
			   "Key exchange failure.\n"));
	 return -1;
       }
       handshake_messages += raw; /* Second hash includes this message,
				   * the first doesn't */
       /* Handshake complete */

       client_verify_data = digest;

       if (!reuse)
       {
	 send_packet(change_cipher_packet());
	 if(version == PROTOCOL_SSL_3_0)
	   send_packet(finished_packet("SRVR"));
	 else if(version >= PROTOCOL_TLS_1_0)
	   send_packet(finished_packet("server finished"));

	 if (context->heartbleed_probe &&
             session->heartbeat_mode == HEARTBEAT_MODE_peer_allowed_to_send) {
	   // Probe for the Heartbleed vulnerability (CVE-2014-0160).
	   send_packet(heartbleed_packet());
	 }

	 expect_change_cipher = 1;
	 context->record_session(session); /* Cache this session */
       }

       // Handshake hash is calculated for both directions above.
       handshake_messages = 0;

       handshake_state = STATE_wait_for_hello;

       return 1;
     }
    }
    break;
  case STATE_wait_for_peer:
    // NB: ALERT_no_certificate can be valid in this state, and
    //     is handled directly by connection:handle_alert().
    handshake_messages += raw;
    switch(type)
    {
    default:
      send_packet(alert(ALERT_fatal, ALERT_unexpected_message,
			"Expected client KEX or cert.\n"));
      return -1;
    case HANDSHAKE_client_key_exchange:
      SSL3_DEBUG_MSG("SSL.ServerConnection: CLIENT_KEY_EXCHANGE\n");

      if (certificate_state == CERT_requested)
      { /* Certificate must be sent before key exchange message */
	send_packet(alert(ALERT_fatal, ALERT_unexpected_message,
			  "Expected client cert.\n"));
	return -1;
      }

      if (!(session->master_secret = server_derive_master_secret(data)))
      {
	return -1;
      } else {

	array(State) res =
	  session->new_server_states(this, client_random, server_random,
				     version);
	pending_read_state = res[0];
	pending_write_state = res[1];

        SSL3_DEBUG_MSG("certificate_state: %d\n", certificate_state);
      }
      // TODO: we need to determine whether the certificate has signing abilities.
      if (certificate_state == CERT_received)
      {
	handshake_state = STATE_wait_for_verify;
      }
      else
      {
	handshake_state = STATE_wait_for_finish;
	expect_change_cipher = 1;
      }

      break;
    case HANDSHAKE_certificate:
     {
       SSL3_DEBUG_MSG("SSL.ServerConnection: CLIENT_CERTIFICATE\n");

       if (certificate_state != CERT_requested)
       {
	 send_packet(alert(ALERT_fatal, ALERT_unexpected_message,
			   "Unexpected client cert.\n"));
	 return -1;
       }
       mixed e;
       if (e = catch {
	 int certs_len = input->read_int(3);
         SSL3_DEBUG_MSG("got %d certificate bytes\n", certs_len);

	 array(string(8bit)) certs = ({ });
	 while(sizeof(input))
	   certs += ({ input->read_hstring(3) });

	  // we have the certificate chain in hand, now we must verify them.
          if((!sizeof(certs) && context->auth_level == AUTHLEVEL_require) ||
             certs_len != [int]Array.sum(map(certs, sizeof)) ||
             !verify_certificate_chain(certs))
          {
	    send_packet(alert(ALERT_fatal, ALERT_bad_certificate,
			      "Bad client certificate.\n"));
	    return -1;
          }
          else
          {
	    session->peer_certificate_chain = certs;
          }
         } || sizeof(input))
       {
	 send_packet(alert(ALERT_fatal, ALERT_unexpected_message,
			   "Unexpected client cert.\n"));
	 return -1;
       }

       if(session->peer_certificate_chain &&
	  sizeof(session->peer_certificate_chain))
          certificate_state = CERT_received;
       else certificate_state = CERT_no_certificate;
       break;
     }
    }
    break;
  case STATE_wait_for_verify:
    // compute challenge first, then update handshake_messages /Sigge
    switch(type)
    {
    default:
      send_packet(alert(ALERT_fatal, ALERT_unexpected_message,
			"Expected cert verify.\n"));
      return -1;
    case HANDSHAKE_certificate_verify:
      SSL3_DEBUG_MSG("SSL.ServerConnection: CERTIFICATE_VERIFY\n");

      if (!ke->message_was_bad)
      {
	int(0..1) verification_ok;
	mixed err = catch {
	    verification_ok = session->cipher_spec->verify(
              session, "", Buffer(handshake_messages), input);
	  };
#ifdef SSL3_DEBUG
	if (err) {
	  master()->handle_error(err);
	}
#endif
	err = UNDEFINED;	// Get rid of warning.
	if (!verification_ok)
	{
	  send_packet(alert(ALERT_fatal, ALERT_unexpected_message,
			    "Verification of CertificateVerify failed.\n"));
	  return -1;
	}
      }
      handshake_messages += raw;
      handshake_state = STATE_wait_for_finish;
      expect_change_cipher = 1;
      break;
    }
    break;
  }
  //  SSL3_DEBUG_MSG("SSL.ServerConnection: messages = %O\n", handshake_messages);
  return 0;
}
