/* csr.pmod
 *
 * Handlig of Certifikate Signing Requests (PKCS-10)
 */

#if __VERSION__ >= 0.6
import ".";
#endif /* __VERSION__ >= 0.6 */

import Standards.ASN1.Encode;

class CSR_Attributes
{
  inherit Certificate.Attributes;
  constant cls = 2;
  constant tag = 0;
}

object build_csr(object rsa, object name,
		 mapping(string:array(object)) attributes)
{
  object info = asn1_sequence(asn1_integer(0), name,
			      RSA.build_rsa_public_key(rsa),
			      CSR_Attributes(Identifiers.attribute_ids,
					 attributes));
  return asn1_sequence(info,
		       asn1_sequence(
			 Identifiers.rsa_md5_id, asn1_null()),
		       asn1_bitstring(rsa->sign(info->der(), Crypto.md5)
			 ->digits(256)));
}

			      
