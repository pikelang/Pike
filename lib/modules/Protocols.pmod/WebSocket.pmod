#pike __REAL_VERSION__

#ifdef WEBSOCKET_DEBUG
# define WS_WERR(level, x...)  do { if (WEBSOCKET_DEBUG >= level) { werror("%O: ", this); werror(x); } } while(0)
#else
# define WS_WERR(level, x...)
#endif

constant websocket_id = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
constant websocket_version = 13;

//! This module implements the WebSocket protocol as described in
//! @rfc{6455@}.

constant MASK = _Roxen.websocket_mask;

private constant agent = sprintf("Pike/%d.%d", __MAJOR__, __MINOR__);

//! 
typedef function(Frame, mixed:void) message_callback;

function curry_back(function cb, mixed ... extra_args) {
    void f(mixed ... args) {
        cb(@args, @extra_args);
    };

    return f;
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

};

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

mapping(string:mapping) parse_websocket_extensions(string header) {
  mapping(string:mapping) retval = ([]);
  if (!header) return retval;
  // Parses extensions conforming RFCs, supports quoted values.
  // FIXME Violates the RFC when commas or semicolons are quoted.
  array tmp = array_sscanf(header,
   "%*[ \t\r\n]%{%{%[^ \t\r\n=;,]%*[= \t\r\n]%[^;,]%*[ \t\r\n;]%}"
   "%*[ \t\r\n,]%}")[0];
  foreach (tmp; int i; array v) {
    mapping m = ([]);
    array d;
    v = v[0];
    retval[v[0][0]] = m;
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
  return retval;
}

string encode_websocket_extensions(mapping(string:mapping) ext) {
    array ev = ({});
    foreach (ext; string name; mapping ext) {
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
    return ev * ",";
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

    if (masked) {
        data = MASK(data, mask);
    }

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
    //! Type of frame eg @expr{FRAME_TEXT@} or @expr{FRAME_BINARY@}
    FRAME opcode;

    //! Set to @expr{1@} if this a final frame, i.e. the last frame of a
    //! fragmented message or a non-fragmentation frame.
    int(0..1) fin = 1;

    //! Three reserved extension bits.  Binary-and with @expr{RSV1@},
    //! @expr{RSV2@} or @expr{RSV3@} to single out the corresponding
    //! extension.  Typically @expr{RSV1@} is set for compressed frames.
    int rsv;

    //! Generic options for this frame.
    mapping(string:mixed) options;

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

    protected array(object) extensions;

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
    protected void create(Stdio.File|SSL.File f, void|int|array(object) extensions) {
        if (arrayp(extensions)) this_program::extensions = extensions;
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
        masking = 1; // Clients must mask data
        state = CLOSED; // We are closed until we have connected and upgraded the connection;
    }

    protected array(mapping) low_connect(Standards.URI endpoint,
					 mapping(string:string) extra_headers,
					 void|array extensions)
    {
      string host = endpoint->host;

      if (endpoint->port) host += ":" + endpoint->port;

      mapping headers = ([
	"Host" : host,
	"Connection" : "Upgrade",
	"User-Agent" : agent,
	"Accept": "*/*",
	"Upgrade" : "websocket",
	"Sec-WebSocket-Key" :
	  MIME.encode_base64(Crypto.Random.random_string(16), 1),
	"Sec-WebSocket-Version": (string)websocket_version,
      ]);

      foreach(extra_headers; string idx; string val) {
	headers[idx] = val;
      }

      expected_accept =
	MIME.encode_base64(Crypto.SHA1.hash(headers["Sec-WebSocket-Key"] +
					    websocket_id));

      mapping rext;

      if (arrayp(extensions)) {
	rext = ([]);

	foreach (extensions; int i; extension_factory f) {
	  mixed o = f(1, 0, rext);
	  if (objectp(o)) extensions[i] = o;
	}

	if (sizeof(rext))
	  headers["Sec-WebSocket-Extensions"] = encode_websocket_extensions(rext);
      }

      return ({ headers, rext });
    }

    //! Connect to the remote @[endpoint] with optional request
    //! headers specified in @[headers]. This method will send the
    //! actual HTTP request to switch protocols to the server and once
    //! a HTTP 101 response is returned, switch the connection to
    //! WebSockets and call the @[onopen] callback.
    int connect(string|Standards.URI endpoint, void|mapping(string:string) extra_headers,
                void|array extensions) {
        if (stringp(endpoint)) endpoint = Standards.URI(endpoint);
        this_program::endpoint = endpoint;
        this_program::extra_headers = extra_headers = extra_headers || ([]);

        if (endpoint->path == "") endpoint->path = "/";

        Stdio.File f = Stdio.File();
        state = CONNECTING;

        int port;

        if (endpoint->scheme == "ws") {
            port = endpoint->port || 80;
        } else if (endpoint->scheme == "wss") {
            port = endpoint->port || 443;
        } else error("Not a WebSocket URL.\n");

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

        if (arrayp(extensions)) {
	    // NB: extensions is altered destructively by low_connect().
            extensions = extensions + ({ });
	}

	[mapping headers, mapping rext] =
	  low_connect(endpoint, extra_headers, extensions);

        stream->set_nonblocking(curry_back(http_read, _Roxen.HeaderParser(), extensions, rext),
                                websocket_write, websocket_closed);


        // We use our output buffer to generate the request.
        send_raw("GET ", endpoint->get_http_path_query(), " HTTP/1.1\r\n");
        foreach(headers; string h; string v) {
            send_raw(h, ": ", v, "\r\n");
        }
        send_raw("\r\n");
        return res;
    }

    //!
    function(mixed,void|mixed:void) onopen;

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

    protected string expected_accept;

    //! Read HTTP response from remote endpoint and handle connection
    //! upgrade.
    protected void http_read(mixed _id, string data,
                             object hp, array(extension_factory) extensions, mapping rext) {

        if (state != CONNECTING) {
          websocket_closed();
          return;
        }

        array tmp = hp->feed(data);

        if (tmp) {
            int major, minor;
            int status;
            mapping headers = tmp[2];
            string status_desc;

	    WS_WERR(2, "http_read: header done. Parsed: %O\n", tmp);

	    // RFC 6455 4.1.(3)

	    // 1:
	    //   If the status code received from the server is not 101, the
            //   client handles the response per HTTP [RFC2616] procedures.
            //   In particular, the client might perform authentication if it
            //   receives a 401 status code; the server might redirect the
            //   client using a 3xx status code (but clients are not required
            //   to follow them), etc.

            if (sscanf(tmp[1], "HTTP/%d.%d %d %s",
		       major, minor, status, status_desc) != 4) {
                websocket_closed();
                return;
            }

            if (status != 101) {
	        WS_WERR(1, "http_read: Bad http status code: %d.\n", status);
	        websocket_closed();
	        return;
            }

	    // 2:
	    //   If the response lacks an |Upgrade| header field or the
	    //   |Upgrade| header field contains a value that is not an
	    //   ASCII case-insensitive match for the value "websocket",
	    //   the client MUST _Fail the WebSocket Connection_.

	    if (lower_case(headers["upgrade"] || "") != "websocket") {
	      WS_WERR(1, "http_read: No upgrade header.\n");
	      websocket_closed();
	      return;
	    }

	    // 3:
	    //   If the response lacks a |Connection| header field or the
            //   |Connection| header field doesn't contain a token that is an
            //   ASCII case-insensitive match for the value "Upgrade", the
	    //   client MUST _Fail the WebSocket Connection_.

	    if (lower_case(headers["connection"] || "") != "upgrade") {
	      WS_WERR(1, "http_read: No connection header with upgrade.\n");
	      websocket_closed();
	      return;
	    }

	    // 4:
	    //   If the response lacks a |Sec-WebSocket-Accept| header field
            //   or the |Sec-WebSocket-Accept| contains a value other than
            //   the base64-encoded SHA-1 of the concatenation of the
	    //   |Sec-WebSocket-Key| (as a string, not base64-decoded) with
	    //   the string "258EAFA5-E914-47DA-95CA-C5AB0DC85B11" but
	    //   ignoring any leading and trailing whitespace, the client
	    //   MUST _Fail the WebSocket Connection_.

	    if (headers["sec-websocket-accept"] != expected_accept) {
	      WS_WERR(1, "http_read: Missing or invalid Sec-WebSocket-Accept.\n");
	      websocket_closed();
	      return;
	    }

	    // NB: The RFC does not have a requirement for the server to
	    //     send a Sec-WebSocket-Version header, unless it doesn't
	    //     support the version that the client requested. It seems
	    //     prudent to require the header (if any) to contain the
	    //     version we requested.

	    if (!has_value((array(int))((headers["sec-websocket-version"] ||
					 (string)websocket_version)/","),
			   websocket_version)) {
	      WS_WERR(1, "http_read: Unsupported Sec-WebSocket-Version: %O.\n",
		      headers["sec-websocket-version"]);
	      websocket_closed();
	      return;
	    }

            if (arrayp(extensions)) {
                mapping ext = parse_websocket_extensions(headers["sec-websocket-extensions"]);
                array tmp = ({ });

                /* we finish the extension negotiation */
                foreach (extensions; int i; object|extension_factory f) {
                  if (!objectp(f))
                      extensions[i] = f(1, ext, rext);
                }

                extensions = filter(extensions, objectp);

                if (sizeof(extensions)) this_program::extensions = extensions;
            }

            if (endpoint->scheme != "wss") {
                stream->set_buffer_mode(in, out);
                buffer_mode = 1;
            }

            stream->set_nonblocking(websocket_in, websocket_write, websocket_closed);

            state = OPEN;
            if (onopen) onopen(id || this, headers);
            WS_WERR(2, "opened\n");

            if (sizeof(tmp[0])) {
                in->add(tmp[0]);
                websocket_in(_id, in);
            }

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

        FRAMES: while (Frame frame = parse(this, in)) {
            if (state == CLOSED) return;

            if (extensions) foreach (extensions;; object e) {
                if (e->receive) {
                  frame = e->receive(frame, this);
                  if (!frame) continue FRAMES;
                }
            }

            int opcode = frame->opcode;
            WS_WERR(2, "%O in %O\n", this, frame);

            switch (opcode) {
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
                    if (catch(frame->close_reason)) {
                        WS_WERR(1, "Non utf8 text in close frame.\n");
                        fail(CLOSE_BAD_DATA);
                        return;
                    }
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
        int opcode = frame->opcode;
        if (state != OPEN)
            error("WebSocket connection is not open: %O.\n", this);
        if (extensions) foreach (extensions;; object e) {
            if (e->send) {
              frame = e->send(frame, this);
              if (!frame) return;
            }
        }
        /* NOTE: the mask always needs to be used, even for
         * empty frames */
        if (masking) frame->mask = random_string(4);
        WS_WERR(2, "sending %O\n", frame);
        frame->encode(out);
        stream->write("");
        if (opcode == FRAME_CLOSE) {
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

    protected int parse_variables() {
        WS_WERR(2, "parse_variables: headers: %O\n", request_headers);
        WS_WERR(2, "parse_variables: query: %O\n", query);
        WS_WERR(2, "parse_variables: variables: %O\n", variables);

	// RFC 6455 4.1.(2)
	//
	// 2:
	//   The method of the request MUST be GET, and the HTTP version MUST
        //   be at least 1.1.
	if ((request_type != "GET") || !has_prefix(protocol, "HTTP/") ||
	    (protocol[sizeof("HTTP/")..] < "1.1")) {
	  WS_WERR(1, "parse_variables: Not a websocket request (2).\n");
	  return ::parse_variables();
	}

	// 5:
	//   The request MUST contain an |Upgrade| header field whose value
        //   MUST include the "websocket" keyword.

	if (!has_value(lower_case(request_headers["upgrade"] || ""),
		       "websocket")) {
	  WS_WERR(1, "parse_variables: Not a websocket request (5).\n");
	  return ::parse_variables();
	}

	// 6:
	//   The request MUST contain a |Connection| header field whose value
        //   MUST include the "Upgrade" token.

	if (!has_value(lower_case(request_headers["connection"] || ""),
		       "upgrade")) {
	  WS_WERR(1, "parse_variables: Not a websocket request (6).\n");
	  return ::parse_variables();
	}

	// 7:
	//   The request MUST include a header field with the name
        //   |Sec-WebSocket-Key|.  The value of this header field MUST be a
        //   nonce consisting of a randomly selected 16-byte value that has
        //   been base64-encoded (see Section 4 of [RFC4648]).  The nonce
        //   MUST be selected randomly for each connection.
	string raw_key;
	catch {
	  raw_key = MIME.decode_base64(request_headers["sec-websocket-key"]);
	};
	if (!raw_key || (sizeof(raw_key) != 16)) {
	    WS_WERR(1, "parse_variables: Not a websocket request (7).\n");
	    return ::parse_variables();
	}

	// 9:
	//   The request MUST include a header field with the name
        //   |Sec-WebSocket-Version|.  The value of this header field MUST be
        //   13.

	if (request_headers["sec-websocket-version"] !=
	    (string)websocket_version) {
	  WS_WERR(1, "parse_variables: Not a websocket request (9).\n");
	  return ::parse_variables();
	}

	if (query!="")
	    .HTTP.Server.http_decode_urlencoded_query(query,variables);
	flatten_headers();
	string proto = request_headers["sec-websocket-protocol"];
	array(string) protocols =  proto ? proto / ", " : ({});
	WS_WERR(1, "websocket request: %O\n", protocols);
	if (cb) {
	    cb(protocols, this);
	}
	return 0;
    }

    array(mapping(string:string)|array)
        low_websocket_accept(string|void protocol,
			     array(extension_factory)|void extensions,
			     mapping(string:string)|void extra_headers)
    {
	string s = request_headers["sec-websocket-key"] + websocket_id;
	mapping heads = ([
	    "Upgrade" : "websocket",
	    "Connection" : "Upgrade",
	    "Sec-WebSocket-Accept" : MIME.encode_base64(Crypto.SHA1.hash(s)),
            "Sec-WebSocket-Version" : (string)websocket_version,
            "Server" : agent,
	]);

        if (extra_headers) heads += extra_headers;

        array _extensions;
        mapping rext = ([]);

        if (extensions && sizeof(extensions)) {
            mapping ext = parse_websocket_extensions(request_headers["sec-websocket-extensions"]);
            array tmp = ({ });

            foreach (extensions;; extension_factory f) {
              object e = f(0, ext, rext);
              if (e) tmp += ({ e });

            }

            if (sizeof(tmp)) _extensions = tmp;
            if (sizeof(rext))
                heads["Sec-WebSocket-Extensions"] = encode_websocket_extensions(rext);
        }

	if (protocol) heads["Sec-Websocket-Protocol"] = protocol;

	return ({ heads, _extensions });
    }

    //! Calling @[websocket_accept] completes the WebSocket connection
    //! handshake.
    //! The @expr{protocol@} parameter should be either @expr{0@} or one of
    //! the protocols advertised by the client when initiating the WebSocket connection.
    //! Extensions can be specified using the @expr{extensions@}
    //! parameter.
    //! Additional HTTP headers in the response can be specified using the @expr{extra_headers@}
    //! argument.
    //!
    //! The returned connection object is in state @[Connection.OPEN].
    Connection websocket_accept(string protocol, void|array(extension_factory) extensions,
                                void|mapping extra_headers) {
        [mapping heads, array _extensions] =
	    low_websocket_accept(protocol, extensions, extra_headers);

        Connection ws = Connection(my_fd, _extensions);
        WS_WERR(2, "Using extensions: %O\n", _extensions);
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

    //!
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

//! Extension factories are used during connection negotiation to create the
//! @[Extension] objects for a @[Connection].
typedef function(int(0..1),mapping,mapping:object)|program extension_factory;

//! Base class for extensions.
class Extension {

    //!
    Frame receive(Frame, Connection con);

    //!
    Frame send(Frame, Connection con);
}

//! Simple extension which automatically recombines fragmented messages.
class defragment {
    inherit Extension;

    private Frame fragment;

    Frame receive(Frame frame, Connection con) {
        int opcode = frame->opcode;
        int(0..1) fin = frame->fin;

        if (opcode == FRAME_CONTINUATION) {
            if (!fragment) {
                con->fail();
                WS_WERR(1, "Bad continuation.\n");
                return 0;
            }
            fragment->data += frame->data;

            if (fin) {
                frame = fragment;
                frame->fin = 1;
                fragment = 0;
            } else return 0;
        } else if (!fin) {
            if (opcode != FRAME_TEXT && opcode != FRAME_BINARY) {
                WS_WERR(1, "Received fragmented control frame. closing connection.\n");
                con->fail();
                return 0;
            }
            if (fragment) {
                con->fail();
                WS_WERR(1, "Unfinished fragmented message.\n");
                return 0;
            }
            fragment = frame;
            return 0;
        } else if (fragment && !(opcode & 0x8)) {
            con->fail();
            WS_WERR(1, "Non control frame during fragmented traffic.\n");
            return 0;
        }

        return frame;
    }
}

#if constant(Gz.deflate)
class _permessagedeflate {
    inherit defragment;

    protected Gz.inflate uncompress;
    protected Gz.deflate compress;

    mapping options;

    protected void create(mapping options) {
        this_program::options = options;
    }

    private void try_compress(Frame frame) {
        return;
        mapping(string:mixed) opts = options;
        if (sizeof(frame->data) >=
             (opts->compressionNoContextTakeover
              ? opts->compressionThresholdNoContext
              : opts->compressionThreshold)) {
            if (!compress)
                compress = Gz.deflate(-opts->compressionLevel,
                 opts->compressionStrategy,
                 opts->compressionWindowSize);
            int wsize = opts->compressionWindowSize
             ? 1<<opts->compressionWindowSize : 1<<15;
            if (opts->compressionNoContextTakeover) {
                string s
                 = compress->deflate(frame->data, Gz.SYNC_FLUSH)[..<4];
                if (sizeof(s) < sizeof(frame->data)) {
                    frame->data = s;
                    frame->rsv |= RSV1;
                }
                compress = 0;
            } else {
                if (opts->compressionHeuristics == OVERRIDE_COMPRESS
                 || frame->opcode == FRAME_TEXT) {
                    // Assume text frames are always compressible.
                    frame->data
                     = compress->deflate(frame->data, Gz.SYNC_FLUSH)[..<4];
                    frame->rsv |= RSV1;
                } else if (4*sizeof(frame->data) <= wsize) {
                    // If a binary frame is smaller than 25% of the
                    // LZ77 window size, test if adding it to the
                    // stream results in zero overhead.  If so, add it,
                    // if not, reset compression state to before adding it.
                    Gz.inflate save = compress->clone();
                    string s
                     = compress->deflate(frame->data, Gz.SYNC_FLUSH);
                    if (sizeof(s) < sizeof(frame->data)) {
                        frame->data = s[..<4];
                        frame->rsv |= RSV1;
                    } else
                      compress = save;
                } else {
                    // Large binary frames we sample the first 1KB of.
                    // If it compresses better than 6.25%, add them
                    // to the compressed stream.
                    Gz.inflate ctest = compress->clone();
                    string sold = frame->data[..1023];
                    string s = ctest->deflate(sold, Gz.PARTIAL_FLUSH);
                    if (sizeof(s) + 64 < sizeof(sold)) {
                        frame->data = compress->deflate(frame->data,
                         Gz.SYNC_FLUSH)[..<4];
                        frame->rsv |= RSV1;
                    }
                }
            }
        }
    }

    Frame send(Frame frame, Connection con) {
        int opcode = frame->opcode;

        if (opcode == FRAME_TEXT || opcode == FRAME_BINARY) try_compress(frame);

        return frame;
    }

    Frame receive(Frame frame, Connection con) {
        frame = ::receive(frame, con);

        if (!frame) return 0;

        int opcode = frame->opcode;
        int rsv1 = frame->rsv & RSV1;

        if (rsv1) {
            if (opcode != FRAME_BINARY && opcode != FRAME_TEXT) {
                con->fail(CLOSE_EXTENSION);
                WS_WERR(1, "Received compressed non-data frame.\n");
                return 0;
            }

            if (!options->compressionLevel) {
                con->fail(CLOSE_EXTENSION);
                WS_WERR(1, "Unexpected compressed frame.\n");
                return 0;
            }

            frame->rsv &= ~RSV1;

            if (!uncompress) uncompress = Gz.inflate(-options->decompressionWindowSize);
            if (mixed err = catch(frame->data = uncompress->inflate(frame->data + "\0\0\377\377"))) {
                con->fail(CLOSE_EXTENSION);
                master()->handle_error(err);
                return 0;
            }
        }

        return frame;
    }
}

//! Global default options for all WebSocket connections using the @expr{permessage-deflate@} extension.
//!
//! @seealso
//!     @[permessagedeflate]
constant deflate_default_options = ([
    "compressionLevel":3,
    "compressionThreshold":5,
    "compressionThresholdNoContext":256,
    "compressionStrategy":Gz.DEFAULT_STRATEGY,
    "compressionWindowSize":15,
    "decompressionWindowSize":15,
    "compressionHeuristics":HEURISTICS_COMPRESS,
]);
#endif

//! Returns an extension factory which implements the @expr{permessage-deflate@} WebSocket extension.
//! It uses the default options from @[deflate_default_options]. Due to how the
//! @expr{permessage-deflate@} extension works, defragmented frames will be recombined
//! automatically.
//!
//! @note
//!     If the @expr{permessage-deflate@} extension is not being used, it falls back to use
//!     @[defragment].
object permessagedeflate(void|mapping default_options) {
#if constant(Gz.deflate)
  default_options = deflate_default_options + (default_options||([]));

  object factory(int(0..1) client_mode, mapping ext, mapping rext) {

    if (client_mode && !ext) {
        /* this is the first step, we just offer the extension without any
         * parameters */
        rext["permessage-deflate"] = ([]);
        return 0;
    }

    mapping parm = ext["permessage-deflate"];

    // this extension was not offered by the client
    if (!parm) return defragment();

    mapping options = default_options + ([]);

    if (!client_mode) {
        mapping rparm = ([]);

        mixed p;

        if (parm->client_no_context_takeover
         || options->decompressionNoContextTakeover) {
            options->decompressionNoContextTakeover = 1;
            rparm->client_no_context_takeover = "";
        }
        if (stringp(p = parm->client_max_window_bits)) {
            if ((p = options->decompressionWindowSize) < 15 && p > 8)
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
            if (p >= 8)
              options->compressionWindowSize
               = rparm->server_max_window_bits = p;
        }

        rext["permessage-deflate"] = rparm;
    }

    return _permessagedeflate(options);
  };
#else
  object factory() {
    return defragment();
  }
#endif

  return factory;
}

//! An extension which performs extra conformance checks on incoming WebSocket frames. It can be used
//! either for testing or in situations where a very strict protocol interpretation is necessary.
class conformance_check {
    inherit Extension;

    Frame receive(Frame frame, Connection con) {
        int opcode = frame->opcode;

        if (opcode == FRAME_TEXT && catch(frame->text)) {
            con->fail(CLOSE_BAD_DATA);
            WS_WERR(1, "Invalid utf8 in text frame.\n");
            return 0;
        }

        if (frame->rsv & (RSV1|RSV2|RSV3)) {
            con->fail(CLOSE_EXTENSION);
            WS_WERR(1, "Unexpected rsv bits.\n");
            return 0;
        }

        if (opcode & 0x8) {
            /* control frames may not be bigger than 125 bytes */
            if (sizeof(frame->data) > 125) {
                WS_WERR(1, "Received too big control frame. closing connection.\n");
                con->fail();
                return 0;
            }
            if (opcode > FRAME_PONG) {
                WS_WERR(1, "Received unknown control frame. closing connection.\n");
                con->fail();
                return 0;
            }
        }
        if (opcode >= 0x3 && opcode <= 0x7) {
            WS_WERR(1, "Received reserved non control opcode frame.\n");
            con->fail();
            return 0;
        }

        return frame;
    }
}
