/*
 * $Id: aes.pike,v 1.1 2003/03/19 17:46:30 nilsson Exp $
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
