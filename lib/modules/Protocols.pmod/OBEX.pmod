 
#pike __REAL_VERSION__

//! The IrDA® Object Exchange Protocol.
//! OBEX is a protocol for sending and receiving binary objects
//! to mobile devices using transports such as IR and Bluetooth.

//! A request opcode, for use with the @[client.do_request()] function.
enum Request {
  REQ_CONNECT = 0x80,		//! Establish a new OBEX connection
  REQ_DISCONNECT = 0x81,	//! Terminate an OBEX connection
  REQ_PUT = 0x02,		//! Send an object to the mobile device
  REQ_GET = 0x03,		//! Receive an object from the mobile devuce
  REQ_SETPATH = 0x85,		//! Change the working directory
  REQ_SESSION = 0x87,		//! Manage a session
  REQ_ABORT = 0xff,		//! Abort the request currently being processed

  //! For @[REQ_PUT] and @[REQ_GET] requests, REQ_FINAL must be set
  //! for the request block containing the last portion of the headers.
  //! Other requests must be sent as a single block and have the REQ_FINAL
  //! bit encoded in their request opcode.
  REQ_FINAL = 0x80
};

//! An identifier for a request or response header
enum HeaderIdentifier {
  HI_COUNT = 0xc0,	//! Number of objects to transfer (used by @[REQ_CONNECT])
  HI_NAME = 0x01,	//! Name of the object (string)
  HI_TYPE = 0x42,	//! Type of the object (IANA media type)
  HI_LENGTH = 0xc3,	//! Length of the object transferred, in octets
  HI_TIME = 0x44,	//! ISO 8601 timestamp (string)
  HI_DESCRIPTION = 0x05,//! Text description of the object
  HI_TARGET = 0x46,	//! Name of service that operation is targeted to
  HI_HTTP = 0x47,	//! Any HTTP 1.x header
  HI_BODY = 0x48,	//! A chunk of the object body
  HI_ENDOFBODY = 0x49,	//! The final chunk of the object body
  HI_WHO = 0x4a,	//! Identifies the OBEX application (string)
  HI_CONNID = 0xcb,	//! An identifier used for OBEX connection multiplexing
  HI_APPPARAM = 0x4c,	//! Extended application request & response information
  HI_AUTHCHALL = 0x4d,	//! Authentication digest-challenge
  HI_AUTHRESP = 0x4e,	//! Authentication digest-response
  HI_CREATORID = 0xcf,	//! Indicates the creator of an object
  HI_WANUUID = 0x50,	//! Uniquely identifies the OBEX server
  HI_OBJCLASS = 0x51,	//! OBEX object class of object
  HI_SESSPARAM = 0x52,	//! Parameters used in session commands/responses
  HI_SESSSEQNR = 0x93,	//! Sequence number used in each OBEX packet for reliability
};

//! A flag for the @[REQ_SETPATH] command indicating that the
//! parent directory should be selected
constant SETPATH_BACKUP = 1;

//! A flag for the @[REQ_SETPATH] command indicating that the
//! selected directory should not be created if it doesn't exist
constant SETPATH_NOCREATE = 2;

//! A set of request or response headers.  Each HI can be associated
//! with either a single value (int or string, depending on the HI in
//! question) or an array with multiple such values.
typedef mapping(HeaderIdentifier:string|int|array) Headers;


//! Serialize a set of headers to wire format
//!
//! @seealso
//!   @[split_headers()]
//!
string encode_headers(Headers h)
{
  string r0 = "", r = "";
  object e = Locale.Charset.encoder("utf16-be");
  foreach(h; HeaderIdentifier hi; string|int|array va)
    foreach(arrayp(va)? va:({va}), string|int v) {
      string vv;
      switch(hi>>6) {
      case 0:
	v = e->clear()->feed(v)->drain()+"\0\0";
      case 1:
	vv = sprintf("%c%2c%s", hi, sizeof(v)+3, v);
	break;
      case 2:
	vv = sprintf("%c%c", hi, v);
	break;
      case 3:
	vv = sprintf("%c%4c", hi, v);
	break;
      }
      if(hi == HI_SESSPARAM)
	r0 += vv;
      else
	r += vv;
    }
  return r0+r;
}

//! Deserialize a set of headers from wire format
//!
Headers decode_headers(string h)
{
  Headers r = ([]);
  object d = Locale.Charset.decoder("utf16-be");
  while(sizeof(h)) {
    HeaderIdentifier hi = h[0];
    string|int v = 0;
    switch(hi>>6) {
    case 0:
    case 1:
      if(sizeof(h)>=3) {
	int l;
	sscanf(h[1..2], "%2c", l);
	v = h[3..2+l];
	h = h[3+l..];
      } else h=v="";
      if(hi < 0x40) {
	if(has_suffix(v, "\0\0"))
	  v = v[..sizeof(v)-3];
	v = d->clear()->feed(v)->drain();
      }
      break;
    case 2:
      if(sizeof(h)>=2)
	v = h[1];
      h = h[2..];
      break;
    case 3:
      if(sizeof(h)>=5)
	sscanf(h[1..4], "%4c", v);
      h = h[5..];
      break;
    }
    if(zero_type(r[hi]))
      r[hi] = v;
    else if(arrayp(r[hi]))
      r[hi] += ({ v });
    else
      r[hi] = ({  r[hi], v });
  }
  return r;
}

//! Given a set of headers in wire format, divide them into
//! portions of no more than @[chunklen] octets each (if possible).
//! No individual header definition will be split into two portions.
//!
array(string) split_headers(string h, int chunklen)
{
  string acc = "";
  array(string) r = ({});

  while(sizeof(h)) {
    int l = 0;
    switch(h[0]>>6) {
    case 0:
    case 1:
      if(sizeof(h)>=3)
	sscanf(h[1..2], "%2c", l);
      if(l<3) l = 3;
      break;
    case 2:
      l = 2;
      break;
    case 3:
      l = 5;
      break;
    }
    if(l>sizeof(h))
      l = sizeof(h);
    if(sizeof(acc) && sizeof(acc)+l > chunklen) {
      r += ({ acc });
      acc = "";
    }
    acc += h[..l-1];
    h = h[l..];
  }

  return r + (sizeof(acc) || !sizeof(r)? ({ acc }) : ({ }));  
}


//! An OBEX client
//!
//! @seealso
//!   @[ATclient]
//!
class client
{
  static Stdio.Stream con;
  static int connected = 0;

  static constant obex_version = 0x10;
  static constant connect_flags = 0;
  static constant default_max_pkt_length = 65535;

  static int server_version, server_connect_flags, max_pkt_length = 255;


  //! Perform a request/response exchange with the server.
  //! No interpretation is preformed of either the request or
  //! response data, they are just passed literally.
  //!
  //! @param r
  //!   Request opcode
  //! @param data
  //!   Raw request data
  //!
  //! @returns
  //!   An array with the response information
  //!
  //!   @array
  //!     @elem int returncode
  //!	    An HTTP response code
  //!	  @elem string data
  //!	    Response data, if any
  //!   @endarray
  //!
  //! @seealso
  //!   @[do_request()]
  //!
  array(int|string) low_do_request(Request r, string data)
  {
    if(3+sizeof(data) > max_pkt_length)
      return ({ 400, "" });

#ifdef OBEX_DEBUG
    werror("OBEX SENDING %O\n", sprintf("%c%2c%s", r, 3+sizeof(data), data));
#endif
    con->write(sprintf("%c%2c%s", r, 3+sizeof(data), data));
    [int rc, int rlen] = array_sscanf(con->read(3), "%c%2c");
#ifdef OBEX_DEBUG
    werror("OBEX RESPONSE: %02x (+ %d bytes)\n", rc, rlen-3);
#endif
    string rdata = (rlen>3? con->read(rlen-3):"");
#ifdef OBEX_DEBUG
    if(sizeof(rdata))
      werror("OBEX EXTRA RESPONSE DATA: %O\n", rdata);
#endif
    if(!(rc & 0x80))
      error("OBEX got response code without final bit!\n");
    rc &= 0x7f;
    return ({ (rc>>4)*100+(rc&15), rdata });
  }

  //! Perform a request/response exchange with the server,
  //! including processing of headers and request splitting.
  //!
  //! @param r
  //!   Request opcode
  //! @param headers
  //!   Request headers
  //! @param extra_req
  //!   Any request data that should appear before the headers,
  //!   but after the opcode
  //!
  //! @returns
  //!   An array with the response information
  //!
  //!   @array
  //!     @elem int returncode
  //!	    An HTTP response code
  //!	  @elem Headers headers
  //!	    Response headers
  //!   @endarray
  //!
  //! @seealso
  //!   @[low_do_request()], @[do_abort()], @[do_put()], @[do_get()],
  //!   @[do_setpath()], @[do_session()]
  //!
  array(int|Headers) do_request(Request r, Headers|void headers,
				string|void extra_req)
  {
    if(!connected)
      return ({ 503, ([]) });

    string hh = encode_headers(headers||([]));

    if(!extra_req)
      extra_req = "";

    if(sizeof(extra_req)+sizeof(hh)+3 > max_pkt_length) {
      array(string) hh_split = split_headers(hh, max_pkt_length-3-
					     sizeof(extra_req));
      hh = hh_split[-1];
      foreach(hh_split[..sizeof(hh_split)-2], string h0) {
	[int rc0, string rdata0] = low_do_request(r&~REQ_FINAL, extra_req+h0);
	if(rc0 != 100)
	  return ({ rc0, (rc0==501? ([]): decode_headers(rdata0)) });
      }
    }

    [int rc, string rdata] = low_do_request(r, extra_req+hh);
    return ({ rc, (rc==501? ([]): decode_headers(rdata)) });
  }

  //! Perform a @[REQ_ABORT] request.
  //!
  //! @seealso
  //!   @[do_request()]
  //!
  array(int|Headers) do_abort(Headers|void headers)
  {
    [int rc, Headers h] = do_request(REQ_ABORT, headers);
    if(rc != 200)
      disconnect();
    return ({ rc, h });
  }

  //! Perform a @[REQ_PUT] request.
  //!
  //! @param data
  //!   Body data to send, or a stream to read the data from
  //! @param extra_headers
  //!   Any additional headers to send (@[HI_LENGTH] and @[HI_BODY]
  //!   are generated by this function)
  //!
  //! @seealso
  //!   @[do_get()], @[do_request()]
  //!
  array(int|Headers) do_put(string|Stdio.Stream data,
			    Headers|void extra_headers)
  {
    string sdata = "";

    if(stringp(data))
      sdata = data;
    else {
      string r;
      while(sizeof(r = data->read(65536)))
	sdata += r;
    }

    [int rc, Headers h] = do_request(REQ_PUT, (extra_headers||([]))|
				     ([HI_LENGTH:sizeof(sdata)]));
    while(rc == 100) {
      Headers h2;
      if(sizeof(sdata)+6 > max_pkt_length) {
	[rc, h2] = do_request(REQ_PUT, ([HI_BODY:sdata[..max_pkt_length-7]]));
	sdata = sdata[max_pkt_length-6..];
      } else
	[rc, h2] = do_request(REQ_PUT|REQ_FINAL, ([HI_ENDOFBODY:sdata]));
      h |= h2;
    }

    return ({ rc, h });
  }

  //! Perform a @[REQ_GET] request.
  //!
  //! @param data
  //!   A stream to write the body data to
  //! @param headers
  //!   Headers for the request
  //!
  //! @returns
  //!   See @[do_request()].  The Headers do not contain any @[HI_BODY]
  //!   headers, they are written to the @[data] stream.
  //!
  //! @seealso
  //!   @[do_put()], @[do_request()]
  //!
  array(int|Headers) do_get(Stdio.Stream data,
			    Headers|void headers)
  {
    [int rc, Headers h] = do_request(REQ_GET|REQ_FINAL, headers||([]));

    for(;;) {
      if(rc == 100 || rc == 200) {
	data->write(((h->HI_BODY||({}))+(h->HI_ENDOFBODY||({})))*"");
	m_delete(h, HI_BODY);
	m_delete(h, HI_ENDOFBODY);
      }
      if(rc != 100)
	break;
      [rc, Headers h2] = do_request(REQ_GET|REQ_FINAL);
      h |= h2;
    }

    return ({ rc, h });
  }

  //! Perform a @[REQ_SETPATH] request.
  //!
  //! @param path
  //!   The directory to set as current working directory
  //!   @string
  //!     @value "/"
  //!       Go to the root directory
  //!     @value ".."
  //!       Go to the parent directory
  //!   @endstring
  //! @param flags
  //!   Logical or of zero or more of @[SETPATH_BACKUP] and @[SETPATH_NOCREATE]
  //! @param extra_headers
  //!   Any additional request headers (the @[HI_NAME] header is generated
  //!   by this function)
  //!
  //! @seealso
  //!   @[do_request()]
  //!
  array(int|Headers) do_setpath(string path, int|void flags,
				Headers|void extra_headers)
  {
    if(path == "/")
      path = "";
    else if(path = "..") {
      path = "";
      flags |= SETPATH_BACKUP;
    }
    return do_request(REQ_SETPATH, (extra_headers||([]))|([HI_NAME:path]),
		      sprintf("%c%c", flags, 0));
  }

  //! Perform a @[REQ_SESSION] request.
  //!
  //! @seealso
  //!   @[do_request()]
  //!
  array(int|Headers) do_session(Headers|void headers)
  {
    return do_request(REQ_SESSION, headers);
  }  

  //! Establish a new connection using the @[REQ_CONNECT] opcode to
  //! negotiate transfer parameters
  //!
  //! @returns
  //!   If the connection succeeds, 1 is returned.  Otherwise, 0 is returned.
  //!
  int(0..1) connect()
  {
    if(connected)
      return 1;

    [int rc, string cdata] =
      low_do_request(REQ_CONNECT,
		     sprintf("%c%c%2c", obex_version, connect_flags,
			     default_max_pkt_length));

    if(rc != 200)
      return 0;

    sscanf(cdata, "%c%c%2c", server_version, server_connect_flags,
	   max_pkt_length);

    connected = 1;

#ifdef OBEX_DEBUG
    werror("OBEX connected, server is version %d.%d, max packet length = %d\n",
	   server_version>>4, server_version&15, max_pkt_length);
#endif

    return 1;
  }

  //! Terminate a connection using the @[REQ_DISCONNECT] opcode
  //!
  //! @returns
  //!   If the disconnection succeeds, 1 is returned.  Otherwise, 0 is returned.
  //!
  int(0..1) disconnect()
  {
    if(!connected)
      return 1;

    [int rc, string ddata] =
      low_do_request(REQ_DISCONNECT, "");

    if(rc != 200)
      return 0;

    connected = 0;    

#ifdef OBEX_DEBUG
    werror("OBEX disconnected.\n");
#endif
  }

  static void destroy()
  {
    if(connected)
      disconnect();
  }

  //! Initialize the client by establishing a connection to the
  //! server at the other end of the provided transport stream
  //!
  //! @param _con
  //!   A stream for writing requests and reading back responses.
  //!   Typically this is some kind of serial port.
  //!
  static void create(Stdio.Stream _con)
  {
    con = _con;
    if(!connect())
      error("Failed to establish OBEX connection\n");
  }
  
}


//! An extension of the @[client] which uses the AT*EOBEX modem command
//! to enter OBEX mode.  Use together with Sony Ericsson data cables.
//!
class ATclient
{
  inherit client;

  int(0..1) connect()
  {
    if(connected)
      return 1;

    con->write("\rAT*EOBEX\r");
    string line, resp = "AT";
    for(;;) {
      int cr;
      resp += con->read(1);
      if((cr = search(resp, "\r\n"))>=0) {
	line = resp[..cr-1];
	if(sizeof(line) && line[..1] != "AT" && line != "OK")
	  break;
	resp = resp[cr+2..];
      }
    }

    if(line != "CONNECT")
      return 0;

    return ::connect();
  }
}
