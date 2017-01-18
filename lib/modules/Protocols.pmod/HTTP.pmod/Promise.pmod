#pike __REAL_VERSION__

//! This HTTP client module utilises the @[Concurrent.Promise] and
//! @[Concurrent.Future] classes and only does asynchronous calls.
//!
//! @example
//! @code
//! Protocols.HTTP.Promise.Arguments a1, a2;
//!
//! a1 = Protocols.HTTP.Promise.Arguments(([
//!   "extra_args" : ({ "Extra arg for Roxen request" }),
//!   "headers"    : ([ "User-Agent" : "My Special HTTP Client" ])
//! ]));
//!
//! a2 = Protocols.HTTP.Promise.Arguments(([
//!   "variables" : ([ "q" : "Pike programming language" ]),
//!   "maxtime"   : 10
//! ]));
//!
//! Concurrent.Future q1 = Protocols.HTTP.Promise.get_url("http://www.roxen.com", a1);
//! Concurrent.Future q2 = Protocols.HTTP.Promise.get_url("http://www.google.com", a2);
//!
//! array(Concurrent.Future) all = ({ q1, q2 });
//!
//! /*
//!   To get a callback for each of the requests
//! */
//!
//! all->on_success(lambda (Protocols.HTTP.Promise.Success ok_resp) {
//!   werror("Got successful response for %O\n", ok_resp->host);
//! });
//! all->on_failure(lambda (Protocols.HTTP.Promise.Failure failed_resp) {
//!   werror("Request for %O failed!\n", failed_resp->host);
//! });
//!
//!
//! /*
//!   To get a callback when all of the requests are done. In this case
//!   on_failure will be called if any of the request fails.
//! */
//!
//! Concurrent.Future all2 = Concurrent.results(all);
//!
//! all2->on_success(lambda (array(Protocols.HTTP.Promise.Success) ok_resp) {
//!   werror("All request were successful: %O\n", ok_resp);
//! });
//! all->on_failure(lambda (Protocols.HTTP.Promise.Failure failed_resp) {
//!   werror("The request to %O failed.\n", failed_resp->host);
//! });
//! @endcode

// #define HTTP_PROMISE_DESTRUCT_DEBUG
// #define HTTP_PROMISE_DEBUG

#ifdef HTTP_PROMISE_DESTRUCT_DEBUG
# define PROMISE_DESTRUCTOR                           \
  void destroy() {                                    \
    werror("%O().destroy()\n", object_program(this)); \
  }
#else
# define PROMISE_DESTRUCTOR
#endif

#ifdef HTTP_PROMISE_DEBUG
# define TRACE(X...)werror("%s:%d: %s",basename(__FILE__),__LINE__,sprintf(X))
#else
# define TRACE(X...)0
#endif


//! @ignore
protected int _timeout;
protected int _maxtime;
//! @endignore


//! @decl void set_timeout(int t)
//! @decl void set_maxtime(int t)
//!
//! @[set_timeout()] sets the default timeout for connecting and data fetching.
//! The watchdog will be reset each time data is fetched.
//!
//! @[set_maxtime()] sets the timeout for the entire operation. If this is set
//! to @tt{30@} seconds for instance, the request will be aborted after
//! @tt{30@} seconds event if data is still being received. By default this is
//! indefinitley.
//!
//! @[t] is the timeout in seconds.
//!
//! @seealso
//!  @[Arguments]

public void set_timeout(int t)
{
  _timeout = t;
}

public void set_maxtime(int t)
{
  _maxtime = t;
}


//! @decl Concurrent.Future get_url(Protocols.HTTP.Session.URL url,    @
//!                                 void|Arguments args)
//! @decl Concurrent.Future post_url(Protocols.HTTP.Session.URL url,   @
//!                                  void|Arguments args)
//! @decl Concurrent.Future put_url(Protocols.HTTP.Session.URL url,    @
//!                                 void|Arguments args)
//! @decl Concurrent.Future delete_url(Protocols.HTTP.Session.URL url, @
//!                                    void|Arguments args)
//!
//! Sends a @tt{GET@}, @tt{POST@}, @tt{PUT@} or @tt{DELETE@} request to @[url]
//! asynchronously. A @[Concurrent.Future] object is returned on which you
//! can register callbacks via @[Concurrent.Future->on_success()] and
//! @[Concurrent.Future.on_failure()] which will get a @[Success] or @[Failure]
//! object as argument respectively.
//!
//! For an example of usage see @[Protocols.HTTP.Promise]

public Concurrent.Future get_url(Protocols.HTTP.Session.URL url,
                                 void|Arguments args)
{
  return do_method("GET", url, args);
}

public Concurrent.Future post_url(Protocols.HTTP.Session.URL url,
                                  void|Arguments args)
{
  return do_method("POST", url, args);
}

public Concurrent.Future put_url(Protocols.HTTP.Session.URL url,
                                 void|Arguments args)
{
  return do_method("PUT", url, args);
}

public Concurrent.Future delete_url(Protocols.HTTP.Session.URL url,
                                    void|Arguments args)
{
  return do_method("POST", url, args);
}


//! Fetch an URL with the @[http_method] method.
public Concurrent.Future do_method(string http_method,
                                   Protocols.HTTP.Session.URL url,
                                   void|Arguments args)
{
  if (!args) {
    args = Arguments();
  }

  Concurrent.Promise p = Concurrent.Promise();
  Session s = Session();

  if (args->maxtime || _maxtime) {
    s->maxtime = args->maxtime || _maxtime;
  }

  if (args->timeout || _timeout) {
    s->timeout = args->timeout || _timeout;
  }

  if (!args->follow_redirects) {
    s->follow_redirects = 0;
  }

  s->async_do_method_url(http_method, url,
                         args->variables,
                         args->data,
                         args->headers,
                         0, // headers received callback
                         lambda (Result ok) {
                           p->success(ok);
                         },
                         lambda (Result fail) {
                           p->failure(fail);
                         },
                         args->extra_args || ({}));
  return p->future();
}


//! Class representing the arguments to give to @[get_url()], @[post_url()]
//! @[put_url()], @[delete_url()] and @[do_method()].
class Arguments
{
  //! Data fetch timeout
  int timeout;

  //! Request timeout
  int maxtime;

  //! Additional request headers
  mapping(string:string) headers;

  //! Query variables
  mapping(string:mixed) variables;

  //! POST data
  void|string|mapping data;

  //! Should redirects be followed. Default is @tt{true@}.
  bool follow_redirects = true;

  //! Extra arguments that will end up in the @[Result] object
  array(mixed) extra_args;

  //! If @[args] is given the indices that match any of this object's
  //! members will set those object members to the value of the
  //! corresponding mapping member.
  protected void create(void|mapping(string:mixed) args)
  {
    if (args) {
      foreach (args; string key; mixed value) {
        if (has_index(this, key)) {
          if ((< "variables", "headers" >)[key]) {
            // Cast all values to string
            value = mkmapping(indices(value), map(values(value),
                                                  lambda (mixed s) {
                                                    return (string) s;
                                                  }));
          }
          this[key] = value;
        }
        else {
          error("Unknown argument %O!\n", key);
        }
      }
    }
  }

  //! @ignore
  PROMISE_DESTRUCTOR
  //! @endignore
}


//! HTTP result class. Consider internal.
//!
//! @seealso
//!  @[Success], @[Failure]
class Result
{
  //! Internal result mapping
  protected mapping result;

  //! Return @tt{true@} if scuccess, @tt{false@} otherwise
  public bool `ok();

  //! The HTTP response headers
  public mapping `headers()
  {
    return result->headers;
  }

  //! The host that was called in the request
  public string `host()
  {
    return result->host;
  }

  //! Returns the requested URL
  public string `url()
  {
    return result->url;
  }

  //! The HTTP status of the response, e.g @tt{200@}, @tt{201@}, @tt{404@}
  //! and so on.
  public int `status()
  {
    return result->status;
  }

  //! The textual representation of @[status].
  public string `status_description()
  {
    return result->status_desc;
  }

  //! Extra arguments set in the @[Arguments] object.
  public array(mixed) `extra_args()
  {
    return result->extra_args;
  }

  //! @ignore
  protected void create(mapping _result)
  {
    TRACE("this_program(%O)\n", _result->headers);
    result = _result;
  }

  PROMISE_DESTRUCTOR
  //! @endignore
}


//! A class representing a successful request and its response. An instance of
//! this class will be given as argument to the
//! @[Concurrent.Future()->on_success()] callback registered on the returned
//! @[Concurrent.Future] object from @[get_url()], @[post_url()],
//! @[delete_url()], @[put_url()] or @[do_method()].
class Success
{
  inherit Result;

  public bool `ok() { return true; }

  //! The response body, i.e the content of the requested URL
  public string `data() { return result->data; }

  //! Returns the value of the @tt{content-length@} header.
  public int `length()
  {
    return headers && (int)headers["content-length"];
  }

  //! Returns the content type of the requested document
  public string `content_type()
  {
    if (string ct = (headers && headers["content-type"])) {
      if (sscanf (ct, "%s;", string c) == 1) {
        return c;
      }

      return ct;
    }
  }

  //! Returns the charset of the requested document, if given by the
  //! response headers.
  public string `charset()
  {
    if (string ce = (headers && headers["content-type"])) {
      if (sscanf (ce, "%*s;%*scharset=%s", ce) == 3) {
        if (ce[0] == '"' || ce[0] == '\'') {
          ce = ce[1..<1];
        }
        return ce;
      }
    }
  }
}


//! A class representing a failed request. An instance of
//! this class will be given as argument to the
//! @[Concurrent.Future()->on_failure()] callback registered on the returned
//! @[Concurrent.Future] object from @[get_url()], @[post_url()],
//! @[delete_url()], @[put_url()] or @[do_method()].
class Failure
{
  inherit Result;
  public bool `ok() { return false; }
}


//! Internal class for the actual HTTP requests
protected class Session
{
  inherit Protocols.HTTP.Session : parent;

  public int(0..) maxtime, timeout;

  Request async_do_method_url(string method,
                              URL url,
                              void|mapping query_variables,
                              void|string|mapping data,
                              void|mapping extra_headers,
                              function callback_headers_ok,
                              function callback_data_ok,
                              function callback_fail,
                              array callback_arguments)
  {
    return ::async_do_method_url(method, url, query_variables, data,
                                 extra_headers, callback_headers_ok,
                                 callback_data_ok, callback_fail,
                                 callback_arguments);
  }


  class Request
  {
    inherit parent::Request;

    protected void set_extra_args_in_result(mapping(string:mixed) r)
    {
      if (extra_callback_arguments && sizeof(extra_callback_arguments) > 1) {
        r->extra_args = extra_callback_arguments[1..];
      }
    }

    protected void async_fail(object q)
    {
      mapping ret = ([
        "status"      : q->status,
        "status_desc" : q->status_desc,
        "host"        : q->host,
        "headers"     : copy_value(q->headers),
        "url"         : url_requested
      ]);

      set_extra_args_in_result(ret);

      // clear callbacks for possible garbation of this Request object
      con->set_callbacks(0, 0);

      function fc = fail_callback;
      set_callbacks(0, 0, 0); // drop all references
      extra_callback_arguments = 0;

      if (fc) {
        fc(Failure(ret));
      }
    }


    protected void async_ok(object q)
    {
      TRACE("async_ok: %O -> %s!\n", q, q->host);

      ::check_for_cookies();

      if (con->status >= 300 && con->status < 400 &&
          con->headers->location && follow_redirects)
      {
        Standards.URI loc = Standards.URI(con->headers->location,url_requested);
        TRACE("New location: %O\n", loc);

        if (loc->scheme == "http" || loc->scheme == "https") {
          destroy(); // clear
          follow_redirects--;
          do_async(prepare_method("GET", loc));
          return;
        }
      }

      // clear callbacks for possible garbation of this Request object
      con->set_callbacks(0, 0);

      if (data_callback) {
        con->timed_async_fetch(async_data, async_fail); // start data downloading
      }
      else {
        extra_callback_arguments = 0; // to allow garb
      }
    }


    protected void async_data()
    {
      mapping ret = ([
        "host"        : con->host,
        "status"      : con->status,
        "status_desc" : con->status_desc,
        "headers"     : copy_value(con->headers),
        "data"        : con->data(),
        "url"         : url_requested
      ]);

      set_extra_args_in_result(ret);

      // clear callbacks for possible garbation of this Request object
      con->set_callbacks(0, 0);

      if (data_callback) {
        data_callback(Success(ret));
      }

      extra_callback_arguments = 0;
    }

    //! @ignore
    PROMISE_DESTRUCTOR
    //! @endignore
  }


  class SessionQuery
  {
    inherit parent::SessionQuery;

    protected void create()
    {
      if (Session::maxtime) {
        this::maxtime = Session::maxtime;
      }

      if (Session::timeout) {
        this::timeout = Session::timeout;
      }
    }

    //! @ignore
    PROMISE_DESTRUCTOR
    //! @endignore
  }

  //! @ignore
  PROMISE_DESTRUCTOR
  //! @endignore
}
