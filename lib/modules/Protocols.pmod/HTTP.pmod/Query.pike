#pike __REAL_VERSION__

// $Id: Query.pike,v 1.75 2004/11/30 17:37:44 mast Exp $

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
//!    o->async_request("pike.ida.liu.se", 80, "HEAD / HTTP/1.0");
//!    return -1;
//! }

/****** variables **************************************************/

// open

//!	Errno copied from the connection.
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

int timeout=120; // seconds

// internal
#if constant(SSL.Cipher.CipherAlgorithm)
 import SSL.Constants;
 SSL.sslfile ssl;
#endif
int(0..1) https = 0;

object con;
string request;

string buf="",headerbuf="";
int datapos, discarded_bytes;

#if constant(thread_create)
object conthread;
#endif

local function request_ok,request_fail;
array extra_args;

/****** internal stuff *********************************************/

static int ponder_answer( int|void start_position )
{
   // read until we have all headers

   int i = start_position, j = 0;
   for(;;)
   {
      string s;

      if (i<0) i=0;
      j=search(buf, "\r\n\r\n", i); if(j==-1) j=10000000;
      i=search(buf, "\n\n", i);     if(i==-1) i=10000000;
      if ((i=min(i,j))!=10000000) break;

      s=con->read(8192,1);
#ifdef HTTP_QUERY_DEBUG
      werror("-> %O\n",s);
#endif
      if (!s) {
	errno = con->errno();
#ifdef HTTP_QUERY_DEBUG
	werror ("<- (read error: %s)\n", strerror (errno));
#endif
	return 0;
      }
      if (s=="") {
	if (sizeof (buf) <= start_position) {
	  // FIXME: Try to fake some kind of errno here, or HTTP
	  // error?
#ifdef HTTP_QUERY_DEBUG
	  werror ("<- (premature EOF)\n");
#endif
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

#ifdef HTTP_QUERY_DEBUG
   werror("** %d bytes of header; %d bytes left in buffer (pos=%d)\n",
	  sizeof(headerbuf),sizeof(buf)-datapos,datapos);
#endif

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

static void connect(string server,int port,int blocking)
{
#ifdef HTTP_QUERY_DEBUG
   werror("<- (connect %O:%d)\n",server,port);
#endif

   int success;
   if(con->_fd)
     success = con->connect(server, port);
   else
     // What is this supposed to do? /mast
     success = con->connect(server, port, blocking);

   if(!success) {
     errno = con->errno();
#ifdef HTTP_QUERY_DEBUG
     werror("<- (connect error: %s)\n", strerror (errno));
#endif
     //con->set_blocking(); // Only to remove callbacks to avoid cycles.
     con->close();
     //destruct(con);
     con = 0;
     ok = 0;
     return;
   }

#ifdef HTTP_QUERY_DEBUG
   werror("<- %O\n",request);
#endif

#if constant(SSL.Cipher.CipherAlgorithm)
   if(https) {
     // Create a context
     SSL.context context = SSL.context();
     // Allow only strong crypto
     context->preferred_suites = ({
       //Strong ciphersuites.
       SSL_rsa_with_idea_cbc_sha,
       SSL_rsa_with_rc4_128_sha,
       SSL_rsa_with_rc4_128_md5,
       SSL_rsa_with_3des_ede_cbc_sha,
#if 0
       //Weaker ciphersuites.
       SSL_rsa_export_with_rc4_40_md5,
       SSL_rsa_export_with_rc2_cbc_40_md5,
       SSL_rsa_export_with_des40_cbc_sha,
#endif /* 0 */
     });
     string ref;
     context->random = Crypto.Random.random_string;
     
     object read_callback=con->query_read_callback();
     object write_callback=con->query_write_callback();
     object close_callback=con->query_close_callback();
     
     ssl = SSL.sslfile(con, context, 1,blocking);
     if(!blocking) {
       ssl->set_read_callback(read_callback);
       ssl->set_write_callback(write_callback);
       ssl->set_close_callback(close_callback);
     }
     con=ssl;
   }
#endif

   if (con->write(request) != sizeof (request)) {
     errno = con->errno();
#ifdef HTTP_QUERY_DEBUG
     werror ("-> (write error: %s)\n", strerror (errno));
#endif
   }
   else
     ponder_answer();
}

static void async_close()
{
  if(!https)
  {
    con->set_blocking();
    ponder_answer();
  }
}

static void async_read(mixed dummy,string s)
{
#ifdef HTTP_QUERY_DEBUG
   werror("-> %d bytes of data\n",sizeof(s));
#endif

   buf+=s;
   if (has_value(buf, "\r\n\r\n") || has_value(buf,"\n\n"))
   {
      con->set_blocking();
      ponder_answer();
   }
}

static void async_write()
{
   con->set_blocking();
#ifdef HTTP_QUERY_DEBUG
   werror("<- %O\n",request);
#endif
   if (con->write(request) != sizeof (request)) {
     errno = con->errno();
#ifdef HTTP_QUERY_DEBUG
     werror ("-> (write error: %s)\n", strerror (errno));
#endif
   }
   con->set_nonblocking(async_read,0,async_close);
}

static void async_connected()
{
   con->set_nonblocking(async_read,async_write,async_close);
#ifdef HTTP_QUERY_DEBUG
   werror("<- %O\n","");
#endif
   con->write("");
}

static void async_failed()
{
   if (con) errno=con->errno(); else errno=113; // EHOSTUNREACH
   ok=0;
   if (request_fail) request_fail(this,@extra_args);
   remove_call_out(async_timeout);
}

static void async_timeout()
{
#ifdef HTTP_QUERY_DEBUG
   werror("** TIMEOUT\n");
#endif
   errno=110; // timeout
   if (con)
   {
      //con->set_blocking(); // Only to remove callbacks to avoid cycles.
      con->close();
      //destruct(con);
   }
   con=0;
   async_failed();
}

void async_got_host(string server,int port)
{
   if (!server)
   {
      async_failed();
      if (con) {
	//con->set_blocking(); // Only to remove callbacks to avoid cycles.
	con->close();	//  we may be destructed here
	//catch { destruct(con); };
      }
      return;
   }

   con = Stdio.File();
   con->async_connect(server, port,
		      lambda(int success)
		      {
			if (success) {
			  // Connect ok.
#if constant(SSL.Cipher.CipherAlgorithm)
			  if(https) {
			    //Create a context
			    SSL.context context = SSL.context();
			    // Allow only strong crypto
			    context->preferred_suites = ({
			      SSL_rsa_with_idea_cbc_sha,
			      SSL_rsa_with_rc4_128_sha,
			      SSL_rsa_with_rc4_128_md5,
			      SSL_rsa_with_3des_ede_cbc_sha,
			    });
			    string ref;
			    context->random = Crypto.Random.random_string;
		 
			    ssl = SSL.sslfile(con, context, 1,0);
			    ssl->set_nonblocking(0,async_connected,async_failed);
			    con=ssl;
			  }
			  else
#endif
			    async_connected();
			} else {
			  // Connect failed.
			  async_failed();
			}
		      });
   // NOTE: In case of failure the timeout is already in place.
}

void async_fetch_read(mixed dummy,string data)
{
#ifdef HTTP_QUERY_DEBUG
   werror("-> %d bytes of data\n",sizeof(data));
#endif
   buf+=data;

   if (sizeof(buf)-datapos>=(int)headers["content-length"])
   {
      con->set_nonblocking(0,0,0);
      request_ok(@extra_args);
   }
}

void async_fetch_close()
{
#ifdef HTTP_QUERY_DEBUG
   werror("-> close\n");
#endif
   if (con) {
     //con->set_blocking();
     con->close();
     //destruct(con);
     con=0;
   }
   if (request_ok) (request_ok)(@extra_args);
}

/****** utilities **************************************************/

string headers_encode(mapping(string:array(string)|string) h)
{
   if (!h || !sizeof(h)) return "";
   String.Buffer buf = String.Buffer();
   foreach(h; string name; array(string)|string value)
     if(stringp(value))
       buf->add( String.capitalize(replace(name,"_","-")), ": ",
		 value, "\r\n" );
     else if(!value)
       continue;
     else if (intp(value))
       buf->add( String.capitalize(replace(name,"_","-")), ": ",
		 (string)value, "\r\n" );
     else if (arrayp(value)) {
       foreach(value, string value)
	 buf->add( String.capitalize(replace(name,"_","-")), ": ",
		   value, "\r\n" );
     } else {
       error("Protocols.HTTP.Query()->headers_encode(): Bad header: %O:%O.\n",
	     name, value);
     }
   return (string)buf;
}

/****** helper methods *********************************************/

//!	Set this to a global mapping if you want to use a cache,
//!	prior of calling *request().
//!
mapping hostname_cache=([]);

static Protocols.DNS.async_client async_dns;
static int last_async_dns;
static mixed async_id;

#ifndef PROTOCOLS_HTTP_DNS_OBJECT_TIMEOUT
#define PROTOCOLS_HTTP_DNS_OBJECT_TIMEOUT	60
#endif

// Check if it's time to clean up the async dns object.
static void clean_async_dns()
{
  int time_left = last_async_dns + PROTOCOLS_HTTP_DNS_OBJECT_TIMEOUT - time(1);
  if (time_left >= 0) {
    // Not yet.
    async_id = call_out(clean_async_dns, time_left + 1);
    return;
  }
  async_id = 0;
  async_dns = 0;
  last_async_dns = 0;
}

void dns_lookup_callback(string name,string ip,function callback,
			 mixed ...extra)
{
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
   sscanf(hostname,"%[0-9.]",id);
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
   sscanf(hostname,"%[0-9.]",id);
   if (hostname==id ||
       (id=hostname_cache[hostname]))
      return id;

   array hosts = (Protocols.DNS.client()->gethostbyname( hostname )||
		  ({ 0, 0 }))[1];
   return sizeof(hosts) && hosts[random(sizeof( hosts ))];
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

   con=Stdio.File();
   if (!con->open_socket())
     error("HTTP.Query(): can't open socket; "+strerror(con->errno())+"\n");

   string server1=dns_lookup(server);

   if (server1) server=server1; // cheaty, if host doesn't exist

   // prepare the request

   errno = ok = protocol = headers = status_desc = status = datapos = 0;
   buf = "";

   if (!data) data="";

   if (!headers) headers="";
   else if (mappingp(headers))
   {
      headers=mkmapping(Array.map(indices(headers),lower_case),
			values(headers));

      if (data!="") headers->content_length=sizeof(data);

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
  string ip = dns_lookup( server );
  if(ip) server = ip; // cheaty, if host doesn't exist

  // start open the connection

  if(con && con->is_open() &&
     con->query_address() == server + " " + port &&
     headers && headers->connection &&
     lower_case( headers->connection ) != "close")
  {
#ifdef HTTP_QUERY_DEBUG
    werror("** Connection kept alive!\n");
#endif
    kept_alive = 1;
    // Remove unread data from the connection.
    this_program::data();
  }
  else
  {
    con = Stdio.File();
    if(!con->open_socket())
      error("HTTP.Query(): can't open socket; "+strerror(con->errno)+"\n");
  }

  // prepare the request

  errno = ok = protocol = headers = status_desc = status = datapos = 0;
  buf = "";

  if(!data)
    data = "";

  if(!stringp( http_headers )) {

    if(mappingp( http_headers ))
      http_headers = mkmapping(map(indices( http_headers ), lower_case),
			       values( http_headers ));
    else
      http_headers = ([]);

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

    if(data != "")
      http_headers->content_length = sizeof( data );

    http_headers = headers_encode( http_headers );
  }

  request = query + "\r\n" + http_headers + "\r\n" + data;

  if(kept_alive)
  {
#ifdef HTTP_QUERY_DEBUG
    werror("<- %O\n",request);
#endif
    if (con->write( request ) != sizeof (request)) {
      errno = con->errno;
#ifdef HTTP_QUERY_DEBUG
      werror ("-> (write error: %s)\n", strerror (errno));
#endif
    }
    else
      if (ponder_answer() == -1) {
	// The keepalive connection was closed from the server end.
	// Retry with a new one.
	con->close();
	con = 0;
	return sync_request (server, port, query, http_headers, data);
      }
  } else
    connect(server, port,1);

  return this;
}

this_program async_request(string server,int port,string query,
			   void|mapping|string headers,void|string data)
{
   // start open the connection

   call_out(async_timeout,timeout);

   // prepare the request

   if (!data) data="";

   if (!headers) headers="";
   else if (mappingp(headers))
   {
      headers=mkmapping(Array.map(indices(headers),lower_case),
			values(headers));

      if (data!="") headers->content_length=sizeof(data);

      headers=headers_encode(headers);
   }

   request=query+"\r\n"+headers+"\r\n"+data;

   if (!con)
   {
      dns_lookup_async(server,async_got_host,port);
      return this;
   }
   else
   {
      async_connected();
   }
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

   if (headers["transfer-encoding"] &&
       lower_case(headers["transfer-encoding"])=="chunked")
   {
      string rbuf=buf[datapos..];
      string lbuf="";

      for (;;)
      {
	 int len;
	 string s;

#ifdef HTTP_QUERY_NOISE
	 werror("got %d; chunk: %O left: %d\n",strlen(lbuf),rbuf[..40],strlen(rbuf));
#endif

	 if (sscanf(rbuf,"%x%*[ ]\r\n%s",len,s)==3)
	 {
	    if (len==0)
	    {
	       for (;;)
	       {
		  int i;
		  if ((i=search(rbuf,"\r\n\r\n"))==-1)
		  {
		     s=con->read(8192,1);
		     if (!s) {
		       errno = con->errno();
#ifdef HTTP_QUERY_DEBUG
		       werror ("<- (read error: %s)\n", strerror (errno));
#endif
		       return 0;
		     }
		     if (s=="") return lbuf;
		     rbuf+=s;
		     buf+=s;
		  }
		  else
		  {
#ifdef HTTP_QUERY_NOISE
		     werror("fin: buf len=%d; res len=%d\n",
			    strlen(buf),strlen(rbuf));
#endif
 	             // entity_headers=rbuf[..i-1];
		     return lbuf;
		  }
	       }
	    }
	    else
	    {
	       if (strlen(s)<len)
	       {
		  string t=con->read(len-strlen(s)+6); // + crlfx3
		  if (!t) {
		    errno = con->errno();
#ifdef HTTP_QUERY_DEBUG
		    werror ("<- (read error: %s)\n", strerror (errno));
#endif
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
	    s=con->read(8192,1);
	    if (!s) {
	      errno = con->errno();
#ifdef HTTP_QUERY_DEBUG
	      werror ("<- (read error: %s)\n", strerror (errno));
#endif
	      return 0;
	    }
	    if (s=="") return lbuf;
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

#ifdef HTTP_QUERY_DEBUG
   werror("fetch data: %d bytes needed, got %d, %d left\n",
	  len,sizeof(buf)-datapos,l);
#endif

   if(l>0 && con)
   {
     if(headers->server == "WebSTAR")
     { // Some servers reporting this name exhibit some really hideous behaviour:
       string s = con->read();
       if (!s) {
	 errno = con->errno();
#ifdef HTTP_QUERY_DEBUG
	 werror ("<- (read error: %s)\n", strerror (errno));
#endif
	 return 0;
       }
       buf += s; // First, they may well lie about the content-length
       if(!discarded_bytes && buf[datapos..datapos+1] == "\r\n")
	 datapos += 2; // And, as if that wasn't enough! *mumble*
     }
     else
     {
       string s = con->read(l);
       if (!s) {
	 errno = con->errno();
#ifdef HTTP_QUERY_DEBUG
	 werror ("<- (read error: %s)\n", strerror (errno));
#endif
	 return 0;
       }
       buf += s;
     }
   }
   if(zero_type( len ))
     len = sizeof( buf ) - datapos - 1;
   if(!zero_type(max_length) && len>max_length)
     len = max_length;
#ifdef HTTP_QUERY_NOISE
   werror("buf[datapos..]     : %O\n", buf[datapos
				       ..min(sizeof(buf), datapos+19)]);
   werror("buf[..datapos+len] : %O\n", buf[max(0, datapos+len-19)
				       ..min(sizeof(buf), datapos+len)]);
#endif
   return buf[datapos..datapos+len];
}

static Locale.Charset.Decoder charset_decoder;

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
      charset_decoder = Locale.Charset.decoder("ascii");
    else
      charset_decoder = Locale.Charset.decoder(charset);
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

//!	Gives back the number of downloaded bytes.
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

class PseudoFile
{
   string buf;
   object con;
   int len;
   int p=0;

   void create(object _con,string _buf,int _len)
   {
      con=_con;
      buf=_buf;
      len=_len;
      if (!con) len=sizeof(buf);
   }

   string read(int n)
   {
      string s;

      if (len && p+n>len) n=len-p;

      if (sizeof(buf)<n && con)
      {
	 string s=con->read(n-sizeof(buf));
	 if (!s) return 0;
	 buf+=s;
      }

      s=buf[..n-1];
      buf=buf[n..];
      p+=sizeof(s);
      return s;
   }

   void close()
   {
      con=0; // forget
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
      if (zero_type(headers["content-length"]))
	 len=0x7fffffff;
      else
	 len=sizeof(protocol+" "+status+" "+status_desc)+2+
	    sizeof(hbuf)+2+(int)headers["content-length"];
      return PseudoFile(con,
			protocol+" "+status+" "+status_desc+"\r\n"+
			hbuf+"\r\n"+buf[datapos..],len);
   }
   if (zero_type(headers["content-length"]))
      len=0x7fffffff;
   else
      len=sizeof(headerbuf)+4+(int)h["content-length"];
   return PseudoFile(con,buf,len);
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
   return PseudoFile(con,buf[datapos..],(int)headers["content-length"]);
}

static void destroy()
{
   if (async_id) {
     remove_call_out(async_id);
   }
   async_id = 0;
   async_dns = 0;

   catch (con->set_blocking()); // Only to remove callbacks to avoid cycles.
   catch { con->close(); };
   //catch { destruct(con); };
}

//!	Fetch all data in background.
void async_fetch(function callback,mixed ... extra)
{
   if (sizeof(buf)-datapos>=(int)headers["content-length"])
   {
      callback(@extra);
      return;
   }

   if (!con)
   {
      callback(@extra); // nothing to do, stupid...
      return;
   }
   extra_args=extra;
   request_ok=callback;
   con->set_nonblocking(async_fetch_read,0,async_fetch_close);
}

static string _sprintf(int t)
{
  return t=='O' && status && sprintf("%O(%d %s)", this_program,
				     status, status_desc);
}
