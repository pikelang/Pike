#!/usr/local/bin/pike

// #define OOB_DEBUG

//#define SOCK_DEBUG

// FIXME: Ought to test all the available backends.

#if !constant(strerror)
#define strerror(X) ("ERRNO = "+(string)(X))
#endif

#ifdef SOCK_DEBUG
#define DEBUG_WERR(X...)	log_msg(X)
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
#define DO_IF_BACKEND(X) (X)
#else
#define DO_IF_BACKEND(X)
#endif

#ifndef SOCKTEST_TIMEOUT
#if constant(_debug)
// RTL debug enabled Pike.
#define SOCKTEST_TIMEOUT	60
#else
#define SOCKTEST_TIMEOUT	2
#endif
#endif

constant log_msg = Tools.Testsuite.log_msg;

int num_tests, num_failed;

string log_prefix = "";

void log_status (string msg, mixed... args)
{
  if (sizeof (args)) msg = sprintf (msg, @args);
  if (msg != "")
    msg = log_prefix + msg;
  Tools.Testsuite.log_status (msg);
}

void exit_test (int failure)
{
  if (failure) num_failed++;
  Tools.Testsuite.report_result (max (num_tests - num_failed, 0), num_failed);
  exit (failure);
}

void fd_fail (string msg, mixed... args)
{
  log_msg (msg, @args);

  array(int) fds = sort(Stdio.get_all_active_fd());
  if (sizeof(fds)) {
    log_msg("%d open fds:\n", sizeof(fds));
    int i;
    for(i=0; i < sizeof(fds); i++) {
      if (i) {
	log_msg(", %d", fds[i]);
      } else {
	log_msg("  %d", fds[i]);
      }
      int j;
      for (j = i; j+1 < sizeof(fds); j++) {
	if (fds[j+1] != fds[j]+1) break;
      }
      if (j != i) {
	log_msg(" - %d", fds[j]);
	i = j;
      }
    }
    log_msg("\n");
  }

  exit_test(1);
}

class Socket {
  Stdio.File o;

  int errno() { return o->errno(); }

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
      DEBUG_WERR("Closing fd:%O\n", o->query_fd());
      o->close();
      o->set_blocking();
      destruct(o);
      finish(this_program);
    }
  }

  string cap_size(string str)
  {
    if( sizeof(str)<100 ) return str;
    return sprintf("%d bytes.", sizeof(str));
  }

  void close_callback()
  {
    int err=o->errno();
    DEBUG_WERR("close_callback[%O]\n", o->query_fd());
    got_callback();
    if(input_buffer != expected_data)
    {
      log_msg("Failed to read complete data, errno=%d, %O.\n",
	      err, strerror(err));
      log_msg(num+":Input buffer: "+cap_size(input_buffer)+"\n");
      log_msg(num+":Expected data: "+cap_size(expected_data)+"\n");
      exit_test(1);
    }

    input_finished++;
    cleanup();
  }

  void write_callback()
  {
    DO_IF_BACKEND(o->set_backend(backend));
    got_callback();
    DEBUG_WERR("write_callback[%O]: output_buffer: %O\n",
	       o->query_fd(), output_buffer);
    if(sizeof(output_buffer))
    {
      int tmp=o->write(output_buffer);
      DEBUG_WERR("write_callback(): Wrote %d bytes.\n", tmp /*, output_buffer*/);
      if(tmp >= 0)
      {
	output_buffer=output_buffer[tmp..];
      } else {
	log_msg("Failed to write all data.\n");
	exit_test(1);
      }
    }else{
      o->set_write_callback(0);
      o->close("w");
      output_finished++;
      cleanup();
    }
  }

  void read_callback(mixed id, string foo)
  {
    got_callback();
    DEBUG_WERR("read_callback[%O]: Got %O\n", o->query_fd(), foo);
    if( !sizeof(foo) )
      fd_fail("Got empty read callback.\n");
    input_buffer+=foo;
  }

  void read_oob_callback(mixed id, string foo)
  {
    got_callback();
    fd_fail("Got unexpected out of band data on %O: %O", o->query_fd(), foo);
  }

  protected void create(object|void o)
  {
    got_callback();
    start();
    if(o)
    {
      this_program::o = o;
    }else{
      this_program::o = o = Stdio.File();
      if(!o->open_socket(0, ANY))
      {
	fd_fail("Failed to open socket: "+strerror(o->errno())+"\n");
      }
    }
    DO_IF_BACKEND(o->set_backend(backend));
    o->set_id(0);
    o->set_nonblocking(read_callback,write_callback,close_callback,
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
      int prerefs = Debug.refs ("%s");
      int tmp=o->write(({"%s"}), output_buffer);
      if (Debug.refs ("%s") != prerefs) {
	log_msg ("Format string leak from %d to %d.\n",
		 prerefs, Debug.refs ("%s"));
	exit_test (1);
      }
      if(tmp >= 0)
      {
	output_buffer=output_buffer[tmp..];
      } else {
	log_msg("Failed to write all data.\n");
	exit_test(1);
      }
    }else{
      o->set_write_callback(0);
      o->close("w");
      output_finished++;
      cleanup();
    }
  }
}

class BufferSocket {
  inherit Socket;

  void write_callback(mixed id, Stdio.Buffer out)
  {
    DO_IF_BACKEND(set_backend(backend));
    got_callback();
    DEBUG_WERR("write_callback[%O]: output_buffer: %O\n",
	       o->query_fd(), output_buffer);
    if(sizeof(output_buffer))
    {
      if(out) {
	out->add(output_buffer);
	output_buffer = "";
      } else {
	// Probably a FakeFile...
	int bytes = o->write(output_buffer);
	DEBUG_WERR("Wrote %d bytes.\n", bytes);
	output_buffer = output_buffer[bytes..];
      }
    }else{
      if (!out || !sizeof(out)) {
	  DEBUG_WERR("Closing write end.\n");
          o->set_write_callback(0);
          o->close("w");
          output_finished++;
          cleanup();
      }
    }
  }

  void read_callback(mixed id, Stdio.Buffer in)
  {
    got_callback();
    DEBUG_WERR("read_callback[%O]: Got %O\n", o->query_fd(), in);
    if( !sizeof(in) )
      fd_fail("Got empty read callback.\n");
    if (stringp(in)) {
      input_buffer += in;
    } else {
      input_buffer += in->read();
    }
  }

  void read_oob_callback(mixed id, string foo)
  {
    got_callback();
    fd_fail("Got unexpected out of band data on %O: %O", o->query_fd(), foo);
  }

  protected void create(object|void o)
  {
    ::create(o);
    if (Socket::o->set_buffer_mode) {
      Socket::o->set_buffer_mode(Stdio.Buffer(), Stdio.Buffer());
    }
  }
};

void die()
{
  fd_fail("No callbacks for %d seconds!\n", SOCKTEST_TIMEOUT);
}

int max_fds = 1024;

int counter;
int verbose;

void got_callback()
{
  counter++;
#ifdef OOB_DEBUG
  Tools.Testsuite.log_twiddler();
#else /* !OOB_DEBUG */
  if(!(counter & 0xf))
    Tools.Testsuite.log_twiddler();
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
    log_msg("loopback: Received unexpected oob data "
	    "(0x%02x != 0x%02x)\n",
	    got[0], expected[0]);
    exit_test(1);
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

void got_oob0(object socket, string got)
{
#ifdef OOB_DEBUG
  write("s");
#endif
  got_callback();
  if (got != expected) {
    log_msg("loopback: Received unexpected oob data "
	    "(0x%02x != 0x%02x)\n",
	    got[0], expected[0]);
    exit_test(1);
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
    finish(object_program(socket));
  } else {
    oob_originator->set_write_oob_callback(send_oob0);
  }
}


inherit Stdio.Port : port1;
inherit Stdio.Port : port2;
protected void create() {
#ifdef BACKEND
  port1::set_backend(backend);
  port2::set_backend(backend);
#endif
}

int portno1;
int portno2;

int protocol_supported;

array(object) stdtest(program Socket)
{
  object sock,sock2;
  int warned = 0;

  sock=Socket();
  int attempt;
  while(1)
  {
    got_callback();
    DEBUG_WERR("Connecting to %O port %d...\n", LOOPBACK, portno2);
    if (sock->o->connect(LOOPBACK, portno2)) {
      protocol_supported = 1;
      break;
    }
#if constant(System.EADDRNOTAVAIL)
    if (sock->errno() == System.EADDRNOTAVAIL) {
      if (attempt++ < 10) {
	// connect(2) on Linux intermittently fails with EADDRNOTAVAIL.
	// A retry usually solves the problem.
	// werror("Connect failure: %s\n", strerror(sock->errno()));
	continue;
      }
    }
#endif /* System.EADDRNOTAVAIL */
#ifdef IPV6
    if (!protocol_supported) {
#if constant(System.ENETUNREACH)
      if (sock->errno() == System.ENETUNREACH) {
	/* No IPv6 support on this machine (Solaris). */
	log_msg("Connect failed: Network unreachable.\n"
		"IPv6 not configured?\n");
	exit_test(0);
      }
#endif /* ENETUNREACH */
#if constant(System.EADDRNOTAVAIL)
      if (sock->errno() == System.EADDRNOTAVAIL) {
	/* No IPv6 support on this machine (OSF/1). */
	log_msg("Connect failed: Address not available.\n"
		"IPv6 not configured?\n");
	exit_test(0);
      }
#endif /* ENETUNREACH */
    }
#endif /* IPV6 */
    if (sock->errno() == System.EADDRINUSE) {
      /* Out of sockets on the loopback interface? */
      log_msg("Connect failed: Address in use. Dropping socket.\n");
      // This is supposed to let go of the socket and consider this
      // socket a success
      sock->input_finished=sock->output_finished=1;
      num_running++;	// Don't let finish end just yet.
      sock->cleanup();
      if (attempt++ < 10) {
	log_msg("Retrying.\n");
	sleep(0.1);
	num_running--;
	sock = Socket();
	continue;
      }
      finish(Socket);
      return 0;
    }
    sleep(1);
    fd_fail("Connect failed: (%d, %O)\n",
	    sock->errno(), strerror(sock->errno()));
  }
  got_callback();
  DEBUG_WERR("Accepting...\n");
  sock2=port2::accept();
  if(!sock2)
  {
    sleep(1);
    fd_fail("Accept returned 0, errno: %d, %O\n",
	    port2::errno(), strerror(port2::errno()));
  }
  DO_IF_BACKEND(sock2->set_backend(backend));
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
  DO_IF_BACKEND(sock1->set_backend(backend));

  switch(type) {
  case 0:
    sock1->connect(LOOPBACK, portno2);
    sock2=port2::accept();
    if(!sock2)
    {
      fd_fail("Accept returned 0, errno: %d, %O\n",
	      port2::errno(), strerror(port2::errno()));
    }
    break;
  case 1:
    sock2=sock1->pipe(Stdio.PROP_BIDIRECTIONAL |
		      Stdio.PROP_NONBLOCK |
		      Stdio.PROP_SHUTDOWN);
    if(!sock2)
    {
      fd_fail("File->pipe() failed 0, errno: %d, %O\n",
	      sock1->errno(), strerror(sock1->errno()));
    }
    break;
  case 2:
    sock1 = Stdio.FakePipe();
    DO_IF_BACKEND(sock1->set_backend(backend));
    sock2 = sock1->get_other();
    break;
  }
  DO_IF_BACKEND(sock2->set_backend(backend));
  return ({sock1,sock2});
}

mixed keeper;
int last_num_failed;

void finish(program Socket)
{
  gc();
  sleep(0.01);	// Allow eg Solaris some time to recycle closed fds.
  num_running--;
  if(!num_running)
  {
    object sock1, sock2;
    array(object) socks;
    int tests;

    _tests++;
    num_tests++;
#ifdef IPV6
    // Linux 2.6.x seems to hang when running out of IPV6 ports
    // on the loopback. Give it some time to clean up its act...
#if constant(uname)
    mapping u = uname();
    if( u->sysname == "Linux" && has_prefix(u->release,"2.6"))
        sleep(!(_tests&3));
#endif
#endif
    switch(_tests)
    {
      case 1:
        log_status("Beginning tests for Socket type %O", Socket);
	log_status("Testing dup & assign");
	sock1=stdtest(Socket)[0]->o;
	sock1->assign(sock1->dup());
	break;
	
      case 2:
	log_status("Testing accept");
	string data1 = long_random_string;
	for(int e=0;e<10;e++)
	{
	  sock1=Socket();
	  sock1->o->connect(LOOPBACK, portno1);
	  sock1->output_buffer=data1;
	}
	break;
	
      case 3:
	log_status("Testing uni-directional shutdown on socket");
	socks=spair(0);
	num_running=1;
	socks[1]->set_id(socks);
	socks[1]->set_nonblocking(lambda() {},lambda(){},
				  lambda(array(Stdio.File) socks) {
				    socks && socks[1]->set_id(UNDEFINED);
				    socks && socks->close();
				    finish(Socket);
				  });
	socks[0]->close("w");
	keeper=socks;
	break;

      case 4:
	log_status("Testing uni-directional shutdown on pipe");
	socks=spair(1);
	num_running=1;
	socks[1]->set_id(socks);
	socks[1]->set_nonblocking(lambda() {},lambda(){},
				  lambda(array(Stdio.File) socks) {
				    socks && socks[1]->set_id(UNDEFINED);
				    socks && socks->close();
				    finish(Socket);
				  });
	socks[0]->close("w");
	keeper=socks;
	break;

      case 5:
	log_status("Testing uni-directional shutdown on fake pipe");
	socks=spair(2);
	num_running=1;
	socks[1]->set_id(socks);
	socks[1]->set_nonblocking(lambda() {},lambda(){},
				  lambda(array(Stdio.File) socks) {
				    socks && socks[1]->set_id(UNDEFINED);
				    socks && socks->close();
				    finish(Socket);
				  });
	socks[0]->close("w");
	keeper=socks;
	break;

      case 6..14:
        if( _tests>6 && num_failed == last_num_failed )
        {
          _tests = 14;
          stdtest(Socket);
          return;
        }
        tests=(17-_tests)*2;
	log_status("Testing "+(tests*2)+" sockets");
	for(int e=0;e<tests;e++) stdtest(Socket);
        stdtest(Socket);
        break;

      case 15..27:
	if (max_fds > 64) {
          if( _tests>15 && num_failed == last_num_failed )
          {
            _tests = 27;
            stdtest(Socket);
            return;
          }
	  /* These tests require more than 64 open fds. */
          tests=(39-_tests)*2;
	  log_status("Testing "+(tests*2)+" sockets");
	  for(int e=0;e<tests;e++) stdtest(Socket);
	  stdtest(Socket);
          break;
	}
	/* Too few available fds; advance to the next set of tests. */
	_tests = 28;
	/* FALL_THROUGH */

      case 28..49:
        if( _tests>28 && num_failed == last_num_failed )
        {
          _tests = 49;
          stdtest(Socket);
          return;
        }
        tests=51-_tests;
	log_status("Copying "+((tests/2)*(2<<(tests/2))*11)+" bytes of data on "+(tests&~1)+" "+(tests&1?"pipes":"sockets"));
	for(int e=0;e<tests/2;e++)
	{
	  string data1 = random_string(6 * (2<<(tests/2)));
	  string data2 = random_string(6 * (2<<(tests/2)));
	  socks=spair(!(tests & 1));
	  sock1=socks[0];
	  sock2=socks[1];
	  if(!sock2)
	  {
	    fd_fail("Failed to open pipe: "+strerror(sock1->errno())+".\n");
	  }
	  sock1=Socket(sock1);
	  sock2=Socket(sock2);

	  sock1->output_buffer=data1;
	  sock2->expected_data=data1;

	  sock2->output_buffer=data2;
	  sock1->expected_data=data2;
        }
        break;

      case 50..55:
        if( _tests>50 && num_failed == last_num_failed )
        {
          _tests = 55;
          stdtest(Socket);
          return;
        }
        tests=57-_tests;
	log_status("Copying "+(tests*(2<<tests)*11)+" bytes of data on "+(tests*2)+" fake pipes");
	for(int e=0;e<tests;e++)
	{
	  string data1 = random_string(6 * (2<<tests));
	  string data2 = random_string(6 * (2<<tests));
	  socks=spair(2);
	  sock1=socks[0];
	  sock2=socks[1];
	  sock1=Socket(sock1);
	  sock2=Socket(sock2);

	  sock1->output_buffer=data1;
	  sock2->expected_data=data1;

	  sock2->output_buffer=data2;
	  sock1->expected_data=data2;
	}
        break;

      case 56: {
	log_status ("Testing leak in write()");
	string data1=random_string(6*20);
	string data2=random_string(6*20);
	socks=spair(1);
	sock1=socks[0];
	sock2=socks[1];
	if(!sock2)
	{
	  fd_fail("Failed to open pipe: "+strerror(sock1->errno())+".\n");
	}
	sock1=Socket(sock1);
	sock2=Socket(sock2);

	sock1->output_buffer=data1;
	sock2->expected_data=data1;

	sock2->output_buffer=data2;
	sock1->expected_data=data2;
	break;
      }

#if constant(Stdio.__OOB__)
    case 57:
      if (Stdio.__OOB__ >= 3) {
	log_status("Testing out-of-band data");
	start();
	socks = spair(0);
	oob_originator = socks[0];
	oob_loopback = socks[1];
#ifdef OOB_DEBUG
	log_msg("originator: %O\n"
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
      // log_status("");
      exit_test(0);
      break;
    }
  }
  last_num_failed = num_failed;
//  log_msg("FINISHED with FINISH %d\n",_tests);
}

string long_random_string = random_string(4711*6);

void accept_callback()
{
  object o=port1::accept();
  if(!o)
  {
    fd_fail("Accept failed, errno: %d, %O\n",
	    port1::errno(), strerror(port1::errno()));
  }
  DO_IF_BACKEND(o->set_backend(backend));
  o=Socket(o);
  o->expected_data = long_random_string;
}

int main(int argc, array(string) argv)
{
  verbose = (int) (getenv()->TEST_VERBOSITY || 2);
  int socket_test_num = argc > 1 && (int)argv[-1];

  if (verbose) {
    string greeting;
    if (socket_test_num == 2) {
        greeting = "Stdio.Buffer Socket test";
    } else if (socket_test_num) {
        greeting = "Leak check Socket test";
    } else {
        greeting = "Socket test";
    }
    DO_IF_BACKEND(greeting += " using " DEFINETOSTRING(BACKEND));
#ifdef IPV6
    greeting += " in IPv6 mode";
#endif /* IPV6 */
    log_status (greeting + "\n");
  }

#if constant(System.getrlimit)
#if SOCK_DEBUG_FDS
  System.setrlimit("nofile", 256, 256);
#endif
  array(int) file_limit = System.getrlimit("nofile");
  if (file_limit && (file_limit[0] > 0)) {
    max_fds = file_limit[0];
    if ((max_fds < 128) && (file_limit[0] < file_limit[1])) {
      // Attempt to raise the limit.
      if (System.setrlimit("nofile", file_limit[1], file_limit[1])) {
	max_fds = file_limit[1];
      }
    }
    if( verbose > 1 )
      log_msg("Available fds: %d\n", max_fds);
  }
#endif /* constant(System.getrlimit) */

#if constant(fork)
  object pid;
  num_tests++;
  if (mixed err = catch { pid = fork(); }) {
    num_failed++;
    log_msg("Fork failed: %s", describe_error (err));
  } else if (pid) {
    int res = pid->wait();
    if (res) {
      if (res == -1) {
	log_msg("Child died of signal %d\n", pid->last_signal());
      } else {
	log_msg("Child failed with errcode %d\n", res);
      }
      num_failed++;
    }
    log_prefix = "Parent: ";
  } else {
    log_prefix = "Child: ";
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
      log_msg("Bind failed: Invalid address.\n"
	      "IPv6 addresses not supported.\n");
      exit_test(0);
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
      log_msg("Bind failed: Address family not supported.\n"
	      "IPv6 not supported.\n");
      exit_test(0);
    }
#endif /* EAFNOSUPPORT */
#if constant(System.EPROTONOSUPPORT)
    if (port1::errno() == System.EPROTONOSUPPORT) {
      /* No IPv6 support on this machine (FreeBSD). */
      log_msg("Bind failed: Protocol not supported.\n"
	      "IPv6 not supported.\n");
      exit_test(0);
    }
#endif /* EAFNOSUPPORT */
#endif /* IPV6 */
    fd_fail("Bind failed. (%d, %O)\n",
	    port1::errno(), strerror(port1::errno()));
  }
  DEBUG_WERR("port1: %O\n", port1::query_address());
  sscanf(port1::query_address(),"%*s %d",portno1);

  if(!port2::bind(0, 0, ANY) || !port2::query_address())
  {
    fd_fail("Bind failed(2). (%d, %O)\n",
	    port2::errno(), strerror(port2::errno()));
  }
  DEBUG_WERR("port2: %O\n", port2::query_address());
  sscanf(port2::query_address(),"%*s %d",portno2);

#ifdef OOB_DEBUG
  start();
  _tests=49;
  finish();
#else /* !OOB_DEBUG */
  constant socket_types = ({ Socket, Socket2, BufferSocket });
  stdtest(socket_types[socket_test_num]);
#endif /* OOB_DEBUG */

#ifdef BACKEND
  while(1) {
    backend(3600.0);
  }
#else
  return -1;
#endif
}
