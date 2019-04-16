/*
 * Clean-room Engine.IO implementation for Pike.
 */

//! This is an implementation of the Engine.IO server-side communication's
//! driver.  It basically is a real-time bidirectional packet-oriented
//! communication's protocol for communicating between a webbrowser
//! and a server.
//!
//! The driver mainly is a wrapper around @[Protocols.WebSocket] with
//! the addition of two fallback mechanisms that work around limitations
//! imposed by firewalls and/or older browsers that prevent native
//! @[Protocols.WebSocket] connections from functioning.
//!
//! This module supports the following features:
//! @ul
//! @item
//!  It supports both UTF-8 and binary packets.
//! @item
//!  If both sides support @[Protocols.WebSocket], then
//!  packet sizes are essentially unlimited.
//!  The fallback methods have a limit on packet sizes from browser
//!  to server, determined by the maximum POST request size the involved
//!  network components allow.
//! @endul
//!
//! In most cases, Engine.IO is not used directly in applications.  Instead
//! one uses Socket.IO instead.
//!
//! @example
//! Sample small implementation of an EngineIO server farm:
//! @code
//! class mysocket {
//!   inherit Web.EngineIO.Socket;
//!
//!   void echo(string|Stdio.Buffer msg) {
//!     write(0, msg);
//!   }
//! }
//!
//! void wsrequest(array(string) protocols, object req) {
//!   httprequest(req);
//! }
//!
//! void httprequest(object req) {
//!   switch (req.not_query) {
//!     case "/engine.io/":
//!       mysocket client = Web.EngineIO.farm(mysocket, req);
//!       if (client) {
//!         client.open(client.echo);
//!         client.write(0, "Hello world!");
//!       }
//!       break;
//!   }
//! }
//!
//! int main(int argc, array(string) argv) {
//!   Protocols.WebSocket.Port(httprequest, wsrequest, 80);
//!   return -1;
//! }
//! @endcode
//!
//! @seealso
//!  @[farm], @[Web.SocketIO], @[Protocols.WebSocket],
//!  @url{http://github.com/socketio/engine.io-protocol@},
//!  @url{http://socket.io/@}

#pike __REAL_VERSION__
#pragma dynamic_dot

//#define EIO_DEBUG		1
//#define EIO_DEBUGMORE		1

//#define EIO_STATS		1	// Collect extra usage statistics

#define DRIVERNAME		"Engine.IO"

#define DERROR(msg ...)		({sprintf(msg),backtrace()})
#define SERROR(msg ...)		(sprintf(msg))
#define USERERROR(msg)		throw(msg)
#define SUSERERROR(msg ...)	USERERROR(SERROR(msg))
#define DUSERERROR(msg ...)	USERERROR(DERROR(msg))

#ifdef EIO_DEBUG
#define PD(X ...)		werror(X)
#define PDT(X ...)		(werror(X), \
				 werror(describe_backtrace(PT(backtrace()))))
				// PT() puts this in the backtrace
#define PT(X ...)		(lambda(object _this){return (X);}(this))
#else
#undef EIO_DEBUGMORE
#define PD(X ...)		0
#define PDT(X ...)		0
#define PT(X ...)		(X)
#endif

#define SIDBYTES		16
#define TIMEBYTES		6

constant emptyarray = ({});

//! Engine.IO protocol version.
constant protocol = 4;				// EIO Protocol version

//! Global options for all EngineIO instances.
//!
//! @seealso
//!  @[Socket.create()], @[Gz.compress()]
final mapping options = ([
  "pingTimeout":		64*1000,	// Safe for NAT
  "pingInterval":		29*1000,	// Allows for jitter
#if constant(Gz.deflate)
  "compressionLevel":		3,		// Setting to zero disables.
  "compressionStrategy":	Gz.DEFAULT_STRATEGY,
  "compressionThreshold":	256,		// Compress when size>=
  "compressionWindowSize":	15,		// LZ77 window 2^x (8..15)
  "compressionHeuristics":	Protocols.WebSocket.HEURISTICS_COMPRESS,
#endif
  "allowUpgrades":		1,
  "server_mtu":			4*1024,
  "server_textmtu":		8*1024,
  "server_inflight":		64*1024,
  "server_delayack":		20,		// 20ms.
]);

final multiset clientoptions = (<
  "EIO",
  "pingTimeout",
  "server_mtu",
  "server_textmtu",
  "server_inflight",
  "server_delayack",
>);

private enum {
  //          0         1      2     3     4        5        6     7
  BASE64='b', OPEN='0', CLOSE, PING, PONG, MESSAGE, UPGRADE, NOOP, ACK,
  SENDQEMPTY, SHUTDOWN
};

// All EngineIO sessions indexed on sid.
private mapping(string:Socket) clients = ([]);

private Regexp acceptgzip = Regexp("(^|,)gzip(,|$)");
private Regexp xxsua = Regexp(";MSIE|Trident/");

//! @param options
//! Optional options to override the defaults.
//! This parameter is passed down as is to the underlying
//!  @[Socket].
//!
//! @seealso
//!  @[Socket.create()]
final Socket farm(object createtype, Protocols.WebSocket.Request req,
 void|mapping(string:mixed) options) {
  string sid;
  PD("Request %O\n", req.query);
  if (sid = req.variables.sid) {
    Socket client;
    if (!(client = clients[sid]))
      req.response_and_finish((["data":"Unknown sid",
       "error":Protocols.HTTP.HTTP_GONE]));
    else
      client.onrequest(req);
  } else
    return createtype(req, options);
  return 0;
}

//! Runs a single Engine.IO session.
class Socket {

  //! Contains the last request seen on this connection.
  //! Can be used to obtain cookies etc.
  final Protocols.WebSocket.Request request;

  //! The unique session identifier (in the Engine.IO docs referred
  //! to as simply: id).
  final string sid;

  final mapping options;
  private Stdio.Buffer ci = Stdio.Buffer();
  private function(string|Stdio.Buffer:void) read_cb;
  private function(:void) close_cb;
  private function(:void) lowmark_cb;
  private PriorityQueue sendq = PriorityQueue();
  private Transport conn;
  private Transport upgtransport;
  private enum {RUNNING = 0, PAUSED, SCLOSING, RCLOSING};
  private int state = RUNNING;
  private int curpacketno = -1;
  private int scheduleack = 0;

  final int sendqbytesize() {
    return sendq.bytesize;
  }

#define PACKETLEN(v)	(1 + 2 + 1 + (sizeof(v) >= 2 && sizeof((v)[1])))
#define ACKRANGE	(1 << 7 << 7)	    // Two 7 bit bytes

  final void flushack(void|int noflush) {
    if (scheduleack) {
      remove_call_out(flushack);
      scheduleack = 0;
      string msg = sprintf("%c%c", curpacketno & 0x7f, curpacketno >> 7);
      if (noflush)
        sendq.write(({ACK, msg}));
      else
        send(ACK, msg);
    }
  }

  final void sendack() {
    if (options->EIO > 3 && !scheduleack) {
      scheduleack = 1;
      call_out(flushack, options->server_delayack);
    }
  }

  class PriorityQueue {
    inherit Thread.Queue;

    final int bytesize;

    private Thread.Queue priority = Thread.Queue();
    private Thread.Queue unacked = Thread.Queue();
    private int lastacked = -1;
    int insync = 1;

    final int write(array v) {
      switch (v[0]) {
        case MESSAGE:
        case CLOSE:
          bytesize += PACKETLEN(v);
          return ::write(v);
      }
      return priority.write(v);
    }

    final int size() {
      return priority.size() + ::size();
    }

    final array try_read() {
      array v = priority.try_read();
      if (v)
        return v;
      if (insync && unacked.size() < ACKRANGE / 2 - 1 && (v = ::try_read()))
        unacked.write(v);
      return v;
    }

    final void gotack(int packetno) {
      array v;				// Accept half the range in flight
      while ((packetno - lastacked & ACKRANGE - 1) < ACKRANGE / 2) {
        lastacked = (lastacked + 1) & ACKRANGE - 1;
        if (v = unacked.try_read())
          bytesize -= PACKETLEN(v);
      }
      PD("Gotack %s %d left %d\n", sid, packetno, bytesize);
      if (!insync) {
        ::write(@(unacked.try_read_array() + ::try_read_array()));
        insync = 1;
      }
      if (lowmark_cb)
        lowmark_cb();
    }
  }

  class Transport {
    final function(int, void|string|Stdio.Buffer:void) read_cb;
    final protected int pingtimeout;

    protected void create(Protocols.WebSocket.Request req,
     function(int, void|string|Stdio.Buffer:void) read_cb) {
      this->read_cb = read_cb;
      pingtimeout = (options->pingTimeout + options->pingInterval)/1000+1;
      kickwatchdog();
    }

    private void droptimeout() {
      remove_call_out(close);
    }

    protected void _destruct() {
      shutdown();
    }

    final protected void kickwatchdog() {
      droptimeout();
      call_out(close, pingtimeout);
    }

    //! Shutdown the transport silently.
    void shutdown() {
      droptimeout();
    }

    //! Close the transport properly, notify the far end.
    void close() {
      shutdown();
      if (read_cb)
        read_cb(SHUTDOWN);
    }

    final protected void sendqempty() {
      if (!sendq.size())
        read_cb(SENDQEMPTY);
    }
  }

  class Polling {
    inherit Transport;

    constant name = "polling";
    private int forceascii;
    final protected Stdio.Buffer c = Stdio.Buffer();
    final protected Stdio.Buffer ci = Stdio.Buffer();
    final protected Protocols.WebSocket.Request req;
    private mapping headers = ([]);
    private Thread.Mutex getreq = Thread.Mutex();
    private Thread.Mutex exclpost = Thread.Mutex();
  #if constant(Gz.deflate)
    private Gz.File gzfile;
  #endif
    protected string noop;

    protected void getbody(Protocols.WebSocket.Request _req);
    protected void wrapfinish(Protocols.WebSocket.Request req, string body,
     mapping(string:mixed) options);

    protected void respfinish(Protocols.WebSocket.Request req, string body,
     int isbin, mapping(string:mixed) options) {
      mapping|string comprheads;
      if (!sizeof(body))
        body = noop;			// Engine.IO does not like empty frames
  #if constant(Gz.deflate)
      if (gzfile
       && options->compressionLevel>0
       && sizeof(body) >= options->compressionThreshold
       && (comprheads = req.request_headers["accept-encoding"])
       && acceptgzip.match(comprheads)) {
        Stdio.FakeFile f = Stdio.FakeFile("", "wb");
        gzfile.open(f, "wb");
        gzfile.setparams(options->compressionLevel,
         options->compressionStrategy, options->compressionWindowSize);
        gzfile.write(body);
        gzfile.close();
        comprheads = headers;
        if (sizeof(body)>f.stat().size) {
          body = (string)f;
          comprheads += (["Content-Encoding":"gzip"]);
        }
      } else
  #endif
        comprheads = headers;
      req.response_and_finish(([
       "data":body,
       "type":isbin ? "application/octet-stream" : "text/plain;charset=UTF-8",
       "extra_heads":comprheads]));
    }

    protected void create(Protocols.WebSocket.Request _req,
     function(int, void|string|Stdio.Buffer:void) read_cb) {
      noop = sprintf("1:%c", NOOP);
      forceascii = !zero_type(_req.variables->b64);
      ci->set_error_mode(1);
  #if constant(Gz.deflate)
      if (options->compressionLevel)
        gzfile = Gz.File();
  #endif
      ::create(_req, read_cb);
      if (_req.request_headers.origin) {
        headers["Access-Control-Allow-Credentials"] = "true";
        headers["Access-Control-Allow-Origin"] = _req.request_headers.origin;
      } else
        headers["Access-Control-Allow-Origin"] = "*";
      // prevent XSS warnings on IE
      string ua = _req.request_headers["user-agent"];
      if (ua && xxsua.match(ua))
        headers["X-XSS-Protection"] = "0";
      onrequest(_req);
    }

    final void onrequest(Protocols.WebSocket.Request _req) {
      kickwatchdog();
      switch (_req.request_type) {
        case "GET":
          flush();
          req = _req;
          flush();
          break;
        case "OPTIONS":
          headers["Access-Control-Allow-Headers"] = "Content-Type";
          _req.response_and_finish((["data":"ok","extra_heads":headers]));
          break;
        case "POST": {
          getbody(_req);
          // Strangely enough, EngineIO 3 does not communicate back
          // over the POST request, so wrap it up.
          function ackpost() {
            _req.response_and_finish((["data":"ok","type":"text/plain",
             "extra_heads":headers]));
          };
  #if constant(Thread.Thread)
          // This requires _req to remain unaltered for the remainder of
          // this function.
          Thread.Thread(ackpost);	// Lower latency by doing it parallel
  #else
          ackpost();
  #endif
          do {
            Thread.MutexKey lock;
            if (lock = exclpost.trylock()) {
              int type;
              string|Stdio.Buffer res;
              Stdio.Buffer.RewindKey rewind;
              rewind = ci->rewind_on_error();
              while (sizeof(ci)) {
                if (catch {
                    int len, isbin;
                    if ((isbin = ci->read_int8()) <= 1) {
                      for (len = 0;
                           (type = ci->read_int8())!=0xff;
                           len=len*10+type);
                      len--;
                      type = ci->read_int8() + (isbin ? OPEN : 0);
                      res = isbin ? ci->read_buffer(len): ci->read(len);
                    } else {
                      ci->unread(1);
                      len = ci->sscanf("%d:")[0]-1;
                      type = ci->read_int8();
                      if (type == BASE64) {
                        type = ci->read_int8();
                        res = Stdio.Buffer(MIME.decode_base64(ci->read(len)));
                      } else
                        res = ci->read(len);
                    }
                  })
                  break;
                else {
                  rewind.update();
                  if (stringp(res))
                    res = utf8_to_string(res);
                  read_cb(type, res);
                }
              }
              lock = 0;
            } else
              break;
          } while (sizeof(ci));
          break;
        }
        default:
          _req.response_and_finish((["data":"Unsupported method",
           "error":Protocols.HTTP.HTTP_METHOD_INVALID,
           "extra_heads":(["Allow":"GET,POST,OPTIONS"])]));
      }
    }

    constant forcebinary = 0;

    final void flush(void|int type, void|string|Stdio.Buffer msg) {
      Thread.MutexKey lock;
      mapping(string:mixed) opts = options;
      Protocols.WebSocket.Request myreq;
      if (type) {
        if (lock = getreq.trylock()) {
          myreq = req;
          req = 0;
          lock = 0;
        }
        if (myreq)
          wrapfinish(myreq,
           sprintf("%d:%c%s", 1+sizeof(msg), type, msg), opts);
      } else if (req && sendq.size() && sendq.bytesize < opts->server_inflight
       && (lock = getreq.trylock())) {
        array tosend;
        flushack(1);
        if ((myreq = req) && (tosend = sendq.try_read())) {
          req = 0;
          array m = tosend;
          tosend = emptyarray;
          do
            tosend += ({m});
          while (sendq.bytesize < opts->server_inflight
           && (m = sendq.try_read()));
          lock = 0;
          int anybinary = 0;
          if (forcebinary && !forceascii)
            foreach (tosend;; m)
              if (sizeof(m)>=2 && !stringp(m[1])) {
                anybinary = 1;
                break;
              }
          foreach (tosend;; m) {
            type = m[0];
            msg = "";
            switch(sizeof(m)) {
              case 3:
                if (m[2])
                  opts += m[2];
              case 2:
                msg = m[1] || "";
            }
            if (!anybinary && stringp(msg)) {
              msg = string_to_utf8(msg);
              c->add((string)(1+sizeof(msg)), ':');
            } else if (!forceascii) {
              if (stringp(msg))
                c->add_int8(0);
              else {
                type -= OPEN;
                c->add_int8(1);
              }
              foreach ((string)(1+sizeof(msg));; int i)
                c->add_int8(i-'0');
              c->add_int8(0xff);
            } else {
              msg = MIME.encode_base64(msg->read(), 1);
              c->add((string)(1+1+sizeof(msg)), ':', BASE64);
            }
            c->add(type, msg);
          }
          if (sizeof(c))
            wrapfinish(myreq, c->read(), opts);
          sendqempty();
        } else
          lock = 0;
      }
    }
  }

  class XHR {
    inherit Polling;
    constant forcebinary = 1;

    final protected void getbody(Protocols.WebSocket.Request _req) {
      ci->add(_req.body_raw);
    }

    final protected
     void wrapfinish(Protocols.WebSocket.Request req, string body,
      mapping(string:mixed) options) {
      respfinish(req, body, String.range(body)[1]==0xff, options);
    }
  }

  class JSONP {
    inherit Polling;

    private string head;

    protected void create(Protocols.WebSocket.Request req,
     function(int, void|string|Stdio.Buffer:void) read_cb) {
      ::create(req, read_cb);
      head = "___eio[" + (int)req.variables->j + "](";
      noop = head+"\""+::noop+"\");";
    }

    final protected void getbody(Protocols.WebSocket.Request _req) {
      string d;
      if (d = _req.variables->d)
        // Reverse-map some braindead escape mechanisms
        ci->add(replace(d,({"\r\n","\\n"}),({"\r","\n"})));
    }

    final protected
     void wrapfinish(Protocols.WebSocket.Request req, string body,
      mapping(string:mixed) options) {
      c->add(head, Standards.JSON.encode(body), ");");
      respfinish(req, c->read(), 0, options);
    }
  }

  class WebSocket {
    inherit Transport;

    constant name = "websocket";
    private Protocols.WebSocket.Connection con;
    private Stdio.Buffer bb = Stdio.Buffer();
    private String.Buffer sb = String.Buffer();

    protected void create(Protocols.WebSocket.Request req,
     function(int, void|string|Stdio.Buffer:void) read_cb,
     Protocols.WebSocket.Connection _con) {
      con = _con;
      con.onmessage = recv;
      ::create(req, read_cb);
    }

    final void flush(void|int type, void|string|Stdio.Buffer msg) {
      mapping(string:mixed) opts = options;
      void sendit() {
        Protocols.WebSocket.Frame f = stringp(msg)
         ? Protocols.WebSocket.Frame(Protocols.WebSocket.FRAME_TEXT,
            sprintf("%c%s", type, msg))
         : Protocols.WebSocket.Frame(Protocols.WebSocket.FRAME_BINARY,
            sprintf("%c%s", type - OPEN, msg->read()));
        f.options += opts;
        if (catch(con.send(f))) {
          con.close();
          shutdown();
        }
      };
      if (msg)
        sendit();
      else {
        array m;
        if (sendq.size() && sendq.bytesize < opts->server_inflight)
          flushack(1);
        while (sendq.bytesize < opts->server_inflight
         && (m = sendq.try_read())) {
          opts = options;
          type = m[0];
          msg = "";
          switch(sizeof(m)) {
            case 3:
              if (m[2])
                opts += m[2];
            case 2:
              msg = m[1] || "";
          }
          sendit();
        }
        sendqempty();
      }
    }

    private void recv(Protocols.WebSocket.Frame f) {
      kickwatchdog();
      switch (f.opcode) {
        case Protocols.WebSocket.FRAME_TEXT:
          sb.add(f.text);
          break;
        case Protocols.WebSocket.FRAME_BINARY:
          bb->add(f.data);
          break;
        case Protocols.WebSocket.FRAME_CONTINUATION:
          if (sizeof(sb))
            sb.add(f.text);
          else
            bb->add(f.data);
          break;
        default:
          return;
      }
      if (f.fin && read_cb)
        if (sizeof(sb)) {
          string s = sb.get();
          read_cb(s[0], s[1..]);
        } else {
          int type = bb->read_int8();
          read_cb(type + OPEN, bb->read_buffer(sizeof(bb)));
        }
    }
  }

  //! Set callbacks and open socket.
  final void open(
    void|function(string|Stdio.Buffer:void) _read_cb,
    void|function(:void) _close_cb,
    void|function(:void) _lowmark_cb) {
    close_cb = _close_cb;
    lowmark_cb = _lowmark_cb;
    read_cb = _read_cb;
    send(OPEN, Standards.JSON.encode(
             (["sid": sid,
               "EIO": this::options->EIO || protocol,
               "upgrades":
                 this::options->allowUpgrades ? ({"websocket"}) : ({}),
               "pingInterval": this::options->pingInterval,
               "pingTimeout":  this::options->pingTimeout
             ])));
  }

  //! Send text @[string] or binary @[Stdio.Buffer] messages.
  final void write(mapping(string:mixed) options,
   string|Stdio.Buffer ... msgs) {
    if (state >= SCLOSING)
      DUSERERROR("Socket already shutting down");
    foreach (msgs;; string|Stdio.Buffer msg) {
      PD("Queue %s %c:%O\n", sid, MESSAGE, (string)msg);
      sendq.write(({MESSAGE, msg, options}));
    }
    if (state == RUNNING)
      flush();
  }

  private void send(int type, void|string|Stdio.Buffer msg) {
    PD("Queue %s %c:%O\n", sid, type, (string)(msg || ""));
    sendq.write(({type, msg}));
    switch (state) {
      case RUNNING:
      case SCLOSING:
        flush();
    }
  }

  private void flush() {
    mixed err;
    if (err = catch(conn.flush())) {
      PD("Flush failed %O\n", describe_backtrace(err));
      catch(conn.shutdown());
      if (upgtransport)
        catch(upgtransport.close());
    }
  }

  //! Close the socket signalling the other side.
  final void close() {
    if (state < SCLOSING) {
      if (close_cb)
        close_cb();
      PT("Send close, state %O\n", state);
      state = SCLOSING;
      catch(send(CLOSE));
    }
  }

  private void rclose() {
    close();
    state = RCLOSING;
    m_delete(clients, sid);
  }

  private void clearcallback() {
    close_cb = 0;
    lowmark_cb = 0;
    read_cb = 0;			// Sort of a race, if multithreading
    upgtransport = conn = 0;
  }

  private void recv(int type, void|string|Stdio.Buffer msg) {
#ifndef EIO_DEBUGMORE
    if (type!=SENDQEMPTY)
#endif
      PD("Received %s %c:%O\n", sid, type, (string)msg);
    switch (type) {
      default:	  // Protocol error
        PD("Protocol error %s %c\n", sid, type);
        close();
        break;
      case OPEN:
        catch {
          mixed m = Standards.JSON.decode(msg);
          PD("Received %s client options %O\n", sid, msg);
          if (mappingp(m)) {
            this::options += m &= clientoptions;
            PD("Resulting %s options %O\n", sid, this::options);
            sendack();
          }
        };
        break;
      case CLOSE:
        rclose();
        if (!sendq.size())
          clearcallback();
        break;
      case SENDQEMPTY:
        if (state == RCLOSING)
          clearcallback();
        break;
      case SHUTDOWN:
        rclose();
	clearcallback();
        break;
      case PING:
        send(PONG, msg);
        break;
      case MESSAGE:
        curpacketno = curpacketno + 1 & ACKRANGE - 1;
        sendack();
        if (read_cb)
          read_cb(msg);
        break;
      case ACK:
        if (sizeof(msg) != 2)
          close();			      // Protocol error
        else {
          if (!stringp(msg))
            msg = msg->read();
          sendq.gotack(msg[1] << 7 | msg[0]);
        }
        break;
    }
  }

  private void upgrecv(int type, string|Stdio.Buffer msg) {
    PD("UPGReceived %s %c:%O\n", sid, type, (string)msg);
    switch (type) {
      default:	  // Protocol error or CLOSE
        if (upgtransport) {
          upgtransport.shutdown();
          upgtransport = 0;
          if (state == PAUSED)
            state = RUNNING;
          if (conn)
            flush();
        }
      case SENDQEMPTY:
        break;
      case PING:
        state = PAUSED;
        if (!sendq.size())
          send(NOOP);
        flush();
        upgtransport.flush(PONG, msg);
        break;
      case UPGRADE: {
        upgtransport.read_cb = recv;
        conn.read_cb = 0;
        conn.shutdown();
        conn = upgtransport;
        sendq.insync = 0;
        sendack();
        flush();
        if (state == PAUSED)
          state = RUNNING;
        upgtransport = 0;
        flush();
        break;
      }
    }
  }

  protected void _destruct() {
    close();
  }

  //! @param options
  //! Optional options to override the defaults.
  //! @mapping
  //!   @member int "pingTimeout"
  //!     If, the connection is idle for longer than this, the connection
  //!     is terminated, unit in @expr{ms@}.
  //!   @member int "pingInterval"
  //!     The browser-client will send a small ping message every
  //!     @expr{pingInterval ms@}.
  //!   @member int "allowUpgrades"
  //!     When @expr{true@} (default), it allows the server to upgrade
  //!     the connection to a real @[Protocols.WebSocket] connection.
  //!   @member int "compressionLevel"
  //!     The gzip compressionlevel used to compress packets.
  //!   @member int "compressionThreshold"
  //!     Packets smaller than this will not be compressed.
  //! @endmapping
  protected void create(Protocols.WebSocket.Request req,
   void|mapping options) {
    request = req;
    this::options = .EngineIO.options;
    if (options && sizeof(options))
      this::options += options;
    switch (req.variables->transport) {
      default:
        req.response_and_finish((["data":"Unsupported transport",
         "error":Protocols.HTTP.HTTP_UNSUPP_MEDIA]));
        return;
      case "websocket":
        conn = WebSocket(req, recv,
          req.websocket_accept(0,
           ({ Protocols.WebSocket.permessagedeflate(options) })));
        break;
      case "polling":
        conn = (req.variables.j ? JSONP : XHR)(req, recv);
        break;
    }
    ci->add(Crypto.Random.random_string(SIDBYTES-TIMEBYTES));
    ci->add_hint(gethrtime(), TIMEBYTES);
    sid = MIME.encode_base64(ci->read());
    clients[sid] = this;
    PD("New EngineIO sid: %O\n", sid);
  }

  //! Handle request, and returns a new Socket object if it's a new
  //! connection.
  final void onrequest(Protocols.WebSocket.Request req) {
    string s;
    request = req;
    if ((s = req.variables->transport) == conn.name)
      conn.onrequest(req);
    else {
      PD("Transport %s %O\n", sid, s);
      switch (s) {
        case "polling":
          if (options->EIO > 3) {
            upgtransport = (req.variables.j ? JSONP : XHR)(req, upgrecv);
            upgtransport.flush(PONG, "probe");
            break;
          }
        default:
          req.response_and_finish((["data":"Invalid transport",
           "error":Protocols.HTTP.HTTP_UNSUPP_MEDIA]));
          return;
        case "websocket":
          upgtransport = WebSocket(req, upgrecv,
           req.websocket_accept(0,
            ({Protocols.WebSocket.permessagedeflate(options) })));
      }
    }
  }

  protected string _sprintf(int type, void|mapping flags) {
    string res=UNDEFINED;
    switch (type) {
      case 'O':
        res = sprintf(DRIVERNAME"(%s.%d,%s,%d,%d,%d)",
         sid, protocol, conn.name, state, sendq.size(),
         sizeof(clients));
        break;
    }
    return res;
  }
}
