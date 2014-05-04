#pike __REAL_VERSION__
#pragma strict_types

//! Alert packet.

import .Constants;

//! Based on the base @[Packet].
inherit .Packet;

int(1..2) level;
int(0..255) description;

string message;

constant is_alert = 1;

//!
void create(int(1..2) level, int(0..255) description,
            ProtocolVersion version, string|void message)
{
  if ((version == PROTOCOL_SSL_3_0) &&
      (description == ALERT_no_renegotiation)) {
    // RFC 5746 4.5:
    // SSLv3 does not define the "no_renegotiation" alert (and does not
    // offer a way to indicate a refusal to renegotiate at a "warning"
    // level). SSLv3 clients that refuse renegotiation SHOULD use a
    // fatal handshake_failure alert.
    this_program::description = ALERT_handshake_failure;
    this_program::level = ALERT_fatal;
  }

  if (! ALERT_levels[level])
    error( "Invalid level\n" );
  if (! ALERT_descriptions[description])
    error( "Invalid description\n" );

  this_program::level = level;
  this_program::description = description;
  this_program::message = message;

#ifdef SSL3_DEBUG
  if(message)
    werror(message);
#endif

  ::create();
  content_type = PACKET_alert;
  protocol_version = version;
  fragment = sprintf("%1c%1c", level, description);
}
