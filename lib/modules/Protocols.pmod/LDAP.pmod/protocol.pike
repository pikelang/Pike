#pike __REAL_VERSION__

// LDAP client protocol implementation for Pike.
//
// $Id: protocol.pike,v 1.16 2005/03/11 16:49:57 mast Exp $
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

#if constant(Standards.ASN1.Types)

import Protocols.LDAP;

#include "ldap_globals.h"

#include "ldap_errors.h"

  // private variables 
  int next_id = 1;				// message id counter
  int ldap_version = LDAP_DEFAULT_VERSION;	// actually used protocol vers.
  string ldap_rem_errstr = ldap_errlist[LDAP_SUCCESS]; // last remote error description
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
  object low_fd = Stdio.File();			// helper fd
  object ldapfd;			// helper fd

  int seterr(int errno) {
  // Sets ldap_err* variables and returns errno

    //ldap_rem_errstr = errstr;
    ldap_errno = errno;
    return errno;
  }

  //!
  //! Returns error number of last transaction.
  //!
  //! @seealso
  //!  @[LDAP.protocol.error_string]
  int error_number() { return ldap_errno; }

  //!
  //! Returns error description of search result.
  //!
  //! @seealso
  //!  @[LDAP.protocol.error_number]
  string error_string() { return ldap_errlist[ldap_errno]; }

  array error() { return ({error_number(), error_string()}); }


  static void read_answer() {
  // ----------------------
  // Reads LDAP PDU (with defined msgid) from the server

    int msglen = 0, ix, ofs;
    string s, shlp;

    if(sizeof(readbuf) < 2)
      readbuf = ldapfd->read(2); 	// 1. byte = 0x0C, 2. byte = msglen
    if (intp(readbuf) || (sizeof(readbuf) < 2)) {
      seterr (LDAP_TIMEOUT);
      DWRITE_HI("protocol.read_answer: ERROR: connection closed.\n");
      THROW(({"LDAP: connection closed.\n",backtrace()}));
      //return -ldap_errno;
      return;
    }
    if (readbuf[0] != '0') {
      seterr (LDAP_PROTOCOL_ERROR);
      DWRITE_HI("protocol.read_answer: ERROR: retv=<"+sprintf("%O",readbuf)+">\n");
      THROW(({"LDAP: Protocol mismatch.\n",backtrace()}));
      //return -ldap_errno;
      return;
    }
    DWRITE(sprintf("protocol.read_answer: sizeof = %d\n", sizeof(readbuf)));

    msglen = readbuf[1];
    ofs = 2;
    if (msglen & 0x80) { // > 0x7f
      if (msglen == 0x80) { // RFC not allows unexplicitly defined length
	seterr (LDAP_PROTOCOL_ERROR);
	THROW(({"LDAP: Protocol mismatch.\n",backtrace()}));
	//return -ldap_errno;
	return;
      }
      ofs = (msglen & 0x7f) + 2;
      ix = ofs - sizeof(readbuf);
      if(ix > 0)
	s = ldapfd->read(ix);
      if (!s || (sizeof(s) < ix)) {
	seterr (LDAP_PROTOCOL_ERROR);
	THROW(({"LDAP: Protocol mismatch.\n",backtrace()}));
	//return -ldap_errno;
	return;
      }
      readbuf += s;
      msglen = 0; // !!! RESTRICTION: 2^32 !!!
      shlp = reverse(readbuf[2..ofs]);
      for (ix=0; ix<sizeof(shlp); ix++) {
        msglen += shlp[ix]*(1<<(ix*8));
      }
    }
    ix = (ofs + msglen) - sizeof(readbuf);
    if(ix > 0)
      s = ldapfd->read(ix);
    if (!s || (sizeof(s) < ix)) {
      seterr (LDAP_SERVER_DOWN);
      THROW(({"LDAP: connection closed by server.\n",backtrace()}));
      //return -ldap_errno;
      return;
    }
    readbuf += s;
    //DWRITE(sprintf("protocol.read_answer: %s\n", .ldap_privates.ldap_der_decode(readbuf)->debug_string()));
    DWRITE("protocol.read_answer: ok=1.\n");
    ok = 1;

    if(con_ok)
      con_ok(this, @extra_args);
  }

  static int is_whole_pdu() {
  // ----------------------
  // Check if LDAP PDU is complete in 'readbuf'

    int msglen, ix, ofs;
    string shlp;

    if (sizeof(readbuf) < 3)
      return 0;  // PDU have min. 3 bytes

    if (readbuf[0] != '0')
      return 1;  // PDU has bad header -> forced execution
    
    msglen = readbuf[1];
    if (msglen & 0x80) { // > 0x7f
      if (msglen == 0x80)
	return 1; // forced execution
      ofs = (msglen & 0x7f) + 2;
      ix = ofs - sizeof(readbuf);
      if(ix > 0)
	return 0;  // incomplete PDU
      msglen = 0;
      shlp = reverse(readbuf[2..ofs]);
      for (ix=0; ix<sizeof(shlp); ix++) {
        msglen += shlp[ix]*(1<<(ix*8));
      }
    }
    ix = (ofs + msglen) - sizeof(readbuf);
    if(ix > 0)
      return 0;  // incomplete PDU

    return 1;
  }

  void create(object fd) {
  // -------------------
    ldapfd = fd;
  }


  string|int do_op(object msgop, object|void controls) {
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
    if (controls) {
      msgval = Standards.ASN1.Types.asn1_sequence(({msgid, msgop, controls}));
    } else {
      msgval = Standards.ASN1.Types.asn1_sequence(({msgid, msgop}));
    }

    if (objectp(msgval)) {
      DWRITE(sprintf("protocol.do_op: msg = [%d]\n",sizeof(msgval->get_der())));
    } else
      DWRITE("protocol.do_op: msg is null!\n");

    // call_out
    writebuf = msgval->get_der();
    rv = ldapfd->write(writebuf); // !!!!! - jak rozlisit async a sync ????
    // call_out
    if (rv < 2) {
      seterr (LDAP_SERVER_DOWN);
      THROW(({"LDAP: connection closed by server.\n",backtrace()}));
      return -ldap_errno;
    }
    DWRITE(sprintf("protocol.do_op: write OK [%d bytes].\n",rv));
    msgval = 0; msgid = 0;
    writebuf= "";
    readbuf= ""; // !!! NEni to pozde ?

    //`()();

    read_answer();
    // now is all in 'readbuf'   

    return readbuf;

  }

/* ------------ legacy support -----------------*/

  string|int readmsg(int msgid) {
  // Reads LDAP PDU (with defined msgid) from server, checks msgid ...

    int msglen = 0, ix;
    string|int retv;
    string s, shlp;

    retv = ldapfd->read(2); 	// 1. byte = 0x0C, 2. byte = msglen
    if (retv == -1) {
      seterr (LDAP_TIMEOUT);
      DWRITE_HI("protocol.readmsg: ERROR: connection timeout.\n");
      THROW(({"LDAP: connection timeout.\n",backtrace()}));
      return -ldap_errno;
    }
    if (!retv || (sizeof(retv) != 2) || (retv[0] != '0')) {
      seterr (LDAP_PROTOCOL_ERROR);
      DWRITE_HI("protocol.readmsg: ERROR: retv=<"+sprintf("%O",retv)+">\n");
      THROW(({"LDAP: Protocol mismatch.\n",backtrace()}));
      return -ldap_errno;
    }
    DWRITE(sprintf("protocol.readmsg: sizeof = %d\n", sizeof(retv)));

    msglen = retv[1];
    if (msglen & 0x80) { // > 0x7f
      if (msglen == 0x80) { // RFC does not allow implicitly defined length
	seterr (LDAP_PROTOCOL_ERROR);
	THROW(({"LDAP: Protocol mismatch.\n",backtrace()}));
	return -ldap_errno;
      }
      s = ldapfd->read(msglen & 0x7f);
      if (!s) {
	seterr (LDAP_PROTOCOL_ERROR);
	THROW(({"LDAP: Protocol mismatch.\n",backtrace()}));
	return -ldap_errno;
      }
      retv += s;
      msglen = 0; // !!! RESTRICTION: 2^32 !!!
      shlp = reverse(s);
      for (ix=0; ix<sizeof(shlp); ix++) {
        msglen += shlp[ix]*(1<<(ix*8));
      }
    }
    DWRITE(sprintf("protocol.readmsg: reading %d bytes.\n", msglen));
    s = ldapfd->read(msglen);
    if (!s | (sizeof(s) < msglen)) {
      seterr (LDAP_SERVER_DOWN);
      THROW(({"LDAP: connection closed by server.\n",backtrace()}));
      return -ldap_errno;
    }
    retv += s;
    DWRITE(sprintf("protocol.readmsg: %s\n", .ldap_privates.ldap_der_decode(retv)->debug_string()));
    return retv;
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
    rv = ldapfd->write(msgval->get_der());
    // call_out
    if (rv < 2) {
      seterr (LDAP_SERVER_DOWN);
      THROW(({"LDAP: connection closed by server.\n",backtrace()}));
      return -ldap_errno;
    }
    DWRITE(sprintf("protocol.writemsg: write OK [%d bytes].\n",rv));
    msgval = 0; msgid = 0;
    return msgnum;
  }

#else
constant this_program_does_not_exist=1;
#endif
