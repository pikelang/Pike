/*
 * Follow the PKCS#1 standard for padding and encryption.
 */

#pike __REAL_VERSION__
#pragma strict_types
#require constant(Crypto.Hash)

inherit Crypto.Sign;

//! Returns the string @expr{"RSA"@}.
string(8bit) name() { return "RSA"; }

protected class LowState {
  inherit Sign::State;

  protected string _sprintf(int t)
  {
    return t=='O' && sprintf("%O(%d)", this_program, n->size());
  }

  //
  // --- Variables and accessors
  //

  protected Gmp.mpz|zero n;  /* modulo */
  protected Gmp.mpz|zero e;  /* public exponent */
  protected Gmp.mpz|zero d;  /* private exponent (if known) */

  /* Extra info associated with a private key. Not currently used. */

  protected Gmp.mpz|zero p;
  protected Gmp.mpz|zero q;

  protected function(int(0..):string(8bit)) random = random_string;

  Gmp.mpz get_n() { return n; } //! Returns the RSA modulo (n).
  Gmp.mpz get_e() { return e; } //! Returns the RSA public exponent (e).

  //! Returns the RSA private exponent (d), if known.
  Gmp.mpz get_d() { return d; }

  Gmp.mpz get_p() { return p; } //! Returns the first RSA prime (p), if known.
  Gmp.mpz get_q() { return q; } //! Returns the second RSA prime (q), if known.

  //! Sets the random function, used to generate keys and parameters, to
  //! the function @[r]. Default is @[random_string].
  this_program set_random(function(int(0..):string(8bit)) r)
  {
    random = r;
    return this;
  }

  //! Returns the string @expr{"RSA"@}.
  string(8bit) name() { return "RSA"; }

  //! Get the JWS algorithm identifier for a hash.
  //!
  //! @returns
  //!   Returns @expr{0@} (zero) on failure.
  //!
  //! @seealso
  //!   @rfc{7518:3.1@}
  string(7bit) jwa(.Hash hash);

  //
  // --- Key methods
  //

  //! Can be initialized with a mapping with the elements n, e, d, p and q.
  //!
  //! The mapping can either contain integer values, or be an @rfc{7517@}
  //! JWK-style mapping with @tt{kty@} set to @expr{"RSA"@} and contain
  //! @[MIME.encode_base64url()]-encoded values.
  //!
  //! @seealso
  //!   @[jwk()]
  protected void create(mapping(string(8bit):Gmp.mpz|int|string(7bit))|void params)
  {
    if(!params) return;
    if (params->kty == "RSA") {
      // RFC 7517 JWK encoded key.
      mapping(string(8bit):string(7bit)) jwk =
	[mapping(string(8bit):string(7bit))]params;
      params = ([]);
      foreach(({ "n", "e", "d", "p", "q" }), string s) {
	string(7bit)|zero val;
	if (!zero_type(val = jwk[s])) {
	  // RFC 7517 A.1:
	  //    In both cases, integers are represented using the base64url
	  //    encoding of their big-endian representations.
	  params[s] = Gmp.mpz(MIME.decode_base64url([string]val), 256);
	}
      }
    }
    if( params->n && params->e )
      set_public_key([object(Gmp.mpz)]params->n,
		     [object(Gmp.mpz)]params->e);
    if( params->d )
      set_private_key([object(Gmp.mpz)]params->d,
		      [array(Gmp.mpz)]({ params->p, params->q, params->n }));
  }

  //! Sets the public key.
  //! @param modulo
  //!   The RSA modulo, often called n. This value needs to be >=12.
  //! @param pub
  //!   The public RSA exponent, often called e.
  this_program set_public_key(Gmp.mpz|int modulo, Gmp.mpz|int pub)
  {
    n = Gmp.mpz(modulo);
    e = Gmp.mpz(pub);
    if (n->size(256) < 12)
      error( "Too small modulo.\n" );
    return this;
  }

  //! Compares the public key of this RSA object with another RSA
  //! object.
  int(0..1) public_key_equal(this_program rsa)
  {
    return n == rsa->get_n() && e == rsa->get_e();
  }

  //! Compares the keys of this RSA object with something other.
  protected int(0..1) _equal(mixed other)
  {
    if (!objectp(other) || (object_program(other) != object_program(this)) ||
	!public_key_equal([object(this_program)]other)) {
      return 0;
    }
    this_program rsa = [object(this_program)]other;
    return d == rsa->get_d();
  }

  //! Sets the private key.
  //! @param priv
  //!   The private RSA exponent, often called d.
  //! @param extra
  //!   @array
  //!     @elem Gmp.mpz|int 0
  //!       The first prime, often called p.
  //!     @elem Gmp.mpz|int 1
  //!       The second prime, often called q.
  //!   @endarray
  this_program set_private_key(Gmp.mpz|int priv, array(Gmp.mpz|int)|void extra)
  {
    d = Gmp.mpz(priv);
    if (extra)
    {
      p = Gmp.mpz(extra[0]);
      q = Gmp.mpz(extra[1]);
      n = [object(Gmp.mpz)](p*q);
    }
    return this;
  }

  //
  // --- Key generation
  //

#if constant(Nettle.rsa_generate_keypair)

  this_program generate_key(int bits, void|int e)
  {
    // While a smaller e is possible, and more efficient, using 0x10001
    // has become standard and is the only value supported by several
    // TLS implementations.
    if(!e)
      e = 0x10001;
    else
    {
      if(!(e&1)) error("e needs to be odd.\n");
      if(e<3) error("e is too small.\n");
      if(e->size()>bits) error("e has to be smaller in size than the key.\n");
    }

    if(bits<89) error("Too small key length.\n");

    array(Gmp.mpz) key = Nettle.rsa_generate_keypair(bits, e, random);
    if(!key) error("Error generating key.\n");
    [ n, d, p, q ] = key;
    this::e = Gmp.mpz(e);
    return this;
  }

#else

  // Generate a prime with @[bits] number of bits using random function
  // @[r].
  protected Gmp.mpz get_prime(int bits, function(int(0..):string(8bit)) r)
  {
    int len = (bits + 7) / 8;
    int bit_to_set = 1 << ( (bits - 1) % 8);

    Gmp.mpz p;

    do {
      string(8bit) s = r([int(0..)]len);
      p = Gmp.mpz(sprintf("%c%s", (s[0] & (bit_to_set - 1))
			  | bit_to_set, s[1..]),
		  256)->next_prime();
    } while (p->size() > bits);

    return p;
  }

  //! Generate a valid RSA key pair with the size @[bits] using the
  //! random function set with @[set_random()]. The public exponent @[e]
  //! will be used, which defaults to 65537. Keys must be at least 89
  //! bits.
  this_program generate_key(int(128..) bits, void|int|Gmp.mpz e)
  {
    if (bits < 128)
      error( "Ridiculously small key.\n" );
    if( e )
    {
      if(!(e&1)) error("e needs to be odd.\n");
      if(e<3) error("e is too small.\n");
      if(e->size()>bits) error("e has to be smaller in size than the key.\n");
      if (e->size() >= 64) {
	// Make the testsuite happy...
	error("e: %O is too large.\n", e);
      }
    }

    /* NB: When multiplying two n-bit integers,
     *     you're most likely to get an (2n - 1)-bit result.
     *     We therefore add an extra bit to s2.
     *
     * cf [bug 6620].
     */

    int s1 = bits / 2; /* Size of the first prime */
    int s2 = 1 + bits - s1;

    string(8bit) msg = "A" * (bits/8-3-8);

    do {
      Gmp.mpz p;
      Gmp.mpz q;
      Gmp.mpz mod;
      do {
	p = get_prime(s1, random);
	q = get_prime(s2, random);
	mod = [object(Gmp.mpz)](p * q);
      } while (mod->size() != bits);
      Gmp.mpz phi = [object(Gmp.mpz)](Gmp.mpz([object(Gmp.mpz)](p-1))*
				      Gmp.mpz([object(Gmp.mpz)](q-1)));

      array(Gmp.mpz) gs; /* gcd(pub, phi), and pub^-1 mod phi */

      // For a while it was thought that small exponents were a security
      // problem, but turned out was a padding problem. The exponent
      // 0x10001 has however become common practice, although a smaller
      // value would be more efficient.
      Gmp.mpz pub = Gmp.mpz(e || 0x10001);

      // For security reason we need to ensure no common denominator
      // between n and phi. We could create a different exponent, but
      // some Crypto packages are hard coded for 0x10001, so instead
      // we'll just start over.
      if ((gs = pub->gcdext2(phi))[0] != 1)
	continue;

      if (gs[1] < 0)
	gs[1] += phi;

      set_public_key(mod, pub);
      set_private_key(gs[1], ({ p, q }));

    } while (!raw_verify(msg, raw_sign(msg)));
    return this;
  }

#endif


  //
  // --- Block cipher compatibility.
  //

  protected int encrypt_mode; // For block cipher compatible functions

  //! Sets the public key to @[key] and the mode to encryption.
  //! @seealso
  //!   @[set_decrypt_key], @[crypt]
  this_program set_encrypt_key(array(Gmp.mpz) key)
  {
    set_public_key(key[0], key[1]);
    encrypt_mode = 1;
    return this;
  }

  //! Sets the public key to @[key]and the mod to decryption.
  //! @seealso
  //!   @[set_encrypt_key], @[crypt]
  this_program set_decrypt_key(array(Gmp.mpz) key)
  {
    set_public_key(key[0], key[1]);
    set_private_key(key[2]);
    encrypt_mode = 0;
    return this;
  }

  //! Returns the size of the key in terms of number of bits.
  int(0..) key_size() { return n->size(); }

  //
  // Prototypes for functions in PKCS1_5State.
  //
  // These are needed to keep the type checker happy when we
  // return this_program, and assign to variables declared as State.
  //
  string(8bit) encrypt(string(8bit) s, function(int:string(8bit))|void r);
  string(8bit) decrypt(string(8bit) s);
  string(8bit) crypt(string(8bit) s);
  int block_size();
  Gmp.mpz rsa_pad(string(8bit) message, int(1..2) type,
		  function(int(0..):string(8bit))|void random);
  string(8bit) rsa_unpad(Gmp.mpz block, int type);
  Gmp.mpz raw_sign(string(8bit) digest);
  int(0..1) raw_verify(string(8bit) digest, Gmp.mpz s);

  //! Generate a JWK-style mapping of the object.
  //!
  //! @param private_key
  //!   If true, include the private key in the result.
  //!   Note that if the private key isn't known, the function
  //!   will fail (and return @expr{0@}).
  //!
  //! @returns
  //!   Returns a JWK-style mapping on success, and @expr{0@} (zero)
  //!   on failure.
  //!
  //! @seealso
  //!   @[create()], @[Web.encode_jwk()], @rfc{7517:4@}, @rfc{7518:6.3@}
  mapping(string(7bit):string(7bit)) jwk(int(0..1)|void private_key)
  {
    if (!n) return 0;	// Not initialized.
    mapping(string(7bit):string(7bit)) jwk = ([
      "kty":"RSA",
      "n": MIME.encode_base64url(n->digits(256)),
      "e": MIME.encode_base64url(e->digits(256)),
    ]);
    if (private_key) {
      if (!d) return 0;	// Private key not known.

      jwk->d = MIME.encode_base64url(d->digits(256));
      jwk->p = MIME.encode_base64url(p->digits(256));
      jwk->q = MIME.encode_base64url(q->digits(256));
    }
    return jwk;
  }

  //
  // Prototypes for switching between different signature methods.
  //
  this_program `PSS();
  this_program `OAEP();
  this_program `PKCS1_5();
}

#define Sequence Standards.ASN1.Types.Sequence

private class PKCS_RSA_class {
  Sequence signature_algorithm_id(.Hash);
  Sequence pss_signature_algorithm_id(.Hash, int(0..)|void saltlen);
  Sequence build_public_key(global::State);
}
private object(PKCS_RSA_class) PKCS_RSA =
  [object(PKCS_RSA_class)]Standards.PKCS["RSA"];

//! Implementation of RSAES-OAEP (Optimal Asymmetric Encryption Padding).
//!
//! @seealso
//!   @rfc{3447:7.1@}
class OAEPState {
  inherit LowState;

  local {
    //! Get the OAEP encryption state.
    this_program `OAEP() { return this_program::this; }

    string(7bit) name() { return "RSAES-OAEP"; }

    protected .Hash hash_alg = .SHA1;

    optional .Hash get_hash_algorithm()
    {
      return hash_alg;
    }

    optional void set_hash_algorithm(.Hash h)
    {
      if ((h->digest_size() * 2 + 2) >= n->size(256)) {
	error("Too large hash digest (%d, max: %d) for modulo.\n",
	      h->digest_size(), n->size(256)/2 - 3);
      }
      hash_alg = h;
    }

    //! Encrypt or decrypt depending on set mode.
    //! @seealso
    //!   @[set_encrypt_key], @[set_decrypt_key]
    string(8bit) crypt(string(8bit) s, string(8bit)|void label)
    {
      return (encrypt_mode ? encrypt(s, UNDEFINED, label) : decrypt(s, label));
    }

    //! Returns the crypto block size, in bytes, or zero if not yet set.
    int block_size()
    {
      // FIXME: This can be both zero and negative...
      return n->size(256) - 2*(hash_alg->digest_size() + 1);
    }

    string(8bit) encrypt(string(8bit) s,
			 function(int(0..):string(8bit))|void r,
			 string(8bit)|void label)
    {
      if (!r) r = random;
      string(8bit) em =
	hash_alg->eme_oaep_encode(s, n->size(256),
				  r(hash_alg->digest_size()),
				  label);
      if (!em) error("Message too long.\n");
      return [string(8bit)]sprintf("%*c",
				   n->size(256), Gmp.mpz(em, 256)->powm(e, n));
    }

    string(8bit) decrypt(string(8bit) s, string(8bit)|void label)
    {
      if (sizeof(s) != n->size(256)) {
	error("Decryption error.\n");
      }
      Gmp.mpz c = Gmp.mpz(s, 256);
      if (c >= n) {
	error("Decryption error.\n");
      }
      string(8bit) m =
	hash_alg->eme_oaep_decode([string(8bit)]
				  sprintf("%*c", n->size(256), c->powm(d, n)),
				  label);
      if (!m) {
	error("Decryption error.\n");
      }
      return m;
    }
  }
}

//! RSA PSS signatures (@rfc{3447:8.1@}).
//!
//! @seealso
//!   @[PKCS1_5State]
class PSSState {
  inherit OAEPState;

  local {
    //! Get the PSS signature state.
    this_program `PSS() { return this_program::this; }

    protected int(0..) default_salt_size = 20;

    //! Calls @[Standards.PKCS.RSA.pss_signature_algorithm_id] with the
    //! provided @[hash] and @[saltlen].
    //!
    //! @param hash
    //!   Hash algorithm for the signature.
    //!
    //! @param saltlen
    //!   Length of the salt for the signature. Defaults to the
    //!   value returned by @[salt_size()].
    Sequence pkcs_signature_algorithm_id(.Hash hash, int(0..)|void saltlen)
    {
      if (undefinedp(saltlen)) saltlen = default_salt_size;
      return PKCS_RSA->pss_signature_algorithm_id(hash, saltlen);
    }

    string(7bit) name() { return "RSASSA-PSS"; }

    //! Get the JWS algorithm identifier for a hash.
    //!
    //! @returns
    //!   Returns @expr{0@} (zero) on failure.
    //!
    //! @seealso
    //!   @rfc{7518:3.1@}
    string(7bit) jwa(.Hash hash)
    {
      switch(hash->name()) {
      case "sha256":
	return "PS256";
      case "sha384":
	return "PS384";
      case "sha512":
	return "PS512";
      }
      return 0;
    }

    optional int(0..) salt_size() { return default_salt_size; }

    optional void set_salt_size(int(0..) salt_size)
    {
      default_salt_size = salt_size;
    }

    protected int(0..1) _equal(mixed x)
    {
      if (!objectp(x) || (object_program(x) != object_program(this))) {
	return 0;
      }
      if (object_program(this) == this::this_program) {
	// We've got a PSSState object.
	if (salt_size != ([object(this_program)]x)->salt_size) return 0;
      }
      return ::_equal(x);
    }

    //! Signs the @[message] with a RSASSA-PSS signature using hash
    //! algorithm @[h].
    //!
    //! @param message
    //!   Message to sign.
    //!
    //! @param h
    //!   Hash algorithm to use.
    //!
    //! @param salt
    //!   Either of
    //!   @mixed
    //!     @type int(0..)
    //!       Use a @[random] salt of this length for the signature.
    //!     @type zero|void
    //!       Use a @[random] salt of length @[salt_size()].
    //!     @type string(8bit)
    //!       Use this specific salt.
    //!   @endmixed
    //!
    //! @returns
    //!   Returns the signature on success, and @expr{0@} (zero)
    //!   on failure (typically that the hash + salt combo is too
    //!   large for the RSA modulo).
    //!
    //! @seealso
    //!   @[pkcs_verify()], @[salt_size()], @rfc{3447:8.1.1@}
    string(8bit) pkcs_sign(string(8bit) message, .Hash h,
			   string(8bit)|int(0..)|void salt)
    {
      if (undefinedp(salt)) salt = default_salt_size;

      //    1. EMSA-PSS encoding: Apply the EMSA-PSS encoding operation
      //       (Section 9.1.1) to the message M to produce an encoded
      //       message EM of length \ceil ((modBits - 1)/8) octets such
      //       that the bit length of the integer OS2IP (EM) (see
      //       Section 4.2) is at most modBits - 1, where modBits is
      //       the length in bits of the RSA modulus n:
      //
      //       EM = EMSA-PSS-ENCODE (M, modBits - 1).
      //
      //       Note that the octet length of EM will be one less than
      //       k if modBits - 1 is divisible by 8 and equal to k
      //       otherwise. If the encoding operation outputs "message
      //       too long," output "message too long" and stop. If the
      //       encoding operation outputs "encoding error," output
      //       "encoding error" and stop.
      if (intp(salt)) {
	salt = random([int(0..)]salt);
      }
      string(8bit) em =
	h->emsa_pss_encode(message, [int(1..)](n->size()-1),
			   [string(8bit)]salt);

      // RSA signature:
      //   a. Convert the encoded message EM to an integer message
      //      representative m (see Section 4.2):
      //
      //      m = OS2IP (EM).
      Gmp.mpz m = Gmp.smpz(em, 256);

      //   b. Apply the RSASP1 signature primitive (Section 5.2.1) to the RSA
      //      private key K and the message representative m to produce an
      //      integer signature representative s:
      //
      //      s = RSASP1 (K, m).
      Gmp.mpz s = m->powm(d, n);

      //   c. Convert the signature representative s to a signature S of
      //      length k octets (see Section 4.1):
      //
      //      S = I2OSP (s, k).
      //
      // Output the signature S.
      return [string(8bit)]sprintf("%*c", n->size(256), s);
    }

    //! Verify RSASSA-PSS signature @[sign] of message @[message] using hash
    //! algorithm @[h].
    //!
    //! @seealso
    //!   @rfc{3447:8.1.2@}
    int(0..1) pkcs_verify(string(8bit) message, .Hash h, string(8bit) sign,
			  int(0..)|void saltlen)
    {
      if (undefinedp(saltlen)) saltlen = default_salt_size;

      // 1. Length checking: If the length of the signature S is not k
      //    octets, output "invalid signature" and stop.
      if (sizeof(sign) != n->size(256)) {
	werror("Bad size\n");
	return 0;
      }

      // 2. RSA verification:
      //    a. Convert the signature S to an integer signature representative
      //       s (see Section 4.2):
      //
      //       s = OS2IP (S).
      Gmp.mpz s = Gmp.smpz(sign, 256);

      //    b. Apply the RSAVP1 verification primitive (Section 5.2.2) to the
      //       RSA public key (n, e) and the signature representative s to
      //       produce an integer message representative m:
      //
      //       m = RSAVP1 ((n, e), s).
      Gmp.mpz m = s->powm(e, n);

      //       If RSAVP1 output "signature representative out of range," output
      //       "invalid signature" and stop.
      if (m >= n) {
	werror("Out of range\n");
	return 0;
      }

      //    c. Convert the message representative m to an encoded message EM
      //       of length emLen = \ceil ((modBits - 1)/8) octets, where modBits
      //       is the length in bits of the RSA modulus n (see Section 4.1):
      //
      //       EM = I2OSP (m, emLen).
      string(8bit) em =
	[string(8bit)]sprintf("%*c", [int(0..)]((n->size()+6)/8), m);

      //       Note that emLen will be one less than k if modBits - 1 is
      //       divisible by 8 and equal to k otherwise. If I2OSP outputs
      //       "integer too large," output "invalid signature" and stop.
      /* FIXME: Is this needed? */

      // 3. EMSA-PSS verification: Apply the EMSA-PSS verification operation
      //    (Section 9.1.2) to the message M and the encoded message EM to
      //    determine whether they are consistent:
      //
      //    Result = EMSA-PSS-VERIFY (M, EM, modBits - 1).
      //
      // 4. If Result = "consistent," output "valid signature." Otherwise,
      //    output "invalid signature."
      return h->emsa_pss_verify(message, em,
				[int(1..)](n->size() - 1), saltlen);
    }

    //! Signs the @[message] with a JOSE JWS RSASSA-PSS signature using hash
    //! algorithm @[h].
    //!
    //! @param message
    //!   Message to sign.
    //!
    //! @param headers
    //!   JOSE headers to use. Typically a mapping with a single element
    //!   @expr{"typ"@}.
    //!
    //! @param h
    //!   Hash algorithm to use. Currently defaults to @[SHA256].
    //!
    //! @returns
    //!   Returns the signature on success, and @expr{0@} (zero)
    //!   on failure (typically that the hash + salt combo is too
    //!   large for the RSA modulo).
    //!
    //! @seealso
    //!   @[pkcs_sign()], @[salt_size()], @rfc{7515:3.5@}
    string(7bit) jose_sign(string(8bit) message,
			   mapping(string(7bit):string(7bit)|int)|void headers,
			   .Hash|void h)
    {
      // FIXME: Consider selecting depending on key size.
      //        https://www.keylength.com/en/4/ says the
      //        minimum lengths are:
      //	Expiry	Modulus		Hash
      //	2010	1024 bits	160 bits
      //	2030	2048 bits	224 bits
      //       >2030	3073 bits	256 bits
      //      >>2030	7680 bits	384 bits
      //     >>>2030	15360 bits	512 bits
      if (!h) h = .SHA256;
      string(7bit) alg = jwa(h);
      if (!alg) return 0;
      headers = headers || ([]);
      headers += ([ "alg": alg ]);
      string(7bit) tbs =
	sprintf("%s.%s",
		MIME.encode_base64url(string_to_utf8(Standards.JSON.encode(headers))),
		MIME.encode_base64url(message));
      string(8bit) raw = pkcs_sign(tbs, h, h->digest_size());
      if (!raw) return 0;
      return sprintf("%s.%s", tbs, MIME.encode_base64url(raw));
    }

    //! Verify and decode a JOSE JWS RSASSA-PSS signed value.
    //!
    //! @param jws
    //!   A JSON Web Signature as returned by @[jose_sign()].
    //!
    //! @returns
    //!   Returns @expr{0@} (zero) on failure, and an array
    //!   @array
    //!     @elem mapping(string(7bit):string(7bit)|int) 0
    //!       The JOSE header.
    //!     @elem string(8bit) 1
    //!       The signed message.
    //!   @endarray
    //!
    //! @seealso
    //!   @[pkcs_verify()], @rfc{7515:3.5@}
    array(mapping(string(7bit):
		  string(7bit)|int)|string(8bit)) jose_decode(string(7bit) jws)
    {
      array(string(7bit)) segments = [array(string(7bit))](jws/".");
      if (sizeof(segments) != 3) return 0;
      catch {
	mapping(string(7bit):string(7bit)|int) headers =
	  [mapping(string(7bit):string(7bit)|int)](mixed)
	  Standards.JSON.decode(utf8_to_string(MIME.decode_base64url(segments[0])));
	if (!mappingp(headers)) return 0;
	object(.Hash)|zero h;
	switch(headers->alg) {
        case "PS256":
	  h = .SHA256;
	  break;
#if constant(Nettle.SHA384)
	case "PS384":
	  h = .SHA384;
	  break;
#endif
#if constant(Nettle.SHA512)
	case "PS512":
	  h = .SHA512;
	  break;
#endif
	default:
	  return 0;
	}
	string(7bit) tbs = sprintf("%s.%s", segments[0], segments[1]);
	if (pkcs_verify(tbs, h, MIME.decode_base64url(segments[2]),
			h->digest_size())) {
	  return ({ headers, MIME.decode_base64url(segments[1]) });
	}
      };
      return 0;
    }
  }
}

//! PKCS#1 1.5 encryption (@rfc{3447:7.2@}) and signatures (@rfc{3447:8.2@}).
//!
//! @seealso
//!    @[PSSState]
class PKCS1_5State
{
  inherit PSSState;

  //! Get the PKCS#1 1.5 state.
  this_program `PKCS1_5() { return this_program::this; }

  //! Calls @[Standards.PKCS.RSA.signature_algorithm_id] with the
  //! provided @[hash].
  Sequence pkcs_signature_algorithm_id(.Hash hash)
  {
    return PKCS_RSA->signature_algorithm_id(hash);
  }

  //! Calls @[Standards.PKCS.RSA.build_public_key] with this object as
  //! argument.
  Sequence pkcs_public_key()
  {
    return PKCS_RSA->build_public_key(this);
  }

#undef Sequence

  //! Returns the string @expr{"RSA"@}.
  string(8bit) name() { return "RSA"; }

  //! Get the JWS algorithm identifier for a hash.
  //!
  //! @returns
  //!   Returns @expr{0@} (zero) on failure.
  //!
  //! @seealso
  //!   @rfc{7518:3.1@}
  string(7bit) jwa(.Hash hash)
  {
    switch(hash->name()) {
    case "sha256":
      return "RS256";
    case "sha384":
      return "RS384";
    case "sha512":
      return "RS512";
    }
    return 0;
  }

  //! Signs the @[message] with a PKCS-1 signature using hash
  //! algorithm @[h]. This is equivalent to
  //! I2OSP(RSASP1(OS2IP(RSAES-PKCS1-V1_5-ENCODE(message)))) in PKCS#1
  //! v2.2.
  string(8bit) pkcs_sign(string(8bit) message, .Hash h)
  {
    string(8bit) di =
      [string]Standards.PKCS.Signature.build_digestinfo(message, h);
    return [string(8bit)]sprintf("%*c", n->size(256), raw_sign(di));
  }

  //! Verify PKCS-1 signature @[sign] of message @[message] using hash
  //! algorithm @[h].
  int(0..1) pkcs_verify(string(8bit) message, .Hash h, string(8bit) sign)
  {
    if( sizeof(sign)!=n->size(256) ) return 0;
    string(8bit) s =
      [string]Standards.PKCS.Signature.build_digestinfo(message, h);
    return raw_verify(s, Gmp.mpz(sign, 256));
  }

  //! Signs the @[message] with a JOSE JWS RSASSA-PKCS-v1.5 signature using hash
  //! algorithm @[h].
  //!
  //! @param message
  //!   Message to sign.
  //!
  //! @param headers
  //!   JOSE headers to use. Typically a mapping with a single element
  //!   @expr{"typ"@}.
  //!
  //! @param h
  //!   Hash algorithm to use. Currently defaults to @[SHA256].
  //!
  //! @returns
  //!   Returns the signature on success, and @expr{0@} (zero)
  //!   on failure (typically that the hash + salt combo is too
  //!   large for the RSA modulo).
  //!
  //! @seealso
  //!   @[pkcs_verify()], @[salt_size()], @rfc{7515@}
  string(7bit) jose_sign(string(8bit) message,
			 mapping(string(7bit):string(7bit)|int)|void headers,
			 .Hash|void h)
  {
    // NB: Identical to the code in PSSState, but duplication
    //     is necessary to bind to the correct variants of
    //     jwa() and pkcs_sign().
    // FIXME: Consider selecting depending on key size.
    //        https://www.keylength.com/en/4/ says the
    //        minimum lengths are:
    //		Expiry	Modulus		Hash
    //		2010	1024 bits	160 bits
    //		2030	2048 bits	224 bits
    //	       >2030	3073 bits	256 bits
    //	      >>2030	7680 bits	384 bits
    //	     >>>2030	15360 bits	512 bits
    h = h || .SHA256;
    string(7bit) alg = jwa(h);
    if (!alg) return 0;
    headers = headers || ([]);
    headers += ([ "alg": alg ]);
    string(7bit) tbs =
      sprintf("%s.%s",
	      MIME.encode_base64url(string_to_utf8(Standards.JSON.encode(headers))),
	      MIME.encode_base64url(message));
    string(8bit) raw = pkcs_sign(tbs, h);
    if (!raw) return 0;
    return sprintf("%s.%s", tbs, MIME.encode_base64url(raw));
  }

  //! Verify and decode a JOSE JWS RSASSA-PKCS-v1.5 signed value.
  //!
  //! @param jws
  //!   A JSON Web Signature as returned by @[jose_sign()].
  //!
  //! @returns
  //!   Returns @expr{0@} (zero) on failure, and an array
  //!   @array
  //!     @elem mapping(string(7bit):string(7bit)|int) 0
  //!       The JOSE header.
  //!     @elem string(8bit) 1
  //!       The signed message.
  //!   @endarray
  //!   on success.
  //!
  //! @seealso
  //!   @[pkcs_verify()], @rfc{7515:3.5@}
  array(mapping(string(7bit):
		string(7bit)|int)|string(8bit)) jose_decode(string(7bit) jws)
  {
    // NB: Not quite identical to the code in PSSState, but almost
    //     as it is necessary to bind to the correct variant of
    //     pkcs_sign(), and compare with the correct alg values.
    array(string(7bit)) segments = [array(string(7bit))](jws/".");
    if (sizeof(segments) != 3) return 0;
    catch {
      mapping(string(7bit):string(7bit)|int) headers =
	[mapping(string(7bit):string(7bit)|int)](mixed)
	Standards.JSON.decode(utf8_to_string(MIME.decode_base64url(segments[0])));
      if (!mappingp(headers)) return 0;
      object(.Hash)|zero h;
      switch(headers->alg) {
      case "RS256":
	h = .SHA256;
	break;
#if constant(Nettle.SHA384)
      case "RS384":
	h = .SHA384;
	break;
#endif
#if constant(Nettle.SHA512)
      case "RS512":
	h = .SHA512;
	break;
#endif
      default:
	return 0;
      }
      string(7bit) tbs = sprintf("%s.%s", segments[0], segments[1]);
      if (pkcs_verify(tbs, h, MIME.decode_base64url(segments[2]))) {
	return ({ headers, MIME.decode_base64url(segments[1]) });
      }
    };
    return 0;
  }

  //
  // --- Encryption/decryption
  //

  //! Pads the message @[s] with @[rsa_pad] type 2, signs it and returns
  //! the signature as a byte string.
  //! @param r
  //!   Optional random function to be passed down to @[rsa_pad].
  string(8bit) encrypt(string(8bit) s, function(int:string(8bit))|void r)
  {
    return rsa_pad(s, 2, r)->powm(e, n)->digits(256);
  }

  //! Decrypt a message encrypted with @[encrypt].
  string(8bit) decrypt(string(8bit) s)
  {
    return rsa_unpad(Gmp.smpz(s, 256)->powm(d, n), 2);
  }

  //
  // --- Block cipher compatibility.
  //

  //! Encrypt or decrypt depending on set mode.
  //! @seealso
  //!   @[set_encrypt_key], @[set_decrypt_key]
  string(8bit) crypt(string(8bit) s)
  {
    return (encrypt_mode ? encrypt(s) : decrypt(s));
  }

  //! Returns the crypto block size, in bytes, or zero if not yet set.
  int block_size()
  {
    // FIXME: This can be both zero and negative...
    return n->size(256) - 3;
  }

  //
  //  --- Below are methods for RSA applied in other standards.
  //


  //! Pads the @[message] to the current block size with method
  //! @[type] and returns the result as an integer. This is equivalent
  //! to OS2IP(RSAES-PKCS1-V1_5-ENCODE(message)) in PKCS#1 v2.2.
  //! @param type
  //!   @int
  //!     @value 1
  //!       The message is padded with @expr{0xff@} bytes.
  //!     @value 2
  //!       The message is padded with random data, using the @[random]
  //!       function if provided. Otherwise the default random function
  //!       set in the object will be used.
  //!   @endint
  Gmp.mpz rsa_pad(string(8bit) message, int(1..2) type,
		  function(int(0..):string(8bit))|void random)
  {
    string(8bit) padding = "";

    // Padding length: RSA size - message size - 3 bytes; delimiter,
    // padding type and leading null (not explicitly coded, as Gmp.mpz
    // does the right thing anyway). Require at least 8 bytes of padding
    // as security margin.
    int len = n->size(256) - 3 - sizeof(message);
    if (len < 8)
      error( "Block too large. (%d>%d)\n", sizeof(message), n->size(256)-11 );

    switch(type)
    {
    case 1:
      padding = sprintf("%@c", allocate(len, 0xff));
      break;
    case 2:
      if( !random ) random = this::random;
      do {
	padding += random([int(0..)](len-sizeof(padding))) - "\0";
      }  while( sizeof(padding)<len );
      break;
    default:
      error( "Unknown type.\n" );
    }
    return Gmp.smpz(sprintf("%c", type) + padding + "\0" + message, 256);
  }

  //! Reverse the effect of @[rsa_pad].
  string(8bit) rsa_unpad(Gmp.mpz block, int type)
  {
    string(8bit) s = block->digits(256);

    // Content independent size information. Not timing sensitive.
    if( sizeof(s)!=(n->size(256)-1) ) return 0;

    int i = Nettle.rsa_unpad(s, [int(1..2)]type);
    if( !i ) return 0;

    return s[i..];
  }

  //! Pads the @[digest] with @[rsa_pad] type 1 and signs it. This is
  //! equivalent to RSASP1(OS2IP(RSAES-PKCS1-V1_5-ENCODE(message))) in
  //! PKCS#1 v2.2.
  Gmp.mpz raw_sign(string(8bit) digest)
  {
    return rsa_pad(digest, 1, 0)->powm(d, n);
  }

  //! Verifies the @[digest] against the signature @[s], assuming pad
  //! type 1.
  //! @seealso
  //!   @[rsa_pad], @[raw_sign]
  int(0..1) raw_verify(string(8bit) digest, Gmp.mpz s)
  {
    return Gmp.smpz(s)->powm(e, n) == rsa_pad(digest, 1, 0);
  }
}

//!
class State
{
  inherit PKCS1_5State;
}

//! Calling `() will return a @[State] object with the given @[params].
//!
//! @seealso
//!   @[State()]
protected State `()(mapping(string(8bit):Gmp.mpz|int|string(7bit))|void params)
{
  return State(params);
}
