#pike __REAL_VERSION__

//! This class represents a connection from a client to the server.
//!
//! There are three different read callbacks that can be active, which
//! have the following call graphs. @[read_cb] is the default read
//! callback, installed by @[attach_fd].
//!
//! @code
//!     | (Incoming data)
//!     v
//!   @[read_cb]
//!     | If complete headers are read
//!     v
//!   @[parse_request]
//!     v
//!   @[parse_variables]
//!     | If callback isn't changed to @[read_cb_chunked] or @[read_cb_post]
//!     v
//!   @[finalize]
//! @endcode
//!
//! @code
//!     | (Incoming data)
//!     v
//!   @[read_cb_post]
//!     | If enough data has been received
//!     v
//!   @[finalize]
//! @endcode
//!
//! @code
//!     | (Incoming data)
//!     v
//!   @[read_cb_chunked]
//!     | If all data chunked transfer-encoding needs
//!     v
//!   @[finalize]
//! @endcode


int max_request_size = 0;

void set_max_request_size(int size)
{
  max_request_size = size;
}

protected class Port {
  Stdio.Port port;
  int portno;
  string|int(0..0) interface;
  function(.Request:void) callback;
  program request_program=.Request;
  void create(function(.Request:void) _callback,
	      void|int _portno,
	      void|string _interface);
  void close();
  protected void _destruct();
}

//! The socket that this request came in on.
Stdio.NonblockingStream my_fd;

Port server_port;
.HeaderParser headerparser;

string buf="";    // content buffer

//! raw unparsed full request (headers and body)
string raw = "";

//! raw unparsed body of the request (@[raw] minus request line and headers)
string body_raw="";

//! full request line (@[request_type] + @[full_query] + @[protocol])
string request_raw;

//! HTTP request method, eg. POST, GET, etc.
string request_type;

//! full resource requested, including attached GET query
string full_query;

//! resource requested minus any attached query
string not_query;

//! query portion of requested resource, starting after the first "?"
string query;

//! request protocol and version, eg. HTTP/1.0
string protocol;

int started = time();

//! all headers included as part of the HTTP request, ie content-type.
mapping(string:string|array(string)) request_headers=([]);

//! all variables included as part of a GET or POST request.
mapping(string:string|array(string)) variables=([]);

//! cookies set by client
mapping(string:string) cookies=([]);

//! external use only
mapping misc=([]);

//! send timeout (no activity for this period with data in send buffer)
//! in seconds, default is 180
int send_timeout_delay=180;

//! connection timeout, delay until connection is closed while
//! waiting for the correct headers:
int connection_timeout_delay=180;

function(this_program:void) request_callback;
function(this_program,array:void) error_callback;

System.Timer startt = System.Timer();

void attach_fd(Stdio.NonblockingStream _fd, Port server,
	       function(this_program:void) _request_callback,
	       void|string already_data,
	       void|function(this_program,array:void) _error_callback)
{
   my_fd=_fd;
   server_port=server;
   headerparser = .HeaderParser();
   request_callback=_request_callback;
   error_callback = _error_callback;
   my_fd->set_nonblocking(read_cb,0,close_cb);
   call_out(connection_timeout,connection_timeout_delay);
   if (already_data && strlen(already_data))
      read_cb(0,already_data);
}


// Some (wap-gateways, specifically) servers send multiple
// content-length, as an example..
constant singular_headers = ({
    "content-length",
    "content-type",
    "connection",
    "transfer-encoding",
    "if-modified-since",
    "date",
    "pragma",
    "range",
});

// We use these as if we only got one, regardless of the number of
// headers received.
constant singular_use_headers = ({
    "cookie",
    "cookie2",
});



private int sent;
private OutputBuffer send_buf = OutputBuffer();
private Stdio.File send_fd=0;
private int send_stop;
private int keep_alive=0;

protected void flatten_headers()
{
  foreach( singular_headers, string x )
    if( arrayp(request_headers[x]) )
      request_headers[x] = request_headers[x][-1];

  foreach( singular_use_headers, string x )
    if( arrayp(request_headers[x]) )
      request_headers[x] = request_headers[x]*";";
}

//! Called when the client is attempting opportunistic TLS on this
//! HTTP port. Overload to handle, i.e. send the data to a TLS
//! port. By default the connection is simply closed.
void opportunistic_tls(string s)
{
  close_cb();
}

// Appends data to raw and feeds the header parse with data. Once the
// header parser has enough data parse_request() and parse_variables()
// are called. If parse_variables() deems the request to be finished
// finalize() is called. If not parse_variables() has replaced the
// read callback.
protected void read_cb(mixed dummy,string s)
{
   if( !sizeof( raw ) )
   {
     // Opportunistic TLS.
     if( has_prefix(s, "\x16\x03\x01") )
     {
       opportunistic_tls(s);
       return;
     }

      sscanf(s,"%*[ \t\n\r]%s", s );
      if( !strlen( s ) )
         return;
   }
   raw+=s;
   remove_call_out(connection_timeout);
   array v=headerparser->feed(s);
   if (v)
   {
      destruct(headerparser);
      headerparser=0;
      buf=v[0];
      request_headers=v[2];

      request_raw=v[1];
      parse_request();

      if (parse_variables())
         finalize();
   }
   else
      call_out(connection_timeout,connection_timeout_delay);
}

protected void connection_timeout()
{
   finish(0);
}

// Parses the request and populates request_type, protocol,
// full_query, query and not_query.
protected void parse_request()
{
   array v=request_raw/" ";
   switch (sizeof(v))
   {
      case 0:
	 request_type="GET";
	 protocol="HTTP/0.9";
	 full_query="";
	 break;
      case 1:
	 request_type="GET";
	 protocol="HTTP/0.9";
	 full_query=v[0];
	 break;
      default:
	 if (v[-1][..3]=="HTTP")
	 {
	    request_type=v[0];
	    protocol=v[-1];
            if(!(< "HTTP/1.0", "HTTP/1.1" >)[protocol])
            {
              int maj, min;
              if(sscanf(protocol, "HTTP/%d.%d", maj, min)==2)
                protocol = sprintf("HTTP/%d.%d", maj, min);
            }
            // FIXME: This is explicitly against RFC7230. Instead a
            // 400 Bad Request response should be given, or if we want
            // to fix the request-target, a 301 Moved Permanently to
            // the fixed location.
	    full_query=v[1..<1]*" ";
	    break;
	 }
         // Fallthrough
      case 2:
	 request_type=v[0];
	 protocol="HTTP/0.9";
	 full_query=v[1..]*" ";
	 break;
   }

   query = "";
   not_query = full_query;
   sscanf(full_query, "%s?%s", not_query, query);
}

enum ChunkedState {
    READ_SIZE = 1,
    READ_CHUNK,
    READ_POSTNL,
    READ_TRAILER,
    FINISHED,
};

private ChunkedState chunked_state = READ_SIZE;
private int chunk_size;
private string current_chunk = "";
private string actual_data = "";
private string trailers = "";

// Appends data to raw and buf. Parses the data with the clunky-
// chunky-algorithm and, when all data has been received, updates
// body_raw and request_headers and calls finalize.
private void read_cb_chunked( mixed dummy, string data )
{
  raw += data;
  buf += data;
  remove_call_out(connection_timeout);
  while( chunked_state == FINISHED || strlen( buf ) )
  {
    switch( chunked_state )
    {
      case READ_SIZE:
	if( !has_value( buf, "\r\n" ) )
	  return;
	// SIZE[ extension]*\r\n
	sscanf( buf, "%x%*[^\r\n]\r\n%s", chunk_size, buf);
	if( chunk_size == 0 )
	  chunked_state = READ_TRAILER;
	else
	  chunked_state = READ_CHUNK;
	break;

      case READ_CHUNK:
	int l = min( strlen(buf), chunk_size );
	chunk_size -= l;
	actual_data += buf[..l-1];
	buf = buf[l..];
	if( !chunk_size )
	  chunked_state = READ_POSTNL;
	break;

      case READ_POSTNL:
	if( strlen( buf ) < 2 )
	  return;
	if( has_prefix( buf, "\r\n" ) )
	  buf = buf[2..];
	chunked_state = READ_SIZE;
	break;

      case READ_TRAILER:
	trailers += buf;
	buf = "";
	if( has_value( trailers, "\r\n\r\n" ) || has_prefix( trailers, "\r\n" ) )
	{
	  chunked_state = FINISHED;
	  if( !has_prefix( trailers, "\r\n" ) )
	    sscanf( trailers, "%s\r\n\r\n%s", trailers, buf );
	  else
	  {
	    buf = buf[2..];
	    trailers = "";
	  }
	}
	break;

      case FINISHED:
	foreach( trailers/"\r\n"-({""}), string header )
	{
	  string hk, hv;
	  if( sscanf( header, "%s:%s", hk, hv ) == 2 )
	  {
	    hk = String.trim_whites(lower_case(hk));
            hv = String.trim_whites(hv);

            // RFC 7230 4.1.2: Ignore framing, routing, modifiers,
            // authentication and response control headers.
            if( (< "transfer-encoding", "content-length",
                   "host", "cache-control", "expect",
                   "max-forwards", "pragma", "range", "te",
                   "age", "expires", "date", "location",
                   "retry-after", "vary", "warning",
                   "authentication", "proxy-authenticate",
                   "proxy-authorization", "www-authenticate" >)[ hk ] )
              continue;

	    if( request_headers[hk] )
	    {
	      if( !arrayp( request_headers[hk] ) )
		request_headers[hk] = ({request_headers[hk]});
	      request_headers[hk]+=({hv});
	    }
	    else
              request_headers[hk] = hv;
	  }
	}


	// And FINALLY we are done..
	body_raw = actual_data;
	request_headers["content-length"] = ""+strlen(actual_data);
	finalize();
	return;
    }
  }
  call_out(connection_timeout,connection_timeout_delay);
}

protected int parse_variables()
{
  if (query!="")
    .http_decode_urlencoded_query(query,variables);

  flatten_headers();

  if ( request_headers->expect )
  {
    if ( lower_case(request_headers->expect) == "100-continue" )
	my_fd->write("HTTP/1.1 100 Continue\r\n\r\n");
  }

  if( request_headers["transfer-encoding"] &&
      has_value(lower_case(request_headers["transfer-encoding"]),"chunked"))
  {
    my_fd->set_read_callback(read_cb_chunked);
    read_cb_chunked(0,"");
    return 0;
  }

  int l = (int)request_headers["content-length"];
  if (l<=sizeof(buf))
  {
    int zap = strlen(buf) - l;
    raw = raw[..<zap-1];
    body_raw = buf[..l-1];
    buf = buf[l..];
    return 1;
  }

  my_fd->set_read_callback(read_cb_post);
  return 0; // delay
}

protected void update_mime_var(string name, string new)
{
  string|array val = variables[name];
  if( !val )
  {
    variables[name] = new;
    return;
  }
  if( !arrayp(val) )
    variables[name] = ({ val });
  variables += ({ new });
}

protected void parse_post()
{
  if ( request_headers["content-type"] &&
       has_prefix(request_headers["content-type"], "multipart/form-data") )
  {
    MIME.Message messg = MIME.Message(body_raw, request_headers, 0, 1);
    if(!messg->body_parts) return;

    foreach(messg->body_parts, object part)
    {
      string name = part->disp_params->name;
      if(!name) continue;
      if(part->disp_params->filename) {
        update_mime_var(name, part->getdata());
        update_mime_var(name+".filename", part->disp_params->filename);
        update_mime_var(name+".mimetype", part->disp_params->filename);
      }
      else
	variables[name] = part->getdata();
    }
  }
  else if( request_headers["content-type"] &&
	   ( has_value(request_headers["content-type"], "url-encoded") ||
             has_value(request_headers["content-type"], "urlencoded") ))

  {
    .http_decode_urlencoded_query(body_raw,variables);
  }
}

protected void finalize()
{
  my_fd->set_blocking();
  flatten_headers();
  if (array err = catch {parse_post();})
  {
    if (error_callback) error_callback(this, err);
    else throw(err);
  }
  else
  {
    if (request_headers->cookie)
      foreach (request_headers->cookie/";";;string cookie)
        if (sscanf(String.trim_whites(cookie),"%s=%s",string a,string b)==2)
          cookies[a]=b;
    request_callback(this);
  }
}

// Adds incoming data to raw and buf. Once content-length or
// max_request_size data has been received, finalize is called.
protected void read_cb_post(mixed dummy,string s)
{
  raw += s;
  buf += s;
  remove_call_out(connection_timeout);

  int l = (int)request_headers["content-length"];
  if (sizeof(buf)>=l ||
      ( max_request_size && sizeof(buf)>max_request_size ))
  {
    body_raw=buf[..l-1];
    buf = buf[l..];
    raw = raw[..strlen(raw)-strlen(buf)];
    finalize();
  }
  else
    call_out(connection_timeout,connection_timeout_delay);
}

protected void close_cb()
{
// closed by peer before request read
   if (my_fd) { my_fd->close(); destruct(my_fd); my_fd=0; }
}

protected string _sprintf(int t)
{
  return t=='O' && sprintf("%O(%O %O)",this_program,request_type,full_query);
}

// ----------------------------------------------------------------
function log_cb;

string make_response_header(mapping m)
{
   return (string)low_make_response_header(m,Stdio.Buffer());
}

Stdio.Buffer low_make_response_header(mapping m, Stdio.Buffer res)
{
   void radd( mixed ... args )
   {
      res->add(@(array(string))args,"\r\n");
   };

   if (protocol!="HTTP/1.0")
   {
      if (protocol=="HTTP/1.1")
      {
   // check for fire and forget here and go back to 1.0 then
      }
      else
	 protocol="HTTP/1.0";
   }

   if (!m->file && !m->data)
      m->data="";
   else if (!m->stat && m->file)
      m->stat=m->file->stat();

   if (undefinedp(m->size))
   {
      if (m->data)
	 m->size=sizeof(m->data);
      else if (m->stat)
      {
	 m->size=m->stat->size;
         if( m->file )
            m->size -= m->file->tell();
      }
      else
	 m->size=-1;
   }

   if (m->size!=-1)
   {
      if (undefinedp(m->start) && m->error==206)
      {
	 if (m->stop==-1) m->stop=m->size-1;
	 if (m->start>=m->size ||
	     m->stop>=m->size ||
	     m->stop<m->start ||
	     m->size<0)
	    res->error = 416;
      }
   }

   switch (m->error)
   {
      case 0:
      case 200:
         if (undefinedp(m->start))
	    radd(protocol," 200 OK"); // HTTP/1.1 when supported
	 else
	 {
	    radd(protocol," 206 Partial content");
	    m->error=206;
	 }
	 break;
      default:
         if(Protocols.HTTP.response_codes[(int)m->error])
            radd(protocol," ", Protocols.HTTP.response_codes[(int)m->error]);
         else
            radd(protocol," ",m->error," ERROR");
         break;
   }

   array extra = ({});
   if (m->extra_heads)
   {
      foreach (m->extra_heads;string name;array|string arr)
      {
        extra += ({ lower_case(name) });
        if( name=="connection" && has_value(arr, "keep-alive") )
          keep_alive=1;
        foreach (Array.arrayify(arr);;string value)
          radd(name, ": ", value);
      }
   }

   if( !has_value(extra, "content-type") )
   {
     if (!m->type)
       m->type = .filename_to_type(not_query);
     radd("Content-Type: ", m->type);
   }

   if( m->error == 206 && !has_value(extra, "content-range") )
      radd("Content-Range: bytes ", m->start,"-",m->stop,"/",
           has_index(m, "instance_size") ? m->instance_size : "*");


   if( m->size >= 0 && !has_value(extra, "content-length") )
      radd("Content-Length: ",(string)m->size);

   if( !has_value(extra, "server") )
     radd("Server: ", m->server || .http_serverid);

   string http_now = .http_date(time(1));
   if( !has_value(extra, "date") )
   {
     radd("Date: ",http_now);
   }

   if( !has_value(extra, "last-modified") )
   {
     if (m->modified)
       radd("Last-Modified: ", .http_date(m->modified));
     else if (m->stat)
       radd("Last-Modified: ", .http_date(m->stat->mtime));
     else
       radd("Last-Modified: ", http_now);
   }

   if( !has_value(extra, "connection") )
   {
     string cc = lower_case(request_headers["connection"]||"");
     if( (protocol=="HTTP/1.1" && !has_value(cc,"close")) || cc=="keep-alive" )
     {
       radd("Connection: keep-alive");
       keep_alive=1;
     }
     else
     {
       radd("Connection: close");
     }
   }

   res->add("\r\n");
   return res;
}

//! Return the IP address that originated the request, or 0 if
//! the IP address could not be determined. In the event of an
//! error, @[my_fd]@tt{->errno()@} will be set.
string get_ip()
{
   if (!my_fd) return 0;
   string addr = my_fd->query_address();
   if (!addr) return 0;
   sscanf(addr,"%s ",addr);
   return addr;
}

//! return a properly formatted response to the HTTP client
//! @param m
//! Contains elements for generating a response to the client.
//! @mapping m
//! @member string "data"
//!   Data to be returned to the client.
//! @member object "file"
//!   File object, the contents of which will be returned to the client.
//! @member int "error"
//!   HTTP error code
//! @member int "size"
//!   length of content returned. If @i{file@} is provided, @i{size@}
//!   bytes will be returned to client.
//! @member string "modified"
//!   contains optional modification date.
//! @member string "type"
//!   contains optional content-type
//! @member mapping "extra_heads"
//!   contains a mapping of additional headers to be
//! returned to client.
//! @member string "server"
//!   contains the server identification header.
//! @endmapping
void response_and_finish(mapping m, function|void _log_cb)
{
   string tmp;
   m += ([ ]);
   log_cb = _log_cb;

   if( !my_fd )
       return;

   if ((tmp = request_headers->range) && !m->start && undefinedp(m->error))
   {
      /*
       * The instance-size is the full size of the resource, while the content-length
       * of a range response is the length of the actual data returned.
       */
      if (!has_index(m, "instance_size")) {
         if (m->file) {
            if (!m->stat) m->stat = m->file->stat();
            if (m->stat) m->instance_size = m->stat->size;
            else if (has_index(m, "size")) m->instance_size = m->size;
         } else if (m->data) {
            m->instance_size = sizeof(m->data);
         } else if (has_index(m, "size")) m->instance_size = m->size;
      }

      int a,b;
      if (sscanf(tmp,"bytes%*[ =]%d-%d",a,b)==3) {
         m->start=a;
         m->stop=b;
      }
      else if (sscanf(tmp,"bytes%*[ =]-%d",b)==2)
      {
         if( !has_index(m, "instance_size") )
         {
	    m_delete(m,"file");
	    m->data="";
	    m->error=416;
         }
         else
         {
            m->start=max(0, m->instance_size-b);
            m->stop=-1;
         }
      }
      else if (sscanf(tmp,"bytes%*[ =]%d-",a)==2) {
         m->start=a;
         m->stop=-1;
      }
      else if (has_value(tmp, ","))
      {
         // Multiple ranges
         m_delete(m,"file");
         m->data="";
         m->error=416;
      }

      if (has_index(m, "instance_size")) {
         /* start > instance_size is an invalid request */
         if (m->start && m->start >= m->instance_size) {
            m_delete(m,"file");
            m->data="";
            m->error=416;
            m_delete(m,"start");
            m_delete(m,"stop");
         } else if (m->stop && m->stop >= m->instance_size) {
            m->stop = m->instance_size - 1;
         }
      }
   }

   if (request_headers["if-modified-since"])
   {
      int t = .http_decode_date(request_headers["if-modified-since"]);
      if (t)
      {
	 if (!m->stat && m->file)
	    m->stat=m->file->stat();
	 if (m->stat && m->stat->mtime<=t)
	 {
	    m_delete(m,"file");
	    m->data="";
	    m->error=304;
	 }
      }
   }

   if (request_headers["if-none-match"] && m->extra_heads )
   {
      string et;
      if((et = m->extra_heads->ETag) || (et =m->extra_heads->etag))
      {
         if( string key = request_headers["if-none-match"] )
         {
            if (key == et)
            {
               m_delete(m,"file");
               m->data="";
               m->error=304;
            }
         }
      }
   }

   if (m->stop) {
      if (m->stop != -1) {
         m->size=1+m->stop-m->start;
      } else if (m->instance_size) {
         m->stop = m->instance_size - 1;
         m->size = m->instance_size - m->start;
      }
   }

   low_make_response_header(m,send_buf);

   if (m->start) {
      if( m->file )
         m->file->seek(m->start, Stdio.SEEK_CUR);
      else if( m->data )
         m->data = ((string)m->data)[m->start..];
   }

   send_stop = sizeof(send_buf);

   if (request_type=="HEAD")
   {
      m->file=0;
      m->data=0;
      m->size=0;
   }
   else if (m->data)
   {
      send_buf->add(m->data);
   }
   if( m->size > 0 )
      send_stop+=m->size;

   if (m->file)
   {
      send_fd=m->file;
      send_buf->range_error(0);
   }

   my_fd->set_nonblocking(send_read,send_write,send_close);
}

//! Finishes this request, as in removing timeouts, calling the
//! logging callback etc. If @[clean] is given, then the processing of
//! this request went fine and all data was sent properly, in which
//! case the connection will be reused if keep-alive was
//! negotiated. Otherwise the connection will be closed and
//! destructed.
void finish(int clean)
{
   if( log_cb )
     log_cb(this);

   remove_call_out(send_timeout);
   remove_call_out(connection_timeout);

   send_buf = 0;

   if (!clean
       || !my_fd
       || !keep_alive)
   {
      if (my_fd) { catch(my_fd->close()); destruct(my_fd); my_fd=0; }
      return;
   }

   // create new request

   this_program r=server_port->request_program();
   r->attach_fd(my_fd,server_port,request_callback,buf,error_callback);

   my_fd=0; // and drop this object
}

class OutputBuffer
{
   inherit Stdio.Buffer;
   int range_error( int n )
   {
      if( send_fd )
      {
         int n = input_from( send_fd, min(128*1024,send_stop-sent-sizeof(this)) );
         if( n < 128*1024 )
         {
            send_fd = 0;
         }
         return !!n;
      }
   }
}

//! Returns the amount of data sent.
int sent_data()
{
   return sent;
}

void send_write()
{
   remove_call_out(send_timeout);
   call_out(send_timeout,send_timeout_delay);

   int n = send_buf->output_to(my_fd);

   // SSL.File->write() does not guarantee that all data has been
   // written upon return; this can cause premature disconnects.
   // By waiting for any buffers to empty before calling finish, we 
   // help ensure the full result has been transmitted.
   if(my_fd->query_version) {
     if( n == 0 && send_stop == sent)
        finish(1);
     else if(n <= 0)
        finish(sent == send_stop);
     else
        sent += n;
   } else {
     if ( n <= 0 || (send_stop==(sent+=n)) )
        finish(sent==send_stop);
   }
}

void send_timeout()
{
   finish(0);
}

void send_close()
{
/* socket closed by peer */
   finish(0);
}

void send_read(mixed dummy,string s)
{
   buf+=s; // for HTTP/1.1
}
