/* $Id: rsa.pike,v 1.11 1997/10/12 14:39:47 grubba Exp $
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

object set_public_key(bignum modulo, bignum pub)
{
  n = modulo;
  e = pub;
  size = n->size(256);
  if (size < 12)
    throw( ({ "Crypto.rsa->set_public_key: Too small modulo.\n",
		backtrace() }) );
  return this_object();
}

object set_private_key(bignum priv)
{
  d = priv;
  return this_object();
}

bignum get_prime(int bits, function r)
{
  int len = (bits + 7) / 8;
  int bit_to_set = 1 << ( (bits - 1) % 8);
  
  string s = r(len);
  return BIGNUM(sprintf("%c%s", (s[0] & (bit_to_set - 1)) | bit_to_set, s[1..]),
		256)->next_prime();
}

int query_blocksize() { return size - 3; }

bignum rsa_pad(string message, int type, mixed|void random)
{
  string cookie;
  int len;

  len = size - 3 - strlen(message);
  /*  write(sprintf("%d, %d, %d, %s", len, size, strlen(message), message)); */
  if (len < 8)
    throw( ({ "Crypto.rsa->rsa_pad: Too large block.\n",
		backtrace() }) );

  if (random)
    cookie = replace(random(len), "\0", "\1");
  else
    cookie = sprintf("%@c", Array.map(allocate(len), lambda(int dummy)
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

object sign(string message, program h, mixed|void r)
{
  return rsa_pad(pkcs.build_digestinfo(message, h()), 1, r)->powm(d, n);
}

int verify(string msg, program h, object sign)
{
  string s = pkcs.build_digestinfo(msg, h());
  return s == rsa_unpad(sign->powm(e, n), 1);
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
  s = hash->digest();
  s = sprintf("%c%s%c%s", 4, "sha1", strlen(s), s);

  return s == rsa_unpad(BIGNUM(signature, 256)->powm(e, n), 1);
}

object generate_key(int bits, function|void r)
{
  if (!r)
    r = Crypto.randomness.really_random()->read;
  if (bits < 128)
    throw( ({ "Crypto.rsa->generate_key: ridicously small key\n",
		backtrace() }) );
  bits /= 2; /* Size of each of the primes */

  string msg = "This is a valid RSA key pair\n";
  
  do
  {
    bignum p = get_prime(bits, r);
    bignum q = get_prime(bits, r);
    bignum phi = Gmp.mpz(p-1)*Gmp.mpz(q-1);

    array gs; /* gcd(pub, phi), and pub^-1 mod phi */
    bignum pub = Gmp.mpz(random(1 << 30) | 0x10001);

    while ((gs = pub->gcdext2(phi))[0] != 1)
      pub += 1;

//    werror(sprintf("p = %s\nq = %s\ne = %s\nd = %s\n",
//		   p->digits(), q->digits(), pub->digits(), gs[1]->digits()));
    if (gs[1] < 0)
      gs[1] += phi;
    
    set_public_key(p * q, pub);
    set_private_key(gs[1]);

//    werror(sprintf("p = %s\nq = %s\ne = %s\nd = %s\n",
//		   p->digits(), q->digits(), pub->digits(), gs[1]->digits()));
  } while (!sha_verify(msg, sha_sign(msg, r)));
  return this_object();
}

string encrypt(string s, mixed|void r)
{
  return rsa_pad(s, 2, r)->powm(e, n)->digits(256);
}

string decrypt(string s)
{
  return rsa_unpad(BIGNUM(s, 256)->powm(d, n), 2);
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

string crypt_block(string s)
{
  return (encrypt_mode ? encrypt(s) : decrypt(s));
}
