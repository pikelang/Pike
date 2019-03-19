/*
 * Compile & Exec
 *
 * This tests creates a small program that juggles with integeres,
 * compiles it once and runs it <runs> times.
 *
 */

#pike __REAL_VERSION__

inherit Tools.Shoot.Compile;

constant name="Compile & Exec";

int perform()
{
   for (int i=0; i<runs; i++)
       compile_string(prog)()->test();
   return (ops+ops/5+12)*runs;
}
