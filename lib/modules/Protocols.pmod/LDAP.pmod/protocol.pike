// LDAP client protocol implementation for Pike.
//
// $Id: protocol.pike,v 1.1 1999/04/24 16:43:30 js Exp $
//
// Honza Petrous, hop@unibase.cz
//
// ----------------------------------------------------------------------
//
// History:
//
//	v1.9  1999-02-19 created separate file
//			 - improved checking of readed bytes in 'readmsg'
//	v1.9.1.1
//	      1999-03-21 - changed FILE to File (unbuffered)
//			 - some inactive code
//
//	v1.10 1999-03-28 - rewritten methods 'readmsg' & 'writemsg'
//			   to new method 'xchgmsg'
//			 - added core for async operation
//


#include "ldap_globals.h"

#include "ldap_errors.h"

  inherit Stdio.File : ldap;

  // private variables 
  int next_id = 1;				// message id counter
  int ldap_version = LDAP_DEFAULT_VERSION;	// actually used protocol vers.
  string ldap_rem_errstr = LDAP_SUCCESS_STR;	// last remote error description
  int ldap_errno = LDAP_SUCCESS;		// last error code

  /*private*/ string readbuf="";		// read buffer
  private int ok;				// read buffer status
  /*private*/ string writebuf="";		// write buffer
//  private written;				// count of written chars
  private function con_ok, con_fail;		// async callback functions
  object conthread;				// thread connection
  array extra_args;				// not used, yet
//  /*private*/ int errno;
  int connected = 0;

  int seterr(int errno) {
  // Sets ldap_err* variables and returns errno

    //ldap_rem_errstr = errstr;
    ldap_errno = errno;
    return(errno);
  }

  int error_number() { return(ldap_errno); }

  string error_string() { return(ldap_errlist[ldap_errno]); }

  array error() { return(({error_number(), error_string()})); }


  static void read_answer() {
  // ----------------------
  // Reads LDAP PDU (with defined msgid) from the server

    int msglen = 0, ix, ofs;
    string s, shlp;

    if(strlen(readbuf) < 2)
      readbuf = ldap::read(2); 	// 1. byte = 0x0C, 2. byte = msglen
    if (intp(readbuf) || (strlen(readbuf) < 2)) {
      seterr (LDAP_TIMEOUT);
      DWRITE_HI("protocol.read_anwer: ERROR: connection timeout.\n");
      THROW(({"LDAP: connection timeout.\n",backtrace()}));
      //return(-ldap_errno);
      return;
    }
    if (readbuf[0] != '0') {
      seterr (LDAP_PROTOCOL_ERROR);
      DWRITE_HI("protocol.read_anwer: ERROR: retv=<"+sprintf("%O",readbuf)+">\n");
      THROW(({"LDAP: Protocol mismatch.\n",backtrace()}));
      //return(-ldap_errno);
      return;
    }
    DWRITE(sprintf("protocol.read_anwer: sizeof = %d\n", sizeof(readbuf)));

    msglen = readbuf[1];
    ofs = 2;
    if (msglen & 0x80) { // > 0x7f
      if (msglen == 0x80) { // RFC not allows unexplicitly defined length
	seterr (LDAP_PROTOCOL_ERROR);
	THROW(({"LDAP: Protocol mismatch.\n",backtrace()}));
	//return(-ldap_errno);
	return;
      }
      ofs = (msglen & 0x7f) + 2;
      ix = ofs - strlen(readbuf);
      if(ix > 0)
	s = ldap::read(ix);
      if (!s || (strlen(s) < ix)) {
	seterr (LDAP_PROTOCOL_ERROR);
	THROW(({"LDAP: Protocol mismatch.\n",backtrace()}));
	//return(-ldap_errno);
	return;
      }
      readbuf += s;
      msglen = 0; // !!! RESTRICTION: 2^32 !!!
      shlp = reverse(readbuf[2..ofs]);
      for (ix=0; ix<strlen(shlp); ix++) {
        msglen += shlp[ix]*(1<<(ix*8));
      }
    }
    ix = (ofs + msglen) - strlen(readbuf);
    if(ix > 0)
      s = ldap::read(ix);
    if (!s || (strlen(s) < ix)) {
      seterr (LDAP_SERVER_DOWN);
      THROW(({"LDAP: connection closed by server.\n",backtrace()}));
      //return(-ldap_errno);
      return;
    }
    readbuf += s;
    //DWRITE(sprintf("protocol.read_anwer: %s\n", .ldap_privates.ldap_der_decode(readbuf)->debug_string()));
    DWRITE("protocol.read_anwer: ok=1.\n");
    ok = 1;

    remove_call_out(async_timeout);

    if(con_ok)
      con_ok(this_object(), @extra_args);
  }

  static void connect(string server, int port) {
  // -----------------------------------------

//DWRITE("DEB: connect ...\n");
    if(catch { ldap::connect(server, port); }) {
      //if(!(errno = ldap::errno())) {
      if(!ldap::errno()) {
	seterr (LDAP_PARAM_ERROR); // or _CONNECT_ERROR ?
	THROW(({"LDAP: connection parameter error.\n",backtrace()}));
      }
      //::destroy();
      //ldap=0;
      ok = 0;
      return;
    }
  }

  static void async_close() {
  // ----------------------

DWRITE("DEB: async_close ...\n");
    ldap::set_blocking();
    read_answer();
  }

  static int is_whole_pdu() {
  // ----------------------
  // Check if LDAP PDU is complete in 'readbuf'

    int msglen, ix, ofs;
    string shlp;

    if (strlen(readbuf) < 3)
      return(0);  // PDU have min. 3 bytes

    if (readbuf[0] != '0')
      return(1);  // PDU has bad header -> forced execution
    
    msglen = readbuf[1];
    if (msglen & 0x80) { // > 0x7f
      if (msglen == 0x80)
	return(1); // forced execution 
      ofs = (msglen & 0x7f) + 2;
      ix = ofs - strlen(readbuf);
      if(ix > 0)
	return(0);  // incomplete PDU
      msglen = 0;
      shlp = reverse(readbuf[2..ofs]);
      for (ix=0; ix<strlen(shlp); ix++) {
        msglen += shlp[ix]*(1<<(ix*8));
      }
    }
    ix = (ofs + msglen) - strlen(readbuf);
    if(ix > 0)
      return(0);  // incomplete PDU

    return(1);
  }

  static void async_read(mixed id, string data) {
  // ------------------------------------------

DWRITE("DEB: async_read ...\n");
    readbuf += data;
    if(is_whole_pdu()) {
      ldap::set_blocking();
      read_answer();
    }
  }

  static void async_write() {
  // ----------------------

DWRITE("DEB: async_write ...\n");
    ldap::set_blocking();
    ldap::write(writebuf); // ??? NEmusi se testovat uspesnost ??
    ldap::set_nonblocking(async_read, 0, async_close);
  }

  static void async_connected() {
  // --------------------------

DWRITE("DEB: async_connected ...\n");
    ldap::set_nonblocking(async_read, async_write, async_close);
    //ldap::write(""); // ??? What is it? Initiator !??
  }

  static void async_failed() {
  // -----------------------

DWRITE("DEB: async_failed ...\n");
    seterr(LDAP_SERVER_DOWN);
    ok = 0;

    if(con_fail)
      con_fail(this_object(), @extra_args);
    remove_call_out(async_timeout);
  }

  static void async_timeout() {
  // ------------------------

    seterr (LDAP_TIMEOUT);
DWRITE("protocol.async_timeout: ERROR: connection timeout.\n");
    //THROW(({"LDAP: connection timeout.\n",backtrace()}));
    catch {
      ldap::close(); 
      //ldap::destroy();
    };
    async_failed();
  }

  void async_got_host(string server, int port) {
  // -----------------------------------------

    if(!server) {
      async_failed();
      //ldap::destroy()
      return;
    }
    if(!ldap::open_socket()) {
      seterr (LDAP_SERVER_DOWN);
      DWRITE("protocol.async_got_host: ERROR: can't open socket.\n");
      THROW(({"LDAP: can't open socket.\n",backtrace()}));
      return;
    }

    ldap::set_nonblocking(0, async_connected, async_failed);
    DWRITE("protocol.async_got_host: connected!\n");

    if(catch { ldap::connect(server, port); }) {
      //errno = ldap::errno();
      seterr (LDAP_SERVER_DOWN);
      DWRITE("protocol.async_got_host: ERROR: can't open socket.\n");
      //ldap::destroy();
      //ldap=0;
      ok = 0;
      async_failed();
    }
  }

  void async_fetch_read(mixed id, string data) {
  // -----------------------------------------

    readbuf += data;
  }

  void async_fetch_close() {
  // ---------------------

    ldap::set_blocking();
    //ldap::destroy();
    //ldap=0;
    if(con_ok)
      con_ok(@extra_args);
  }




/*********** API methods **************/

  object set_callbacks(function _ok,
			function _fail,
			 mixed ...extra) {
  // -----------------------------------

    extra_args = extra;
    con_ok = _ok;
    con_fail = _fail;
    return(this_object());
  }

  object thread_create(string server, int port) {
  // ------------------------------------------

    return (0); // unimplemented !!!
  }

  object async_create(string server, int port) {
  // -----------------------------------------

    return (0); // unimplemented !!!
#if 0
    call_out(async_timeout);
    dns_lookup_async(server, async_got_host, port);
    return(this_object());
#endif
  }

  void create(string server, int port) {
  // ---------------------------------

    if(!ldap::connect(server, port)) {
      //errno = ldap::errno();
      seterr (LDAP_SERVER_DOWN);
      DWRITE("protocol.create: ERROR: can't open socket.\n");
      //ldap::destroy();
      //ldap=0;
      ok = 0;
      if(con_fail)
	con_fail(this_object(), @extra_args);
    }

    connected = 1;
    DWRITE("protocol.create: connected!\n");

  }


  string|int do_op(object msgop) {
  // ---------------------------
  // Make LDAP PDU envelope for 'msgop', send it and read answer ...

    object msgval;
    object msgid;
    int rv = 0, msgnum;
    string s;

    //THREAD_LOCK
    msgnum = next_id++;
    //THREAD_UNLOCK
    msgid = Standards.ASN1.Types.asn1_integer(msgnum);
    msgval = Standards.ASN1.Types.asn1_sequence(({msgid, msgop}));

    if (objectp(msgval)) {
      DWRITE(sprintf("protocol.do_op: msg = [%d]\n",sizeof(msgval->get_der())));
    } else
      DWRITE("protocol.do_op: msg is null!\n");

    // call_out
    writebuf = msgval->get_der();
    rv = ldap::write(writebuf); // !!!!! - jak rozlisit async a sync ????
    // call_out
    if (rv < 2) {
      seterr (LDAP_SERVER_DOWN);
      THROW(({"LDAP: connection closed by server.\n",backtrace()}));
      return(-ldap_errno);
    }
    DWRITE(sprintf("protocol.do_op: write OK [%d bytes].\n",rv));
    msgval = 0; msgid = 0;
    writebuf= "";
    readbuf= ""; // !!! NEni to pozde ?

    //`()();

    read_answer();
    // now is all in 'readbuf'   

    return(readbuf);

  }

/* ------------ legacy support -----------------*/

  string|int readmsg(int msgid) {
  // Reads LDAP PDU (with defined msgid) from server, checks msgid ...

    int msglen = 0, ix;
    string retv, s, shlp;

    retv = ldap::read(2); 	// 1. byte = 0x0C, 2. byte = msglen
    if (intp(retv) && (retv == -1)) {
      seterr (LDAP_TIMEOUT);
      DWRITE_HI("protocol.readmsg: ERROR: connection timeout.\n");
      THROW(({"LDAP: connection timeout.\n",backtrace()}));
      return(-ldap_errno);
    }
    if (!retv || (sizeof(retv) != 2) || (retv[0] != '0')) {
      seterr (LDAP_PROTOCOL_ERROR);
      DWRITE_HI("protocol.readmsg: ERROR: retv=<"+sprintf("%O",retv)+">\n");
      THROW(({"LDAP: Protocol mismatch.\n",backtrace()}));
      return(-ldap_errno);
    }
    DWRITE(sprintf("protocol.readmsg: sizeof = %d\n", sizeof(retv)));

    msglen = retv[1];
    if (msglen & 0x80) { // > 0x7f
      if (msglen == 0x80) { // RFC not allows unexplicitly defined length
	seterr (LDAP_PROTOCOL_ERROR);
	THROW(({"LDAP: Protocol mismatch.\n",backtrace()}));
	return(-ldap_errno);
      }
      s = ldap::read(msglen & 0x7f);
      if (!s) {
	seterr (LDAP_PROTOCOL_ERROR);
	THROW(({"LDAP: Protocol mismatch.\n",backtrace()}));
	return(-ldap_errno);
      }
      retv += s;
      msglen = 0; // !!! RESTRICTION: 2^32 !!!
      shlp = reverse(s);
      for (ix=0; ix<sizeof(shlp); ix++) {
        msglen += shlp[ix]*(1<<(ix*8));
      }
    }
    s = ldap::read(msglen);
    if (!s | (sizeof(s) < msglen)) {
      seterr (LDAP_SERVER_DOWN);
      THROW(({"LDAP: connection closed by server.\n",backtrace()}));
      return(-ldap_errno);
    }
    retv += s;
    DWRITE(sprintf("protocol.readmsg: %s\n", .ldap_privates.ldap_der_decode(retv)->debug_string()));
    return (retv);
  }

  int writemsg(object msgop) {
  // Make LDAP PDU envelope for 'msgop' and send it

    object msgval;
    object msgid;
    int rv = 0, msgnum;
    string s;

    //THREAD_LOCK
    msgnum = next_id++;
    //THREAD_UNLOCK
    msgid = Standards.ASN1.Types.asn1_integer(msgnum);
    msgval = Standards.ASN1.Types.asn1_sequence(({msgid, msgop}));

    if (objectp(msgval)) {
      DWRITE(sprintf("protocol.writemsg: msg = [%d]\n",sizeof(msgval->get_der())));
    } else
      DWRITE("protocol.writemsg: msg is null!\n");

    // call_out
    rv = ldap::write(msgval->get_der());
    // call_out
    if (rv < 2) {
      seterr (LDAP_SERVER_DOWN);
      THROW(({"LDAP: connection closed by server.\n",backtrace()}));
      return(-ldap_errno);
    }
    DWRITE(sprintf("protocol.writemsg: write OK [%d bytes].\n",rv));
    msgval = 0; msgid = 0;
    return (msgnum);
  }


