#pike __REAL_VERSION__
#pragma strict_types
#require constant(Nettle.Fortuna)

//! This class implements a detministic random generator by combining
//! a Fortuna random generator with the @[Random.Interface]. The
//! generator is not reseeded after the initial seed.
//!
//! In case of a process fork the random generators in both processes
//! will continue to generate identical results.

inherit Builtin.RandomInterface;
inherit Nettle.Fortuna;

//! Initialize the random generator with seed. The internal state is
//! 256 bits, but all seed sizes are allowed.
protected void create(string(8bit)|int seed)
{
  reseed((string(8bit))seed);
}
