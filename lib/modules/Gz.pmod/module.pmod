#pike __REAL_VERSION__
#require constant(_Gz)

//!
inherit _Gz;

//! Check whether a file has a valid gzip header.
//!
//! @param f
//!   File to check.
//!
//! @param buf
//!   Prefix of @[f].
//!
//! @returns
//!   Returns the content of @[f] after the gzip header
//!   if a header was found. Returns @expr{0@} (zero)
//!   if there was no header.
string(8bit)|zero
 check_header(Stdio.Stream|void f, Stdio.Buffer|string(8bit)|void buf) {
  int flags, len;

  buf = Stdio.Buffer(buf);

  int buf_read(int n) {
    n -= sizeof(buf);
    return n <= 0 || f && n == buf->input_from(f, n);
  };

  if (!buf_read(4))
    return 0;

  array res = buf->sscanf("\x1f\x8b\x08%1c");
  if (!res)
    error("Missing gzip magic header %O\n", ((string)buf)[..2]);

  flags = res[0];
  if (flags & 0xe0 || !buf_read(6))
    return 0;
  buf->consume(6);

  if (flags & 4) {
    if (!buf_read(2))
      return 0;
    [len] = buf->sscanf("%-2c");
    if (!buf_read(len))
      return 0;
    buf->consume(len);
  }

  if (flags & 8)
    do
      if (!buf_read(1))
        return 0;
    while (buf->read_int8());

  if (flags & 16)
    do
      if (!buf_read(1))
        return 0;
    while (buf->read_int8());

  if (flags & 2) {
    if (!buf_read(2))
      return 0;
    buf->consume(2);
  }

  return buf->read();
}

//! Write a gzip header to a file or buffer.
//!
//! @param f
//!   File or buffer to write a gzip header to.
//!
//! @returns
//!   Returns @expr{1@} on success and @expr{0@} (zero)
//!   on failure.
int make_header(Stdio.Stream|Stdio.Buffer f) {
  constant fmt = "\x1f\x8b\x08\0\0\0\0\0\0\3";
  if (f->add) {
    f->add(fmt);
    return 1;
  }
  return f->write(fmt) == sizeof(fmt);
}
