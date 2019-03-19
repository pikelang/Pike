#pike __REAL_VERSION__

//! Base class for implementing a @tt{(RESTful) WebApi@} like Facebook's
//! Graph API, Instagram's API, Twitter's API and so on.
//!
//! @b{Note:@} This class is useless in it self, and is intended to be
//! inherited by classes implementing a given @tt{Web.Api@}.
//!
//! Look at the code in @[Web.Api.Github], @[Web.Api.Instagram],
//! @[Web.Api.Linkedin] etc to see some examples of implementations.

#if defined(SOCIAL_REQUEST_DEBUG) || defined(SOCIAL_REQUEST_DATA_DEBUG)
# define TRACE(X...) werror("%s:%d: %s",basename(__FILE__),__LINE__,sprintf(X))
#else
# define TRACE(X...) 0
#endif

//! The URI to the remote API
constant API_URI = 0;

//! In some API's (LinkedIn f ex) this is named something else so it needs
//! to be overridden i cases where it has a different name than the
//! standard one
protected constant ACCESS_TOKEN_PARAM_NAME = "access_token";

//! If @expr{1@} @[Standards.JSON.decode_utf8()] will be used when JSON data
//! is decoded.
protected constant DECODE_UTF8 = 0;

//! If @expr{1@} @[Standards.JSON.decode_utf8()] will be used when JSON data
//! is decoded.
public int(0..1) utf8_decode = DECODE_UTF8;

//! Typedef for the async callback method signature.
typedef function(mapping,Protocols.HTTP.Query:void) Callback;

//! Typef for a parameter argument
typedef mapping|Web.Auth.Params ParamsArg;

//! Authorization object.
//!
//! @seealso
//!  @[Web.Auth.OAuth2]
protected Web.Auth.OAuth2.Client _auth;

//! Authentication class to use
protected constant AuthClass = Web.Auth.OAuth2.Client;

protected mapping(string:string) default_headers = ([
  "user-agent" : .USER_AGENT
]);

//! Creates a new Api instance
//!
//! @param client_id
//!  The application ID
//!
//! @param client_secret
//!  The application secret
//!
//! @param redirect_uri
//!  Where the authorization page should redirect back to. This must be
//!  fully qualified domain name.
//!
//! @param scope
//!  Extended permissions to use for this authentication.
protected void create(string client_id, string client_secret,
                      void|string redirect_uri,
                      void|string|array(string)|multiset(string) scope)
{
  if (AuthClass)
    _auth = AuthClass(client_id, client_secret, redirect_uri, scope);
}

//! Getter for the authentication object. Most likely this will be a class
//! derived from @[Web.Auth.OAuth2.Client].
//!
//! @seealso
//!  @[Web.Auth.OAuth2.Client] or @[Web.Auth.OWeb.Auth.Client]
Web.Auth.OAuth2.Client `auth()
{
  return _auth;
}

//! This can be used to parse a link resource returned from a REST api.
//! Many API returns stuff like:
//!
//! @code
//! {
//!   ...
//!   "pagination" : {
//!     "next" : "/api/v1/method/path?some=variables&page=2&per_page=20"
//!   }
//! }
//! @endcode
//!
//! If @tt{pagination->next@} is passed to this method it will return a path
//! of @tt{/method/path@}, given  that the base URI of the web api is
//! something along the line of @tt{https://some.url/api/v1@}, and a
//! mapping containing the query variables (which can be passed as a parameter
//! to any of the @tt{get, post, delete, put@} methods.
//!
//! @param url
//!
//! @returns
//!  @mapping
//!   @member string "path"
//!   @member mapping "params"
//!  @endmapping
mapping(string:string|mapping) parse_canonical_url(string url)
{
  Standards.URI api_uri = Standards.URI(API_URI);
  Standards.URI this_url;

  // The links returned from a REST api doesn't always contain the
  // domain name. If not we catch it and set localhost as domain since
  // we only want the path and variables
  if (catch(this_url = Standards.URI(url))) {
    this_url = Standards.URI("http://localhost" + url);
  }

  mapping params = this_url->get_query_variables() || ([]);

  foreach (indices(params), string k) {
    if (!sizeof(params[k]))
      m_delete(params, k);
  }

  string path = this_url->path;

  if (has_prefix(path, api_uri->path))
    path -= api_uri->path;

  return ([ "path" : path, "params" : params ]);
}

//! Invokes a call with a GET method
//!
//! @param api_method
//!  The remote API method to call
//! @param params
//! @param cb
//!  Callback function to get into in async mode
mixed get(string api_method, void|ParamsArg params, void|Callback cb)
{
  return call(api_method, params, "GET", 0, cb);
}

//! Invokes a call with a POST method
//!
//! @param api_method
//!  The remote API method to call
//! @param params
//! @param data
//!  Eventual inline data to send
//! @param cb
//!  Callback function to get into in async mode
mixed post(string api_method, void|ParamsArg params, void|string data,
           void|Callback cb)
{
  return call(api_method, params, "POST", data, cb);
}

//! Invokes a call with a DELETE method
//!
//! @param api_method
//!  The remote API method to call
//! @param params
//! @param cb
//!  Callback function to get into in async mode
mixed delete(string api_method, void|ParamsArg params, void|Callback cb)
{
  return call(api_method, params, "DELETE", 0, cb);
}

//! Invokes a call with a PUT method
//!
//! @param api_method
//!   The remote API method to call
//! @param params
//! @param cb
//!  Callback function to get into in async mode
mixed put(string api_method, void|ParamsArg params, void|Callback cb)
{
  return call(api_method, params, "PUT", 0, cb);
}

//! Invokes a call with a PATCH method
//!
//! @param api_method
//!   The remote API method to call
//! @param params
//! @param cb
//!  Callback function to get into in async mode
mixed patch(string api_method, void|ParamsArg params, void|Callback cb)
{
  return call(api_method, params, "PATCH", 0, cb);
}

//! Calls a remote API method.
//!
//! @throws
//!  An exception is thrown if the response status code is other than
//!  @expr{200@}, @expr{301@} or @expr{302@}.
//!
//! @param api_method
//!  The remote API method to call!
//!  This should be a Fully Qualified Domain Name
//!
//! @param params
//!  Additional params to send in the request
//!
//! @param http_method
//!  HTTP method to use. @expr{GET@} is default
//!
//! @param data
//!  Inline data to send in a @expr{POST@} request for instance.
//!
//! @param cb
//!  Callback function to get into in async mode
//!
//! @returns
//!  If JSON is available the JSON response from servie will be decoded
//!  and returned. If not, the raw response (e.g a JSON string) will be
//!  returned. The exception to this is if the status code in the response is a
//!  @expr{30x@} (a redirect), then the response headers mapping will be
//!  returned.
mixed call(string api_method, void|ParamsArg params,
           void|string http_method, void|string data, void|Callback cb)
{
  http_method = upper_case(http_method || "get");
  Web.Auth.Params p = Web.Auth.Params();
  p->add_mapping(default_params());

  if (params) p += params;

  if (_auth && !_auth->is_expired()) {
    if (string a = _auth->access_token) {
      p += Web.Auth.Param(ACCESS_TOKEN_PARAM_NAME, a);
    }
  }

  if (http_method == "POST") {
    if (!data) data = (string) p;
    params = 0;
  }
  else {
    params = (mapping) p;
  }

  //Request req;
  Protocols.HTTP.Query req = Protocols.HTTP.Query();

#ifdef SOCIAL_REQUEST_DEBUG
    TRACE("\n> Request: %s %s?%s\n", http_method, api_method, (string) p);
    if (data) TRACE("> data: %s\n", data);
#endif

  if (cb) {
    req->set_callbacks(
      lambda (Protocols.HTTP.Query qq, mixed ... args) {
        if (qq->status == 200) {
          cb(handle_response(qq), qq);
        }
      },
      lambda (Protocols.HTTP.Query qq, mixed ... args) {
        cb(0, qq);
      }
    );

    Protocols.HTTP.do_async_method(http_method, api_method, params,
                                   default_headers, req, data);


    return 0;
  }

  req = Protocols.HTTP.do_method(http_method, api_method, params,
                                 default_headers, req, data);
  return req && handle_response(req);
}

private mixed handle_response(Protocols.HTTP.Query req)
{
  TRACE("Handle response: %O\n", req);

  if ((< 301, 302 >)[req->status])
    return req->headers;

#ifdef SOCIAL_REQUEST_DATA_DEBUG
    TRACE("Data: [%s]\n\n", req->data()||"(empty)");
#endif

  if (req->status != 200) {
    string d = req->data();

    TRACE("Bad resp[%d]: %s\n\n%O\n",
          req->status, req->data(), req->headers);

    if (has_value(d, "error")) {
      mapping e;
      mixed err = catch {
        e = Standards.JSON.decode(d);
      };

      if (e) {
        if (e->error)
          error("Error %d: %s. ", e->error->code, e->error->message);
        else if (e->meta && e->meta->code)
          error("Error %d: %s. ", e->meta->code, e->meta->error_message);
      }

      error("Error: %s", "Unknown");
    }

    error("Bad status (%d) in HTTP response! ", req->status);
  }

  if (utf8_decode) {
    TRACE("Decode UTF8: %s\n", req->data());
    return Standards.JSON.decode_utf8(unescape_forward_slashes(req->data()));
  }

  return Standards.JSON.decode(unescape_forward_slashes(req->data()));
}

protected string _sprintf(int t)
{
  return sprintf("%O(authorized:%O)", this_program,
                 (_auth && !!_auth->access_token));
}

//! Convenience method for getting the URI to a specific API method
//!
//! @param method
protected string get_uri(string method)
{
  if (has_suffix(API_URI, "/")) {
    if (has_prefix(method, "/"))
      method = method[1..];
  }
  else {
    if (!has_prefix(method, "/"))
      method = "/" + method;
  }

  // Remove eventual double slashes.
  if (search(method, "//") > -1)
    method = replace(method, "//", "/");

  return API_URI + method;
}

//! Returns the encoding from a response
//!
//! @param h
//!  The headers mapping from a HTTP respose
protected string get_encoding(mapping h)
{
  if (h["content-type"]) {
    sscanf(h["content-type"], "%*scharset=%s", string s);
    return s && lower_case(String.trim_all_whites(s)) || "";
  }

  return "";
}

//! Unescapes escaped forward slashes in a JSON string
protected string unescape_forward_slashes(string s)
{
  return replace(s, "\\/", "/");
}

//! Return default params
protected mapping default_params()
{
  return ([]);
}

//! Internal class ment to be inherited by implementing Api's classes that
//! corresponds to a given API endpoint.
class Method
{
  //! API method location within the API
  //!
  //! @code
  //!  https://api.instagram.com/v1/media/search
  //!  ............................^^^^^^^
  //! @endcode
  protected constant METHOD_PATH = 0;

  //! Hidden constructor. This class can not be instantiated directly
  protected void create()
  {
    if (this_program == Web.Api.Api.Method)
      error("This class can not be instantiated directly! ");
  }

  //! Internal convenience method
  protected mixed _get(string s, void|ParamsArg p, void|Callback cb);

  //! Internal convenience method
  protected mixed _put(string s, void|ParamsArg p, void|Callback cb);

  //! Internal convenience method
  protected mixed _post(string s, void|ParamsArg p, void|string data,
                        void|Callback cb);

  //! Internal convenience method
  protected mixed _delete(string s, void|ParamsArg p, void|Callback cb);

  //! Internal convenience method
  protected mixed _patch(string s, void|ParamsArg p, void|Callback cb);
}
