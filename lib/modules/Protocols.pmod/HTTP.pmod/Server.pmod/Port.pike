#pike __REAL_VERSION__

Stdio.Port port;
int portno;
string|int(0..0) interface;
function(.Request:void) callback;

//!
object|function|program request_program=.Request;

//! The simplest server possible. Binds a port and calls
//! a callback with @[request_program] objects.

//!
protected void create(function(.Request:void) callback,
		      void|int portno,
		      void|string interface,
		      void|int reuse_port)
{
  this::portno=portno || 80;

  this::callback=callback;
  this::interface=interface;
  port=Stdio.Port();
  if (!port->bind(portno,new_connection,interface,reuse_port))
    error("HTTP.Server.Port: failed to bind port %s%d: %s.\n",
          interface?interface+":":"",
          portno,strerror(port->errno()));
}

//! Closes the HTTP port.
void close()
{
   destruct(port);
   port=0;
}

protected void _destruct() { close(); }

// the port accept callback

protected void new_connection()
{
    while( Stdio.File fd=port->accept() )
      request_program()->attach_fd(fd,this,callback);
}
