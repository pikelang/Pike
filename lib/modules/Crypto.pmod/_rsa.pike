/* $Id: _rsa.pike,v 1.2 2003/03/23 22:44:03 nilsson Exp $
 *
 * Follow the PKCS#1 standard for padding and encryption.
 */

#pike __REAL_VERSION__

#if constant(Gmp.mpz)

Gmp.mpz n;  /* modulo */
Gmp.mpz e;  /* public exponent */
Gmp.mpz d;  /* private exponent (if known) */
int size;

/* Extra info associated with a private key. Not currently used. */
   
Gmp.mpz p;
Gmp.mpz q;

Gmp.mpz get_n()
{
  return n;
}

Gmp.mpz get_e()
{
  return e;
}

Gmp.mpz get_d()
{
  return d;
}

Gmp.mpz get_p()
{
  return p;
}

Gmp.mpz get_q()
{
  return q;
}

string cooked_get_n()
{
  return n->digits(256);
}

string cooked_get_e()
{
  return e->digits(256);
}

string cooked_get_d()
{
  return d->digits(256);
}

string cooked_get_p()
{
  return p->digits(256);
}

string cooked_get_q()
{
  return q->digits(256);
}

object set_public_key(Gmp.mpz|int modulo, Gmp.mpz|int pub)
{
  n = Gmp.mpz(modulo);
  e = Gmp.mpz(pub);
  size = n->size(256);
  if (size < 12)
    error( "Crypto.rsa->set_public_key: Too small modulo.\n" );
  return this_object();
}

object set_private_key(Gmp.mpz|int priv, array(Gmp.mpz|int)|void extra)
{
  d = Gmp.mpz(priv);
  if (extra)
  {
    p = Gmp.mpz(extra[0]);
    q = Gmp.mpz(extra[1]);
    n = p*q;
    size = n->size(256);
  }
  return this_object();
}

int query_blocksize() { return size - 3; }

Gmp.mpz rsa_pad(string message, int type, mixed|void random)
{
  string cookie;
  int len;

  len = size - 3 - sizeof(message);
  /*  write(sprintf("%d, %d, %d, %s", len, size, sizeof(message), message)); */
  if (len < 8)
    error( "Block too large. (%d,%d)\n", sizeof(message), size-3 );

  switch(type)
  {
  case 1:
    cookie = sprintf("%@c", replace(allocate(len), 0, 0xff));
    break;
  case 2:
    if (random)
      cookie = replace(random(len), "\0", "\1");
    else
      cookie = sprintf("%@c", Array.map(allocate(len), lambda(int dummy)
					{
					  return predef::random(255) + 1;
					} ));
    break;
  default:
    error( "Crypto.rsa->rsa_pad: Unknown type.\n" );
  }
  return Gmp.mpz(sprintf("%c", type) + cookie + "\0" + message, 256);
}

string rsa_unpad(Gmp.mpz block, int type)
{
  string s = block->digits(256);
  int i = search(s, "\0");

  if ((i < 9) || (sizeof(s) != (size - 1)) || (s[0] != type))
    return 0;
  return s[i+1..];
}

object raw_sign(string digest)
{
  return rsa_pad(digest, 1, 0)->powm(d, n);
}

string cooked_sign(string digest)
{
  return raw_sign(digest)->digits(256);
}

int raw_verify(string digest, object s)
{
  return s->powm(e, n) == rsa_pad(digest, 1, 0);
}

string encrypt(string s, mixed|void r)
{
  return rsa_pad(s, 2, r)->powm(e, n)->digits(256);
}

string decrypt(string s)
{
  return rsa_unpad(Gmp.mpz(s, 256)->powm(d, n), 2);
}

int rsa_size() { return n->size(); }

int public_key_equal (object rsa)
{
  return n == rsa->n && e == rsa->e;
}

#endif
