
#pike 7.5

inherit Crypto.RSA;

Gmp.mpz sign(string m, program|object h) {
  if(!h->hash) h=h();
  return ::sign(m,h);
}

int(0..1) verify(string m, program|object h, object s) {
  if(!h->hash) h=h();
  return ::verify(m,h,s);
}
