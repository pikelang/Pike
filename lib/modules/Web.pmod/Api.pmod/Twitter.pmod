#pike __REAL_VERSION__

//! Instantiates the default Twitter API.
//! See @[Web.Api.Api()] for further information.
//!
//! @param client_id
//!  Your application key/id
//! @param client_secret
//!  Your application secret
//! @param redirect_uri
//!  The redirect URI after an authentication
//! @param scope
//!  The application scopes to grant access to
protected this_program `()(string client_id, string client_secret,
                           void|string redirect_uri,
                           void|string|array(string)|multiset(string) scope)
{
  return V1_1(client_id, client_secret, redirect_uri, scope);
}

class V1_1
{
  inherit Web.Api.Api : parent;

  constant API_URI = "https://api.twitter.com/1.1/";

  protected constant AuthClass = Web.Auth.Twitter;

  //! Getter for the @[Any] object which is a generic object for making request
  //! to the Twitter API
  //!
  //! @seealso
  //!  @[Any]
  Any `any()
  {
    return _any || (_any = Any());
  }

  //! Make a generic @expr{GET@} request to the Twitter API.
  //!
  //! @param path
  //!  What to request. Like @expr{me@}, @expr{me/pictures@},
  //!  @expr{[some_id]/something@}.
  //!
  //! @param params
  //! @param cb
  //!  Callback for async requests
  mapping get(string path, void|ParamsArg params, void|Callback cb)
  {
    return `any()->get(path, params, cb);
  }

  //! Make a generic @expr{PUT@} request to the Twitter API.
  //!
  //! @param path
  //!  What to request. Like @expr{me@}, @expr{me/pictures@},
  //!  @expr{[some_id]/something@}.
  //!
  //! @param params
  //! @param cb
  //!  Callback for async requests
  mapping put(string path, void|ParamsArg params, void|Callback cb)
  {
    return `any()->put(path, params, cb);
  }

  //! Make a generic @expr{POST@} request to the Twitter API.
  //!
  //! @param path
  //!  What to request. Like @expr{me@}, @expr{me/pictures@},
  //!  @expr{[some_id]/something@}.
  //!
  //! @param params
  //! @param data
  //! @param cb
  //!  Callback for async requests
  mapping post(string path, void|ParamsArg params, void|string data,
               void|Callback cb)
  {
    return `any()->post(path, params, data, cb);
  }

  //! Make a generic @expr{DELETE@} request to the Twitter API.
  //!
  //! @param path
  //!  What to request. Like @expr{me@}, @expr{me/pictures@},
  //!  @expr{[some_id]/something@}.
  //!
  //! @param params
  //! @param cb
  //!  Callback for async requests
  mapping delete(string path, void|ParamsArg params, void|Callback cb)
  {
    return `any()->delete(path, params, cb);
  }

  //! Default parameters that goes with every call
  protected mapping default_params()
  {
    return ([ "format" : "json" ]);
  }

  // Just a convenience class
  protected class Method
  {
    inherit Web.Api.Api.Method;

    //! Internal convenience method
    public mixed get(string s, void|ParamsArg p, void|Callback cb)
    {
      return parent::get(get_uri(METHOD_PATH + s), p, cb);
    }

    //! Internal convenience method
    public mixed put(string s, void|ParamsArg p, void|Callback cb)
    {
      return parent::put(get_uri(METHOD_PATH + s), p, cb);
    }

    //! Internal convenience method
    public mixed post(string s, void|ParamsArg p, void|string data,
                      void|Callback cb)
    {
      return parent::post(get_uri(METHOD_PATH + s), p, data, cb);
    }

    //! Internal convenience method
    public mixed delete(string s, void|ParamsArg p, void|Callback cb)
    {
      return parent::delete(get_uri(METHOD_PATH + s), p, cb);
    }
  }

  //! A generic wrapper around @[Method]
  protected class Any
  {
    inherit Method;
    protected constant METHOD_PATH = "/";
  }

  mixed call(string api_method, void|ParamsArg params,
             void|string http_method, void|string data, void|Callback cb)

  {
    if (!has_suffix(api_method, ".json"))
      api_method += ".json";

    string res = auth->call(api_method, params, http_method);

    mixed result = Standards.JSON.decode(res);

    return result;
  }

  private Any _any;
}
