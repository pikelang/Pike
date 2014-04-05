#pike __REAL_VERSION__
//#pragma strict_types
#require constant(SSL.Cipher)

//! SSL packet layer.
//!
//! @[SSL.connection] inherits @[SSL.handshake], and in addition to the state in
//! the handshake super class, it contains the current read and write
//! states, packet queues. This object is responsible for receiving and
//! sending packets, processing handshake packets, and providing clear
//! text packets for some application.

// SSL/TLS Protocol Specification documents:
//
// SSL 2		http://wp.netscape.com/eng/security/SSL_2.html
// SSL 3.0		http://wp.netscape.com/eng/ssl3/draft302.txt
//			(aka draft-freier-ssl-version3-02.txt).
// TLS 1.0 (SSL 3.1)	RFC 2246 "The TLS Protocol Version 1.0".
// TLS 1.1 (SSL 3.2)	draft-ietf-tls-rfc2246-bis
// Renegotiation	RFC 5746 "Renegotiation Indication Extension".

.state current_read_state;
.state current_write_state;
string left_over;
Packet packet;

int sent;
int dying;
int closing; // Bitfield: 1 if a close is sent, 2 of one is received.

function(object,int|object,string:void) alert_callback;

import .Constants;

inherit .handshake;

#ifdef SSL3_DEBUG
#define SSL3_DEBUG_MSG(X ...)  werror(X)
#else /*! SSL3_DEBUG */
#define SSL3_DEBUG_MSG(X ...)
#endif /* SSL3_DEBUG */

constant PRI_alert = 1;
constant PRI_urgent = 2;
constant PRI_application = 3;

inherit ADT.Queue : alert;
inherit ADT.Queue : urgent;
inherit ADT.Queue : application;

void create(int is_server, void|SSL.context ctx)
{
  alert::create();
  urgent::create();
  application::create();
  current_read_state = SSL.state(this);
  current_write_state = SSL.state(this);
  handshake::create(is_server, ctx);
}

#if 0
protected void destroy()
{
  werror("Connection destructed:\n%s\n", describe_backtrace(backtrace()));
}
#endif /* 0 */

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
protected object recv_packet(string data)
{
  mixed res;

  //  SSL3_DEBUG_MSG("SSL.connection->recv_packet(%O)\n", data);
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
      SSL3_DEBUG_MSG("SSL.connection->recv_packet(): version=0x%x\n",
		     version);
      return current_read_state->decrypt_packet(packet, version);
    } else {
      SSL3_DEBUG_MSG("SSL.connection->recv_packet(): current_read_state is zero!\n");
      return 0;
    }
  }
  else /* Partial packet read, or error */
    left_over = 0;
  return res;
}

//! Queues a packet for write. Handshake and and change cipher
//! must use the same priority, so must application data and
//! close_notifies.
void send_packet(object packet, int|void priority)
{
  if (closing & 1) {
    SSL3_DEBUG_MSG("SSL.connection->send_packet: ignoring packet after close\n");
    return;
  }

  if (packet->content_type == PACKET_alert &&
      packet->description == ALERT_close_notify)
    closing |= 1;

#ifdef SSL3_FRAGDEBUG
  werror(" SSL.connection->send_packet: sizeof(packet)="+sizeof(packet)+"\n");
#endif
  if (!priority)
    priority = ([ PACKET_alert : PRI_alert,
		  PACKET_change_cipher_spec : PRI_urgent,
	          PACKET_handshake : PRI_urgent,
		  PACKET_heartbeat : PRI_urgent,
		  PACKET_application_data : PRI_application ])[packet->content_type];
  SSL3_DEBUG_MSG("SSL.connection->send_packet: type %d, desc %d, pri %d, %O\n",
		 packet->content_type, packet->description, priority,
		 packet->fragment[..5]);
  switch (priority)
  {
  default:
    error( "Internal error\n" );
  case PRI_alert:
    alert::put(packet);
    break;
  case PRI_urgent:
    urgent::put(packet);
    break;
  case PRI_application:
    application::put(packet);
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

  object packet = alert::get() || urgent::get() || application::get();
  if (!packet) {
    return closing ? 1 : "";
  }

  SSL3_DEBUG_MSG("SSL.connection: writing packet of type %d, %O\n",
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
  send_packet(Alert(ALERT_warning, ALERT_close_notify, version),
	      PRI_application,);
}

//! Send an application data packet. If the data block is too large
//! then as much as possible of the beginning of it is sent. The size
//! of the sent data is returned.
int send_streaming_data (string data)
{
  if (!sizeof(data)) return 0;
  Packet packet = Packet();
  packet->content_type = PACKET_application_data;
  int max_packet_size = current_write_state->session->max_packet_size;
  int size;
  if ((!sent) && (version < PROTOCOL_TLS_1_1) &&
      (current_write_state->session->cipher_spec->cipher_type ==
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

int handle_alert(string s)
{
  int level = s[0];
  int description = s[1];
  if (! (ALERT_levels[level] && ALERT_descriptions[description]))
  {
    send_packet(Alert(ALERT_fatal, ALERT_unexpected_message, version,
		      "SSL.connection->handle_alert: invalid alert\n", backtrace()));
    return -1;
  }
  if (level == ALERT_fatal)
  {
    SSL3_DEBUG_MSG("SSL.connection: Fatal alert %d\n", description);
    return -1;
  }
  if (description == ALERT_close_notify)
  {
    SSL3_DEBUG_MSG("SSL.connection: Close notify  alert %d\n", description);
    closing |= 2;
    return 1;
  }
  if (description == ALERT_no_certificate)
  {
    SSL3_DEBUG_MSG("SSL.connection: No certificate  alert %d\n", description);

    if ((certificate_state == CERT_requested) && (context->auth_level == AUTHLEVEL_ask))
    {
      certificate_state = CERT_no_certificate;
      return 0;
    } else {
      send_packet(Alert(ALERT_fatal, ((certificate_state == CERT_requested)
			       ? ALERT_handshake_failure
				: ALERT_unexpected_message), version));
      return -1;
    }
  }
#ifdef SSL3_DEBUG
  else
    werror("SSL.connection: Received warning alert %d\n", description);
#endif
  return 0;
}

int handle_change_cipher(int c)
{
  if (!expect_change_cipher || (c != 1))
  {
    SSL3_DEBUG_MSG("SSL.connection: handle_change_cipher: Unexcepted message!");
    send_packet(Alert(ALERT_fatal, ALERT_unexpected_message, version));
    return -1;
  }
  else
  {
    current_read_state = pending_read_state;
    expect_change_cipher = 0;
    return 0;
  }
}

private Crypto.AES heartbeat_encode;
private Crypto.AES heartbeat_decode;

Packet heartbeat_packet(string s)
{
  Packet packet = Packet();
  packet->content_type = PACKET_heartbeat;
  packet->fragment = s;
  return packet;
}

void send_heartbeat()
{
  if (!handshake_finished ||
      (session->heartbeat_mode != HEARTBEAT_MODE_peer_allowed_to_send)) {
    // We're not allowed to send heartbeats.
    return;
  }

  if (!heartbeat_encode) {
    // NB: We encrypt the payload with a random AES key
    //     to reduce the amount of known plaintext in
    //     the heartbeat masseages. This is needed now
    //     that many cipher suites (such as GCM and CCM)
    //     use xor with a cipher stream, to reduce risk
    //     of revealing larger segments of the stream.
    heartbeat_encode = Crypto.AES();
    heartbeat_decode = Crypto.AES();
    string heartbeat_key = random_string(16);
    heartbeat_encode->set_encrypt_key(heartbeat_key);
    heartbeat_decode->set_decrypt_key(heartbeat_key);
  }

  ADT.struct hb_msg = ADT.struct();
  hb_msg->put_uint(HEARTBEAT_MESSAGE_request, 1);
  hb_msg->put_uint(16, 2);
  int now = gethrtime();
  hb_msg->put_fix_string(heartbeat_encode->crypt(sprintf("%8c%8c", now, now)));
  // We pad to an even 64 bytes.
  hb_msg->put_fix_string(random_string(64 - sizeof(hb_msg)));
  send_packet(heartbeat_packet(hb_msg->get()));
}

void handle_heartbeat(string s)
{
  if (sizeof(s) < 19) return;	// Minimum size for valid heartbeats.
  ADT.struct hb_msg = ADT.struct(s);
  int hb_type = hb_msg->get_uint(1);
  int hb_len = hb_msg->get_uint(2);

  SSL3_DEBUG_MSG("SSL.connection: Heartbeat %s (%d bytes)",
		 fmt_constant("HEARTBEAT_MESSAGE_", hb_type), hb_len);

  // RFC 6520 4:
  // If the payload_length of a received HeartbeatMessage is too
  // large, the received HeartbeatMessage MUST be discarded silently.
  if ((hb_len < 0) || ((hb_len + 16) > sizeof(hb_msg))) return;

  string payload = hb_msg->get_fix_string(hb_len);
  int pad_len = sizeof(hb_msg);

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
    hb_msg->put_unit(hb_len, 2);
    hb_msg->put_fix_string(payload);
    hb_msg->put_fix_string(random_string(pad_len));
    send_packet(heartbeat_packet(hb_msg->get()));
    break;
  case HEARTBEAT_MESSAGE_response:
    // RFC 6520 4:
    // If a received HeartbeatResponse message does not contain the
    // expected payload, the message MUST be discarded silently.
    if ((sizeof(payload) == 16) && heartbeat_decode) {
      hb_msg = ADT.struct(heartbeat_decode->crypt(payload));
      int a = hb_msg->get_uint(8);
      int b = hb_msg->get_uint(8);
      if (a != b) break;
      int delta = gethrtime() - a;
      SSL3_DEBUG_MSG("SSL.connection: Heartbeat roundtrip: %dus", delta);
    }
    break;
  default:
    break;
  }
}

string alert_buffer = "";
string handshake_buffer = "";

//! Main receive handler. Returns a string of received application
//! data, or 1 if a close was received, or -1 if an error occurred.
//!
//! This function is intended to be called from an i/o read callback.
string|int got_data(string|int s)
{
  if(!stringp(s)) {
    return s;
  }

  if (closing & 2) {
    return 1;
  }
  // If closing == 1 we continue to try to read a remote close
  // message. That enables the caller to check for a clean close, and
  // to get the leftovers after the SSL connection.

  /* If alert_callback is called, this data is passed as an argument */
  string alert_context = (left_over || "") + s;

  string res = "";
  object packet;
  while (packet = recv_packet(s))
  {
    s = "";

    if (packet->is_alert)
    { /* Reply alert */
      SSL3_DEBUG_MSG("SSL.connection: Bad received packet\n");
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
      SSL3_DEBUG_MSG("SSL.connection: received packet of type %d\n",
                     packet->content_type);
      switch (packet->content_type)
      {
      case PACKET_alert:
       {
	 SSL3_DEBUG_MSG("SSL.connection: ALERT\n");

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
	 SSL3_DEBUG_MSG("SSL.connection: CHANGE_CIPHER_SPEC\n");

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
	 SSL3_DEBUG_MSG("SSL.connection: HANDSHAKE\n");

	 if (handshake_finished && !secure_renegotiation) {
	   // Don't allow renegotiation in unsecure mode, to address
	   // http://web.nvd.nist.gov/view/vuln/detail?vulnId=CVE-2009-3555.
	   // For details see: http://www.g-sec.lu/practicaltls.pdf and
	   // RFC 5746.
	   send_packet (Alert (ALERT_warning, ALERT_no_renegotiation,
			       version));
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
	   send_packet(Alert(ALERT_fatal, ALERT_unexpected_message,
			     version));
	   return -1;
	 }
	 int err, len;
	 handshake_buffer += packet->fragment;

	 while (sizeof(handshake_buffer) >= 4)
	 {
	   sscanf(handshake_buffer, "%*c%3c", len);
	   if (sizeof(handshake_buffer) < (len + 4))
	     break;
	   err = handle_handshake(handshake_buffer[0],
				  handshake_buffer[4..len + 3],
				  handshake_buffer[.. len + 3]);
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
	SSL3_DEBUG_MSG("SSL.connection: APPLICATION_DATA\n");

	if (!handshake_finished)
	{
	  send_packet(Alert(ALERT_fatal, ALERT_unexpected_message, version));
	  return -1;
	}
	res += packet->fragment;
	break;
      case PACKET_V2:
       {
	 SSL3_DEBUG_MSG("SSL.connection: V2\n");

	 if (handshake_finished) {
	   // Don't allow renegotiation using SSLv2 packets at all, to address
	   // http://web.nvd.nist.gov/view/vuln/detail?vulnId=CVE-2009-3555.
	   send_packet (Alert (ALERT_warning, ALERT_no_renegotiation,
			       version));
	   return -1;
	 }
	 int err = handle_handshake(HANDSHAKE_hello_v2,
				    packet->fragment[1 .. ],
				    packet->fragment);
	 // FIXME: Can err ever be 1 here? In that case we're probably
	 // not returning the right value below.
	 if (err)
	   return err;
       }
       break;
      case PACKET_heartbeat:
	{
	  // RFC 6520.
	  SSL3_DEBUG_MSG("SSL.connection: Heartbeat.\n");
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
	    send_packet(Alert(ALERT_warning, ALERT_unexpected_message,
			      version));
	    break;
	  }
	  handle_heartbeat(packet->fragment);
	}
	break;
      default:
	if (!handshake_finished)
	{
	  send_packet(Alert(ALERT_fatal, ALERT_unexpected_message, version));
	  return -1;
	}
	// RFC 4346 6:
	//   If a TLS implementation receives a record type it does not
	//   understand, it SHOULD just ignore it.
	SSL3_DEBUG_MSG("SSL.connection: Ignoring packet of type %s\n",
		       fmt_constant("PACKET_", packet->content_type));
	break;
      }
    }
  }
  return closing & 2 ? 1 : res;
}
