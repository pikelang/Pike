// LDAP client protocol implementation for -*- Pike -*-.
//
// $Id: ldap_errors.h,v 1.3 2005/03/11 16:49:57 mast Exp $
//
// Honza Petrous, hop@unibase.cz
//
// ----------------------------------------------------------------------
//
// ToDo List:
//
//	- v2 operations: 
//		modify
//
// History:
//
//	v1.0  1998-06-21 Core functions (open, bind, unbind, delete, add,
//			 compare, search), only V2 operations,
//

import Protocols.LDAP;

/*static* mapping(int:string)*/ constant ldap_errlist = ([
  LDAP_SUCCESS:			"Success",
  LDAP_OPERATIONS_ERROR:	"Operations error",
  LDAP_PROTOCOL_ERROR:		"Protocol error",
  LDAP_TIMELIMIT_EXCEEDED:	"Timelimit exceeded",
  LDAP_SIZELIMIT_EXCEEDED:	"Sizelimit exceeded",
  LDAP_COMPARE_FALSE:		"Compare false",
  LDAP_COMPARE_TRUE:		"Compare true",
  LDAP_AUTH_METHOD_NOT_SUPPORTED: "Auth method not supported",
  LDAP_STRONG_AUTH_REQUIRED:	"Strong auth required",
  LDAP_PARTIAL_RESULTS:		"Partial results",
  LDAP_REFERRAL:		"Referral",
  LDAP_ADMINLIMIT_EXCEEDED:	"Adminlimit exceeded",
  LDAP_UNAVAILABLE_CRITICAL_EXTENSION: "Unavailable critical extension",
  LDAP_CONFIDENTIALITY_REQUIRED: "Confidentiality required",
  LDAP_SASL_BIND_IN_PROGRESS:	"SASL bind in progress",
  
  LDAP_NO_SUCH_ATTRIBUTE:	"No such attribute",
  LDAP_UNDEFINED_TYPE:		"Undefined type",
  LDAP_INAPPROPRIATE_MATCHING:	"Inappropriate matching",
  LDAP_CONSTRAINT_VIOLATION:	"Constraint violation",
  LDAP_TYPE_OR_VALUE_EXISTS:	"Type or value exists",
  LDAP_INVALID_SYNTAX:		"Invalid syntax",
  
  LDAP_NO_SUCH_OBJECT:		"No such object",
  LDAP_ALIAS_PROBLEM:		"Alias problem",
  LDAP_INVALID_DN_SYNTAX:	"Invalid DN syntax",
  LDAP_IS_LEAF:			"Is leaf",
  LDAP_ALIAS_DEREF_PROBLEM:	"Alias deref problem",
  
  LDAP_INAPPROPRIATE_AUTH:	"Inappropriate auth",
  LDAP_INVALID_CREDENTIALS:	"Invalid credentials",
  LDAP_INSUFFICIENT_ACCESS:	"Insufficient access",
  LDAP_BUSY:			"Busy",
  LDAP_UNAVAILABLE:		"Unavailable",
  LDAP_UNWILLING_TO_PERFORM:	"Unwilling to perform",
  LDAP_LOOP_DETECT:		"Loop detect",
  
  LDAP_SORT_CONTROL_MISSING:	"Sort control missing",
  
  LDAP_NAMING_VIOLATION:	"Naming violation",
  LDAP_OBJECT_CLASS_VIOLATION:	"Object class violation",
  LDAP_NOT_ALLOWED_ON_NONLEAF:	"Not allowed on nonleaf",
  LDAP_NOT_ALLOWED_ON_RDN:	"Not allowed on RDN",
  LDAP_ALREADY_EXISTS:		"Already exists",
  LDAP_NO_OBJECT_CLASS_MODS:	"No object class mods",
  LDAP_RESULTS_TOO_LARGE:	"Results too large",
  LDAP_AFFECTS_MULTIPLE_DSAS:	"Affects multiple DSAS",
  
  LDAP_OTHER:			"Other str",
  LDAP_SERVER_DOWN:		"Server is down",
  LDAP_LOCAL_ERROR:		"Internal/local error",
  LDAP_ENCODING_ERROR:		"Encoding error",
  LDAP_DECODING_ERROR:		"Decoding error",
  LDAP_TIMEOUT:			"Timeout",
  LDAP_AUTH_UNKNOWN:		"Auth unknown",
  LDAP_FILTER_ERROR:		"Filter error",
  LDAP_USER_CANCELLED:		"User cancelled",
  LDAP_PARAM_ERROR:		"Param error",
  LDAP_NO_MEMORY:		"No memory",
  LDAP_CONNECT_ERROR:		"Connect error",
  LDAP_NOT_SUPPORTED:		"Not supported",
  LDAP_CONTROL_NOT_FOUND:	"Control not found",
  LDAP_NO_RESULTS_RETURNED:	"No results returned",
  LDAP_MORE_RESULTS_TO_RETURN:	"More results to return",
  LDAP_CLIENT_LOOP:		"Client loop",
  LDAP_REFERRAL_LIMIT_EXCEEDED:	"Referral limit exceeded",
]);
