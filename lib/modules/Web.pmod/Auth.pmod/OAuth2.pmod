#pike __REAL_VERSION__

//! OAuth2 client
//!
//! A base OAuth2 class can be instantiated either via @[`()]
//! (@tt{Web.Auth.OAuth2(params...)@}) or via @[Web.Auth.OAuth2.Base()].

#ifdef SOCIAL_REQUEST_DEBUG
# define TRACE(X...) werror("%s:%d: %s", basename(__FILE__),__LINE__,sprintf(X))
#else
# define TRACE(X...) 0
#endif

//! Instantiate a generic OAuth2 @[Base] class.
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
//!  @[Base()->get_auth_uri()] and/or @[Base()->set_redirect_uri()].
//!
//! @param scope
//!  Extended permissions to use for this authentication. This can be
//!  set/overridden in @[Base()->get_auth_uri()].
protected Base `()(string client_id, string client_secret,
                   void|string redirect_uri,
                   void|string|array(string)|multiset(string) scope)
{
  return Base(client_id, client_secret, redirect_uri, scope);
}

//! Generic OAuth2 client class.
class Base
{
  //! Creates an OAuth2 object.
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
  //!  @[get_auth_uri()] and/or @[set_redirect_uri()].
  //!
  //! @param scope
  //!  Extended permissions to use for this authentication. This can be
  //!  set/overridden in @[get_auth_uri()].
  protected void create(string client_id,
                        string client_secret,
                        void|string redirect_uri,
                        void|string|array(string)|multiset(string) scope)
  {
    _client_id     = client_id;
    _client_secret = client_secret;
    _redirect_uri  = redirect_uri || _redirect_uri;
    _scope         = scope || _scope;
  }

  //! Request timeout in seconds. Only affects async queries.
  int(1..) http_request_timeout = 0;

  //! Grant types.
  enum GrantType {
    //!
    GRANT_TYPE_AUTHORIZATION_CODE = "authorization_code",

    //!
    GRANT_TYPE_IMPLICIT = "implicit",

    //!
    GRANT_TYPE_PASSWORD = "password",

    //!
    GRANT_TYPE_CLIENT_CREDENTIALS = "client_credentials",

    //!
    GRANT_TYPE_JWT = "urn:ietf:params:oauth:grant-type:jwt-bearer",

    //!
    GRANT_TYPE_REFRESH_TOKEN = "refresh_token"
  }

  //! Response types.
  enum ResponseType {
    //!
    RESPONSE_TYPE_CODE  = "code",

    //!
    RESPONSE_TYPE_TOKEN = "token"
  }

  //! Getter for @expr{access_token@}.
  string `access_token()
  {
    return gettable->access_token;
  }

  //! Can be used to set a stored access_token. Will also set creation and
  //! expiration time. This can be useful in apps that support non-expiring
  //! authentications.
  void `access_token=(string value)
  {
    gettable->access_token = value;
    gettable->created = time();
    gettable->expires = time() + (3600);
  }

  //! Getter for @expr{refresh_token@}.
  string `refresh_token()
  {
    return gettable->refresh_token;
  }

  //! Getter for @expr{token_type@}.
  string `token_type()
  {
    return gettable->token_type;
  }

  //! Getter for when the authentication @expr{expires@}.
  Calendar.Second `expires()
  {
    return gettable->expires && Calendar.Second("unix", gettable->expires);
  }

  //! Getter for when the authentication was @expr{created@}.
  Calendar.Second `created()
  {
    return Calendar.Second("unix", gettable->created);
  }

  //! Getter for the @expr{user@} mapping which may or may not be set.
  mapping `user()
  {
    return gettable->user;
  }

  //! Returns the application ID.
  string get_client_id()
  {
    return _client_id;
  }

  //! Returns the application secret.
  string get_client_secret()
  {
    return _client_secret;
  }

  //! Returns the redirect uri.
  string get_redirect_uri()
  {
    return _redirect_uri;
  }

  //! Returns the @expr{grant_type@} of the object.
  string get_grant_type()
  {
    return _grant_type;
  }

  //! Set the grant type to use.
  void set_grant_type(GrantType type)
  {
    _grant_type = type;
  }

  //! Get an @tt{access_token@} from a JWT.
  //! @url{http://jwt.io/@}
  //!
  //! @param jwt
  //!  JSON string.
  //! @param token_endpoint
  //!  URI to the request access_token endpoint.
  //! @param sub
  //!  Email/id of the requesting user.
  //! @param async_callback
  //!  If given the request will be made asynchronously.  The
  //!  signature is @tt{callback(bool, string)@}.  If successful the
  //!  second argument will be the result encoded with
  //!  @[predef::encode_value()], which can be stored and later used
  //!  to populate an instance via @[set_from_cookie()].
  //!
  //! @seealso
  //!  @[Web.encode_jwt()]
  string get_token_from_jwt(string jwt, void|string token_endpoint,
                            string|void sub,
                            void|function(bool,string:void) async_cb)
  {
    mapping j = Standards.JSON.decode(jwt);

    if (!token_endpoint)
      token_endpoint = j->token_uri;

    mapping claims = ([
      "iss"   : j->client_email,
      "scope" : get_valid_scopes(_scope),
      "aud"   : token_endpoint,
      "exp"   : time(1) + 3600,
    ]);

    if (sub) {
      claims->sub = sub;
    }

    string key =
      Standards.PEM.simple_decode(j->private_key);

    object(Standards.ASN1.Types.Sequence) x =
      [object(Standards.ASN1.Types.Sequence)]
      Standards.ASN1.Decode.simple_der_decode(key);

    Crypto.Sign.State sign = Standards.PKCS.parse_private_key(x);

    string s = Web.encode_jwt(sign, claims);

    string body = "grant_type="+Protocols.HTTP.uri_encode(GRANT_TYPE_JWT)+"&"+
                  "assertion="+Protocols.HTTP.uri_encode(s);

    if (async_cb) {
      Protocols.HTTP.Query q = Protocols.HTTP.Query();
      q->set_callbacks(
        lambda (Protocols.HTTP.Query qq, mixed ... args) {
          if (qq->status == 200) {
            mapping res = Standards.JSON.decode(q->data());
            if (!decode_access_token_response(q->data())) {
              async_cb(false, sprintf("Bad result! Expected an access_token "
                                      "but none were found!\nData: %s.\n",
                                      q->data()));
              return;
            }

            async_cb(true, encode_value(gettable));
          }
        },
        lambda (Protocols.HTTP.Query qq, mixed ... args) {
          async_cb(false, "Connection failed!");
        }
      );

      Protocols.HTTP.do_async_method("POST", token_endpoint, 0, request_headers,
                                     q, body);
      return 0;
    }
    else {
      Protocols.HTTP.Query q;
      q = Protocols.HTTP.do_method("POST", token_endpoint, 0, request_headers,
                                   0, body);

      if (q->status == 200) {
        mapping res = Standards.JSON.decode(q->data());
        if (!decode_access_token_response(q->data())) {
          error("Bad result! Expected an access_token but none were found!"
                "\nData: %s.\n", q->data());
        }

        return encode_value(gettable);
      }

      string ee = try_get_error(q->data());
      error("Bad status (%d) in response: %s! ",
            q->status, ee||"Unknown error");
    }
  }

  //! Setter for the redirect uri.
  void set_redirect_uri(string uri)
  {
    _redirect_uri = uri;
  }

  //! Returns the valid scopes.
  multiset list_valid_scopes()
  {
    return valid_scopes;
  }

  //! Set access_type explicilty.
  //!
  //! @param access_type
  //!  Like: offline
  void set_access_type(string access_type)
  {
    _access_type = access_type;
  }

  //! Getter for the access type, if any.
  string get_access_type()
  {
    return _access_type;
  }

  //! Set scopes.
  void set_scope(string scope)
  {
    _scope = scope;
  }

  //! Returns the scope/scopes set, if any.
  mixed get_scope()
  {
    return _scope;
  }

  //! Check if @[scope] exists in this object.
  int(0..1) has_scope(string scope)
  {
    if (!_scope || !sizeof(_scope))
      return 0;

    string sp = search(_scope, ",") > -1 ? "," :
                search(_scope, " ") > -1 ? " " : "";
    array p = map(_scope/sp, String.trim);

    return has_value(p, scope);
  }

  //! Populate this object with the result from @[request_access_token()].
  //!
  //! @throws
  //!  An error if the decoding of @[encoded_value] fails.
  //!
  //! @param encoded_value
  //!  The value from a previous call to @[request_access_token()] or
  //!  @[refresh_access_token()].
  //!
  //! @returns
  //!  The object being called.
  this_program set_from_cookie(string encoded_value)
  {
    mixed e = catch {
      gettable = decode_value(encoded_value);

      if (gettable->scope)
        _scope = gettable->scope;

      if (gettable->access_type)
        _access_type = gettable->access_type;

      return this;
    };

    error("Unable to decode cookie! %s. ", describe_error(e));
  }

  //! Returns an authorization URI.
  //!
  //! @param auth_uri
  //!  The URI to the remote authorization page
  //! @param args
  //!  Additional argument.
  string get_auth_uri(string auth_uri, void|mapping args)
  {
    Web.Auth.Params p;
    p = Web.Auth.Params(Web.Auth.Param("client_id",     _client_id),
                        Web.Auth.Param("response_type", _response_type));

    if (args && args->redirect_uri || _redirect_uri)
      p += Web.Auth.Param("redirect_uri",
                      args && args->redirect_uri || _redirect_uri);

    if (STATE)
      p += Web.Auth.Param("state", (string) Standards.UUID.make_version4());

    if (args && args->scope || _scope) {
      string sc = get_valid_scopes(args && args->scope || _scope);

      if (sc && sizeof(sc)) {
        _scope = sc;
        p += Web.Auth.Param("scope", sc);
      }
    }

    if (args && args->access_type || _access_type) {
      p += Web.Auth.Param("access_type", args &&
                                         args->access_type || _access_type);

      if (!_access_type && args && args->access_type)
        _access_type = args->access_type;
    }

    if (args) {
      m_delete(args, "scope");
      m_delete(args, "redirect_uri");
      p += args;
    }

    TRACE("auth_uri(%s)\n", (string) p["redirect_uri"]);

    return auth_uri + "?" + p->to_query();
  }

  //! Requests an access token.
  //!
  //! @throws
  //!  An error if the access token request fails.
  //!
  //! @param oauth_token_uri
  //!  An URI received from @[get_auth_uri()].
  //!
  //! @param code
  //!  The code returned from the authorization page via @[get_auth_uri()].
  //!
  //! @param async_cb
  //!  If given an async request will be made and this function will
  //!  be called when the request is finished. The first argument
  //!  passed to the callback will be @expr{true@} or @expr{false@}
  //!  depending on if the request was successfull or not. The second
  //!  argument will be a string. If the request failed it will be an
  //!  error message. If it succeeded it will be the result as a
  //!  string encoded with @[predef::encode_value()].
  //!
  //! @returns
  //!  If @expr{OK@} a Pike encoded mapping (i.e it's a string) is
  //!  returned which can be used to populate an @[Web.Auth.OAuth2]
  //!  object at a later time.
  //!
  //!  The mapping looks like
  //!  @mapping
  //!   @member string "access_token"
  //!   @member int    "expires"
  //!   @member int    "created"
  //!   @member string "refresh_token"
  //!   @member string "token_type"
  //!  @endmapping
  //!
  //!  Depending on the authorization service it might also contain more
  //!  members.
  string request_access_token(string oauth_token_uri, string code,
                              void|function(bool,string:void) async_cb)
  {
    TRACE("request_access_token: %O, %O\n", oauth_token_uri, code);

    Web.Auth.Params p = get_default_params();
    p += Web.Auth.Param("code", code);

    if (async_cb) {
      do_query(oauth_token_uri, p, async_cb);
      return 0;
    }
    else {
      if (string s = do_query(oauth_token_uri, p))
        return s;

      error("Failed getting access token! ");
    }
  }

  //! Refreshes the access token, if a refresh token exists in the object.
  //!
  //! @param oauth_token_uri
  //!  Endpoint of the authentication service.
  //! @param async_cb
  //!  If given an async request will be made and this function will
  //!  be called when the request is finished. The first argument
  //!  passed to the callback will be @expr{true@} or @expr{false@}
  //!  depending on if the request was successfull or not. The second
  //!  argument will be a string. If the request failed it will be an
  //!  error message. If it succeeded it will be the result as a
  //!  string encoded with @[predef::encode_value()].
  string refresh_access_token(string oauth_token_uri,
                              void|function(bool,string:void) async_cb)
  {
    TRACE("Refresh: %s @ %s\n", gettable->refresh_token, oauth_token_uri);

    if (!gettable->refresh_token)
      error("No refresh_token in object! ");

    Web.Auth.Params p = get_default_params(GRANT_TYPE_REFRESH_TOKEN);
    p += Web.Auth.Param("refresh_token", gettable->refresh_token);

    if (async_cb) {
      do_query(oauth_token_uri, p, async_cb);
      return 0;
    }
    else {
      if (string s = do_query(oauth_token_uri, p, async_cb)) {
        TRACE("Got result: %O\n", s);
        return s;
      }

      error("Failed refreshing access token! ");
    }
  }

  //! Send a request to @[oauth_token_uri] with params @[p]
  //!
  //! @param async_cb
  //!  If given an async request will be made and this function will
  //!  be called when the request is finished. The first argument
  //!  passed to the callback will be @expr{true@} or @expr{false@}
  //!  depending on if the request was successfull or not. The second
  //!  argument will be a string. If the request failed it will be an
  //!  error message. If it succeeded it will be the result as a
  //!  string encoded with @[predef::encode_value()].
  protected string do_query(string oauth_token_uri, Web.Auth.Params p,
                            void|function(bool,string:void) async_cb)
  {
    int qpos = 0;

    if ((qpos = search(oauth_token_uri, "?")) > -1) {
      //string qs = oauth_token_uri[qpos..];
      oauth_token_uri = oauth_token_uri[..qpos];
    }

    TRACE("params: %O\n", p);
    TRACE("request_access_token(%s?%s)\n", oauth_token_uri, (string) p);

    if (async_cb) {
      Protocols.HTTP.Query q = Protocols.HTTP.Query();

      if (http_request_timeout) {
        q->timeout = http_request_timeout;
      }

      q->set_callbacks(
        lambda (Protocols.HTTP.Query qq, mixed ... args) {
          if (q->status != 200) {
            string emsg = sprintf("Bad status (%d) in HTTP response! ",
                                  q->status);
            if (mapping reason = try_get_error(q->data()))
              emsg += sprintf("Reason: %O!\n", reason);

            async_cb(false, emsg);
          }
          else {
            if (decode_access_token_response(q->data()))
              async_cb(true, encode_value(gettable));
            else
              async_cb(false, "Failed decoding response!");
          }
        },

        lambda (Protocols.HTTP.Query qq, mixed ... args) {
          TRACE("Failed callback\n");
          async_cb(false, "Connection failed!");
        }
      );

      Protocols.HTTP.do_async_method("POST", oauth_token_uri, 0,
                                     request_headers, q, p->to_query());

      return 0;
    }
    else {
      Protocols.HTTP.Session sess = Protocols.HTTP.Session();
      Protocols.HTTP.Session.Request q;
      q = sess->post_url(oauth_token_uri, p->to_mapping());

      if (!q) {
        error("Unable to create request!\n");
      }

      TRACE("Query OK: %O : %O : %s\n", q, q->status(), q->data());

      string c = q && q->data();

      if (q->status() != 200) {
        string emsg = sprintf("Bad status (%d) in HTTP response! ",
                              q->status());

        if (mapping reason = try_get_error(c))
          emsg += sprintf("Reason: %O!\n", reason);

        error(emsg);
      }

      TRACE("Got data: %O\n", c);

      if (decode_access_token_response(c))
        return encode_value(gettable);
    }
  }

  //! Returns a set of default parameters.
  protected Web.Auth.Params get_default_params(void|string grant_type)
  {
    Web.Auth.Params p;
    p = Web.Auth.Params(Web.Auth.Param("client_id",     _client_id),
                    Web.Auth.Param("redirect_uri",  _redirect_uri),
                    Web.Auth.Param("client_secret", _client_secret),
                    Web.Auth.Param("grant_type",    grant_type || _grant_type));
    if (STATE) {
      p += Web.Auth.Param("state", (string)Standards.UUID.make_version4());
    }

    return p;
  }

  //! Checks if the authorization is renewable. This is true if the
  //! @[Web.Auth.OAuth2.Base()] object has been populated from
  //! @[Web.Auth.OAuth2.Base()->set_from_cookie()], i.e the user has been
  //! authorized but the session has expired.
  int(0..1) is_renewable()
  {
    return !!gettable->refresh_token;
  }

  //! Checks if this authorization has expired.
  int(0..1) is_expired()
  {
    // This means no expiration date was set from the API
    if (gettable->created && !gettable->expires)
      return 0;

    return gettable->expires ? time() > gettable->expires : 1;
  }

  //! Do we have a valid authentication.
  int(0..1) is_authenticated()
  {
    return !!gettable->access_token && !is_expired();
  }

  //! If casted to @tt{string@} the @tt{access_token@} will be
  //! returned. If casted to @tt{int@} the @tt{expires@} timestamp
  //! will be returned.
  protected mixed cast(string how)
  {
    switch (how) {
      case "string":  return gettable->access_token;
      case "int":     return gettable->expires;
      case "mapping": return gettable;
    }

    error("Can't cast %O to %s! ", object_program(this), how);
  }

  protected string _sprintf(int t)
  {
    switch (t) {
      case 's': return gettable->access_token;
    }

    return sprintf("%O(%O, %O, %O, %O)",
                   object_program(this), gettable->access_token,
                   _redirect_uri,
                   gettable->created &&
                     Calendar.Second("unix", gettable->created),
                   gettable->expires &&
                     Calendar.Second("unix", gettable->expires));
  }

  /*
    Internal API
    The internal API can be used by other classes inheriting this class
  */

  //! A mapping of valid scopes for the API.
  protected multiset(string) valid_scopes = (<>);

  //! Version of this implementation.
  protected constant VERSION = "1.0";

  //! User agent string.
  protected constant USER_AGENT  = "Mozilla 4.0 (Pike OAuth2 Client " +
                                   VERSION + ")";

  //! Some OAuth2 verifiers need the STATE parameter. If this is not
  //! @tt{0@} a random string will be generated and the @tt{state@}
  //! parameter will be added to the request.
  protected constant STATE = 0;

  //! The application ID.
  protected string _client_id;

  //! The application secret.
  protected string _client_secret;

  //! Where the authorization page should redirect to.
  protected string _redirect_uri;

  //! The scope of the authorization. Limits the access.
  protected string|array(string)|multiset(string) _scope;

  //! Access type of the request.
  protected string _access_type;

  //! @ul
  //!  @item
  //!   @[GRANT_TYPE_AUTHORIZATION_CODE] for apps running on a web server
  //!  @item
  //!   @[GRANT_TYPE_IMPLICIT] for browser-based or mobile apps
  //!  @item
  //!   @[GRANT_TYPE_PASSWORD] for logging in with a username and password
  //!  @item
  //!   @[GRANT_TYPE_CLIENT_CREDENTIALS] for application access
  //! @endul
  protected string _grant_type = GRANT_TYPE_AUTHORIZATION_CODE;

  //! @ul
  //!  @item
  //!   @[RESPONSE_TYPE_CODE] for apps running on a webserver
  //!  @item
  //!   @[RESPONSE_TYPE_TOKEN] for apps browser-based or mobile apps
  //! @endul
  protected string _response_type = RESPONSE_TYPE_CODE;

  //! Default request headers.
  protected mapping request_headers = ([
    "User-Agent"   : USER_AGENT,
    "Content-Type" : "application/x-www-form-urlencoded"
  ]);

  protected constant json_decode = Standards.JSON.decode;
  protected constant Params      = Web.Auth.Params;
  protected constant Param       = Web.Auth.Param;

  protected mapping gettable = ([ "access_token"  : 0,
                                  "refresh_token" : 0,
                                  "expires"       : 0,
                                  "created"       : 0,
                                  "token_type"    : 0 ]);

  //! Returns a space separated list of all valid scopes in @[s].
  //! @[s] can be a comma or space separated string or an array or
  //! multiset of strings. Each element in @[s] will be matched
  //! against the valid scopes set in the module inheriting this
  //! class.
  protected string get_valid_scopes(string|array(string)|multiset(string) s)
  {
    if (!s) return "";

    array r = ({});

    if (stringp(s))
      s = map(s/",", String.trim);

    if (multisetp(s))
      s = (array) s;

    if (!sizeof(valid_scopes))
      r = s;

    foreach (s, string x) {
      if (valid_scopes[x])
        r += ({ x });
    }

    return r*" ";
  }

  //! Decode the response from an authentication call. If the response
  //! was ok the internal mapping @[gettable] will be populated with
  //! the members/variables in @[r].
  //!
  //! @param r
  //!  The response from @[do_query()]
  protected int(0..1) decode_access_token_response(string r)
  {
    if (!r) return 0;

    TRACE("Decode response: %s\n", r);

    mapping v = ([]);

    if (has_prefix(r, "access_token")) {
      foreach (r/"&", string s) {
        sscanf(s, "%s=%s", string key, string val);
        v[key] = val;
      }
    }
    else {
      if (catch(v = json_decode(r)))
        return 0;
    }

    if (!v->access_token)
      return 0;

    gettable->scope = _scope;
    gettable->created = time();

    foreach (v; string key; string val) {
      if (search(key, "expires") > -1)
        gettable->expires = gettable->created + (int)val;
      else
        gettable[key] = val;
    }

    if (_access_type) {
      gettable->access_type = _access_type;
    }

    return 1;
  }

  //! Try to get an error message from @[data]. Only successful if
  //! @[data] is a JSON string and contains the key @tt{error@}.
  private mixed try_get_error(string data)
  {
    catch {
      mixed x = json_decode(data);
      return x->error;
    };
  }

  #if 0
  // Parses a signed request.
  //
  // @throws
  //  An error if the signature doesn't match the expected signature.
  protected mapping parse_signed_request(string sign)
  {
    if( sscanf(sign, "%s.%s", string sig, string payload)!=2 )
      error("Illegal signature format.\n");

    sig = MIME.decode_base64url(sig);
    mapping data = json_decode(MIME.decode_base64url(payload));

    if (upper_case(data->algorithm) != "HMAC-SHA256")
      error("Unknown algorithm. Expected HMAC-SHA256");

    // FIXME: Shouldn't payload and _client_secret be swapped?
    string expected_sig = Crypto.SHA256.HMAC(payload)(_client_secret);

    if (sig != expected_sig)
      error("Badly signed signature. ");

    return data;
  }
  #endif
}

//! This class is intended to be extended by classes implementing
//! different OAuth2 services.
class Client
{
  inherit Base;

  //! Authorization URI.
  constant OAUTH_AUTH_URI = 0;

  //! Request access token URI.
  constant OAUTH_TOKEN_URI = 0;

  //! Scope to set if none is set.
  protected constant DEFAULT_SCOPE = 0;

  protected void create(string client_id,
                        string client_secret,
                        void|string redirect_uri,
                        void|string|array(string)|multiset(string) scope)
  {
    ::create(client_id, client_secret, redirect_uri, scope);
  }

  //! Make a JWT (JSON Web Token) authentication.
  string get_token_from_jwt(string jwt, void|string sub,
                            void|function(bool,string:void) async_cb)
  {
    return ::get_token_from_jwt(jwt, OAUTH_TOKEN_URI, sub, async_cb);
  }

  //! Returns an authorization URI.
  //!
  //! @param args
  //!  Additional argument.
  string get_auth_uri(void|mapping args)
  {
    if ((args && !args->scope || !args) && DEFAULT_SCOPE) {
      if (!args) args = ([]);
      args->scope = DEFAULT_SCOPE;
    }

    return ::get_auth_uri(OAUTH_AUTH_URI, args);
  }

  //! Requests an access token.
  //!
  //! @throws
  //!  An error if the access token request fails.
  //!
  //! @param code
  //!  The code returned from the authorization page via @[get_auth_uri()].
  //!
  //! @param async_cb
  //!  If given an async request will be made and this function will
  //!  be called when the request is finished. The first argument
  //!  passed to the callback will be @expr{true@} or @expr{false@}
  //!  depending on if the request was successfull or not. The second
  //!  argument will be a string. If the request failed it will be an
  //!  error message. If it succeeded it will be the result as a
  //!  string encoded with @[predef::encode_value()].
  //!
  //! @returns
  //!  If @expr{OK@} a Pike encoded mapping (i.e it's a string) is
  //!  returned which can be used to populate an @[Base] object at a
  //!  later time.
  //!
  //!  The mapping looks like
  //!  @mapping
  //!   @member string "access_token"
  //!   @member int    "expires"
  //!   @member int    "created"
  //!   @member string "refresh_token"
  //!   @member string "token_type"
  //!  @endmapping
  //!
  //!  Depending on the authorization service it might also contain
  //!  more members.
  string request_access_token(string code,
                              void|function(bool,string:void) async_cb)
  {
    return ::request_access_token(OAUTH_TOKEN_URI, code, async_cb);
  }

  //! Refreshes the access token, if a refresh token exists in the
  //! object.
  //!
  //! @param async_cb
  //!  If given an async request will be made and this function will
  //!  be called when the request is finished. The first argument
  //!  passed to the callback will be @expr{true@} or @expr{false@}
  //!  depending on if the request was successfull or not. The second
  //!  argument will be a string. If the request failed it will be an
  //!  error message. If it succeeded it will be the result as a
  //!  string encoded with @[predef::encode_value()].
  string refresh_access_token(void|function(bool,string:void) async_cb)
  {
    return ::refresh_access_token(OAUTH_TOKEN_URI, async_cb);
  }
}
