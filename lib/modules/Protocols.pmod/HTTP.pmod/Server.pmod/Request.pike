#pike __REAL_VERSION__

import ".";

constant MAXIMUM_REQUEST_SIZE=1000000;

Stdio.File my_fd;
Port server_port;
HeaderParser headerparser;

string buf="";    // content buffer

//! raw unparsed full request (headers and body)
string raw;

//! raw unparsed body of the request (@[raw] minus request line and headers)
string body_raw;

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

function(this_program:void) request_callback;

void attach_fd(Stdio.File _fd,Port server,
	       function(this_program:void) _request_callback)
{
   my_fd=_fd;
   server_port=server;
   headerparser=HeaderParser();
   request_callback=_request_callback;

   my_fd->set_nonblocking(read_cb,0,close_cb);
}

static void read_cb(mixed dummy,string s)
{
   if(!raw) raw="";
   raw+=s;
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
      {
	 my_fd->set_blocking();
	 populate_raw();
	 request_callback(this);
      }
   }
}

static void parse_request()
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
	    full_query=v[1..sizeof(v)-2]*" ";
	    break;
	 }
      case 2:
	 request_type=v[0];
	 protocol="HTTP/0.9";
	 full_query=v[1..]*" ";
	 break;
   }
   query="";
   sscanf(not_query=full_query,"%s?%s",not_query,query);

   if (request_headers->cookie)
      foreach (Array.arrayify(request_headers->cookie);;string cookie)
	 if (sscanf(cookie,"%s=%s",string a,string b)==2)
	    cookies[a]=b;
}

static int parse_variables()
{
   if (query!="")
      http_decode_urlencoded_query(query,variables);

   if (request_type=="POST" &&
       request_headers["content-type"]=="application/x-www-form-urlencoded")
   {
      if ((int)request_headers["content-length"]<=sizeof(buf))
      {
	 parse_post();
	 return 1;
      }
      my_fd->set_read_callback(read_cb_post);
      return 0; // delay
   }
   return 1;
}

static void populate_raw()
{
   int i=search(raw, "\r\n\r\n");
   if(i>0) body_raw=raw[i+4..];
   return;
}

static void parse_post()
{
   int n=(int)request_headers["content-length"];

   string s=buf[..n-1];
   buf=buf[n..];

   http_decode_urlencoded_query(s,variables);
}

static void read_cb_post(mixed dummy,string s)
{
   raw+=s;
   buf+=s;

   if (sizeof(buf)>=(int)request_headers["content-length"] ||
       sizeof(buf)>MAXIMUM_REQUEST_SIZE)
   {
      my_fd->set_blocking();
      populate_raw();
      parse_post();
      request_callback(this);
   }
}

static void close_cb()
{
// closed by peer before request read
}

string _sprintf(int t)
{
  return t=='O' && sprintf("%O(%O %O)",this_program,request_type,full_query);
}

// ----------------------------------------------------------------
function log_cb;
string make_response_header(mapping m)
{
   array(string) res=({});
   switch (m->error)
   {
      case 0:
      case 200: 
	 res+=({"HTTP/1.0 200 OK"}); // HTTP/1.1 when supported
	 break;
      case 302:
	 res+=({"HTTP/1.0 302 TEMPORARY REDIRECT"}); 
	 break;
      default:
   // better error names?
	 res+=({"HTTP/1.0 "+m->error+" ERROR"});
	 break;
   }

   if (!m->type)
      m->type=filename_to_type(not_query);

   res+=({"Content-Type: "+m->type});
   
   res+=({"Server: "+(m->server||http_serverid)});

   string http_now=http_date(time());
   res+=({"Date: "+http_now});

   if (!m->stat && m->file)
      m->stat=m->file->stat();

   if (m->modified)
      res+=({"Last-Modified: "+http_date(m->modified)});
   else if (m->stat)
      res+=({"Last-Modified: "+http_date(m->stat->mtime)});
   else
      res+=({"Last-Modified: "+http_now});

   if (m->extra_heads)
      foreach (m->extra_heads;string name;array|string arr)
	 foreach (Array.arrayify(arr);;string value)
	    res+=({String.capitalize(name)+": "+value});

// FIXME: insert cookies here?

   if (zero_type(m->size))
      if (m->data)
	 m->size=sizeof(m->data);
      else if (m->stat)
	 m->size=m->stat->size;
      else 
	 m->size=-1;

   if (m->size!=-1)
      res+=({"Content-Length: "+m->size});

   return res*"\r\n"+"\r\n\r\n";
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
//! @member int "length"
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
// insert HTTP 1.1 stuff here
   log_cb = _log_cb;

   string header=make_response_header(m);

   if (m->file && 
       m->size!=-1 && 
       m->size+sizeof(header)<4096) // fit in buffer
   {
      m->data=m->file->read(m->size);
      m->file->close();
      m->file=0;
   }

   if (request_type=="HEAD")
   {
      send_buf=header;
      if (m->file) m->file->close(),m->file=0;
   }
   else if (m->data)
      send_buf=header+m->data;
   else
      send_buf=header;

   if (sizeof(send_buf)<4096 &&
       !m->file)
   {
      sent = my_fd->write(send_buf);
      send_buf="";

      finish();
      return;
   }

   send_pos=0;
   my_fd->set_nonblocking(0,send_write,send_close);

   if (m->file)
   {
      send_fd=m->file;
      send_buf+=send_fd->read(8192); // start read-ahead if possible
   }
   else
      send_fd=0;
}

void finish()
{
   if( log_cb )
     log_cb(this);
   if (my_fd) my_fd->close();
}

int sent;
string send_buf="";
int send_pos;
Stdio.File send_fd=0;

void send_write()
{
   if (sizeof(send_buf)-send_pos<8192 &&
       send_fd)
   {
      string q=send_fd->read(131072);
      if (!q || q=="")
      {
	 send_fd->close();
	 send_fd=0;
      }
      else
      {
	 send_buf=send_buf[send_pos..]+q;
	 send_pos=0;
      }
   }
   else if (send_pos==sizeof(send_buf) && !send_fd)
   {
      finish();
      return;
   }

   int n=my_fd->write(send_buf[send_pos..]);
   sent += n;
   send_pos+=n;
}

void send_close()
{
/* socket closed by peer */
   finish();
}
