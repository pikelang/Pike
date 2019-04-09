#pike __REAL_VERSION__
#pragma strict_types

//! Base class for hash algorithms.
//!
//! Implements common meta functions, such as key expansion
//! algoritms and convenience functions.
//!
//! Note that no actual hash algorithm is implemented
//! in the base class. They are implemented in classes
//! that inherit this class.

inherit .__Hash;

//! Calling `() will return a @[State] object.
protected State `()() { return State(); }

//!  Works as a (possibly faster) shortcut for e.g.
//!  @expr{State(data)->digest()@}, where @[State] is the hash state
//!  class corresponding to this @[Hash].
//!
//! @param data
//!   String to hash.
//!
//! @seealso
//!   @[Stdio.File], @[State()->update()] and @[State()->digest()].
string(8bit) hash(string(8bit) data)
{
  return State(data)->digest();
}

//!  Works as a (possibly faster) shortcut for e.g. @expr{State(
//!  obj->read() )->digest()@}, where @[State] is the hash state class
//!  corresponding to this @[Hash].
//!
//! @param source
//!   Object to read some data to hash from.
//!
//! @param bytes
//!   The number of bytes of the @[source] object that should be
//!   hashed. Zero and negative numbers are ignored and the whole file
//!   is hashed.
//!
//! @[Stdio.File], @[Stdio.Buffer], @[String.Buffer], @[System.Memory]
variant string(8bit) hash(Stdio.File|Stdio.Buffer|String.Buffer|System.Memory source,
                    int|void bytes)
{
  function(int|void:string(8bit)) f;

  if (source->read)
  {
    // Stdio.File, Stdio.Buffer
    f = [function(int|void:string(8bit))]source->read;
  }
  else if (source->get)
  {
    // String.Buffer
    f = [function(int|void:string(8bit))]source->get;
  }
  else if (source->pread)
  {
    // System.Memory
    f = lambda(int|void b)
        {
          System.Memory m = [object(System.Memory)]source;
          return m->pread(0, b || sizeof(source));
        };
  }

  if (f)
  {
    if (bytes>0)
      return hash( f(bytes) );
    else
      return hash( f() );
  }
  error("Incompatible object\n");
}

//! JWS algorithm id (if any) for the HMAC sub-module.
//! Overloaded by the actual implementations.
protected constant hmac_jwa_id = "";

//! @module HMAC
//!
//! HMAC (Hashing for Message Authenticity Control) for the hash
//! algorithm. Typically used as
//! e.g. @expr{Crypto.SHA256.HMAC(key)(data)@} or
//! @expr{Crypto.SHA256.HMAC(key)->update(data)->update(more_data)->digest()@}.
//!
//! @rfc{2104@}.
//!
//! @seealso
//!   @[Crypto.HMAC]

//! @ignore
protected class _HMAC
{
//! @endignore

  inherit .MAC;

  //! JWS algorithm identifier (if any, otherwise @expr{0@}).
  //!
  //! @seealso
  //!   @rfc{7518:3.1@}
  string(7bit) jwa()
  {
    return (hmac_jwa_id != "") && [string(7bit)]hmac_jwa_id;
  }

  int(0..) digest_size()
  {
    return global::digest_size();
  }

  int(1..) block_size()
  {
    return global::block_size();
  }

  //! Returns the block size of the encapsulated hash.
  //!
  //! @note
  //!   Other key sizes are allowed, and will be expanded/compressed
  //!   to this size.
  int(0..) key_size()
  {
    return global::block_size();
  }

  //! HMAC has no modifiable iv.
  int(0..0) iv_size()
  {
    return 0;
  }

  //! The HMAC hash state.
  class State
  {
    inherit ::this_program;

    protected string(8bit) ikey; /* ipad XOR:ed with the key */
    protected string(8bit) okey; /* opad XOR:ed with the key */

    protected global::State h;

    //! @param passwd
    //!   The secret password (K).
    //!
    //! @param b
    //!   Block size. Must be larger than or equal to the @[digest_size()].
    //!   Defaults to the @[block_size()].
    protected void create (string(8bit) passwd, void|int b)
    {
      if (!b)
	b = block_size();
      else if (digest_size()>b)
	error("Block size is less than hash digest size.\n");
      if (sizeof(passwd) > b)
	passwd = hash(passwd);
      if (sizeof(passwd) < b)
	passwd = passwd + "\0" * (b - sizeof(passwd));

      ikey = [string(8bit)](passwd ^ ("6" * b));
      okey = [string(8bit)](passwd ^ ("\\" * b));
    }

    string(8bit) name()
    {
      return [string(8bit)]sprintf("HMAC(%s)", global::name());
    }

    //! HMAC does not have a modifiable iv.
    this_program set_iv(string(8bit) iv)
    {
      if (sizeof(iv)) error("Not supported for HMAC.\n");
    }

    //! Hashes the @[text] according to the HMAC algorithm and returns
    //! the hash value.
    //!
    //! This works as a combined @[update()] and @[digest()].
    protected string(8bit) `()(string(8bit) text)
    {
      return hash(okey + hash(ikey + text));
    }

    this_program update(string(8bit) data)
    {
      if( !h )
      {
	h = global::State();
	h->update(ikey);
      }
      h->update(data);
      return this;
    }

    this_program init(string(8bit)|void data)
    {
      h = 0;
      if (data) update(data);
      return this;
    }

    string digest(int(0..)|void length)
    {
      string res = hash(okey + h->digest());
      h = 0;

      if (length) return res[..length-1];
      return res;
    }

    int(0..) digest_size()
    {
      return global::digest_size();
    }

    int(1..) block_size()
    {
      return global::block_size();
    }

    //! Hashes the @[text] according to the HMAC algorithm and returns
    //! the hash value as a PKCS-1 digestinfo block.
    string(8bit) digest_info(string(8bit) text)
    {
      return pkcs_digest(okey + hash(ikey + text));
    }

    //! Generate a JWK-style mapping of the object.
    //!
    //! @param private_key
    //!   Ignored.
    //!
    //! @returns
    //!   Returns a JWK-style mapping on success, and @expr{0@} (zero)
    //!   on failure.
    //!
    //! @seealso
    //!   @[create()], @[Web.encode_jwk()], @rfc{7517:4@}, @rfc{7518:6.4@}
    mapping(string(7bit):string(7bit)) jwk(int(0..1)|void private_key)
    {
      if (!jwa()) return 0;	// Not supported for this hash.
      mapping(string(7bit):string(7bit)) jwk = ([
	"kty":"oct",
	"alg":jwa(),
	"k": MIME.encode_base64url(ikey ^ ("6" * block_size())),
      ]);
      return jwk;
    }
  }

  //! Returns a new @[State] object initialized with a @[password],
  //! and optionally block size @[b]. Block size defaults to the hash
  //! function block size.
  protected State `()(string(8bit) password, void|int b)
  {
    if (!b || (b == block_size())) {
      return State(password);
    }
    // Unusual block size.
    // NB: Nettle's implementation of HMAC doesn't support
    //     non-standard block sizes, so always use the
    //     generic implementation in this case.
    return local::State(password, b);
  }

//! @ignore
}

_HMAC HMAC = _HMAC();

//! @endignore

//! @endmodule HMAC

/* NOTE: This is NOT the MIME base64 table! */
private constant b64tab =
  "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

private void b64enc(String.Buffer dest, int a, int b, int c, int sz)
{
  int bitbuf = a | (b << 8) | (c << 16);
  while (sz--) {
    dest->putchar( b64tab[bitbuf & 63] );
    bitbuf >>= 6;
  }
}

//!   Password hashing function in @[crypt_md5()]-style.
//!
//!   Implements the algorithm described in
//!   @url{http://www.akkadia.org/drepper/SHA-crypt.txt@}.
//!
//!   This is the algorithm used by @tt{crypt(2)@} in
//!   methods @tt{$5$@} (SHA256) and @tt{$6$@} (SHA512).
//!
//! @seealso
//!   @[crypt_md5()]
string(8bit) crypt_hash(string(8bit) password, string(8bit) salt, int rounds)
{
  int dsz = digest_size();
  int plen = sizeof(password);

  if (!rounds) rounds = 5000;
  if (rounds < 1000) rounds = 1000;
  if (rounds > 999999999) rounds = 999999999;

  // FIXME: Send the first param directly to create()?
  State hash_obj = State();

  function(string(8bit):State) update = hash_obj->update;
  function(:string(8bit)) digest = hash_obj->digest;

  salt = salt[..15];

  /* NB: Comments refer to http://www.akkadia.org/drepper/SHA-crypt.txt */
  string(8bit) b = update(password + salt + password)->digest();/* 5-8 */
  update(password);						/* 2 */
  update(salt);							/* 3 */

  void crypt_add(string(8bit) in, int len)
  {
    int i;
    for (; i+dsz<len; i += dsz)
      update(in);
    update(in[..len-i-1]);
  };

  crypt_add(b, plen);						/* 9-10 */

  for (int i = 1; i < plen; i <<= 1) {				/* 11 */
    if (plen & i)
      update(b);
    else
      update(password);
  }

  string(8bit) a = digest();					/* 12 */

  for (int i = 0; i < plen; i++)				/* 14 */
    update(password);

  string(8bit) dp = digest();					/* 15 */

  if (dsz != plen) {
    dp *= 1 + (plen-1)/dsz;					/* 16 */
    dp = dp[..plen-1];
  }

  for(int i = 0; i < 16 + (a[0] & 0xff); i++)			/* 18 */
    update(salt);

  string(8bit) ds = digest();					/* 19 */

  if (dsz != sizeof(salt)) {
    ds *= 1 + (sizeof(salt)-1)/dsz;				/* 20 */
    ds = ds[..sizeof(salt)-1];
  }

  for (int r = 0; r < rounds; r++) {				/* 21 */
    if (r & 1)
      crypt_add(dp, plen);					/* b */
    else							/* c */
      update(a);

    if (r % 3)							/* d */
      crypt_add(ds, sizeof(salt));

    if (r % 7)							/* e */
      crypt_add(dp, plen);

    if (r & 1)							/* f */
      update(a);
    else							/* g */
      crypt_add(dp, plen);

    a = digest();						/* h */
  }

  /* And now time for some pointless shuffling of the result.
   * Note that the shuffling is slightly different between
   * the two cases.
   *
   * Instead of having fixed tables for the shuffling, we
   * generate the table incrementally. Note that the
   * specification document doesn't say how the shuffling
   * should be done when the digest size % 3 is zero
   * (or actually for that matter when the digest size
   * is other than 32 or 64). We assume that the shuffler
   * index rotation is based on the modulo, and that zero
   * implies no rotation.
   *
   * This is followed by a custom base64-style encoding.
   */

  /* We do some table magic here to avoid modulo operations
   * on the table index.
   */
  array(array(int)) shuffler = allocate(5, allocate)(2);
  shuffler[3] = shuffler[0];
  shuffler[4] = shuffler[1];

  int sublength = sizeof(a)/3;
  shuffler[0][0] = 0;
  shuffler[1][0] = sublength;
  shuffler[2][0] = sublength*2;

  int shift = sizeof(a) % 3;

  int t;
  String.Buffer ret = String.Buffer();
  for (int i = 0; i + 3 < dsz; i+=3)
  {
    b64enc(ret, a[shuffler[2][t]], a[shuffler[1][t]], a[shuffler[0][t]], 4);
    shuffler[0][!t] = shuffler[shift][t]+1;
    shuffler[1][!t] = shuffler[shift+1][t]+1;
    shuffler[2][!t] = shuffler[shift+2][t]+1;
    t=!t;
  }

  a+="\0\0";
  t = dsz/3*3;
  b64enc(ret, a[t], a[t+1], a[t+2], dsz%3+1);

  return (string(8bit))ret;
}

//! Password Based Key Derivation Function #1 from @rfc{2898@}. This
//! method is compatible with the one from PKCS#5 v1.5.
//!
//! @param password
//! @param salt
//!   Password and salt for the keygenerator.
//!
//! @param rounds
//!   The number of iterations to rehash the input.
//!
//! @param bytes
//!   The number of bytes of output. Note that this has an upper limit
//!   of the size of a single digest.
//!
//! @returns
//!   Returns the derived key.
//!
//! @note
//!   @rfc{2898@} does not recommend this function for anything else
//!   than compatibility with existing applications, due to the limits
//!   in the length of the generated keys.
//!
//! @seealso
//!   @[hkdf()], @[pbkdf2()], @[openssl_pbkdf()], @[crypt_password()]
string(8bit) pbkdf1(string(8bit) password, string(8bit) salt, int rounds, int bytes)
{
  if( bytes>digest_size() )
    error("Requested bytes %d exceeds hash digest size %d.\n",
          bytes, digest_size());
  if( rounds <=0 )
    error("Rounds needs to be 1 or higher.\n");

  string(8bit) res = password + salt;

  password = "CENSORED";

  while (rounds--) {
    res = hash(res);
  }

  return res[..bytes-1];
}

//! Password Based Key Derivation Function #2 from @rfc{2898@}, PKCS#5
//! v2.0.
//!
//! @param password
//! @param salt
//!   Password and salt for the keygenerator.
//!
//! @param rounds
//!   The number of iterations to rehash the input.
//!
//! @param bytes
//!   The number of bytes of output.
//!
//! @returns
//!   Returns the derived key.
//!
//! @seealso
//!   @[hkdf()], @[pbkdf1()], @[openssl_pbkdf()], @[crypt_password()]
string(8bit) pbkdf2(string(8bit) password, string(8bit) salt,
		    int rounds, int bytes)
{
  if( rounds <=0 )
    error("Rounds needs to be 1 or higher.\n");

  object(_HMAC.State) hmac = HMAC(password);
  password = "CENSORED";

  string(8bit) res = "";
  int dsz = digest_size();
  int fragno;
  while (sizeof(res) < bytes) {
    string(8bit) frag = "\0" * dsz;
    string(8bit) buf = salt + sprintf("%4c", ++fragno);
    for (int j = 0; j < rounds; j++) {
      buf = hmac(buf);
      frag ^= buf;
    }
    res += frag;
  }

  return res[..bytes-1];
}

//! HMAC-based Extract-and-Expand Key Derivation Function, HKDF,
//! @rfc{5869@}. This is very similar to @[pbkdf2], with a few
//! important differences. HKDF can use an "info" string that binds a
//! generated password to a specific use or application (e.g. port
//! number or cipher suite). It does not however support multiple
//! rounds of hashing to add computational cost to brute force
//! attacks.
class HKDF
{
  protected string(8bit) prk;
  protected object(_HMAC.State) hmac;

  //! Initializes the HKDF object with a RFC 5869 2.2
  //! HKDF-Extract(salt, IKM) call.
  protected void create(string(8bit) password, void|string(8bit) salt)
  {
    if(!salt) salt = "\0"*digest_size();
    prk = salt;
    extract(password);
  }

  //! This is similar to the RFC 5869 2.2 HKDF-Extract(salt, IKM)
  //! function, but the salt is the previously generated PRK.
  string(8bit) extract(string(8bit) password)
  {
    prk = HMAC(prk)(password);
    hmac = HMAC(prk);
  }

  //! This is similar to the RFC 5869 2.3 HKDF-Expand(PRK, info, L)
  //! function, but PRK is taken from the object.
  string(8bit) expand(string(8bit) info, int bytes)
  {
    string(8bit) t = "";
    string(8bit) res = "";
    if(!info) info = "";
    int(8bit) i;
    while (sizeof(res) < bytes )
    {
      i++;
      t = hmac(sprintf("%s%s%c", t, info, i));
      res += t;
    }

    return res[..bytes-1];
  }
}

//! Password Based Key Derivation Function from OpenSSL.
//!
//! This when used with @[Crypto.MD5] and a single round
//! is the function used to derive the key to encrypt
//! @[Standards.PEM] body data.
//!
//! @fixme
//!   Derived from OpenSSL. Is there any proper specification?
//!
//!   It seems to be related to PBKDF1 from @rfc{2898@}.
//!
//! @seealso
//!   @[pbkdf1()], @[pbkdf2()], @[crypt_password()]
string(8bit) openssl_pbkdf(string(8bit) password, string(8bit) salt,
			   int rounds, int bytes)
{
  string(8bit) out = "";
  string(8bit) h = "";
  string(8bit) seed = password + salt;

  password = "CENSORED";

  for (int j = 1; j < rounds; j++) {
    h = hash(h + seed);
  }

  while (sizeof(out) < bytes) {
    h = hash(h + seed);
    out += h;
  }
  return out[..bytes-1];
}

//! Make a PKCS-1 digest info block with the message @[s].
//!
//! @seealso
//!   @[Standards.PKCS.build_digestinfo()]
string(8bit) pkcs_digest(object|string(8bit) s)
{
  return [string(8bit)]
    Pike.Lazy.Standards.PKCS.Signature.build_digestinfo(s, this);
}

//! This is the Password-Based Key Derivation Function used in TLS.
//!
//! @param password
//!   The prf secret.
//!
//! @param salt
//!   The prf seed.
//!
//! @param rounds
//!   Ignored.
//!
//! @param bytes
//!   The number of bytes to generate.
string(8bit) P_hash(string(8bit) password, string(8bit) salt,
		    int rounds, int bytes)
{
  _HMAC.State hmac = HMAC(password);
  string(8bit) temp = salt;
  string(8bit) res="";

  while (sizeof(res) < bytes) {
    temp = hmac(temp);
    res += hmac(temp + salt);
  }
  return res[..(bytes-1)];
}

//! This is the mask generation function @tt{MFG1@} from
//! @rfc{3447:B.2.1@}.
//!
//! @param seed
//!   Seed from which the mask is to be generated.
//!
//! @param bytes
//!   Length of output.
//!
//! @returns
//!   Returns a pseudo-random string of length @[bytes].
//!
//! @note
//!   This function is compatible with the mask generation functions
//!   defined in PKCS #1, IEEE 1363-2000 and ANSI X9.44.
string(8bit) mgf1(string(8bit) seed, int(0..) bytes)
{
  if ((bytes>>32) >= digest_size()) {
    error("Mask too long.\n");
  }
  Stdio.Buffer t = Stdio.Buffer();
  for (int counter = 0; sizeof(t) < bytes; counter++) {
    t->add(hash(sprintf("%s%4c", seed, counter)));
  }
  return t->read(bytes);
}

//! This is the encoding algorithm used in RSAES-OAEP
//! (@rfc{3447:7.1.1@}).
//!
//! @param message
//!   Message to encode.
//!
//! @param bytes
//!   Number of bytes of destination encoding.
//!
//! @param seed
//!   A string of random bytes at least @[digest_size()] long.
//!
//! @param label
//!   An optional encoding label. Defaults to @expr{""@}.
//!
//! @param mgf
//!   The mask generation function to use. Defaults to @[mgf1()].
//!
//! @returns
//!   Returns the encoded string on success and @expr{0@} (zero)
//!   on failure (typically too few bytes to represent the result).
//!
//! @seealso
//!   @[eme_oaep_decode()]
string(8bit) eme_oaep_encode(string(8bit) message,
			     int(1..) bytes,
			     string(8bit) seed,
			     string(8bit)|void label,
			     function(string(8bit), int(0..):
				      string(8bit))|void mgf)
{
  int(0..) hlen = digest_size();

  if ((bytes < (2 + 2*hlen + sizeof(message))) ||
      (sizeof(seed) < hlen)) {
    return 0;
  }
  if (!mgf) mgf = mgf1;

  // EME-OAEP encoding (see Figure 1 below):
  // a. If the label L is not provided, let L be the empty string. Let
  //    lHash = Hash(L), an octet string of length hLen (see the note below).
  if (!label) label = "";
  string(8bit) lhash = hash(label);

  // b. Generate an octet string PS consisting of k - mLen - 2hLen - 2
  //    zero octets. The length of PS may be zero.
  string(8bit) ps = "\0" * (bytes - (sizeof(message) + 2*hlen + 2));

  // c. Concatenate lHash, PS, a single octet with hexadecimal value
  //    0x01, and the message M to form a data block DB of length
  //    k - hLen - 1 octets as
  //
  //    DB = lHash || PS || 0x01 || M.
  string(8bit) db = lhash + ps + "\1" + message;

  // d. Generate a random octet string seed of length hLen.
  /* Supplied by caller. */
  if (sizeof(seed) > hlen) seed = seed[..hlen-1];

  // e. Let dbMask = MGF(seed, k - hLen - 1).
  string(8bit) dbmask = mgf(seed, [int(0..)](bytes - (hlen + 1)));

  // f. Let maskedDB = DB \xor dbMask.
  string(8bit) maskeddb = [string(8bit)](db ^ dbmask);

  // g. Let seedMask = MGF(maskedDB, hLen).
  string(8bit) seedmask = mgf(maskeddb, hlen);

  // h. Let maskedSeed = seed \xor seedMask.
  string(8bit) maskedseed = [string(8bit)](seed ^ seedmask);

  // i. Concatenate a single octet with hexadecimal value 0x00,
  //    maskedSeed, and maskedDB to form an encoded message EM
  //    of length k octets as
  //
  //    EM = 0x00 || maskedSeed || maskedDB.
  return "\0" + maskedseed + maskeddb;
}

//! Decode an EME-OAEP encoded string.
//!
//! @param message
//!   Message to decode.
//!
//! @param label
//!   Label that was used when the message was encoded.
//!   Defaults to @expr{""@}.
//!
//! @param mgf
//!   Mask generation function to use. Defaults to @[mgf1()].
//!
//! @returns
//!   Returns the decoded message on success, and @expr{0@} (zero)
//!   on failure.
//!
//! @note
//!   The decoder attempts to take a constant amount of time on failure.
//!
//! @seealso
//!   @[eme_oaep_encode()], @rfc{3447:7.1.2@}
string(8bit) eme_oaep_decode(string(8bit) message,
			     string(8bit)|void label,
			     function(string(8bit), int(0..):
				      string(8bit))|void mgf)
{
  int(0..) hlen = digest_size();

  if (sizeof(message) < (2*hlen + 2)) {
    return 0;
  }
  if (!mgf) mgf = mgf1;

  // EME-OAEP decoding:
  // a. If the label L is not provided, let L be the empty string. Let
  //    lHash = Hash(L), an octet string of length hLen (see the note
  //    in Section 7.1.1).
  if (!label) label = "";
  string(8bit) lhash = hash(label);

  // b. Separate the encoded message EM into a single octet Y, an octet
  //    string maskedSeed of length hLen, and an octet string maskedDB
  //    of length k - hLen - 1 as
  //
  //    EM = Y || maskedSeed || maskedDB.
  string(8bit) maskedseed = message[1..hlen];
  string(8bit) maskeddb = message[hlen+1..];

  // c. Let seedMask = MGF(maskedDB, hLen).
  string(8bit) seedmask = mgf(maskeddb, hlen);

  // d. Let seed = maskedSeed \xor seedMask.
  string(8bit) seed = [string(8bit)](maskedseed ^ seedmask);

  // e. Let dbMask = MGF(seed, k - hLen - 1).
  string(8bit) dbmask = mgf(seed, sizeof(maskeddb));

  // f. Let DB = maskedDB \xor dbMask.
  string(8bit) db = [string(8bit)](maskeddb ^ dbmask);

  // g. Separate DB into an octet string lHash' of length hLen, a
  //    (possibly empty) padding string PS consisting of octets with
  //    hexadecimal value 0x00, and a message M as
  //
  //    DB = lHash' || PS || 0x01 || M.
  //
  //    If there is no octet with hexadecimal value 0x01 to separate PS
  //    from M, if lHash does not equal lHash', or if Y is nonzero, output
  //    "decryption error" and stop. (See the note below.)
  int problem = message[0];
  int found = 0;
  for(int i = 0; i < sizeof(db); i++) {
    if (i < hlen) {
      problem |= db[i] ^ lhash[i];
    } else {
      problem |= (db[i] & 0xfe) & ~found;
      found |= -(db[i] & 1);
    }
  }
  problem |= ~found;
  if (problem) return 0;
  for (int i = hlen; i < sizeof(db); i++) {
    if (db[i] == 1) return db[i+1..];
  }
  // NOT REACHED.
  return 0;	// Paranoia.
}

//! This is the signature digest algorithm used in RSASSA-PSS
//! (@rfc{3447:9.1.1@}).
//!
//! @param message
//!   Message to sign.
//!
//! @param bits
//!   Number of bits in result.
//!
//! @param salt
//!   Random string to salt the signature.
//!   Defaults to the empty string.
//!
//! @param mgf
//!   Mask generation function to use.
//!   Defaults to @[mgf1()].
//!
//! @returns
//!   Returns the signature digest on success and @expr{0@} (zero)
//!   on failure (typically too few bits to represent the result).
//!
//! @seealso
//!   @[emsa_pss_verify()], @[mgf1()].
string(8bit) emsa_pss_encode(string(8bit) message, int(1..) bits,
			     string(8bit)|void salt,
			     function(string(8bit), int(0..):
				      string(8bit))|void mgf)
{
  if (!mgf) mgf = mgf1;
  if (!salt) salt = "";

  // 1. If the length of M is greater than the input limitation for the
  //    hash function (2^61 - 1 octets for SHA-1), output "message too
  //    long" and stop.
  /* N/A */

  int emlen = (bits+7)/8;

  // 3. If emLen < hLen + sLen + 2, output "encoding error" and stop.
  if (emlen < sizeof(salt) + digest_size() + 2) return 0;

  // 2. Let mHash = Hash(M), an octet string of length hLen.
  string(8bit) mhash = hash(message);

  // 4. Generate a random octet string salt of length sLen; if sLen = 0,
  //    then salt is the empty string.
  /* N/A - Passed as argument. */

  // 5. Let
  //
  //    M' = (0x)00 00 00 00 00 00 00 00 || mHash || salt;
  //    M' is an octet string of length 8 + hLen + sLen with eight initial
  //    zero octets.
  string(8bit) m = "\0\0\0\0\0\0\0\0" + mhash + salt;

  // 6. Let H = Hash(M'), an octet string of length hLen.
  string(8bit) h = hash(m);

  // 7. Generate an octet string PS consisting of emLen - sLen - hLen - 2
  //    zero octets. The length of PS may be 0.
  string(8bit) ps = "\0" * (emlen - (sizeof(salt) + sizeof(h) + 2));

  // 8. Let DB = PS || 0x01 || salt; DB is an octet string of length
  //    emLen - hLen - 1.
  string(8bit) db = ps + "\1" + salt;

  // 9. Let dbMask = MGF(H, emLen - hLen - 1).
  string(8bit) dbmask = mgf(h, [int(1..)](emlen - (sizeof(h) + 1)));

  // 10. Let maskedDB = DB \xor dbMask.
  string(8bit) maskeddb = [string(8bit)](db ^ dbmask);

  // 11. Set the leftmost 8emLen - emBits bits of the leftmost octet in
  //     maskedDB to zero.
  if (bits & 0x07) {
    int(0..255) mask = [int(0..255)]((1 << (bits & 0x07)) - 1);
    maskeddb = sprintf("%c%s", maskeddb[0] & mask, maskeddb[1..]);
  }

  // 12. Let EM = maskedDB || H || 0xbc.
  // 13. Output EM.
  return maskeddb + h + "\xbc";
}

//! This is the algorithm used to verify in RSASSA-PSS (@rfc{3447:9.1.2@}).
//!
//! @param message
//!   Message that was signed.
//!
//! @param sign
//!   Signature digest to verify.
//!
//! @param bits
//!   Number of significant bits in @[sign].
//!
//! @param saltlen
//!   Length of the salt used.
//!
//! @param mgf
//!   Mask generation function to use.
//!   Defaults to @[mgf1()].
//!
//! @returns
//!   Returns @expr{1@} on success and @expr{0@} (zero) on failure.
//!
//! @seealso
//!   @[emsa_pss_verify()], @[mgf1()].
int(0..1) emsa_pss_verify(string(8bit) message, string(8bit) sign,
			  int(1..) bits, int(0..)|void saltlen,
			  function(string(8bit), int(0..):
				   string(8bit))|void mgf)
{
  if (!mgf) mgf = mgf1;

  // 1. If the length of M is greater than the input limitation for
  //    the hash function (2^61 - 1 octets for SHA-1), output
  //    "inconsistent" and stop.
  /* N/A */

  // 3. If emLen < hLen + sLen + 2, output "inconsistent" and stop.
  if (sizeof(sign) < digest_size() + saltlen + 2) {
    return 0;
  }

  // 4. If the rightmost octet of EM does not have hexadecimal value
  //    0xbc, output "inconsistent" and stop.
  if (sign[-1] != 0xbc) {
    return 0;
  }

  // 2. Let mHash = Hash(M), an octet string of length hLen.
  string(8bit) mhash = hash(message);

  // 5. Let maskedDB be the leftmost emLen - hLen - 1 octets of EM,
  //    and let H be the next hLen octets.
  string(8bit) maskeddb = sign[..sizeof(sign) - (sizeof(mhash)+2)];
  string(8bit) h = sign[sizeof(sign) - (sizeof(mhash)+1)..sizeof(sign)-2];

  // 6. If the leftmost 8emLen - emBits bits of the leftmost octet in
  //    maskedDB are not all equal to zero, output "inconsistent" and stop.
  if (bits & 0x07) {
    int(0..255) mask = [int(0..255)]((1 << (bits & 0x07)) - 1);
    if (maskeddb[0] & ~mask) {
      return 0;
    }
  }

  // 7. Let dbMask = MGF(H, emLen - hLen - 1).
  string(8bit) dbmask = mgf(h, [int(1..)](sizeof(sign) - (sizeof(mhash)+1)));

  // 8. Let DB = maskedDB \xor dbMask.
  string(8bit) db = [string(8bit)](maskeddb ^ dbmask);

  // 9. Set the leftmost 8emLen - emBits bits of the leftmost octet
  //    in DB to zero.
  if (bits & 0x07) {
    int(0..255) mask = [int(0..255)]((1 << (bits & 0x07)) - 1);
    db = sprintf("%c%s", db[0] & mask, db[1..]);
  }

  // 10. If the emLen - hLen - sLen - 2 leftmost octets of DB are
  //     not zero or if the octet at position emLen - hLen - sLen - 1
  //     (the leftmost position is "position 1") does not have
  //     hexadecimal value 0x01, output "inconsistent" and stop.
  string(8bit) ps = db[..sizeof(sign) -(sizeof(mhash) + saltlen + 3)];
  if ((ps != "\0"*sizeof(ps)) || (db[sizeof(ps)] != 0x01)) {
    return 0;
  }

  // 11. Let salt be the last sLen octets of DB.
  string(8bit) salt = db[sizeof(db) - saltlen..];

  // 12. Let
  //
  //     M' = (0x)00 00 00 00 00 00 00 00 || mHash || salt ;
  //     M' is an octet string of length 8 + hLen + sLen with eight
  //     initial zero octets.
  string(8bit) m = "\0\0\0\0\0\0\0\0" + mhash + salt;

  // 13. Let H' = Hash(M'), an octet string of length hLen.
  // 14. If H = H', output "consistent." Otherwise, output "inconsistent."
  return h == hash(m);
}

//! HMAC-Based One-Time Password as defined by @rfc{4226@}.
//!
//! Can be used to implement the @rfc{6238@} Time-Based One-Time
//! Password Algorithm by giving the factor
//! @expr{(time()-T0)/X@}. Specifically for Google Authenticator this
//! is @expr{Crypto.SHA1.hotp(secret,time()/30)@}, using an 80 bit
//! secret.
//!
//! @param secret
//!   A shared secret between both parties. Typically the same size as
//!   the hash output.
//! @param factor
//!   A moving factor. Defined in @rfc{4226@} to be a counter
//!   synchronized between both parties.
//! @param length
//!   The maximum number of digits of the one-time password. Defaults
//!   to 6. Note that the result is usually 0-padded to this length
//!   for user display purposes.
int hotp(string(8bit) secret, int factor, void|int length)
{
  // 1
  string(8bit) hs = HMAC(secret)(sprintf("%8c",factor));

  // 2
  int offset = hs[-1] & 0xf;
  int snum;
  sscanf(hs[offset..], "%4c", snum);
  snum &= 0x7fffffff;

  // 3
  return snum % [int]pow(10, length||6);
}

// Salted password cache for SCRAM
// FIXME: Consider mark as weak?
private mapping(string:string(8bit)) SCRAM_salted_password_cache = ([]);

final string(8bit) SCRAM_get_salted_password(string key) {
  mapping(string:string(8bit)) m = SCRAM_salted_password_cache;
  return m && m[key];
}

final void SCRAM_set_salted_password(string(8bit) SaltedPassword, string key) {
  mapping(string:string(8bit)) m = SCRAM_salted_password_cache;
  if (!m || sizeof(m) > 16)
    SCRAM_salted_password_cache = m = ([]);
  m[key] = SaltedPassword;
}

//! SCRAM, defined by @rfc{5802@}.
//!
//! This implements both the client- and the serverside.
//! You normally run either the server or the client, but if you would
//! run both (use a separate client and a separate server object!),
//! the sequence would be:
//!
//! @[client_1] -> @[server_1] -> @[server_2] -> @[client_2] ->
//! @[server_3] -> @[client_3]
//!
//! @note
//! If you are a client, you must use the @ref{client_*@} methods; if you are
//! a server, you must use the @ref{server_*@} methods.
//! You cannot mix both client and server methods in a single object.
//!
//! @note
//!   This implementation does not pretend to support the full protocol.
//!   Most notably optional extension arguments are not supported (yet).
//!
//! @seealso
//!   @[client_1], @[server_1]
class SCRAM
{
  private string(8bit) first, nonce;

  private string(7bit) encode64(string(8bit) raw) {
    return MIME.encode_base64(raw, 1);
  }

  private string(7bit) randomstring() {
    return encode64(random_string(18));
  }

  private string(7bit) clientproof(string(8bit) salted_password) {
    _HMAC.State hmacsaltedpw = HMAC(salted_password);
    salted_password = hmacsaltedpw("Client Key");
    // Returns ServerSignature through nonce
    nonce = encode64(HMAC(hmacsaltedpw("Server Key"))(first));
    return encode64([string(8bit)]
		    (salted_password ^ HMAC(hash(salted_password))(first)));
  }

  //! Client-side step 1 in the SCRAM handshake.
  //!
  //! @param username
  //!   The username to feed to the server.  Some servers already received
  //!   the username through an alternate channel (usually during
  //!   the hash-function selection handshake), in which case it
  //!   should be omitted here.
  //!
  //! @returns
  //!   The client-first request to send to the server.
  //!
  //! @seealso
  //!   @[client_2]
  string(7bit) client_1(void|string username) {
    nonce = randomstring();
    return [string(7bit)](first = [string(8bit)]sprintf("n,,n=%s,r=%s",
      username && username != "" ? Standards.IDNA.to_ascii(username, 1) : "",
      nonce));
  }

  //! Server-side step 1 in the SCRAM handshake.
  //!
  //! @param line
  //!   The received client-first request from the client.
  //!
  //! @returns
  //!   The username specified by the client.  Returns null
  //!   if the response could not be parsed.
  //!
  //! @seealso
  //!   @[server_2]
  string server_1(string(8bit) line) {
    constant format = "n,,n=%s,r=%s";
    string username, r;
    catch {
      first = [string(8bit)]line[3..];
      [username, r] = array_sscanf(line, format);
      nonce = [string(8bit)]r;
      r = Standards.IDNA.to_unicode(username);
    };
    return r;
  }

  //! Server-side step 2 in the SCRAM handshake.
  //!
  //! @param salt
  //!   The salt corresponding to the username that has been specified earlier.
  //!
  //! @param iters
  //!   The number of iterations the hashing algorithm should perform
  //!   to compute the authentication hash.
  //!
  //! @returns
  //!   The server-first challenge to send to the client.
  //!
  //! @seealso
  //!   @[server_3]
  string(7bit) server_2(string(8bit) salt, int iters) {
    string response = sprintf("r=%s,s=%s,i=%d",
      nonce += randomstring(), encode64(salt), iters);
    first += "," + response + ",";
    return [string(7bit)]response;
  }

  //! Client-side step 2 in the SCRAM handshake.
  //!
  //! @param line
  //!   The received server-first challenge from the server.
  //!
  //! @param pass
  //!   The password to feed to the server.
  //!
  //! @returns
  //!   The client-final response to send to the server.  If the response is
  //!   null, the server sent something unacceptable or unparseable.
  //!
  //! @seealso
  //!   @[client_3]
  string(7bit) client_2(string(8bit) line, string(8bit) pass) {
    constant format = "r=%s,s=%s,i=%d";
    string(8bit) r, salt;
    int iters;
    if (!catch([r, salt, iters] = [array(string(8bit)|int)]
				   array_sscanf(line, format))
	&& iters > 0
	&& has_prefix(r, nonce)) {
      line = [string(8bit)]sprintf("c=biws,r=%s", r);
      first = [string(8bit)]sprintf("%s,r=%s,s=%s,i=%d,%s",
				    first[3..], r, salt, iters, line);
      if (pass != "")
	pass = [string(7bit)]Standards.IDNA.to_ascii(pass);
      salt = MIME.decode_base64(salt);
      nonce = [string(8bit)]sprintf("%s,%s,%d", pass, salt, iters);
      if (!(r = SCRAM_get_salted_password(nonce))) {
	r = pbkdf2(pass, salt, iters, digest_size());
	SCRAM_set_salted_password(r, nonce);
      }
      salt = sprintf("%s,p=%s", line, clientproof(r));
      first = 0;                         // Free memory
    } else
      salt = 0;
    return [string(7bit)]salt;
  }

  //! Final server-side step in the SCRAM handshake.
  //!
  //! @param line
  //!   The received client-final challenge and response from the client.
  //!
  //! @param salted_password
  //!   The salted (using the salt provided earlier) password belonging
  //!   to the specified username.
  //!
  //! @returns
  //!   The server-final response to send to the client.  If the response
  //!   is null, the client did not supply the correct credentials or
  //!   the response was unparseable.
  string(7bit) server_3(string(8bit) line,
			string(8bit) salted_password) {
    constant format = "c=biws,r=%s,p=%s";
    string r, p;
    if (!catch([r, p] = array_sscanf(line, format))
	&& r == nonce) {
      first += sprintf("c=biws,r=%s", r);
      p = p == clientproof(salted_password) && sprintf("v=%s", nonce);
    }
    return [string(7bit)]p;
  }

  //! Final client-side step in the SCRAM handshake.  If we get this far, the
  //! server has already verified that we supplied the correct credentials.
  //! If this step fails, it means the server does not have our
  //! credentials at all and is an imposter.
  //!
  //! @param line
  //!   The received server-final verification response.
  //!
  //! @returns
  //!   True if the server is valid, false if the server is invalid.
  int(0..1) client_3(string(8bit) line) {
    constant format = "v=%s";
    string v;
    return !catch([v] = array_sscanf(line, format))
      && v == nonce;
  }
}
