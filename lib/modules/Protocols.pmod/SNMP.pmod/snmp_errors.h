// SNMP protocol implementation for Pike.
//
// $Id: snmp_errors.h,v 1.1 2000/12/05 00:55:52 hop Exp $
//
// Honza Petrous, hop@roxen.com
//
//




/* 
 * possible error codes we can return
 */

#define SNMP_NOERROR                    0x00    /* 0 */
#define SNMP_NOERROR_STR		"No error"
#define SNMP_TOOBIG			0x01    /* 1 */
#define SNMP_TOOBIG_STR			"Too big"
#define SNMP_NOSUCHNAME			0x02    /* 2 */
#define SNMP_NOSUCHNAME_STR		"No such name"
#define SNMP_BADVALUE			0x03    /* 3 */
#define SNMP_BADVALUE_STR		"Bad value"
#define SNMP_READONLY			0x04    /* 4 */
#define SNMP_READONLY_STR		"Read only"
#define SNMP_GENERROR			0x05    /* 5 */
#define SNMP_GENERROR_STR		"General error"
// end of V1 error list

/*static mapping(int:string) */ constant snmp_errlist = ([
	SNMP_NOERROR			: SNMP_NOERROR_STR,
	SNMP_TOOBIG			: SNMP_TOOBIG_STR,
	SNMP_NOSUCHNAME			: SNMP_NOSUCHNAME_STR,
	SNMP_BADVALUE			: SNMP_BADVALUE_STR,
	SNMP_READONLY			: SNMP_READONLY_STR,
	SNMP_GENERROR			: SNMP_GENERROR_STR
]);
