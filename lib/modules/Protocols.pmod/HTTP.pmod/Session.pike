#pike __REAL_VERSION__

// $Id: Session.pike,v 1.2 2003/02/15 08:59:39 mirar Exp $

import Protocols.HTTP;

//!	The number of redirects to follow, if any.
//!	This is the default to the created Request objects.
//!
//!	A redirect automatically turns into a GET request,
//!	and all header, query, post or put information is dropped.
//!
//!	Default is 20 redirects. A negative number will mean infinity.
//! @bugs
//!	Loops will currently not be detected, only the limit
//!	works to stop loops.
//! @seealso
//!	@[Request.follow_redirects]

int follow_redirects=20;

class Request
{
//!	Raw connection object
   Query con;

//!	URL requested (set by prepare_method).
//!	This will update according to followed redirects.
   Standards.URI url_requested;

//!	Number of redirects to follow;
//!	the request will perform another request if the 
//!	HTTP answer is a 3xx redirect.
//!	Default from the parent @[Session.follow_redirects].
//!
//!	A redirect automatically turns into a GET request,
//!	and all header, query, post or put information is dropped.
//! @bugs
//!	Loops will currently not be detected, only the limit
//!	works to stop loops.

   int follow_redirects= // parent
      function_object(object_program(this_object()))->follow_redirects;

//!	Cookie callback. When a request is performed,
//!	the result is checked for cookie changes and
//!	additions. If a cookie is encountered, this
//!	function is called. Default is to call 
//!	@[set_http_cookie] in the @[Session] object.

   function(string,Standards.URI:mixed) cookie_encountered=set_http_cookie;

// ----------------

//!	Prepares the HTTP Query object for the connection,
//!	and returns the parameters to use with @[do_sync],
//!	@[do_async] or @[do_thread].
//!
//!	This method will also use cookie information from the 
//!	parent @[Session], and may reuse connections (keep-alive).

   array(string|int|mapping) prepare_method(
      string method,
      string|Standards.URI url,
      void|mapping query_variables,
      void|mapping request_headers,
      void|string data)
   {
      if(stringp(url))
	 url=Standards.URI(url);
      url_requested=url;

      if(!con) con = give_me_connection(url);

      if(!request_headers)
	 request_headers = ([]);
  
  
#if constant(SSL.sslfile) 	
      if(url->scheme!="http" && url->scheme!="https")
	 error("Protocols.HTTP can't handle %O or any other "
	       "protocols than HTTP or HTTPS\n",
	       url->scheme);
  
      con->https= (url->scheme=="https")? 1 : 0;
#else
      if(url->scheme!="http"	)
	 error("Protocols.HTTP can't handle %O or any other "
	       "protocol than HTTP\n",
	       url->scheme);
#endif
  
      if(!request_headers)
	 request_headers = ([]);
      mapping default_headers = ([
	 "user-agent":"Mozilla/5.0 (compatible; MSIE 6.0; Pike HTTP client)"
	    " Pike/"+__REAL_MAJOR__+"."+__REAL_MINOR__+"."+__REAL_BUILD__,
	 "host":url->host,
	 "connection":"keep-alive",
      ]);

      if(url->user || url->passwd)
	 default_headers->authorization = "Basic "
	    + MIME.encode_base64(url->user + ":" +
				 (url->password || ""));
      request_headers = default_headers | request_headers;

      array v=get_cookies(url_requested);
      if (v)
	 if (request_headers->cookie)
	    if (!arrayp(request_headers->cookie))
	       request_headers->cookie=
		  ({request_headers->cookie})+v;
	    else
	       request_headers->cookie=
		  request_headers->cookie+v;
	 else
	    request_headers->cookie=v;

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

      return ({url->host,
	       url->port,
	       method+" "+path+(query?("?"+query):"")+" HTTP/1.1",
	       request_headers, 
	       data});
   }

// ---------------- sync

//!	Perform a request synchronously.
//!	Get arguments from @[prepare_method].
//! @returns
//!	0 upon failure, this object upon success
//! @seealso
//!     @[prepare_method], @[do_async], @[do_thread]

   Request do_sync(array(string|int|mapping) args)
   {
      for (;;)
      {
	 con->sync_request(@args);
	 if (con->ok) 
	 {
	    check_for_cookies();
	    if (con->status>=300 && con->status<400 &&
		con->headers->location && follow_redirects)
	    {
	       Standards.URI loc=
		  Standards.URI(con->headers->location,url_requested);

	       if(loc->scheme=="http" || loc->scheme=="https")
	       {
		  destroy(); // clear
		  args=prepare_method("GET",loc);
		  follow_redirects--;
		  continue;
	       }
	    }
	    return this_object();
	 }
	 return 0;
      }
   }

// ---------------- thread

//!	Start a request in the background, using a thread.
//!	Call @[wait] to wait for the thread to finish.
//!	Get arguments from @[prepare_method].
//! @returns
//!	The called object.
//! @seealso
//!     @[prepare_method], @[do_sync], @[do_async], @[wait]

   Request do_thread(array(string|int|mapping) args)
   {
      con->thread_request(@args);
      return this_object();
   }

//!	Wait for the request thread to finish.
//! @returns
//!	0 upon failure, or the called object upon success.
//! @seealso
//!     @[do_thread]

   Request wait()
   {
      if (con->`()()) 
      {
	 check_for_cookies();
	 return this_object();
      }
      return 0;
   }

// ---------------- async

   static function(mixed...:mixed) headers_callback;
   static function(mixed...:mixed) data_callback;
   static function(mixed...:mixed) fail_callback;
   static array(mixed) extra_callback_arguments;

//!	Setup callbacks for async mode,
//!	@[headers] will be called when the request got connected,
//!	and got data headers; @[data] will be called when the request
//!	got the amount of data it's supposed to get and @[fail] is
//!	called whenever the request failed.
//!	
//!	Note here that an error message from the server isn't 
//!	considered a failure, only a failed TCP connection.

   void set_callbacks(function(mixed...:mixed) headers,
		      function(mixed...:mixed) data,
		      function(mixed...:mixed) fail,
		      mixed ...callback_arguments)
   {
      headers_callback=headers;
      data_callback=data;
      fail_callback=fail;
      extra_callback_arguments=callback_arguments;
   }
   
//!	Start a request asyncroneously. It will perform in 
//!	the background using callbacks (make sure the backend
//!	thread is free).
//!	Call @[set_callbacks] to setup the callbacks.
//!	Get arguments from @[prepare_method].
//! @returns
//!	The called object.
//! @seealso
//!     @[set_callbacks], @[prepare_method], @[do_sync], @[do_thread]

   Request do_async(array(string|int|mapping) args)
   {
      con->set_callbacks(async_ok,async_fail);
      con->async_request(@args);
      return this_object();
   }
   
   static void async_ok(object q)
   {
      check_for_cookies();

      if (con->status>=300 && con->status<400 &&
	  con->headers->location && follow_redirects)
      {
	 Standards.URI loc=
	    Standards.URI(con->headers->location,url_requested);

	 if(loc->scheme=="http" || loc->scheme=="https")
	 {
	    destroy(); // clear
	    follow_redirects--;

	    do_async(prepare_method("GET",loc));
	    return;
	 }
      }

      if (data_callback)
	 con->async_fetch(async_data); // start data downloading

      if (headers_callback)
	 headers_callback(@extra_callback_arguments);
   }

   static void async_fail(object q)
   {
      if (fail_callback)
	 fail_callback(@extra_callback_arguments);
   }

   static void async_data()
   {
      if (data_callback)
	 data_callback(@extra_callback_arguments);
   }

// ----------------
// shortcuts to the Query object

   string data()
   {
      return con->data();
   }

   mapping headers()
   {
      return con->headers;
   }

   int(0..1) ok()
   {
      return con->ok;
   }

   int(0..999) status()
   {
      return con->status;
   }

   string status_desc()
   {
      return con->status_desc;
   }

// ----------------
// cookie calculations

   void check_for_cookies()
   {
      if (!con->ok || !con->headers || !cookie_encountered) return;
      
      foreach (con->headers["set-cookie"]||({});;string cookie)
	 cookie_encountered(cookie,url_requested);
   }

// ----------------

//! 	@[destroy] is called when an object is destructed.
//!	But since this clears the HTTP connection from the Request object,
//!	it can also be used to reuse a @[Request] object.

   void destroy()
   {
      if (con) return_connection(url_requested,con);
      con=0;
   }

// ----------------

   string _sprintf(int t)
   {
      if (t=='O')
	 return sprintf("Request(%O",(string)url_requested)+
	    (!con?" - no connection"
	     :((con->con?" - connected":"")+
	       (!con->ok?" - failed"
		:" - "+(sizeof(con->buf)+" bytes recieved"))))+
	    ")";
   }
}

// ================================================================
// Cookies

multiset(Cookie) all_cookies=(<>);
mapping(string:mapping(string:Cookie)) cookie_lookup=([]);

class Cookie
{
   string key="?";
   string data="?";
   int expires=-1;
   string path="/";
   string site="?";

   string _sprintf(int t)
   {
      if (t=='O')
	 return sprintf("Cookie(%O: %O=%O; expires=%s; path=%O)",
			site,
			key,data,
			Calendar.ISO.Second(expires)->format_http(),
			path);
   }

   void from_http(string s,Standards.URI at)
   {
      array v=array_sscanf(s,"%{%s=%[^;]%*[; ]%}");

      site=at->host+":"+at->port;

      if (sizeof(v)<1) return;
      v=v[0];
      if (sizeof(v)<1) return;
      [key,data]=v[0];

      foreach (v[1..];;[string what,string value])
	 switch (lower_case(what))
	 {
	    case "expires":
	       expires=Calendar.ISO.parse("%e, %D %M %Y %h:%m:%s %z",value)
		  ->unix_time();
	       break;

	    case "path":
	       path=value;
	       break;
	 }
   }

   string encode()
   {
      return sprintf("%O\t%O=%O\t%O\t%O",
		     site,
		     key,data,
		     expires,
		     path);
   }

   Cookie decode(string indata)
   {
      array v=array_sscanf(indata,"%O\t%O=%O\t%O\t%O");
      if (sizeof(v)!=5) error("Cookie.decode: parse error\n");
      [site,key,data,expires,path]=v;
      if (!stringp(site) ||
	  !stringp(key) ||
	  !stringp(data) ||
	  !stringp(path) ||
	  !intp(expires))
	 error("Cookie.decode: parse error\n");
      return this_object();
   }
}


//!	Parse and set a cookie recieved in the HTTP protocol.
//!	The cookie will be checked against current security levels et al. 

void set_http_cookie(string cookie,Standards.URI at)
{
   object c=Cookie();
   c->from_http(cookie,at);
   set_cookie(c,at);
}

//!	Set a cookie. 
//!	The cookie will be checked against current security levels et al,
//!	using the parameter @[who].
//!	If @[who] is zero, no security checks will be performed.

void set_cookie(Cookie cookie,Standards.URI who)
{
// fixme: insert security checks here

   mapping sc=([]);
   if ( (sc=cookie_lookup[cookie->site]) )
   {
      Cookie old=sc[cookie->key];
      if (old) all_cookies[old]=0;
   }
   else
      sc=cookie_lookup[cookie->site]=([]);
   sc[cookie->key]=cookie;
   all_cookies[cookie]=1;
}

//! @decl string encode_cookies()
//! @decl void decode_cookies(string data,void no_clear)
//!	Dump all cookies to a string and read them back. This is useful to
//!	store cookies in between sessions (on disk, for instance).
//!	@[decode_cookies] will throw an error upon parse failures.
//!	Also note, @[decode_cookies] will clear out any previously
//!	learned cookies from the @[Session] object, unless no_clear
//!	is given and true.

string encode_cookies()
{
   return map(indices(all_cookies),"encode")*"\n"+"\n";
}

void decode_cookies(string data,void|int(0..1) no_clear)
{
   array cookies=map(data/"\n"-({""}),
		     lambda(string line)
		     {
			Cookie c=Cookie();
			c->decode(line);
			return c;
		     });
   if (!no_clear) all_cookies=(<>);
   map(cookies,set_cookie,0);
}

//! 	Get the cookies that we should send to this server,
//!	for this url. They are presented in the form suitable
//!	for HTTP headers (as an array).
//!	This will also take in count expiration of cookies,
//!	and delete expired cookies from the @[Session] unless 
//!	@[no_delete] is true.

array(string) get_cookies(Standards.URI for_url,
			  void|int(0..1) no_delete)
{
   mapping(string:Cookie) sc=
      cookie_lookup[for_url->host+":"+for_url->port]||([]);

   array(string) res=({});
   int now=time();

   foreach (sc;string key;Cookie c)
   {
      if (c->expires<now && c->expires!=-1)
      {
	 if (!no_delete)
	 {
	    m_delete(sc,key);
	    all_cookies[c]=0;
	 }
      }
      else 
      {
	 if (c->path!="/")
	 {
	    string path=for_url->path;
	    if (path=="") continue; // =="/" and we didn't get that
	    if (c->path[..strlen(path)]!=path)
	       continue; // not our path
	 }
	 res+=({key+"="+c->data});
      }
   }
   return res;
}

// ================================================================
// connections

//!	Cache of hostname to IP lookups. Given to and used by the
//!	@[Query] objects.

mapping hostname_cache=([]);

//!	Request a @[Query] object suitable to use for the
//!	given URL. This may be a reused object from a keep-alive
//!	connection.

Query give_me_connection(Standards.URI url)
{
   Query q=Query();
   q->hostname_cache=hostname_cache;
   return q;
}

//!	Return a previously used Query object to the keep-alive
//!	storage. This function will determine if the given object
//!	is suitable to keep or not by checking status and headers.

void return_connection(Standards.URI url,Query query)
{
   if (query->con) destruct(query->con);
   destruct(query);
}

// ================================================================

Request do_method(string method,
		  string url,
		  void|mapping query_variables,
		  void|string data)
{
   mapping extra_headers=0;
   if (method=="POST")
      extra_headers=(["content-type":"application/x-www-form-urlencoded"]);
   
   Request p=Request();
   p->do_sync(p->prepare_method(method,url,query_variables,
				extra_headers,data));
   return p;
}

// Request|array(string) generic_do_method(
//    string method,
//    string mode,
//    string|Standards.URI url,
//    string|mapping|function ...misc)
// {
//    array args;
//    mapping query_variables=0;
//    mapping extra_headers=0;
//    string data=0;
// 
//    misc+=({0,0,0,0});
// 
//    switch (lower_case(method))
//    {
//       case "get":
//       case "delete":
// 	 query_variables=misc[0];
// 	 misc=misc[1..];
// 	 break;
//       case "put":
// 	 query_variables=misc[0];
// 	 data=misc[1];
// 	 if (data && !stringp(data))
// 	    error("Bad argument 5 to Session.put*, expected string\n");
// 	 misc=misc[2..];
// 	 break;
//       case "post":
// 	 if (!mappingp(misc[0]))
// 	    error("Bad argument 4 to Session.put*, expected mapping\n");
// 	 data=http_encode_query(misc[0]);
// 	 extra_headers=(["content-type":"application/x-www-form-urlencoded"]);
// 	 break;
//       default:
// 	 error("unknown HTTP method %O (expected get, post, put or delete)\n");
//    }
//    if (query_variables && !mappingp(query_variables))
//       error("Bad argument 4 to Session."+method+"*, "
// 	    "expected mapping or void\n");
// 
//    args=p->prepare_method(upper_case(method),url,query_variables,
// 			  extra_headers,data);
// }

//! @decl Request get_url(string|Standards.URI url, @
//!                       void|mapping query_variables)
//! @decl Request post_url(string|Standards.URI url, @
//!                        mapping query_variables)
//! @decl Request put_url(string|Standards.URI url,string file, @
//!                       void|mapping query_variables)
//! @decl Request delete_url(string|Standards.URI url, @
//!                          void|mapping query_variables)
//! 	Sends a HTTP GET, POST, PUT or DELETE request to the server in the URL
//!	and returns the created and initialized @[Request] object.
//!	0 is returned upon failure. 
//!
Request get_url(string|Standards.URI url,
		void|mapping query_variables)
{
   return do_method("GET", url, query_variables);
}

Request put_url(string|Standards.URI url,
		void|string file,
		void|mapping query_variables)
{
   return do_method("PUT", url, query_variables, file);
}

Request delete_url(string|Standards.URI url,
		   void|mapping query_variables)
{
   return do_method("DELETE", url, query_variables);
}

Request post_url(string|Standards.URI url,
		 mapping query_variables)
{
   return do_method("POST", url, 0,
		    http_encode_query(query_variables));
}


//! @decl array(string) get_url_nice(string|Standards.URI url, @
//!                                  mapping query_variables)
//! @decl string get_url_data(string|Standards.URI url, @
//!                           mapping query_variables)
//! @decl array(string) post_url_nice(string|Standards.URI url, @
//!                                   mapping query_variables)
//! @decl string post_url_data(string|Standards.URI url, @
//!                            mapping query_variables)
//!	Returns an array of @tt{({content_type,data})@} and just the data
//!	string respective, 
//!	after calling the requested server for the information.
//!	0 is returned upon failure.
//!
//! 	post* is similar to the @[get_url()] class of functions,
//!	except that the query variables is sent as a POST request instead
//!	of as a GET.
//!

array(string) get_url_nice(string|Standards.URI url,
			   void|mapping query_variables)
{
   Request c = get_url(url, query_variables);
   return c && ({ c->headers()["content-type"], c->data() });
}

string get_url_data(string|Standards.URI url,
		    void|mapping query_variables)
{
   Request z = get_url(url, query_variables);
   return z && z->data();
}


array(string) post_url_nice(string|Standards.URI url,
			    mapping query_variables)
{
   object c = post_url(url, query_variables);
   return c && ({ c->headers["content-type"], c->data() });
}

string post_url_data(string|Standards.URI url,
		     mapping query_variables)
{
   Request z = post_url(url, query_variables);
   return z && z->data();
}

// ================================================================
// async operation

Request async_do_method(string method,
			string url,
			void|mapping query_variables,
			void|string data,
			function callback_headers_ok,
			function callback_data_ok,
			function callback_fail,
			array callback_arguments)
{
   mapping extra_headers=0;
   if (method=="POST")
      extra_headers=(["content-type":"application/x-www-form-urlencoded"]);
   
   Request p=Request();
   p->set_callbacks(callback_headers_ok,
		    callback_data_ok,
		    callback_fail,
		    p,@callback_arguments);
   p->do_async(p->prepare_method(method,url,query_variables,
				 extra_headers,data));
   return p;
}


//! @decl Request async_get_url(string|Standards.URI url,
//! 			    void|mapping query_variables,
//! 			    function callback_headers_ok,
//! 			    function callback_data_ok,
//! 			    function callback_fail,
//! 			    mixed... callback_arguments);
//! @decl Request async_put_url(string|Standards.URI url,
//! 			    void|string file,
//! 			    void|mapping query_variables,
//! 			    function callback_headers_ok,
//! 			    function callback_data_ok,
//! 			    function callback_fail,
//! 			    mixed... callback_arguments);
//! @decl Request async_delete_url(string|Standards.URI url,
//! 			       void|mapping query_variables,
//! 			       function callback_headers_ok,
//! 			       function callback_data_ok,
//! 			       function callback_fail,
//! 			       mixed... callback_arguments);
//! @decl Request async_post_url(string|Standards.URI url,
//! 			     mapping query_variables,
//! 			     function callback_headers_ok,
//! 			     function callback_data_ok,
//! 			     function callback_fail,
//! 			     mixed... callback_arguments);
//!
//! 	Sends a HTTP GET, POST, PUT or DELETE request to the server in
//!     the URL asynchroneously, and call the corresponding callbacks
//!	when result arrives (or not). The callbacks will receive 
//!	the created Request object as first argument, then 
//!	the given @[callback_arguments], if any.
//!	
//!	@[callback_headers_ok] is called when the HTTP request has received
//!	headers. 
//!
//!	@[callback_data_ok] is called when the HTTP request has been 
//!	received completely, data and all.
//!
//!	@[callback_fail] is called when the HTTP request has failed,
//!	on a TCP/IP or DNS level, or has recieved a forced timeout.
//!
//!	The created Request object is returned.

Request async_get_url(string|Standards.URI url,
		      void|mapping query_variables,
		      function callback_headers_ok,
		      function callback_data_ok,
		      function callback_fail,
		      mixed ...callback_arguments)
{
   return async_do_method("GET", url, query_variables,0,
			  callback_headers_ok,callback_data_ok,
			  callback_fail,callback_arguments);
}

Request async_put_url(string|Standards.URI url,
		      void|string file,
		      void|mapping query_variables,
		      function callback_headers_ok,
		      function callback_data_ok,
		      function callback_fail,
		      mixed ...callback_arguments)
{
   return async_do_method("PUT", url, query_variables, file,
			  callback_headers_ok,callback_data_ok,
			  callback_fail,callback_arguments);
}

Request async_delete_url(string|Standards.URI url,
			 void|mapping query_variables,
			 function callback_headers_ok,
			 function callback_data_ok,
			 function callback_fail,
			 mixed ...callback_arguments)
{
   return async_do_method("DELETE", url, query_variables, 0,
			  callback_headers_ok,callback_data_ok,
			  callback_fail,callback_arguments);
}

Request async_post_url(string|Standards.URI url,
		       mapping query_variables,
		       function callback_headers_ok,
		       function callback_data_ok,
		       function callback_fail,
		       mixed ...callback_arguments)
{
   return async_do_method("POST", url, 0,
			  http_encode_query(query_variables),
			  callback_headers_ok,callback_data_ok,
			  callback_fail,callback_arguments);
}
