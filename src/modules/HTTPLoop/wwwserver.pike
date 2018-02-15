import Stdio;

mapping exts = ([]);
constant days = ({ "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" });
constant months = ({ "Jan", "Feb", "Mar", "Apr", "May", "Jun",
		     "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" });

constant prefix = ({ "bytes", "kB", "MB", "GB", "TB", "HB"});

string sizetostring( int size )
{
  int s;
  if(size<0)
    return "--------";
  s=size*10; 
  size = 0;
  while( s > 10240 || size==0)
  {
    s /= 1024;
    size ++;
  }
  return sprintf("%d.%01d %s", s/10, s%10, prefix[ size ]);
}


// The server root.
#ifdef __NT__
#define BASE "H:\\html\\per.hedbor.org\\"
#else
#define BASE "/tmp/"
#endif

#define PORT 4002

string http_date(int t)
{
  mapping l = gmtime(t);
  return(sprintf("%s, %02d %s %04d %02d:%02d:%02d GMT",
		 days[l->wday], l->mday, months[l->mon], 1900+l->year,
		 l->hour, l->min, l->sec));
}


class request_program 
{
  inherit HTTPAccept.prog;
}

mixed handle(object o)
{
  string url = o->not_query;
  string f = combine_path(BASE, url[1..]);
  array|object s;
  if(f[-1] == '/' && (s = file_stat(f+"index.html"))) {
    url += "index.html";
    f += "index.html";
  } else
    s = file_stat(f);
  if(!s || s[1] == -1 || search(f, BASE) == -1)
  {
    string nofile = "<title>No such file</title>"
      "<h1>No such file or directory: "+f+"</h1>";
    o->reply_with_cache("HTTP/1.0 404 Unknown file\r\n"
			"Content-type: text/html\r\n"
			"Content-Length: "+sizeof(nofile)+"\r\n"
			"MIME-Version: 1.0\r\n"
			"Server: Neo-FastSpeed\r\n"
			"Connection: Keep-Alive\r\n"
			"\r\n"
			+nofile, 60);
  }
  else if(s[1] == -2) 
  {
    if(f[-1] == '/' || f[-1]=='\\')
    {
      string head = ("HTTP/1.1 200 Ok\r\n"
		     "Content-type: text/html\r\n"
		     "Last-Modified: "+http_date(s[4])+"\r\n"
		     "MIME-Version: 1.0\r\n"
		     "Server: Neo-FastSpeed\r\n"
		     "Connection: Keep-Alive\r\n");
      string res = ("<title>Directory listing of "+url+"</title>"
		    "<h1 align=center>Directory listing of "+
		    url+"</h1>\n<pre>");
      if(url != "/")
	res += "<b><a href=../>Parent Directory...</a></b>\n\n";
      string file2;
      foreach(sort(get_dir(f)||({})), string file) {
	int size = file_size(f+file);
	int islink;
#if constant(readlink)
	catch { readlink(f+file); islink = 1; };
#endif
	string ext = (file / ".") [-1];
	string ctype = (size >= 0 ? exts[ext] || "text/plain" :
			"Directory");
	if(size < -1)
	  file += "/";
	if(islink)
	  file2 = "@"+file;
	else
	  file2 = file;
	res += sprintf("<a href=%s>%-30s</a> %15s %s\n",
		       file, file2, sizetostring(size),
		       ctype);
      }
      return o->reply_with_cache(head +
				 "Content-Length: "+sizeof(res)+
				 "\r\n\r\n"+res, 30);
    } else {
      o->reply_with_cache("HTTP/1.0 302  Redirect\r\n"
			  "Content-type: text/plain\r\n"
			  "MIME-Version: 1.0\r\n"
			  "Server: Neo-FastSpeed\r\n"
			  "Connection: Keep-Alive\r\n"
			  "Content-Length: 0\r\n"
			  "Location: "+url+"/"+
			  (o->query ? "?"+o->query : "")+
			  "\r\n\r\n", 60);
    }
  } else {
    string ext = (f / ".") [-1];
    string ctype = exts[ext] || "text/plain";
#if 1
    o->reply_with_cache("HTTP/1.1 200 Ok\r\n"
			"Content-type: "+ctype+"\r\n"
			"Content-Length: "+s[1]+"\r\n"
			"Last-Modified: "+http_date(s[4])+"\r\n"
			"MIME-Version: 1.0\r\n"
			"Server: Neo-FastSpeed\r\n"
			"Connection: Keep-Alive\r\n"
			"\r\n"+
			read_bytes(f), 20);
#else
    o->reply("HTTP/1.1 200 Ok\r\n"
             "Content-type: "+ctype+"\r\n"
             "Content-Length: "+s[1]+"\r\n"
             "Last-Modified: "+http_date(s[4])+"\r\n"
             "MIME-Version: 1.0\r\n"
             "Server: Neo-FastSpeed\r\n"
             "Connection: Keep-Alive\r\n"
             "\r\n",
             Stdio.File(f, "r"), s[1]);
#endif
    destruct(o);
  }
}

object port;
object l;

int main(int argc, array (string) argv)
{
#if efun(thread_set_concurrency)
  thread_set_concurrency(100);
#endif
  port = Stdio.Port();
  array foo;
  foreach(read_file((argv[0] - "wwwserver.pike") + "extensions")/"\n",
	  string s) {
    if(sizeof(s) && s[0] != '#' && (foo = (s / "\t" - ({""}))) &&
       sizeof(foo) == 2)
      exts[foo[0]] = foo[1];
  }
  werror(sprintf("Found %d extensions...\n", sizeof(exts)));
  if(!port->bind(PORT))
  {
    werror("Bind failed.\n");
    return 1;
  }
  
  l = HTTPAccept.Loop( port, request_program, handle, 0, 1024*1024, 0, 
		       0);
  werror("WWW-server listening to port "+PORT+".\n");
  return -1;
}
