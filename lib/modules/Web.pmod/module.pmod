//!
//! Modules implementing various web standards.
//!

//! Encode a JSON Web Token (JWT).
//!
//! @param sign
//!   The asymetric private or MAC key to use for signing the result.
//!
//! @param claims
//!   The set of claims for the token. See @rfc{7519:4@}.
//!
//! @returns
//!   Returns @expr{0@} (zero) on encoding failure (usually
//!   that @[sign] doesn't support JWS.
//!
//!   Returns a corresponding JWT on success.
//!
//! @note
//!   The claim @expr{"iat"@} (@rfc{7519:4.1.6@} is added unconditionally,
//!   and the claim @expr{"jti"@} (@rfc{7519:4.1.7@}) is added
//!   if not already present.
//!
//! @seealso
//!   @[decode_jwt()], @rfc{7519:4@}
string(7bit) encode_jwt(Crypto.Sign.State|Crypto.MAC.State sign,
			mapping(string:string|int) claims)
{
  claims->iat = time(1);
  if (!claims->jti) claims->jti = (string)Standards.UUID.make_version4();
  string json_claims = Standards.JSON.encode(claims);
  return sign->jose_sign &&
    sign->jose_sign(string_to_utf8(json_claims), ([ "typ": "JWT" ]));
}

//! Decode a JSON Web Token (JWT).
//!
//! @param sign
//!   The asymetric public or MAC key(s) to validate the jwt against.
//!
//! @param jwt
//!   A JWT as eg returned by @[encode_jwt()].
//!
//! @returns
//!   Returns @expr{0@} (zero) on validation failure (this
//!   includes validation of expiry times).
//!
//!   Returns a mapping of the claims for the token on success.
//!   See @rfc{7519:4@}.
//!
//! @note
//!   The time check of the @expr{"nbf"@} value has a hard coded
//!   60 second grace time (as allowed by @rfc{7519:4.1.5@}).
//!
//! @seealso
//!   @[encode_jwt()], @rfc{7519:4@}
mapping(string:string|int) decode_jwt(Crypto.Sign.State|Crypto.MAC.State|
				      array(Crypto.Sign|Crypto.MAC.State) sign,
				      string(7bit) jwt)
{
  if (!arrayp(sign)) sign = ({ sign });
  array(mapping(string(7bit):string(7bit)|int)|string(8bit)) jws;
  foreach(sign, Crypto.Sign s) {
    if (jws = s->jose_decode(jwt)) break;
  }
  if (!jws) return 0;
  [mapping(string(7bit):string(7bit)|int) jose_header,
   string(8bit) encoded_claims] = jws;
  if ((jose_header->typ || "JWT") != "JWT") return 0;
  catch {
    mapping(string:string|int) claims = Standards.JSON.decode(encoded_claims);
    if (!mappingp(claims)) return 0;
    int now = time(1);
    if (!zero_type(claims->exp) && (claims->exp < now)) return 0;
    if (claims->nbf - 60 > now) return 0;
    return claims;
  };
  return 0;
}
