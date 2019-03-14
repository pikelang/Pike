#pike __REAL_VERSION__

constant websocket_id = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

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
    protected string buf = "";

    //! Add more data to the internal parsing buffer.
    void feed(string data) {
	buf += data;
    }

    //! Parses and returns one WebSocket frame from the internal buffer. If
    //! the buffer does not contain a full frame, @expr{0@} is returned.
    object parse() {
	if (sizeof(buf) < 2) return 0;

	int opcode, len, hlen = 2;
	int(0..1) masked;
	string mask, data;

	sscanf(buf, "%c%c", opcode, len);

	masked = len >> 7;
	len &= 127;

	if (len == 126) {
	    if (2 != sscanf(buf, "%2*s%2c", len)) return 0;
            hlen = 4;
	}  else if (len == 127) {
	    if (2 != sscanf(buf, "%2*s%8c", len)) return 0;
            hlen = 10;
	}

	if (masked) {
            hlen += 4;
            if (sizeof(buf) < hlen) return 0;
	    mask = buf[hlen-4..hlen-1];
	}

        if (sizeof(buf) < len+hlen) return 0;

        object f = Frame(opcode & 15);
        f->fin = opcode >> 7;
        f->mask = mask;

        data = buf[hlen..hlen+len-1];
        buf = buf[hlen+len..];

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
        this_program::opcode = opcode;
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
            this_program::data = data;
            break;
        case FRAME_CLOSE:
            if (!intp(data))
                error("Bad argument. Expected CLOSE_STATUS.\n");
            this_program::data = sprintf("%2c", data);
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
    string encode() {
        String.Buffer b = String.Buffer();

        b->putchar(fin << 7 | opcode);

        if (sizeof(data) > 0xffff) {
            b->sprintf("%c%8c", !!mask << 7 | 127, sizeof(data));
        } else if (sizeof(data) > 125) {
            b->sprintf("%c%2c", !!mask << 7 | 126, sizeof(data));
        } else b->putchar(!!mask << 7 | sizeof(data));

        if (mask) {
            b->add(mask);
            b->add(MASK(data, mask));
        } else {
            b->add(data);
        }

        return b->get();
    }

    protected string cast(string to) {
        if (to == "string") {
            return encode();
        }

        error("Cannot cast %O to %O\n", this, to);
    }
}

//!
class Connection {
    protected object parser;

    protected Stdio.File stream;
    protected array(string) stream_buf = ({ });
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
        this_program::id = id;
    }

    protected void create(Stdio.File f) {
        parser = Parser();
        stream = f;
        f->set_nonblocking(websocket_in, websocket_write, websocket_closed);
        state = OPEN;
        if (onopen) onopen(id || this);
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
        return `+(@map(stream_buf, sizeof));
    }

    void send_raw(string s) {
        stream_buf += ({ s });
    }

    protected void websocket_write() {
        if (sizeof(stream_buf)) {
            int n = stream->write(stream_buf);
            int i;

            if (n == -1) {
                int e = errno();
                if (e) {
                    websocket_closed();
                }
                will_write = 0;
                return;
            }

            foreach (stream_buf; i; string s) {
                n -= sizeof(s);

                if (!n) break;
                if (n < 0) {
                    stream_buf[i] = s[sizeof(s)+n..];
                    i--;
                    break;
                }
            }
            stream_buf = stream_buf[i+1..];
            will_write = 1;
        } else {
            will_write = 0;
        }
    }

    protected void websocket_in(mixed id, string data) {

        // it would be nicer to set the read callback to zero
        // once a close handshake has been received. however,
        // without a read callback pike does not trigger the
        // close event.
        if (state == CLOSED) return;
        parser->feed(data);

        while (object frame = parser->parse()) {
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
        stream_buf += ({ (string) frame });
        if (frame->opcode == FRAME_CLOSE) {
            state = CLOSING;
            close_reason = frame->reason;
            // TODO: time out the connection
        }
        if (!will_write) websocket_write();
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
class Request {
    inherit Protocols.HTTP.Server.Request;

    function(array(string), Request:void) cb;

    //!
    protected void create(function(array(string), Request:void) cb) {
	this_program::cb = cb;
    }

    protected void parse_request() {
	if (!cb || !has_index(request_headers, "sec-websocket-key")) {
	    ::parse_request();
	    return;
	} else {
	    string proto = request_headers["sec-websocket-protocol"];
	    array(string) protocols =  proto ? proto / ", " : ({});
#ifdef WEBSOCKET_DEBUG
            werror("websocket request: %O %O\n", this, protocols);
#endif
	    cb(protocols, this);
	}
    }

    protected int parse_variables() {
	if (has_index(request_headers, "sec-websocket-key"))
	    return 0;
	return ::parse_variables();
    }

    //! Calling @[websocket_accept] completes the WebSocket connection
    //! handshake. The protocol should be either @expr{0@} or a protocol
    //! advertised by the client when initiating the WebSocket connection.
    //! The returned connection object is in state @[Connection.OPEN].
    Connection websocket_accept(string protocol) {
	string s = request_headers["sec-websocket-key"] + websocket_id;
	mapping heads = ([
	    "Upgrade" : "websocket",
	    "Connection" : "Upgrade",
	    "sec-websocket-accept" : MIME.encode_base64(Crypto.SHA1.hash(s)),
            "sec-websocket-version" : "13",
	]);
	if (protocol) heads["sec-websocket-protocol"] = protocol;

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

    void create(function(Protocols.HTTP.Server.Request:void) http_cb,
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

    void create(function(Protocols.HTTP.Server.Request:void) http_cb,
                function(array(string), Request:void)|void ws_cb,
                void|int portno, void|string interface,
                void|string key, void|string|array certificate) {

        ::create(http_cb, portno, interface, key, certificate);

        if (ws_cb)
            request_program = Function.curry(Request)(ws_cb);
    }
}
