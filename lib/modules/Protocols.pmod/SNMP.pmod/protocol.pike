//: SNMP protocol implementation for Pike
//:
//: Honza Petrous, 2000-10-07 (on the 'coding party' after user conference :)
//:
//: RFC:
//:  implemented (yet):
//:      1157 - v1 (only `TRAP' PDU is missing)
//:  planned:
//:      1901 - v2/community
//:      2570 - v3 description
//:

//: For debuging use -DDEBUG_PIKE_PROTOCOL_SNMP=1

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
		       68 : asn1_application_octet_string  // opaque
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
int port = SNMP_DEFAULT_PORT;
string local_host = SNMP_DEFAULT_HOST;
string remote_host;
int request_id = 1;
int next_id = 1;
int snmp_version = SNMP_DEFAULT_VERSION;
string snmp_community = SNMP_DEFAULT_COMMUNITY;
int snmp_errno = SNMP_SUCCESS;
int ok;

//: 
//: msg pool
//:
mapping msgpool = ([]);

//:
//: callback support
//:
function con_ok, con_fail;
array extra_args;

//:
//: create
//:
//: *_port: local and remote UDP port
//: *_addr: local and remote address
//:
void create(int|void rem_port, string|void rem_addr, int|void loc_port, string|void loc_addr) {

  int lport = loc_port;

  local_host = (!loc_addr || !sizeof(loc_addr)) ? SNMP_DEFAULT_LOCHOST : loc_addr;
  if(stringp(rem_addr) && sizeof(rem_addr)) remote_host = rem_addr;

  if (!snmp::bind(lport, local_host)) {
    //# error ...
    DWRITE("protocol.create: can't bind to the socket.\n");
    ok = 0;
    if(con_fail)
      con_fail(this_object(), @extra_args);
  }

  if(snmp_errno)
    THROW(({"Failed to bind to SNMP port.\n", backtrace()}));
  DWRITE("protocol.bind: success!\n");

  DWRITE(sprintf("protocol.create: local adress:port binded: [%s:%d].\n", local_host, lport));

}


mapping readmsg() {
  //: returns the whole SNMP message in raw format
  mapping rv;

#if 1
  rv = read();
#else
  if(catch { rv = read(); }) {
        //seterr or _READ_ERROR ?
        THROW(({"SNMP.protocol: read error.\n",backtrace()}));
  }
#endif

  return(rv);
}

void to_pool(mapping rawd) {
  //: decode ASN1 data and put it into pool, if garbaged ignore it

  object xdec = snmp_der_decode(rawd->data);
  string msgid = (string)xdec->elements[2]->elements[0]->value;
  int errno = xdec->elements[2]->elements[1]->value;
  mapping msg = ([ "ip":rawd->ip,
		   "port":rawd->port,
		   "error-status":errno,
		   "error-string":snmp_errlist[errno],
		   "error-index":xdec->elements[2]->elements[2]->value,
		   "attribute":Array.map(xdec->elements[2]->elements[3]->elements, lambda(object duo) { return(([(array(string))(duo->elements[0]->id)*".":duo->elements[1]->value])); } )
  ]);

  // to the queue
  msgpool += ([msgid:msg]);

}

mapping|int from_pool(string msgid) {
  //: get data from poll
  mapping msg;

  if(zero_type(msgpool[msgid]))
     return 0;

   msg = msgpool[msgid];
   msgpool -= ([msgid:msg]);
   return(msg);
}


mapping readmsg_from_pool(int msgid) {
  //: read SNMP response PDU from PDU pool

  mapping rv = from_pool((string)msgid);
  if(rv)
    return(rv);

#define HACK 1
#ifdef HACK
  mapping tmpm = readmsg();
  to_pool(tmpm);
#endif

  return(from_pool((string)msgid));
}



int writemsg(string rem_addr, int rem_port, object pdu) {
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
  return(msize = sizeof(rawd) ? SNMP_SUCCESS : SNMP_SEND_ERROR);
}


int get_req_id() {
  //: returns unicate id

  //LOCK
  int rv = request_id++;
  //UNLOCK
  return(rv);
}

//:
//: get_response
//:
array(mapping)|int get_response(int msgid) {
  //: GetResponse-PDU low call
  mapping rawpdu = readmsg_from_pool(msgid);


  return(({rawpdu}));
}

//:
//: get_request
//:
int get_request(array(string) varlist, string|void rem_addr,
                         int|void rem_port) {
  //: GetRequest-PDU low call
  object pdu;
  int id = get_req_id(), flg;
  array vararr = ({});

  foreach(varlist, string varname)
    vararr += ({Standards.ASN1.Types.asn1_sequence(
	      ({Standards.ASN1.Types.asn1_identifier(@Array.map(varname/".", lambda(string el){ return((int)el);})),
		Standards.ASN1.Types.asn1_integer(1)}) //doesn't sense but req
	      )});

  pdu = Protocols.LDAP.ldap_privates.asn1_context_sequence(0,
							       ({Standards.ASN1.Types.asn1_integer(id), // request-id
								 Standards.ASN1.Types.asn1_integer(0), // error-status
								 Standards.ASN1.Types.asn1_integer(0), // error-index
								 Standards.ASN1.Types.asn1_sequence(vararr)})
							       );

  // now we have PDU ...
  flg = writemsg(rem_addr||remote_host, rem_port || port, pdu);

  return id;

}


//:
//: get_nextrequest
//:
int get_nextrequest(array(string) varlist, string|void rem_addr,
                         int|void rem_port) {
  //: GetNextRequest-PDU low call
  object pdu;
  int id = get_req_id(), flg;
  array vararr = ({});

  foreach(varlist, string varname)
    vararr += ({Standards.ASN1.Types.asn1_sequence(
	      ({Standards.ASN1.Types.asn1_identifier(@Array.map(varname/".", lambda(string el){ return((int)el);})),
		Standards.ASN1.Types.asn1_integer(1)}) //doesn't sense but req
	      )});

  pdu = Protocols.LDAP.ldap_privates.asn1_context_sequence(1,
							       ({Standards.ASN1.Types.asn1_integer(id), // request-id
								 Standards.ASN1.Types.asn1_integer(0), // error-status
								 Standards.ASN1.Types.asn1_integer(0), // error-index
								 Standards.ASN1.Types.asn1_sequence(vararr)})
							       );

  // now we have PDU ...
  flg = writemsg(rem_addr||remote_host, rem_port || port, pdu);

  return id;

}

object mktype(array(mixed) varval) {

  if(sizeof(varval) != 2) // error!
    return 0;

  switch(varval[0]) {
	case "i": // integer
	  return Standards.ASN1.Types.asn1_integer(varval[1]);

	case "s": // string
	  return Standards.ASN1.Types.asn1_octet_string(varval[1]);
  }

  return 0;
}

//:
//: set_request
//:
int set_request(mapping varlist, string|void rem_addr,
                         int|void rem_port) {
  //: SetRequest-PDU low call
  object pdu;
  int id = get_req_id(), flg;
  array vararr = ({});

  foreach(indices(varlist), string varname)
    vararr += ({Standards.ASN1.Types.asn1_sequence(
	      ({Standards.ASN1.Types.asn1_identifier(@Array.map(varname/".", lambda(string el){ return((int)el);})),
		mktype(varlist[varname])})
	      )});

  pdu = Protocols.LDAP.ldap_privates.asn1_context_sequence(3,
							       ({Standards.ASN1.Types.asn1_integer(id), // request-id
								 Standards.ASN1.Types.asn1_integer(0), // error-status
								 Standards.ASN1.Types.asn1_integer(0), // error-index
								 Standards.ASN1.Types.asn1_sequence(vararr)})
							       );

  // now we have PDU ...
  flg = writemsg(rem_addr||remote_host, rem_port || port, pdu);

  return id;
}
