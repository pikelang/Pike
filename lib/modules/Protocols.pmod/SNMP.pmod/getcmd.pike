//
// getcmd.pike
// Example of using low level SNMP API
//
//


object o;
mixed rv, co;

void rc(mapping rdata, int idx) {

  o->set_blocking();
  o->to_pool(rdata);
  rv = o->read_response(idx);

  write(sprintf("Returned object: %O\n", rv));
 
  exit(0);
}

void tt() {

  remove_call_out(co);
  o->set_blocking();

  write("Timed out!\n");
  exit(1);
}

int main(int argc, array(string) argv) {

  mapping m;
  string host = sizeof(argv) > 1 ? argv[1] : "localhost";
  
  o = Protocols.SNMP.protocol();
  // o->snmp_community = "public";
  int idx = o->get_request(({
    "1.3.6.1.2.1.1.1.0",	// system.sysDescr  -> String
    "1.3.6.1.2.1.1.3.0",	// system.sysUpTime -> TimeTicks
    "1.3.6.1.2.1.6.9.0"		// tcp.tcpCurrEstab -> Gauge
   }), host,161);
write("Index: "+(string)idx+"\n");
  
#define NONBLOCK
#ifdef NONBLOCK
  o->set_read_callback(rc, idx);
  o->set_nonblocking();
  co = call_out(tt, 2);
  return(-13);
#else
  rv = o->read_response(idx);
  write(sprintf("Returned object: %O\n", rv));
#endif

}
