#!/usr/local/bin/pike

/* $Id: socktest.pike,v 1.58 2010/02/21 13:28:17 srb Exp $ */

// #define OOB_DEBUG

//#define SOCK_DEBUG

// FIXME: Ought to test all the available backends.

#if !efun(strerror)
#define strerror(X) ("ERRNO = "+(string)(X))
#endif

#ifdef SOCK_DEBUG
#define DEBUG_WERR(X...)	predef::write(X)
#else /* !SOCK_DEBUG */
#define DEBUG_WERR(X...)
#endif /* SOCK_DEBUG */

#ifdef IPV6
/* Run in IPv6 mode. */
#define ANY		"::"
#define LOOPBACK	"::1"
#else /* !IPV6 */
/* Run in IPv4 mode. */
#define ANY		0
#define LOOPBACK	"127.0.0.1"
#endif /* IPV6 */

#define TOSTR(X)		#X
#define DEFINETOSTRING(X)	TOSTR(X)

#ifdef BACKEND
Pike.BACKEND backend = Pike.BACKEND();
#endif

#ifndef SOCKTEST_TIMEOUT
#if constant(_debug)
// RTL debug enabled Pike.
#define SOCKTEST_TIMEOUT	60
#else
#define SOCKTEST_TIMEOUT	20
#endif
#endif

//int idnum;
//mapping in_cleanup=([]);

int delayed_failure = 0;

void fd_fail()
{
#if constant(Stdio.get_all_active_fd)
  array(int) fds = sort(Stdio.get_all_active_fd());

  if (sizeof(fds)) {
    write("%d open fds:\n", sizeof(fds));
    int i;
    for(i=0; i < sizeof(fds); i++) {
      if (i) {
	write(", %d", fds[i]);
      } else {
	write("  %d", fds[i]);
      }
      int j;
      for (j = i; j+1 < sizeof(fds); j++) {
	if (fds[j+1] != fds[j]+1) break;
      }
      if (j != i) {
	write(" - %d", fds[j]);
	i = j;
      }
    }
    write("\n");
  }
#endif /* constant(Stdio.get_all_active_fd) */
  exit(1);
}

class Socket {
  inherit Stdio.File;

//  int id=idnum++;

  int num=num_running;

  string input_buffer="";

  string output_buffer="";
  string expected_data="";

  int input_finished;
  int output_finished;

  void cleanup()
  {
    DEBUG_WERR("Cleanup: Input: %d, Output: %d\n",
	       input_finished, output_finished);
    if(input_finished && output_finished)
    {
      finish();
      DEBUG_WERR("Closing fd:%O\n", query_fd());
      close();
      set_blocking();
      destruct(this_object());
    }
  }

  void close_callback()
  {
    int err=errno();
    DEBUG_WERR("close_callback[%O]\n", query_fd());
    got_callback();
    if(input_buffer != expected_data)
    {
      predef::write("Failed to read complete data, errno=%d, %O.\n",
		    err, strerror(err));
      if(sizeof(input_buffer) < 100)
      {
	predef::write(num+":Input buffer: "+input_buffer+"\n");
      }else{
	predef::write(num+":Input buffer: "+sizeof(input_buffer)+" bytes.\n");
      }

      if(sizeof(expected_data) < 100)
      {
	predef::write(num+":Expected data: "+expected_data+"\n");
      }else{
	predef::write(num+":Expected data: "+sizeof(expected_data)+" bytes.\n");
      }

      exit(1);
    }

    input_finished++;
    cleanup();
  }

  void write_callback()
  {
#ifdef BACKEND
    set_backend(backend);
#endif
    got_callback();
    DEBUG_WERR("write_callback[%O]: output_buffer: %O\n",
	       query_fd(), output_buffer);
    if(sizeof(output_buffer))
    {
      int tmp=write(output_buffer);
      DEBUG_WERR("write_callback(): Wrote %d bytes.\n", tmp /*, output_buffer*/);
      if(tmp >= 0)
      {
	output_buffer=output_buffer[tmp..];
      } else {
	predef::write("Failed to write all data.\n");
	exit(1);
      }
    }else{
      set_write_callback(0);
      close("w");
      output_finished++;
      cleanup();
    }
  }

  void read_callback(mixed id, string foo)
  {
    got_callback();
    DEBUG_WERR("read_callback[%O]: Got %O\n", query_fd(), foo);
    input_buffer+=foo;
  }

  void read_oob_callback(mixed id, string foo)
  {
    got_callback();
    predef::write("Got unexpected out of band data on %O: %O", query_fd(), foo);
    fd_fail();
  }

  void create(object|void o)
  {
    got_callback();
    start();
    if(o)
    {
      o->set_id(0);
      assign(o);
      destruct(o);
    }else{
      if(!open_socket(0, ANY))
      {
	predef::write("Failed to open socket: "+strerror(errno())+"\n");
	fd_fail();
      }
    }
#ifdef BACKEND
    set_backend(backend);
#endif
    set_id(0);
    set_nonblocking(read_callback,write_callback,close_callback,
		    read_oob_callback, UNDEFINED);
  }
};

class Socket2
{
  inherit Socket;

  void write_callback()
  {
    got_callback();
    if(sizeof(output_buffer))
    {
      int prerefs = _refs ("%s");
      int tmp=write(({"%s"}), output_buffer);
      if (_refs ("%s") != prerefs) {
	predef::write ("Format string leak from %d to %d.\n", prerefs, _refs ("%s"));
	exit (1);
      }
      if(tmp >= 0)
      {
	output_buffer=output_buffer[tmp..];
      } else {
	predef::write("Failed to write all data.\n");
	exit(1);
      }
    }else{
      set_write_callback(0);
      close("w");
      output_finished++;
      cleanup();
    }
  }
}

void die()
{
  write("No callbacks for %d seconds!\n", SOCKTEST_TIMEOUT);
  fd_fail();
}

int max_fds = 1024;

int counter;
int verbose;

void got_callback()
{
  counter++;
#ifdef OOB_DEBUG
  predef::write(sprintf("%c\b","|/-\\" [ counter & 3 ]));
#else /* !OOB_DEBUG */
  if(verbose && !(counter & 0xf))
    predef::write(sprintf("%c\b","|/-\\" [ (counter>>4) & 3 ]));
#endif /* OOB_DEBUG */
#ifdef BACKEND
  backend->remove_call_out(die);
  backend->call_out(die, SOCKTEST_TIMEOUT);
#else
  remove_call_out(die);
  call_out(die, SOCKTEST_TIMEOUT);
#endif
}

int num_running;
int _tests;

void start()
{
  num_running++;
}


object oob_loopback;
object oob_originator;
string expected;
int oob_sent;

void send_oob0()
{
#ifdef OOB_DEBUG
  write("S");
#endif
  got_callback();
  expected = sprintf("%c", random(256));
  oob_originator->write_oob(expected);
  oob_originator->set_write_oob_callback(0);
  oob_loopback->set_read_oob_callback(got_oob1);
}

void got_oob1(mixed ignored, string got)
{
#ifdef OOB_DEBUG
  write("G");
#endif
  got_callback();
  if (got != expected) {
    write(sprintf("\nloopback: Received unexpected oob data "
		   "(0x%02x != 0x%02x)\n",
		   got[0], expected[0]));
    exit(1);
  }
  oob_loopback->set_write_oob_callback(send_oob1);
}

void send_oob1()
{
#ifdef OOB_DEBUG
  write("s");
#endif
  got_callback();
  oob_loopback->write_oob(expected);
  oob_loopback->set_write_oob_callback(0);
  oob_originator->set_read_oob_callback(got_oob0);
}

void got_oob0(mixed ignored, string got)
{
#ifdef OOB_DEBUG
  write("s");
#endif
  got_callback();
  if (got != expected) {
    write(sprintf("\nloopback: Received unexpected oob data "
		   "(0x%02x != 0x%02x)\n",
		   got[0], expected[0]));
    exit(1);
  }
  oob_sent++;
  if (oob_sent >
#ifdef OOB_DEBUG
      5
#else /* !OOB_DEBUG */
      511
#endif /* OOB_DEBUG */
      ) {
    oob_loopback->set_blocking();
    oob_loopback->close();
    oob_originator->set_blocking();
    oob_originator->close();
    finish();
  } else {
    oob_originator->set_write_oob_callback(send_oob0);
  }
}


inherit Stdio.Port : port1;
inherit Stdio.Port : port2;
void create() {
#ifdef BACKEND
  port1::set_backend(backend);
  port2::set_backend(backend);
#endif
}

int portno1;
int portno2;

array(object(Socket)) stdtest()
{
  object sock,sock2;
  int warned = 0;

  sock=Socket();
  do
  {
    got_callback();
    DEBUG_WERR("Connecting to %O port %d...\n", LOOPBACK, portno2);
    if (sock->connect(LOOPBACK, portno2)) {
      break;
    }
#ifdef IPV6
#if constant(System.ENETUNREACH)
    if (sock->errno() == System.ENETUNREACH) {
      /* No IPv6 support on this machine (Solaris). */
      write("Connect failed: Network unreachable.\n"
	    "IPv6 not configured?\n");
      exit(0);
    }
#endif /* ENETUNREACH */
#if constant(System.EADDRNOTAVAIL)
    if (sock->errno() == System.EADDRNOTAVAIL) {
      /* No IPv6 support on this machine (OSF/1). */
      write("Connect failed: Address not available.\n"
	    "IPv6 not configured?\n");
      exit(0);
    }
#endif /* ENETUNREACH */
#endif /* IPV6 */
    if (sock->errno() == System.EADDRINUSE) {
      /* Out of sockets on the loopback interface? */
      write("\nConnect failed: Address in use. Dropping socket.\n");
      // This is supposed to let go of the socket and consider this
      // socket a success
      sock->input_finished=sock->output_finished=1;
      sock->cleanup();
      return 0;
    }
    write("\nConnect failed: (%d, %O)\n",
	  sock->errno(), strerror(sock->errno()));
    sleep(1);
    fd_fail();
  }
  while(0);
  got_callback();
  DEBUG_WERR("Accepting...\n");
  sock2=port2::accept();
  if(!sock2)
  {
    write("Accept returned 0, errno: %d, %O\n",
	  port2::errno(), strerror(port2::errno()));
    sleep(1);
    fd_fail();
  }
#ifdef BACKEND
  sock2->set_backend(backend);
#endif
  DEBUG_WERR("Socket connected: %O <==> %O\n",
	     sock2->query_address(1),
	     sock2->query_address());
  sock2=Socket(sock2);
  sock->output_buffer="foo";
  sock2->expected_data="foo";

  return ({sock,sock2});
}

array(object) spair(int type)
{
  object sock1,sock2;
  sock1=Stdio.File();
#ifdef BACKEND
  sock1->set_backend(backend);
#endif
  if(!type)
  {
    sock1->connect(LOOPBACK, portno2);
    sock2=port2::accept();
    if(!sock2)
    {
      write("Accept returned 0, errno: %d, %O\n",
	    port2::errno(), strerror(port2::errno()));
      fd_fail();
    }
  }else{
    sock2=sock1->pipe(Stdio.PROP_BIDIRECTIONAL |
		      Stdio.PROP_NONBLOCK |
		      Stdio.PROP_SHUTDOWN);
    if(!sock2)
    {
      write("File->pipe() failed 0, errno: %d, %O\n",
	    sock1->errno(), strerror(sock1->errno()));
      fd_fail();
    }
  }
#ifdef BACKEND
  sock2->set_backend(backend);
#endif
  return ({sock1,sock2});
}

mixed keeper;

void test_write(string str)
{
  if( verbose )
    write("\n" + str);
  else
    write("\r                                                       \r" + str);
  //  else
  //    write("Test %d\r", _tests);
}

void finish()
{
  gc();
  num_running--;
  if(!num_running)
  {
    object sock1, sock2;
    array(object) socks;
    int tests;

    _tests++;
#ifdef IPV6
    // Linux 2.6.x seems to hang when running out of IPV6 ports
    // on the loopback. Give it some time to clean up its act...
    sleep(!(_tests&3));
#endif
    switch(_tests)
    {
      case 1:
	test_write("Testing dup & assign. ");
	sock1=stdtest()[0];
	sock1->assign(sock1->dup());
	break;
	
      case 2:
	test_write("Testing accept. ");
	string data1 = "foobar" * 4711;
	for(int e=0;e<10;e++)
	{
	  sock1=Socket();
	  sock1->connect(LOOPBACK, portno1);
	  sock1->output_buffer=data1;
	}
	break;
	
      case 3:
	test_write("Testing uni-directional shutdown on socket ");
	socks=spair(0);
	num_running=1;
	socks[1]->set_nonblocking(lambda() {},lambda(){},finish);
	socks[0]->close("w");
	keeper=socks;
	break;

      case 4:
	test_write("Testing uni-directional shutdown on pipe ");
	socks=spair(1);
	num_running=1;
	socks[1]->set_nonblocking(lambda() {},lambda(){},finish);
	socks[0]->close("w");
	keeper=socks;
	break;

      case 5..13:
	tests=(_tests-2)*2;
	test_write("Testing "+(tests*2)+" sockets. ");
	for(int e=0;e<tests;e++) stdtest();
	stdtest();
	break;

      case 14..26:
	if (max_fds > 64) {
	  /* These tests require mare than 64 open fds. */
	  tests=(_tests-2)*2;
	  test_write("Testing "+(tests*2)+" sockets. ");
	  for(int e=0;e<tests;e++) stdtest();
	  stdtest();
	  break;
	}
	/* Too few available fds; advance to the next set of tests. */
	_tests = 27;
	/* FALL_THROUGH */

      case 27..48:
	tests=_tests-25;
	test_write("Copying "+((tests/2)*(2<<(tests/2))*11)+" bytes of data on "+(tests&~1)+" "+(tests&1?"pipes":"sockets")+" ");
	for(int e=0;e<tests/2;e++)
	{
	  string data1 = "foobar" * (2<<(tests/2));
	  string data2 = "fubar" * (2<<(tests/2));
	  socks=spair(!(tests & 1));
	  sock1=socks[0];
	  sock2=socks[1];
	  if(!sock2)
	  {
	    write("Failed to open pipe: "+strerror(sock1->errno())+".\n");
	    fd_fail();
	  }
	  sock1=Socket(sock1);
	  sock2=Socket(sock2);
	  
	  sock1->output_buffer=data1;
	  sock2->expected_data=data1;
	  
	  sock2->output_buffer=data2;
	  sock1->expected_data=data2;
	}
	break;

      case 49: {
	test_write ("Testing leak in write(). ");
	string data1="foobar" * 20;
	string data2="fubar" * 20;
	socks=spair(1);
	sock1=socks[0];
	sock2=socks[1];
	if(!sock2)
	{
	  write("Failed to open pipe: "+strerror(sock1->errno())+".\n");
	  fd_fail();
	}
	sock1=Socket2(sock1);
	sock2=Socket2(sock2);
	  
	sock1->output_buffer=data1;
	sock2->expected_data=data1;
	  
	sock2->output_buffer=data2;
	sock1->expected_data=data2;
	break;
      }

#if constant(Stdio.__OOB__)
    case 50:
      if (Stdio.__OOB__ >= 3) {
	test_write("Testing out-of-band data. ");
	start();
	socks = spair(0);
	oob_originator = socks[0];
	oob_loopback = socks[1];
#ifdef OOB_DEBUG
	write("originator: %O\n"
	       "loopback: %O\n",
	       oob_originator,
	       oob_loopback);
#endif /* OOB_DEBUG */
      
	socks[0]->set_nonblocking(0,0,0,got_oob0,send_oob0);
	socks[1]->set_nonblocking(0,0,0,got_oob1,0);
	break;
      }
#endif /* constant(Stdio.__OOB__) */

    default:
      write("\n");
      exit(delayed_failure);
      break;
    }
  }
//  write("FINISHED with FINISH %d\n",_tests);
}

void accept_callback()
{
  object o=port1::accept();
  if(!o)
  {
    write("Accept failed, errno: %d, %O\n",
	  port1::errno(), strerror(port1::errno()));
  }
#ifdef BACKEND
  o->set_backend(backend);
#endif
  o=Socket(o);
  o->expected_data = "foobar" * 4711;
}

int main(int argc, array(string) argv)
{
  string testargs=getenv()->TESTARGS;
  argv += testargs?(testargs/" "):({});
  if(has_value(argv[1..], "-v") || has_value(argv[1..], "--verbose"))
    verbose=1;

  write("\nSocket test");
#ifdef BACKEND
  write(" using " DEFINETOSTRING(BACKEND));
#endif
#ifdef IPV6
  write(" in IPv6 mode");
#endif /* IPV6 */

#if constant(System.getrlimit)
  array(int) file_limit = System.getrlimit("nofile");
  if (file_limit && (file_limit[0] > 0)) {
    max_fds = file_limit[0];
    if ((max_fds < 128) && (file_limit[0] < file_limit[1])) {
      // Attempt to raise the limit.
      if (System.setrlimit("nofile", file_limit[1], file_limit[1])) {
	max_fds = file_limit[1];
      }
    }
    if( verbose )
      write("\n"
            "Available fds: %d\n", max_fds);
  }
#endif /* constant(System.getrlimit) */

#if constant(fork)
  write("\nForking...");
  object pid;
  if (catch { pid = fork(); }) {
    write(" failed.\n");
  } else if (pid) {
    int res = pid->wait();
    if (res) {
      if (res == -1) {
	write("\nChild died of signal %d\n", pid->last_signal());
      } else {
	write("\nChild failed with errcode %d\n", res);
      }
      delayed_failure = 1;
    }
    write("\nRunning in parent...\n");
  } else {
    write(" ok.\n");
  }
#endif /* constant(fork) */

  int code;

  mixed err = catch {
      code = port1::bind(0, accept_callback, ANY);
    };

  if (err) {
#ifdef IPV6
    if (has_prefix(describe_error(err), "Invalid address")) {
      /* No IPv6 support at all. */
      write("\nBind failed: Invalid address.\n"
	     "IPv6 addresses not supported.\n");
      exit(0);
    }
#endif /* IPV6 */
    throw(err);
  }

  if(!code || !port1::query_address())
  {
#ifdef IPV6
#if constant(System.EAFNOSUPPORT)
    if (port1::errno() == System.EAFNOSUPPORT) {
      /* No IPv6 support on this machine (Linux). */
      write("\nBind failed: Address family not supported.\n"
	     "IPv6 not supported.\n");
      exit(0);      
    }
#endif /* EAFNOSUPPORT */
#if constant(System.EPROTONOSUPPORT)
    if (port1::errno() == System.EPROTONOSUPPORT) {
      /* No IPv6 support on this machine (FreeBSD). */
      write("\nBind failed: Protocol not supported.\n"
	     "IPv6 not supported.\n");
      exit(0);      
    }
#endif /* EAFNOSUPPORT */
#endif /* IPV6 */
    write("Bind failed. (%d, %O)\n", port1::errno(), strerror(port1::errno()));
    fd_fail();
  }
  DEBUG_WERR("port1: %O\n", port1::query_address());
  sscanf(port1::query_address(),"%*s %d",portno1);

  if(!port2::bind(0, 0, ANY) || !port2::query_address())
  {
    write("Bind failed(2). (%d, %O)\n",
	  port2::errno(), strerror(port2::errno()));
    fd_fail();
  }
  DEBUG_WERR("port2: %O\n", port2::query_address());
  sscanf(port2::query_address(),"%*s %d",portno2);

#ifdef OOB_DEBUG
  start();
  _tests=49;
  finish();
#else /* !OOB_DEBUG */
  test_write("\nDoing simple tests. ");
  stdtest();
#endif /* OOB_DEBUG */

#ifdef BACKEND
  while(1) {
    backend(3600.0);
  }
#else
  return -1;
#endif
}
