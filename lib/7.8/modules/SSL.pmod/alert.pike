#pike 7.8
#pragma strict_types

#pragma no_deprecation_warnings

//! Alert packet.

import .Constants;

//! Based on the base @[packet].
inherit .packet : packet;

int level;
int description;

string message;
mixed trace;

constant is_alert = 1;

//! @decl void create(int level, int description,@
//!                   int version, string|void message, mixed|void trace)
void create(int l, int d, int version, string|void m, mixed|void t)
{
  if (!version && (d == ALERT_no_renegotiation)) {
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
  trace = t;

#ifdef SSL3_DEBUG
  if(m)
    werror("%s", [string]m);
  if(t)
    werror(describe_backtrace(t));
#endif

  packet::create();
  packet::content_type = PACKET_alert;
  packet::protocol_version = ({ PROTOCOL_major, version });
  packet::fragment = sprintf("%c%c", level, description);
}
