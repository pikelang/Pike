#pike __REAL_VERSION__

// $Id: Session.pike,v 1.16 2004/06/08 15:07:13 vida Exp $

import Protocols.HTTP;

typedef string|Standards.URI|SessionURL URL;

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

//! Default HTTP headers. 
mapping default_headers = ([
   "user-agent":"Mozilla/5.0 (compatible; MSIE 6.0; Pike HTTP client)"
   " Pike/"+__REAL_MAJOR__+"."+__REAL_MINOR__+"."+__REAL_BUILD__,
]);

//! Request
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
   int follow_redirects= // from parent, will count down
      function_object(this_program)->follow_redirects;

//!	Cookie callback. When a request is performed,
//!	the result is checked for cookie changes and
//!	additions. If a cookie is encountered, this
//!	function is called. Default is to call 
//!	@[set_http_cookie] in the @[Session] object.
   function(string,Standards.URI:mixed) cookie_encountered=set_http_cookie;

// ----------------

   int(0..1) con_https; // internal flag

//!	Prepares the HTTP Query object for the connection,
//!	and returns the parameters to use with @[do_sync],
//!	@[do_async] or @[do_thread].
//!
//!	This method will also use cookie information from the 
//!	parent @[Session], and may reuse connections (keep-alive).
   array(string|int|mapping) prepare_method(
      string method,
      URL url,
      void|mapping query_variables,
      void|mapping extra_headers,
      void|string data)
   {
      if(stringp(url))
	 url=Standards.URI(url);
      url_requested=url;

#if constant(SSL.sslfile) 	
      if(url->scheme!="http" && url->scheme!="https")
	 error("Protocols.HTTP can't handle %O or any other "
	       "protocols than HTTP or HTTPS\n",
	       url->scheme);
  
      con_https= (url->scheme=="https")? 1 : 0;
#else
      if(url->scheme!="http"	)
	 error("Protocols.HTTP can't handle %O or any other "
	       "protocol than HTTP\n",
	       url->scheme);
#endif
      mapping request_headers = copy_value(default_headers);
      if (url->referer)
	 request_headers->referer=(string)url->referer;

      if(url->user || url->passwd)
	 request_headers->authorization = "Basic "
	    + MIME.encode_base64(url->user + ":" +
				 (url->password || ""));

      request_headers->connection=
	 (time_to_keep_unused_connections<=0)?"Close":"Keep-Alive";

      request_headers->host=url->host;
  
      if (extra_headers)
	 request_headers|=extra_headers;

      array v=get_cookies(url_requested);
      if (v && sizeof(v))
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
	 if(!con) con=give_me_connection(url_requested);
	 con->sync_request(@args);
	 if (con->ok)
	 {
	    check_for_cookies();
	    while( con->status == 100 )
	       con->ponder_answer( con->datapos );
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
	    return this;
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
//! @note
//!	@[do_thread] does not rerun redirections automatically
   Request do_thread(array(string|int|mapping) args)
   {
      if(!con) con=give_me_connection(url_requested);
      con->thread_request(@args);
      return this;
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
	 return this;
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
      if(!con)
      {
	 if (connections_host_n[connection_lookup(url_requested)]>=
	     maximum_connections_per_server ||
	     (!connections_kept_n &&
	      connections_inuse_n>=maximum_total_connections))
	 {
	    wait_for_connection(do_async,args);
	    return this;
	 }
	 con=give_me_connection(url_requested);
      }
      con->set_callbacks(async_ok,async_fail);
      con->async_request(@args);
      return this;
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

      if (headers_callback)
	 headers_callback(@extra_callback_arguments);

      // clear callbacks for possible garbation of this Request object
      con->set_callbacks(0,0);

      if (data_callback)
	 con->async_fetch(async_data); // start data downloading
      else
	 extra_callback_arguments=0; // to allow garb
   }

   static void async_fail(object q)
   {
      // clear callbacks for possible garbation of this Request object
      con->set_callbacks(0,0);

      array eca=extra_callback_arguments;
      function fc=fail_callback;
      set_callbacks(0,0,0); // drop all references

      if (fc) fc(@eca); // note that we may be destructed here
   }

   static void async_data()
   {
      // clear callbacks for possible garbation of this Request object
      con->set_callbacks(0,0);

      if (data_callback) data_callback(@extra_callback_arguments);
      extra_callback_arguments=0; // clear references there too
   }

   void wait_for_connection(function callback,mixed ...args)
   {
      freed_connection_callbacks+=({({connection_lookup(url_requested),
				      callback,args})});
   }

// ----------------
// shortcuts to the Query object

   string data()
   {
      if (!con) error("Request: no connection\n");
      return con->data();
   }

   mapping headers()
   {
      if (!con) error("Request: no connection\n");
      return con->headers;
   }

   int(0..1) ok()
   {
      if (!con) error("Request: no connection\n");
      return con->ok;
   }

   int(0..999) status()
   {
      if (!con) error("Request: no connection\n");
      return con->status;
   }

   string status_desc()
   {
      if (!con) error("Request: no connection\n");
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
		:" - "+(sizeof(con->buf)+" bytes received"))))+
	    ")";
   }
}

// ================================================================
// Cookies

multiset(Cookie) all_cookies=(<>);
mapping(string:mapping(string:Cookie)) cookie_lookup=([]);

// this class is internal for now, until there is a
// decent use for it externally

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
	       expires=
		  (Calendar.ISO.parse("%e, %D %M %Y %h:%m:%s %z",value)||
		   Calendar.ISO.parse("%e, %D-%M-%y %h:%m:%s %z",value) )
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
      return this;
   }
}


//!	Parse and set a cookie received in the HTTP protocol.
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
array(string) get_cookies(Standards.URI|SessionURL for_url,
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

//!	The time to keep unused connections in seconds. Set to zero
//!	to never save any kept-alive connections.
//!	(Might be good in a for instance totaly synchroneous script
//!	that keeps the backend thread busy and never will get call_outs.)
//!	Defaults to 10 seconds.
int|float time_to_keep_unused_connections=10;

//!	Maximum number of connections to the same server.
//!	Used only by async requests.
//!	Defaults to 10 connections.
int maximum_connections_per_server=10;

//!	Maximum total number of connections. Limits only
//!	async requests, and the number of kept-alive connections
//!	(live connections + kept-alive connections <= this number)
//!	Defaults to 50 connections.
int maximum_total_connections=50;

//!	Maximum times a connection is reused. 
//!	Defaults to 1000000. <2 means no reuse at all.
int maximum_connection_reuse=1000000;

// internal (but readable for debug purposes)
mapping(string:array(KeptConnection)) connection_cache=([]);
int connections_kept_n=0;
int connections_inuse_n=0;
mapping(string:int) connections_host_n=([]);

static class KeptConnection
{
   string lookup;
   Query q;

   void create(string _lookup,Query _q)
   {
      lookup=_lookup;
      q=_q;

      call_out(disconnect,time_to_keep_unused_connections);
      connection_cache[lookup]=
	 (connection_cache[lookup]||({}))+({this});
      connections_kept_n++;
   }

   void disconnect()
   {
      connection_cache[lookup]-=({this});
      if (!sizeof(connection_cache[lookup]))
	 m_delete(connection_cache,lookup);
      remove_call_out(disconnect); // if called externally

      if (q->con) destruct(q->con);
      connections_kept_n--;
      if (!--connections_host_n[lookup])
	 m_delete(connections_host_n,lookup);
      destruct(q);
      destruct(this);
   }

   Query use()
   {
      connection_cache[lookup]-=({this});
      if (!sizeof(connection_cache[lookup]))
	 m_delete(connection_cache,lookup);
      remove_call_out(disconnect);

      connections_kept_n--;
      return q; // subsequently, this object is removed (no refs)
   }
}

static inline string connection_lookup(Standards.URI url)
{
   return url->scheme+"://"+url->host+":"+url->port;
}

//!	Request a @[Query] object suitable to use for the
//!	given URL. This may be a reused object from a keep-alive
//!	connection.
Query give_me_connection(Standards.URI url)
{
   Query q;

   if (array(KeptConnection) v =
       connection_cache[connection_lookup(url)])
   {
      q=v[0]->use(); // removes itself
      // clean up
      q->buf="";
      q->headerbuf="";
      q->n_used++;
   }
   else
   {
      if (connections_kept_n+connections_inuse_n+1
	  >= maximum_total_connections &&
	  sizeof(connection_cache))
      {
         // close one if we have it kept
	 array(KeptConnection) all=`+(@values(connection_cache));
	 KeptConnection one=all[random(sizeof(all))];
	 one->disconnect(); // removes itself
      }

      q=SessionQuery();
      q->hostname_cache=hostname_cache;
      connections_host_n[connection_lookup(url)]++; // new
   }
   connections_inuse_n++;
   return q;
}

//
// called when there might be a free connection

// queue of what to call when we get new free connections
array(array) freed_connection_callbacks=({});
                    // ({lookup,callback,args})

static inline void freed_connection(string lookup_freed)
{
   if (connections_inuse_n>=maximum_total_connections)
      return;

   foreach (freed_connection_callbacks;;array v)
   {
      [string lookup, function callback, array args]=v;
      if (lookup==lookup_freed ||
	  connections_host_n[lookup]<
	  maximum_connections_per_server)
      {
	 freed_connection_callbacks-=({v}); 
	 callback(@args);
	 return;
      }
   }
}

//!	Return a previously used Query object to the keep-alive
//!	storage. This function will determine if the given object
//!	is suitable to keep or not by checking status and headers.
void return_connection(Standards.URI url,Query query)
{
   connections_inuse_n--;
   string lookup=connection_lookup(url);
   if (query && query->con && query->is_sessionquery && query->headers) 
   {
      if (query->headers->connection &&
	  lower_case(query->headers->connection)=="keep-alive" &&
	  connections_kept_n+connections_inuse_n
	  < maximum_total_connections &&
	  time_to_keep_unused_connections>0 &&
	  query->n_used < maximum_connection_reuse)
      {
         // clean up
	 query->set_callbacks(0,0);
	 KeptConnection(lookup,query);
	 freed_connection(lookup);
	 return;
      }
      destruct(query->con);
   }
   destruct(query);
   if (!--connections_host_n[lookup])
      m_delete(connections_host_n,lookup);
   freed_connection(lookup);
}

// ================================================================

Request do_method_url(string method,
		      string url,
		      void|mapping query_variables,
		      void|string data,
		      void|mapping extra_headers)
{
   if (method=="POST")
      extra_headers=
	 (extra_headers||([]))||
	 (["content-type":"application/x-www-form-urlencoded"]);
   
   Request p=Request();
   p->do_sync(p->prepare_method(method,url,query_variables,
				extra_headers,data));
   return p->ok() && p;
}

//! @decl Request get_url(URL url, @
//!                       void|mapping query_variables)
//! @decl Request post_url(URL url, @
//!                        mapping query_variables)
//! @decl Request put_url(URL url,string file, @
//!                       void|mapping query_variables)
//! @decl Request delete_url(URL url, @
//!                          void|mapping query_variables)
//! 	Sends a HTTP GET, POST, PUT or DELETE request to the server in the URL
//!	and returns the created and initialized @[Request] object.
//!	0 is returned upon failure. 
//!

Request get_url(URL url,
		void|mapping query_variables)
{
   return do_method_url("GET", url, query_variables);
}

Request put_url(URL url,
		void|string file,
		void|mapping query_variables)
{
   return do_method_url("PUT", url, query_variables, file);
}

Request delete_url(URL url,
		   void|mapping query_variables)
{
   return do_method_url("DELETE", url, query_variables);
}

Request post_url(URL url,
		 mapping query_variables)
{
   return do_method_url("POST", url, 0,
			http_encode_query(query_variables));
}


//! @decl array(string) get_url_nice(URL url, @
//!                                  mapping query_variables)
//! @decl string get_url_data(URL url, @
//!                           mapping query_variables)
//! @decl array(string) post_url_nice(URL url, @
//!                                   mapping query_variables)
//! @decl string post_url_data(URL url, @
//!                            mapping query_variables)
//!	Returns an array of @expr{({content_type,data})@} and
//!     just the data string respective, 
//!	after calling the requested server for the information.
//!	@expr{0@} is returned upon failure.
//!
//! 	post* is similar to the @[get_url()] class of functions,
//!	except that the query variables is sent as a POST request instead
//!	of as a GET.
//!

array(string) get_url_nice(URL url,
			   void|mapping query_variables)
{
   Request c = get_url(url, query_variables);
   return c && ({ c->headers()["content-type"], c->data() });
}

string get_url_data(URL url,
		    void|mapping query_variables)
{
   Request z = get_url(url, query_variables);
   return z && z->data();
}


array(string) post_url_nice(URL url,
			    mapping query_variables)
{
   object c = post_url(url, query_variables);
   return c && ({ c->headers["content-type"], c->data() });
}

string post_url_data(URL url,
		     mapping query_variables)
{
   Request z = post_url(url, query_variables);
   return z && z->data();
}

// ================================================================
// async operation

Request async_do_method_url(string method,
			    URL url,
			    void|mapping query_variables,
			    void|string data,
			    void|mapping extra_headers,
			    function callback_headers_ok,
			    function callback_data_ok,
			    function callback_fail,
			    array callback_arguments)
{
   if(stringp(url)) url=Standards.URI(url);

   Request p=Request();

   p->set_callbacks(callback_headers_ok,
		    callback_data_ok,
		    callback_fail,
		    p,@callback_arguments);

   if (method=="POST")
      extra_headers=
	 (extra_headers||([]))||
	 (["content-type":"application/x-www-form-urlencoded"]);

   p->do_async(p->prepare_method(method,url,query_variables,
				 extra_headers,data));
   return p;
}


//! @decl Request async_get_url(URL url,@
//! 			    void|mapping query_variables,@
//! 			    function callback_headers_ok,@
//! 			    function callback_data_ok,@
//! 			    function callback_fail,@
//! 			    mixed... callback_arguments)
//! @decl Request async_put_url(URL url,@
//! 			    void|string file,@
//! 			    void|mapping query_variables,@
//! 			    function callback_headers_ok,@
//! 			    function callback_data_ok,@
//! 			    function callback_fail,@
//! 			    mixed... callback_arguments)
//! @decl Request async_delete_url(URL url,@
//! 			       void|mapping query_variables,@
//! 			       function callback_headers_ok,@
//! 			       function callback_data_ok,@
//! 			       function callback_fail,@
//! 			       mixed... callback_arguments)
//! @decl Request async_post_url(URL url,@
//! 			     mapping query_variables,@
//! 			     function callback_headers_ok,@
//! 			     function callback_data_ok,@
//! 			     function callback_fail,@
//! 			     mixed... callback_arguments)
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
//!	on a TCP/IP or DNS level, or has received a forced timeout.
//!
//!	The created Request object is returned.

Request async_get_url(URL url,
		      void|mapping query_variables,
		      function callback_headers_ok,
		      function callback_data_ok,
		      function callback_fail,
		      mixed ...callback_arguments)
{
   return async_do_method_url("GET", url, query_variables,0,0,
			      callback_headers_ok,callback_data_ok,
			      callback_fail,callback_arguments);
}

Request async_put_url(URL url,
		      void|string file,
		      void|mapping query_variables,
		      function callback_headers_ok,
		      function callback_data_ok,
		      function callback_fail,
		      mixed ...callback_arguments)
{
   return async_do_method_url("PUT", url, query_variables, file,0,
			      callback_headers_ok,callback_data_ok,
			      callback_fail,callback_arguments);
}

Request async_delete_url(URL url,
			 void|mapping query_variables,
			 function callback_headers_ok,
			 function callback_data_ok,
			 function callback_fail,
			 mixed ...callback_arguments)
{
   return async_do_method_url("DELETE", url, query_variables, 0,0,
			      callback_headers_ok,callback_data_ok,
			      callback_fail,callback_arguments);
}

Request async_post_url(URL url,
		       mapping query_variables,
		       function callback_headers_ok,
		       function callback_data_ok,
		       function callback_fail,
		       mixed ...callback_arguments)
{
   return async_do_method_url("POST", url, 0,
			      http_encode_query(query_variables),0,
			      callback_headers_ok,callback_data_ok,
			      callback_fail,callback_arguments);
}

// ----------------------------------------------------------------
//

//! Class to store URL+referer
class SessionURL
{
   inherit Standards.URI;

//! the referer to this URL
   URL referer;

//! instantiate a SessionURL object;
//! when fed to Protocols.HTTP.Session calls, will add
//! referer to the HTTP handshaking variables
   void create(URL uri,
	       URL base_uri,
	       URL _referer)
   {
      ::create(uri,base_uri);
      referer=_referer;
   }
}

class SessionQuery
{
   inherit Query;
   
   int n_used=1;
   constant is_sessionquery=1;
}
