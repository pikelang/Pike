/*
 * Clean-room Socket.IO implementation for Pike.
 */

//! This is an implementation of the Socket.IO server-side communication's
//! driver.  It basically is a real-time bidirectional object-oriented
//! communication's protocol for communicating between a webbrowser
//! and a server.
//!
//! This module supports the following features:
//! @ul
//! @item
//!  Passing any arbitrarily deep nested data object
//!  which can be represented in basic JavaScript datatypes.
//! @item
//!  In addition to the normal JavaScript datatypes, it also
//!  supports passing (nested) binary blobs in an efficient manner.
//! @item
//!  Every message/event sent, allows for a callback acknowledge
//!  to be specified.
//! @item
//!  Acknowledge callbacks which will be called when the other side
//!  actively decides to acknowledge it (not automatically upon
//!  message reception).
//! @item
//!  Acknowledgement messages can carry an arbitrary amount of data.
//! @endul
//!
//! The driver uses @[Web.EngineIO] as the underlying protocol.
//!
//! @example
//! Sample minimal implementation of a SocketIO server farm:
//! @code
//! Web.SocketIO.Universe myuniverse;
//!
//! class myclient {
//!   inherit Web.SocketIO.Client;
//! }
//!
//! void echo(myclient id, function sendack, string namespace, string event,
//!  mixed ... data) {
//!   id->emit(event, @@data);
//!   if (sendack)
//!     sendack("Ack","me","harder");
//! }
//!
//! void wsrequest(array(string) protocols, object req) {
//!   httprequest(req);
//! }
//!
//! void httprequest(object req) {
//!   switch (req.not_query) {
//!     case "/socket.io/":
//!       myclient client = myuniverse.farm(myclient, req);
//!       if (client)
//!         client->emit("Hello world!");
//!       break;
//!   }
//! }
//!
//! int main(int argc, array(string) argv) {
//!   myuniverse = Web.SocketIO.Universe(); // Create universe
//!   myuniverse.register("", echo); // Register root namespace
//!   Protocols.WebSocket.Port(httprequest, wsrequest, 80);
//!   return -1;
//! }
//! @endcode
//!
//! @seealso
//!  @[farm], @[Web.EngineIO], @[Protocols.WebSocket],
//!  @url{http://github.com/socketio/socket.io-protocol@},
//!  @url{http://socket.io/@}

#pike __REAL_VERSION__
#pragma dynamic_dot

//#define SIO_DEBUG		1
//#define SIO_DEBUGMORE		1

//#define SIO_STATS		1	// Collect extra usage statistics

#define DRIVERNAME		"Socket.IO"

#define DERROR(msg ...)		({sprintf(msg),backtrace()})
#define SERROR(msg ...)		(sprintf(msg))
#define USERERROR(msg)		throw(msg)
#define SUSERERROR(msg ...)	USERERROR(SERROR(msg))
#define DUSERERROR(msg ...)	USERERROR(DERROR(msg))

#ifdef SIO_DEBUG
#define PD(X ...)		werror(X)
#define PDT(X ...)		(werror(X), \
				 werror(describe_backtrace(PT(backtrace()))))
				// PT() puts this in the backtrace
#define PT(X ...)		(lambda(object _this){return (X);}(this))
#else
#undef SIO_DEBUGMORE
#define PD(X ...)		0
#define PDT(X ...)		0
#define PT(X ...)		(X)
#endif

constant protocol = 5;		// SIO Protocol version

//! Global options for all SocketIO instances.
//!
//! @seealso
//!  @[SocketIO.Universe.farm()]
final mapping(string:mixed) options = ([
// Forcing the protocol SIO > 4 is a problem when a protocol <= 4 client
// client connects.  So avoid presetting SIO > 4 here.
  "sio_server_maxslots":	4,
  "maxdelay":			20*1000,  // = 20ms (unit in microseconds).
]);

private enum {
  //    0      1           2      3    4      5             6
  CONNECT='0', DISCONNECT, EVENT, ACK, ERROR, BINARY_EVENT, BINARY_ACK,
  //   7
  CONTAINER,
};

constant emptyarray = ({});

//! Convenience function to aid assist in sending the acknowledgment callback.
final void sendackcb(function(mixed ...:void) fn, mixed arg) {
  if (fn)
    if (arrayp(arg))
      fn(@arg);
    else
      fn(arg);
}

// Exchanges protocol support information with the client
private void exchangeoptions(Client client, function(mixed ...:void) ackcb,
 string namespace, string event, mixed coptions) {
  mapping(string:mixed) ret;
  if (mappingp(coptions)) {
    int i;
    ret = ([]);
    client.conn.options->SIO = ret->SIO
     = min(client.conn.options->SIO, coptions->SIO || 4);
    if (i = client.conn.options->sio_server_maxslots)
      ret->sio_server_maxslots = i;
  } else
    ret = ([
      "error": Protocols.HTTP.HTTP_BAD,
      "msg":   "Invalid options object"
    ]);
  if (ackcb)
    ackcb(ret);    // Inform the client about the lowest common denominator
}

class Universe {
  // All Socket.IO nsps with connected clients in this universe.
  private mapping(string:mapping(string|int: function(Client,
   void|function(Client, mixed ...:void),mixed ...:void))) nsps = ([]);

  //! All SocketIO sessions in this universe.
  final multiset(object) clients = (<>);

  //! @param options
  //!  Optional options to override the defaults.
  //!  This parameter is passed down as-is to the underlying @[EngineIO.Socket].
  //!
  //! @seealso
  //!  @[EngineIO.farm()]
  Client farm(object createtype, Protocols.WebSocket.Request req,
   void|mapping options) {
    .EngineIO.Socket con = seed(req, options);
    return con && createtype(this, con);
  }

  //! Seed farm with EngineIO connection.
  protected .EngineIO.Socket seed(Protocols.WebSocket.Request req,
   void|mapping options) {
    if (options)
      options = .SocketIO->options + options;
    else
      options = .SocketIO->options;
    return .EngineIO.farm(.EngineIO.Socket, req, options);
  }

  protected void low_register(string namespace, void|mixed event,
   void|function(Client, function(mixed ...:void), mixed ...:void) fn) {
    mapping nsp = nsps[namespace];
    if (!nsp)
      nsps[namespace] = nsp = ([]);
    if (fn)
      nsp[event] = fn;
    else {
      m_delete(nsp, event);
      if (!sizeof(nsp))
        m_delete(nsps, namespace);
    }
  }

  //! Register worker callback on namespace and event.
  //! There can be only one worker per tuple.
  //! Workers can be unregistered by omitting @expr{fn@}.
  //! Having a default worker per namespace in addition zero or more
  //! event specific ones, is supported.
  variant void register(string namespace, string event,
   void|function(Client, function(mixed ...:void),
     string, string, mixed ...:void) fn) {
    low_register(namespace, event, fn);
  }
  variant void register(string namespace,
   void|function(Client, function(mixed ...:void),
     string, mixed ...:void) fn) {
    low_register(namespace, 0, fn);
  }

  protected void create() {
    register("/", "sio_protocol", exchangeoptions);
  }

  private string invalidnamespace = "Invalid namespace";

  //! Connects and disconnects clients.
  //!
  //! @returns
  //!  0 upon success, the error itself otherwise.
  mixed
   connect(Client client, void|string newnamespace) {
    clients[client] = !!newnamespace;
    return nsps[newnamespace] ? 0 : invalidnamespace;
  }

  //!
  protected string|function lookupcb(string namespace, array data) {
    function fn;
    mapping nsp = nsps[namespace];
    if (!nsp)
      return "\"" + invalidnamespace + "\"";
    if (!(fn = nsp[sizeof(data) && data[0]] || nsp[0]))
      return (sizeof(data) ? Standards.JSON.encode(data[0]) : "")
       + ",\"Unregistered event\"";
    return fn;
  }

  //!
  mixed read(string namespace, mixed id,
   function(mixed ...:void) sendackcb, mixed ... data) {
    string|function(Client, function(mixed ...:void),
     string, mixed ...:void) fn;
    if (!functionp(fn = lookupcb(namespace, data)))
      return fn;
    fn(id, sendackcb, namespace, @data);
    return 0;
  }
}

//! Runs a single Socket.IO session.
class Client {
  private Universe universe;

  //!
  function(void:void) onclose;

  private enum {RUNNING = 0, SDISCONNECT, RDISCONNECT};
  private int state = RUNNING;
  private array curevent;
  private array curbins;
  private string curnamespace;

  private int ackid = 0, bins = 0, curackid, curtype;
  .EngineIO.Socket conn;
  private mapping(int:function(Client, mixed ...:void)) ack_cbs = ([]);

  //! This session's unique session id.
  final string `sid() {
    return conn.sid;
  }

  //! Contains the last request seen on this connection.
  //! Can be used to obtain cookies etc.
  final Protocols.WebSocket.Request `request() {
    return conn.request;
  }

  //! Send text or binary messages to the client.  When in doubt
  //! use the emit() interface instead.  write() allows messages
  //! to be sent which do not have a string event-name up front.
  final void write(array data,
   void|function(Client, mixed ...:void) ack_cb,
   void|mapping(string:mixed) options) {
    if (state >= SDISCONNECT)
      DUSERERROR("Socket already shutting down");
    send(EVENT, data, ack_cb, options);
  }

  //! Send text or binary events to the client.
  final variant void emit(function(Client, mixed ...:void) ack_cb,
   string event, mixed ... data) {
    write(({event}) + data, ack_cb);
  }
  final variant void emit(string event, mixed ... data) {
    write(({event}) + data);
  }
  final variant void emit(function(Client, mixed ...:void) ack_cb,
   mapping(string:mixed) options, string event, mixed ... data) {
    write(({event}) + data, ack_cb, options);
  }
  final variant void emit(mapping(string:mixed) options,
   string event, mixed ... data) {
    write(({event}) + data, 0, options);
  }

  private void send(int type, void|string|array data,
   void|int|function(Client, mixed ...:void) ack_cb,
   void|mapping(string:mixed) options) {
    PD("Send %s %d %c:%O %O\n", sid, intp(ack_cb)?ack_cb:-1, type, data,
     options);
    array sbins = emptyarray;
    int cackid;

    if (options)
      options = conn.options + options;
    else
      options = conn.options;

    string exportbinary(mixed value) {
      if (objectp(value)) {
        sbins += ({value});				// Save binary blobs
        value = sprintf("{\"_placeholder\":1,\"num\":%d}", sizeof(sbins) - 1);
      }
      return value;
    };

    if (arrayp(data))
      data = Standards.JSON.encode(data, 0, exportbinary);
    if (!zero_type(ack_cb)) {
      if (type == ACK)
        cackid = ack_cb;
      else				// Save ackid, to make it threadsafe
        ack_cbs[cackid = ackid++] = ack_cb;
    }
    if (sizeof(sbins)) {
      switch (type) {
        case EVENT: type = BINARY_EVENT; break;
        case ACK: type = BINARY_ACK; break;
      }
      if (zero_type(ack_cb))
        data = sprintf("%c%d-%s", type, sizeof(sbins), data);
      else
        data = sprintf("%c%d-%d%s", type, sizeof(sbins), cackid, data);
      aggregate(options, data, sbins);
    } else {
      if (!data)
        data = "";
      if (zero_type(ack_cb))
        data = sprintf("%c%s", type, data);
      else
        data = sprintf("%c%d%s", type, cackid, data);
      aggregate(options, data);
    }
  }

  private array(String.Buffer)
   trslots;				  // Text receive slots
  private array(array(int|string|Stdio.Buffer))
   brslots;				  // Binary receive slots
  private array(partqueue)
   qsslots;				  // Send queue slots
  private int maxslots;
  private String.Buffer aggtext;
  private Stdio.Buffer aggbin;
  private int stamptext, stampbin;
  private Thread.Mutex flushone;

  private void flushaggtext() {
    conn.write(conn.options, aggtext.get());
  };

  private void flushaggbin() {
    conn.write(conn.options, aggbin);	  // Avoid copying the content
    aggbin = Stdio.Buffer();		  // Create a new buffer instead
  };

  /*
   * CONTAINER:
   *
   * Field specs:
   * ss = stream (0-3)
   * f = finish packet
   * b = with more binaries
   * f = 0 and b = 1 is reserved and should not be used
   * lllllllll = length in bytes of fragment (1-512) - 1
   * llllllllllllllll = length in bytes of fragment (1-65536) - 1
   *
   * Fragment header:
   * 76543210 76543210 76543210
   * 0ll0bfss 0lllllll
   * 0ll1bfss 0lllllll 0lllllll
   *
   *
   * BINARY_CONTAINER:
   *
   * Field specs:
   * llllllllllll = length in bytes of fragment (0-4095)
   * llllllllllllllllllll = length in bytes of fragment (0-1048575)
   *
   * Fragment header:
   * 76543210 76543210 76543210
   * lll0bfss llllllll
   * lll1bfss llllllll llllllll
   *
   * Both CONTAINER and BINARY_CONTAINER are then followed by a content
   * fragment.
   */

  class partqueue {
    inherit Thread.Queue;
    private int slot;

    protected void create(int slot) {
      this::slot = slot;
    }

    private int timestamp;
    private mapping(string:mixed) opts = conn.options;
    private array(int|mapping(string:mixed)|string|Stdio.Buffer) cv;
    private int curoffset;

    final int getchunk() {
      int sentany = 0;
      if (!cv && (cv = try_read())) {
        timestamp = cv[0];
        opts = cv[1];
        cv = cv[2..];
        curoffset = 0;
      }
      if (cv) {
        int finf = 0, mtu = opts->server_textmtu;
        string|Stdio.Buffer ret = cv[0];
        if (stringp(ret)) {
          int room = min(mtu - sizeof(aggtext), 0x10000);
          if (room <= 0) {
            flushaggtext();
            room = mtu;
            sentany = 1;
          }
          if (!sizeof(aggtext)) {
            stamptext = timestamp;
            aggtext.putchar(CONTAINER);
          }
          int tocopy = sizeof(ret) - curoffset;
          if (tocopy > room)
            tocopy = room;
          else
            finf = 1;
          int acc, len = --tocopy;
          acc = (len&3)<<5 | (finf && sizeof(cv)>1 && 8)
           | (finf && 4) | slot;
          if ((len >>= 2) > 0x7f)
            acc |= 0x10;
          aggtext.putchar(acc);
          aggtext.putchar(len & 0x7f);
          if (len >>= 7)
            aggtext.putchar(len);
          aggtext.add(ret[curoffset .. curoffset + tocopy]);
          if (!finf)
            curoffset += tocopy;
          if (sizeof(aggtext) >= mtu)
            flushaggtext(), sentany = 1;
        } else {
          int room = min(mtu - sizeof(aggbin), 0xfffff);
          if (room <= 0) {
            flushaggbin();
            room = mtu;
          }
          if (!sizeof(aggbin))
            stampbin = timestamp;
          int tocopy = sizeof(ret) - curoffset;
          if (tocopy > room)
            tocopy = room;
          else
            finf = 1;
          int acc, len = tocopy;
          acc = ((len&7)<<5) | (finf && sizeof(cv)>1 && 8)
           | (finf && 4) | slot;
          if ((len >>= 3) > 0xff)
            acc |= 0x10;
          aggbin->add(acc, len);
          if (len >>= 8)
            aggbin->add_int8(len);
          aggbin->add(ret->read_buffer(tocopy));
          if (sizeof(aggbin) >= mtu)
            flushaggbin(), sentany = 1;
        }
        cv = finf && sizeof(cv) > 1 ? cv[1..] : 0;
      }
      return sentany;
    }
  }

  private void aggregate(mapping(string:mixed) options,
   string text, void|array(Stdio.Buffer) bins) {
    if (!(maxslots = options->sio_server_maxslots)
     ||  options->SIO < 5
     || !options->server_mtu)
      conn.write(options, text, @(bins || emptyarray));
    else {
      int slot;
      if (!qsslots[slot = options->prio])
        qsslots[slot] = partqueue(slot);
      qsslots[slot].write(({gethrtime(), options, text, @bins}));
      flushsendq();
    }
  }

  private void flushsendq() {
    Thread.MutexKey lock = flushone.trylock(2);
    if (lock) {		      // There can be only one queue-flusher
      remove_call_out(flushsendq);
      {
        int inflight = conn.options->server_inflight;
        if (conn.sendqbytesize() < inflight) {
          int slot;
outer:    foreach (qsslots; slot; partqueue sendq)  // Lower slots first
            if (sendq)
              while (sendq.getchunk())
                if (conn.sendqbytesize() >= inflight)
                  break outer;
        }
      }
      {
        int t, target, maxdelay = conn.options->maxdelay;
        if (sizeof(aggtext)) {
          t = gethrtime();
          if (maxdelay > (t = gethrtime()) - stamptext)
            flushaggtext();
          else
            target = stamptext;
        }
        if (sizeof(aggbin)) {
          if (!t)
            t = gethrtime();
          if (maxdelay > t - stampbin)
            flushaggbin();
          else
            target = min(target || stampbin, stampbin);
        }
        if (target)
          call_out(flushsendq, (maxdelay - target)/1000000.0);
      }
      lock = 0;
    }
  }

  //! Close the socket signalling the other side.
  void close() {
    if (state < SDISCONNECT) {
      PD("Send disconnect, %s state %O\n", sid, state);
      state = SDISCONNECT;
      send(DISCONNECT);
      conn.close();
      if (onclose)
        onclose();
    }
  }

  private void treeimport(mixed level) {
    foreach (level; mixed i; mixed v) {
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
    void readthread(string namespace, int cackid, mixed ... data) {
      void sendackcb(mixed ... data) {		// saved callback-stackframe
        PD("Ack %d %O\n", cackid, data);
        send(ACK, data, cackid);
      };
      string err = universe ? universe.read(curnamespace, this,
       cackid >= 0 && sendackcb, @data) : "\"Unknown universe\"";
      if (err)
        send(ERROR, curnamespace + "," + err);
    };
    switch (curtype) {
      default: {
#if constant(Thread.Thread)
        // Spawn one thread per message/callback.
        // FIXME Too much overhead?
        Thread.Thread(readthread, curnamespace, curackid, @curevent);
#else
        readthread(curnamespace, curackid, @curevent);
#endif
        break;
      }
      case ACK:
      case BINARY_ACK: {
        function(Client, mixed ...:void) ackfn;
        if (ackfn = ack_cbs[curackid]) {
          m_delete(ack_cbs, curackid);		// ACK callbacks can only be
          ackfn(this, @curevent);			// executed once
        }
        break;
      }
    }
  }

  private void closedown() {
    if (state < RDISCONNECT) {
      close();
      state = RDISCONNECT;
      universe.connect(this);
    }
  }

  private void recv(string|Stdio.Buffer data) {
    void flushslot(int slot, int|string ready) {
      if (ready) {
        foreach (brslots[slot];; string|Stdio.Buffer v)
          recv(v);
        brslots[slot] = ({0, Stdio.Buffer()});
      }
    };
    PD("Recv %s %O\n", sid, (string)data);
    if (!stringp(data)) {
      if (bins) {
        curbins[-bins] = data->read_buffer(sizeof(data),1);
        if (!--bins) {
          treeimport(curevent);
          recvcb();
          curbins = emptyarray;
        }
      } else {
        do {
          int acc, slot, len;
          acc = data->read_int8();
          slot = acc & 3;
          Stdio.Buffer buf = brslots[slot][-1];
          len = slot >> 5 | data->read_int8() << 3;
          if (acc & 0x10)
            len |= data->read_int8() << 11;
          buf->add(data->read_buffer(len));
          switch (acc & 0xc) {
            case 4:				      // finf
              flushslot(slot, brslots[slot][0]);
              break;
            case 0xc:				      // bin + finf
              brslots[slot] += ({Stdio.Buffer()});
              break;
          }
        } while (sizeof(data));
      }
    } else {
      curtype = data[0];
      data = data[1..];
      switch (curtype) {
        default:	  // Protocol error
          send(ERROR, sprintf("%c,\"Invalid packet type\"", curtype));
          break;
        case ERROR:
          SUSERERROR(data);				// Pass error up
        case CONNECT:
          if (universe) {
            mixed e;
            if (e = universe.connect(this, data)) {
              send(ERROR, data + "," + Standards.JSON.encode(e));
              break;
            }
            state = RUNNING;
            send(CONNECT, data);			// Confirm namespace
          } else
            send(ERROR, data + ",\"No universe to register to\"");
          break;
        case DISCONNECT:
          closedown();
          universe = 0;
          break;
        case EVENT:
        case ACK:
        case BINARY_EVENT:
        case BINARY_ACK: {
          curevent = array_sscanf(data, "%[0-9]%[-]%[0-9]%[^[,]%*[,]%[0-9]");
          string s;
          int i = 0;
          curackid = -1;
          if (sizeof(s = curevent[2]) || sizeof(s = curevent[4]))
            curackid = (int)s;
          curnamespace = curevent[3];
          if (sizeof(s = curevent[0]))
            if (sizeof(curevent[1]))
              bins = (int)s;
            else
              curackid = (int)s;
          foreach (curevent;; string s)
            i += sizeof(s);
          curevent = Standards.JSON.decode(data[i..]);
          curbins = allocate(bins);
          PD("Incoming %c %d %d %O\n", curtype, bins, curackid, curevent);
          switch (curtype) {
            case EVENT:
            case ACK:
              recvcb();
          }
          break;
        }
        case CONTAINER: {
          int offset = 0;
          do {
            int acc, slot, len;
            String.Buffer buf;
            acc = data[offset++];
            slot = acc & 3;
            if (!(buf = trslots[slot]))
              trslots[slot] = buf = String.Buffer();
            len = slot >> 5 | data[offset++] << 2;
            if (acc & 0x10)
              len |= data[offset++] << 9;
            buf->add(data[offset..offset += len]);
            switch (acc & 0xc) {
              case 4:				      // finf
                recv(buf->get());
                break;
              case 0xc:				      // bin + finf
                len = brslots[slot][0];
                brslots[slot][0] = buf->get();
                flushslot(slot, len);
                break;
            }
          } while (++offset < sizeof(data));
        }
      }
    }
  }

  protected void _destruct() {
    closedown();
  }

  protected void create(Universe _universe, .EngineIO.Socket _con) {
    universe = _universe;
    (conn = _con).open(recv, closedown, flushsendq);
    int slots;
    if (slots = conn.options->sio_server_maxslots) {
      aggtext = String.Buffer();
      aggbin = Stdio.Buffer();
      flushone = Thread.Mutex();
      qsslots = allocate(slots);
      trslots = allocate(slots);
      brslots = allocate(slots, ({0, 0}));
      foreach (brslots;; array v)
        v[1] = Stdio.Buffer();
    }
    send(CONNECT);			// Autoconnect to root namespace
    PD("New SocketIO sid: %O\n", sid);
  }

  protected string _sprintf(int type, void|mapping flags) {
    string res=UNDEFINED;
    switch (type) {
      case 'O':
        res = sprintf(DRIVERNAME"(%s.%d,%d)",
         sid, protocol, state);
        break;
    }
    return res;
  }
}
