#pike __REAL_VERSION__

//!
//! This module emulates a bidirectional pipe/socket
//! without using any actual file descriptors.
//!

#ifndef DEFAULT_WRITE_LIMIT
#define DEFAULT_WRITE_LIMIT	0x10000
#endif

//! Class that implements one end of an emulated  bi-directional pipe/socket.
protected class InternalSocket( protected this_program _other,
				Stdio.Buffer _read_buffer,
				Stdio.Buffer _write_buffer,
                                protected Thread.Mutex mux,
                                protected Thread.Condition cond)

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
      return !!_read_buffer;
    case "w":
      return !!_write_buffer;
    default:
      return !!(_read_buffer || _write_buffer);
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

  protected Pike.Backend backend = Pike.DefaultBackend;
  protected mixed poll_co;

  //!
  Pike.Backend query_backend()
  {
    return backend;
  }

  //!
  void set_backend(Pike.Backend be)
  {
    {
      Thread.MutexKey key = mux->lock();
      if (poll_co) {
        backend->remove_call_out(poll_co);
        poll_co = UNDEFINED;
      }
      backend = be;
    }
    return schedule_poll();
  }

  protected function __read_cb;
  protected function __write_cb;
  protected function __close_cb;
  protected mixed __id;

  void _run_close_cb() {
    if (__close_cb && _read_buffer && !sizeof(_read_buffer))
      __close_cb(0, __id);
  }

  void _fill_write_buffer() {
    if (__write_cb && sizeof(_write_buffer) < write_limit)
      __write_cb(0, __id);
  }

  protected void internal_poll() {
    poll_co = UNDEFINED;
    if (_write_buffer)
      _fill_write_buffer();
    if (this && __read_cb && _read_buffer)
      if (sizeof(_read_buffer)) {
        __read_cb(__id, _read_buffer->read());
        if (_other) {
          if (_other->_write_buffer)
            _other->_fill_write_buffer();
          else
            _other->_run_close_cb();
          if (this)
            return schedule_poll();
        } else
          return _run_close_cb();
      } else
        return _run_close_cb();
  }

  // Internal
  //
  // Wake up any thread that waits on this connection,
  // and schedule a poll for calling of callbacks.
  void schedule_poll()
  {
    Thread.MutexKey key = mux->lock(2);
    cond->broadcast();
    if (!poll_co)
      poll_co = backend->call_out(internal_poll, 0);
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
    {
      Thread.MutexKey key = mux->lock();
      [__read_cb, __write_cb, __close_cb] = ({ rcb, wcb, ccb });
    }
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
ebadf:
    do {
      switch (direction) {
        case "r":
          if (!_read_buffer)
            break ebadf;
          __close_cb = __read_cb = UNDEFINED;
          break;
        case "w":
          if (!_write_buffer)
            break ebadf;
          _write_buffer = UNDEFINED;
          __write_cb = UNDEFINED;
          if (_other)
            _other->_run_close_cb();
          break;
        default:
          if (!_read_buffer && !_write_buffer)
            break ebadf;
          _read_buffer = _write_buffer = UNDEFINED;
          __close_cb = __read_cb = __write_cb = UNDEFINED;
          if (_other)
            _other->_run_close_cb();
          break;
      }
      if (!_read_buffer && !_write_buffer) {
        _other = 0;			// Eliminate circular reference
        if (poll_co) {
          backend->remove_call_out(poll_co);
          poll_co = UNDEFINED;
          cond->broadcast();
        }
      }
      return 0;
    } while(0);
    internal_errno = System.EBADF;
    return -1;
  }

  //!
  string(8bit) read(int|void nbytes, bool|void not_all)
  {
    if (!nbytes) nbytes = 0x7fffffff;
    string(8bit) ret;
    Thread.MutexKey key = mux->lock();
    while(1) {
      if (!_read_buffer) {
	// Read direction closed.
	internal_errno = System.EBADF;
	break;
      }
      if (nbytes <= sizeof(_read_buffer)
       || not_all || (state & STATE_NONBLOCKING)
       || !(_other && _other->_write_buffer)) {
	ret = _read_buffer->try_read(nbytes);
	break;
      }
      if (nbytes > DEFAULT_WRITE_LIMIT && _other) {
	_other->set_write_limit(nbytes);
	_other->schedule_poll();
      }
      cond->wait(key);
    }
    if (_other) {
      _other->set_write_limit(DEFAULT_WRITE_LIMIT);
      _other->schedule_poll();
    }
    schedule_poll();
    return ret;
  }

  //!
  int write(sprintf_format|array(string(8bit)) data,
	    sprintf_args ...args)
  {
    if (!_write_buffer) {
      // Write direction closed.
      internal_errno = System.EBADF;
      return -1;
    }
    if (!(_other && _other->_read_buffer)) {
      // _other end has closed.
      internal_errno = System.EPIPE;
      return -1;
    }
    if (arrayp(data)) data = data * "";
    if (sizeof(args)) data = sprintf(data, @args);
    if (!sizeof(data)) return 0;
    if (state & STATE_NONBLOCKING) {
      if (sizeof(_write_buffer) >= write_limit) return 0;
      if (sizeof(data) > write_limit) data = data[..write_limit-1];
    }
    _write_buffer->add(data);

    if (_other)
      _other->schedule_poll();
    schedule_poll();
    return sizeof(data);
  }

  //!
  int(-1..1) peek(int|float|void timeout, int|void not_eof)
  {
    Thread.MutexKey key = mux->lock();
    while (1) {
      if (sizeof(_read_buffer)) return 1;
      if (!(_other && _other->_write_buffer)) {
	if (!not_eof) return 1;
	internal_errno = System.EPIPE;
	return -1;
      }
      if (timeout <= 0) return 0;
      cond->wait(key, timeout);
      timeout = 0;
    }
  }

  //! The other end of the emulated pipe/socket.
  this_program `other()
  {
    return _other;
  }

  protected void _destruct()
  {
    close();
  }
}

//!
inherit InternalSocket;

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

  ::create(InternalSocket(this, abuf, bbuf, mux, cond),
                                bbuf, abuf, mux, cond);
}
