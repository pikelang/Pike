#pike __REAL_VERSION__
#pragma strict_types

//! Alert packet.

import .Constants;

//! Based on the base @[Packet].
inherit .Packet;

int(1..2) level;
int(8bit) description;

string message;

constant is_alert = 1;

//!
protected void create(int(1..2) level, int(8bit) description,
		      ProtocolVersion version, string|void message)
{
  if ((version == PROTOCOL_SSL_3_0) &&
      (description == ALERT_no_renegotiation)) {
    // RFC 5746 4.5:
    // SSLv3 does not define the "no_renegotiation" alert (and does not
    // offer a way to indicate a refusal to renegotiate at a "warning"
    // level). SSLv3 clients that refuse renegotiation SHOULD use a
    // fatal handshake_failure alert.
    this::description = ALERT_handshake_failure;
    this::level = ALERT_fatal;
  }

  if (! ALERT_levels[level])
    error( "Invalid level\n" );
  if (! ALERT_descriptions[description])
    error( "Invalid description\n" );

  this::level = level;
  this::description = description;
  this::message = message;

#ifdef SSL3_DEBUG
  if(message)
    werror(message);
#endif

  ::create(version);
  content_type = PACKET_alert;
  fragment = sprintf("%1c%1c", level, description);
}
