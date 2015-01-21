#pike __REAL_VERSION__
#pragma strict_types

extern int size();

class Point {
  extern void set(Gmp.mpz|int x, Gmp.mpz|int y);

  protected void create(Gmp.mpz|int x, Gmp.mpz|int y)
  {
    set(x, y);
  }

  // Restrict the integer to not cause problems in the sprintf in
  // encode.
  private int(16bit) bytes()
  {
    return [int(16bit)]((size()+7)>>3);
  }

//! @decl void create(Gmp.mpz|int x, Gmp.mpz|int y)
//! @decl void create(Stdio.Buffer|string(7bit) data)
//!
//! Initialize to the selected point on the curve.
//!
//! @note
//!   Throws errors if the point isn't on the curve.
variant protected void create(string(8bit)|Stdio.Buffer data)
{
  // FIXME: Perhaps we want to send the agreed upon point format as
  // optional second argument to verify against?

  Stdio.Buffer buf = stringp(data) ?
    Stdio.Buffer(data) : [object(Stdio.Buffer)]data;
  Gmp.mpz x,y;

  // ANSI x9.62 4.3.7.
  switch(buf->read_int(1))
  {
  case 4:
    int size = bytes();

    if (sizeof(buf) != size*2)
      error("Invalid size in point format.\n");

    x = Gmp.mpz(buf->read_int(size));
    y = Gmp.mpz(buf->read_int(size));

    // FIXME: Are there any security implications of (x, y) above
    //        being == curve.g (ie remote.secret == 1)?
    break;

  default:
    // Compressed points not supported yet.
    // Infinity points not supported yet.
    // Hybrid points not supported (and prohibited in TLS).
    error("Unsupported point format.\n");
    break;
  }

  set(x, y);
}

  extern Gmp.mpz get_x();
  extern Gmp.mpz get_y();

  // FIXME: Parameter to select encoding format.
  string encode()
  {
    int(31bit) size = bytes();
    return sprintf("%c%*c%*c", 4, size, get_x(), size, get_y());
  }

}
