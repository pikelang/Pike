#pike __REAL_VERSION__
#pragma strict_types

//! Base class for Message Authentication Codes (MAC)s.
//!
//! These are hashes that have been extended with a secret key.

inherit .__Hash;

//! Returns the recomended size of the key.
int(0..) key_size();

//! Returns the size of the iv/nonce (if any).
//!
//! Some MACs like eg @[Crypto.SHA1.HMAC] have fixed ivs,
//! in which case this function will return @expr{0@}.
int(0..) iv_size();

//! JWS algorithm id (if any).
//! Overloaded by the actual implementations.
//!
//! @note
//!   Never access this value directly. Use @[jwa()].
//!
//! @seealso
//!   @[jwa()]
protected constant mac_jwa_id = "";

//! JWS algorithm identifier (if any, otherwise @expr{0@}).
//!
//! @seealso
//!   @rfc{7518:3.1@}
string(7bit) jwa()
{
  return (mac_jwa_id != "") && [string(7bit)]mac_jwa_id;
}

//! The state for the MAC.
class State
{
  inherit ::this_program;

  //! @param key
  //!   The secret key for the hash.
  protected void create(string key);

  //! Returns the recomended size of the key.
  int(0..) key_size()
  {
    return global::key_size();
  }

  //! Returns the size of the iv/nonce (if any).
  //!
  //! Some MACs like eg @[Crypto.SHA1.HMAC] have fixed ivs,
  //! in which case this function will return @expr{0@}.
  int(0..) iv_size()
  {
    return global::iv_size();
  }

  //! Signs the @[message] with a JOSE JWS MAC signature.
  //!
  //! @param message
  //!   Message to sign.
  //!
  //! @param headers
  //!   JOSE headers to use. Typically a mapping with a single element
  //!   @expr{"typ"@}.
  //!
  //! @returns
  //!   Returns the signature on success, and @expr{0@} (zero)
  //!   on failure (typically that JOSE doesn't support this MAC).
  //!
  //! @seealso
  //!   @[jose_decode()], @rfc{7515@}
  string(7bit)|zero
    jose_sign(string(8bit) message,
              mapping(string(7bit):string(7bit)|int)|void headers)
  {
    string(7bit) alg = jwa();
    if (!alg) return 0;
    headers = headers || ([]);
    headers += ([ "alg": alg ]);
    string(7bit) tbs =
      sprintf("%s.%s",
              [string(7bit)]Pike.Lazy.MIME.encode_base64url(string_to_utf8([string]Pike.Lazy.Standards.JSON.encode(headers))),
              [string(7bit)]Pike.Lazy.MIME.encode_base64url(message));
    init(tbs);
    string(8bit) raw = digest();
    return sprintf("%s.%s", tbs, [string(7bit)]Pike.Lazy.MIME.encode_base64url(raw));
  }

  //! Verify and decode a JOSE JWS MAC signed value.
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
  //!   @[jose_sign()], @rfc{7515:3.5@}
  array(mapping(string(7bit):string(7bit)|int)|string(8bit))|zero
    jose_decode(string(7bit) jws)
  {
    string(7bit) alg = jwa();
    if (!alg) return 0;
    array(string(7bit)) segments = [array(string(7bit))](jws/".");
    if (sizeof(segments) != 3) return 0;
    catch {
      mapping(string(7bit):string(7bit)|int) headers =
	[mapping(string(7bit):string(7bit)|int)](mixed)
        Pike.Lazy.Standards.JSON.decode(utf8_to_string([string(8bit)]Pike.Lazy.MIME.decode_base64url(segments[0])));
      if (!mappingp(headers)) return 0;
      if (headers->alg != alg) return 0;
      string(7bit) tbs = sprintf("%s.%s", segments[0], segments[1]);
      init(tbs);
      string(8bit) raw = digest();
      if (Pike.Lazy.MIME.encode_base64url(raw) == segments[2]) {
        return ({ headers, [string(8bit)]Pike.Lazy.MIME.decode_base64url(segments[1]) });
      }
    };
    return 0;
  }
}
