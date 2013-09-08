constant websocket_id = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

//! This module implements the WebSocket protocol as described in RFC 6455.

//! Parses WebSocket frames.
class Parser {
    string buf = "";

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

string MASK(string data, string mask) {
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

class Frame {
    //!
    FRAME opcode;

    //! Set to @expr{1@} if this a final frame, i.e. the last frame of a
    //! fragmented message or a non-fragmentation frame.
    int(0..1) fin = 1;

    string mask;

    //! Data part of the frame. Valid for frames of type @[FRAME_BINARY].
    string data = "";

    //! @decl void create(FRAME opcode, void|string|CLOSE_STATUS)
    //! @decl void create(FRAME_TEXT, string text)
    //! @decl void create(FRAME_BINARY, string(0..255) data)
    //! @decl void create(FRAME_CLOSE, CLOSE_STATUS reason)
    //! @decl void create(FRAME_PING, string(0..255) data)
    //! @decl void create(FRAME_PONG, string(0..255) data)
    void create(FRAME opcode, void|string|CLOSE_STATUS data) {
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
            this_program::data = sprintf("%2c", CLOSE_NORMAL);
            break;
        }
    }

    string _sprintf(int type) {
        return sprintf("%O(%s, fin: %d, %d bytes)", this_program,
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

    string cast(string to) {
        if (to == "string") {
            return encode();
        }

        error("Cannot cast %O to %O\n", this, to);
    }
}

//!
class Request {
    inherit Protocols.HTTP.Server.Request;

    object parser;

    function(array(string), Request:void) cb;
    function(Frame, mixed:void) msg_cb;
    function(Frame, mixed:void) ws_close_cb;

    mixed id;

    void set_id(mixed id) {
        this_program::id = id;
    }

    //!
    void create(function(array(string), Request:void) cb) {
	this_program::cb = cb;
    }

    protected void parse_request() {
	if (!has_index(request_headers, "sec-websocket-key")) {
	    ::parse_request();
	    return;
	} else {
	    string proto = request_headers["sec-websocket-protocol"];
	    array(string) protocols =  proto ? proto / ", " : ({});
	    cb(protocols, this);
	}
    }

    protected int parse_variables() {
	if (has_index(request_headers, "sec-websocket-key"))
	    return 0;
	return ::parse_variables();
    }

    Stdio.File stream;
    array(string) stream_buf = ({ });
    int(0..1) will_write = 1;
    int(0..1) closing = 0;

    //! @decl int bufferedAmount
    //! Number of bytes in the send buffer.

    int `bufferdAmount() {
        return `+(@map(stream_buf, sizeof));
    }

    void websocket_write() {
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

    void websocket_in(mixed id, string data) {
        parser->feed(data);

        while (object frame = parser->parse()) {
            switch (frame->opcode) {
            case FRAME_PING:
                websocket_send(Frame(FRAME_PONG, frame->data));
                break;
            case FRAME_CLOSE:
                if (!closing) {
                    websocket_close(frame->reason);
                } else {
                    finish(0);
                }
                if (ws_close_cb) ws_close_cb(frame, id || this);
                return;
            }

            if (msg_cb) msg_cb(frame, id || this);
            else werror("received: %O\n", frame);
        }
    }

    void finish(int clean) {
        if (stream) {
            // we force shutdown here, if this was a websocket connection
            cb = 0;
            ws_close_cb = 0;
            msg_cb = 0;
            stream->set_nonblocking(0, 0, 0);
            catch { stream->close(); };
            stream = 0;
            ::finish(0);
        } else {
            if (!clean) cb = 0;
            ::finish(clean);
        }
    }

    //! Calling @[websocket_accept] completes the WebSocket connection
    //! handshake. The protocol should be either @expr{0@} or a protocol
    //! advertised by the client when initiating the WebSocket conneciton.
    //! @expr{msg_cb@} will be called subsequently for each incoming WebSocket
    //! frame.
    //! @expr{close_cb@} will be called once the WebSocket connection has been
    //! closed by a proper close handshake. If the connection is terminated
    //! without a proper handshake, the first argument to @expr{close_cb@} will
    //! be @expr{0@}.
    void websocket_accept(string protocol, function(Frame,this_program:void) msg_cb,
                          function(int|Frame,this_program:void) close_cb) {
	string s = request_headers["sec-websocket-key"] + websocket_id;
	mapping heads = ([
	    "Upgrade" : "websocket",
	    "Connection" : "Upgrade",
	    "sec-websocket-accept" : MIME.encode_base64(Crypto.SHA1.hash(s)),
            "sec-websocket-version" : "13",
	]);
	if (protocol) heads["sec-websocket-protocol"] = protocol;

        this_program::msg_cb = msg_cb;
        ws_close_cb = close_cb;

	stream = my_fd;

        stream_buf += ({ make_response_header(([
	    "error" : 101,
	    "type" : "application/octet-stream",
	    "extra_heads" : heads,
	])) });
        stream->set_nonblocking(websocket_in, websocket_write,
                                websocket_closed);
        parser = Parser();
    }

    void websocket_closed() {
        if (!closing && ws_close_cb) ws_close_cb(0, this || id);
        finish(0);
    }

    //! Send a WebSocket ping frame.
    void websocket_ping(void|string s) {
        websocket_send(Frame(FRAME_PING, s));
    }

    //! Send a WebSocket connection close frame. The close callback will be
    //! called when the close handshake has been completed. No more frames
    //! can be sent after initiating the close handshake.
    void websocket_close(void|CLOSE_STATUS reason) {
        websocket_send(Frame(FRAME_CLOSE, reason||CLOSE_NORMAL));
    }

    //! Send a WebSocket frame.
    void websocket_send(Frame frame) {
        if (closing) error("Cannot send frames after close.\n");
        if (!stream) error("%O is not a websocket frame.\n", this);
        stream_buf += ({ (string) frame });
        if (frame->opcode == FRAME_CLOSE)
            closing = 1;
        if (!will_write) websocket_write();
    }

    //! Send a WebSocket text frame.
    void send_text(string s) {
        websocket_send(Frame(FRAME_TEXT, s));
    }

    //! Send a WebSocket binary frame.
    void send_binary(string(0..255) s) {
        websocket_send(Frame(FRAME_BINARY, s));
    }

    protected string _sprintf(int type) {
        if (!stream) return ::_sprintf(type);
        return sprintf("%O(%O)", this_program, stream);
    }
}

//! Creates a simple HTTP Server. @expr{ws_cb@} will be called for all incoming
//! WebSocket connections. Its first argument are the list of protocols
//! requested by the client and the second argument the corresponding
//! @[Request] object. The WebSocket connection handshake is completed
//! by calling @[Request.websocket_accept].
//! @expr{http_cb@} will be called for all other HTTP Requests.
//! @seealso
//!     @[Protocols.HTTP.Server.Port]
Protocols.HTTP.Server.Port
    Port(function(Protocols.HTTP.Server.Request:void) http_cb,
         function(array(string), Request:void) ws_cb,
         void|int portno, void|string interface) {

    object p = Protocols.HTTP.Server.Port(http_cb, portno, interface);
    p->request_program = Function.curry(Request)(ws_cb);
    return p;
}

//! Opens a simple HTTPS Server which supports WebSocket connections.
//! @seealso
//!     @[Port], @[Protocols.HTTP.Server.SSLPort]
Protocols.HTTP.Server.SSLPort
    SSLPort(function(Protocols.HTTP.Server.Request:void) http_cb,
            function(array(string), Request:void) ws_cb,
            void|int portno, void|string interface,
            void|string key, void|string|array certificate) {

    object p = Protocols.HTTP.Server.SSLPort(http_cb, portno, interface, key, certificate);
    p->request_program = Function.curry(Request)(ws_cb);
    return p;
}
