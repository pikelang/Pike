#pike __REAL_VERSION__

//!
//! This module emulates a bidirectional pipe/socket
//! without using any actual file descriptors.
//!

#ifndef DEFAULT_WRITE_LIMIT
#define DEFAULT_WRITE_LIMIT	0x10000
#endif

//! Class that implements one end of an emulated  bi-directional pipe/socket.
protected class InternalSocket( protected this_program other,
				protected Stdio.Buffer read_buffer,
				protected Stdio.Buffer write_buffer,
				protected Thread.Mutex mux,
				protected Thread.Condition cond )
{
  protected int internal_errno = 0;

  //!
  int errno()
  {
    return internal_errno;
  }

  //!
  int(0..1) is_open(string|void direction)
  {
    switch(direction || "rw") {
    case "r":
      return !!read_buffer;
    case "w":
      return !!write_buffer;
    default:
      return !!(read_buffer || write_buffer);
    }
  }

  protected int write_limit = DEFAULT_WRITE_LIMIT;

  // Internal.
  //
  // Increase the write buffer limit to the size of a blocking read()
  // that is in progress.
  void set_write_limit(int limit)
  {
    if (limit >= DEFAULT_WRITE_LIMIT) {
      write_limit = limit;
      schedule_poll();
    }
  }

  // Callback handling.

  protected Pike.Backend backend;
  protected mixed poll_co;

  //!
  Pike.Backend query_backend()
  {
    return backend;
  }

  //!
  void set_backend(Pike.Backend be)
  {
    Thread.MutexKey key = mux->lock();
    if (poll_co) {
      (backend || Pike.DefaultBackend)->remove_call_out(poll_co);
      poll_co = UNDEFINED;
    }
    backend = be;
    destruct(key);
    schedule_poll();
  }

  protected function __read_cb;
  protected function __write_cb;
  protected function __close_cb;
  protected mixed __id;

  protected void internal_poll()
  {
    poll_co = UNDEFINED;
    if (__read_cb) {
      if (read_buffer && sizeof(read_buffer)) {
	__read_cb(__id, read_buffer->read());
	other->schedule_poll();
	schedule_poll();
	return;
      } else if (__close_cb) {
	if (!other->is_open("w")) {
	  __close_cb(__id);
	  schedule_poll();
	  return;
	}
      }
    }
    if (__write_cb) {
      if (write_buffer &&
	  ((sizeof(write_buffer) < write_limit) || !other->is_open("r"))) {
	__write_cb(__id);
	schedule_poll();
	return;
      }
    }
  }

  // Internal
  //
  // Wake up any thread that waits on this connection,
  // and schedule a poll for calling of callbacks.
  void schedule_poll()
  {
    Thread.MutexKey key = mux->lock(2);
    cond->broadcast();
    if (!poll_co) {
      poll_co = (backend || Pike.DefaultBackend)->call_out(internal_poll, 0);
    }
  }

  protected enum State {
    STATE_NONBLOCKING		= 1,
  }

  protected State state = 0;

  //!
  void set_nonblocking_keep_callbacks()
  {
    state |= STATE_NONBLOCKING;

    schedule_poll();
  }

  //!
  void set_callbacks(function rcb, function wcb, function ccb)
  {
    Thread.MutexKey key = mux->lock();
    [__read_cb, __write_cb, __close_cb] = ({ rcb, wcb, ccb });
    destruct(key);

    schedule_poll();
  }

  //!
  void set_nonblocking(function rcb, function wcb, function ccb)
  {
    set_nonblocking_keep_callbacks();

    set_callbacks(rcb, wcb, ccb);
  }

  //!
  void set_blocking()
  {
    state &= ~STATE_NONBLOCKING;
  }

  //!
  void set_id(mixed id)
  {
    __id = id;
  }

  //!
  void set_read_callback(function rcb)
  {
    if (__read_cb = rcb) {
      schedule_poll();
    }
  }

  //!
  void set_write_callback(function wcb)
  {
    if (__write_cb = wcb) {
      schedule_poll();
    }
  }

  //!
  void set_close_callback(function ccb)
  {
    if (__close_cb = ccb) {
      schedule_poll();
    }
  }

  //!
  mixed get_id()
  {
    return __id;
  }

  //!
  function get_read_callback()
  {
    return __read_cb;
  }

  //!
  function get_write_callback()
  {
    return __write_cb;
  }

  //!
  function get_close_callback()
  {
    return __close_cb;
  }

  // I/O.

  //!
  int close(string|void direction)
  {
    switch(direction || "rw") {
    case "r":
      if (!read_buffer) {
	internal_errno = System.EBADF;
	return -1;
      }
      read_buffer = UNDEFINED;
      break;
    case "w":
      if (!write_buffer) {
	internal_errno = System.EBADF;
	return -1;
      }
      write_buffer = UNDEFINED;
      break;
    default:
      if (!read_buffer && !write_buffer) {
	internal_errno = System.EBADF;
	return -1;
      }
      read_buffer = write_buffer = UNDEFINED;
      break;
    }
    other->schedule_poll();
    return 0;
  }

  //!
  string(8bit) read(int|void nbytes, bool|void not_all)
  {
    if (!nbytes) nbytes = 0x7fffffff;
    Stdio.Buffer ret;
    Thread.MutexKey key = mux->lock();
    while(1) {
      if (!read_buffer) {
	// Read direction closed.
	internal_errno = System.EBADF;
	return 0;
      }
      if (nbytes <= sizeof(read_buffer)) {
	ret = read_buffer->read_buffer(nbytes);
	break;
      }
      if (not_all || (state & STATE_NONBLOCKING) || (!other->is_open("w"))) {
	ret = read_buffer;
	break;
      }
      if (nbytes > DEFAULT_WRITE_LIMIT) {
	other->set_write_limit(nbytes);
	other->schedule_poll();
      }
      cond->wait(key);
    }
    other->set_write_limit(DEFAULT_WRITE_LIMIT);
    return ret->read();
  }

  //!
  int write(sprintf_format|array(string(8bit)) data,
	    sprintf_args ...args)
  {
    if (!write_buffer) {
      // Write direction closed.
      internal_errno = System.EBADF;
      return -1;
    }
    if (!other->is_open("r")) {
      // Other end has closed.
      internal_errno = System.EPIPE;
      return -1;
    }
    if (arrayp(data)) data = data * "";
    if (sizeof(args)) data = sprintf(data, @args);
    if (!sizeof(data)) return 0;
    if (state & STATE_NONBLOCKING) {
      if (sizeof(write_buffer) >= write_limit) return 0;
      if (sizeof(data) > write_limit) data = data[..write_limit-1];
    }
    write_buffer->add(data);

    other->schedule_poll();
    schedule_poll();
    return sizeof(data);
  }

  //!
  int(-1..1) peek(int|float|void timeout, int|void not_eof)
  {
    Thread.MutexKey key = mux->lock();
    while (1) {
      if (sizeof(read_buffer)) return 1;
      if (!other->is_open("w")) {
	if (!not_eof) return 1;
	internal_errno = System.EPIPE;
	return -1;
      }
      if (timeout <= 0) return 0;
      cond->wait(key, timeout);
      timeout = 0;
    }
  }

  //! Get the other end of the emulated pipe/socket.
  this_program get_other()
  {
    return other;
  }

  protected void _destruct()
  {
    close();
    other->close();
    if (poll_co) {
      (backend || Pike.DefaultBackend)->remove_call_out(poll_co);
      poll_co = UNDEFINED;
    }
    cond->broadcast();
  }
}

protected local inherit InternalSocket : Other;

//!
local inherit InternalSocket : Base;

//!
protected void create(void|string direction)
{
  Stdio.Buffer abuf, bbuf;
  Thread.Mutex mux = Thread.Mutex();
  Thread.Condition cond = Thread.Condition();

  if (!direction)
    direction = "rw";

  if (has_value(direction, "w"))
    abuf = Stdio.Buffer();
  if (has_value(direction, "r"))
    bbuf = Stdio.Buffer();

  Other::create(Base::this, abuf, bbuf, mux, cond);
  Base::create(Other::this, bbuf, abuf, mux, cond);
}

