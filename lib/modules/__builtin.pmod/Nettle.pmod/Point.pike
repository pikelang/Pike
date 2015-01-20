#pike __REAL_VERSION__
#pragma strict_types

extern void set(Gmp.mpz|int x, Gmp.mpz|int y);

protected void create(Gmp.mpz|int x, Gmp.mpz|int y)
{
  set(x, y);
}

// FIXME: Perhaps we want to send the agreed upon point format as
// optional second argument to verify against?
variant protected void create(string(7bit)|Stdio.Buffer data)
{
  Stdio.Buffer buf = stringp(data) ?
    Stdio.Buffer(data) : [object(Stdio.Buffer)]data;
  Gmp.mpz x,y;

  // ANSI x9.62 4.3.7.
  switch(buf->read_int(1))
  {
  case 4:
    int len = sizeof(data);
    if (!len || len & 1)
      error("Invalid size in point format.\n");

    len /= 2;
    x = Gmp.mpz(data->read_int(len));
    y = Gmp.mpz(data->read_int(len));

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
