#pike __REAL_VERSION__

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

#if constant(Crypto.ECC.Curve)
// NB: We index the Curve with the dot operator to get the Point.
#pragma dynamic_dot
protected mapping(string(7bit):Crypto.ECC.Curve) curve_lookup;
#endif

protected mapping(string(7bit):Crypto.MAC) mac_lookup;

//! Decode a JSON Web Key (JWK).
//!
//! @returns
//!   Returns an initialized @[Crypto.Sign.State] or @[Crypto.MAC.State]
//!   on success and @[UNDEFINED] on failure.
Crypto.Sign.State|Crypto.MAC.State decode_jwk(mapping(string(7bit):string(7bit)) jwk)
{
  switch(jwk->kty) {
  case "RSA":
    return Crypto.RSA(jwk);
#if constant(Crypto.ECC.Curve)
  case "EC":
    // RFC 7518:6.2
    if (!curve_lookup) {
      // NB: Thread safe.
      mapping(string(7bit):Crypto.ECC.Curve) m = ([]);
      foreach(values(Crypto.ECC), Crypto.ECC.Curve c) {
	string(7bit) n = objectp(c) && c->jose_name && c->jose_name();
	if (n) {
	  m[n] = c;
	}
      }
      curve_lookup = m;
    }
    // RFC 7518:6.2.1.1
    Crypto.ECC.Curve c = curve_lookup[jwk->crv];
    if (!c) break;
    return c.Point(jwk);
#endif /* constant(Crypto.ECC.Curve) */
  case "oct":
    // RFC 7518:6.4
    if (!mac_lookup) {
      // NB: Thread safe.
      mapping(string(7bit):Crypto.MAC) m = ([]);
      foreach(values(Crypto), mixed x) {
	if (!objectp(x) || !objectp(x = x["HMAC"]) || !x->jwa) continue;
	string(7bit) jwa = x->jwa();
	if (!jwa) continue;
	m[jwa] = x;
      }
      mac_lookup = m;
    }
    Crypto.MAC mac = mac_lookup[jwk->alg];
    if (!mac) break;
    string(7bit) key = jwk->k;
    if (!key) break;
    return mac(MIME.decode_base64url(key));
  default:
    break;
  }
  return UNDEFINED;
}

#pragma no_dynamic_dot

//! Decode a JSON Web Key (JWK).
//!
//! @returns
//!   Returns an initialized @[Crypto.Sign.State] or @[Crypto.MAC.State]
//!   on success and @[UNDEFINED] on failure.
variant Crypto.Sign.State|Crypto.MAC.State decode_jwk(string(7bit) jwk)
{
  return decode_jwk(Standards.JSON.decode(MIME.decode_base64url(jwk)));
}

//! Decode a JSON Web Key (JWK) Set.
//!
//! @seealso
//!   @rfc{7517:5@}, @[decode_jwk()]
array(Crypto.Sign.State|Crypto.MAC.State)
  decode_jwk_set(mapping(string(8bit):
			 array(mapping(string(7bit):string(7bit)))) jwk_set)
{
  return filter(map(jwk_set->keys, decode_jwk), objectp);
}

//! Decode a JSON Web Key (JWK) Set.
//!
//! @seealso
//!   @rfc{7517:5@}, @[decode_jwk()]
variant array(Crypto.Sign.State|Crypto.MAC.State)
  decode_jwk_set(string(7bit) jwk_set)
{
  return decode_jwk_set(Standards.JSON.decode(jwk_set));
}
