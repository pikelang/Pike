#!/usr/local/bin/pike

/* $Id: socktest.pike,v 1.37 2005/02/22 09:31:36 grubba Exp $ */

// #define OOB_DEBUG

//#define SOCK_DEBUG

import Stdio;
import String;

#if !efun(strerror)
#define strerror(X) ("ERRNO = "+(string)(X))
#endif

#ifdef SOCK_DEBUG
#define DEBUG_WERR(X...)	werror(X)
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

//int idnum;
//mapping in_cleanup=([]);

int delayed_failure = 0;

void fd_fail()
{
#if constant(Stdio.get_all_active_fd)
  array(int) fds = sort(Stdio.get_all_active_fd());

  if (sizeof(fds)) {
    werror("%d open fds:\n", sizeof(fds));
    int i;
    for(i=0; i < sizeof(fds); i++) {
      if (i) {
	werror(", %d", fds[i]);
      } else {
	werror("  %d", fds[i]);
      }
      int j;
      for (j = i; j+1 < sizeof(fds); j++) {
	if (fds[j+1] != fds[j]+1) break;
      }
      if (j != i) {
	werror(" - %d", fds[j]);
	i = j;
      }
    }
    werror("\n");
  }
#endif /* constant(Stdio.get_all_active_fd) */
  exit(1);
}

class Socket {
  import Stdio;
  import String;

  inherit File;

//  int id=idnum++;

  int num=num_running;

  string input_buffer="";

  string output_buffer="";
  string expected_data="";

  int input_finished;
  int output_finished;

  void cleanup()
  {
    if(input_finished && output_finished)
    {
      finish();
      DEBUG_WERR("Closing fd:%O\n", query_fd());
      close();
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
      werror("Failed to read complete data, errno=%d.\n",err);
      if(sizeof(input_buffer) < 100)
      {
	werror(num+":Input buffer: "+input_buffer+"\n");
      }else{
	werror(num+":Input buffer: "+sizeof(input_buffer)+" bytes.\n");
      }

      if(sizeof(expected_data) < 100)
      {
	werror(num+":Expected data: "+expected_data+"\n");
      }else{
	werror(num+":Expected data: "+sizeof(expected_data)+" bytes.\n");
      }

      exit(1);
    }

    input_finished++;
    cleanup();
  }

  void write_callback()
  {
    got_callback();
    DEBUG_WERR("write_callback[%O]: output_buffer: %O\n",
	       query_fd(), output_buffer);
    if(sizeof(output_buffer))
    {
      int tmp=write(output_buffer);
      DEBUG_WERR("write_callback(): Wrote %d bytes.\n", tmp, output_buffer);
      if(tmp >= 0)
      {
	output_buffer=output_buffer[tmp..];
      } else {
	werror("Failed to write all data.\n");
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
	werror("Failed to open socket: "+strerror(errno())+"\n");
	fd_fail();
      }
    }
    set_id(0);
    set_nonblocking(read_callback,write_callback,close_callback);
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
	werror ("Format string leak from %d to %d.\n", prerefs, _refs ("%s"));
	exit (1);
      }
      if(tmp >= 0)
      {
	output_buffer=output_buffer[tmp..];
      } else {
	werror("Failed to write all data.\n");
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
  werror("No callbacks for 20 seconds!\n");
  fd_fail();
}

int max_fds = 1024;

int counter;
int quiet;

void got_callback()
{
  counter++;
#ifdef OOB_DEBUG
  predef::write(sprintf("%c\b","|/-\\" [ counter & 3 ]));
#else /* !OOB_DEBUG */
  if(!quiet && !(counter & 0xf))
    predef::write(sprintf("%c\b","|/-\\" [ (counter>>4) & 3 ]));
#endif /* OOB_DEBUG */
  remove_call_out(die);
  call_out(die,20);
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
  werror("S");
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
  werror("G");
#endif
  got_callback();
  if (got != expected) {
    werror(sprintf("loopback: Received unexpected oob data "
		   "(0x%02x != 0x%02x)\n",
		   got[0], expected[0]));
    exit(1);
  }
  oob_loopback->set_write_oob_callback(send_oob1);
}

void send_oob1()
{
#ifdef OOB_DEBUG
  werror("s");
#endif
  got_callback();
  oob_loopback->write_oob(expected);
  oob_loopback->set_write_oob_callback(0);
  oob_originator->set_read_oob_callback(got_oob0);
}

void got_oob0(mixed ignored, string got)
{
#ifdef OOB_DEBUG
  werror("s");
#endif
  got_callback();
  if (got != expected) {
    werror(sprintf("loopback: Received unexpected oob data "
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


inherit Port : port1;
inherit Port : port2;
void create() {}

int portno1;
int portno2;

array(object(Socket)) stdtest()
{
  object sock,sock2;
  sock=Socket();
  DEBUG_WERR("Connecting to %O port %d...\n", LOOPBACK, portno2);
  if (!sock->connect(LOOPBACK, portno2)) {
#ifdef IPV6
#if constant(System.ENETUNREACH)
    if (sock->errno() == System.ENETUNREACH) {
      werror("Connect failed: Network unreachable.\n"
	     "IPv6 not configured?\n");
      exit(0);
    }
#endif /* ENETUNREACH */
#endif /* IPV6 */
    werror("Connect failed: (%d)\n", sock->errno());
    sleep(1);
    fd_fail();
  }
  DEBUG_WERR("Accepting...\n");
  sock2=port2::accept();
  if(!sock2)
  {
    werror("Accept returned 0, errno: %d\n", port2::errno());
    sleep(1);
    fd_fail();
  }
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
  sock1=File();
  if(!type)
  {
    sock1->connect(LOOPBACK, portno2);
    sock2=port2::accept();
    if(!sock2)
    {
      werror("Accept returned 0, errno: %d\n", port2::errno());
      fd_fail();
    }
  }else{
    sock2=sock1->pipe(Stdio.PROP_BIDIRECTIONAL |
		      Stdio.PROP_NONBLOCK |
		      Stdio.PROP_SHUTDOWN);
    if(!sock2)
    {
      werror("File->pipe() failed 0, errno: %d\n", sock1->errno());
      fd_fail();
    }
  }
  return ({sock1,sock2});
}

mixed keeper;

void finish()
{
  gc();
  num_running--;
  if(!num_running)
  {
    werror("\n");

    object sock1, sock2;
    array(object) socks;
    int tests;

    _tests++;
    switch(_tests)
    {
      case 1:
	werror("Testing dup & assign. ");
	sock1=stdtest()[0];
	sock1->assign(sock1->dup());
	break;
	
      case 2:
	werror("Testing accept. ");
	string data1 = "foobar" * 4711;
	for(int e=0;e<10;e++)
	{
	  sock1=Socket();
	  sock1->connect(LOOPBACK, portno1);
	  sock1->output_buffer=data1;
	}
	break;
	
      case 3:
	werror("Testing uni-directional shutdown on socket ");
	socks=spair(0);
	num_running=1;
	socks[1]->set_nonblocking(lambda() {},lambda(){},finish);
	socks[0]->close("w");
	keeper=socks;
	break;

      case 4:
	werror("Testing uni-directional shutdown on pipe ");
	socks=spair(1);
	num_running=1;
	socks[1]->set_nonblocking(lambda() {},lambda(){},finish);
	socks[0]->close("w");
	keeper=socks;
	break;

      case 5..13:
	tests=(_tests-2)*2;
	werror("Testing "+(tests*2)+" sockets. ");
	for(int e=0;e<tests;e++) stdtest();
	stdtest();
	break;

      case 14..26:
	if (max_fds > 64) {
	  /* These tests require mare than 64 open fds. */
	  tests=(_tests-2)*2;
	  werror("Testing "+(tests*2)+" sockets. ");
	  for(int e=0;e<tests;e++) stdtest();
	  stdtest();
	  break;
	}
	/* To few available fds; advance to the next set of tests. */
	_tests = 27;
	/* FALL_THROUGH */

      case 27..48:
	tests=_tests-25;
	werror("Copying "+((tests/2)*(2<<(tests/2))*11)+" bytes of data on "+(tests&~1)+" "+(tests&1?"pipes":"sockets")+" ");
	for(int e=0;e<tests/2;e++)
	{
	  string data1 = "foobar" * (2<<(tests/2));
	  string data2 = "fubar" * (2<<(tests/2));
	  socks=spair(!(tests & 1));
	  sock1=socks[0];
	  sock2=socks[1];
	  if(!sock2)
	  {
	    werror("Failed to open pipe: "+strerror(sock1->errno())+".\n");
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
	werror ("Testing leak in write(). ");
	string data1="foobar" * 20;
	string data2="fubar" * 20;
	socks=spair(1);
	sock1=socks[0];
	sock2=socks[1];
	if(!sock2)
	{
	  werror("Failed to open pipe: "+strerror(sock1->errno())+".\n");
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
	werror("Testing out-of-band data. ");
	start();
	socks = spair(0);
	oob_originator = socks[0];
	oob_loopback = socks[1];
#ifdef OOB_DEBUG
	werror("originator: %O\n"
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
      exit(delayed_failure);
      break;
    }
  }
//  werror("FINISHED with FINISH %d\n",_tests);
}

void accept_callback()
{
  object o=port1::accept();
  if(!o)
  {
    werror("Accept failed, errno: %d\n", port1::errno());
  }
  o=Socket(o);
  o->expected_data = "foobar" * 4711;
}

int main()
{
  string testargs=getenv()->TESTARGS;
  if(testargs &&
     (has_value(testargs/" ", "-q") ||
      has_value(testargs/" ", "-quiet") ) )
    quiet=1;

  werror("\nSocket test");
#ifdef IPV6
  werror(" IPv6 mode");
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
    werror("\n"
	   "Available fds: %d\n", max_fds);
  }
#endif /* constant(System.getrlimit) */

#if constant(fork)
  werror("\nForking...");
  object pid;
  if (catch { pid = fork(); }) {
    werror(" failed.\n");
  } else if (pid) {
    int res = pid->wait();
    if (res) {
      if (res == -1) {
	werror("\nChild died of signal %d\n", pid->last_signal());
      } else {
	werror("\nChild failed with errcode %d\n", res);
      }
      delayed_failure = 1;
    }
    werror("\nRunning in parent...\n");
  } else {
    werror(" ok.\n");
  }
#endif /* constant(fork) */

  int code;

  mixed err = catch {
      code = port1::bind(0, accept_callback, ANY);
    };

  if (err) {
#ifdef IPV6
    if (has_prefix(describe_error(err), "Invalid address")) {
      werror("\nIPv6 addresses not supported.\n");
      exit(0);
    }
#endif /* IPV6 */
    throw(err);
  }

  if(!code)
  {
    werror("Bind failed. (%d)\n",port1::errno());
    fd_fail();
  }
  DEBUG_WERR("port1: %O\n", port1::query_address());
  sscanf(port1::query_address(),"%*s %d",portno1);

  if(!port2::bind(0, 0, ANY))
  {
    werror("Bind failed(2). (%d)\n",port2::errno());
    fd_fail();
  }
  DEBUG_WERR("port2: %O\n", port2::query_address());
  sscanf(port2::query_address(),"%*s %d",portno2);

#ifdef OOB_DEBUG
  start();
  _tests=49;
  finish();
  return -1;
#endif /* OOB_DEBUG */

  werror("Doing simple tests. ");
  stdtest();
  return -1;
}
