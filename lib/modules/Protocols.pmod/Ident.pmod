// An implementation of the IDENT protocol, specified in RFC 931.
//
// $Id: Ident.pmod,v 1.10 2003/01/20 17:44:00 nilsson Exp $

#pike __REAL_VERSION__

// #define IDENT_DEBUG

class lookup_async
{
  object con;

  function(array(string), mixed ...:void) callback;
  array cb_args;

  string query;
  string read_buf = "";

  void do_callback(array(string) reply)
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

  void write_cb()
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
  
  void read_cb(mixed ignored, string data)
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

  void close_cb()
  {
#ifdef IDENT_DEBUG
    werror("Protocols.Ident: Connection closed\n");
#endif /* IDENT_DEBUG */

    do_callback(({ "ERROR", "CONNECTION CLOSED" }));
  }

  void timeout()
  {
#ifdef IDENT_DEBUG
    werror("Protocols.Ident: Timeout\n");
#endif /* IDENT_DEBUG */

    do_callback(({ "ERROR", "TIMEOUT" }));
  }

  void connected()
  {
#ifdef IDENT_DEBUG
    werror(sprintf("Protocols.Ident: Connection OK, query:%O\n", query));
#endif /* IDENT_DEBUG */
    con->set_nonblocking(read_cb, write_cb, close_cb);
  }

  void create(object fd, function(array(string), mixed ...:void) cb,
	      mixed ... args)
  {
    string|array(string) raddr = fd->query_address();
    string|array(string) laddr = fd->query_address(1);

    if(!raddr || !laddr) {
      // Does this ever happen?
      error("Protocols.Ident - cannot lookup address");
    }

    laddr = laddr / " ";
    raddr = raddr / " ";

    query = raddr[1]+","+laddr[1]+"\r\n";

    con = Stdio.File();
    if (!con->open_socket(0, laddr[0])) {
      destruct(con);
      error("Protocols.Ident: open_socket() failed.");
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

int|array (string) lookup(object fd)
{
  mixed raddr; // Remote Address.
  mixed laddr; // Local Address.
  array err;
  object remote_fd;
  int i;
  if(!fd)
    return 0;
  err = catch(raddr = fd->query_address());
  if(err)
    throw(err + ({"Error in Protocols.Ident:"}));
  err = catch(laddr = fd->query_address(1));
  if(err)
    throw(err + ({"Error in Protocols.Ident:" }));
  if(!raddr || !laddr)
    throw(backtrace() +({ "Protocols.Ident - cannot lookup address"}));

  laddr = laddr / " ";
  raddr = raddr / " ";

  remote_fd = Stdio.FILE();
  if(!remote_fd->open_socket(0, laddr[0])) {
    destruct(remote_fd);
    throw(backtrace() +({ "Protocols.Ident: open_socket() failed."}));
  }

  if(err = catch(remote_fd->connect(raddr[0], 113)))
  {
    destruct(remote_fd);
    throw(err);
  }
  remote_fd->set_blocking();
  string query = raddr[1]+","+laddr[1]+"\r\n";
  int written;
  if((written = remote_fd->write(query)) != sizeof(query)) {
    destruct(remote_fd);
    throw(backtrace() +({ "Protocols.Ident: short write ("+written+")."}));
  }
  mixed response = remote_fd->gets();//0xefffffff, 1);
  if(!response || !sizeof(response))
  {
    destruct(remote_fd);
    throw(backtrace() +({ "Protocols.Ident: read failed."}));
  }
  remote_fd->close();
  destruct(remote_fd);
  response -= " ";
  response -= "\r";
  response /= ":";
  if(sizeof(response) < 2)
    return ({ "ERROR", "UNKNOWN-ERROR" });
  return response[1..];
}
