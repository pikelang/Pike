//! Establish a TCP/IP socket connection to the given host and port.
//! If successful, resolves with an open @[File] object.
//! Follows the Happy Eyeballs algorithm as defined in RFC 8305.
class HappyEyeballs {
	constant RESOLUTION_DELAY = 0.05; //If IPv4 results come in first, delay by this many seconds in case IPv6 comes in.
	//TODO: Make it configurable which family is preferred (currently, hard-coded to prefer IPv6)
	constant FIRST_ADDRESS_FAMILY_WEIGHTING = 1; //Number of favoured addresses to attempt before going to the other family
	constant CONNECTION_ATTEMPT_DELAY = 0.25; //Minimum gap between socket connection attempts
	Protocols.DNS.async_client resolver;
	protected void create() {resolver = Protocols.DNS.async_client();}

	class Connection(string host, int port) {
		Concurrent.Promise promise = Concurrent.Promise();
		//Addresses not yet connected to
		//Connection attempts will remove addresses from all three arrays.
		array(string) ipv6, ipv4;
		array(string) addresses;
		System.Timer connection_delay;
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
			write("Got IPv6: %O%{ %s%}\n", host, ipv6);
			//TODO: Sort according to Destination Address Selection
			attempt_connect(); //Preferred family immediately causes connection attempt
		}
		void got_ipv4(string host, mapping result) {
			ipv4 = result->an->a - ({0});
			write("Got IPv4: %O%{ %s%}\n", host, ipv4);
			//Note that this will spawn a second connection attempt iterator,
			//which may cause some minor unnecessary load.
			if (ipv6) attempt_connect(); //Unpreferred family delays connection attempt unless we have both results.
			else call_out(attempt_connect, RESOLUTION_DELAY);
		}

		void attempt_connect() {
			//Step 2: Sort addresses, interleaving address families as available.
			//We actually don't sort the addresses per se; instead, we select from
			//the IPv6 or IPv4 pool based on the current address weight counter.
			float wait = 0.0;
			if (!connection_delay) connection_delay = System.Timer();
			else wait = CONNECTION_ATTEMPT_DELAY - connection_delay->peek();
			if (wait > 0.0) {call_out(attempt_connect, wait); return;}
			int family;
			int have4 = ipv4 && sizeof(ipv4);
			if (ipv6 && sizeof(ipv6)) family = (!have4 || address_weight > 0) ? 6 : 4;
			else if (have4) family = 4;
			else {
				//No addresses left. If one of the lookups isn't done yet, we may get
				//retriggered (by the duplicate spawn, cf got_ipv4), but otherwise,
				//we've started every connection we're going to, and it's up to the
				//connections to succeed or fail.
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
			write("Attempting to connect to %s via %s...\n", host, address);
			Stdio.File new_con = Stdio.File();
			if (!new_con->async_connect(address, port, got_connection, new_con)) {
				//Unable to initialize a socket for this address family. Immediately
				//reject this entire family and try again.
				if (family == 6) ipv6 = ({ });
				else ipv4 = ({ });
				connection_delay = 0;
				attempt_connect();
				return;
			}
			++connecting;
		}

		void got_connection(int success, Stdio.File conn) {
			if (connecting < 0) return; //Already succeeded!
			if (!success) {
				if (--connecting || !ipv4 || !ipv6 || sizeof(ipv4) || sizeof(ipv6)) {
					//Other attempts are already being made.
					//Should we speed up the connection attempts if one fails fast?
					return;
				}
				//We've run out of addresses to try. The connection cannot succeed.
				promise->failure(error("Unable to connect to %s.\n", host));
				return;
			}
			promise->success(conn);
			ipv4 = ipv6 = ({ }); //No more attempts needed, yay!
		}
	}

	Concurrent.Future connect(string host, int port) {
		return Connection(host, port)->promise->future();
	}
}
