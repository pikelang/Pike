#pike __REAL_VERSION__

//! Instantiates the default Google+ API.
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
  return V1(client_id, client_secret, redirect_uri, scope);
}

class V1
{
  inherit Web.Api.Google.Api;

  //! API base URI.
  protected constant API_URI = "https://www.googleapis.com/plus/v1";

  protected constant AuthClass = Web.Auth.Google.Plus;


  //! Getter for the @[People] object which has methods for all @expr{people@}
  //! related Google+ API methods.
  //!
  //! @seealso
  //!  @url{https://developers.google.com/+/api/latest/people@}
  People `people()
  {
    return _people || (_people = People());
  }

  //! Getter for the @[Activities] object which has methods for all
  //! @expr{activities@} related Google+ API methods.
  //!
  //! @seealso
  //!  @url{https://developers.google.com/+/api/latest/activities@}
  Activities `activities()
  {
    return _activities || (_activities = Activities());
  }

  //! Class implementing the Google+ People API.
  //! @url{https://developers.google.com/+/api/latest/people@}
  //!
  //! Retreive an instance of this class through the
  //! @[Social.Google.Plus()->people] property
  class People
  {
    inherit Method;
    protected constant METHOD_PATH = "/people/";

    //! Get info ablut a person.
    //!
    //! @param user_id
    //!  If empty the currently authenticated user will be fetched.
    //! @param cb
    //!  Callback for async request
    mapping get(void|string user_id, void|Callback cb)
    {
      return _get(user_id||"me", 0, cb);
    }

    //! List all of the people in the specified @[collection].
    //!
    //! @param user_id
    //!  If empty the currently authenticated user will be used.
    //!
    //! @param collection
    //!  If empty "public" activities will be listed. Acceptable values are:
    //!  @ul
    //!   @item "public"
    //!    The list of people who this user has added to one or more circles,
    //!    limited to the circles visible to the requesting application.
    //!  @endul
    //!
    //! @param params
    //!  @mapping
    //!   @member int "maxResult"
    //!    Max number of items ti list
    //!   @member string "orderBy"
    //!    The order to return people in. Acceptable values are:
    //!    @ul
    //!     @item "alphabetical"
    //!      Order the people by their display name.
    //!     @item "best"
    //!      Order people based on the relevence to the viewer.
    //!    @endul
    //!   @member string "pageToken"
    //!    The continuation token, which is used to page through large result
    //!    sets. To get the next page of results, set this parameter to the value
    //!    of @expr{nextPageToken@} from the previous response.
    //!  @endmapping
    //!
    //! @param cb
    //!  Callback for async request
    mapping list(void|string user_id, void|string collection,
                 void|ParamsArg params, void|Callback cb)
    {
      return _get((user_id||"me") + "/activities/" + (collection||"public"),
                  params, cb);
    }
  }

  //! Class implementing the Google+ Activities API.
  //! @url{https://developers.google.com/+/api/latest/activities@}
  //!
  //! Retreive an instance of this class through the
  //! @[activities] property
  class Activities
  {
    inherit Method;
    protected constant METHOD_PATH = "/activities/";

    mapping activity(string activity_id, void|Callback cb)
    {
      return _get(activity_id, 0, cb);
    }
  }

  //! Internal singleton objects. Get an instance via @[people] and
  //! @[activities].
  private People _people;
  private Activities _activities;
}
