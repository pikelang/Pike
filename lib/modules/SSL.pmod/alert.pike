#pike __REAL_VERSION__
#pragma strict_types

//! Alert packet.

import .Constants;

//! Based on the base @[packet].
inherit .packet;

int level;
int description;

string message;

constant is_alert = 1;

//! @decl void create(int level, int description,@
//!                   Protocolversion version, string|void message)
void create(int l, int d, ProtocolVersion version, string|void m)
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
  fragment = sprintf("%c%c", level, description);
}
