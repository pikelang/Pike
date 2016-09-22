/*
 * Clean-room Engine.IO implementation
 */

#pike __REAL_VERSION__

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

//! Global options for all EngineIO instances.
final mapping options = ([
  "pingTimeout":		64*1000,	// Safe for NAT
  "pingInterval":		29*1000,	// Allows for jitter
  "allowUpgrades":		1,
  "compressionLevel":		1,		// gzip
  "compressionThreshold":	256		// Compress when size>=
]);

constant protocol = 3;				// EIO Protocol version

private enum {
  //          0         1      2     3     4        5        6
  BASE64='b', OPEN='0', CLOSE, PING, PONG, MESSAGE, UPGRADE, NOOP,
  SENDQEMPTY, FORCECLOSE
};

// All EngineIO sessions indexed on sid.
private mapping(string:Server) clients = ([]);

private Regexp acceptgzip = Regexp("(^|,)gzip(,|$)");
private Regexp xxsua = Regexp(";MSIE|Trident/");

final void setoptions(mapping(string:mixed) _options) {
  options += _options;
}

//! @example
//! Sample minimal implementation of an EngineIO server farm:
//!
//!void echo(mixed id, string|Stdio.Buffer msg) {
//!  id->write(msg);
//!}
//!
//!void wsrequest(array(string) protocols, object req) {
//!  httprequest(req);
//!}
//!
//!void httprequest(object req)
//!{ switch (req.not_query)
//!  { case "/engine.io/":
//!      Protocols.EngineIO.Server server = Protocols.EngineIO.farm(req);
//!      if (server) {
//!        server.set_callbacks(echo);
//!        server.write("Hello world!");
//!      }
//!      break;
//!  }
//!}
//!
//!int main(int argc, array(string) argv)
//!{ Protocols.WebSocket.Port(httprequest, wsrequest, 80);
//!  return -1;
//!}
final Server farm(Protocols.WebSocket.Request req) {
  string sid;
  PD("Request %O\n", req.query);
  if (sid = req.variables.sid) {
    Server client;
    if (!(client = clients[sid]))
      req.response_and_finish((["data":"Unknown sid",
       "error":Protocols.HTTP.HTTP_GONE]));
    else
      client.onrequest(req);
  } else
    return Server(req);
  return 0;
}

class Transport {
  final function(int, void|string|Stdio.Buffer:void) read_cb;
  final Thread.Queue sendq;
  final protected int pingtimeout;

  protected void create(Protocols.WebSocket.Request req) {
    pingtimeout = options->pingTimeout/1000+1;
    kickwatchdog();
  }

  private void droptimeout() {
    remove_call_out(close);
  }

  protected void destroy() {
    droptimeout();
  }

  final protected void kickwatchdog() {
    droptimeout();
    call_out(close, pingtimeout);
  }

  //! Close the transport.
  final void close() {
    droptimeout();
    read_cb(FORCECLOSE);
  }

  final protected void sendqempty() {
    if (!sendq.size())
      read_cb(SENDQEMPTY);
  }
}

class Polling {
  inherit Transport:t;

  private int forceascii;
  final protected Stdio.Buffer c = Stdio.Buffer();
  final protected Stdio.Buffer ci = Stdio.Buffer();
  final protected Protocols.WebSocket.Request req;
  private mapping headers = ([]);
  private Thread.Mutex getreq = Thread.Mutex();
  private Thread.Mutex exclpost = Thread.Mutex();
#if constant(Gz.File)
  private Gz.File gzfile;
#endif
  protected string noop;

  protected void getbody(Protocols.WebSocket.Request _req);
  protected void wrapfinish(Protocols.WebSocket.Request req, string body);

  protected void respfinish(Protocols.WebSocket.Request req,
   void|string body, void|string mimetype) {
    mapping|string comprheads;
    if (!body)
      body = noop;
#if constant(Gz.File)
    if (gzfile && sizeof(body) >= options->compressionThreshold
     && (comprheads = req.request_headers["accept-encoding"])
     && acceptgzip.match(comprheads)) {
      Stdio.FakeFile f = Stdio.FakeFile("", "wb");
      gzfile.open(f, "wb");
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
     "type":mimetype||"text/plain;charset=UTF-8",
     "extra_heads":comprheads]));
  }

  protected void create(Protocols.WebSocket.Request _req) {
    noop = sprintf("1:%c", NOOP);
    forceascii = !zero_type(_req.variables->b64);
    ci->set_error_mode(1);
    int clevel;
#if constant(Gz.File)
    if (clevel = options->compressionLevel)
      (gzfile = Gz.File()).setparams(clevel, Gz.DEFAULT_STRATEGY);
#endif
    t::create(_req);
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
        Thread.Thread(ackpost);		// Lower latency by doing it parallel
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
    if (req && sendq && sendq.size() && (lock = getreq.trylock())) {
      Protocols.WebSocket.Request myreq;
      array tosend;
      if ((myreq = req) && sizeof(tosend = sendq.try_read_array())) {
        req = 0;
        lock = 0;
        array m;
        int anybinary = 0;
        if (forcebinary && !forceascii)
          foreach (tosend;; m)
            if (!stringp(m[1])) {
              anybinary = 1;
              break;
            }
        foreach (tosend;; m) {
          type = m[0];
          msg = m[1];
          if (!anybinary && stringp(msg)) {
             if (String.width(msg) > 8)
               msg = string_to_utf8(msg);
            c->add((string)(1+sizeof(msg)))->add_int8(':');
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
            c->add((string)(1+1+sizeof(msg)))->add_int8(':')->add_int8(BASE64);
          }
          c->add_int8(type)->add(msg);
        }
        if (sizeof(c))
          wrapfinish(myreq, c->read());
        sendqempty();
      } else
        lock = 0;
    }
  }
}

class XHR {
  inherit Polling:p;
  constant forcebinary = 1;

  final protected void getbody(Protocols.WebSocket.Request _req) {
    ci->add(_req.body_raw);
  }

  final protected
   void wrapfinish(Protocols.WebSocket.Request req, string body) {
    respfinish(req, body, String.range(body)[1]==0xff
     ? "application/octet-stream" : 0);
  }
}

class JSONP {
  inherit Polling:p;

  private string head;

  protected void create(Protocols.WebSocket.Request req) {
    p::create(req);
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
   void wrapfinish(Protocols.WebSocket.Request req, string body) {
    c->add(head)->add(Standards.JSON.encode(body))->add(");");
    respfinish(req, c->read());
  }
}

class WebSocket {
  inherit Transport:t;

  private Protocols.WebSocket.Connection con;
  private Stdio.Buffer bb = Stdio.Buffer();
  private String.Buffer sb = String.Buffer();

  protected void create(Protocols.WebSocket.Request req,
   Protocols.WebSocket.Connection _con) {
    con = _con;
    con.onmessage = recv;
    con.onclose = close;
    t::create(req);
  }

  final void flush(void|int type, void|string|Stdio.Buffer msg) {
    void sendit() {
      con.send_text(sprintf("%c%s",type,stringp(msg) ? msg : msg->read()));
    };
    if (msg)
      sendit();
    else {
      array tosend;
      while (sizeof(tosend = sendq.try_read_array())) {
        array m;
        foreach (tosend;; m) {
          type = m[0];
          msg = m[1];
          sendit();
        }
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
    if (f.fin)
      if (sizeof(sb)) {
        string s = sb.get();
        read_cb(s[0], s[1..]);
      } else {
        int type = bb->read_int8();
        read_cb(type, bb->read_buffer(sizeof(bb)));
      }
  }
}

//! Runs a single Engine.IO session.
class Server {
  private Stdio.Buffer ci = Stdio.Buffer();
  private function(mixed, string|Stdio.Buffer:void) read_cb;
  private function(mixed:void) close_cb;
  //! Contains the last request seen on this connection.
  //! Can be used to obtain cookies etc.
  final Protocols.WebSocket.Request lastrequest;
  private mixed id;
  //! The unique session identifier.
  final string sid;
  private Thread.Queue sendq = Thread.Queue();
  private ADT.Queue recvq = ADT.Queue();
  private string curtransport;
  private Transport transport;
  private Transport upgtransport;
  private enum {RUNNING = 0, PAUSED, SCLOSING, RCLOSING};
  private int state = RUNNING;

  //! Set initial argument on callbacks.
  final void set_id(mixed _id) {
    id = _id;
  }

  //! Retrieve initial argument on callbacks.  Defaults to the Server
  //! object itself.
  final mixed query_id() {
    return id || this;
  }

  //! As long as the read callback has not been set, all received messages
  //! will be buffered.
  final void set_callbacks(
    void|function(mixed, string|Stdio.Buffer:void) _read_cb,
    void|function(mixed:void) _close_cb) {
    close_cb = _close_cb;		// Set close callback first
    read_cb = _read_cb;			// to avoid losing the close event
    flushrecvq();
  }

  //! Send text (string) or binary (Stdio.Buffer) messages.
  final void write(string|Stdio.Buffer ... msgs) {
    if (state >= SCLOSING)
      DUSERERROR("Socket already shutting down");
    foreach (msgs;; string|Stdio.Buffer msg) {
      PD("Queue %s %c:%O\n", sid, MESSAGE, (string)msg);
      sendq.write(({MESSAGE, msg}));
    }
    if (state == RUNNING)
      flush();
  }

  private void send(int type, void|string|Stdio.Buffer msg) {
    PD("Queue %s %c:%O\n", sid, type, (string)(msg || ""));
    sendq.write(({type, msg || ""}));
    switch (state) {
      case RUNNING:
      case SCLOSING:
        flush();
    }
  }

  private void flush() {
    if(catch(transport.flush()))
      transport.close();
  }

  private void flushrecvq() {
    while (read_cb && !recvq.is_empty())
      read_cb(query_id(), recvq.read());
  }

  //! Close the socket signalling the other side.
  final void close() {
    if (state < SCLOSING) {
      if (close_cb)
        close_cb(query_id());
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
    read_cb = 0;			// Sort of a race, if multithreading
    id = 0;				// Delete all references to this Server
    transport = 0;
  }

  private void recv(int type, void|string|Stdio.Buffer msg) {
#ifndef EIO_DEBUGMORE
    if (type!=SENDQEMPTY)
#endif
      PD("Received %s %c:%O\n", sid, type, (string)msg);
    switch (type) {
      default:	  // Protocol error or CLOSE
        close();
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
      case FORCECLOSE:
        rclose();
	clearcallback();
        break;
      case PING:
        send(PONG, msg);
        break;
      case MESSAGE:
        if (read_cb && recvq.is_empty())
          read_cb(query_id(), msg);
        else {
          recvq.write(msg);
          flushrecvq();
        }
        break;
    }
  }

  private void upgrecv(int type, string|Stdio.Buffer msg) {
    switch (type) {
      default:	  // Protocol error or CLOSE
        upgtransport = 0;
        if (state == PAUSED)
          state = RUNNING;
        if(transport)
          flush();
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
        transport = upgtransport;
        curtransport = "websocket";
        if (state == PAUSED)
          state = RUNNING;
        upgtransport = 0;
        flush();
        break;
      }
    }
  }

  protected void destroy() {
    close();
  }

  protected void create(Protocols.WebSocket.Request req) {
    lastrequest = req;
    switch (curtransport = req.variables.transport) {
      default:
        req.response_and_finish((["data":"Unsupported transport",
         "error":Protocols.HTTP.HTTP_UNSUPP_MEDIA]));
        return;
      case "websocket":
        transport = WebSocket(req, req.websocket_accept(0));
        break;
      case "polling":
        transport = req.variables.j ? JSONP(req) : XHR(req);
        break;
    }
    transport.read_cb = recv;
    transport.sendq = sendq;
    ci->add(Crypto.Random.random_string(SIDBYTES-TIMEBYTES));
    ci->add_hint(gethrtime(), TIMEBYTES);
    sid = MIME.encode_base64(ci->read());
    clients[sid] = this;
    send(OPEN, Standards.JSON.encode(
             (["sid":sid,
               "upgrades":
                 options->allowUpgrades ? ({"websocket"}) : ({}),
               "pingInterval":options->pingInterval,
               "pingTimeout":options->pingTimeout
             ])));
    PD("New EngineIO sid: %O\n", sid);
  }

  //! Handle request, and returns a new Server object if it's a new
  //! connection.
  final void onrequest(Protocols.WebSocket.Request req) {
    string s;
    lastrequest = req;
    if ((s = req.variables->transport) == curtransport)
      transport.onrequest(req);
    else
      switch (s) {
        default:
          req.response_and_finish((["data":"Invalid transport",
           "error":Protocols.HTTP.HTTP_UNSUPP_MEDIA]));
          return 0;
        case "websocket":
          upgtransport = WebSocket(req, req.websocket_accept(0));
          upgtransport.read_cb = upgrecv;
          upgtransport.sendq = sendq;
      }
  }

  private string _sprintf(int type, void|mapping flags) {
    string res=UNDEFINED;
    switch (type) {
      case 'O':
        res = sprintf(DRIVERNAME"(%s.%d,%s,%d,%d,%d,%d)",
         sid, protocol, curtransport, state, sendq.size(),
         recvq.is_empty(),sizeof(clients));
        break;
    }
    return res;
  }
}
