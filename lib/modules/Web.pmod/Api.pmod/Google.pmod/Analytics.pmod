/*
  Author: Pontus Östlund <https://profiles.google.com/poppanator>

  Permission to copy, modify, and distribute this source for any legal
  purpose granted as long as my name is still attached to it. More
  specifically, the GPL, LGPL and MPL licenses apply to this software.
*/

//! Instantiates the default Analytics API.
//! See @[WebApi.Api()] for further information about the arguments
//!
//! @param client_id
//!  Your application key/id
//! @param client_secret
//!  Your application secret
//! @param redirect_uri
//!  The redirect URI after an authentication
//! @param scope
//!  The application scopes to grant access to
this_program `()(string client_id, string client_secret,
                 void|string redirect_uri,
                 void|string|array(string)|multiset(string) scope)
{
  return V3(client_id, client_secret, redirect_uri, scope);
}

class V3
{
  inherit WebApi.Google.Api : parent;

  protected constant AuthClass = Auth.Google.Analytics;

  //! API base URI.
  protected constant API_URI = "https://www.googleapis.com/analytics/v3";

  //! Getter for the @[Core] API
  Core `core()
  {
    return _core || (_core = Core());
  }

  //! Getter for the @[RealTime] API
  RealTime `realtime()
  {
    return _realtime || (_realtime = RealTime());
  }

  //! Getter for the @[Management] API
  Management `management()
  {
    return _management || (_management = Management());
  }

  //! Interface to the Google Analytics core API
  class Core
  {
    inherit Method;

    protected constant METHOD_PATH = "/data/ga";

    //! Get data from the core api
    //!
    //! @param params
    //!
    //! @param cb
    mixed get(mapping params, void|Callback cb)
    {
      return _get("", params, cb);
    }
  }

  //! Interface to the Google Analytics realtime API
  class RealTime
  {
    inherit Method;

    protected constant METHOD_PATH = "/data/realtime";

    //! Get data from the realtime api
    //!
    //! @param params
    //!
    //! @param cb
    mixed get(mapping params, void|Callback cb)
    {
      return _get("", params, cb);
    }
  }

  //! Interface to the Google Analytics managment API
  class Management
  {
    inherit Method;

    protected constant METHOD_PATH = "/management";

    //! Get account summaries
    //!
    //! @param params
    //!
    //! @param cb
    mixed account_summaries(void|ParamsArg params, void|Callback cb)
    {
      return _get("/accountSummaries", params, cb);
    }
  }

  //! Internal singleton objects. Retrieve an instance via @[core],
  //! @[realtime] and @[management].
  protected Core _core;
  protected RealTime _realtime;
  protected Management _management;
}
