#pike __REAL_VERSION__
#pragma strict_types
#require constant(Nettle.Fortuna)

inherit Builtin.RandomInterface;
inherit Nettle.Fortuna;

protected void create(string(8bit)|int seed)
{
  ::reseed((string(8bit))seed);
}
