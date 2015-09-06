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
void create(function(.Request:void) callback,
            void|int portno,
            void|string interface,
            void|int share)
{
  this::portno=portno || 80;

  this::callback=callback;
  this::interface=interface;
  port=Stdio.Port();
  if (!port->bind(portno,new_connection,interface,share))
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

void destroy() { close(); }

// the port accept callback

protected void new_connection()
{
    while( Stdio.File fd=port->accept() )
      request_program()->attach_fd(fd,this,callback);
}
