/* $Id: rsa.pike,v 1.19 2000/05/04 11:22:56 grubba Exp $
 *
 * Follow the PKCS#1 standard for padding and encryption.
 */

#if constant(Gmp.mpz)

inherit Crypto._rsa;

#define bignum object(Gmp.mpz)
#define BIGNUM (Gmp.mpz)

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
					  return predef::random(255) + 1;
					} ));
    break;
  default:
    throw( ({ "Crypto.rsa->rsa_pad: Unknown type.\n",
		backtrace() }) );
  }    
  return BIGNUM(sprintf("%c", type) + cookie + "\0" + message, 256);
}

#if constant(_Crypto.rsa)
// Only the cooked variant is implemented in C. */
object raw_sign(string digest)
{
  return BIGNUM(cooked_sign(digest), 256);
}
#endif /* constant(_Crypto.rsa) */

object sign(string message, program h)
{
  string digest = Standards.PKCS.Signature.build_digestinfo(message, h());
  return raw_sign(digest);
}

int verify(string msg, program h, object sign)
{
  // werror(sprintf("msg: '%s'\n", Crypto.string_to_hex(msg)));
  string s = Standards.PKCS.Signature.build_digestinfo(msg, h());
  // werror(sprintf("rsa: s = '%s'\n", s));
  return raw_verify(s, sign);
}

string sha_sign(string message, mixed|void r)
{
  object hash = Crypto.sha();
  string s;

  hash->update(message);
  s = hash->digest();
  s = sprintf("%c%s%c%s", 4, "sha1", strlen(s), s);
  return cooked_sign(s);
}
  
int sha_verify(string message, string signature)
{
  object hash = Crypto.sha();
  string s;
  
  hash->update(message);
  s = hash->digest();
  s = sprintf("%c%s%c%s", 4, "sha1", strlen(s), s);

  return raw_verify(s, BIGNUM(signature, 256));
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

#endif
