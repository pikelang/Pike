//  $Id$
//!	This class contains nice abstraction for calls into the
//!	server. They are named "@tt{@i{call@}@}",
//!	"@tt{async_@i{call@}@}" or
//!	"@tt{async_cb_@i{call@}@}", depending on
//!	how you want the call to be done.

//! @decl mixed XXX(mixed ...args)
//! @decl object async_XXX(mixed ...args)
//! @decl object async_cb_XXX(function callback,mixed ...args)
//!	Do a call to the server. This really
//!	clones a @[Protocols.LysKOM.Request] object,
//!	and initialises it. @tt{XXX@} is to be read as
//!	one of the calls in the lyskom protocol. ('-' is replaced
//!	with '_'.) (ie, logout, async_login or async_cb_get_conf_stat.)
//!
//!	The first method is a synchronous call. This will
//!	send the command, wait for the server to execute it,
//!	and then return the result.
//!
//!	The last two are asynchronous calls, returning an
//!	initialised @[Protocols.LysKOM.Request] object.
//!

#pike __REAL_VERSION__

import ".";

object con; // LysKOM.Raw

//!	Description of the connected server.
int protocol_level;
string session_software;
string software_version;

//! @decl void create(string server)
//! @decl void create(string server, mapping options)
//!
//! The @[options] argument is a mapping with the following members:
//! @mapping
//!   @member int|string "login"
//!     login as this person number (get number from name).
//!   @member string "password"
//!	send this login password.
//!   @member int(0..1) "invisible"
//!	if set, login invisible.
//!   @member int(0..65535) "port"
//!	server port (default is 4894).
//!   @member string "whoami"
//!	present as this user (default is from uid/getpwent and hostname).
//! @endmapping
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
      return SyncRequest(p,this);
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
