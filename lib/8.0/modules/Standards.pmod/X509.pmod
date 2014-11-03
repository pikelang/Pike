#pike 8.1
#require constant(Crypto.Hash)

inherit Standards.X509 : pre;

class Verifier
{
  inherit pre::Verifier;
  optional Crypto.RSA rsa;
  optional Crypto.DSA dsa;
}

protected class RSAVerifier
{
  inherit pre::RSAVerifier;

  Crypto.RSA.State `rsa() {
    return pkc;
  }
}

protected class DSAVerifier
{
  inherit pre::DSAVerifier;

  Crypto.DSA.State `dsa() {
    return pkc;
  }
}
