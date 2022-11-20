#pike __REAL_VERSION__

//! TLS handshake packet.

int handshake_type;

protected variant void create(ProtocolVersion version,
			      int handshake_type,
			      string(8bit)|Buffer|object(Stdio.Buffer) data)
{
  this::handshake_type = handshake_type;
  ::create(version, PACKET_handshake, data);
}

protected string(8bit) format_payload()
{
  return sprintf("%1c%3H", handshake_type, fragment);
}