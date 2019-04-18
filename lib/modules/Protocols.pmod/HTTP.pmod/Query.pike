#pike __REAL_VERSION__

//! Open and execute an HTTP query.
//!
//! @example
//! HTTP.Query o=HTTP.Query();
//!
//! void ok()
//! {
//!    write("ok...\n");
//!    write("%O\n", o->headers);
//!    exit(0);
//! }
//!
//! void fail()
//! {
//!    write("fail\n");
//!    exit(0);
//! }
//!
//! int main()
//! {
//!    o->set_callbacks(ok, fail);
//!    o->async_request("pike.lysator.liu.se", 80, "HEAD / HTTP/1.0");
//!    return -1;
//! }

#ifdef HTTP_QUERY_DEBUG
#define DBG(X ...) werror(X)
#else
#define DBG(X ...)
#endif

#define RUNTIME_RESOLVE(X) master()->resolv(#X)
/****** variables **************************************************/

// open

//!	Errno copied from the connection or simulated for async operations.
//!	@note
//!		In Pike 7.8 and earlier hardcoded Linux values were used in
//!		async operations, 110 instead of @expr{System.ETIMEDOUT@} and
//!		113 instead of @expr{System.EHOSTUNREACH@}.
int errno;

//!	Tells if the connection is successfull.
int ok;

//!	Headers as a mapping. All header names are in lower case,
//!	for convinience.
//!
mapping headers;

//!	Protocol string, ie @expr{"HTTP/1.0"@}.
string protocol;

//!	Status number and description (eg @expr{200@} and @expr{"ok"@}).
int status;
string status_desc;

//! @ignore
// Deprecated, use the same value as in timeout unless explicitly set
int data_timeout = 0;	// seconds
//! @endignore

//! @[timeout] is the time to wait in seconds on connection and/or data.
//! If data is fetched asynchronously the watchdog will be reset every time
//! data is received. Defaults to @tt{120@} seconds.
//! @[maxtime] is the time the entire operation is allowed to take, no matter
//! if the connection and data fetching is successful. This is by default
//! indefinitely.
//!
//! @note
//!  These values only have effect in asynchroneous calls
int timeout = 120;	// seconds
int maxtime;

// internal
int(0..1) https = 0;

// This is used in async_fetch* and aync_timeout to keep track of when we last
// got data and if the timeout should abort or be reset
protected int(0..) timeout_watchdog;

//! Connected host and port.
//!
//! Used to detect whether keep-alive can be used.
string host;
string real_host; // the hostname passed during the call to *_request()
int port;

object con;
string request;
protected Stdio.Buffer send_buffer;

string buf="",headerbuf="";
int datapos, discarded_bytes, cpos;

#if constant(thread_create)
object conthread;
#endif

local function request_ok,request_fail;
array extra_args;

void init_async_timeout()
{
  call_out(async_timeout, timeout);
}

void remove_async_timeout()
{
  remove_call_out(async_timeout);
}

/****** internal stuff *********************************************/

// Callout id if maxtime is set.
protected array(function) co_maxtime;

#define TOUCH_TIMEOUT_WATCHDOG() (timeout_watchdog = time(1))

#define SET_MAXTIME_CALL_OUT()                                              \
  do {                                                                      \
    if (maxtime) {                                                          \
      DBG("+++ SET_MAXTIME_CALL_OUT()\n");                                  \
      co_maxtime = call_out(lambda() {                                      \
        /* Proper timeout status is set in async_timeout if status is 0 */  \
        status = 0;                                                         \
        async_timeout(true);                                                \
      }, maxtime);                                                          \
    }                                                                       \
  } while (0)

#define REMOVE_MAXTIME_CALL_OUT()             \
  do {                                        \
    if (co_maxtime) {                         \
      DBG("--- REMOVE_MAXTIME_CALL_OUT()\n"); \
      remove_call_out(co_maxtime);            \
      co_maxtime = 0;                         \
    }                                         \
    remove_call_out(async_timeout);           \
  } while (0)

protected int ponder_answer( int|void start_position )
{
   // read until we have all headers

   int i = start_position, j = 0;
   for(;;)
   {
      string s;

      if (i<0) i=0;
      j=search(buf, "\r\n\r\n", i);
      i=search(buf, "\n\n", i);
      if (`!=(-1, i, j)) {
	  if (i*j >= 0) i = min(i, j);
	  else if (i == -1) i = j;
	  break;
      }

      s=con->read(8192,1);
      DBG("-> %O\n",s);

      if (!s) {
	errno = con->errno();
	DBG ("<- (read error: %s)\n", strerror (errno));
	return 0;
      }
      if (s=="") {
	if (sizeof (buf) <= start_position) {
          // Fake a connection reset by peer errno.
#if constant(System.ECONNRESET)
          errno = System.ECONNRESET;
#else
          errno = 104;
#endif
	  DBG ("<- (premature EOF)\n");
	  return -1;
	}
	i=strlen(buf);
	break;
      }

      i=sizeof(buf)-3;
      buf+=s;
   }

   headerbuf = buf[start_position..i-1]-"\n";

   if (buf[i..i+1]=="\n\n") datapos=i+2;
   else datapos=i+4;

   DBG("** %d bytes of header; %d bytes left in buffer (pos=%d)\n",
       sizeof(headerbuf),sizeof(buf)-datapos,datapos);

   // split headers

   headers=([]);
   sscanf(headerbuf,"%s%*[ ]%d%*[ ]%s%*[\r]",protocol,status,status_desc);
   foreach ((headerbuf/"\r")[1..],string s)
   {
      string n,d;
      sscanf(s,"%[!-9;-~]%*[ \t]:%*[ \t]%s",n,d);
      switch(n=lower_case(n))
      {
      case "set-cookie":
        headers[n]=(headers[n]||({}))+({d});
        break;

      case "accept-ranges":
      case "age":
      case "connection":
      case "content-type":
      case "content-encoding":
      case "content-range":
      case "content-length":
      case "date":
      case "etag":
      case "keep-alive":
      case "last-modified":
      case "location":
      case "retry-after":
      case "transfer-encoding":
        headers[n]=d;
        break;

      case "allow":
      case "cache-control":
      case "vary":
        if( headers[n] )
          headers[n] += ", " +d;
        else
          headers[n] = d;
        break;

      default:
        if( headers[n] )
          headers[n] += "; " +d;
        else
          headers[n] = d;
        break;
      }
   }

   // done
   ok=1;
   remove_async_timeout();
   TOUCH_TIMEOUT_WATCHDOG();

   if (request_ok) request_ok(this,@extra_args);
   return 1;
}

protected void close_connection(int|void terminate_now)
{
  object(Stdio.File)|SSL.File con = this::con;
  if (!con) return;
  this::con = 0;
  con->set_callbacks(0, 0, 0, 0, 0);	// Clear any remaining callbacks.

  if (terminate_now) {
    // We don't want to wait for the close() to complete...
    con->set_nonblocking();
    con->close();

    if (!con->shutdown) return;

    // Probably SSL.
    //
    // Force the connection to shutdown.
    con = con->shutdown();

    if (!con) return;
  }

  con->close();
}

object/*SSL.Context*/ context;
object/*SSL.Session*/ ssl_session;

void start_tls(int|void blocking, int|void async)
{
  DBG("start_tls(%d,%d)\n", blocking, async);

  if( !context )
  {
    context = RUNTIME_RESOLVE(SSL.Context)();
    if(!context)
      error("Can not handle HTTPs queries, pike compiled without crypto support\n");
    context->trusted_issuers_cache = RUNTIME_RESOLVE(Standards.X509.load_authorities)(0,1);
  }

  object read_callback=con->query_read_callback();
  object write_callback=con->query_write_callback();
  object close_callback=con->query_close_callback();

  object/*SSL.File*/ ssl = RUNTIME_RESOLVE(SSL.File)(con, context);
  if (blocking) {
    ssl->set_blocking();
  }

  if (!(ssl_session = ssl->connect(real_host, ssl_session))) {
    error("HTTPS connection failed.\n");
  }

  con=ssl;
  if(!blocking) {
    if (async) {
      ssl->set_nonblocking(0,async_connected,async_failed);
    } else {
      ssl->set_read_callback(read_callback);
      ssl->set_write_callback(write_callback);
      ssl->set_close_callback(close_callback);
    }
  }
}

protected void connect(string server,int port,int blocking)
{
   DBG("<- (connect %O:%d)\n",server,port);

   int count;
   while(1) {
     int success;
     if(con->_fd)
       success = con->connect(server, port);
     else
       // What is this supposed to do? /mast
       // SSL.File?
       success = con->connect(server, port, blocking);

     if (success) {
       if (count) DBG("<- (connect success)\n");
       break;
     }

     errno = con->errno();
     DBG("<- (connect error: %s)\n", strerror (errno));
     count++;
     if ((count > 10) || (errno != System.EADDRINUSE)) {
       DBG("<- (connect fail)\n");
       close_connection();
       ok = 0;
       return;
     }
     // EADDRINUSE: All tuples hostip:hport:serverip:sport are busy.
     // Wait for a bit to allow the OS to recycle some ports.
     sleep(0.1);
     if (con->_fd) {
       // Normal file. Close and reopen.
       // NB: This seems to be needed on Solaris 11 as otherwise
       //     the connect() above instead will fail with ETIMEDOUT.
       close_connection();
       con = Stdio.File();
       con->open_socket(-1, 0, server);
     }
   }

   DBG("<- %O\n",request);

   if(https) {
     start_tls(blocking);
   }

   if (con->write(request) != sizeof (request)) {
     errno = con->errno();
     DBG ("-> (write error: %s)\n", strerror (errno));
   }
   else
     ponder_answer();
}

protected void async_close()
{
  con->set_blocking();
  int res = ponder_answer();
  if (res == -1) {
    async_reconnect();
  } else if (!res) {
    async_failed();
  }
}

protected void async_read(mixed dummy,string s)
{
   DBG("-> %d bytes of data\n",sizeof(s));

   buf+=s;
   if (has_value(buf, "\r\n\r\n") || has_value(buf,"\n\n"))
   {
      con->set_blocking();
      if (ponder_answer() == -1)
	async_reconnect();
   }
}

protected void async_reconnect()
{
  close_connection();
  dns_lookup_async(host, async_got_host, port);
}

protected void async_write()
{
   DBG("<- %O\n", send_buffer);
   int bytes = send_buffer->output_to(con);
   if (sizeof(send_buffer)) {
     if (bytes >= 0) {
       return;
     }
     errno = con->errno();
     DBG ("-> (write error: %s)\n", strerror (errno));
     // We'll clear the write callback below.
   }
   con->set_nonblocking(async_read,0,async_close);
}

protected void async_connected()
{
   con->set_nonblocking(async_read,async_write,async_close);
   DBG("<- %O\n","");
   con->write("");
}

protected void low_async_failed(int errno)
{
   REMOVE_MAXTIME_CALL_OUT();

   DBG("** calling failed cb %O (%O:%O:%O)\n", request_fail, errno, host, this);
   if (errno)
     this::errno = errno;
   ok=0;
   if (request_fail) request_fail(this,@extra_args);

   remove_async_timeout();
}

protected void async_failed()
{
  DBG("** Async connection failure.\n");
  if (!status) {
    status = 502;	// HTTP_BAD_GW
    status_desc = "Bad Gateway";
  }
#if constant(System.EHOSTUNREACH)
  low_async_failed(con?con->errno():System.EHOSTUNREACH);
#else
  low_async_failed(con?con->errno():113);	// EHOSTUNREACH/Linux-i386
#endif
}

protected void async_timeout(void|bool force)
{
   if (!force && !zero_type(timeout_watchdog)) {
     int deltat = time(1) - timeout_watchdog;
     if (deltat >= 0) {
       call_out(this_function, data_timeout || timeout);
       return;
     }
   }

   DBG("** TIMEOUT\n");
   if (!status) {
     status = 504;	// HTTP_GW_TIMEOUT
     status_desc = "Gateway timeout";
   }
   close_connection();
#if constant(System.ETIMEDOUT)
   low_async_failed(System.ETIMEDOUT);
#else
   low_async_failed(110);	// ETIMEDOUT/Linux-i386
#endif
}

void async_got_host(string server,int port)
{
   DBG("async_got_host %s:%d\n", server || "NULL", port);
   if (!server)
   {
      async_failed();
      if (this) {
	//  we may be destructed here
	close_connection();
      }
      return;
   }

   con = Stdio.File();
   if( !con->async_connect(server, port,
                           lambda(int success)
                           {
                             if (success) {
                               // Connect ok.
                               if(https) {
                                 start_tls(0, 1);
                               }
                               else
                                 async_connected();
                             } else {
                               // Connect failed.
                               if (!con->is_open())
                                 // Other code assumes an existing con is
                                 // an open one.
                                 con = 0;
                               async_failed();
                             }
                           }))
   {
     DBG("Failed to open socket: %s.\n", strerror(con->errno()));
     async_failed();
   }
   // NOTE: In case of failure the timeout is already in place.
}

void async_fetch_read(mixed dummy,string data)
{
   TOUCH_TIMEOUT_WATCHDOG();
   DBG("-> %d bytes of data\n",sizeof(data));
   buf+=data;

   if (has_index(headers, "content-length") &&
       sizeof(buf)-datapos>=(int)headers["content-length"])
   {
      REMOVE_MAXTIME_CALL_OUT();
      remove_async_timeout(); // Bug 4773
      con->set_nonblocking(0,0,0);
      request_ok(this, @extra_args);
   }
}

// This function is "liberal in what it receives" because it has no
// real way to inform the user of errors in the chunked encoding sent
// by the server. Except for calling the callback and have ->data() be
// the messenger...
void async_fetch_read_chunked(mixed dummy, string data) {
    TOUCH_TIMEOUT_WATCHDOG();
    int np = -1, f;
    buf+=data;
OUTER: while (sizeof(buf) > cpos) {
	do {
	// in here:
	// break calls the callback
	// return waits for more input
	// continue OUTER tries to parse one more chunk from buffer
	    if ((f = search(buf, "\r\n", cpos)) != -1) {
		if (sscanf(buf[cpos..f], "%x", np)) {
		    if (np) cpos = f+np+4;
		    else {
			if (sscanf(buf[cpos..f+3], "%*x%*[^\r\n]%s", data)
				== 3 && sizeof(data) == 4) break;
			return;
		    }
		    continue OUTER;
		}
		break;
	    }
	    return;
	} while(0);
	remove_async_timeout();
	con->set_nonblocking(0,0,0);
        REMOVE_MAXTIME_CALL_OUT();
	request_ok(this, @extra_args);
	break;
    }
}

void async_fetch_close()
{
   DBG("-> close\n");
   close_connection();
   remove_async_timeout();
   REMOVE_MAXTIME_CALL_OUT();
   if (errno) {
     if (request_fail) (request_fail)(this, @extra_args);
   } else {
     if (request_ok) (request_ok)(this, @extra_args);
   }
}

/****** utilities **************************************************/
string headers_encode(mapping(string:array(string)|string) h)
{
    constant rfc_headers = ({ "accept-charset", "accept-encoding", "accept-language",
                              "accept-ranges", "cache-control", "content-length",
                              "content-type", "if-match", "if-modified-since",
                              "if-none-match", "if-range", "if-unmodified-since",
                              "max-forwards", "proxy-authorization", "transfer-encoding",
                              "user-agent", "www-autenticate" });
   constant replace_headers = mkmapping(replace(rfc_headers[*],"-","_"),rfc_headers);

   if (!h || !sizeof(h)) return "";
   String.Buffer buf = String.Buffer();
   foreach(h; string name; array(string)|string value)
   {
     if( replace_headers[name] )
       name = replace_headers[name];
     if(stringp(value))
       buf->add( String.capitalize(name), ": ",
		 value, "\r\n" );
     else if(!value)
       continue;
     else if (intp(value))
       buf->add( String.capitalize(name), ": ",
		 (string)value, "\r\n" );
     else if (arrayp(value)) {
       foreach(value, string value)
	 buf->add( String.capitalize(name), ": ",
		   value, "\r\n" );
     } else {
       error("Protocols.HTTP.Query()->headers_encode(): Bad header: %O:%O.\n",
	     name, value);
     }
   }
   return (string)buf;
}

/****** helper methods *********************************************/

//!	Set this to a global mapping if you want to use a cache,
//!	prior of calling *request().
//!
mapping hostname_cache=([]);

protected object/*Protocols.DNS.async_client*/ async_dns;
protected int last_async_dns;
protected mixed async_id;

#ifndef PROTOCOLS_HTTP_DNS_OBJECT_TIMEOUT
#define PROTOCOLS_HTTP_DNS_OBJECT_TIMEOUT	60
#endif

// Check if it's time to clean up the async dns object.
protected void clean_async_dns()
{
  int time_left = last_async_dns + PROTOCOLS_HTTP_DNS_OBJECT_TIMEOUT - time(1);
  if (time_left >= 0) {
    // Not yet.
    async_id = call_out(clean_async_dns, time_left + 1);
    return;
  }
  async_id = 0;

  if(async_dns)
    async_dns->close();
  async_dns = 0;

  last_async_dns = 0;
}

void dns_lookup_callback(string name,string ip,function callback,
			 mixed ...extra)
{
   DBG("dns_lookup_callback %s = %s\n", name, ip||"NULL");
   hostname_cache[name]=ip;
   if (functionp(callback))
      callback(ip,@extra);
}

void dns_lookup_async(string hostname,function callback,mixed ...extra)
{
   string id;
   if (!hostname || hostname=="")
   {
      call_out(callback,0,0,@extra); // can't lookup that...
      return;
   }
   if (has_value(hostname, ":")) {
     //  IPv6
     sscanf(hostname, "%[0-9a-fA-F:.]", id);
   } else {
     sscanf(hostname,"%[0-9.]",id);
   }
   if (hostname==id ||
       (id=hostname_cache[hostname]))
   {
      call_out(callback,0,id,@extra);
      return;
   }

   if (!async_dns) {
     async_dns = RUNTIME_RESOLVE(Protocols.DNS.async_client)();
   }
   async_dns->host_to_ip(hostname, dns_lookup_callback, callback, @extra);
   last_async_dns = time(1);
   if (!async_id) {
     async_id = call_out(clean_async_dns, PROTOCOLS_HTTP_DNS_OBJECT_TIMEOUT+1);
   }
}

string dns_lookup(string hostname)
{
   string id;
   if (has_value(hostname, ":")) {
     //  IPv6
     sscanf(hostname, "%[0-9a-fA-F:.]", id);
   } else {
     sscanf(hostname,"%[0-9.]",id);
   }
   if (hostname==id ||
       (id=hostname_cache[hostname]))
      return id;

   array hosts = gethostbyname(hostname);//RUNTIME_RESOLVE(Protocols.DNS.client)()->gethostbyname( hostname );
   if (array ip = hosts && hosts[1])
     if (sizeof(ip)) {
       //  Prefer IPv4 addresses
       array(string) v6 = filter(ip, has_value, ":");
       array(string) v4 = ip - v6;
       
       if (sizeof(v4))
	 return v4[random(sizeof(v4))];
       return sizeof(v6) && v6[random(sizeof(v6))];
     }
   return 0;
}


/****** called methods *********************************************/

//! @decl Protocols.HTTP.Query set_callbacks(function request_ok, @
//!                                          function request_fail, @
//!                                          mixed ... extra)
//! @decl Protocols.HTTP.Query async_request(string server, int port, @
//!                                          string query)
//! @decl Protocols.HTTP.Query async_request(string server, int port, @
//!                                          string query, mapping headers, @
//!                                          string|void data)
//!
//!	Setup and run an asynchronous request,
//!	otherwise similar to @[thread_request()].
//!
//!	@[request_ok](Protocols.HTTP.Query httpquery,...extra args)
//!	will be called when connection is complete,
//!	and headers are parsed.
//!
//!	@[request_fail](Protocols.HTTP.Query httpquery,...extra args)
//!	is called if the connection fails.
//!
//! @returns
//!	Returns the called object

this_program set_callbacks(function(object,mixed...:mixed) _ok,
			   function(object,mixed...:mixed) _fail,
			   mixed ...extra)
{
   extra_args=extra;
   request_ok=_ok;
   request_fail=_fail;
   return this;
}

#if constant(thread_create)

//! @decl Protocols.HTTP.Query thread_request(string server, int port, @
//!                                           string query)
//! @decl Protocols.HTTP.Query thread_request(string server, int port, @
//!                                           string query, mapping headers, @
//!                                           void|string data)
//!
//!	Create a new query object and begin the query.
//!
//!	The query is executed in a background thread;
//!	call @[`()] in the object to wait for the request
//!	to complete.
//!
//!	@[query] is the first line sent to the HTTP server;
//!	for instance @expr{"GET /index.html HTTP/1.1"@}.
//!
//!	@[headers] will be encoded and sent after the first line,
//!	and @[data] will be sent after the headers.
//!
//! @returns
//!     Returns the called object.
this_program thread_request(string server, int port, string query,
			    void|mapping|string headers, void|string data)
{
   // start open the connection

   string server1=dns_lookup(server);

   if (server1) server=server1; // cheaty, if host doesn't exist

   con = 0;
   Stdio.File new_con = Stdio.File();
   if (!new_con->open_socket(-1, 0, server)) {
     int errno = con->errno();
     new_con = 0;
     error("HTTP.Query(): can't open socket; "+strerror(errno)+"\n");
   }
   con = new_con;

   // prepare the request

   errno = ok = protocol = this::headers = status_desc = status =
     discarded_bytes = datapos = 0;
   buf = "";
   headerbuf = "";

   if (!data) data="";

   if (!headers) headers="";
   else if (mappingp(headers))
   {
      headers=mkmapping(map(indices(headers),lower_case),
			values(headers));

      if (data) headers["content-length"]=(string)sizeof(data);

      headers=headers_encode(headers);
   }

   request=query+"\r\n"+headers+"\r\n"+data;

   conthread=thread_create(connect,server,port,1);

   return this;
}

#endif

this_program sync_request(string server, int port, string query,
			  void|mapping|string http_headers,
			  void|string data)
{
  int kept_alive;

  this::real_host = server;
  // start open the connection
  if(con && con->is_open() &&
     this::host == server &&
     this::port == port &&
     headers && headers->connection &&
     lower_case( headers->connection ) != "close")
  {
    DBG("** Connection kept alive!\n");
    kept_alive = 1;
    // Remove unread data from the connection.
    this::data();
  }
  else
  {
    DBG("** Starting new connection to %O:%d.\n"
        "   con: %O (%d)\n"
        "   Previously connected to %O:%d\n",
        server, port, con, con && con->is_open(),
        this::host, this::port);
    close_connection();	// Close any old connection.

    this::host = server;
    this::port = port;

    string ip = dns_lookup( server );
    if(ip) server = ip; // cheaty, if host doesn't exist

    con = 0;
    Stdio.File new_con = Stdio.File();
    if(!new_con->open_socket(-1, 0, server)) {
      int errno = con->errno();
      new_con = 0;
      error("HTTP.Query(): can't open socket; "+strerror(errno)+"\n");
    }
    con = new_con;
  }

  // prepare the request

  errno = ok = protocol = headers = status_desc = status =
    datapos = discarded_bytes = 0;
  buf = "";
  headerbuf = "";

  if(!stringp( http_headers )) {

    if(mappingp( http_headers ))
      http_headers = mkmapping(map(indices( http_headers ), lower_case),
			       values( http_headers ));
    else
      http_headers = ([]);

    if(data) {
      if(String.width(data)>8) {
	if(!http_headers["content-type"])
	  error("Wide string as data and no content-type header set.\n");
	array split = http_headers["content-type"]/"charset=";
	if(sizeof(split)==1)
	  http_headers["content-type"] += "; charset=utf-8";
	else {
	  string tail = split[1];
	  sscanf(tail, "%s ", tail);
	  http_headers["content-type"] = split[0] + "utf-8" + tail;
	}
	data = string_to_utf8(data);
      }

      http_headers["content-length"] = (string)sizeof( data );
    }

    http_headers = headers_encode( http_headers );
  }

  request = query + "\r\n" + http_headers + "\r\n" + (data||"");

  if(kept_alive)
  {
    DBG("<- %O\n",request);
    if (con->write( request ) != sizeof (request)) {
      errno = con->errno;
      DBG ("-> (write error: %s)\n", strerror (errno));
    }
    else
      if (ponder_answer() == -1) {
	// The keepalive connection was closed from the server end.
	// Retry with a new one.
	// FIXME: Proxy handling?
	close_connection();
	return sync_request (server, port, query, http_headers, data);
      }
  } else
    connect(server, port,1);

  return this;
}

this_program async_request(string server,int port,string query,
			   void|mapping|string headers,void|string data)
{
   SET_MAXTIME_CALL_OUT();
   DBG("async_request %s:%d %q\n", server, port, query);

  this::real_host = server;

   if (con) con->set_nonblocking_keep_callbacks();

   int keep_alive = con && con->is_open() && (this::host == server) &&
     (this::port == port) && this::headers &&
     (lower_case(this::headers->connection||"close") != "close");

   // start open the connection

   init_async_timeout();

   // prepare the request

   if (!headers) headers="";
   else if (mappingp(headers))
   {
      headers=mkmapping(map(indices(headers),lower_case),
			values(headers));

      if (data) headers["content-length"]=(string)sizeof(data);

      headers=headers_encode(headers);
   }

   send_buffer = Stdio.Buffer(request = query+"\r\n"+headers+"\r\n"+(data||""));

   errno = ok = protocol = this::headers = status_desc = status =
     discarded_bytes = datapos = 0;
   buf = "";
   headerbuf = "";

   if (!keep_alive) {
      close_connection();
      this::host = server;
      this::port = port;
      dns_lookup_async(server,async_got_host,port);
   }
   else
   {
      async_connected();
   }
   return this;
}

#if constant(thread_create)

//!	Wait for connection to complete.
//! @returns
//!	Returns @expr{1@} on successfull connection, @expr{0@} on failure.
//!
protected int `()()
{
   // wait for completion
   if (conthread) conthread->wait();
   return ok;
}

#endif

//!	Gives back the data as a string.
string data(int|void max_length)
{
#if constant(thread_create)
   `()();
#endif

   if (buf=="") return ""; // already emptied
   if (is_empty_response()) return "";

   if (headers["transfer-encoding"] &&
       lower_case(headers["transfer-encoding"])=="chunked")
   {
      string rbuf=buf[datapos..];
      string lbuf="";

      for (;;)
      {
	 int len;
	 string s;

	 DBG("got %d; chunk: %O left: %d\n",strlen(lbuf),rbuf[..40],strlen(rbuf));

	 if (sscanf(rbuf,"%x%*[^\r\n]\r\n%s",len,s)==3)
	 {
	    if (len==0)
	    {
	       for (;;)
	       {
		  int i;
		  if ((i=search(rbuf,"\r\n\r\n"))==-1)
		  {
		    DBG ("<- data() read\n");
		     s=con->read(8192,1);
		     if (!s) {
		       errno = con->errno();
		       DBG ("<- (read error: %s)\n", strerror (errno));
		       return 0;
		     }
		     if (s=="") return lbuf;
		     rbuf+=s;
		     buf+=s;
		  }
		  else
		  {
 	             // entity_headers=rbuf[..i-1];
		     return lbuf;
		  }
	       }
	    }
	    else
	    {
	       if (strlen(s)<len)
	       {
		 DBG ("<- data() read 2\n");
		  string t=con->read(len-strlen(s)+6); // + crlfx3
		  if (!t) {
		    errno = con->errno();
		    DBG ("<- (read error: %s)\n", strerror (errno));
		    return 0;
		  }
		  if (t=="") return lbuf+s;
		  buf+=t;
		  lbuf+=s+t[..len-strlen(s)-1];
		  rbuf=t[len-strlen(s)+2..];
	       }
	       else
	       {
		  lbuf+=s[..len-1];
		  rbuf=s[len+2..];
	       }
	    }
	 }
	 else
	 {
	   DBG ("<- data() read 3\n");
	    s=con->read(8192,1);
	    if (!s) {
	      errno = con->errno();
	      DBG ("<- (read error: %s)\n", strerror (errno));
	      return 0;
	    }
	    if (s=="") return lbuf;
	    buf+=s;
	    rbuf+=s;
	 }
      }
   }

   int len=(int)headers["content-length"];
   int l = (1<<31)-1;

   if(!undefinedp( len ))
   {
      len -= discarded_bytes;
      l=len-sizeof(buf)+datapos;
   }
   if(!undefinedp(max_length))
     l = min(l, max_length-sizeof(buf)+datapos);

   DBG("fetch data: %d bytes needed, got %d, %d left\n",
       len,sizeof(buf)-datapos,l);

   if(l>0 && con)
   {
     if(headers->server == "WebSTAR")
     { // Some servers reporting this name exhibit some really hideous behaviour:
       DBG ("<- data() read 4\n");
       string s = con->read();
       if (!s) {
	 errno = con->errno();
	 DBG ("<- (read error: %s)\n", strerror (errno));
	 return 0;
       }
       buf += s; // First, they may well lie about the content-length
       if(!discarded_bytes && buf[datapos..datapos+1] == "\r\n")
	 datapos += 2; // And, as if that wasn't enough! *mumble*
     }
     else
     {
       DBG ("<- data() read 5\n");
       string s = con->read(l);
       if (!s && strlen(buf) <= datapos) {
	 errno = con->errno();
	 DBG ("<- (read error: %s)\n", strerror (errno));
	 return 0;
       }
       if( s )
	 buf += s;
     }
   }
   if(undefinedp( len ))
     len = sizeof( buf ) - datapos;
   if(!undefinedp(max_length) && len>max_length)
     len = max_length;
   return buf[datapos..datapos+len-1];
}

protected Charset.Decoder charset_decoder;

//! Gives back data, but decoded according to the content-type
//! character set.
//! @seealso
//!   @[data]
string unicode_data() {
  if(!charset_decoder) {
    string charset;
    if(headers["content-type"])
      sscanf(headers["content-type"], "%*scharset=%s", charset);
    if(!charset)
      charset_decoder = Charset.decoder("ascii");
    else
      charset_decoder = Charset.decoder(charset);
  }
  return charset_decoder->feed(data())->drain();
}

void discard_bytes(int n)
{
  if(n > sizeof(buf) - datapos)
    error("HTTP.Query: discarding more bytes than read\n");
  if(n > 0) {
    discarded_bytes += n;
    buf = buf[..datapos-1]+buf[datapos+n..];
  }
}

string incr_data(int max_length)
{
  string ret = data(max_length);
  discard_bytes(sizeof(ret));
  return ret;
}

//! Gives back the number of downloaded bytes.
int downloaded_bytes()
{
  if(datapos)
    return sizeof(buf)-datapos+discarded_bytes;
  else
    return 0;
}

//!     Gives back the size of a file if a content-length header is present
//!     and parsed at the time of evaluation. Otherwise returns -1.
int total_bytes()
{
  if(!headers)
    return -1;
  int len=(int)headers["content-length"];
  if(undefinedp(len))
    return -1;
  else
    return len;
}

//! @decl array cast("array")
//! @returns
//!   @array
//!     @elem mapping 0
//!       Headers
//!     @elem string 1
//!       Data
//!     @elem string 2
//!       Protocol
//!     @elem int 3
//!       Status
//!     @elem string 4
//!       Status description
//!   @endarray

//! @decl mapping cast("mapping")
//! @returns
//!   The header mapping ORed with the following mapping.
//!   @mapping
//!     @member string "protocol"
//!       The protocol.
//!     @member int "status"
//!       The status code.
//!     @member string "status_desc"
//!       The status description.
//!     @member string "data"
//!       The returned data.
//!   @endmapping

//! @decl string cast("string")
//!	Gives back the answer as a string.
protected array|mapping|string cast(string to)
{
   switch (to)
   {
      case "mapping":
	 return headers|
	    (["data":data(),
	      "protocol":protocol,
	      "status":status,
	      "status_desc":status_desc]);
      case "array":
	 return ({headers,data(),protocol,status,status_desc});
      case "string":
         data();
         return buf;
   }
   return UNDEFINED;
}

//! Minimal simulation of a @[Stdio.File] object.
//!
//! Objects of this class are returned by @[file()] and @[datafile()].
//!
//! @note
//!   Do not attempt further queries using this @[Query] object
//!   before having read all data.
class PseudoFile
{
   Stdio.Buffer buf;
   int len;

   protected void create(string _buf, int _len)
   {
      buf=Stdio.Buffer(_buf);
      len=_len;
   }

   //!
   string read(int n, int(0..1)|void not_all)
   {
      if (!len) return "";
      if (n > len) n = len;

      if (sizeof(buf)<n && con)
         buf->input_from( con, n-sizeof(buf), not_all);

      string s = buf->read(min(n,sizeof(buf)));

      if (len != 0x7fffffff)
        len -= strlen(s);

      return s;
   }

   //!
   void close()
   {
      destruct();
   }
}

//! @decl Protocols.HTTP.Query.PseudoFile file()
//! @decl Protocols.HTTP.Query.PseudoFile file(mapping newheaders, @
//!                                            void|mapping removeheaders)
//!	Gives back a pseudo-file object,
//!	with the methods @expr{read()@} and @expr{close()@}.
//!	This could be used to copy the file to disc at
//!	a proper tempo.
//!
//!	@[newheaders], @[removeheaders] is applied as:
//!	@expr{(oldheaders|newheaders))-removeheaders@}
//!	Make sure all new and remove-header indices are lower case.
//!
//! @seealso
//!     @[datafile()]
object file(void|mapping newheader,void|mapping removeheader)
{
#if constant(thread_create)
   `()();
#endif
   mapping h=headers;
   int len;
   if (newheader||removeheader)
   {
      h=(h|(newheader||([])))-(removeheader||([]));
      string hbuf=headers_encode(h);
      if (hbuf=="") hbuf="\r\n";
      hbuf = protocol + " " + status + " " + status_desc + "\r\n" +
	hbuf + "\r\n";
      if (has_index(headers, "content-length"))
	 len = sizeof(hbuf)+(int)headers["content-length"];
      else
	 len=0x7fffffff;
      return PseudoFile(hbuf + buf[datapos..],len);
   }
   if (has_index(headers, "content-length"))
      len=sizeof(headerbuf)+4+(int)h["content-length"];
   else
      len=0x7fffffff;
   return PseudoFile(headerbuf + "\r\n\r\n" + buf[datapos..], len);
}

//! @decl Protocols.HTTP.Query.PseudoFile datafile();
//!	Gives back a pseudo-file object,
//!	with the methods @expr{read()@} and @expr{close()@}.
//!	This could be used to copy the file to disc at
//!	a proper tempo.
//!
//!	@[datafile()] doesn't give the complete request,
//!	just the data.
//!
//! @seealso
//!     @[file()]
object datafile()
{
#if constant(thread_create)
   `()();
#endif
   return PseudoFile(buf[datapos..], (int)(headers["content-length"]||0x7ffffff));
}

protected void _destruct()
{
   if (async_id) {
     remove_call_out(async_id);
   }
   async_id = 0;
   if(async_dns) {
     async_dns->close();
     async_dns = 0;
   }

   catch(close_connection(1));
}

//! Close all associated file descriptors.
//!
void close()
{
  close_connection();
  if(async_dns) {
    async_dns->close();
    async_dns = 0;
  }
}

private int(0..1) is_empty_response()
{
  return (status >= 100 && status < 200) ||
    (status == 204) || (status == 304) ||
    (request && (upper_case(request[..4]) == "HEAD "));
}

private int(0..1) body_is_fetched()
{
  // There is no body in these requests
  // and the content-length header must be ignored
  if (is_empty_response() || !con) {
    return 1;
  }

  // the length is given by chunked encoding if the following is true
  // see section 4.4 of rfc 2616
  int(0..1) ignore_length =
          !(headers->connection == "close" || (protocol == "HTTP/1.0" && !headers->connection)) &&
          headers["transfer-encoding"] && headers["transfer-encoding"] != "identity";

  if (!ignore_length && has_index(headers, "content-length")) {
      if (sizeof(buf)-datapos >= (int)headers["content-length"]) {
        return 1;
      }
  }

  return 0;
}

//! Fetch all data in background.
//!
//! @seealso
//!   @[timed_async_fetch()], @[async_request()], @[set_callbacks()]
void async_fetch(function callback,mixed ... extra)
{
  if (body_is_fetched()) {
    call_out(callback, 0, this, @extra);
    return;
  }

   extra_args=extra;
   request_ok=callback;
   cpos = datapos;
   if (lower_case(headers["transfer-encoding"]) != "chunked")
       con->set_nonblocking(async_fetch_read,0,async_fetch_close);
   else {
       con->set_nonblocking(async_fetch_read_chunked,0,async_fetch_close);
       async_fetch_read_chunked(0, ""); // might already be complete
					// in buffer
   }
}

//! Like @[async_fetch()], except with a timeout and a corresponding fail
//! callback function.
//!
//! @seealso
//!   @[async_fetch()], @[async_request()], @[set_callbacks()]
void timed_async_fetch(function(object, mixed ...:void) ok_callback,
		       function(object, mixed ...:void) fail_callback,
		       mixed ... extra) {
  if (body_is_fetched()) {
    call_out(ok_callback, 0, this, @extra);
    return;
  }

  extra_args = extra;
  request_ok = ok_callback;
  request_fail = fail_callback;
  cpos = datapos;

  // NB: Different timeout than init_async_timeout().
  call_out(async_timeout, data_timeout || timeout);

  // NB: The timeout is currently not reset on each read, so the whole
  // response has to be read within data_timeout seconds. Is that
  // intentional? /mast
  //
  // This is hopefully fixed now.
  if (lower_case(headers["transfer-encoding"]) != "chunked")
    con->set_nonblocking(async_fetch_read,0,async_fetch_close);
  else {
    con->set_nonblocking(async_fetch_read_chunked,0,async_fetch_close);
    async_fetch_read_chunked(0, ""); // might already be complete
                                     // in buffer
  }
}

protected string _sprintf(int t)
{
  return t=='O' && status && sprintf("%O(%d %s)", this_program,
				     status, status_desc);
}
