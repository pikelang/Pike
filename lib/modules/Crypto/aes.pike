/*
 * $Id: aes.pike,v 1.2 2001/08/21 20:55:56 grubba Exp $
 *
 * Advanced Encryption Standard (AES).
 *
 * Henrik Grubbström 2000-10-03
 */

#pike __REAL_VERSION__

//! Previously known as @[Crypto.rijndael].
inherit Crypto.rijndael;

//! Returns the string @tt{"AES"@}.
string name()
{
  return "AES";
}
