#pike __REAL_VERSION__
#pragma strict_types
#require constant(SSL.Cipher)

import .Constants;
inherit .Connection;

protected void create(.context ctx)
{
  ::create(ctx);
  handshake_state = STATE_client_wait_for_hello;
  handshake_messages = "";
  session = context->new_session();
  send_packet(client_hello());
}