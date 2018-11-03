#pike __REAL_VERSION__
#pragma strict_types
#require constant(SSL.Cipher)

//! SSL.Connection keeps the state relevant for a single SSL connection.
//! This includes the @[Context] object (which doesn't change), various
//! buffers, the @[Session] object (reused or created as appropriate),
//! and pending read and write states being negotiated.
//!
//! Each connection will have two sets of read and write @[State]s: The
//! current read and write states used for encryption, and pending read
//! and write states to be taken into use when the current keyexchange
//! handshake is finished.
//!
//! This object is also responsible for managing incoming and outgoing
//! packets. Outgoing packets are stored in queue objects and sent in
//! priority order.
//!
//! @note
//!   This class should never be created directly, instead one of the
//!   classes that inherits it should be used (ie either
//!   @[ClientConnection] or @[ServerConnection]) depending on whether
//!   this is to be a client-side or server-side connection. These in
//!   turn are typically created by @[File()->create()].
//!
//! @seealso
//!   @[ClientConnection], @[ServerConnection], @[Context],
//!   @[Session], @[File], @[State]

//#define SSL3_PROFILING

#include "tls.h"

import .Constants;

private constant State = .State;
private constant Session = .Session;
private constant Context = .Context;
private constant Buffer = .Buffer;

Session session;
Context context;

array(State) pending_read_state = ({});
array(State) pending_write_state = ({});

/* State variables */

int handshake_state; // Constant.STATE_*
int previous_handshake;	// Constant.HANDSHAKE_*
int reuse;

constant CERT_none = 0;
constant CERT_requested = 1;
constant CERT_received = 2;
constant CERT_no_certificate = 3;
int certificate_state;

int expect_change_cipher; /* Reset to 0 if a change_cipher message is
			   * received */

// RFC 5746-related fields
int secure_renegotiation;
string(8bit) client_verify_data = "";
string(8bit) server_verify_data = "";
// 3.2: Initially of zero length for both the
//      ClientHello and the ServerHello.

//! The active @[Cipher.KeyExchange] (if any).
.Cipher.KeyExchange ke;

ProtocolVersion version;
ProtocolVersion client_version; /* Used to check for version roll-back attacks. */

//! Random cookies, sent and received with the hello-messages.
string(8bit) client_random;
string(8bit) server_random;

private constant Packet = .Packet;
private constant Alert = .Alert;

int(0..3) tickets_enabled = 0;

// RFC 7301 (ALPN) 3.1:
//   Unlike many other TLS extensions, this extension does not establish
//   properties of the session, only of the connection.  When session
//   resumption or session tickets [RFC5077] are used, the previous
//   contents of this extension are irrelevant, and only the values in the
//   new handshake messages are considered.
//! Selected ALPN (@rfc{7301@}) protocol (if any).
//!
//! @note
//!   Note that this is a connection property, and needs to be renegotiated
//!   on session resumption.
string(8bit) application_protocol;

Alert alert(int(1..2) level, int(8bit) description,
            string|void message)
{
  return context->alert_factory(this, level, description, version,
				message);
}

Buffer get_signature_algorithms()
{
  Buffer sign_algs = Buffer();
  foreach(context->get_signature_algorithms(), [int hash, int sign])
  {
    sign_algs->add_int(hash, 1);
    sign_algs->add_int(sign, 1);
  }
  return sign_algs;
}

#ifdef SSL3_PROFILING
System.Timer timer = System.Timer();
float last_time = 0.0;
void addRecord(int t,int s) {
  addRecord(sprintf("sender: %d type: %s", s, fmt_constant(t, "HANDSHAKE")));
}
variant void addRecord(string label) {
  float stamp = timer->peek();
  Stdio.stdout.write("time: %.6f (%.6f) %s\n", stamp, stamp-last_time, label);
  last_time = stamp;
}
#endif

Buffer handshake_messages = Buffer();
protected void add_handshake_message(Buffer|Stdio.Buffer|string(8bit) data)
{
  handshake_messages->add(data);
}

Packet handshake_packet(int(8bit) type,
			string(8bit)|Buffer|object(Stdio.Buffer) data)
{
#ifdef SSL3_PROFILING
  addRecord(type,1);
#endif
  string(8bit) str = sprintf("%1c%3H", type, (string(8bit))data);
  add_handshake_message(str);

  /* FIXME: One need to split large packages. */
  return Packet(version, PACKET_handshake, str);
}

Packet change_cipher_packet()
{
  expect_change_cipher++;
  return Packet(version, PACKET_change_cipher_spec, "\001");
}

string(8bit) hash_messages(string(8bit) sender, int|void len)
{
  switch( version )
  {
  case PROTOCOL_SSL_3_0:
    {
      string(8bit) data = (string(8bit))handshake_messages + sender;
      return .Cipher.MACmd5(session->master_secret)->hash(data) +
        .Cipher.MACsha(session->master_secret)->hash(data);
    }
  case PROTOCOL_TLS_1_0:
  case PROTOCOL_TLS_1_1:
    return session->cipher_spec->prf(session->master_secret, sender,
				     Crypto.MD5.hash(handshake_messages)+
				     Crypto.SHA1.hash(handshake_messages),
                                     len || 12);
  case PROTOCOL_TLS_1_2:
  default:
    return session->cipher_spec->prf(session->master_secret, sender,
                                     session->cipher_spec->hash
                                     ->hash(handshake_messages),
                                     len || 12);
  }
}

Packet certificate_packet(array(string(8bit)) certificates)
{
  return handshake_packet(HANDSHAKE_certificate,
                          Buffer()->add_string_array(certificates, 3, 3));
}

Packet certificate_verify_packet(string(8bit)|void signature_context)
{
  SSL3_DEBUG_MSG("SSL.Connection: CERTIFICATE_VERIFY\n"
		 "%O: handshake_messages: %d bytes.\n",
		 this, sizeof(handshake_messages));
  Buffer struct = Buffer();

  if (signature_context) {
    // TLS 1.3 and later.
    session->cipher_spec->sign(session,
			       signature_context +
			       session->cipher_spec->hash
                               ->hash(handshake_messages),
			       struct);
  } else {
    session->cipher_spec->sign(session, (string(8bit))handshake_messages, struct);
  }

  return handshake_packet(HANDSHAKE_certificate_verify, struct);
}

int(-1..0) validate_certificate_verify(Buffer input,
				       string(8bit) signature_context)
{
  int(0..1) verification_ok;
  string(8bit) signed = (string(8bit))handshake_messages;
  if (version >= PROTOCOL_TLS_1_3)
    signed = signature_context + session->cipher_spec->hash->hash(signed);

  mixed err = catch {
      verification_ok = session->cipher_spec->verify(
        session, signed, input);
    };
#ifdef SSL3_DEBUG
  if (err) {
    master()->handle_error(err);
  }
#endif
  err = UNDEFINED;	// Get rid of warning.
  COND_FATAL(!verification_ok, ALERT_unexpected_message,
             "Validation of CertificateVerify failed.\n");
  return 0;
}

Packet heartbeat_packet(Buffer s)
{
  return Packet(version, PACKET_heartbeat, s->read());
}

protected Crypto.AES heartbeat_encode;
protected Crypto.AES heartbeat_decode;

Packet heartbleed_packet()
{
  if (!heartbeat_encode) {
    // NB: We encrypt the payload with a random AES key
    //     to reduce the amount of known plaintext in
    //     the heartbeat masseages. This is needed now
    //     that many cipher suites (such as GCM and CCM)
    //     use xor with a cipher stream, to reduce risk
    //     of revealing larger segments of the stream.
    heartbeat_encode = Crypto.AES();
    heartbeat_decode = Crypto.AES();
    string(8bit) heartbeat_key = random_string(16);
    heartbeat_encode->set_encrypt_key(heartbeat_key);
    heartbeat_decode->set_decrypt_key(heartbeat_key);
  }

  // This packet probes for the Heartbleed vulnerability (CVE-2014-0160)
  // by crafting a heartbeat packet with insufficient (0) padding.
  //
  // If we get a response, the peer doesn't validate the message sizes
  // properly, and probably suffers from the Heartbleed vulnerability.
  //
  // Note that we don't use negative padding (as per the actual attack),
  // to avoid actually stealing information from the peer.
  //
  // Note that we detect the packet on return by it having all zeros
  // in the second field.
  Buffer hb_msg = Buffer();
  hb_msg->add_int(HEARTBEAT_MESSAGE_request, 1);
  hb_msg->add_int(16, 2);
  int now = gethrtime();
  hb_msg->add(heartbeat_encode->crypt(sprintf("%8c%8c", now, 0)));
  // No padding.
  return heartbeat_packet(hb_msg);
}

// Verify that a certificate chain is acceptable
private array(Standards.X509.TBSCertificate)
  verify_certificate_chain(array(string) certs)
{
  // If we're not requiring the certificate, and we don't provide one,
  // that should be okay.
  if((context->auth_level < AUTHLEVEL_require) && !sizeof(certs))
    return ({});

  // If we are not verifying the certificates, we only need to decode
  // the leaf certificate for its public key.
  if(context->auth_level == AUTHLEVEL_none)
    return ({ Standards.X509.decode_certificate(certs[0]) });

  // A lack of certificates when we reqiure and must verify the
  // certificates is probably a failure.
  if(!sizeof(certs))
    return 0;

  // See if the issuer of the certificate is acceptable. This means
  // the issuer of the certificate must be one of the authorities.
  // NOTE: This code is only relevant when acting as a server dealing
  // with client certificates.
  if(sizeof(context->authorities_cache))
  {
    string r=Standards.X509.decode_certificate(certs[-1])->issuer
      ->get_der();
    int issuer_known = 0;
    foreach(context->authorities_cache, string c)
    {
      if(r == c) // we have a trusted issuer
      {
        issuer_known = 1;
        break;
      }
    }

    if(issuer_known==0)
    {
      return 0;
    }
  }

  // Decode the chain, verify each certificate and verify that the
  // chain is unbroken.
  mapping result = ([]);
  catch {
    result = Standards.X509.verify_certificate_chain(certs,
                                     context->trusted_issuers_cache,
                                     context->auth_level >= AUTHLEVEL_require);
  };
  if( !result ) return 0;

  if (session->server_name && sizeof([array](result->certificates || ({})))) {
    array(Standards.X509.TBSCertificate) certs =
      [array(Standards.X509.TBSCertificate)](result->certificates);
    Standards.X509.TBSCertificate cert = certs[-1];
    array(string) globs = Standards.PKCS.Certificate.
      decode_distinguished_name(cert->subject)->commonName - ({ 0 });
    if (cert->ext_subjectAltName_dNSName) {
      globs += cert->ext_subjectAltName_dNSName;
    }
    result->verified = glob(map(globs, lower_case),
			    lower_case(session->server_name));
  }

  // This data isn't actually used internally.
  session->cert_data = result;

  if(result->verified)
    return [array(Standards.X509.TBSCertificate)]result->certificates;

  return 0;
}

// Decodes certificate data. Leaves session->peer_certificate_chain
// either 0 or with an array with 1 or more certificates. If
// certificates are received session->peer_public_key is updated with
// the public key object. If that is an ECC object, the curve is set
// in session->curve.
int(0..1) handle_certificates(Buffer packet)
{
  // FIXME: Throw exception if called more than once?

  Stdio.Buffer input = packet->read_hbuffer(3);
  array(string(8bit)) certs = ({ });
  while(sizeof(input))
    certs += ({ input->read_hstring(3) });
  // No need to check remainder input, as the above loop will either
  // drain input fully or read out of bounds and trigger a decode
  // error alert packet.

  if(sizeof(packet))
  {
    send_packet(alert(ALERT_fatal, ALERT_unexpected_message,
                      "Unknown additional data in packet.\n"));
    return 0;
  }

  // This array is in the reverse order of the certs array.
  array(Standards.X509.TBSCertificate) decoded =
    verify_certificate_chain(certs);
  if( !decoded )
  {
    send_packet(alert(ALERT_fatal, ALERT_bad_certificate,
                      "Bad certificate chain.\n"));
    return 0;
  }
  if( !sizeof(certs) )
    return 1;

  // This data isn't actually used internally.
  session->peer_certificate_chain = certs;

  session->peer_public_key = decoded[-1]->public_key->pkc;
  return 1;
}

//! Generate new pending cipher states.
void new_cipher_states();

//! Derive the master secret from the premaster_secret
//! and the random seeds, and configure the keys.
void derive_master_secret(string(8bit) premaster_secret)
{
  SSL3_DEBUG_MSG("%O: derive_master_secret: %s (%s)\n",
		 this, fmt_constant(handshake_state, "STATE"),
		 fmt_version(version));

  if (!sizeof(premaster_secret)) {
    // Clear text mode.
    session->master_secret = "";
  } else if (session->extended_master_secret) {
    // Extended Master Secret Draft.
    session->master_secret = premaster_secret;
    session->master_secret = hash_messages("extended master secret", 48);
  } else {
    session->master_secret =
      session->cipher_spec->prf(premaster_secret, "master secret",
				client_random + server_random, 48);
  }

  new_cipher_states();
}


//! Do handshake processing. Type is one of HANDSHAKE_*, data is the
//! contents of the packet, and raw is the raw packet received (needed
//! for supporting SSLv2 hello messages).
//!
//! This function returns 0 if handshake is in progress, 1 if handshake
//! is finished, and -1 if a fatal error occurred. It uses the
//! send_packet() function to transmit packets.
int(-1..1) handle_handshake(int type, Buffer input, Stdio.Buffer raw);

//! Initialize the connection state.
//!
//! @param ctx
//!   The context for the connection.
protected void create(Context ctx)
{
  current_read_state = State(this);
  current_write_state = State(this);

  version = min([int]max(@ctx->supported_versions), PROTOCOL_TLS_1_2);
  context = ctx;
}

//! Remove cyclic references as best we can.
void shutdown()
{
  current_read_state = current_write_state = UNDEFINED;
  pending_read_state = pending_write_state = ({});
  ke = UNDEFINED;
  alert_callback = UNDEFINED;
}

//
// --- Old connection.pike below
//


State current_read_state;
State current_write_state;
Stdio.Buffer read_buffer = Stdio.Buffer();
Packet packet;

//! Number of application data bytes sent by us.
int sent;

//! Bitfield with the current connection state.
ConnectionState state = CONNECTION_handshaking;

function(object,int|object,string:void) alert_callback;

constant PRI_alert = 1;
constant PRI_urgent = 2;
constant PRI_application = 3;

protected ADT.Queue alert_q = ADT.Queue();
protected ADT.Queue urgent_q = ADT.Queue();
protected ADT.Queue application_q = ADT.Queue();

//! Returns a string describing the current connection state.
string describe_state()
{
  if (!state) return "ready";
  array(string) res = ({});
  if (state & CONNECTION_handshaking) res += ({ "handshaking" });
  if (state & CONNECTION_local_failing) {
    if (state & CONNECTION_local_fatal) {
      res += ({ "local_fatal" });
    } else {
      res += ({ "local_failing" });
    }
  }
  if (state & CONNECTION_local_closing) {
    if (state & CONNECTION_local_closed) {
      res += ({ "local_closed" });
    } else {
      res += ({ "local_closing" });
    }
  }
  if (state & CONNECTION_peer_fatal) res += ({ "peer_fatal" });
  if (state & CONNECTION_peer_closed) res += ({ "peer_closed" });
  return res * "|";
}

protected string _sprintf(int t)
{
  if (t == 'O') return sprintf("SSL.Connection(%s)", describe_state());
}

//! Called with alert object, sequence number of bad packet,
//! and raw data as arguments, if a bad packet is received.
//!
//! Can be used to support a fallback redirect https->http.
void set_alert_callback(function(object,int|object,string:void) callback)
{
  alert_callback = callback;
}

//! Low-level receive handler. Returns a packet, an alert, or zero if
//! more data is needed to get a complete packet.
protected Packet recv_packet()
{
  if (!packet)
    packet = Packet(version, 2048);

  int res = packet->recv(read_buffer);

  switch(res)
  {
  case 1:
    // Finished a packet
    if (current_read_state)
      SSL3_DEBUG_MSG("SSL.Connection->recv_packet(): version=0x%x\n",
		     version);
    return current_read_state->decrypt_packet(packet);
  case 0:
    SSL3_DEBUG_MSG("SSL.Connection->recv_packet(): current_read_state is zero!\n");
    return 0;
  case -1:
    if( state & CONNECTION_handshaking )
    {
      // This is likely HTTP request on the TLS port. We could parse
      // the path and host header, and create a request with https://
      // schema. Would need to go through context for several
      // reasons. This could be a non-HTTP port or we would want to
      // use HSTS.
      SSL3_DEBUG_MSG("Initial incorrect data %O\n",
                     ((string)read_buffer[..25]));
    }
    return alert(ALERT_fatal, ALERT_unexpected_message);
  default:
    error("Internal error.\n");
  }

  return 0;
}

//! Queues a packet for write. Handshake and and change cipher
//! must use the same priority, so must application data and
//! close_notifies.
void send_packet(Packet packet, int|void priority)
{
  if (state & (CONNECTION_local_closed | CONNECTION_local_failing)) {
    SSL3_DEBUG_MSG("send_packet: Ignoring packet after close/fail.\n");
    return;
  }

  session->last_activity = time(1);

  if (packet->content_type == PACKET_alert) {
    if (packet->level == ALERT_fatal) {
      state = [int(0..0)|ConnectionState](state | CONNECTION_local_failing);
    } else if (packet->description == ALERT_close_notify) {
      state = [int(0..0)|ConnectionState](state | CONNECTION_local_closing);
    }
  }

  if (!priority)
    priority = ([ PACKET_alert : PRI_alert,
		  PACKET_change_cipher_spec : PRI_urgent,
	          PACKET_handshake : PRI_urgent,
		  PACKET_heartbeat : PRI_urgent,
		  PACKET_application_data : PRI_application
    ])[packet->content_type];

  if ((packet->content_type == PACKET_handshake) &&
      (priority == PRI_application)) {
    // Assume the packet is either hello_request or client_hello,
    // and that we want to renegotiate.
    expect_change_cipher = 0;
    certificate_state = 0;
    state = [int(0..0)|ConnectionState](state | CONNECTION_handshaking);
    handshake_state = STATE_wait_for_hello;
    previous_handshake = 0;
  }

  SSL3_DEBUG_MSG("SSL.Connection->send_packet: type %d, pri %d, %O\n",
		 packet->content_type, priority, packet->fragment[..5]);
  switch (priority)
  {
  default:
    error( "Internal error\n" );
  case PRI_alert:
    alert_q->put(packet);
    break;
  case PRI_urgent:
    urgent_q->put(packet);
    break;
  case PRI_application:
    application_q->put(packet);
    break;
  }

}

//! Returns the number of packets queued for writing.
//!
//! @returns
//!   Returns the number of times @[to_write()] can be called before
//!   it stops returning non-empty strings.
int query_write_queue_size()
{
  return sizeof(alert_q) + sizeof(urgent_q) + sizeof(application_q);
}

//! Extracts data from the packet queues. Returns 2 if data has been
//! written, 0 if there are no pending packets, 1 of the connection is
//! being closed politely, and -1 if the connection died unexpectedly.
//!
//! This function is intended to be called from an i/o write callback.
//!
//! @seealso
//!   @[query_write_queue_size()], @[send_streaming_data()].
int(-1..2) to_write(Stdio.Buffer output)
{
  if (state & CONNECTION_local_fatal)
    return -1;

  Packet packet = [object(Packet)](alert_q->get() || urgent_q->get() ||
                                   application_q->get());
  if (!packet)
    return !!(state & CONNECTION_local_closing);

  SSL3_DEBUG_MSG("SSL.Connection: writing packet of type %d, %O\n",
                 packet->content_type, packet->fragment[..6]);
  if (packet->content_type == PACKET_alert)
  {
    if (packet->level == ALERT_fatal) {
      state = [int(0..0)|ConnectionState](state | CONNECTION_local_fatal |
					  CONNECTION_peer_closed);
      current_read_state = UNDEFINED;
      pending_read_state = ({});
      // SSL3 5.4:
      // Alert messages with a level of fatal result in the immediate
      // termination of the connection. In this case, other
      // connections corresponding to the session may continue, but
      // the session identifier must be invalidated, preventing the
      // failed session from being used to establish new connections.
      if (session) {
	context->purge_session(session);
      }
    } else if (packet->description == ALERT_close_notify) {
      state = [int(0..0)|ConnectionState](state | CONNECTION_local_closed);
    }
  }
  packet = current_write_state->encrypt_packet(packet, context);
  if (packet->content_type == PACKET_change_cipher_spec) {
    if (sizeof(pending_write_state)) {
      current_write_state = pending_write_state[0];
      pending_write_state = pending_write_state[1..];
    } else {
      error("Invalid Change Cipher Spec.\n");
    }
  }

  packet->send(output);
  return 2;
}

//! Initiate close.
void send_close()
{
  send_packet(alert(ALERT_warning, ALERT_close_notify,
                    "Closing connection.\n"), PRI_application);
}

//! Renegotiate the connection.
void send_renegotiate();

//! Send an application data packet. If the data block is too large
//! then as much as possible of the beginning of it is sent. The size
//! of the sent data is returned.
int send_streaming_data (string(8bit) data)
{
  int size = sizeof(data);
  if (!size) return 0;

  if ((!sent) && (version < PROTOCOL_TLS_1_1) &&
      (session->cipher_spec->cipher_type == CIPHER_block) &&
      (size>1))
  {
    // Workaround for the BEAST attack.
    // This method is known as the 1/(n-1) split:
    //   Send just one byte of payload in the first packet
    //   to improve the initialization vectors in TLS 1.0.
    send_packet(Packet(version, PACKET_application_data, data[..0]));
    data = data[1..];
  }

  send_packet(Packet(version, PACKET_application_data,
                     data[..session->max_packet_size-1]));;
  sent += size;
  return size;
}

// @returns
// @int
//   @elem value -1
//     A Fatal error occurred and processing should stop.
//   @elem value 0
//     Processing can continue.
//   @elem value 1
//     Connection should close.
// @endint
protected int(-1..1) handle_alert(string s)
{
  // sizeof(s)==2, checked at caller.
  int level = s[0];
  int description = s[1];
  COND_FATAL(!(ALERT_levels[level] && ALERT_descriptions[description]),
             ALERT_unexpected_message, "Invalid alert\n");

  // Consider all deprecated alerts as fatals.
  COND_FATAL((ALERT_deprecated[description] &&
              ALERT_deprecated[description] < version),
             ALERT_unexpected_message, "Deprecated alert\n");

  if (level == ALERT_fatal)
  {
    SSL3_DEBUG_MSG("SSL.Connection: Fatal alert %O\n",
                   ALERT_descriptions[description]);
    state = [int(0..0)|ConnectionState](state | CONNECTION_peer_fatal |
					CONNECTION_peer_closed);
    // SSL3 5.4:
    // Alert messages with a level of fatal result in the immediate
    // termination of the connection. In this case, other
    // connections corresponding to the session may continue, but
    // the session identifier must be invalidated, preventing the
    // failed session from being used to establish new connections.
    if (session) {
      context->purge_session(session);
    }
    return -1;
  }
  if (description == ALERT_close_notify)
  {
    SSL3_DEBUG_MSG("SSL.Connection: %O\n", ALERT_descriptions[description]);
    state = [int(0..0)|ConnectionState](state | CONNECTION_peer_closed);
    return 1;
  }
  if (description == ALERT_no_certificate)
  {
    SSL3_DEBUG_MSG("SSL.Connection: %O\n", ALERT_descriptions[description]);

    if ( (certificate_state == CERT_requested) &&
         (context->auth_level == AUTHLEVEL_ask) )
    {
      certificate_state = CERT_no_certificate;
      return 0;
    } else {
      COND_FATAL(1, ((certificate_state == CERT_requested)
                     ? ALERT_handshake_failure
                     : ALERT_unexpected_message),
                 "Certificate required.\n");
    }
  }
#ifdef SSL3_DEBUG
  else
    werror("SSL.Connection: Received warning alert %O\n",
           ALERT_descriptions[description]);
#endif
  return 0;
}

int(-1..0) handle_change_cipher(int c)
{
  COND_FATAL(!expect_change_cipher || (c != 1),
             ALERT_unexpected_message, "Unexpected change cipher!\n");

  if (sizeof(pending_read_state)) {
    SSL3_DEBUG_MSG("%O: Changing read state.\n", this);
    current_read_state = pending_read_state[0];
    pending_read_state = pending_read_state[1..];
  } else {
    error("No new read state pending!\n");
  }
  expect_change_cipher--;
  return 0;
}

void send_heartbeat()
{
  if ((state != CONNECTION_ready) ||
      (session->heartbeat_mode != HEARTBEAT_MODE_peer_allowed_to_send)) {
    // We're not allowed to send heartbeats.
    return;
  }

  Buffer hb_msg = Buffer();
  hb_msg->add_int(HEARTBEAT_MESSAGE_request, 1);
  hb_msg->add_int(16, 2);
  int now = gethrtime();
  hb_msg->add(heartbeat_encode->crypt(sprintf("%8c%8c", now, now)));
  // We pad to an even 64 bytes.
  int(0..) bytes = [int(0..)]max(0, 64 - sizeof(hb_msg));
  hb_msg->add(random_string(bytes));
  send_packet(heartbeat_packet(hb_msg));
}

void handle_heartbeat(string(8bit) s)
{
  if (sizeof(s) < 19) return;	// Minimum size for valid heartbeats.
  Buffer hb_msg = Buffer(s);
  int hb_type = hb_msg->read_int(1);
  int hb_len = hb_msg->read_int(2);

  SSL3_DEBUG_MSG("SSL.Connection: Heartbeat %s (%d bytes)\n",
		 fmt_constant(hb_type, "HEARTBEAT_MESSAGE"), hb_len);

  string(8bit) payload;
  int(0..) pad_len = 16;

  // RFC 6520 4:
  // If the payload_length of a received HeartbeatMessage is too
  // large, the received HeartbeatMessage MUST be discarded silently.
  if ((hb_len < 0) || ((hb_len + 16) > sizeof(hb_msg))) {
#ifdef SSL3_SIMULATE_HEARTBLEED
    payload = hb_msg->read();
    if (sizeof(payload) < hb_len) {
      payload = payload + random_string(hb_len - sizeof(payload));
    } else {
      payload = payload[..hb_len-1];
    }
#else
    return;
#endif
  } else {
    payload = hb_msg->read(hb_len);
    pad_len = sizeof(hb_msg);
  }

  switch(hb_type) {
  case HEARTBEAT_MESSAGE_request:
    // RFC 6520 4:
    // When a HeartbeatRequest message is received and sending a
    // HeartbeatResponse is not prohibited as described elsewhere in
    // this document, the receiver MUST send a corresponding
    // HeartbeatResponse message carrying an exact copy of the payload
    // of the received HeartbeatRequest.
    hb_msg = Buffer();
    hb_msg->add_int(HEARTBEAT_MESSAGE_response, 1);
    hb_msg->add_int(hb_len, 2);
    hb_msg->add(payload);
    hb_msg->add(random_string(pad_len));
    send_packet(heartbeat_packet(hb_msg));
    break;
  case HEARTBEAT_MESSAGE_response:
    // RFC 6520 4:
    // If a received HeartbeatResponse message does not contain the
    // expected payload, the message MUST be discarded silently.
    if ((sizeof(payload) == 16) && heartbeat_decode) {
      hb_msg = Buffer(heartbeat_decode->crypt(payload));
      int a = hb_msg->read_int(8);
      int b = hb_msg->read_int(8);
      if (a != b) {
	if (!b) {
	  // Heartbleed probe response.
	  send_packet(alert(ALERT_fatal, ALERT_insufficient_security,
			    "Peer suffers from a bleeding heart.\n"));
	}
	break;
      }
#ifdef SSL3_DEBUG
      int delta = gethrtime() - a;
      SSL3_DEBUG_MSG("SSL.Connection: Heartbeat roundtrip: %dus\n", delta);
#endif
    }
    break;
  default:
    break;
  }
}

Stdio.Buffer handshake_buffer = Stdio.Buffer(); // Error mode 0.
Stdio.Buffer alert_buffer = Stdio.Buffer();

//! Main receive handler.
//!
//! @param data
//!   String of data received from the peer.
//!
//! @returns
//!    Returns one of:
//!    @mixed
//!      @type string(zero)
//!        Returns an empty string if there's neither application data
//!        nor errors (eg during the initial handshake).
//!      @type string(8bit)
//!        Returns a string of received application data.
//!      @type int(1..1)
//!        Returns @expr{1@} if the peer has closed the connection.
//!      @type int(-1..-1)
//!        Returns @expr{-1@} if an error has occurred.
//!
//!        These are the main cases of errors:
//!        @ul
//!          @item
//!            There was a low-level protocol communications failure
//!            (the data didn't look like an SSL packet), in which case
//!            the alert_callback will be called with the raw packet data.
//!            This can eg be used to detect HTTP clients connecting to
//!            an HTTPS server and similar.
//!          @item
//!            The peer has sent an @[Alert] packet, and @[handle_alert()]
//!            for it has returned -1.
//!          @item
//!            The peer has sent an unsupported/illegal sequence of
//!            packets, in which case a suitable @[Alert] will have been
//!            generated and queued for sending to the peer.
//!        @endul
//!    @endmixed
//!
//! This function is intended to be called from an i/o read callback.
string(8bit)|int(-1..1) got_data(string(8bit) data)
{
  if (state & CONNECTION_peer_closed) {
    // The peer has closed the connection.
    return 1;
  }
  // If closing we continue to try to read a remote close message.
  // That enables the caller to check for a clean close, and
  // to get the leftovers after the SSL connection.

  session->last_activity = time(1);
  read_buffer->add(data);
  Stdio.Buffer.RewindKey read_buffer_key = read_buffer->rewind_key();

  string(8bit) res = "";
  Packet packet;
  while (packet = recv_packet())
  {
    if (packet->is_alert)
    {
      // recv_packet returns packets with is_alert set if it is
      // generated on our side, as opposed to an alert that is
      // received. These are always fatal (wrong packet type, packet
      // version, packet size).
      SSL3_DEBUG_MSG("SSL.Connection: Bad received packet (%s)\n",
                     fmt_constant([int]packet->description, "ALERT"));
      if (alert_callback)
      {
        Stdio.Buffer.RewindKey here = read_buffer->rewind_key();
        read_buffer_key->rewind();
        alert_callback(packet, current_read_state->seq_num,
                       (string)read_buffer);
        here->rewind();
      }

      // We or the packet may have been destructed by the
      // alert_callback.
      if (this && packet)
	send_packet(packet);

      return -1;
    }

    SSL3_DEBUG_MSG("SSL.Connection: received packet of type %d\n",
                   packet->content_type);
    switch (packet->content_type)
    {
    case PACKET_alert:
      {
        SSL3_DEBUG_MSG("SSL.Connection: ALERT\n");

        COND_FATAL(!sizeof(packet->fragment), ALERT_unexpected_message,
                   "Zero length Alert fragments not allowed.\n");

        int(-1..1) err = 0;
        alert_buffer->add( packet->fragment );
        while(!err && sizeof(alert_buffer)>1)
          err = handle_alert(alert_buffer->read(2));

        if (err)
        {
          if (err > 0 && sizeof (res))
          {
            // If we get a close then we return the data we got so
            // far. state has CONNECTION_peer_closed at this point.
            return res;
          }
          return err;
        }
        break;
      }
    case PACKET_change_cipher_spec:
      {
        SSL3_DEBUG_MSG("SSL.Connection: CHANGE_CIPHER_SPEC\n");

        COND_FATAL(!sizeof(packet->fragment), ALERT_unexpected_message,
                   "Zero length ChangeCipherSpec fragments not allowed.\n");

        COND_FATAL(version >= PROTOCOL_TLS_1_3, ALERT_unexpected_message,
                   "ChangeCipherSpec not allowed in TLS 1.3 and later.\n");

        foreach(packet->fragment;; int c)
        {
          int(-1..0) err = handle_change_cipher(c);
          SSL3_DEBUG_MSG("tried change_cipher: %d\n", err);
          if (err)
            return err;
        }
        break;
      }
    case PACKET_handshake:
      {
        SSL3_DEBUG_MSG("SSL.Connection: HANDSHAKE\n");

        COND_FATAL(!sizeof(packet->fragment), ALERT_unexpected_message,
                   "Zero length Handshake fragments not allowed.\n");

        // Don't allow renegotiation in unsecure mode, to address
        // http://web.nvd.nist.gov/view/vuln/detail?vulnId=CVE-2009-3555.
        // For details see: http://www.g-sec.lu/practicaltls.pdf and
        // RFC 5746.
        COND_FATAL(!(state & CONNECTION_handshaking) &&
                   !secure_renegotiation, ALERT_no_renegotiation,
                   "Renegotiation not supported in unsecure mode.\n");

	COND_FATAL(!(state & CONNECTION_handshaking) &&
		   !context->enable_renegotiation, ALERT_no_renegotiation,
		   "Renegotiation disabled by context.\n");

        /* No change_cipher message was received */
        // FIXME: There's a bug somewhere since expect_change_cipher
        // often remains set after the handshake is completed. The
        // effect is that renegotiation doesn't work all the time.
        //
        // A side effect is that we are partly invulnerable to the
        // renegotiation vulnerability mentioned above. It is however
        // not safe to assume that, since there might be routes past
        // this, maybe through the use of a version 2 hello message
        // below.
        COND_FATAL(expect_change_cipher && (version < PROTOCOL_TLS_1_3),
                   ALERT_unexpected_message, "Expected change cipher.\n");

        int(-1..1) err;
        handshake_buffer->add( packet->fragment );

        while (sizeof(handshake_buffer) >= 4)
        {
          Stdio.Buffer.RewindKey key = handshake_buffer->rewind_key();
          int type = handshake_buffer->read_int8();
          Buffer input = Buffer(handshake_buffer->read_hbuffer(3));
          if(!input)
          {
            // Not enough data.
            key->rewind();
            break;
          }

          int len = 1+3+sizeof(input);
          key->rewind();
          Stdio.Buffer raw = handshake_buffer->read_buffer(len);

          mixed exception = catch {
              err = handle_handshake(type, input, raw);
              COND_FATAL(err>=0 && sizeof(input), ALERT_record_overflow,
                         sprintf("Extraneous handshake packet data (%O).\n",
                                 type));
            };
          if( exception )
          {
            if( objectp(exception) && ([object]exception)->buffer_error )
            {
              Error.Generic e = [object(Error.Generic)]exception;
              COND_FATAL(1, ALERT_decode_error, e->message());
            }
            throw(exception);
          }
          if (err < 0)
            return err;
          if (err > 0) {
            state &= ~CONNECTION_handshaking;
            if ((version >= PROTOCOL_TLS_1_3) || expect_change_cipher) {
              // NB: Renegotiation is available in TLS 1.2 and earlier.
              COND_FATAL(sizeof(handshake_buffer), ALERT_unexpected_message,
                         "Extraneous handshake packets.\n");
            }
            COND_FATAL(sizeof(handshake_buffer) && !secure_renegotiation,
                       ALERT_no_renegotiation,
                       "Renegotiation not supported in unsecure mode.\n");
          }
        }
        break;
      }
    case PACKET_application_data:
      SSL3_DEBUG_MSG("SSL.Connection: APPLICATION_DATA\n");

      COND_FATAL(state & CONNECTION_handshaking,
                 ALERT_unexpected_message,
                 "Handshake not finished yet!\n");

      res += packet->fragment;
      break;
    case PACKET_heartbeat:
      {
        // RFC 6520.
        SSL3_DEBUG_MSG("SSL.Connection: Heartbeat.\n");
        if (state != CONNECTION_ready) {
          // RFC 6520 3:
          // The receiving peer SHOULD discard the message silently,
          // if it arrives during the handshake.
          break;
        }
        if (!session->heartbeat_mode) {
          // RFC 6520 2:
          // If an endpoint that has indicated
          // peer_not_allowed_to_send receives a HeartbeatRequest
          // message, the endpoint SHOULD drop the message silently
          // and MAY send an unexpected_message Alert message.
          send_packet(alert(ALERT_warning, ALERT_unexpected_message,
                            "Heart beat mode not enabled.\n"));
          break;
        }

        mixed exception = catch {
            handle_heartbeat(packet->fragment);
          };
        if( exception )
        {
          if( objectp(exception) && ([object]exception)->buffer_error )
          {
            Error.Generic e = [object(Error.Generic)]exception;
            COND_FATAL(1, ALERT_decode_error, e->message());
          }
          throw(exception);
        }

      }
      break;
    default:
      COND_FATAL(state & CONNECTION_handshaking,
                 ALERT_unexpected_message,
                 "Unexpected message during handshake!\n");

      // RFC 4346 6:
      //   If a TLS implementation receives a record type it does not
      //   understand, it SHOULD just ignore it.
      SSL3_DEBUG_MSG("SSL.Connection: Ignoring packet of type %s\n",
                     fmt_constant(packet->content_type, "PACKET"));
      break;
    }
  }

  if (sizeof(res)) return res;
  if (state & CONNECTION_peer_closed) return 1;
  return "";
}
