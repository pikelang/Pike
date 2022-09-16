#pike __REAL_VERSION__

//! HTTP Status codes.
//!
//! @seealso
//!   @[response_codes],
//!   @url{https://www.iana.org/assignments/http-status-codes/http-status-codes.txt@}
enum StatusCode {
  // 1xx: Informational - Request received, continuing process
  HTTP_CONTINUE			= 100, //! @rfc{2616:10.1.1@}: 100 Continue
  HTTP_SWITCH_PROT		= 101, //! @rfc{2616:10.1.2@}: 101 Switching protocols
  DAV_PROCESSING		= 102, //! @rfc{2518:10.1@}: 102 Processing
  HTTP_EARLY_HINTS		= 103, //! @rfc{8297:2@}: 103 Early Hints

  // 2xx: Success - The action was successfully received, understood, and accepted
  HTTP_OK			= 200, //! @rfc{2616:10.2.1@}: 200 OK
  HTTP_CREATED			= 201, //! @rfc{2616:10.2.2@}: 201 Created
  HTTP_ACCEPTED			= 202, //! @rfc{2616:10.2.3@}: 202 Accepted
  HTTP_NONAUTHORATIVE		= 203, //! @rfc{2616:10.2.4@}: 203 Non-Authorative Information
  HTTP_NO_CONTENT		= 204, //! @rfc{2616:10.2.5@}: 204 No Content
  HTTP_RESET_CONTENT		= 205, //! @rfc{2616:10.2.6@}: 205 Reset Content
  HTTP_PARTIAL_CONTENT		= 206, //! @rfc{2616:10.2.7@}: 206 Partial Content
  DAV_MULTISTATUS		= 207, //! @rfc{2518:10.2@}: 207 Multi-Status
  DAV_ALREADY_REPORTED		= 208, //! @rfc{5842:7.1@}: 208 Already Reported
  DELTA_HTTP_IM_USED		= 226, //! @rfc{3229:10.4.1@}: 226 IM Used

  // 3xx: Redirection - Further action must be taken in order to complete the request
  HTTP_MULTIPLE			= 300, //! @rfc{2616:10.3.1@}: 300 Multiple Choices
  HTTP_MOVED_PERM		= 301, //! @rfc{2616:10.3.2@}: 301 Moved Permanently
  HTTP_FOUND			= 302, //! @rfc{2616:10.3.3@}: 302 Found
  HTTP_SEE_OTHER		= 303, //! @rfc{2616:10.3.4@}: 303 See Other
  HTTP_NOT_MODIFIED		= 304, //! @rfc{2616:10.3.5@}: 304 Not Modified
  HTTP_USE_PROXY		= 305, //! @rfc{2616:10.3.6@}: 305 Use Proxy
  // @rfc{2616:10.3.7@}: 306 not used but reserved.
  HTTP_TEMP_REDIRECT		= 307, //! @rfc{2616:10.3.8@}: 307 Temporary Redirect
  HTTP_PERM_REDIRECT		= 308, //! @rfc{7538:3@}: 308 Permanent Redirect

  // 4xx: Client Error - The request contains bad syntax or cannot be fulfilled
  HTTP_BAD			= 400, //! @rfc{2616:10.4.1@}: 400 Bad Request
  HTTP_UNAUTH			= 401, //! @rfc{2616:10.4.2@}: 401 Unauthorized
  HTTP_PAY			= 402, //! @rfc{2616:10.4.3@}: 402 Payment Required
  HTTP_FORBIDDEN		= 403, //! @rfc{2616:10.4.4@}: 403 Forbidden
  HTTP_NOT_FOUND		= 404, //! @rfc{2616:10.4.5@}: 404 Not Found
  HTTP_METHOD_INVALID		= 405, //! @rfc{2616:10.4.6@}: 405 Method Not Allowed
  HTTP_NOT_ACCEPTABLE		= 406, //! @rfc{2616:10.4.7@}: 406 Not Acceptable
  HTTP_PROXY_AUTH_REQ		= 407, //! @rfc{2616:10.4.8@}: 407 Proxy Authentication Required
  HTTP_TIMEOUT			= 408, //! @rfc{2616:10.4.9@}: 408 Request Timeout
  HTTP_CONFLICT			= 409, //! @rfc{2616:10.4.10@}: 409 Conflict
  HTTP_GONE			= 410, //! @rfc{2616:10.4.11@}: 410 Gone
  HTTP_LENGTH_REQ		= 411, //! @rfc{2616:10.4.12@}: 411 Length Required
  HTTP_PRECOND_FAILED		= 412, //! @rfc{2616:10.4.13@}: 412 Precondition Failed
  HTTP_REQ_TOO_LARGE		= 413, //! @rfc{2616:10.4.14@}: 413 Request Entity Too Large
  HTTP_URI_TOO_LONG		= 414, //! @rfc{2616:10.4.15@}: 414 Request-URI Too Long
  HTTP_UNSUPP_MEDIA		= 415, //! @rfc{2616:10.4.16@}: 415 Unsupported Media Type
  HTTP_BAD_RANGE		= 416, //! @rfc{2616:10.4.17@}: 416 Requested Range Not Satisfiable
  HTTP_EXPECT_FAILED		= 417, //! @rfc{2616:10.4.18@}: 417 Expectation Failed
  HTCPCP_TEAPOT			= 418, //! @rfc{2324:2.3.2@}: 418 I'm a teapot
  HTTP_MISDIRECTED_REQ		= 421, //! @rfc{7540:9.1.2@}: 421 Misdirected Request
  DAV_UNPROCESSABLE		= 422, //! @rfc{2518:10.3@}: 422 Unprocessable Entry
  DAV_LOCKED			= 423, //! @rfc{2518:10.4@}: 423 Locked
  DAV_FAILED_DEP		= 424, //! @rfc{2518:10.5@}: 424 Failed Dependency
  TLS_TOO_EARLY			= 425, //! @rfc{8470:5.2@}: 425 Too Early
  TLS_UPGRADE_REQUIRED		= 426, //! @rfc{2817:4.2@}: 426 Upgrade Required
  HTTP_PRECOND_REQUIRED		= 428, //! @rfc{6585:3@}: 428 Precondition required
  HTTP_TOO_MANY_REQUESTS	= 429, //! @rfc{6585:4@}: 429 Too Many Requests
  HTTP_HEADERS_TOO_LARGE	= 431, //! @rfc{6585:5@}: 431 Request Header Fields Too Large

  HTTP_LEGALLY_RESTRICTED	= 451, //! @rfc{7725:3@}: 451 Unavailable For Legal Reasons

  // 5xx: Server Error - The server failed to fulfill an apparently valid request
  HTTP_INTERNAL_ERR		= 500, //! @rfc{2616:10.5.1@}: 500 Internal Server Error
  HTTP_NOT_IMPL			= 501, //! @rfc{2616:10.5.2@}: 501 Not Implemented
  HTTP_BAD_GW			= 502, //! @rfc{2616:10.5.3@}: 502 Bad Gateway
  HTTP_UNAVAIL			= 503, //! @rfc{2616:10.5.4@}: 503 Service Unavailable
  HTTP_GW_TIMEOUT		= 504, //! @rfc{2616:10.5.5@}: 504 Gateway Timeout
  HTTP_UNSUPP_VERSION		= 505, //! @rfc{2616:10.5.6@}: 505 HTTP Version Not Supported
  TCN_VARIANT_NEGOTIATES	= 506, //! @rfc{2295:8.1@}: 506 Variant Also Negotiates
  DAV_STORAGE_FULL		= 507, //! @rfc{2518:10.6@}: 507 Insufficient Storage
  DAV_LOOP_DETECTED		= 508, //! @rfc{5842:7.2@}: 508 Loop Detected
  HTTP_NOT_EXTENDED		= 510, //! @rfc{2774:7@}: 510 Not Extended (obsolete)
  HTTP_NET_AUTH_REQUIRED	= 511, //! @rfc{6585:6@}: 511 Network Authentication Required
};

//! Mapping from @[StatusCode] to descriptive string.
//!
//! @seealso
//!   @[StatusCode]
constant response_codes =
([
  // Informational
  100:"100 Continue",
  101:"101 Switching Protocols",
  102:"102 Processing", // WebDAV
  103:"103 Early Hints",
  122:"122 Request-URI too long", // a non standard IE7 error

  // Successful
  200:"200 OK",
  201:"201 Created, URI follows",
  202:"202 Accepted",
  203:"203 Non-Authoritative Information",
  204:"204 No Content",
  205:"205 Reset Content",
  206:"206 Partial Content", // Byte ranges
  207:"207 Multi-Status", // WebDAV
  208:"208 Already Reported", // WebDAV
  226:"226 IM Used", // RFC 3229

  // Redirection
  300:"300 Moved",
  301:"301 Permanent Relocation",
  302:"302 Found", // a potential alligator swamp. for HTTP/1.1, use 303/307.
  303:"303 See Other", // temporary redirect, any POST data is received, use GET.
  304:"304 Not Modified",
  305:"305 Use Proxy",
  306:"306 Switch Proxy", // Deprecated
  307:"307 Temporary Redirect", // retry request elsewhere, don't change method.
  308:"308 Permanent Redirect",

  // Client Error
  400:"400 Bad Request",
  401:"401 Access denied",
  402:"402 Payment Required",
  403:"403 Forbidden",
  404:"404 No such file or directory.",
  405:"405 Method not allowed",
  406:"406 Not Acceptable",
  407:"407 Proxy authorization needed",
  408:"408 Request timeout",
  409:"409 Conflict",
  410:"410 Gone",
  411:"411 Length Required",
  412:"412 Precondition Failed",
  413:"413 Request Entity Too Large",
  414:"414 Request-URI Too Large",
  415:"415 Unsupported Media Type",
  416:"416 Requested range not statisfiable",
  417:"417 Expectation Failed",
  418:"418 I'm a teapot", // Ha ha
  421:"421 Misdirected Request",
  422:"422 Unprocessable Entity", // WebDAV
  423:"423 Locked", // WebDAV
  424:"424 Failed Dependency", // WebDAV
  425:"425 Too Early",
  426:"426 Upgrade Required", // RFC2817
  428:"428 Precondition Required",
  429:"429 Too Many Requests",
  431:"431 Request Header Fields Too Large",
  451:"451 Unavailable For Legal Reasons", // RFC7725

  // Internal Server Errors
  500:"500 Internal Server Error.",
  501:"501 Not Implemented",
  502:"502 Bad Gateway",
  503:"503 Service unavailable",
  504:"504 Gateway Timeout",
  505:"505 HTTP Version Not Supported",
  506:"506 Variant Also Negotiates", // RFC2295
  507:"507 Insufficient Storage", // WebDAV / RFC4918
  508:"508 Loop Detected", // WebDAV / RFC5842
  509:"509 Bandwidth Limit Exceeded", // An Apache defined extension in popular use
  510:"510 Not Extended", // RFC2774
  511:"511 Network Authentication Required",
  598:"598 Network read timeout error", // Informal extension used by some HTTP proxies
  599:"599 Network connect timeout error", // Informal extension used by some HTTP proxies
]);


//! Makes an HTTP request through a proxy.
//!
//! @param proxy
//!   URL for the proxy.
//!
//! @param user
//! @param password
//!   Proxy authentication credentials.
//!
//! @param method
//! @param url
//! @param query_variables
//! @param request_headers
//! @param con
//! @param data
//!   The remaining arguments are identical to @[do_method()].
//!
//! @seealso
//!   @[do_method()], @[do_async_proxied_method()]
.Query do_proxied_method(string|Standards.URI proxy,
                         string user, string password,
                         string method,
                         string|Standards.URI url,
                         void|mapping(string:int|string|array(string)) query_variables,
                         void|mapping(string:string|array(string)|int) request_headers,
                         void|Protocols.HTTP.Query con, void|string data)
{
  if (!proxy || (proxy == "")) {
    return do_method(method, url, query_variables, request_headers, con, data);
  }
  // Make sure we don't propagate our changes to the
  // url and proxy objects below to the caller.
  proxy = Standards.URI(proxy);
  url = Standards.URI(url);

  mapping(string:string|array(string)) proxy_headers = ([]);

  if( user || password )
  {
    proxy_headers["Proxy-Authorization"] = "Basic "
      + MIME.encode_base64((user || "") + ":" + (password || ""), 1);
  }
  if (has_value(proxy->host, ":")) {
    proxy_headers["host"] = "[" + proxy->host + "]:" + proxy->port;
  } else {
    proxy_headers["host"] = proxy->host + ":" + proxy->port;
  }

  if (url->scheme == "http") {
    if( query_variables )
      url->set_query_variables( url->get_query_variables() +
				query_variables );
    string web_url = (string)url;

    // Note: url object is wrecked here
    url->scheme = proxy->scheme;
    url->host = proxy->host;
    url->port = proxy->port;
    query_variables = url->query = 0;
    url->path = web_url;

    if (request_headers) {
      proxy_headers = request_headers + proxy_headers;
    }
  } else if (url->scheme == "https") {
#ifdef HTTP_QUERY_DEBUG
    werror("Proxied SSL request.\n");
#endif
    if (!con || (con->host != url->host) || (con->port != url->port)) {
      // Make a CONNECT request to the proxy,
      // and use keep-alive to stack the real request on top.
      proxy->path = url->host + ":" + url->port;
      proxy_headers->connection = "keep-alive";
      con = do_method("CONNECT", proxy, 0, proxy_headers);
      con->data(0);
      if (con->status >= 300) {
	// Proxy did not like us or failed to connect to the remote.
	return con;
      }
      con->headers["connection"] = "keep-alive";
      con->headers["content-length"] = "0";
      con->host = url->host;
      con->port = url->port;
      con->https = 1;
      con->start_tls(1);
    }
    proxy_headers = request_headers;
  } else {
    error("Can't handle proxying of %O.\n", url->scheme);
  }

  return do_method(method, url, query_variables, proxy_headers, con, data);
}

//! Low level HTTP call method.
//!
//! @param method
//!   The HTTP method to use, e.g. @expr{"GET"@}.
//! @param url
//!   The URL to perform @[method] on. Should be a complete URL,
//!   including protocol, e.g. @expr{"https://pike.lysator.liu.se/"@}.
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
object(.Query)|zero
  do_method(string method,
            string|Standards.URI url,
            void|mapping(string:int|string|array(string)) query_variables,
            void|mapping(string:string|array(string)|int) request_headers,
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

  if(url->scheme!="http" && url->scheme!="https")
    error("Can't handle %O or any other protocols than HTTP or HTTPS.\n",
	  url->scheme);

  con->https = (url->scheme=="https")? 1 : 0;

  mapping default_headers = ([
    "user-agent" : "Mozilla/5.0 (compatible; MSIE 6.0; Pike HTTP client)"
    " Pike/" + __REAL_MAJOR__ + "." + __REAL_MINOR__ + "." + __REAL_BUILD__,
    "host" : url->host +
    (url->port!=(url->scheme=="https"?443:80)?":"+url->port:"")]);

  if(url->user || url->password)
    default_headers->authorization = "Basic "
				   + MIME.encode_base64(url->user + ":" +
							(url->password || ""),
                                                        1);

  if(!request_headers)
    request_headers = default_headers;
  else
    request_headers = default_headers |
      mkmapping(lower_case(indices(request_headers)[*]),
                values(request_headers));

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
      error ("I/O error: %s.\n", strerror (con->errno));
    return 0;
  }
  return con;
}

protected .Query do_udp_method(string method, Standards.URI url,
			    void|mapping(string:int|string|array(string)) query_variables,
			    void|mapping(string:string|array(string)|int)
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
    if( !catch( udp->bind(port++, 0, 1) ) ) break;
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
//!   including protocol, e.g. @expr{"https://pike.lysator.liu.se/"@}.
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
		     void|mapping(string:string|array(string)|int) request_headers,
		     Protocols.HTTP.Query con, void|string data)
{
  if(stringp(url))
    url=Standards.URI(url);

  if( (< "httpu", "httpmu" >)[url->scheme] ) {
    error("Asynchronous httpu or httpmu not yet supported.\n");
  }

  if(url->scheme!="http" && url->scheme!="https")
    error("Can't handle %O or any other protocols than HTTP or HTTPS.\n",
	  url->scheme);

  con->https = (url->scheme=="https")? 1 : 0;

  if(!request_headers)
    request_headers = ([]);
  mapping default_headers = ([
    "user-agent" : "Mozilla/5.0 (compatible; MSIE 6.0; Pike HTTP client)"
    " Pike/" + __REAL_MAJOR__ + "." + __REAL_MINOR__ + "." + __REAL_BUILD__,
    "host" : url->host +
    (url->port!=(url->scheme=="https"?443:80)?":"+url->port:"")]);

  if(url->user || url->password)
    default_headers->authorization = "Basic "
				   + MIME.encode_base64(url->user + ":" +
							(url->password || ""),
                                                        1);
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

  con->async_request(url->host, url->port,
		     method+" "+path+(query?("?"+query):"")+" HTTP/1.0",
		     request_headers, data);
}

protected void https_proxy_connect_fail(Protocols.HTTP.Query con,
					array(mixed) orig_cb_info,
					Standards.URI url, string method,
					mapping(string:string) query_variables,
					mapping(string:string) request_headers,
					string data)
{
  con->set_callbacks(@orig_cb_info);
  con->request_fail(con, @con->extra_args);
}

protected void https_proxy_connect_ok(Protocols.HTTP.Query con,
				      array(mixed) orig_cb_info,
				      Standards.URI url, string method,
				      mapping(string:string) query_variables,
				      mapping(string:string) request_headers,
				      string data)
{
  con->set_callbacks(@orig_cb_info);
  if (con->status >= 300) {
    // Proxy did not like us or failed to connect to the remote.
    // Return the failure message.
    if (con->request_ok) {
      con->request_ok(con, @con->extra_args);
    }
    return;
  }

  // Install the timeout handler for the interval until
  // the TLS connection is up.
  con->init_async_timeout();

  con->headers["connection"] = "keep-alive";
  con->headers["content-length"] = "0";
  con->host = url->host;
  con->port = url->port;
  con->https = 1;
  con->start_tls(0);

  con->con->set_nonblocking(0,
			    lambda() {
			      // Remove the timeout handler; it will be
			      // reinstated by do_async_method() below.
			      con->remove_async_timeout();
			      do_async_method(method, url, query_variables,
					      request_headers, con, data);
			    }, con->async_failed);
}

//! Low level asynchronous proxied HTTP call method.
//!
//! Makes an HTTP request through a proxy.
//!
//! @param proxy
//!   URL for the proxy.
//!
//! @param user
//! @param password
//!   Proxy authentication credentials.
//!
//! @param method
//!   The HTTP method to use, e.g. @expr{"GET"@}.
//! @param url
//!   The URL to perform @[method] on. Should be a complete URL,
//!   including protocol, e.g. @expr{"https://pike.lysator.liu.se/"@}.
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
//!   @[do_async_method()], @[do_proxied_method()], @[Query.set_callbacks()]
void do_async_proxied_method(string|Standards.URI proxy,
			     string user, string password,
			     string method,
			     string|Standards.URI url,
			     void|mapping(string:int|string|array(string)) query_variables,
			     void|mapping(string:string|array(string)|int) request_headers,
			     Protocols.HTTP.Query con, void|string data)
{
  if (!proxy || (proxy == "")) {
    do_async_method(method, url, query_variables, request_headers, con, data);
    return;
  }
  // Make sure we don't propagate our changes to the
  // url and proxy objects below to the caller.
  proxy = Standards.URI(proxy);
  url = Standards.URI(url);

  if( (< "httpu", "httpmu" >)[url->scheme] ) {
    error("Asynchronous httpu or httpmu not yet supported.\n");
  }

  mapping(string:string|array(string)) proxy_headers = ([]);

  if( user || password )
  {
    proxy_headers["Proxy-Authorization"] = "Basic "
      + MIME.encode_base64((user || "") + ":" + (password || ""), 1);
  }
  if (has_value(proxy->host, ":")) {
    proxy_headers["host"] = "[" + proxy->host + "]:" + proxy->port;
  } else {
    proxy_headers["host"] = proxy->host + ":" + proxy->port;
  }

  if (url->scheme == "http") {
    if( query_variables )
      url->set_query_variables( url->get_query_variables() +
				query_variables );
    string web_url = (string)url;

    // Note: url object is wrecked here
    url->scheme = proxy->scheme;
    url->host = proxy->host;
    url->port = proxy->port;
    query_variables = url->query = 0;
    url->path = web_url;

    if (request_headers) {
      proxy_headers = request_headers + proxy_headers;
    }
  } else if(url->scheme == "https") {
#ifdef HTTP_QUERY_DEBUG
    werror("Proxied SSL request.\n");
#endif
    if (!con || (con->host != url->host) || (con->port != url->port)) {
      // Make a CONNECT request to the proxy,
      // and use keep-alive to stack the real request on top.
      proxy->path = url->host + ":" + url->port;
      if (!proxy_headers) proxy_headers = ([]);
      proxy_headers->connection = "keep-alive";

      array(mixed) orig_cb_info = ({
	con->request_ok,
	con->request_fail,
	@con->extra_args,
      });
      con->set_callbacks(https_proxy_connect_ok,
			 https_proxy_connect_fail,
			 orig_cb_info,
			 url, method,
			 query_variables,
			 request_headers,
			 data);
      method = "CONNECT";
      url = proxy;
      data = 0;
    } else {
      proxy_headers = request_headers;
    }
  } else {
    error("Can't handle proxying of %O.\n", url->scheme);
  }

  do_async_method(method, url, query_variables, proxy_headers, con, data);
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
	       void|mapping(string:string|array(string)|int) request_headers,
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
	       void|mapping(string:string|array(string)|int) request_headers,
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
		  void|mapping(string:string|array(string)|int) request_headers,
		  void|Protocols.HTTP.Query con)
{
  return do_method("DELETE", url, query_variables, request_headers, con);
}

//! Returns an array of @expr{({content_type, data})@} after calling
//! the requested server for the information. @expr{0@} is returned
//! upon failure. Redirects (HTTP 302) are automatically followed.
//!
array(string)|zero
  get_url_nice(string|Standards.URI url,
               void|mapping(string:int|string|array(string)) query_variables,
               void|mapping(string:string|array(string)|int) request_headers,
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
    if(c->status==302)
      url = Standards.URI(c->headers->location, url);
  } while( c->status!=200 );
  return ({ c->headers["content-type"], c->data() });
}

//! Returns the returned data after calling the requested server for
//! information through HTTP GET. @expr{0@} is returned upon failure.
//! Redirects (HTTP 302) are automatically followed.
//!
string get_url_data(string|Standards.URI url,
		    void|mapping(string:int|string|array(string)) query_variables,
		    void|mapping(string:string|array(string)|int) request_headers,
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
		void|mapping(string:string|array(string)|int) request_headers,
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
			    void|mapping(string:string|array(string)|int) request_headers,
			    void|Protocols.HTTP.Query con)
{
  .Query c = post_url(url, query_variables, request_headers, con);
  return c && ({ c->headers["content-type"], c->data() });
}

//! Similar to @[get_url_data], except that query variables is sent as
//! a POST request instead of a GET request.
string post_url_data(string|Standards.URI url,
		     mapping(string:int|string|array(string))|string query_variables,
		     void|mapping(string:string|array(string)|int) request_headers,
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
//!     > Protocols.HTTP.http_encode_query( (["&amp;":"&","'=\"":"\0\0\0\u0434"]) );
//!     Result: "%27%3D%22=%00%00%00%D0%B4&%26amp%3B=%26"
//!	@}
string http_encode_query(mapping(string:int|string|array(string)) variables)
{
   return map((array)variables,
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
//! @tt{~@} (see @rfc{2396:2.3@}).
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
//! @rfc{2396:2.3@}).
//!
//! 8-bit chars and wider are encoded using UTF-8 followed by
//! percent-encoding. This follows @rfc{3986:2.5@}, the IRI-to-URI
//! conversion method in the IRI standard (@rfc{3987@}) and appendix
//! B.2 in the HTML 4.01 standard. It should work regardless of the
//! charset used in the XML document the URI might be inserted into.
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
//! delimiter chars except those in the URI @tt{reserved@} set
//! (@rfc{2396:2.2@}).
//!
//! Since this function doesn't touch the URI @tt{reserved@} chars nor
//! the escape char @tt{%@}, it can be used on a complete formatted
//! URI or IRI.
//!
//! 8-bit chars and wider are encoded using UTF-8 followed by
//! percent-encoding. This follows @rfc{3986:2.5@}, the IRI standard
//! (@rfc{3987@}) and appendix B.2 in the HTML 4.01 standard.
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
  s = _Roxen.http_decode_string (s);
  catch { s = utf8_to_string (s); };
  return s;
}

string iri_encode (string s)
//! Encodes the given string using @tt{%XX@} encoding to be used as a
//! component part in an IRI (Internationalized Resource Identifier,
//! see @rfc{3987@}). This means that all chars outside the IRI
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

string quoted_string_encode (string s)
//! Encodes the given string quoted to be used as content inside a
//! @tt{quoted-string@} according to @rfc{2616:2.2@}. The returned
//! string does not include the surrounding @tt{"@} chars.
//!
//! @note
//! The @tt{quoted-string@} quoting rules in @rfc{2616@} have several
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
  return MIME.quote(({s}))[1..<1];
}

string quoted_string_decode (string s)
//! Decodes the given string which has been encoded as a
//! @tt{quoted-string@} according to @rfc{2616:2.2@}. @[s] is assumed
//! to not include the surrounding @tt{"@} chars.
//!
//! @seealso
//! @[quoted_string_encode]
{
  return MIME.tokenize("\"" + s + "\"")[0];
}
