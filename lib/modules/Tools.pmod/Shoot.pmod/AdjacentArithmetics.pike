#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name = "Adjacent arithmetics (locals)";
constant ITER = 3100000;
int checksum;

int perform()
{
  int a = 1, b = 2;
  for (int i = 0; i < ITER; i++) {
    a++; a += 2; b += a; a += b; b--; a += b; a -= 3; b++;
    // Keep the benchmark in the native integer range.
    a &= 0xffffff;
    b &= 0xffffff;
  }
  checksum = a ^ b;
  return ITER * 8;
}
