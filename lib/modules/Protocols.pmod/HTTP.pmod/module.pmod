#pike __REAL_VERSION__

// Informational
constant HTTP_CONTINUE		= 100; // RFC 2616 10.1.1: Continue
constant HTTP_SWITCH_PROT	= 101; // RFC 2616 10.1.2: Switching protocols
constant DAV_PROCESSING		= 102; // RFC 2518 10.1: Processing

// Successful
constant HTTP_OK		= 200; // RFC 2616 10.2.1: OK
constant HTTP_CREATED		= 201; // RFC 2616 10.2.2: Created
constant HTTP_ACCEPTED		= 202; // RFC 2616 10.2.3: Accepted
constant HTTP_NONAUTHORATIVE	= 203; // RFC 2616 10.2.4: Non-Authorative Information
constant HTTP_NO_CONTENT	= 204; // RFC 2616 10.2.5: No Content
constant HTTP_RESET_CONTENT	= 205; // RFC 2616 10.2.6: Reset Content
constant HTTP_PARTIAL_CONTENT	= 206; // RFC 2616 10.2.7: Partial Content
constant DAV_MULTISTATUS	= 207; // RFC 2518 10.2: Multi-Status
constant DELTA_HTTP_IM_USED	= 226; // RFC 3229 10.4.1: IM Used

// Redirection
constant HTTP_MULTIPLE		= 300; // RFC 2616 10.3.1: Multiple Choices
constant HTTP_MOVED_PERM	= 301; // RFC 2616 10.3.2: Moved Permanently
constant HTTP_FOUND		= 302; // RFC 2616 10.3.3: Found
constant HTTP_SEE_OTHER		= 303; // RFC 2616 10.3.4: See Other
constant HTTP_NOT_MODIFIED	= 304; // RFC 2616 10.3.5: Not Modified
constant HTTP_USE_PROXY		= 305; // RFC 2616 10.3.6: Use Proxy
// RFC 2616 10.3.7: 306 not used but reserved.
constant HTTP_TEMP_REDIRECT	= 307; // RFC 2616 10.3.8: Temporary Redirect

// Client errors
constant HTTP_BAD		= 400; // RFC 2616 10.4.1: Bad Request
constant HTTP_UNAUTH		= 401; // RFC 2616 10.4.2: Unauthorized
constant HTTP_PAY		= 402; // RFC 2616 10.4.3: Payment Required
constant HTTP_FORBIDDEN		= 403; // RFC 2616 10.4.4: Forbidden
constant HTTP_NOT_FOUND		= 404; // RFC 2616 10.4.5: Not Found
constant HTTP_METHOD_INVALID	= 405; // RFC 2616 10.4.6: Method Not Allowed
constant HTTP_NOT_ACCEPTABLE	= 406; // RFC 2616 10.4.7: Not Acceptable
constant HTTP_PROXY_AUTH_REQ	= 407; // RFC 2616 10.4.8: Proxy Authentication Required
constant HTTP_TIMEOUT		= 408; // RFC 2616 10.4.9: Request Timeout
constant HTTP_CONFLICT		= 409; // RFC 2616 10.4.10: Conflict
constant HTTP_GONE		= 410; // RFC 2616 10.4.11: Gone
constant HTTP_LENGTH_REQ	= 411; // RFC 2616 10.4.12: Length Required
constant HTTP_PRECOND_FAILED	= 412; // RFC 2616 10.4.13: Precondition Failed
constant HTTP_REQ_TOO_LARGE	= 413; // RFC 2616 10.4.14: Request Entity Too Large
constant HTTP_URI_TOO_LONG	= 414; // RFC 2616 10.4.15: Request-URI Too Long
constant HTTP_UNSUPP_MEDIA	= 415; // RFC 2616 10.4.16: Unsupported Media Type
constant HTTP_BAD_RANGE		= 416; // RFC 2616 10.4.17: Requested Range Not Satisfiable
constant HTTP_EXPECT_FAILED	= 417; // RFC 2616 10.4.18: Expectation Failed
constant HTCPCP_TEAPOT		= 418; // RFC 2324 2.3.2: I'm a teapot
constant DAV_UNPROCESSABLE	= 422; // RFC 2518 10.3: Unprocessable Entry
constant DAV_LOCKED		= 423; // RFC 2518 10.4: Locked
constant DAV_FAILED_DEP		= 424; // RFC 2518 10.5: Failed Dependency

// Server errors
constant HTTP_INTERNAL_ERR	= 500; // RFC 2616 10.5.1: Internal Server Error
constant HTTP_NOT_IMPL		= 501; // RFC 2616 10.5.2: Not Implemented
constant HTTP_BAD_GW		= 502; // RFC 2616 10.5.3: Bad Gateway
constant HTTP_UNAVAIL		= 503; // RFC 2616 10.5.4: Service Unavailable
constant HTTP_GW_TIMEOUT	= 504; // RFC 2616 10.5.5: Gateway Timeout
constant HTTP_UNSUPP_VERSION	= 505; // RFC 2616 10.5.6: HTTP Version Not Supported
constant DAV_STORAGE_FULL	= 507; // RFC 2518 10.6: Insufficient Storage

object do_method(string method,
		 string|Standards.URI url,
		 void|mapping query_variables,
		 void|mapping request_headers,
		 void|Protocols.HTTP.Query con, void|string data)
{
  if(!con) {
    con = Protocols.HTTP.Query();
  }
  if(!request_headers)
    request_headers = ([]);
  
  
  if(stringp(url))
    url=Standards.URI(url);
  
#if constant(SSL.sslfile) 	
  if(url->scheme!="http" && url->scheme!="https")
    error("Protocols.HTTP can't handle %O or any other protocols than HTTP or HTTPS\n",
	  url->scheme);
  
  con->https= (url->scheme=="https")? 1 : 0;
#else
  if(url->scheme!="http"	)
    error("Protocols.HTTP can't handle %O or any other protocol than HTTP\n",
	  url->scheme);
  
#endif
  

  if(!request_headers)
    request_headers = ([]);
  mapping default_headers = ([
    "user-agent" : "Mozilla/4.0 compatible (Pike HTTP client)",
    "host" : url->host ]);

  if(url->user || url->passwd)
    default_headers->authorization = "Basic "
				   + MIME.encode_base64(url->user + ":" +
							(url->password || ""));
  request_headers = default_headers | request_headers;

  string query=url->query;
  if(query_variables && sizeof(query_variables))
  {
    if(query)
      query+="&"+http_encode_query(query_variables);
    else
      query=http_encode_query(query_variables);
  }

  string path=url->path;
  if(path=="") path="/";

  con->sync_request(url->host,url->port,
		    method+" "+path+(query?("?"+query):"")+" HTTP/1.0",
		    request_headers, data);
  
  if (!con->ok) return 0;
  return con;
}

//! @decl Protocols.HTTP.Query get_url(string|Standards.URI url)
//! @decl Protocols.HTTP.Query get_url(string|Standards.URI url, @
//!                                    mapping query_variables)
//! @decl Protocols.HTTP.Query get_url(string|Standards.URI url, @
//!                                    mapping query_variables, @
//!                                    mapping request_headers)
//! @decl Protocols.HTTP.Query get_url(string|Standards.URI url, @
//!                                    mapping query_variables, @
//!                                    mapping request_headers, @
//!                                    Protocols.HTTP.Query query)
//! 	Sends a HTTP GET request to the server in the URL
//!	and returns the created and initialized @[Query] object.
//!	0 is returned upon failure. If a query object having
//!	@tt{request_headers->Connection=="Keep-Alive"@} from a previous
//!     request is provided and the already established server connection
//!     can be used for the next request, you may gain some performance.
//!
object get_url(string|Standards.URI url,
	       void|mapping query_variables,
	       void|mapping request_headers,
	       void|Protocols.HTTP.Query con)
{
  return do_method("GET", url, query_variables, request_headers, con);
}

//! @decl Protocols.HTTP.Query put_url(string|Standards.URI url)
//! @decl Protocols.HTTP.Query put_url(string|Standards.URI url,string file)
//! @decl Protocols.HTTP.Query put_url(string|Standards.URI url,string file, @
//!                                    mapping query_variables)
//! @decl Protocols.HTTP.Query put_url(string|Standards.URI url,string file, @
//!                                    mapping query_variables, @
//!                                    mapping request_headers)
//! @decl Protocols.HTTP.Query put_url(string|Standards.URI url,string file, @
//!                                    mapping query_variables, @
//!                                    mapping request_headers, @
//!                                    Protocols.HTTP.Query query)
//! 	Sends a HTTP PUT request to the server in the URL
//!	and returns the created and initialized @[Query] object.
//!	0 is returned upon failure. If a query object having
//!	@tt{request_headers->Connection=="Keep-Alive"@} from a previous
//!     request is provided and the already established server connection
//!     can be used for the next request, you may gain some performance.
//!
object put_url(string|Standards.URI url,
	       void|string file,
	       void|mapping query_variables,
	       void|mapping request_headers,
	       void|Protocols.HTTP.Query con)
{
  return do_method("PUT", url, query_variables, request_headers, con, file);
}

//! @decl Protocols.HTTP.Query delete_url(string|Standards.URI url)
//! @decl Protocols.HTTP.Query delete_url(string|Standards.URI url, @
//!                                       mapping query_variables)
//! @decl Protocols.HTTP.Query delete_url(string|Standards.URI url, @
//!                                       mapping query_variables, @
//!                                       mapping request_headers)
//! @decl Protocols.HTTP.Query delete_url(string|Standards.URI url, @
//!                                       mapping query_variables, @
//!                                       mapping request_headers, @
//!                                       Protocols.HTTP.Query query)
//! 	Sends a HTTP DELETE request to the server in the URL
//!	and returns the created and initialized @[Query] object.
//!	0 is returned upon failure. If a query object having
//!	@tt{request_headers->Connection=="Keep-Alive"@} from a previous
//!     request is provided and the already established server connection
//!     can be used for the next request, you may gain some performance.
//!
object delete_url(string|Standards.URI url,
		  void|mapping query_variables,
		  void|mapping request_headers,
		  void|Protocols.HTTP.Query con)
{
  return do_method("DELETE", url, query_variables, request_headers, con);
}

//! @decl array(string) get_url_nice(string|Standards.URI url, @
//!                                  mapping query_variables)
//! @decl array(string) get_url_nice(string|Standards.URI url, @
//!                                  mapping query_variables, @
//!                                  mapping request_headers)
//! @decl array(string) get_url_nice(string|Standards.URI url, @
//!                                  mapping query_variables, @
//!                                  mapping request_headers, @
//!                                  Protocols.HTTP.Query query)
//! @decl string get_url_data(string|Standards.URI url, @
//!                           mapping query_variables)
//! @decl string get_url_data(string|Standards.URI url, @
//!                                  mapping query_variables, @
//!                                  mapping request_headers)
//! @decl string get_url_data(string|Standards.URI url, @
//!                                  mapping query_variables, @
//!                                  mapping request_headers, @
//!                                  object(Protocols.HTTP.Query) query)
//!	Returns an array of @tt{({content_type,data})@} and just the data
//!	string respective, 
//!	after calling the requested server for the information.
//!	0 is returned upon failure.
//!

array(string) get_url_nice(string|Standards.URI url,
			   void|mapping query_variables,
			   void|mapping request_headers,
			   void|Protocols.HTTP.Query con)
{
  object c = get_url(url, query_variables, request_headers, con);
  return c && ({ c->headers["content-type"], c->data() });
}

string get_url_data(string|Standards.URI url,
		    void|mapping query_variables,
		    void|mapping request_headers,
		    void|Protocols.HTTP.Query con)
{
  object z = get_url(url, query_variables, request_headers, con);
  return z && z->data();
}

//! @decl array(string) post_url_nice(string|Standards.URI url, @
//!                                   mapping query_variables)
//! @decl array(string) post_url_nice(string|Standards.URI url, @
//!                                   mapping query_variables, @
//!                                   mapping request_headers)
//! @decl array(string) post_url_nice(string|Standards.URI url, @
//!                                   mapping query_variables, @
//!                                   mapping request_headers, @
//!                                   Protocols.HTTP.Query query)
//! @decl string post_url_data(string|Standards.URI url, @
//!                            mapping query_variables)
//! @decl string post_url_data(string|Standards.URI url, @
//!                            mapping query_variables, @
//!                            mapping request_headers)
//! @decl string post_url_data(string|Standards.URI url, @
//!                            mapping query_variables, @
//!                            mapping request_headers, @
//!                            Protocols.HTTP.Query query)
//! @decl object(Protocols.HTTP.Query) post_url(string|Standards.URI url, @
//!                                             mapping query_variables)
//! @decl object(Protocols.HTTP.Query) post_url(string|Standards.URI url, @
//!                                             mapping query_variables, @
//!                                             mapping request_headers)
//! @decl object(Protocols.HTTP.Query) post_url(string|Standards.URI url, @
//!                                             mapping query_variables, @
//!                                             mapping request_headers, @
//!                                             Protocols.HTTP.Query query)
//! 	Similar to the @[get_url()] class of functions, except that the
//!	query variables is sent as a POST request instead of as a GET.
//!

object post_url(string|Standards.URI url,
		mapping query_variables,
		void|mapping request_headers,
		void|Protocols.HTTP.Query con)
{
  return do_method("POST", url, 0,
		   (request_headers||([]))|
		   (["content-type":
		     "application/x-www-form-urlencoded"]),
		   con,
		   http_encode_query(query_variables));
}

array(string) post_url_nice(string|Standards.URI url,
			    mapping query_variables,
			    void|mapping request_headers,
			    void|Protocols.HTTP.Query con)
{
  object c = post_url(url, query_variables, request_headers, con);
  return c && ({ c->headers["content-type"], c->data() });
}

string post_url_data(string|Standards.URI url,
		     mapping query_variables,
		     void|mapping request_headers,
		     void|Protocols.HTTP.Query con)
{
  object z = post_url(url, query_variables, request_headers, con);
  return z && z->data();
}

//!	Helper function for replacing HTML entities
//!	with the corresponding iso-8859-1 characters.
//! @note
//! 	All characters aren't replaced, only those with
//!	corresponding iso-8859-1 characters.
string unentity(string s)
{
   return replace(
      s,

      ({"&AElig;", "&Aacute;", "&Acirc;", "&Agrave;", "&Aring;", "&Atilde;",
	"&Auml;", "&Ccedil;", "&ETH;", "&Eacute;", "&Ecirc;", "&Egrave;",
	"&Euml;", "&Iacute;", "&Icirc;", "&Igrave;", "&Iuml;", "&Ntilde;",
	"&Oacute;", "&Ocirc;", "&Ograve;", "&Oslash;", "&Otilde;", "&Ouml;",
	"&THORN;", "&Uacute;", "&Ucirc;", "&Ugrave;", "&Uuml;", "&Yacute;",
	"&aacute;", "&acirc;", "&aelig;", "&agrave;", "&apos;", "&aring;",
	"&ast;", "&atilde;", "&auml;", "&brvbar;", "&ccedil;", "&cent;",
	"&colon;", "&comma;", "&commat;", "&copy;", "&deg;", "&dollar;",
	"&eacute;", "&ecirc;", "&egrave;", "&emsp;", "&ensp;", "&equals;",
	"&eth;", "&euml;", "&excl;", "&frac12;", "&frac14;", "&frac34;",
	"&frac18;", "&frac38;", "&frac58;", "&frac78;", "&gt;", "&gt",
	"&half;", "&hyphen;", "&iacute;", "&icirc;", "&iexcl;", "&igrave;",
	"&iquest;", "&iuml;", "&laquo;", "&lpar;", "&lsqb;", "&lt;", "&lt",
	"&mdash;", "&micro;", "&middot;", "&nbsp;", "&ndash;", "&not;",
	"&ntilde;", "&oacute;", "&ocirc;", "&ograve;", "&oslash;", "&otilde;",
	"&ouml;", "&para;", "&percnt;", "&period;", "&plus;", "&plusmn;",
	"&pound;", "&quest;", "&quot;", "&raquo;", "&reg;", "&rpar;",
	"&rsqb;", "&sect;", "&semi;", "&shy;", "&sup1;", "&sup2;", "&sup3;",
	"&szlig;", "&thorn;", "&tilde;", "&trade;", "&uacute;", "&ucirc;",
	"&ugrave;", "&uuml;", "&yacute;", "&yen;", "&yuml;", "&verbar;",
	"&amp;", "&#34;", "&#39;", "&#0;", "&#58;" }),

      ({ "Æ", "Á", "Â", "À", "Å", "Ã", "Ä", "Ç", "Ð", "É", "Ê", "È",
	 "Ë", "Í", "Î", "Ì", "Ï", "Ñ", "Ó", "Ô", "Ò", "Ø", "Õ", "Ö", "Þ", "Ú",
	 "Û", "Ù", "Ü", "Ý", "á", "â", "æ", "à", "&apos;", "å", "&ast;", "ã",
	 "ä", "¦", "ç", "¢", ":", ",", "&commat;", "©", "°",
	 "$", "é", "ê", "è", "&emsp;", "&ensp;", "&equals;", "ð", "ë",
	 "!", "½", "¼", "¾", "&frac18;", "&frac38;", "&frac58;",
	 "&frac78;", ">", ">", "&half;", "&hyphen;", "í", "î", "¡", "ì", "¿",
	 "ï", "«", "(", "&lsqb;", "<", "<", "&mdash;", "µ", "·", "",
	 "&ndash;", "¬", "ñ", "ó", "ô", "ò", "ø", "õ", "ö", "¶", "%",
	 ".", "+", "±", "£", "?", "\"", "»", "®", ")",
	 "&rsqb;", "§", "&semi;", "­", "¹", "²", "³", "ß", "þ", "~",
	 "&trade;", "ú", "û", "ù", "ü", "ý", "¥", "ÿ", "&verbar;", "&",
         "\"", "\'", "\000", ":" }),
      );
}

//!	Encodes a query mapping to a string;
//!	this protects odd - in http perspective - characters
//!	like '&' and '#' and control characters,
//!	and packs the result together in a HTTP query string.
//!
//!	Example:
//!	@pre{
//!	> Protocols.HTTP.http_encode_query( (["anna":"eva","lilith":"blue"]) );  
//!     Result: "lilith=blue&anna=eva"
//!     > Protocols.HTTP.http_encode_query( (["&amp;":"&","'=\"":"\0\0\0"]) );  
//!     Result: "%26amp%3b=%26&%27%3d%22=%00%00%00"
//!	@}
string http_encode_query(mapping(string:int|string) variables)
{
   return Array.map((array)variables,
		    lambda(array(string|int|array(string)) v)
		    {
		       if (intp(v[1]))
			  return http_encode_string(v[0]);
		       if (arrayp(v[1]))
			 return map(v[1], lambda (string val) {
					    return 
					      http_encode_string(v[0])+"="+
					      http_encode_string(val);
					  })*"&";
		       return http_encode_string(v[0])+"="+
			 http_encode_string(v[1]);
		    })*"&";
}

//!	This protects all odd - see @[http_encode_query()] - 
//!	characters for transfer in HTTP.
//!
//!	Do not use this function to protect URLs, since
//!	it will protect URL characters like @tt{'/'@} and @tt{'?'@}.
//! @param in
//!     The string to encode
//! @returns
//!     The HTTP encoded string
string http_encode_string(string in)
{
   return replace(
      in, 
      ({ "\000", "\001", "\002", "\003", "\004", "\005", "\006", "\007", 
	 "\010", "\011", "\012", "\013", "\014", "\015", "\016", "\017", 
	 "\020", "\021", "\022", "\023", "\024", "\025", "\026", "\027", 
	 "\030", "\031", "\032", "\033", "\034", "\035", "\036", "\037", 
	 "\200", "\201", "\202", "\203", "\204", "\205", "\206", "\207", 
	 "\210", "\211", "\212", "\213", "\214", "\215", "\216", "\217", 
	 "\220", "\221", "\222", "\223", "\224", "\225", "\226", "\227", 
	 "\230", "\231", "\232", "\233", "\234", "\235", "\236", "\237", 
	 " ", "%", "'", "\"", "+", "&", "=", "/",
	 "#", ";", "\\", "<", ">", "\t", "\n", "\r", "@" }),
      ({ 
	 "%00", "%01", "%02", "%03", "%04", "%05", "%06", "%07",    
	 "%08", "%09", "%0a", "%0b", "%0c", "%0d", "%0e", "%0f", 
	 "%10", "%11", "%12", "%13", "%14", "%15", "%16", "%17", 
	 "%18", "%19", "%1a", "%1b", "%1c", "%1d", "%1e", "%1f", 
	 "%80", "%81", "%82", "%83", "%84", "%85", "%86", "%87", 
	 "%88", "%89", "%8a", "%8b", "%8c", "%8d", "%8e", "%8f", 
	 "%90", "%91", "%92", "%93", "%94", "%95", "%96", "%97", 
	 "%98", "%99", "%9a", "%9b", "%9c", "%9d", "%9e", "%9f", 
	 "%20", "%25", "%27", "%22", "%2b", "%26", "%3d", "%2f",
	 "%23", "%3b", "%5c", "%3c", "%3e", "%09", "%0a", "%0d",
         "%40" }));
}

//!    Encode the specified string in as to the HTTP cookie standard.
//! @param f
//!    The string to encode.
//! @returns
//!    The HTTP cookie encoded string.
string http_encode_cookie(string f)
{
   return replace(
      f, 
      ({ "\000", "\001", "\002", "\003", "\004", "\005", "\006", "\007", 
	 "\010", "\011", "\012", "\013", "\014", "\015", "\016", "\017", 
	 "\020", "\021", "\022", "\023", "\024", "\025", "\026", "\027", 
	 "\030", "\031", "\032", "\033", "\034", "\035", "\036", "\037", 
	 "\200", "\201", "\202", "\203", "\204", "\205", "\206", "\207", 
	 "\210", "\211", "\212", "\213", "\214", "\215", "\216", "\217", 
	 "\220", "\221", "\222", "\223", "\224", "\225", "\226", "\227", 
	 "\230", "\231", "\232", "\233", "\234", "\235", "\236", "\237", 
	 " ", "%", "'", "\"", ",", ";", "=", ":" }),
      ({ 
	 "%00", "%01", "%02", "%03", "%04", "%05", "%06", "%07",    
	 "%08", "%09", "%0a", "%0b", "%0c", "%0d", "%0e", "%0f", 
	 "%10", "%11", "%12", "%13", "%14", "%15", "%16", "%17", 
	 "%18", "%19", "%1a", "%1b", "%1c", "%1d", "%1e", "%1f", 
	 "%80", "%81", "%82", "%83", "%84", "%85", "%86", "%87", 
	 "%88", "%89", "%8a", "%8b", "%8c", "%8d", "%8e", "%8f", 
	 "%90", "%91", "%92", "%93", "%94", "%95", "%96", "%97", 
	 "%98", "%99", "%9a", "%9b", "%9c", "%9d", "%9e", "%9f", 
	 "%20", "%25", "%27", "%22", "%2c", "%3b", "%3d", "%3a" }));
}
