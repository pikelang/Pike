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
//!   turn are typically created by @[sslfile()->create()].
//!
//! @seealso
//!   @[ClientConnection], @[ServerConnection], @[Context],
//!   @[Session], @[sslfile], @[state]

//#define SSL3_PROFILING

import .Constants;
#define State .State
#define Session .Session
#define Context .Context

#ifdef SSL3_DEBUG
#define SSL3_DEBUG_MSG(X ...)  werror(X)
#else /*! SSL3_DEBUG */
#define SSL3_DEBUG_MSG(X ...)
#endif /* SSL3_DEBUG */

Session session;
Context context;

State pending_read_state;
State pending_write_state;

/* State variables */

int handshake_state; // Constant.STATE_*

int handshake_finished = 0;

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

#define Packet .Packet
#define Alert .Alert

Alert alert(int(1..2) level, int(8bit) description,
            string|void message)
{
  return context->alert_factory(this, level, description, version,
				message);
}

string(8bit) get_signature_algorithms()
{
  ADT.struct sign_algs = ADT.struct();
  foreach(sort(indices(HASH_lookup)), int h) {
    sign_algs->put_uint(h, 1);
    sign_algs->put_uint(SIGNATURE_rsa, 1);
    sign_algs->put_uint(h, 1);
    sign_algs->put_uint(SIGNATURE_dsa, 1);
#if constant(Crypto.ECC.Curve)
    // NB: MD5 is not supported with ECDSA signatures.
    if (h != HASH_md5) {
      sign_algs->put_uint(h, 1);
      sign_algs->put_uint(SIGNATURE_ecdsa, 1);
    }
#endif
  }
  return sign_algs->pop_data();
}

#ifdef SSL3_PROFILING
System.Timer timer = System.Timer();
void addRecord(int t,int s) {
  Stdio.stdout.write("time: %.6f sender: %d type: %s\n", timer->get(), s,
                     fmt_constant(t, "HANDSHAKE"));
}
#endif

string(8bit) handshake_messages;

Packet handshake_packet(int(8bit) type, string data)
{
#ifdef SSL3_PROFILING
  addRecord(type,1);
#endif
  /* Perhaps one need to split large packages? */
  Packet packet = Packet();
  packet->content_type = PACKET_handshake;
  packet->fragment = sprintf("%1c%3H", type, [string(8bit)]data);
  handshake_messages += packet->fragment;
  return packet;
}

Packet change_cipher_packet()
{
  Packet packet = Packet();
  packet->content_type = PACKET_change_cipher_spec;
  packet->fragment = "\001";
  return packet;
}

string(8bit) hash_messages(string(8bit) sender)
{
  if(version == PROTOCOL_SSL_3_0) {
    return .Cipher.MACmd5(session->master_secret)->hash(handshake_messages + sender) +
      .Cipher.MACsha(session->master_secret)->hash(handshake_messages + sender);
  }
  else if(version <= PROTOCOL_TLS_1_1) {
    return session->cipher_spec->prf(session->master_secret, sender,
				     Crypto.MD5.hash(handshake_messages)+
				     Crypto.SHA1.hash(handshake_messages), 12);
  } else if(version >= PROTOCOL_TLS_1_2) {
    return session->cipher_spec->prf(session->master_secret, sender,
				     session->cipher_spec->hash->hash(handshake_messages), 12);
  }
}

Packet certificate_packet(array(string(8bit)) certificates)
{
  ADT.struct struct = ADT.struct();
  int len = 0;

  if(certificates && sizeof(certificates))
    len = `+( @ Array.map(certificates, sizeof));
  //  SSL3_DEBUG_MSG("SSL.Connection: certificate_message size %d\n", len);
  struct->put_uint(len + 3 * sizeof(certificates), 3);
  foreach(certificates, string(8bit) cert)
    struct->put_var_string(cert, 3);

  return handshake_packet(HANDSHAKE_certificate, struct->pop_data());
}

Packet heartbeat_packet(string(8bit) s)
{
  Packet packet = Packet();
  packet->content_type = PACKET_heartbeat;
  packet->fragment = s;
  return packet;
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
  ADT.struct hb_msg = ADT.struct();
  hb_msg->put_uint(HEARTBEAT_MESSAGE_request, 1);
  hb_msg->put_uint(16, 2);
  int now = gethrtime();
  hb_msg->put_fix_string(heartbeat_encode->crypt(sprintf("%8c%8c", now, 0)));
  // No padding.
  return heartbeat_packet(hb_msg->pop_data());
}

// verify that a certificate chain is acceptable
//
int verify_certificate_chain(array(string) certs)
{
  // do we need to verify the certificate chain?
  if(!context->verify_certificates)
    return 1;

  // if we're not requiring the certificate, and we don't provide one, 
  // that should be okay. 
  if((context->auth_level < AUTHLEVEL_require) && !sizeof(certs))
    return 1;

  // a lack of certificates when we reqiure and must verify the
  // certificates is probably a failure.
  if(!certs || !sizeof(certs))
    return 0;


  // See if the issuer of the certificate is acceptable. This means
  // the issuer of the certificate must be one of the authorities.
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

  // ok, so we have a certificate chain whose client certificate is 
  // issued by an authority known to us.
  
  // next we must verify the chain to see if the chain is unbroken

  mapping result =
    Standards.X509.verify_certificate_chain(certs,
                                            context->trusted_issuers_cache,
					    context->require_trust);

  if(result->verified)
  {
    // This data isn't actually used internally.
    session->cert_data = result;
    return 1;
  }

 return 0;
}

//! Do handshake processing. Type is one of HANDSHAKE_*, data is the
//! contents of the packet, and raw is the raw packet received (needed
//! for supporting SSLv2 hello messages).
//!
//! This function returns 0 if handshake is in progress, 1 if handshake
//! is finished, and -1 if a fatal error occurred. It uses the
//! send_packet() function to transmit packets.
int(-1..1) handle_handshake(int type, string(8bit) data, string(8bit) raw);

//! Initialize the connection state.
//!
//! @param ctx
//!   The context for the connection.
protected void create(Context ctx)
{
  current_read_state = State(this);
  current_write_state = State(this);

  if ((ctx->max_version < PROTOCOL_SSL_3_0) ||
      (ctx->max_version > PROTOCOL_TLS_MAX)) {
    ctx->max_version = PROTOCOL_TLS_MAX;
  }

  if (ctx->min_version < PROTOCOL_SSL_3_0) {
    ctx->min_version = PROTOCOL_SSL_3_0;
  } else if (ctx->min_version > ctx->max_version) {
    ctx->min_version = ctx->max_version;
  }

  version = ctx->max_version;
  context = ctx;
}


//
// --- Old connection.pike below
//


State current_read_state;
State current_write_state;
string(8bit) left_over;
Packet packet;

int sent;
int dying;
int closing; // Bitfield: 1 if a close is sent, 2 of one is received.

function(object,int|object,string:void) alert_callback;

constant PRI_alert = 1;
constant PRI_urgent = 2;
constant PRI_application = 3;

protected ADT.Queue alert_q = ADT.Queue();
protected ADT.Queue urgent_q = ADT.Queue();
protected ADT.Queue application_q = ADT.Queue();

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
protected Packet recv_packet(string(8bit) data)
{
  string(8bit)|Packet res;

  //  SSL3_DEBUG_MSG("SSL.Connection->recv_packet(%O)\n", data);
  if (left_over || !packet)
  {
    packet = Packet(2048);
    res = packet->recv( (left_over || "")  + data, version);
  }
  else
    res = packet->recv(data, version);

  if (stringp(res))
  { /* Finished a packet */
    left_over = [string]res;
    if (current_read_state) {
      SSL3_DEBUG_MSG("SSL.Connection->recv_packet(): version=0x%x\n",
		     version);
      return current_read_state->decrypt_packet(packet, version);
    } else {
      SSL3_DEBUG_MSG("SSL.Connection->recv_packet(): current_read_state is zero!\n");
      return 0;
    }
  }
  else /* Partial packet read, or error */
    left_over = 0;

  return [object]res;
}

//! Queues a packet for write. Handshake and and change cipher
//! must use the same priority, so must application data and
//! close_notifies.
void send_packet(Packet packet, int|void priority)
{
  if (closing & 1) {
    SSL3_DEBUG_MSG("SSL.Connection->send_packet: ignoring packet after close\n");
    return;
  }

  if (packet->content_type == PACKET_alert &&
      packet->description == ALERT_close_notify)
    closing |= 1;

  if (!priority)
    priority = ([ PACKET_alert : PRI_alert,
		  PACKET_change_cipher_spec : PRI_urgent,
	          PACKET_handshake : PRI_urgent,
		  PACKET_heartbeat : PRI_urgent,
		  PACKET_application_data : PRI_application ])[packet->content_type];
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

//! Extracts data from the packet queues. Returns a string of data
//! to be written, "" if there are no pending packets, 1 of the
//! connection is being closed politely, and -1 if the connection
//! died unexpectedly.
//!
//! This function is intended to be called from an i/o write callback.
string|int to_write()
{
  if (dying)
    return -1;

  Packet packet = [object(Packet)](alert_q->get() || urgent_q->get() ||
                                   application_q->get());
  if (!packet) {
    return closing ? 1 : "";
  }

  SSL3_DEBUG_MSG("SSL.Connection: writing packet of type %d, %O\n",
                 packet->content_type, packet->fragment[..6]);
  if (packet->content_type == PACKET_alert)
  {
    if (packet->level == ALERT_fatal) {
      dying = 1;
      // SSL3 5.4:
      // Alert messages with a level of fatal result in the immediate
      // termination of the connection. In this case, other
      // connections corresponding to the session may continue, but
      // the session identifier must be invalidated, preventing the
      // failed session from being used to establish new connections.
      if (session) {
	context->purge_session(session);
      }
    }
  }
  string res = current_write_state->encrypt_packet(packet, version)->send();
  if (packet->content_type == PACKET_change_cipher_spec)
    current_write_state = pending_write_state;
  return res;
}

//! Initiate close.
void send_close()
{
  send_packet(alert(ALERT_warning, ALERT_close_notify,
                    "Closing connection.\n"), PRI_application);
}

//! Send an application data packet. If the data block is too large
//! then as much as possible of the beginning of it is sent. The size
//! of the sent data is returned.
int send_streaming_data (string(8bit) data)
{
  if (!sizeof(data)) return 0;
  Packet packet = Packet();
  packet->content_type = PACKET_application_data;
  int max_packet_size = session->max_packet_size;
  int size;
  if ((!sent) && (version < PROTOCOL_TLS_1_1) &&
      (session->cipher_spec->cipher_type ==
       CIPHER_block)) {
    // Workaround for the BEAST attack.
    // This method is known as the 1/(n-1) split:
    //   Send just one byte of payload in the first packet
    //   to improve the initialization vectors in TLS 1.0.
    size = sizeof((packet->fragment = data[..0]));
    if (sizeof(data) > 1) {
      // If we have more data, take the opportunity to queue some of it too.
      send_packet(packet);

      packet = Packet();
      packet->content_type = PACKET_application_data;
      size += sizeof((packet->fragment = data[1..max_packet_size-1]));
    }
  } else {
    size = sizeof ((packet->fragment = data[..max_packet_size-1]));
  }
  send_packet (packet);
  sent += size;
  return size;
}

protected int handle_alert(string s)
{
  // sizeof(s)==2, checked at caller.
  int level = s[0];
  int description = s[1];
  if (! (ALERT_levels[level] && ALERT_descriptions[description]))
  {
    send_packet(alert(ALERT_fatal, ALERT_unexpected_message,
		      "invalid alert\n"));
    return -1;
  }
  if (level == ALERT_fatal)
  {
    SSL3_DEBUG_MSG("SSL.Connection: Fatal alert %O\n",
                   ALERT_descriptions[description]);
    return -1;
  }
  if (description == ALERT_close_notify)
  {
    SSL3_DEBUG_MSG("SSL.Connection: %O\n", ALERT_descriptions[description]);
    closing |= 2;
    return 1;
  }
  if (description == ALERT_no_certificate)
  {
    SSL3_DEBUG_MSG("SSL.Connection: %O\n", ALERT_descriptions[description]);

    if ((certificate_state == CERT_requested) && (context->auth_level == AUTHLEVEL_ask))
    {
      certificate_state = CERT_no_certificate;
      return 0;
    } else {
      send_packet(alert(ALERT_fatal,
			((certificate_state == CERT_requested)
			 ? ALERT_handshake_failure
			 : ALERT_unexpected_message),
			"Certificate required.\n"));
      return -1;
    }
  }
#ifdef SSL3_DEBUG
  else
    werror("SSL.Connection: Received warning alert %O\n",
           ALERT_descriptions[description]);
#endif
  return 0;
}

int handle_change_cipher(int c)
{
  if (!expect_change_cipher || (c != 1))
  {
    SSL3_DEBUG_MSG("SSL.Connection: handle_change_cipher: Unexcepted message!");
    send_packet(alert(ALERT_fatal, ALERT_unexpected_message,
		      "Unexpected change cipher!\n"));
    return -1;
  }
  else
  {
    current_read_state = pending_read_state;
    expect_change_cipher = 0;
    return 0;
  }
}

void send_heartbeat()
{
  if (!handshake_finished ||
      (session->heartbeat_mode != HEARTBEAT_MODE_peer_allowed_to_send)) {
    // We're not allowed to send heartbeats.
    return;
  }

  ADT.struct hb_msg = ADT.struct();
  hb_msg->put_uint(HEARTBEAT_MESSAGE_request, 1);
  hb_msg->put_uint(16, 2);
  int now = gethrtime();
  hb_msg->put_fix_string(heartbeat_encode->crypt(sprintf("%8c%8c", now, now)));
  // We pad to an even 64 bytes.
  hb_msg->put_fix_string(random_string(64 - sizeof(hb_msg)));
  send_packet(heartbeat_packet(hb_msg->pop_data()));
}

void handle_heartbeat(string(8bit) s)
{
  if (sizeof(s) < 19) return;	// Minimum size for valid heartbeats.
  ADT.struct hb_msg = ADT.struct(s);
  int hb_type = hb_msg->get_uint(1);
  int hb_len = hb_msg->get_uint(2);

  SSL3_DEBUG_MSG("SSL.Connection: Heartbeat %s (%d bytes)",
		 fmt_constant(hb_type, "HEARTBEAT_MESSAGE"), hb_len);

  string(8bit) payload;
  int pad_len = 16;

  // RFC 6520 4:
  // If the payload_length of a received HeartbeatMessage is too
  // large, the received HeartbeatMessage MUST be discarded silently.
  if ((hb_len < 0) || ((hb_len + 16) > sizeof(hb_msg))) {
#ifdef SSL3_SIMULATE_HEARTBLEED
    payload = hb_msg->get_rest();
    if (sizeof(payload) < hb_len) {
      payload = payload + random_string(hb_len - sizeof(payload));
    } else {
      payload = payload[..hb_len-1];
    }
#else
    return;
#endif
  } else {
    payload = hb_msg->get_fix_string(hb_len);
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
    hb_msg = ADT.struct();
    hb_msg->put_uint(HEARTBEAT_MESSAGE_response, 1);
    hb_msg->put_uint(hb_len, 2);
    hb_msg->put_fix_string(payload);
    hb_msg->put_fix_string(random_string(pad_len));
    send_packet(heartbeat_packet(hb_msg->pop_data()));
    break;
  case HEARTBEAT_MESSAGE_response:
    // RFC 6520 4:
    // If a received HeartbeatResponse message does not contain the
    // expected payload, the message MUST be discarded silently.
    if ((sizeof(payload) == 16) && heartbeat_decode) {
      hb_msg = ADT.struct(heartbeat_decode->crypt(payload));
      int a = hb_msg->get_uint(8);
      int b = hb_msg->get_uint(8);
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

string(8bit) alert_buffer = "";
string(8bit) handshake_buffer = "";

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
string(8bit)|int got_data(string(8bit) data)
{
  if (closing & 2) {
    // The peer has closed the connection.
    return 1;
  }
  // If closing == 1 we continue to try to read a remote close
  // message. That enables the caller to check for a clean close, and
  // to get the leftovers after the SSL connection.

  /* If alert_callback is called, this data is passed as an argument */
  string(8bit) alert_context = (left_over || "") + data;

  string(8bit) res = "";
  Packet packet;
  while (packet = recv_packet(data))
  {
    data = "";

    if (packet->is_alert)
    { /* Reply alert */
      SSL3_DEBUG_MSG("SSL.Connection: Bad received packet\n");
      send_packet(packet);
      if (alert_callback)
	alert_callback(packet, current_read_state->seq_num, alert_context);
      if ((!packet) || (!this) || (packet->level == ALERT_fatal))
	return -1;
      if (alert_callback)
	break;
    }
    else
    {
      SSL3_DEBUG_MSG("SSL.Connection: received packet of type %d\n",
                     packet->content_type);
      switch (packet->content_type)
      {
      case PACKET_alert:
       {
	 SSL3_DEBUG_MSG("SSL.Connection: ALERT\n");

	 int i;
	 int err = 0;
	 alert_buffer += packet->fragment;
	 for (i = 0;
	      !err && ((sizeof(alert_buffer) - i) >= 2);
	      i+= 2)
	   err = handle_alert(alert_buffer[i..i+1]);

	 alert_buffer = alert_buffer[i..];
	 if (err)
	   if (err > 0 && sizeof (res))
	     // If we get a close then we return the data we got so far.
	     return res;
	   else
	     return err;
	 break;
       }
      case PACKET_change_cipher_spec:
       {
	 SSL3_DEBUG_MSG("SSL.Connection: CHANGE_CIPHER_SPEC\n");

	 int i;
	 int err;
	 for (i = 0; (i < sizeof(packet->fragment)); i++)
	 {
	   err = handle_change_cipher(packet->fragment[i]);
           SSL3_DEBUG_MSG("tried change_cipher: %d\n", err);
	   if (err)
	     return err;
	 }
	 break;
       }
      case PACKET_handshake:
       {
	 SSL3_DEBUG_MSG("SSL.Connection: HANDSHAKE\n");

	 if (handshake_finished && !secure_renegotiation) {
	   // Don't allow renegotiation in unsecure mode, to address
	   // http://web.nvd.nist.gov/view/vuln/detail?vulnId=CVE-2009-3555.
	   // For details see: http://www.g-sec.lu/practicaltls.pdf and
	   // RFC 5746.
	   send_packet(alert(ALERT_warning, ALERT_no_renegotiation,
			     "Renegotiation not supported in unsecure mode.\n"));
	   return -1;
	 }
	 if (expect_change_cipher)
	 {
	   /* No change_cipher message was received */
	   // FIXME: There's a bug somewhere since expect_change_cipher often
	   // remains set after the handshake is completed. The effect is that
	   // renegotiation doesn't work all the time.
	   //
	   // A side effect is that we are partly invulnerable to the
	   // renegotiation vulnerability mentioned above. It is however not
	   // safe to assume that, since there might be routes past this,
	   // maybe through the use of a version 2 hello message below.
	   send_packet(alert(ALERT_fatal, ALERT_unexpected_message,
			     "Expected change cipher.\n"));
	   return -1;
	 }
	 int err, len;
	 handshake_buffer += packet->fragment;

	 while (sizeof(handshake_buffer) >= 4)
	 {
	   sscanf(handshake_buffer, "%*c%3c", len);
	   if (sizeof(handshake_buffer) < (len + 4))
	     break;
           mixed exception = catch {
               err = handle_handshake(handshake_buffer[0],
                                      handshake_buffer[4..len + 3],
                                      handshake_buffer[.. len + 3]);
             };
           if( exception )
           {
             if( objectp(exception) && ([object]exception)->ADT_struct )
             {
               Error.Generic e = [object(Error.Generic)]exception;
               send_packet(alert(ALERT_fatal, ALERT_decode_error,
                                 e->message()));
               return -1;
             }
             throw(exception);
           }
	   handshake_buffer = handshake_buffer[len + 4..];
	   if (err < 0)
	     return err;
	   if (err > 0) {
	     handshake_finished = 1;
	   }
	 }
	 break;
       }
      case PACKET_application_data:
	SSL3_DEBUG_MSG("SSL.Connection: APPLICATION_DATA\n");

	if (!handshake_finished)
	{
	  send_packet(alert(ALERT_fatal, ALERT_unexpected_message,
			    "Handshake not finished yet!\n"));
	  return -1;
	}
	res += packet->fragment;
	break;
      case PACKET_heartbeat:
	{
	  // RFC 6520.
	  SSL3_DEBUG_MSG("SSL.Connection: Heartbeat.\n");
	  if (!handshake_finished) {
	    // RFC 6520 3:
	    // The receiving peer SHOULD discard the message silently,
	    // if it arrives during the handshake.
	    break;
	  }
	  if (!session->heartbeat_mode) {
	    // RFC 6520 2:
	    // If an endpoint that has indicated peer_not_allowed_to_send
	    // receives a HeartbeatRequest message, the endpoint SHOULD
	    // drop the message silently and MAY send an unexpected_message
	    // Alert message.
	    send_packet(alert(ALERT_warning, ALERT_unexpected_message,
			      "Heart beat mode not enabled.\n"));
	    break;
	  }

          mixed exception = catch {
              handle_heartbeat(packet->fragment);
            };
          if( exception )
          {
            if( objectp(exception) && ([object]exception)->ADT_struct )
            {
              Error.Generic e = [object(Error.Generic)]exception;
              send_packet(alert(ALERT_fatal, ALERT_decode_error,
                                e->message()));
              return -1;
            }
            throw(exception);
          }

	}
	break;
      default:
	if (!handshake_finished)
	{
	  send_packet(alert(ALERT_fatal, ALERT_unexpected_message,
			    "Unexpected message during handshake!\n"));
	  return -1;
	}
	// RFC 4346 6:
	//   If a TLS implementation receives a record type it does not
	//   understand, it SHOULD just ignore it.
	SSL3_DEBUG_MSG("SSL.Connection: Ignoring packet of type %s\n",
		       fmt_constant(packet->content_type, "PACKET"));
	break;
      }
    }
  }
  if (sizeof(res)) return res;
  if (closing & 2) return 1;
  return "";
}
