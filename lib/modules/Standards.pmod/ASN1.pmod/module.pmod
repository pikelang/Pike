// $Id$

string encode_der_oid (string dotted_decimal)
//! Convenience function to convert an oid (object identifier) on
//! dotted-decimal form (e.g. @expr{"1.3.6.1.4.1.1466.115.121.1.38"@})
//! to its DER (and hence also BER) encoded form.
//!
//! @seealso
//! @[decode_der_oid]
{
  // NB: No syntax checking at all..
  .Types.asn1_identifier id = .Types.asn1_identifier();
  id->id = (array(int)) (dotted_decimal / ".");
  return id->der_encode();
}

string decode_der_oid (string der_oid)
//! Convenience function to convert a DER/BER encoded oid (object
//! identifier) to the human readable dotted-decimal form.
//!
//! @seealso
//! @[encode_der_oid]
{
  // NB: No syntax checking at all..
  .Types.asn1_identifier id = .Types.asn1_identifier();
  id->decode_primitive (der_oid[2..]);
  return (array(string)) id->id * ".";
}
