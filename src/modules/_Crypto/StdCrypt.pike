/* Crypto.pike
 *
 * Cryptography module
 */

/* Do magic loading */
#include "crypto.h"

#define P(x) (program) ("/precompiled/crypto/" + x)

constant Crypto = P("crypto");
constant DES = P("des");
constant IDEA = P("idea");
constant ROT256 = P("invert");
constant SHA = P("sha");
constant CBC = P("cbc");
constant crypto_pipe = P("pipe");

