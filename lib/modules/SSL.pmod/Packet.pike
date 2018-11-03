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
  if (version >= PROTOCOL_TLS_1_3)
    version = PROTOCOL_TLS_1_2; // TLS 1.3 record_version
  protocol_version = version;
  marginal_size = extra;
}

protected variant void create(ProtocolVersion version,
                              int content_type, string(8bit) fragment)
{
  if (version >= PROTOCOL_TLS_1_3)
    version = PROTOCOL_TLS_1_2; // TLS 1.3 record_version
  protocol_version = version;
  this::content_type = content_type;
  this::fragment = fragment;
}

protected object check_size(string data, int extra)
{
  if (sizeof(data) > (PACKET_MAX_SIZE + extra))
    return Alert(ALERT_fatal, ALERT_record_overflow, version);
  marginal_size = extra;
  fragment = data;
  return 0;
}

object set_plaintext(string data)
{
  return check_size(data, 0);
}

object set_compressed(string data)
{
  return check_size(data, 1024);
}

object set_encrypted(string data)
{
  return check_size(data, 2048);
}

//! Receive data read from the network.
//!
//! @param data
//!   Raw data from the network.
//!
//! @returns
//!   Returns a @expr{1@} data if packet is complete, otherwise
//!   @expr{0@}.
//!
//!   If there's an error, an alert object is returned.
//!
int(-1..1) recv(Stdio.Buffer data)
{
  if(sizeof(data)<HEADER_SIZE) return 0;

  Stdio.Buffer.RewindKey key = data->rewind_key();
  content_type = data->read_int8();
  if( !PACKET_types[content_type] )
  {
    key->rewind();
    return -1;
  }

  protocol_version = data->read_int16();
  if ((protocol_version & ~0xff) != PROTOCOL_SSL_3_0 ||
      protocol_version > PROTOCOL_TLS_1_2)
    return -1;

  int length = data->read_int16();
  if ( (length <= 0) || (length > (PACKET_MAX_SIZE + marginal_size)))
    return -1;

  if(length > sizeof(data))
  {
    key->rewind();
    return 0;
  }

  fragment = data->read(length);
  return 1;
}

//! Serialize the packet for sending.
void send(Stdio.Buffer output)
{
  if (! PACKET_types[content_type] )
    error( "Invalid type" );

  if (sizeof(fragment) > (PACKET_MAX_SIZE + marginal_size))
    error( "Maximum packet size exceeded\n" );

  output->add_int8(content_type);
  output->add_int16(protocol_version);
  output->add_hstring(fragment, 2);
}

protected string _sprintf(int t)
{
  if(t!='O') return UNDEFINED;
  if(!fragment) return sprintf("SSL.Packet(unfinished)");
  string type = fmt_constant(content_type, "PACKET")[7..];
  return sprintf("SSL.Packet(%s)", type);
}
