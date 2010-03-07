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
		 void|mapping(string:int|string|array(string)) query_variables,
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

protected .Query do_udp_method(string method, Standards.URI url,
			    void|mapping(string:int|string|array(string)) query_variables,
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
		     void|mapping(string:int|string|array(string)) query_variables,
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

  // Reset the state of con.
  con->errno = 0;
  con->ok = 0;
  con->headers = 0;
  con->protocol = 0;
  con->status = 0;
  con->status_desc = 0;
  con->data_timeout = 120;
  con->timeout = 120;
  if (con->ssl)
    con->ssl = 0;
  con->con = 0;
  con->request = 0;
  con->buf = "";
  con->headerbuf = "";
  con->datapos = 0;
  con->discarded_bytes = 0;
  // con->conthread = 0;

  con->async_request(url->host, url->port,
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
	       void|mapping(string:int|string|array(string)) query_variables,
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
	       void|mapping(string:int|string|array(string)) query_variables,
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
		  void|mapping(string:int|string|array(string)) query_variables,
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
			   void|mapping(string:int|string|array(string)) query_variables,
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
		    void|mapping(string:int|string|array(string)) query_variables,
		    void|mapping(string:string|array(string)) request_headers,
		    void|Protocols.HTTP.Query con)
{
  array(string) z = get_url_nice(url, query_variables, request_headers, con);
  return z && z[1];
}

//! Similar to @[get_url], except that query variables is sent as a
//! POST request instead of a GET request.  If query_variables is a
//! simple string, it is assumed to contain the verbatim
//! body of the POST request; Content-Type must be properly specified
//! manually, in this case.
.Query post_url(string|Standards.URI url,
	mapping(string:int|string|array(string))|string query_variables,
		void|mapping(string:string|array(string)) request_headers,
		void|Protocols.HTTP.Query con)
{
  return do_method("POST", url, 0, stringp(query_variables) ? request_headers
		   : (request_headers||([]))|
		   (["content-type":
		     "application/x-www-form-urlencoded"]),
		   con,
		   stringp(query_variables) ? query_variables
		    : http_encode_query(query_variables));
}

//! Similar to @[get_url_nice], except that query variables is sent as
//! a POST request instead of a GET request.
array(string) post_url_nice(string|Standards.URI url,
			    mapping(string:int|string|array(string))|string query_variables,
			    void|mapping(string:string|array(string)) request_headers,
			    void|Protocols.HTTP.Query con)
{
  .Query c = post_url(url, query_variables, request_headers, con);
  return c && ({ c->headers["content-type"], c->data() });
}

//! Similar to @[get_url_data], except that query variables is sent as
//! a POST request instead of a GET request.
string post_url_data(string|Standards.URI url,
		     mapping(string:int|string|array(string))|string query_variables,
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
string http_encode_query(mapping(string:int|string|array(string)) variables)
{
   return Array.map((array)variables,
		    lambda(array(string|int|array(string)) v)
		    {
		       if (intp(v[1]))
			  return uri_encode(v[0]);
		       if (arrayp(v[1]))
			 return map(v[1], lambda (string val) {
					    return 
					      uri_encode(v[0])+"="+
					      uri_encode(val);
					  })*"&";
		       return uri_encode(v[0])+"="+ uri_encode(v[1]);
		    })*"&";
}

protected local constant gen_delims = ":/?#[]@" // RFC 3986, section 2.2
  // % is not part of the gen-delims set, but it effectively must be
  // treated as a reserved character wrt encoding and decoding.
  "%";

protected local constant sub_delims = "!$&'()*+,;="; // RFC 3986, section 2.2

// US-ASCII chars that are neither reserved nor unreserved in RFC 3986.
protected local constant other_chars =
  (string) enumerate (0x20) + "\x7f" // Control chars
  " \"<>\\^`{|}";

protected local constant eight_bit_chars = (string) enumerate (0x80, 1, 0x80);

string percent_encode (string s)
//! Encodes the given string using @tt{%XX@} encoding, except that URI
//! unreserved chars are not encoded. The unreserved chars are
//! @tt{A-Z@}, @tt{a-z@}, @tt{0-9@}, @tt{-@}, @tt{.@}, @tt{_@}, and
//! @tt{~@} (see RFC 2396 section 2.3).
//!
//! 8-bit chars are encoded straight, and wider chars are not allowed.
//! That means this encoding is applicable if @[s] is a binary octet
//! string. If it is a character string then @[uri_encode] should be
//! used instead.
//!
//! It is also slightly faster than @[uri_encode] if @[s] is known to
//! contain only US-ASCII.
{
  constant replace_chars = (gen_delims + sub_delims +
			    other_chars + eight_bit_chars);
  return replace (s,
		  // The [*] syntax is hideous, but lambdas currently
		  // don't work in constant expressions. :P
		  sprintf ("%c", ((array(int)) replace_chars)[*]),
		  // RFC 3986, 2.1: "For consistency, URI producers
		  // and normalizers should use uppercase hexadecimal
		  // digits for all percent- encodings."
		  sprintf ("%%%02X", ((array(int)) replace_chars)[*]));
}

string percent_decode (string s)
//! Decodes URI-style @tt{%XX@} encoded chars in the given string.
//!
//! @seealso
//! @[percent_encode], @[uri_decode]
//!
//! @bugs
//! This function currently does not accept wide string input, which
//! is necessary to work as the reverse of @[iri_encode].
{
  return _Roxen.http_decode_string (s);
}

string uri_encode (string s)
//! Encodes the given string using @tt{%XX@} encoding to be used as a
//! component part in a URI. This means that all URI reserved and
//! excluded characters are encoded, i.e. everything except @tt{A-Z@},
//! @tt{a-z@}, @tt{0-9@}, @tt{-@}, @tt{.@}, @tt{_@}, and @tt{~@} (see
//! RFC 2396 section 2.3).
//!
//! 8-bit chars and wider are encoded using UTF-8 followed by
//! percent-encoding. This follows RFC 3986 section 2.5, the
//! IRI-to-URI conversion method in the IRI standard (RFC 3987) and
//! appendix B.2 in the HTML 4.01 standard. It should work regardless
//! of the charset used in the XML document the URI might be inserted
//! into.
//!
//! @seealso
//! @[uri_decode], @[uri_encode_invalids], @[iri_encode]
{
  return percent_encode (string_to_utf8 (s));
}

string uri_encode_invalids (string s)
//! Encodes all "dangerous" chars in the given string using @tt{%XX@}
//! encoding, so that it can be included as a URI in an HTTP message
//! or header field. This includes control chars, space and various
//! delimiter chars except those in the URI @tt{reserved@} set (RFC
//! 2396 section 2.2).
//!
//! Since this function doesn't touch the URI @tt{reserved@} chars nor
//! the escape char @tt{%@}, it can be used on a complete formatted
//! URI or IRI.
//!
//! 8-bit chars and wider are encoded using UTF-8 followed by
//! percent-encoding. This follows RFC 3986 section 2.5, the IRI
//! standard (RFC 3987) and appendix B.2 in the HTML 4.01 standard.
//!
//! @note
//! The characters in the URI @tt{reserved@} set are: @tt{:@},
//! @tt{/@}, @tt{?@}, @tt{#@}, @tt{[@}, @tt{]@}, @tt{@@@}, @tt{!@},
//! @tt{$@}, @tt{&@}, @tt{'@}, @tt{(@}, @tt{)@}, @tt{*@}, @tt{+@},
//! @tt{,@}, @tt{;@}, @tt{=@}. In addition, this function doesn't
//! touch the escape char @tt{%@}.
//!
//! @seealso
//! @[uri_decode], @[uri_encode]
{
  constant replace_chars = other_chars + eight_bit_chars;
  return replace (string_to_utf8 (s),
		  sprintf ("%c", ((array(int)) replace_chars)[*]),
		  sprintf ("%%%02X", ((array(int)) replace_chars)[*]));
}

string uri_decode (string s)
//! Decodes URI-style @tt{%XX@} encoded chars in the given string, and
//! then UTF-8 decodes the result. This is the reverse of
//! @[uri_encode] and @[uri_encode_invalids].
//!
//! @seealso
//! @[uri_encode], @[uri_encode_invalids]
{
  // Note: This currently does not quite work for URI-to-IRI
  // conversion according to RFC 3987 section 3.2. Most importantly
  // any invalid utf8-sequences should be left percent-encoded in the
  // result.
  return utf8_to_string (_Roxen.http_decode_string (s));
}

string iri_encode (string s)
//! Encodes the given string using @tt{%XX@} encoding to be used as a
//! component part in an IRI (Internationalized Resource Identifier,
//! see RFC 3987). This means that all chars outside the IRI
//! @tt{iunreserved@} set are encoded, i.e. this function encodes
//! equivalently to @[uri_encode] except that all 8-bit and wider
//! characters are left as-is.
//!
//! @bugs
//! This function currently does not encode chars in the Unicode
//! private ranges, although that is strictly speaking required in
//! some but not all IRI components. That could change if it turns out
//! to be a problem.
//!
//! @seealso
//! @[percent_decode], @[uri_encode]
{
  constant replace_chars = gen_delims + sub_delims + other_chars;
  return replace (s,
		  sprintf ("%c", ((array(int)) replace_chars)[*]),
		  sprintf ("%%%02X", ((array(int)) replace_chars)[*]));
}

#if 0
// These functions are disabled since I haven't found a way to
// implement them even remotely efficiently using pike only. /mast

string uri_normalize (string s)
//! Normalizes the URI-style @tt{%XX@} encoded string @[s] by decoding
//! all URI @tt{unreserved@} chars, i.e. US-ASCII digits, letters,
//! @tt{-@}, @tt{.@}, @tt{_@}, and @tt{~@}.
//!
//! Since only unreserved chars are decoded, the result is always
//! semantically equivalent to the input. It's therefore safe to use
//! this on a complete formatted URI.
//!
//! @seealso
//! @[uri_decode], @[uri_encode], @[iri_normalize]
{
  // FIXME
}

string iri_normalize (string s)
//! Normalizes the IRI-style UTF-8 and @tt{%XX@} encoded string @[s]
//! by decoding all IRI @tt{unreserved@} chars, i.e. everything except
//! the URI @tt{reserved@} chars and control chars.
//!
//! Since only unreserved chars are decoded, the result is always
//! semantically equivalent to the input. It's therefore safe to use
//! this on a complete formatted IRI.
//!
//! @seealso
//! @[iri_decode], @[uri_normalize]
{
  // FIXME
}

#endif

string quoted_string_encode (string s)
//! Encodes the given string quoted to be used as content inside a
//! @tt{quoted-string@} according to RFC 2616 section 2.2. The
//! returned string does not include the surrounding @tt{"@} chars.
//!
//! @note
//! The @tt{quoted-string@} quoting rules in RFC 2616 have several
//! problems:
//!
//! @ul
//! @item
//!   Quoting is inconsistent since @tt{"@} is quoted as @tt{\"@}, but
//!   @tt{\@} does not need to be quoted. This is resolved in the HTTP
//!   bis update to mandate quoting of @tt{\@} too, which this
//!   function performs.
//!
//! @item
//!   Many characters are not quoted sufficiently to make the result
//!   safe to use in an HTTP header, so this quoting is not enough if
//!   @[s] contains NUL, CR, LF, or any 8-bit or wider character.
//! @endul
//!
//! @seealso
//! @[quoted_string_decode]
{
  return replace (s, (["\"": "\\\"", "\\": "\\\\"]));
}

string quoted_string_decode (string s)
//! Decodes the given string which has been encoded as a
//! @tt{quoted-string@} according to RFC 2616 section 2.2. @[s] is
//! assumed to not include the surrounding @tt{"@} chars.
//!
//! @seealso
//! @[quoted_string_encode]
{
  return map (s / "\\\\", replace, "\\", "") * "\\";
}

// --- Compatibility code

__deprecated__ string http_encode_string(string in)
//! This is a deprecated alias for @[uri_encode], for compatibility
//! with Pike 7.6 and earlier.
//!
//! In 7.6 this function didn't handle 8-bit and wider chars
//! correctly. It encoded 8-bit chars directly to @tt{%XX@} escapes,
//! and it used nonstandard @tt{%uXXXX@} escapes for 16-bit chars.
//!
//! That is considered a bug, and hence the function is changed. If
//! you need the old buggy encoding then use the 7.6 compatibility
//! version (@expr{#pike 7.6@}).
//!
//! @deprecated uri_encode
{
  return uri_encode (in);
}

//! This function used to claim that it encodes the specified string
//! according to the HTTP cookie standard. If fact it does not - it
//! applies URI-style (i.e. @expr{%XX@}) encoding on some of the
//! characters that cannot occur literally in cookie values. There
//! exist some web servers (read Roxen and forks) that usually perform
//! a corresponding nonstandard decoding of %-style escapes in cookie
//! values in received requests.
//!
//! This function is deprecated. The function @[quoted_string_encode]
//! performs encoding according to the standard, but it is not safe to
//! use with arbitrary chars. Thus URI-style encoding using
//! @[uri_encode] or @[percent_encode] is normally a good choice, if
//! you can use @[uri_decode]/@[percent_decode] at the decoding end.
//!
//! @deprecated
__deprecated__ string http_encode_cookie(string f)
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

//! Helper function for replacing HTML entities with the corresponding
//! unicode characters.
//! @deprecated Parser.parse_html_entities
__deprecated__ string unentity(string s)
{
  return master()->resolv("Parser.parse_html_entities")(s,1);
}
