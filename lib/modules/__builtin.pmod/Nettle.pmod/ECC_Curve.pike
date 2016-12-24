#pike __REAL_VERSION__
#pragma strict_types

//! Base class for Elliptic Curve Definitions.
//!
//! @seealso
//!   @[Crypto.ECC.Curve], @[Nettle.ECC_Curve]

extern int size();
extern string(7bit) name();
extern string(7bit) jose_name();

//! Base class for a point on an elliptic curve.
class Point {
  extern global::this_program get_curve();
  extern void set(Gmp.mpz|int x, Gmp.mpz|int y);
  extern Gmp.mpz get_x();
  extern Gmp.mpz get_y();

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

  variant protected void create(this_program p)
  {
    if (p->get_curve() != get_curve())
      error("Mismatching curves!\n");
    set(p->get_x(), p->get_y());
  }

  variant protected void create(mapping(string(7bit):int|Gmp.mpz|string(8bit)) p)
  {
    if (p->kty == jose_name()) {
      mapping(string(7bit):string(7bit)) jwk =
	[mapping(string(7bit):string(7bit))]p;
      p = ([]);
      foreach(({ "x", "y" }), string coord) {
	p[coord] = MIME.decode_base64url(jwk[coord] || "");
      }
    }
    int bytes = (size() + 7)/8;
    if (stringp(p->x)) {
      // RFC 7518 6.2.1.2:
      //   The length of this octet string MUST be the full size of a
      //   coordinate for the curve specified in the "crv" parameter
      string(8bit) c = [string(8bit)]p->x;
      if (sizeof(c) != bytes) {
	error("Invalid x coordinate size for curve %s: %d\n",
	      name(), sizeof(c));
      }
      p->x = Gmp.mpz(c, 256);
    }
    if (stringp(p->y)) {
      // RFC 7518 6.2.1.3:
      //   The length of this octet string MUST be the full size of a
      //   coordinate for the curve specified in the "crv" parameter
      string(8bit) c = [string(8bit)]p->y;
      if (sizeof(c) != bytes) {
	error("Invalid y coordinate size for curve %s: %d\n",
	      name(), sizeof(c));
      }
      p->y = Gmp.mpz(c, 256);
    }
    set([object(Gmp.mpz)]p->x, [object(Gmp.mpz)]p->y);
  }

  //! @decl void create(Point p)
  //! @decl void create(mapping(string(7bit):int|Gmp.mpz) p)
  //! @decl void create(mapping(string(7bit):string(7bit)) jwk)
  //! @decl void create(Gmp.mpz|int x, Gmp.mpz|int y)
  //! @decl void create(Stdio.Buffer|string(8bit) data)
  //!
  //! Initialize the object and optionally also select
  //! a point on the curve.
  //!
  //! The point on the curve can be selected via either
  //! via specifying the two coordinates explicitly, or via
  //! @mixed
  //!   @type Point
  //!     A @[Point] on the same @[Curve] to copy.
  //!   @type mapping(string(7bit):int|Gmp.mpz)
  //!     A mapping with coordinates @expr{"x"@} and @expr{"y"@}.
  //!   @type mapping(string(7bit):string(7bit))
  //!     A mapping representing a JWK for the @[Point]
  //!     on the same @[Curve].
  //!   @type mapping(string(7bit):string(8bit))
  //!     A mapping with coordinates @expr{"x"@} and @expr{"y"@}
  //!     in big-endian.
  //!   @type Stdio.Buffer|string(8bit)
  //!     The ANSI x9.62 representation of the @[Point].
  //! @endmixed
  //!
  //! @note
  //!   Throws errors if the point isn't on the @[Curve].
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

  // FIXME: Parameter to select encoding format.
  //! Serialize the @[Point].
  //!
  //! The default implementation serializes according to ANSI x9.62
  //! encoding #4 (uncompressed point format).
  string encode()
  {
    int(31bit) size = bytes();
    return sprintf("%c%*c%*c", 4, size, get_x(), size, get_y());
  }

  protected string _sprintf(int type)
  {
    return type=='O' && sprintf("%O(0x%x,0x%x)", this_program,
                                get_x(), get_y());
  }
}

protected string _sprintf(int type)
{
  return type=='O' && sprintf("%O(%s)", this_program, name() || "UNKOWN");
}
