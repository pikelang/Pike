#pike 7.8
//#pragma strict_types

#pragma no_deprecation_warnings

//! SSL packet layer.
//!
//! @[SSL.connection] inherits @[SSL.handshake], and in addition to the state in
//! the handshake super class, it contains the current read and write
//! states, packet queues. This object is responsible for receiving and
//! sending packets, processing handshake packets, and providing a clear
//! text packages for some application.

// SSL/TLS Protocol Specification documents:
//
// SSL 2		http://wp.netscape.com/eng/security/SSL_2.html
// SSL 3.0		http://wp.netscape.com/eng/ssl3/draft302.txt
//			(aka draft-freier-ssl-version3-02.txt).
// TLS 1.0 (SSL 3.1)	RFC 2246 "The TLS Protocol Version 1.0".
// TLS 1.1 (SSL 3.2)	draft-ietf-tls-rfc2246-bis
// Renegotiation	RFC 5746 "Renegotiation Indication Extension".

#if constant(SSL.Cipher.CipherAlgorithm)

.state current_read_state;
.state current_write_state;
string left_over;
Packet packet;

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

void create(int is_server, void|SSL.context ctx,
	    void|ProtocolVersion min_version,
	    void|ProtocolVersion max_version)
{
  alert::create();
  urgent::create();
  application::create();
  current_read_state = SSL.state(this);
  current_write_state = SSL.state(this);
  handshake::create(is_server, ctx, min_version, max_version);
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
protected object recv_packet(string data)
{
  mixed res;

  //  SSL3_DEBUG_MSG("SSL.connection->recv_packet(%O)\n", data);
  if (left_over || !packet)
  {
    packet = Packet(2048);
    res = packet->recv( (left_over || "")  + data, version[1]);
  }
  else
    res = packet->recv(data, version[1]);

  if (stringp(res))
  { /* Finished a packet */
    left_over = [string]res;
    if (current_read_state) {
      SSL3_DEBUG_MSG("Decrypting packet.. version[1]="+version[1]+"\n");
      return current_read_state->decrypt_packet(packet,version[1]);
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
    if (packet->level == ALERT_fatal)
      dying = 1;
  }
  string res = current_write_state->encrypt_packet(packet,version[1])->send();
  if (packet->content_type == PACKET_change_cipher_spec)
    current_write_state = pending_write_state;
  return res;
}

//! Initiate close.
void send_close()
{
  send_packet(Alert(ALERT_warning, ALERT_close_notify, version[1]),
	      PRI_application,);
}

//! Send an application data packet. If the data block is too large
//! then as much as possible of the beginning of it is sent. The size
//! of the sent data is returned.
int send_streaming_data (string data)
{
  Packet packet = Packet();
  packet->content_type = PACKET_application_data;
  int size = sizeof ((packet->fragment = data[..PACKET_MAX_SIZE-1]));
  send_packet (packet);
  return size;
}

int handle_alert(string s)
{
  int level = s[0];
  int description = s[1];
  if (! (ALERT_levels[level] && ALERT_descriptions[description]))
  {
    send_packet(Alert(ALERT_fatal, ALERT_unexpected_message, version[1],
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
				: ALERT_unexpected_message), version[1]));
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
    send_packet(Alert(ALERT_fatal, ALERT_unexpected_message, version[1]));
    return -1;
  }
  else
  {
    current_read_state = pending_read_state;
    expect_change_cipher = 0;
    return 0;
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
			       version[1]));
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
			     version[1]));
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
	  send_packet(Alert(ALERT_fatal, ALERT_unexpected_message, version[1]));
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
			       version[1]));
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
      }
    }
  }
  return closing & 2 ? 1 : res;
}

#else // constant(SSL.Cipher.CipherAlgorithm)
constant this_program_does_not_exist = 1;
#endif
