//  $Id: Connection.pike,v 1.4 1999/09/28 02:07:43 js Exp $
//! module Protocols
//! submodule LysKOM
//! class Session
//  ^^^ this is just to get the classes sorted right. Session first,
//      then Connection, then it doesn't matter
//! module Protocols
//! submodule LysKOM
//! class Connection
//!	This class contains nice abstraction for calls into the
//!	server. They are named "<i>call</i>", 
//!	"<tt>async_</tt><i>call</i>" or 
//!	"<tt>async_cb_</tt><i>call</i>", depending on 
//!	how you want the call to be done. 
//!
//! method mixed /call/(mixed ...args)
//! method object async_/call/(mixed ...args)
//! method object async_cb_/call/(function callback,mixed ...args)
//!	Do a call to the server. This really 
//!	clones a <link to=Protocols.LysKOM.Request>request</link> object,
//!	and initialises it. /call/ is to be read as 
//!	one of the calls in the lyskom protocol. ('-' is replaced
//!	with '_'.) (ie, logout, async_login or async_cb_get_conf_stat.)
//!	
//!	The first method is a synchronous call. This will
//!	send the command, wait for the server to execute it,
//!	and then return the result.
//!
//!	The last two is asynchronous calls, returning the
//!	initialised <link to=Protocols.LysKOM.Request>request</link> object.
//!

import ".";

object this=this_object();
object con; // LysKOM.Raw

//! variable int protocol_level
//! variable string session_software
//! variable string software_version
//!	Description of the connected server.

int protocol_level;
string session_software;
string software_version;

//!
//! method void create(string server)
//! method void create(string server,mapping options)
//!	<data_description type=mapping>
//!	<elem name=login type="int|string">login as this person number<br>(get number from name)</elem>
//!	<elem name=password type=string>send this login password</elem>
//!	<elem name=invisible type="int(0..1)">if set, login invisible</elem>
//!	<elem>advanced</elem>
//!	<elem name=port type=int(0..65535)>server port (default is 4894)</elem>
//!	<elem name=whoami type=string>present as this user<br>(default is from uid/getpwent and hostname)</elem>
//!	</data_description>

void create(string server,void|mapping options)
{
   if (!options) options=([]);
   con=Raw(server,options->port,options->whoami);
   if (!con->con)
   {
      con=0;
      return;
   }

   mixed err=catch {
      object vi=this->get_version_info();
      protocol_level=vi->protocol_version;
      session_software=vi->session_software;
      software_version=vi->software_version;
   };
   if (err)
      if (objectp(err) && err->no==2)
      {
	 protocol_level=0;
	 session_software=software_version="unknown";
      }
      else throw(err);

   if (options->login)
   {
      if (stringp(options->login))
      {
	 array a=this->lookup_z_name(options->login,1,0);
	 if (sizeof(a)!=1)
	    error("LysKOM: The login name %O does not match 1 name "
		  "exact (%d hits)\n",
		  options->login,sizeof(a));
	 options->login=a[0]->conf_no;
      }
      (protocol_level<3 ?
       this->login_old:
       this->login)
	 (options->login,
	  options->password||"",
	  options->invisible);
   }
}

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
      object req=prog(con);
      m=req->sync(@args);
      if (!req->ok)
      {
	 throw(req->error);
// 	 error(sprintf("lyskom-error %d,%d: %s\n%-=75s\n",
// 		       req->error->no,
// 		       req->error->status,
// 		       req->error->name,
// 		       req->error->desc));
      }
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
      object req=prog(con);
      req->async(@args);
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
      object req=prog(con);
      req->callback=callback;
      req->async(@args);
      return req;
   }
}

mixed `->(string request)
{
   program p;
   if ( (p=Request[String.capitalize(request)]) )
      return SyncRequest(p,this_object());
   else if ( request[..5]=="async_" &&
	     (p=Request[String.capitalize(request[6..])]) )
      return AsyncRequest(p);
   else if ( request[..8]=="async_cb_" &&
	     (p=Request[String.capitalize(request[9..])]) )
      return AsyncCBRequest(p);
   else
      return ::`[](request);
}

mixed `[](string request) { return `->(request); }
