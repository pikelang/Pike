/* $Id: rsa.pike,v 1.14 1998/08/26 06:12:52 nisse Exp $
 *
 * Follow the PKCS#1 standard for padding and encryption.
 */

#define bignum object(Gmp.mpz)
#define BIGNUM (Gmp.mpz)

import Standards.PKCS;

bignum n;  /* modulo */
bignum e;  /* public exponent */
bignum d;  /* private exponent (if known) */
int size;

/* Extra info associated with a private key. Not currently used. */
   
bignum p;
bignum q;

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

bignum get_prime(int bits, function r)
{
  int len = (bits + 7) / 8;
  int bit_to_set = 1 << ( (bits - 1) % 8);

  object p;
  
  do {
    string s = r(len);
    p = BIGNUM(sprintf("%c%s", (s[0] & (bit_to_set - 1))
			      | bit_to_set, s[1..]),
		      256)->next_prime();
  } while (p->size() > bits);
  
  return p;
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
					  return random(255) + 1;
					} ));
    break;
  default:
    throw( ({ "Crypto.rsa->rsa_pad: Unknown type.\n",
		backtrace() }) );
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

int raw_verify(string digest, object s)
{
  return s->powm(e, n) == rsa_pad(digest, 1, 0);
}

object sign(string message, program h, mixed|void r)
{
  // FIXME: The r argument is ignored and should be removed
  return raw_sign(Signature.build_digestinfo(message, h()));
}

int verify(string msg, program h, object sign)
{
  // FIXME: Use raw_verify()
  
  // werror(sprintf("msg: '%s'\n", Crypto.string_to_hex(msg)));
  string s = Signature.build_digestinfo(msg, h());
  // werror(sprintf("rsa: s = '%s'\n", s));
  // werror(sprintf("decrypted: '%s'\n", sign->powm(e, n)->digits(256)));
  string s2 = rsa_unpad(sign->powm(e, n), 1);
  // werror(sprintf("rsa: s2 = '%s'\n", s2));
  return s == s2;
}

string sha_sign(string message, mixed|void r)
{
  // FIXME: Use raw_sign()
  object hash = Crypto.sha();
  string s;

  hash->update(message);
  s = hash->digest();
  s = sprintf("%c%s%c%s", 4, "sha1", strlen(s), s);
  return rsa_pad(s, 1, r)->powm(d, n)->digits(256);
}
  
int sha_verify(string message, string signature)
{
  // FIXME: Use raw_verify()
  
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
  int s1 = bits / 2; /* Size of the first prime */
  int s2 = bits - s1;
  
  string msg = "This is a valid RSA key pair\n";
  
  do
  {
    p = get_prime(s1, r);
    q = get_prime(s2, r);
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

int rsa_size() { return n->size(); }
