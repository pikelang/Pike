#pike __REAL_VERSION__

//! SSL Record Layer. Handle formatting and parsing of packets.

inherit .Packet;

import .Constants;

constant HEADER_SIZE = 13;

int `dtls_epoch()
{
  return (seq_num>>48) & 0xffff;
}

void `dtls_epoch=(int val)
{
  seq_num = (seq_num & 0xffffffffffff) | ((val & 0xffff)<<48);
}

int `dtls_sequence_number()
{
  return seq_num & 0xffffffffffff;
}

void `dtls_sequence_number=(int val)
{
  seq_num = (seq_num & ~0xffffffffffff) | (val & 0xffffffffffff);
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
int(-1..1) recv(Stdio.Buffer data)
{
#ifdef SSL3_FRAGDEBUG
  werror(" SSL.DTLSPacket->recv: sizeof(data)="+sizeof(data)+"\n");
#endif

  if(sizeof(data)<HEADER_SIZE) return 0;

  Stdio.Buffer.RewindKey key = data->rewind_key();
  content_type = data->read_int8();
  if( !PACKET_types[content_type] )
  {
    key->rewind();
    return -1;
  }

  protocol_version = data->read_int16();
  if (((protocol_version & ~0xff) != 0xfe00) || (protocol_version == 0xfefe))
    return -1;

#ifdef SSL3_DEBUG
  if (protocol_version < PROTOCOL_DTLS_MAX)
    werror("SSL.DTLSPacket->recv: received %s packet\n",
	   fmt_version(protocol_version));
#endif

  seq_num = data->read_int64();

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
//!
//! @returns
//!   Returns the serialized packet.
string send(Stdio.Buffer output)
{
  if (! PACKET_types[content_type] )
    error( "Invalid type" );
  
  if (sizeof(fragment) > (PACKET_MAX_SIZE + marginal_size))
    error( "Maximum packet size exceeded\n" );

  if (((protocol_version & ~0xff) != 0xfe00) || (protocol_version == 0xfefe))
    error( "%s is not a supported version of DTLS.\n",
	   fmt_version(protocol_version));
#ifdef SSL3_DEBUG
  if (protocol_version < PROTOCOL_DTLS_MAX)
    werror("SSL.DTLSPacket->send: Sending %s packet\n",
	   fmt_version(protocol_version));
#endif


  output->add_int8(content_type);
  output->add_int16(protocol_version);
  output->add_int64(seq_num);
  output->add_hstring(fragment, 2);
}
