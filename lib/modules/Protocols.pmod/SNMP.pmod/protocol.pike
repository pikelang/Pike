#pike __REAL_VERSION__

//:
//: Honza Petrous, 2000-10-07 (on the 'coding party' after user conference :)
//! SNMP protocol implementation for Pike
//!
//! RFCs:
//!
//!  implemented (yet):
//!      1155-7 : v1
//!
//!      1901-4 : v2/community (Bulk ops aren't implemented!)
//!
//!  planned:
//!	 2742   : agentX
//!
//!      2570   : v3 description
//!

// $Id$


#include "snmp_globals.h"
#include "snmp_errors.h"

#if 1
// --- ASN.1 hack
class asn1_application_octet_string
{
  inherit Standards.ASN1.Types.asn1_octet_string;
  constant cls = 1;
  constant type_name = "APPLICATION OCTET_STRING";
  int tagx;

  int get_tag() { return tagx; }

  void init(int tagid, string arg) {
    ::value = arg;
    tagx = tagid;
  }

}

class asn1_application_integer
{
  inherit Standards.ASN1.Types.asn1_integer;
  constant cls = 1;
  constant type_name = "APPLICATION INTEGER";
  int tagx;

  int get_tag() { return tagx; }

  void init(int tagid, int arg) {
    ::init(arg);
    tagx = tagid;
  }

}

object|mapping der_decode(object data, mapping types)
{
  int raw_tag = data->get_uint(1);
  int len;
  string contents;

  if ( (raw_tag & 0x1f) == 0x1f)
    error("ASN1.Decode: High tag numbers is not supported\n");

  len = data->get_uint(1);
  if (len & 0x80)
    len = data->get_uint(len & 0x7f);

  contents = data->get_fix_string(len);

  int tag = raw_tag & 0xdf; // Class and tag bits
//werror(sprintf("DEBx: tag=%O\n",tag));

  program p = types[tag];

  if (raw_tag & 0x20)
  {
    /* Constructed encoding */

    array elements = ({ });
    object struct = ADT.struct(contents);

    if (!p)
    {
      while (!struct->is_empty())
        elements += ({ der_decode(struct, types) });

      return Standards.ASN1.Decode.constructed(tag, contents, elements);
    }

    object res = p();
    // hop: For non-universal classes we provide tag number
    if(tag & 0xC0)
      res = p(tag & 0x1F, ({}));

    res->begin_decode_constructed(contents);

    int i;

    /* Ask object which types it expects for field i, decode it, and
     * record the decoded object */
    for(i = 0; !struct->is_empty(); i++)
    {
      res->decode_constructed_element
        (i, der_decode(struct,
                       res->element_types(i, types)));
    }
    return res->end_decode_constructed(i);
  }
  else
  {
    /* Primitive encoding */
    return p ? p()->decode_primitive(contents)
      : Standards.ASN1.Decode.primitive(tag, contents);
  }
}

static mapping snmp_type_proc =
                    ([ 1 : Standards.ASN1.Types.asn1_boolean,
                       2 : Standards.ASN1.Types.asn1_integer,
                       3 : Standards.ASN1.Types.asn1_bit_string,
                       4 : Standards.ASN1.Types.asn1_octet_string,
                       5 : Standards.ASN1.Types.asn1_null,
                       6 : Standards.ASN1.Types.asn1_identifier,
                       // 9 : asn1_real,
                       //10 : asn1_enumerated,
                       16 : Standards.ASN1.Types.asn1_sequence,
                       17 : Standards.ASN1.Types.asn1_set,
                       19 : Standards.ASN1.Types.asn1_printable_string,
                       20 : Standards.ASN1.Types.asn1_teletex_string,
                       23 : Standards.ASN1.Types.asn1_utc,

		 	    // from RFC-1065 :
		       64 : asn1_application_octet_string, // ipaddress
		       65 : asn1_application_integer,      // counter
		       66 : asn1_application_integer,      // gauge
		       67 : asn1_application_integer,      // timeticks
		       68 : asn1_application_octet_string,  // opaque

			// v2
		       70 : asn1_application_integer,	   // counter64

			// context PDU
                       128 : Protocols.LDAP.ldap_privates.asn1_context_sequence,
                       129 : Protocols.LDAP.ldap_privates.asn1_context_sequence,
                       130 : Protocols.LDAP.ldap_privates.asn1_context_sequence,
                       131 : Protocols.LDAP.ldap_privates.asn1_context_sequence
                    ]);

object|mapping snmp_der_decode(string data)
{
  return /*Standards.ASN1.Decode.*/der_decode(ADT.struct(data), snmp_type_proc);
}

// --- end of hack
#endif

inherit Stdio.UDP : snmp;

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
private function con_ok, con_fail;
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
void create(int|void rem_port, string|void rem_addr, int|void loc_port, string|void loc_addr) {

  int lport = loc_port;

  local_host = (!loc_addr || !sizeof(loc_addr)) ? SNMP_DEFAULT_LOCHOST : loc_addr;
  if(stringp(rem_addr) && sizeof(rem_addr)) remote_host = rem_addr;
  if(rem_port) remote_port = rem_port;

  if (!snmp::bind(lport, local_host)) {
    //# error ...
    DWRITE("protocol.create: can't bind to the socket.\n");
    ok = 0;
    if(con_fail)
      con_fail(this, @extra_args);
  }

  if(snmp_errno)
    THROW(({"Failed to bind to SNMP port.\n", backtrace()}));
  DWRITE("protocol.bind: success!\n");

  DWRITE(sprintf("protocol.create: local adress:port bound: [%s:%d].\n", local_host, lport));

}


//! return the whole SNMP message in raw format
mapping readmsg() {
  mapping rv;

  rv = read();
  return rv;
}

//! decode ASN1 data, if garbaged ignore it
mapping decode_asn1_msg(mapping rawd) {

  object xdec = snmp_der_decode(rawd->data);
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
		   "attribute":Array.map(xdec->elements[2]->elements[3]->elements, lambda(object duo) { return ([(array(string))(duo->elements[0]->id)*".":duo->elements[1]->value]); } )
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
  mapping msg;

  if(zero_type(msgpool[msgid]))
     return 0;

   msg = msgpool[msgid];
   msgpool -= ([msgid:msg]);
   return msg;
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

  msg = Standards.ASN1.Types.asn1_sequence(({
	   Standards.ASN1.Types.asn1_integer(snmp_version-1),
           Standards.ASN1.Types.asn1_octet_string(snmp_community),
	   pdu}));

  rawd = msg->get_der();

  DWRITE(sprintf("protocol.writemsg: %O\n", rawd));

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
    vararr += ({Standards.ASN1.Types.asn1_sequence(
	      ({Standards.ASN1.Types.asn1_identifier(
		  @Array.map(varname/".", lambda(string el){ return (int)el;})),
		Standards.ASN1.Types.asn1_integer(1)}) //doesn't sense but req
	      )});

  pdu = Protocols.LDAP.ldap_privates.asn1_context_sequence(0,
	       ({Standards.ASN1.Types.asn1_integer(id), // request-id
		 Standards.ASN1.Types.asn1_integer(0), // error-status
		 Standards.ASN1.Types.asn1_integer(0), // error-index
		 Standards.ASN1.Types.asn1_sequence(vararr)})
	       );

  // now we have PDU ...
  flg = writemsg(rem_addr||remote_host, rem_port || remote_port || SNMP_DEFAULT_PORT, pdu);

  return id;

}

object mk_asn1_val(string type, int|string val) {
// returns appropriate ASN.1 value

  object rv;

  switch(type) {
    case "oid":		// OID
	rv = Standards.ASN1.Types.asn1_identifier(
		@Array.map(val/".", lambda(string el){ return (int)el;}));
	break;

    case "int":		// INTEGER
	rv = Standards.ASN1.Types.asn1_integer(val);
	break;

    case "str":		// STRING
	rv = Standards.ASN1.Types.asn1_octet_string(val);
	break;

    case "ipaddr":	// ipAddress
	rv = asn1_application_octet_string(0,val[..3]);
	break;

    case "count":	// COUNTER
	rv = asn1_application_integer(1,val);
	break;

    case "gauge":	// GAUGE
	rv = asn1_application_integer(2,val);
	break;

    case "tick":	// TICK
	rv = asn1_application_integer(3,val);
	break;

    case "opaque":	// OPAQUE
	rv = asn1_application_octet_string(4,val);
	break;

    case "count64":	// COUNTER64 - v2 object
	rv = asn1_application_integer(6,val);
	break;

    default:		// bad type!
        error("Unknown SNMP data type " + type + ".");
	return 0;
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
int get_response(mapping varlist, mapping origdata, int|void errcode, int|void erridx) {
  //: GetResponse-PDU low call
  object pdu;
  int id = indices(origdata)[0], flg;
  array vararr = ({});

  foreach(indices(varlist), string varname)
    if(arrayp(varlist[varname]) || sizeof(varlist[varname]) > 1) {
      vararr += ({Standards.ASN1.Types.asn1_sequence(
		 ({Standards.ASN1.Types.asn1_identifier(
		    @Array.map(varname/".", lambda(string el){ return (int)el;})),
		   mk_asn1_val(varlist[varname][0], varlist[varname][1])})
	      )});
    }

  pdu = Protocols.LDAP.ldap_privates.asn1_context_sequence(2,
	       ({Standards.ASN1.Types.asn1_integer(id), // request-id
		 Standards.ASN1.Types.asn1_integer(errcode), // error-status
		 Standards.ASN1.Types.asn1_integer(erridx), // error-index
		 Standards.ASN1.Types.asn1_sequence(vararr)})
	       );

  // now we have PDU ...
  flg = writemsg(origdata[id]->ip||remote_host, origdata[id]->port || remote_port || SNMP_DEFAULT_PORT, pdu);

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
    vararr += ({Standards.ASN1.Types.asn1_sequence(
	      ({Standards.ASN1.Types.asn1_identifier(
		 @Array.map(varname/".", lambda(string el){ return (int)el; })),
		Standards.ASN1.Types.asn1_integer(1)}) //doesn't sense but req
	      )});

  pdu = Protocols.LDAP.ldap_privates.asn1_context_sequence(1,
	       ({Standards.ASN1.Types.asn1_integer(id), // request-id
		 Standards.ASN1.Types.asn1_integer(0), // error-status
		 Standards.ASN1.Types.asn1_integer(0), // error-index
		 Standards.ASN1.Types.asn1_sequence(vararr)})
	       );

  // now we have PDU ...
  flg = writemsg(rem_addr||remote_host, rem_port || remote_port || SNMP_DEFAULT_PORT, pdu);

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
    vararr += ({Standards.ASN1.Types.asn1_sequence(
	        ({Standards.ASN1.Types.asn1_identifier(
		    @Array.map(varname/".", lambda(string el){ return (int)el; })),
		  mk_asn1_val(varlist[varname][0], varlist[varname][1])})
	      )});

  pdu = Protocols.LDAP.ldap_privates.asn1_context_sequence(3,
	   ({Standards.ASN1.Types.asn1_integer(id), // request-id
		 Standards.ASN1.Types.asn1_integer(0), // error-status
		 Standards.ASN1.Types.asn1_integer(0), // error-index
		 Standards.ASN1.Types.asn1_sequence(vararr)})
       );

  // now we have PDU ...
  flg = writemsg(rem_addr||remote_host, rem_port || remote_port || SNMP_DEFAULT_PORT, pdu);

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

//  DWRITE(sprintf("protocols.trap: varlist: %O, oid: %O, type: %O, spectype: %O,"
//				  " ticks: %O, locip: %O, remaddr: %O, remport: %O\n",
//		 varlist, oid, type, spectype, ticks, locip, remaddr, remport));
  locip = locip || "0.0.0.0";
  if (has_value(locip,":")) // FIXME: Can't handle IPv6
    locip = "0.0.0.0";
  if (sizeof(locip/".") != 4)
    locip = "0.0.0.0"; //FIXME: what for hell I want to do with such ugly value?
  //sscanf(locip, "%d.%d.%d.%d", @lip);
  sscanf(locip, "%d.%d.%d.%d", lip[0], lip[1], lip[2], lip[3]);

  foreach(indices(varlist), string varname)
    vararr += ({Standards.ASN1.Types.asn1_sequence(
	        ({Standards.ASN1.Types.asn1_identifier(
		    @Array.map(varname/".", lambda(string el){ return (int)el; })),
		  mk_asn1_val(varlist[varname][0], varlist[varname][1])})
	      )});
  pdu = Protocols.LDAP.ldap_privates.asn1_context_sequence(4,
	  ({
		mk_asn1_val("oid", oid),			// enterprise OID
		mk_asn1_val("ipaddr", lip),			// ip address (UGLY!)
		mk_asn1_val("int", type),			// type (0 = coldstart, ...)
		mk_asn1_val("int", spectype),			// enterprise type
		mk_asn1_val("tick", ticks),			// uptime
		Standards.ASN1.Types.asn1_sequence(vararr)	// optional vars
	    })
		   
       );

  // now we have PDU ...
  flg = writemsg(remaddr||remote_host, remport || remote_port || SNMP_DEFAULT_TRAPPORT, pdu);

  return id;
}
