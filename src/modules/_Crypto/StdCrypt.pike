/* $Id: StdCrypt.pike,v 1.4 1997/05/31 22:04:07 grubba Exp $
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

