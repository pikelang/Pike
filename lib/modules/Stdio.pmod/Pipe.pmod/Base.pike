#pike __REAL_VERSION__

//!
//! This module provides a generic data processing non-blocking pipe interface.
//! Set it to a pool of dedicated backends to use more than one CPU core
//! (use one thread per backend).
//! Use it in conjunction with the Shuffler to scale to an unlimited number
//! of CPU cores.
//!

protected Stdio.Buffer buffer;
protected object _engine;               // The data processing engine
protected function read_cb, write_cb, close_cb;
protected int payloadlen;               // Number of bytes processed
protected Pike.Backend backend;

//!
final void set_backend(Pike.Backend _backend) {
  backend = _backend;
  set_write_callback(write_cb);
}

//!
final object `engine() { return _engine; }

final void read_cb_worker() {
  string(8bit) s;
  function rcb = read_cb;
  if (rcb && sizeof(s = buffer->read()))
    rcb(0, s);
}

//!
final void set_read_callback(function rcb) {
  if (read_cb = rcb) {
    if (sizeof(buffer))
      backend->call_out(read_cb_worker, 0);
    if (payloadlen < 0 && close_cb)
      backend->call_out(close_cb, 0);				// Signal EOF
  }
}

//!
final void set_write_callback(function wcb) {
  if (write_cb = wcb)
    backend->call_out(wcb, 0);
}

//!
final void set_close_callback(function ccb) {
  if (close_cb = ccb)
    set_read_callback(read_cb);
}

//!
final void set_nonblocking(function rcb, function wcb, function ccb) {
  set_read_callback(rcb);
  set_write_callback(wcb);
  set_close_callback(ccb);
}

private void close_worker() {
  process_trailer();
  payloadlen = -1;
  set_read_callback(read_cb);
}

//!
final int close() {
  backend->call_out(close_worker, 0);
  return 0;
}

private void write_worker(string(8bit) data) {
  if (!payloadlen && sizeof(data))
    data = process_header(data);
  process_data(data);
  payloadlen += sizeof(data);
  set_read_callback(read_cb);
  set_write_callback(write_cb);
}

//!
final int write(string(8bit) data) {
  backend->call_out(write_worker, 0, data);
  return sizeof(data);
}

//! Note that the Gz.deflate engine provided needs to be preconfigured
//! using negative compression levels.
//!
protected void create(Gz.deflate|void engine) {
  buffer = Stdio.Buffer();
  _engine = engine;
  payloadlen = 0;
  backend = Pike.DefaultBackend;
  process_init();
}

protected void _destruct() { close(); }

//!
protected void process_init() { }

//!
protected string(8bit) process_header(string(8bit) data) { return data; }

//!
protected void process_data(string(8bit) data) { }

//!
protected void process_trailer() { }
