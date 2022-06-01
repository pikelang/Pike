#pike __REAL_VERSION__

//! OAuth module
//!
//! @b{Example@}
//!
//! @code
//!  import Web.Auth.OAuth;
//!
//!  string endpoint = "http://twitter.com/users/show.xml";
//!
//!  Consumer consumer = Consumer(my_consumer_key, my_consumer_secret);
//!  Token    token    = Token(my_access_token_key, my_access_token_secret);
//!  Params   params   = Params(Param("user_id", 12345));
//!  Request  request  = request(consumer, token, params);
//!
//!  request->sign_request(Signature.HMAC_SHA1, consumer, token);
//!  Protocols.HTTP.Query query = request->submit();
//!
//!  if (query->status != 200)
//!    error("Bad response status: %d\n", query->status);
//!
//!  werror("Data is: %s\n", query->data());
//! @endcode

//! Verion
constant VERSION = "1.0";

//! Query string variable name for the consumer key.
constant CONSUMER_KEY_KEY = "oauth_consumer_key";

//! Query string variable name for a callback URL.
constant CALLBACK_KEY = "oauth_callback";

//! Query string variable name for the version.
constant VERSION_KEY = "oauth_version";

//! Query string variable name for the signature method.
constant SIGNATURE_METHOD_KEY = "oauth_signature_method";

//! Query string variable name for the signature.
constant SIGNATURE_KEY = "oauth_signature";

//! Query string variable name for the timestamp.
constant TIMESTAMP_KEY = "oauth_timestamp";

//! Query string variable name for the nonce.
constant NONCE_KEY = "oauth_nonce";

//! Query string variable name for the token key.
constant TOKEN_KEY = "oauth_token";

//! Query string variable name for the token secret.
constant TOKEN_SECRET_KEY = "oauth_token_secret";

#include "oauth.h"

//! Helper method to create a @[Request] object.
//!
//! @throws
//!  An error if @[consumer] is null.
//!
//! @param http_method
//!  Defaults to GET.
Request request(string|Standards.URI uri, Consumer consumer, Token token,
                void|Params params, void|string http_method)
{
  if (!consumer)
    ARG_ERROR("consumer", "Can not be NULL.");

  TRACE("### Token: %O\n", token);

  Params dparams = get_default_params(consumer, token);

  if (params) dparams += params;

  return Request(uri, http_method||"GET", dparams);
}

//! Returns the default params for authentication/signing.
Params get_default_params(Consumer consumer, Token token)
{
  Params p = Params(
    Param(VERSION_KEY, VERSION),
    Param(NONCE_KEY, nonce()),
    Param(TIMESTAMP_KEY, time(1)),
    Param(CONSUMER_KEY_KEY, consumer->key)
  );

  if (token && token->key)
    p += Param(TOKEN_KEY, token->key);

  return p;
}


//! Converts a query string, or a mapping, into a @[Params] object.
Params query_to_params(string|Standards.URI|mapping q)
{
  if (objectp(q))
    q = (string)q;

  Params ret = Params();

  if (!q || !sizeof(q))
    return ret;

  if (mappingp(q)) {
    foreach(q; string n; string v)
      ret += Param(n, v);

    return ret;
  }

  int pos = 0, len = sizeof(q);
  if ((pos = search(q, "?")) > -1)
    q = ([string]q)[pos+1..];

  foreach (q/"&", string p) {
    sscanf(p, "%s=%s", string n, string v);
    if (n && v)
      ret += Param(n, v);
  }

  return ret;
}

//! Class for building a signed request and querying the remote
//! service.
class Request
{
  //! The remote endpoint.
  protected Standards.URI uri;

  //! The signature basestring.
  protected string base_string;

  //! String representation of the HTTP method.
  protected string method;

  //! The parameters to send.
  protected Params params;

  //! Creates a new @[Request.]
  //!
  //! @seealso
  //!  @[request()]
  //!
  //! @param uri
  //!  The uri to request.
  //! @param http_method
  //!  The HTTP method to use. Either @expr{"GET"@} or @expr{"POST"@}.
  protected void create(string|Standards.URI uri, string http_method,
                        void|Params params)
  {
    this::uri = ASSURE_URI(uri);
    method = upper_case(http_method);
    this::params = query_to_params(uri);

    if (params) add_params(params);

    if (!(< "GET", "POST" >)[method])
      ARG_ERROR("http_method", "Must be one of \"GET\" or \"POST\".");
  }

  //! Add a param.
  //!
  //! @returns
  //!  The object being called.
  this_program add_param(Param|string name, void|string value)
  {
    if (objectp(name))
      params += name;
    else
      params += Param(name, value);

    return this;
  }

  //! Add a @[Params] object.
  void add_params(Params params)
  {
    this::params += params;
  }

  //! Get param with name @[name].
  object(Param)|zero get_param(string name)
  {
    foreach (values(params), Param p)
      if (p[name])
        return p;

    return 0;
  }

  //! Returns the @[Params] collection.
  Params get_params()
  {
    return params;
  }

  //! Signs the request.
  //!
  //! @param signature_type
  //!  One of the types in @[Signature].
  void sign_request(int signature_type, Consumer consumer, Token token)
  {
    object sig = .Signature.get_object(signature_type);
    params += Param(SIGNATURE_METHOD_KEY, sig->get_method());
    params += Param(SIGNATURE_KEY, sig->build_signature(this, consumer, token));
  }

  //! Generates a signature base.
  string get_signature_base()
  {
    TRACE("+++ get_signature_base(%s, %s, %s)\n\n",
          method, (normalize_uri(uri)), (params->get_signature()));

    return ({
      method,
      Protocols.HTTP.uri_encode(normalize_uri(uri)),
      Protocols.HTTP.uri_encode(params->get_signature())
    }) * "&";
  }

  //! Send the request to the remote endpoint.
  Protocols.HTTP.Query submit(void|mapping extra_headers)
  {
    mapping args = params->get_variables();
    foreach (args; string k; string v) {
      if (!v) continue;
      if (String.width(v) == 8)
        catch(args[k] = utf8_to_string(v));
    }

    if (!extra_headers)
      extra_headers = ([]);

    string realm = uri->scheme + "://" + uri->host;
    extra_headers["Authorization"] = "OAuth realm=\"" + realm + "\"," +
                                     params->get_auth_header();
    extra_headers["Content-Type"] = "text/plain; charset=utf-8";

    TRACE("submit(%O, %O, %O, %O)\n", method, uri, args, extra_headers);

    return Protocols.HTTP.do_method(method, uri, args, extra_headers);
  }

  //! It is possible to case the Request object to a string, which
  //! will be the request URL.
  protected string cast(string how)
  {
    if (how != "string") {
      ARG_ERROR("how", "%O can not be casted to \"%s\", only to \"string\"\n",
                this, how);
    }

    return (method == "GET" ? normalize_uri(uri) + "?" : "")+(string)params;
  }

  protected string _sprintf(int t)
  {
    return t == 'O' && sprintf("%O(%O, %O, %O)", object_program(this),
                               (string)uri, base_string, params);
  }
}

//! An OAuth user
class Consumer
{
  //! Consumer key
  string key;

  //! Consumer secret
  string secret;

  //! Callback url that the remote verifying page will return to.
  string|Standards.URI callback;

  //! Creates a new @[Consumer] object.
  //!
  //! @param callback
  //!  NOTE: Has no effect in this implementation.
  protected void create(string key, string secret, void|string|Standards.URI callback)
  {
    this::key      = key;
    this::secret   = secret;
    this::callback = ASSURE_URI(callback);
  }

  protected string _sprintf(int t)
  {
    return t == 'O' && sprintf("%O(%O, %O, %O)", object_program(this),
                               key, secret, callback);
  }
}

//! Token class.
class Token (string key, string secret)
{
  //! Only supports casting to string wich will return a query string
  //! of the object.
  protected string cast(string how)
  {
    switch (how) {
      case "string":
        return "oauth_token=" + key + "&"
               "oauth_token_secret=" + secret;
    }

    error("Can't cast %O() to %O\n", object_program(this), how);
  }

  protected string _sprintf(int t)
  {
    return t == 'O' && sprintf("%O(%O, %O)", object_program(this),
                               key, secret);
  }
}

//! Represents a query string parameter, i.e. @tt{key=value@}.
class Param
{
  //! Param name
  protected string name;

  //! Param value
  protected string value;

  protected int(0..1) is_null = 1;

  //! Creates a new @[Param].
  protected void create(string name, mixed value)
  {
    this::name = name;
    set_value(value);
  }

  //! Getter for the name attribute.
  string get_name() { return name; }

  //! Setter for the value attribute.
  void set_name(string value) { name = value; }

  //! Getter for the value attribute.
  string get_value() { return value; }

  //! Setter for the value attribute.
  void set_value(mixed _value)
  {
    value = (string)_value;
    is_null = !_value;
  }

  //! Returns the value encoded.
  string get_encoded_value()
  {
    return value && Protocols.HTTP.uri_encode(value);
  }

  //! Returns the name and value for usage in a signature string.
  string get_signature()
  {
    return name && value && Protocols.HTTP.uri_encode(name) + "=" +
      Protocols.HTTP.uri_encode(value);
  }

  //! Comparer method. Checks if @[other] equals this object.
  protected int(0..1) `==(mixed other)
  {
    if (object_program(other) != Param) return 0;
    if (name == other->get_name())
      return value == other->get_value();

    return 0;
  }

  //! Checks if this object is greater than @[other].
  protected int(0..1) `>(mixed other)
  {
    if (object_program(other) != Param) return 0;
    if (name == other->get_name())
      return value > other->get_value();

    return name > other->get_name();
  }

  //! Index lookup.
  protected object|zero `[](string key)
  {
    if (key == name)
      return this;

    return 0;
  }

  protected string _sprintf(int t)
  {
    return t == 'O' && sprintf("%O(%O, %O)", object_program(this), name, value);
  }
}


//! Collection of @[Param]
class Params
{
  //! Storage for @[Param]s of this object.
  private array(Param) params;

  //! Create a new @[Params]
  //!
  //! @param params
  //!  Arbitrary number of @[Param] objects.
  protected void create(Param ... params)
  {
    this::params = params||({});
  }

  //! Returns the params for usage in an authentication header.
  string get_auth_header()
  {
    array a = ({});
    foreach (params, Param p) {
      if (has_prefix(p->get_name(), "oauth_")) {
        if (p->get_value())
          a += ({ p->get_name() + "=\"" + p->get_encoded_value() + "\"" });
      }
      else {
        TRACE("**** Some shitty param: %O\n", p);
      }
    }

    return a*",";
  }

  //! Returns the parameters as a mapping.
  mapping get_variables()
  {
    mapping m = ([]);

    foreach (params, Param p) {
      if (!has_prefix(p->get_name(), "oauth_"))
        m[p->get_name()] = p->get_value();
    }

    return m;
  }

  //! Returns the parameters as a query string.
  string get_query_string()
  {
    array s = ({});
    foreach (params, Param p)
      if (!has_prefix(p->get_name(), "oauth_"))
        s += ({ p->get_name() + "=" + Protocols.HTTP.uri_encode(p->get_value()) });

    return s*"&";
  }

  //! Returns the parameters as a mapping with encoded values.
  //!
  //! @seealso
  //!  @[get_variables()]
  mapping get_encoded_variables()
  {
    mapping m = ([]);

    foreach (params, Param p)
      if (!has_prefix(p->get_name(), "oauth_"))
        m[p->get_name()] = Protocols.HTTP.uri_encode(p->get_value());

    return m;
  }

  //! Returns the parameters for usage in a signature base string.
  string get_signature()
  {
    return ((sort(params)->get_signature()) - ({ 0 })) * "&";
  }

  //! Supports casting to @tt{mapping@}, which will map parameter
  //! names to their values.
  protected mapping cast(string how)
  {
    switch (how)
    {
      case "mapping":
        mapping m = ([]);
        foreach (params, Param p)
          m[p->get_name()] = p->get_value();

        return m;
        break;
    }
  }

  //! Append mapping @[args] as @[Param] objects.
  //!
  //! @returns
  //!  The object being called.
  this_program add_mapping(mapping args)
  {
    foreach (args; string k; string v)
      params += ({ Param(k, v) });

    return this_object;
  }

  //! Append @[p] to the internal array.
  //!
  //! @returns
  //!  The object being called.
  protected this_program `+(Param|Params p)
  {
    params += object_program(p) == Params ? values(p) : ({ p });
    return this_object();
  }

  //! Removes @[p] from the internal array.
  //!
  //! @returns
  //!  The object being called.
  protected this_program `-(Param p)
  {
    foreach (params, Param pm) {
      if (pm == p) {
        params -= ({ pm });
        break;
      }
    }

    return this_object();
  }

  //! Index lookup
  //!
  //! @returns
  //!  If no @[Param] is found returns @tt{0@}.
  //!  If multiple @[Param]s with name @[key] is found a new @[Params] object
  //!  with the found params will be retured.
  //!  If only one @[Param] is found that param will be returned.
  protected mixed `[](string key)
  {
    array(Param) p = params[key]-({0});
    if (!p) return 0;
    return sizeof(p) == 1 ? p[0] : Params(@p);
  }

  //! Returns the @[params].
  protected mixed _values()
  {
    sort(params);
    return params;
  }

  //! Returns the size of the @[params] array.
  protected int(0..) _sizeof()
  {
    return sizeof(params);
  }

  protected string _sprintf(int t)
  {
    return t == 'O' && sprintf("%O(%O)", object_program(this), params);
  }
}

//! Normalizes @[uri]
//!
//! @param uri
//!  A @tt{string@} or @[Standards.URI].
string normalize_uri(string|Standards.URI uri)
{
  uri = ASSURE_URI(uri);
  string nuri = sprintf("%s://%s", uri->scheme, uri->host);

  if (!(<"http","https">)[uri->scheme] || !(<80,443>)[uri->port])
    nuri += ":" + uri->port;

  return nuri + uri->path;
}

//! Generates a @tt{nonce@}.
string nonce()
{
  return MIME.encode_base64(random_string(15),1);
}

