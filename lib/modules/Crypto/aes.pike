/*
 * $Id$
 *
 */

//! Advanced Encryption Standard (AES), previously known as @[Crypto.rijndael].

#pike __REAL_VERSION__

inherit Crypto.rijndael;

//! Returns the string @tt{"AES"@}.
string name()
{
  return "AES";
}
