#pike __REAL_VERSION__

// LDAP client protocol implementation for Pike.
//
// $Id: client.pike,v 1.59 2004/06/18 15:16:32 grubba Exp $
//
// Honza Petrous, hop@unibase.cz
//
// ----------------------------------------------------------------------
//
// History:
//
//	v0.0  1998-05-25 Starting up!
//	v1.0  1998-06-21 Core functions (open, bind, unbind, delete, add,
//			 compare, search), only V2 operations,
//			 some additional restrictions
//	v1.1u 1998-07-03 Unfinished version (for testing)
//	v1.2a 1998-07-14 New beta code (I love jump over versions like Roxen ;)
//			 - added asn1_enumerated, asn1_boolean, asn1_nulllist
//			   and asn1_application_null
//			 - search op: corrected ASN1 type
//			 - unbind op: corrected ASN1 type
//	v1.2d 1998-08    - added logging of bad LDAP PDU in readmsg
//	v1.3  1998-09-18 - resolved "null attribute list" bug  in search
//			   (Now is compatible with MS Exchange ;-)
//	v1.9  1999-03	 - redesign of LDAP.pmod
//			 !!! Library uses new ASN1 API => uncompatible !!!
//			 !!! with Pike 0.5 (needed Pike 0.6 or higher) !!!
//			 - sliced library code to the several files:
//			   * LDAP.pmod/protocol.pike
//			     (low level code)
//			   * LDAP.pmod/client.pike
//			     (main code)
//			   * LDAP.pmod/ldap_errors.h
//			     (error array)
//			   * LDAP.pmod/ldap_globals.h
//			     (global defininitions)
//			   * LDAP.pmod/ldap_privates.pmod
//			     (ASN.1 LDAP-related classes and hacks)
//			 - changed default of 'ldap_scope' to 0
//			 - search filter now correctly processed '\(' & '\)'
//			   [! Still unimplemented escaped conditions chars!]
//
//	v1.10 1999-03-28 - moved core to the new 'protocol' code
//	      1999-03-28 - rewritten ldap_[op] startup code
//
//	v1.11 1999-04-10 - search filter now processed multiple wild '*' chars
//			   [ Escaping untested, yet ]
//	v1.13 2000-02-12 - fixed search NOT op bug (end revision normalized)
//
//	v1.14 2000-02-17 - added decoding of UTF8 strings for v3 protocol
//
//	newer versions - see CVS at roxen.com (hop)
//
//			 - corrected deUTF8 values in result
//			 -
//
// Specifications:
//
//	RFC 1558			  (search filter representations)
//	RFC 1777,1778,1779		  (version2 spec)
//	RFC 1823			  (v2 API)
//	RFC 2251,2252,2253,2254,2255,2256 (version3 spec)
//	draft-ietf-asid-ldap-c-api-00.txt (v3 API)
//	RFC 2279   			  (UTF-8)
//	RFC 2696			  (paged requests)
//
//	Interesting, applicable
//	RFC 2307   (LDAP as network information services; draft?)


#if constant(.ldap_privates)

#include "ldap_globals.h"

#include "ldap_errors.h"

#if constant(SSL.Cipher.CipherAlgorithm)
import SSL.Constants;
#endif

// ------------------------

// ASN.1 decode macros
#define ASN1_DECODE_RESULTAPP(X)	(.ldap_privates.ldap_der_decode(X)->elements[1]->get_tag())
#define ASN1_DECODE_RESULTCODE(X)	(int)(.ldap_privates.ldap_der_decode(X)->elements[1]->elements[0]->value->cast_to_int())
#define ASN1_DECODE_RESULTSTRING(X)	(.ldap_privates.ldap_der_decode(X)->elements[1]->elements[2]->value)
#define ASN1_DECODE_RESULTREFS(X)	(.ldap_privates.ldap_der_decode(X)->elements[1]->elements[3]->elements)
#define ASN1_DECODE_ENTRIES(X)		_New_decode(X)
#define ASN1_DECODE_DN(X)		(string)((X)->elements[0]->value)
#define ASN1_DECODE_RAWDEBUG(X)		(.ldap_privates.ldap_der_decode(X)->debug_string())
#define ASN1_GET_ATTR_ARRAY(X)		(array)((X)->elements[1]->elements)
#define ASN1_GET_ATTR_NAME(X)		((X)->elements[0]->value)

 //! Contains the client implementation of the LDAP protocol.
 //! All of the version 2 protocol features are implemented
 //! but only the base parts of the version 3. 

  inherit .protocol;

  private {
    int binded = 0;	// flag for v2 operations
    string ldap_basedn = "";	// baseDN
    int ldap_scope = 0;		// 0: base, 1: onelevel, 2: subtree
    int ldap_deref = 0;		// 0: ...
    int ldap_sizelimit = 0;
    int ldap_timelimit = 0;
    mapping lauth = ([]);
    object last_rv = 0; // last returned value
  }

  //! Contains the result of a LDAP search.
  //!
  //! @seealso
  //!  @[LDAP.client.search], @[LDAP.client.result.fetch]
  //!
  class result // ------------------
  {

    private int resultcode = LDAP_SUCCESS;
    private string resultstring;
    private int entrycnt = 0;
    private int actnum = 0;
    private array(mapping(string:array(string))) entry = ({});
    array(string) referrals;

    private string utf2s(string in) {
    // catched variant of utf8_to_string needed for tagged octet string data

      string out = "";
      catch( out = utf8_to_string(in) );
      return out;

    }

    private array _get_attr_values(int ver, object x) {

      array res = ({});

      if(!sizeof(x->elements))
	return res;
      foreach(x->elements[1]->elements, object val1) 
	res += ({ val1->value });
      if(ver == 3) {
        // deUTF8
        res = Array.map(res, utf2s);
      }
      return res;
    }

    private array _New_decode(array ar) {

      array res = ({});
      array entry1;
      mapping attrs;
      object oder;

      foreach(ar, string raw1)  {
	oder = (.ldap_privates.ldap_der_decode(raw1)->elements[1]);
	attrs = (["dn":({ASN1_DECODE_DN(oder)})]);
	if(catch(entry1 = ASN1_GET_ATTR_ARRAY(oder))) continue;
	foreach(entry1, object attr1) {
	  attrs += ([ASN1_GET_ATTR_NAME(attr1):_get_attr_values(ldap_version, attr1)]);
	}
	res += ({attrs});
      }

      return res;
    } // _New_decode

    //!
    //! You can't create instances of this object yourself.
    //! The only way to create it is via a search of a LDAP server.
    object|int create(array rawres, int|void stuff) {
    // rawres: array of result in raw format, but WITHOUT LDAP PDU !!!
    // stuff: 1=bind result; ...

      int lastel = sizeof(rawres) - 1;

      if (lastel < 0) {
        global::seterr (LDAP_LOCAL_ERROR);
        THROW(({"LDAP: Internal error.\n",backtrace()}));
        return -global::ldap_errno;
      }
      DWRITE(sprintf("result.create: rawres=%O\n",rawres[lastel]));

      // The last element of 'rawres' is result itself
      resultcode = ASN1_DECODE_RESULTCODE(rawres[lastel]);
      DWRITE(sprintf("result.create: code=%d\n",resultcode));
      resultstring = ASN1_DECODE_RESULTSTRING(rawres[lastel]);
      DWRITE(sprintf("result.create: str=%s\n",resultstring));
#ifdef V3_REFERRALS
      // referral (v3 mode)
      if(resultcode == 10) {
        referrals = ({});
        foreach(ASN1_DECODE_RESULTREFS(rawres[lastel]), object ref1)
	  referrals += ({ ref1->value });
        DWRITE(sprintf("result.create: refs=%O\n",referrals));
      }
#endif
      DWRITE(sprintf("result.create: elements=%d\n",lastel+1));
      if (lastel) { // Have we any entry?
        entry = ASN1_DECODE_ENTRIES(rawres[..lastel-1]);
        entrycnt = sizeof(entry); //num_entries();
      }

#if 0
      // Context specific proccessing of 'rawres'
      switch(stuff) {
        case 1:	DWRITE("result.create: stuff=1\n");
		break;
        default:	DWRITE(sprintf("result.create: stuff=%d\n", stuff));

      }
#endif

      return this;

    }

    //!
    //! Returns error number of search result.
    //!
    //! @seealso
    //!  @[LDAP.client.result.error_string]
    int error_number() { return resultcode; }

    //!
    //! Returns error description of search result.
    //!
    //! @seealso
    //!  @[LDAP.client.result.error_number]
    string error_string() {
      return ((stringp(resultstring) && sizeof(resultstring)) ?
	      resultstring : ldap_errlist[resultcode]);
    }

    //!
    //! Returns the number of entries.
    //!
    //! @seealso
    //!  @[LDAP.client.result.count_entries]
    int num_entries() { return entrycnt; }

    //!
    //! Returns the number of entries from current cursor
    //! possition till end of the list.
    //!
    //! @seealso
    //!  @[LDAP.client.result.first], @[LDAP.client.result.next]
    int count_entries() { return entrycnt - actnum; }

    //! Returns a mapping with an entry for each attribute.
    //! Each entry is an array of values of the attribute.
    //!
    //! @param index
    //!  Optional argument can be used for direct access
    //!  to the entry other then currently pointed by cursor.
    int|mapping(string:array(string)) fetch(int|void idx) {

      if (!idx)
	idx = actnum + 1;
      if ((idx <= num_entries()) && (idx > 0)) {
	actnum = idx - 1;
        return entry[actnum];
      }
      return 0;
    }

    //!
    //! Returns distinguished name (DN) of the current entry
    //! in the result list. Notice that this is the same
    //! as fetch()->dn[0].
    string get_dn() { return fetch()["dn"][0]; }

    //!
    //! Initialized the result cursor to the first entry
    //! in the result list.
    //!
    //! @seealso
    //!  @[LDAP.client.result.next]
    void first() { actnum = 0; }

    //!
    //! Moves the result cursor to the next entry
    //! in the result list. Returns number of remained entries
    //! in the result list. Returns 0 at the end.
    //!
    //! @seealso
    //!  @[LDAP.client.result.next]
    int next() {
      if (actnum < (num_entries()-1)) {
	actnum++;
	return count_entries();
      }
      return 0;
    }

  } // end of class 'result' ---------------

  // helper functions and macros

#ifdef ENABLE_PAGED_SEARCH
#define IF_ELSE_PAGED_SEARCH(X,Y)	X
#else /* !ENABLE_PAGED_SEARCH */
#define IF_ELSE_PAGED_SEARCH(X,Y)	Y
#endif

#ifdef LDAP_PROTOCOL_PROFILE
#define PROFILE(STR, CODE) DWRITE_PROF(STR + ": %O\n", gauge {CODE;})
#else
#define PROFILE(STR, CODE) do { CODE; } while(0)
#endif

  private int chk_ver() {

    if ((ldap_version != 2) && (ldap_version != 3)) {
      seterr (LDAP_PROTOCOL_ERROR);
      THROW(({"LDAP: Unknown/unsupported protocol version.\n",backtrace()}));
      return -ldap_errno;
    }
    return 0;
  }

  private int chk_binded() {
  // For version 2: we must be 'binded' first !!!

    if ((ldap_version == 2) && !binded) {
      seterr (LDAP_PROTOCOL_ERROR);
      THROW(({"LDAP: Must binded first.\n",backtrace()}));
      return -ldap_errno;
    }
    if ((ldap_version == 3) && !binded)
      bind();
    return 0;
  }

  private int chk_dn(string dn) {

    if ((!dn) || (!sizeof(dn))) {
      seterr (LDAP_INVALID_DN_SYNTAX);
      THROW(({"LDAP: Invalid DN syntax.\n",backtrace()}));
      return -ldap_errno;
    }
    return 0;
  }

  //! Several information about code itself and about active connection too
  mapping info;

  //! @decl void create()
  //! @decl void create(string url)
  //! @decl void create(string url, object context)
  //!
  //! Create object. The first optional argument can be used later
  //! for subsequence operations. The second one can specify
  //! TLS context of connection. The default context only allows
  //! 128-bit encryption methods, so you may need to provide your
  //! own context if your LDAP server supports only export encryption.
  //!
  //! @param url
  //!  LDAP server URL in form
  //!    @expr{"ldap://hostname/basedn?attrlist?scope?ext"@}
  //!
  //! @param context
  //!  TLS context of connection
  //!
  //! @seealso
  //!  @[LDAP.client.bind], @[LDAP.client.search]
  void create(string|void url, object|void context)
  {

    info = ([ "code_revision" : ("$Revision: 1.59 $"/" ")[1] ]);

    if(!url || !sizeof(url))
      url = LDAP_DEFAULT_URL;

    lauth = parse_url(url);

    if(!stringp(lauth->scheme) ||
       ((lauth->scheme != "ldap")
#if constant(SSL.Cipher.CipherAlgorithm)
	&& (lauth->scheme != "ldaps")
#endif
	)) {
      THROW(({"Unknown scheme in server URL.\n",backtrace()}));
    }

    if(!lauth->host)
      lauth += ([ "host" : LDAP_DEFAULT_HOST ]);
    if(!lauth->port)
      lauth += ([ "port" : lauth->scheme == "ldap" ? LDAP_DEFAULT_PORT : LDAPS_DEFAULT_PORT ]);

#if constant(SSL.Cipher.CipherAlgorithm)
    if(lauth->scheme == "ldaps" && !context) {
      context = SSL.context();
      // Allow only strong crypto
      context->preferred_suites = ({
	SSL_rsa_with_idea_cbc_sha,
	SSL_rsa_with_rc4_128_sha,
	SSL_rsa_with_rc4_128_md5,
	SSL_rsa_with_3des_ede_cbc_sha,
      });
    }
#endif
 
    if(!(::connect(lauth->host, lauth->port))) {
      //errno = ldapfd->errno();
      seterr (LDAP_SERVER_DOWN);
      DWRITE("client.create: ERROR: can't open socket.\n");
      //ldapfd->destroy();
      //ldap=0;
      //ok = 0;
      //if(con_fail)
      //  con_fail(this, @extra_args);
      THROW(({"Failed to connect to LDAP server.\n",backtrace()}));
    }

#if constant(SSL.Cipher.CipherAlgorithm)
    if(lauth->scheme == "ldaps") {
      context->random = Crypto.Random.random_string;
      ::create(SSL.sslfile(this, context, 1,1));
      info->tls_version = ldapfd->version;
    } else
      ::create(::_fd);
#else
    if(lauth->scheme == "ldaps") {
	THROW(({"LDAP: LDAPS is not available without SSL support.\n",backtrace()}));
    }
    else
      ::create(::_fd);
#endif

    DWRITE("client.create: connected!\n");

    DWRITE(sprintf("client.create: remote = %s\n", query_address()));
    DWRITE_HI("client.OPEN: " + lauth->host + ":" + (string)(lauth->port) + " - OK\n");

    binded = 0;

    if(lauth->scope)
      set_scope(lauth->scope);
    if(lauth->basedn)
      set_basedn(lauth->basedn);

  } // create

  private mixed send_bind_op(string name, string password) {
  // Simple BIND operation

    object msgval, vers, namedn, auth, app;
    string pass = password;
    password = "censored";

    vers = Standards.ASN1.Types.asn1_integer(ldap_version);
    namedn = Standards.ASN1.Types.asn1_octet_string(name);
    auth = ASN1_CONTEXT_OCTET_STRING(0, pass);
    // SASL credentials ommited

    msgval = ASN1_APPLICATION_SEQUENCE(0, ({vers, namedn, auth}));

    return do_op(msgval);
  }

  //! @decl int bind()
  //! @decl int bind(string dn, string password)
  //! @decl int bind(string dn, string password, int version)
  //!
  //! Authenticates connection to the direcory.
  //!
  //! First form uses default value previously entered in create.
  //!
  //! Second form uses value from parameters:
  //!
  //! @param dn
  //!  The distinguished name (DN) of an entry aginst which will
  //!  be made authentication.
  //! @param password
  //!  Password used for authentication.
  //!
  //! Third form allows specify the version of LDAP protocol used
  //! by connection to the LDAP server.
  //!
  //! @param version
  //!  Only @expr{2@} or @expr{3@} can be entered.
  //!
  //! @returns
  //!  Returns @expr{1@} on success, @expr{0@} otherwise.
  //!
  //! @note
  //!  Only simple authentication type is implemented. So be warned
  //!  clear text passwords are sent to the directory server.
  //!
  //! @note
  //!   The API change: the returning code was changed in Pike 7.3+
  //!	to follow his logic better.
  int bind (string|void dn, string|void password, int|void version) {

    int id;
    mixed raw;
    string pass = password;
    password = "censored";

    if (!version)
      version = LDAP_DEFAULT_VERSION;
    if (chk_ver())
      return 0;
    if (!stringp(dn))
      dn = mappingp(lauth->ext) ? lauth->ext->bindname||"" : "";
    if (!stringp(pass))
      pass = "";
    ldap_version = version;
    if(ldap_version == 3) {
      dn = string_to_utf8(dn);
      pass = string_to_utf8(pass);
    }
    if(intp(raw = send_bind_op(dn, pass))) {
      THROW(({error_string()+"\n",backtrace()}));
      return -ldap_errno;
    }

   binded = 0;
   last_rv = result(({raw}),1);
   if (!last_rv->error_number())
     binded = 1;
   DWRITE_HI(sprintf("client.BIND: %s\n", last_rv->error_string()));
   seterr (last_rv->error_number());
   return binded;

  } // bind

  private int send_unbind_op() {
  // UNBIND operation

    writemsg(ASN1_APPLICATION_OCTET_STRING(2, ""));

    //ldap::close();

    return 1;
  }

  void destroy() {

    //send_unbind_op();
    destruct(this);
  }

  //!
  //! Unbinds from the directory and close the connection.
  int unbind () {

    if (send_unbind_op() < 1) {
      THROW(({error_string()+"\n",backtrace()}));
      return -ldap_errno;
    }
    binded = 0;
    DWRITE_HI("client.UNBIND: OK\n");

  } // unbind

  private int|string send_op_withdn(int op, string dn) {
  // DELETE, ...

    return do_op(ASN1_APPLICATION_OCTET_STRING(op, dn));

  }

  //!
  //! Deletes entry from the LDAP server.
  //!
  //! @param dn
  //!  The distinguished name of deleted entry.
  //!
  //! @returns
  //!  Returns @expr{1@} on success, @expr{0@} otherwise.
  //!
  //! @note
  //!   The API change: the returning code was changed in Pike 7.3+
  //!	to follow his logic better.
  int delete (string dn) {

    int id;
    mixed raw;

    if (chk_ver())
      return 0;
    if (chk_binded())
      return 0;
    if (chk_dn(dn))
      return 0;
    if(ldap_version == 3) {
      dn = string_to_utf8(dn);
    }
    if(intp(raw = send_op_withdn(10, dn))) {
      THROW(({error_string()+"\n",backtrace()}));
      return 0;
    }

   last_rv = result(({raw}));
   DWRITE_HI(sprintf("client.DELETE: %s\n", last_rv->error_string()));
   seterr(last_rv->error_number());
   return !last_rv->error_number();

  } // delete

  private int|string send_compare_op(string dn, array(string) aval) {
  // COMPARE

    object msgval;

    msgval = ASN1_APPLICATION_SEQUENCE(14, 
		({ Standards.ASN1.Types.asn1_octet_string(dn),
		Standards.ASN1.Types.asn1_sequence(
		  ({ Standards.ASN1.Types.asn1_octet_string(aval[0]),
		  Standards.ASN1.Types.asn1_octet_string(aval[1])
		  }))
		})
	     );

    return do_op(msgval);
  }


  //!
  //! Compares given attribute value with one in the directory.
  //!
  //! @param dn
  //!  The distinguished name of compared entry.
  //!
  //! @param aval
  //!  The mapping of compared attributes and theirs values.
  //!
  //! @returns
  //!  Returns @expr{1@} on success, @expr{0@} otherwise.
  //!
  //! @note
  //!   The API change: the returning code was changed in Pike 7.3+
  //!	to follow his logic better.
  int compare (string dn, array(string) aval) {

    int id;
    mixed raw;

    // if (!aval || sizeof(aval)<2)
    //  error
    if (chk_ver())
      return 0;
    if (chk_binded())
      return 0;
    if (chk_dn(dn))
      return 0;
    if(ldap_version == 3) {
      dn = string_to_utf8(dn);
      aval = Array.map(aval, string_to_utf8);
    }
    if(intp(raw = send_compare_op(dn, aval))) {
      THROW(({error_string()+"\n",backtrace()}));
      return 0;
    }

   last_rv = result(({raw}));
   DWRITE_HI(sprintf("client.COMPARE: %s\n", last_rv->error_string()));
   seterr(last_rv->error_number());
   return !last_rv->error_number();

  } // compare

  private int|string send_add_op(string dn, mapping(string:array(string)) attrs) {
  // ADD

    object msgval;
    string atype;
    array(object) oatt = ({});

    foreach(indices(attrs), atype) {
      string aval;
      array(object) ohlp = ({});

      foreach(values(attrs[atype]), aval)
	ohlp += ({Standards.ASN1.Types.asn1_octet_string(aval)});
      oatt += ({Standards.ASN1.Types.asn1_sequence(
		({Standards.ASN1.Types.asn1_octet_string(atype),
		  Standards.ASN1.Types.asn1_set(ohlp)
		}))
	      });
    }

    msgval = ASN1_APPLICATION_SEQUENCE(8, 
		({Standards.ASN1.Types.asn1_octet_string(dn),
		  Standards.ASN1.Types.asn1_sequence(oatt)
		}));

    return do_op(msgval);
  }


  //!  The Add Operation allows a client to request the addition
  //!  of an entry into the directory
  //!
  //! @param dn
  //!  The Distinguished Name of the entry to be added.
  //!
  //! @param attrs
  //!  The mapping of attributes and their values that make up the content
  //!  of the entry being added.
  //!
  //! @returns
  //!  Returns @expr{1@} on success, @expr{0@} otherwise.
  //!    
  //! @note
  //!   The API change: the returning code was changed in Pike 7.3+
  //!	to follow his logic better.
  int add (string dn, mapping(string:array(string)) attrs) {

    int id;
    mixed raw;

    if (chk_ver())
      return 0;
    if (chk_binded())
      return 0;
    if (chk_dn(dn))
      return 0;
    if(ldap_version == 3) {
      dn = string_to_utf8(dn);
      array(string) keys = indices(attrs);
      array(array(string)) vals = values(attrs);
      attrs = mkmapping(Array.map(keys, string_to_utf8),
			Array.map(vals, Array.map, string_to_utf8));
    }
    if(intp(raw = send_add_op(dn, attrs))) {
      THROW(({error_string()+"\n",backtrace()}));
      return 0;
    }

    last_rv = result(({raw}));
    DWRITE_HI(sprintf("client.ADD: %s\n", last_rv->error_string()));
    seterr(last_rv->error_number());
    return !last_rv->error_number();

  } // add

  private static array(string) filter_get_sub1expr(string fstr) {
  // returns one-level brackets enclosed expressions

    array(string) rvarr = ({});
    int leftflg = 0, nskip = 0;

    for(int ix=0; ix<sizeof(fstr); ix++)
      if((fstr[ix] == '(') && (!ix || (fstr[ix-1] != '\\'))) {
	leftflg = ix+1;
	while(++ix < sizeof(fstr)) {
	  if((fstr[ix] == '(') && (fstr[ix-1] != '\\')) {
	    nskip++;		// deeper expr.
	    continue;
	  }
	  if((fstr[ix] == ')') && (fstr[ix-1] != '\\'))
	    if(nskip) {		// we are deeply?
	      nskip--;
	      continue;
	    } else {		// ok, here is end of expr.
	      rvarr += ({fstr[leftflg..(ix-1)]});
	      leftflg = 0;
	      break;
	    }
	} // while
      }
    if(leftflg) {
      ; // silent error: Missed right-enclosed bracket !
    }
    //DWRITE(sprintf("client.sub1: arr=%O\n",rvarr));
    return rvarr;
  }

  /*private*/ object make_simple_filter(string filter) {
  // filter expression parser - only simple expressions!

    object rv;
    int op, ix;

    DWRITE(sprintf("client.make_simple_filter: filter: [%s]\n", filter));
    if((op = predef::search(filter, ">=")) > 0)	{ // greater or equal
      DWRITE("client.make_simple_filter: [>=]\n");
      return ASN1_CONTEXT_SEQUENCE(5,
		({Standards.ASN1.Types.asn1_octet_string(filter[..(op-1)]),
		  Standards.ASN1.Types.asn1_octet_string(filter[(op+2)..])
		}));
    }
    if((op = predef::search(filter, "<=")) > 0)	{ // less or equal
      DWRITE("client.make_simple_filter: [<=]\n");
      return ASN1_CONTEXT_SEQUENCE(6,
		({Standards.ASN1.Types.asn1_octet_string(filter[..(op-1)]),
		  Standards.ASN1.Types.asn1_octet_string(filter[(op+2)..])
		}));
    }
    if((op = predef::search(filter, "=")) > 0) {	// equal, substring
	if((filter[-2] == '=') && (filter[-1] == '*'))  // any (=*)
	  return make_simple_filter(filter[..op-1]);
	ix = predef::search(filter[(op+1)..], "*");
	if ((ix != -1) && (filter[op+ix] != '\\')) {	// substring
	    object ohlp;
	    array oarr = ({}), ahlp = ({});
	    array filtval = filter[(op+1)..] / "*";

	    // escape processing
	    for(int cnt = 0; cnt < sizeof(filtval); cnt++) {
		if(cnt) {
		    if(sizeof(filtval[cnt-1]) && filtval[cnt-1][-1] == '\\')
			ahlp[-1] = reverse(reverse(ahlp[-1])[1..]) + filtval[cnt];
		    else
			ahlp += ({ filtval[cnt] });
		} else
		    ahlp = ({ filtval[cnt] });
	    } // for

	    // filter elements processing (left, center & right)
	    ix = sizeof(ahlp);
	    for (int cnt = 0; cnt < ix; cnt++)
		if(!cnt) {	// leftmost element
		    if(sizeof(ahlp[0]))
			oarr = ({ASN1_CONTEXT_OCTET_STRING(0, ahlp[0])});
		} else
		    if(cnt == ix-1) {	// rightmost element
			if(sizeof(ahlp[ix-1]))
			    oarr += ({ASN1_CONTEXT_OCTET_STRING(2, ahlp[ix-1])});
		    } else {	// inside element
			if(sizeof(ahlp[cnt]))
			    oarr += ({ASN1_CONTEXT_OCTET_STRING(1, ahlp[cnt])});
		    }
	    // for

	    DWRITE(sprintf("client.make_simple_filter: substring: [%s]:\n%O\n", filter, ahlp));
	    return ASN1_CONTEXT_SEQUENCE(4,
		({Standards.ASN1.Types.asn1_octet_string(filter[..(op-1)]),
		  Standards.ASN1.Types.asn1_sequence(oarr)
		}));
      } else {				   // equal
        DWRITE("client.make_simple_filter: [=]\n");
	return ASN1_CONTEXT_SEQUENCE(3,
		({Standards.ASN1.Types.asn1_octet_string(filter[..(op-1)]),
		  Standards.ASN1.Types.asn1_octet_string(filter[(op+1)..])
		}));
      }
    } // if equal,substring
    // present
    DWRITE("client.make_simple_filter: [present]\n");
    return ASN1_CONTEXT_OCTET_STRING(7, filter);
}


  /*private*/ object|int make_filter(string filter) {
  // filter expression parser

    object ohlp;
    array(object) oarr = ({});
    int op ;

    DWRITE("client.make_filter: filter=["+filter+"]\n");

    if (!sizeof(filter)) return make_simple_filter(filter);

    // strip leading and trailing spaces
    filter = String.trim_all_whites(filter);

    // strip leading and trailing brackets
#if 1
    if(filter[0] == '(') {
      int ix;
      string f2 = reverse(filter[1..]);
      if((ix = predef::search(f2, ")")) > -1) {
	filter = reverse(f2[ix+1..]);
	return make_filter(filter);
      }
      return -1; // error in filter expr.
    }
#endif

    op = -1;

    DWRITE(sprintf("client.make_filter: ftype=%c\n",filter[0]));
    switch (filter[0]) {
      case '&':		// and
      case '|':		// or
	      foreach(filter_get_sub1expr(filter[1..]), string sub1expr)
		if (objectp(ohlp = make_filter(sub1expr)))
		  oarr += ({ohlp});
		else
		  return 0; // error: Filter parameter error!
      	      DWRITE(sprintf("client.make_filter: expr_cnt=%d\n",sizeof(oarr)));
	      //ohlp = Standards.ASN1.Encode.asn1_set(@oarr);
	      op = 0;
	      if (filter[0] == '|')
		op = 1;
	      return ASN1_CONTEXT_SET(op, oarr);
      case '!':		// not
	      if (objectp(ohlp = make_filter(filter_get_sub1expr(filter[1..])[0])))
	        return ASN1_CONTEXT_SEQUENCE(2, ({ ohlp}) );
	      else
		return 0; // error: Filter parameter error!
	      break;
      default :		// we assume simple filter
	      return make_simple_filter(filter);
    }
}

  private object|int make_search_op(string basedn, int scope, int deref,
				    int sizelimit, int timelimit,
				    int attrsonly, string filter,
				    void|array(string) attrs)
  {
    // SEARCH
  // limitations: !!! sizelimit and timelimit should be unsigned int !!!

    object msgval, ofilt;
    array(object) ohlp;

    if(!objectp(ofilt = make_filter(filter))) {
      return -seterr(LDAP_FILTER_ERROR);
    }
    ohlp = ({ofilt});
    if (arrayp(attrs)) { //explicitly defined attributes
      array(object) o2 = ({});
      foreach(attrs, string s2)
	o2 += ({Standards.ASN1.Types.asn1_octet_string(s2)});
      ohlp += ({Standards.ASN1.Types.asn1_sequence(o2)});
    } else
      ohlp += ({Standards.ASN1.Types.asn1_sequence(({}))});

    return ASN1_APPLICATION_SEQUENCE(3,
		({ Standards.ASN1.Types.asn1_octet_string(basedn),
		   ASN1_ENUMERATED(scope),
		   ASN1_ENUMERATED(deref),
		   Standards.ASN1.Types.asn1_integer(sizelimit),
		   Standards.ASN1.Types.asn1_integer(timelimit),
		   ASN1_BOOLEAN(attrsonly ? -1 : 0),
		   @ohlp
		})) ;
  }

IF_ELSE_PAGED_SEARCH(static multiset(string) supported_controls;,)

  //! Search LDAP directory.
  //!
  //! @param filter
  //!   Search filter used when searching directory objects.
  //!
  //! @param attrs
  //!   The array of attribute names which will be returned by server.
  //!   for every entry.
  //!
  //! @param attrsonly
  //!   The flag causes server return only attribute name but not
  //!   the attribute values.
  //!
  //! @returns
  //!   Returns object @[LDAP.client.result] on success, @expr{0@}
  //!	otherwise.
  //!
  //! @note
  //!   For syntax of search filter see at RFC 1960
  //!   (http://community.roxen.com/developers/idocs/rfc/rfc1960.html).
  //!    
  //! @note
  //!   The API change: the returning code was changed in Pike 7.3+
  //!	to follow his logic better.
  //!
  //! @seealso
  //!  @[LDAP.client.result], @[LDAP.client.result.fetch]
  object|int search (string|void filter, array(string)|void attrs,
  		     int|void attrsonly) {

    int id,nv;
    mixed raw;
    array(string) rawarr = ({});

    filter=filter||lauth->filter; // default from LDAP URI

    DWRITE_HI("client.SEARCH: " + (string)filter + "\n");
    if (chk_ver())
      return 0;
    if (chk_binded())
      return 0;
    if(ldap_version == 3) {
      filter = string_to_utf8(filter);
    }

    object|int search_request;

    IF_ELSE_PAGED_SEARCH({
	if (!supported_controls) {
	  // We need to find out if controls are supported.
	  PROFILE("supported_controls", {
	      supported_controls = (<>);
	      search_request =
		make_search_op("", 0, 0, 0, 0, 0,
			       "(objectClass=*)", ({"supportedControl"}));
	      //werror("search_request: %O\n", search_request);
	      if(intp(raw = do_op(search_request))) {
		THROW(({error_string()+"\n",backtrace()}));
		return 0;
	      }
	      do {
		object res = .ldap_privates.ldap_der_decode(raw);
		if (res->elements[1]->get_tag() == 5) break;
		//werror("res: %O\n", res);
		foreach(res->elements[1]->elements[1]->elements, object attr) {
		  if (attr->elements[0]->value == "supportedControl") {
		    supported_controls |= (<
		      @(attr->elements[1]->elements->value)
		      >);
		    //werror("supported_controls: %O\n", supported_controls);
		  }
		}
		if (intp(raw = readmsg(id))) {
		  THROW(({error_string()+"\n",backtrace()}));
		  return 0;
		}
	      } while (0);
	    });
	}
      },);

    search_request =
      make_search_op(ldap_basedn, ldap_scope, ldap_deref,
	ldap_sizelimit, ldap_timelimit, attrsonly, filter,
	attrs||lauth->attributes);

    if(intp(search_request)) {
      THROW(({error_string()+"\n",backtrace()}));
      return 0;
    }

    object cookie = Standards.ASN1.Types.asn1_octet_string("");
    rawarr = ({});
    do {
      PROFILE("send_search_op", {
	  IF_ELSE_PAGED_SEARCH(
            array ctrls = ({});
	    if (supported_controls["1.2.840.113556.1.4.1339"]) {
	      // LDAP_SERVER_DOMAIN_SCOPE_OID
	      // "Tells server not to generate referrals" (NtLdap.h)
	      ctrls += ({
		Standards.ASN1.Types.asn1_sequence(({
						// controlType
		  Standards.ASN1.Types.asn1_octet_string("1.2.840.113556.1.4.1339"),
		  ASN1_BOOLEAN(0),		// criticality (FALSE)
						// controlValue
		  Standards.ASN1.Types.asn1_octet_string(""),
		  })),
	      });
	    }
	    if (supported_controls["1.2.840.113556.1.4.319"]) {
	      // LDAP Control Extension for Simple Paged Results Manipulation
	      // RFC 2696.
	      ctrls += ({
		Standards.ASN1.Types.asn1_sequence(({
						// controlType
		  Standards.ASN1.Types.asn1_octet_string("1.2.840.113556.1.4.319"),
		  ASN1_BOOLEAN(sizeof(cookie->value)?0:0xff),	// criticality
						// controlValue
		  Standards.ASN1.Types.asn1_octet_string(
		    Standards.ASN1.Types.asn1_sequence(({
						// size
		      Standards.ASN1.Types.asn1_integer(0x7fffffff),
		      cookie,			// cookie
		    }))->get_der()),
		  })),
	      });
	    }
	    object controls;
	    if (sizeof(ctrls)) {
	      controls = .ldap_privates.asn1_sequence(0, ctrls);
	    }
	    ,);

	  if(intp(raw = do_op(search_request,
			      IF_ELSE_PAGED_SEARCH(controls, 0)))) {
	    THROW(({error_string()+"\n",backtrace()}));
	    return 0;
	  }
	});

      PROFILE("rawarr++", {
	  rawarr += ({raw});
	  while (ASN1_DECODE_RESULTAPP(raw) != 5) {
	    PROFILE("readmsg", raw = readmsg(id));
	    if (intp(raw)) {
	      THROW(({error_string()+"\n",backtrace()}));
	      return 0;
	    }
	    rawarr += ({raw});
	  } // while
	});

      // At this point @[raw] contains a SearchResultDone.
      cookie = 0;
      IF_ELSE_PAGED_SEARCH({
	  if ((ASN1_DECODE_RESULTCODE(raw) != 10) &&
	      (sizeof(.ldap_privates.ldap_der_decode(raw)->elements) > 2)) {
	    object controls = .ldap_privates.ldap_der_decode(raw)->elements[2];
	    foreach(controls->elements, object control) {
	      if (!control->constructed ||
		  !sizeof(control) ||
		  control->elements[0]->type_name != "OCTET STRING") {
		//werror("Protocol error in control %O\n", control);
		// FIXME: Fail?
		continue;
	      }
	      if (control->elements[0]->value != "1.2.840.113556.1.4.319") {
		//werror("Unknown control %O\n", control->elements[0]->value);
		// FIXME: Should look at criticallity flag.
		continue;
	      }
	      if (sizeof(control) == 1) continue;
	      int pos = 1;
	      if (control->elements[1]->type_name == "BOOLEAN") {
		if (sizeof(control) == 2) continue;
		pos = 2;
	      }
	      if (control->elements[pos]->type_name != "OCTET STRING") {
		// FIXME: Error?
		continue;
	      }
	      object control_info =
		.ldap_privates.ldap_der_decode(control->elements[pos]->value);
	      if (!control_info->constructed ||
		  sizeof(control_info) < 2 ||
		  control_info->elements[1]->type_name != "OCTET STRING") {
		// Unexpected control information.
		continue;
	      }
	      if (sizeof(control_info->elements[1]->value)) {
		cookie = control_info->elements[1];
	      }
	    }
	    if (cookie) {
	      // Remove the extra end marker.
	      rawarr = rawarr[..sizeof(rawarr)-2];
	    }
	  }
	    
	},);
    } while (cookie);

    PROFILE("result", last_rv = result(rawarr));
    if(objectp(last_rv))
      seterr (last_rv->error_number());
    //if (rv->error_number() || !rv->num_entries())	// if error or entries=0
    //  rv = rv->error_number();

    DWRITE_HI(sprintf("client.SEARCH: %s (entries: %d)\n",
    			last_rv->error_string(), last_rv->num_entries()));
    return last_rv;

  } // search


  //! @param base_dn
  //!   base DN for search
  string set_basedn (string base_dn) {

    string old_dn = ldap_basedn;

    if(ldap_version == 3) {
      base_dn = string_to_utf8(base_dn);
    }
    ldap_basedn = base_dn;
    DWRITE_HI("client.SET_BASEDN = " + base_dn + "\n");
    return old_dn;
  }

  //!
  //! Sets value of scope for search operation.
  //!
  //! @param scope
  //!  Value can be integer or its corresponding string value.
  //!  0: base, 1: one, 2: sub
  //!
  int set_scope (int|string scope) {

    int old_scope = ldap_scope;

    // support for string based values
    if(stringp(scope))
      switch (lower_case(scope)) {
	case "sub": scope = 2; break;
	case "one": scope = 1; break;
	case "base": scope = 0; break;
	default: return -1;
      }
    else
     if(scope != 0 && scope != 1 && scope != 2)
       return -1;
 
    ldap_scope = scope;
    DWRITE_HI("client.SET_SCOPE = " + (string)scope + "\n");
    return old_scope;
  }

  //! @param option_type
  //!   LDAP_OPT_xxx
  //! @param value
  //!   new value for option
  int set_option (int opttype, int value) {

    DWRITE_HI("client.SET_OPTION: " + (string)opttype + " = " + (string)value + "\n");
    switch (opttype) {
	case 1: // LDAP_OPT_DEREF
		  //if (intp(value))
		    ldap_deref = value;
		  //else
		  //  return -1;
		  break;
	case 2: // LDAP_OPT_SIZELIMIT
		  //if (intp(value))
		    ldap_sizelimit = value;
		  //else
		  //  return -1;
		  break;
	case 3: // LDAP_OPT_TIMELIMIT
		  //if (intp(value))
		    ldap_timelimit = value;
		  //else
		  //  return -1;
		  break;
	case 4: // LDAP_OPT_REFERRALS
	default: return -1;
    }
	

    return 0;
  }

  //! @param option_type
  //!   LDAP_OPT_xxx
  int get_option (int opttype) {


    DWRITE_HI("client.GET_OPTION: " + (string)opttype + "\n");
    switch (opttype) {
	case 1: // LDAP_OPT_DEREF
		  return ldap_deref;
	case 2: // LDAP_OPT_SIZELIMIT
		  return ldap_sizelimit;
	case 3: // LDAP_OPT_TIMELIMIT
		  return ldap_timelimit;
	case 4: // LDAP_OPT_REFERRALS
    }

    return -1;
  }

  private int|string send_modify_op(string dn,
				    mapping(string:array(mixed)) attropval) {
  // MODIFY

    object o, msgval;
    string atype;
    array(object) oatt = ({}), attrarr;


    foreach(indices(attropval), atype) {
      if(!intp((attropval[atype])[0]))
	return seterr(LDAP_PROTOCOL_ERROR);
      attrarr = ({});
      for(int ix=1; ix<sizeof(attropval[atype]); ix++)
	attrarr += ({Standards.ASN1.Types.asn1_octet_string(
			(attropval[atype])[ix])});
//      if(sizeof(attrarr)) // attributevalue ?
	o = Standards.ASN1.Types.asn1_sequence(
		({Standards.ASN1.Types.asn1_octet_string(atype),
		  Standards.ASN1.Types.asn1_set(attrarr)
		}));
//      else
//	o = Standards.ASN1.Encode.asn1_sequence(
//		Standards.ASN1.Encode.asn1_octet_string(atype));
      oatt += ({Standards.ASN1.Types.asn1_sequence(
		  ({ASN1_ENUMERATED((attropval[atype])[0]),
		    o
		  }))});
    } //foreach

    msgval = ASN1_APPLICATION_SEQUENCE(6, 
		({ Standards.ASN1.Types.asn1_octet_string(dn),
		   Standards.ASN1.Types.asn1_sequence(oatt)
		}));

    return do_op(msgval);
  }

  private int|string send_modifydn_op(string dn, string newrdn,
    int deleteoldrdn, string newsuperior) {

    object msgval;
    array seq=({ Standards.ASN1.Types.asn1_octet_string(dn),
               Standards.ASN1.Types.asn1_octet_string(newrdn),
               Standards.ASN1.Types.asn1_boolean(deleteoldrdn)
         });
    if(newsuperior)
          seq+=({Standards.ASN1.Types.asn1_octet_string(newsuperior)});

    msgval = ASN1_APPLICATION_SEQUENCE(12, seq);

    return do_op(msgval);
  }

  //!  The Modify DN Operation allows a client to change the leftmost
  //!  (least significant) component of the name of an entry in the directory,
  //!  or to move a subtree of entries to a new location in the directory.
  //!
  //! @param dn
  //!  DN of source object
  //!
  //! @param newrdn
  //!  RDN of destination
  //!
  //! @param deleteoldrdn
  //!  The parameter controls whether the old RDN attribute values
  //!  are to be retained as attributes of the entry, or deleted
  //!  from the entry.
  //!
  //! @param newsuperior
  //!  If present, this is the Distinguished Name of the entry
  //!  which becomes the immediate superior of the existing entry.
  //!
  //! @returns
  //!  Returns @expr{1@} on success, @expr{0@} otherwise.
  //!
  //! @note
  //!   The API change: the returning code was changed in Pike 7.3+
  //!	to follow his logic better.
  int modifydn (string dn, string newrdn, int deleteoldrdn,
                string|void newsuperior) {

    mixed raw;

    if (chk_ver())
      return 0;
    if (chk_binded())
      return 0;
    if (chk_dn(dn))
      return 0;
    if(ldap_version == 3) {
      dn = string_to_utf8(dn);
      newrdn = string_to_utf8(newrdn);
      if(newsuperior) newsuperior = string_to_utf8(newsuperior);
    }
    if(intp(raw = send_modifydn_op(dn, newrdn, deleteoldrdn, newsuperior))) {
      THROW(({error_string()+"\n",backtrace()}));
      return 0;
    }

    last_rv = result(({raw}));
    DWRITE_HI(sprintf("client.MODIFYDN: %s\n", last_rv->error_string()));
    seterr(last_rv->error_number());
    return !last_rv->error_number();

  }  //modifydn

  //!  The Modify Operation allows a client to request that a modification
  //!  of an entry be performed on its behalf by a server.
  //!
  //! @param dn
  //!  The distinguished name of modified entry.
  //!
  //! @param attropval
  //!  The mapping of attributes with requested operation and attribute's
  //!  values.
  //!
  //!   attropval=([ attribute: ({operation, value1, value2, ...}) ])
  //!
  //!   where operation is one of the following:
  //!    0 (LDAP_OPERATION_ADD) -
  //!      add values listed to the given attribute, creating the attribute
  //!      if necessary
  //!    1 (LDAP_OPERATION_DELETE) -
  //!      delete values listed from the given attribute, removing the entire
  //!      attribute if no values are listed, or if all current values
  //!      of the attribute are listed for deletion
  //!    2 (LDAP_OPERATION_REPLACE) -
  //!      replace all existing values of the given attribute with the new
  //!      values listed, creating the attribute if it did not already exist.
  //!      A replace with no value will delete the entire attribute if it
  //!      exists, and is ignored if the attribute does not exist
  //!
  //! @returns
  //!  Returns @expr{1@} on uccess, @expr{0@} otherwise.
  //!
  //! @note
  //!   The API change: the returning code was changed in Pike 7.3+
  //!	to follow his logic better.
  int modify (string dn, mapping(string:array(mixed)) attropval) {

    int id;
    mixed raw;

    if (chk_ver())
      return 0;
    if (chk_binded())
      return 0;
    if (chk_dn(dn))
      return 0;
    if(ldap_version == 3) {
      dn = string_to_utf8(dn);
      array(string) keys = indices(attropval);
      array(array(mixed)) vals = values(attropval);
      attropval = mkmapping(Array.map(keys, string_to_utf8),
			    Array.map(vals, Array.map, lambda(mixed x) {
							 return
							   (stringp(x)?
							    string_to_utf8(x) :
							    x);
						       }));
    }
    if(intp(raw = send_modify_op(dn, attropval))) {
      THROW(({error_string()+"\n",backtrace()}));
      return 0;
    }

    last_rv = result(({raw}));
    DWRITE_HI(sprintf("client.MODIFY: %s\n", last_rv->error_string()));
    seterr(last_rv->error_number());
    return !last_rv->error_number();

  } // modify

  //! Gets referrals.
  //!
  //! @returns
  //!   Returns array of referrals or @expr{0@}.
  array|int get_referrals() {
    if(last_rv->referrals)
      return last_rv->referrals;
    return 0;
  }

  //! @param  ldapuri
  //!   LDAP URL
  mapping|int parse_url (string ldapuri) {

  // ldap://machine.at.home.cz:123/c=cz?attr1,attr2,attr3?sub?(uid=*)?!bindname=uid=hop,dc=unibase,dc=cz"

  string url=ldapuri, s, scheme;
  array ar;
  mapping res;

    s = (url / ":")[0];
    url = url[sizeof(s)..];

    res = ([ "scheme" : s ]);

#ifdef LDAP_URL_STRICT
    if (url[..2] != "://")
      return -1;
#endif

    s = (url[3..] / "/")[0];
    url = url[sizeof(s)+4..];

    res += ([ "host" : (s / ":")[0] ]);

    if(sizeof(s / ":") > 1)
      res += ([ "port" : (int)((s / ":")[1]) ]);

    ar = url / "?";

    switch (sizeof(ar)) {
      case 5: if (sizeof(ar[4])) {
		mapping extensions = ([]);
		foreach(ar[4] / ",", string ext) {
		  int ix = predef::search(ext, "=");
		  if(ix)
		    extensions += ([ ext[..(ix-1)] : replace(ext[ix+1..],QUOTED_COMMA, ",") ]);
		}
		if (sizeof(extensions))
		  res += ([ "ext" : extensions ]);
	      }
      //case 5: res += ([ "ext" : ar[4] ]);
      case 4: res += ([ "filter" : ar[3] ]);
      case 3: switch (ar[2]) {
		case "sub": res += ([ "scope" : 2 ]); break;
		case "one": res += ([ "scope" : 1 ]); break;
		default: res += ([ "scope" : 0]); // = "base"
	      }
      case 2: res += sizeof(ar[1]) ? ([ "attributes" : ar[1] / "," ]) : ([]);
      case 1: res += ([ "basedn" : ar[0] ]);
    }

    return res;

  } //parse_uri

#else
constant this_program_does_not_exist=1;
#endif
