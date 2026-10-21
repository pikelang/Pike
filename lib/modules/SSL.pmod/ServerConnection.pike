#pike __REAL_VERSION__
#pragma strict_types
#require constant(SSL.Cipher)

//! Server-side connection state.

#include "tls.h"

import ".";
import Constants;
inherit Connection;

multiset(int) remote_extensions = (<>);

//! TLS 1.3 (@rfc{8446:4.1.4@}) HelloRetryRequest state: set once
//! we've sent one, since a server MUST NOT send more than one.
protected int(0..1) sent_hello_retry_request;

//! The group named in our HelloRetryRequest, if any -- the second
//! ClientHello's @[EXTENSION_key_share] must supply exactly this
//! group.
protected int hello_retry_group;

//! TLS 1.3 (@rfc{8446:4.2.9@}): true once we've seen the client's
//! ClientHello offer @[PSK_KE_psk_dhe_ke] in its
//! @[EXTENSION_psk_key_exchange_modes] -- gates whether we bother
//! issuing a @[HANDSHAKE_new_session_ticket] after the handshake
//! completes (a client that can't use one has no use for it).
protected int(0..1) client_offered_psk_dhe_ke;

//! TLS 1.3 (@rfc{8446:4.2.3@}): the signature scheme chosen alongside
//! @[session]->certificate_chain in @[reply_new_session_1_3()] (not
//! set when resuming via PSK, which sends no CertificateVerify at
//! all) -- remembered here since certificate/scheme selection and the
//! later @[certificate_verify_packet()] call happen in different
//! parts of that function.
protected int certificate_signature_scheme_1_3;

protected string _sprintf(int t)
{
  if (t == 'O') return sprintf("SSL.ServerConnection(%s)", describe_state());
}

//! Generate a @[HANDSHAKE_hello_request] @[Packet].
protected Packet hello_request()
{
  return handshake_packet(HANDSHAKE_hello_request, "");
}

protected Packet hello_verify_request(string(8bit) cookie)
{
  // FIXME: Dynamic DTLS version.
  return handshake_packet(HANDSHAKE_hello_verify_request,
			  sprintf("%2c%1H",
				  PROTOCOL_DTLS_1_0,
				  cookie));
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
  object(Alert)|zero fail;

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

  ext (EXTENSION_session_ticket, tickets_enabled) {
    // RFC 4507 and RFC 5077.
    SSL3_DEBUG_MSG("SSL.ServerConnection: Accepting session tickets.\n");
    return Buffer();
  };

  ext (EXTENSION_application_layer_protocol_negotiation, !!application_protocol)
  {
    return Buffer()->add_string_array(({application_protocol}), 1, 2);
  };

  if (fail) return [object]fail;

  // NB: Assume that the client understands extensions
  //     if it has sent extensions...
  if (sizeof(extensions))
      struct->add_hstring(extensions, 2);

  return handshake_packet(HANDSHAKE_server_hello, struct);
}

//! Initialize the KeyExchange @[ke], and generate a
//! @[HANDSHAKE_server_key_exchange] packet if the
//! key exchange needs one.
protected object(Packet)|zero server_key_exchange_packet()
{
  if (ke) error("KE!\n");
  ke = session->cipher_spec->ke_factory(context, session, this, client_version);
  if (!ke->init_server()) return 0;
  string(8bit)|zero data =
    ke->server_key_exchange_packet(client_random, server_random);
  return data && handshake_packet(HANDSHAKE_server_key_exchange, data);
}

protected Packet certificate_request_packet(Context context)
{
    /* Send a CertificateRequest message */
    Buffer struct = Buffer();
    struct->add_int_array(context->client_auth_methods, 1, 1);
    if (version >= PROTOCOL_TLS_1_2) {
      // TLS 1.2 has var_uint_array of hash and sign pairs here.
      struct->add_hstring(get_signature_algorithms(), 2);
    }
    struct->add_hstring([string(8bit)]
                        sprintf("%{%2H%}", context->authorities_cache), 2);
    return handshake_packet(HANDSHAKE_certificate_request, struct);
}

protected Packet new_session_ticket_packet(int lifetime_hint,
					   string(8bit) ticket)
{
  SSL3_DEBUG_MSG("SSL.ServerConnection: New session ticket.\n");
  Buffer struct = Buffer();
  if (lifetime_hint < 0) lifetime_hint = 0;
  struct->add_int(lifetime_hint, 4);
  struct->add_hstring(ticket, 2);
  return handshake_packet(HANDSHAKE_new_session_ticket, struct);
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

  object(Packet)|zero key_exchange = server_key_exchange_packet();

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

//! Build a TLS 1.3 @[HANDSHAKE_server_hello] @[Packet].
//! @param selected_psk
//!   True if we're resuming via the client's offered PSK identity 0
//!   (@[try_resume_psk_1_3()]) -- adds a @[EXTENSION_pre_shared_key]
//!   extension naming it (@rfc{8446:4.2.11@}: ServerHello's
//!   extension_data is just the 2-byte selected_identity index,
//!   always 0 here since we only ever consider one).
protected Packet server_hello_packet_1_3(string(8bit) session_id,
                                        int group,
                                        string(8bit) key_share_response,
                                        int(0..1)|void selected_psk)
{
  Buffer struct = Buffer();
  // RFC 8446 4.1.3: legacy_version is always {3,3}; the real version
  // travels in the supported_versions extension below.
  struct->add_int(PROTOCOL_TLS_1_2, 2);
  struct->add(server_random);
  // Compatibility mode (RFC 8446 D.4): echo the client's session id.
  struct->add_hstring(session_id, 1);
  struct->add_int(session->cipher_suite, 2);
  struct->add_int(COMPRESSION_null, 1);

  Buffer extensions = Buffer();
  extensions->add_int(EXTENSION_supported_versions, 2);
  extensions->add_hstring(sprintf("%2c", version), 2);

  extensions->add_int(EXTENSION_key_share, 2);
  Buffer key_share = Buffer();
  key_share->add_int(group, 2);
  key_share->add_hstring(key_share_response, 2);
  extensions->add_hstring(key_share, 2);

  if (selected_psk) {
    extensions->add_int(EXTENSION_pre_shared_key, 2);
    // selected_identity: always 0, the only index we ever offer.
    extensions->add_hstring("\0\0", 2);
  }

  struct->add_hstring(extensions, 2);
  return handshake_packet(HANDSHAKE_server_hello, struct);
}

//! Send a @[HANDSHAKE_hello_retry_request] (@rfc{8446:4.1.4@}) asking
//! the client to retry with a @[EXTENSION_key_share] for @[group], and
//! arrange to receive the resulting second ClientHello.
//!
//! RFC 8446 doesn't give HelloRetryRequest its own wire message type
//! -- it's a @[HANDSHAKE_server_hello] distinguished only by its
//! Random field (@[TLS13_HRR_RANDOM]) -- and RFC 8446 4.4.1's
//! synthetic @[HANDSHAKE_message_hash] transcript entry replaces the
//! first ClientHello (the caller's) in the transcript.
protected int(-1..0) send_hello_retry_request_1_3(string(8bit) session_id,
                                                 int suite, int group)
{
  // RFC 8446 4.4.1: replace ClientHello1 in the transcript with a
  // synthetic message_hash entry before anything else is hashed in.
  string(8bit) ch1_hash = transcript_hash();
  handshake_messages = Buffer();
  handshake_messages->add(sprintf("%1c%3H", HANDSHAKE_message_hash, ch1_hash));

  sent_hello_retry_request = 1;
  hello_retry_group = group;

  // The generic handshake-ordering check in handle_handshake() rejects
  // a second message of a type <= the previous one -- which a second
  // ClientHello (same HANDSHAKE_client_hello type as the first) always
  // is. Reset it, since RFC 8446 4.1.4 legitimately expects exactly
  // that here.
  previous_handshake = 0;

  Buffer struct = Buffer();
  // Same shape as server_hello_packet_1_3(), except for the Random
  // field and the key_share extension's body (see below).
  struct->add_int(PROTOCOL_TLS_1_2, 2);
  struct->add(TLS13_HRR_RANDOM);
  struct->add_hstring(session_id, 1);
  struct->add_int(suite, 2);
  struct->add_int(COMPRESSION_null, 1);

  Buffer extensions = Buffer();
  extensions->add_int(EXTENSION_supported_versions, 2);
  extensions->add_hstring(sprintf("%2c", version), 2);

  extensions->add_int(EXTENSION_key_share, 2);
  // RFC 8446 4.2.8: in HelloRetryRequest, key_share's extension_data
  // is a bare NamedGroup, not a full KeyShareEntry (group + key).
  extensions->add_hstring(sprintf("%2c", group), 2);

  struct->add_hstring(extensions, 2);

  send_packet_1_3(handshake_packet(HANDSHAKE_server_hello, struct));

  // Wait for the second ClientHello.
  handshake_state = STATE_wait_for_hello;
  return 0;
}

//! Build a TLS 1.3 @[HANDSHAKE_encrypted_extensions] @[Packet].
//!
//! @param early_data_accepted
//!   True to include the empty @[EXTENSION_early_data] accept
//!   acknowledgement (@rfc{8446:4.2.10@}).
//!
//! @note
//!   Minimal otherwise: no ALPN/SNI-ack/other extensions are
//!   negotiated here yet.
protected Packet encrypted_extensions_packet_1_3(int(0..1)|void
                                                early_data_accepted)
{
  Buffer extensions = Buffer();
  if (early_data_accepted) {
    extensions->add_int(EXTENSION_early_data, 2);
    extensions->add_hstring("", 2);
  }
  return handshake_packet(HANDSHAKE_encrypted_extensions,
                          Buffer()->add_hstring(extensions, 2));
}

//! Build a TLS 1.3 @[HANDSHAKE_certificate] @[Packet]
//! (@rfc{8446:4.4.2@} -- a different wire format from the TLS 1.2
//! and earlier @[certificate_packet()]: a certificate_request_context
//! prefix, and per-certificate extensions).
protected Packet certificate_packet_1_3(array(string(8bit)) certificates)
{
  Buffer struct = Buffer();
  // We never request post-handshake client auth, so our
  // certificate_request_context is always empty.
  struct->add_hstring("", 1);
  Buffer cert_list = Buffer();
  foreach(certificates, string(8bit) cert) {
    cert_list->add_hstring(cert, 3);
    cert_list->add_hstring("", 2); // No per-certificate extensions.
  }
  struct->add_hstring(cert_list, 3);
  return handshake_packet(HANDSHAKE_certificate, struct);
}

//! Build and send a TLS 1.3 (@rfc{8446:4.6.1@})
//! @[HANDSHAKE_new_session_ticket], carrying a resumption PSK derived
//! from @[key_schedule]'s @expr{resumption_master_secret@} -- a
//! distinct wire shape and semantics from the kept RFC 5077
//! @[new_session_ticket_packet()], which stays TLS <= 1.2-only.
//!
//! Issues exactly one ticket (with an arbitrary nonce, since RFC
//! 8446's ticket_nonce only needs to be unique among multiple tickets
//! sharing one resumption_master_secret).
protected void send_new_session_ticket_1_3()
{
  string(8bit) ticket_nonce = context->random(8);
  string(8bit) psk =
    key_schedule->expand_label(
      [string(8bit)]key_schedule->resumption_master_secret,
      "resumption", ticket_nonce, key_schedule->hash_size);

  // encode_ticket_1_3()'s own @note: it has no access to
  // connection-level ALPN state, so set it here first.
  session->tls13_alpn_protocol = application_protocol;
  array(string(8bit)|int)|zero ticket_info =
    context->encode_ticket_1_3(session, psk);
  if (!ticket_info) return;
  string(8bit) ticket = [string(8bit)]ticket_info[0];
  int lifetime = [int]ticket_info[1];
  int age_add = [int]ticket_info[2];

  Buffer struct = Buffer();
  struct->add_int(lifetime, 4);
  struct->add_int(age_add, 4);
  struct->add_hstring(ticket_nonce, 1);
  struct->add_hstring(ticket, 2);

  Buffer extensions = Buffer();
  if (context->accept_early_data && (context->max_early_data_size > 0)) {
    // RFC 8446 4.6.1: advertise the ticket's 0-RTT allowance -- kept
    // in sync with the same condition encode_ticket_1_3() used to set
    // session->tls13_max_early_data_size (above).
    extensions->add_int(EXTENSION_early_data, 2);
    extensions->add_hstring(sprintf("%4c", context->max_early_data_size), 2);
  }
  struct->add_hstring(extensions, 2);
  send_packet_1_3(handshake_packet(HANDSHAKE_new_session_ticket, struct));
}

//! TLS 1.3 (@rfc{8446:4.6.3@}): update our own sending keys, and
//! optionally ask the client to reciprocate with its own KeyUpdate.
//! May be called at any time once the handshake has completed.
//!
//! @seealso
//!   @[handle_key_update_1_3()]
void update_keys(int(0..1) request_peer_update)
{
  key_schedule->server_application_traffic_secret =
    send_key_update(request_peer_update,
                    [string(8bit)]key_schedule->server_application_traffic_secret);
}

//! TLS 1.3 (@rfc{8446:4.6.3@}) post-handshake KeyUpdate, received
//! while idle (@expr{STATE_wait_for_hello@}, which doubles as "idle"
//! once a 1.3 handshake has completed -- see its caller).
protected int(-1..0) handle_key_update_1_3(Buffer input)
{
  int request_update = input->read_int(1);
  COND_FATAL((request_update != KEY_UPDATE_not_requested) &&
             (request_update != KEY_UPDATE_requested),
             ALERT_illegal_parameter,
             "Invalid KeyUpdate request_update value.\n");

  // The generic handshake-ordering check in handle_handshake() rejects
  // a second message of a type <= the previous one -- which a second
  // KeyUpdate (same HANDSHAKE_key_update type as any earlier one)
  // always is. Reset it, same as send_hello_retry_request_1_3() does
  // for the analogous second-ClientHello case: unlike the TLS <= 1.2
  // handshake this check was designed for, post-handshake messages
  // like KeyUpdate can legitimately repeat.
  previous_handshake = 0;

  // RFC 8446 7.2/4.6.3: the receiver of a KeyUpdate updates its own
  // receiving keys immediately, applying to the next record it reads.
  key_schedule->client_application_traffic_secret =
    key_schedule->update_traffic_secret(
      [string(8bit)]key_schedule->client_application_traffic_secret);
  current_read_state =
    new_traffic_state([string(8bit)]key_schedule->client_application_traffic_secret, 0);

  // RFC 9846 4.7.3: if reciprocating would push our own KeyUpdate
  // count past the 2^48-1 cap, ignore update_requested rather than
  // violate it (the graceful path -- send_key_update()'s own check
  // is the hard backstop for paths with no "just don't" option).
  if ((request_update == KEY_UPDATE_requested) &&
      (key_update_count < (1<<48)-1)) {
    // update_keys() itself updates our sending keys too, as any
    // KeyUpdate send does -- not a second, redundant update.
    update_keys(KEY_UPDATE_not_requested);
  }
  return 0;
}

//! TLS 1.3 (@rfc{8446:4.2.11@}) PSK resumption: verify the client's
//! offered PSK identity 0 (if any) and its binder, and return the
//! cached @[Session] to resume from if everything checks out.
//!
//! On success, @[key_schedule] is created here (with its
//! @expr{early_secret@} already derived from the real PSK) --
//! @[reply_new_session_1_3()] must not create another one in that
//! case, just continue advancing the same object.
//!
//! @returns
//!   The @[Session] to resume, or @expr{0@} (zero) if resumption
//!   isn't possible or the binder doesn't check out -- in which case
//!   the caller must fall back to a full (certificate-based)
//!   handshake, exactly as if no PSK had been offered.
protected object(Session)|zero try_resume_psk_1_3(string(8bit) psk_identity,
                                                 string(8bit) psk_binder,
                                                 int binders_total_bytes,
                                                 array(int) psk_modes,
                                                 array(int) cipher_suites)
{
  if (!sizeof(psk_identity) || !has_value(psk_modes, PSK_KE_psk_dhe_ke))
    return 0;

  object(Session)|zero candidate = context->decode_ticket(psk_identity);
  if (!candidate || !candidate->tls13_psk ||
      !candidate->ticket_expiry_time ||
      (candidate->ticket_expiry_time <= time(1)) ||
      !has_value(cipher_suites, candidate->cipher_suite) ||
      !has_value(context->preferred_suites, candidate->cipher_suite)) {
    // No usable ticket, expired, or (RFC 8446 4.2.11: the PSK's hash
    // must match the selected cipher suite's) a suite we can't use
    // for resumption any more -- fall back to a full handshake.
    return 0;
  }

  array(int)|zero suite_info =
    [array(int)|zero]CIPHER_SUITES[candidate->cipher_suite];
  object(Crypto.Hash)|zero psk_hash =
    suite_info && [object(Crypto.Hash)]HASH_lookup[suite_info[2]];
  if (!psk_hash) return 0;

  // RFC 8446 4.2.11.2: the binder covers everything in this
  // ClientHello up to, but not including, the pre_shared_key
  // extension's binders field -- which, since that extension is
  // last, is just "the whole transcript so far, minus its own tail".
  string(8bit) full_transcript = (string(8bit))handshake_messages;
  if (binders_total_bytes <= 0 ||
      (binders_total_bytes >= sizeof(full_transcript)))
    return 0;
  string(8bit) truncated =
    full_transcript[..sizeof(full_transcript)-binders_total_bytes-1];
  string(8bit) truncated_hash = psk_hash->hash(truncated);

  KeySchedule ks = KeySchedule(psk_hash);
  ks->generate_early_secret([string(8bit)]candidate->tls13_psk);
  string(8bit) binder_key =
    ks->derive_secret([string(8bit)]ks->early_secret,
                      "res binder", psk_hash->hash(""));
  string(8bit) expected_binder = ks->verify_data(binder_key, truncated_hash);

  if (expected_binder != psk_binder) return 0;

  key_schedule = ks;
  return candidate;
}

//! RFC 8446 4.2.3: pick a signature scheme for @[cp] that @[peer_schemes]
//! (the peer's offered @[EXTENSION_signature_algorithms]) also accepts,
//! restricted to the schemes TLS 1.3 actually allows for a live
//! CertificateVerify (RSASSA-PSS or ECDSA; EdDSA isn't implemented,
//! see @[Cipher.sign_1_3()]'s @note).
//!
//! @returns
//!   The chosen @[SignatureScheme], or @expr{-1@} if none fit.
protected int select_signature_scheme_1_3(CertificatePair cp,
                                         array(int) peer_schemes)
{
  int family = [int]cp->sign_algs[0] & SIGNATURE_MASK;

  if (family == SIGNATURE_rsa) {
    // cp->sign_algs reflects whatever algorithm the certificate
    // itself happens to be signed with (typically plain
    // RSASSA-PKCS1-v1_5, eg from a default self-signed test cert) --
    // relevant for validating the *chain*, but RFC 8446 4.2.3
    // forbids PKCS1-v1_5 for the live CertificateVerify signature.
    // Any RSA key can sign with RSASSA-PSS instead, regardless of
    // how its certificate was itself signed, so build that candidate
    // list directly here rather than trusting cp->sign_algs.
    //
    // RFC 8017 9.1.1: emLen >= hLen + sLen + 2; this implementation's
    // (and Nettle's) PSS uses sLen == hLen, so emLen >= 2*hLen + 2 --
    // skip any hash that wouldn't fit the key's modulus (eg SHA-512
    // doesn't fit a 1024-bit RSA key).
    int key_bits = [int]cp->key->key_size();
    int em_len = (key_bits - 1 + 7) / 8;
    foreach(({ SIGNATURE_rsa_pss_sha512, SIGNATURE_rsa_pss_sha384,
               SIGNATURE_rsa_pss_sha256 }), int scheme) {
      if (!has_value(peer_schemes, scheme)) continue;
      int hash_bytes =
        [int]PSS_SIG_HASH[scheme & SIGNATURE_MASK]->digest_size();
      if (em_len >= 2*hash_bytes + 2) return scheme;
    }
    return -1;
  }

  // ECDSA (and, if it's ever implemented, EdDSA): cp->sign_algs
  // already names the exact scheme(s) the key/curve combination can
  // use, unlike RSA's PKCS1-vs-PSS wrinkle above.
  foreach([array(int)]cp->sign_algs, int scheme) {
    if (!has_value(peer_schemes, scheme)) continue;
    if ((scheme & SIGNATURE_MASK) == SIGNATURE_ecdsa) return scheme;
  }
  return -1;
}

//! TLS 1.3 (RFC 8446 4) full 1-RTT server handshake flow, replacing
//! @[reply_new_session()]/the legacy ServerKeyExchange/ClientKeyExchange
//! dance for @expr{version >= PROTOCOL_TLS_1_3@}.
//!
//! @note
//!   Client certificate authentication is not implemented yet -- see
//!   @tt{TLS13_PLAN.md@}. A group mismatch triggers a
//!   HelloRetryRequest (@rfc{8446:4.1.4@}, see
//!   @[send_hello_retry_request_1_3()]) rather than failing outright.
//!   A valid PSK offer (@[try_resume_psk_1_3()]) resumes instead of
//!   doing a full certificate-based handshake, per
//!   @rfc{8446:4.1.3@} (Certificate/CertificateVerify are then
//!   omitted entirely).
protected int(-1..0) reply_new_session_1_3(array(int) cipher_suites,
                                          string(8bit) session_id,
                                          mapping(int:string(8bit))
                                            client_key_shares,
                                          string(8bit) psk_identity,
                                          string(8bit) psk_binder,
                                          int binders_total_bytes,
                                          array(int) psk_modes,
                                          int(0..1) client_offered_early_data,
                                          int obfuscated_ticket_age)
{
  client_offered_psk_dhe_ke = has_value(psk_modes, PSK_KE_psk_dhe_ke);

  object(Session)|zero psk_session =
    try_resume_psk_1_3(psk_identity, psk_binder, binders_total_bytes,
                      psk_modes, cipher_suites);

  int suite;

  if (psk_session) {
    // RFC 8446 4.1.3: resuming via PSK -- no certificate exchange at
    // all, so no suite13/cert-compatibility filtering is needed
    // either: the suite is simply whichever one the original full
    // handshake settled on. key_schedule already exists (set by
    // try_resume_psk_1_3(), with early_secret already derived from the
    // real PSK) -- generate_handshake_secret() below continues it.
    suite = [int]psk_session->cipher_suite;
    int scs_ok = session->set_cipher_suite(suite, version,
                                           context->signature_algorithms,
                                           512);
    COND_FATAL(!scs_ok, ALERT_handshake_failure,
               "Unsupported cipher suite.\n");
    session->compression_algorithm = COMPRESSION_null;
    reuse = 1;
  } else {
    array(int) suite13 =
      filter([array(int)](context->preferred_suites & cipher_suites),
            lambda(int s) {
              array(int) info = [array(int)]CIPHER_SUITES[s];
              return info && (info[0] == KE_tls13);
            });
    COND_FATAL(!sizeof(suite13), ALERT_handshake_failure,
               "No common TLS 1.3 cipher suite.\n");
    suite = suite13[0];

    // Select a certificate *and* a signature scheme for it that the
    // peer also accepts (select_signature_scheme_1_3()). (Unlike TLS
    // 1.2 and earlier, suite selection and certificate selection are
    // independent in TLS 1.3 -- the suite only names the
    // record-protection AEAD+hash, see Cipher.pmod's KE_tls13; the
    // scheme instead has to be remembered here and threaded through
    // to certificate_verify_packet() below.)
    array(CertificatePair) certs = [array(CertificatePair)]
      (context->find_cert_domain(session->server_name) || ({}));
    object(CertificatePair)|zero cert;
    int signature_scheme_1_3 = -1;
    foreach(certs, CertificatePair cp) {
      int scheme =
        select_signature_scheme_1_3(cp, session->signature_algorithms);
      if (scheme != -1) {
        cert = cp;
        signature_scheme_1_3 = scheme;
        break;
      }
    }
    COND_FATAL(!cert, ALERT_handshake_failure,
               "No certificate compatible with the client's "
               "signature_algorithms.\n");

    int scs_ok = session->set_cipher_suite(suite, version,
                                           context->signature_algorithms,
                                           512);
    COND_FATAL(!scs_ok,
               ALERT_handshake_failure, "Unsupported cipher suite.\n");
    session->private_key = [object(Crypto.Sign.State)]cert->key;
    session->certificate_chain = cert->certs;
    session->compression_algorithm = COMPRESSION_null;
    certificate_signature_scheme_1_3 = signature_scheme_1_3;
  }

  // Select a group from the client's key_share list, in our order of
  // preference.
  int group = -1;
  string(8bit) peer_key = "";
  if (sent_hello_retry_request) {
    // RFC 8446 4.1.2: the second ClientHello's key_share MUST supply
    // exactly the group we asked for in the HelloRetryRequest.
    COND_FATAL(!client_key_shares[hello_retry_group],
               ALERT_illegal_parameter,
               "Second ClientHello didn't supply the key_share group "
               "requested by our HelloRetryRequest.\n");
    group = hello_retry_group;
    peer_key = [string(8bit)]client_key_shares[hello_retry_group];
  } else {
    foreach(context->ecc_curves, int g) {
      if (client_key_shares[g]) {
        group = g;
        peer_key = [string(8bit)]client_key_shares[g];
        break;
      }
    }
  }

  if (group == -1) {
    // RFC 8446 4.1.4: the client didn't offer a key_share for any
    // group we support. If it at least listed one we support among
    // its supported_groups, ask for it via a HelloRetryRequest --
    // but only once (a server MUST NOT send more than one).
    int retry_group = -1;
    foreach(context->ecc_curves, int g) {
      if (has_value(session->ecc_curves, g)) {
        retry_group = g;
        break;
      }
    }
    COND_FATAL(sent_hello_retry_request || (retry_group == -1),
               ALERT_handshake_failure,
               "No supported group in key_share, and no group left "
               "to retry with.\n");
    return send_hello_retry_request_1_3(session_id, suite, retry_group);
  }

  array(Gmp.mpz|string(8bit)) our_share =
    Cipher.generate_key_share(context, group);
  string(8bit)|zero shared_secret =
    Cipher.compute_key_share_secret(group, our_share[0], peer_key);
  COND_FATAL(!shared_secret, ALERT_decode_error, "Bad key_share.\n");

  // RFC 8446 2.3/4.2.10: decide whether to accept 0-RTT, before
  // server_hello_packet_1_3() (below) starts appending to the
  // transcript -- client_early_traffic_secret is derived from the
  // ClientHello-only transcript hash. Only ever possible on top of
  // PSK resumption, and never after a HelloRetryRequest (the client
  // MUST NOT re-offer early_data on a retry, and by construction
  // doesn't -- but reject defensively if it does anyway).
  int(0..1) accept_0rtt = 0;
  string(8bit) ch_hash = transcript_hash();
  if (psk_session && client_offered_early_data && !sent_hello_retry_request &&
      context->accept_early_data && (context->max_early_data_size > 0) &&
      ([int]psk_session->tls13_max_early_data_size > 0) &&
      (!psk_session->tls13_alpn_protocol ||
       ([string(8bit)]psk_session->tls13_alpn_protocol == application_protocol))) {
    // RFC 8446 4.2.10: demote to ordinary (non-0-RTT) resumption,
    // rather than failing the handshake, if the ticket's claimed age
    // doesn't match how old we think it actually is, within our
    // tolerance.
    int ticket_age_ms =
      (obfuscated_ticket_age - [int]psk_session->tls13_ticket_age_add) &
      0xffffffff;
    int actual_age_ms =
      (time(1) - [int]psk_session->tls13_ticket_issue_time) * 1000;
    accept_0rtt = (abs(ticket_age_ms - actual_age_ms) <=
                   context->early_data_ticket_age_tolerance * 1000);
  }

  if (accept_0rtt) {
    string(8bit) client_early_traffic_secret =
      key_schedule->derive_secret([string(8bit)]key_schedule->early_secret,
                                  "c e traffic", ch_hash);
    current_read_state = new_traffic_state(client_early_traffic_secret, 0);
    early_data_accepted_1_3 = 1;
    // Single-use ticket (RFC 8446 8.1): only purge now, after the
    // binder has already validated (try_resume_psk_1_3(), above) --
    // never before, since the ticket identity travels in cleartext
    // and a pre-validation purge would let an attacker burn a real
    // client's ticket with a forged ClientHello.
    context->purge_session(psk_session);
  } else if (client_offered_early_data) {
    // RFC 8446 2.3: we're declining 0-RTT, but the client may already
    // have queued early-data records optimistically -- tolerate (by
    // skipping, not fatally erroring on) ones that fail to decrypt,
    // up to our declared budget, until its real next flight arrives.
    skip_undecryptable_early_data_1_3 = 1;
    early_data_skip_budget_1_3 = context->max_early_data_size;
  }

  send_packet_1_3(server_hello_packet_1_3(session_id, group,
                                        [string(8bit)]our_share[1],
                                        !!psk_session));

  if (!psk_session) {
    Crypto.Hash hash = [object(Crypto.Hash)]session->cipher_spec->hash;
    key_schedule = KeySchedule(hash);
    key_schedule->generate_early_secret(0);
  }
  key_schedule->generate_handshake_secret([string(8bit)]shared_secret);

  string(8bit) hs_hash = transcript_hash();
  key_schedule->client_handshake_traffic_secret =
    key_schedule->derive_secret(key_schedule->handshake_secret,
                                "c hs traffic", hs_hash);
  key_schedule->server_handshake_traffic_secret =
    key_schedule->derive_secret(key_schedule->handshake_secret,
                                "s hs traffic", hs_hash);

  // From here on everything we send is encrypted under the server's
  // handshake traffic secret.
  current_write_state =
    new_traffic_state([string(8bit)]key_schedule->server_handshake_traffic_secret, 1);

  send_packet_1_3(encrypted_extensions_packet_1_3(accept_0rtt));
  if (!psk_session) {
    // RFC 8446 4.1.3: Certificate/CertificateVerify MUST be omitted
    // entirely when resuming via PSK.
    send_packet_1_3(certificate_packet_1_3(session->certificate_chain));
    send_packet_1_3(certificate_verify_packet(SIGN_server_certificate_verify,
                                             certificate_signature_scheme_1_3));
  }

  string(8bit) finished_hash = transcript_hash();
  string(8bit) verify_data =
    key_schedule->verify_data(
      [string(8bit)]key_schedule->server_handshake_traffic_secret,
      finished_hash);
  send_packet_1_3(handshake_packet(HANDSHAKE_finished, verify_data));
  server_verify_data = verify_data;

  if (!accept_0rtt) {
    // Ready to receive the client's (encrypted) Finished. If 0-RTT
    // was accepted instead, current_read_state is already set to
    // client_early_traffic_secret (above), and stays that way until
    // the client's HANDSHAKE_end_of_early_data switches it over (see
    // STATE_wait_for_finish).
    current_read_state =
      new_traffic_state([string(8bit)]key_schedule->client_handshake_traffic_secret, 0);
  }

  key_schedule->generate_master_secret();
  string(8bit) ap_hash = transcript_hash(); // Includes server Finished.
  key_schedule->client_application_traffic_secret =
    key_schedule->derive_secret([string(8bit)]key_schedule->master_secret,
                                "c ap traffic", ap_hash);
  key_schedule->server_application_traffic_secret =
    key_schedule->derive_secret([string(8bit)]key_schedule->master_secret,
                                "s ap traffic", ap_hash);
  key_schedule->exporter_master_secret =
    key_schedule->derive_secret([string(8bit)]key_schedule->master_secret,
                                "exp master", ap_hash);

  // Nothing more is sent under the handshake traffic secret.
  current_write_state =
    new_traffic_state([string(8bit)]key_schedule->server_application_traffic_secret, 1);

  handshake_state = STATE_wait_for_finish;
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
//!
//! @note
//!   On entry the handshake header has been removed from @[input].
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
    if ((version >= PROTOCOL_TLS_1_3) && (type == HANDSHAKE_key_update)) {
      // TLS 1.3 has no renegotiation (@rfc{8446:D.4@}) -- once a 1.3
      // handshake completes, this state just means "idle," and
      // KeyUpdate (@rfc{8446:4.6.3@}) is the only post-handshake
      // message a *client* can send us (NewSessionTicket is
      // server-to-client only). Handled separately from (and before)
      // the ClientHello-specific reset logic below, which doesn't
      // apply here.
      return handle_key_update_1_3(input);
    }
    {
      /* Reset all extra state variables */
      expect_change_cipher = certificate_state = 0;
      pending_read_state = pending_write_state = ({});
      ke = 0;
      // Each ClientHello gets its own duplicate-extension bookkeeping
      // -- without this reset, a second ClientHello (renegotiation,
      // or TLS 1.3's HelloRetryRequest retry) would spuriously trip
      // the "Same extension sent twice" check against the *previous*
      // ClientHello's extensions.
      remote_extensions = (<>);

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

      SSL3_DEBUG_MSG("SSL.ServerConnection: CLIENT_HELLO\n");

      client_version =
        [int(0x300..0x300)|ProtocolVersion]input->read_int(2);

      // TLS 1.3: Servers MAY abort the handshake upon receiving a
      // ClientHello with legacy_version 0x0304 or later.
      COND_FATAL(client_version > PROTOCOL_TLS_1_2, ALERT_protocol_version,
                 sprintf("Unsupported version %s.\n",
                         fmt_version(client_version)));

      array(int(16bit)) versions = context->get_versions(client_version);
      COND_FATAL(!sizeof(versions), ALERT_protocol_version,
                 sprintf("Unsupported version %s.\n",
                         fmt_version(client_version)));
      version = versions[0];
      SSL3_DEBUG_MSG("Got client version %s in ClientHello. Use %s.\n",
                     fmt_version(client_version), fmt_version(version));

      client_random = input->read(32);
      string(8bit) session_id = input->read_hstring(1);

      int cipher_len = input->read_int(2);
      COND_FATAL(cipher_len & 1, ALERT_unexpected_message,
                 "Invalid client_hello.\n");

      array(int) cipher_suites = input->read_ints(cipher_len/2, 2);
      array(int) client_cipher_suites = cipher_suites;

      array(int) compression_methods = input->read_int_array(1, 1);
      SSL3_DEBUG_MSG("STATE_wait_for_hello: received hello\n"
                     "version = %s\n"
                     "session_id=%O\n"
                     "cipher suites:\n%s\n"
                     "compression methods: %O\n",
                     fmt_version(client_version),
                     session_id, fmt_cipher_suites(cipher_suites),
                     compression_methods);

      int missing_secure_renegotiation = secure_renegotiation;
      mapping(int:string(8bit)) client_key_shares = ([]);
      // TLS 1.3 (RFC 8446 4.2.11) PSK resumption offer, if any -- see
      // try_resume_psk_1_3(). binders_total_bytes is the exact wire
      // byte count of the pre_shared_key extension's whole binders
      // field (2-byte list length prefix included), needed to
      // truncate the transcript for binder validation.
      string(8bit) psk_identity = "";
      string(8bit) psk_binder = "";
      int binders_total_bytes;
      // RFC 8446 4.2.11.1: the first offered identity's obfuscated
      // ticket age, reversed in reply_new_session_1_3()'s 0-RTT
      // freshness check.
      int obfuscated_ticket_age;
      array(int) psk_modes = ({});
      // RFC 8446 4.2.10: true if the client offered a bare (empty)
      // early_data extension -- independent of whether we go on to
      // accept 0-RTT.
      int(0..1) client_offered_early_data;
      if (sizeof(input))
      {
	Stdio.Buffer extensions = input->read_hbuffer(2);

        // FIXME: Control Safari workaround detection from the
        // context.
        int maybe_safari_10_8 = 1;

        while (sizeof(extensions)) {
          int extension_type = extensions->read_int(2);
          string(8bit) raw = extensions->read_hstring(2);
          Buffer extension_data = Buffer(raw);
          int size = sizeof(extension_data);
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
                           extension_type, size);
            continue;
          }

          switch(extension_type) {
          case EXTENSION_signature_algorithms:
            // RFC 5246
            string bytes = extension_data->read_hstring(2);
            COND_FATAL( sizeof(bytes)&1, ALERT_handshake_failure,
                        "Corrupt signature algorithms.\n" );
            // Pairs of <hash_alg, signature_alg>.
	    session->signature_algorithms =
	      [array(int)]column(map(bytes/2, array_sscanf, "%2c"), 0);
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

                string(8bit) host = server_name->read_hstring(2);
                COND_FATAL( host=="",
                            ALERT_unrecognized_name,
                            "Empty server name not allowed.\n" );
                session->server_name = host;

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
            COND_FATAL(size,
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
            COND_FATAL(size, ALERT_handshake_failure,
                       "Invalid NPN extension.\n");
            break;

	  case EXTENSION_session_ticket:
	    SSL3_DEBUG_MSG("SSL.ServerConnection: Got session ticket.\n");
            if (!context->offers_tickets()) {
              SSL3_DEBUG_MSG("SSL.ServerConnection: ...but !context->offers_tickets(), thus ignoring.\n");
              // Not consuming extension_data at all here -- continue,
              // not break, so the RFC 9846 4.3 "must consume all
              // data" check below doesn't misfire on an (unread)
              // non-empty ticket.
              continue;
            }
	    tickets_enabled = 1;
	    // NB: RFC 4507 and 5077 differ in encoding here.
	    //     Apparently no implementations actually followed
	    //     the RFC 4507 encoding.
	    session->ticket = extension_data->read();
	    break;

          case EXTENSION_signed_certificate_timestamp:
            COND_FATAL(size, ALERT_handshake_failure,
                       "Invalid signed certificate timestamp extension.\n");
            break;

          case EXTENSION_application_layer_protocol_negotiation:
            {
              application_protocol = 0;
              if( !context->advertised_protocols )
                // Not consuming extension_data at all here (we don't
                // care about ALPN) -- continue, not break, so the
                // RFC 9846 4.3 "must consume all data" check below
                // doesn't misfire on bytes we deliberately never
                // looked at.
                continue;
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
              foreach([array(string(8bit))]context->advertised_protocols;;
                      string(8bit) prot)
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
              COND_FATAL(!size ||
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
              COND_FATAL(size,
                         ALERT_illegal_parameter,
                         "Encrypt-then-MAC: Invalid extension.\n");

              SSL3_DEBUG_MSG("Encrypt-then-MAC: Tentatively enabled.\n");
              session->encrypt_then_mac = 1;
            }
            break;

          case EXTENSION_extended_master_secret:
            {
              COND_FATAL(size,
                         ALERT_illegal_parameter,
                         "Extended-master-secret: Invalid extension.\n");

              SSL3_DEBUG_MSG("Extended-master-secret: Enabled.\n");
              session->extended_master_secret = 1;
            }
            break;

          case EXTENSION_early_data:
            // RFC 8446 4.2.10: the ClientHello's early_data extension
            // carries no data of its own -- it's a bare flag.
            COND_FATAL(size, ALERT_illegal_parameter,
                       "early_data: Invalid extension.\n");
            client_offered_early_data = 1;
            break;

          case EXTENSION_padding:
            // RFC 7685: the padding bytes themselves are deliberately
            // never consumed (they're filler, not meaningful data) --
            // the RFC 9846 4.3 "must consume all extension_data" rule
            // below doesn't apply here, so skip it via continue.
            COND_FATAL(size &&
                       !equal(String.range((string)extension_data),({0,0})),
                       ALERT_illegal_parameter,
                       "Possible covert side channel in padding.\n");
            continue;

          case EXTENSION_supported_versions:
            {
              // RFC 8446 4.2.1: ClientHello carries a length-prefixed
              // vector (1-byte length, since max size 254 fits), not
              // a bare list -- unlike ServerHello's single bare
              // version (parsed on the client side; no vector there).
              COND_FATAL(size<1, ALERT_illegal_parameter,
                         "Illegal payload size in supported versions.\n");
              Stdio.Buffer list = extension_data->read_hbuffer(1);
              int list_size = sizeof(list);
              COND_FATAL(list_size<2 || (list_size&1) || sizeof(extension_data),
                         ALERT_illegal_parameter,
                         "Illegal payload size in supported versions.\n");
              versions = [array(int(16bit))]list->read_ints(list_size/2, 2);
              versions = context->supported_versions & versions;
              COND_FATAL(!sizeof(versions),
                   ALERT_handshake_failure, "No supported version!\n");
              version = versions[0];
            }
            break;

          case EXTENSION_key_share:
            {
              // RFC 8446 4.2.8: ClientHello KeyShareClientHello.
              Stdio.Buffer shares = extension_data->read_hbuffer(2);
              while (sizeof(shares)) {
                int g = shares->read_int(2);
                string(8bit) key = shares->read_hstring(2);
                client_key_shares[g] = key;
              }
            }
            break;

          case EXTENSION_psk_key_exchange_modes:
            // RFC 8446 4.2.9.
            psk_modes = extension_data->read_int_array(1, 1);
            break;

          case EXTENSION_pre_shared_key:
            {
              // RFC 8446 4.2.11: must be the last extension in
              // ClientHello; we don't enforce that, just parse it
              // wherever it appears. Only the first offered identity
              // is ever used (this implementation's scope is a single
              // resumption PSK per ClientHello -- see TLS13_PLAN.md).
              Stdio.Buffer identities = extension_data->read_hbuffer(2);
              array(string(8bit)) offered_identities = ({});
              array(int) offered_ticket_ages = ({});
              while (sizeof(identities)) {
                offered_identities += ({ identities->read_hstring(2) });
                offered_ticket_ages += ({ identities->read_int(4) });
              }
              int binders_list_len = extension_data->read_int(2);
              string(8bit) binders_raw = extension_data->read(binders_list_len);
              binders_total_bytes = 2 + binders_list_len;
              Buffer binders = Buffer(binders_raw);
              array(string(8bit)) offered_binders = ({});
              while (sizeof(binders)) {
                offered_binders += ({ binders->read_hstring(1) });
              }
              if (sizeof(offered_identities) && sizeof(offered_binders)) {
                psk_identity = offered_identities[0];
                psk_binder = offered_binders[0];
                obfuscated_ticket_age = offered_ticket_ages[0];
              }
            }
            break;

          default:
            SSL3_DEBUG_MSG("Unhandled extension %O.\n", extension_type);
            continue;
          }

          // RFC 9846 4.3: a recognized extension's own structure MUST
          // consume all of extension_data; anything left over is a
          // decode error, not silently ignored trailing bytes.
          COND_FATAL(sizeof(extension_data), ALERT_decode_error,
                     "Extraneous data in extension.\n");
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

      COND_FATAL(version==PROTOCOL_IN_EXTENSION,
                 ALERT_handshake_failure,
                 "Missing supported versions extension.\n");

      if (version >= PROTOCOL_TLS_1_3) {
        // TLS 1.3 negotiates key exchange, authentication, and
        // record protection entirely differently from TLS 1.2 and
        // earlier (no ChangeCipherSpec-driven rekeying, no
        // ServerKeyExchange/ClientKeyExchange, a different
        // Certificate wire format, encrypted post-ServerHello
        // messages, ...), so it gets its own self-contained flow
        // here rather than threading through the TLS 1.2-and-earlier
        // code below.
        return reply_new_session_1_3(cipher_suites, session_id,
                                    client_key_shares, psk_identity,
                                    psk_binder, binders_total_bytes,
                                    psk_modes, client_offered_early_data,
                                    obfuscated_ticket_age);
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
        // NB: client_version (the legacy ClientHello.client_version
        // field) can never reach a TLS 1.3 value -- see the matching
        // comment in ClientConnection.pike's client_hello() -- so
        // the comparison is capped at TLS 1.2 here too, or every
        // non-retry ClientHello from a TLS-1.3-capable client would
        // misidentify as an attempted fallback once our own max is
        // TLS 1.3 or later.
        COND_FATAL(client_version < min(max(@context->supported_versions),
                                        PROTOCOL_TLS_1_2),
                   ALERT_inappropriate_fallback,
                   "Too low client version.\n");
      }

      SSL3_DEBUG_MSG("ciphers: me:\n%s, client:\n%s",
                     fmt_cipher_suites(context->preferred_suites),
                     fmt_cipher_suites(cipher_suites));
      cipher_suites = [array(int)] context->preferred_suites & cipher_suites;
      SSL3_DEBUG_MSG("intersection:\n%s\n",
                     fmt_cipher_suites((array(int))cipher_suites));

      if (sizeof(cipher_suites)) {
        array(CertificatePair)|zero certs =
          context->find_cert_domain(session->server_name);

        ProtocolVersion orig_version = version;

        while (!session->select_cipher_suite(certs, cipher_suites, version))
        {
          if (sizeof(versions)>1) {
            // Try falling back to an older version of SSL/TLS.
            versions = versions[1..];
            version = versions[0];
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
	// No overlapping cipher suites, or obsolete cipher suite
	// selected, or incompatible certificates.
	// FIXME: Consider ALERT_insufficient_security
	//        (cf RFC 7465 section 2).
	COND_FATAL(1, ALERT_handshake_failure,
		   sprintf("No common suites! %s\n"
			   "Client suites:\n"
			   "%s\n",
			   fmt_version(version),
			   fmt_cipher_suites(client_cipher_suites)));
      }

      // TLS 1.2 or earlier.

      compression_methods =
	context->preferred_compressors & compression_methods;
      COND_FATAL(!sizeof(compression_methods),
		 ALERT_handshake_failure,
		 "Unsupported compression method.\n");

      session->set_compression_method(compression_methods[0]);

      object(Session)|zero old_session;
      if (tickets_enabled) {
	SSL3_DEBUG_MSG("SSL.ServerConnection: Decoding ticket: %O...\n",
		       session->ticket);
	old_session = sizeof(session->ticket) &&
	  context->decode_ticket(session->ticket);

	// RFC 4507 3.4:
	//   If a server is planning on issuing a SessionTicket to a
	//   client that does not present one, it SHOULD include an
	//   empty Session ID in the ServerHello. If the server
	//   includes a non-empty session ID, then it is indicating
	//   intent to use stateful session resume.
	//[...]
	//   If the server accepts the ticket and the Session ID is
	//   not empty, then it MUST respond with the same Session
	//   ID present in the ClientHello.

	session->identity = "";
	if (old_session) {
	  old_session->identity = session_id;
	}
      } else {
#ifdef SSL3_DEBUG
	if (sizeof(session_id))
	  werror("SSL.ServerConnection: Looking up session %O\n", session_id);
#endif
	old_session = sizeof(session_id) &&
	  context->lookup_session(session_id);
      }
      if (old_session && old_session->reusable_as(session))
      {
        SSL3_DEBUG_MSG("SSL.ServerConnection: Reusing session %O\n",
                       session_id);

        /* Reuse session */
        session = old_session;
        send_packet(server_hello_packet());

        if (tickets_enabled) {
          if( !session->ticket ) {
            SSL3_DEBUG_MSG("Missing ticket in session.\n");
          } else if( !session->ticket_expiry_time ) {
            SSL3_DEBUG_MSG("Missing ticket expiry time in session.\n");
          } else {
            SSL3_DEBUG_MSG("SSL.ServerConnection: Resending ticket.\n");
            int lifetime_hint = max(0, [int](session->ticket_expiry_time - time(1)));
            string(8bit) ticket = [string(8bit)]session->ticket;
            send_packet(new_session_ticket_packet(lifetime_hint, ticket));
          }
	}

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
  case STATE_wait_for_finish:
    {
      if ((version >= PROTOCOL_TLS_1_3) &&
          (type == HANDSHAKE_end_of_early_data)) {
        // RFC 8446 4.5: only legal when we accepted 0-RTT -- signals
        // the client is done sending early data and has switched to
        // its handshake traffic key for what follows (its real
        // Finished, still awaited in this same state).
        COND_FATAL(!early_data_accepted_1_3, ALERT_unexpected_message,
                   "Unexpected end_of_early_data.\n");
        COND_FATAL(sizeof(input), ALERT_decode_error,
                   "end_of_early_data: Invalid extension.\n");
        add_handshake_message(raw);
        current_read_state =
          new_traffic_state(
            [string(8bit)]key_schedule->client_handshake_traffic_secret, 0);
        return 0;
      }

      if (type != HANDSHAKE_finished)
        COND_FATAL(1, ALERT_unexpected_message, "Expected finished.\n");

      SSL3_DEBUG_MSG("SSL.ServerConnection: FINISHED\n");

      if (version >= PROTOCOL_TLS_1_3) {
        string(8bit) expected =
          key_schedule->verify_data(
            [string(8bit)]key_schedule->client_handshake_traffic_secret,
            transcript_hash());
        string(8bit) digest = input->read(sizeof(expected));
        COND_FATAL(digest != expected, ALERT_unexpected_message,
                   "Key exchange failure.\n");
        add_handshake_message(raw);
        client_verify_data = digest;

        // RFC 8446 7.1: resumption_master_secret is derived over the
        // transcript including the client's Finished.
        key_schedule->resumption_master_secret =
          key_schedule->derive_secret(
            [string(8bit)]key_schedule->master_secret,
            "res master", transcript_hash());

        // Nothing more is received under the handshake traffic secret.
        current_read_state =
          new_traffic_state(
            [string(8bit)]key_schedule->client_application_traffic_secret, 0);

        // RFC 8446 4.6.1: NewSessionTicket is sent after the
        // handshake completes, under the application traffic secret
        // (current_write_state already is that, set in
        // reply_new_session_1_3()) -- only useful to a client that
        // said it could use one.
        if (context->offers_tickets() && client_offered_psk_dhe_ke) {
          send_new_session_ticket_1_3();
        }

        handshake_messages = Buffer();
        handshake_state = STATE_wait_for_hello;
        return 1;
      }

      string(8bit) my_digest = "";
      string(8bit) digest = "";

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

      if (!reuse)
      {
        if (tickets_enabled) {
          array(string(8bit)|int)|zero ticket_info =
            context->encode_ticket(session);
          if (ticket_info) {
            SSL3_DEBUG_MSG("SSL.ServerConnection: Sending ticket %O.\n",
                           ticket_info);
            session->ticket = [string(8bit)](ticket_info[0]);
            session->ticket_expiry_time = [int](ticket_info[1] + time(1));
            send_packet(new_session_ticket_packet([int](ticket_info[1]),
                                                  [string(8bit)](ticket_info[0])));
          }
        }
        send_packet(change_cipher_packet());
        // We've already received the CCS from the peer.
        expect_change_cipher--;

        if(version == PROTOCOL_SSL_3_0)
          send_packet(finished_packet("SRVR"));
        else
          send_packet(finished_packet("server finished"));

        if (context->heartbleed_probe &&
            session->heartbeat_mode == HEARTBEAT_MODE_peer_allowed_to_send) {
          // Probe for the Heartbleed vulnerability (CVE-2014-0160).
          send_packet(heartbleed_packet());
        }
        context->record_session(session); /* Cache this session */
      }

      // Handshake hash is calculated for both directions above.
      handshake_messages = Buffer();

      handshake_state = STATE_wait_for_hello;

      return 1;
    }
    break;
  case STATE_wait_for_peer:
    // NB: ALERT_no_certificate can be valid in this state, and
    //     is handled directly by connection:handle_alert().
    switch(type)
    {
    default:
      COND_FATAL(1, ALERT_unexpected_message,
                 "Expected client KEX or cert.\n");
      break;
    case HANDSHAKE_client_key_exchange:
      SSL3_DEBUG_MSG("SSL.ServerConnection: CLIENT_KEY_EXCHANGE\n");

      // Certificate must be sent before key exchange message
      COND_FATAL((certificate_state == CERT_requested),
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
      if (validate_certificate_verify(input) < 0) {
	return -1;
      }

      add_handshake_message(raw);
      handshake_state = STATE_wait_for_finish;

      // We expect a CCS next.
      expect_change_cipher++;
    }
    break;
  }

  return 0;
}
