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
//! @param media_type
//!   The media type of @[tbs], cf @rfc{7515:4.1.9@}.
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
			mixed tbs,
			string(7bit)|void media_type)
{
  string json_tbs = Standards.JSON.encode(tbs);
  mapping(string(7bit):string(7bit)) header = ([]);
  if (media_type) {
    header->typ = media_type;
  }
  return sign->jose_sign &&
    sign->jose_sign(string_to_utf8(json_tbs), header);
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
    decoded_jws[1] = Standards.JSON.decode_utf8(decoded_jws[1]);
    return decoded_jws;
  };
  return 0;
}

//! Encode a JSON Web Token (JWT). Automatically adds the optional
//! "issued at" timestamp and JWD ID (using UUID v4).
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
  return encode_jws(sign, claims, "JWT");
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
    if (jwk->d) {
      Crypto.ECC.Curve.ECDSA ecdsa = c.ECDSA(jwk);
      ecdsa->set_private_key(MIME.decode_base64url(jwk->d));
      return ecdsa;
    }
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
variant Crypto.Sign.State|Crypto.MAC.State decode_jwk(string(/*7bit*/8bit) jwk)
{
  return decode_jwk(Standards.JSON.decode_utf8(MIME.decode_base64url(jwk)));
}

//! Encode a JSON Web Key (JWK).
string(7bit) encode_jwk(mapping(string(7bit):string(7bit)) jwk)
{
  if (!mappingp(jwk)) return UNDEFINED;
  return MIME.encode_base64url(string_to_utf8(Standards.JSON.encode(jwk)));
}

//! Encode a JSON Web Key (JWK).
//!
//! @param sign
//!   The initialized @[Crypto.Sign.State] or @[Crypto.MAC.State]
//!   for which a JWK is to be generated.
//!
//! @param private_key
//!   If true the private fields of @[sign] will also be encoded into
//!   the result.
//!
//! @returns
//!   Returns the corresponding JWK.
variant string(7bit) encode_jwk(Crypto.Sign.State|Crypto.MAC.State sign,
				int(0..1)|void private_key)
{
  mapping(string(7bit):string(7bit)) jwk = sign && sign->jwk(private_key);
  if (!jwk) return UNDEFINED;
  return encode_jwk(jwk);
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
  return decode_jwk_set(Standards.JSON.decode_utf8(jwk_set));
}
