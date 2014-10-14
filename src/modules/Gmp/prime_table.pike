#!/usr/bin/env pike

/*
 * Generates a table of primes.
 * Used when cross-compiling.
 *
 * Henrik Grubbström 2000-08-01
 */
int main(int argc, array(string) argv)
{
  if (argc != 2) {
    werror("Bad number of arguments!\n");
    exit(1);
  }
  int count = (int)argv[1];
  write(sprintf("/* Automatically generated.\n"
		" *%{ %s%}\n"
		" * Do not edit.\n"
		" */\n"
		"\n"
		"#define NUMBER_OF_PRIMES %d\n"
		"\n"
		"const unsigned short primes[NUMBER_OF_PRIMES] = {\n"
                "   2, 3,",
		argv, count));

  Gmp.mpz prime = Gmp.mpz(3);
  for (int i=2; i < count; i++) {
    prime = (prime+1)->next_prime();
    if (!(i%10)) {
      write(sprintf("\n   %d,", prime));
    } else {
      write(sprintf(" %d,", prime));
    }
  }
  write("\n};\n"
	"\n");
  return 0;
}
