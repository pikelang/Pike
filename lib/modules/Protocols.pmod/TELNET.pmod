//
// $Id$
//
// The TELNET protocol as described by RFC 764 and others.
//
// Henrik Grubbström <grubba@roxen.com> 1998-04-04
//

#pike __REAL_VERSION__

// #define TELNET_DEBUG

#ifdef TELNET_DEBUG
#define DWRITE(X)	werror(X)
#else
#define DWRITE(X)
#endif /* TELNET_DEBUG */

//! Implements TELNET as described by RFC 764 and RFC 854
//!
//! Also implements the Q method of TELNET option negotiation
//! as specified by RFC 1143.

//! Table of IAC-codes.
class TelnetCodes {

constant IAC = 255;		/* interpret as command: */
constant DONT = 254;		/* you are not to use option */
constant DO = 253;		/* please, you use option */
constant WONT = 252;		/* I won't use option */
constant WILL = 251;		/* I will use option */
constant SB = 250;		/* interpret as subnegotiation */
constant GA = 249;		/* you may reverse the line */
constant EL = 248;		/* erase the current line */
constant EC = 247;		/* erase the current character */
constant AYT = 246;		/* are you there */
constant AO = 245;		/* abort output--but let prog finish */
constant IP = 244;		/* interrupt process--permanently */
constant BREAK = 243;		/* break */
constant DM = 242;		/* data mark--for connect. cleaning */
constant NOP = 241;		/* nop */
constant SE = 240;		/* end sub negotiation */
constant EOR = 239;             /* end of record (transparent mode) */
constant ABORT = 238;		/* Abort process */
constant SUSP = 237;		/* Suspend process */
constant xEOF = 236;		/* End of file: EOF is already used... */

constant SYNCH = 242;		/* for telfunc calls */
};

inherit TelnetCodes;


//! Table of TELNET options.
class Telopts {

constant TELOPT_BINARY = 0;	/* 8-bit data path */
constant TELOPT_ECHO = 1;	/* echo */
constant TELOPT_RCP = 2;	/* prepare to reconnect */
constant TELOPT_SGA = 3;        /* suppress go ahead */
constant TELOPT_NAMS = 4;       /* approximate message size */
constant TELOPT_STATUS = 5;     /* give status */
constant TELOPT_TM = 6;         /* timing mark */
constant TELOPT_RCTE = 7;	/* remote controlled transmission and echo */
constant TELOPT_NAOL = 8;	/* negotiate about output line width */
constant TELOPT_NAOP = 9;	/* negotiate about output page size */
constant TELOPT_NAOCRD = 10;	/* negotiate about CR disposition */
constant TELOPT_NAOHTS = 11;	/* negotiate about horizontal tabstops */
constant TELOPT_NAOHTD = 12;	/* negotiate about horizontal tab disposition */
constant TELOPT_NAOFFD = 13;	/* negotiate about formfeed disposition */
constant TELOPT_NAOVTS = 14;	/* negotiate about vertical tab stops */
constant TELOPT_NAOVTD = 15;	/* negotiate about vertical tab disposition */
constant TELOPT_NAOLFD = 16;	/* negotiate about output LF disposition */
constant TELOPT_XASCII = 17;	/* extended ascic character set */
constant TELOPT_LOGOUT = 18;	/* force logout */
constant TELOPT_BM = 19;	/* byte macro */
constant TELOPT_DET = 20;	/* data entry terminal */
constant TELOPT_SUPDUP = 21;	/* supdup protocol */
constant TELOPT_SUPDUPOUTPUT = 22;	/* supdup output */
constant TELOPT_SNDLOC = 23;	/* send location */
constant TELOPT_TTYPE = 24;	/* terminal type */
constant TELOPT_EOR = 25;	/* end or record */
constant TELOPT_TUID = 26;	/* TACACS user identification */
constant TELOPT_OUTMRK = 27;	/* output marking */
constant TELOPT_TTYLOC = 28;	/* terminal location number */
constant TELOPT_3270REGIME = 29;	/* 3270 regime */
constant TELOPT_X3PAD = 30;	/* X.3 PAD */
constant TELOPT_NAWS = 31;	/* window size */
constant TELOPT_TSPEED = 32;	/* terminal speed */
constant TELOPT_LFLOW = 33;	/* remote flow control */
constant TELOPT_LINEMODE = 34;	/* Linemode option */
constant TELOPT_XDISPLOC = 35;	/* X Display Location */
constant TELOPT_OLD_ENVIRON = 36;	/* Old - Environment variables */
constant TELOPT_AUTHENTICATION = 37; /* Authenticate */
constant TELOPT_ENCRYPT = 38;	/* Encryption option */
constant TELOPT_NEW_ENVIRON = 39;	/* New - Environment variables */
constant TELOPT_EXOPL = 255;	/* extended-options-list */

};

inherit Telopts;

mapping lookup_telopt=mkmapping(values(Telopts()),indices(Telopts()));
mapping lookup_telnetcodes=mkmapping(values(TelnetCodes()),indices(TelnetCodes()));


/* sub-option qualifiers */
constant TELQUAL_IS=	0;	/* option is... */
constant TELQUAL_SEND=	1;	/* send option */
constant TELQUAL_INFO=	2;	/* ENVIRON: informational version of IS */
constant TELQUAL_REPLY=	2;	/* AUTHENTICATION: client version of IS */
constant TELQUAL_NAME=	3;	/* AUTHENTICATION: client version of IS */

constant LFLOW_OFF=		0;	/* Disable remote flow control */
constant LFLOW_ON=		1;	/* Enable remote flow control */
constant LFLOW_RESTART_ANY=	2;	/* Restart output on any char */
constant LFLOW_RESTART_XON=	3;	/* Restart output only on XON */


constant	LM_MODE	= 1;
constant	LM_FORWARDMASK = 2;
constant	LM_SLC = 3;

constant	MODE_EDIT =     0x01;
constant	MODE_TRAPSIG =  0x02;
constant	MODE_ACK =	0x04;
constant	MODE_SOFT_TAB =	0x08;
constant	MODE_LIT_ECHO =	0x10;

constant	MODE_MASK =	0x1f;

/* Not part of protocol, but needed to simplify things... */
constant	MODE_FLOW =	0x0100;
constant	MODE_ECHO =	0x0200;
constant	MODE_INBIN =	0x0400;
constant	MODE_OUTBIN =	0x0800;
constant	MODE_FORCE =	0x1000;

constant	SLC_SYNCH=	1;
constant	SLC_BRK=	2;
constant	SLC_IP=		3;
constant	SLC_AO=		4;
constant	SLC_AYT=	5;
constant	SLC_EOR=	6;
constant	SLC_ABORT=	7;
constant	SLC_EOF=	8;
constant	SLC_SUSP=	9;
constant	SLC_EC=		10;
constant	SLC_EL=		11;
constant	SLC_EW=		12;
constant	SLC_RP=		13;
constant	SLC_LNEXT=	14;
constant	SLC_XON=	15;
constant	SLC_XOFF=	16;
constant	SLC_FORW1=	17;
constant	SLC_FORW2=	18;

constant	NSLC=		18;

constant	SLC_NOSUPPORT=	0;
constant	SLC_CANTCHANGE=	1;
constant	SLC_VARIABLE=	2;
constant	SLC_DEFAULT=	3;
constant	SLC_LEVELBITS=	0x03;

constant	SLC_FUNC=	0;
constant	SLC_FLAGS=	1;
constant	SLC_VALUE=	2;

constant	SLC_ACK=	0x80;
constant	SLC_FLUSHIN=	0x40;
constant	SLC_FLUSHOUT=	0x20;

constant	OLD_ENV_VAR=	1;
constant	OLD_ENV_VALUE=	0;
constant	NEW_ENV_VAR=	0;
constant	NEW_ENV_VALUE=	1;
constant	ENV_ESC=	2;
constant	ENV_USERVAR=	3;

/*
 * AUTHENTICATION suboptions
 */

/*
 * Who is authenticating who ...
 */
constant	AUTH_WHO_CLIENT= 0;	/* Client authenticating server */
constant	AUTH_WHO_SERVER= 1;	/* Server authenticating client */
constant	AUTH_WHO_MASK=	 1;

/*
 * amount of authentication done
 */
constant	AUTH_HOW_ONE_WAY=	0;
constant	AUTH_HOW_MUTUAL=	2;
constant	AUTH_HOW_MASK=		2;

constant	AUTHTYPE_NULL=		0;
constant	AUTHTYPE_KERBEROS_V4=	1;
constant	AUTHTYPE_KERBEROS_V5=	2;
constant	AUTHTYPE_SPX=		3;
constant	AUTHTYPE_MINK=		4;
constant	AUTHTYPE_CNT=		5;

constant	AUTHTYPE_TEST=		99;


/*
 * ENCRYPTion suboptions
 */
constant ENCRYPT_IS=		0;	/* I pick encryption type ... */
constant ENCRYPT_SUPPORT=	1;	/* I support encryption types ... */
constant ENCRYPT_REPLY=		2;	/* Initial setup response */
constant ENCRYPT_START=		3;	/* Am starting to send encrypted */
constant ENCRYPT_END=		4;	/* Am ending encrypted */
constant ENCRYPT_REQSTART=	5;	/* Request you start encrypting */
constant ENCRYPT_REQEND=	6;	/* Request you send encrypting */
constant ENCRYPT_ENC_KEYID=	7;
constant ENCRYPT_DEC_KEYID=	8;
constant ENCRYPT_CNT=		9;

constant ENCTYPE_ANY=		0;
constant ENCTYPE_DES_CFB64=	1;
constant ENCTYPE_DES_OFB64=	2;
constant ENCTYPE_CNT=		3;

#define C(X) sprintf("%c",X)
#define C2(X,Y) sprintf("%c%c",X,Y)
#define C3(X,Y,Z) sprintf("%c%c%c",X,Y,Z)

//! Implementation of the TELNET protocol.
class protocol
{
  //! The connection.
  static object fd;

  //! Mapping containing extra callbacks.
  static mapping cb;

  //! Value to send to the callbacks.
  static mixed id;

  //! Write callback.
  static function(mixed|void:string) write_cb;

  //! Read callback.
  static function(mixed,string:void) read_cb;

  //! Close callback.
  static function(mixed|void:void) close_cb;

  // See RFC 1143 for the use and meaning of these.

  static constant UNKNOWN = 0;
  static constant YES = 1;
  static constant NO = 2;
  static constant WANT = 4;
  static constant OPPOSITE = 8;

  //! Negotiation states of all WILL/WON'T options.
  //! See RFC 1143 for a description of the states.
  static array(int) remote_options = allocate(256,NO);
  static array(int) local_options = allocate(256,NO);


  //! Data queued to be sent.
  static string to_send = "";

  //! Indicates that connection should be closed
  static int done;

  //! Tells if we have set the nonblocking write callback or not.
  static int nonblocking_write;

  //! Turns on the write callback if apropriate.
  static void enable_write()
  {
    if (!nonblocking_write && (write_cb || sizeof(to_send) || done)) {
      fd->set_nonblocking(got_data, send_data, close_cb, got_oob);
      nonblocking_write = 1;
    }
  }

  //! Turns off the write callback if apropriate.
  static void disable_write()
  {
    if (!write_cb && !sizeof(to_send) && !done && nonblocking_write) {
      fd->set_nonblocking(got_data, 0, close_cb, got_oob);
      nonblocking_write = 0;
    }
  }

  //! Queues data to be sent to the other end of the connection.
  //!
  //! @param s
  //!   String to send.
  void write(string s)
  {
    DWRITE(sprintf("TELNET, writing :%O\n",s));
    to_send += replace(s, C(IAC), C2(IAC,IAC));
    enable_write();
  }

  //! Queues raw data to be sent to the other end of the connection.
  //!
  //! @param s
  //!   String with raw telnet data to send.
  void write_raw(string s)
  {
    to_send += s;
    enable_write();
  }

  //! Closes the connetion neatly
  void close()
  {
    done=1;
    enable_write();
  }

  //! Callback called when it is clear to send data over the connection.
  //! This function does the actual sending.
  static void send_data()
  {
    if (!sizeof(to_send)) {
      if (write_cb) {
	if(!(to_send = write_cb(id)))
	{
	  done=1;
	  to_send="";
	}
      }
    }

    if (sizeof(to_send))
    {
      if (to_send[0] == 242) {
	// DataMark needs extra quoting... Stupid.
	to_send = C2(IAC,NOP) + to_send;
      }

      int n = fd->write(to_send);

      to_send = to_send[n..];
    } else if(done) {
      fd->close();
      fd=0;
      nonblocking_write=0;
      return;
    }
    disable_write();
  }

  //! Sends a TELNET synch command.
  void send_synch()
  {
    // Clear send-queue.
    to_send = "";

    if (fd->write_oob) {
      fd->write_oob(C(IAC));

      fd->write(C(DM));
    } else {
      // Fallback...
      fd->write(C2(IAC,DM));
    }
  }

  //! @decl void send_DO(int option)
  //!
  //! Starts negotiation to enable a TELNET option.
  //!
  //! @param option
  //!   The option to enable.

  //! @decl void send_DONT(int option)
  //!
  //! Starts negotiation to disable a TELNET option.
  //!
  //! @param option
  //!   The option to disable.


#define CONTROL(OPTIONS,DO,DONT,WILL,WONT,YES,NO)					\
  void send_##DO(int option)								\
  {											\
    if ((option < 0) || (option > 255)) {						\
      error( "Bad TELNET option #%d\n", option);					\
    }											\
     DWRITE(sprintf("TELNET: send_" #DO "(%s) state is %d\n",lookup_telopt[option] || (string)option,OPTIONS##_options[option])); \
    switch(OPTIONS##_options[option])							\
    {											\
      case NO:										\
      case UNKNOWN:									\
	OPTIONS##_options[option]= WANT | YES;						\
        DWRITE(sprintf("TELNET: => " #DO " %s\n",lookup_telopt[option] || (string)option)); \
	to_send += sprintf("%c%c%c",IAC,DO,option); 					\
	break;										\
											\
      case YES: /* Already enabled */							\
      case WANT | YES: /* Will be enabled soon */					\
	break;										\
											\
      case WANT | NO:									\
      case WANT | YES | OPPOSITE:							\
	OPTIONS##_options[option]^=OPPOSITE;						\
	break;										\
											\
      default:										\
	error("TELNET: Strange remote_options[%d]=%d\n",option,remote_options[option]);	\
	/* ERROR: weird state! */							\
	break;										\
    }											\
    enable_write();									\
  }

  //! @ignore
  CONTROL(remote,DO,DONT,WILL,WONT,YES,NO)

  CONTROL(remote,DONT,DO,WONT,WILL,NO,YES)

  CONTROL(local,WILL,WONT,DO,DONT,YES,NO)

  CONTROL(local,WONT,WILL,DONT,DO,NO,YES)
  //! @endignore

  void remote_option_callback(int opt, int onoff)
  {
    call_callback("Remote "+ lookup_telopt[opt], onoff);
    call_callback("Remote option", onoff);
  }

  void local_option_callback(int opt, int onoff)
  {
    call_callback("Local "+ lookup_telopt[opt], onoff);
    call_callback("Local option", onoff);
  }

  void set_remote_option(int opt, int state)
  {
    remote_options[opt]=state;
    switch(state)
    {
      case YES: remote_option_callback(opt,1); break;
      case NO:  remote_option_callback(opt,0); break;
    }
  }

  void set_local_option(int opt, int state)
  {
    local_options[opt]=state;
    switch(state)
    {
      case YES: local_option_callback(opt,1); break;
      case NO:  local_option_callback(opt,0); break;
    }
  }

  int WILL_callback(int opt)
  {
    return call_callback(WILL,opt);
  }

  int DO_callback(int opt)
  {
    return call_callback(DO,opt);
  }

  void send_SB(int|string ... args)
  {
    to_send+=
      C2(IAC,SB)+
      replace(Array.map(args,lambda(int|string s)
			{
			  return sprintf(intp(s)?"%c":"%s",s);
			})*"",C(IAC),C2(IAC,IAC))+
	  C2(IAC,SE);
    enable_write();
  }
  

  //! Indicates wether we are in synch-mode or not.
  static int synch = 0;

  static mixed call_callback(mixed what, mixed ... args)
  {
    if(mixed cb=cb[what])
    {
      mixed err=catch {
	return cb(@args);
      };
      if(err)
      {
	throw(err);
      }else{
	if(!this_object()) return 0;
	throw(err);
      }
    }
    switch(what)
    {
    case BREAK:
      destruct(this_object());
      throw(0);
      break;

    case AYT:
      to_send += C2(IAC,NOP);	// NOP
      enable_write();
      break;
    }
  }

  //! Callback called when Out-Of-Band data has been received.
  //!
  //! @param ignored
  //!   The id from the connection.
  //!
  //! @param s
  //!   The Out-Of-Band data received.
  //!
  static void got_oob(mixed ignored, string s)
  {
#ifdef TELNET_DEBUG
  werror("TELNET: got_oob(\"%s\")\n",Array.map(values(s),lambda(int s) { 
    switch(s)
    {
      case ' '..'z':
	return sprintf("%c",s);
	
      default:
	return sprintf("\\0x%02x",s);
    }
  })*"");
#endif
    synch = synch || (s == C(IAC));
    call_callback("URG",id,s);
  }

  //! Calls @[read_cb()].
  //!
  //! Specifically provided for overloading
  //!
  static void call_read_cb(string data)
  {
    DWRITE("Fnurgel!\n");
    if(read_cb && strlen(data)) read_cb(id,data);
  }

  //! Callback called when normal data has been received.
  //! This function also does most of the TELNET protocol parsing.
  //!
  //! @param ignored
  //!   The id from the connection.
  //! @param s
  //!   The received data.
  //!
  static void got_data(mixed ignored, string line)
  {
#ifdef TELNET_DEBUG
  werror("TELNET: got_data(\"%s\")\n",Array.map(values(line),lambda(int s) { 
    switch(s)
    {
      case ' '..'z':
	return sprintf("%c",s);
	
      default:
	return sprintf("\\0x%02x",s);
    }
  })*"");
#endif

    if (sizeof(line) && (line[0] == DM)) {
      DWRITE("TELNET: Data Mark\n");
      // Data Mark handing.
      line = line[1..];
      synch = 0;
    }

      if (search(line, C(IAC)) != -1) {
	array a = line / C(IAC);

	string parsed_line = a[0];
	int i;
	for (i=1; i < sizeof(a); i++) {
	  string part = a[i];
	  if (sizeof(part)) {

	    DWRITE(sprintf("TELNET: Code %s\n", lookup_telnetcodes[part[0]] || (string)part[0]));

	    int j;
	    function fun;
	    switch (part[0]) {
	    default:
	      call_callback(part[0]);
	      a[i] = a[i][1..];
	      break;

	      // FIXME, find true end of subnegotiation!
	    case SB:
	      call_callback(SB,part[1..]);
	      a[i] = "";
	      break;

	    case EC:	// Erase Character
	      for (j=i; j--;) {
		if (sizeof(a[j])) {
		  a[j] = a[j][..sizeof(a[j])-2];
		  break;
		}
	      }
	      a[i] = a[i][1..];
	      break;

#if 0
	    case EL:	// Erase Line
	      for (j=0; j < i; j++) {
		a[j] = "";
	      }
	      a[i] = a[i][1..];
	      break;
#endif

#define HANDLE(OPTIONS,WILL,WONT,DO,DONT)						\
	    case WILL:{									\
	      int option = a[i][1];							\
	      int state = OPTIONS##_options[option];				       	\
	      a[i] = a[i][2..];								\
											\
	      DWRITE(sprintf(#WILL " %s, state 0x%04x\n", lookup_telopt[option], state));		\
											\
	      switch(state)								\
	      {										\
		case NO:								\
		case UNKNOWN:								\
		  if (WILL##_callback(option))						\
		  {									\
		    /* Agree about enabling */						\
		  state=YES;								\
		    send_##DO(option);							\
		  } else {								\
		    state=NO;								\
		    send_##DONT(option);						\
		  }									\
		  break;								\
											\
		case YES:								\
		  /* Ignore */								\
		  break;								\
											\
		case WANT | NO:								\
		  state=NO;								\
		  break;								\
											\
		case WANT | YES:							\
		  state=YES;								\
		  break;								\
											\
		case WANT | YES | OPPOSITE:						\
		  state=WANT | NO;							\
		  send_##DONT(option);							\
		  break;								\
											\
		default:								\
		  error("TELNET: Strange remote_options[%d]=%d\n",			\
			option,remote_options[option]);					\
		  /* Weird state ! */							\
	      }										\
	      DWRITE(sprintf("=> " #WILL " %s, state 0x%04x\n", lookup_telopt[option], state));	\
	      set_##OPTIONS##_option(option,state);					\
	      break;}									\
											\
	    case WONT:{									\
	      int option = a[i][1];							\
	      int state = OPTIONS##_options[option];					\
	      a[i] = a[i][2..];								\
											\
	      DWRITE(sprintf(#WONT " %s, state 0x%04x\n", lookup_telopt[option], state));		\
											\
	      switch(state)								\
	      {										\
		case UNKNOWN:								\
		case NO:								\
		  state=NO;								\
		  break;								\
											\
		case YES:								\
		  state=NO;								\
		  send_##DONT(option);							\
		  break;								\
											\
		case WANT | NO:								\
		  state=NO;								\
		  break;								\
											\
		case WANT | NO | OPPOSITE:						\
		  state=WANT | YES;							\
		  send_##DO(option);							\
		  break;								\
											\
		case WANT | YES:							\
		case WANT | YES | OPPOSITE:						\
		  state=NO;								\
		  break;								\
											\
		default:								\
		  error("TELNET: Strange remote_options[%d]=%d\n",			\
			option,remote_options[option]);					\
		  /* Weird state */							\
	      }										\
											\
	      DWRITE(sprintf("=> " #WONT " %s, state 0x%04x\n", lookup_telopt[option], state));	\
	      set_##OPTIONS##_option(option,state);					\
	      }break



	      HANDLE(remote,WILL,WONT,DO,DONT);
	      HANDLE(local,DO,DONT,WILL,WONT);

	    case DM:	// Data Mark
	      if (synch) {
		for (j=0; j < i; j++) {
		  a[j] = "";
		}
	      }
	      a[i] = a[i][1..];
	      synch = 0;
	      break;
	    }
	  } else {
	    // IAC IAC => IAC
	    a[i] = C(IAC);
	    i++;
	  }
	}
//	werror("%O\n",a);
	line = a * "";
      }

      if ((!synch)) {
#ifdef TELNET_DEBUG
	werror("TELNET: calling read_callback(X,\"%s\")\n",Array.map(values(line),lambda(int s) { 
	  switch(s)
	  {
	    case ' '..'z':
	      return sprintf("%c",s);
	      
	    default:
	      return sprintf("\\0x%02x",s);
	  }
	})*"");
#endif
	call_read_cb(line);
      }
    enable_write();
  }

  //! Sets the callback to be called when it is clear to send.
  //!
  //! @param w_cb
  //!   The new write callback.
  void set_write_callback(function(mixed|void:string) w_cb)
  {
    write_cb = w_cb;
    if (w_cb) {
      enable_write();
    } else {
      disable_write();
    }
  }

  //! Called when the initial setup is done.
  //!
  //! Created specifically for overloading
  //!
  void setup()
  {
  }

  //! Creates a TELNET protocol handler, and sets its callbacks.
  //!
  //! @param f
  //!   File to use for the connection.
  //! @param r_cb
  //!   Function to call when data has arrived.
  //! @param w_cb
  //!   Function to call when data can be sent.
  //! @param c_cb
  //!   Function to call when the connection is closed.
  //! @param callbacks
  //!   Mapping with callbacks for the various TELNET commands.
  //! @param new_id
  //!   Value to send to the various callbacks.
  //!
  void create(object f,
	      function(mixed,string:void) r_cb,
	      function(mixed|void:string) w_cb,
	      function(mixed|void:void) c_cb,
	      mapping callbacks, mixed|void new_id)
  {
    fd = f;
    cb = callbacks;

    read_cb = r_cb;
    write_cb = w_cb;
    close_cb = c_cb;
    id = new_id;

    fd->set_nonblocking(got_data, w_cb && send_data, close_cb, got_oob);
    nonblocking_write = !!w_cb;
    setup();
  }

  void set_nonblocking(function r_cb, function w_cb, function c_cb)
  {
    read_cb = r_cb;
    write_cb = w_cb;
    close_cb = c_cb;
    fd->set_nonblocking(got_data, w_cb && send_data, close_cb, got_oob);
    nonblocking_write = !!w_cb;
  }

  void set_blocking()
  {
    read_cb = 0;
    write_cb = 0;
    close_cb = 0;
    fd->set_blocking();
    nonblocking_write = 0;
  }

  string query_address(int|void remote)
  {
    return fd->query_address(remote);
  }
}

//! Line-oriented TELNET protocol handler.
class LineMode
{
  inherit protocol;

  static string line_buffer="";

  static void call_read_cb(string data)
  {
    if(read_cb)
    {
      DWRITE(sprintf("Line callback... %O\n",data));
      data=replace(data,
		   ({"\r\n", "\n", "\r", "\r\0"}),
		   ({"\r",   "\r", "\r", "\r",}));
      line_buffer+=data;
      array(string) tmp=line_buffer/"\r";
      line_buffer=tmp[-1];
      for(int e=0;e<sizeof(tmp)-1;e++) read_cb(id,tmp[e]+"\n");
    }
  }


  void setup()
  {
    send_DO(TELOPT_BINARY);
    send_DO(TELOPT_SGA);
    send_DONT(TELOPT_LINEMODE);
    send_WILL(TELOPT_SGA);
  }
}

//! Line-oriented TELNET protocol handler with @[Stdio.Readline] support.
class Readline
{
  inherit LineMode;
  object readline;
  
  string term;
  int width=80;
  int height=24;
  int icanon;
  
  mapping tcgetattr()
  {
    return ([
      "rows":height,
      "columns":width,
      "ECHO":local_options[TELOPT_ECHO],
      "ICANON": icanon,
      ]);
  }
  
  static void call_read_cb(string data)
  {
    if(read_cb)
    {
      if(!icanon)
      {
	if(strlen(data)) read_cb(id,data);
      }else{
	DWRITE(sprintf("Line callback... %O\n",data));
	data=replace(data,
		     ({"\r\n","\r\n","\r","\r\0"}),
		     ({"\r",  "\r",  "\r","\r",}));
	line_buffer+=data;
	array(string) tmp=line_buffer/"\r";
	line_buffer=tmp[-1];
	for(int e=0;e<sizeof(tmp)-1;e++)
	{
	  read_cb(id,tmp[e]+"\n");
	  write(prompt);
	}
      }
    }
  }
  
  int tcsetattr(mapping options)
  {
//    werror("%O\n",options);
    ( options->ECHO ? send_WONT : send_WILL )(TELOPT_ECHO);
    ( (icanon=options->ICANON) ? send_DONT : send_DO )(TELOPT_LINEMODE);
  }

  void set_secret(int onoff)
  {
    if(readline)
    {
      readline->set_echo(!onoff);
    }else{
      ( onoff ? send_WILL : send_WONT )(TELOPT_ECHO);
    }
  }
  
  static function(mixed,string:void) read_cb2;
  
  static void readline_callback(string data)
  {
    read_cb2(id,data+"\n");
  }
  
  static string prompt="";
  static mixed call_callback(mixed what, mixed ... args)
  {
    switch(what)
    {
      case SB:
	string data=args[0];
	DWRITE(sprintf("SB callback %O\n",data));
	switch(data[0])
	{
	  case TELOPT_TTYPE:
	    if(data[1]==TELQUAL_IS)
	    {
	      if(!read_cb2)
	      {
		read_cb2=read_cb;
		term=data[2..];
// 		werror("Enabeling READLINE, term=%s\n",term);
		readline=Stdio.Readline(this_object(),lower_case(term));
		readline->set_nonblocking(readline_callback);
		readline->set_prompt(prompt);
		readline->enable_history(200);
		/* enable data processing */
	      }
	    }
	    break;
	    
	  case TELOPT_NAWS:
	    if(sscanf(data[1..],"%2c%2c",width,height)==2)
	      if(readline)
		readline->redisplay();
	    break;
	}
    }
  }

  void remote_option_callback(int opt, int onoff)
  {
    switch(opt)
    {
      case TELOPT_TTYPE:
	if(onoff)
	{
	  send_SB(TELOPT_TTYPE,TELQUAL_SEND);
	}else{
	  werror("Revert to line mode not yet operational.\n");
	  /* revert to stupid line mode */
	}
    }
    ::remote_option_callback(opt,onoff);
  }
  
  void setup()
  {
    send_DO(TELOPT_SGA);
    send_DO(TELOPT_BINARY);
    send_DO(TELOPT_NAWS);
    send_DO(TELOPT_TTYPE);
    /* disable data processing */
  }
  
  void message(string s,void|int word_wrap)
  {
    if(readline)
    {
      readline->write(s,word_wrap);
    }else{
      write(replace(s,"\n","\r\n"));
    }
  }
  
  void set_prompt(string s)
  {
//    werror("TELNET: prompt=%O\n",s);
    if(readline)
    {
      prompt=s;
      readline->set_prompt(prompt);
    }else{
      if(prompt!=s)
      {
	if(s[..strlen(prompt)-1]==prompt)
	  write(s);
	prompt=s;
      }
    }
  }

  void close()
  {
    readline->set_blocking();
    readline=0;
    ::close();
  }
}

