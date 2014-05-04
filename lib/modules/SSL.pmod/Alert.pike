#pike __REAL_VERSION__
#pragma strict_types

//! Alert packet.

import .Constants;

//! Based on the base @[packet].
inherit .Packet;

int(1..2) level;
int(0..255) description;

string message;

constant is_alert = 1;

//! @decl void create(int(1..2) level, int(0..255) description,@
//!                   Protocolversion version, string|void message)
void create(int(1..2) l, int(0..255) d, ProtocolVersion version, string|void m)
{
  if ((version == PROTOCOL_SSL_3_0) && (d == ALERT_no_renegotiation)) {
    // RFC 5746 4.5:
    // SSLv3 does not define the "no_renegotiation" alert (and does not
    // offer a way to indicate a refusal to renegotiate at a "warning"
    // level). SSLv3 clients that refuse renegotiation SHOULD use a
    // fatal handshake_failure alert.
    d = ALERT_handshake_failure;
    l = ALERT_fatal;
  }

  if (! ALERT_levels[l])
    error( "Invalid level\n" );
  if (! ALERT_descriptions[d])
    error( "Invalid description\n" );

  level = l;
  description = d;
  message = m;

#ifdef SSL3_DEBUG
  if(m)
    werror(m);
#endif

  ::create();
  content_type = PACKET_alert;
  protocol_version = version;
  fragment = sprintf("%1c%1c", level, description);
}
