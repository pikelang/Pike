import ".";

object raw;
string pass=MIME.encode_base64(Crypto.randomness.reasonably_random()->read(6));

mapping options;

mapping channels=([]);

void create(string _server,void|mapping _options)
{
   options=
      ([
	 "port":6667,
#if constant(getpwuid) && constant(getuid)
	 "user":(getpwuid(getuid())||({"-"}))[0],
#else
	 "user":"unknown",
#endif
#if constant(getpwuid) && constant(getuid)
	 "nick":String.capitalize((getpwuid(getuid())||({"-"}))[0]),
#else
	 "nick":"Unknown"+random(10000),
#endif
#if constant(getpwuid) && constant(getuid)
	 "realname":(getpwuid(getuid())||({0,0,0,0,"Mr. Nobody"}))[4],
#else
	 "realname":"Mr. Anonymous",
#endif
#if constant(uname)
	 "host":uname()->nodename||"localhost",
#else
	 "host":"localhost",
#endif
      ])|(_options||([]));

   options->server=_server;

   me=person(options->nick,options->user+"@"+options->host);

   raw=Raw(options->server,options->port,got_command,got_notify,
	   connection_lost);

   cmd->create(raw);

   cmd->pass(pass); 
   cmd->nick(options->nick);
   cmd->user(options->user,options->host,options->server,options->realname);

   call_out(da_ping,30);
}

string expecting_pong;

void da_ping()
{
   call_out(da_ping,30);
   call_out(no_ping_reply,30); // timeout
   cmd->ping(expecting_pong=
	     options->host+" "+Array.shuffle("pike""IRC""client"/"")*"");
}

void no_ping_reply()
{
   remove_call_out(da_ping);
   werror("no ping reply\n");
   connection_lost();
}

void connection_lost()
{
   if (options->connection_lost)
      options->connection_lost();
   else
   {
      werror("destructing self\n");
      catch { destruct(raw->con); };
      destruct(raw);
      destruct(this_object());
   }
}

void got_command(string what,string ... args)
{
   // werror("got command: %O, %O\n",what,args);
   // most commands can be handled immediately
   switch (what)
   {
      case "PING":
	 cmd->pong(args[0]);
	 return;
      case "KILL":
      case "ERROR":
	 if (options->error_notify)
	    options->error_notify(@args);
	 return;
   }
   werror("got command: %O, %O\n",what,args);
}

void got_notify(string from,string type,
		void|string to,void|string message,
		void|string extra,void|string extra2)
{
   object c;
   if (options->system_notify && glob("2??",type))
   {
      options->system_notify(type,message);
      return;
   }
   if (options->motd_notify && (<"372","375","376">)[type])
   {
      options->motd_notify((["372":"cont",
			     "375":"start",
			     "376":"end"])[type],message);
      return;
   }

   object originator=person(@(from/"!"));

   switch (type)
   {
      case "433": // nick in use
	 if (options->error_nickinuse)
	    options->error_nickinuse(message);
	 else if (options->generic_notify) break;
	 cmd->nick("unknown"+random(10000));
	 return;

      case "473": // failed to join
	 if ((c=channels[lower_case(message||"")]))
	 {
	    c->not_failed_to_join();
	    return;
	 }
	 break;

      case "353": // names list
	 if (extra && (c=channels[lower_case(extra||"")]))
	 {
	    c->not_names(map(extra2/" "-({""}),
			     lambda(string name)
			     {
				string a,b,c;
				sscanf(name,"%[@]%s%[+]",
				       a,b,c);
				return ({person(b),a+c});
			     }));
	    return;
	 }
	 break;

	 /* --- */

      case "PONG":
	 if (message==expecting_pong)
	 {
	    remove_call_out(no_ping_reply);
	    return;
	 }
	 break;

      case "MODE":
	 if ((c=channels[lower_case(to||"")]))
	 {
	    // who, mode, by
	    c->not_mode(extra?person(extra):originator,message,originator);
	    return;
	 }
	 break;

      case "JOIN":
	 if ((c=channels[lower_case(to||"")]))
	 {
	    c->not_join(originator);
	    return;
	 }
	 break;

      case "PART":
	 if ((c=channels[lower_case(to||"")]))
	 {
	    // who, why, by
	    c->not_part(originator,message,originator);
	    return;
	 }
	 break;

      case "KICK":
	 if ((c=channels[lower_case(to||"")]))
	 {
	    // who, why, by
	    c->not_part(person(message),extra,originator);
	    return;
	 }
	 break;

      case "QUIT":
	 forget_person(originator);
	 foreach (values(channels),c)
	    c->not_part(originator,message,originator);
	 if (options->quit_notify)
	 {
	    // who, why
	    options->quit_notify(originator,to);
	    return;
	 }
	 break;

      case "PRIVMSG":
	 if ((c=channels[lower_case(to||"")]))
	 {
	    c->not_message(originator,message);
	    return;
	 }
	 if (!options->privmsg_notify) break;
	 options->privmsg_notify(originator,message,to);
	 return;

      case "NICK":
// 	 werror("%s is known as %s (aka %s)\n",
// 		originator->nick,
// 		to,
// 		((array)originator->aka)*",");
	 mixed err=0;
	 originator->aka[originator->nick]=1;
	 originator->aka[to]=1;
	 if (options->nick_notify)
	    err=catch { options->nick_notify(originator,to); };
	 originator->nick=to;
	 nick2person[to]=originator;
	 if (err) throw(err);
	 return;
   }
//    werror("got notify: %O, %O, %O, %O\n",from,type,to,message);
   if (options->generic_notify)
      options->generic_notify(from,type,to,message,extra);
}

object cmd=class
{
   object raw;
   public void create(object _raw) { raw=_raw; }

   class SyncRequest
   {
      program prog;
      object ret;
   	    
      void create(program p,object _ret)
      {
   	 prog=p;
   	 ret=_ret;
      }
   
      mixed `()(mixed ...args)
      {
   	 mixed m;
   	 object req=prog();
   	 m=req->sync(raw,@args);
   	 if (!m) return ret;
   	 else return m;
      }
   }
   	 
   class AsyncRequest
   {
      program prog;
   	    
      void create(program p)
      {
   	 prog=p;
      }
   
      mixed `()(mixed ...args)
      {
   	 object req=prog();
   	 req->async(this_object(),@args);
   	 return req;
      }
   }
   
   class AsyncCBRequest
   {
      program prog;
   
      void create(program p)
      {
   	 prog=p;
      }
   
      mixed `()(function callback,mixed ...args)
      {
   	 object req=prog();
   	 req->callback=callback;
   	 req->async(this_object(),@args);
   	 return req;
      }
   }
   
   mixed `->(string request)
   {
      mixed|program p;


      if ( (p=Requests[request]) )
   	 return SyncRequest(p,this_object());
      else if ( request[..5]=="async_" &&
   		(p=Requests[request[6..]]) )
   	 return AsyncRequest(p);
      else if ( request[..8]=="async_cb_" &&
   		(p=Requests[request[9..]]) )
   	 return AsyncCBRequest(p);
      else
      {
	 p=::`[](request);
	 if (!p)
	 {
	    werror("FOO! FOO! FOO! %O %O\n",request,p);
	    Error.internal("unknown command %O",request);
	    return 0;
	 }
	 return p;
      }
   }
} (raw);

// ----- commands

void send_message(string|array to,string msg)
{
   cmd->privmsg( (arrayp(to)?to*",":to), msg);
}

// ----- channel

class Channel
{
   string name;

   void	not_message(string who,string message);
   void	not_join(string who);
   void	not_part(string who,string message,string executor);
   void	not_mode(string who,string mode);
   void	not_failed_to_join();
}

// ----- persons

class Person
{
   string nick; // Mirar
   string user; // mirar
   string ip;   // mistel.idonex.se
   int last_action; // time_t
   multiset aka=(<>);

   void say(string message)
   {
      cmd->privmsg(nick,message);
   }

   void me(string what)
   {
      say("\1ACTION "+what+"\1");
   }
}

mapping nick2person=([]);
object me;

Person person(string who,void|string ip)
{
   Person p;

   if ( ! (p=nick2person[who]) )
   {
      p=Person();
      p->nick=who;
      nick2person[who]=p;
   }
   if (ip && !p->ip)
      sscanf(ip,"%*[~]%s@%s",p->user,p->ip);

//    werror("%O is %O %O %O\n",who,p->nick,p->user,p->ip);

   return p;
}

void forget_person(object p)
{
   m_delete(nick2person,p->nick);
}
