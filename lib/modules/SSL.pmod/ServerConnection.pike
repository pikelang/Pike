#pike __REAL_VERSION__
#pragma strict_types
#require constant(SSL.Cipher)

import .Constants;
inherit .Connection;

protected void create(.context ctx)
{
  ::create(ctx);
  handshake_state = STATE_server_wait_for_hello;
}