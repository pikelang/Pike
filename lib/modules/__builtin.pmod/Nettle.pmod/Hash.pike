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

//! Returns a human readable name for the algorithm.
string name();

//! Returns the size of a hash digests.
int digest_size();

//! Returns the internal block size of the hash algorithm.
int block_size();

//! This is the context for a single incrementally updated hash.
//!
//! Most of the functions here are only prototypes, and need to
//! be overrided via inherit.
class State
{
  //! Create the new context, and optionally
  //! add some initial data to hash.
  //!
  //! The default implementation calls @[update()] with @[data] if any,
  //! so there's usually no reason to override this function, since
  //! overriding @[update()] should be sufficient.
  protected void create(string|void data)
  {
    if (data) {
      update(data);
    }
  }

  //! Reset the context, and optionally
  //! add some initial data to the hash.
  this_program init(string|void data);

  //! Add some more data to hash.
  this_program update(string data);

  //! Generates a digests, and resets the hashing contents.
  //!
  //! @param length
  //!   If the length argument is provided, the digest is truncated
  //!   to the given length.
  //!
  //! @returns
  //!   The digest.
  string digest(int|void length);
}

//! Calling `() will return a @[State] object.
State `()() { return State(); }

//! @decl string hash(string data)
//! @decl string hash(Stdio.File file, void|int bytes)
//!
//! Shortcut for hashing some data.
//!
//! @param data
//!   String to hash.
//!
//! @param file
//!   @[Stdio.File] object to read some data to hash from.
//!
//! @param bytes
//!   The number of bytes of the file object @[file] that should be
//!   hashed. Negative numbers are ignored and the whole file is
//!   hashed.
//!
//!  Works as a (possibly faster) shortcut for
//!  @expr{State(data)->digest()@}, where @[State] is
//!  the hash state class corresponding to this @[Hash].
//!
//! @seealso
//!   @[Stdio.File], @[State()->update()] and @[State()->digest()].
string hash(string|Stdio.File in, int|void bytes)
{
  if (objectp(in)) {
    if (bytes) {
      in = in->read(bytes);
    } else {
      in = in->read();
    }
  }
  return State([string]in)->digest();
}

/* NOTE: This is NOT the MIME base64 table! */
protected constant b64tab =
  "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

/* NOTE: This IS the MIME base64 table! */
protected constant base64tab =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

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
string crypt_hash(string password, string salt, int rounds)
{
  if (!rounds) rounds = 5000;
  if (rounds < 1000) rounds = 1000;
  if (rounds > 999999999) rounds = 999999999;

  // FIXME: Send the first param directly to create()?
  State hash_obj = State();

  function(string:State) update = hash_obj->update;
  function(:string) digest = hash_obj->digest;

  salt = salt[..15];

  /* NB: Comments refer to http://www.akkadia.org/drepper/SHA-crypt.txt */
  string b = update(password + salt + password)->digest();	/* 5-8 */

  update(password + salt);					/* 2-3 */

  if (sizeof(b)) {
    int i;
    for (i=sizeof(password); i <= sizeof(b); i += sizeof(b)) {	/* 9 */
      update(b);
    }
    if (i) {							/* 10 */
      update(b[..i-1]);
    }
  }

  for (int i = 1; i < sizeof(password); i <<= 1) {		/* 11 */
    if (sizeof(password) & i) {
      update(b);
    } else {
      update(password);
    }
  }

  string a = digest();						/* 12 */

  for (int i = 0; i < sizeof(password); i++) {			/* 14 */
    update(password);
  }
  string dp = digest();						/* 15 */

  if (sizeof(dp) && (sizeof(dp) != sizeof(password))) {
    dp *= 1 + (sizeof(password)-1)/sizeof(dp);			/* 16 */
    dp = dp[..sizeof(password)-1];
  }

  for(int i = 0; i < 16 + (a[0] & 0xff); i++) {			/* 18 */
    update(salt);
  }
  string ds = digest();						/* 19 */

  if (sizeof(ds) && (sizeof(ds) != sizeof(salt))) {
    ds *= 1 + (sizeof(salt)-1)/sizeof(ds);			/* 20 */
    ds = ds[..sizeof(salt)-1];
  }

  for (int r = 0; r < rounds; r++) {				/* 21 */
    if (r & 1) {						/* b */
      hash(dp);
    } else {							/* c */
      hash(a);
    }
    if (r % 3) {						/* d */
      hash(ds);
    }
    if (r % 7) {						/* e */
      hash(dp);
    }
    if (r & 1) {						/* f */
      hash(a);
    } else {							/* g */
      hash(dp);
    }
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

  array(int) shuffled = ({});
  int shift = sizeof(a) % 3;

  for (int i = 0; i < sublength; i++) {
    int t = i & 1;
    int tt = !i;
    for (int j = 0; j < 3; j++) {
      shuffled += ({ a[shuffler[j][t]] });
      shuffler[j][tt] = shuffler[j + shift][t] + 1;
    }
  }

  return replace(MIME.encode_base64((string)shuffled, 1),
		 base64tab/"", b64tab/"");
}

//! Password Based Key Derivation Function #1 from RFC 2898. This
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
//!   RFC 2898 does not recommend this function for anything else than
//!   compatibility with existing applications, due to the limits in
//!   the length of the generated keys.
//!
//! @seealso
//!   @[pbkdf2()], @[openssl_pbkdf()], @[crypt_password()]
string pbkdf1(string password, string salt, int rounds, int bytes)
{
  if( bytes>digest_size() )
    error("Requested bytes %d exceeds hash digest size %d.\n",
          bytes, digest_size());
  if( rounds <=0 )
    error("Rounds needs to be 1 or higher.\n");

  string res = password + salt;

  password = "CENSORED";

  while (rounds--) {
    res = hash(res);
  }

  return res[..bytes-1];
}

//! Password Based Key Derivation Function #2 from RFC 2898, PKCS#5
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
//!   @[pbkdf1()], @[openssl_pbkdf()], @[crypt_password()]
string pbkdf2(string password, string salt, int rounds, int bytes)
{
  if( rounds <=0 )
    error("Rounds needs to be 1 or higher.\n");

  HMAC hmac = HMAC(password);
  password = "CENSORED";

  string res = "";
  int dsz = digest_size();
  int fragno;
  while (sizeof(res) < bytes) {
    string frag = "\0" * dsz;
    string buf = salt + sprintf("%4c", ++fragno);
    for (int j = 0; j < rounds; j++) {
      buf = hmac(buf);
      frag ^= buf;
    }
    res += frag;
  }

  return res[..bytes-1];
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
//!   It seems to be related to PBKDF1 from RFC2898.
//!
//! @seealso
//!   @[pbkdf1()], @[pbkdf2()], @[crypt_password()]
string openssl_pbkdf(string password, string salt, int rounds, int bytes)
{
  string out = "";
  string h = "";
  string seed = password + salt;

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

protected function(string, this_program:string) build_digestinfo;

//! Make a PKCS-1 digest info block with the message @[s].
//!
//! @seealso
//!   @[Standards.PKCS.build_digestinfo()]
string pkcs_digest(string s)
{
  if (!build_digestinfo) {
    // NB: We MUST NOT use other modules at compile-time,
    //     so we load Standards.PKCS.Signature on demand.
    object pkcs = [object]master()->resolve("Standards.PKCS.Signature");
    build_digestinfo = [function(string,this_program:string)]pkcs->build_digestinfo;
  }
  return build_digestinfo(s, this);
}

//! HMAC (Hashing for Message Authenticity Control) for the hash algorithm.
//!
//! RFC 2104.
//!
//! @seealso
//!   @[Crypto.HMAC]
class HMAC
{
  inherit State;

  protected string ikey; /* ipad XOR:ed with the key */
  protected string okey; /* opad XOR:ed with the key */

  protected State h;

  //! @param passwd
  //!   The secret password (K).
  protected void create (string passwd, void|int b)
  {
    if (!b)
      b = block_size();
    else if (digest_size()>b)
      error("Block size is less than hash digest size.\n");
    if (sizeof(passwd) > b)
      passwd = hash(passwd);
    if (sizeof(passwd) < b)
      passwd = passwd + "\0" * (b - sizeof(passwd));

    ikey = passwd ^ ("6" * b);
    okey = passwd ^ ("\\" * b);
  }

  //! Hashes the @[text] according to the HMAC algorithm and returns
  //! the hash value.
  //!
  //! This works as a combined @[update()] and @[digest()].
  string `()(string text)
  {
    return hash(okey + hash(ikey + text));
  }

  this_program update(string data)
  {
    if( !h )
    {
      h = State();
      h->update(ikey);
    }
    h->update(data);
    return this;
  }

  string digest(int|void length)
  {
    string res = hash(okey + h->digest());
    h = 0;

    if (length) return res[..length-1];
    return res;
  }

  //! Hashes the @[text] according to the HMAC algorithm and returns
  //! the hash value as a PKCS-1 digestinfo block.
  string digest_info(string text)
  {
    return pkcs_digest(okey + hash(ikey + text));
  }
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
string P_hash(string password, string salt, int rounds, int bytes)
{
  HMAC hmac = HMAC(password);
  string temp = salt;
  string res="";

  while (sizeof(res) < bytes) {
    temp = hmac(temp);
    res += hmac(temp + salt);
  }
  return res[..(bytes-1)];
}

