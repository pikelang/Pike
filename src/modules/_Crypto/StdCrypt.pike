/* Crypto.pike
 *
 * Cryptography module
 */

/* Do magic loading */
// #include "crypto.h"

#define P(x) (program) (Crypto[x])

constant DES = P("des");
constant IDEA = P("idea");
constant ROT256 = P("invert");
constant RC4 = P("rc4");
constant SHA = P("sha");
constant CBC = P("cbc");
constant crypto_pipe = P("pipe");
constant Crypto = P("crypto");

