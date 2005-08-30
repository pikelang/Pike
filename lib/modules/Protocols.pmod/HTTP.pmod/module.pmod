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
constant TCN_VARIANT_NEGOTIATES	= 506; // RFC 2295 8.1: Variant Also Negotiates
constant DAV_STORAGE_FULL	= 507; // RFC 2518 10.6: Insufficient Storage

//! Low level HTTP call method.
//!
//! @param method
//!   The HTTP method to use, e.g. @expr{"GET"@}.
//! @param url
//!   The URL to perform @[method] on. Should be a complete URL,
//!   including protocol, e.g. @expr{"https://pike.ida.liu.se/"@}.
//! @param query_variables
//!   Calls @[http_encode_query] and appends the result to the URL.
//! @param request_headers
//!   The HTTP headers to be added to the request. By default the
//!   headers User-agent, Host and, if needed by the url,
//!   Authorization will be added, with generated contents.
//!   Providing these headers will override the default. Setting
//!   the value to 0 will remove that header from the request.
//! @param con
//!   Old connection object.
//! @param data
//!   Data payload to be transmitted in the request.
//!
//! @seealso
//!   @[do_sync_method()]
.Query do_method(string method,
		 string|Standards.URI url,
		 void|mapping(string:int|string) query_variables,
		 void|mapping(string:string|array(string)) request_headers,
		 void|Protocols.HTTP.Query con, void|string data)
{
  if(stringp(url))
    url=Standards.URI(url);

  if( (< "httpu", "httpmu" >)[url->scheme] ) {
    return do_udp_method(method, url, query_variables, request_headers,
			 con, data);
  }

  if(!con)
    con = .Query();

#if constant(SSL.sslfile) 	
  if(url->scheme!="http" && url->scheme!="https")
    error("Can't handle %O or any other protocols than HTTP or HTTPS.\n",
	  url->scheme);

  con->https = (url->scheme=="https")? 1 : 0;
#else
  if(url->scheme!="http")
    error("Can't handle %O or any other protocol than HTTP.\n",
	  url->scheme);
#endif

  if(!request_headers)
    request_headers = ([]);
  mapping default_headers = ([
    "user-agent" : "Mozilla/5.0 (compatible; MSIE 6.0; Pike HTTP client)"
    " Pike/" + __REAL_MAJOR__ + "." + __REAL_MINOR__ + "." + __REAL_BUILD__,
    "host" : url->host + 
    (url->port!=(url->scheme=="https"?443:80)?":"+url->port:"")]);

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

  if (!con->ok) {
    if (con->errno)
      error ("I/O error: %s\n", strerror (con->errno));
    return 0;
  }
  return con;
}

static .Query do_udp_method(string method, Standards.URI url,
			    void|mapping(string:int|string) query_variables,
			    void|mapping(string:string|array(string))
			    request_headers, void|Protocols.HTTP.Query con,
			    void|string data)
{
  if(!request_headers)
    request_headers = ([]);

  string path = url->path;
  if(path=="") {
    if(url->method=="httpmu")
      path = "*";
    else
      path = "/";
  }
  string msg = method + " " + path + " HTTP/1.1\r\n";

  Stdio.UDP udp = Stdio.UDP();
  int port = 10000 + random(1000);
  int i;
  while(1) {
    if( !catch( udp->bind(port++) ) ) break;
    if( i++ > 1000 ) error("Could not open a UDP port.\n");
  }
  if(url->method=="httpmu") {
    mapping ifs = Stdio.gethostip();
    if(!sizeof(ifs)) error("No Internet interface found.\n");
    foreach(ifs; string i; mapping data)
      if(sizeof(data->ips)) {
	udp->enable_multicast(data->ips[0]);
	break;
      }
    udp->add_membership(url->host, 0, 0);
  }
  udp->set_multicast_ttl(4);
  udp->send(url->host, url->port, msg);
  if (!con) {
    con = .Query();
  }
  con->con = udp;
  return con;
}

//! Low level asynchronous HTTP call method.
//!
//! @param method
//!   The HTTP method to use, e.g. @expr{"GET"@}.
//! @param url
//!   The URL to perform @[method] on. Should be a complete URL,
//!   including protocol, e.g. @expr{"https://pike.ida.liu.se/"@}.
//! @param query_variables
//!   Calls @[http_encode_query] and appends the result to the URL.
//! @param request_headers
//!   The HTTP headers to be added to the request. By default the
//!   headers User-agent, Host and, if needed by the url,
//!   Authorization will be added, with generated contents.
//!   Providing these headers will override the default. Setting
//!   the value to 0 will remove that header from the request.
//! @param con
//!   Previously initialized connection object.
//!   In particular the callbacks must have been set
//!   (@[Query.set_callbacks()]).
//! @param data
//!   Data payload to be transmitted in the request.
//!
//! @seealso
//!   @[do_method()], @[Query.set_callbacks()]
void do_async_method(string method,
		     string|Standards.URI url,
		     void|mapping(string:int|string) query_variables,
		     void|mapping(string:string|array(string)) request_headers,
		     Protocols.HTTP.Query con, void|string data)
{
  if(stringp(url))
    url=Standards.URI(url);

  if( (< "httpu", "httpmu" >)[url->scheme] ) {
    error("Asynchronous httpu or httpmu not yet supported.\n");
  }

#if constant(SSL.sslfile) 	
  if(url->scheme!="http" && url->scheme!="https")
    error("Can't handle %O or any other protocols than HTTP or HTTPS.\n",
	  url->scheme);

  con->https = (url->scheme=="https")? 1 : 0;
#else
  if(url->scheme!="http")
    error("Can't handle %O or any other protocol than HTTP.\n",
	  url->scheme);
#endif

  if(!request_headers)
    request_headers = ([]);
  mapping default_headers = ([
    "user-agent" : "Mozilla/5.0 (compatible; MSIE 6.0; Pike HTTP client)"
    " Pike/" + __REAL_MAJOR__ + "." + __REAL_MINOR__ + "." + __REAL_BUILD__,
    "host" : url->host + 
    (url->port!=(url->scheme=="https"?443:80)?":"+url->port:"")]);

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

  con->async_sync_request(url->host, url->port,
			  method+" "+path+(query?("?"+query):"")+" HTTP/1.0",
			  request_headers, data);
}

//! Sends a HTTP GET request to the server in the URL and returns the
//! created and initialized @[Query] object. @expr{0@} is returned
//! upon failure. If a query object having
//! @expr{request_headers->Connection=="Keep-Alive"@} from a previous
//! request is provided and the already established server connection
//! can be used for the next request, you may gain some performance.
//!
.Query get_url(string|Standards.URI url,
	       void|mapping(string:int|string) query_variables,
	       void|mapping(string:string|array(string)) request_headers,
	       void|Protocols.HTTP.Query con)
{
  return do_method("GET", url, query_variables, request_headers, con);
}

//! Sends a HTTP PUT request to the server in the URL and returns the
//! created and initialized @[Query] object. @expr{0@} is returned upon
//! failure. If a query object having
//! @expr{request_headers->Connection=="Keep-Alive"@} from a previous
//! request is provided and the already established server connection
//! can be used for the next request, you may gain some performance.
//!
.Query put_url(string|Standards.URI url,
	       void|string file,
	       void|mapping(string:int|string) query_variables,
	       void|mapping(string:string|array(string)) request_headers,
	       void|Protocols.HTTP.Query con)
{
  return do_method("PUT", url, query_variables, request_headers, con, file);
}

//! Sends a HTTP DELETE request to the server in the URL and returns
//! the created and initialized @[Query] object. @expr{0@} is returned
//! upon failure. If a query object having
//! @expr{request_headers->Connection=="Keep-Alive"@} from a previous
//! request is provided and the already established server connection
//! can be used for the next request, you may gain some performance.
//!
.Query delete_url(string|Standards.URI url,
		  void|mapping(string:int|string) query_variables,
		  void|mapping(string:string|array(string)) request_headers,
		  void|Protocols.HTTP.Query con)
{
  return do_method("DELETE", url, query_variables, request_headers, con);
}

//! Returns an array of @expr{({content_type, data})@} after calling
//! the requested server for the information. @expr{0@} is returned
//! upon failure. Redirects (HTTP 302) are automatically followed.
//!
array(string) get_url_nice(string|Standards.URI url,
			   void|mapping(string:int|string) query_variables,
			   void|mapping(string:string|array(string)) request_headers,
			   void|Protocols.HTTP.Query con)
{
  .Query c;
  multiset seen = (<>);
  do {
    if(!url) return 0;
    if(seen[url] || sizeof(seen)>1000) return 0;
    seen[url]=1;
    c = get_url(url, query_variables, request_headers, con);
    if(!c) return 0;
    if(c->status==302) url = c->headers->location;
  } while( c->status!=200 );
  return ({ c->headers["content-type"], c->data() });
}

//! Returns the returned data after calling the requested server for
//! information through HTTP GET. @expr{0@} is returned upon failure.
//! Redirects (HTTP 302) are automatically followed.
//!
string get_url_data(string|Standards.URI url,
		    void|mapping(string:int|string) query_variables,
		    void|mapping(string:string|array(string)) request_headers,
		    void|Protocols.HTTP.Query con)
{
  array(string) z = get_url_nice(url, query_variables, request_headers, con);
  return z && z[1];
}

//! Similar to @[get_url], except that query variables is sent as a
//! POST request instead of a GET request.
.Query post_url(string|Standards.URI url,
		mapping(string:int|string) query_variables,
		void|mapping(string:string|array(string)) request_headers,
		void|Protocols.HTTP.Query con)
{
  return do_method("POST", url, 0,
		   (request_headers||([]))|
		   (["content-type":
		     "application/x-www-form-urlencoded"]),
		   con,
		   http_encode_query(query_variables));
}

//! Similar to @[get_url_nice], except that query variables is sent as
//! a POST request instead of a GET request.
array(string) post_url_nice(string|Standards.URI url,
			    mapping(string:int|string) query_variables,
			    void|mapping(string:string|array(string)) request_headers,
			    void|Protocols.HTTP.Query con)
{
  .Query c = post_url(url, query_variables, request_headers, con);
  return c && ({ c->headers["content-type"], c->data() });
}

//! Similar to @[get_url_data], except that query variables is sent as
//! a POST request instead of a GET request.
string post_url_data(string|Standards.URI url,
		     mapping(string:int|string) query_variables,
		     void|mapping(string:string|array(string)) request_headers,
		     void|Protocols.HTTP.Query con)
{
  .Query z = post_url(url, query_variables, request_headers, con);
  return z && z->data();
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

// RFC 1738, 2.2. URL Character Encoding Issues
static constant url_non_corresponding = enumerate(0x21) +
  enumerate(0x81,1,0x7f);
static constant url_unsafe = ({ '<', '>', '"', '#', '%', '{', '}',
				'|', '\\', '^', '~', '[', ']', '`' });
static constant url_reserved = ({ ';', '/', '?', ':', '@', '=', '&' });

// Encode these chars
static constant url_chars = url_non_corresponding + url_unsafe +
  url_reserved + ({ '+', '\'' });
static constant url_from = sprintf("%c", url_chars[*]);
static constant url_to   = sprintf("%%%02x", url_chars[*]);


//!	This protects all odd - see @[http_encode_query()] - 
//!	characters for transfer in HTTP.
//!
//!	Do not use this function to protect URLs, since
//!	it will protect URL characters like @expr{'/'@} and @expr{'?'@}.
//! @param in
//!     The string to encode
//! @returns
//!     The HTTP encoded string
string http_encode_string(string in)
{
  return replace(in, url_from, url_to);
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
	 "\177",
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
	 "%7f",
	 "%80", "%81", "%82", "%83", "%84", "%85", "%86", "%87",
	 "%88", "%89", "%8a", "%8b", "%8c", "%8d", "%8e", "%8f",
	 "%90", "%91", "%92", "%93", "%94", "%95", "%96", "%97",
	 "%98", "%99", "%9a", "%9b", "%9c", "%9d", "%9e", "%9f",
	 "%20", "%25", "%27", "%22", "%2c", "%3b", "%3d", "%3a" }));
}

// --- Compatibility code

//! Helper function for replacing HTML entities with the corresponding
//! unicode characters.
//! @deprecated Parser.parse_html_entities
string unentity(string s)
{
  return master()->resolv("Parser.parse_html_entities")(s,1);
}
