#pike __REAL_VERSION__
#pragma strict_types
#require constant(SSL.Cipher)

//! Server-side connection state.

#include "tls.h"

import ".";
import Constants;
inherit Connection;

multiset(int) remote_extensions = (<>);

protected string _sprintf(int t)
{
  if (t == 'O') return sprintf("SSL.ServerConnection(%s)", describe_state());
}

protected Packet hello_request()
{
  return handshake_packet(HANDSHAKE_hello_request, "");
}

protected Packet finished_packet(string(8bit) sender)
{
  SSL3_DEBUG_MSG("Sending finished_packet, with sender=\""+sender+"\"\n" );
  // We're the server.
  server_verify_data = hash_messages(sender);
  return handshake_packet(HANDSHAKE_finished, server_verify_data);
}

protected Packet server_hello_packet()
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
    if(context->extensions[id] && condition)
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

  ext (EXTENSION_extended_master_secret, session->extended_master_secret) {
    // draft-ietf-tls-session-hash
    return Buffer();
  };

  ext (EXTENSION_application_layer_protocol_negotiation, !!application_protocol)
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

protected Packet server_hello_retry_request_packet(int suite, int group)
{
  Buffer struct = Buffer();
  struct->add_int(version, 2);
  struct->add_int(suite, 2);
  struct->add_int(group, 2);

  // TLS 1.3 draft 3 7.4.2.4:
  //   The server SHOULD send only the extensions necessary for the
  //   client to generate a correct ClientHello/ClientKeyShare pair.
  struct->add_hstring("", 2);	// extensions.

  return handshake_packet(HANDSHAKE_hello_retry_request, struct);
}

protected Packet server_key_share_packet(Stdio.Buffer offer)
{
  return handshake_packet(HANDSHAKE_server_key_share, offer);
}

//! Initialize the KeyExchange @[ke], and generate a
//! @[HANDSHAKE_server_key_exchange] packet if the
//! key exchange needs one.
protected Packet server_key_exchange_packet()
{
  if (ke) error("KE!\n");
  ke = session->cipher_spec->ke_factory(context, session, this, client_version);
  if (!ke->init_server()) return 0;
  string(8bit) data =
    ke->server_key_exchange_packet(client_random, server_random);
  return data && handshake_packet(HANDSHAKE_server_key_exchange, data);
}

protected Packet certificate_request_packet(Context context)
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
  if (!context->enable_renegotiation) {
    error("Renegotiation disabled in context.\n");
  }
  send_packet(hello_request(), PRI_application);
}

protected int(-1..0) reply_new_session(array(int) cipher_suites,
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

//! Derive the new master secret from the state of @[ke] and
//! the payload @[data] received fron the client in its
//! @[HANDSHAKE_client_key_exchange] packet.
protected int(0..1) server_derive_master_secret(Buffer data)
{
  string(8bit)|int(8bit) premaster_secret =
    ke->got_client_key_exchange(data, version);
  ke = UNDEFINED;

  if (intp(premaster_secret)) {
    send_packet(alert(ALERT_fatal, [int(8bit)]premaster_secret,
		      "Failed to derive master secret.\n"));
    return 0;
  }

  derive_master_secret([string(8bit)]premaster_secret);
  return 1;
}

protected void create(Context ctx)
{
  ::create(ctx);
  session = context->new_session();
  handshake_state = STATE_wait_for_hello;
}

void new_cipher_states()
{
  SSL3_DEBUG_MSG("master: %O\n", session->master_secret);

  array(State) res =
    session->new_server_states(this, client_random, server_random,
			       version);
  pending_read_state += ({ res[0] });
  pending_write_state += ({ res[1] });
}


//! Do handshake processing. Type is one of HANDSHAKE_*, data is the
//! contents of the packet, and raw is the raw packet received (needed
//! for supporting SSLv2 hello messages).
//!
//! This function returns 0 if handshake is in progress, 1 if handshake
//! is finished, and -1 if a fatal error occurred. It uses the
//! send_packet() function to transmit packets.
int(-1..1) handle_handshake(int type, Buffer input, Stdio.Buffer raw)
{
#ifdef SSL3_PROFILING
  addRecord(type,0);
#endif
#ifdef SSL3_DEBUG_HANDSHAKE_STATE
  werror("SSL.ServerConnection: state %s, type %s\n",
	 fmt_constant(handshake_state, "STATE"),
         fmt_constant(type, "HANDSHAKE"));
  werror("sizeof(data)="+sizeof(data)+"\n");
#endif

  // Enforce packet ordering.
  COND_FATAL((type <= previous_handshake) &&
      // NB: certificate_verify <=> client_key_exchange are out of order!
      ((type != HANDSHAKE_certificate_verify) ||
       (previous_handshake != HANDSHAKE_client_key_exchange)),
             ALERT_unexpected_message,
             "Invalid handshake packet order.\n");

  previous_handshake = type;

  switch(handshake_state)
  {
  default:
    error( "Internal error\n" );
  case STATE_wait_for_hello:
    {
      /* Reset all extra state variables */
      expect_change_cipher = certificate_state = 0;
      pending_read_state = pending_write_state = ({});
      ke = 0;

      add_handshake_message(raw);

      // The first four bytes of the client_random is specified to be
      // the timestamp on the client side. This is to guard against
      // bad random generators, where a client could produce the same
      // random numbers if the seed is reused. This argument is
      // flawed, since a broken random generator will make the
      // connection insecure anyways. The standard explicitly allows
      // these bytes to not be correct, so sending random data instead
      // is safer and reduces client fingerprinting.
      server_random = context->random(32);

      if (type != HANDSHAKE_client_hello)
        COND_FATAL(1, ALERT_unexpected_message, "Expected client_hello.\n");

      string session_id;
      int cipher_len;
      array(int) cipher_suites;
      array(int) compression_methods;

      SSL3_DEBUG_MSG("SSL.ServerConnection: CLIENT_HELLO\n");

      client_version =
        [int(0x300..0x300)|ProtocolVersion]input->read_int(2);
      COND_FATAL(((client_version & ~0xff) != PROTOCOL_SSL_3_0) ||
                 (client_version < context->min_version),
                 ALERT_protocol_version,
                 sprintf("Unsupported version %s.\n",
                         fmt_version(client_version)));

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

      client_random = input->read(32);
      session_id = input->read_hstring(1);

      cipher_len = input->read_int(2);
      COND_FATAL(cipher_len & 1, ALERT_unexpected_message,
                 "Invalid client_hello.\n");

      cipher_suites = input->read_ints(cipher_len/2, 2);

      compression_methods = input->read_int_array(1, 1);
      SSL3_DEBUG_MSG("STATE_wait_for_hello: received hello\n"
                     "version = %s\n"
                     "session_id=%O\n"
                     "cipher suites:\n%s\n"
                     "compression methods: %O\n",
                     fmt_version(client_version),
                     session_id, fmt_cipher_suites(cipher_suites),
                     compression_methods);

      Stdio.Buffer extensions;
      if (sizeof(input))
        extensions = input->read_hbuffer(2);

      Stdio.Buffer early_data = Stdio.Buffer();
      int missing_secure_renegotiation = secure_renegotiation;
      if (extensions)
      {
        // FIXME: Control Safari workaround detection from the
        // context.
        int maybe_safari_10_8 = 1;

        while (sizeof(extensions)) {
          int extension_type = extensions->read_int(2);
          string(8bit) raw = extensions->read_hstring(2);
          Buffer extension_data = Buffer(raw);
          SSL3_DEBUG_MSG("SSL.ServerConnection->handle_handshake: "
                         "Got extension %s.\n",
                         fmt_constant(extension_type, "EXTENSION"));
          COND_FATAL(remote_extensions[extension_type],
                     ALERT_decode_error,
                     "Same extension sent twice.\n");

          remote_extensions[extension_type] = 1;

          if (maybe_safari_10_8)
          {
            switch( sizeof(remote_extensions) )
            {
            case 1: // First extension is always SNI
              if( extension_type != EXTENSION_server_name )
                maybe_safari_10_8 = 0;
              break;
            case 2: // followed by signature algorithms
              if( extension_type != EXTENSION_signature_algorithms ||
                  raw != "\0\12\5\1\4\1\2\1\4\3\2\3" )
                maybe_safari_10_8 = 0;
              break;
            case 3: // TLS 1.2 also have elliptic curves
              if( client_version != PROTOCOL_TLS_1_2 ||
                  extension_type != EXTENSION_elliptic_curves ||
                  raw != "\0\6\0\x17\0\x18\0\x19" )
                maybe_safari_10_8 = 0;
              break;
            case 4: // followed by ec point formats
              if( extension_type != EXTENSION_ec_point_formats ||
                  raw != "\1\0" )
                maybe_safari_10_8 = 0;
                break;
            default: // Any more extensions and it's not Safari.
              maybe_safari_10_8 = 0;
              break;
            }
          }

          if( !context->extensions[extension_type] )
          {
            SSL3_DEBUG_MSG("Ignored extension %O (%d bytes)\n",
                           extension_type, sizeof(extension_data));
            continue;
          }

          switch(extension_type) {
          case EXTENSION_signature_algorithms:
            // RFC 5246
            string bytes = extension_data->read_hstring(2);
            COND_FATAL( sizeof(bytes)&1, ALERT_handshake_failure,
                        "Corrupt signature algorithms.\n" );
            // Pairs of <hash_alg, signature_alg>.
            session->signature_algorithms = ((array(int))bytes)/2;
            SSL3_DEBUG_MSG("New signature_algorithms:\n"+
                           fmt_signature_pairs(session->signature_algorithms));
            break;
          case EXTENSION_elliptic_curves:
            // FIXME: We are sorting the named groups in reverse order
            // of enumeration under the assumption that the strongest
            // one is defined last, and that we want to select the
            // strongest one. This should be controlled from the
            // context.
            array(int) named_groups =
              reverse(sort(extension_data->read_int_array(2, 2)));
            session->ecc_curves = filter(named_groups, ECC_CURVES);
            SSL3_DEBUG_MSG("Elliptic and finite field groups: %O\n",
                           map(session->ecc_curves, fmt_constant, "GROUP"));
            session->ffdhe_groups = context->ffdhe_groups &
              filter(named_groups,
                     FFDHE_GROUPS | context->private_ffdhe_groups);
            if (!sizeof(session->ffdhe_groups) &&
                (version <= PROTOCOL_TLS_1_2)) {
              // FFDHE group extension not supported by the client, or
              // client wants a group we don't support. Select our
              // configured set of groups to either assume that the
              // client doesn't support the extension, or to pretend
              // that we don't support the extension.
              //
              // NB: This behaviour violates the FFDHE draft, but
              //     leaves it to the client to determine whether it
              //     wants to accept the group.
              //
              // NB: In TLS 1.3 support for this extension is
              //     mandatory.
              session->ffdhe_groups = context->ffdhe_groups;
            }
            break;
          case EXTENSION_ec_point_formats:
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
            // RFC 6066 3.1 "Server Name Indication"
            session->server_name = 0;
            while (sizeof(extension_data)) {
              Stdio.Buffer server_name = extension_data->read_hbuffer(2);
              switch(server_name->read_int(1)) {	// name_type
              case 0:	// host_name
                COND_FATAL(session->server_name,
                           ALERT_unrecognized_name,
                           "Multiple names given.\n");

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
            int mfsz = extension_data->read_int(1);
            if (sizeof(extension_data)) mfsz = 0;
            switch(mfsz) {
            case FRAGMENT_512:  session->max_packet_size = 512; break;
            case FRAGMENT_1024: session->max_packet_size = 1024; break;
            case FRAGMENT_2048: session->max_packet_size = 2048; break;
            case FRAGMENT_4096: session->max_packet_size = 4096; break;
            default:
              COND_FATAL(1, ALERT_illegal_parameter,
                         "Invalid fragment size.\n");
            }
            SSL3_DEBUG_MSG("Maximum frame size %O.\n", session->max_packet_size);
            break;

          case EXTENSION_truncated_hmac:
            // RFC 3546 3.5 "Truncated HMAC"
            COND_FATAL(sizeof(extension_data),
                       ALERT_illegal_parameter,
                       "Invalid trusted HMAC extension.\n");

            session->truncated_hmac = 1;
            SSL3_DEBUG_MSG("Trucated HMAC\n");
            break;

          case EXTENSION_renegotiation_info:
            string renegotiated_connection =
              extension_data->read_hstring(1);

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
            COND_FATAL((renegotiated_connection != client_verify_data) ||
                       (!(state & CONNECTION_handshaking) &&
                        !secure_renegotiation),
                       ALERT_handshake_failure,
                       "Invalid renegotiation data.\n");

            secure_renegotiation = 1;
            missing_secure_renegotiation = 0;
            SSL3_DEBUG_MSG("Renego extension: %O\n", renegotiated_connection);
            break;

          case EXTENSION_next_protocol_negotiation:
            COND_FATAL(sizeof(extension_data), ALERT_handshake_failure,
                       "Invalid NPN extension.\n");
            break;

          case EXTENSION_signed_certificate_timestamp:
            COND_FATAL(sizeof(extension_data), ALERT_handshake_failure,
                       "Invalid signed certificate timestamp extension.\n");
            break;

          case EXTENSION_application_layer_protocol_negotiation:
            {
              application_protocol = 0;
              if( !context->advertised_protocols )
                break;
              multiset(string) protocols = (<>);
              COND_FATAL(extension_data->read_int(2) != sizeof(extension_data),
                         ALERT_handshake_failure,
                         "ALPN: Length mismatch.\n");

              while (sizeof(extension_data)) {
                string protocol_name = extension_data->read_hstring(1);
                COND_FATAL(sizeof(protocol_name)<2, ALERT_handshake_failure,
                           "ALPN: Protocol name too short.\n");

                protocols[ protocol_name ] = 1;
              }

              COND_FATAL(!sizeof(protocols), ALERT_handshake_failure,
                         "ALPN: Empty list of protocols.\n");

              // Although the protocol list is sent in client
              // preference order, it is the server preference that
              // wins.
              foreach(context->advertised_protocols;; string(8bit) prot)
                if( protocols[prot] )
                {
                  application_protocol = prot;
                  break;
                }

              COND_FATAL(!application_protocol,
                         ALERT_no_application_protocol,
                         "ALPN: No compatible protocol.\n");

              SSL3_DEBUG_MSG("ALPN extension: %O %O\n",
                             protocols, application_protocol);
            }
            break;

          case EXTENSION_heartbeat:
            {
              int hb_mode;

              // RFC 6520 2:
              // Upon reception of an unknown mode, an error Alert
              // message using illegal_parameter as its
              // AlertDescription MUST be sent in response.
              COND_FATAL(!sizeof(extension_data) ||
                         !(hb_mode = extension_data->read_int(1)) ||
                         sizeof(extension_data) ||
                         ((hb_mode != HEARTBEAT_MODE_peer_allowed_to_send) &&
                          (hb_mode != HEARTBEAT_MODE_peer_not_allowed_to_send)),
                         ALERT_illegal_parameter,
                         "Heartbeat: Invalid extension.\n");

              SSL3_DEBUG_MSG("heartbeat extension: %s\n",
                             fmt_constant(hb_mode, "HEARTBEAT_MODE"));
              session->heartbeat_mode = [int(0..1)]hb_mode;
            }
            break;

          case EXTENSION_encrypt_then_mac:
            {
              COND_FATAL(sizeof(extension_data),
                         ALERT_illegal_parameter,
                         "Encrypt-then-MAC: Invalid extension.\n");

              SSL3_DEBUG_MSG("Encrypt-then-MAC: Tentatively enabled.\n");
              session->encrypt_then_mac = 1;
            }
            break;

          case EXTENSION_extended_master_secret:
            {
              COND_FATAL(sizeof(extension_data),
                         ALERT_illegal_parameter,
                         "Extended-master-secret: Invalid extension.\n");

              SSL3_DEBUG_MSG("Extended-master-secret: Enabled.\n");
              session->extended_master_secret = 1;
            }
            break;

          case EXTENSION_early_data:
            early_data->add(extension_data);
            break;

          case EXTENSION_padding:
            COND_FATAL(sizeof(extension_data) &&
                       !equal(String.range((string)extension_data),({0,0})),
                       ALERT_illegal_parameter,
                       "Possible covert side channel in padding.\n");
            break;

          default:
            SSL3_DEBUG_MSG("Unhandled extension %O.\n", extension_type);
            break;
          }
        }

        if (maybe_safari_10_8) {
          // According to OpenSSL (ssl/t1_lib.c:ssl_check_for_safari()),
          // the Safari browser versions 10.8.0..10.8.3 have broken
          // support for ECDHE_ECDSA, but advertise such suites
          // anyway. We attempt to fingerprint Safari 10.8 above by
          // the set of extensions and the order it sends them in.
          SSL3_DEBUG_MSG("Safari 10.8 (or similar) detected.\n");
          cipher_suites = filter(cipher_suites,
            lambda(int suite) {
              return CIPHER_SUITES[suite] &&
                (CIPHER_SUITES[suite][0] != KE_ecdhe_ecdsa);
            });
          SSL3_DEBUG_MSG("Remaining cipher suites:\n"
                         "%s\n",
                         fmt_cipher_suites(cipher_suites));
        }
      }

      // RFC 5746 3.7: (secure_renegotiation)
      // The server MUST verify that the "renegotiation_info"
      // extension is present; if it is not, the server MUST abort the
      // handshake.
      COND_FATAL(missing_secure_renegotiation,
                 ALERT_handshake_failure,
                 "Missing secure renegotiation extension.\n");

      if (has_value(cipher_suites, TLS_empty_renegotiation_info_scsv)) {

        // RFC 5746 3.7: (secure_renegotiation)
        // When a ClientHello is received, the server MUST verify that
        // it does not contain the TLS_EMPTY_RENEGOTIATION_INFO_SCSV
        // SCSV. If the SCSV is present, the server MUST abort the
        // handshake.
        //
        // RFC 5746 4.4: (!secure_renegotiation)
        // When a ClientHello is received, the server MUST verify that
        // it does not contain the TLS_EMPTY_RENEGOTIATION_INFO_SCSV
        // SCSV. If the SCSV is present, the server MUST abort the
        // handshake.
        COND_FATAL(secure_renegotiation || !(state & CONNECTION_handshaking),
                   ALERT_handshake_failure, "SCSV is present.\n");

        // RFC 5746 3.6:
        // When a ClientHello is received, the server MUST check if it
        // includes the TLS_EMPTY_RENEGOTIATION_INFO_SCSV SCSV. If it
        // does, set the secure_renegotiation flag to TRUE.
        secure_renegotiation = 1;
      }

      if (has_value(cipher_suites, TLS_fallback_scsv)) {
        // RFC 7507 3:
	// If TLS_FALLBACK_SCSV appears in ClientHello.cipher_suites
	// and the highest protocol version supported by the server is
	// higher than the version indicated in
	// ClientHello.client_version, the server MUST respond with a
	// fatal inappropriate_fallback alert (unless it responds with
	// a fatal protocol_version alert because the version
	// indicated in ClientHello.client_version is unsupported).
        COND_FATAL(client_version < context->max_version,
                   ALERT_inappropriate_fallback,
                   "Too low client version.\n");
      }

      SSL3_DEBUG_MSG("ciphers: me:\n%s, client:\n%s",
                     fmt_cipher_suites(context->preferred_suites),
                     fmt_cipher_suites(cipher_suites));
      cipher_suites = context->preferred_suites & cipher_suites;
      SSL3_DEBUG_MSG("intersection:\n%s\n",
                     fmt_cipher_suites((array(int))cipher_suites));

      if (sizeof(cipher_suites)) {
        array(CertificatePair) certs =
          context->find_cert_domain(session->server_name);

        ProtocolVersion orig_version = version;

        while (!session->select_cipher_suite(certs, cipher_suites, version))
        {
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

      // No overlapping cipher suites, or obsolete cipher suite
      // selected, or incompatible certificates.
      // FIXME: Consider ALERT_insufficient_security
      //        (cf RFC 7465 section 2).
      COND_FATAL(!sizeof(cipher_suites),
                 ALERT_handshake_failure, "No common suites!\n");

      // FIXME: Support TLS 1.3 fallback to TLS 1.2 or earlier
      //        for clients that don't use early_data?
      if( version >= PROTOCOL_TLS_1_3 ) {

        // TLS 1.3 draft 3 7.4.1:
        //
        // If a TLS 1.3 ClientHello is received with any other value
        // in this field, the server MUST generate a fatal
        // "illegal_parameter" alert.
        COND_FATAL(!equal(compression_methods, ({ COMPRESSION_null })),
                   ALERT_illegal_parameter,
                   "Illegal with compression in TLS 1.3 and later.\n");

        Session old_session = sizeof(session_id) &&
          context->lookup_session(session_id);
        if (old_session && old_session->reusable_as(session))
        {
          SSL3_DEBUG_MSG("SSL.ServerConnection: Reusing session %O\n",
                         session_id);

          /* Reuse session */
          session = old_session;
          reuse = 1;
        }

        /* TLS 1.3 or later.
         *
         * We must read the ClientKeyShare packet.
         */
        handshake_state = STATE_wait_for_key_share;

        string(8bit) handshake_buffer = "";
        while(sizeof(early_data)) {
          SSL3_DEBUG_MSG("Handling early data packet...\n");
          Packet p = Packet(version);
          int res = p->recv(early_data);
          switch(res)
          {
          case 0:
            send_packet(alert(ALERT_fatal, ALERT_record_overflow,
                              "Early data extension contains a "
                              "partial packet.\n"));
            return -1;
          case -1:
            send_packet(alert(ALERT_fatal, ALERT_unexpected_message));
            return -1;
          }
          if (p->content_type != PACKET_handshake) {
            // FIXME: This should probably be a fatal.
            SSL3_DEBUG_MSG("Ignoring non-handshake early data packet.\n");
            continue;
          }
          handshake_buffer += p->fragment;
        }

        while(sizeof(handshake_buffer) >= 4) {
          int handshake_type;
          int len;
          sscanf(handshake_buffer, "%1c%3c", handshake_type, len);
          if (sizeof(handshake_buffer) < (len + 4))
            break;
          // NB: Empty string as last argument to avoid hashing the
          //     early data packets twice.
          int(-1..1) ret =
            handle_handshake(handshake_type,
                             Buffer(handshake_buffer[4..len + 3]),
                             Buffer(""));
          handshake_buffer = handshake_buffer[len + 4..];
          if (ret) {
            if (ret < 0) return ret;
            COND_FATAL(sizeof(handshake_buffer),
                       ALERT_record_overflow,
                       "Early data extension contains extraneous "
                       "handshake packets.\n");

            return 1;
          }
        }
        COND_FATAL(sizeof(handshake_buffer),
                   ALERT_record_overflow,
                   "Early data extension contains incomplete "
                   "handshake packets.\n");

        return 0;
      } else {
        compression_methods =
          context->preferred_compressors & compression_methods;
        COND_FATAL(!sizeof(compression_methods),
                   ALERT_handshake_failure,
                   "Unsupported compression method.\n");

        session->set_compression_method(compression_methods[0]);
      }

#ifdef SSL3_DEBUG
      if (sizeof(session_id))
        werror("SSL.ServerConnection: Looking up session %O\n", session_id);
#endif
      Session old_session = sizeof(session_id) &&
        context->lookup_session(session_id);
      if (old_session && old_session->reusable_as(session))
      {
        SSL3_DEBUG_MSG("SSL.ServerConnection: Reusing session %O\n",
                       session_id);

        /* Reuse session */
        session = old_session;
        send_packet(server_hello_packet());

        new_cipher_states();
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
   }
   break;
  case STATE_wait_for_key_share:
    {
      if (client_version < PROTOCOL_TLS_1_3)
        error("Waiting for key share in %s.\n", fmt_version(version));

      add_handshake_message(raw);
      if (type != HANDSHAKE_client_key_share)
      {
        SSL3_DEBUG_MSG("Got %s packet.\n", fmt_constant(type, "HANDSHAKE"));
        COND_FATAL(1, ALERT_unexpected_message, "Expected key share.\n");
      }

      int wanted_group;
      if (session->cipher_spec->ke_factory == .Cipher.KeyShareDHE) {
        // These limits are taken from RFC 3526 8 "Strength Estimate 1".
        //
        // See also RFC 3766 5.
        switch(session->cipher_spec->key_bits) {
        case ..110:
          wanted_group = GROUP_ffdhe2048;
          break;
        case 111..130:
          wanted_group = GROUP_ffdhe3072;
          break;
        case 131..150:
          wanted_group = GROUP_ffdhe4096;
          break;
        case 151..170:
          wanted_group = GROUP_ffdhe6144;
          break;
        case 171..190:
          wanted_group = GROUP_ffdhe8192;
          break;
        case 191..:
          wanted_group = /* GROUP_ffdhe15424; */ GROUP_ffdhe8192;
          break;
        }

        if (!has_value(session->ffdhe_groups, wanted_group)) {
          int a = 0;
          int b = 0x10000;
          // Preferred group not available.
          // Select the the closest in strength available.
          foreach(session->ffdhe_groups, int g) {
            if (g > wanted_group) {
              if (g < b) b = g;
            } else if (g > a) {
              a = g;
            }
          }
          if (b != 0x10000) {
            // There's a stronger group available.
            wanted_group = b;
          } else {
            // Select the strongest available.
            wanted_group = a;
          }
        }
#if constant(Crypto.ECC.Curve)
      } else if (session->cipher_spec->ke_factory == .Cipher.KeyShareECDHE) {
        switch(session->cipher_spec->key_bits) {
        case 257..:
          wanted_group = GROUP_secp521r1;
          break;
        case 129..256:
          // Suite B requires SECP384r1
          wanted_group = GROUP_secp384r1;
          break;
        case ..128:
          wanted_group = GROUP_secp256r1;
          break;
        }

        if (!has_value(session->ecc_curves, wanted_group)) {
          int a = 0;
          int b = 0x10000;
          // Preferred curve not available.
          // Select the the closest in strength available.
          foreach(session->ecc_curves, int c) {
            if (c > wanted_group) {
              if (c < b) b = c;
            } else if (c > a) {
              a = c;
            }
          }
          if (b != 0x10000) {
            // There's a stronger curve available.
            wanted_group = b;
          } else {
            // Select the strongest available.
            wanted_group = a;
          }
        }
#endif
      } else {
        error("Unsupported KeyExchange factory for KeyShare: %O.\n",
              session->cipher_spec->ke_factory);
      }
      SSL3_DEBUG_MSG("Wanted group: %s\n",
                     fmt_constant(wanted_group, "GROUP"));
      Stdio.Buffer offers = input->read_hbuffer(2);

      mapping(int:string(8bit)) kes = ([]);
      string(8bit) premaster_secret;
      int best_group = 0x10000;
      ke = UNDEFINED;
      while (sizeof(offers)) {
        int group = offers->read_int(2);
        string(8bit) key_offer = offers->read_hstring(2);
        SSL3_DEBUG_MSG("Offer: %s: %O\n",
                       fmt_constant(group, "GROUP"),
                       key_offer);

        // Clients MUST NOT offer multiple ClientKeyShareOffers for
        // the same parameters.
        COND_FATAL(kes[group], ALERT_handshake_failure,
                   "Duplicate key share offers.\n");

        kes[group] = key_offer;
        if (((group & 0xff00) == (wanted_group & 0xff00)) &&
            (group >= wanted_group) && (group < best_group)) {
          // Select the smallest offered group that is at least as
          // large as the wanted group.
          best_group = group;
        }
      }

      if (reuse) {
        SSL3_DEBUG_MSG("Reuse the existing session.\n");

        send_packet(server_hello_packet());
        derive_master_secret(session->master_secret);
        send_packet(change_cipher_packet());
        handle_change_cipher(1);

        handshake_state = STATE_wait_for_finish;
      } else if (!kes[best_group]) {
        // Send HelloRetryRequest.
        send_packet(server_hello_retry_request_packet(session->cipher_suite,
                                                      wanted_group));
        // Reset the state to accept a new ClientHello.
        secure_renegotiation = 0;
        remote_extensions = (<>);
        previous_handshake = 0;
        handshake_state = STATE_wait_for_hello;
        break;
      } else {
        SSL3_DEBUG_MSG("Got an acceptable key share offer for %s.\n",
                       fmt_constant(best_group, "GROUP"));
        ke = session->cipher_spec->ke_factory(context, session, this,
                                              client_version);
        // ke->init_server();
        ke->set_group(best_group);
#if constant(Crypto.ECC.Curve)
        session->curve = [object(Crypto.ECC.Curve)]ke->curve;
#endif
        Stdio.Buffer sks = Stdio.Buffer();
        ke->make_key_share_offer(sks);
        premaster_secret = ke->receive_key_share_offer(kes[best_group]);
        COND_FATAL(!premaster_secret, ALERT_decode_error,
                   "Unable to decode key share offer.\n");

        send_packet(server_hello_packet());
        send_packet(server_key_share_packet(sks));
        ke = UNDEFINED;

        derive_master_secret(premaster_secret);
        send_packet(change_cipher_packet());
        handle_change_cipher(1);

        handshake_state = STATE_wait_for_finish;

        // NB: From this point encryption is enabled on our side.

        // FIXME: Encrypted extensions here.

        // Don't send any certificate in anonymous mode.
        if (session->cipher_spec->signature_alg != SIGNATURE_anonymous) {
          // NB: session->certificate_chain is set by
          // session->select_cipher_suite() above.

          SSL3_DEBUG_MSG("Checking for Certificate.\n");

          if (session->certificate_chain)
          {
            SSL3_DEBUG_MSG("Sending Certificate.\n");
            send_packet(certificate_packet(session->certificate_chain));
          } else {
            // Otherwise the server will just silently send an invalid
            // ServerHello sequence.
            error ("Certificate(s) missing.\n");
          }

          // NB: Client certificates are only supported in
          // non-anonymous mode.
          if (context->auth_level >= AUTHLEVEL_ask)
          {
            // we can send a certificate request packet, even if we
            // don't have any authorized issuers.
            SSL3_DEBUG_MSG("Sending CertificateRequest.\n");
            send_packet(certificate_request_packet(context));
            certificate_state = CERT_requested;
            handshake_state = STATE_wait_for_peer;
          }

          send_packet(certificate_verify_packet(SIGN_server_certificate_verify));
        }
      }

      send_packet(finished_packet("server finished"));

      if (handshake_state == STATE_wait_for_finish) {
        // No need to wait for the client's certificate, etc.

        derive_master_secret(session->master_secret);
        send_packet(change_cipher_packet());
        // Note: Don't switch to the new keys for the client
        //       before we've received it's finished packet.

        // NB: From this point on we can send application data.
      }
    }
    break;
  case STATE_wait_for_finish:
    {
      if (type != HANDSHAKE_finished)
        COND_FATAL(1, ALERT_unexpected_message, "Expected finished.\n");

      string(8bit) my_digest;
      string(8bit) digest;

      SSL3_DEBUG_MSG("SSL.ServerConnection: FINISHED\n");

      if(version == PROTOCOL_SSL_3_0) {
        my_digest=hash_messages("CLNT");
        digest = input->read(36);
      } else if(version >= PROTOCOL_TLS_1_0) {
        my_digest=hash_messages("client finished");
        digest = input->read(12);
      }

      if ((ke && ke->message_was_bad)	/* Error delayed until now */
          || (my_digest != digest))
      {
        if(ke && ke->message_was_bad)
          SSL3_DEBUG_MSG("message_was_bad\n");
        if(my_digest != digest)
          SSL3_DEBUG_MSG("digests differ\n");
        COND_FATAL(1, ALERT_unexpected_message, "Key exchange failure.\n");
      }
      add_handshake_message(raw);
      /* Second hash includes this message, the first doesn't */

      /* Handshake complete */
      client_verify_data = digest;

      if (version >= PROTOCOL_TLS_1_3) {
        // The client will use the main keys from this point on.
        handle_change_cipher(1);
      }

      if (!reuse)
      {
	if (version < PROTOCOL_TLS_1_3) {
	  send_packet(change_cipher_packet());
	  // We've already received the CCS from the peer.
	  expect_change_cipher--;
	}
        if(version == PROTOCOL_SSL_3_0)
          send_packet(finished_packet("SRVR"));
        else if(version < PROTOCOL_TLS_1_3)
          send_packet(finished_packet("server finished"));

        if (context->heartbleed_probe &&
            session->heartbeat_mode == HEARTBEAT_MODE_peer_allowed_to_send) {
          // Probe for the Heartbleed vulnerability (CVE-2014-0160).
          send_packet(heartbleed_packet());
        }
        context->record_session(session); /* Cache this session */
      }

      // Handshake hash is calculated for both directions above.
      handshake_messages = 0;

      handshake_state = STATE_wait_for_hello;

      return 1;
    }
    break;
  case STATE_wait_for_peer:
    // NB: ALERT_no_certificate can be valid in this state, and
    //     is handled directly by connection:handle_alert().
    switch(type)
    {
    case HANDSHAKE_client_key_share:
      // We can come here with TLS 1.3 clients that:
      //
      //  * Have no TLS 1.3 suites that we support.
      //
      //  * Have suites valid for TLS 1.2 or earlier that we support.
      //
      //  * Don't use the early data extension for this packet.
      //
      // Note that such clients aren't compatible with TLS 1.2
      // and earlier servers anyway...
      SSL3_DEBUG_MSG("SSL.ServerConnection: CLIENT_KEY_SHARE\n");
      // FALL_THROUGH.
    default:
      COND_FATAL(1, ALERT_unexpected_message,
                 "Expected client KEX or cert.\n");
      break;
    case HANDSHAKE_client_key_exchange:
      SSL3_DEBUG_MSG("SSL.ServerConnection: CLIENT_KEY_EXCHANGE\n");

      // Certificate must be sent before key exchange message
      COND_FATAL((certificate_state == CERT_requested) ||
                 (version >= PROTOCOL_TLS_1_3),
                 ALERT_unexpected_message, "Expected client cert.\n");

      if (!server_derive_master_secret(input))
	return -1;

      add_handshake_message(raw);

      SSL3_DEBUG_MSG("certificate_state: %d\n", certificate_state);
      // TODO: we need to determine whether the certificate has signing abilities.
      if (certificate_state == CERT_received)
      {
	handshake_state = STATE_wait_for_verify;
      }
      else
      {
	handshake_state = STATE_wait_for_finish;
	// We expect a CCS next.
	expect_change_cipher++;
      }

      break;
    case HANDSHAKE_certificate:
     {
       SSL3_DEBUG_MSG("SSL.ServerConnection: CLIENT_CERTIFICATE\n");

       COND_FATAL(certificate_state != CERT_requested,
                  ALERT_unexpected_message, "Unexpected client cert.\n");

       add_handshake_message(raw);

       if( !handle_certificates(input) )
         return -1;

       if(session->peer_public_key)
         certificate_state = CERT_received;
       else
         certificate_state = CERT_no_certificate;

       if (version >= PROTOCOL_TLS_1_3) {
	 if (certificate_state == CERT_received) {
	   handshake_state = STATE_wait_for_verify;
	 } else {
	   handshake_state = STATE_wait_for_finish;
	   derive_master_secret(session->master_secret);
	   send_packet(change_cipher_packet());
	 }
       }
       break;
     }
    }
    break;
  case STATE_wait_for_verify:
    {
      // compute challenge first, then update handshake_messages /Sigge
      if (type != HANDSHAKE_certificate_verify)
        COND_FATAL(1, ALERT_unexpected_message, "Expected cert verify.\n");

      SSL3_DEBUG_MSG("SSL.ServerConnection: CERTIFICATE_VERIFY\n");

      SSL3_DEBUG_MSG("SERVER: handshake_messages: %d bytes.\n",
		     sizeof(handshake_messages));
      if (validate_certificate_verify(input,
				      SIGN_client_certificate_verify) < 0) {
	return -1;
      }

      add_handshake_message(raw);
      handshake_state = STATE_wait_for_finish;

      if (version >= PROTOCOL_TLS_1_3) {
	derive_master_secret(session->master_secret);
      }

      // We expect a CCS next.
      expect_change_cipher++;

      // NB TLS 1.3: From this point on we can send application data.
    }
    break;
  }

  return 0;
}
