#pike __REAL_VERSION__

//!
//! Modules implementing various web standards.
//!

//! Encode a JSON Web Signature (JWS).
//!
//! @param sign
//!   The asymetric private or MAC key to use for signing the result.
//!
//! @param tbs
//!   The value to sign.
//!
//! @param content_type
//!   The content type of @[tbs].
//!
//! @returns
//!   Returns @expr{0@} (zero) on encoding failure (usually
//!   that @[sign] doesn't support JWS.
//!
//!   Returns a corresponding JWS on success.
//!
//! @seealso
//!   @[decode_jwt()], @rfc{7515@}
string(7bit) encode_jws(Crypto.Sign.State|Crypto.MAC.State sign,
			string(7bit) content_type,
			mixed tbs)
{
  string json_tbs = Standards.JSON.encode(tbs);
  return sign->jose_sign &&
    sign->jose_sign(string_to_utf8(json_tbs), ([ "typ": content_type ]));
}

//! Decode a JSON Web Signature (JWS).
//!
//! @param sign
//!   The asymetric public or MAC key(s) to validate the jws against.
//!
//! @param jws
//!   A JWS as eg returned by @[encode_jws()].
//!
//! @returns
//!   Returns @expr{0@} (zero) on validation failure.
//!
//!   Returns an array with two elements on success:
//!   @array
//!     @elem mapping(string(7bit):string(7bit)|int) 0
//!       The JOSE header.
//!     @elem mixed 1
//!       The JWS payload.
//!   @endarray
//!   See @rfc{7515:3@}.
//!
//! @seealso
//!   @[encode_jws()], @[decode_jwt()], @[Crypto.Sign.State()->jose_decode()],
//!   @rfc{7515@}
array decode_jws(array(Crypto.Sign.State|Crypto.MAC.State)|
		 Crypto.Sign.State|Crypto.MAC.State sign,
		 string(7bit) jws)
{
  if (!arrayp(sign)) sign = ({ sign });
  array(mapping(string(7bit):string(7bit)|int)|string(8bit)) decoded_jws;
  foreach(sign, Crypto.Sign s) {
    if (decoded_jws = s->jose_decode(jws)) {
      break;
    }
  }
  if (!decoded_jws) return 0;
  catch {
    decoded_jws[1] = Standards.JSON.decode(decoded_jws[1]);
    return decoded_jws;
  };
  return 0;
}

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
  return encode_jws(sign, "JWT", claims);
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
//!   @[encode_jwt()], @[decode_jws()], @rfc{7519:4@}
mapping(string:string|int) decode_jwt(array(Crypto.Sign.State|Crypto.MAC.State)|
				      Crypto.Sign.State|Crypto.MAC.State sign,
				      string(7bit) jwt)
{
  array(mapping(string(7bit):string(7bit)|int)|string(8bit)) jws =
    decode_jws(sign, jwt);
  if (!jws) return 0;
  [mapping(string(7bit):string(7bit)|int) jose_header,
   mapping(string:string|int) claims] = jws;
  if ((jose_header->typ || "JWT") != "JWT") return 0;
  if (!mappingp(claims)) return 0;
  int now = time(1);
  if (!zero_type(claims->exp) && (claims->exp < now)) return 0;
  if (claims->nbf - 60 > now) return 0;
  return claims;
}

#if constant(Crypto.ECC.Curve)
constant jose_to_pike = ([
  "P-256": "SECP_256R1",
  "P-384": "SECP_384R1",
  "P-521": "SECP_521R1",
]);
#endif

protected mapping(string(7bit):Crypto.MAC) mac_lookup;

//! Decode a JSON Web Key (JWK).
//!
//! @returns
//!   Returns an initialized @[Crypto.Sign.State] or @[Crypto.MAC.State]
//!   on success and @[UNDEFINED] on failure.
Crypto.Sign.State|Crypto.MAC.State decode_jwk(mapping(string(7bit):string(7bit)) jwk)
{
  mapping(string(7bit):Gmp.mpz) decoded_coordinates = ([]);
  foreach(({ "n", "e", "d", "x", "y" }), string coord) {
    if (!jwk[coord]) continue;
    string(8bit) bin = MIME.decode_base64url(jwk[coord]);
    decoded_coordinates[coord] = Gmp.mpz(bin, 256);
  }
  switch(jwk->kty) {
  case "RSA":
    return Crypto.RSA(decoded_coordinates);
#if constant(Crypto.ECC.Curve)
  case "EC":
    // RFC 7517:6.2.1.1
    string(7bit) pike_curve = jose_to_pike[jwk->crv];
    if (!pike_curve) break;
    Crypto.ECC.Curve c = Crypto.ECC[pike_curve];
    if (!c) break;
    return c.Point(decoded_coordinates->x, decoded_coordinates->y);
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
