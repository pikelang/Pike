#pike __REAL_VERSION__

import .Helper;

/* debug */
// function f=lambda()
// {
//    call_out(f,0.5);

//    if (!con) 
//       return;

//    if (con->query_read_callback()!=recv)
//       werror("no read callback\n");

//    werror("asyncs: %4d queue: %4d out: %4d\n",
// 	  sizeof(indices(async)),
// 	  sizeof(sendqueue),
// 	  out_req);
// };
// mixed g=f();

//---------------- raw protocol --------------------------

/* connection object */
object con;

/* recieve buffer */
string buf="";

/* outstanding calls */
// static private
mapping(int:function(string:void)) async=([]);
int ref=1;

/* asynchronous messages callback list */
mapping(int:array(function(string:void))) async_callbacks=([]);

/* max number of outstanding requests */
int max_out_req=4;

/* queue of send-objects */
array(object) sendqueue=({});

/* outstanding requests */
int out_req;


/*--- send/recv ------------------------ */

static inline int conwrite(string what)
{
#ifdef LYSKOM_DEBUG
   werror("-> %O\n",what);
#endif
   int i=con->write(what)==sizeof(what);
   if (!i) { werror("write failed!!!\n"); _exit(1); }
   return i;
}

array komihåg=({});

class Send
{
   int ref;
   string request;
   function(string:void) callback;
   
   void create(int rf,string r,function(string:void) c)
   {
      ref=rf;
      request=r;
      callback=c;
   }
   int cast(string i) { return ref; }
   void write()
   {
      out_req++;
      async[ref]=callback;
      conwrite(ref+" "+request+"\r\n");
#ifdef LYSKOM_DEBUG
//       werror("LYSKOM inserting callback %O for call %d\n",callback,ref);
//       werror("async: %O\n",async);
#endif
      komihåg+=({function_object(callback)});
   }
}


int send(string request,
	 function(string:void) callback)
{
   int r=++ref;
   sendqueue+=({Send(r,request,callback)});
   flush_queue();
   return r;
}



/* FIXME: The code below isn't really threadsafe, but depends
   on context switching to be done when there's I/O...  */
void flush_queue()
{
  while (sizeof(sendqueue) && out_req<max_out_req)
  {
    mixed tmp=sendqueue[0];
    sendqueue=sendqueue[1..];
    tmp->write();
  }
}

#if !constant(thread_create) || LYSKOM_UNTHREADED
mixed send_sync(string request)
{
   return sync_do(++ref,request);
}

mixed sync_do(int ref,string|void request)
{
   mixed err;
   string s;
   array res;

   con->set_blocking();
   if (request)
   {
      if (!conwrite(ref+" "+request+"\r\n"))
      {
	 connection_lost();
	 return lyskom_error(CONNECTION_CLOSED);
      }
      out_req++;
   }
   else
   {
      array(int) o=(array(int))sendqueue;
      int j=search(o,ref);
      if (j==-1) 
      {
	 if (!async[ref])
	    error("request ref %d not in queue, but no callback\n",ref);
#ifdef LYSKOM_DEBUG
// 	 werror("LYSKOM removing callback %O for call %d (handling ref %d in sync mode)\n",
// 		async[ref],ref);
#endif
	 m_delete(async,ref);
      }
      else
      {
	 sendqueue[j]->write();
	 sendqueue=sendqueue[..j-1]+sendqueue[j+1..];
      }
   }

   for (;;)
   {
      s=con->read(0x7fffffff,1);
      if (s=="") 
      {
	 con->set_nonblocking(recv,0,0);
	 connection_lost();
	 return lyskom_error(CONNECTION_CLOSED);
      }
      err=catch { res=recv(0,s,ref); };
      if (err)
      {
	 con->set_nonblocking(recv,0,0);
	 throw(err);
      }
      if (res) break;
   }

   con->set_nonblocking(recv,0,0);

   return res;
}
#endif

void|array|object recv(mixed x,string what,void|int syn)
{
#ifdef LYSKOM_DEBUG
   werror("<- %O\n",what);
#endif
//    werror("<- (%d bytes) (%d asyncs left)\n",sizeof(what),
// 	  sizeof(indices(async)));

   array|object ires=0;
   buf+=what;
   // wait for newline before we do anything at all
   while (search(buf,"\n")!=-1)
   {
      mixed res;
      int len;
      // ok, try to figure out what we got
      switch (buf[0])
      {
	 case ':':
	    [res,len]=try_parse(buf[1..]);
	    if (res)
	    {
	       buf=buf[len+1..];
#if ! constant(thread_create) || LYSKOM_UNTHREADED
	       if (ref)
		  call_out(got_async_message,0,res);
	       else
#endif
		  got_async_message(res);
	       break;
	    }
	    return;

	 case '=':
	    [res,len]=try_parse(buf[1..]);
	    if (res)
	    {
	       buf=buf[len+1..];
	       if (syn && (int)res[0] == syn)
	       {
		  ires=res[1..]; // this is what we wait for
		  out_req--;
		  break;
	       }
#if ! constant(thread_create) || LYSKOM_UNTHREADED
	       if (ref)
		  call_out(got_reply,0,(int)res[0],res[1..]);
	       else
#endif
		  got_reply((int)res[0],res[1..]);
	       out_req--;
	       flush_queue();
	       break;
	    }
	    return;

	 case '%':
	    sscanf(buf,"%s\n%s",res,buf);
	    int ref,no,status;
	    if (sscanf(res,"%%%d %d %d",ref,no,status)==3)
	    {
	       if (ref==syn)
	       {
		  ires=lyskom_error(no,status);
		  out_req--;
		  break;
	       }
#if ! constant(thread_create) || LYSKOM_UNTHREADED
	       if (ref)
		  call_out(got_reply,0,ref,lyskom_error(no,status));
	       else
#endif
		  got_reply(ref,lyskom_error(no,status));
	       out_req--;
	       flush_queue();
	    }
	    else
	    {
	       werror("LysKOM.Raw: generic error recieved: %O\n",res);
	    }
	    break;

	 default:
	    werror("LysKOM.Raw: protocol error: %O\n",buf);
	    connection_lost();
	    return;
      }
   }
   return ires;
}

void read_thread()
{
   string s;
   while ((s=con->read(8192,1)))
   {
      if (s=="")
      {
	 connection_lost();
	 return;
      }
      recv(0,s);
   }
}

#if constant(thread_create) && !LYSKOM_UNTHREADED
Thread.Fifo call_fifo=Thread.Fifo();

void call_thread()
{
   array a;
   while ( (a=call_fifo->read()) )
      a[0](a[1]);
}
#endif

void connection_lost()
{
   werror("CONNECTION LOST\n");
   catch { con->close(); };
   con=0;
   // send error to all outstanding requests
   foreach (values(async),function f)
#if constant(thread_create) && !LYSKOM_UNTHREADED
      if (f) call_fifo->write( ({f,lyskom_error(CONNECTION_CLOSED)}) );
   call_fifo->write(0);
#else
      if (f) f(lyskom_error(CONNECTION_CLOSED));
#endif
}


void create(string server,void|int port,void|string whoami)
{
   if (!port) port=4894;
   con=Stdio.File();
   if (!con->connect(server,port))
   {
     object err=LysKOMError(-1,strerror(con->errno())-"\n",
			    "Failed to connect to server "
			    +server+".\n");
     throw(err);
     return;
   }

   conwrite("A"+H(whoami||
#if constant(getpwuid) && constant(getuid)
		  (getpwuid(getuid())||({"*unknown*"}))[0]+
#else
		  "*unknown*"
#endif
		  "%"
#if constant(uname)
		  +uname()->nodename
#else
		  "*unknown*"
#endif
		  ));
   string reply=con->read(7);
#ifdef LYSKOM_DEBUG
   werror("<- %O\n",reply);
#endif
   if (reply!="LysKOM\n")
   {
      con->close();
      con=0;
      return;
   }
#if constant(thread_create) && !LYSKOM_UNTHREADED
   thread_create(read_thread);
   thread_create(call_thread);
#ifdef LYSKOM_DEBUG
   werror("LysKOM running threaded\n");
#endif   
#else
   con->set_nonblocking(recv,0,0);
#ifdef LYSKOM_DEBUG
   werror("LysKOM running unthreaded\n");
#endif   
#endif
   return;
}

array(array(mixed)|int) try_parse(string what)
{
   array res=({});
   int len=0;
   
   array stack=({});

   while (sizeof(what)>1)
   {
      string a,b;

      switch(what[0])
      {
	 case '0'..'9':
	    // int, bitfield or hollerith

	    if (sscanf(what,"%[0-9]%s",a,b)<2 ||
		b=="") 
	       return ({0,0}); // incomplete

	    if (b[0]=='H') // hollerith
	    {
	       if (sizeof(b)<=(int)a)
		  return ({0,0}); // incomplete
	       res+=({b[1..(int)a]});
	       len+=sizeof(a)+sizeof(res[-1])+2;
	       b=b[(int)a+1..];
	    }
	    else // bitfield or int
	    {
	       res+=({a});
	       len+=sizeof(a)+1;
	    }

	    if (b=="") return ({0,0}); // incomplete

	    switch (b[0])
	    {
	       case ' ': // cont
		  break;
	       case '\n': // done
// 		  werror("ret: len=%d %O\n",len,res);
		  return ({res,len});
	       default:
		  werror("reached unknown: %O\n",b);
		  exit(-1);
	    }

	    what=b[1..];

	    break;

	 case '{': // array start;
	    if (what[..1]=="{ ")
	    {
	       stack=({res})+stack;
	       res=({});
	       len+=2;
	       what=what[2..];
	       break;
	    }
	    if (what[..1]!="{")
	    {
	       werror("reached unknown: %O\n",what);
	       exit(-1);
	    }
	    return ({0,0});

	 case '*': // empty array
	    res+=({({})});
	    len+=2;
	    switch (what[1])
	    {
	       case ' ': // cont
		  break;
	       case '\n': // done
		  return ({res,len});
	       default:
		  werror("reached unknown: %O\n",b);
		  exit(-1);
	    }
	    what=what[2..];
	    break;

	 case '}': // array end;

	    if (stack==({}))
	    {
	       werror("protocol error: stack underflow\n",what);
	       connection_lost();
	       return ({0,0});
	    }

	    res=stack[0]+({res});
	    stack=stack[1..];
	    
	    len+=2;

	    switch (what[1])
	    {
	       case ' ': // cont
		  break;
	       case '\n': // done
		  return ({res,len});
	       default:
		  werror("reached unknown: %O\n",b);
		  exit(-1);
	    }

	    what=what[2..];
	    break;

	 default:
	    werror("reached unknown: %O\n",what);
	    exit(-1);
      }
   }
   
   return ({0,0}); // incomplete
}


void got_reply(int ref,object|array what)
{
//   werror("got_reply(): async: %O\n",async);
   function call=async[ref];
#ifdef LYSKOM_DEBUG
//    werror("LYSKOM removing callback %O for call %d\n",call,ref);
#endif
   m_delete(async,ref);
   if (!call)
   {
      werror("LysKOM.Raw: lost callback for call %d %s\n",ref,
	     zero_type(call)?"(lost from mapping??!)":"(zero value in mapping)");
      //      werror("komihåg: %O\n",komihåg);
      werror(master()->describe_backtrace(backtrace()));
      return;
   }
#if constant(thread_create) && !LYSKOM_UNTHREADED
   call_fifo->write( ({call,what}) );
#else
   call_out(call,0,what);
#endif
}

void delete_async(int ref)
{
   m_delete(async,ref);
}

void add_async_callback(string which, function what, int dont_update)
{
   int no=.ASync.name2no[which];
   
   if (!no && zero_type(.ASync.name2no[which]))
      throw(LysKOMError( -1,"LysKOM: unsupported async",
			 sprintf("There is no supported async call named %O",
				 which) ));

   if (!async_callbacks[no]) async_callbacks[no]=({what});
   else async_callbacks[no]+=({what});
}

void remove_async_callback(string which, function what, void|int dont_update)
{
   int no=.ASync.name2no[which];
   
   if (!no && zero_type(.ASync.name2no[which]))
      throw(LysKOMError( -1,"LysKOM: unsupported async",
			 sprintf("There is no supported async call named %O",
				 which) ));

   async_callbacks[no]-=({what});
}

array active_asyncs()
{
   foreach (indices(async_callbacks),int z)
      if (async_callbacks[z]==({})) m_delete(async_callbacks,z);
   return indices(async_callbacks);
}

void got_async_message(array what)
{
#ifdef LYSKOM_DEBUG
  werror("got_async_message: :%s %s %O\n",
	 what[0], mkmapping( values(.ASync.name2no),
			    indices(.ASync.name2no))[(int)what[1]],
	 @what[2..]);
  //werror("got_async_message: %O\n", what);
#endif
  catch {
   if (async_callbacks[(int)what[1]])
      async_callbacks[(int)what[1]](@.ASync["decode_"+what[1]](what[2..]));
  };
}
