/*
 * Compatibility with Pike 7.6 Standards.UUID.
 *
 * $Id: module.pmod,v 1.1 2005/01/06 21:15:54 grubba Exp $
 *
 * Henrik Grubbström 2005-01-06
 */

#pike 7.7

//! Return a new binary UUID.
string new()
{
  return Standards.UUID.make_version1(-1)->encode();
}

//! Return a new UUID string.
string new_string()
{
  return Standards.UUID.make_version1(-1)->str();
}
