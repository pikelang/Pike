// SNMP protocol implementation for Pike.
//
// $Id$
//
// Honza Petrous, hop@roxen.com
//
//




/* 
 * possible error codes we can return
 */

#define SNMP_ERR_NOERROR                 0
#define SNMP_ERR_NOERROR_STR		 "No error"
#define SNMP_ERR_TOOBIG			 1
#define SNMP_ERR_TOOBIG_STR		 "Too big"
#define SNMP_ERR_NOSUCHNAME		 2
#define SNMP_ERR_NOSUCHNAME_STR		 "No such name"
#define SNMP_ERR_BADVALUE		 3
#define SNMP_ERR_BADVALUE_STR		 "Bad value"
#define SNMP_ERR_READONLY		 4
#define SNMP_ERR_READONLY_STR		 "Read only"
#define SNMP_ERR_GENERROR		 5
#define SNMP_ERR_GENERROR_STR		 "General error"
// end of V1 error list
#define SNMP_ERR_NOACCESS		 6
#define SNMP_ERR_WRONGTYPE		 7
#define SNMP_ERR_WRONGLENGTH		 8
#define SNMP_ERR_WRONGENCODING		 9
#define SNMP_ERR_WRONGVALUE		10
#define SNMP_ERR_NOCREATION		11
#define SNMP_ERR_INCONSISTENCEVALUE	12
#define SNMP_ERR_RESOURCEUNAVAILABLE	13
#define SNMP_ERR_COMMITFAILED		14
#define SNMP_ERR_UNDOFAILED		15
#define SNMP_ERR_AUTHORIZATIONERROR	16
#define SNMP_ERR_NOTWRITEBLE		17
#define SNMP_ERR_INCONSISTENTNAME	18

/*static mapping(int:string) */ constant snmp_errlist = ([
	SNMP_ERR_NOERROR		: SNMP_ERR_NOERROR_STR,
	SNMP_ERR_TOOBIG			: SNMP_ERR_TOOBIG_STR,
	SNMP_ERR_NOSUCHNAME		: SNMP_ERR_NOSUCHNAME_STR,
	SNMP_ERR_BADVALUE		: SNMP_ERR_BADVALUE_STR,
	SNMP_ERR_READONLY		: SNMP_ERR_READONLY_STR,
	SNMP_ERR_GENERROR		: SNMP_ERR_GENERROR_STR
]);
