/*
 * $Id: aes.pike,v 1.3 2001/11/08 01:45:38 nilsson Exp $
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
