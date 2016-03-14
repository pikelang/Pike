#pike __REAL_VERSION__
#pragma strict_types

//! This class implements a fast and secure random generator. The
//! generator is a Fortuna PRNG that is fed additional entropy from
//! the system RNG (and hardware RNG, if available) for every
//! generated megabyte of data.

inherit Builtin.RandomInterface;
inherit Nettle.Fortuna : Fortuna;

#if constant(Random.Hardware)
protected .Interface rnd = .Hardware();
#else
protected .Interface rnd = .System();
#endif

protected int counter;

protected void create()
{
  counter = 0;
  reseed(rnd->random_string(32));
}

string(8bit) random_string(int(0..) length)
{
  if( counter > (1<<20) )
    create();
  counter += length;
  return Fortuna::random_string(length);
}
