/*
 * $Id: aes.pike,v 1.2 2003/04/07 17:16:03 nilsson Exp $
 *
 */

//! Advanced Encryption Standard (AES), previously known as
//! @[Crypto.rijndael].

#pike __REAL_VERSION__

inherit Crypto.rijndael;

//! Returns the string @expr{"AES"@}.
string name()
{
  return "AES";
}
