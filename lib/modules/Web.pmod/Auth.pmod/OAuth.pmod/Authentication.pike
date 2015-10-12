#pike __REAL_VERSION__

//! The purpose of this class is to streamline OAuth1 with OAuth2.
//! This class will not do much on it's own, since its purpose is to
//! be inherited by some other class implementing a specific authorization
//! service.

inherit Web.Auth.OAuth2.Client : oauth2;
inherit .Client : oauth;

#include "oauth.h"

//! The endpoint to send request for a request token.
constant REQUEST_TOKEN_URL = 0;

//! The endpoint to send request for an access token.
constant ACCESS_TOKEN_URL = 0;

//! The enpoint to redirect to when authorize an application.
constant USER_AUTH_URL = 0;

//! Creates an OAuth object.
//!
//! @param client_id
//!  The application ID.
//!
//! @param client_secret
//!  The application secret.
//!
//! @param redirect_uri
//!  Where the authorization page should redirect back to. This must be a
//!  fully qualified domain name. This can be set/overridden in
//!  @[get_request_token()].
//!
//! @param scope
//!  Extended permissions to use for this authentication. This can be
//!  set/overridden in @[get_auth_uri()].
protected void create(string client_id, string client_secret,
                      void|string redir,
                      void|string|array(string)|multiset(string) scope)
{
  oauth2::create(client_id, client_secret, redir, scope);
  oauth::create(.Consumer(client_id, client_secret),
                .Token(0, 0));
}

//! Set authentication.
void set_authentication(string key, void|string secret)
{
  token->key = key;
  token->secret = secret;
  oauth2::access_token = key;
}

//! Returns true if authenticated.
int(0..1) is_authenticated()
{
  return !!token->key && !!token->secret && !oauth2::is_expired();
}

//! Populate this object with the result from
//! @[request_access_token()].
//!
//! @returns
//!  The object being called.
this_program set_from_cookie(string encoded_value)
{
  mixed e = catch {
    oauth2::set_from_cookie(encoded_value);
    if (gettable->oauth_token) {
      oauth2::access_token = gettable->oauth_token;
      gettable->access_token = oauth2::access_token;
    }
    token = .Token(gettable->oauth_token, gettable->oauth_token_secret);
    return this;
  };

  error("Unable to decode cookie! %s. ", describe_error(e));
}

//! Fetches a request token.
//!
//! @param callback_uri
//!  Overrides the callback uri in the application settings.
//! @param force_login
//!  If @tt{1@} forces the user to provide its credentials at the Twitter
//!  login page.
.Token get_request_token(void|string|Standards.URI callback_uri,
                         void|int(0..1) force_login)
{
  mapping p = ([]);

  if (callback_uri)
    p->oauth_callback = (string)callback_uri;

  if (force_login)
    p->force_login = "true";

  TRACE("OAuth1: get_request_token(%O, %O)\n", REQUEST_TOKEN_URL, p);

  string ctoken = call(REQUEST_TOKEN_URL, p, "POST");
  mapping res = ctoken && (mapping).query_to_params(ctoken);

  TRACE("OAuth1: get_request_token result: %O\n", res);

  token = .Token(res[.TOKEN_KEY],
                 res[.TOKEN_SECRET_KEY]);
  return token;
}

//! Fetches an access token.
protected string low_get_access_token(void|string oauth_verifier)
{
  if (!token)
    error("Can't fetch access token when no request token is set!\n");

  .Params pm;

  if (oauth_verifier) {
    pm = .Params(.Param("oauth_verifier", oauth_verifier));
  }

  TRACE("OAuth1: get_access_token(%O, %O)\n", ACCESS_TOKEN_URL, pm);

  string ctoken = call(ACCESS_TOKEN_URL, pm, "POST");
  return ctoken;
}

//! Fetches an access token.
.Token get_access_token(void|string oauth_verifier)
{
  string ctoken = low_get_access_token(oauth_verifier);
  mapping p = (mapping).query_to_params(ctoken);

  token = .Token(p[.TOKEN_KEY],
                 p[.TOKEN_SECRET_KEY]);

  return token;
}

//! Same as @[get_access_token] except this returns a string to comply
//! with the OAuth2 authentication process.
string request_access_token(string code)
{
  string ctoken = low_get_access_token(code);
  mapping p = (mapping).query_to_params(ctoken);

  token = .Token(p[.TOKEN_KEY],
                 p[.TOKEN_SECRET_KEY]);

  return encode_value(p);
}

//!
string get_auth_uri(void|mapping args)
{
  if (!args) args = ([]);
  get_request_token(args->callback_uri||oauth2::_redirect_uri,
                    args->force_login);
  return sprintf("%s?%s=%s", USER_AUTH_URL, .TOKEN_KEY,
                 (token && token->key)||"");
}

//! Does the low level HTTP call to a service.
//!
//! @throws
//!  An error if HTTP status != 200
//!
//! @param url
//!  The full address to the service e.g:
//!  @tt{http://twitter.com/direct_messages.xml@}
//! @param args
//!  Arguments to send with the request
//! @param mehod
//!  The HTTP method to use
string call(string|Standards.URI url, void|mapping|.Params args,
            void|string method)
{
  method = normalize_method(method);

  if (mappingp(args)) {
    mapping m = copy_value(args);
    args = .Params();
    args->add_mapping(m);
  }

  TRACE("Token: %O\n", token);
  TRACE("Consumer: %O\n", consumer);
  TRACE("Args: %O\n", args);
  TRACE("Method: %O\n", method);

  .Request r = .request(url, consumer, token, args, method);
  r->sign_request(.Signature.HMAC_SHA1, consumer, token);

  Protocols.HTTP.Query q = r->submit();

  TRACE("Web.Auth.OAuth.Authentication()->call(%O) : %O : %s\n",
        q, q->headers, q->data());

  if (q->status != 200) {
    if (mapping e = parse_error_xml(q->data()))
      error("Error in %O: %s\n", e->request, e->error);
    else
      error("Bad status, %d, from HTTP query!\n", q->status);
  }

  return q->data();
}

//! Normalizes and verifies the HTTP method to be used in a HTTP call.
protected string normalize_method(string method)
{
  method = upper_case(method||"GET");
  if (!(< "GET", "POST", "DELETE", "PUT", "HEAD", "PATCH" >)[method])
    error("HTTP method must be GET, POST, PUT, HEAD, PATCH or DELETE! ");

  return method;
}

import Parser.XML.Tree;

//! Parses an error xml tree.
//!
//! @returns
//!  A mapping:
//!  @mapping
//!   @member string "request"
//!   @member string "error"
//!  @endmapping
mapping parse_error_xml(string xml)
{
  mapping m;
  if (Node n = get_xml_root(xml)) {
    m = ([]);
    foreach (n->get_children(), Node cn) {
      if (cn->get_node_type() == XML_ELEMENT)
        m[cn->get_tag_name()] = cn->value_of_node();
    }
  }

  return m;
}

// Returns the first @tt{XML_ELEMENT@} node in an XML tree.
//
// @param xml
//  Either an XML tree as a string or a node object.
private Node get_xml_root(string|Node xml)
{
  catch {
    if (stringp(xml))
      xml = parse_input(xml);
  };
  return xml->get_first_element();
}
