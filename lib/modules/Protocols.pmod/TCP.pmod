//! Utilities relating to the TCP/IP protocol family.

#ifdef HAPPY_EYEBALLS_DEBUG
#define DBG(X ...) werror(X)
#else
#define DBG(X ...)
#endif

//! Establish a TCP/IP socket connection to the given host and port.
//! If successful, resolves with an open @[File] object.
//! Follows the Happy Eyeballs algorithm as defined in RFC 8305.
//! @example
//!   @code
//!     // Attempt to connect to an HTTP server via either IPv6 or IPv4
//!     object he = Protocols.TCP.HappyEyeballs();
//!     he->connect("pike.lysator.liu.se", 80)->then() {
//!       [Stdio.File sock] = __ARGS__;
//!        write("Connected socket: %O\n", sock);
//!     };
//!   @endcode
class HappyEyeballs {
  //! If IPv4 results come in first, delay by this many seconds in case IPv6 comes in.
  constant RESOLUTION_DELAY = 0.05;

  //! Number of favoured addresses to attempt before going to the other family
  constant FIRST_ADDRESS_FAMILY_WEIGHTING = 1;

  //! Minimum gap between socket connection attempts
  constant CONNECTION_ATTEMPT_DELAY = 0.25;

  //! DNS resolver to use. Defaults to a vanilla async_client; may be replaced as required.
  Protocols.DNS.async_client resolver;

  //!
  protected void create() {resolver = Protocols.DNS.async_client();}

  //! Pending connection
  protected class Connection(string host, int port, array bt) {
    Concurrent.Promise promise = Concurrent.Promise();
    //Addresses not yet connected to
    //Connection attempts will remove addresses from all three arrays.
    array(string) ipv6, ipv4;
    array(string) addresses;
    System.Timer connection_delay = System.Timer();
    float no_connection_before = 0.0;
    mixed next_connection_call_out;
    int address_weight = FIRST_ADDRESS_FAMILY_WEIGHTING;
    int connecting = 0; //Number of in-flight socket connections

    protected void create() {
      //Step 1: Resolve name to AAAA and A in parallel
      resolver->do_query(host, Protocols.DNS.C_IN, Protocols.DNS.T_AAAA, got_ipv6);
      resolver->do_query(host, Protocols.DNS.C_IN, Protocols.DNS.T_A, got_ipv4);
    }

    //NOTE: Currently, error responses are treated identically to empty
    //success responses.
    void got_ipv6(string host, mapping result) {
      ipv6 = result->an->aaaa - ({0});
      DBG("Got IPv6: %O%{ %s%}\n", host, ipv6);
      //TODO: Sort according to Destination Address Selection
      attempt_connect(); //Preferred family immediately causes connection attempt
    }
    void got_ipv4(string host, mapping result) {
      ipv4 = result->an->a - ({0});
      DBG("Got IPv4: %O%{ %s%}\n", host, ipv4);
      //Unpreferred family delays connection attempt unless we have both results.
      attempt_connect(!ipv6 && RESOLUTION_DELAY);
    }

    void attempt_connect(float|void wait) {
      if (connecting < 0) return;
      if (connection_delay) wait = max(wait, no_connection_before - connection_delay->peek());
      DBG("Attempt connect, wait %.3fs\n", (float)wait);
      if (next_connection_call_out) remove_call_out(next_connection_call_out);
      if (wait > 0.0) {next_connection_call_out = call_out(attempt_connect, wait); return;}
      next_connection_call_out = 0;
      //Step 2: Sort addresses, interleaving address families as available.
      //We actually don't sort the addresses per se; instead, we select from
      //the IPv6 or IPv4 pool based on the current address weight counter.
      int family;
      int have4 = ipv4 && sizeof(ipv4);
      if (ipv6 && sizeof(ipv6)) family = (!have4 || address_weight > 0) ? 6 : 4;
      else if (have4) family = 4;
      else {
        //No addresses left. If one of the lookups isn't done yet, we'll get
        //retriggered, but otherwise, we've started every connection we're
        //going to, and it's up to the connections to succeed or fail.
        if (!connecting && ipv4 && ipv6) got_connection(0, 0);
        return;
      }
      string address;
      if (family == 6) {
        [address, ipv6] = Array.shift(ipv6);
        --address_weight;
      }
      else {
        [address, ipv4] = Array.shift(ipv4);
        address_weight = FIRST_ADDRESS_FAMILY_WEIGHTING;
      }
      DBG("Attempting to connect to %s via %s...\n", host, address);
      Stdio.File new_con = Stdio.File();
      if (!new_con->async_connect(address, port, got_connection, new_con)) {
        //Unable to initialize a socket for this address family. Immediately
        //reject this entire family and try again.
        if (family == 6) ipv6 = ({ });
        else ipv4 = ({ });
        attempt_connect();
        return;
      }
      ++connecting;
      DBG("That makes %d connections in flight.\n", connecting);
      connection_delay->get();
      no_connection_before = CONNECTION_ATTEMPT_DELAY;
      attempt_connect(); //Queue the next connection attempt
    }

    void got_connection(int success, Stdio.File conn) {
      if (connecting < 0) return; //Already succeeded!
      if (!success) {
        if (--connecting > 0) return; //Other connections are in flight, all's good
        if (!ipv4 || sizeof(ipv4) || !ipv6 || sizeof(ipv6)) {
          //There are more addresses to try. Hasten failures by queueing
          //another connection ASAP.
          DBG("Hastening failure reconnect\n");
          no_connection_before = 0.025;
          attempt_connect();
          return;
        }
        //if (!ipv4 || !ipv6) return; //There are DNS lookups in flight, hang tight, try later.
        //We've run out of addresses to try. The connection cannot succeed.
        promise->failure(({sprintf("Unable to connect to %s.\n", host), bt}));
        connecting = -1;
        return;
      }
      promise->success(conn);
      connecting = -1; //No more attempts needed, yay!
    }
  }

  //! Establish a connection to the given @[host]:@[port].
  //! Resolves with a connected socket, or rejects with a failure message.
  Concurrent.Future connect(string host, int port) {
    return Connection(host, port, backtrace())->promise->future();
  }
}
