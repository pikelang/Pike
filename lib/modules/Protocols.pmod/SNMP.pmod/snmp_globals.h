//
// snmp_globals.h
//
// $Id$
//

#define SNMP_DEFAULT_PORT       161
#define SNMP_DEFAULT_TRAPPORT   162
#define SNMP_DEFAULT_HOST       "127.0.0.1"
#define SNMP_DEFAULT_LOCHOST    "0.0.0.0"
#define SNMP_DEFAULT_COMMUNITY  "public"
#define SNMP_DEFAULT_VERSION    1

#define SNMP_MINMAXOCTETS_V1	484
// from RFC1157

#define SNMP_SUCCESS		0
#define SNMP_SEND_ERROR		1

#define SNMP_REQUEST_GET	0
#define SNMP_REQUEST_GETNEXT	1
#define SNMP_REQUEST_GET_RESPONSE	2
#define SNMP_REQUEST_SET	3
#define SNMP_REQUEST_TRAP	4

// debug
#ifdef DEBUG_PIKE_PROTOCOL_SNMP
#define DWRITE(X)	werror("Protocols.SNMP: "+X)
#define DWRITE_HI(X)	werror("Protocols.SNMP: "+X)
#define THROW(X)	throw(X)
#else
#define DWRITE(X)
#define DWRITE_HI(X)
#define THROW(X)
#endif



