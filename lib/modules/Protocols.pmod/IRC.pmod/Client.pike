#pike __REAL_VERSION__

import ".";

Raw raw;
string pass=MIME.encode_base64(Crypto.randomness.reasonably_random()->read(6));

mapping options;

mapping channels=([]);

//! @decl void create(string server, void|mapping(string:mixed) options)
//! @param server
//!   The IRC server to connect to.
//! @param options
//!   An optional mapping with additional IRC client options.
//!   @mapping
//!     @member int "port"
//!       Defaults to 6667.
//!     @member string "user"
//!       Defaults to @expr{"unknown"@} on systems without @[getpwuid] and
//!       @[getuid] and to @expr{getpwuid(getuid())[0]@} on systems with.
//!     @member string "nick"
//!       Defaults to @expr{"Unknown"@} on systems without @[getpwuid] and
//!       @[getuid] and to @expr{String.capitalize(getpwuid(getuid())[0])@}
//!       on systems with.
//!     @member string "realname"
//!       Defaults to @expr{"Mr. Anonymous"@} on systems without @[getpwuid]
//!       and @[getuid] and to @expr{getpwuid(getuid())[4]@} on systems with.
//!     @member string "host"
//!       Defaults to @expr{"localhost"@} on systems without @[uname] and
//!       to @expr{uname()->nodename@} on systems with.
//!     @member int "ping_interval"
//!       Defaults to 120.
//!     @member int "ping_timeout"
//!       Defaults to 120.
//!     @member function(void:void) "connection_lost"
//!       This function is called when the connection to the IRC server is
//!       lost or when a ping isn't answered with a pong within the time
//!       set by the @tt{ping_timeout@} option. The default behaviour is
//!       to complain on stderr and self destruct.
//!     @member function(mixed ...:void) "error_notify"
//!       This function is called when a KILL or ERROR command is recieved
//!       from the IRC server.
//!     @member function(string,void|string:void) "system_notify"
//!
//!     @member function(string,void|string:void) "motd_notify"
//!
//!     @member function(string) "error_nickinuse"
//!
//!     @member function(string,string,string,string,string) "generic_notify"
//!       The arguments are from, type, to, message and extra.
//!     @member function(string,string) "quit_notify"
//!       The arguments are who and why.
//!     @member function(Person,string,string) "privmsg_notify"
//!       The arguments are originator, message and to.
//!     @member function(Person,string,string) "notice_notify"
//!       The arguments are originator, message and to.
//!     @member function(Person,string) "nick_notify"
//!       The arguments are originator and to.
//!   @endmapping
void create(string _server,void|mapping(string:mixed) _options)
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

   call_out(da_ping,options->ping_interval || 60);
}

//! Closes the connection to the server.
void close()
{
   if (raw->con) raw->con->close();
   destruct(raw);
   raw=0;
}

string expecting_pong;

void da_ping()
{
   call_out(da_ping,options->ping_interval || 120);
   call_out(no_ping_reply,options->ping_timeout || 120); // timeout
   cmd->ping(expecting_pong=
	     options->host+" "+Array.shuffle("pikeIRCclient"/1)*"");
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
      destruct(this);
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
//     werror("got command: %O, %O\n",what,args);
}

void got_notify(string from,string type,
		void|string to,void|string message,
		string ...extra)
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

   Person originator=person(@(from/"!"));

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
	    c->not_failed_to_join(@extra);
	    return;
	 }
	 break;

      case "353": // names list
	 if (sizeof(extra) && (c=channels[lower_case(extra[0]||"")]) &&
	     c->not_names)
	 {
	    c->not_names(map(extra[1]/" "-({""}),
			     lambda(string name)
			     {
				string a,b,c;
				sscanf(name,"%[+@%]%s%[+@%]",
				       a,b,c);
				Person p=person(b);
				p->channels[lower_case(extra[0])]=1;
				return ({p,a+c});
			     }));
	    return;
	 }
	 break;
      case "366": // "end of names list"
	 break;

      case "352": // who list
	 if (sizeof(extra)>2 && 
	     message && (c=channels[lower_case(message||"")]))
	 {
	    Person p=person(extra[3],extra[0]+"@"+extra[1]);
	    p->server=extra[2];
	    p->realname=extra[6..]*" ";
	    p->channels[lower_case(message)]=1;
	    if (c->not_who) c->not_who(p,extra[4]);
	    return;
	 }
	 break;
      case "315": // "end of who list"
	 break;

      case "482": // "you're not channel operator"
	 if ((c=channels[lower_case(message||"")]))
	 {
	    if (c->not_not_oper) c->not_not_oper();
	    return;
	 }
	 break;

      case "474": // "cannot join channel"
	werror("%O\n",extra);
	 if ((c=channels[lower_case(message||"")]))
	 {
	    if (c->not_join_fail) c->not_join_fail(extra*" ");
	    return;
	 }
	 break;

      case "401": // no such nick
// 	 werror("%O\n",({from,type,to,message,extra}));
// 	 werror("(got 401 %O %O)\n",message,extra*" ");
	 break;

      case "367": // mode b line
	 if ((c=channels[lower_case(message||"")]))
	 {
	    if (c->not_mode_b) c->not_mode_b(extra*" ");
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
	    c->not_mode(extra[0]?person(extra[0]):originator,
			message+( ({""})+extra)*" ",originator);
	    return;
	 }
	 break;

      case "JOIN":
//  	 werror("me=%O\n",me);
	 if ((c=channels[lower_case(to||"")]))
	 {
	    c->not_join(originator);
	    return;
	 }
	 break;

      case "PART":
	 originator->channels[lower_case(to)]=0;
	 if ((c=channels[lower_case(to||"")]))
	 {
	    // who, why, by
	    c->not_part(originator,message,originator);
	    return;
	 }
	 break;

      case "KICK":
	 person(message)->channels[lower_case(extra[0])]=0;
	 if ((c=channels[lower_case(to||"")]))
	 {
	    // who, why, by
	    c->not_part(person(message),extra[0],originator);
	    return;
	 }
	 break;

      case "QUIT":
	 forget_person(originator);

	 if (options->quit_notify)
	 {
	    // who, why
	    options->quit_notify(originator,to);
	    return;
	 }

	 foreach (values(channels),c)
	    if (c)
	       if (c->not_quit || c->not_part) 
		  (c->not_quit||c->not_part)(originator,message,originator);

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

      case "NOTICE":
	 if ((c=channels[lower_case(to||"")]))
	 {
	    c->not_message(originator,message);
	    return;
	 }
	 if (!options->notice_notify) break;
	 options->notice_notify(originator,message,to);
	 return;

      case "NICK":
	 werror("%s is known as %s (aka %s)\n",
		originator->nick,
		to,
		((array)originator->aka)*",");
	 mixed err=0;
	 originator->aka[originator->nick]=1;
	 originator->aka[to]=1;
	 if (options->nick_notify)
	    err=catch { options->nick_notify(originator,to); };
	 m_delete(nick2person,originator->nick);
	 originator->nick=to;
	 nick2person[to]=originator;
	 if (err) throw(err);
	 return;

      case "INVITE":
	 if ((c=channels[lower_case(message||"")]))
	 {
	    c->not_invite(originator,@extra);
	    return;
	 }
	 if (!options->notice_invite) break;
	 options->notice_invite(originator,message,@extra);
	 return;

      default:
	 werror("got unknown message: %O, %O, %O, %O\n",from,type,to,message);
   }
//    werror("got notify: %O, %O, %O, %O\n",from,type,to,message);
   if (options->generic_notify)
      options->generic_notify(from,type,to,message,extra[0]);
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
   	 req->async(this,@args);
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
   	 req->async(this,@args);
   	 return req;
      }
   }
   
   mixed `->(string request)
   {
      mixed|program p;


      if ( (p=Requests[request]) )
   	 return SyncRequest(p,this);
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

// ----- persons

class Person
{
   inherit .Person;

   void say(string message)
   {
      cmd->privmsg(nick,message);
   }

   void notice(string message)
   {
      cmd->notice(nick,message);
   }

   void action(string what)
   {
      say("\1ACTION "+what+"\1");
   }

   string _sprintf(int t)
   {
     return t=='O' && sprintf("%O(%s!%s@%s%s)", this_program,
			      nick,user||"?",ip||"?",
			      (realname!="?")?"("+realname+")":"");
   }
}

mapping nick2person=([]);
Person me;

Person person(string who,void|string ip)
{
   Person p;

   if ( ! (p=nick2person[lower_case(who)]) )
   {
      p=Person();
      p->nick=who;
      nick2person[lower_case(who)]=p;
//        werror("new person: %O\n",p);
   }
   else if (lower_case(p->nick)!=lower_case(who))
   {
      werror("nick mismatch: %O was %O\n",who,p->nick);
      p->nick=who;
   }
   if (ip && !p->ip)
      sscanf(ip,"%*[~]%s@%s",p->user,p->ip);

   return p;
}

void forget_person(object p)
{
   m_delete(nick2person,p->nick);
}
