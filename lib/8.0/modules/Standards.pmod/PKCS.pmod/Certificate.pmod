#pike 8.1

import Standards.ASN1.Types;

inherit Standards.PKCS.Certificate;

//! Return the certificate issuer RDN from a certificate string.
//!
//! @param cert
//! A string containing an X509 certificate.
//!
//! Note that the certificate usually must be decoded using
//! @[Standards.PEM.simple_decode()].
//!
//! @returns
//!  An Standards.ASN1.Sequence object containing the certificate issuer
//!  Distinguished Name (DN).
//!
//! @deprecated Standards.X509.decode_certificate
__deprecated__ Sequence get_certificate_issuer(string cert)
{
  return Standards.X509.decode_certificate(cert)->issuer;
}

//! Return the certificate subject RDN from a certificate string.
//!
//! @param cert
//! A string containing an X509 certificate.
//!
//! Note that the certificate usually must be decoded using
//! @[PEM.simpe_decode()].
//!
//! @returns
//!  An Standards.ASN1.Sequence object containing the certificate subject
//!  Distinguished Name (DN).
//!
//! @deprecated Standards.X509.decode_certificate
__deprecated__ Sequence get_certificate_subject(string cert)
{
  return Standards.X509.decode_certificate(cert)->subject;
}
