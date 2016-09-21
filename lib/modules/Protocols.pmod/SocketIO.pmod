/*
 * Clean-room Socket.IO implementation
 */

#pike __REAL_VERSION__

//#define SIO_DEBUG		1
//#define SIO_DEBUGMORE		1

//#define SIO_STATS		1	// Collect extra usage statistics

#define DRIVERNAME		"Socket.IO"

#define DERROR(msg ...)		({sprintf(msg),backtrace()})
#define SERROR(msg ...)		(sprintf(msg))
#define USERERROR(msg)		throw(msg)
#define SUSERERROR(msg ...)	USERERROR(SERROR(msg))

#ifdef SIO_DEBUG
#define PD(X ...)		werror(X)
				// PT() puts this in the backtrace
#define PT(X ...)		(lambda(object _this){return (X);}(this))
#else
#undef SIO_DEBUGMORE
#define PD(X ...)		0
#define PT(X ...)		(X)
#endif

//! Global options for all SocketIO instances.
final mapping options = ([
]);

constant protocol = 4;		// SIO Protocol version

private enum {
  //    0      1           2      3    4      5             6
  CONNECT='0', DISCONNECT, EVENT, ACK, ERROR, BINARY_EVENT, BINARY_ACK
};

private array emptyarray = ({});

// All Socket.IO namespaces with connected clients
private mapping(string:array(multiset(.EngineIO.Server)|function))
 namespaces = ([]);

final void setoptions(mapping(string:mixed) _options) {
  options += _options;
}

//! @example
//! Sample minimal implementation of an SocketIO server farm:
//!
//!void echo(mixed id, function sendack, mixed ... data) {
//!  id->write(data);
//!  if (sendack)
//!    sendack("Ack","me","harder");
//!}
//!
//!void wsrequest(array(string) protocols, object req) {
//!  httprequest(req);
//!}
//!
//!void httprequest(object req)
//!{ switch (req.not_query)
//!  { case "/socket.io/":
//!      Protocols.SocketIO.Server server = Protocols.SocketIO.farm(req);
//!      if (server)
//!        server->write("Hello world!");
//!      break;
//!  }
//!}
//!
//!int main(int argc, array(string) argv)
//!{ Protocols.SocketIO.of("", echo); // Register root namespace
//!  Protocols.WebSocket.Port(httprequest, wsrequest, 80);
//!  return -1;
//!}
final Server farm(Protocols.WebSocket.Request req) {
  Protocols.EngineIO.Server con = Protocols.EngineIO.farm(req);
  return con && Server(con);
}

//! Register a new namespace
final void of(string namespace,
 void|function(mixed, function(mixed, mixed ...:void), mixed ...:void) read_cb,
 void|function(mixed:void) close_cb,
 void|function(mixed:void) open_cb) {
  array nsp = namespaces[namespace];
  if (!nsp)
    namespaces[namespace] = ({(<>), read_cb, close_cb, open_cb});
  else {
    nsp[1] = read_cb;
    nsp[2] = close_cb;
    nsp[3] = open_cb;
  }
}

//! Drop namespace
final void dropnamespace(string namespace) {
  array nsp = namespaces[namespace];
  if (!nsp)
    SUSERERROR("Unknown namespace %s", namespace);
  m_delete(namespaces, namespace);
  foreach (nsp[0];; Server client)
    if (client)
      client.close();
}

//! Runs a single Socket.IO session.
class Server {
  string namespace="";

  private function(mixed,
   function(mixed, mixed ...:void), mixed ...:void) read_cb;
  private function(mixed:void) open_cb, close_cb;
  private mixed id;
  private enum {RUNNING = 0, SDISCONNECT, RDISCONNECT};
  private int state = RUNNING;
  private array curevent;
  private array curbins;
  private int ackid = 0, bins = 0, curackid, curtype;
  private .EngineIO.Server con;
  private mapping(int:function(mixed, mixed ...:void)) ack_cbs = ([]);

  //! Set initial argument on callbacks.
  final void set_id(mixed _id) {
    id = _id;
  }

  //! Retrieve initial argument on callbacks.
  final mixed query_id() {
    return id || this;
  }

  private void fetchcallbacks() {
    array nsp = namespaces[namespace];
    if (nsp) {
      read_cb = nsp[1];
      close_cb = nsp[2];
      open_cb = nsp[3];
    } else {
      read_cb = 0;
      close_cb = 0;
      open_cb = 0;
    }
  }

  //! Send text or binary messages.
  final void write(mixed|function(mixed, mixed ...:void) ack_cb,
   mixed ... data) {
    if (state >= SDISCONNECT)
      USERERROR("Socket already shutting down");
    if (functionp(ack_cb))
      send(EVENT, data, ack_cb);
    else
      send(EVENT, ({ack_cb}) + data);
  }

  //! Alias for write().
  final void emit(mixed|function(mixed, mixed ...:void) ack_cb,
   mixed ... data) {
    write(ack_cb, @data);
  }

  private void send(int type, void|string|array data,
   void|int|function(mixed, mixed ...:void) ack_cb) {
    PD("Send %s %c:%O\n", con.sid, type, data);
    array sbins = emptyarray;
    int cackid;

    void treeexport(mixed level) {
      foreach(level; mixed i; mixed v)
        if (arrayp(v) || mappingp(v))
          treeexport(v);
        else if(objectp(v)) {
          level[i] = (["_placeholder":1, "num":sizeof(sbins)]);
          sbins += ({v});				// Save binary blobs
        }
    };

    if (arrayp(data)) {
      treeexport(data);
      data = Standards.JSON.encode(data);
    }
    if (!zero_type(ack_cb)) {
      if (type == ACK)
        cackid = ack_cb;
      else				// Save ackid, to make it threadsafe
        ack_cbs[cackid = ackid++] = ack_cb;
    }
    if (sizeof(sbins)) {
      switch(type) {
        case EVENT: type = BINARY_EVENT; break;
        case ACK: type = BINARY_ACK; break;
      }
      if (zero_type(ack_cb))
        data = sprintf("%c%d-%s", type, sizeof(sbins), data);
      else
        data = sprintf("%c%d-%d%s", type, sizeof(sbins), cackid, data);
      con.write(@(({data}) + sbins));
    } else {
      if (!data)
        data = "";
      if (zero_type(ack_cb))
        data = sprintf("%c%s", type, data);
      else
        data = sprintf("%c%d%s", type, cackid, data);
      con.write(data);
    }
  }

  //! Close the socket signalling the other side.
  final void close() {
    if (state < SDISCONNECT) {
      PT("Send disconnect, state %O\n", state);
      state = SDISCONNECT;
      send(DISCONNECT);
      con.close();
    }
  }

  private void treeimport(mixed level) {
    foreach(level; mixed i; mixed v) {
      if (arrayp(v))
        ;
      else if (mappingp(v)) {
        if (v->_placeholder) {
          level[i] = curbins[v->num];			// Fill in binary blobs
          continue;
        }
      } else
        continue;
      treeimport(v);
    }
  }

  private void recvcb() {
    if (read_cb)
      switch(curtype) {
        default: {
          int cackid = curackid;		// Instantiate ackid in
          void sendackcb(mixed ... data) {	// saved callback-stackframe
            send(ACK, data, cackid);
          };
          read_cb(query_id(), cackid < 0 && sendackcb, @curevent);
          break;
        }
        case ACK:
        case BINARY_ACK: {
          function(mixed, mixed ...:void) ackfn;
          if(ackfn = ack_cbs[curackid]) {
            m_delete(ack_cbs, curackid);	// ACK callbacks can only be
            ackfn(id, @curevent);		// executed once
          }
          break;
        }
      }
  }

  private void unregister() {
    array nsp = namespaces[namespace];
    if (nsp)
      nsp[0][this] = 0;
  }

  private void closedown() {
    if (state < RDISCONNECT) {
      close();
      state = RDISCONNECT;
      unregister();
      if (close_cb)
        close_cb(query_id());
    }
  }

  private void recv(mixed eid, string|Stdio.Buffer data) {
    PD("Recv %s %O\n", con.sid, (string)data);
    if (!stringp(data)) {
      curbins[-bins] = data;
      if (!--bins) {
        treeimport(curevent);
        recvcb();
      }
    } else {
      curtype = data[0];
      data = data[1..];
      switch (curtype) {
        default:	  // Protocol error
          send(ERROR, sprintf("%c,\"Invalid packet type\"", curtype));
          break;
        case CONNECT:
          if (namespaces[data]) {
            unregister();				// Old namespace
            namespaces[namespace = data][0][this] = 1;	// New namespace
            fetchcallbacks();
            state = RUNNING;
            send(CONNECT, namespace);			// Confirm namespace
            if (open_cb)
              open_cb(query_id());
          } else
            send(ERROR, data+",\"Invalid namespace\"");
          break;
        case DISCONNECT:
          closedown();
          close_cb = 0;
          open_cb = 0;
          read_cb = 0;
          id = 0;			// Delete all references to this Server
          break;
        case EVENT:
        case ACK:
        case BINARY_EVENT:
        case BINARY_ACK: {
          curevent = array_sscanf(data, "%[0-9]%[-]%[0-9]");
          string s;
          int i = 0;
          curackid = -1;
          if (sizeof(s = curevent[2]))
            curackid = (int)s;
          if (sizeof(s = curevent[0]))
            if (sizeof(curevent[1]))
              bins = (int)s;
            else
              curackid = (int)s;
          foreach(curevent;; string s)
            i += sizeof(s);
          curevent = Standards.JSON.decode(data[i..]);
          curbins = allocate(bins);
          switch (curtype) {
            case EVENT:
            case ACK:
              recvcb();
          }
        }
      }
    }
  }

  protected void destroy() {
    closedown();
  }

  protected void create(Protocols.EngineIO.Server _con) {
    con = _con;
    fetchcallbacks();
    con->set_callbacks(recv, closedown);
    send(CONNECT);			// Autconnect to root namespace
    PD("New SocketIO sid: %O\n", con.sid);
  }

  private string _sprintf(int type, void|mapping flags) {
    string res=UNDEFINED;
    switch (type) {
      case 'O':
        res = sprintf(DRIVERNAME"(%s.%d,%d,%d)",
         con.sid, protocol, state, sizeof(namespaces));
        break;
    }
    return res;
  }
}
