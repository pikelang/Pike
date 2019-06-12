#pike __REAL_VERSION__

//!
//! This module provides a gzip-compressing non-blocking pipe interface.
//! Set it to a pool of dedicated backends to use more than one CPU core to
//! compress data (use one thread per backend).
//! Use it in conjunction with the Shuffler to scale to an unlimited number
//! of CPU cores.
//!

#define DEFAULT_COMPRESSION_LEVEL 3

protected Stdio.Buffer buffer;
protected Gz.deflate _engine;
protected function read_cb, write_cb, close_cb;
protected int crc, payloadlen;
protected Pike.Backend backend;

//!
void set_backend(Pike.Backend _backend) {
  backend = _backend;
  set_write_callback(write_cb);
}

//!
Gz.deflate `engine() { return _engine; }

void read_cb_worker() {
  string s;
  function rcb = read_cb;
  if (rcb && (s = buffer->read()))
    rcb(0, s);
}

//!
void set_read_callback(function rcb) {
  if (read_cb = rcb) {
    if (sizeof(buffer))
      backend->call_out(read_cb_worker, 0);
    if (payloadlen < 0 && close_cb)
      backend->call_out(close_cb, 0);				// Signal EOF
  }
}

//!
void set_write_callback(function wcb) {
  if (write_cb = wcb)
    backend->call_out(wcb, 0);
}

//!
void set_close_callback(function ccb) {
  if (close_cb = ccb)
    set_read_callback(read_cb);
}

//!
void set_nonblocking(function rcb, function wcb, function ccb) {
  set_read_callback(rcb);
  set_write_callback(wcb);
  set_close_callback(ccb);
}

private void close_worker() {   // Gz.FINISH
  buffer->sprintf("%s%-4c%-4c", _engine->deflate(""), crc, payloadlen);
  payloadlen = -1;
  set_read_callback(read_cb);
}

//!
int close() {
  backend->call_out(close_worker, 0);
  return 0;
}

private void write_worker(string data) {
  if (!payloadlen && sizeof(data))
    Gz.make_header(buffer);
  buffer->add(_engine->deflate(data, Gz.NO_FLUSH));
  crc = Gz.crc32(data, crc);
  payloadlen += sizeof(data);
  set_read_callback(read_cb);
  set_write_callback(write_cb);
}

//!
int write(string data) {
  backend->call_out(write_worker, 0, data);
  return sizeof(data);
}

//! Note that the Gz.deflate engine provided needs to be preconfigured
//! using negative compression levels.
//!
protected void create(Gz.deflate|void engine) {
  buffer = Stdio.Buffer();
  _engine = engine || Gz.deflate(-DEFAULT_COMPRESSION_LEVEL);
  crc = payloadlen = 0;
  backend = Pike.DefaultBackend;
}

protected void _destruct() {
  close();
}
