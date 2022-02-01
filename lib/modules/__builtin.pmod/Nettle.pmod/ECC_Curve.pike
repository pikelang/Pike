#pike __REAL_VERSION__
#pragma strict_types

//! Base class for Elliptic Curve Definitions.
//!
//! @seealso
//!   @[Crypto.ECC.Curve], @[Nettle.ECC_Curve]

//! Return the size in bits of a single coordinate on the curve.
extern int size();

//! Return the name of the Curve.
extern string(7bit) name();

//! Return the JOSE name of the Curve (if any).
//!
//! @returns
//!   The default implementation returns @[UNDEFINED].
string(7bit) jose_name()
{
  return UNDEFINED;
}

//! Generate a new scalar suitable for use as an ECDSA private key
//! or as an ECDH secret factor.
//!
//! @note
//!   Returns the scalar in the preferred representation for the Curve.
string(8bit)|Gmp.mpz new_scalar(function(int(0..):string(8bit)) rnd);

//! Base class for a point on an elliptic curve.
class Point {
  //! Get the @[Crypto.ECC.Curve] that this @[Point] belongs to.
  global::this_program get_curve()
  {
    return global::this;
  }

  // Restrict the integer to not cause problems in the sprintf in
  // encode.
  private int(16bit) bytes()
  {
    return [int(16bit)]((size()+7)>>3);
  }

  //! Set a new coordinate on the @[Curve] for the @[Point].
  //!
  //! @note
  //!   Some curves (eg @[Crypto.ECC.Curve25519] do not support
  //!   numeric coordinates); on the other hand the SECP curves
  //!   prefer numeric coordinates.
  extern void set(Gmp.mpz|int x, Gmp.mpz|int y);
  variant void set(string(8bit) x, string(8bit) y)
  {
    int bytes = this::bytes();
    if ((sizeof(x) != bytes) || (sizeof(y) != bytes)) {
      // RFC 7518 6.2.1.2 & 6.2.1.3:
      //   The length of this octet string MUST be the full size of a
      //   coordinate for the curve specified in the "crv" parameter
      error("Invalid key sizes x: %d, y: %d, expected %d bytes.\n",
	    sizeof(x), sizeof(y), bytes);
    }
    set(Gmp.mpz(x, 256), Gmp.mpz(y, 256));
  }

  //! Get the coordinates for the curve in the preferred representation.
  extern Gmp.mpz|string(8bit) get_x();
  extern Gmp.mpz|string(8bit) get_y();

  //! Get the canonic string representation of the x coordinate.
  string(8bit) get_x_str()
  {
    string(8bit)|Gmp.mpz x = get_x();
    if (!stringp(x)) {
      x = [string(8bit)]sprintf("%*c", bytes(), [object(Gmp.mpz)]x);
    }
    return [string(8bit)]x;
  }

  //! Get the canonic string representation of the y coordinate.
  string(8bit) get_y_str()
  {
    string(8bit)|Gmp.mpz y = get_y();
    if (!stringp(y)) {
      y = [string(8bit)]sprintf("%*c", bytes(), [object(Gmp.mpz)]y);
    }
    return [string(8bit)]y;
  }

  //! Get the numeric representation of the x coordinate.
  Gmp.mpz get_x_num()
  {
    string(8bit)|Gmp.mpz x = get_x();
    if (stringp(x)) {
      x = Gmp.mpz([string(8bit)]x, 256);
    }
    return [object(Gmp.mpz)]x;
  }

  //! Get the numeric representation of the y coordinate.
  Gmp.mpz get_y_num()
  {
    string(8bit)|Gmp.mpz y = get_y();
    if (stringp(y)) {
      y = Gmp.mpz([string(8bit)]y, 256);
    }
    return [object(Gmp.mpz)]y;
  }

  protected void create(Gmp.mpz|int|string(8bit) x, Gmp.mpz|int|string(8bit) y)
  {
    set(x, y);
  }

  variant protected void create(this_program p)
  {
    if (p->get_curve() != get_curve())
      error("Mismatching curves!\n");
    set(p->get_x(), p->get_y());
  }

  variant protected void create(mapping(string(7bit):int|Gmp.mpz|string(8bit)) p)
  {
    if (p->kty == "EC") {
      if (p->crv != jose_name()) {
	error("Invalid curve selected. %O != %O.\n",
	      p->crv, jose_name());
      }
      mapping(string(7bit):string(7bit)) jwk =
	[mapping(string(7bit):string(7bit))]p;
      p = ([]);
      foreach(({ "x", "y" }), string coord) {
        p[coord] = [string(8bit)]Pike.Lazy.MIME.decode_base64url(jwk[coord] || "");
      }
    }
    set(p->x, p->y);
  }

  //! @decl void create()
  //! @decl void create(Point p)
  //! @decl void create(mapping(string(7bit):int|Gmp.mpz|string(8bit)) p)
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
  //!     A mapping with integer coordinates @expr{"x"@} and @expr{"y"@}.
  //!   @type mapping(string(7bit):string(7bit))
  //!     A mapping representing a JWK for the @[Point]
  //!     on the same @[Curve].
  //!   @type mapping(string(7bit):string(8bit))
  //!     A mapping with coordinates @expr{"x"@} and @expr{"y"@}
  //!     in big-endian.
  //!   @type Stdio.Buffer|string(8bit)
  //!     The ANSI x9.62 representation of the @[Point].
  //!     Cf @[encode()].
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
    object(Gmp.mpz)|zero x,y;

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

  variant protected void create()
  {
  }

  // FIXME: Parameter to select encoding format.
  //! Serialize the @[Point].
  //!
  //! The default implementation serializes according to ANSI x9.62
  //! encoding #4 (uncompressed point format).
  string encode()
  {
    return sprintf("%c%s%s", 4, get_x_str(), get_y_str());
  }

  protected string _sprintf(int type)
  {
    return type=='O' && sprintf("%O(0x%x,0x%x)", this_program,
                                get_x_num(), get_y_num());
  }
}

protected string _sprintf(int type)
{
  return type=='O' && sprintf("%O(%s)", this_program, name() || "UNKOWN");
}
