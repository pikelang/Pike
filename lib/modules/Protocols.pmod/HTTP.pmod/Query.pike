/*
**! module Protocols
**! submodule HTTP
**! class Query
**!
**! 	Open and execute a HTTP query.
**!
**! method object thread_request(string server,int port,string query);
**! method object thread_request(string server,int port,string query,mapping headers,void|string data);
**!	Create a new query object and begin the query.
**!	
**!	The query is executed in a background thread;
**!	call '() in this object to wait for the request
**!	to complete.
**!
**!	'query' is the first line sent to the HTTP server;
**!	for instance "GET /index.html HTTP/1.1".
**!
**!	headers will be encoded and sent after the first line,
**!	and data will be sent after the headers.
**!
**! returns the called object
**!
**! method object set_callbacks(function request_ok,function request_fail,mixed ...extra)
**! method object async_request(string server,int port,string query);
**! method object async_request(string server,int port,string query,mapping headers,void|string data);
**!	Setup and run an asynchronous request,
**!	otherwise similar to thread_request.
**!
**!	request_ok(object httpquery,...extra args)
**!	will be called when connection is complete,
**!	and headers are parsed.
**!
**!	request_fail(object httpquery,...extra args)
**!	is called if the connection fails.
**!
**! returns the called object
**!
**! variable int ok
**!	Tells if the connection is successfull.
**! variable int errno
**!	Errno copied from the connection.
**!
**! variable mapping headers
**!	Headers as a mapping. All header names are in lower case,
**!	for convinience.
**!
**! variable string protocol
**!	Protocol string, ie "HTTP/1.0".
**!
**! variable int status
**! variable string status_desc
**!	Status number and description (ie, 200 and "ok").
**!
**! variable mapping hostname_cache
**!	Set this to a global mapping if you want to use a cache,
**!	prior of calling *request().
**!
**! variable mapping async_dns
**!	Set this to an array of Protocols.DNS.async_clients,
**!	if you wish to limit the number of outstanding DNS
**!	requests. Example:
**!	   async_dns=allocate(20,Protocols.DNS.async_client)();
**!
**! method int `()()
**!	Wait for connection to complete.
**! returns 1 on successfull connection, 0 if failed
**!
**! method array cast("array")
**!	Gives back ({mapping headers,string data,
**!		     string protocol,int status,string status_desc});
**!
**! method mapping cast("mapping")
**!	Gives back 
**!	headers | 
**!	(["protocol":protocol,
**!	  "status":status number,
**!	  "status_desc":status description,
**!	  "data":data]);
**!		
**! method string cast("string")
**!	Gives back the answer as a string.
**!
**! method string data()
**!	Gives back the data as a string.
**!
**! method int downloaded_bytes()
**!	Gives back the number of downloaded bytes.
**!
**! method int total_bytes()
**!     Gives back the size of a file if a content-length header is present
**!     and parsed at the time of evaluation. Otherwise returns -1.
**!
**! object(pseudofile) file()
**! object(pseudofile) file(mapping newheaders,void|mapping removeheaders)
**! object(pseudofile) datafile();
**!	Gives back a pseudo-file object,
**!	with the method read() and close().
**!	This could be used to copy the file to disc at
**!	a proper tempo.
**!
**!	datafile() doesn't give the complete request, 
**!	just the data.
**!
**!	newheaders, removeheaders is applied as:
**!	<tt>(oldheaders|newheaders))-removeheaders</tt>
**!	Make sure all new and remove-header indices are lower case.
**!
**! void async_fetch(function done_callback);
**!	Fetch all data in background.
*/

#define error(S) throw( ({(S),backtrace()}) )

/****** variables **************************************************/

// open

int errno;
int ok;

mapping headers;

string protocol;
int status;
string status_desc;

int timeout=120; // seconds

// internal 

object con;
string request;

string buf="",headerbuf="";
int datapos;

#if constant(thread_create)
object conthread;
#endif

local function request_ok,request_fail;
array extra_args;

/****** internal stuff *********************************************/

static void ponder_answer()
{
   // read until we have all headers

   int i=0;
   int j=0;

   for (;;)
   {
      string s;

      if (i<0) i=0;
      j=search(buf,"\r\n\r\n",i); if (j==-1) j=10000000;
      i=search(buf,"\n\n",i);     if (i==-1) i=10000000;
      if ((i=min(i,j))!=10000000)  break;

      s=con->read(8192,1);
      if (!s || s=="") { i=strlen(buf); break; }
      
      i=strlen(buf)-3;
      buf+=s;
   }

   headerbuf=buf[..i-1]-"\n";

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
      headers[lower_case(n)]=d;
   }

   // done
   ok=1;
   remove_call_out(async_timeout);

   if (request_ok) request_ok(this_object(),@extra_args);
}

static void connect(string server,int port)
{
#ifdef HTTP_QUERY_DEBUG
   werror("<- (connect %O:%d)\n",server,port);
#endif
   if (catch { con->connect(server,port); })
   {
      if (!(errno=con->errno())) errno=22; /* EINVAL */
#ifdef HTTP_QUERY_DEBUG
      werror("<- (error %d)\n",errno);
#endif
      destruct(con);
      con=0;
      ok=0;
      return;
   }
#ifdef HTTP_QUERY_DEBUG
   werror("<- %O\n",request);
#endif
   con->write(request);

   ponder_answer();
}

static void async_close()
{
   con->set_blocking();
   ponder_answer();
}

static void async_read(mixed dummy,string s)
{
#ifdef HTTP_QUERY_DEBUG
   werror("-> %d bytes of data\n",strlen(s));
#endif
   
   buf+=s;
   if (-1!=search(buf,"\r\n\r\n") || -1!=search(buf,"\n\n")) 
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
   con->write(request);
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
   if (con) errno=con->errno; else errno=113; // EHOSTUNREACH
   ok=0;
   if (request_fail) request_fail(this_object(),@extra_args);
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
      catch { con->close(); };
      destruct(con); 
   }
   con=0;
   async_failed();
}

void async_got_host(string server,int port)
{
   if (!server)
   {
      async_failed();
      catch { destruct(con); }; //  we may be destructed here
      return;
   }
   if (catch
   {
      con=Stdio.File();
      if (!con->open_socket())
	 error("HTTP.Query(): can't open socket; "+strerror(con->errno)+"\n");
   }) 
   {
      return; // got timeout already, we're destructed
   }

   con->set_nonblocking(0,async_connected,async_failed);

   //   werror(server+"\n");

   if (catch { con->connect(server,port); })
   {
      if (!(errno=con->errno())) errno=22; /* EINVAL */
      destruct(con);
      con=0;
      ok=0;
      async_failed();
   }
}

void async_fetch_read(mixed dummy,string data)
{
#ifdef HTTP_QUERY_DEBUG
   werror("-> %d bytes of data\n",strlen(data));
#endif
   buf+=data;
}

void async_fetch_close()
{
#ifdef HTTP_QUERY_DEBUG
   werror("-> close\n");
#endif
   con->set_blocking();
   destruct(con);
   con=0;
   request_ok(@extra_args);
}

/****** utilities **************************************************/

string headers_encode(mapping h)
{
   if (!h || !sizeof(h)) return "";
   return Array.map( indices(h),
		     lambda(string hname,mapping headers)
		     {
			if (stringp(headers[hname]) ||
			    intp(headers[hname]))
			   return String.capitalize(replace(hname,"_","-")) + 
			      ": " + headers[hname];
		     }, h )*"\r\n" + "\r\n";
}

/****** helper methods *********************************************/

mapping hostname_cache=([]);
array async_dns=0;

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
   if (hostname=="")
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

   if (!async_dns)
      Protocols.DNS.async_client()
	 ->host_to_ip(hostname,dns_lookup_callback,callback,@extra);
   else
      async_dns[random(sizeof(async_dns))]->
	 host_to_ip(hostname,dns_lookup_callback,callback,@extra);
}

string dns_lookup(string hostname)
{
   string id;
   sscanf(hostname,"%*[0-9.]",id);
   if (hostname==id ||
       (id=hostname_cache[hostname]))
      return id;

   array hosts=((Protocols.DNS.client()->gethostbyname(hostname)||
		 ({0,0}))[1]+({0}));
   return hosts[random(sizeof(hosts))];
}


/****** called methods *********************************************/

object set_callbacks(function(object,mixed...:mixed) _ok,
		     function(object,mixed...:mixed) _fail,
		     mixed ...extra)
{
   extra_args=extra;
   request_ok=_ok;
   request_fail=_fail;
   return this_object();
}

#if constant(thread_create)

object thread_request(string server,int port,string query,
		      void|mapping|string headers,void|string data)
{
   // start open the connection
   
   con=Stdio.File();
   if (!con->open_socket())
      error("HTTP.Query(): can't open socket; "+strerror(con->errno)+"\n");

   string server1=dns_lookup(server);

   if (server1) server=server1; // cheaty, if host doesn't exist

   // prepare the request

   if (!data) data="";

   if (!headers) headers="";
   else if (mappingp(headers))
   {
      headers=mkmapping(Array.map(indices(headers),lower_case),
			values(headers));

      if (data!="") headers->content_length=strlen(data);

      headers=headers_encode(headers);
   }
   
   request=query+"\r\n"+headers+"\r\n"+data;

   conthread=thread_create(connect,server,port);

   return this_object();
}

#endif

object sync_request(string server,int port,string query,
		    void|mapping|string headers,void|string data)
{
   // start open the connection
   
   con=Stdio.File();
   if (!con->open_socket())
      error("HTTP.Query(): can't open socket; "+strerror(con->errno)+"\n");

   string server1=dns_lookup(server);

   if (server1) server=server1; // cheaty, if host doesn't exist

   // prepare the request

   if (!data) data="";

   if (!headers) headers="";
   else if (mappingp(headers))
   {
      headers=mkmapping(Array.map(indices(headers),lower_case),
			values(headers));

      if (data!="") headers->content_length=strlen(data);

      headers=headers_encode(headers);
   }
   
   request=query+"\r\n"+headers+"\r\n"+data;

   connect(server,port);

   return this_object();
}

object async_request(string server,int port,string query,
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

      if (data!="") headers->content_length=strlen(data);

      headers=headers_encode(headers);
   }
   
   request=query+"\r\n"+headers+"\r\n"+data;
   
   dns_lookup_async(server,async_got_host,port);

   return this_object();
}

#if constant(thread_create)

int `()()
{
   // wait for completion
   if (conthread) conthread->wait();
   return ok;
}

#endif

string data()
{
#if constant(thread_create)
   `()();
#endif
   int len=(int)headers["content-length"];
   int l;
   if (zero_type(len)) 
      l=0x7fffffff;
   else
      l=len-strlen(buf)+datapos;
   if (l>0 && con)
   {
     if(headers->server == "WebSTAR")
     { // Some servers reporting this name exhibit some really hideous behaviour:
       buf += con->read(); // First, they may well lie about the content-length
       if(buf[datapos..datapos+1] == "\r\n")
	 datapos += 2; // And, as if that wasn't enough! *mumble*
     }
     else
       buf += con->read(l);
   }
   // note! this can't handle keep-alive × HEAD requests.
   return buf[datapos..];
}

int downloaded_bytes()
{
  if(datapos)
    return sizeof(buf)-datapos;
  else
    return 0;
}

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
      if (!con) len=strlen(buf);
   }

   string read(int n)
   {
      string s;
      
      if (len && p+n>len) n=len-p;

      if (strlen(buf)<n && con)
      {
	 string s=con->read(n-strlen(buf));
	 buf+=s;
      }

      s=buf[..n-1];
      buf=buf[n..];
      p+=strlen(s);
      return s;
   }

   void close()
   {
      con=0; // forget
   }
};

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
	 len=strlen(protocol+" "+status+" "+status_desc)+2+
	    strlen(hbuf)+2+(int)headers["content-length"];
      return PseudoFile(con,
			protocol+" "+status+" "+status_desc+"\r\n"+
			hbuf+"\r\n"+buf[datapos..],len);
   }
   if (zero_type(headers["content-length"]))
      len=0x7fffffff;
   else 
      len=strlen(headerbuf)+4+(int)h["content-length"];
   return PseudoFile(con,buf,len);
}

object datafile()
{
#if constant(thread_create)
   `()();
#endif
   return PseudoFile(con,buf[datapos..],(int)headers["content-length"]);
}

void destroy()
{
   catch { con->close(); destruct(con); };
}

void async_fetch(function callback,array ... extra)
{
   if (!con)
   {
      callback(@extra); // nothing to do, stupid...
      return; 
   }
   extra_args=extra;
   request_ok=callback;
   con->set_nonblocking(async_fetch_read,0,async_fetch_close);
}

/************************ example *****************************/

#if 0

object o=HTTP.Query();

void ok()
{
   write("ok...\n");
   write(sprintf("%O\n",o->headers));
   exit(0);
}

void fail()
{
   write("fail\n");
   exit(0);
}

int main()
{
   o->set_callbacks(ok,fail);
   o->async_request("www.roxen.com",80,"HEAD / HTTP/1.0");
   return -1;
}

#endif
