#pike __REAL_VERSION__

//!
//! This module provides a gzip-uncompressing non-blocking pipe interface.
//!

inherit .Base;

protected void process_init() { _engine = Gz.inflate(-15); }

protected string(8bit) process_header(string(8bit) data) {
  Gz.make_header(buffer);
  return data;
}

protected void process_data(string(8bit) data) {
  for (;;) {
    buffer->add(_engine->inflate(data));
    data = _engine->end_of_stream();
    if (data && (data = Gz.check_header(0, data))) {
      process_init();           // Latch onto concatenated gzip streams
      continue;
    }
    break;
  }
}

protected void process_trailer() { }
