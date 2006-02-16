/*
 * $Id: tds.pike,v 1.16 2006/02/16 12:09:51 grubba Exp $
 *
 * A Pike implementation of the TDS protocol.
 *
 * Henrik Grubbström 2006-02-08.
 */

/* #define TDS_DEBUG */
/* #define TDS_CONVERT_DEBUG */

#ifdef TDS_DEBUG
#define TDS_WERROR(X...)	werror("TDS:" + X)
#else
#define TDS_WERROR(X...)
#endif
#ifdef TDS_CONVERT_DEBUG
#define TDS_CONV_WERROR(X...)	werror("TDS: Convert: " + X)
#else
#define TDS_CONV_WERROR(X...)
#endif

//!
//! The TDS SQL-protocol.
//!
//! This protocol is used by Sybase and Microsoft's SQL-servers.
//!
//! @example
//!   @tt{Sql.Sql con = Sql.Sql("tds://user:pass@@host/database");@}
//!
//! @seealso
//!   @[Sql.Sql()]

static int filter_noprint(int char)
{
  return ((char == 32) || ((char != 0x7f) && (char & 0x60)))?char:'.';
}

static string hex_dump(string data) {
  array(string) lines = data/16.0;
  int off;
  foreach(lines; int i; string line) {
    array(array(int)) halves = (array(array(int)))((line + ("\0"*16))/8);
    lines[i] = sprintf("%04x:%{ %02x%}:%{%02x %} %s:%s\n",
		       off, halves[0], halves[1],
		       (string)map(halves[0], filter_noprint),
		       (string)map(halves[1], filter_noprint));
    off += 16;
  }
  return lines * "";
}

#if (__REAL_MAJOR__ > 7) || ((__REAL_MAJOR__ == 7) && (__REAL_MINOR__ >= 6))
// Static blocks affect nested classes in Pike 7.4.
// We don't want that...
static {
#endif /* Pike 7.6 or later */
  constant DEF_MAJOR = 8;
  constant DEF_MINOR = 0;
  constant DEF_PORT = 1433;
  constant TDS8VERSION = "\x01\x00\x00\x71";
  constant CLIENTVERSION = "\x06\x83\xf2\xf8";
  constant CONNECTIONID = "\0\0\0\0";
  constant TIMEZONE = "\x88\xff\xff\xff";
  constant COLLATION = "\x36\x04\x00\x00";

  //! Tokens that may occur in the answers from the server.
  enum Token {
    TDS_ERROR			= 3,
    TDS_DONT_RETURN		= 42,

    TDS5_PARAMFMT2_TOKEN	= 32,	/* 0x20 */
    TDS_LANGUAGE_TOKEN		= 33,	/* 0x21    TDS 5.0 only              */
    TDS_ORDERBY2_TOKEN		= 34,	/* 0x22 */
    TDS_ROWFMT2_TOKEN		= 97,	/* 0x61    TDS 5.0 only              */
    TDS_LOGOUT_TOKEN		= 113,	/* 0x71    TDS 5.0 only? ct_close()  */
    TDS_RETURNSTATUS_TOKEN	= 121,	/* 0x79                              */
    TDS_PROCID_TOKEN		= 124,	/* 0x7C    TDS 4.2 only - TDS_PROCID */
    TDS7_RESULT_TOKEN		= 129,	/* 0x81    TDS 7.0 only              */
    TDS7_COMPUTE_RESULT_TOKEN	= 136,	/* 0x88    TDS 7.0 only              */
    TDS_COLNAME_TOKEN		= 160,	/* 0xA0    TDS 4.2 only              */
    TDS_COLFMT_TOKEN		= 161,	/* 0xA1    TDS 4.2 only - TDS_COLFMT */
    TDS_DYNAMIC2_TOKEN		= 163,	/* 0xA3 */
    TDS_TABNAME_TOKEN		= 164,	/* 0xA4 */
    TDS_COLINFO_TOKEN		= 165,	/* 0xA5 */
    TDS_OPTIONCMD_TOKEN		= 166,	/* 0xA6 */
    TDS_COMPUTE_NAMES_TOKEN	= 167,	/* 0xA7 */
    TDS_COMPUTE_RESULT_TOKEN	= 168,	/* 0xA8 */
    TDS_ORDERBY_TOKEN		= 169,	/* 0xA9    TDS_ORDER                 */
    TDS_ERROR_TOKEN		= 170,	/* 0xAA                              */
    TDS_INFO_TOKEN		= 171,	/* 0xAB                              */
    TDS_PARAM_TOKEN		= 172,	/* 0xAC    RETURNVALUE?              */
    TDS_LOGINACK_TOKEN		= 173,	/* 0xAD                              */
    TDS_CONTROL_TOKEN		= 174,	/* 0xAE    TDS_CONTROL               */
    TDS_ROW_TOKEN		= 209,	/* 0xD1                              */
    TDS_CMP_ROW_TOKEN		= 211,	/* 0xD3                              */
    TDS5_PARAMS_TOKEN		= 215,	/* 0xD7    TDS 5.0 only              */
    TDS_CAPABILITY_TOKEN	= 226,	/* 0xE2                              */
    TDS_ENVCHANGE_TOKEN		= 227,	/* 0xE3                              */
    TDS_EED_TOKEN		= 229,	/* 0xE5                              */
    TDS_DBRPC_TOKEN		= 230,	/* 0xE6                              */
    TDS5_DYNAMIC_TOKEN		= 231,	/* 0xE7    TDS 5.0 only              */
    TDS5_PARAMFMT_TOKEN		= 236,	/* 0xEC    TDS 5.0 only              */
    TDS_AUTH_TOKEN		= 237,	/* 0xED                              */
    TDS_RESULT_TOKEN		= 238,	/* 0xEE                              */
    TDS_DONE_TOKEN		= 253,	/* 0xFD    TDS_DONE                  */
    TDS_DONEPROC_TOKEN		= 254,	/* 0xFE    TDS_DONEPROC              */
    TDS_DONEINPROC_TOKEN	= 255,	/* 0xFF    TDS_DONEINPROC            */
    /* CURSOR support: TDS 5.0 only*/
    TDS_CURCLOSE_TOKEN		= 128,	/* 0x80    TDS 5.0 only              */
    TDS_CURFETCH_TOKEN		= 130,	/* 0x82    TDS 5.0 only              */
    TDS_CURINFO_TOKEN		= 131,	/* 0x83    TDS 5.0 only              */
    TDS_CUROPEN_TOKEN		= 132,	/* 0x84    TDS 5.0 only              */
    TDS_CURDECLARE_TOKEN	= 134,	/* 0x86    TDS 5.0 only              */
  };

  //! Environment types.
  //!
  //! Used by @[TDS_ENV_CHANGE_TOKEN]
  enum EnvType {
    /* environment type field */
    TDS_ENV_DATABASE		= 1,
    TDS_ENV_LANG		= 2,
    TDS_ENV_CHARSET		= 3,
    TDS_ENV_PACKSIZE		= 4,
    TDS_ENV_LCID		= 5,
    TDS_ENV_SQLCOLLATION	= 7,
  };

  //! Field types.
  enum FieldType {
    SYBBINARY		= 45,	/* 0x2d */
    SYBBIT		= 50,	/* 0x32 */
    SYBBITN		= 104,	/* 0x68 */
    SYBCHAR		= 47,	/* 0x2f */
    SYBDATETIME		= 61,	/* 0x3d */
    SYBDATETIME4	= 58,	/* 0x3a */
    SYBDATETIMN		= 111,	/* 0x6f */
    SYBDECIMAL		= 106,	/* 0x6a */
    SYBFLT8		= 62,	/* 0x3e */
    SYBFLTN		= 109,	/* 0x6d */
    SYBIMAGE		= 34,	/* 0x22 */
    SYBINT1		= 48,	/* 0x30 */
    SYBINT2		= 52,	/* 0x34 */
    SYBINT4		= 56,	/* 0x38 */
    SYBINT8		= 127,	/* 0x7f */
    SYBINTN		= 38,	/* 0x26 */
    SYBLONGBINARY	= 225,	/* 0xe1 */
    SYBMONEY		= 60,	/* 0x3c */
    SYBMONEY4		= 122,	/* 0x7a */
    SYBMONEYN		= 110,	/* 0x6e */
    SYBNTEXT		= 99,	/* 0x63 */
    SYBNUMERIC		= 108,	/* 0x6c */
    SYBNVARCHAR		= 103,	/* 0x67 */
    SYBREAL		= 59,	/* 0x3b */
    SYBSINT1		= 64,	/* 0x40 */
    SYBTEXT		= 35,	/* 0x23 */
    SYBUINT2		= 65,	/* 0x41 */
    SYBUINT4		= 66,	/* 0x42 */
    SYBUINT8		= 67,	/* 0x43 */
    SYBUNIQUE		= 36,	/* 0x24 */
    SYBVARBINARY	= 37,	/* 0x25 */
    SYBVARCHAR		= 39,	/* 0x27 */
    SYBVARIANT		= 98,	/* 0x62 */
    SYBVOID		= 31,	/* 0x1f */
    XSYBBINARY		= 173,	/* 0xad */
    XSYBCHAR		= 175,	/* 0xaf */
    XSYBNCHAR		= 239,	/* 0xef */
    XSYBNVARCHAR	= 231,	/* 0xe7 */
    XSYBVARBINARY	= 165,	/* 0xa5 */
    XSYBVARCHAR		= 167,	/* 0xa7 */

    TDS_UT_TIMESTAMP	= 80,	/* User type. */
  };

  string server_data;
  string last_error;

  //! Format and report an error.
  void tds_error(string msg, mixed ... args)
  {
    if (sizeof(args)) msg = sprintf(msg, @args);
    TDS_WERROR("ERROR: %s", msg);
    predef::error(last_error = msg);
  }

  static object utf16enc = Locale.Charset.encoder("UTF16LE");
  static string string_to_utf16(string s)
  {
    return utf16enc->feed(s)->drain();
  }
  static object utf16dec = Locale.Charset.decoder("UTF16LE");
  static string utf16_to_string(string s)
  {
    return utf16dec->feed(s)->drain();
  }

  //! A connection to a TDS server.
  class Connection {
    int major_version = DEF_MAJOR;
    int minor_version = DEF_MINOR;
    int port = DEF_PORT;
    int block_size = 4096;
    string server = "";
    string server_charset = "";
    string hostname = gethostname();
    string appname = "";
    string username = "";
    string password = "";
    string language = "";//"us_english";
    string library_name = "TDS-Library";
    string database = "";
    string domain;

    //! The actual TCP connection.
    Stdio.File socket;

#define FMT_SMALLINT	"%-2c"
#define FMT_INT		"%-4c"
#define FMT_INT8	"%-4c"

    //! An incoming packet from the TDS server.
    class InPacket
    {
      int inpos = 0;
      string inbuf = "";
      int done;

      static void fill_buf()
      {
	if (done) {
	  TDS_WERROR("Filling buffer on finished packet!\n"
		     "%s\n", hex_dump(inbuf));
	  tds_error("Filling buffer on finished packet!\n");
	}

	string header = socket->read(8);
	if (!header || sizeof(header) < 8) {
	  busy = !(done = 1);
	  int errno = socket->errno();
	  Disconnect();
	  predef::error("Failed to read packet header: %O, %s.\n",
			header, strerror(errno));
	}
	TDS_WERROR("Read header:\n%s\n", hex_dump(header));
	int packet_type;
	int last_packet;
	int len;
	// NOTE: Network byteorder!!
	sscanf(header, "%-1c%-1c%2c", packet_type, last_packet, len);
	len -= 8;

	busy = !(done = last_packet);

	string data = socket->read(len);
	if (!data || sizeof(data) < len) {
	  tds_error("Failed to read packet data (%d bytes), got %O (%d bytes), %s\n",
		    len, data, sizeof(data||""), strerror(socket->errno()));
	}
	TDS_WERROR("Read packet with %d bytes.\n%s\n",
		   sizeof(data), hex_dump(data));
	inbuf = inbuf[inpos..] + data;
	inpos = 0;
      }

      static void destroy()
      {
	// Return the connection to the idle state.
	while (!done) {
	  inbuf = "";
	  inpos = 0;
	  fill_buf();
	}
      }

      string get_raw(int bytes)
      {
	while (inpos + bytes > sizeof(inbuf)) {
	  fill_buf();
	}
	string raw = inbuf[inpos..inpos + bytes - 1];
	inpos += bytes;
	return raw;
      }

      string peek_raw(int bytes)
      {
	while (inpos + bytes > sizeof(inbuf)) {
	  fill_buf();
	}
	string raw = inbuf[inpos..inpos + bytes - 1];
	return raw;
      }

      int get_int8()
      {
	return array_sscanf(get_raw(8), "%-8c")[0];
      }
      int get_int()
      {
	return array_sscanf(get_raw(4), "%-4c")[0];
      }
      int get_int_be()
      {
	return array_sscanf(get_raw(4), "%4c")[0];
      }
      int get_smallint()
      {
	return array_sscanf(get_raw(2), "%-2c")[0];
      }
      int get_byte()
      {
	return array_sscanf(get_raw(1), "%-1c")[0];
      }
      int peek_byte()
      {
	return array_sscanf(peek_raw(1), "%-1c")[0];
      }

      string get_string(int len)
      {
	if (!len) return "";
	TDS_WERROR("get_string(%d)...\n", len);
	return utf16_to_string(get_raw(len*2));
      }

      void expect(string s)
      {
	string r = get_raw(sizeof(s));
	if (r != s) {
	  tds_error("Expectation failed: Got %O, expected %O\n",
		    r, s);
	}
      }

      static void create()
      {
	if (busy) {
	  tds_error("Creating InPacket on busy connection!\n");
	}
	busy = 1;
      }
    }

    //static InPacket login_answer;

    //! An outgoing packet to the TDS server.
    class Packet
    {
      array(string|int) segments = ({});
      array(string) strings = ({});
      int flags;

      static void create(int|void flags)
      {
	this_program::flags = flags;
      }

      void put_int8(int i)
      {
	segments += ({ sprintf("%-8c", i) });
      }
      void put_int(int i)
      {
	segments += ({ sprintf("%-4c", i) });
      }
      void put_smallint(int i)
      {
	segments += ({ sprintf("%-2c", i) });
      }
      void put_byte(int i)
      {
	segments += ({ sprintf("%-1c", i) });
      }
      void put_raw(string raw, int len)
      {
	if (sizeof(raw) != len) {
	  tds_error("Internal error: unexpected raw length: %O (expected %d)\n",
		    raw, len);
	}
	segments += ({raw});
      }
      void put_raw_string(string s)
      {
	if (1 || sizeof(s)) {
	  segments += ({ sizeof(strings) });
	  strings += ({ s });
	} else {
	  segments += ({ sprintf("%-2c%-2c", 0, 0) });
	}
      }
      void put_string(string s)
      {
	put_raw_string(string_to_utf16(s));
      }
      void put_long_raw_string(string s)
      {
	if (sizeof(s)) {
	  segments += ({ ~sizeof(strings) });
	  strings += ({ s });
	} else {
	  segments += ({ sprintf("%-2c%-2c%-4c", 0, 0, 0) });
	}
      }
      void put_long_string(string s)
      {
	put_long_raw_string(string_to_utf16(s));
      }

      void put_domain_login(string domain, string hostname)
      {
	string raw = sprintf("NTLMSSP\0s%-4c%-4c%-2c%-2c%-4c%-2c%-2c%-4c%s%s",
			     1, 0xb201,
			     sizeof(domain), sizeof(domain),
			     32 + sizeof(hostname),
			     sizeof(hostname), sizeof(hostname),
			     32,
			     hostname, domain);
	segments += ({ sizeof(strings) });
	strings += ({ raw });
      }

      mixed cast(string s)
      {
	int trailer_start = flags && 4;
	foreach(segments, string|int seg) {
	  trailer_start += stringp(seg)?sizeof(seg):(seg<0)?8:4;
	}
	foreach(segments; int i; string|int seg) {
	  if (intp(seg)) {
	    if (seg < 0) {
	      seg = ~seg;
	      segments[i] = sprintf("%-2c%-2c%-4c",
				    sizeof(strings[seg]),
				    sizeof(strings[seg]),
				    trailer_start);
	      TDS_WERROR("Long string %O at offset %d\n",
			 strings[seg], trailer_start);
	    } else {
	      segments[i] = sprintf("%-2c%-2c",
				    trailer_start, sizeof(strings[seg])/2);
	      TDS_WERROR("Short string %O at offset %d\n",
			 strings[seg], trailer_start);
	    }
	    segments += ({ strings[seg] });
	    trailer_start += sizeof(strings[seg]);
	  }
	}
	string res = segments * "";
	TDS_WERROR("Generated packet: %O (%d bytes)\n",
		   res, sizeof(res));
	if (!flags) return res;
	return sprintf("%-4c%s", sizeof(res)+4, res);
      }
    }

    //! Send a packet to the TDS server.
    //!
    //! @note
    //!   May only be called when the connection is idle.
    //!
    //! @returns
    //!   If @[last] is true an @[InPacket] with the result
    //!   will be returned.
    InPacket send_packet(Packet p, int flag, int|void last)
    {
      if (busy) {
	tds_error("Sending packet on busy connection!\n");
      }

      array(string) packets = ((string) p)/(float)(block_size-8);
      foreach(packets; int i; string raw) {
	// NOTE: Network byteorder!!
	raw = sprintf("%1c%1c%2c\0\0%1c\0%s",
		      flag, (i == (sizeof(packets)-1)) && last,
		      sizeof(raw) + 8,
		      1,		/* TDS 7 or 8. */
		      raw);
	TDS_WERROR("Wrapped packet: %O\n%s\n", raw, hex_dump(raw));
	if (socket->write(raw) != sizeof(raw)) {
	  Disconnect();
	  predef::error("Failed to send packet.\n"
			"raw: %O\n", raw);
	}
      }
      //Stdio.write_file("packet.bin", raw);
      if (last) return InPacket();
      return 0;
    }

    static string crypt_pass(string password)
    {
      password = string_to_utf16(password);
      password ^= "\x5a"*sizeof(password);
      return (string)map((array)password,
			 lambda(int byte) {
			   return ((byte >> 4) | (byte << 4)) & 0xff;
			 });
    }

    static InPacket send_login()
    {
      password = password[..127];

      Packet p = Packet(1);

      p->put_raw(TDS8VERSION, 4);	// TDS version
      p->put_int(0);			// block size
      p->put_raw(CLIENTVERSION, 4);	// client version
      p->put_int(getpid());		// pid
      p->put_raw(CONNECTIONID, 4);	// connection identifier
      p->put_byte(0x80|0x40|0x20);	// warnings + force select db
      p->put_byte(3|(domain && 0x80));	// domain login
      p->put_byte(0);			// sql type
      p->put_byte(0);			// reserved
      p->put_raw(TIMEZONE, 4);		// time zone
      p->put_raw(COLLATION, 4);		// collation

      p->put_string(hostname);		// hostname
      
      if (domain) {
	p->put_string("");
	p->put_string("");
      } else {
	p->put_string(username);	// user name
	p->put_raw_string(crypt_pass(password));	// password
      }
      p->put_string(appname);
      p->put_string(server);
      p->put_smallint(0);		// Unknown
      p->put_smallint(0);		// Unknown
      p->put_string("");
      p->put_string(language);
      p->put_string(database);
      p->put_raw("\0"*6, 6);		// MAC address
      if (domain) {
	p->put_domain_login(domain, hostname);
      } else {
	p->put_string("");
      }
      p->put_string("");
      
      TDS_WERROR("Sending login packet.\n");
      return send_packet(p, 0x10, 1);
    }

    string ecb_encrypt(string data, string key)
    {
#if constant(Crypto.DES)
      Crypto.DES des = Crypto.DES();
      des->set_encrypt_key(des->fix_parity(key));
#else
      Crypto.des des = Crypto.des();
      des->set_encrypt_key(Crypto.des_parity(key));
#endif
      return des->crypt(data);
    }

    string encrypt_answer(string hash, string nonce)
    {
      return ecb_encrypt(nonce, hash[..6]) +
	ecb_encrypt(nonce, hash[7..13]) +
	ecb_encrypt(nonce, hash[14..]);
    }

    string answer_lan_mgr_challenge(string passwd, string nonce)
    {
      string magic = "KGS!@#$%";

      /* FIXME: Filter wide characters in passwd! */
      passwd = upper_case((passwd + "\0"*14)[..13]);
      string hash =
	ecb_encrypt(magic, passwd[..6]) +
	ecb_encrypt(magic, passwd[7..]);
      return encrypt_answer(hash, nonce);
    }

    string answer_nt_challenge(string passwd, string nonce)
    {
      string nt_passwd = string_to_utf16(passwd);
#if constant(Crypto.MD4)
      Crypto.MD4 md4 = Crypto.MD4();
#else
      Crypto.md4 md4 = Crypto.md4();
#endif
      md4->update(nt_passwd);
      return encrypt_answer(md4->digest() + "\0"*16, nonce);
    }

    static void send_auth(string nonce)
    {
      int out_flag = 0x11;
      Packet p = Packet();
      p->put_raw("NTLMSSP\0", 8);
      p->put_int(3);		/* sequence 3 */
      p->put_long_raw_string(answer_lan_mgr_challenge(password, nonce));
      p->put_long_raw_string(answer_nt_challenge(password, nonce));
      p->put_long_string(domain);
      p->put_long_string(username);
      p->put_long_string(hostname);
      p->put_long_string("");	/* Unknown */
      p->put_int(0x8201);	/* flags */

      TDS_WERROR("Sending auth packet.\n");
      send_packet(p, 0x11);
    }

    static void process_auth(InPacket inp)
    {
      int pdu_size = inp->get_smallint();
      if (pdu_size < 32) tds_error("Bad pdu size: %d\n", pdu_size);
      inp->expect("NTLMSSP\0");
      inp->get_int();	/* sequence -> 2 */
      inp->get_int();	/* domain len * 2 */
      inp->get_int();	/* domain offset */
      inp->get_int();	/* flags */
      string nonce = inp->get_raw(8);
      /* Discard context, target and data info */
      inp->get_raw(pdu_size - 32);
      TDS_WERROR("Got nonce: %O\n", nonce);
      send_auth(nonce);
    }

    static void process_msg(InPacket inp, int token_type)
    {
      TDS_WERROR("TDS_ERROR_TOKEN | TDS_INFO_TOKEN | TDS_EED_TOKEN\n");
      int len = inp->get_smallint();
      int no = inp->get_int();
      int state = inp->get_byte();
      int level = inp->get_byte();
      int is_error = 0;
      int has_eed = 0;
      switch(token_type) {
      case TDS_EED_TOKEN:
	TDS_WERROR("TDS_EED_TOKEN\n");
	if (level > 10) is_error = 1;

	int state_len = inp->get_byte();
	string state = inp->get_raw(state_len);
	has_eed = inp->get_byte();
	/* Ignore status and transaction state */
	inp->get_smallint();
	break;
      case TDS_INFO_TOKEN:
	TDS_WERROR("TDS_INFO_TOKEN\n");
	break;
      case TDS_ERROR_TOKEN:
	TDS_WERROR("TDS_ERROR_TOKEN\n");
	is_error = 1;
	break;
      }
      string message = inp->get_string(inp->get_smallint());
      string server = inp->get_string(inp->get_byte());
      string proc_name = inp->get_string(inp->get_byte());
      int line = inp->get_smallint();
      if (has_eed) {
	while (1) {
	  switch(inp->peek_byte()) {
	  case TDS5_PARAMFMT_TOKEN:
	  case TDS5_PARAMFMT2_TOKEN:
	  case TDS5_PARAMS_TOKEN:
	    process_default_tokens(inp, inp->get_byte());
	    continue;
	  }
	  break;
	}
      }
      if (is_error) {
	tds_error("%d: %s:%s:%d %s\n",
		  level, proc_name, server, line, message);
      } else {
#if 0
	last_error = sprintf("%d: %s:%s:%d %s\n",
			     level, proc_name, server, line, message);
#endif
	TDS_WERROR("%d: %s:%s:%d %s\n",
		   level, proc_name, server, line, message);
      }
    }

    static void process_env_chg(InPacket inp)
    {
      int size = inp->get_smallint();
      int env_type = inp->get_byte();
      if (env_type == TDS_ENV_SQLCOLLATION) {
	size = inp->get_byte();
	string block = inp->get_raw(size);
	if (size >= 5) {
	  string collation = block[..4];
	  int lcid;
	  sscanf(collation, "%-3c", lcid);
	  /* FIXME: Should we care? */
	}
	/* Discard old collation. */
	inp->get_raw(inp->get_byte());
	return;
      }
      string new_val = inp->get_string(inp->get_byte());
      string old_val = inp->get_string(inp->get_byte());
      switch(env_type) {
      case TDS_ENV_PACKSIZE:
	int new_block_size = (int)new_val;
	TDS_WERROR("Change block size from %O to %O\n", old_val, new_val);
	break;
      case TDS_ENV_DATABASE:
	TDS_WERROR("Database changed to %O\n", new_val);
	break;
      case TDS_ENV_LANG:
	TDS_WERROR("Language changed to %O from %O\n", new_val, old_val);
	break;
      case TDS_ENV_CHARSET:
	TDS_WERROR("Charset changed from %O to %O\n", new_val, old_val);
	break;
      }
    }

    int get_size_by_type(int col_type)
    {
      switch (col_type) {
      case SYBINT1:
      case SYBBIT:
      case SYBBITN:
	return 1;
      case SYBINT2:
	return 2;
      case SYBINT4:
      case SYBREAL:
      case SYBDATETIME4:
      case SYBMONEY4:
	return 4;
      case SYBINT8:
      case SYBFLT8:
      case SYBDATETIME:
      case SYBMONEY:
	return 8;
      case SYBUNIQUE:
	return 16;
      default:
	return -1;
      }
    }

    int get_cardinal_type(int col_type)
    {
      // FIXME: This function could be done away with.
      switch (col_type) {
      case XSYBVARBINARY:
	return SYBVARBINARY;
      case XSYBBINARY:
	return SYBBINARY;
      case SYBNTEXT:
	return SYBTEXT;
      case XSYBNVARCHAR:
      case XSYBVARCHAR:
	return SYBVARCHAR;
      case XSYBNCHAR:
      case XSYBCHAR:
	return SYBCHAR;
      }
      return col_type;
    }

    int get_varint_size(int col_type)
    {
      switch (col_type) {
      case SYBLONGBINARY:
      case SYBTEXT:
      case SYBNTEXT:
      case SYBIMAGE:
      case SYBVARIANT:
	return 4;
      case SYBVOID:
      case SYBINT1:
      case SYBBIT:
      case SYBINT2:
      case SYBINT4:
      case SYBINT8:
      case SYBDATETIME4:
      case SYBREAL:
      case SYBMONEY:
      case SYBDATETIME:
      case SYBFLT8:
      case SYBMONEY4:
      case SYBSINT1:
      case SYBUINT2:
      case SYBUINT4:
      case SYBUINT8:
	return 0;
      case XSYBNCHAR:
      case XSYBNVARCHAR:
      case XSYBCHAR:
      case XSYBVARCHAR:
      case XSYBBINARY:
      case XSYBVARBINARY:
	return 2;
      default:
	return 1;
      }
    }

    int is_unicode_type(int col_type)
    {
      return (col_type == XSYBNVARCHAR) ||
	(col_type == XSYBNCHAR) ||
	(col_type == SYBNTEXT);
    }

    int is_ascii_type(int col_type)
    {
      return (<XSYBCHAR,XSYBVARCHAR,SYBTEXT,SYBCHAR,SYBVARCHAR>)[col_type];
    }

    int is_char_type(int col_type)
    {
      return is_unicode_type(col_type) || is_ascii_type(col_type);
    }

    int is_blob_type(int col_type)
    {
      return (col_type == SYBTEXT) ||
	(col_type == SYBIMAGE) ||
	(col_type == SYBNTEXT);
    }

    int is_collate_type(int col_type)
    {
      return (col_type==XSYBVARCHAR) ||
	(col_type==XSYBCHAR) ||
	(col_type==SYBTEXT) ||
	(col_type==XSYBNVARCHAR) ||
	(col_type==XSYBNCHAR) ||
	(col_type==SYBNTEXT);
    }

    int is_numeric_type(int col_type)
    {
      return (col_type == SYBNUMERIC) || (col_type == SYBDECIMAL);
    }

    mapping(string:mixed) tds7_get_data_info(InPacket inp)
    {
      mapping(string:mixed) res = ([
	"usertype":inp->get_smallint(),
	"flags":inp->get_smallint(),
	"column_type":inp->get_byte(),
      ]);
      res->nullable == res->flags & 0x01;
      res->writeable == !!(res->flags & 0x08);
      res->identity == !!(res->flags & 0x10);

      res->cardinal_type = get_cardinal_type(res->column_type);
      res->varint_size = get_varint_size(res->column_type);
      switch(res->varint_size) {
      case 0:
	res->column_size = get_size_by_type(res->column_type);
	break;
      case 4:
	res->column_size = inp->get_int();
	break;
      case 2:
	res->column_size = inp->get_smallint();
	break;
      case 1:
	res->column_size = inp->get_byte();
	break;
      }

      res->timestamp = (res->cardinal_type == SYBBINARY) ||
	(res->cardinal_type == TDS_UT_TIMESTAMP);

      if (is_numeric_type(res->cardinal_type)) {
	TDS_WERROR("is_numeric\n");
	res->column_prec = inp->get_byte();
	res->column_scale = inp->get_byte();
      }
      if (is_collate_type(res->column_type)) {
	TDS_WERROR("is_collate\n");
	inp->get_raw(4);	// Collation
	inp->get_byte();	// charset
      }
      if (is_blob_type(res->cardinal_type)) {
	TDS_WERROR("is_blob\n");
	res->table = inp->get_string(inp->get_smallint());
      }
      res->name = inp->get_string(inp->get_byte());
      TDS_WERROR("Column info: %O\n", res);
      return res;
    }

    static array(mapping(string:mixed)) tds7_process_result(InPacket inp)
    {
      int num_cols = inp->get_smallint();
      if (num_cols == 0xffff) {
	TDS_WERROR("No meta data.\n");
	return 0;
      }
      TDS_WERROR("%d columns in result.\n", num_cols);
      array(mapping(string:mixed)) column_info = allocate(num_cols);
      foreach(column_info; int col; ) {
	column_info[col] = tds7_get_data_info(inp);
      }
      return column_info;
    }

    static void process_default_tokens(InPacket inp, int token_type)
    {
      if (token_type == TDS_DONE_TOKEN) return;
      switch(token_type) {
      case TDS_DONE_TOKEN:
	return;
      case TDS_ERROR_TOKEN:
      case TDS_INFO_TOKEN:
      case TDS_EED_TOKEN:
	process_msg(inp, token_type);
	return;
      case TDS_ENVCHANGE_TOKEN:
	process_env_chg(inp);
	return;
      case TDS7_RESULT_TOKEN:
	tds_error("TDS7_RESULT_TOKEN in default handler!\n");
	//tds7_process_result(inp);
	break;
      case TDS5_DYNAMIC_TOKEN:
      case TDS_LOGINACK_TOKEN:
      case TDS_ORDERBY_TOKEN:
      case TDS_CONTROL_TOKEN:
      case TDS_TABNAME_TOKEN:
	TDS_WERROR("Skipping token: %d\n", token_type);
	inp->get_raw(inp->get_smallint());
	break;
      default:
	werror("TDS: WARNING: Got unknown token in process_default_tokens: %d\n",
	       token_type);
	break;
      }
    }

    array(mapping(string:mixed)) process_result_tokens(InPacket inp)
    {
      array(mapping(string:mixed)) column_info;
      while (1) {
	int token_type = inp->peek_byte();
	TDS_WERROR("Got result token %d\n", token_type);
	switch(token_type) {
	case TDS7_RESULT_TOKEN:
	  TDS_WERROR("TDS7_RESULT_TOKEN\n");
	  inp->get_byte();
	  column_info = tds7_process_result(inp);
	  if (inp->peek_byte() == TDS_TABNAME_TOKEN) {
	    TDS_WERROR("TDS_TABNAME_TOKEN\n");
	    process_default_tokens(inp, inp->get_byte());
	    if (inp->peek_byte() == TDS_COLINFO_TOKEN) {
	      TDS_WERROR("TDS_COLINFO_TOKEN\n");
	      inp->get_byte();
	      //process_colinfo();	// FIXME!
	    }
	  }
	  break;
	case TDS_DONE_TOKEN:
	  TDS_WERROR("  TDS_DONE_TOKEN pending\n");
	  return column_info;
	case TDS_ROW_TOKEN:
	  TDS_WERROR("  TDS_ROW_TOKEN pending\n");
	  return column_info;
	case TDS_RETURNSTATUS_TOKEN:
	  inp->get_byte();
	  int ret_status = inp->get_int();
	  TDS_WERROR("Return status: %d\n", ret_status);
	  break;
	default:
	  werror("TDS: WARNING: Unhandled token in process_result_tokens: %d\n",
		 token_type);
	  // FALL_THROUGH
	case TDS_ORDERBY_TOKEN:
	  /* Ignore. */
	case TDS_ERROR_TOKEN:
	case TDS_INFO_TOKEN:
	case TDS_EED_TOKEN:
	  inp->get_byte();
	  process_default_tokens(inp, token_type);
	  break;
	}
      }
    }

    static string|int get_data(InPacket inp,
			       mapping(string:mixed) info, int col)
    {
      TDS_WERROR("get_data for column %d info:%O\n", col, info);
      mapping blobinfo;
      int colsize = 0;

    outer:
      switch(info->varint_size) {
      case 4:
	switch(info->cardinal_type) {
	case SYBVARIANT:
	  int sz = inp->get_int();
	  if (!sz) return 0;	// NULL;
	  return inp->get_raw(sz);
	case SYBLONGBINARY:
	  colsize = inp->get_int();
	  break outer;
	}
	int len = inp->get_byte();
	if (len == 16) {
	  blobinfo = ([
	    "textptr":inp->get_raw(16),
	    "timestamp":inp->get_raw(8),
	  ]);
	  colsize = inp->get_int();
	  TDS_WERROR("BLOB: size:%d info:%O\n", colsize, blobinfo);
	}
	break;
      case 2:
	colsize = inp->get_smallint();
	if (!colsize) {
	  // Empty string.
	  return "";
	}
	if (colsize == 0xffff) colsize = 0;
	break;
      case 1:
	colsize = inp->get_byte();
	break;
      case 0:
	colsize = get_size_by_type(info->cardinal_type);
	break;
      }
      TDS_WERROR("Column size is %d\n", colsize);
      if (!colsize) return 0;	// NULL.
      if (is_numeric_type(info->cardinal_type)) {
	// NUMERIC
	string arr = inp->get_raw(colsize);
	TDS_WERROR("NUMERIC: %O\n", arr);
	return arr;	// FIXME
      } else if (is_blob_type(info->cardinal_type)) {
	// BLOB
	string raw = inp->get_raw(colsize);
	TDS_WERROR("BLOB. colsize:%d, raw: %O\n", colsize, raw);
	if (is_char_type(info->cardinal_type)) {
	  // FIXME: Move this to convert()?
	  return utf16_to_string(raw);
	}
	return raw;
      } else {
	string raw = inp->get_raw(colsize);
	TDS_WERROR("Got raw data: %O\ncolumn_size:%d colsize:%d\n%s\n",
		   raw, info->column_size, colsize, hex_dump(raw));
	if (is_unicode_type(info->column_type)) {
	  // FIXME: Move this to convert()?
	  raw = utf16_to_string(raw);
	}
	if (colsize < info->column_size) {
	  // Handle padding.
	  switch(info->cardinal_type) {
	  case SYBLONGBINARY:
	    // FIXME: ??
	    break;
	  case SYBCHAR:
	  case XSYBCHAR:
	    raw += " "*(info->column_size - colsize);
	    break;
	  case SYBBINARY:
	  case XSYBBINARY:
	    raw += "\0"*(info->column_size - colsize);
	    break;
	  }
	}
	if (info->cardinal_type == SYBDATETIME4) {
	  TDS_WERROR("Datetime4:%{ %d}\n", (array)raw);
	}
	TDS_WERROR("ROW: %O\n", raw);
	return raw;
      }
    }

    static string|int convert(string|int raw, mapping(string:mixed) info)
    {
      if (!raw) {
	TDS_CONV_WERROR("%O ==> NULL\n", raw);
	return raw; /* NULL */
      }
      int cardinal_type;
      switch(cardinal_type = info->cardinal_type) {
      case SYBCHAR:
      case SYBVARCHAR:
      case SYBTEXT:
      case XSYBCHAR:
      case XSYBVARCHAR:
      case SYBNVARCHAR:
      case SYBNTEXT:
      case SYBLONGBINARY:
      case SYBBINARY:
      case SYBVARBINARY:
      case SYBIMAGE:
      case XSYBBINARY:
      case XSYBVARBINARY:
	// Strings have already been converted.
	TDS_CONV_WERROR("%O ==> %O\n", raw, raw);
	return raw;
      case SYBMONEYN:
      case SYBMONEY4:
      case SYBMONEY:
	{
	  int val;
	  string sgn = "";
	  if (sizeof(raw) == 8) {
	    sscanf(raw, "%-8c", val);
	  } else {
	    sscanf(raw, "%-4c", val);
	  }
	  if (val < 0) {
	    sgn = "-";
	    val = -val;
	  }
	  int cents = (val + 50)/100;
	  string res = sprintf("%s%d.%02d", sgn, cents/100, cents%100);
	  TDS_CONV_WERROR("%O ==> %O\n", raw, res);
	  return res;
	}
      case SYBNUMERIC:
	{
	  string res =
	    sprintf("%d", array_sscanf(raw[1..],
				       "%-" + (sizeof(raw)-1) + "c")[0]);

	  // Move decimal point @[scale] positions.
	  int scale = info->column_scale;
	  if (sizeof(res) < scale) {
	    res = "0." + ("0" * (scale - sizeof(res))) + res;
	  } else if (sizeof(res) == scale) {
	    res = "0." + res;
	  } else if (scale) {
	    res = res[..sizeof(res)-(scale+1)] + "." +
	      res[sizeof(res)-scale..];
	  }

	  // Fix the sign.
	  if (!res[0]) {
	    res = "-" + res;
	  }
	  TDS_CONV_WERROR("%O (scale: %d) ==> %O\n",
			  raw, scale, res);
	  return res;
	}
      case SYBDECIMAL:
      case SYBREAL:
      case SYBFLT8:
      case SYBUNIQUE:
      default:
	// FIXME:
	TDS_CONV_WERROR("Not yet supported: %d (%O)\n",
			info->cardinal_type, raw);
	werror("TDS: WARNING: Datatype %d not yet supported. raw: %O\n",
	       info->cardinal_type, raw);
	return raw;
      case SYBBIT:
      case SYBBITN:
	TDS_CONV_WERROR("%O ==> \"%d\"", raw, !!raw[0]);
	return raw[0]?"1":"0";
      case SYBINT1:
      case SYBINT2:
      case SYBINT4:
      case SYBINT8:
      case SYBINTN:
	{
	  int val;
	  if (raw[-1]& 0x80) {
	    // Negative.
	    val = ~array_sscanf((~raw) + ("\x00"*8), "%-8c")[0];
	  } else {
	    val = array_sscanf(raw + ("\x00"*8), "%-8c")[0];
	  }
	  TDS_CONV_WERROR("%O ==> \"%d\"\n", raw, val);
	  return sprintf("%d", val);
	}
      case SYBDATETIMN:
      case SYBDATETIME:
      case SYBDATETIME4:
	{
	  int day, min, sec, sec_300;
	  if (sizeof(raw) == 8) {
	    sscanf(raw, "%-4c%-4c", day, sec_300);
	    sec = sec_300/300;
	    sec_300 %= 300;
	    min = sec/60;
	    sec %= 60;
	  } else {
	    sscanf(raw, "%-2c%-2c", day, min);
	  }
	  int hour = min/60;
	  min %= 60;

	  int l = day + 146038;
	  int century = (l * 4)/146097;
	  l -= (century*146097 + 3)/4;
	  int year = ((l + 1)*4000)/1461001;
	  l -= (year * 1461)/4;
	  int yday = (l > 306)?(l - 305):(l + 60);
	  l += 31;
	  int j = (l * 80)/2447;
	  int mday = l - (j * 2447)/80;
	  l = j/11;
	  int mon = j + 1 - l*12;
	  year += (century + 15)*100 + l;
	  if (!l && !(year & 3) && ((year % 100) || !(year % 400)))
	    yday++;
	  string res = sprintf("%04d-%02d-%02dT%02d:%02d:%02d",
			       year, mon, mday, hour, min, sec);
	  TDS_CONV_WERROR("%O ==> %O\n", raw, res);
	  return res;
	}
      }
    }

    static array(string|int) process_row(InPacket inp,
					 array(mapping(string:mixed)) col_info)
    {
      if (!col_info) return 0;
      array(string|int) res = allocate(sizeof(col_info));
      foreach(col_info; int i; mapping(string:mixed) info) {
	res[i] = convert(get_data(inp, info, i), info);
      }
      return res;
    }

    array(string|int) process_row_tokens(InPacket inp,
					 array(mapping(string:mixed)) col_info,
					 int|void leave_end_token)
    {
      //if (!busy) return 0;	// No more rows.
      while (1) {
	int token_type = inp->peek_byte();
	TDS_WERROR("Process row tokens: Token type: %d\n", token_type);
	switch(token_type) {
	case TDS_RESULT_TOKEN:
	case TDS_ROWFMT2_TOKEN:
	case TDS7_RESULT_TOKEN:
	  // Some other result starts here.
	  return 0;
	case TDS_ROW_TOKEN:
	  inp->get_byte();
	  return process_row(inp, col_info);
	  break;
	case TDS_DONE_TOKEN:
	case TDS_DONEPROC_TOKEN:
	case TDS_DONEINPROC_TOKEN:
	  if (!leave_end_token) inp->get_byte();
	  return 0;
	default:
	  inp->get_byte();
	  process_default_tokens(inp, token_type);
	  break;
	}
      }
    }

    static void process_login_tokens(InPacket inp)
    {
      int ok = 0;
      int token_type;
      do {
	token_type = inp->get_byte();
	TDS_WERROR("Got token: %d\n", token_type);
	switch(token_type) {
	case TDS_DONE_TOKEN:
	  TDS_WERROR("TDS_DONE_TOKEN\n");
	  break;
	case TDS_AUTH_TOKEN:
	  TDS_WERROR("TDS_AUTH_TOKEN\n");
	  process_auth(inp);
	  break;
	case TDS_LOGINACK_TOKEN:
	  TDS_WERROR("TDS_LOGINACK_TOKEN\n");
	  int len = inp->get_smallint();
	  int ack = inp->get_byte();
	  int major = inp->get_byte();
	  int minor = inp->get_byte();
	  inp->get_smallint();	/* Ignored. */
	  inp->get_byte();		/* Product name len */
	  server_product_name = inp->get_string((len-10)/2);
	  int product_version = inp->get_int_be();
	  if ((major == 4) && (minor == 2) &&
	      ((product_version & 0xff0000ff) == 0x5f0000ff)) {
	    // Workaround for bugs in MSSQL 6.5 & 7.0 for TDS 4.2.
	    product_version = (product_version & 0xffff00) | 0x800000000;
	  }
	  
	  // TDS 5.0 reports 5 on success 6 on fail.
	  // TDS 4.2 reports 1 on success and N/A on failure.
	  if ((ack == 5) || (ack == 1)) ok = 1;
	  TDS_WERROR("  ok:%d ack:%d %s major:%d minor:%d version:%08x\n",
		     ok, ack,
		     server_product_name, major, minor, product_version);
	  server_product_name = (server_product_name/"\0")[0];
	  server_data = sprintf("%s %d.%d.%d.%d",
				server_product_name,
				product_version>>24,
				product_version & 0xffffff,
				major, minor);
	  break;
	default:
	  process_default_tokens(inp, token_type);
	  break;
	}
      } while (token_type != TDS_DONE_TOKEN);
      //if (!(spid = rows_affected)) set_spid();
      if (!ok) tds_error("Login failed.\n");
      TDS_WERROR("Login ok!\n");
    }

    InPacket submit_query(compile_query query, void|array(mixed) params)
    {
      Packet p = Packet();
      if (!query->n_param || !params || !sizeof(params)) {
	string raw = query->splitted_query*"?";
	p->put_raw(raw, sizeof(raw));
	return send_packet(p, 0x01, 1);
      } else {
	tds_error("parametrized queries not supported yet.\n");
      }
    }

    void disconnect()
    {
      socket->close();
      destruct();
    }

    static void create(string server, int port, string database,
		       string username, string auth)
    {
      string domain;
      array(string) tmp = username/"\\";
      if (sizeof(tmp) > 1) {
	// Domain login.
	domain = tmp[0];
	username = tmp[1..]*"\\";
      }

      this_program::server = server;
      this_program::port = port;
      this_program::database = database;
      this_program::username = username;
      this_program::password = auth;
      this_program::domain = domain;

      TDS_WERROR("Connecting to %s:%d with TDS version %d.%d\n",
		 server, port, major_version, minor_version);

      socket = Stdio.File();
      if (!socket->connect(server, port)) {
	socket = 0;
	tds_error("Failed to connect to %s:%d\n", server, port);
      }
      process_login_tokens(send_login());
    }
  }

  object(Connection) con;

  int affected_rows;
  string server_product_name = "";

  int busy;

  void Disconnect(int|void keep_alive)
  {
    con->disconnect();
    con = 0;
    if (!keep_alive) {
      destruct();
    }
  }

  void Connect(string server, int port, string database,
	       string uid, string password)
  {
    con = Connection(server, port, database, uid, password);
  }

#if (__REAL_MAJOR__ > 7) || ((__REAL_MAJOR__ == 7) && (__REAL_MINOR__ >= 6))
};
#endif /* Pike 7.6 or later */


//! A compiled query.
class compile_query
{
  int n_param;
  array(string) splitted_query;

  array(mixed) params;

  void parse_prepared_query()
  {
    // FIXME:
  }

  static array(string) split_query_on_placeholders(string query)
  {
    array(string) res = ({});
    int i;
    int j = 0;
    for (i = 0; i < sizeof(query); i++) {
      switch(query[i]) {
      case '\'':
	i = search(query, "\'", i+1);
	if (i == -1) {
	  TDS_WERROR("Bad quoting!\n");
	  i = sizeof(query);
	}
	break;
      case '\"':
	i = search(query, "\"", i+1);
	if (i == -1) {
	  TDS_WERROR("Bad quoting!\n");
	  i = sizeof(query);
	}
	break;
      case '[':
	i = search(query, "]", i+1);
	if (i == -1) {
	  TDS_WERROR("Bad quoting!\n");
	  i = sizeof(query);
	}
	break;
      case '-':
	if (query[i..i+1] == "--") {
	  i = search(query, "\n", i+1);
	  if (i == -1) {
	    TDS_WERROR("Unterminated comment.\n");
	    i = sizeof(query);
	  }
	}
	break;
      case '/':
	if (query[i..i+1] == "/*") {
	  i = search(query, "*/", i+2);
	  if (i == -1) {
	    TDS_WERROR("Unterminated comment.\n");
	    i = sizeof(query);
	  }
	}
	break;
      case '?':
	res += ({ query[j..i-1] });
	j = i+1;
	break;
      }
    }
    res += ({ query[j..] });
    return map(res, string_to_utf16);
  }

  //! Compile a query.
  //!
  //! @seealso
  //!   @[big_query()]
  static void create(string query)
  {
    TDS_WERROR("Compiling query: %O\n", query);
    splitted_query = split_query_on_placeholders(query);
    n_param = sizeof(splitted_query)-1;
  }
}

//! A query result set.
class big_query
{
  static int row_no;
  static int eot;

  static Connection.InPacket result_packet;
  static array(mapping(string:mixed)) column_info;

  //! Fetch the next row from the result set.
  //!
  //! @returns
  //!   Returns @expr{0@} (zero) if all rows have been returned.
  //!
  //!   Otherwise returns an @expr{array(string|int)@} with one
  //!   entry for each field. If the field is @tt{NULL@} the
  //!   entry will be @expr{0@} (zero), otherwise the entry
  //!   will contain a string representing the value.
  int|array(string|int) fetch_row()
  {
    if (eot) return 0;
    TDS_WERROR("fetch_row()::::::::::::::::::::::::::::::::::::::::::\n");
    int|array(string|int) res = con->process_row_tokens(result_packet,
							column_info);
    eot = !res;
    row_no++;
    return res;
  }

  //! Fetch a description of the fields in the result.
  //!
  //! @returns
  //!   Returns an array with a mapping for each of the fields in
  //!   the result.
  //!
  //!   The mappings contain the following information:
  //!   @ul
  //!     @item
  //!       Standard fields:
  //!       @mapping
  //!         @member string "name"
  //!           The name of the field.
  //!         @member string|void "table"
  //!           The name of the table (if available).
  //!       @endmapping
  //!     @item
  //!       TDS-specific fields:
  //!       @mapping
  //!         @member int(0..1) "nullable"
  //!           @expr{1@} if the field may contain @tt{NULL@}.
  //!         @member int(0..1) "writeable"
  //!           @expr{1@} if the field may be changed.
  //!         @member int(0..1) "identity"
  //!           @expr{1@} if the field is the identity for the row.
  //!         @member int "column_size"
  //!           Width of the field.
  //!         @member int(0..1) "timestamp"
  //!           Time stamp information for last change is available.
  //!         @member int|void "column_prec"
  //!           Precision of the field.
  //!         @member int|void "column_scale"
  //!           Scale exponent of the field.
  //!         @member int "usertype"
  //!         @member int "flags"
  //!         @member int "column_type"
  //!         @member int "cardinal_type"
  //!         @member int "varint_size"
  //!           Internal use only.
  //!       @endmapping
  //!   @endul
  array(mapping(string:mixed)) fetch_fields()
  {
    TDS_WERROR("fetch_fields()::::::::::::::::::::::::::::::::::::::::::\n");
    return copy_value(column_info);
  }

  //! Execute a query against the database.
  //!
  //! @param query
  //!   The query to execute. This can either be a string, or
  //!   a compiled query.
  //!
  //! @seealso
  //!   @[compile_query()]
  static void create(string|compile_query query)
  {
    if (stringp(query)) {
      query = compile_query(query);
    }

    query->parse_prepared_query();
    if (busy) {
      tds_error("Connection not idle.\n");
    }
    if (!query->params) {
      result_packet = con->submit_query(query);
    } else {
      result_packet = con->submit_execdirect(query, query->params);
    }
    column_info = con->process_result_tokens(result_packet);
    if (!column_info) destruct();
  }
}

static compile_query compiled_insert_id =
  compile_query("SELECT @@identity AS insert_id");

//! Fetch the identity of the last insert (if available).
//!
//! This performs the query @expr{"SELECT @@@@identity AS insert_id"@}.
//!
//! @returns
//!   Returns the identity of the last insert as an integer if available.
//!   Otherwise returns @expr{0@} (zero).
int insert_id()
{
  object res = big_query(compiled_insert_id);
  array(mapping(string:mixed)) field_info =
    res->fetch_fields();
  if (!field_info) return 0;
  array(string|int) row = res->fetch_row();
  if (!row) return 0;
  foreach(field_info->name; int i; string name) {
    if (name == "insert_id") return (int)row[i];
  }
  return 0;
}

//! Return a string describing the server.
string server_info()
{
  return server_data;
}

//! Return the last error (or possibly the last warning or
//! informational message).
string error()
{
  return last_error;
}

//! Connect to a remote SQL server via the TDS protocol.
//!
//! @param server
//!   Server to connect to.
//!
//! @param database
//!   Database to connect to.
//!
//! @param user
//!   User to access as.
//!
//!   An explicit domain may be specified by preceeding the user name
//!   with the domain name and a @expr{'\\'@}.
//!
//! @param password
//!   Password to access with.
//!
//! Usually accessed via @[Sql.Sql()].
//!
//! @seealso
//!   @[Sql.Sql()]
static void create(string|void server, string|void database,
		   string|void user, string|void password)
{
  if (con) {
    Disconnect(1);
  }
  int port = DEF_PORT;
  if (server) {
    array(string) tmp = server/":";
    if (sizeof(tmp) > 1) {
      port = (int)tmp[-1];
      server = tmp[..sizeof(tmp)-2]*":";
    }
  } else {
    server = "127.0.0.1";
  }
  Connect(server, port, database || "default",
	  user || "", password || "");
}
