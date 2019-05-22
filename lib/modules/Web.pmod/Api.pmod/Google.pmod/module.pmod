#pike __REAL_VERSION__

private typedef Web.Api.Api.Callback Callback;

private mapping(string:mapping) discoveries = ([]);
private Regexp substitutes = Regexp("{[^}]*}");

private mixed mkargs(function(string, void|mapping,
                     void|Callback:mixed) get,
                     string path, array(string) pathparams,
                     void|mapping params, mixed ... rest)
{
  if (!params)
    params = ([]);
  return get(
   sprintf(path, @map(pathparams, lambda(mixed i) { return params[i]; })),
   params - pathparams, @rest);
}

//! Internal class meant to be inherited by other Google API's
class Api
{
  inherit Web.Api.Api : parent;

  // Just a convenience class
  protected class Method
  {
    inherit Web.Api.Api.Method;

    //! Internal convenience method
    protected mixed _get(string s, void|ParamsArg p, void|Callback cb)
    {
      return parent::get(get_uri(METHOD_PATH + s), p, cb);
    }

    //! Internal convenience method
    protected mixed _post(string s, void|ParamsArg p, void|Callback cb)
    {
      return parent::post(get_uri(METHOD_PATH + s), p, 0, cb);
    }

    //! Internal convenience method
    protected mixed _delete(string s, void|ParamsArg p, void|Callback cb)
    {
      return parent::delete(get_uri(METHOD_PATH + s), p, cb);
    }
  }
}

class Discovery
{
  //! Instantiates the default Google Discovery Service API.
  //! See @[Web.Api.Api()] for further information.

  inherit Api : parent;

  //! API base URI.
  protected constant API_URI = "https://www.googleapis.com/discovery/v1";

  //! Getter for the @[Apis] API
  Apis `apis()
  {
    return _apis || (_apis = Apis());
  }

  //! Interface to the Google Discovery Service API
  class Apis
  {
    inherit Method;

    protected constant METHOD_PATH = "/apis";

    //! Retrieve the description of a particular version of an API
    //!
    //! @param params
    //!
    //! @param cb
    mixed getRest(mapping params, void|Callback cb)
    {
      return mkargs(_get, "/%s/%s/rest", ({"api", "version"}), params, cb);
    }

    //! Retrieve the list of APIs supported at this endpoint
    //!
    //! @param params
    //!
    //! @param cb
    mixed list(void|mapping params, void|Callback cb)
    {
      return _get("", params, cb);
    }
  }

  //! Internal singleton objects. Retrieve an apis via @[apis].
  protected Apis _apis;
}

//!
class Base
{
  inherit Api : parent;

  constant AuthClass = Web.Auth.Google.Authorization;

  protected mapping dc;

  class chain
  {
    mapping _methods;

    protected void create(mapping methods)
    {
      _methods = methods;
    }

    protected mixed `[](string method)
    {
      mapping m = _methods[method];
      if (m->_func)
        return m->_func;
      m -= ({"description", "id", "response", "scopes"});
      string path = substitutes->replace(m->path, "%s");
      return m->_func
        = lambda(void|mapping params, void|string|Callback cb) {
          string|mapping data = params - m->parameters;
          data = sizeof(data) ? Standards.JSON.encode(data) : "";
          if (cb != "promise")
            return mkargs(parent::call, dc->baseUrl + path, m->parameterOrder,
              m->parameters & params, m->httpMethod, data, cb);
          Concurrent.Promise p = Concurrent.Promise();
          mixed err = catch(
           mkargs(parent::call, dc->baseUrl + path, m->parameterOrder,
            m->parameters & params, m->httpMethod, data,
            lambda(mixed ret) { p->success(ret); }));
          if (err)
            p->failure(err);
          return p->future();
      };
    }

    protected function `-> = `[];

    protected array(string) _indices()
    {
      return indices(_methods);
    }
  }

  class resource
  {
    mixed `[](string resrc)
    {
      mapping tm = dc->resources[resrc];
      return tm->_chain || (tm->_chain = chain(tm->methods));
    }

    function `-> = `[];

    protected array(string) _indices()
    {
      return indices(dc->resources);
    }
  }

  //!
  protected void create(mapping discovered, mixed ... rest)
  {
    dc = discovered;
    ::create(@rest);
    _auth->valid_scopes = (multiset)valid_scopes;
    // Set default scope
    set_scope(dc->name);
    default_headers["content-type"] = "application/json";
  }

  //!
  void set_scope(string scope)
  {
    mapping scopes = dc->auth->oauth2->scopes;
    if (!scopes[scope]) {
      scope = "/" + scope;
      foreach (scopes; string i; )
        if (has_suffix(i, scope)) {
          scope = i;
          break;
        }
    }
    auth->set_scope(scope);
  }

  //!
  array(string) `valid_scopes()
  {
    return sort(indices(dc->auth->oauth2->scopes));
  }

  //!
  resource `resrc() {
    return resource();
  }
}

//! Get a Google API object
//!
//! @example
//!   This example shows how you can use the Google API
//!   to find out all the compute zones that exist at Google Compute Engine.
//! @code
//! #define SECRET_FILE     "googleserviceaccount.json"
//! #define PROJECT         "somegoogleproject"
//! #define TOKENTIME       3600
//!
//! string jwt_secret = Stdio.File(SECRET_FILE, "r").read();
//! Web.Api.Google.Base api = Web.Api.Google.discover("compute", "v1");
//!
//! void authenticated() {
//!   if (!api->auth->is_authenticated()
//!    || api->auth->expires->unix_time() - time(1) < TOKENTIME / 2)
//!     api->auth->get_token_from_jwt(jwt_secret);
//! };
//!
//! authenticated();
//! mixed allzones = api->resrc->zones->list( ([ "project":PROJECT ]) );
//! @endcode
Base discover(string api, string version,
   void|string client_id, void|string client_secret,
   void|string redirect_uri,
   void|string|array(string)|multiset(string) scope)
{
  string id = api + ":" + version;
  if (!discoveries[id]) {
    discoveries[id]
     = Discovery()->apis->getRest((["api":api, "version":version]));
    discoveries[id] -= ({"parameters", "icons"});
  }
  return Base(discoveries[id], client_id, client_secret, redirect_uri, scope);
}
