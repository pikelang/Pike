#pike __REAL_VERSION__

//:
//: Honza Petrous, 2000-10-07 (on the 'coding party' after user conference :)

//! SNMP protocol implementation for Pike, implementing @rfc{1155@}
//! and @rfc{1901@}.

#include "snmp_globals.h"
#include "snmp_errors.h"

inherit Stdio.UDP : snmp;

import Standards.ASN1.Types;

#define U(C,T) make_combined_tag(C,T)

protected mapping snmp_type_proc =
  ([
    // from RFC-1065 :
    U(1,0) : OctetString, // ipaddress
    U(1,1) : Integer,     // counter
    U(1,2) : Integer,     // gauge
    U(1,3) : Integer,     // timeticks
    U(1,4) : OctetString, // opaque

    // v2
    U(1,6) : Integer,     // counter64

    // context PDU
    U(2,0) : Sequence,
    U(2,1) : Sequence,
    U(2,2) : Sequence,
    U(2,3) : Sequence,
  ]);


//:
//: public vars
//:


//:
//:  private variables
//:
private int remote_port; // = SNMP_DEFAULT_PORT;
private string local_host = SNMP_DEFAULT_HOST;
private string remote_host;
private int request_id = 1;
private int next_id = 1;

//! SNMP version
//!
//! currently version 1 and 2 are supported.
int snmp_version = SNMP_DEFAULT_VERSION;

//! SNMP community string
//!
//! should be set to the appropriate SNMP community before sending a request.
//!
//! @note
//!  Set to "public" by default.
string snmp_community = SNMP_DEFAULT_COMMUNITY;

int snmp_errno = SNMP_SUCCESS;
int ok;

//:
//: msg pool
//:
private mapping msgpool = ([]);

//:
//: callback support
//:
private function con_fail;
private array extra_args;

//! create a new SNMP protocol object
//!
//! @param rem_port
//! @param rem_addr
//!   remote address and UDP port (optional)
//! @param loc_port
//! @param loc_addr
//!   local address and UDP port (optional)
//!
protected void create(int|void rem_port, string|void rem_addr,
		      int|void loc_port, string|void loc_addr) {

  int lport = loc_port;

  local_host = (!loc_addr || !sizeof(loc_addr)) ? SNMP_DEFAULT_LOCHOST : loc_addr;
  if(stringp(rem_addr) && sizeof(rem_addr)) remote_host = rem_addr;
  if(rem_port) remote_port = rem_port;

  if (!snmp::bind(lport, local_host, 1)) {
    //# error ...
    DWRITE("protocol.create: can't bind to the socket.\n");
    ok = 0;
    if(con_fail)
      con_fail(this, @extra_args);
  }

  if(snmp_errno)
    ERROR("Failed to bind to SNMP port.\n");
  DWRITE("protocol.bind: success!\n");

  DWRITE("protocol.create: local adress:port bound: [%s:%d].\n",
	 local_host, lport);

}


//! return the whole SNMP message in raw format
mapping|zero readmsg(int|float|void timeout) {
  mapping rv;

  if(timeout && !wait(timeout))
    return 0;

  rv = read();
  return rv;
}

//! decode ASN1 data, if garbaged ignore it
mapping decode_asn1_msg(mapping rawd) {

  Object xdec = Standards.ASN1.Decode.simple_der_decode(rawd->data,
                                                        snmp_type_proc);
  string msgid = (string)xdec->elements[2]->elements[0]->value;
  int errno = xdec->elements[2]->elements[1]->value;
  mapping msg = ([ "ip":rawd->ip,
		   "port":rawd->port,
		   "error-status":errno,
		   "error-string":snmp_errlist[errno],
		   "error-index":xdec->elements[2]->elements[2]->value,
		   "version":xdec->elements[0]->value,
		   "community":xdec->elements[1]->value,
		   "op":xdec->elements[2]->get_tag(),
		   "attribute":map(xdec->elements[2]->elements[3]->elements,
				   lambda(object duo) {
				     return ([
				       (array(string))(duo->elements[0]->id)*".":duo->elements[1]->value
				     ]);
				   } )
  ]);

  return ([msgid:msg]);
}


//! decode raw pdu message and place in message pool
void to_pool(mapping rawd) {
  //: put decoded msg to the pool
  msgpool += decode_asn1_msg(rawd);
}

mapping|int from_pool(string msgid) {
  //: get data from poll
  return m_delete(msgpool, msgid);
}


//! read decoded message from pool
private mapping readmsg_from_pool(int msgid) {
  //: read SNMP response PDU from PDU pool

  mapping rv = from_pool((string)msgid);
  if(rv)
    return rv;

#define HACK 1
#ifdef HACK
  mapping tmpm = readmsg();
  to_pool(tmpm);
#endif

  return from_pool((string)msgid);
}



private int writemsg(string rem_addr, int rem_port, object pdu) {
  //: send SNMP encoded message and return status
  //: OK, in most cases :)

  object msg;
  string rawd;
  int msize;

  msg = Sequence(({
	   Integer(snmp_version-1),
           OctetString(snmp_community),
	   pdu}));

  rawd = msg->get_der();

  DWRITE("protocol.writemsg: %O\n", rawd);

  msize = send(rem_addr, rem_port, rawd);
  return (msize = sizeof(rawd) ? SNMP_SUCCESS : SNMP_SEND_ERROR);
}


private int get_req_id() {
  //: returns unicate id

  //LOCK
  int rv = request_id++;
  //UNLOCK
  return rv;
}

//:
//: read_response
//:
array(mapping)|int read_response(int msgid) {
  mapping rawpdu = readmsg_from_pool(msgid);


  return ({rawpdu});
}

//!
//! GetRequest-PDU call
//!
//! @param varlist
//!   an array of OIDs to GET
//! @param rem_addr
//! @param rem_port
//!   remote address an UDP port to send request to (optional)
//! @returns
//!   request ID
int get_request(array(string) varlist, string|void rem_addr,
                         int|void rem_port) {
  object pdu;
  int id = get_req_id(), flg;
  array vararr = ({});

  foreach(varlist, string varname)
    vararr += ({Sequence(
	        ({Identifier(@(array(int))(varname/".")),
                  Null()})
	      )});

  pdu = ASN1_CONTEXT_SEQUENCE(0,
	       ({Integer(id), // request-id
		 Integer(0), // error-status
		 Integer(0), // error-index
		 Sequence(vararr)})
	       );

  // now we have PDU ...
  flg = writemsg(rem_addr||remote_host,
		 rem_port || remote_port || SNMP_DEFAULT_PORT, pdu);

  return id;
}

Object mk_asn1_val(string type, int|string val) {
// returns appropriate ASN.1 value

  Object rv;

  switch(type) {
    case "oid":		// OID
	rv = Identifier( @(array(int))(val/".") );
	break;

    case "int":		// INTEGER
	rv = Integer(val);
	break;

    case "str":		// STRING
	rv = OctetString(val);
	break;

    case "ipaddr":	// ipAddress
	rv = OctetString(val[..3]);
        rv->cls = 1;
        rv->tag = 0;
	break;

    case "count":	// COUNTER
	rv = Integer(val);
        rv->cls = 1;
        rv->tag = 1;
	break;

    case "gauge":	// GAUGE
	rv = Integer(val);
        rv->cls = 1;
        rv->tag = 2;
	break;

    case "tick":	// TICK
	rv = Integer(val);
        rv->cls = 1;
        rv->tag = 3;
	break;

    case "opaque":	// OPAQUE
	rv = OctetString(val);
        rv->cls = 1;
        rv->tag = 4;
	break;

    case "count64":	// COUNTER64 - v2 object
	rv = Integer(val);
        rv->cls = 1;
        rv->tag = 6;
	break;

    default:		// bad type!
        error("Unknown SNMP data type %O.", type);
  }

  return rv;
}

//!
//! GetResponse-PDU call
//!
//! @param varlist
//!   a mapping containing data to return
//!   @mapping
//!     @member array oid1
//!       @array
//!         @elem string type
//!            data type such as tick, oid, gauge, etc
//!         @elem mixed data
//!            data to return for oid1
//!       @endarray
//!     @member array oid2
//!       @array
//!         @elem string type
//!            data type such as tick, oid, gauge, etc
//!         @elem mixed data
//!            data to return for oid2
//!       @endarray
//!     @member array oidn
//!       @array
//!         @elem string type
//!            data type such as tick, oid, gauge, etc
//!         @elem mixed data
//!            data to return for oidn
//!       @endarray
//!   @endmapping
//! @param origdata
//!   original received decoded pdu that this response corresponds to
//! @param errcode
//!   error code
//! @param erridx
//!   error index
//! @returns
//!   request ID
int get_response(mapping varlist, mapping origdata, int|void errcode,
		 int|void erridx) {
  //: GetResponse-PDU low call
  object pdu;
  int id = indices(origdata)[0], flg;
  array vararr = ({});

  foreach(indices(varlist), string varname)
    if(arrayp(varlist[varname]) || sizeof(varlist[varname]) > 1) {
      vararr += ({Sequence(
		 ({Identifier(@(array(int))(varname/".")),
		   mk_asn1_val(varlist[varname][0], varlist[varname][1])})
	      )});
    }

  pdu = ASN1_CONTEXT_SEQUENCE(2,
	       ({Integer(id), // request-id
		 Integer(errcode), // error-status
		 Integer(erridx), // error-index
		 Sequence(vararr)})
	       );

  // now we have PDU ...
  flg = writemsg(origdata[id]->ip||remote_host,
		 origdata[id]->port || remote_port || SNMP_DEFAULT_PORT, pdu);

  return id;
}


//!
//! GetNextRequest-PDU call
//!
//! @param varlist
//!   an array of OIDs to GET
//! @param rem_addr
//! @param rem_port
//!   remote address an UDP port to send request to (optional)
//! @returns
//!   request ID
int get_nextrequest(array(string) varlist, string|void rem_addr,
                         int|void rem_port) {
  //: GetNextRequest-PDU low call
  object pdu;
  int id = get_req_id(), flg;
  array vararr = ({});

  foreach(varlist, string varname)
    vararr += ({Sequence(
	        ({Identifier(@(array(int))(varname/".")),
                  Null()})
	      )});

  pdu = ASN1_CONTEXT_SEQUENCE(1,
	       ({Integer(id), // request-id
		 Integer(0), // error-status
		 Integer(0), // error-index
		 Sequence(vararr)})
	       );

  // now we have PDU ...
  flg = writemsg(rem_addr||remote_host,
		 rem_port || remote_port || SNMP_DEFAULT_PORT, pdu);

  return id;
}

//!
//! SetRequest-PDU call
//!
//! @param varlist
//!   a mapping of OIDs to SET
//!   @mapping
//!     @member array oid1
//!       @array
//!         @elem string type
//!            data type such as tick, oid, gauge, etc
//!         @elem mixed data
//!            data to return for oid1
//!       @endarray
//!     @member array oid2
//!       @array
//!         @elem string type
//!            data type such as tick, oid, gauge, etc
//!         @elem mixed data
//!            data to return for oid2
//!       @endarray
//!     @member array oidn
//!       @array
//!         @elem string type
//!            data type such as tick, oid, gauge, etc
//!         @elem mixed data
//!            data to return for oidn
//!       @endarray
//!   @endmapping
//! @param rem_addr
//! @param rem_port
//!   remote address an UDP port to send request to (optional)
//! @returns
//!   request ID
//!
//! @example
//!   // set the value of 1.3.6.1.4.1.1882.2.1 to "blah".
//!   object s=Protocols.SNMP.protocol();
//!   s->snmp_community="mysetcommunity";
//!   mapping req=(["1.3.6.1.4.1.1882.2.1": ({"str", "blah"})]);
//!   int id=s->set_request(req, "172.21.124.32");
//!
int set_request(mapping varlist, string|void rem_addr,
                         int|void rem_port) {
  //: SetRequest-PDU low call
  object pdu;
  int id = get_req_id(), flg;
  array vararr = ({});

  foreach(indices(varlist), string varname)
    vararr += ({Sequence(
	        ({Identifier(@(array(int))(varname/".")),
		  mk_asn1_val(varlist[varname][0], varlist[varname][1])})
	      )});

  pdu = ASN1_CONTEXT_SEQUENCE(3,
	   ({Integer(id), // request-id
             Integer(0), // error-status
             Integer(0), // error-index
             Sequence(vararr)})
       );

  // now we have PDU ...
  flg = writemsg(rem_addr||remote_host,
		 rem_port || remote_port || SNMP_DEFAULT_PORT, pdu);

  return id;
}


//! send an SNMP-v1 trap
//!
//! @param varlist
//!   a mapping of OIDs to include in trap
//!   @mapping
//!     @member array oid1
//!       @array
//!         @elem string type
//!            data type such as tick, oid, gauge, etc
//!         @elem mixed data
//!            data to return for oid1
//!       @endarray
//!     @member array oid2
//!       @array
//!         @elem string type
//!            data type such as tick, oid, gauge, etc
//!         @elem mixed data
//!            data to return for oid2
//!       @endarray
//!     @member array oidn
//!       @array
//!         @elem string type
//!            data type such as tick, oid, gauge, etc
//!         @elem mixed data
//!            data to return for oidn
//!       @endarray
//!   @endmapping
//! @param oid
//! @param type
//!   generic trap-type
//! @param spectype
//!   specific trap-type
//! @param ticks
//!   uptime
//! @param locip
//!   originating ip address of the trap
//! @param remaddr
//! @param remport
//!   address and UDP to send trap to
//! @returns
//!   request id
int trap(mapping varlist, string oid, int type, int spectype, int ticks,
         string|void locip, string|void remaddr, int|void remport) {
  //: Trap-PDU low call
  object pdu;
  int id = get_req_id(), flg;
  array vararr = ({});
  string lip = "1234";

  locip = locip || "0.0.0.0";
  if (has_value(locip,":")) // FIXME: Can't handle IPv6
    locip = "0.0.0.0";
  if (sizeof(locip/".") != 4)
    locip = "0.0.0.0"; //FIXME: what for hell I want to do with such ugly value?
  sscanf(locip, "%d.%d.%d.%d", lip[0], lip[1], lip[2], lip[3]);

  foreach(indices(varlist), string varname)
    vararr += ({Sequence(
	        ({Identifier(@(array(int))(varname/".")),
		  mk_asn1_val(varlist[varname][0], varlist[varname][1])})
	      )});
  pdu = ASN1_CONTEXT_SEQUENCE(4,
	  ({
	    // enterprise OID
	    mk_asn1_val("oid", oid),

	    // ip address (UGLY!)
	    mk_asn1_val("ipaddr", lip),

	    // type (0 = coldstart, ...)
	    mk_asn1_val("int", type),

	    // enterprise type
	    mk_asn1_val("int", spectype),

	    // uptime
	    mk_asn1_val("tick", ticks),

	    // optional vars
	    Sequence(vararr)
	  })

       );

  // now we have PDU ...
  flg = writemsg(remaddr||remote_host,
		 remport || remote_port || SNMP_DEFAULT_TRAPPORT, pdu);

  return id;
}
