/*
  Author: Pontus Östlund <https://profiles.google.com/poppanator>

  Permission to copy, modify, and distribute this source for any legal
  purpose granted as long as my name is still attached to it. More
  specifically, the GPL, LGPL and MPL licenses apply to this software.
*/

//! Module for creating OAuth signatures

#include "oauth.h"

import ".";

//! Signature type for the Base class.
protected constant NONE = 0;

//! Signature type for plaintext signing
constant PLAINTEXT = 1;

//! Signature type for hmac sha1 signing
constant HMAC_SHA1 = 2;

//! Signature type for rsa sha1 signing
constant RSA_SHA1  = 3;

//! Signature types to signature key mapping
constant SIGTYPE = ([
  NONE      : "",
  PLAINTEXT : "PLAINTEXT",
  HMAC_SHA1 : "HMAC-SHA1",
  RSA_SHA1  : "RSA-SHA1"
]);

//! Returns a signature class for signing with @[type]
//!
//! @throws
//!  An error if @[type] is unknown
//!
//! @param type
//!  Either @[PLAINTEXT], @[HMAC_SHA1] or
//!  @[RSA_SHA1].
object get_object(int type)
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

//! Base signature class
protected class Base
{
  //! Signature type
  protected int type = NONE;

  //! String representation of signature typ
  protected string method = SIGTYPE[NONE];

  //! Returns the @[type]
  int get_type()
  {
    return type;
  }

  //! Returns the @[method]
  string get_method()
  {
    return method;
  }

  //! Builds the signature string
  //!
  //! @param request
  //! @param cosumer
  //! @param token
  string build_signature(Request request, Consumer consumer, Token token);
}

//! Plaintext signature
protected class Plaintext
{
  inherit Base;
  protected int    type   = PLAINTEXT;
  protected string method = SIGTYPE[PLAINTEXT];

  //! Builds the signature string
  //!
  //! @param request
  //! @param cosumer
  //! @param token
  string build_signature(Request request, Consumer consumer, Token token)
  {
    return uri_encode(sprintf("%s&%s", consumer->secret, token->secret));
  }
}

//! HMAC_SHA1 signature
protected class HmacSha1
{
  inherit Base;
  protected int    type   = HMAC_SHA1;
  protected string method = SIGTYPE[HMAC_SHA1];

  //! Builds the signature string
  //!
  //! @param request
  //! @param cosumer
  //! @param token
  string build_signature(Request request, Consumer consumer, Token token)
  {
    if (!token) token = Token("","");
    string sigbase = request->get_signature_base();
    string key = sprintf("%s&%s", uri_encode(consumer->secret),
                                  uri_encode(token->secret||""));
    return MIME.encode_base64(
#if constant(Crypto.HMAC) && constant(Crypto.SHA1)
      Crypto.HMAC(Crypto.SHA1)(key)(sigbase)
#else
      MY_HMAC_SHA1(Crypto.sha)(key)(sigbase)
#endif
    );
  }
}

//! RSA_SHA1 signature
protected class RsaSha1
{
  inherit Base;
  protected int    type   = RSA_SHA1;
  protected string method = SIGTYPE[RSA_SHA1];

  //! Builds the signature string
  //!
  //! @param request
  //! @param cosumer
  //! @param token
  string build_signature(Request request, Consumer consumer, Token token)
  {
    error("%s is not implemented.\n", CLASS_NAME(this));
  }
}

#if !(constant(Crypto.HMAC) && constant(Crypto.SHA1))
// Compat class for Pike 7.4
// This is a mashup of the 7.4 Crypto.hmac and 7.8 Crypto.HMAC
class MY_HMAC_SHA1
{
  function H;
  int B;

  void create(function h, int|void b)
  {
    H = h;
    B = b || 64;
  }

  string raw_hash(string s)
  {
    return H()->update(s)->digest();
  }

  string pkcs_digest(string s)
  {
    return Standards.PKCS.Signature.build_digestinfo(s, H());
  }

  class `()
  {
    string ikey, okey;

    void create(string passwd)
    {
      if (sizeof(passwd) > B)
        passwd = raw_hash(passwd);
      if (sizeof(passwd) < B)
        passwd = passwd + "\0" * (B - sizeof(passwd));

      ikey = passwd ^ ("6" * B);
      okey = passwd ^ ("\\" * B);
    }

    string `()(string text)
    {
      return raw_hash(okey + raw_hash(ikey + text));
    }
  }
}
#endif
