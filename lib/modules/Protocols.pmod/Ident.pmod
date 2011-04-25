//
// $Id$

//! An implementation of the IDENT protocol, specified in RFC 931.

#pike __REAL_VERSION__

// #define IDENT_DEBUG

constant lookup_async = AsyncLookup;

//!
class AsyncLookup
{
  Stdio.File con;

  function(array(string), mixed ...:void) callback;
  array cb_args;

  string query;
  string read_buf = "";

  static void do_callback(array(string) reply)
  {
#ifdef IDENT_DEBUG
    werror("Protocols.Ident: calling callback\n");
#endif /* IDENT_DEBUG */

    mixed err;
    if (callback) {
      err = catch {
	callback(reply, @cb_args);
      };
      callback = 0;
      cb_args = 0;
    }
    if (con) {
      con->close();
      destruct(con);
    }
    query = "";
    read_buf = "";
    con = 0;
    if (err) {
      throw(err);
    }
  }

  static void write_cb()
  {
#ifdef IDENT_DEBUG
    werror("Protocols.Ident: sending query\n");
#endif /* IDENT_DEBUG */

    int i = con->write(query);
    if (i >= 0) {
      query = query[i..];
      if (sizeof(query)) {
	return;
      }
      con->set_write_callback(0);
    } else {
      do_callback(({ "ERROR", "FAILED TO SEND REQUEST" }));
    }
  }

  static void read_cb(mixed ignored, string data)
  {
#ifdef IDENT_DEBUG
    werror("Protocols.Ident: reading data\n");
#endif /* IDENT_DEBUG */

    read_buf += data;
    int i = search(read_buf, "\r\n");
    if (i != -1) {
      string reply = read_buf[..i-1];
      read_buf = read_buf[i+1..];

      array(string) response = reply/":";
      if (sizeof(response) < 2) {
	do_callback(({ "ERROR", "BAD REPLY" }));
      } else {
	do_callback(response[1..]);
      }
    } else if (sizeof(read_buf) > 1024) {
      do_callback(({ "ERROR", "REPLY TOO LARGE" }));
    }
  }

  static void close_cb()
  {
#ifdef IDENT_DEBUG
    werror("Protocols.Ident: Connection closed\n");
#endif /* IDENT_DEBUG */

    do_callback(({ "ERROR", "CONNECTION CLOSED" }));
  }

  static void timeout()
  {
#ifdef IDENT_DEBUG
    werror("Protocols.Ident: Timeout\n");
#endif /* IDENT_DEBUG */

    do_callback(({ "ERROR", "TIMEOUT" }));
  }

  static void connected(int dummy)
  {
#ifdef IDENT_DEBUG
    werror("Protocols.Ident: Connection OK, query:%O\n", query);
#endif /* IDENT_DEBUG */
    con->set_nonblocking(read_cb, write_cb, close_cb);
  }

  //!
  void create(object fd, function(array(string), mixed ...:void) cb,
	      mixed ... args)
  {
    array(string) raddr = fd->query_address()/" ";
    array(string) laddr = fd->query_address(1)/" ";

    query = raddr[1]+","+laddr[1]+"\r\n";

    con = Stdio.File();
    if (!con->open_socket(0, laddr[0])) {
      destruct(con);
      error("open_socket() failed.\n");
    }

    callback = cb;
    cb_args = args;

    call_out(timeout, 60);

    mixed err;
    if (err = catch(con->async_connect(raddr[0], 113, connected, close_cb))) {
      callback = 0;
      cb_args = 0;
      destruct(con);
      throw(err);
    }
  }
}

//! @throws
//!   Throws exception upon any error.
array(string) lookup(object fd)
{
  array(string) raddr = fd->query_address()/" ";
  array(string) laddr = fd->query_address(1)/" ";

  Stdio.FILE remote_fd = Stdio.FILE();
  if(!remote_fd->open_socket(0, laddr[0]))
    error("open_socket() failed.\n");

  remote_fd->connect(raddr[0], 113);
  remote_fd->set_blocking();
  string query = raddr[1]+","+laddr[1]+"\r\n";
  int written;
  if((written = remote_fd->write(query)) != sizeof(query))
    error("Short write ("+written+").\n");

  string response = remote_fd->gets(); //0xefffffff, 1);
  if(!response || !sizeof(response))
    error("Read failed.\n");

  remote_fd->close();
  destruct(remote_fd);
  response -= " ";
  response -= "\r";
  array(string) ret = response / ":";
  if(sizeof(ret) < 2)
    error("Malformed response.\n");
  return ret[1..];
}
