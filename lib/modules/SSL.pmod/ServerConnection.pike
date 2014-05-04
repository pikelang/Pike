#pike __REAL_VERSION__
#pragma strict_types
#require constant(SSL.Cipher)

#ifdef SSL3_DEBUG
#define SSL3_DEBUG_MSG(X ...)  werror(X)
#else
#define SSL3_DEBUG_MSG(X ...)
#endif

import .Constants;
inherit .Connection;

#define Packet .Packet

Packet hello_request()
{
  return handshake_packet(HANDSHAKE_hello_request, "");
}

protected void create(.context ctx)
{
  ::create(ctx);
  handshake_state = STATE_server_wait_for_hello;
}