
//! The Digital Signature Algorithm DSA is part of the NIST Digital
//! Signature Standard DSS, FIPS-186 (1993).

#pike __REAL_VERSION__
#pragma strict_types
#require constant(Crypto.Hash)

inherit Crypto.Sign;

@Pike.Annotations.Implements(Crypto.Sign);

//! Returns the string @expr{"DSA"@}.
string(7bit) name() { return "DSA"; }

class State {
  inherit ::this_program;

  @Pike.Annotations.Implements(Crypto.Sign.State);

  protected string _sprintf(int t)
  {
    return t=='O' && sprintf("%O(%d,%d)", this_program, p->size(), q->size());
  }

  //
  // --- Variables and accessors
  //

  protected Gmp.mpz|zero p; // Modulo
  protected Gmp.mpz|zero q; // Group order
  protected Gmp.mpz|zero g; // Generator

  protected Gmp.mpz|zero y; // Public key
  protected Gmp.mpz|zero x; // Private key

  protected function(int(0..):string(8bit)) random = random_string;

  Gmp.mpz|zero get_p() { return p; } //! Returns the DSA modulo (p).
  Gmp.mpz|zero get_q() { return q; } //! Returns the DSA group order (q).
  Gmp.mpz|zero get_g() { return g; } //! Returns the DSA generator (g).
  Gmp.mpz|zero get_y() { return y; } //! Returns the DSA public key (y).
  Gmp.mpz|zero get_x() { return x; } //! Returns the DSA private key (x).

  //! Sets the random function, used to generate keys and parameters, to
  //! the function @[r]. Default is @[random_string].
  this_program set_random(function(int(0..):string(8bit)) r)
  {
    random = r;
    return this;
  }

  //! Returns the string @expr{"DSA"@}.
  string(7bit) name() { return "DSA"; }

  //
  // --- Key methods
  //

  //! Sets the public key in this DSA object.
  //! @param modulo
  //!   This is the p parameter.
  //! @param order
  //!   This is the group order q parameter.
  //! @param generator
  //!   This is the g parameter.
  //! @param kye
  //!   This is the public key y parameter.
  this_program set_public_key(Gmp.mpz modulo, Gmp.mpz order,
			      Gmp.mpz generator, Gmp.mpz key)
  {
    p = modulo;
    q = order;
    g = generator;
    y = key;
    return this;
  }

  //! Sets the public key in this DSA object.
  //!
  //! @param params
  //!   The finite-field diffie-hellman group parameters.
  //! @param key
  //!   The public key y parameter.
  variant this_program set_public_key(.DH.Parameters params,
				      Gmp.mpz key)
  {
    p = params->p;
    q = params->q;
    g = params->g;
    y = key;
    return this;
  }

  //! Compares the public key in this object with that in the provided
  //! DSA object.
  int(0..1) public_key_equal(object other)
  {
    this_program dsa = [object(this_program)]other;
    return (p == dsa->get_p()) && (q == dsa->get_q()) &&
      (g == dsa->get_g()) && (y == dsa->get_y());
  }

  //! Compares the keys of this DSA object with something other.
  protected int(0..1) _equal(mixed other)
  {
    if (!objectp(other) || (object_program(other) != object_program(this)) ||
	!public_key_equal([object(this_program)]other)) {
      return 0;
    }
    this_program dsa = [object(this_program)]other;
    return x == dsa->get_x();
  }

  //! Sets the private key, the x parameter, in this DSA object.
  this_program set_private_key(Gmp.mpz secret)
  {
    x = secret;
    return this;
  }

  //
  // --- Key generation
  //

#if !constant(Nettle.dsa_generate_keypair)

#define SEED_LENGTH 20
  protected string(8bit) nist_hash(Gmp.mpz x)
  {
    string(8bit) s = x->digits(256);
    return .SHA1.hash(s[sizeof(s) - SEED_LENGTH..]);
  }

  // The (slow) NIST method of generating a DSA prime pair. Algorithm
  // 4.56 of Handbook of Applied Cryptography.
  protected array(Gmp.mpz) nist_primes(int l)
  {
    if ( (l < 0) || (l > 8) )
      error( "Unsupported key size.\n" );

    int L = 512 + 64 * l;

    int n = (L-1) / 160;
    //  int b = (L-1) % 160;

    for (;;)
    {
      /* Generate q */
      string(8bit) seed = random(SEED_LENGTH);
      Gmp.mpz s = Gmp.mpz(seed, 256);

      string(8bit) h = (nist_hash(s) ^ nist_hash(s + 1));

      h = sprintf("%c%s%c",
                  (h[0] | 0x80),
		  h[1..<1],
                  (h[-1] | 1));

      Gmp.mpz q = Gmp.mpz(h, 256);

      if (!q->probably_prime_p())
	continue;

      /* q is a prime, with overwelming probability. */

      int i, j;

      for (i = 0, j = 2; i < 4096; i++, j += n+1)
      {
	string(8bit) buffer = "";
	int k;

	for (k = 0; k<= n; k++)
          buffer = nist_hash(s + j + k) + buffer;

	buffer = buffer[sizeof(buffer) - L/8 ..];
        buffer[0] = buffer[0] | 0x80;

	Gmp.mpz p = Gmp.mpz(buffer, 256);

	p -= p % (2 * q) - 1;

	if (p->probably_prime_p())
	{
	  /* Done */
	  return ({ p, q });
	}
      }
    }
  }

  protected Gmp.mpz find_generator(Gmp.mpz p, Gmp.mpz q)
  {
    Gmp.mpz e = ((p - 1) / q);
    Gmp.mpz g = p;

    do {
      /* A random number in { 2, 3, ... p - 2 } */
      g = (random_number(p-3) + 2)
	/* Exponentiate to get an element of order 1 or q */
	->powm(e, p);
    } while (g == 1);

    return g;
  }

  // Generate key parameters (p, q and g) using the NIST DSA prime pair
  // generation algorithm. @[bits] must be multiple of 64.
  protected void generate_parameters(int bits)
  {
    if (!bits || bits % 64)
      error( "Unsupported key size.\n" );

    [p, q] = nist_primes(bits / 64 - 8);

    if (p % q != 1)
      error( "Internal error.\n" );

    if (q->size() != 160)
      error( "Internal error.\n" );

    g = find_generator(p, q);

    if ( (g == 1) || (g->powm(q, p) != 1))
      error( "Internal error.\n" );
  }

  variant this_program generate_key(int p_bits, int q_bits)
  {
    if(q_bits!=160)
      error("Only 1024/160 supported with Nettle version < 2.0\n");
    generate_parameters(1024);
    return generate_key();
  }

#else // !constant(Nettle.dsa_generate_keypair)

  //! Generates DSA parameters (p, q, g) and key (x, y). Depending on
  //! Nettle version @[q_bits] can be 160, 224 and 256 bits. 160 works
  //! for all versions.
  variant this_program generate_key(int p_bits, int q_bits)
  {
    [ p, q, g, y, x ] = Nettle.dsa_generate_keypair(p_bits, q_bits, random);
    return this;
  }

#endif

  //! Generates a public/private key pair with the specified
  //! finite field diffie-hellman parameters.
  variant this_program generate_key(.DH.Parameters params)
  {
    p = params->p;
    g = params->g;
    q = params->q;

    [y, x] = params->generate_keypair(random);
    return this;
  }

  //! Generates a public/private key pair. Needs the public parameters
  //! p, q and g set, through one of @[set_public_key],
  //! @[generate_key(int,int)] or @[generate_key(params)].
  variant this_program generate_key()
  {
    /* x in { 2, 3, ... q - 1 } */
    if(!p || !q || !g) error("Public parameters not set..\n");
    x = [object(Gmp.mpz)](random_number( [object(Gmp.mpz)](q-2) ) + 2);
    y = g->powm(x, p);

    return this;
  }


  //
  // --- PKCS methods
  //

#define Sequence Standards.ASN1.Types.Sequence
#define Integer Standards.ASN1.Types.Integer
#define BitString Standards.ASN1.Types.BitString

  //! Returns the AlgorithmIdentifier as defined in
  //! @rfc{5280:4.1.1.2@} including the DSA parameters.
  Sequence pkcs_algorithm_identifier()
  {
    return
      Sequence( ({ Standards.PKCS.Identifiers.dsa_id,
		   Sequence( ({ Integer(get_p()),
				Integer(get_q()),
				Integer(get_g())
			     }) )
		}) );
  }


  //! Returns the PKCS-1 algorithm identifier for DSA and the provided
  //! hash algorithm. Only @[SHA1] supported.
  object(Sequence)|zero pkcs_signature_algorithm_id(__builtin.Nettle.Hash hash)
  {
    switch(hash->name())
    {
    case "sha1":
      return Sequence( ({ Standards.PKCS.Identifiers.dsa_sha_id }) );
      break;
    case "sha224":
      return Sequence( ({ Standards.PKCS.Identifiers.dsa_sha224_id }) );
      break;
    case "sha256":
      return Sequence( ({ Standards.PKCS.Identifiers.dsa_sha256_id }) );
      break;
    }
    return 0;
  }

  //! Creates a SubjectPublicKeyInfo ASN.1 sequence for the object.
  //! See @rfc{5280:4.1.2.7@}.
  Sequence pkcs_public_key()
  {
    return Sequence(({
		      pkcs_algorithm_identifier(),
		      BitString(Integer(get_y())->get_der()),
		    }));
  }

#undef BitString
#undef Integer
#undef Sequence

  //! Signs the @[message] with a PKCS-1 signature using hash algorithm
  //! @[h].
  string(8bit) pkcs_sign(string(8bit) message, __builtin.Nettle.Hash h)
  {
    array sign = map(raw_sign(hash(message, h)), Standards.ASN1.Types.Integer);
    return Standards.ASN1.Types.Sequence(sign)->get_der();
  }

  // FIXME: Consider implementing RFC 6979.

#define Object Standards.ASN1.Types.Object

  //! Verify PKCS-1 signature @[sign] of message @[message] using hash
  //! algorithm @[h].
  int(0..1) pkcs_verify(string(8bit) message, __builtin.Nettle.Hash h,
                        string(8bit) sign)
  {
    object(Object)|zero a = Standards.ASN1.Decode.secure_der_decode(sign);

    // The signature is the DER-encoded ASN.1 sequence Dss-Sig-Value
    // with the two integers r and s. See RFC 3279 section 2.2.2.
    if (!a
	|| (a->type_name != "SEQUENCE")
	|| (sizeof([array]a->elements) != 2)
	|| (sizeof( ([array(object(Object))]a->elements)->type_name -
		    ({ "INTEGER" }))))
      return 0;

    return raw_verify(hash(message, h),
		      [object(Gmp.mpz)]([array(object(Object))]a->elements)[0]->
		      value,
		      [object(Gmp.mpz)]([array(object(Object))]a->elements)[1]->
		      value);
  }

#undef Object

  //
  //  --- Below are methods for DSA applied in other standards.
  //

  //! Makes a DSA hash of the message @[msg].
  Gmp.mpz hash(string(8bit) msg, __builtin.Nettle.Hash h)
  {
    string(8bit) digest = h->hash(msg)[..q->size()/8-1];
    return [object(Gmp.mpz)](Gmp.mpz(digest, 256) % q);
  }

  protected Gmp.mpz random_number(Gmp.mpz n)
  {
    return [object(Gmp.mpz)](Gmp.mpz(random( [int(0..)](q->size() + 10 / 8)),
				     256) % n);
  }

  protected Gmp.mpz random_exponent()
  {
    return [object(Gmp.mpz)](random_number([object(Gmp.mpz)](q - 1)) + 1);
  }

  //! Sign the message @[h]. Returns the signature as two @[Gmp.mpz]
  //! objects.
  array(Gmp.mpz) raw_sign(Gmp.mpz h, Gmp.mpz k = random_exponent())
  {
    Gmp.mpz r = [object(Gmp.mpz)](g->powm(k, p) % q);
    Gmp.mpz s = [object(Gmp.mpz)]((k->invert(q) * (h + [object(Gmp.mpz)](x*r))) % q);

    return ({ r, s });
  }

  //! Verify the signature @[r],@[s] against the message @[h].
  int(0..1) raw_verify(Gmp.mpz h, Gmp.mpz r, Gmp.mpz s)
  {
    if ((r > q) || (s > q)) {
      return 0;
    }
    object(Gmp.mpz)|zero w;
    if (catch
      {
	w = s->invert(q);
      })
      /* Non-invertible */
      return 0;

    /* The inner %q's are redundant, as g^q == y^q == 1 (mod p) */
    return r == (g->powm( [object(Gmp.mpz)](w * h % q), p) *
		 y->powm( [object(Gmp.mpz)](w * r % q), p) % p) % q;
  }

  int(0..) key_size()
  {
    return p->size();
  }
}

//! Calling `() will return a @[State] object.
protected State `()()
{
  return State();
}
