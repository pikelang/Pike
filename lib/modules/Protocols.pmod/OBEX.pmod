
enum Request {
  REQ_CONNECT = 0x80,
  REQ_DISCONNECT = 0x81,
  REQ_PUT = 0x02,
  REQ_GET = 0x03,
  REQ_SETPATH = 0x85,
  REQ_SESSION = 0x87,
  REQ_ABORT = 0xff,

  REQ_FINAL = 0x80
};

enum HeaderIdentifier {
  HI_COUNT = 0xc0,
  HI_NAME = 0x01,
  HI_TYPE = 0x42,
  HI_LENGTH = 0xc3,
  HI_TIME = 0x44,
  HI_DESCRIPTION = 0x05,
  HI_TARGET = 0x46,
  HI_HTTP = 0x47,
  HI_BODY = 0x48,
  HI_ENDOFBODY = 0x49,
  HI_WHO = 0x4a,
  HI_CONNID = 0xcb,
  HI_APPPARAM = 0x4c,
  HI_AUTHCHALL = 0x4d,
  HI_AUTHRESP = 0x4e,
  HI_CREATORID = 0xcf,
  HI_WANUUID = 0x50,
  HI_OBJCLASS = 0x51,
  HI_SESSPARAM = 0x52,
  HI_SESSSEQNR = 0x93,
};

constant SETPATH_BACKUP = 1;
constant SETPATH_NOCREATE = 2;

typedef mapping(HeaderIdentifier:string|int|array) Headers;

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

class client
{
  static Stdio.Stream con;
  static int connected = 0;

  static constant obex_version = 0x10;
  static constant connect_flags = 0;
  static constant default_max_pkt_length = 65535;

  static int server_version, server_connect_flags, max_pkt_length = 255;

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

  array(int|Headers) do_abort(Headers|void headers)
  {
    [int rc, Headers h] = do_request(REQ_ABORT, headers);
    if(rc != 200)
      disconnect();
    return ({ rc, h });
  }

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
				     ([HI_COUNT:sizeof(sdata)]));
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

  array(int|Headers) do_session(Headers|void headers)
  {
    return do_request(REQ_SESSION, headers);
  }  

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

  static void create(Stdio.Stream _con)
  {
    con = _con;
    if(!connect())
      error("Failed to establish OBEX connection\n");
  }
  
}


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
