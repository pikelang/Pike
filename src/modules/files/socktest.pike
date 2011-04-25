#!/usr/local/bin/pike

/* $Id$ */


import Stdio;
import String;

#if !efun(strerror)
#define strerror(X) ("ERRNO = "+(string)(X))
#endif

//int idnum;
//mapping in_cleanup=([]);

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
      close();
      destruct(this_object());
    }
  }

  void close_callback()
  {
    int err=errno();
    got_callback();
    if(input_buffer != expected_data)
    {
      werror("Failed to read complete data, errno=%d.\n",err);
      if(strlen(input_buffer) < 100)
      {
	werror(num+":Input buffer: "+input_buffer+"\n");
      }else{
	werror(num+":Input buffer: "+strlen(input_buffer)+" bytes.\n");
      }

      if(strlen(expected_data) < 100)
      {
	werror(num+":Expected data: "+expected_data+"\n");
      }else{
	werror(num+":Expected data: "+strlen(expected_data)+" bytes.\n");
      }

      exit(1);
    }

    input_finished++;
    cleanup();
  }

  void write_callback()
  {
    got_callback();
    if(strlen(output_buffer))
    {
      int tmp=write(output_buffer);
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
      if(!open_socket())
      {
	werror("Failed to open socket: "+strerror(errno())+"\n");
	exit(1);
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
    if(strlen(output_buffer))
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
  exit(1);
}

int counter;
int quiet;

void got_callback()
{
  counter++;
  if(!quiet && !(counter & 0xf))
    predef::write(sprintf("%c\b","|/-\\" [ (counter>>4) & 3 ]));
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
  got_callback();
  expected = sprintf("%c", random(256));
  oob_originator->write_oob(expected);
  oob_originator->set_write_oob_callback(0);
  oob_loopback->set_read_oob_callback(got_oob1);
}

void got_oob1(mixed ignored, string got)
{
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
  got_callback();
  oob_loopback->write_oob(expected);
  oob_loopback->set_write_oob_callback(0);
  oob_originator->set_read_oob_callback(got_oob0);
}

void got_oob0(mixed ignored, string got)
{
  got_callback();
  if (got != expected) {
    werror(sprintf("loopback: Received unexpected oob data "
		   "(0x%02x != 0x%02x)\n",
		   got[0], expected[0]));
    exit(1);
  }
  oob_sent++;
  if (oob_sent > 511) {
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

int portno1;
int portno2;

array(object(Socket)) stdtest()
{
  object sock,sock2;
  sock=Socket();
  sock->connect("127.0.0.1",portno2);
  sock2=port2::accept();
  if(!sock2)
  {
    werror("Accept returned 0\n");
    sleep(1);
    exit(1);
  }
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
    sock1->connect("127.0.0.1",portno2);
    sock2=port2::accept();
    if(!sock2)
    {
      werror("Accept returned 0\n");
      exit(1);
    }
  }else{
    sock2=sock1->pipe(Stdio.PROP_BIDIRECTIONAL |
		      Stdio.PROP_NONBLOCK |
		      Stdio.PROP_SHUTDOWN);
    if(!sock2)
    {
      werror("File->pipe() failed 0\n");
      exit(1);
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
	  sock1->connect("127.0.0.1",portno1);
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

      case 5..26:
	tests=(_tests-2)*2;
	werror("Testing "+(tests*2)+" sockets. ");
	for(int e=0;e<tests;e++) stdtest();
	stdtest();
	break;

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
	    exit(1);
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
	  exit(1);
	}
	sock1=Socket2(sock1);
	sock2=Socket2(sock2);
	  
	sock1->output_buffer=data1;
	sock2->expected_data=data1;
	  
	sock2->output_buffer=data2;
	sock1->expected_data=data2;
	break;
      }

#if Stdio.__OOB__ >= 3
    case 50:
      werror("Testing out-of-band data. ");
      start();
      socks = spair(0);
      oob_originator = socks[0];
      oob_loopback = socks[1];
      
      socks[0]->set_nonblocking(0,0,0,got_oob0,send_oob0);
      socks[1]->set_nonblocking(0,0,0,got_oob1,0);
      break;
#endif /* __HAVE_OOB__ */

    default:
      exit(0);
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
    werror("Accept failed");
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

  if(!port1::bind(0, accept_callback))
  {
    werror("Bind failed. (%d)\n",port1::errno());
    exit(1);
  }
  sscanf(port1::query_address(),"%*s %d",portno1);

  if(!port2::bind(0))
  {
    werror("Bind failed(2). (%d)\n",port2::errno());
    exit(1);
  }

  sscanf(port2::query_address(),"%*s %d",portno2);

  werror("\nDoing simple tests. ");
  stdtest();
  return -1;
}
