/* packet.pike
 *
 * SSL Record Layer */

inherit "constants";

constant SUPPORT_V2 = 1;

int content_type;
array(int) protocol_version;
string fragment;  /* At most 2^14 */

constant HEADER_SIZE = 5;

private string buffer;
private int needed_chars;

int marginal_size;

// constant Alert = (program) "alert";
#define Alert ((program) "alert")

void create(void|int extra)
{
  marginal_size = extra;
  buffer = "";
  needed_chars = HEADER_SIZE;
  protocol_version = ({ 3, 0 });
}

object check_size(int|void extra)
{
  marginal_size = extra;
  return (strlen(fragment) > (PACKET_MAX_SIZE + extra))
    ? Alert(ALERT_fatal, ALERT_unexpected_message) : 0;
}

/* Called with data read from network.
 *
 * Return string of leftover data if packet is complete, otherwise 0.
 * If there's an error, an alert object is returned.
 */

object|string recv(string data)
{
  buffer += data;
  while (strlen(buffer) >= needed_chars)
  {
//    werror(sprintf("SSL.packet->recv: needed = %d, avail = %d\n",
//		     needed_chars, strlen(buffer)));
    if (needed_chars == HEADER_SIZE)
    {
      content_type = buffer[0];
      int length;
      if (! PACKET_types[content_type] )
      {
	if (SUPPORT_V2)
	{
//	  werror(sprintf("SSL.packet: Recieving SSL2 packet '%s'\n", buffer[..4]));

	  content_type = PACKET_V2;
	  if ( (!(buffer[0] & 0x80)) /* Support only short SSL2 headers */
	       || (buffer[2] != 1))
	    return Alert(ALERT_fatal, ALERT_unexpected_message);
	  length = ((buffer[0] & 0x7f) << 8 | buffer[1]
		    - 3);
//	  werror(sprintf("SSL2 length = %d\n", length));
	  protocol_version = values(buffer[3..4]);
	}
	else
	  return Alert(ALERT_fatal, ALERT_unexpected_message,
		       "SSL.packet->recv: invalid type\n", backtrace());
      } else {
	protocol_version = values(buffer[1..2]);
	sscanf(buffer[3..4], "%2c", length);
	if ( (length <= 0) || (length > (PACKET_MAX_SIZE + marginal_size)))
	  return Alert(ALERT_fatal, ALERT_unexpected_message);
      }
      if (protocol_version[0] != 3)
	return Alert(ALERT_fatal, ALERT_unexpected_message,
		     sprintf("SSL.packet->send: Version %d is not supported\n",
			     protocol_version[0]), backtrace());
      if (protocol_version[1] > 0)
	werror(sprintf("SSL.packet->recv: recieved version %d.%d packet\n",
		       @ protocol_version));
      
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

string send()
{
  if (! PACKET_types[content_type] )
    throw( ({ "SSL.packet->send: invalid type", backtrace() }) );
  
  if (protocol_version[0] != 3)
    throw( ({ sprintf("SSL.packet->send: Version %d is not supported\n",
		      protocol_version[0]), backtrace() }) );
  if (protocol_version[1] > 0)
    werror(sprintf("SSL.packet->send: recieved version %d.%d packet\n",
		   @ protocol_version));
  if (strlen(fragment) > (PACKET_MAX_SIZE + marginal_size))
    throw( ({ "SSL.packet->send: maximum packet size exceeded\n",
		backtrace() }) );
  return sprintf("%c%c%c%2c%s", content_type, @protocol_version,
		 strlen(fragment), fragment);
}
