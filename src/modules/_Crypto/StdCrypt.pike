/* $Id: StdCrypt.pike,v 1.5 2000/03/28 12:22:49 grubba Exp $
 *
 * Cryptography module
 */

/* Do magic loading */
// #include "crypto.h"

#define P(x) (program) (Crypto[x])

constant DES = P("des");
constant IDEA = P("idea");
constant ROT256 = P("invert");
constant ARCFOUR = P("arcfour");
constant SHA = P("sha");
constant CBC = P("cbc");
constant crypto_pipe = P("pipe");
constant Crypto = P("crypto");

