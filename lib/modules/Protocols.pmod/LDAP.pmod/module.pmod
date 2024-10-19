#pike __REAL_VERSION__


#include "ldap_globals.h"

import ".";

//! LDAP result codes.
//!
//! @seealso
//!   @[Protocols.LDAP.client.error_number],
//!   @[Protocols.LDAP.client.result.error_number]
constant LDAP_SUCCESS                    = 0x00;    /* 0 */
constant LDAP_OPERATIONS_ERROR           = 0x01;    /* 1 */
constant LDAP_PROTOCOL_ERROR             = 0x02;    /* 2 */
constant LDAP_TIMELIMIT_EXCEEDED         = 0x03;    /* 3 */
constant LDAP_SIZELIMIT_EXCEEDED         = 0x04;    /* 4 */
constant LDAP_COMPARE_FALSE              = 0x05;    /* 5 */
constant LDAP_COMPARE_TRUE               = 0x06;    /* 6 */
constant LDAP_AUTH_METHOD_NOT_SUPPORTED  = 0x07;    /* 7 */
constant LDAP_STRONG_AUTH_NOT_SUPPORTED  = LDAP_AUTH_METHOD_NOT_SUPPORTED;
constant LDAP_STRONG_AUTH_REQUIRED       = 0x08;    /* 8 */
constant LDAP_PARTIAL_RESULTS            = 0x09;    /* 9 (UMich LDAPv2 extn) */
constant LDAP_REFERRAL                   = 0x0a;    /* 10 - LDAPv3 */
constant LDAP_ADMINLIMIT_EXCEEDED        = 0x0b;    /* 11 - LDAPv3 */
constant LDAP_UNAVAILABLE_CRITICAL_EXTENSION = 0x0c; /* 12 - LDAPv3 */
constant LDAP_CONFIDENTIALITY_REQUIRED   = 0x0d;    /* 13 */
constant LDAP_SASL_BIND_IN_PROGRESS      = 0x0e;    /* 14 - LDAPv3 */
constant LDAP_NO_SUCH_ATTRIBUTE          = 0x10;    /* 16 */
constant LDAP_UNDEFINED_TYPE             = 0x11;    /* 17 */
constant LDAP_INAPPROPRIATE_MATCHING     = 0x12;    /* 18 */
constant LDAP_CONSTRAINT_VIOLATION       = 0x13;    /* 19 */
constant LDAP_TYPE_OR_VALUE_EXISTS       = 0x14;    /* 20 */
constant LDAP_INVALID_SYNTAX             = 0x15;    /* 21 */
constant LDAP_NO_SUCH_OBJECT             = 0x20;    /* 32 */
constant LDAP_ALIAS_PROBLEM              = 0x21;    /* 33 */
constant LDAP_INVALID_DN_SYNTAX          = 0x22;    /* 34 */
constant LDAP_IS_LEAF                    = 0x23;    /* 35 (not used in LDAPv3) */
constant LDAP_ALIAS_DEREF_PROBLEM        = 0x24;    /* 36 */
constant LDAP_INAPPROPRIATE_AUTH         = 0x30;    /* 48 */
constant LDAP_INVALID_CREDENTIALS        = 0x31;    /* 49 */
constant LDAP_INSUFFICIENT_ACCESS        = 0x32;    /* 50 */
constant LDAP_BUSY                       = 0x33;    /* 51 */
constant LDAP_UNAVAILABLE                = 0x34;    /* 52 */
constant LDAP_UNWILLING_TO_PERFORM       = 0x35;    /* 53 */
constant LDAP_LOOP_DETECT                = 0x36;    /* 54 */
constant LDAP_SORT_CONTROL_MISSING       = 0x3C;    /* 60 */
constant LDAP_NAMING_VIOLATION           = 0x40;    /* 64 */
constant LDAP_OBJECT_CLASS_VIOLATION     = 0x41;    /* 65 */
constant LDAP_NOT_ALLOWED_ON_NONLEAF     = 0x42;    /* 66 */
constant LDAP_NOT_ALLOWED_ON_RDN         = 0x43;    /* 67 */
constant LDAP_ALREADY_EXISTS             = 0x44;    /* 68 */
constant LDAP_NO_OBJECT_CLASS_MODS       = 0x45;    /* 69 */
constant LDAP_RESULTS_TOO_LARGE          = 0x46;    /* 70 - CLDAP */
constant LDAP_AFFECTS_MULTIPLE_DSAS      = 0x47;    /* 71 */
constant LDAP_OTHER                      = 0x50;    /* 80 */
// 81-90 are reserved for (client) APIs (c.f.
// http://www.iana.org/assignments/ldap-parameters). These are not
// standardized but are common among many client APIs.
constant LDAP_SERVER_DOWN                = 0x51;    /* 81 */
constant LDAP_LOCAL_ERROR                = 0x52;    /* 82 */
constant LDAP_ENCODING_ERROR             = 0x53;    /* 83 */
constant LDAP_DECODING_ERROR             = 0x54;    /* 84 */
constant LDAP_TIMEOUT                    = 0x55;    /* 85 */
constant LDAP_AUTH_UNKNOWN               = 0x56;    /* 86 */
constant LDAP_FILTER_ERROR               = 0x57;    /* 87 */
constant LDAP_USER_CANCELLED             = 0x58;    /* 88 */
constant LDAP_PARAM_ERROR                = 0x59;    /* 89 */
constant LDAP_NO_MEMORY                  = 0x5a;    /* 90 */
// 91-112 are reserved for "LDAP APIs" - see above.
constant LDAP_CONNECT_ERROR              = 0x5b;    /* 91 */
constant LDAP_NOT_SUPPORTED              = 0x5c;    /* 92 */
constant LDAP_CONTROL_NOT_FOUND          = 0x5d;    /* 93 */
constant LDAP_NO_RESULTS_RETURNED        = 0x5e;    /* 94 */
constant LDAP_MORE_RESULTS_TO_RETURN     = 0x5f;    /* 95 */
constant LDAP_CLIENT_LOOP                = 0x60;    /* 96 */
constant LDAP_REFERRAL_LIMIT_EXCEEDED    = 0x61;    /* 97 */
constant LCUP_RESOURCES_EXHAUSTED        = 0x71;    /* 113 */
constant LCUP_SECURITY_VIOLATION         = 0x72;    /* 114 */
constant LCUP_INVALID_DATA               = 0x73;    /* 115 */
constant LCUP_UNSUPPORTED_SCHEME         = 0x74;    /* 116 */
constant LCUP_RELOAD_REQUIRED            = 0x75;    /* 117 */
constant LDAP_CANCELED                   = 0x76;    /* 118 */
constant LDAP_NO_SUCH_OPERATION          = 0x77;    /* 119 */
constant LDAP_TOO_LATE                   = 0x78;    /* 120 */
constant LDAP_CANNOT_CANCEL              = 0x79;    /* 121 */
constant LDAP_ASSERTION_FAILED           = 0x7a;    /* 122 */
constant LDAP_AUTHORIZATION_DENIED       = 0x7b;    /* 123 */

//! Mapping from @expr{LDAP_*@} result codes to descriptive strings.
constant ldap_error_strings = ([
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

constant SEARCH_LOWER_ATTRS = 1;
constant SEARCH_MULTIVAL_ARRAYS_ONLY = 2;
constant SEARCH_RETURN_DECODE_ERRORS = 4;
//! Bitfield flags given to @[Protocols.LDAP.client.search]:
//!
//! @dl
//! @item SEARCH_LOWER_ATTRS
//!   Lowercase all attribute values. This makes it easier to match
//!   specific attributes in the mappings returned by
//!   @[Protocols.LDAP.client.result.fetch] since LDAP attribute names
//!   are case insensitive.
//!
//! @item SEARCH_MULTIVAL_ARRAYS_ONLY
//!   Only use arrays for attribute values where the attribute syntax
//!   specify multiple values. I.e. the values for single valued
//!   attributes are returned as strings instead of arrays containing
//!   one string element.
//!
//!   If no value is returned for a single valued attribute, e.g. when
//!   @expr{attrsonly@} is set in the search call, then a zero will be
//!   used as value.
//!
//!   The special @expr{"dn"@} value is also returned as a string when
//!   this flag is set.
//!
//!   Note that it's the attribute type descriptions that are used to
//!   decide this, not the number of values a particular attribute
//!   happens to have in the search result.
//!
//! @item SEARCH_RETURN_DECODE_ERRORS
//!   Don't throw attribute value decode errors, instead return them
//!   in the result from @[Protocols.LDAP.client.result.fetch] in
//!   place of the value. I.e. anywhere an attribute value string
//!   occurs, you might instead have a @[Charset.DecodeError] object.
//! @enddl

constant SCOPE_BASE = 0;
constant SCOPE_ONE = 1;
constant SCOPE_SUB = 2;
//! Constants for the search scope used with e.g.
//! @[Protocols.LDAP.client.set_scope].
//!
//! @dl
//! @item SCOPE_BASE
//!   Return the object specified by the DN.
//! @item SCOPE_ONE
//!   Return the immediate subobjects of the object specified by the DN.
//! @item SCOPE_SUB
//!   Return the object specified by the DN and all objects below it
//!   (on any level).
//! @enddl

constant MODIFY_ADD = 0;
constant MODIFY_DELETE = 1;
constant MODIFY_REPLACE = 2;
//! Constants used in the @expr{attropval@} argument to
//! @[Protocols.LDAP.client.modify].

string ldap_encode_string (string str)
//! Quote characters in the given string as necessary for use as a
//! string literal in filters and various composite LDAP attributes.
//!
//! The quoting is compliant with @rfc{2252:4.3@} and
//! @rfc{2254:4@}. All characters that can be special in those RFCs
//! are quoted using the @expr{\xx@} syntax, but the set might be
//! extended.
//!
//! @seealso
//!   @[ldap_decode_string], @[Protocols.LDAP.client.search]
{
  return replace (str,
    ({"\0",   "#",    "$",    "'",    "(",    ")",    "*",    "\\"}),
    ({"\\00", "\\23", "\\24", "\\27", "\\28", "\\29", "\\2a", "\\5c"}));
}

string ldap_decode_string (string str)
//! Decodes all @expr{\xx@} escapes in @[str].
//!
//! @seealso
//!   @[ldap_encode_string]
{
  string rest = str, res = "";
  while (1) {
    sscanf (rest, "%[^\\]%s", string val, rest);
    res += val;
    if (rest == "") break;
    // rest[0] == '\\' now.
    if (sscanf (rest, "\\%1x%1x%s", int high, int low, rest) == 3)
      // Use two %1x to force reading of exactly two hex digits;
      // something like %2.2x is currently not supported.
      res += sprintf ("%c", (high << 4) + low);
    else {
      // Treat invalid escapes as literal backslashes. At least Tivoli
      // and Lotus Domino are known to have an attribute description
      // with quoting errors in it, as noted by Dan Nelson.
      res += "\\";
      rest = rest[1..];
    }
  }
  return res;
}

string encode_dn_value (string str)
//! Encode the given string for use as an attribute value in a
//! distinguished name (on string form).
//!
//! The encoding is according to @rfc{2253:2.4@} with the exception
//! that characters above @expr{0x7F@} aren't UTF-8 encoded.  UTF-8
//! encoding can always be done afterwards on the complete DN, which
//! also is done internally by the @[Protocols.LDAP] functions when
//! LDAPv3 is used.
{
  str = replace (str,
		 ({",",   "+",   "\"",   "\\",   "<",   ">",   ";"}),
		 ({"\\,", "\\+", "\\\"", "\\\\", "\\<", "\\>", "\\;"}));
  if (has_suffix (str, " "))
    str = str[..<1] + "\\ ";
  if (has_prefix (str, " ") || has_prefix (str, "#"))
    str = "\\" + str;
  return str;
}

string canonicalize_dn (string dn, void|int strict)
//! Returns the given distinguished name on a canonical form, so it
//! reliably can be used in comparisons for equality. This means
//! removing surplus whitespace, lowercasing attributes, normalizing
//! quoting in string attribute values, lowercasing the hex digits in
//! binary attribute values, and sorting the RDN parts separated by
//! "+".
//!
//! The returned string follows @rfc{2253@}. The input string may use
//! legacy LDAPv2 syntax and is treated according to @rfc{2253:4@}.
//!
//! If @[strict] is set then errors will be thrown if the given DN is
//! syntactically invalid. Otherwise the invalid parts remain
//! untouched in the result.
//!
//! @note
//! The result is not entirely canonical since no conversion is done
//! from or to hexadecimal BER encodings of the attribute values. It's
//! assumed that the input already has the suitable value encoding
//! depending on the attribute type.
//!
//! @note
//! No UTF-8 encoding or decoding is done. The function can be used on
//! both encoded and decoded input strings, and the result will be
//! likewise encoded or decoded.
{
  string rest = dn, tmp_rest = rest, res = "";
  array(string) last_rdn = ({});

parse_loop:
  while (1) {
    // Parse the attribute.

    string attr;
    if (sscanf (tmp_rest, "%*[ ]%[-A-Za-z0-9.]%*[ ]=%*[ ]%s", attr, tmp_rest) != 5)
      break parse_loop;
    if (strict && attr == "")
      break parse_loop;
    attr = lower_case (attr);
    if (sscanf (attr, "oid.%*[0-9.]%*c") == 1)
      attr = attr[sizeof ("oid.")..];

    // Parse the value.

    string val;

    if (has_prefix (tmp_rest, "#")) {
      sscanf (tmp_rest, "#%[0-9A-Fa-f]%s", val, tmp_rest);
      if (strict && val == "")
	break parse_loop;
      // Slightly lenient: We don't check for an even number of hex chars.
      last_rdn += ({attr + "=#" + lower_case (val)});
      rest = tmp_rest;
    }

    else {
      string val_fmt;
      int quoted = has_prefix (tmp_rest, "\"");

      if (quoted) {
	val_fmt = "%[^\\\" ]%[ ]%s";
	tmp_rest = tmp_rest[1..];
      }
      else
	val_fmt = "%[^,=+<>#;\\\" ]%[ ]%s";

      val = "";
      string ws = "";
      while (1) {
	sscanf (tmp_rest, val_fmt, string piece, ws, tmp_rest);
	val += piece;
	if (ws != "")
	  while (1) {
	    sscanf (tmp_rest, val_fmt, piece, string new_ws, tmp_rest);
	    if (piece == "") break;
	    val += ws + piece;
	    ws = new_ws;
	  }
	if (!has_prefix (tmp_rest, "\\")) break;
	val += ws;
	if (sscanf (tmp_rest, "\\%1x%1x%s", int high, int low, tmp_rest) == 3)
	  // Use two %1x to force reading of exactly two hex digits;
	  // something like %2.2x is currently not supported.
	  val += sprintf ("%c", (high << 4) + low);
	else {
	  string quoted;
	  if (sscanf (tmp_rest, "\\%1s%s", quoted, tmp_rest) != 2)
	    break parse_loop;
	  if (strict && !(<",", "=", "+", "<", ">", "#", ";", "\\", "\"", " ">)[quoted])
	    break parse_loop;
	  val += quoted;
	}
      }

      if (quoted) {
	if (has_prefix (tmp_rest, "\"")) {
	  val += ws;
	  sscanf (tmp_rest, "\"%*[ ]%s", tmp_rest);
	}
	else
	  break parse_loop;
      }

      last_rdn += ({attr + "=" + encode_dn_value (val)});
      rest = tmp_rest;
    }

    // Prepare for the next attribute/value pair.

    string sep = tmp_rest[..0];

    if (sep == "," || sep == ";") {
      // Finish this rdn and start another.
      if (sizeof (last_rdn)) {
	sort (last_rdn);
	if (res != "") res += "," + (last_rdn * "+");
	else res = last_rdn * "+";
	last_rdn = ({});
      }
    }

    else if (sep != "+")
      break;

    tmp_rest = tmp_rest[1..];
  }

  if (sizeof (last_rdn)) {
    sort (last_rdn);
    if (res) res += "," + (last_rdn * "+");
    else res = last_rdn * "+";
  }

  if (rest != "") {
    if (strict)
      ERROR ("Syntax error at or after position %d in DN %O.\n",
	     sizeof (dn) - sizeof (rest), dn);
    else
      res += rest;
  }

  //werror ("canonicalize \"%s\" -> \"%s\"\n", dn, res);
  return res;
}

// Constants to make more human readable names for some known
// LDAP related object identifiers.

// LDAP controls

constant LDAP_CONTROL_MANAGE_DSA_IT = "2.16.840.1.113730.3.4.2";
//! LDAP control: Manage DSA IT LDAPv3 control (@rfc{3296@}): Control
//! to indicate that the operation is intended to manage objects
//! within the DSA (server) Information Tree.

constant LDAP_CONTROL_VLVREQUEST = "2.16.840.1.113730.3.4.9";
//! LDAP control: LDAP Extensions for Scrolling View Browsing of
//! Search Results (internet draft): Control used to request virtual
//! list view support from the server.

constant LDAP_CONTROL_VLVRESPONSE = "2.16.840.1.113730.3.4.10";
//! LDAP control: LDAP Extensions for Scrolling View Browsing of
//! Search Results (internet draft): Control used to pass virtual list
//! view (VLV) data from the server to the client.

constant LDAP_PAGED_RESULT_OID_STRING = "1.2.840.113556.1.4.319";
//! LDAP control: Microsoft AD: Control to instruct the server to
//! return the results of a search request in smaller, more manageable
//! packets rather than in one large block.

constant LDAP_SERVER_ASQ_OID = "1.2.840.113556.1.4.1504";
//! LDAP control: Microsoft AD: Control to force the query to be based
//! on a specific DN-valued attribute.

constant LDAP_SERVER_CROSSDOM_MOVE_TARGET_OID = "1.2.840.113556.1.4.521";
//! LDAP control: Microsoft AD: Control used with an extended LDAP
//! rename function to move an LDAP object from one domain to another.

constant LDAP_SERVER_DIRSYNC_OID = "1.2.840.113556.1.4.841";
//! LDAP control: Microsoft AD: Control that enables an application to
//! search the directory for objects changed from a previous state.

constant LDAP_SERVER_DOMAIN_SCOPE_OID = "1.2.840.113556.1.4.1339";
//! LDAP control: Microsoft AD: Control used to instruct the LDAP
//! server not to generate any referrals when completing a request.

constant LDAP_SERVER_EXTENDED_DN_OID = "1.2.840.113556.1.4.529";
//! LDAP control: Microsoft AD: Control to request an extended form of
//! an Active Directory object distinguished name.

constant LDAP_SERVER_LAZY_COMMIT_OID = "1.2.840.113556.1.4.619";
//! LDAP control: Microsoft AD: Control used to instruct the server to
//! return the results of a DS modification command, such as add,
//! delete, or replace, after it has been completed in memory, but
//! before it has been committed to disk.

constant LDAP_SERVER_NOTIFICATION_OID = "1.2.840.113556.1.4.528";
//! LDAP control: Microsoft AD: Control used with an extended LDAP
//! asynchronous search function to register the client to be notified
//! when changes are made to an object in Active Directory.

constant LDAP_SERVER_PERMISSIVE_MODIFY_OID = "1.2.840.113556.1.4.1413";
//! LDAP control: Microsoft AD: An LDAP modify request will normally
//! fail if it attempts to add an attribute that already exists, or if
//! it attempts to delete an attribute that does not exist. With this
//! control, as long as the attribute to be added has the same value
//! as the existing attribute, then the modify will succeed. With this
//! control, deletion of an attribute that does not exist will also
//! succeed.

constant LDAP_SERVER_QUOTA_CONTROL_OID = "1.2.840.113556.1.4.1852";
//! LDAP control: Microsoft AD: Control used to pass the SID of a
//! security principal, whose quota is being queried, to the server in
//! a LDAP search operation.

constant LDAP_SERVER_RESP_SORT_OID = "1.2.840.113556.1.4.474";
//! LDAP control: Microsoft AD: Control used by the server to indicate
//! the results of a search function initiated using the
//! @[LDAP_SERVER_SORT_OID] control.

constant LDAP_SERVER_SD_FLAGS_OID = "1.2.840.113556.1.4.801";
//! LDAP control: Microsoft AD: Control used to pass flags to the
//! server to control various security descriptor results.

constant LDAP_SERVER_SEARCH_OPTIONS_OID = "1.2.840.113556.1.4.1340";
//! LDAP control: Microsoft AD: Control used to pass flags to the
//! server to control various search behaviors.

constant LDAP_SERVER_SHOW_DELETED_OID = "1.2.840.113556.1.4.417";
//! LDAP control: Microsoft AD: Control used to specify that the
//! search results include any deleted objects that match the search
//! filter.

constant LDAP_SERVER_SORT_OID = "1.2.840.113556.1.4.473";
//! LDAP control: Microsoft AD: Control used to instruct the server to
//! sort the search results before returning them to the client
//! application.

constant LDAP_SERVER_TREE_DELETE_OID = "1.2.840.113556.1.4.805";
//! LDAP control: Microsoft AD: Control used to delete an entire
//! subtree in the directory.

constant LDAP_SERVER_VERIFY_NAME_OID = "1.2.840.113556.1.4.1338";
//! LDAP control: Microsoft AD: Control used to instruct the DC
//! accepting the update which DC it should verify with, the existence
//! of any DN attribute values.

// LDAP syntaxes

constant SYNTAX_ATTR_TYPE_DESCR = "1.3.6.1.4.1.1466.115.121.1.3"; // RFC 2252, 6.1
constant SYNTAX_BINARY = "1.3.6.1.4.1.1466.115.121.1.5"; // RFC 2252, 6.2
constant SYNTAX_BIT_STRING = "1.3.6.1.4.1.1466.115.121.1.6"; // RFC 2252, 6.3
constant SYNTAX_BOOLEAN = "1.3.6.1.4.1.1466.115.121.1.7"; // RFC 2252, 6.4
constant SYNTAX_CERT = "1.3.6.1.4.1.1466.115.121.1.8"; // RFC 2252, 6.5
constant SYNTAX_CERT_LIST = "1.3.6.1.4.1.1466.115.121.1.9"; // RFC 2252, 6.6
constant SYNTAX_CERT_PAIR = "1.3.6.1.4.1.1466.115.121.1.10"; // RFC 2252, 6.7
constant SYNTAX_COUNTRY_STR = "1.3.6.1.4.1.1466.115.121.1.11"; // RFC 2252, 6.8
constant SYNTAX_DN = "1.3.6.1.4.1.1466.115.121.1.12"; // RFC 2252, 6.9
constant SYNTAX_DIRECTORY_STR = "1.3.6.1.4.1.1466.115.121.1.15"; // RFC 2252, 6.10
constant SYNTAX_DIT_CONTENT_RULE_DESCR = "1.3.6.1.4.1.1466.115.121.1.16"; // RFC 2252, 6.11
constant SYNTAX_FACSIMILE_PHONE_NUM = "1.3.6.1.4.1.1466.115.121.1.22"; // RFC 2252, 6.12
constant SYNTAX_FAX = "1.3.6.1.4.1.1466.115.121.1.23"; // RFC 2252, 6.13
constant SYNTAX_GENERALIZED_TIME = "1.3.6.1.4.1.1466.115.121.1.24"; // RFC 2252, 6.14
constant SYNTAX_IA5_STR = "1.3.6.1.4.1.1466.115.121.1.26"; // RFC 2252, 6.15
constant SYNTAX_INT = "1.3.6.1.4.1.1466.115.121.1.27"; // RFC 2252, 6.16
constant SYNTAX_JPEG = "1.3.6.1.4.1.1466.115.121.1.28";	// RFC 2252, 6.17
constant SYNTAX_MATCHING_RULE_DESCR = "1.3.6.1.4.1.1466.115.121.1.30"; // RFC 2252, 6.18
constant SYNTAX_MATCHING_RULE_USE_DESCR = "1.3.6.1.4.1.1466.115.121.1.31"; // RFC 2252, 6.19
constant SYNTAX_MHS_OR_ADDR = "1.3.6.1.4.1.1466.115.121.1.33"; // RFC 2252, 6.20
constant SYNTAX_NAME_AND_OPTIONAL_UID = "1.3.6.1.4.1.1466.115.121.1.34"; // RFC 2252, 6.21
constant SYNTAX_NAME_FORM_DESCR = "1.3.6.1.4.1.1466.115.121.1.35"; // RFC 2252, 6.22
constant SYNTAX_NUMERIC_STRING = "1.3.6.1.4.1.1466.115.121.1.36"; // RFC 2252, 6.23
constant SYNTAX_OBJECT_CLASS_DESCR = "1.3.6.1.4.1.1466.115.121.1.37"; // RFC 2252, 6.24
constant SYNTAX_OID = "1.3.6.1.4.1.1466.115.121.1.38"; // RFC 2252, 6.25
constant SYNTAX_OTHER_MAILBOX = "1.3.6.1.4.1.1466.115.121.1.39"; // RFC 2252, 6.26
constant SYNTAX_POSTAL_ADDR = "1.3.6.1.4.1.1466.115.121.1.41"; // RFC 2252, 6.27
constant SYNTAX_PRESENTATION_ADDR = "1.3.6.1.4.1.1466.115.121.1.43"; // RFC 2252, 6.28
constant SYNTAX_PRINTABLE_STR = "1.3.6.1.4.1.1466.115.121.1.44"; // RFC 2252, 6.29
constant SYNTAX_PHONE_NUM = "1.3.6.1.4.1.1466.115.121.1.50"; // RFC 2252, 6.30
constant SYNTAX_UTC_TIME = "1.3.6.1.4.1.1466.115.121.1.53"; // RFC 2252, 6.31
constant SYNTAX_LDAP_SYNTAX_DESCR = "1.3.6.1.4.1.1466.115.121.1.54"; // RFC 2252, 6.32
constant SYNTAX_DIT_STRUCTURE_RULE_DESCR = "1.3.6.1.4.1.1466.115.121.1.17"; // RFC 2252, 6.33
//! LDAP syntax: Standard syntaxes from @rfc{2252@}.

constant SYNTAX_CASE_EXACT_STR = SYNTAX_DIRECTORY_STR;
//! @expr{"caseExactString"@} is an alias used in e.g. @rfc{2079@}.

constant SYNTAX_DELIVERY_METHOD = "1.3.6.1.4.1.1466.115.121.1.14"; // RFC 2256, 6.1
constant SYNTAX_ENHANCED_GUIDE = "1.3.6.1.4.1.1466.115.121.1.21"; // RFC 2256, 6.2
constant SYNTAX_GUIDE = "1.3.6.1.4.1.1466.115.121.1.25"; // RFC 2256, 6.3
constant SYNTAX_OCTET_STR = "1.3.6.1.4.1.1466.115.121.1.40"; // RFC 2256, 6.4
constant SYNTAX_TELETEX_TERMINAL_ID = "1.3.6.1.4.1.1466.115.121.1.51"; // RFC 2256, 6.5
constant SYNTAX_TELETEX_NUM = "1.3.6.1.4.1.1466.115.121.1.52"; // RFC 2256, 6.6
constant SYNTAX_SUPPORTED_ALGORITHM = "1.3.6.1.4.1.1466.115.121.1.49"; // RFC 2256, 6.7
//! LDAP syntax: Standard syntaxes from @rfc{2256@}.

//constant SYNTAX_ACI_ITEM = "1.3.6.1.4.1.1466.115.121.1.1";
//constant SYNTAX_ACCESS_POINT = "1.3.6.1.4.1.1466.115.121.1.2";
//constant SYNTAX_AUDIO = "1.3.6.1.4.1.1466.115.121.1.4";
//constant SYNTAX_DATA_QUALITY_SYNTAX = "1.3.6.1.4.1.1466.115.121.1.13";
//constant SYNTAX_DL_SUBMIT_PERMISSION = "1.3.6.1.4.1.1466.115.121.1.18";
//constant SYNTAX_DSA_QUALITY_SYNTAX = "1.3.6.1.4.1.1466.115.121.1.19";
//constant SYNTAX_DSE_TYPE = "1.3.6.1.4.1.1466.115.121.1.20";
//constant SYNTAX_MASTER_AND_SHADOW_ACCESS_POINTS = "1.3.6.1.4.1.1466.115.121.1.29";
//constant SYNTAX_MAIL_PREFERENCE = "1.3.6.1.4.1.1466.115.121.1.32";
//constant SYNTAX_PROTOCOL_INFO = "1.3.6.1.4.1.1466.115.121.1.42";
//constant SYNTAX_SUBTREE_SPEC = "1.3.6.1.4.1.1466.115.121.1.45";
//constant SYNTAX_SUPPLIER_INFO = "1.3.6.1.4.1.1466.115.121.1.46";
//constant SYNTAX_SUPPLIER_OR_CONSUMER = "1.3.6.1.4.1.1466.115.121.1.47";
//constant SYNTAX_SUPPLIER_AND_CONSUMER = "1.3.6.1.4.1.1466.115.121.1.48";
//constant SYNTAX_MODIFY_RIGHTS = "1.3.6.1.4.1.1466.115.121.1.55";
//constant SYNTAX_LDAP_SCHEMA_DEF = "1.3.6.1.4.1.1466.115.121.1.56";
//constant SYNTAX_LDAP_SCHEMA_DESCR = "1.3.6.1.4.1.1466.115.121.1.57";
//constant SYNTAX_SUBSTR_ASSERTION = "1.3.6.1.4.1.1466.115.121.1.58";
// Other syntaxes mentioned but not defined in RFC 2252. They are
// commented out to enable the debug warning in
// LDAP.Protocol.client.result.decode_entries.

// NB: According to <http://msdn.microsoft.com/library/en-us/
// adschema/adschema/syntaxes.asp>, Microsoft AD can use a bunch of
// X.500 syntaxes (mostly for the same types as those covered above),
// but they are very vaguely described. Also, they are apparently
// translated to LDAP syntax attributes over the LDAP protocol (at
// least for some attributes, e.g. msDS-hasMasterNCs).

//constant SYNTAX_AD_OR_NAME = "1.2.840.113556.1.4.1221";
constant SYNTAX_AD_DN_WITH_OCTET_STR = "1.2.840.113556.1.4.903";
constant SYNTAX_AD_DN_WITH_STR = "1.2.840.113556.1.4.904";
// The three syntaxes above can contain DN:s. It's not entirely clear
// whether those DN:s are UTF-8 encoded according to RFC 2253 or
// (obsolete) unencoded according to RFC 1779.
//
// Anyway, the AD attribute msDS-hasMasterNCs is associated with
// SYNTAX_DN (i.e. is UTF-8 encoded), and msDS-HasInstantiatedNCs is
// associated with SYNTAX_AD_DN_WITH_OCTET_STR. Both are automatically
// updated DNs in AD (see
// http://msdn.microsoft.com/library/en-us/adschema/adschema/s_object_ds_dn.asp
// and
// http://msdn.microsoft.com/library/en-us/adschema/adschema/s_object_dn_binary.asp,
// respectively), so it seems reasonable to assume that both are
// modern UTF-8 thingies. And SYNTAX_AD_DN_WITH_STR is another close
// variant
// (http://msdn.microsoft.com/library/en-us/adschema/adschema/s_object_dn_string.asp).
//
// However, SYNTAX_AD_OR_NAME is still uncertain, so it's commented
// out to enable the debug warning in client.pike.
constant SYNTAX_AD_CASE_IGNORE_STR = "1.2.840.113556.1.4.905";
constant SYNTAX_AD_LARGE_INT = "1.2.840.113556.1.4.906";
constant SYNTAX_AD_OBJECT_SECURITY_DESCRIPTOR = "1.2.840.113556.1.4.907";
//! LDAP syntax: Microsoft AD: Additional syntaxes used in AD. C.f.
//! <http://community.roxen.com/(all)/developers/idocs/drafts/
//! draft-armijo-ldap-syntax-00.html>.

constant syntax_decode_fns = ([
  SYNTAX_DN:				utf8_to_string,
  SYNTAX_DIRECTORY_STR:			utf8_to_string,
  SYNTAX_DIT_CONTENT_RULE_DESCR:	utf8_to_string,
  SYNTAX_MATCHING_RULE_DESCR:		utf8_to_string,
  SYNTAX_MATCHING_RULE_USE_DESCR:	utf8_to_string,
  SYNTAX_NAME_AND_OPTIONAL_UID:		utf8_to_string,
  SYNTAX_NAME_FORM_DESCR:		utf8_to_string,
  SYNTAX_OBJECT_CLASS_DESCR:		utf8_to_string,
  SYNTAX_POSTAL_ADDR:			utf8_to_string,
  SYNTAX_LDAP_SYNTAX_DESCR:		utf8_to_string,
  SYNTAX_DIT_STRUCTURE_RULE_DESCR:	utf8_to_string,
  SYNTAX_AD_DN_WITH_OCTET_STR:		utf8_to_string,
  SYNTAX_AD_DN_WITH_STR:		utf8_to_string,
]);

//! @decl constant mapping(string:function(string:string)) syntax_decode_fns;
//!
//! Mapping containing functions to decode charsets in syntaxes where
//! that's necessary. If the syntax is complex in a way that makes the
//! result ambiguous if decoded with a single charset transformation
//! then it should typically not be decoded here.
//!
//! These decoders are used on all attribute values returned by
//! @[Protocols.LDAP.client.result] functions.

mapping(string:function(string:string)|zero) syntax_encode_fns = ([]); // Filled in from syntax_decode_fns by create().

//! @decl constant mapping(string:function(string:string)) syntax_encode_fns;
//!
//! Mapping containing the reverse functions from @[syntax_decode_fns].

// Constant attribute descriptions for various standardized attributes
// for use with Protocols.LDAP.get_attr_type_descr.

constant ATD_createTimestamp = ([ // RFC 2252, 5.1.1
  "oid": "2.5.18.1",
  "NAME": ({"createTimestamp"}),
  "EQUALITY": "generalizedTimeMatch",
  "ORDERING": "generalizedTimeOrderingMatch",
  "syntax_oid": SYNTAX_GENERALIZED_TIME,
  "SINGLE-VALUE": 1,
  "NO-USER-MODIFICATION": 1,
  "USAGE": "directoryOperation",
]);
constant ATD_modifyTimestamp = ([ // RFC 2252, 5.1.2
  "oid": "2.5.18.2",
  "NAME": ({"modifyTimestamp"}),
  "EQUALITY": "generalizedTimeMatch",
  "ORDERING": "generalizedTimeOrderingMatch",
  "syntax_oid": SYNTAX_GENERALIZED_TIME,
  "SINGLE-VALUE": 1,
  "NO-USER-MODIFICATION": 1,
  "USAGE": "directoryOperation",
]);
constant ATD_creatorsName = ([ // RFC 2252, 5.1.3
  "oid": "2.5.18.3",
  "NAME": ({"creatorsName"}),
  "EQUALITY": "distinguishedNameMatch",
  "syntax_oid": SYNTAX_DN,
  "SINGLE-VALUE": 1,
  "NO-USER-MODIFICATION": 1,
  "USAGE": "directoryOperation",
]);
constant ATD_modifiersName = ([ // RFC 2252, 5.1.4
  "oid": "2.5.18.4",
  "NAME": ({"modifiersName"}),
  "EQUALITY": "distinguishedNameMatch",
  "syntax_oid": SYNTAX_DN,
  "SINGLE-VALUE": 1,
  "NO-USER-MODIFICATION": 1,
  "USAGE": "directoryOperation",
]);
constant ATD_subschemaSubentry = ([ // RFC 2252, 5.1.5
  "oid": "2.5.18.10",
  "NAME": ({"subschemaSubentry"}),
  "EQUALITY": "distinguishedNameMatch",
  "syntax_oid": SYNTAX_DN,
  "SINGLE-VALUE": 1,
  "NO-USER-MODIFICATION": 1,
  "USAGE": "directoryOperation",
]);
constant ATD_attributeTypes = ([ // RFC 2252, 5.1.6
  "oid": "2.5.21.5",
  "NAME": ({"attributeTypes"}),
  "EQUALITY": "objectIdentifierFirstComponentMatch",
  "syntax_oid": SYNTAX_ATTR_TYPE_DESCR,
  "USAGE": "directoryOperation",
]);
constant ATD_objectClasses = ([ // RFC 2252, 5.1.7
  "oid": "2.5.21.6",
  "NAME": ({"objectClasses"}),
  "EQUALITY": "objectIdentifierFirstComponentMatch",
  "syntax_oid": SYNTAX_OBJECT_CLASS_DESCR,
  "USAGE": "directoryOperation",
]);
constant ATD_matchingRules = ([ // RFC 2252, 5.1.8
  "oid": "2.5.21.4",
  "NAME": ({"matchingRules"}),
  "EQUALITY": "objectIdentifierFirstComponentMatch",
  "syntax_oid": SYNTAX_MATCHING_RULE_DESCR,
  "USAGE": "directoryOperation",
]);
constant ATD_matchingRuleUse = ([ // RFC 2252, 5.1.9
  "oid": "2.5.21.8",
  "NAME": ({"matchingRuleUse"}),
  "EQUALITY": "objectIdentifierFirstComponentMatch",
  "syntax_oid": SYNTAX_MATCHING_RULE_USE_DESCR,
  "USAGE": "directoryOperation",
]);
constant ATD_namingContexts = ([ // RFC 2252, 5.2.1
  "oid": "1.3.6.1.4.1.1466.101.120.5",
  "NAME": ({"namingContexts"}),
  "syntax_oid": SYNTAX_DN,
  "USAGE": "dSAOperation",
]);
constant ATD_altServer = ([ // RFC 2252, 5.2.2
  "oid": "1.3.6.1.4.1.1466.101.120.6",
  "NAME": ({"altServer"}),
  "syntax_oid": SYNTAX_IA5_STR,
  "USAGE": "dSAOperation",
]);
constant ATD_supportedExtension = ([ // RFC 2252, 5.2.3
  "oid": "1.3.6.1.4.1.1466.101.120.7",
  "NAME": ({"supportedExtension"}),
  "syntax_oid": SYNTAX_OID,
  "USAGE": "dSAOperation",
]);
constant ATD_supportedControl = ([ // RFC 2252, 5.2.4
  "oid": "1.3.6.1.4.1.1466.101.120.13",
  "NAME": ({"supportedControl"}),
  "syntax_oid": SYNTAX_OID,
  "USAGE": "dSAOperation",
]);
constant ATD_supportedSASLMechanisms = ([ // RFC 2252, 5.2.5
  "oid": "1.3.6.1.4.1.1466.101.120.14",
  "NAME": ({"supportedSASLMechanisms"}),
  "syntax_oid": SYNTAX_DIRECTORY_STR,
  "USAGE": "dSAOperation",
]);
constant ATD_supportedLDAPVersion = ([ // RFC 2252, 5.2.6
  "oid": "1.3.6.1.4.1.1466.101.120.15",
  "NAME": ({"supportedLDAPVersion"}),
  "syntax_oid": SYNTAX_INT,
  "USAGE": "dSAOperation",
]);
constant ATD_ldapSyntaxes = ([ // RFC 2252, 5.3.1
  "oid": "1.3.6.1.4.1.1466.101.120.16",
  "NAME": ({"ldapSyntaxes"}),
  "EQUALITY": "objectIdentifierFirstComponentMatch",
  "syntax_oid": SYNTAX_LDAP_SYNTAX_DESCR,
  "USAGE": "directoryOperation",
]);
constant ATD_dITStructureRules = ([ // RFC 2252, 5.4.1
  "oid": "2.5.21.1",
  "NAME": ({"dITStructureRules"}),
  "EQUALITY": "integerFirstComponentMatch",
  "syntax_oid": SYNTAX_DIT_STRUCTURE_RULE_DESCR,
  "USAGE": "directoryOperation",
]);
constant ATD_nameForms = ([ // RFC 2252, 5.4.2
  "oid": "2.5.21.7",
  "NAME": ({"nameForms"}),
  "EQUALITY": "objectIdentifierFirstComponentMatch",
  "syntax_oid": SYNTAX_NAME_FORM_DESCR,
  "USAGE": "directoryOperation",
]);
constant ATD_ditContentRules = ([ // RFC 2252, 5.4.3
  "oid": "2.5.21.2",
  "NAME": ({"dITContentRules"}),
  "EQUALITY": "objectIdentifierFirstComponentMatch",
  "syntax_oid": SYNTAX_DIT_CONTENT_RULE_DESCR,
  "USAGE": "directoryOperation",
]);

constant ATD_objectClass = ([ // RFC 2256, 5.1
  "oid": "2.5.4.0",
  "NAME": ({"objectClass"}),
  "EQUALITY": "objectIdentifierMatch",
  "syntax_oid": SYNTAX_OID,
]);
constant ATD_aliasedObjectName = ([ // RFC 2256, 5.2
  "oid": "2.5.4.1",
  "NAME": ({"aliasedObjectName"}),
  "EQUALITY": "distinguishedNameMatch",
  "syntax_oid": SYNTAX_DN,
  "SINGLE-VALUE": 1,
]);
constant ATD_knowledgeInformation = ([ // RFC 2256, 5.3
  "oid": "2.5.4.2",
  "NAME": ({"knowledgeInformation"}),
  "EQUALITY": "caseIgnoreMatch",
  "syntax_oid": SYNTAX_DIRECTORY_STR,
  "syntax_len": 32768,
]);
constant ATD_cn = ([ // RFC 2256, 5.4
  "oid": "2.5.4.3",
  "NAME": ({"cn"}),
  "SUP": "name",
]);
constant ATD_sn = ([ // RFC 2256, 5.5
  "oid": "2.5.4.4",
  "NAME": ({"sn"}),
  "SUP": "name",
]);
constant ATD_serialNumber = ([ // RFC 2256, 5.6
  "oid": "2.5.4.5",
  "NAME": ({"serialNumber"}),
  "EQUALITY": "caseIgnoreMatch",
  "SUBSTR": "caseIgnoreSubstringsMatch",
  "syntax_oid": SYNTAX_PRINTABLE_STR,
  "syntax_len": 64,
]);
constant ATD_c = ([ // RFC 2256, 5.7
  "oid": "2.5.4.6",
  "NAME": ({"c"}),
  "SUP": "name",
  "SINGLE-VALUE": 1,
]);
constant ATD_l = ([ // RFC 2256, 5.8
  "oid": "2.5.4.7",
  "NAME": ({"l"}),
  "SUP": "name",
]);
constant ATD_st = ([ // RFC 2256, 5.9
  "oid": "2.5.4.8",
  "NAME": ({"st"}),
  "SUP": "name",
]);
constant ATD_street = ([ // RFC 2256, 5.10
  "oid": "2.5.4.9",
  "NAME": ({"street"}),
  "EQUALITY": "caseIgnoreMatch",
  "SUBSTR": "caseIgnoreSubstringsMatch",
  "syntax_oid": SYNTAX_DIRECTORY_STR,
  "syntax_len": 128,
]);
constant ATD_o = ([ // RFC 2256, 5.11
  "oid": "2.5.4.10",
  "NAME": ({"o"}),
  "SUP": "name",
]);
constant ATD_ou = ([ // RFC 2256, 5.12
  "oid": "2.5.4.11",
  "NAME": ({"ou"}),
  "SUP": "name",
]);
constant ATD_title = ([ // RFC 2256, 5.13
  "oid": "2.5.4.12",
  "NAME": ({"title"}),
  "SUP": "name",
]);
constant ATD_description = ([ // RFC 2256, 5.14
  "oid": "2.5.4.13",
  "NAME": ({"description"}),
  "EQUALITY": "caseIgnoreMatch",
  "SUBSTR": "caseIgnoreSubstringsMatch",
  "syntax_oid": SYNTAX_DIRECTORY_STR,
  "syntax_len": 1024,
]);
constant ATD_searchGuide = ([ // RFC 2256, 5.15
  "oid": "2.5.4.14",
  "NAME": ({"searchGuide"}),
  "syntax_oid": SYNTAX_GUIDE,
]);
constant ATD_businessCategory = ([ // RFC 2256, 5.16
  "oid": "2.5.4.15",
  "NAME": ({"businessCategory"}),
  "EQUALITY": "caseIgnoreMatch",
  "SUBSTR": "caseIgnoreSubstringsMatch",
  "syntax_oid": SYNTAX_DIRECTORY_STR,
  "syntax_len": 128,
]);
constant ATD_postalAddress = ([ // RFC 2256, 5.17
  "oid": "2.5.4.16",
  "NAME": ({"postalAddress"}),
  "EQUALITY": "caseIgnoreListMatch",
  "SUBSTR": "caseIgnoreListSubstringsMatch",
  "syntax_oid": SYNTAX_POSTAL_ADDR,
]);
constant ATD_postalCode = ([ // RFC 2256, 5.18
  "oid": "2.5.4.17",
  "NAME": ({"postalCode"}),
  "EQUALITY": "caseIgnoreMatch",
  "SUBSTR": "caseIgnoreSubstringsMatch",
  "syntax_oid": SYNTAX_DIRECTORY_STR,
  "syntax_len": 40,
]);
constant ATD_postOfficeBox = ([ // RFC 2256, 5.19
  "oid": "2.5.4.18",
  "NAME": ({"postOfficeBox"}),
  "EQUALITY": "caseIgnoreMatch",
  "SUBSTR": "caseIgnoreSubstringsMatch",
  "syntax_oid": SYNTAX_DIRECTORY_STR,
  "syntax_len": 40,
]);
constant ATD_physicalDeliveryOfficeName = ([ // RFC 2256, 5.20
  "oid": "2.5.4.19",
  "NAME": ({"physicalDeliveryOfficeName"}),
  "EQUALITY": "caseIgnoreMatch",
  "SUBSTR": "caseIgnoreSubstringsMatch",
  "syntax_oid": SYNTAX_DIRECTORY_STR,
  "syntax_len": 128,
]);
constant ATD_telephoneNumber = ([ // RFC 2256, 5.21
  "oid": "2.5.4.20",
  "NAME": ({"telephoneNumber"}),
  "EQUALITY": "telephoneNumberMatch",
  "SUBSTR": "telephoneNumberSubstringsMatch",
  "syntax_oid": SYNTAX_PHONE_NUM,
  "syntax_len": 32,
]);
constant ATD_telexNumber = ([ // RFC 2256, 5.22
  "oid": "2.5.4.21",
  "NAME": ({"telexNumber"}),
  "syntax_oid": SYNTAX_TELETEX_NUM,
]);
constant ATD_teletexTerminalIdentifier = ([ // RFC 2256, 5.23
  "oid": "2.5.4.22",
  "NAME": ({"teletexTerminalIdentifier"}),
  "syntax_oid": SYNTAX_TELETEX_TERMINAL_ID,
]);
constant ATD_facsimileTelephoneNumber = ([ // RFC 2256, 5.24
  "oid": "2.5.4.23",
  "NAME": ({"facsimileTelephoneNumber"}),
  "syntax_oid": SYNTAX_FACSIMILE_PHONE_NUM,
]);
constant ATD_x121Address = ([ // RFC 2256, 5.25
  "oid": "2.5.4.24",
  "NAME": ({"x121Address"}),
  "EQUALITY": "numericStringMatch",
  "SUBSTR": "numericStringSubstringsMatch",
  "syntax_oid": SYNTAX_NUMERIC_STRING,
  "syntax_len": 15,
]);
constant ATD_internationaliSDNNumber = ([ // RFC 2256, 5.26
  "oid": "2.5.4.25",
  "NAME": ({"internationaliSDNNumber"}),
  "EQUALITY": "numericStringMatch",
  "SUBSTR": "numericStringSubstringsMatch",
  "syntax_oid": SYNTAX_NUMERIC_STRING,
  "syntax_len": 16,
]);
constant ATD_registeredAddress = ([ // RFC 2256, 5.27
  "oid": "2.5.4.26",
  "NAME": ({"registeredAddress"}),
  "SUP": "postalAddress",
  "syntax_oid": SYNTAX_POSTAL_ADDR,
]);
constant ATD_destinationIndicator = ([ // RFC 2256, 5.28
  "oid": "2.5.4.27",
  "NAME": ({"destinationIndicator"}),
  "EQUALITY": "caseIgnoreMatch",
  "SUBSTR": "caseIgnoreSubstringsMatch",
  "syntax_oid": SYNTAX_PRINTABLE_STR,
  "syntax_len": 128,
]);
constant ATD_preferredDeliveryMethod = ([ // RFC 2256, 5.29
  "oid": "2.5.4.28",
  "NAME": ({"preferredDeliveryMethod"}),
  "syntax_oid": SYNTAX_DELIVERY_METHOD,
  "SINGLE-VALUE": 1,
]);
constant ATD_presentationAddress = ([ // RFC 2256, 5.30
  "oid": "2.5.4.29",
  "NAME": ({"presentationAddress"}),
  "EQUALITY": "presentationAddressMatch",
  "syntax_oid": SYNTAX_PRESENTATION_ADDR,
  "SINGLE-VALUE": 1,
]);
constant ATD_supportedApplicationContext = ([ // RFC 2256, 5.31
  "oid": "2.5.4.30",
  "NAME": ({"supportedApplicationContext"}),
  "EQUALITY": "objectIdentifierMatch",
  "syntax_oid": SYNTAX_OID,
]);
constant ATD_member = ([ // RFC 2256, 5.32
  "oid": "2.5.4.31",
  "NAME": ({"member"}),
  "SUP": "distinguishedName",
]);
constant ATD_owner = ([ // RFC 2256, 5.33
  "oid": "2.5.4.32",
  "NAME": ({"owner"}),
  "SUP": "distinguishedName",
]);
constant ATD_roleOccupant = ([ // RFC 2256, 5.34
  "oid": "2.5.4.33",
  "NAME": ({"roleOccupant"}),
  "SUP": "distinguishedName",
]);
constant ATD_seeAlso = ([ // RFC 2256, 5.35
  "oid": "2.5.4.34",
  "NAME": ({"seeAlso"}),
  "SUP": "distinguishedName",
]);
constant ATD_userPassword = ([ // RFC 2256, 5.36
  "oid": "2.5.4.35",
  "NAME": ({"userPassword"}),
  "EQUALITY": "octetStringMatch",
  "syntax_oid": SYNTAX_OCTET_STR,
  "syntax_len": 128,
]);
constant ATD_userCertificate = ([ // RFC 2256, 5.37
  "oid": "2.5.4.36",
  "NAME": ({"userCertificate"}),
  "syntax_oid": SYNTAX_CERT,
]);
constant ATD_cACertificate = ([ // RFC 2256, 5.38
  "oid": "2.5.4.37",
  "NAME": ({"cACertificate"}),
  "syntax_oid": SYNTAX_CERT,
]);
constant ATD_authorityRevocationList = ([ // RFC 2256, 5.39
  "oid": "2.5.4.38",
  "NAME": ({"authorityRevocationList"}),
  "syntax_oid": SYNTAX_CERT_LIST,
]);
constant ATD_certificateRevocationList = ([ // RFC 2256, 5.40
  "oid": "2.5.4.39",
  "NAME": ({"certificateRevocationList"}),
  "syntax_oid": SYNTAX_CERT_LIST,
]);
constant ATD_crossCertificatePair = ([ // RFC 2256, 5.41
  "oid": "2.5.4.40",
  "NAME": ({"crossCertificatePair"}),
  "syntax_oid": SYNTAX_CERT_PAIR,
]);
constant ATD_name = ([ // RFC 2256, 5.42
  "oid": "2.5.4.41",
  "NAME": ({"name"}),
  "EQUALITY": "caseIgnoreMatch",
  "SUBSTR": "caseIgnoreSubstringsMatch",
  "syntax_oid": SYNTAX_DIRECTORY_STR,
  "syntax_len": 32768,
]);
constant ATD_givenName = ([ // RFC 2256, 5.43
  "oid": "2.5.4.42",
  "NAME": ({"givenName"}),
  "SUP": "name",
]);
constant ATD_initials = ([ // RFC 2256, 5.44
  "oid": "2.5.4.43",
  "NAME": ({"initials"}),
  "SUP": "name",
]);
constant ATD_generationQualifier = ([ // RFC 2256, 5.45
  "oid": "2.5.4.44",
  "NAME": ({"generationQualifier"}),
  "SUP": "name",
]);
constant ATD_x500UniqueIdentifier = ([ // RFC 2256, 5.46
  "oid": "2.5.4.45",
  "NAME": ({"x500UniqueIdentifier"}),
  "EQUALITY": "bitStringMatch",
  "syntax_oid": SYNTAX_BIT_STRING,
]);
constant ATD_dnQualifier = ([ // RFC 2256, 5.47
  "oid": "2.5.4.46",
  "NAME": ({"dnQualifier"}),
  "EQUALITY": "caseIgnoreMatch",
  "ORDERING": "caseIgnoreOrderingMatch",
  "SUBSTR": "caseIgnoreSubstringsMatch",
  "syntax_oid": SYNTAX_PRINTABLE_STR,
]);
constant ATD_enhancedSearchGuide = ([ // RFC 2256, 5.48
  "oid": "2.5.4.47",
  "NAME": ({"enhancedSearchGuide"}),
  "syntax_oid": SYNTAX_ENHANCED_GUIDE,
]);
constant ATD_protocolInformation = ([ // RFC 2256, 5.49
  "oid": "2.5.4.48",
  "NAME": ({"protocolInformation"}),
  "EQUALITY": "protocolInformationMatch",
  "syntax_oid": "1.3.6.1.4.1.1466.115.121.1.42",
]);
constant ATD_distinguishedName = ([ // RFC 2256, 5.50
  "oid": "2.5.4.49",
  "NAME": ({"distinguishedName"}),
  "EQUALITY": "distinguishedNameMatch",
  "syntax_oid": SYNTAX_DN,
]);
constant ATD_uniqueMember = ([ // RFC 2256, 5.51
  "oid": "2.5.4.50",
  "NAME": ({"uniqueMember"}),
  "EQUALITY": "uniqueMemberMatch",
  "syntax_oid": SYNTAX_NAME_AND_OPTIONAL_UID,
]);
constant ATD_houseIdentifier = ([ // RFC 2256, 5.52
  "oid": "2.5.4.51",
  "NAME": ({"houseIdentifier"}),
  "EQUALITY": "caseIgnoreMatch",
  "SUBSTR": "caseIgnoreSubstringsMatch",
  "syntax_oid": SYNTAX_DIRECTORY_STR,
  "syntax_len": 32768,
]);
constant ATD_supportedAlgorithms = ([ // RFC 2256, 5.53
  "oid": "2.5.4.52",
  "NAME": ({"supportedAlgorithms"}),
  "syntax_oid": SYNTAX_SUPPORTED_ALGORITHM,
]);
constant ATD_deltaRevocationList = ([ // RFC 2256, 5.54
  "oid": "2.5.4.53",
  "NAME": ({"deltaRevocationList"}),
  "syntax_oid": SYNTAX_CERT_LIST,
]);
constant ATD_dmdName = ([ // RFC 2256, 5.55
  "oid": "2.5.4.54",
  "NAME": ({"dmdName"}),
  "SUP": "name",
]);

constant ATD_labeledURI = ([ // RFC 2079
  "oid": "1.3.6.1.4.1.250.1.57",
  "NAME": ({"labeledURI"}),
  "EQUALITY": "caseExactMatch",
  "syntax_oid": SYNTAX_CASE_EXACT_STR,
]);

constant ATD_supportedFeatures = ([ // RFC 3674, 2
  "oid": "1.3.6.1.4.1.4203.1.3.5",
  "NAME": ({"supportedFeatures"}),
  "EQUALITY": "objectIdentifierMatch",
  "syntax_oid": SYNTAX_OID,
  "USAGE": "dSAOperation",
]);

// Filled in by create(). Used by client.pike.
mapping(string:mixed) _standard_attr_type_descrs = ([]);

//! Constants for Microsoft AD Well-Known Object GUIDs. These are e.g.
//! used in LDAP URLs:
//!
//! @code
//!   "ldap://server/<WKGUID=" + Protocols.LDAP.GUID_USERS_CONTAINER +
//!   ",dc=my,dc=domain,dc=com>"
//! @endcode
constant GUID_USERS_CONTAINER              = "a9d1ca15768811d1aded00c04fd8d5cd";
constant GUID_COMPUTERS_CONTAINER          = "aa312825768811d1aded00c04fd8d5cd";
constant GUID_SYSTEMS_CONTAINER            = "ab1d30f3768811d1aded00c04fd8d5cd";
constant GUID_DOMAIN_CONTROLLERS_CONTAINER = "a361b2ffffd211d1aa4b00c04fd7d83a";
constant GUID_INFRASTRUCTURE_CONTAINER     = "2fbac1870ade11d297c400c04fd8d5cd";
constant GUID_DELETED_OBJECTS_CONTAINER    = "18e2ea80684f11d2b9aa00c04f79f805";
constant GUID_LOSTANDFOUND_CONTAINER       = "ab8153b7768811d1aded00c04fd8d5cd";
constant GUID_FOREIGNSECURITYPRINCIPALS_CONTAINER = "22b70c67d56e4efb91e9300fca3dc1aa";
constant GUID_PROGRAM_DATA_CONTAINER       = "09460c08ae1e4a4ea0f64aee7daa1e5a";
constant GUID_MICROSOFT_PROGRAM_DATA_CONTAINER = "f4be92a4c777485e878e9421d53087db";
constant GUID_NTDS_QUOTAS_CONTAINER        = "6227f0af1fc2410d8e3bb10615bb5b0f";

// Misc stuff

protected mapping(mixed:string) constant_val_lookup;

string get_constant_name (mixed val)
//! If @[val] matches any non-integer constant in this module, its
//! name is returned.
{
  if (!constant_val_lookup) {
    array(string) names = indices (this_program);
    array(mixed) vals = values (this_program);
    for (int i = 0; i < sizeof (names);)
      if (intp (vals[i]) ||
	  // Match all known secondary aliases here.
	  names[i] == "SYNTAX_CASE_EXACT_STR") {
	names = names[..i-1] + names[i+1..];
	vals = vals[..i-1] + vals[i+1..];
      }
      else
	i++;
    constant_val_lookup = mkmapping (vals, names);
    if (sizeof (constant_val_lookup) != sizeof (names))
      error ("Found %d duplicate constant names.\n",
	     sizeof (names) - sizeof (constant_val_lookup));
  }
  return constant_val_lookup[val];
}

protected void create()
// Fill in various calculated constants.
{
  // syntax_encode_fns

  foreach (indices (syntax_decode_fns), string syntax) {
    function(string:string)|zero encoder = ([
      utf8_to_string: string_to_utf8,
    ])[syntax_decode_fns[syntax]];
    if (!encoder)
      error ("Don't know the encoder corresponding to %O.\n",
	     syntax_decode_fns[syntax]);
    syntax_encode_fns[syntax] = encoder;
  }

  // _standard_attr_type_descrs

  // Note partial code dup with client.get_attr_type_descr.
  array(mapping(string:mixed)) incomplete = ({});

  foreach (indices (this_program), string name) {
    if (has_prefix (name, "ATD_")) {
      mapping(string:mixed) descr =
	// Pike doesn't allow a type as first arg in the native []
	// operator syntax.
	predef::`[] (this_program, name);
      if (_standard_attr_type_descrs[descr->oid])
	error ("OID conflict between %O and %O.\n",
	       _standard_attr_type_descrs[descr->oid], descr);
      if (descr->SUP) incomplete += ({descr});
      _standard_attr_type_descrs[descr->oid] = descr;
      foreach (descr->NAME, string name)
	_standard_attr_type_descrs[lower_case (name)] = descr;
    }
  }

  void complete (mapping(string:mixed) descr) {
    mapping(string:mixed) sup_descr =
      ([mapping(mixed:mapping(string:mixed))](mixed)_standard_attr_type_descrs)
      [lower_case (descr->SUP)];
    if (!sup_descr)
      error ("Got SUP reference to unknown attribute %O: %O\n",
	     descr->SUP, descr);
    if (sup_descr->SUP)
      complete (sup_descr);
    foreach (indices (sup_descr), string term)
      if (!has_index (descr, term))
	descr[term] = sup_descr[term];
  };
  foreach (incomplete, mapping(string:mixed) descr)
    complete (descr);

#ifdef LDAP_DEBUG
  // To discover duplicate constant names.
  get_constant_name (0);
#endif
}

protected constant supported_extensions = (<"bindname">);

//! Parses an LDAP URL and returns its fields in a mapping.
//!
//! @returns
//!   The returned mapping contains these fields:
//!
//!   @mapping
//!   @member string scheme
//!     The URL scheme, either @expr{"ldap"@} or @expr{"ldaps"@}.
//!   @member string host
//!   @member int port
//!   @member string basedn
//!     Self-explanatory.
//!   @member array(string) attributes
//!     Array containing the attributes. Undefined if none was
//!     specified.
//!   @member int scope
//!     The scope as one of the @expr{SEARCH_*@} constants.
//!     Undefined if none was specified.
//!   @member string filter
//!     The search filter. Undefined if none was specified.
//!   @member mapping(string:string|int(1..1)) ext
//!     The extensions. Undefined if none was specified. The mapping
//!     values are @expr{1@} for extensions without values. Critical
//!     extensions are checked and the leading @expr{"!"@} do not
//!     remain in the mapping indices.
//!   @member string url
//!     The original unparsed URL.
//!   @endmapping
//!
//! @seealso
//!   @[get_parsed_url]
mapping(string:mixed) parse_ldap_url (string ldap_url)
{
  array ar;
  mapping(string:mixed) res = (["url": ldap_url]);

  if (sscanf (ldap_url, "%[^:]://%s", res->scheme, ldap_url) != 2)
    ERROR ("Failed to parse scheme from ldap url.\n");

  sscanf (ldap_url, "%[^/]/%s", string hostport, ldap_url);
  sscanf (hostport, "%[^:]:%s", res->host, string port);
  if (port)
    if (sscanf (port, "%d%*c", res->port) != 1)
      ERROR ("Failed to parse port number from %O.\n", port);

  ar = ldap_url / "?";

  switch (sizeof(ar)) {
    case 5: if (sizeof(ar[4])) {
      mapping extensions = ([]);
      foreach(ar[4] / ",", string ext) {
	sscanf (ext, "%[^=]=%s", string extype, string exvalue);
	extype = _Roxen.http_decode_string (extype);
	if (has_prefix (extype, "!")) {
	  extype = extype[1..];
	  if (!supported_extensions[extype[1..]])
	    ERROR ("Critical extension %s is not supported.\n", extype);
	}
	if (extype == "")
	  ERROR ("Failed to parse extension type from %O.\n", ext);
	extensions[extype] =
	  exvalue ? _Roxen.http_decode_string (exvalue) : 1;
      }
      res->ext = extensions;
    }
    case 4: res->filter = _Roxen.http_decode_string (ar[3]);
    case 3: res->scope = (["base": SCOPE_BASE,
			   "one": SCOPE_ONE,
			   "sub": SCOPE_SUB])[ar[2]];
    case 2: if (sizeof(ar[1])) res->attributes =
				 map (ar[1] / ",", _Roxen.http_decode_string);
    case 1: res->basedn = _Roxen.http_decode_string (ar[0]);
      break;
    default: res->basedn = "";
  }

  return res;
}

class FilterError
//! Error object thrown by @[make_filter] for parse errors.
{
  constant is_ldap_filter_error = 1;
  //! Recognition constant.

  constant is_generic_error = 1;
  string error_message;
  array error_backtrace;
  protected string|array `[] (int i) {
    return i ? error_backtrace : error_message;
  }
  protected void create (string msg, mixed... args)
  {
    if (sizeof (args)) msg = sprintf (msg, @args);
    error_message = msg;
    error_backtrace = backtrace();
    error_backtrace = error_backtrace[..<1];
    throw (this);
  }
}

object make_filter (string filter, void|int ldap_version)
//! Parses an LDAP filter string into an ASN1 object tree that can be
//! given to @[Protocols.LDAP.search].
//!
//! Using this function instead of giving the filter string directly
//! to the search function has two advantages: This function provides
//! better error reports for syntax errors, and the same object tree
//! can be used repeatedly to avoid reparsing the filter string.
//!
//! @param filter
//!   The filter to parse, according to the syntax specified in
//!   @rfc{2254@}. The syntax is extended a bit to allow and ignore
//!   whitespace everywhere except inside and next to the filter
//!   values.
//!
//! @param ldap_version
//!   LDAP protocol version to make the filter for. This controls what
//!   syntaxes are allowed depending on the protocol version. Also, if
//!   the protocol is @expr{3@} or later then full Unicode string
//!   literals are supported. The default is the latest supported
//!   version.
//!
//! @returns
//!   An ASN1 object tree representing the filter.
//!
//! @throws
//!   @[FilterError] is thrown if there's a syntax error in the
//!   filter.
{
#define EXCERPT(STR) (sizeof (STR) > 30 ? (STR)[..27] + "..." : (STR))

  if (!ldap_version) ldap_version = 3;
  if (ldap_version >= 3)
    filter = string_to_utf8 (filter);

  // Afaict from the RFCs no insignificant whitespace is allowed in
  // query filters, but the old parser and other query tools (at
  // least LDP in Windows XP) seem to accept it anyway. This makes
  // it somewhat unclear whether whitespace around values should be
  // significant or not. This parser always treat whitespace as
  // significant there (and inside operators) but not anywhere else.
  // /mast
#define WS "%*[ \t\n\r]"

  string read_filter_value()
  // Reads a filter value encoded according to section 4 in RFC 2254
  // and section 3 in the older RFC 1960.
  {
    string res = "";
    //werror ("read_filter_value %O\n", filter);
    while (1) {
      sscanf (filter, "%[^()*\\]%s", string val, filter);
      res += val;
      if (filter == "" || (<'(', ')', '*'>)[filter[0]]) break;
      // filter[0] == '\\' now.
      if (sscanf (filter, "\\%1x%1x%s", int high, int low, filter) == 3)
	// RFC 2254. (Use two %1x to force reading of exactly two
	// hex digits; something like %2.2x is currently not
	// supported.)
	res += sprintf ("%c", (high << 4) + low);
      else {
	// RFC 1960. Note that this RFC doesn't define a consistent
	// quoting since a "\" shouldn't be quoted. That means it
	// isn't possible to specify a value ending with "\".
	if (sscanf (filter, "\\%1[*()]%s", val, filter) == 2)
	  res += val;
	else {
	  res += filter[..1];
	  filter = filter[2..];
	}
      }
    }
    //werror ("read_filter_value => %O (rest: %O)\n", res, filter);
    return res;
  };

  object make_filter_recur()
  {
    string subfilter = filter;
    object res;

    //werror ("make_filter_recur %O\n", filter);

    if (sscanf (filter, WS"("WS"%s", filter) != 3)
      FilterError ("Expected opening parenthesis at the start of %O.\n",
		   EXCERPT (subfilter));
    if (filter == "")
      FilterError ("Unexpected end of filter expression.\n");

    switch (filter[0]) {
      case '&':
      case '|': {
	int op = filter[0];
	filter = filter[1..];
	array(object) subs = ({});
	do {
	  subs += ({make_filter_recur()});
	} while (has_prefix (filter, "("));
	res = ASN1_CONTEXT_SET (op == '&' ? 0 : 1, subs); // 'and' or 'or'
	break;
      }

      case '!': {
	filter = filter[1..];
	res = ASN1_CONTEXT_SEQUENCE (2, ({make_filter_recur()})); // 'not'
	break;
      }

      default: {
	string attr;
	// Doing a slightly lax check of the attribute here: Periods
	// are only allowed if's a string of decimal digits separated
	// by them.
	sscanf (filter, "%[-A-Za-z0-9.]"WS"%s", attr, filter);
	if (filter == "")
	  FilterError ("Unexpected end of the filter expression starting at %O.\n",
		       EXCERPT (subfilter));

	string op;
	switch (filter[0]) {
	  case ':': case '=':
	    sscanf (filter, "%1s%s", op, filter);
	    break;
	  case '~': case '>': case '<':
	    if (sscanf (filter, "%2s%s", op, filter) != 2 || op[1] != '=')
	      FilterError ("Invalid filter type in the expression starting at %O.\n",
			   EXCERPT (subfilter));
	    break;
	  default:
	    FilterError ("Invalid filter type in the expression starting at %O.\n",
			 EXCERPT (subfilter));
	}

	if (op == ":") {
	  // LDAP 3 extensible match. Note that we haven't really
	  // parsed the operator in this case.

	  if (ldap_version < 3)
	    FilterError ("Invalid filter type in the expression starting at %O.\n",
			 EXCERPT (subfilter));

	  int dn_attrs = sscanf (filter, "dn"WS":"WS"%s", filter) == 3;
	  string matching_rule;
	  if (has_prefix (filter, "=")) {
	    if (attr == "")
	      FilterError ("Must specify either an attribute, "
			   "a matching rule identifier or both "
			   "in the expression starting at %O.\n",
			   EXCERPT (subfilter));
	    filter = filter[1..];
	  }
	  else {
	    // Same kind of slightly lax check on the matching rule
	    // identifier here as on the attribute above.
	    if (sscanf (filter, "%[-A-Za-z0-9.]"WS":=%s", matching_rule, filter) != 3 ||
		matching_rule == "")
	      FilterError ("Error parsing matching rule identifier "
			   "in the expression starting at %O.\n",
			   EXCERPT (subfilter));
	  }

	  string val = read_filter_value();

	  res = ASN1_CONTEXT_SEQUENCE (	// 'extensibleMatch'
	    9, ((matching_rule ?
		 ({ASN1_CONTEXT_OCTET_STRING (1, matching_rule)}) : ({})) +
		(attr != "" ?
		 ({ASN1_CONTEXT_OCTET_STRING (2, attr)}) : ({})) +
		({ASN1_CONTEXT_OCTET_STRING (3, val),
		  ASN1_CONTEXT_BOOLEAN (4, dn_attrs)})));
	}

	else {
	  if (attr == "")
	    FilterError ("Expected attribute in the expression starting at %O.\n",
			 EXCERPT (subfilter));

	  string|object val = read_filter_value();

	  if (op == "=" && has_prefix (filter, "*")) {
	    array(string) parts = ({val});
	    do {
	      filter = filter[1..];
	      val = read_filter_value();
	      parts += ({val});
	    } while (has_prefix (filter, "*"));

	    array(object) subs = sizeof (parts[0]) ?
	      ({ASN1_CONTEXT_OCTET_STRING (0, parts[0])}) : ({});
	    foreach (parts[1..<1], string middle)
	      subs += ({ASN1_CONTEXT_OCTET_STRING (1, middle)});
	    if (sizeof (parts) > 1 && sizeof (parts[-1]))
	      subs += ({ASN1_CONTEXT_OCTET_STRING (2, parts[-1])});

	    if (!sizeof (subs))
	      res = ASN1_CONTEXT_OCTET_STRING (7, attr); // 'present'
	    else
	      res = ASN1_CONTEXT_SEQUENCE ( // 'substrings'
		4, ({Standards.ASN1.Types.OctetString (attr),
		     Standards.ASN1.Types.Sequence (subs)}));
	  }

	  else {
	    int op_number;
	    switch (op) {
	      case "=": op_number = 3; break;	// 'equalityMatch'
	      case ">=": op_number = 5; break; // 'greaterOrEqual'
	      case "<=": op_number = 6; break; // 'lessOrEqual'
	      case "~=": op_number = 8; break; // 'approxMatch'
	      default:			// Shouldn't be reached.
		FilterError ("Invalid filter type in the expression "
			     "starting at %O.\n", EXCERPT (subfilter));
	    }
	    res = ASN1_CONTEXT_SEQUENCE (
	      op_number, ({Standards.ASN1.Types.OctetString (attr),
			   Standards.ASN1.Types.OctetString (val)}));
	  }
	}

	break;
      }
    }

    //werror ("make_filter_recur => %O (rest: %O)\n", res, filter);

    if (sscanf (filter, WS")"WS"%s", filter) != 3)
      FilterError ("Missing closing parenthesis in the expression "
		   "starting at %O.\n", EXCERPT (subfilter));
    return res;
  };

  object res = make_filter_recur();
  if (filter != "")
    FilterError ("Unexpected data beginning with %O "
		 "after end of filter expression.\n", EXCERPT (filter));
  return res;

#undef WS
#undef EXCERPT
}

protected mapping(string:array(object)) cached_filters = ([]);

object get_cached_filter (string filter, void|int ldap_version)
//! Like @[make_filter] but saves the generated objects for reuse.
//! Useful for filters that reasonably will occur often. The cache is
//! never garbage collected, however.
//!
//! @throws
//!   If there's a parse error in the filter then a @[FilterError] is
//!   thrown as from @[make_filter].
{
  if (!ldap_version) ldap_version = 3;
  array(object) arr = cached_filters[filter] || ({});
  if (object res = sizeof (arr) > ldap_version && arr[ldap_version])
    return res;
  object res = make_filter (filter, ldap_version);
  if (sizeof (arr) <= ldap_version)
    arr = cached_filters[filter] = arr + allocate (ldap_version + 1 - sizeof (arr));
  return arr[ldap_version] = res;
}

// Client connection pool

constant connection_idle_timeout = 30;
// This better be shorter than the server side timeout. (According to
// http://support.microsoft.com/kb/315071, the AD default value is 900
// seconds, and the server sends an LDAP disconnect notification
// before closing it, which we ought to look for.)

constant connection_rebind_threshold = 4;
// Let there be a couple of differently bound connections before
// starting to rebind the same ones.

constant connection_idle_garb_interval = 60;
// Garb idle connections once every minute.

protected mapping(string:array(object/*(client)*/)) idle_conns = ([]);
protected Thread.Mutex idle_conns_mutex = Thread.Mutex();
protected mixed periodic_idle_conns_garb_call_out;

protected void periodic_idle_conns_garb()
{
  Thread.MutexKey lock = idle_conns_mutex->lock();
  DWRITE ("Periodic connection garb. Got %d urls.\n", sizeof (idle_conns));

  int garb_time = time() - connection_idle_timeout;
  foreach (idle_conns; string ldap_url; array(object/*(client)*/) conns) {
    int garbed = 0;
    foreach (conns; int i; object/*(client)*/ conn)
      if (conn->get_last_io_time() <= garb_time) {
	DWRITE ("Garbing connection to %O which has been idle for %d s.\n",
		ldap_url, time() - conn->get_last_io_time());
	conns[i] = 0, garbed = 1;
      }
    if (garbed) {
      conns -= ({0});
      if (sizeof (conns)) idle_conns[ldap_url] = conns;
      else m_delete (idle_conns, ldap_url);
    }
  }

  if (sizeof (idle_conns))
    periodic_idle_conns_garb_call_out =
      call_out (periodic_idle_conns_garb, connection_idle_garb_interval);
  else {
    DWRITE ("No connections left - disabling periodic garb.\n");
    periodic_idle_conns_garb_call_out = 0;
  }
}

// This mapping is used to attempt to pin connections to a single
// LDAP server in the case of clustering with DNS round-robin.
//
// Mapping from hostname to an array with the index of the most
// recently working connection in index 0, followed by ip-numbers.
protected mapping(string:array(int|string)) host_lookup = ([]);

object/*(client)*/ get_connection (string ldap_url, void|string binddn,
				   void|string password, void|int version,
				   void|SSL.Context ctx)
//! Returns a client connection to the specified LDAP URL. If a bind
//! DN is specified (either through a @expr{"bindname"@} extension in
//! @[ldap_url] or, if there isn't one, through @[binddn]) then the
//! connection will be bound using that DN and the optional password.
//! If no bind DN is given then any connection is returned, regardless
//! of the bind DN it is using.
//!
//! @[version] may be used to specify the required protocol version in
//! the bind operation. If zero or left out, a bind attempt with the
//! default version (currently @expr{3@}) is done with a fallback to
//! @expr{2@} if that fails. Also, a cached connection for any version
//! might be returned if @[version] isn't specified.
//!
//! @[ctx] may be specified to control SSL/TLS parameters to use
//! with the @expr{"ldaps"@}-protocol. Note that changing this only
//! affects new connections.
//!
//! As opposed to creating an @[Protocols.LDAP.client] instance
//! directly, this function can return an already established
//! connection for the same URL, provided connections are given back
//! using @[return_connection] when they aren't used anymore.
//!
//! A client object with an error condition is returned if there's a
//! bind error, e.g. invalid password.
{
  string pass = password;
  password = "CENSORED";
  object/*(client)*/ conn;

  mapping(string:mixed) parsed_url = parse_ldap_url (ldap_url);
  if (string url_bindname = parsed_url->ext && parsed_url->ext->bindname)
    binddn = url_bindname;

  // Could achieve a bit better caching here by removing the bindname
  // extension that ldap_url might contain, but it's probably not
  // worth the bother.

  if (idle_conns[ldap_url]) {
    object(Thread.MutexKey)|zero lock = idle_conns_mutex->lock();
    if (array(object/*(client)*/) conns = idle_conns[ldap_url]) {
    find_connection: {
	int now = time();
	object/*(client)*/ rebind_conn;
	string md5_pass;

	for (int i = 0; i < sizeof (conns);) {
	  conn = conns[i];
	  int last_use = conn->get_last_io_time();
	  if (last_use <= now - connection_idle_timeout) {
	    DWRITE ("Dropping old connection which has been idle for %d s.\n",
		    now - last_use);
	    conns = conns[..i-1] + conns[i+1..];
	  }
	  else {
	    if (!binddn ||
		(conn->get_bound_dn() == binddn &&
		 (conn->get_bind_password_hash() ==
		  (md5_pass ||
		   (md5_pass = Crypto.MD5.hash (pass || "")))) &&
		 (!version || conn->get_protocol_version() == version))) {
	      DWRITE ("Reusing connection which has been idle for %d s.\n",
		      now - last_use);
	      conns = conns[..i-1] + conns[i+1..];
	      lock = 0;
	      break find_connection;
	    }
	    else if (!rebind_conn && conn->get_protocol_version() >= 3) {
	      DWRITE ("Found differently bound connection "
		      "which has been idle for %d s.\n", now - last_use);
	      rebind_conn = conn;
	    }
	    i++;
	  }
	}

	if (rebind_conn && sizeof (conns) >= connection_rebind_threshold) {
	  conn = rebind_conn;
	  conns -= ({rebind_conn});
	  DWRITE ("Reusing differently bound connection.\n");
	}
	else
	  conn = 0;
      }

      if (!sizeof (conns))
	m_delete (idle_conns, ldap_url);
      else
	idle_conns[ldap_url] = conns;
    }
    lock = 0;
  }

  if (!sizeof (idle_conns) && periodic_idle_conns_garb_call_out) {
    DWRITE ("No connections left in pool - disabling periodic garb.\n");
    remove_call_out (periodic_idle_conns_garb_call_out);
    periodic_idle_conns_garb_call_out = 0;
  }

  if (!conn) {
    DWRITE("Connecting to %O.\n", ldap_url);
    string host = parsed_url->host;
    if (host) {
      if (!host_lookup[host]) {
	array(string|array(string)) entry =
	  Protocols.DNS.gethostbyname(host);
	if ((sizeof(entry) >= 2) && sizeof(entry[1])) {
	  DWRITE("DNS lookup: %O.\n", entry[1]);
	  host_lookup[host] = ({ random(sizeof(entry[1])) + 1 }) + entry[1];
	} else {
	  entry = gethostbyname(host);
	  if ((sizeof(entry) >= 2) && sizeof(entry[1])) {
	    DWRITE("Hosts lookup: %O.\n", entry[1]);
	    host_lookup[host] = ({ random(sizeof(entry[1])) + 1 }) + entry[1];
	  }
	}
      }
      array(int|string) ips = host_lookup[host];
      if (ips) {
	int i = ips[0];
	for (int j = 1; j < sizeof(ips); j++) {
	  mixed err = catch {
	      DWRITE("Attempt #%d: %O.\n", j, ips[i]);
	      parsed_url->host = ips[i];
	      conn = Protocols.LDAP["client"](parsed_url, ctx);
	      ips[0] = i;
	      break;
	    };
	  if (!err) {
	    break;
	  }
	  DWRITE("Failure: %s\n", describe_error(err));
	  if (++i >= sizeof(ips)) i = 1;
	}
	parsed_url->host = host;
	if (!conn) {
	  m_delete(host_lookup, host);
	}
      }
    }
  }

  if (!conn) {
    conn = Protocols.LDAP["client"] (parsed_url, ctx);
  }

  if (!binddn)
    DWRITE ("Bound connection not requested.\n");
  else {
    if (binddn == "")
      DWRITE ("Not binding - no bind DN set.\n");
    else if (conn->get_bound_dn() == binddn &&
	     (!version || conn->get_protocol_version() == version))
      DWRITE ("Not binding - already bound to %O using version %d.\n",
	      binddn, conn->get_protocol_version());
    else {
      conn->bind (binddn, pass, version);

      int err_num = conn->error_number();
      if (err_num && !version && conn->get_protocol_version() >= 3) {
	string server_err_str = conn->server_error_string();
	conn->bind (binddn, pass, 2);
	if (conn->error_number()) {
	  DWRITE ("Bind attempt to %O using fallback version 2 failed: %s\n",
		  binddn, conn->error_string());
	  // Restore the original error which probably is more
	  // interesting when the server doesn't like LDAPv2.
	  conn->seterr (err_num, server_err_str);
	}
      }
#ifdef DEBUG_PIKE_PROTOCOL_LDAP
      if (conn->error_number())
	DWRITE ("Bind attempt to %O failed: %s\n",
		binddn, conn->error_string());
      else
	DWRITE ("Bind to %O successful using version %d.\n",
		binddn, conn->get_protocol_version());
#endif
    }
  }

  return conn;
}

void return_connection (object/*(client)*/ conn)
//! Use this to return a connection to the connection pool after
//! you've finished using it. The connection is assumed to be working.
//!
//! @note
//!   Ensure that persistent connection settings such as the scope and
//!   the base DN are restored to the defaults
{
  Thread.MutexKey lock = idle_conns_mutex->lock();
  DWRITE ("Returning connection to %O to pool.\n", conn->get_parsed_url()->url);

  // Reset the connection now to remove the cyclic ref between the
  // client object and its last result object.
  conn->reset_options();
  idle_conns[conn->get_parsed_url()->url] += ({conn});

  if (!periodic_idle_conns_garb_call_out)
    periodic_idle_conns_garb_call_out =
      call_out (periodic_idle_conns_garb, connection_idle_garb_interval);
}

int num_connections (string ldap_url)
//! Returns the number of currently stored connections for the given
//! LDAP URL.
{
  return sizeof (idle_conns[ldap_url] || ({}));
}
