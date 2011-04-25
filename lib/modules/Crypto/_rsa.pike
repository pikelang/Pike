/* $Id$
 *
 * Follow the PKCS#1 standard for padding and encryption.
 */

#pike __REAL_VERSION__

#if constant(Gmp.mpz)

#define bignum object(Gmp.mpz)
#define BIGNUM (Gmp.mpz)

bignum n;  /* modulo */
bignum e;  /* public exponent */
bignum d;  /* private exponent (if known) */
int size;

/* Extra info associated with a private key. Not currently used. */
   
bignum p;
bignum q;

bignum get_n()
{
  return n;
}

bignum get_e()
{
  return e;
}

bignum get_d()
{
  return d;
}

bignum get_p()
{
  return p;
}

bignum get_q()
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

object set_public_key(bignum modulo, bignum pub)
{
  n = modulo;
  e = pub;
  size = n->size(256);
  if (size < 12)
    error( "Crypto.rsa->set_public_key: Too small modulo.\n" );
  return this_object();
}

object set_private_key(bignum priv, array(bignum)|void extra)
{
  d = priv;
  if (extra)
  {
    p = extra[0];
    q = extra[1];
  }
  return this_object();
}

int query_blocksize() { return size - 3; }

bignum rsa_pad(string message, int type, mixed|void random)
{
  string cookie;
  int len;

  len = size - 3 - strlen(message);
  /*  write(sprintf("%d, %d, %d, %s", len, size, strlen(message), message)); */
  if (len < 8)
    error( "Crypto.rsa->rsa_pad: Too large block.\n" );

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
  return BIGNUM(sprintf("%c", type) + cookie + "\0" + message, 256);
}

string rsa_unpad(bignum block, int type)
{
  string s = block->digits(256);
  int i = search(s, "\0");

  if ((i < 9) || (strlen(s) != (size - 1)) || (s[0] != type))
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
  return rsa_unpad(BIGNUM(s, 256)->powm(d, n), 2);
}

int rsa_size() { return n->size(); }

int public_key_equal (object rsa)
{
  return n == rsa->n && e == rsa->e;
}

#endif
