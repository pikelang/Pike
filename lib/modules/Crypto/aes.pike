/*
 * $Id: aes.pike,v 1.2 2001/08/21 20:56:23 grubba Exp $
 *
 * Advanced Encryption Standard (AES).
 *
 * Henrik Grubbström 2000-10-03
 */

#pike __REAL_VERSION__

/* aka rijndael */
inherit Crypto.rijndael;

string name()
{
  return "AES";
}
