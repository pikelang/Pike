/* $Id: alert.pike,v 1.3 2000/08/04 19:07:11 sigge Exp $
 *
 */

inherit "packet" : packet;

int level;
int description;

string message;
mixed trace;

constant is_alert = 1;

object create(int l, int d, string|void m, mixed|void t)
{
  if (! ALERT_levels[l])
    throw( ({ "SSL.alert->create: Invalid level\n", backtrace() }));
  if (! ALERT_descriptions[d])
    throw( ({ "SSL.alert->create: Invalid description\n", backtrace() }));    

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
  packet::protocol_version = ({ 3, 0 });
  packet::fragment = sprintf("%c%c", level, description);
}
    
