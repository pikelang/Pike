#pike __REAL_VERSION__

//!
//! This module provides a gzip-compressing non-blocking pipe interface.
//! Set it to a pool of dedicated backends to use more than one CPU core to
//! compress data (use one thread per backend).
//! Use it in conjunction with the Shuffler to scale to an unlimited number
//! of CPU cores.
//!

#define DEFAULT_COMPRESSION_LEVEL	3
#define MINFLUSHBUFFERSIZE		2048

inherit .Base;

protected int crc;

protected void process_init() {
  crc = 0;
  if (!_engine)
    _engine = Gz.deflate(-DEFAULT_COMPRESSION_LEVEL);
}

protected string(8bit) process_header(string(8bit) data) {
  Gz.make_header(buffer);
  return data;
}

protected void process_data(string(8bit) data) {
  crc = Gz.crc32(data, crc);
  buffer->add(_engine->deflate(data,
   // Avoid a small leftover header/buffer
   sizeof(buffer) && sizeof(buffer) < MINFLUSHBUFFERSIZE
    ? Gz.PARTIAL_FLUSH : Gz.NO_FLUSH));
}

protected void process_trailer() {        // Gz.FINISH
  buffer->sprintf("%s%-4c%-4c", _engine->deflate(""), crc, payloadlen);
}
