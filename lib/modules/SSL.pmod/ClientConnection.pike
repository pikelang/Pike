#pike __REAL_VERSION__
#pragma strict_types
#require constant(SSL.Cipher)

//! Client-side connection state.

#include "tls.h"

import ".";
import Constants;
inherit Connection;

//! A few storage variables for client certificate handling on the client side.
array(int)|zero client_cert_types;
array(string(8bit))|zero client_cert_distinguished_names;

//! TLS 1.3 and later: our ephemeral key share, set up in
//! @[client_hello()] and consumed once the server's key_share
//! response arrives in ServerHello.
protected int client_key_share_group;
protected Gmp.mpz|string(8bit)|zero client_key_share_secret;

//! TLS 1.3 (@rfc{8446:4.1.4@}): set once we've processed one
//! HelloRetryRequest, since a client MUST abort on a second one.
protected int(0..1) got_hello_retry_request;

//! TLS 1.3 (@rfc{8446:2.3@}/@rfc{8446:4.2.10@}): true once
//! @[client_hello()] has queued 0-RTT early-data records for this
//! connection (independent of whether the server ends up accepting
//! them -- see @[early_data_accepted_1_3]).
//!
//! @seealso
//!   @[early_data_write_state_1_3]
int(0..1) early_data_sent_1_3;

//! TLS 1.3: the @[State] (keyed from client_early_traffic_secret)
//! built in @[client_hello()] to send 0-RTT early-data records, kept
//! around until either reused once more to send
//! @[HANDSHAKE_end_of_early_data] (if the server accepted -- see
//! @rfc{8446:4.5@}) or simply discarded (if not). Reusing the exact
//! same object (rather than building a fresh one from the same
//! secret) matters: a fresh @[State] would restart its AEAD sequence
//! number at 0, reusing a nonce already used by an earlier early-data
//! chunk and desyncing the server's own (single, continuously
//! incrementing) read-state counter for this key.
protected object(State)|zero early_data_write_state_1_3;

protected string _sprintf(int t)
{
  if (t == 'O') return sprintf("SSL.ClientConnection(%s)", describe_state());
}

//! Generate a @[HANDSHAKE_client_hello] @[Packet].
//!
//! @param early_data
//!   TLS 1.3 (@rfc{8446:2.3@}) 0-RTT application data to send
//!   immediately after this ClientHello, if @[session] has a usable
//!   resumption ticket that supports it (see @[offering_psk] and
//!   @expr{session->tls13_max_early_data_size@} below) -- opportunistic:
//!   silently not sent as 0-RTT if the session can't support it (eg no
//!   ticket, or @[retry_group] is set -- @rfc{8446:4.1.2@} forbids
//!   re-offering @expr{early_data@} on a HelloRetryRequest retry). Never
//!   automatically resent as ordinary data if that happens; see
//!   @[early_data_sent_1_3].
//!
//! @param retry_group
//!   Set when (re)sending in response to a HelloRetryRequest
//!   (@rfc{8446:4.1.2@}): the @[EXTENSION_key_share] extension offers
//!   only this group, and -- per the RFC 8446 4.1.2 requirement that
//!   the retried ClientHello be unmodified except for a short list of
//!   fields -- @[client_random] is *not* regenerated, reusing the
//!   value from the original ClientHello.
protected Packet client_hello(string(8bit)|void server_name,
                              string(8bit)|void early_data,
                              int|void retry_group)
{
  Buffer struct = Buffer();
  /* Build client_hello message */
  client_version = version;
  // Clamp version to TLS 1.2. 1.3 and later are negotiated using
  // supported versions extension.
  struct->add_int(min(client_version, PROTOCOL_TLS_1_2), 2);

  if (!retry_group) {
    // The first four bytes of the client_random is specified to be
    // the timestamp on the client side. This is to guard against bad
    // random generators, where a client could produce the same
    // random numbers if the seed is reused. This argument is flawed,
    // since a broken random generator will make the connection
    // insecure anyways. The standard explicitly allows these bytes to
    // not be correct, so sending random data instead is safer and
    // reduces client fingerprinting.
    client_random = context->random(32);
  }

  struct->add(client_random);
  struct->add_hstring(session->identity || "", 1);

  array(int) cipher_suites = [array] context->preferred_suites;
  if ((state & CONNECTION_handshaking) && !secure_renegotiation) {
    // Initial handshake.
    // Use the backward-compat way of asking for
   // support for secure renegotiation.
    cipher_suites += ({ TLS_empty_renegotiation_info_scsv });

    if (client_version < min(max(@context->supported_versions),
                             PROTOCOL_TLS_1_2)) {
      // Negotiating a version lower than the max supported version.
      //
      // NB: client_version (the legacy ClientHello.client_version
      // field) is always clamped to at most TLS 1.2 -- TLS 1.3 and
      // later are negotiated via the supported_versions extension
      // instead, never via this field -- so comparing it directly
      // against max(@supported_versions) would misidentify *every*
      // non-retry ClientHello from a TLS-1.3-capable client as a
      // fallback, since client_version can never reach a TLS 1.3
      // value in the first place. Capping the comparison at TLS 1.2
      // restores the original RFC 7507 comparison for contexts whose
      // max is TLS 1.2 or earlier (unaffected by this change) while
      // fixing it for TLS 1.3-and-later ones.
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

  array(int) compression_methods = context->preferred_compressors;
  struct->add_int_array(compression_methods, 1, 1);

  // Only claim TLS 1.3 support (via the extensions below) if we're
  // actually offering a suite usable with it -- a context configured
  // for TLS 1.3 among other versions but restricted (via
  // preferred_suites) to eg TLS-1.2-and-earlier-only PSK suites has
  // nothing in common with a TLS 1.3 peer, and offering
  // supported_versions/key_share anyway would make the server (if it
  // also supports 1.3) select version 1.3 and then find no common
  // suite, failing a handshake that TLS 1.2 negotiation would have
  // completed fine.
  int offering_tls13 =
    has_value(context->supported_versions, PROTOCOL_TLS_1_3) &&
    sizeof(filter(cipher_suites,
                  lambda(int s) {
                    array(int) info = [array(int)]CIPHER_SUITES[s];
                    return info && (info[0] == KE_tls13);
                  }));

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
       ( has_value(context->supported_versions, PROTOCOL_TLS_1_0) ||
         has_value(context->supported_versions, PROTOCOL_TLS_1_1) ||
         has_value(context->supported_versions, PROTOCOL_TLS_1_2) )) {
    // RFC 7627
    // NB: This extension is implicit in TLS 1.3, and invalid in SSL 3.0.
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
    hostname->add_hstring([string]server_name, 2); // hostname

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

  ext (EXTENSION_supported_versions, offering_tls13)
  {
    // RFC 8446 4.2.1 ClientHello shape: a list of versions, 1-byte
    // length prefix (unlike ServerHello's single bare version).
    Buffer versions = Buffer();
    foreach(context->supported_versions, int v)
      versions->add_int(v, 2);
    return Buffer()->add_hstring(versions, 1);
  };

  ext (EXTENSION_key_share, offering_tls13)
  {
    // RFC 8446 4.2.8: offer exactly one key share -- our first choice
    // of group, normally, or (see the @[retry_group] parameter above)
    // whichever single group a HelloRetryRequest asked for.
    client_key_share_group = retry_group || context->ecc_curves[0];
    array(Gmp.mpz|string(8bit)) share =
      Cipher.generate_key_share(context, client_key_share_group);
    client_key_share_secret = share[0];
    Buffer entry = Buffer();
    entry->add_int(client_key_share_group, 2);
    entry->add_hstring([string(8bit)]share[1], 2);
    return Buffer()->add_hstring(entry, 2);
  };

  ext (EXTENSION_psk_key_exchange_modes, offering_tls13)
  {
    // RFC 8446 4.2.9: we only ever offer psk_dhe_ke -- a PSK is
    // always combined with a fresh (EC)DHE key_share, never used
    // alone (see TLS13_PLAN.md).
    return Buffer()->add_int_array(({ PSK_KE_psk_dhe_ke }), 1, 1);
  };

  // RFC 8446 4.2.11: a resumable ticket from a prior connection using
  // this same Session object (see ClientConnection.create()'s
  // session-reuse parameter).
  int(0..1) offering_psk =
    context->extensions[EXTENSION_pre_shared_key] && offering_tls13 &&
    session->ticket && session->tls13_psk &&
    (session->ticket_expiry_time > time(1));

  object(Crypto.Hash)|zero psk_hash;
  // Hoisted out of the "if (offering_psk)" block below (which builds
  // the pre_shared_key extension itself) so it can also be reused,
  // once that block is done, for the client_early_traffic_secret
  // derivation further down -- see offering_early_data.
  object(KeySchedule)|zero psk_key_schedule_1_3;
  if (offering_psk) {
    array(int)|zero suite_info =
      [array(int)|zero]CIPHER_SUITES[session->cipher_suite];
    psk_hash = suite_info && [object(Crypto.Hash)]HASH_lookup[suite_info[2]];
    // A ticket's PSK can only be used with a suite sharing its hash
    // (RFC 8446 4.2.11) -- simplify to requiring the exact original
    // suite still be among what we're offering this time.
    if (!psk_hash || !has_value(cipher_suites, session->cipher_suite))
      offering_psk = 0;
  }

  // RFC 8446 2.3/4.2.10: only offer 0-RTT alongside a PSK that
  // supports it (no PSK, no early_secret to derive
  // client_early_traffic_secret from), never on a HelloRetryRequest
  // retry (4.1.2 forbids re-offering early_data there), and never
  // more than the ticket's own advertised max_early_data_size allows.
  int(0..1) offering_early_data =
    offering_psk && early_data && sizeof([string(8bit)]early_data) &&
    !retry_group && (session->tls13_max_early_data_size > 0);

  // When the client HELLO packet data is in the range 256-511 bytes
  // f5 SSL terminators will intepret it as SSL2 requiring an
  // additional 8k of data, which will cause the connection to hang.
  // The solution is to pad the package to more than 511 bytes using a
  // dummy exentsion.
  // Reference: draft-agl-tls-padding
  int packet_size = sizeof(struct)+sizeof(extensions)+2;
  ext (EXTENSION_padding, packet_size>255 && packet_size<512)
  {
    int(0..) padding = max(0, 512-packet_size-4);
    SSL3_DEBUG_MSG("SSL.ClientConnection: Adding %d bytes of padding.\n",
                   padding);
    return Buffer()->add("\0"*padding);
  };

  // RFC 8446 4.2.10: this extension's body is empty in ClientHello --
  // the early-data records themselves travel as separate
  // application_data records sent right after this ClientHello (see
  // offering_early_data's use below, after the ClientHello itself is
  // built and sent), encrypted under the client's early traffic
  // secret, not stuffed into the extension body.
  ext(EXTENSION_early_data, offering_early_data) {
    return Buffer();
  };

  // NB: WebSphere Application Server 7.0 doesn't like having an
  //     empty extension last, so don't put any such extensions
  //     in the list here.
  //     cf https://bugs.chromium.org/p/chromium/issues/detail?id=363583#c17

  if (offering_psk) {
    // RFC 8446 4.2.11/4.2.11.2: pre_shared_key MUST be the last
    // extension, and its binder is an HMAC over everything that comes
    // before it -- including the rest of this very extension, just
    // not the binder bytes themselves. Built by hand (not via ext()
    // above, which can't do this two-step dance): first with a
    // correctly-sized all-zero placeholder binder, so the hash input
    // can be assembled and measured, then patched with the real
    // value, all before anything is committed to handshake_messages
    // (that only happens once, below, in the real handshake_packet()
    // call).
    int(0..) binder_len = [int(0..)]psk_hash->digest_size();
    int obfuscated_age =
      (((time(1) - session->tls13_ticket_issue_time) * 1000) +
       session->tls13_ticket_age_add) & 0xffffffff;

    Buffer identities = Buffer();
    identities->add_hstring([string(8bit)]session->ticket, 2);
    identities->add_int(obfuscated_age, 4);

    Buffer binders = Buffer();
    binders->add_hstring("\0" * binder_len, 1);

    Buffer psk_ext = Buffer();
    psk_ext->add_hstring(identities, 2);
    psk_ext->add_hstring(binders, 2);

    extensions->add_int(EXTENSION_pre_shared_key, 2);
    extensions->add_hstring(psk_ext, 2);
    struct->add_hstring(extensions, 2);

    string(8bit) placeholder_wire =
      sprintf("%1c%3H", HANDSHAKE_client_hello, (string(8bit))struct);
    // 2-byte binders-list length prefix + 1-byte entry length prefix
    // + the (placeholder) binder itself.
    int binders_field_bytes = 3 + binder_len;
    string(8bit) truncated =
      placeholder_wire[..sizeof(placeholder_wire)-binders_field_bytes-1];
    string(8bit) truncated_hash =
      [string(8bit)]psk_hash->hash((string(8bit))handshake_messages + truncated);

    // Assigned (not declared) here: hoisted to function scope so it
    // can be reused below, once the ClientHello itself has been sent,
    // for the client_early_traffic_secret derivation (same early_secret,
    // same hash -- see offering_early_data's doc comment above).
    psk_key_schedule_1_3 = KeySchedule([object(Crypto.Hash)]psk_hash);
    psk_key_schedule_1_3->generate_early_secret([string(8bit)]session->tls13_psk);
    string(8bit) binder_key =
      psk_key_schedule_1_3->derive_secret(
        [string(8bit)]psk_key_schedule_1_3->early_secret,
        "res binder", [string(8bit)]psk_hash->hash(""));
    string(8bit) binder =
      psk_key_schedule_1_3->verify_data(binder_key, truncated_hash);

    // Patch in the real binder (only the raw bytes -- the length
    // prefixes around it are already correctly sized).
    string(8bit) patched =
      ((string(8bit))struct)[..sizeof(struct)-binder_len-1] + binder;
    struct = Buffer(patched);
  } else if (sizeof(extensions) && (version >= PROTOCOL_TLS_1_0)) {
    struct->add_hstring(extensions, 2);
  }

  SSL3_DEBUG_MSG("SSL.ClientConnection: Client hello: %q\n", struct);
  Packet ret = handshake_packet(HANDSHAKE_client_hello, struct);

  if (offering_early_data) {
    // RFC 8446 7.1: client_early_traffic_secret = Derive-Secret(
    // early_secret, "c e traffic", Hash(ClientHello1)) -- computed
    // only now, after handshake_packet() above has committed the
    // final (binder-patched) ClientHello bytes to handshake_messages.
    string(8bit) ch_hash =
      [string(8bit)]psk_hash->hash((string(8bit))handshake_messages);
    string(8bit) client_early_traffic_secret =
      psk_key_schedule_1_3->derive_secret(
        [string(8bit)]psk_key_schedule_1_3->early_secret,
        "c e traffic", ch_hash);

    // One State for every early-data record: rebuilding per chunk
    // would reset the AEAD nonce counter and reuse a nonce, a real
    // crypto bug.
    State early_write_state =
      new_traffic_state(client_early_traffic_secret, 1, psk_key_schedule_1_3);

    string(8bit) remaining = [string(8bit)]early_data;
    if (session->tls13_max_early_data_size &&
        (sizeof(remaining) > session->tls13_max_early_data_size))
      remaining = remaining[..session->tls13_max_early_data_size-1];
    while (sizeof(remaining)) {
      string(8bit) chunk = remaining[..session->max_packet_size-1];
      remaining = remaining[sizeof(chunk)..];
      Packet p = Packet(version, PACKET_application_data, chunk);
      p->bound_write_state = early_write_state;
      send_packet(p, PRI_application);
    }

    early_data_sent_1_3 = 1;
    early_data_write_state_1_3 = early_write_state;
  }

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

protected object(Packet)|zero client_key_exchange_packet()
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
  string(8bit)|zero premaster_secret =
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
//!
//! @param early_data
//!   TLS 1.3 (@rfc{8446:2.3@}) 0-RTT application data to send
//!   immediately, opportunistically -- see @[client_hello()]'s own
//!   parameter doc for exactly when this is (and isn't) actually sent
//!   as 0-RTT.
protected void create(Context ctx, string(8bit)|void server_name,
                      Session|void session, string(8bit)|void early_data)
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
  send_packet(client_hello(server_name, early_data));
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
  object(CertificatePair)|zero cert;

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
               foreach([array] cp->sign_algs, int cert_scheme)
               {
                 int match;
                 foreach(session->signature_algorithms, int signature_scheme)
                 {
                   if( cert_scheme == signature_scheme )
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

  object(Packet)|zero key_exchange = client_key_exchange_packet();

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
    session->signature_algorithms =
      [array(int)]column(map(bytes/2, array_sscanf, "%2c"), 0);
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
  tickets_enabled = 0;

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

//! TLS 1.3 (@rfc{8446:4.6.1@}) analogue of @[got_new_session_ticket()]
//! -- a @[HANDSHAKE_new_session_ticket] received post-handshake
//! (@expr{handshake_state == STATE_handshake_finished@}), carrying a
//! resumption PSK rather than TLS 1.0-1.2's master_secret-based
//! ticket.
//!
//! Stores the ticket and derived PSK on @[session], so that a future
//! @[ClientConnection] created with this same @expr{Session@} object
//! (see @[create()]'s @expr{session@} parameter) can offer it for
//! resumption via @[client_hello()].
protected void got_new_session_ticket_1_3(Buffer input)
{
  // RFC 9846 4.6.1: a client that doesn't support resumption (ie
  // never offers pre_shared_key) MUST ignore a NewSessionTicket. We
  // always advertise psk_dhe_ke whenever offering TLS 1.3 at all, so
  // this is unreachable in practice -- a defensive gate only, in case
  // a caller ever disables that extension on an otherwise-1.3 context.
  // Still fully drain input: the generic post-handshake dispatch
  // treats any leftover bytes as a protocol error, "ignore" or not.
  if (!context->extensions[EXTENSION_pre_shared_key]) {
    input->read(sizeof(input));
    return;
  }

  int lifetime = input->read_int(4);
  int age_add = input->read_int(4);
  string(8bit) ticket_nonce = input->read_hstring(1);
  string(8bit) ticket = input->read_hstring(2);
  Buffer extensions = Buffer(input->read_hstring(2));

  int max_early_data_size;
  while (sizeof(extensions)) {
    int extension_type = extensions->read_int(2);
    Buffer extension_data = Buffer(extensions->read_hstring(2));
    if (extension_type == EXTENSION_early_data) {
      // RFC 8446 4.6.1: max_early_data_size, a bare uint32.
      max_early_data_size = extension_data->read_int(4);
      // RFC 9846 4.3: extension structure must consume all its data.
      // COND_FATAL doesn't fit here (this function returns void, not
      // an Alert-compatible int) -- inlined by hand instead.
      if (sizeof(extension_data)) {
        send_packet(alert(ALERT_fatal, ALERT_decode_error,
                          "Extraneous data in extension.\n"));
        return;
      }
    }
    // Other NewSessionTicket extensions: none else defined by RFC 8446.
  }

  string(8bit) psk =
    key_schedule->expand_label(
      [string(8bit)]key_schedule->resumption_master_secret,
      "resumption", ticket_nonce, key_schedule->hash_size);

  session->ticket = ticket;
  session->ticket_expiry_time = time(1) + lifetime;
  session->tls13_psk = psk;
  session->tls13_ticket_age_add = age_add;
  session->tls13_ticket_issue_time = time(1);
  session->tls13_max_early_data_size = max_early_data_size;
}

//! TLS 1.3 (@rfc{8446:4.6.3@}): update our own sending keys, and
//! optionally ask the server to reciprocate with its own KeyUpdate.
//! May be called at any time once the handshake has completed.
//!
//! @seealso
//!   @[handle_key_update_1_3()]
void update_keys(int(0..1) request_peer_update)
{
  key_schedule->client_application_traffic_secret =
    send_key_update(request_peer_update,
                    [string(8bit)]key_schedule->client_application_traffic_secret);
}

//! TLS 1.3 (@rfc{8446:4.6.3@}) post-handshake KeyUpdate, received
//! while idle (@expr{STATE_handshake_finished@}).
protected int(-1..0) handle_key_update_1_3(Buffer input)
{
  int request_update = input->read_int(1);
  COND_FATAL((request_update != KEY_UPDATE_not_requested) &&
             (request_update != KEY_UPDATE_requested),
             ALERT_illegal_parameter,
             "Invalid KeyUpdate request_update value.\n");

  // RFC 8446 7.2/4.6.3: the receiver of a KeyUpdate updates its own
  // receiving keys immediately, applying to the next record it reads.
  key_schedule->server_application_traffic_secret =
    key_schedule->update_traffic_secret(
      [string(8bit)]key_schedule->server_application_traffic_secret);
  current_read_state =
    new_traffic_state([string(8bit)]key_schedule->server_application_traffic_secret, 0);

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

//! TLS 1.3 (@rfc{8446:4.1.4@}) HelloRetryRequest handling, invoked
//! from @expr{STATE_wait_for_hello@}'s @[HANDSHAKE_server_hello] case
//! when the Random field equals @[TLS13_HRR_RANDOM] -- RFC 8446 gives
//! HelloRetryRequest no wire message type of its own, so that's the
//! only way to recognize one.
//!
//! @param cipher_suite
//!   Already read off @[input] by the caller (a HelloRetryRequest
//!   shares @[HANDSHAKE_server_hello]'s wire shape up to and including
//!   this field) -- needed here only to look up the transcript hash
//!   algorithm, since the real @expr{session->cipher_spec@} isn't set
//!   up until the real ServerHello that (hopefully) follows.
//! @param input
//!   The rest of the HelloRetryRequest body (just the extensions --
//!   the caller has already consumed everything up to and including
//!   legacy_compression_method), not yet parsed.
//! @param raw
//!   The raw packet; added to the transcript by this function (after
//!   the message_hash substitution below) rather than by the caller.
protected int(-1..0) handle_hello_retry_request_1_3(int cipher_suite,
                                                   Buffer input,
                                                   Stdio.Buffer raw)
{
  // RFC 8446 4.1.4: "clients MUST abort the handshake with an
  // "unexpected_message" alert" on a second HelloRetryRequest.
  COND_FATAL(got_hello_retry_request, ALERT_unexpected_message,
             "Received a second HelloRetryRequest.\n");
  got_hello_retry_request = 1;

  // NB: legacy_compression_method has already been read off input by
  // our caller (the shared prefix of ServerHello/HelloRetryRequest
  // parsing) -- only the extensions remain.

  array(int)|zero suite_info = [array(int)|zero]CIPHER_SUITES[cipher_suite];
  object(Crypto.Hash)|zero hash =
    suite_info && [object(Crypto.Hash)]HASH_lookup[suite_info[2]];
  COND_FATAL(!hash, ALERT_handshake_failure,
             "Unsupported cipher suite in HelloRetryRequest.\n");

  int retry_group = -1;
  if (sizeof(input)) {
    Stdio.Buffer extensions = input->read_hbuffer(2);
    while (sizeof(extensions)) {
      int extension_type = extensions->read_int(2);
      Buffer extension_data = Buffer(extensions->read_hstring(2));
      if (extension_type == EXTENSION_key_share) {
        // RFC 8446 4.2.8: HelloRetryRequest's key_share extension_data
        // is a bare NamedGroup, not a full KeyShareEntry.
        retry_group = extension_data->read_int(2);
        // RFC 9846 4.3: extension structure must consume all its data.
        COND_FATAL(sizeof(extension_data), ALERT_decode_error,
                  "Extraneous data in extension.\n");
      }
      // NB: supported_versions may also legitimately appear (echoing
      // 1.3); nothing else is expected or acted on -- its own content
      // isn't parsed here at all, so there's nothing to validate.
    }
  }

  COND_FATAL(retry_group == -1 || !has_value(context->ecc_curves, retry_group),
             ALERT_illegal_parameter,
             "HelloRetryRequest named a group we didn't offer.\n");

  // RFC 8446 4.4.1: replace ClientHello1 in the transcript with a
  // synthetic message_hash entry, then hash in the actual
  // HelloRetryRequest bytes after it (unlike ClientHello1, the HRR
  // itself *is* hashed normally).
  string(8bit) ch1_hash = hash->hash((string(8bit))handshake_messages);
  handshake_messages = Buffer();
  handshake_messages->add(sprintf("%1c%3H", HANDSHAKE_message_hash, ch1_hash));
  add_handshake_message(raw);

  // RFC 8446 4.1.2: resend an otherwise-unmodified ClientHello with a
  // key_share for exactly the requested group.
  send_packet(client_hello(session->server_name, UNDEFINED, retry_group));
  return 0;
}

//! TLS 1.3 (RFC 8446 4) continuation of ClientHello/ServerHello
//! processing for @expr{version >= PROTOCOL_TLS_1_3@}, replacing the
//! resumption-id/ServerKeyExchange-oriented code in
//! @expr{STATE_wait_for_hello@} for TLS 1.2 and earlier.
//!
//! @note
//!   Client certificate authentication is not implemented yet -- see
//!   @tt{TLS13_PLAN.md@}. A HelloRetryRequest (@rfc{8446:4.1.4@})
//!   received before this real ServerHello is handled separately, in
//!   @[handle_hello_retry_request_1_3()].
protected int(-1..0) got_server_hello_1_3(int server_key_share_group,
                                        string(8bit) server_key_share_key,
                                        int(0..1) selected_psk)
{
  // The earlier session->set_cipher_suite() call (above, in the TLS
  // 1.2-and-earlier-shaped code) ran before the supported_versions
  // extension had updated `version` from the ServerHello legacy
  // field (always {3,3}) to the real negotiated version, so the
  // resulting CipherSpec has TLS 1.2-shaped AEAD iv_size/
  // explicit_iv_size (Cipher.pmod's TLS 1.3 override is gated on
  // version). Redo it now that `version` is correct.
  COND_FATAL(!session->set_cipher_suite(session->cipher_suite, version,
                                        context->signature_algorithms, 512),
             ALERT_handshake_failure, "Unsupported cipher suite.\n");

  // NB: Not "the group we didn't offer" -- client_key_share_group is
  // updated by client_hello() on a HelloRetryRequest retry, so by
  // this point it names whatever group we most recently offered.
  COND_FATAL(server_key_share_group != client_key_share_group,
             ALERT_handshake_failure,
             "Server's final key_share group doesn't match what we "
             "most recently offered.\n");

  string(8bit)|zero shared_secret =
    Cipher.compute_key_share_secret(client_key_share_group,
                                    client_key_share_secret,
                                    server_key_share_key);
  COND_FATAL(!shared_secret, ALERT_decode_error, "Bad key_share.\n");

  // NB: Certificate/CertificateVerify simply won't arrive below when
  // resuming (RFC 8446 4.1.3 has the server omit them entirely), so
  // no special-casing is needed in handle_handshake_1_3() for that --
  // this just mirrors the TLS <= 1.2 meaning of reuse (abbreviated
  // handshake, no fresh certificate exchange).
  reuse = selected_psk;

  Crypto.Hash hash = [object(Crypto.Hash)]session->cipher_spec->hash;
  key_schedule = KeySchedule(hash);
  // RFC 8446 7.1: Early Secret is always derived, either from the
  // real PSK (if the server echoed our offered identity) or an
  // all-zero one (if not, or we didn't offer one at all).
  key_schedule->generate_early_secret(selected_psk &&
                                      [string(8bit)]session->tls13_psk);
  key_schedule->generate_handshake_secret([string(8bit)]shared_secret);

  string(8bit) hs_hash = transcript_hash(); // Hash(ClientHello..ServerHello).
  key_schedule->client_handshake_traffic_secret =
    key_schedule->derive_secret(key_schedule->handshake_secret,
                                "c hs traffic", hs_hash);
  key_schedule->server_handshake_traffic_secret =
    key_schedule->derive_secret(key_schedule->handshake_secret,
                                "s hs traffic", hs_hash);

  // Ready to decrypt EncryptedExtensions/Certificate/CertificateVerify/
  // Finished.
  current_read_state =
    new_traffic_state([string(8bit)]key_schedule->server_handshake_traffic_secret, 0);

  handshake_state = STATE_wait_for_peer;
  return 0;
}

//! TLS 1.3 (RFC 8446 4) handling of the server's post-ServerHello
//! flight, called from @expr{STATE_wait_for_peer@} for
//! @expr{version >= PROTOCOL_TLS_1_3@}.
int(-1..1) handle_handshake_1_3(int type, Buffer input, Stdio.Buffer raw)
{
  switch (type) {
  default:
    COND_FATAL(1, ALERT_unexpected_message,
              sprintf("Unexpected server message: %s.\n",
                      fmt_constant(type, "HANDSHAKE")));
    break;

  case HANDSHAKE_encrypted_extensions:
    {
      // Minimal for now: nothing except early_data is acted on here
      // (ALPN/SNI-ack/other extensions aren't yet) -- but the
      // (possibly empty) extensions vector must still be fully
      // consumed, or the generic post-dispatch "extraneous data"
      // check in Connection.pike's handshake loop will reject it.
      Buffer extensions = Buffer(input->read_hstring(2));
      while (sizeof(extensions)) {
        int extension_type = extensions->read_int(2);
        Buffer extension_data = Buffer(extensions->read_hstring(2));
        if (extension_type == EXTENSION_early_data) {
          // RFC 8446 4.2.10: this ack's extension_data is empty; its
          // mere presence is what matters. RFC 8446 4.5: whether we
          // send EndOfEarlyData is driven *solely* by this ack, so
          // only honor it if we actually offered early_data in the
          // first place (a server acking data we never sent would be
          // a protocol violation we simply ignore rather than act on).
          COND_FATAL(sizeof(extension_data), ALERT_illegal_parameter,
                    "early_data acknowledgement must be empty.\n");
          early_data_accepted_1_3 = early_data_sent_1_3;
        }
        // Other EncryptedExtensions contents (ALPN-ack etc.) unhandled,
        // as before.
      }
      add_handshake_message(raw);
    }
    break;

  case HANDSHAKE_certificate_request:
    COND_FATAL(1, ALERT_handshake_failure,
              "Client certificate authentication not implemented "
              "for TLS 1.3.\n");
    break;

  case HANDSHAKE_certificate:
    if (!handle_certificates_1_3(input))
      return -1;
    add_handshake_message(raw);
    break;

  case HANDSHAKE_certificate_verify:
    // NB: Must validate against the transcript hash *before* this
    // message is added -- CertificateVerify doesn't sign itself.
    if (validate_certificate_verify(input, SIGN_server_certificate_verify) < 0)
      return -1;
    add_handshake_message(raw);
    break;

  case HANDSHAKE_finished:
    {
      string(8bit) expected =
        key_schedule->verify_data(
          [string(8bit)]key_schedule->server_handshake_traffic_secret,
          transcript_hash());
      string(8bit) digest = input->read(sizeof(expected));
      COND_FATAL(digest != expected, ALERT_unexpected_message,
                "Digests differ.\n");
      server_verify_data = digest;
      add_handshake_message(raw);

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

      if (early_data_accepted_1_3) {
        // RFC 8446 4.5: EndOfEarlyData is sent (and hashed into the
        // transcript, via handshake_packet()'s usual side effect)
        // *before* the client's own Finished -- and, unlike Finished,
        // it's still encrypted under client_early_traffic_secret, not
        // client_handshake_traffic_secret (that switch happens right
        // after this, below) -- hence send_packet() with an
        // explicitly-bound write state here, not send_packet_1_3()
        // (which would bind to whatever current_write_state is right
        // now, the wrong key). Reuses the *same* State object the
        // early-data records themselves were sent under (see
        // early_data_write_state_1_3's doc comment) rather than
        // building a fresh one from the same secret, so its AEAD
        // sequence number continues rather than restarting at 0.
        Packet eoed = handshake_packet(HANDSHAKE_end_of_early_data, "");
        eoed->bound_write_state = early_data_write_state_1_3;
        send_packet(eoed, PRI_urgent);
      }

      // Our Finished is the last message sent under the handshake
      // traffic secret.
      current_write_state =
        new_traffic_state([string(8bit)]key_schedule->client_handshake_traffic_secret, 1);

      string(8bit) verify_data =
        key_schedule->verify_data(
          [string(8bit)]key_schedule->client_handshake_traffic_secret,
          transcript_hash());
      send_packet_1_3(handshake_packet(HANDSHAKE_finished, verify_data));
      client_verify_data = verify_data;

      // Nothing more is sent under the handshake traffic secret, and
      // nothing more is received under it either.
      current_write_state =
        new_traffic_state([string(8bit)]key_schedule->client_application_traffic_secret, 1);
      current_read_state =
        new_traffic_state([string(8bit)]key_schedule->server_application_traffic_secret, 0);

      // RFC 8446 7.1: resumption_master_secret is derived over the
      // transcript including our own Finished.
      key_schedule->resumption_master_secret =
        key_schedule->derive_secret([string(8bit)]key_schedule->master_secret,
                                    "res master", transcript_hash());

      handshake_messages = Buffer();
      handshake_state = STATE_handshake_finished;
      return 1;
    }
  }
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
    switch(type) {
    default:
      add_handshake_message(raw);
      COND_FATAL(1, ALERT_unexpected_message, "Expected server hello.\n");
      break;
    case HANDSHAKE_server_hello:
      SSL3_DEBUG_MSG("SSL.ClientConnection: SERVER_HELLO\n");

      version = [int(0x300..0x300)|ProtocolVersion]input->read_int(2);
      server_random = input->read(32);
      string(8bit) session_id = input->read_hstring(1);
      int cipher_suite = input->read_int(2);
      int compression_method = input->read_int(1);

      if (server_random == TLS13_HRR_RANDOM) {
        // RFC 8446 4.1.4: HelloRetryRequest shares HANDSHAKE_server_hello's
        // wire type -- only this magic Random value distinguishes it.
        // handle_hello_retry_request_1_3() does its own transcript
        // bookkeeping (it must replace ClientHello1 with a synthetic
        // message_hash entry before hashing in anything else), so --
        // unlike every other case here -- don't add_handshake_message(raw)
        // first.
        if (handle_hello_retry_request_1_3(cipher_suite, input, raw) < 0)
          return -1;
        break;
      }

      add_handshake_message(raw);

      if( !has_value(context->supported_versions, version) &&
          // RFC 8446 4.1.3: a TLS 1.3 ServerHello's legacy_version is
          // always {3,3} -- the real version is only known once the
          // supported_versions extension is parsed below, so don't
          // reject the legacy field alone when we might be
          // negotiating 1.3.
          !((version == PROTOCOL_TLS_1_2) &&
            has_value(context->supported_versions, PROTOCOL_TLS_1_3)) )
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

      session->set_compression_method(compression_method);

      SSL3_DEBUG_MSG("STATE_wait_for_hello: received hello\n"
		     "version = %s\n"
		     "session_id=%O\n"
                     "cipher suite: 0x%x\n"
		     "compression method: %O\n",
		     fmt_version(version),
		     session_id, cipher_suite, compression_method);

      int missing_secure_renegotiation = secure_renegotiation;
      int server_key_share_group = -1;
      string(8bit) server_key_share_key = "";
      int(0..1) selected_psk;

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
              COND_FATAL(sizeof(extension_data),
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

          case EXTENSION_key_share:
            {
              // RFC 8446 4.2.8: ServerHello KeyShareServerHello -- a
              // single KeyShareEntry, not a list.
              server_key_share_group = extension_data->read_int(2);
              server_key_share_key = extension_data->read_hstring(2);
            }
            break;

          case EXTENSION_pre_shared_key:
            {
              // RFC 8446 4.2.11: ServerHello's extension_data is just
              // the 2-byte selected_identity index -- we only ever
              // offer identity 0, so any non-error response selects
              // that one.
              extension_data->read_int(2);
              selected_psk = 1;
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

          // RFC 9846 4.3: a recognized extension's own structure MUST
          // consume all of extension_data; anything left over is a
          // decode error, not silently ignored trailing bytes.
          COND_FATAL(sizeof(extension_data), ALERT_decode_error,
                     "Extraneous data in extension.\n");
	}
      }

      // RFC 5746 3.5:
      // When a ServerHello is received, the client MUST verify that the
      // "renegotiation_info" extension is present; if it is not, the
      // client MUST abort the handshake.
      COND_FATAL(missing_secure_renegotiation, ALERT_handshake_failure,
                 "Missing secure renegotiation extension.\n");

      if (version >= PROTOCOL_TLS_1_3) {
        // See SSL.ServerConnection->reply_new_session_1_3(): TLS 1.3's
        // handshake differs too much from TLS 1.2 and earlier to
        // share the resumption/ServerKeyExchange/Certificate code
        // below.
        return got_server_hello_1_3(server_key_share_group,
                                   server_key_share_key, selected_psk);
      }

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
    if (version >= PROTOCOL_TLS_1_3) {
      // NB: Unlike the TLS 1.2-and-earlier case below, handshake
      // transcript bookkeeping is done inside handle_handshake_1_3()
      // itself -- Finished and CertificateVerify must each be
      // validated against the transcript *before* they're added to
      // it, so a single unconditional add_handshake_message() here
      // (ahead of knowing which message this is) would be wrong.
      return handle_handshake_1_3(type, input, raw);
    }
    add_handshake_message(raw);
    switch(type)
    {
    default:
      COND_FATAL(1, ALERT_unexpected_message,
		 sprintf("Unexpected server message: %s.\n",
			 fmt_constant(type, "HANDSHAKE")));
      break;
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

      string my_digest = "";
      if (version == PROTOCOL_SSL_3_0) {
        server_verify_data = input->read(36);
        my_digest = hash_messages("SRVR");
      } else if (version >= PROTOCOL_TLS_1_0) {
        server_verify_data = input->read(12);
        my_digest = hash_messages("server finished");
      }

      COND_FATAL(my_digest != server_verify_data,
                 ALERT_unexpected_message, "Digests differ.\n");

      if (reuse) {
	add_handshake_message(raw);
        /* Second hash includes this message, the first doesn't */

	if(version == PROTOCOL_SSL_3_0)
	  send_packet(finished_packet("CLNT"));
        else
	  send_packet(finished_packet("client finished"));
        if (context->heartbleed_probe &&
	    session->heartbeat_mode == HEARTBEAT_MODE_peer_allowed_to_send) {
	  // Probe for the Heartbleed vulnerability (CVE-2014-0160).
	  send_packet(heartbleed_packet());
	}
      }

      // Handshake hash is calculated for both directions above.
      handshake_messages = Buffer();

      handshake_state = STATE_handshake_finished;

      return 1;			// We're done shaking hands
    }

  case STATE_handshake_finished:
    // TLS 1.3: NewSessionTicket (@rfc{8446:4.6.1@}) and KeyUpdate
    // (@rfc{8446:4.6.3@}) are post-handshake messages -- they can
    // legitimately arrive here, after @[handle_handshake_1_3()]
    // already returned 1. Not hashed into @[handshake_messages]:
    // nothing checks that transcript again once the 1-RTT handshake
    // itself is done.
    switch (type) {
      case HANDSHAKE_new_session_ticket:
        got_new_session_ticket_1_3(input);
        return 0;
      case HANDSHAKE_key_update:
        return handle_key_update_1_3(input);
      default:
        COND_FATAL(1, ALERT_unexpected_message,
                   "Unexpected post-handshake message.\n");
    }
  }

  return 0;
}
