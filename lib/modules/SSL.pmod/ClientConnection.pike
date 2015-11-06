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

//! Active key share offers.
//!
//! NB: Only used with TLS 1.3 and later.
mapping(int:Cipher.KeyExchange) keyshares = ([]);

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
  struct->add_int(client_version, 2); /* version */

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

    if (client_version < context->max_version) {
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
       context->min_version < PROTOCOL_TLS_1_3) {
    // draft-ietf-tls-session-hash
    // NB: This extension is implicit in TLS 1.3.
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

protected void make_key_share_offer(int group)
{
  SSL3_DEBUG_MSG("make_key_share_offer(%s)\n",
		 fmt_constant(group, "GROUP"));
  if (keyshares[group]) return;

  Cipher.KeyExchange ke;

  if ((group & 0xff00) == 0x0100) {
    if (!FFDHE_GROUPS[group] && !context->private_ffdhe_groups[group]) return;
    ke = Cipher.KeyShareDHE(context, 0, this, client_version);
    // ke->init_client();
    ke->set_group(group);
#if constant(Crypto.ECC.Curve)
  } else if (!(group & 0xff00)) {
    if (!ECC_CURVES[group]) return;
    ke = Cipher.KeyShareECDHE(context, 0, this, client_version);
    // ke->init_client();
    ke->set_group(group);
#endif
  } else {
    SSL3_DEBUG_MSG("Can't create key share for unknown group: %s\n",
		   fmt_constant(group, "GROUP"));
  }

  keyshares[group] = ke;
}

protected Packet client_key_share_packet()
{
  Stdio.Buffer offers = Stdio.Buffer();

  // NB: Prefer any ECDHE exchange to any DHE exchange.

  SSL3_DEBUG_MSG("Key shares: %O\n", keyshares);

#if constant(Crypto.ECC.Curve)
  foreach(context->ecc_curves, int c) {
    Cipher.KeyExchange ke = keyshares[c];
    if (!ke) continue;
    SSL3_DEBUG_MSG("%s: %O\n",
		   fmt_constant(c, "GROUP"), ke);
    ke->make_key_share_offer(offers);
  }
#endif

  foreach(context->ffdhe_groups, int g) {
    Cipher.KeyExchange ke = keyshares[g];
    if (!ke) continue;
    SSL3_DEBUG_MSG("%s: %O\n",
		   fmt_constant(g, "GROUP"), ke);
    ke->make_key_share_offer(offers);
  }

  Stdio.Buffer struct = Stdio.Buffer();
  struct->add_hstring(offers, 2);
  return handshake_packet(HANDSHAKE_client_key_share, struct);
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

  if (ctx->max_version >= PROTOCOL_TLS_1_3) {
    // Generate some key shares.
    SSL3_DEBUG_MSG("Generating default key shares for %s.\n",
		   fmt_version(ctx->max_version));
#if constant(Crypto.ECC.Curve)
    if (sizeof(ctx->ecc_curves)) {
      make_key_share_offer(ctx->ecc_curves[0]);
    }
#endif
    if (sizeof(ctx->ffdhe_groups)) {
      make_key_share_offer(ctx->ffdhe_groups[0]);
    }
  }

  if (ctx->min_version >= PROTOCOL_TLS_1_3) {
    // TLS 1.3.
    SSL3_DEBUG_MSG("CLIENT: True TLS 1.3 handshake.\n");
    send_packet(client_hello(server_name));
    send_packet(client_key_share_packet());
  } else if (ctx->max_version >= PROTOCOL_TLS_1_3) {
    // Backward compat mode TLS 1.3.
    SSL3_DEBUG_MSG("CLIENT: Compat TLS 1.3 handshake.\n");
    // NB: The client key share packet is not to be hashed separately.
    Buffer save_handshake_messages = handshake_messages;
    handshake_messages = Buffer();
    Packet cksp = client_key_share_packet();
    handshake_messages = save_handshake_messages;
    send_packet(client_hello(server_name, ({ cksp })));
  } else {
    // Pre-TLS 1.3.
    SSL3_DEBUG_MSG("CLIENT: Legacy TLS handshake.\n");
    send_packet(client_hello(server_name));
  }
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

  if (version < PROTOCOL_TLS_1_3) {
    Packet key_exchange = client_key_exchange_packet();

    if (key_exchange) {
      send_packet(key_exchange);
    }

    if(cert) {
      // We sent a certificate, so we should send the verification.
      send_packet(certificate_verify_packet());
    }

    send_packet(change_cipher_packet());

  } else {

    if(cert) {
      // We sent a certificate, so we should send the verification.
      send_packet(certificate_verify_packet(SIGN_client_certificate_verify));
    }

    derive_master_secret(session->master_secret);
  }

  if(version == PROTOCOL_SSL_3_0)
    send_packet(finished_packet("CLNT"));
  else if(version >= PROTOCOL_TLS_1_0) {
    send_packet(finished_packet("client finished"));
    if (version >= PROTOCOL_TLS_1_3) {
      send_packet(change_cipher_packet());
      handle_change_cipher(1);
    }
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
  werror("SSL.ClientConnection: state %s, type %s\n",
	 fmt_constant(handshake_state, "STATE"),
         fmt_constant(type, "HANDSHAKE"));
  werror("sizeof(data)="+sizeof(data)+"\n");
#endif

  // Enforce packet ordering.
  COND_FATAL(type <= previous_handshake, ALERT_unexpected_message,
             "Invalid handshake packet order.\n");

  previous_handshake = type;

  switch(handshake_state)
  {
  default:
    error( "Internal error\n" );
  case STATE_wait_for_hello:
    add_handshake_message(raw);
    switch(type) {
    case HANDSHAKE_hello_retry_request:
      SSL3_DEBUG_MSG("SSL.ClientConnection: HELLO_RETRY_REQUEST\n");
      if (version >= PROTOCOL_TLS_1_3) {
	// The server didn't like our key shares.
	// Add the one requested.

	ProtocolVersion suggested_version =
	  [int(0..0)|ProtocolVersion]input->read_int(2);
	int suggested_suite = input->read_int(2);
	int suggested_group = input->read_int(2);

	string(8bit) extensions = input->read_hstring(2);
	// NB: We currently don't care about the extensions.
	extensions = 0;	// Silence warning.

	SSL3_DEBUG_MSG("Suggested: %s %s %s.\n",
		       fmt_version(suggested_version),
		       fmt_cipher_suite(suggested_suite),
		       fmt_constant(suggested_group, "GROUP"));

	if ((suggested_version >= PROTOCOL_TLS_1_3) &&
	    (suggested_version <= version) &&
	    has_value(context->preferred_suites, suggested_suite) &&
	    (has_value(context->ecc_curves, suggested_group) ||
	     has_value(context->ffdhe_groups, suggested_group))) {
	  // Valid parameters.
	  COND_FATAL(suggested_version < context->min_version,
                     ALERT_protocol_version,
                     "Unsupported protocol version.\n");

	  // Generate the suggested keyshare.
	  make_key_share_offer(suggested_group);

	  // Resend the hello.
	  // NB: No need for TLS 1.2 and earlier compat here, as we
	  //     know that the server supports TLS 1.3 and later.
	  send_packet(client_hello(session->server_name));
	  send_packet(client_key_share_packet());
	  handshake_state = STATE_wait_for_hello;
	  previous_handshake = 0;
	  break;
	}
	SSL3_DEBUG_MSG("Invalid suggestion.\n");
      } else {
	SSL3_DEBUG_MSG("Not valid in %s.\n", fmt_version(version));
      }
      // FALL_THROUGH
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

      if (((version & ~0xff) != PROTOCOL_SSL_3_0) ||
	  (version < context->min_version) ||
	  (version > client_version)) {
	SSL3_DEBUG_MSG("Unsupported version of SSL: %s (Requested %s).\n",
		       fmt_version(version), fmt_version(client_version));
	version = client_version;
	COND_FATAL(1, ALERT_protocol_version, "Unsupported version.\n");
      }
      if (client_version > version) {
	SSL3_DEBUG_MSG("Falling back client from %s to %s.\n",
		       fmt_version(client_version),
		       fmt_version(version));
      }

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
		     "cipher suite: %O\n"
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
                         (context->min_version >= PROTOCOL_TLS_1_3),
                         ALERT_illegal_parameter,
                         "Extended-master-secret: Invalid extension.\n");

	      SSL3_DEBUG_MSG("Extended-master-secret: Enabled.\n");
	      session->extended_master_secret = 1;
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
            COND_FATAL(1, ALERT_unsupported_extension,
                       "Unrequested extension.\n");
	  }
	}
      }

      // RFC 5746 3.5:
      // When a ServerHello is received, the client MUST verify that the
      // "renegotiation_info" extension is present; if it is not, the
      // client MUST abort the handshake.
      COND_FATAL(missing_secure_renegotiation, ALERT_handshake_failure,
                 "Missing secure renegotiation extension.\n");

      if ((session_id == session->identity) && sizeof(session_id)) {
	SSL3_DEBUG_MSG("Resuming session %O.\n", session_id);

	if (version < PROTOCOL_TLS_1_3) {
	  new_cipher_states();
	  send_packet(change_cipher_packet());
	} else {
	  derive_master_secret(session->master_secret);
	  send_packet(change_cipher_packet());
	  handle_change_cipher(1);
	}

	handshake_state = STATE_wait_for_finish;
	reuse = 1;
	break;
      }

      session->identity = session_id;
      if (version >= PROTOCOL_TLS_1_3) {
	handshake_state = STATE_wait_for_key_share;
      } else {
	handshake_state = STATE_wait_for_peer;
      }
      break;
    }
    break;

  case STATE_wait_for_key_share:
    if (version < PROTOCOL_TLS_1_3) {
      error("Waiting for key share in %s.\n", fmt_version(version));
    }
    add_handshake_message(raw);
    switch(type)
    {
    default:
      COND_FATAL(1, ALERT_unexpected_message, "Expected key share.\n");
      break;
    case HANDSHAKE_server_key_share:
      {
	int group = input->read_int(2);
	string(8bit) key_offer = input->read_hstring(2);

        // Not a group that we've offered.
	COND_FATAL(!(ke = keyshares[group]), ALERT_handshake_failure,
                   "Unknown key share offer.\n");

	string(8bit) premaster_secret;
	premaster_secret = ke->receive_key_share_offer(key_offer);
        COND_FATAL(!premaster_secret, ALERT_decode_error,
                   "Unable to decode key share offer.\n");

	// Derive the handshake master secret.
	derive_master_secret(premaster_secret);
	send_packet(change_cipher_packet());
	handle_change_cipher(1);

	if (session->cipher_spec->signature_alg == SIGNATURE_anonymous) {
	  handshake_state = STATE_wait_for_finish;
	} else {
	  handshake_state = STATE_wait_for_peer;
	}

	ke = UNDEFINED;

	// NB: From this point encryption is enabled on our side.
      }
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

	if (version >= PROTOCOL_TLS_1_3) {
	  handshake_state = STATE_wait_for_verify;
	}
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
      handshake_state = STATE_wait_for_finish;
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
	else if (version >= PROTOCOL_TLS_1_3) {
	  // NB: send_certs() sends the finished_packet for us.
	  if (reuse) {
	    // Derive the extended master secret.
	    derive_master_secret(session->master_secret);
	    send_packet(finished_packet("client finished"));
	    send_packet(change_cipher_packet());
	    handle_change_cipher(1);
	  } else if (send_certs()) return -1;
	}
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
