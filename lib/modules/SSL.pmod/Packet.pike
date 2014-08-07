#pike __REAL_VERSION__

//! SSL Record Layer. Handle formatting and parsing of packets.

import .Constants;

int content_type;
ProtocolVersion protocol_version;
string(8bit) fragment;  /* At most 2^14 */

constant HEADER_SIZE = 5;

private string buffer = "";
private int needed_chars = HEADER_SIZE;

// The fragment max size is 2^14 (RFC 5246 6.2.1). Compressed
// fragments are however allowed to be 1024 bytes over (6.2.2), and
// Ciphertexts 2048 bytes (6.2.3). State the additional headroom in
// this variable.
protected int marginal_size;

/* Circular dependence */
program Alert = master()->resolv("SSL")["Alert"];

//! @param version
//!   The version sent packets will be created for.
//! @param extra
//!   Additional fragment size, over the 2^14 bytes for a plaintext
//!   TLS fragment.
protected void create(ProtocolVersion version, void|int extra)
{
  protocol_version = version;
  marginal_size = extra;
}

protected object check_size(string data, int extra)
{
  if (sizeof(data) > (PACKET_MAX_SIZE + extra))
    return Alert(ALERT_fatal, ALERT_unexpected_message, version);
  marginal_size = extra;
  fragment = data;
}

object set_plaintext(string data)
{
  check_size(data, 0);
}

object set_compressed(string data)
{
  check_size(data, 1024);
}

object set_encrypted(string data)
{
  check_size(data, 2048);
}

//! Receive data read from the network.
//!
//! @param data
//!   Raw data from the network.
//!
//! @returns
//!   Returns a string of leftover data if packet is complete,
//!   otherwise @expr{0@}.
//!
//!   If there's an error, an alert object is returned.
//!
string(8bit)|.Packet recv(string(8bit) data)
{
  buffer += data;
  while (sizeof(buffer) >= needed_chars)
  {
    if (needed_chars == HEADER_SIZE)
    {
      content_type = buffer[0];
      fragment = 0;
      int length;

      /* Support only short SSL2 headers */
      if ( (content_type & 0x80) && (buffer[2] == 1) )
      {
#ifdef SSL3_DEBUG
	werror("SSL.Packet: Receiving SSL2 packet %O\n", buffer[..4]);
#endif
        return Alert(ALERT_fatal, ALERT_insufficient_security, PROTOCOL_SSL_3_0);
      }
      if( !PACKET_types[content_type] )
        return Alert(ALERT_fatal, ALERT_unexpected_message, protocol_version);

      sscanf(buffer, "%*c%2c%2c", protocol_version, length);
      if ( (length <= 0) || (length > (PACKET_MAX_SIZE + marginal_size)))
	return Alert(ALERT_fatal, ALERT_unexpected_message, protocol_version);
      if ((protocol_version & ~0xff) != PROTOCOL_SSL_3_0)
	return Alert(ALERT_fatal, ALERT_unexpected_message, protocol_version,
		     sprintf("SSL.Packet->send: Version %d.%d "
			     "is not supported\n",
			     protocol_version>>8, protocol_version & 0xff));
#ifdef SSL3_DEBUG
      if (protocol_version > PROTOCOL_TLS_MAX)
	werror("SSL.Packet->recv: received version %d.%d packet\n",
	       protocol_version>>8, protocol_version & 0xff);
#endif

      needed_chars += length;
    } else {
      fragment = buffer[HEADER_SIZE .. needed_chars - 1];
      return buffer[needed_chars ..];
    }
  }
  return 0;
}

//! Serialize the packet for sending.
//!
//! @returns
//!   Returns the serialized packet.
string send()
{
  if (! PACKET_types[content_type] )
    error( "Invalid type" );
  
  if ((protocol_version & ~0xff) != PROTOCOL_SSL_3_0)
    error( "Version %d is not supported\n", protocol_version>>8 );
#ifdef SSL3_DEBUG
  if (protocol_version > PROTOCOL_TLS_MAX)
    werror("SSL.Packet->send: Sending version %d.%d packet\n",
	   protocol_version>>8, protocol_version & 0xff);
#endif
  if (sizeof(fragment) > (PACKET_MAX_SIZE + marginal_size))
    error( "Maximum packet size exceeded\n" );

  return sprintf("%c%2c%2c%s", content_type, protocol_version,
		 sizeof(fragment), fragment);
}
