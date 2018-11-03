#pike __REAL_VERSION__

//! Module for creating OAuth signatures

#include "oauth.h"

import ".";

//! Signature type for the Base class.
protected constant NONE = 0;

//! Signature type for plaintext signing.
constant PLAINTEXT = 1;

//! Signature type for hmac sha1 signing.
constant HMAC_SHA1 = 2;

//! Signature type for rsa sha1 signing.
constant RSA_SHA1  = 3;

//! Signature types to signature key mapping.
constant SIGTYPE = ([
  NONE      : "",
  PLAINTEXT : "PLAINTEXT",
  HMAC_SHA1 : "HMAC-SHA1",
  RSA_SHA1  : "RSA-SHA1"
]);

//! Returns a signature class for signing with @[type].
//!
//! @throws
//!  An error if @[type] is unknown
//!
//! @param type
//!  Either @[PLAINTEXT], @[HMAC_SHA1] or @[RSA_SHA1].
Base get_object(int type)
{
  switch (type)
  {
    case PLAINTEXT: return Plaintext();
    case HMAC_SHA1: return HmacSha1();
    case RSA_SHA1:  return RsaSha1();
    default: /* nothing */
  }

  error("Uknknown signature type");
}

//! Base signature class.
protected class Base
{
  //! Signature type.
  protected int type = NONE;

  //! String representation of signature type.
  protected string method = SIGTYPE[NONE];

  //! Returns the @[type].
  int get_type()
  {
    return type;
  }

  //! Returns the @[method].
  string get_method()
  {
    return method;
  }

  //! Builds the signature string.
  string build_signature(Request request, Consumer consumer, Token token);
}

//! Plaintext signature.
protected class Plaintext
{
  inherit Base;
  protected int    type   = PLAINTEXT;
  protected string method = SIGTYPE[PLAINTEXT];

  //! Builds the signature string.
  string build_signature(Request request, Consumer consumer, Token token)
  {
    return Protocols.HTTP.uri_encode(sprintf("%s&%s", consumer->secret, token->secret));
  }
}

//! HMAC_SHA1 signature.
protected class HmacSha1
{
  inherit Base;
  protected int    type   = HMAC_SHA1;
  protected string method = SIGTYPE[HMAC_SHA1];

  //! Builds the signature string.
  string build_signature(Request request, Consumer consumer, Token token)
  {
    if (!token) token = Token("","");
    string sigbase = request->get_signature_base();
    string key = sprintf("%s&%s", Protocols.HTTP.uri_encode(consumer->secret),
                                  Protocols.HTTP.uri_encode(token->secret||""));
    return MIME.encode_base64(Crypto.SHA1.HMAC(key)(sigbase),1);
  }
}

//! RSA_SHA1 signature. Currently not implemented.
protected class RsaSha1
{
  inherit Base;
  protected int    type   = RSA_SHA1;
  protected string method = SIGTYPE[RSA_SHA1];

  //! Builds the signature string.
  string build_signature(Request request, Consumer consumer, Token token)
  {
    error("%s is not implemented.\n", CLASS_NAME(this));
  }
}
