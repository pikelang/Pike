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
#define DBG(X ...) werror(sprintf("%q:%d: ", __FILE__, __LINE__) + X)
#else
#define DBG(X ...)
#endif

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

int data_timeout = 120;	// seconds
int timeout = 120;	// seconds

// internal
#if constant(SSL.Cipher)
 import SSL.Constants;
#endif
int(0..1) https = 0;

//! Connected host and port.
//!
//! Used to detect whether keep-alive can be used.
string host;
string real_host; // the hostname passed during the call to *_request()
int port;

object con;
string request;
protected string send_buffer;

string buf="",headerbuf="";
int datapos, discarded_bytes, cpos;

#if constant(thread_create)
object conthread;
#endif

local function request_ok,request_fail;
array extra_args;

/****** internal stuff *********************************************/

//! Attempt to read the result header block from @[buf] and @[con].
//!
//! @returns
//!   @int
//!     @value 0
//!       Returns @expr{0@} if the header block has not yet been
//!       received.
//!     @value 1
//!       Sets @[headerbuf] to the raw header block and @[headers]
//!       to the parsed header block, calls the @[request_ok] callback
//!       (if any) and returns @expr{1@} on successful read of the
//!       header block.
//!   @endint
//!
//! @note
//!   The data part of the result may still be pending on
//!   @expr{1@} return.
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
	 default:
	   headers[n]=d;
      }
   }

   // done
   ok=1;
   remove_call_out(async_timeout);

   if (request_ok) request_ok(this,@extra_args);
   return 1;
}

protected void close_connection()
{
  Stdio.File con = this_program::con;
  if (!con) return;
  this_program::con = 0;
  con->set_callbacks(0, 0, 0, 0, 0);	// Clear any remaining callbacks.
  con->close();
}

#if constant(SSL.Cipher)
SSL.Context context;
#endif

void start_tls(int|void blocking, int|void async)
{
  DBG("start_tls(%d,%d)\n", blocking, async);
#if constant(SSL.Cipher)
  if( !context )
  {
    context = SSL.Context();
  }

  object read_callback=con->query_read_callback();
  object write_callback=con->query_write_callback();
  object close_callback=con->query_close_callback();

  SSL.sslfile ssl = SSL.sslfile(con, context);
  if (blocking) {
    ssl->set_blocking();
  }

  if (!ssl->connect(real_host)) {
    error("HTTPS connection failed.\n");
  }

  if(!blocking) {
    if (async) {
      ssl->set_nonblocking(0,async_connected,async_failed);
    } else {
      ssl->set_read_callback(read_callback);
      ssl->set_write_callback(write_callback);
      ssl->set_close_callback(close_callback);
    }
  }
  con=ssl;
#else
  error ("HTTPS not supported (Nettle support is required).\n");
#endif
}

protected void connect(string server,int port,int blocking)
{
   DBG("<- (connect %O:%d)\n",server,port);

   int success;
   if(con->_fd)
     success = con->connect(server, port);
   else
     // What is this supposed to do? /mast
     success = con->connect(server, port, blocking);

   if(!success) {
     errno = con->errno();
     DBG("<- (connect error: %s)\n", strerror (errno));
     close_connection();
     ok = 0;
     return;
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
  if (ponder_answer() <= 0) {
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
      ponder_answer();
   }
}

protected void async_write()
{
   DBG("<- %O\n", send_buffer);
   int bytes;
   if ((bytes = con->write(send_buffer)) < sizeof(send_buffer)) {
     if (bytes < 0) {
       errno = con->errno();
       DBG ("-> (write error: %s)\n", strerror (errno));
     } else if (bytes) {
       send_buffer = send_buffer[bytes..];
       return;
     }
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
   DBG("** calling failed cb %O\n", request_fail);
   if (errno)
     this_program::errno = errno;
   ok=0;
   if (request_fail) request_fail(this,@extra_args);
   remove_call_out(async_timeout);
}

protected void async_failed()
{
#if constant(System.EHOSTUNREACH)
  low_async_failed(con?con->errno():System.EHOSTUNREACH);
#else
  low_async_failed(con?con->errno():113);	// EHOSTUNREACH/Linux-i386
#endif
}

protected void async_timeout()
{
   DBG("** TIMEOUT\n");
   close_connection();
#if constant(System.ETIMEDOUT)
   low_async_failed(System.ETIMEDOUT);
#else
   low_async_failed(110);	// ETIMEDOUT/Linux-i386
#endif
}

void async_got_host(string server,int port)
{
   DBG("async_got_host %s:%d\n", server, port);
   if (!server)
   {
      async_failed();
      if (this_object()) {
	//  we may be destructed here
	close_connection();
      }
      return;
   }

   con = Stdio.File();
   con->async_connect(server, port,
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
		      });
   // NOTE: In case of failure the timeout is already in place.
}

void async_fetch_read(mixed dummy,string data)
{
   DBG("-> %d bytes of data\n",sizeof(data));
   buf+=data;

   if (!zero_type (headers["content-length"]) &&
       sizeof(buf)-datapos>=(int)headers["content-length"])
   {
      remove_call_out(async_timeout); // Bug 4773
      con->set_nonblocking(0,0,0);
      request_ok(this_object(), @extra_args);
   }
}

// This function is "liberal in what it receives" because it has no
// real way to inform the user of errors in the chunked encoding sent
// by the server. Except for calling the callback and have ->data() be
// the messenger...
void async_fetch_read_chunked(mixed dummy, string data) {
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
			if (sscanf(buf[cpos..f+3], "%*x%*[ ]%s", data)
				== 3 && sizeof(data) == 4) break;
			return;
		    }
		    continue OUTER;
		}
		break;
	    }
	    return;
	} while(0);
	remove_call_out(async_timeout);
	con->set_nonblocking(0,0,0);
	request_ok(this, @extra_args);
	break;
    }
}

void async_fetch_close()
{
   DBG("-> close\n");
   close_connection();
   remove_call_out(async_timeout);
   if (errno) {
     if (request_fail) (request_fail)(this_object(), @extra_args);
   } else {
     if (request_ok) (request_ok)(this_object(), @extra_args);
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

protected Protocols.DNS.async_client async_dns;
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
     async_dns = Protocols.DNS.async_client();
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

   array hosts = Protocols.DNS.client()->gethostbyname( hostname );
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

   errno = ok = protocol = this_program::headers = status_desc = status =
     discarded_bytes = datapos = 0;
   buf = "";
   headerbuf = "";

   if (!data) data="";

   if (!headers) headers="";
   else if (mappingp(headers))
   {
      headers=mkmapping(Array.map(indices(headers),lower_case),
			values(headers));

      if (data!="") headers["content-length"]=sizeof(data);

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

  this_program::real_host = server;
  // start open the connection
  if(con && con->is_open() &&
     this_program::host == server &&
     this_program::port == port &&
     headers && headers->connection &&
     lower_case( headers->connection ) != "close")
  {
    DBG("** Connection kept alive!\n");
    kept_alive = 1;
    // Remove unread data from the connection.
    this_program::data();
  }
  else
  {
    DBG("** Starting new connection to %O:%d.\n"
        "   con: %O (%d)\n"
        "   Previously connected to %O:%d\n",
        server, port, con, con && con->is_open(),
        this_program::host, this_program::port);
    close_connection();	// Close any old connection.

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

    this_program::host = server;
    this_program::port = port;
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

      http_headers["content-length"] = sizeof( data );
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
   DBG("async_request %s:%d %q\n", server, port, query);

  this_program::real_host = server;

   int keep_alive = con && con->is_open() && (this_program::host == server) &&
     (this_program::port == port) && this_program::headers &&
     (lower_case(this_program::headers->connection||"close") != "close");

   // start open the connection

   call_out(async_timeout,timeout);

   // prepare the request

   if (!headers) headers="";
   else if (mappingp(headers))
   {
      headers=mkmapping(map(indices(headers),lower_case),
			values(headers));

      if (data) headers["content-length"]=sizeof(data);

      headers=headers_encode(headers);
   }

   send_buffer = request = query+"\r\n"+headers+"\r\n"+(data||"");

   errno = ok = protocol = this_program::headers = status_desc = status =
     discarded_bytes = datapos = 0;
   buf = "";
   headerbuf = "";

   if (!keep_alive) {
      close_connection();
      this_program::host = server;
      this_program::port = port;
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
int `()()
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

   mixed timeout_co;
#if constant(thread_create) && constant(Stdio.__HAVE_CONCURRENT_CLOSE__)
   if (data_timeout && (Thread.this_thread() != master()->backend_thread())) {
      DBG("data timeout: %O\n", data_timeout);
      timeout_co = call_out(async_timeout, data_timeout);
   }
#endif

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

	 if (sscanf(rbuf,"%x%*[ ]\r\n%s",len,s)==3)
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
                     if (!s || !sizeof(s)) {
                        if (timeout_co) {
                           DBG("remove timeout.\n");
                           remove_call_out(timeout_co);
                        }
                        if (!s) {
                           errno = con->errno();
                           DBG ("<- (read error: %s)\n", strerror (errno));
                           return 0;
                        }
                        return lbuf;
                     }
		     rbuf+=s;
		     buf+=s;
		  }
		  else
		  {
 	             // entity_headers=rbuf[..i-1];
                     if (timeout_co) {
                        DBG("remove timeout.\n");
                        remove_call_out(timeout_co);
                     }
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
                  if (!t || !sizeof(t)) {
                     if (timeout_co) {
                        DBG("remove timeout.\n");
                        remove_call_out(timeout_co);
                     }
                     if (!t) {
                        errno = con->errno();
                        DBG ("<- (read error: %s)\n", strerror (errno));
                        return 0;
                     }
                     return lbuf+s;
                  }
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
            if (!s || !sizeof(s)) {
               if (timeout_co) {
                  DBG("remove timeout.\n");
                  remove_call_out(timeout_co);
               }
               if (!s) {
                  errno = con->errno();
                  DBG ("<- (read error: %s)\n", strerror (errno));
                  return 0;
               }
               return lbuf;
            }
	    buf+=s;
	    rbuf+=s;
	 }
      }
   }

   int len=(int)headers["content-length"];
   int l;
// test if len is zero to get around zero_type bug (??!?) /Mirar
   if(!len && zero_type( len ))
      l=0x7fffffff;
   else {
      len -= discarded_bytes;
      l=len-sizeof(buf)+datapos;
   }
   if(!zero_type(max_length) && l>max_length-sizeof(buf)+datapos)
     l = max_length-sizeof(buf)+datapos;

   DBG("fetch data: %d bytes needed, got %d, %d left\n",
       len,sizeof(buf)-datapos,l);

   if(l>0 && con)
   {
     if(headers->server == "WebSTAR")
     { // Some servers reporting this name exhibit some really hideous behaviour:
       DBG ("<- data() read 4\n");
       string s = con->read();
       if (timeout_co) {
         DBG("remove timeout.\n");
         remove_call_out(timeout_co);
       }
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
       while ((l > 0) && con) {
         string s = con->read(l, 1);
         if (!s || !sizeof(s)) {
           // Error or EOF.
           if (!s && (strlen(buf) <= datapos)) {
             // Error and we have not received any data yet.
             if (timeout_co) {
               DBG("remove timeout.\n");
               remove_call_out(timeout_co);
             }
             errno = con->errno();
             DBG ("<- (read error: %s)\n", strerror (errno));
             return 0;
           }
           DBG("Truncated result. Got %d bytes of %d (missing %d).\n",
               sizeof(buf) - datapos, sizeof(buf) + l - datapos, l);
           break;
         }
         buf += s;
         l -= sizeof(s);
       }
     }
   }
   if (timeout_co) {
     DBG("remove timeout.\n");
     remove_call_out(timeout_co);
   }
   if(zero_type( len ))
     len = sizeof( buf ) - datapos;
   if(!zero_type(max_length) && len>max_length)
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
  if(zero_type(len))
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
array|mapping|string cast(string to)
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
   error("HTTP.Query: can't cast to "+to+"\n");
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
   string buf;
   int len;

   protected void create(string _buf, int _len)
   {
      buf=_buf;
      len=_len;
   }

   //!
   string read(int n, int(0..1)|void not_all)
   {
      string s;

      if (!len) return "";
      if (n > len) n = len;

      if (sizeof(buf)<n && con)
      {
	 if (!not_all || !sizeof(buf)) {
	    string s = con->read(n-sizeof(buf), not_all);
	    if (s) {
	       buf += s;
	    }
	 }
      }

      s=buf[..n-1];
      buf=buf[n..];
      if (len != 0x7fffffff) {
	len -= sizeof(s);
      }
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
      if (zero_type(headers["content-length"]))
	 len=0x7fffffff;
      else
	 len = sizeof(hbuf)+(int)headers["content-length"];
      return PseudoFile(hbuf + buf[datapos..],len);
   }
   if (zero_type(headers["content-length"]))
      len=0x7fffffff;
   else
      len=sizeof(headerbuf)+4+(int)h["content-length"];
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
   return PseudoFile(buf[datapos..], (int)headers["content-length"]);
}

protected void destroy()
{
   if (async_id) {
     remove_call_out(async_id);
   }
   async_id = 0;

   catch(close());
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

//! Fetch all data in background.
//!
//! @seealso
//!   @[timed_async_fetch()], @[async_request()], @[set_callbacks()]
void async_fetch(function callback,mixed ... extra)
{
   if (!zero_type (headers["content-length"]) &&
       sizeof(buf)-datapos>=(int)headers["content-length"])
   {
      // FIXME: This is triggered erroneously for chunked transfer!
      call_out(callback, 0, this_object(), @extra);
      return;
   }

   if (!con)
   {
      call_out(callback, 0, this_object(), @extra); // nothing to do, stupid...
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

  if (!zero_type (headers["content-length"]) &&
      sizeof(buf)-datapos>=(int)headers["content-length"])
  {
    call_out(ok_callback, 0, this_object(), @extra);
    return;
  }

  if (!con)
  {
    // nothing to do, stupid...
    call_out(fail_callback, 0, this_object(), @extra);
    return;
  }

  extra_args = extra;
  request_ok = ok_callback;
  request_fail = fail_callback;
  call_out(async_timeout, data_timeout);

  // NB: The timeout is currently not reset on each read, so the whole
  // response has to be read within data_timeout seconds. Is that
  // intentional? /mast
  con->set_nonblocking(async_fetch_read,0, async_fetch_close);
}

protected string _sprintf(int t)
{
  return t=='O' && status && sprintf("%O(%d %s)", this_program,
				     status, status_desc);
}
