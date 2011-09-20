//! Fork Daemon
//!
//! This is a light-weight daemon that can be used via @[Process.Process]
//! to spawn new processes (by specifying the @expr{"forkd"@} modifier).
//!
//! The typical use is when the main program is large and/or
//! when it has lots of open file descriptors. This can cause
//! considerable overhead in process creation.
//!
//! @seealso
//!   @[Process.RemoteProcess], @[Process.create_process]

//! This is a process unique @[Stdio.Fd] that is
//! used to receive remote file descriptors and
//! arguments to @[Process.create_process], as
//! well as signalling back status changes and
//! errors.
class ForkFd
{
  inherit Stdio.Fd;

  //! The remote file descriptors received so far in order.
  array(Stdio.Fd) fds = ({});

  Stdio.Fd fd_factory()
  {
    return Stdio.Fd();
  }

  void receive_fd(Stdio.Fd fd)
  {
    fds += ({ fd });
  }
}

//! This class maps 1 to 1 to Process.RemoteProcess,
//! and implements the daemon side of the RPC protocol.
class ForkStream
{
  inherit Stdio.File;

  protected void create(ForkFd fd)
  {
    _fd = fd;
  }

  void do_close(mixed|void ignored)
  {
    recv_buf = send_buf = "";
    set_blocking();
    close();
  }

  protected string send_buf = "";

  protected int state;
  protected void send_some_data()
  {
    int bytes = write(send_buf);
    if (bytes >= 0) {
      send_buf = send_buf[bytes..];
      if (!sizeof(send_buf)) {
	set_write_callback(UNDEFINED);
	if (state == 2) {
	  do_close();
	}
      }
    } else {
      close();
    }
  }

  protected void send_packet(string tag, mixed value)
  {
    string data = encode_value(({ tag, value }));
    data = sprintf("%4c%s", sizeof(data), data);
    send_buf += data;
    set_write_callback(send_some_data);
  }

  Process.create_process pid;

  protected void send_pid()
  {
    if (state || !pid) return;
    send_packet("PID", pid->pid());
    state = 1;
  }

  void send_error(mixed err)
  {
    send_packet("ERROR", err[0]);
  }

  protected void got_callback(Process.Process pid)
  {
    // Note: This function is called from what amounts to a signal
    //       context, and may thus trigger race-conditions.

    send_pid();	// Make sure that the PID is sent first of all.
    int status = pid->status();
    send_packet("SIGNAL", pid->last_signal());
    switch(status) {
    case 0: send_packet("START", 0); break;
    case 1: send_packet("STOP", 0); break;
    case 2:
      send_packet("EXIT", pid->wait());
      state = 2;
      break;
    }
  }

  string recv_buf = "";

  void got_data(mixed ignored, string data)
  {
    recv_buf += data;
    while (has_prefix(recv_buf, "\0\0\0\0")) recv_buf = recv_buf[4..];
    if (sizeof(recv_buf) < 5) return;
    int len = 0;
    sscanf(recv_buf, "%04c", len);
    if (sizeof(recv_buf) < len + 4) return;
    mixed err = catch {
	ForkFd ffd = function_object(_fd->read);
	[array(string) command_args,
	 mapping(string:mixed) modifiers] =
	  decode_value(recv_buf[4..len+3],
		       Process.ForkdDecoder(ffd->fds));
	ffd->fds = ({});

	// Adjust the modifiers.
	if (modifiers->uid == geteuid()) m_delete(modifiers, "uid");
	if (modifiers->gid == getegid()) m_delete(modifiers, "gid");
	if (equal(modifiers->setgroups, getgroups())) {
	  m_delete(modifiers, "setgroups");
	  modifiers->noinitgroups = 1;
	}
	if (equal(modifiers->env, getenv())) {
	  m_delete(modifiers, "env");
	}
	modifiers->callback = got_callback;

	pid = Process.Process(command_args, modifiers);
	send_pid();
      };
    if (err) {
      send_error(err);
    }
  }

  void start()
  {
    // Inform the initiator that we're alive and ready.
    write("\0\0\0\0");
    set_nonblocking(got_data, UNDEFINED, do_close);
  }
}

//! This is the main control @[Stdio.Fd] and is
//! always on fd number @expr{3@}.
//!
//! To spawn a new process, a new @[Stdio.PROP_SEND_FD]
//! capable @[Stdio.Fd] is sent over this fd, and a
//! single byte of data is sent as payload.
//!
//! The sent fd will become a @[ForkFd] inside a @[ForkStream].
class FdStream
{
  inherit Stdio.Fd;

  Stdio.Fd fd_factory()
  {
    ForkFd fd = ForkFd();
    return fd->_fd;
  }

  void receive_fd(Stdio.Fd fd)
  {
    ForkStream f = ForkStream((object)fd);
    f->start();
  }
}

void ignore(mixed ignored, string data) {}

void terminate() { exit(0); }

int main(int argc, array(string) argv)
{
  Stdio.File fork_file = Stdio.File();
  FdStream fd = FdStream(3, "");
  fork_file->_fd = fd->_fd;

  // Inform the dispatcher that we're up and running.
  fork_file->write("\0");

  fork_file->set_nonblocking(ignore, UNDEFINED, terminate);

  return -1;
}
