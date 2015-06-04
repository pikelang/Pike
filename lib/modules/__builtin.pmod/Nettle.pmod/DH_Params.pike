//! Diffie-Hellman parameters.

#if constant(Nettle.DH_Params)
inherit Nettle.DH_Params;
#else
//! Prime.
Gmp.mpz p;

//! Generator.
Gmp.mpz g;

//! Subgroup size.
Gmp.mpz q;

// FIXME: generate().

//! Generate a Diffie-Hellman key pair.
//!
//! @returns
//!   Returns the following array:
//!   @array
//!     @elem Gmp.mpz 0
//!       The generated public key.
//!     @elem Gmp.mpz 1
//!       The corresponding private key.
//!   @endarray
array(Gmp.mpz) generate_keypair(function(int(0..):string(8bit)) rnd)
{
  Gmp.mpz key = [object(Gmp.mpz)]
    (Gmp.mpz(rnd([int(0..)](q->size() / 8 + 16)), 256) % (q - 1) + 1);

  Gmp.mpz pub = g->powm(key, p);

  return ({ pub, key });
}
#endif

//! Initialize the set of Diffie-Hellman parameters.
//!
//! @param other
//!   Copy the parameters from this object.
protected void create(this_program other)
{
  p = other->p;
  g = other->g;
  q = other->q;
}

//! Initialize the set of Diffie-Hellman parameters.
//!
//! @param p
//!   The prime for the group.
//!
//! @param g
//!   The generator for the group. Defaults to @expr{2@}.
//!
//! @param q
//!   The order of the group. Defaults to @expr{(p-1)/2@}.
protected variant void create(Gmp.mpz|int p, Gmp.mpz|int|void g,
			      Gmp.mpz|int|void q)
{
  this::p = Gmp.mpz(p);
  this::g = g && Gmp.mpz(g) || Gmp.mpz(2);
  this::q = q && Gmp.mpz(q) || Gmp.mpz( [int](p-1)/2 );
}
