#pike __REAL_VERSION__

import ".";

Stdio.Port port;
int portno;
string|int(0..0) interface;
function(Request:void) callback;

program request_program=Request;

//! module Protocols
//! submodule HTTP
//! submodule Server
//! class Port
//!	The simplest server possible. Binds a port and calls
//!	a callback with <ref to=Request>Server.Request</ref> objects.

//! method void create(function(Request:void) callback)
//! method void create(function(Request:void) callback,int portno,void|string interface)

void create(function(Request:void) _callback,
	    void|int _portno,
	    void|string _interface)
{
   portno=_portno;
   if (!portno) portno=80; // default HTTP port

   callback=_callback;
   interface=_interface;

   port=Stdio.Port();
   if (!port->bind(portno,new_connection,interface))
      error("HTTP.Server.Port: failed to bind port %s%d: %s\n",
	    interface?interface+":":"",
	    portno,strerror(port->errno()));
}

//! method void close()
//!	Closes the HTTP port. 

void close()
{
   destruct(port);
   port=0;
}

void destroy() { close(); }

// the port accept callback

static void new_connection()
{
   Stdio.File fd=port->accept();
   Request r=request_program();
   r->attach_fd(fd,this_object(),callback);
}
