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
State `()() { return State(); }

//! @decl string(8bit) hash(string(8bit) data)
//! @decl string(8bit) hash(Stdio.File file, void|int bytes)
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
string(8bit) hash(string(8bit)|Stdio.File in, int|void bytes)
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

//! @module HMAC
//!
//! HMAC (Hashing for Message Authenticity Control) for the hash algorithm.
//!
//! RFC 2104.
//!
//! @seealso
//!   @[Crypto.HMAC]

//! JWS algorithm id (if any) for the HMAC sub-module.
//! Overloaded by the actual implementations.
protected constant hmac_jwa_id = "";

//! @ignore
private class _HMAC
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
    string(8bit) `()(string(8bit) text)
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

    string digest(int|void length)
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
	"k": MIME.encode_base64url([string(8bit)](ikey ^ ("6" * block_size()))),
      ]);
      return jwk;
    }
  }

  //! Returns a new @[State] object initialized with a @[password].
  State `()(string(8bit) password, void|int b)
  {
    return State(password, b);
  }

//! @ignore
}

_HMAC HMAC = _HMAC();

//! @endignore

//! @endmodule HMAC

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
string(7bit) crypt_hash(string(8bit) password, string(8bit) salt, int rounds)
{
  if (!rounds) rounds = 5000;
  if (rounds < 1000) rounds = 1000;
  if (rounds > 999999999) rounds = 999999999;

  // FIXME: Send the first param directly to create()?
  State hash_obj = State();

  function(string(8bit):State) update = hash_obj->update;
  function(:string(8bit)) digest = hash_obj->digest;

  salt = salt[..15];

  /* NB: Comments refer to http://www.akkadia.org/drepper/SHA-crypt.txt */
  string(8bit) b = update(password + salt + password)->digest(); /* 5-8 */

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

  string(8bit) a = digest();					/* 12 */

  for (int i = 0; i < sizeof(password); i++) {			/* 14 */
    update(password);
  }
  string(8bit) dp = digest();					/* 15 */

  if (sizeof(dp) && (sizeof(dp) != sizeof(password))) {
    dp *= 1 + (sizeof(password)-1)/sizeof(dp);			/* 16 */
    dp = dp[..sizeof(password)-1];
  }

  for(int i = 0; i < 16 + (a[0] & 0xff); i++) {			/* 18 */
    update(salt);
  }
  string(8bit) ds = digest();					/* 19 */

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

  return [string(7bit)]
    replace(MIME.encode_base64((string)shuffled, 1),
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
string(8bit) pbkdf1(string(8bit) password, string(8bit) salt,
		    int rounds, int bytes)
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

protected function(object|string(8bit), this_program:string(8bit)) build_digestinfo;

//! Make a PKCS-1 digest info block with the message @[s].
//!
//! @seealso
//!   @[Standards.PKCS.build_digestinfo()]
string(8bit) pkcs_digest(object|string(8bit) s)
{
  if (!build_digestinfo) {
    // NB: We MUST NOT use other modules at compile-time,
    //     so we load Standards.PKCS.Signature on demand.
    object pkcs = [object]master()->resolve("Standards.PKCS.Signature");
    build_digestinfo = [function(object|string(8bit),this_program:string(8bit))]
      pkcs->build_digestinfo;
  }
  return build_digestinfo(s, this);
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
    string(8bit) r, p;
    string(7bit) ret;
    if (!catch([r, p] = array_sscanf(line, format))
	&& r == nonce) {
      first += sprintf("c=biws,r=%s", r);
      ret = p == clientproof(salted_password) && sprintf("v=%s", nonce);
    }
    return ret;
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
