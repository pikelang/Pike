#pike __REAL_VERSION__

#ifdef WEBSOCKET_DEBUG
# define WS_WERR(level, x...)  do { if (WEBSOCKET_DEBUG >= level) { werror("%O: ", this); werror(x); } } while(0)
#else
# define WS_WERR(level, x...)
#endif

constant websocket_id = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

//! This module implements the WebSocket protocol as described in
//! @rfc{6455@}.

protected string MASK(string data, string mask) {
    return data ^ (mask * (sizeof(data)/(float)sizeof(mask)));
}

//! 
typedef function(Frame, mixed:void) message_callback;

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
    CLOSE_OVERFLOW,

    //!
    CLOSE_EXTENSION = 1010,

    //!
    CLOSE_UNEXPECTED,
};

//! WebSocket RSV extension bits.
enum RSV {

    //!
    RSV1 = 0x40,

    //!
    RSV2 = 0x20,

    //!
    RSV3 = 0x10

};

//! WebSocket frame compression heuristics override
enum COMPRESSION {

    //!
    HEURISTICS_COMPRESS = 0,

    //!
    OVERRIDE_COMPRESS,

    //!
    OVERRIDE_SKIPCOMPRESS,

};

//! Global default options for all WebSocket connections.
mapping options = ([
#if constant(Gz.deflate)
    "compressionLevel":3,
    "compressionThreshold":5,
    "compressionThresholdNoContext":256,
    "compressionStrategy":Gz.DEFAULT_STRATEGY,
    "compressionWindowSize":15,
    "decompressionWindowSize":15
#endif
]);

int(0..1) is_valid_close(int close_status) {
    switch (close_status) {
    case CLOSE_NORMAL:
    case CLOSE_GONE_AWAY:
    case CLOSE_ERROR:
    case CLOSE_BAD_TYPE:
    /* case CLOSE_NONE: */
    case CLOSE_BAD_DATA:
    case CLOSE_POLICY:
    case CLOSE_OVERFLOW:
    case CLOSE_EXTENSION:
    case CLOSE_UNEXPECTED:
    case 3000 .. 3999: /* reserved for registration with IANA */
    case 4000 .. 4999: /* reserved for private use */
        return 1;
    }
    return 0;
}

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

//! Parses one WebSocket frame. Throws an error if there isn't enough data in the buffer.
protected Frame low_parse(Connection con, Stdio.Buffer buf) {
    int opcode, len;
    int(0..1) masked;
    string mask, data;

    Stdio.Buffer.RewindKey rewind_key = buf->rewind_on_error();
    opcode = buf->read_int8();
    len = buf->read_int8();

    masked = len >> 7;
    len &= 127;

    if (len == 126) {
        len = buf->read_int16();
    }  else if (len == 127) {
        len = buf->read_int(8);
    }

    if (masked) {
        mask = buf->read(4);
    }

    data = buf->read(len);
    rewind_key->release();

    Frame f = Frame(opcode & 15);
    f->fin = opcode >> 7;
    f->mask = mask;
    f->rsv = opcode;

    if (con.lastopcode & 0x80)	       // FIN
        con.firstrsv = opcode;
    con.lastopcode = opcode;

    if (masked) {
        data = MASK(data, mask);
    }

#if constant(Gz.deflate)
    mapping options = con.options;
    if (options->compressionLevel && con.firstrsv & RSV1 && sizeof(data)) {
        Gz.inflate uncompress = con.uncompress;
        if (!uncompress)
            uncompress = Gz.inflate(-options->decompressionWindowSize);
        data = uncompress.inflate(data + "\0\0\377\377");
        string s;
        while (s = uncompress.end_of_stream()) {
            // FIXME uncompress.create(...) should have been sufficient.
            // Probably due to an upstream bug in zlib, this does not work.
            uncompress = Gz.inflate(-options->decompressionWindowSize);
            if (s != "\0\0\377\377")
                data += uncompress.inflate(s);
        }
        if (options->decompressionNoContextTakeover)
          uncompress = 0;	      // Save memory between packets
        con.uncompress = uncompress;
    }
#endif

    f->data = data;

    return f;
}

//! Parses one WebSocket frame. Returns @expr{0@} if the buffer does not contain enough data.
Frame parse(Connection con, Stdio.Buffer in) {
    // We wrap the low_parse() method to catch read errors, which are thrown, in one place
    mixed err = catch {
        return low_parse(con, in);
      };

    if (!objectp(err) || !err->buffer_error) {
      // This was not a read out-of-bound error
      throw(err);
    }

    return UNDEFINED;
}

class Frame {
    //!
    FRAME opcode;

    //! Set to @expr{1@} if this a final frame, i.e. the last frame of a
    //! fragmented message or a non-fragmentation frame.
    int(0..1) fin = 1;

    //! Three reserved extension bits.  Binary-and with @expr{RSV1@},
    //! @expr{RSV2@} or @expr{RSV3@} to single out the corresponding
    //! extension.  Typically @expr{RSV1@} is set for compressed frames.
    int rsv;

    //! Set to any of @expr{OVERRIDE_COMPRESS@}
    //! or @expr{OVERRIDE_SKIPCOMPRESS@}
    //! to bypass of the compression heuristics.
    int skip_compression = HEURISTICS_COMPRESS;

    string mask;

    //! Data part of the frame. Valid for frames of type @[FRAME_BINARY],
    //! @[FRAME_PING] and @[FRAME_PONG].
    string data = "";

    //! @decl void create(FRAME opcode, void|string|CLOSE_STATUS)
    //! @decl void create(FRAME_TEXT, string text, void|int(0..1) fin)
    //! @decl void create(FRAME_BINARY, string(0..255) data, void|int(0..1) fin)
    //! @decl void create(FRAME_CONTINUATION, string(0..255) data, void|int(0..1) fin)
    //! @decl void create(FRAME_CLOSE, CLOSE_STATUS reason)
    //! @decl void create(FRAME_PING, string(0..255) data)
    //! @decl void create(FRAME_PONG, string(0..255) data)
    protected void create(FRAME opcode, void|string|CLOSE_STATUS data, void|int(0..1) fin) {
        this::opcode = opcode;
        if (data) switch (opcode) {
        case FRAME_TEXT:
            this::data = string_to_utf8(data);
            this::fin = undefinedp(fin) || fin;
            break;
        case FRAME_PONG:
        case FRAME_PING:
            if (!stringp(data) || String.width(data) != 8)
                error("Bad argument. Expected string(8bit).\n");
            this::data = data;
            break;
        case FRAME_BINARY:
            if (!stringp(data) || String.width(data) != 8)
                error("Bad argument. Expected string(8bit).\n");
            this::fin = undefinedp(fin) || fin;
            this::data = data;
            break;
        case FRAME_CLOSE:
            if (intp(data)) {
                this::data = sprintf("%2c", data);
            } else if (stringp(data) && String.width(data) == 8) {
                this::data = data;
            } else error("Bad argument. Expected CLOSE_STATUS or string(8bit).\n");
            break;
        case FRAME_CONTINUATION:
            if (!stringp(data))
                error("Bad argument. Expected string.\n");
            if (String.width(data) != 8)
                error("%s frames cannot hold widestring data.\n",
                      describe_opcode(opcode));
            this::data = data;
            this::fin = undefinedp(fin) || fin;
            break;
        }
    }

    protected string _sprintf(int type) {
      return type=='O' && sprintf("%O(%s, fin: %d, rsv: %d, %d bytes)",
                       this_program,
                       describe_opcode(opcode), fin, rsv & (RSV1|RSV2|RSV3),
                       sizeof(data));
    }

    private string _text;

    //! @decl string text
    //! Only exists for frames of type @[FRAME_TEXT].

    string `text() {
        if (opcode != FRAME_TEXT) error("Not a text frame.\n");
        if (!_text) _text = utf8_to_string(data);
        return _text;
    }

    string `text=(string s) {
        if (opcode != FRAME_TEXT) error("Not a text frame.\n");
        _text = s;
        data = string_to_utf8(s);
        return s;
    }

    //! @decl CLOSE_STATUS reason
    //! Only exists for frames of type @[FRAME_CLOSE].

    CLOSE_STATUS `reason() {
        int i;
        if (opcode != FRAME_CLOSE)
            error("This is not a close frame.\n");
        if (!sizeof(data)) return CLOSE_NORMAL;
        if (sscanf(data, "%2c", i) == 1) return i;
        return CLOSE_ERROR;
    }

    CLOSE_STATUS `reason=(CLOSE_STATUS r) {
        if (opcode != FRAME_CLOSE)
            error("This is not a close frame.\n");
        data = sprintf("%2c", r);
        return r;
    }

    //! @decl string close_reason

    string `close_reason() {
        if (opcode != FRAME_CLOSE)
            error("This is not a close frame.\n");
        if (sizeof(data) <= 2) return 0;
        return utf8_to_string(data[2..]);
    }

    //!
    void encode(Stdio.Buffer buf) {
        buf->add_int8(fin << 7 | rsv | opcode);

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
    //! The actual client connection.
    Stdio.File|SSL.File stream;

    Stdio.Buffer out = Stdio.Buffer();
    Stdio.Buffer in = Stdio.Buffer()->set_error_mode(1); // Throw errors when we attempt to read out-of-bound data

    protected int buffer_mode = 0;

    //! Remote endpoint URI when we are a client
    protected Standards.URI endpoint;

    //! Extra headers when connecting to a server
    protected mapping(string:string) extra_headers;

    protected mixed id;

    mapping options;

#if constant(Gz.deflate)
    final Gz.inflate uncompress;
    private Gz.deflate compress;
#endif

    final int lastopcode = 0x80;   // FIN
    final int firstrsv;

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

    protected string _sprintf(int type) {
        return sprintf("%O(%d, %O, %s, %s)", this_program, state, stream,
                       endpoint?(string)endpoint:"server",
                       buffer_mode?"buffer mode": "callback mode only");
    }

    //! Set the @expr{id@}. It will be passed as last argument to all
    //! callbacks.
    void set_id(mixed id) {
        this::id = id;
    }

    //! Constructor for server mode
    protected void create(Stdio.File|SSL.File f,
     void|mapping _options) {
        options = .WebSocket.options + (_options || ([]));
        stream = f;
        if (f->set_buffer_mode) {
            f->set_buffer_mode(in, out);
            buffer_mode = 1;
        }
        f->set_nonblocking(websocket_in, websocket_write, websocket_closed);

        state = OPEN;
        if (onopen) onopen(id || this);
        WS_WERR(2, "opened\n");
    }

    //! Constructor for client mode connections
    protected variant void create() {
        options = ([]);
        masking = 1; // Clients must mask data
        state = CLOSED; // We are closed until we have connected and upgraded the connection;
    }

    //! Connect to the remote @[endpoint] with optional request
    //! headers specified in @[headers]. This method will send the
    //! actual HTTP request to switch protocols to the server and once
    //! a HTTP 101 response is returned, switch the connection to
    //! WebSockets and call the @[onopen] callback.
    int connect(string|Standards.URI endpoint, void|mapping(string:string) extra_headers) {
        if (stringp(endpoint)) endpoint = Standards.URI(endpoint);
        this_program::endpoint = endpoint;
        this_program::extra_headers = extra_headers || ([]);

        if (endpoint->path == "") endpoint->path = "/";

        Stdio.File f = Stdio.File();
        state = CONNECTING;

        string host = endpoint->host;
        int port;

        if (endpoint->scheme == "ws") {
            port = endpoint->port || 80;
        } else if (endpoint->scheme == "wss") {
            port = endpoint->port || 443;
        } else error("Not a WebSocket URL.\n");

        if (endpoint->port) host += ":" + endpoint->port;

        int res = f->connect(endpoint->host, port);

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
                WS_WERR(1, "Handshake failed\n");
                websocket_closed();
                return 0;
            }
        } else {
            stream = f;
        }

        buffer_mode = 0;

        stream->set_nonblocking(http_read, websocket_write, websocket_closed);
        mapping headers = ([
            "Host" : host,
            "Connection" : "Upgrade",
            "User-Agent" : "Pike/8.0",
            "Accept": "*/*",
            "Upgrade" : "websocket",
            "Sec-WebSocket-Key" : "x4JJHMbDL1EzLkh9GBhXDw==",
            "Sec-WebSocket-Version": "13",
        ]);

        foreach(this_program::extra_headers; string idx; string val) {
            headers[idx] = val;
        }

        // We use our output buffer to generate the request.
        out->add("GET ", endpoint->get_http_path_query(), " HTTP/1.1\r\n");
        foreach(headers; string h; string v) {
            out->add(h, ": ", v, "\r\n");
        }
        out->add("\r\n");
        return res;
    }

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
        return sizeof(out);
    }

    void send_raw(string(8bit) ... s) {
        WS_WERR(3, "out:\n----\n%s\n----\n", s*"\n----\n");
        out->add(@s);
        stream->write("");
    }


    //! Read HTTP response from remote endpoint and handle connection
    //! upgrade.
    protected void http_read(mixed _id, string data) {

        if (state != CONNECTING) {
          websocket_closed();
          return;
        }

        in->add(data);

        array tmp = in->sscanf("%s\r\n\r\n");
        if (sizeof(tmp)) {
            // We should now have an HTTP response
            array(string) lines = tmp[0]/"\r\n";
            int major, minor;
            int status;
            string status_desc;

            if (sscanf(lines[0], "HTTP/%d.%d %d %s", major, minor, status, status_desc) != 4) {
                websocket_closed();
                return;
            }

            switch(status) {
              case 100..199: break;
              default:
                  websocket_closed();
                  return;
            }

            // FIXME: Parse headers and make them available to onopen?

            if (endpoint->scheme != "wss") {
                stream->set_buffer_mode(in, out);
                buffer_mode = 1;
            }

            stream->set_nonblocking(websocket_in, websocket_write, websocket_closed);

            state = OPEN;
            if (onopen) onopen(id || this);
            WS_WERR(2, "opened\n");

            // Is this needed?
            websocket_in(_id, in);
        }
    }

    //! Write callback in non-buffer mode
    protected void websocket_write() {
        if (buffer_mode) return;
        if (sizeof(out)) {
            int n = out->output_to(stream);
            if (n < 0) {
                int e = errno();
                if (e) {
                    websocket_closed();
                }
            }
        }
    }

    //! Read callback in non-buffer mode.
    protected void websocket_in(mixed _id, string data) {
      in->add(data);
      websocket_in(_id, in);
    }

    //! Read callback in buffer mode
    protected variant void websocket_in(mixed _id, Stdio.Buffer in) {
        // it would be nicer to set the read callback to zero
        // once a close handshake has been received. however,
        // without a read callback pike does not trigger the
        // close event.

        while (Frame frame = parse(this, in)) {
            if (state == CLOSED) return; 

            int opcode = frame->opcode;
            WS_WERR(2, "%O in %O\n", this, frame);

            if (opcode & 0x8) {
                /* control frames may not be bigger than 125 bytes */
                if (sizeof(frame->data) > 125) {
                    WS_WERR(1, "Received too big control frame. closing connection.\n");
                    fail();
                    return;
                }
                if (!frame->fin) {
                    WS_WERR(1, "Received fragmented control frame. closing connection.\n");
                    fail();
                    return;
                }
                if (opcode > FRAME_PONG) {
                    WS_WERR(1, "Received unknown control frame. closing connection.\n");
                    fail();
                    return;
                }
            }

#ifdef WEBSOCKET_DEBUG
            if (opcode >= 0x3 && opcode <= 0x7) {
                WS_WERR(1, "Received reserved non control opcode frame.\n");
                fail();
                return;
            }
#endif

            switch (opcode) {
            case FRAME_TEXT:
                /* check if the incoming text is valid utf8 */
                if (frame->fin && catch(frame->text)) {
                    fail(CLOSE_BAD_DATA);
                    return;
                }
                break;
            case FRAME_PING:
                send(Frame(FRAME_PONG, frame->data));
                continue;
            case FRAME_CLOSE:
                if (!is_valid_close(frame->reason)) {
                    WS_WERR(1, "Received invalid close reason: %d\n", frame->reason);
                    fail();
                    return;
                }
                if (state == OPEN) {
#ifdef WEBSOCKET_DEBUG
                    if (catch(frame->close_reason)) {
                        WS_WERR(1, "Non utf8 text in close frame.\n");
                        fail(CLOSE_BAD_DATA);
                        return;
                    }
#endif
                    close(frame->reason);
                    // we call close_event here early to allow applications to stop
                    // sending packets. i think this makes more sense than what the
                    // websocket api specification proposes.
                    close_event(frame->reason);
                } else if (state == CLOSING) {
                    destruct(stream);
                    // we dont use frame->reason here, since that is not guaranteed
                    // to be the same as the one used to start the close handshake
                    close_event(close_reason);
                }
                return;
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
        if (stream) destruct(stream);
        // if this is the end of a proper close handshake, this wont do anything
        close_event(0);
        WS_WERR(2, "closed\n");
    }

    //! Send a WebSocket ping frame.
    void ping(void|string s) {
        send(Frame(FRAME_PING, s));
    }

    //! Send a WebSocket connection close frame. The close callback will be
    //! called when the close handshake has been completed. No more frames
    //! can be sent after initiating the close handshake.
    void close(void|string(8bit)|CLOSE_STATUS reason, void|string msg) {
        if (!reason) reason = CLOSE_NORMAL;
        if (msg) {
            send(Frame(FRAME_CLOSE, sprintf("%2c%s", reason, string_to_utf8(msg))));
        } else send(Frame(FRAME_CLOSE, reason));
    }

    //! Send a WebSocket connection close frame and terminate the connection.
    //! The default @expr{reason@} is @[CLOSE_ERROR].
    void fail(void|CLOSE_STATUS reason) {
        if (!reason) reason = CLOSE_ERROR;
        close(reason);
        close_event(reason);
        destruct(stream);
    }

    //! Send a WebSocket frame.
    void send(Frame frame) {
        if (state != OPEN)
            error("WebSocket connection is not open: %O.\n", this);
        if (sizeof(frame->data)) {
            if (masking)
                frame->mask = random_string(4);
#if constant(Gz.deflate)
            if (options->compressionLevel
             && frame->compress != OVERRIDE_SKIPCOMPRESS
             && sizeof(frame->data) >=
                 (options->compressionNoContextTakeover
                  ? options->compressionThresholdNoContext
                  : options->compressionThreshold)) {
                if (!compress)
                    compress = Gz.deflate(-options->compressionLevel,
                     options->compressionStrategy,
                     options->compressionWindowSize);
                int wsize = options->compressionWindowSize
                 ? 1<<options->compressionWindowSize : 1<<15;
                if (options->compressionNoContextTakeover) {
                    string s
                     = compress.deflate(frame->data, Gz.SYNC_FLUSH)[..<4];
                    if (sizeof(s) < sizeof(frame->data)) {
                        frame->data = s;
                        frame->rsv |= RSV1;
                    }
                    compress = 0;
                } else {
                    if (frame->compress == OVERRIDE_COMPRESS
                     || frame->opcode == FRAME_TEXT) {
                        // Assume text frames are always compressible.
                        frame->data
                         = compress.deflate(frame->data, Gz.SYNC_FLUSH)[..<4];
                        frame->rsv |= RSV1;
                    } else if (4*sizeof(frame->data) <= wsize) {
                        // If a binary frame is smaller than 25% of the
                        // LZ77 window size, test if adding it to the
                        // stream results in zero overhead.  If so, add it,
                        // if not, reset compression state to before adding it.
                        Gz.inflate save = compress.clone();
                        string s
                         = compress.deflate(frame->data, Gz.SYNC_FLUSH);
                        if (sizeof(s) < sizeof(frame->data)) {
                            frame->data = s[..<4];
                            frame->rsv |= RSV1;
                        } else
                          compress = save;
                    } else {
                        // Large binary frames we sample the first 1KB of.
                        // If it compresses better than 6.25%, add them
                        // to the compressed stream.
                        Gz.inflate ctest = compress.clone();
                        string sold = frame->data[..1023];
                        string s = ctest.deflate(sold, Gz.PARTIAL_FLUSH);
                        if (sizeof(s) + 64 < sizeof(sold)) {
                            frame->data = compress.deflate(frame->data,
                             Gz.SYNC_FLUSH)[..<4];
                            frame->rsv |= RSV1;
                        }
                    }
                }
            }
#endif
        }
        WS_WERR(2, "sending %O\n", frame);
        frame->encode(out);
        stream->write("");
        if (frame->opcode == FRAME_CLOSE) {
            state = CLOSING;
            close_reason = frame->reason;
            stream->close("w");
        }
    }

    //! Send a WebSocket text frame.
    void send_text(string s) {
        send(Frame(FRAME_TEXT, s));
    }

    void send_continuation(string(8bit) data, void|int(0..1) fin) {
        send(Frame(FRAME_CONTINUATION, data, fin));
    }

    //! Send a WebSocket binary frame.
    void send_binary(string(0..255) s) {
        send(Frame(FRAME_BINARY, s));
    }

}

//!
class Request(function(array(string), Request:void) cb) {
    inherit Protocols.HTTP.Server.Request;

    protected int alternatecallback() {
      if (cb && has_index(request_headers, "sec-websocket-key")) {
          string proto = request_headers["sec-websocket-protocol"];
          array(string) protocols =  proto ? proto / ", " : ({});
          WS_WERR(3, "websocket request: %O\n", protocols);
          cb(protocols, this);
          return 1;
      }
      return 0;
    }

    protected int parse_variables() {
	return ::parse_variables(
          has_index(request_headers, "sec-websocket-key"));
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
	    "Sec-Websocket-Accept" : MIME.encode_base64(Crypto.SHA1.hash(s)),
            "Sec-Websocket-Version" : "13",
	]);

        array parse_extensions() {
          string exs;
          array retval;
          if (exs = request_headers["sec-websocket-extensions"]) {
            // Parses extensions conforming RFCs, supports quoted values.
            // FIXME Violates the RFC when commas or semicolons are quoted.
            array v;
            int i;
            retval = array_sscanf(exs,
             "%*[ \t\r\n]%{%{%[^ \t\r\n=;,]%*[= \t\r\n]%[^;,]%*[ \t\r\n;]%}"
             "%*[ \t\r\n,]%}")[0];
            foreach (retval; i; v) {
              mapping m;
              array d;
              v = v[0];
              retval[i] = ({v[0][0], m = ([])});
              v = v[1..];
              foreach (v;; d) {
                string sv = String.trim_whites(d[1]);
                if (sizeof(sv) && sv[0] == '"')
                  sv = sv[1..<1];	    // Strip doublequotes
                int|float|string tv;	    // Store numeric values natively
                if ((string)(tv=(int)sv)!=sv && (string)(tv=(float)sv)!=sv)
                  tv = sv;
                m[d[0]] = tv;
              }
            }
          } else
            retval = ({});
          return retval;
        };

        mapping rext = ([]);

        // Code has been tuned for serverside.  If used as clientside,
        // it will need tweaking.
        foreach (parse_extensions();; array ext) {
            string name = ext[0];
            mapping parm = ext[1], rparm = ([]);
            int|float|string p;
            switch (name) {
#if constant(Gz.deflate)
                case "permessage-deflate":
                    if (rext[name])
                        continue;
                    if (parm->client_no_context_takeover
                     || options->decompressionNoContextTakeover) {
                        options->decompressionNoContextTakeover = 1;
                        rparm->client_no_context_takeover = "";
                    }
                    if (stringp(p = parm->client_max_window_bits)) {
                        if ((p = options->decompressionWindowSize) < 15)
                            rparm->client_max_window_bits = p;
                    } else if (!zero_type(p)) {
                        p = min(p, options->decompressionWindowSize);
                        options->decompressionWindowSize
                         = max(rparm->client_max_window_bits = p, 8);
                    }
                    if (parm->server_no_context_takeover
                     || options->compressionNoContextTakeover) {
                        options->compressionNoContextTakeover = 1;
                        rparm->server_no_context_takeover = "";
                    }
                    if (stringp(p = parm->server_max_window_bits)) {
                        if ((p = options->compressionWindowSize) < 15)
                        rparm->server_max_window_bits = p;
                    } else if (!zero_type(p)) {
                        p = min(p, options->compressionWindowSize);
                        if (p < 8)
                            continue;
                        options->compressionWindowSize
                         = rparm->server_max_window_bits = p;
                    }
                    rext[name] = rparm;
                    break;
#endif
            }
        }

        if (sizeof(rext)) {
            if (!rext["permessage-deflate"])
                m_delete(options, "compressionLevel");
            array ev = ({});
            foreach (rext; string name; mapping ext) {
                array res = ({name});
                foreach (ext; string pname; int|float|string pval) {
                    // FIXME We only look for embedded spaces to decide if
                    // we need to quote the parametervalue.  If you want to
                    // embed tabs or other whitespace, this needs to be
                    // amended.
                    if (stringp(pval) && has_value(pval, " "))
                        pval = "\"" + pval + "\"";
                    pval = (string)pval;
                    if (sizeof(pval))
                        pval = "="+pval;
                    res += ({pname+pval});
                }
                ev += ({res * ";"});
            }
            heads["Sec-WebSocket-Extensions"] = ev * ",";
        } else
            m_delete(options, "compressionLevel");

	if (protocol) heads["Sec-Websocket-Protocol"] = protocol;

        Connection ws = Connection(my_fd);
        my_fd = 0;

        ws->send_raw("HTTP/1.1 101 SwitchingProtocols\r\n");

        foreach (heads; string k; string v) {
          ws->send_raw(sprintf("%s: %s\r\n", k, v));
        }

        ws->send_raw("\r\n");

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
                          void|string key, void|string|array certificate) {

        ::create(http_cb, portno, interface, key, certificate);

        if (ws_cb)
            request_program = Function.curry(Request)(ws_cb);
    }
}

//! Returns a new callback which recombines all fragmented frames before
//! passing them to @expr{cb@}.
message_callback defragment(message_callback cb, Connection connection) {
    Frame fragment;
    void onmessage(Frame frame, mixed id) {
        int opcode = frame->opcode;
        int(0..1) fin = frame->fin;

        if (opcode == FRAME_CONTINUATION) {
            if (!fragment) {
                connection->fail();
                WS_WERR(1, "Bad continuation.\n");
                return;
            }
            fragment->data += frame->data;

            if (fin) {
                frame = fragment;
                frame->fin = 1;
                fragment = 0;
                if (frame->opcode == FRAME_TEXT && catch(frame->text)) {
                    connection->fail(CLOSE_BAD_DATA);
                    WS_WERR(1, "Invalid utf8 in text frame.\n");
                    return;
                }
            } else return;
        } else if (!fin) {
            if (fragment) {
                connection->fail();
                WS_WERR(1, "Unfinished fragmented message.\n");
                return;
            }
            fragment = frame;
            return;
        } else if (fragment && !(opcode & 0x8)) {
            connection->fail();
            WS_WERR(1, "Non control frame during fragmented traffic.\n");
            return;
        }

        cb(frame, id);
    };

    return onmessage;
}
