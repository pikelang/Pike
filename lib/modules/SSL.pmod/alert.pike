#pike __REAL_VERSION__
#pragma strict_types

// $Id: alert.pike,v 1.11 2004/04/19 23:01:03 agehall Exp $

//! Alert package.

import .Constants;
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
    werror(m);
  if(t)
    werror(describe_backtrace(t));
#endif

  packet::create();
  packet::content_type = PACKET_alert;
  packet::protocol_version = ({ 3, version });
  packet::fragment = sprintf("%c%c", level, description);
}
    
