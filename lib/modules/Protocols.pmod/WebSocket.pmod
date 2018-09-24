#pike __REAL_VERSION__

constant websocket_id = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

constant websocket_version = 13;

//! This module implements the WebSocket protocol as described in RFC 6455.

protected string MASK(string data, string mask) {
    return data ^ (mask * (sizeof(data)/(float)sizeof(mask)));
}

//! WebSocket frame opcodes.
enum FRAME {
    //!
    FRAME_CONTINUATION = 0x0,

    //!
    FRAME_TEXT,

    //!
    FRAME_BINARY,

    //!
    FRAME_CLOSE = 0x8,

    //!
    FRAME_PING,

    //!
    FRAME_PONG,
};

//! WebSocket close status codes, as explained in the WebSocket protocol
//! specification.
enum CLOSE_STATUS {
    //!
    CLOSE_NORMAL = 1000,

    //!
    CLOSE_GONE_AWAY,

    //!
    CLOSE_ERROR,

    //!
    CLOSE_BAD_TYPE,

    //!
    CLOSE_NONE = 1005,

    //!
    CLOSE_BAD_DATA = 1007,

    //!
    CLOSE_POLICY,

    //!
    CLOSE_EXTENSION = 1010,

    //!
    CLOSE_UNEXPECTED,
};

#define FOO(x)  if (op == x) return #x
string describe_opcode(FRAME op) {
    FOO(FRAME_CONTINUATION);
    FOO(FRAME_TEXT);
    FOO(FRAME_BINARY);
    FOO(FRAME_CLOSE);
    FOO(FRAME_PING);
    FOO(FRAME_PONG);
    return sprintf("0x%x", op);
}

//! Parses WebSocket frames.
class Parser {
    //! Unparsed data.
    Stdio.Buffer buf = Stdio.Buffer();

    //! Add more data to the internal parsing buffer.
    void feed(string data) {
        buf->add(data);
    }

    //! Parses and returns one WebSocket frame from the internal buffer. If
    //! the buffer does not contain a full frame, @expr{0@} is returned.
    Frame parse() {
	if (sizeof(buf) < 2) return 0;

	int opcode, len, hlen = 2;
	int(0..1) masked;
	string mask, data;

        opcode = buf->read_int8();
        len = buf->read_int8();

	masked = len >> 7;
	len &= 127;

	if (len == 126) {
            len = buf->read_int16();
            if (len == -1) {
                buf->unread(2);
                return 0;
            }
            hlen = 4;
	}  else if (len == 127) {
            len = buf->read_int(8);
            if (len == -1) {
                buf->unread(2);
                return 0;
            }
            hlen = 10;
	}

	if (masked) {
            if (sizeof(buf) < 4 + len) {
                buf->unread(hlen);
                return 0;
            }
            mask = buf->read(4);
	} else if (sizeof(buf) < len) {
            buf->unread(hlen);
            return 0;
        }

        Frame f = Frame(opcode & 15);
        f->fin = opcode >> 7;
        f->mask = mask;

        data = buf->read(len);

        if (masked) {
            data = MASK(data, mask);
        }

        f->data = data;

        return f;
    }
}

class Frame {
    //!
    FRAME opcode;

    //! Set to @expr{1@} if this a final frame, i.e. the last frame of a
    //! fragmented message or a non-fragmentation frame.
    int(0..1) fin = 1;

    string mask;

    //! Data part of the frame. Valid for frames of type @[FRAME_BINARY],
    //! @[FRAME_PING] and @[FRAME_PONG].
    string data = "";

    //! @decl void create(FRAME opcode, void|string|CLOSE_STATUS)
    //! @decl void create(FRAME_TEXT, string text)
    //! @decl void create(FRAME_BINARY, string(0..255) data)
    //! @decl void create(FRAME_CLOSE, CLOSE_STATUS reason)
    //! @decl void create(FRAME_PING, string(0..255) data)
    //! @decl void create(FRAME_PONG, string(0..255) data)
    protected void create(FRAME opcode, void|string|CLOSE_STATUS data) {
        this::opcode = opcode;
        if (data) switch (opcode) {
        case FRAME_TEXT:
            data = string_to_utf8(data);
        case FRAME_PONG:
        case FRAME_PING:
        case FRAME_BINARY:
            if (!stringp(data))
                error("Bad argument. Expected string.\n");
            if (String.width(data) != 8)
                error("%s frames cannot hold widestring data.\n",
                      describe_opcode(opcode));
            this::data = data;
            break;
        case FRAME_CLOSE:
            if (!intp(data))
                error("Bad argument. Expected CLOSE_STATUS.\n");
            this::data = sprintf("%2c", data);
            break;
        }
    }

    protected string _sprintf(int type) {
      return type=='O' && sprintf("%O(%s, fin: %d, %d bytes)", this_program,
                       describe_opcode(opcode), fin, sizeof(data));
    }

    //! @decl string text
    //! Only exists for frames of type @[FRAME_TEXT].

    string `text() {
        if (opcode != FRAME_TEXT) error("Not a text frame.\n");
        return utf8_to_string(data);
    }

    string `text=(string s) {
        if (opcode != FRAME_TEXT) error("Not a text frame.\n");
        data = string_to_utf8(s);
        return s;
    }

    //! @decl CLOSE_STATUS reason
    //! Only exists for frames of type @[FRAME_CLOSE].

    CLOSE_STATUS `reason() {
        int i;
        if (opcode != FRAME_CLOSE)
            error("This is not a close frame.\n");
        if (sscanf(data, "%2c", i) != 1) {
            i = CLOSE_NORMAL;
        }
        return i;
    }

    CLOSE_STATUS `reason=(CLOSE_STATUS r) {
        if (opcode != FRAME_CLOSE)
            error("This is not a close frame.\n");
        data = sprintf("%2c", r);
        return r;
    }

    //!
    void encode(Stdio.Buffer buf) {
        buf->add_int8(fin << 7 | opcode);

        if (sizeof(data) > 0xffff) {
            buf->add_int8(!!mask << 7 | 127);
            buf->add_int(sizeof(data), 8);
        } else if (sizeof(data) > 125) {
            buf->add_int8(!!mask << 7 | 126);
            buf->add_int16(sizeof(data));
        } else buf->add_int8(!!mask << 7 | sizeof(data));

        if (mask) {
            buf->add(mask, MASK(data, mask));
        } else {
            buf->add(data);
        }
    }

    protected string cast(string to)
    {
      if (to == "string") {
        Stdio.Buffer buf = Stdio.Buffer();
        encode(buf);
        return buf->read();
      }
      return UNDEFINED;
    }
}

//!
class Connection {
    //! An instance of @[Parser] used to parse incoming data.
    Parser parser;

    //! The actual client connection.
    Stdio.File|SSL.File stream;

    //! Output buffer
    Stdio.Buffer buf = Stdio.Buffer();

    //! Used in client mode to read server response.
    protected Stdio.Buffer http_buffer = Stdio.Buffer();

    //! Remote endpoint in client mode
    Standards.URI endpoint;

    protected int(0..1) will_write = 1;
    protected mixed id;

    //! If true, all outgoing frames are masked.
    int(0..1) masking;

    //!
    enum STATE {

        //!
        CONNECTING = 0x0,

        //!
        OPEN,

        //!
        CLOSING,

        //!
        CLOSED,
    };

    //!
    STATE state = CONNECTING;

    protected CLOSE_STATUS close_reason;

    //! Set the @expr{id@}. It will be passed as last argument to all
    //! callbacks.
    void set_id(mixed id) {
        this::id = id;
    }

    //! Create a WebSocket client
    protected void create() {
	  // Clients start in state CLOSED.
      state = CLOSED;
	  masking = 1;
      parser = Parser();
    }

    //! Create a WebSocket server out of the given Stdio.File-like object
    protected variant void create(Stdio.File|SSL.File f) {
        state = CONNECTING;
        parser = Parser();
        stream = f;
        f->set_nonblocking(websocket_in, websocket_write, websocket_closed);
        state = OPEN;
        if (onopen) onopen(id || this);
    }

    //! Read callback for HTTP answer when we are in client mode and
    //! have requested an upgrade to WebSocket.
    protected void http_read(mixed id, string data) {
        http_buffer->add(data);

        array tmp = http_buffer->sscanf("%s\r\n\r\n");
                if (tmp && sizeof(tmp)) {
		  stream->set_blocking_keep_callbacks();

		  array lines = tmp[0]/"\r\n";
		  string resp = lines[0];

		  int http_major, http_minor, status;
		  if (sscanf(resp, "HTTP/%d.%d %d", http_major, http_minor, status) != 3) {
			websocket_closed();
			return;
		  }

		  if (status != 101) {
#ifdef WEBSOCKET_DEBUG
			werror("Failed to upgrade connection!\n");
#endif
			websocket_closed();
			return;
		  }

		  mapping headers = ([]);
		  foreach(lines[1..];int i; string l) {
			int res = sscanf(l, "%s: %s", string h, string v);
			if (res != 2) {
			  break;
			}
			headers[h] = v;
		  }

		  // At this point, we are upgraded, so let's set the
		  // websocket callback handlers
          masking = 1; // RFC6455 dictates that clients always use masking!
          state = OPEN;
          if (onopen) onopen(this_program::id || this);

          if (sizeof(http_buffer))
            websocket_in(id, http_buffer->read());

		  stream->set_nonblocking(websocket_in, websocket_write, websocket_closed);
		}
	}

    //! Write callback during the upgrade of a socket.
    protected void request_upgrade() {
		int res = buf->output_to(stream);

		if (res < 0) {
		  websocket_closed();
		  return;
		}

		if (!sizeof(buf)) {
		  // Whole request written to server - we now rest until we
		  // have the reply!
		  stream->set_write_callback(0);
		}
	}

    mapping low_connect(Standards.URI endpoint,
			void|mapping(string:string) extra_headers)
    {
      mapping headers = ([
	"Host" : endpoint->host,
	"Connection" : "Upgrade",
	"User-Agent" : "Pike/8.0",
	"Accept": "*/*",
	"Upgrade" : "websocket",
	"Sec-WebSocket-Key" : "x4JJHMbDL1EzLkh9GBhXDw==",
	"Sec-WebSocket-Version": (string)websocket_version,
      ]);

      foreach(extra_headers; string idx; string val) {
	headers[idx] = val;
      }

      return headers;
    }

    //! Connect to a remote WebSocket.
    //! This method will send the actual HTTP request to switch
    //! protocols to the server and once a HTTP 101 response is
    //! returned, switch the connection to WebSockets and call the
    //! @[onopen] callback.
    int connect(Standards.URI endpoint, void|mapping(string:string) extra_headers) {
	    this_program::endpoint = endpoint;
		extra_headers = extra_headers || ([]);

		Stdio.File f = Stdio.File();
		state = CONNECTING;

        // FIXME:
        // We should probably do an async connect here.
		int res = f->connect(endpoint->host, endpoint->port);
		if (!res) {
		  websocket_closed();
		  return 0;
		}

		if (endpoint->scheme == "wss") {
          // If we are connecting a TLS endpoint, so let's turn our
          // connection into a TLS one.
		  SSL.Context ctx = SSL.Context();
		  stream = SSL.File(f, ctx);
		  object ssl_session = stream->connect(endpoint->host,0);
		  if (!ssl_session) {
#ifdef WEBSOCKET_DEBUG
			werror("Handshake failed\n");
#endif
			websocket_closed();
			return 0;
		  }
		} else {
		  stream = f;
		}

		mapping headers = low_connect(endpoint, extra_headers);

		// We use our output buffer to generate the request.
		buf->add("GET ", endpoint->path," HTTP/1.1\r\n");
		buf->add("Host: ", endpoint->host, "\r\n");
		foreach(headers; string h; string v) {
		  buf->add(h, ": ", v, "\r\n");
		}
		buf->add("\r\n");

		stream->set_nonblocking(http_read, request_upgrade, websocket_closed);
		return res;
	}

    // Sorry guys...

    //!
    function(mixed:void) onopen;

    //!
    function(Frame, mixed:void) onmessage;

    //! This callback will be called once the WebSocket has been closed.
    //! No more frames can be sent or will be received after the close
    //! event has been triggered.
    //! This happens either when receiving a frame initiating the close
    //! handshake or after the TCP connection has been closed. Note that
    //! this is a deviation from the WebSocket API specification.
    function(CLOSE_STATUS, mixed:void) onclose;

    //! @decl int bufferedAmount
    //! Number of bytes in the send buffer.

    int `bufferdAmount() {
        return sizeof(buf);
    }

    void send_raw(string s) {
        buf->add(s);
    }

    protected void websocket_write() {
        if (sizeof(buf)) {
            int n = buf->output_to(stream);

            if (n < 0) {
                int e = errno();
                if (e) {
                    websocket_closed();
                }
                will_write = 0;
            } else will_write = 1;

        } else will_write = 0;
    }

    protected void websocket_in(mixed _id, string data) {

        // it would be nicer to set the read callback to zero
        // once a close handshake has been received. however,
        // without a read callback pike does not trigger the
        // close event.
        if (state == CLOSED) return;
        parser->feed(data);

        while (Frame frame = parser->parse()) {
#ifdef WEBSOCKET_DEBUG
            werror("%O in %O\n", this, frame);
#endif
            switch (frame->opcode) {
            case FRAME_PING:
                send(Frame(FRAME_PONG, frame->data));
                continue;
            case FRAME_CLOSE:
                if (state == OPEN) {
                    close(frame->reason);
                    // we call close_event here early to allow applications to stop
                    // sending packets. i think this makes more sense than what the
                    // websocket api specification proposes.
                    close_event(frame->reason);
                    break;
                } else if (state == CLOSING) {
                    stream->set_nonblocking(0,0,0);
                    catch { stream->close(); };
                    stream = 0;
                    // we dont use frame->reason here, since that is not guaranteed
                    // to be the same as the one used to start the close handshake
                    close_event(close_reason);
                    break;
                }
            }

            if (onmessage) onmessage(frame, id || this);
        }
    }

    protected void close_event(CLOSE_STATUS reason) {
        state = CLOSED;
        if (onclose) {
            onclose(reason, id || this);
            onclose = 0;
        }
    }

    protected void websocket_closed() {
        stream->set_nonblocking(0,0,0);
        stream = 0;
        // if this is the end of a proper close handshake, this wont do anything
        close_event(0);
    }

    //! Send a WebSocket ping frame.
    void ping(void|string s) {
        send(Frame(FRAME_PING, s));
    }

    //! Send a WebSocket connection close frame. The close callback will be
    //! called when the close handshake has been completed. No more frames
    //! can be sent after initiating the close handshake.
    void close(void|CLOSE_STATUS reason) {
        send(Frame(FRAME_CLOSE, reason||CLOSE_NORMAL));
    }

    //! Send a WebSocket frame.
    void send(Frame frame) {
        if (state != OPEN) error("WebSocket connection is not open: %O.\n", this);
        if (masking && sizeof(frame->data))
            frame->mask = Crypto.Random.random_string(4);
        frame->encode(buf);
        if (frame->opcode == FRAME_CLOSE) {
            state = CLOSING;
            close_reason = frame->reason;
            // TODO: time out the connection
        }
	if (!will_write && stream) stream->write("");
    }

    //! Send a WebSocket text frame.
    void send_text(string s) {
        send(Frame(FRAME_TEXT, s));
    }

    //! Send a WebSocket binary frame.
    void send_binary(string(0..255) s) {
        send(Frame(FRAME_BINARY, s));
    }

}

//!
class Request(function(array(string), Request:void) cb) {
    inherit Protocols.HTTP.Server.Request;

    protected int parse_variables() {
	if (!has_index(request_headers, "sec-websocket-key"))
	    return ::parse_variables();
	if (query!="")
	    .HTTP.Server.http_decode_urlencoded_query(query,variables);
	flatten_headers();
	if (cb) {
	    string proto = request_headers["sec-websocket-protocol"];
	    array(string) protocols =  proto ? proto / ", " : ({});
#ifdef WEBSOCKET_DEBUG
            werror("websocket request: %O %O\n", this, protocols);
#endif
	    cb(protocols, this);
	}
	return 0;
    }

    mapping(string:string) low_websocket_accept(string protocol)
    {
	string s = request_headers["sec-websocket-key"] + websocket_id;
	mapping heads = ([
	    "Upgrade" : "websocket",
	    "Connection" : "Upgrade",
	    "sec-websocket-accept" : MIME.encode_base64(Crypto.SHA1.hash(s)),
            "sec-websocket-version" : (string)websocket_version,
	]);
	if (protocol) heads["sec-websocket-protocol"] = protocol;
	return heads;
    }

    //! Calling @[websocket_accept] completes the WebSocket connection
    //! handshake. The protocol should be either @expr{0@} or a protocol
    //! advertised by the client when initiating the WebSocket connection.
    //! The returned connection object is in state @[Connection.OPEN].
    Connection websocket_accept(string protocol) {
        mapping heads = low_websocket_accept(protocol);

        Connection ws = Connection(my_fd);
        my_fd = 0;

        array a = allocate(1 + sizeof(heads));
        int i = 1;

        a[0] = "HTTP/1.1 101 SwitchingProtocols";

        foreach (heads; string k; string v) {
            a[i++] = sprintf("%s: %s", k, v);
        }

        string reply = a * "\r\n" + "\r\n\r\n";

        ws->send_raw(reply);

#ifdef WEBSOCKET_DEBUG
        werror("%O reply %O\n", protocol, reply);
#endif

        finish(0);

        return ws;
    }
}

//! Creates a simple HTTP Server. @expr{ws_cb@} will be called for all incoming
//! WebSocket connections. Its first argument are the list of protocols
//! requested by the client and the second argument the corresponding
//! @[Request] object. The WebSocket connection handshake is completed
//! by calling @[Request.websocket_accept].
//! @expr{http_cb@} will be called for all other HTTP Requests or if @expr{ws_cb@}
//! is zero.
//! @seealso
//!     @[Protocols.HTTP.Server.Port]
class Port {
    inherit Protocols.HTTP.Server.Port;

    protected void create(function(Protocols.HTTP.Server.Request:void) http_cb,
                          function(array(string), Request:void)|void ws_cb,
                          void|int portno, void|string interface) {

        ::create(http_cb, portno, interface);

        if (ws_cb)
            request_program = Function.curry(Request)(ws_cb);
    }
}

//! Opens a simple HTTPS Server which supports WebSocket connections.
//! @seealso
//!     @[Port], @[Protocols.HTTP.Server.SSLPort]
class SSLPort {
    inherit Protocols.HTTP.Server.SSLPort;

    protected void create(function(Protocols.HTTP.Server.Request:void) http_cb,
                          function(array(string), Request:void)|void ws_cb,
                          void|int portno, void|string interface,
                          void|string|Crypto.Sign.State key, void|string|array(string) certificate) {

        ::create(http_cb, portno, interface, key, certificate);

        if (ws_cb)
            request_program = Function.curry(Request)(ws_cb);
    }
}
