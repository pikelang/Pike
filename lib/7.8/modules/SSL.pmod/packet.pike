#pike 7.8

/*
 * SSL Record Layer
 */

//! SSL Record Layer. Handle formatting and parsing of packets.


import .Constants;

constant SUPPORT_V2 = 1;

int content_type;
array(int) protocol_version;
string fragment;  /* At most 2^14 */

constant HEADER_SIZE = 5;

private string buffer;
private int needed_chars;

int marginal_size;

/* Circular dependence */
program Alert = master()->resolv("SSL")["alert"];
// #define Alert ((program) "alert")

void create(void|int extra)
{
  marginal_size = extra;
  buffer = "";
  needed_chars = HEADER_SIZE;
}

object check_size(ProtocolVersion version, int|void extra)
{
  marginal_size = extra;
  return (sizeof(fragment) > (PACKET_MAX_SIZE + extra))
    ? Alert(ALERT_fatal, ALERT_unexpected_message, version) : 0;
}

//! Receive data read from the network.
//!
//! @param data
//!   Raw data from the network.
//!
//! @param version
//!   Minor version of the SSL 3 protocol suite to create a packet for.
//!
//! @returns
//!   Returns a string of leftover data if packet is complete,
//!   otherwise @expr{0@}.
//!
//!   If there's an error, an alert object is returned.
//!
object|string recv(string data, ProtocolVersion version)
{

#ifdef SSL3_FRAGDEBUG
  werror(" SSL.packet->recv: sizeof(data)="+sizeof(data)+"\n");
#endif 


  buffer += data;
  while (sizeof(buffer) >= needed_chars)
  {
    if (needed_chars == HEADER_SIZE)
    {
      content_type = buffer[0];
      int length;
      if (! PACKET_types[content_type] )
      {
	if (SUPPORT_V2)
	{
#ifdef SSL3_DEBUG
	  werror("SSL.packet: Receiving SSL2 packet %O\n", buffer[..4]);
#endif

	  content_type = PACKET_V2;
	  if ( (!(buffer[0] & 0x80)) /* Support only short SSL2 headers */
	       || (buffer[2] != 1))
	    return Alert(ALERT_fatal, ALERT_unexpected_message, version);
	  length = ((buffer[0] & 0x7f) << 8 | buffer[1]
		    - 3);
	  protocol_version = values(buffer[3..4]);
	}
	else
	  return Alert(ALERT_fatal, ALERT_unexpected_message, version,
		       "SSL.packet->recv: invalid type\n", backtrace());
      } else {
	protocol_version = values(buffer[1..2]);
	sscanf(buffer[3..4], "%2c", length);
	if ( (length <= 0) || (length > (PACKET_MAX_SIZE + marginal_size)))
	  return Alert(ALERT_fatal, ALERT_unexpected_message, version);
      }
      if (protocol_version[0] != PROTOCOL_major)
	return Alert(ALERT_fatal, ALERT_unexpected_message, version,
		     sprintf("SSL.packet->send: Version %d.%d "
			     "is not supported\n",
			     protocol_version[0], protocol_version[1]),
		     backtrace());
#ifdef SSL3_DEBUG
      if (protocol_version[1] > PROTOCOL_minor)
	werror("SSL.packet->recv: received version %d.%d packet\n",
	       @ protocol_version);
#endif

      needed_chars += length;
    } else {
      if (content_type == PACKET_V2)
	fragment = buffer[2 .. needed_chars - 1];
      else
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
  
  if (protocol_version[0] != PROTOCOL_major)
    error( "Version %d is not supported\n", protocol_version[0] );
#ifdef SSL3_DEBUG
  if (protocol_version[1] > PROTOCOL_minor)
    werror("SSL.packet->send: received version %d.%d packet\n",
	   @ protocol_version);
#endif
  if (sizeof(fragment) > (PACKET_MAX_SIZE + marginal_size))
    error( "Maximum packet size exceeded\n" );

  return sprintf("%c%c%c%2c%s", content_type, @protocol_version,
		 sizeof(fragment), fragment);
}
