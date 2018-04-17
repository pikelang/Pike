#pike __REAL_VERSION__
#pragma strict_types
#require constant(SSL.Cipher)

//! Client-side connection state.

#include "tls.h"

import ".";
import Constants;
inherit Connection;

//! A few storage variables for client certificate handling on the client side.
array(int) client_cert_types;
array(string(8bit)) client_cert_distinguished_names;

protected string _sprintf(int t)
{
  if (t == 'O') return sprintf("SSL.ClientConnection(%s)", describe_state());
}

protected Packet client_hello(string(8bit)|void server_name,
                              array(Packet)|void early_data)
{
  Buffer struct = Buffer();
  /* Build client_hello message */
  client_version = version;
  // Clamp version to TLS 1.2. 1.3 and later are negotiated using
  // supported versions extension.
  struct->add_int(min(client_version, PROTOCOL_TLS_1_2), 2);

  // The first four bytes of the client_random is specified to be the
  // timestamp on the client side. This is to guard against bad random
  // generators, where a client could produce the same random numbers
  // if the seed is reused. This argument is flawed, since a broken
  // random generator will make the connection insecure anyways. The
  // standard explicitly allows these bytes to not be correct, so
  // sending random data instead is safer and reduces client
  // fingerprinting.
  client_random = context->random(32);

  struct->add(client_random);
  struct->add_hstring(session->identity || "", 1);

  array(int) cipher_suites = context->preferred_suites;
  if ((state & CONNECTION_handshaking) && !secure_renegotiation) {
    // Initial handshake.
    // Use the backward-compat way of asking for
   // support for secure renegotiation.
    cipher_suites += ({ TLS_empty_renegotiation_info_scsv });

    if (client_version < max(@context->supported_versions)) {
      // Negotiating a version lower than the max supported version.
      //
      // RFC 7507 4:
      // If a client sends a ClientHello.client_version containing a lower
      // value than the latest (highest-valued) version supported by the
      // client, it SHOULD include the TLS_FALLBACK_SCSV cipher suite value
      // in ClientHello.cipher_suites; see Section 6 for security
      // considerations for this recommendation.  (The client SHOULD put
      // TLS_FALLBACK_SCSV after all cipher suites that it actually intends
      // to negotiate.)
      cipher_suites += ({ TLS_fallback_scsv });
    }
  }
  SSL3_DEBUG_MSG("Client ciphers:\n%s",
                 fmt_cipher_suites(cipher_suites));
  struct->add_int_array(cipher_suites, 2, 2);

  array(int) compression_methods;
  if (client_version >= PROTOCOL_TLS_1_3) {
    // TLS 1.3 (draft 3) does not allow any compression.
    compression_methods = ({ COMPRESSION_null });
  } else {
    compression_methods = context->preferred_compressors;
  }
  struct->add_int_array(compression_methods, 1, 1);

  Buffer extensions = Buffer();

  void ext(int id, int condition, function(void:Buffer) code)
  {
    if(context->extensions[id] && condition)
    {
      extensions->add_int(id, 2);
      extensions->add_hstring(code(), 2);
    }
  };

  ext (EXTENSION_supported_versions, client_version >= PROTOCOL_TLS_1_3) {
    Buffer versions = Buffer();
    foreach(context->supported_versions;; ProtocolVersion v)
      v->add_int(v, 2);
    return versions;
  };

  ext (EXTENSION_renegotiation_info, secure_renegotiation) {
    // RFC 5746 3.4:
    // The client MUST include either an empty "renegotiation_info"
    // extension, or the TLS_EMPTY_RENEGOTIATION_INFO_SCSV signaling
    // cipher suite value in the ClientHello.  Including both is NOT
    // RECOMMENDED.
    return Buffer()->add_hstring(client_verify_data, 1);
  };

  ext (EXTENSION_elliptic_curves,
       sizeof(context->ecc_curves)||sizeof(context->ffdhe_groups)) {
    // RFC 4492 5.1:
    // The extensions SHOULD be sent along with any ClientHello message
    // that proposes ECC cipher suites.
    Buffer curves = Buffer();
    foreach(context->ecc_curves, int curve) {
      curves->add_int(curve, 2);
    }
    // FFDHE draft 4 3:
    //   The compatible client that wants to be able to negotiate
    //   strong FFDHE SHOULD send a "Supported Groups" extension
    //   (identified by type elliptic_curves(10) in [RFC4492]) in the
    //   ClientHello, and include a list of known FFDHE groups in the
    //   extension data, ordered from most preferred to least preferred.
    //
    // NB: The ffdhe_groups in the context has the smallest group first,
    //     so we reverse it here in case the server actually follows
    //     our priority order.
    foreach(reverse(context->ffdhe_groups), int group) {
      curves->add_int(group, 2);
    }
    // FIXME: FFDHE draft 4 6.1:
    //   More subtly, clients MAY interleave preferences between ECDHE
    //   and FFDHE groups, for example if stronger groups are
    //   preferred regardless of cost, but weaker groups are
    //   acceptable, the Supported Groups extension could consist of:
    //   <ffdhe8192,secp384p1,ffdhe3072,secp256r1>. In this example,
    //   with the same CipherSuite offered as the previous example, a
    //   server configured to respect client preferences and with
    //   support for all listed groups SHOULD select
    //   TLS_DHE_RSA_WITH_AES_128_CBC_SHA with ffdhe8192. A server
    //   configured to respect client preferences and with support for
    //   only secp384p1 and ffdhe3072 SHOULD select
    //   TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA with secp384p1.
    return Buffer()->add_hstring(curves, 2);
  };

  ext (EXTENSION_ec_point_formats, sizeof(context->ecc_curves)) {
    Buffer point = Buffer();
    point->add_int(POINT_uncompressed, 1);
    return Buffer()->add_hstring(point, 1);
  };

  // We always attempt to enable the heartbeat extension.
  ext (EXTENSION_heartbeat, 1) {
    // RFC 6420
    return Buffer()->add_int(HEARTBEAT_MODE_peer_allowed_to_send, 1);
  };

  ext (EXTENSION_encrypt_then_mac, 1) {
    // RFC 7366
    return Buffer();
  };

  ext (EXTENSION_extended_master_secret,
       context->extended_master_secret &&
       ( has_value(context->supported_versions, PROTOCOL_TLS_1_0) ||
         has_value(context->supported_versions, PROTOCOL_TLS_1_1) ||
         has_value(context->supported_versions, PROTOCOL_TLS_1_2) )) {
    // draft-ietf-tls-session-hash
    // NB: This extension is implicit in TLS 1.3 and N/A in SSL.
    return Buffer();
  };

  ext (EXTENSION_signature_algorithms, client_version >= PROTOCOL_TLS_1_2) {
    // RFC 5246 7.4.1.4.1:
    // If the client supports only the default hash and signature algorithms
    // (listed in this section), it MAY omit the signature_algorithms
    // extension.  If the client does not support the default algorithms, or
    // supports other hash and signature algorithms (and it is willing to
    // use them for verifying messages sent by the server, i.e., server
    // certificates and server key exchange), it MUST send the
    // signature_algorithms extension, listing the algorithms it is willing
    // to accept.

    // We list all hashes and signature formats that we support.
    return Buffer()->add_hstring(get_signature_algorithms(), 2);
  };

  ext (EXTENSION_server_name, !!server_name)
  {
    Buffer hostname = Buffer();
    hostname->add_int(0, 1); // name_time host_name(0)
    hostname->add_hstring(server_name, 2); // hostname

    return Buffer()->add_hstring(hostname, 2);
  };

  ext (EXTENSION_session_ticket, 1)
  {
    // RFC 4507 and RFC 5077.
    if (session->ticket_expiry_time < time(1)) {
      session->ticket = UNDEFINED;
      session->ticket_expiry_time = 0;
    }
    SSL3_DEBUG_MSG("SSL.ClientConnection: Sending ticket %O.\n",
		   session->ticket);
    // NB: RFC 4507 and RFC 5077 differ in encoding here.
    //     Apparently no implementations actually followed
    //     the RFC 4507 encoding.
    return Buffer()->add(session->ticket || "");
  };

  ext (EXTENSION_application_layer_protocol_negotiation,
       !!(context->advertised_protocols))
  {
    return Buffer()->add_string_array(context->advertised_protocols, 1, 2);
  };

  // When the client HELLO packet data is in the range 256-511 bytes
  // f5 SSL terminators will intepret it as SSL2 requiring an
  // additional 8k of data, which will cause the connection to hang.
  // The solution is to pad the package to more than 511 bytes using a
  // dummy exentsion.
  // Reference: draft-agl-tls-padding
  int packet_size = sizeof(struct)+sizeof(extensions)+2;
  ext (EXTENSION_padding, packet_size>255 && packet_size<512)
  {
    int padding = max(0, 512-packet_size-4);
    SSL3_DEBUG_MSG("SSL.ClientConnection: Adding %d bytes of padding.\n",
                   padding);
    return Buffer()->add("\0"*padding);
  };

  ext(EXTENSION_early_data, early_data && sizeof(early_data)) {
    SSL3_DEBUG_MSG("SSL.ClientConnection: Adding %d packets of early data.\n",
		   sizeof(early_data));
    Buffer buf = Buffer();
    foreach(early_data, Packet p) {
      p->send(buf);
    }
    return buf;
  };

  // NB: WebSphere Application Server 7.0 doesn't like having an
  //     empty extension last, so don't put any such extensions
  //     in the list here.
  //     cf https://bugs.chromium.org/p/chromium/issues/detail?id=363583#c17

  if(sizeof(extensions) && (version >= PROTOCOL_TLS_1_0))
    struct->add_hstring(extensions, 2);

  SSL3_DEBUG_MSG("SSL.ClientConnection: Client hello: %q\n", struct);
  Packet ret = handshake_packet(HANDSHAKE_client_hello, struct);
  if (version > PROTOCOL_TLS_1_0) {
    // For maximum interoperability, it seems having
    // version TLS 1.0 at the packet level is best.
    //
    // From Xiaoyin Liu <xiaoyin.l@outlook.com> on
    // the TLS mailing list 2015-01-21:
    //
    // (1) Number of sites scanned: 1,000,001
    // (2) Number of DNS Error: 45,402
    // (3) Number of sites that refuse TCP connection on port 443
    //     (RST, timeout): 289,334
    // (4) Number of sites that fail sending ServerHello in all 4
    //     attempts: 238,846
    // (5) Number of sites that are tolerant to (TLS1.3, TLS1.3):
    //     397,152 (93.1%)
    // (6) Number of sites that need to fallback to (TLS1.0, TLS1.3):
    //     22,461 (5.3%)
    // (7) Number of sites that need to fallback to (TLS1.0, TLS1.2):
    //     6,352 (1.5%)
    // (8) Number of sites that need to fallback to (TLS1.0, TLS1.0):
    //     454 (0.1%)
    ret->protocol_version = PROTOCOL_TLS_1_0;
  }
  return ret;
}

protected Packet finished_packet(string(8bit) sender)
{
  SSL3_DEBUG_MSG("Sending finished_packet, with sender=\""+sender+"\"\n" );
  // We're the client.
  client_verify_data = hash_messages(sender);
  return handshake_packet(HANDSHAKE_finished, client_verify_data);
}

protected Packet client_key_exchange_packet()
{
  Stdio.Buffer packet_data = Stdio.Buffer();
  if (!ke) {
    ke = session->cipher_spec->ke_factory(context, session, this,
					  client_version);
    if (!ke->init_client()) {
      send_packet(alert(ALERT_fatal, ALERT_handshake_failure,
			"Invalid KEX.\n"));
      return 0;
    }
  }
  string(8bit) premaster_secret =
    ke->client_key_exchange_packet(packet_data, version);

  if (!premaster_secret) {
    send_packet(alert(ALERT_fatal, ALERT_handshake_failure,
		      "Invalid KEX.\n"));
    return 0;
  }

  derive_master_secret(premaster_secret);

  return handshake_packet(HANDSHAKE_client_key_exchange, packet_data);
}

//! Initialize a new @[ClientConnection].
//!
//! @param ctx
//!   @[Context] to use.
//!
//! @param server_name
//!   Optional host name of the server.
//!
//! @param session
//!   Optional @[Session] to resume.
protected void create(Context ctx, string(8bit)|void server_name,
		      Session|void session)
{
  ::create(ctx);
  handshake_state = STATE_wait_for_hello;
  if (session &&
      (!server_name || session->server_name == server_name)) {
    // Reuse the session.
    this_program::session = session;
  } else {
    this_program::session = Session();
    this_program::session->server_name = server_name;
    this_program::session->ffdhe_groups = ctx->ffdhe_groups;
  }

  // Pre-TLS 1.3.
  SSL3_DEBUG_MSG("CLIENT: TLS <= 1.2 handshake.\n");
  send_packet(client_hello(server_name));
}

//! Renegotiate the connection (client initiated).
//!
//! Sends a @[client_hello] to force a new round of handshaking.
void send_renegotiate()
{
  if (!context->enable_renegotiation) {
    error("Renegotiation disabled in context.\n");
  }
  send_packet(client_hello(session->server_name), PRI_application);
}

protected int send_certs()
{
  /* Send Certificate, ClientKeyExchange, CertificateVerify and
   * ChangeCipherSpec as appropriate, and then Finished.
   */
  CertificatePair cert;

  /* Only send a certificate if it's been requested. */
  if(client_cert_types)
  {
    SSL3_DEBUG_MSG("Searching for a suitable client certificate...\n");

    // Okay, we have a list of certificate types and DN:s that are
    // acceptable to the remote server. We should weed out the certs
    // we have so that we only send certificates that match what they
    // want.

    SSL3_DEBUG_MSG("All certs: %O\n"
		   "distinguished_names: %O\n",
		   context->get_certificates(),
		   client_cert_distinguished_names);

    array(CertificatePair) certs =
      context->find_cert_issuer(client_cert_distinguished_names) || ({});

    certs = [array(CertificatePair)]
      filter(certs,
	     lambda(CertificatePair cp) {

               // Is the certificate type supported?
               if( !has_value(client_cert_types, cp->cert_type) )
                 return 0;

               // Are the individual hash and sign algorithms in the
               // certificate chain supported?
               foreach(cp->sign_algs, [int cert_hash, int cert_sign])
               {
                 int match;
                 foreach(session->signature_algorithms, [int hash, int sign])
                 {
                   if( hash==cert_hash && sign==cert_sign )
                   {
                     match = 1;
                     break;
                   }
                 }
                 if( !match )
                   return 0;
               }

	       return 1;
	     });

    if (sizeof(certs)) {
      SSL3_DEBUG_MSG("Found %d matching client certs.\n", sizeof(certs));
      cert = certs[0];
      session->private_key = cert->key;
      session->certificate_chain = cert->certs;
      send_packet(certificate_packet(session->certificate_chain));
    } else {
      SSL3_DEBUG_MSG("No suitable client certificate found.\n");
      send_packet(certificate_packet(({})));
    }
  }

  COND_FATAL(!session->has_required_certificates(),
             ALERT_unexpected_message, "Certificate message missing.\n");

  Packet key_exchange = client_key_exchange_packet();

  if (key_exchange) {
    send_packet(key_exchange);
  }

  if(cert) {
    // We sent a certificate, so we should send the verification.
    send_packet(certificate_verify_packet());
  }

  send_packet(change_cipher_packet());

  if(version == PROTOCOL_SSL_3_0)
    send_packet(finished_packet("CLNT"));
  else if(version >= PROTOCOL_TLS_1_0) {
    send_packet(finished_packet("client finished"));
  }

  // NB: The server direction hash will be calculated
  //     when we've received the server finished packet.

  if (context->heartbleed_probe &&
      session->heartbeat_mode == HEARTBEAT_MODE_peer_allowed_to_send) {
    // Probe for the Heartbleed vulnerability (CVE-2014-0160).
    send_packet(heartbleed_packet());
  }

  return 0;
}

void new_cipher_states()
{
  SSL3_DEBUG_MSG("CLIENT: master: %O\n", session->master_secret);

  array(State) res =
    session->new_client_states(this, client_random, server_random,
			       version);
  pending_read_state += ({ res[0] });
  pending_write_state += ({ res[1] });
}


protected int(-1..0) got_certificate_request(Buffer input)
{
  SSL3_DEBUG_MSG("SSL.ClientConnection: CERTIFICATE_REQUEST\n");

  // It is a fatal handshake_failure alert for an anonymous server to
  // request client authentication.
  //
  // RFC 5246 7.4.4:
  //   A non-anonymous server can optionally request a certificate from
  //   the client, if appropriate for the selected cipher suite.
  COND_FATAL(session->cipher_spec->signature_alg == SIGNATURE_anonymous,
             ALERT_handshake_failure,
             "Anonymous server requested authentication by certificate.\n");

  client_cert_types = input->read_int_array(1, 1);
  client_cert_distinguished_names = ({});

  if (version >= PROTOCOL_TLS_1_2) {
    // TLS 1.2 has var_uint_array of hash and sign pairs here.
    string bytes = input->read_hstring(2);
    COND_FATAL(sizeof(bytes)&1, ALERT_handshake_failure,
               "Odd number of bytes in supported_signature_algorithms.\n");

    // Pairs of <hash_alg, signature_alg>.
    session->signature_algorithms = ((array(int))bytes)/2;
    SSL3_DEBUG_MSG("New signature_algorithms:\n"+
		   fmt_signature_pairs(session->signature_algorithms));
  }

  Stdio.Buffer s = input->read_hbuffer(2);
  while(sizeof(s))
    client_cert_distinguished_names += ({ s->read_hstring(2) });

  SSL3_DEBUG_MSG("Got %O potential certificate names.",
                 sizeof(client_cert_distinguished_names));

  COND_FATAL(sizeof(input), ALERT_handshake_failure,
             "Badly formed Certificate Request.\n");

  return 0;
}

protected int(-1..1) got_new_session_ticket(Buffer input)
{
  COND_FATAL(!tickets_enabled, ALERT_handshake_failure,
	     "Unexpected session ticket.\n");
  // Make sure that we only get one ticket.
  tickets_enabled = 3;

  int lifetime_hint = input->read_int(4);
  string(8bit) ticket = input->read_hstring(2);

  SSL3_DEBUG_MSG("SSL.ClientConnection: Got ticket %O (%d seconds).\n",
		 ticket, lifetime_hint);

  COND_FATAL(!sizeof(ticket), ALERT_handshake_failure,
	     "Empty ticket.\n");

  if (!lifetime_hint) {
    // Unspecified lifetime. Handle as one hour.
    lifetime_hint = 3600;
  }

  session->ticket = ticket;
  session->ticket_expiry_time = lifetime_hint + time(1);
  return 0;
}

//! Do handshake processing.
//!
//! @param type
//!   One of HANDSHAKE_*.
//! @param input
//!   The contents of the packet.
//! @param raw
//!   The raw packet received (needed for supporting SSLv2 hello messages).
//!
//! @returns
//!   This function returns:
//!   @int
//!     @value 0
//!       If handshaking is in progress.
//!     @value 1
//!       If handshaking has completed.
//!     @value -1
//!       If a fatal error occurred.
//!   @endint
//!
//!   It uses the @[send_packet()] function to transmit packets.
int(-1..1) handle_handshake(int type, Buffer input, Stdio.Buffer raw)
{
#ifdef SSL3_PROFILING
  addRecord(type,0);
#endif
#ifdef SSL3_DEBUG_HANDSHAKE_STATE
  werror("SSL.ClientConnection: state %s, type %s\n",
	 fmt_constant(handshake_state, "STATE"),
         fmt_constant(type, "HANDSHAKE"));
  werror("sizeof(data)="+sizeof(data)+"\n");
#endif

#if 0	// Not compatible with session tickets...
  // Enforce packet ordering.
  COND_FATAL(type <= previous_handshake, ALERT_unexpected_message,
             "Invalid handshake packet order.\n");

  previous_handshake = type;
#endif

  switch(handshake_state)
  {
  default:
    error( "Internal error\n" );
  case STATE_wait_for_hello:
    add_handshake_message(raw);
    switch(type) {
    default:
      COND_FATAL(1, ALERT_unexpected_message, "Expected server hello.\n");
      break;
    case HANDSHAKE_server_hello:
      SSL3_DEBUG_MSG("SSL.ClientConnection: SERVER_HELLO\n");

      string(8bit) session_id;
      int cipher_suite, compression_method;

      version = [int(0x300..0x300)|ProtocolVersion]input->read_int(2);
      server_random = input->read(32);
      session_id = input->read_hstring(1);
      cipher_suite = input->read_int(2);
      compression_method = input->read_int(1);

      if( !has_value(context->supported_versions, version) )
      {
        SSL3_DEBUG_MSG("Unsupported version of SSL: %s (Requested {%s}).\n",
                       fmt_version(version),
                       fmt_version(context->supported_versions[*])*",");
        version = client_version;
        COND_FATAL(1, ALERT_protocol_version, "Unsupported version.\n");
      }
      SSL3_DEBUG_MSG("Selecting version %s.\n", fmt_version(version));

      if( !has_value(context->preferred_suites, cipher_suite) ||
	  !has_value(context->preferred_compressors, compression_method) ||
	  !session->is_supported_suite(cipher_suite, ~0, version))
      {
	// The server tried to trick us to use some other cipher suite
	// or compression method than we wanted
	version = client_version; // FIXME: Do we need this?
        COND_FATAL(1, ALERT_handshake_failure,
                   "Server selected bad suite.\n");
      }

      SSL3_DEBUG_MSG("Server selected suite %s.\n",
                     fmt_cipher_suite(cipher_suite));
      COND_FATAL(!session->set_cipher_suite(cipher_suite, version,
                                            context->signature_algorithms,
                                            512),
                 ALERT_handshake_failure,
                 "Unsupported or obsolete cipher suite.\n");

      COND_FATAL(version >= PROTOCOL_TLS_1_3 &&
                 compression_method!=COMPRESSION_null,
                 ALERT_insufficient_security,
                 "Compression not supported in TLS 1.3 and later.\n");

      session->set_compression_method(compression_method);

      SSL3_DEBUG_MSG("STATE_wait_for_hello: received hello\n"
		     "version = %s\n"
		     "session_id=%O\n"
                     "cipher suite: 0x%x\n"
		     "compression method: %O\n",
		     fmt_version(version),
		     session_id, cipher_suite, compression_method);

      int missing_secure_renegotiation = secure_renegotiation;

      if (sizeof(input)) {
	Stdio.Buffer extensions = input->read_hbuffer(2);
        multiset(int) remote_extensions = (<>);

	while (sizeof(extensions)) {
	  int extension_type = extensions->read_int(2);
	  Buffer extension_data =
	    Buffer(extensions->read_hstring(2));
	  SSL3_DEBUG_MSG("SSL.ClientConnection->handle_handshake: "
			 "Got extension %s.\n",
			 fmt_constant(extension_type, "EXTENSION"));
          COND_FATAL(remote_extensions[extension_type],
                     ALERT_decode_error, "Same extension sent twice.\n");

          remote_extensions[extension_type] = 1;

          if( !context->extensions[extension_type] )
          {
            SSL3_DEBUG_MSG("Ignored extension %O (%d bytes)\n",
                           extension_type, sizeof(extension_data));
            continue;
          }

	  switch(extension_type) {
	  case EXTENSION_renegotiation_info:
	    string renegotiated_connection = extension_data->read_hstring(1);

            // RFC 5746 3.5: (secure_renegotiation)
            // The client MUST then verify that the first half of the
            // "renegotiated_connection" field is equal to the saved
            // client_verify_data value, and the second half is equal
            // to the saved server_verify_data value. If they are not,
            // the client MUST abort the handshake.
            //
            // RFC 5746 4.2: (!secure_renegotiation)
            // When the ServerHello is received, the client MUST
            // verify that it does not contain the
            // "renegotiation_info" extension. If it does, the client
            // MUST abort the handshake. (Because the server has
            // already indicated it does not support secure
            // renegotiation, the only way that this can happen is if
            // the server is broken or there is an attack.)
            COND_FATAL((renegotiated_connection !=
                        (client_verify_data + server_verify_data)) ||
                       (!(state & CONNECTION_handshaking) &&
                        !secure_renegotiation),
                       ALERT_handshake_failure,
                       "Invalid renegotiation data.\n");

	    secure_renegotiation = 1;
	    missing_secure_renegotiation = 0;
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
            break;
          case EXTENSION_elliptic_curves:
            /* This is only supposed to be included in ClientHello, but some
             * servers send it anyway, and other SSLs ignore it. */
            break;
	  case EXTENSION_server_name:
            break;

	  case EXTENSION_application_layer_protocol_negotiation:
	    array(string(8bit)) selected_prot =
	      extension_data->read_string_array(1, 2);
	    COND_FATAL((!context->advertised_protocols ||
			(sizeof(selected_prot) != 1) ||
			!has_value(context->advertised_protocols,
				   selected_prot[0])),
		       ALERT_handshake_failure,
		       "Invalid ALPN.\n");
	    application_protocol = selected_prot[0];
	    break;

	  case EXTENSION_session_ticket:
	    COND_FATAL(sizeof(extension_data), ALERT_handshake_failure,
		       "Invalid server session ticket extension.\n");
	    SSL3_DEBUG_MSG("SSL.ClientConnection: Server supports tickets.\n");
	    tickets_enabled = 1;
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
                         "Invalid heartbeat extension.\n");

	      SSL3_DEBUG_MSG("heartbeat extension: %s\n",
			     fmt_constant(hb_mode, "HEARTBEAT_MODE"));
	      session->heartbeat_mode = [int(0..1)]hb_mode;
	    }
	    break;

	  case EXTENSION_extended_master_secret:
	    {
	      COND_FATAL(sizeof(extension_data) ||
                         (min(@context->supported_versions) >= PROTOCOL_TLS_1_3),
                         ALERT_illegal_parameter,
                         "Extended-master-secret: Invalid extension.\n");

	      SSL3_DEBUG_MSG("Extended-master-secret: Enabled.\n");
	      session->extended_master_secret = 1;
	    }
            break;

          case EXTENSION_supported_versions:
            {
              COND_FATAL(sizeof(extension_data)!=2,
                         ALERT_illegal_parameter,
                         "Illegal size of supported version extension.\n");
              version = extension_data->read_int16();
              COND_FATAL( !has_value(context->supported_versions, version),
                         ALERT_illegal_parameter,
                         "Received version not offered.\n");
            }
            break;

	  case EXTENSION_encrypt_then_mac:
	    {
	      if (context->encrypt_then_mac) {
		COND_FATAL(sizeof(extension_data), ALERT_illegal_parameter,
                           "Encrypt-then-MAC: Invalid extension.\n");

		COND_FATAL(((sizeof(CIPHER_SUITES[cipher_suite]) == 3) &&
		     (< CIPHER_rc4, CIPHER_rc4_40 >)[CIPHER_SUITES[cipher_suite][1]]) ||
		    ((sizeof(CIPHER_SUITES[cipher_suite]) == 4) &&
		     (CIPHER_SUITES[cipher_suite][3] != MODE_cbc)),
                           ALERT_illegal_parameter,
                           "Encrypt-then-MAC: Invalid for selected suite.\n");

		SSL3_DEBUG_MSG("Encrypt-then-MAC: Enabled.\n");
		session->encrypt_then_mac = 1;
		break;
	      }
	      /* We didn't request the extension, so complain loudly. */
	    }
	    /* FALL_THROUGH */

	  default:
	    // RFC 5246 7.4.1.4:
	    // If a client receives an extension type in ServerHello
	    // that it did not request in the associated ClientHello, it
	    // MUST abort the handshake with an unsupported_extension
            // fatal alert.
            SSL3_DEBUG_MSG("Unrequested extension %s.\n",
                           fmt_constant(extension_type, "EXTENSION"));
            COND_FATAL(1, ALERT_unsupported_extension,
                       "Unrequested extension.\n");
	  }
	}
      }

      if (session->ticket && !tickets_enabled) {
	// The server has stopped supporting session tickets?
	// Make sure not to compare the server-generated
	// session id with the one that we may have generated.
	session_id = "";
      }

      // RFC 5746 3.5:
      // When a ServerHello is received, the client MUST verify that the
      // "renegotiation_info" extension is present; if it is not, the
      // client MUST abort the handshake.
      COND_FATAL(missing_secure_renegotiation, ALERT_handshake_failure,
                 "Missing secure renegotiation extension.\n");

      if ((session_id == session->identity) && sizeof(session_id)) {
	SSL3_DEBUG_MSG("Resuming session %O.\n", session_id);

        new_cipher_states();
        send_packet(change_cipher_packet());

	if (tickets_enabled) {
	  handshake_state = STATE_wait_for_ticket;
	  // Expect the ticket before the CC.
	  expect_change_cipher--;
	} else {
	  handshake_state = STATE_wait_for_finish;
	}
	reuse = 1;
	break;
      }

      if ((session_id == "") && tickets_enabled) {
	// Generate a session identifier.
	// NB: We currently do NOT support resumption based on an
	//     empty session id and a HANDSHAKE_new_session_ticket.
	//     We thus need a non-empty session id when we use
	//     this new session for resumption. We don't care much
	//     about the value (as long as it is non-empty), as
	//     a compliant server will return either the value we
	//     provided, or the empty string.
	session_id = "RESUMPTION_TICKET";
      }

      session->identity = session_id;
      handshake_state = STATE_wait_for_peer;
      break;
    }
    break;

  case STATE_wait_for_peer:
    add_handshake_message(raw);
    switch(type)
    {
    default:
      COND_FATAL(1, ALERT_unexpected_message, "Unexpected server message.\n");
      break;
      /* FIXME: HANDSHAKE_encrypted_extensions */
    case HANDSHAKE_certificate:
      {
	SSL3_DEBUG_MSG("SSL.ClientConnection: Certificate received\n");

        // we're anonymous, so no certificate is requred.
        if(ke && ke->anonymous)
          break;

        if( !handle_certificates(input) )
          return -1;

        certificate_state = CERT_received;
        break;
      }

    case HANDSHAKE_server_key_exchange:
      {
	COND_FATAL(version >= PROTOCOL_TLS_1_3, ALERT_unexpected_message,
		   "Unexpected server message.\n");

	if (ke) error("KE!\n");
	ke = session->cipher_spec->ke_factory(context, session, this,
					      client_version);
	COND_FATAL(!ke->init_server() ||
                   (ke->got_server_key_exchange(input, client_random,
                                                server_random) < 0),
                   ALERT_handshake_failure,
                   "Verification of ServerKeyExchange failed.\n");
	break;
      }

    case HANDSHAKE_certificate_request:
      return got_certificate_request(input);

    case HANDSHAKE_server_hello_done:
      SSL3_DEBUG_MSG("SSL.ClientConnection: SERVER_HELLO_DONE\n");

      COND_FATAL(version >= PROTOCOL_TLS_1_3, ALERT_unexpected_message,
		 "Unexpected server message.\n");

      if (send_certs()) return -1;

      if (tickets_enabled) {
	handshake_state = STATE_wait_for_ticket;
	// Expect the ticket before the CC.
	expect_change_cipher--;
      } else {
	handshake_state = STATE_wait_for_finish;
      }
      break;
    }
    break;

  case STATE_wait_for_verify:
    if (version < PROTOCOL_TLS_1_3) {
      error("Waiting for verify in %s.\n",
	    fmt_version(version));
    }
    switch(type) {
    default:
      COND_FATAL(1, ALERT_unexpected_message, "Unexpected server message.\n");
      break;
    case HANDSHAKE_certificate_request:
      add_handshake_message(raw);
      return got_certificate_request(input);
    case HANDSHAKE_certificate_verify:
      SSL3_DEBUG_MSG("SSL.ClientConnection: CERTIFICATE_VERIFY\n");

      COND_FATAL(version < PROTOCOL_TLS_1_3, ALERT_unexpected_message,
                 "Unexpected server message.\n");

      SSL3_DEBUG_MSG("SERVER: handshake_messages: %d bytes.\n",
		     sizeof(handshake_messages));
      if (validate_certificate_verify(input,
				      SIGN_server_certificate_verify) < 0) {
	return -1;
      }

      add_handshake_message(raw);

      handshake_state = STATE_wait_for_finish;
      break;
    }
    break;

  case STATE_wait_for_ticket:
    {
      COND_FATAL(type != HANDSHAKE_new_session_ticket, ALERT_unexpected_message,
                 "Expected new session ticket.\n");

      SSL3_DEBUG_MSG("SSL.ClientConnection: NEW_SESSION_TICKET\n");
      add_handshake_message(raw);

      // Expect CC.
      expect_change_cipher++;
      handshake_state = STATE_wait_for_finish;
      return got_new_session_ticket(input);
    }
    break;

  case STATE_wait_for_finish:
    {
      COND_FATAL(type != HANDSHAKE_finished, ALERT_unexpected_message,
                 "Expected handshake finished.\n");

      SSL3_DEBUG_MSG("SSL.ClientConnection: FINISHED\n");

      string my_digest;
      if (version == PROTOCOL_SSL_3_0) {
        server_verify_data = input->read(36);
        my_digest = hash_messages("SRVR");
      } else if (version >= PROTOCOL_TLS_1_0) {
        server_verify_data = input->read(12);
        my_digest = hash_messages("server finished");
      }

      COND_FATAL(my_digest != server_verify_data,
                 ALERT_unexpected_message, "Digests differ.\n");

      if (reuse || (version >= PROTOCOL_TLS_1_3)) {
	add_handshake_message(raw);
        /* Second hash includes this message, the first doesn't */

	if(version == PROTOCOL_SSL_3_0)
	  send_packet(finished_packet("CLNT"));
	else if(version <= PROTOCOL_TLS_1_2)
	  send_packet(finished_packet("client finished"));
        if (context->heartbleed_probe &&
	    session->heartbeat_mode == HEARTBEAT_MODE_peer_allowed_to_send) {
	  // Probe for the Heartbleed vulnerability (CVE-2014-0160).
	  send_packet(heartbleed_packet());
	}
      }

      // Handshake hash is calculated for both directions above.
      handshake_messages = 0;

      handshake_state = STATE_handshake_finished;

      return 1;			// We're done shaking hands
    }
  }

  return 0;
}
