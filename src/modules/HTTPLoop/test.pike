string MP;

class request_program 
{
  inherit HTTPLoop.prog;
}

string type_from_fname(string fname)
{
  if(sscanf(fname, "%*s.ht")) return "text/html";
  if(sscanf(fname, "%*s.gi"))  return "image/gif";
  if(sscanf(fname, "%*s.jp"))  return "image/jpeg";
  return "text/plain";
}

void handle(object o)
{
  object fd = Stdio.File();
  string q = o->not_query;
  int len;
  if(!sizeof(q)) q="foo";
  if(q[-1]=='/') 
    q += "index.html";
  sscanf(q, "%*[/]%s", q);
  if(fd->open(combine_path(MP, q), "r"))
  {
    len = fd->stat()[1];
    string data = 
      ("HTTP/1.0 200 Ok\r\n"
       "Server: Quick'n'dirty\r\n"
       "Content-type: "+type_from_fname(q)+"\r\n"
       "Connection: Keep-Alive\r\n"
       "Content-length: "+len+"\r\n"
       "\r\n");
    if(len < 65535)
    {
      data += fd->read();
      o->reply_with_cache( data, 20 );
    } else {
      o->reply(data, fd, len);
    }
  } else {
    string er = "No such file";
    o->reply_with_cache("HTTP/1.0 402 No such file\r\n"
			"Server: Quick'n'dirty\r\n"
			"Content-type: text/plain\r\n"
			"Connection: Keep-Alive\r\n"
			"Content-length: "+sizeof(er)+"\r\n"
			"\r\n"+er, 200);
  }
}

function lw;
#define PCT(X) ((int)(((X)/(float)(c->total+0.1))*100))

void cs()
{
  call_out(cs, 10);
  catch
  {
    mapping c = HTTPLoop.cache_status()[0];
    werror("\nCache statistics\n");
    c->total = c->hits + c->misses + c->stale;
    werror(" %d elements in cache, size is %1.1fKb max is %1.1fMb\n"
	   " %d cache lookups, %d%% hits, %d%% misses and %d%% stale.\n",
	   c->entries, c->size/1024.0, c->max_size/(1024*1024.0),
	   c->total, PCT(c->hits), PCT(c->misses), PCT(c->stale));
  };
}

object port;
int main(int argc, array argv)
{
  MP = argv[-1];
  object t = Stdio.File();
  t->open("log", "cwt");

  thread_create(lambda(object lf) 
		{
		  while(1)
		  {
		    HTTPLoop.log_as_commonlog_to_file(lf);
		    sleep(2);
		  }
		}, t);

#if efun(thread_set_concurrency)
  thread_set_concurrency(10);
#endif
  port = files.port();
  if(!port->bind(3000))
  {
    werror("Bind failed.\n");
    return 1;
  }
  HTTPLoop.accept_http_loop( port, request_program, handle, 0, 1024*1024, 1);
  call_out(cs, 0, 0);
//   call_out(destruct, 20, port);
  return -1;
}
