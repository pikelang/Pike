// LDAP client protocol implementation for Pike.
//
// $Id: client.pike,v 1.1 1999/04/24 16:43:28 js Exp $
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
//
// Specifications:
//
//	RFC 1558			  (search filter representations)
//	RFC 1777,1778,1779		  (version2 spec)
//	RFC 1823			  (v2 API)
//	RFC 2251,2252,2253,2254,2255,2256 (version3 spec)
//	draft-ietf-asid-ldap-c-api-00.txt (v3 API)
//	RFC2279	   			  (UTF-8)
//
//	Interesting, applicable
//	RFC 2307   (LDAP as network information services; draft?)



#include "ldap_globals.h"

#include "ldap_errors.h"

// ------------------------

// ASN.1 decode macros
#define ASN1_DECODE_RESULTAPP(X)	(.ldap_privates.ldap_der_decode(X)->elements[1]->get_tag())
#define ASN1_DECODE_RESULTCODE(X)	(int)(.ldap_privates.ldap_der_decode(X)->elements[1]->elements[0]->value->cast_to_int())
#define ASN1_DECODE_RESULTSTRING(X)	(.ldap_privates.ldap_der_decode(X)->elements[1]->elements[1]->value)
#define ASN1_DECODE_ENTRIES(X)		_New_decode(X)
#define ASN1_DECODE_DN(X)		(string)(.ldap_privates.ldap_der_decode(X)->elements[1]->elements[0]->value)
#define ASN1_DECODE_RAWDEBUG(X)		(.ldap_privates.ldap_der_decode(X)->debug_string())
#define ASN1_GET_ATTR_ARRAY(X)		(array)(.ldap_privates.ldap_der_decode(X)->elements[1]->elements[1]->elements[1..])
#define ASN1_GET_ATTR_NAME(X)		((X)->elements[0]->value)

  inherit .protocol;

    private int binded = 0;	// flag for v2 operations
    private string ldap_basedn = "";	// baseDN
    private int ldap_scope = 0;		// 0: base, 1: onelevel, 2: subtree
    private int ldap_deref = 0;		// 0: ...
    private int ldap_sizelimit = 0;
    private int ldap_timelimit = 0;



  class result // ------------------
  {

    private int resultcode = LDAP_SUCCESS;
    //private string resultstring = LDAP_SUCCESS_STR;
    private int entrycnt = 0;
    private int actnum = 0;
    private array(mapping(string:array(string))) entry = ({});

    int error_number() { return(resultcode); }

    string error_string() { return(ldap_errlist[resultcode]); }

    int num_entries() { return(entrycnt); }

    int count_entries() { return(entrycnt - actnum); }


    private array _get_attr_values(object x) {

      array res = ({});

      if(!sizeof(x->elements))
	return(res);
      foreach(x->elements[1]->elements, object val1) 
	res += ({ val1->value });
      return(res);
    }

    private array _New_decode(array ar) {

      array res = ({});
      array entry1;
      mapping attrs;

      foreach(ar, string raw1)  {
	attrs = (["dn":({ASN1_DECODE_DN(raw1)})]);
	entry1 = ASN1_GET_ATTR_ARRAY(raw1);
	foreach(entry1, object attr1) {
	  attrs += ([ASN1_GET_ATTR_NAME(attr1):_get_attr_values(attr1)]);
	}
	res += ({attrs});
      }
      return (res);
    } // _New_decode

    object|int create(array rawres, int|void stuff) {
    // rawres: array of result in raw format, but WITHOUT LDAP PDU !!!
    // stuff: 1=bind result; ...

      int lastel = sizeof(rawres) - 1;

      if (lastel < 0) {
        ::seterr (LDAP_LOCAL_ERROR);
        THROW(({"LDAP: Internal error.\n",backtrace()}));
        return(-::ldap_errno);
      }
      //DWRITE(sprintf("DEB: lastel=%d\n",lastel));
      DWRITE(sprintf("result.create: rawres=%O\n",rawres[lastel]));

      // The last element of 'rawres' is result itself
      resultcode = ASN1_DECODE_RESULTCODE(rawres[lastel]);
      DWRITE(sprintf("result.create: code=%d\n",resultcode));
      #if 0
      resultstring = ASN1_DECODE_RESULTSTRING(rawres[lastel]);
      DWRITE(sprintf("result.create: resstr=%s\n",resultstring));
      #endif
      // referral (v3 mode)
      // ...
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

      return(this_object());

    } // create

    int|mapping(string:array(string)) fetch(int|void ix) {

      if (!ix)
	ix = actnum + 1;
      if ((ix <= num_entries()) && (ix > 0)) {
	actnum = ix - 1;
        return(entry[actnum]);
      }
      return(0);
    }

    string get_dn() { return(fetch()["dn"][0]); }

    void first() { actnum = 0; }

    int next() {
      if (actnum < (num_entries()-1)) {
	actnum++;
	return(count_entries());
      }
      return(0);
    }
    
  } // end of class 'result' ---------------

  // helper functions

  private int chk_ver() {

    if ((ldap_version != 2) && (ldap_version != 3)) {
      seterr (LDAP_PROTOCOL_ERROR);
      THROW(({"LDAP: Unknown/unsupported protocol version.\n",backtrace()}));
      return(-ldap_errno);
    }
    return(0);
  }

  private int chk_binded() {
  // For version 2: we must be 'binded' first !!!

    if ((ldap_version == 2) && !binded) {
      seterr (LDAP_PROTOCOL_ERROR);
      THROW(({"LDAP: Must binded first.\n",backtrace()}));
      return(-ldap_errno);
    }
    return(0);
  }

  private int chk_dn(string dn) {

    if ((!dn) || (!sizeof(dn))) {
      seterr (LDAP_INVALID_DN_SYNTAX);
      THROW(({"LDAP: Invalid DN syntax.\n",backtrace()}));
      return(-ldap_errno);
    }
    return(0);
  }

  // API function (ldap_open)
  //
  // create(string|void server)
  //
  //	server:		server name (hostname or IP, default: 127.0.0.1)
  //			with optional port number (default: 389)
  void create(string|void server)
  {
    int port = LDAP_DEFAULT_PORT;

    if(!server || !sizeof(server))
      server = (string)LDAP_DEFAULT_HOST;
    else
      if(predef::search(server,":")>0) {
	port = (int)((server / ":")[1]);
	server = (server / ":")[0];
      }
      
    ::create(server, port);
    if(!::connected) 
    {
      THROW(({"Failed to connect to LDAP server.\n",backtrace()}));
    }
    DWRITE(sprintf("client.create: remote = %s\n", query_address()));
    DWRITE_HI("client.OPEN: " + server + " - OK\n");

    binded = 0;

  } // create

  private mixed send_bind_op(string name, string password) {
  // Simple BIND operation

    object msgval, vers, namedn, auth, app;

    vers = Standards.ASN1.Types.asn1_integer(ldap_version);
    namedn = Standards.ASN1.Types.asn1_octet_string(name);
    auth = ASN1_CONTEXT_OCTET_STRING(0, password);
    // SASL credentials ommited

    msgval = ASN1_APPLICATION_SEQUENCE(0, ({vers, namedn, auth}));

    return (do_op(msgval));
  }

  // API function (ldap_bind)
  //
  // bind(string|void name, string|void password, int|void proto)
  //
  //	name:
  //	password:
  //	proto:		protocol version, supported 2 and 3
  int bind (string|void name, string|void password, int|void proto) {

    int id;
    mixed raw;
    object rv;

    if (!proto)
      proto = LDAP_DEFAULT_VERSION;
    if (chk_ver())
      return(-ldap_errno);
    if (!stringp(name))
      name = "";
    if (!stringp(password))
      password = "";
    ldap_version = proto;
#if UTF8_SUPPORT
    if(ldap_version == 3) {
      dn = Standards.Unicode.UTF8.encode(dn, charset);
      password = Standards.Unicode.UTF8.encode(password, charset);
    }
#endif
    if(intp(raw = send_bind_op(name, password))) {
      THROW(({error_string()+"\n",backtrace()}));
      return(-ldap_errno);
    }

   rv = result(({raw}),1);
   if (!rv->error_number())
     binded = 1;
   DWRITE_HI(sprintf("client.BIND: %s\n", rv->error_string()));
   return (seterr (rv->error_number()));

  } // bind

  private int send_unbind_op() {
  // UNBIND operation

    writemsg(ASN1_APPLICATION_OCTET_STRING(2, ""));

    //ldap::close();

    return (1);
  }

  void destroy() {

    //send_unbind_op();
    destruct(this_object());
  }

  // API function (ldap_unbind)
  //
  // unbind()
  //
  int unbind () {

    if (send_unbind_op() < 1) {
      THROW(({error_string()+"\n",backtrace()}));
      return(-ldap_errno);
    }
    binded = 0;
    DWRITE_HI("client.UNBIND: OK\n");

  } // unbind

  private int|string send_op_withdn(int op, string dn) {
  // DELETE, ...

    return (do_op(ASN1_APPLICATION_OCTET_STRING(op, dn)));

  }

  // API function (ldap_delete)
  //
  // delete(string dn)
  //
  //	dn:		DN of deleted object
  int delete (string dn) {

    int id;
    mixed raw;
    object rv;

    if (chk_ver())
      return(-ldap_errno);
    if (chk_binded())
      return(-ldap_errno);
    if (chk_dn(dn))
      return(-ldap_errno);
#if UTF8_SUPPORT
    if(ldap_version == 3) {
      dn = Standards.Unicode.UTF8.encode(dn, charset);
    }
#endif
    if(intp(raw = send_op_withdn(10, dn))) {
      THROW(({error_string()+"\n",backtrace()}));
      return(-ldap_errno);
    }

   rv = result(({raw}));
   DWRITE_HI(sprintf("client.DELETE: %s\n", rv->error_string()));
   return (seterr (rv->error_number()));

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

    return (do_op(msgval));
  }


  // API function (ldap_compare)
  //
  // compare(string dn, array(string) aval)
  //
  //	dn:		DN of compared object
  //	aval:		attribute value
  int compare (string dn, array(string) aval) {

    int id;
    mixed raw;
    object rv;

    // if (!aval || sizeof(aval)<2)
    //  error
    if (chk_ver())
      return(-ldap_errno);
    if (chk_binded())
      return(-ldap_errno);
    if (chk_dn(dn))
      return(-ldap_errno);
#if UTF8_SUPPORT
    if(ldap_version == 3) {
      dn = Standards.Unicode.UTF8.encode(dn, charset);
      aval = Standards.Unicode.UTF8.encode(aval, charset);
    }
#endif
    if(intp(raw = send_compare_op(dn, aval))) {
      THROW(({error_string()+"\n",backtrace()}));
      return(-ldap_errno);
    }

   rv = result(({raw}));
   DWRITE_HI(sprintf("client.COMPARE: %s\n", rv->error_string()));
   return (seterr (rv->error_number()));

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

    return (do_op(msgval));
  }


  // API function (ldap_add)
  //
  // add(string dn, mapping(string:array(string)) attrs)
  //
  //	dn:		DN of compared object
  //	ttrs:		attribute(s) and value
  int add (string dn, mapping(string:array(string)) attrs) {

    int id;
    mixed raw;
    object rv;

    if (chk_ver())
      return(-ldap_errno);
    if (chk_binded())
      return(-ldap_errno);
    if (chk_dn(dn))
      return(-ldap_errno);
#if UTF8_SUPPORT
    if(ldap_version == 3) {
      dn = Standards.Unicode.UTF8.encode(dn, charset);
      attrs = Standards.Unicode.UTF8.encode(attrs, charset);
    }
#endif
    if(intp(raw = send_add_op(dn, attrs))) {
      THROW(({error_string()+"\n",backtrace()}));
      return(-ldap_errno);
    }

    rv = result(({raw}));
    DWRITE_HI(sprintf("client.ADD: %s\n", rv->error_string()));
    return (seterr (rv->error_number()));

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
    return(rvarr);
}

  /*private*/ object make_simple_filter(string filter) {
  // filter expression parser - only simple expressions!

    object rv;
    int op, ix;

    DWRITE(sprintf("client.make_simple_filter: filter: [%s]\n", filter));
    if((op = predef::search(filter, ">=")) > 0)	{ // greater or equal
      DWRITE("client.make_simple_filter: [>=]\n");
      return(ASN1_CONTEXT_SEQUENCE(5,
		({Standards.ASN1.Types.asn1_octet_string(filter[..(op-1)]),
		  Standards.ASN1.Types.asn1_octet_string(filter[(op+2)..])
		})));
    }
    if((op = predef::search(filter, "<=")) > 0)	{ // less or equal
      DWRITE("client.make_simple_filter: [<=]\n");
      return(ASN1_CONTEXT_SEQUENCE(6,
		({Standards.ASN1.Types.asn1_octet_string(filter[..(op-1)]),
		  Standards.ASN1.Types.asn1_octet_string(filter[(op+2)..])
		})));
    }
    if((op = predef::search(filter, "=")) > 0) {	// equal, substring
	if((filter[-2] == '=') && (filter[-1] == '*'))  // any (=*)
	  return(make_simple_filter(filter[..op-1]));
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
	    return(ASN1_CONTEXT_SEQUENCE(4,
		({Standards.ASN1.Types.asn1_octet_string(filter[..(op-1)]),
		  Standards.ASN1.Types.asn1_sequence(oarr)
		})));
      } else {				   // equal
        DWRITE("client.make_simple_filter: [=]\n");
	return(ASN1_CONTEXT_SEQUENCE(3,
		({Standards.ASN1.Types.asn1_octet_string(filter[..(op-1)]),
		  Standards.ASN1.Types.asn1_octet_string(filter[(op+1)..])
		})));
      }
    } // if equal,substring
    // present
    DWRITE("client.make_simple_filter: [present]\n");
    return(ASN1_CONTEXT_OCTET_STRING(7, filter));
}


  /*private*/ object|int make_filter(string filter) {
  // filter expression parser

    object ohlp;
    array(object) oarr = ({});
    int op ;

    DWRITE("client.make_filter: filter=["+filter+"]\n");
    // strip leading and trailing spaces
    while(filter[0] == ' ')
      filter = filter[1..];
    while(filter[-1] == ' ')
      filter = reverse(reverse(filter)[1..]);

    // strip leading and trailing brackets
#if 1
    if(filter[0] == '(') {
      int ix;
      string f2 = reverse(filter[1..]);
      if((ix = predef::search(f2, ")")) > -1) {
	filter = reverse(f2[ix+1..]);
//DWRITE("DEB: make_filter: filter=["+filter+"]\n");
	return(make_filter(filter));
      }
      return(-1); // error in filter expr.
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
		  return(0); // error: Filter parameter error!
      	      DWRITE(sprintf("client.make_filter: expr_cnt=%d\n",sizeof(oarr)));
	      //ohlp = Standards.ASN1.Encode.asn1_set(@oarr);
	      op = 0;
	      if (filter[0] == '|')
		op = 1;
	      return(ASN1_CONTEXT_SET(op, oarr));
      case '!':		// not
	      if (objectp(ohlp = make_filter(filter_get_sub1expr(filter[1..])[0])))
	        return(ASN1_CONTEXT_SEQUENCE(2, ohlp));
	      else
		return(0); // error: Filter parameter error!
	      break;
      default :		// we assume simple filter
	      return(make_simple_filter(filter));
    }
}

  private int|string send_search_op(string basedn, int scope, int deref,
		     int sizelimit, int timelimit, int attrsonly,
		     string filter, void|array(string) attrs){
  // SEARCH
  // limitations: !!! sizelimit and timelimit should be unsigned int !!!

    object msgval, ofilt;
    array(object) ohlp;

    if(!objectp(ofilt = make_filter(filter))) {
      return(-seterr(LDAP_FILTER_ERROR));
    }
    ohlp = ({ofilt});
    if (arrayp(attrs)) { //explicitly defined attributes
      array(object) o2 = ({});
      //DWRITE(sprintf("DEB: attrs=%O\n",attrs));
      foreach(attrs, string s2)
	o2 += ({Standards.ASN1.Types.asn1_octet_string(s2)});
      ohlp += ({Standards.ASN1.Types.asn1_sequence(o2)});
    } else
      ohlp += ({Standards.ASN1.Types.asn1_sequence(({}))});

    msgval = ASN1_APPLICATION_SEQUENCE(3,
		({ Standards.ASN1.Types.asn1_octet_string(basedn),
		   ASN1_ENUMERATED(scope),
		   ASN1_ENUMERATED(deref),
		   Standards.ASN1.Types.asn1_integer(sizelimit),
		   Standards.ASN1.Types.asn1_integer(timelimit),
		   ASN1_BOOLEAN(attrsonly ? -1 : 0),
		   @ohlp
		})) ;

    return (do_op(msgval));
  }


  // API function (ldap_search)
  //
  // search(string filter, int|void attrsonly, array(string)|void attrs)
  //
  //	filter:		search filter
  //	attrsonly:	flag
  //	attrsy:		attribute(s) name
  object|int search (string filter, int|void attrsonly, array(string)|void attrs) {

    int id;
    mixed raw;
    array(string) rawarr = ({});
    mixed rv;

    DWRITE_HI("client.SEARCH: " + (string)filter + "\n");
    if (chk_ver())
      return(-ldap_errno);
    if (chk_binded())
      return(-ldap_errno);
#if UTF8_SUPPORT
    if(ldap_version == 3) {
      filter = Standards.Unicode.UTF8.encode(filter, charset);
    }
#endif
    if(intp(raw = send_search_op(ldap_basedn, ldap_scope, ldap_deref,
			ldap_sizelimit, ldap_timelimit, attrsonly, filter,
			attrs))) {
      THROW(({error_string()+"\n",backtrace()}));
      return(-ldap_errno);
    }

    rawarr = ({raw});
    while (ASN1_DECODE_RESULTAPP(raw) != 5) {
      raw = readmsg(id);
      if (intp(raw)) {
        THROW(({error_string()+"\n",backtrace()}));
        return(-ldap_errno);
      }
      if((ASN1_DECODE_RESULTAPP(raw) == 4) || (ASN1_DECODE_RESULTAPP(raw) == 5))
	rawarr += ({raw});
      else {
        DWRITE(sprintf("client.search: APP[%d]\n",ASN1_DECODE_RESULTAPP(raw)));
	// in version 3 - referrals
      }
    } // while

    rv = result(rawarr);
    if(objectp(rv))
      seterr (rv->error_number());
    //if (rv->error_number() || !rv->num_entries())	// if error or entries=0
    //  rv = rv->error_number();

    DWRITE_HI(sprintf("client.SEARCH: %s (entries: %d)\n", rv->error_string(), rv->num_entries()));
    return(rv);

  } // search


  // API function (ldap_setbasedn)
  //
  // set_basedn(string base_dn)
  //
  //	base_dn:	base DN for search
  string set_basedn (string base_dn) {

    string old_dn = ldap_basedn;

#if UTF8_SUPPORT
    if(ldap_version == 3) {
      base_dn = Standards.Unicode.UTF8.encode(base_dn, charset);
    }
#endif
    ldap_basedn = base_dn;
    DWRITE_HI("client.SET_BASEDN = " + base_dn + "\n");
    return(old_dn);
  }

  // API function (ldap_setscope)
  //
  // set_scope(string base_dn)
  //
  //	scope:		scope; 0: base, 1: onelevel, 2: subtree
  int set_scope (int scope) {

    int old_scope = ldap_scope;

    ldap_scope = scope;
    DWRITE_HI("client.SET_SCOPE = " + (string)scope + "\n");
    return(old_scope);
  }

  // API function (ldap_setoption)
  //
  // set_option(int option_type, mixed value)
  //
  //	option_type:	LDAP_OPT_xxx
  //	value:		new value for option
  int set_option (int opttype, int value) {

    DWRITE_HI("client.SET_OPTION: " + (string)opttype + " = " + (string)value + "\n");
    switch (opttype) {
	case 1: // LDAP_OPT_DEREF
		  //if (intp(value))
		    ldap_deref = value;
		  //else
		  //  return(-1);
		  break;
	case 2: // LDAP_OPT_SIZELIMIT
		  //if (intp(value))
		    ldap_sizelimit = value;
		  //else
		  //  return(-1);
		  break;
	case 3: // LDAP_OPT_TIMELIMIT
		  //if (intp(value))
		    ldap_timelimit = value;
		  //else
		  //  return(-1);
		  break;
	case 4: // LDAP_OPT_REFERRALS
	default: return(-1);
    }
	

    return(0);
  }

  // API function (ldap_getoption)
  //
  // get_option(int option_type)
  //
  //	option_type:	LDAP_OPT_xxx
  int get_option (int opttype) {


    DWRITE_HI("client.GET_OPTION: " + (string)opttype + "\n");
    switch (opttype) {
	case 1: // LDAP_OPT_DEREF
		  return(ldap_deref);
	case 2: // LDAP_OPT_SIZELIMIT
		  return(ldap_sizelimit);
	case 3: // LDAP_OPT_TIMELIMIT
		  return(ldap_timelimit);
	case 4: // LDAP_OPT_REFERRALS
    }

    return(-1);
  }


  private int|string send_modify_op(string dn,
				    mapping(string:array(mixed)) attropval) {
  // MODIFY

    object o, msgval;
    string atype;
    array(object) oatt = ({}), attrarr;


    foreach(indices(attropval), atype) {
      if(!intp((attropval[atype])[0]))
	return(seterr (LDAP_PROTOCOL_ERROR));
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

    return (do_op(msgval));
  }


  // API function (ldap_modify)
  //
  // modify(string dn, mapping(string:array(mix)) attropval)
  //
  //	dn:		DN of compared object
  //	attropval:	attribute(s), operation and value(s)
  int modify (string dn, mapping(string:array(mixed)) attropval) {

    int id;
    mixed raw;
    object rv;

    if (chk_ver())
      return(-ldap_errno);
    if (chk_binded())
      return(-ldap_errno);
    if (chk_dn(dn))
      return(-ldap_errno);
#if UTF8_SUPPORT
    if(ldap_version == 3) {
      dn = Standards.Unicode.UTF8.encode(dn, charset);
      attrs = Standards.Unicode.UTF8.encode(attropval, charset);
    }
#endif
    if(intp(raw = send_modify_op(dn, attropval))) {
      THROW(({error_string()+"\n",backtrace()}));
      return(-ldap_errno);
    }

    rv = result(({raw}));
    DWRITE_HI(sprintf("client.MODIFY: %s\n", rv->error_string()));
    return (seterr (rv->error_number()));

  } // modify


