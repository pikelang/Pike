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
#if constant(Random.Hardware)
  /*
    Tested on a Intel(R) Core(TM) i5-4460  CPU @ 3.20GHz

                       Hardware     Fortuna
    1 byte per call    4.08 MB/s    1.48 MB/s
    2 bytes per call   7.89 MB/s    2.92 MB/s
    4 bytes per call   15.68 MB/s   5.89 MB/s
    8 bytes per call   22.78 MB/s   11.77 MB/s
    16 bytes per call  35.75 MB/s   23.37 MB/s
    32 bytes per call  49.69 MB/s   41.38 MB/s
    48 bytes per call  54.23 MB/s   53.77 MB/s
    64 bytes per call  60.86 MB/s   65.52 MB/s
  */
  if( length<48 )
    return rnd->random_string(length);
#endif
  if( counter > (1<<20) )
    create();
  counter += length;
  return Fortuna::random_string(length);
}
