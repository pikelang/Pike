/* csr.pmod
 *
 * Handlig of Certifikate Signing Requests (PKCS-10)
 */

import asn1.encode;

class Attributes
{
  inherit certificate.attribute_set;
  constant cls = 2;
  constant tag = 0;
}

object build_csr(object rsafoo, object name,
		 mapping(string:object) attributes)
{
  object info = asn1_sequence(asn1_integer(0), name,
			      rsa.build_rsa_public_key(rsafoo),
			      Attributes(identifiers.attribute_ids,
					 attributes));
  return asn1_sequence(info,
		       asn1_sequence(
			 identifiers.rsa_md5_id, asn1_null()),
		       asn1_bitstring(rsafoo->sign(info->der(), Crypto.md5)
			 ->digits(256)));
}

			      
