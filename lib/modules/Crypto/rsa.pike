/* rsa.pike
 *
 * Follow the PKCS#1 standard for padding and encryption.
 * However, PKCS#1 style signing (involving ISO Object Identifiers) is
 * not yet implemented.
 */

#define bignum object(Gmp.mpz)
#define BIGNUM (Gmp.mpz)

bignum n;  /* modulo */
bignum e;  /* public exponent */
bignum d;  /* private exponent (if known) */
int size;

int encrypt_mode; /* For block cipher compatible functions */

void set_public_key(bignum modulo, bignum pub)
{
  n = modulo;
  e = pub;
  size = n->size(256);
  if (size < 12)
    throw( ({ "Crypto.rsa->set_public_key: Too small modulo.\n",
		backtrace() }) );
}

void set_private_key(bignum priv)
{
  d = priv;
}

int query_blocksize() { return size - 3; }

bignum rsa_pad(string message, int type, mixed|void random)
{
  string cookie;
  int len;

  len = size - 3 - strlen(message);
  /*  write(sprintf("%d, %d, %d, %s", len, size, strlen(message), message)); */
  if (len < 8)
    throw( ({ "Crypto.rsa->rsa_pad: To large block.\n",
		backtrace() }) );

  if (random)
    cookie = replace(random->read(len), "\0", "\1");
  else
    cookie = sprintf("%@c", map(allocate(len), lambda(int dummy)
				{
				  return random(255) + 1;
				} ));
  
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

string sha_sign(string message, mixed|void r)
{
  object hash = Crypto.sha();
  string s;

  hash->update(message);
  s = hash->digest();
  s = sprintf("%c%s%c%s", 4, "sha1", strlen(s), s);
  return rsa_pad(s, 1, r)->powm(d, n)->digits(256);
}
  
int sha_verify(string message, string signature)
{
  object hash = Crypto.sha();
  string s;
  
  hash->update(message);
  s = hash->digest;
  s = sprintf("%c%s%c%s", 4, "sha1", strlen(s), s);

  return s == rsa_unpad(BIGNUM(signature, 256)->powm(e, n), 1);
}

object set_encrypt_key(array(bignum) key)
{
  set_public_key(key[0], key[1]);
  encrypt_mode = 1;
  return this_object();
}

object set_decrypt_key(array(bignum) key)
{
  set_public_key(key[0], key[1]);
  set_private_key(key[2]);
  encrypt_mode = 0;
  return this_object();
}

object crypt_block(string msg)
{
  return (encrypt_mode ?
	  rsa_pad(msg, 2)->powm(e, n)->digits(256)
	  : rsa_unpad(BIGNUM(msg, 256)->powm(d, n), 2));
}
