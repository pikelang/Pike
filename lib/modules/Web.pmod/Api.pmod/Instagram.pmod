#pike __REAL_VERSION__

//! Instagram API implementation.
//! https://instagram.com/developer/

// FIXME?: Missing support for
// /media/shortcode/
// /geographies/
// /subscriptions/

//! Instantiates the default Instagram API.
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

//! Class for communicating with version 1 of the Instagram API.
class V1
{
  inherit Web.Api.Api : parent;

  protected constant AuthClass = Web.Auth.Instagram;

  //! Getter for the @[Users] object which has methods for all @expr{users@}
  //! related Instagram API methods.
  //!
  //! @seealso
  //!  @url{http://instagram.com/developer/endpoints/users/@}
  Users `users()
  {
    return _users || (_users = Users());
  }

  //! Getter for the @[Tags] object which has methods for all @expr{tags@}
  //! related Instagram API methods.
  //!
  //! @seealso
  //!  @url{http://instagram.com/developer/endpoints/tags/@}
  Tags `tags()
  {
    return _tags || (_tags = Tags());
  }

  //! Getter for the @[Media] object which has methods for all @expr{media@}
  //! related Instagram API methods.
  //!
  //! @seealso
  //!  @url{http://instagram.com/developer/endpoints/media/@}
  Media `media()
  {
    return _media || (_media = Media());
  }

  //! Getter for the @[Comments] object which has methods for all
  //! @expr{comments@} related Instagram API methods.
  //!
  //! @seealso
  //!  @url{http://instagram.com/developer/endpoints/comments/@}
  Comments `comments()
  {
    return _comments || (_comments = Comments());
  }

  //! Getter for the @[Likes] object which has methods for all @expr{likes@}
  //! related Instagram API methods.
  //!
  //! @seealso
  //!  @url{http://instagram.com/developer/endpoints/likes/@}
  Likes `likes()
  {
    return _likes || (_likes = Likes());
  }

  //! Getter for the @[Locations] object which has methods for all
  //! @expr{locations@} related Instagram API methods.
  //!
  //! @seealso
  //!  @url{http://instagram.com/developer/endpoints/locations/@}
  Locations `locations()
  {
    return _locations || (_locations = Locations());
  }

  //! Getter for the @[Any] object which can be used to query any method in
  //! the Instagram web api. The @tt{METHOD_PATH@} is set to @tt{/@} in
  //! this object.
  Any `any()
  {
    return _any || (_any = Any());
  }

  //! Make a generic @expr{GET@} request to the Instagram API.
  //!
  //! @param path
  //!  What to request. Like @expr{me@}, @expr{users/self@},
  //!  @expr{[some_id]/something@}.
  //!
  //! @param params
  //!
  //! @param cb
  //!  Callback for async requests
  mapping get(string path, void|ParamsArg params, void|Callback cb)
  {
    return `any()->get(path, params, cb);
  }

  //! Make a generic @expr{PUT@} request to the Instagram API.
  //!
  //! @param path
  //!  What to request. Like @expr{me@}, @expr{users/self@},
  //!  @expr{[some_id]/something@}.
  //!
  //! @param params
  //!
  //! @param cb
  //!  Callback for async requests
  mapping put(string path, void|ParamsArg params, void|Callback cb)
  {
    return `any()->put(path, params, cb);
  }

  //! Make a generic @expr{POST@} request to the Instagram API.
  //!
  //! @param path
  //!  What to request. Like @expr{me@}, @expr{users/self@},
  //!  @expr{[some_id]/something@}.
  //!
  //! @param params
  //!
  //! @param data
  //!
  //! @param cb
  //!  Callback for async requests
  mapping post(string path, void|ParamsArg params, string data,
               void|Callback cb)
  {
    return `any()->post(path, params, 0, cb);
  }

  //! Make a generic @expr{DELETE@} request to the Instagram API.
  //!
  //! @param path
  //!  What to request. Like @expr{me@}, @expr{users/self@},
  //!  @expr{[some_id]/something@}.
  //!
  //! @param params
  //!
  //! @param cb
  //!  Callback for async requests
  mapping delete(string path, void|ParamsArg params, void|Callback cb)
  {
    return `any()->delete(path, params, cb);
  }

  // Throws an error if scope S isn't set in the authorization object
  // FUN is the method in which this macro is called
  #define ASSERT_SCOPE(S,FUN) do {                                        \
    if (!auth->has_scope(S)) {                                            \
      error("The method %O requires the scope \"%s\" which is not set! ", \
            FUN, S);                                                      \
    }                                                                     \
  } while (0)

  //! Internal convenience class.
  protected class Method
  {
    inherit Web.Api.Api.Method;

    //! Internal convenience method
    protected mixed _get(string s, void|ParamsArg p, void|Callback cb)
    {
      return parent::get(get_uri(METHOD_PATH + s), p, cb);
    }

    //! Internal convenience method
    protected mixed _post(string s, void|ParamsArg p, string data,
                          void|Callback cb)
    {
      return parent::post(get_uri(METHOD_PATH + s), p, data, cb);
    }

    //! Internal convenience method
    protected mixed _delete(string s, void|ParamsArg p, void|Callback cb)
    {
      return parent::delete(get_uri(METHOD_PATH + s), p, cb);
    }

    //! Internal convenience method
    protected mixed _put(string s, void|ParamsArg p, void|Callback cb)
    {
      return parent::put(get_uri(METHOD_PATH + s), p, cb);
    }
  }

  //! Class implementing the Instagram Users API.
  //! @url{http://instagram.com/developer/endpoints/users/@}
  //!
  //! Retreive an instance of this class via the @[users] property
  protected class Users
  {
    inherit Method;

    protected constant METHOD_PATH = "/users/";

    //! Get basic information about a user.
    //!
    //! @param uid
    //!  An Instagram user ID. If not given the currently authenticated user
    //!  will be fetched
    //!
    //! @param cb
    //!  Callback function when in async mode
    mapping user(void|string uid, void|Callback cb)
    {
      return _get(uid||"self", 0, cb);
    }

    //! Get the currently authenticated users feed.
    //! @url{http://instagram.com/developer/endpoints/users/#get_users@}
    //!
    //! @param params
    //!  Valida parameters are:
    //!  @mapping
    //!   @member string|int "count"
    //!    Number of items to return. Default is 20.
    //!   @member string "min_id"
    //!    Return media later than this min_id
    //!   @member string "max_id"
    //!    Return media earlier than this max_id
    //!  @endmapping
    //!
    //! @param cb
    //!  Callback function when in async mode
    mapping feed(void|ParamsArg params, void|Callback cb)
    {
      return _get("self/feed", params, cb);
    }

    //! Get the most recent media published by a user. May return a mix of both
    //! image and video types.
    //! @url{http://instagram.com/developer/endpoints/users/@
    //!get_users_media_recent@}
    //!
    //! @param uid
    //!  An Instagram user ID
    //!
    //! @param params
    //!  Valida parameters are:
    //!  @mapping
    //!   @member string|int "count"
    //!    Number of items to return. Default is 20.
    //!   @member string "min_id"
    //!    Return media later than this min_id
    //!   @member string "max_id"
    //!    Return media earlier than this max_id
    //!   @member int "max_timestamp"
    //!    Return media before this UNIX timestamp.
    //!   @member int "min_timestamp"
    //!    Return media after this UNIX timestamp.
    //!  @endmapping
    //!
    //! @param cb
    //!  Callback function when in async mode
    mapping recent(void|string uid, void|ParamsArg params, void|Callback cb)
    {
      return _get((uid||"self") + "/media/recent", params, cb);
    }

    //! See the authenticated user's list of media they've liked. May return a
    //! mix of both image and video types.
    //! Note: This list is ordered by the order in which the user liked the
    //! media. Private media is returned as long as the authenticated user has
    //! permission to view that media. Liked media lists are only available for
    //! the currently authenticated user.
    //!
    //! @url{http://instagram.com/developer/endpoints/users/@
    //!#get_users_feed_liked@}
    //!
    //! @param params
    //!  Valida parameters are:
    //!  @mapping
    //!   @member string|int "count"
    //!    Number of items to return. Default is 20.
    //!   @member string "max_like_id"
    //!    Return media liked before this id.
    //!  @endmapping
    //!
    //! @param cb
    //!  Callback function when in async mode
    mapping liked(void|ParamsArg params, void|Callback cb)
    {
      return _get("self/media/liked", params, cb);
    }

    //! Search for a user by name.
    //!
    //! @url{http://instagram.com/developer/endpoints/users/#get_users_search@}
    //!
    //! @param query
    //!  A query string.
    //!
    //! @param count
    //!
    //!  Max number of users to return
    //! @param cb
    //!  Callback function when in async mode
    mapping search(string query, void|int count, void|Callback cb)
    {
      mapping p = ([ "q" : query ]);
      if (count) p->count = count;
      return _get("search", p, cb);
    }

    //! Get the list of users this user follows.
    //!
    //! @note
    //!  Required scope: @tt{relationships@}
    //!
    //! @seealso
    //!  @url{http://instagram.com/developer/endpoints/relationships/@
    //!#get_users_follows@}
    //!
    //! @param user_id
    //!
    //! @param cb
    //!  Callback function when in async mode
    mapping follows(void|string user_id, void|Callback cb)
    {
      ASSERT_SCOPE("relationships", "Instagram()->users->follows()");
      return _get((user_id||"self") + "/follows", 0, cb);
    }

    //! Get the list of users this user is followed by.
    //!
    //! @note
    //!  Required scope: @tt{relationships@}
    //!
    //! @seealso
    //!  @url{http://instagram.com/developer/endpoints/relationships/@
    //!#get_users_followed_by@}
    //!
    //! @param user_id
    //!
    //! @param cb
    //!  Callback function when in async mode
    mapping followed_by(string|void user_id, void|Callback cb)
    {
      ASSERT_SCOPE("relationships", "Instagram()->users->followed_by()");
      return _get((user_id||"self") + "/followed-by", 0, cb);
    }

    //! List the users who have requested this user's permission to follow.
    //!
    //! @note
    //!  Required scope: @tt{relationships@}
    //!
    //! @seealso
    //!  @url{http://instagram.com/developer/endpoints/relationships/@
    //!#get_incoming_requests@}
    //!
    //! @param cb
    //!  Callback function when in async mode
    mapping requested_by(void|Callback cb)
    {
      ASSERT_SCOPE("relationships", "Instagram()->users->requested_by()");
      return _get("self/requested-by", 0, cb);
    }

    //! Get information about a relationship to another user.
    //!
    //! @note
    //!  Required scope: @tt{relationships@}
    //!
    //! @seealso
    //!  @url{http://instagram.com/developer/endpoints/relationships/@
    //!#get_relationship@}
    //!
    //! @param user_id
    //!
    //! @param cb
    //!  Callback function when in async mode
    mapping relationship(string user_id, void|Callback cb)
    {
      ASSERT_SCOPE("relationships", "Instagram()->users->relationship()");
      return _get(user_id + "/relationship", 0, cb);
    }

    //! Modify the relationship between the current user and the target user.
    //!
    //! @note
    //!  Required scope: @tt{relationships@}
    //!
    //! @param user_id
    //!  The user to change the relationship to
    //!
    //! @param action
    //!  How to change the relationship. Can be:
    //!  @ul
    //!   @item "follow"
    //!   @item "unfollow"
    //!   @item "block"
    //!   @item "unblock"
    //!   @item "approve"
    //!   @item "deny"
    //!  @endul
    //!
    //! @param cb
    //!  Callback function if in async mode
    mapping relationship_change(string user_id, string action, void|Callback cb)
    {
      ASSERT_SCOPE("relationships",
                   "Instagram()->users->relationship_change()");

      if (!(<"follow","unfollow","block","unblock","approve","deny">)[action])
        error("Unknown action %O! ", action);

      mapping p = ([ "action" : action ]);
      return _post(user_id + "/relationship", p, 0, cb);
    }
  }

  //! Class implementing the Instagram Tags API.
  //! @url{http://instagram.com/developer/endpoints/tags/@}
  //!
  //! Retreive an instance of this class via the @[tags] property.
  protected class Tags
  {
    inherit Method;

    protected constant METHOD_PATH = "/tags/";

    //! Get information about a tag object.
    //!
    //! @seealso
    //!  @url{http://instagram.com/developer/endpoints/tags/#get_tags@}
    //!
    //! @param tag_name
    //!
    //! @param cb
    //!  Callback function when in async mode
    mapping tag(string tag_name, void|Callback cb)
    {
      return _get(normalize_tag(tag_name), 0, cb);
    }

    //! Get a list of recently tagged media. Note that this media is ordered by
    //! when the media was tagged with this tag, rather than the order it was
    //! posted. Use the max_tag_id and min_tag_id parameters in the pagination
    //! response to paginate through these objects. Can return a mix of image
    //! and video types.
    //!
    //! @seealso
    //!  @url{http://instagram.com/developer/endpoints/tags/@
    //!#get_tags_media_recent@}
    //!
    //! @param tag_name
    //!
    //! @param params
    //!  Can be:
    //!  @mapping
    //!   @member string "min_id"
    //!    Return media before this min_id.
    //!   @member string "max_id"
    //!    Return media after this max_id.
    //!  @endmapping
    //!
    //! @param cb
    //!  Callback function when in async mode
    mapping recent(string tag_name, void|ParamsArg params, void|Callback cb)
    {
      return _get(normalize_tag(tag_name) + "/media/recent", params, cb);
    }

    //! Search for tags by name. Results are ordered first as an exact match,
    //! then by popularity. Short tags will be treated as exact matches.
    //!
    //! @seealso
    //!  @url{http://instagram.com/developer/endpoints/tags/#get_tags_search@}
    //!
    //! @param tag_name
    //!
    //! @param cb
    //!  Callback function when in async mode
    mapping search(string tag_name, Callback cb)
    {
      return _get("search", ([ "q" : normalize_tag(tag_name) ]), cb);
    }

    // Normalize the tag, ie remove any leading #.
    private string normalize_tag(string t)
    {
      sscanf(t, "#%s", t);
      return t;
    }
  }

  //! Class implementing the Instagram Media API.
  //! @url{http://instagram.com/developer/endpoints/media/@}
  //!
  //! Retreive an instance of this class via the @[media] property
  protected class Media
  {
    inherit Method;

    protected constant METHOD_PATH = "/media/";

    //! Get information about a media object. The returned type key will allow
    //! you to differentiate between image and video media.
    //! Note: if you authenticate with an OAuth Token, you will receive the
    //! user_has_liked key which quickly tells you whether the current user has
    //! liked this media item.
    //!
    //! @seealso
    //!  @url{http://instagram.com/developer/endpoints/media/#get_media@}
    //!
    //! @param media_id
    //!
    //! @param cb
    //!  Callback function when in async mode
    mapping item(string media_id, void|Callback cb)
    {
      return _get(media_id, 0, cb);
    }

    //! Search for media in a given area. The default time span is set to 5
    //! days. The time span must not exceed 7 days. Defaults time stamps cover
    //! the last 5 days. Can return mix of image and video types.
    //!
    //! @param params
    //!  Can have:
    //!  @mapping
    //!   @member string|float "lat"
    //!    Latitude of the center search coordinate. If used, lng is required.
    //!   @member int "min_timestamp"
    //!    A unix timestamp. All media returned will be taken later than this
    //!    timestamp.
    //!   @member string|float "lng"
    //!    Longitude of the center search coordinate. If used, lat is required.
    //!   @member int "max_timestamp"
    //!    A unix timestamp. All media returned will be taken earlier than this
    //!    timestamp.
    //!   @member int "distance"
    //!    Default is 1km (distance=1000), max distance is 5km.
    //!  @endmapping
    //!
    //! @param cb
    //!  Callback function when in async mode
    mapping search(void|ParamsArg params, void|Callback cb)
    {
      return _get("search", params, cb);
    }

    //! Get a list of what media is most popular at the moment. Can return a
    //! mix of image and video types.
    //!
    //! @param cb
    //!  Callback function when in async mode
    mapping popular(void|Callback cb)
    {
      return _get("popular", 0, cb);
    }
  }

  //! Class implementing the Instagram Comments API.
  //! @url{http://instagram.com/developer/endpoints/comments/@}
  //!
  //! Retreive an instance of this class via the @[comments] property.
  protected class Comments
  {
    inherit Method;

    protected constant METHOD_PATH = "/media/";

    //! Get a full list of comments on a media.
    //!
    //! @note
    //!  Required scope: @tt{comments@}
    //!
    //! @param media_id
    //!  The media to retreive comments for
    //!
    //! @param cb
    //!  Callback for async mode
    mapping list(string media_id, void|Callback cb)
    {
      ASSERT_SCOPE("comments", "Instagram()->comments->get()");
      return _get(media_id + "/comments", 0, cb);
    }

    //! Get a full list of comments on a media.
    //!
    //! @note
    //!  This method is not allowed by default. You have to contact Instagram
    //!  in order to activate this method. @url{http://bit.ly/instacomments@}
    //!
    //! @note
    //!  Required scope: @tt{comments@}
    //!
    //! @param media_id
    //!  The media to retreive comments for
    //!
    //! @param cb
    //!  Callback for async mode
    mapping add(string media_id, string comment, void|Callback cb)
    {
      ASSERT_SCOPE("comments", "Instagram()->comments->add()");
      return _post(media_id + "/comments", ([ "text" : comment ]), 0, cb);
    }

    //! Remove a comment either on the authenticated user's media or authored by
    //! the authenticated user.
    //!
    //! @note
    //!  Required scope: @tt{comments@}
    //!
    //! @param media_id
    //!  The media the comment is for
    //!
    //! @param comment_id
    //!  The comment to remove
    //!
    //! @param cb
    //!  Callback for async mode
    mapping remove(string media_id, string comment_id, void|Callback cb)
    {
      ASSERT_SCOPE("comments", "Instagram()->comments->delete()");
      return _delete(media_id + "/comments/" + comment_id, 0, cb);
    }
  }

  //! Class implementing the Instagram Likes API.
  //! @url{http://instagram.com/developer/endpoints/likes/@}
  //!
  //! Retreive an instance of this class via the @[likes] property.
  protected class Likes
  {
    inherit Method;

    protected constant METHOD_PATH = "/media/";

    //! Get a list of users who have liked this media.
    //!
    //! @seealso
    //!  @url{http://instagram.com/developer/endpoints/likes/#get_media_likes@}
    //!
    //! @note
    //!  Required scope: @tt{likes@}
    //!
    //! @param media_id
    //!
    //! @param cb
    mapping list(string media_id, void|Callback cb)
    {
      ASSERT_SCOPE("likes", "Instagram()->likes->list()");
      return _get(media_id + "/likes", 0, cb);
    }

    //! Set a like on this media by the currently authenticated user.
    //!
    //! @seealso
    //!  @url{http://instagram.com/developer/endpoints/likes/#post_likes@}
    //!
    //! @note
    //!  Required scope: @tt{likes@}
    //!
    //! @param media_id
    //!
    //! @param cb
    mapping add(string media_id, void|Callback cb)
    {
      ASSERT_SCOPE("likes", "Instagram()->likes->add()");
      return _post(media_id + "/likes", 0, 0, cb);
    }

    //!  Remove a like on this media by the currently authenticated user.
    //!
    //! @seealso
    //!  @url{http://instagram.com/developer/endpoints/likes/#delete_likes@}
    //!
    //! @note
    //!  Required scope: @tt{likes@}
    //!
    //! @param media_id
    //!
    //! @param cb
    mapping remove(string media_id, void|Callback cb)
    {
      ASSERT_SCOPE("likes", "Instagram()->likes->remove()");
      return _delete(media_id + "/likes", 0, cb);
    }
  }

  //! Class implementing the Instagram Likes API.
  //! @url{http://instagram.com/developer/endpoints/likes/@}
  //!
  //! Retreive an instance of this class via the @[locations] property.
  protected class Locations
  {
    inherit Method;

    protected constant METHOD_PATH = "/locations/";

    //! Get information about a location.
    //!
    //! @seealso
    //!  @url{http://instagram.com/developer/endpoints/locations/@
    //!#get_locations@}
    //!
    //! @param location_id
    //!
    //! @param cb
    mapping location(string location_id, void|Callback cb)
    {
      return _get(location_id, 0, cb);
    }

    //!  Get a list of recent media objects from a given location. May return a
    //!  mix of both image and video types.
    //!
    //! @seealso
    //!  @url{http://instagram.com/developer/endpoints/locations/@
    //!#get_locations_media_recent@}
    //!
    //!
    //! @param location_id
    //!
    //! @param params
    //!  @mapping
    //!   @member int "min_timestamp"
    //!    Return media after this UNIX timestamp
    //!   @member string "min_id"
    //!    Return media before this min_id.
    //!   @member string "max_id"
    //!    Return media after this max_id.
    //!   @member int "max_timestamp"
    //!    Return media before this UNIX timestamp
    //!  @endmapping
    //!
    //! @param cb
    mapping recent_media(string location_id, void|ParamsArg params,
                         void|Callback cb)
    {
      return _get(location_id + "/media/recent", params, cb);
    }

    //! Search for a location by geographic coordinate.
    //!
    //! @seealso
    //!  @url{http://instagram.com/developer/endpoints/locations/@
    //!#get_locations_search@}
    //!
    //! @param params
    //!  @mapping
    //!   @member float "lat"
    //!    Latitude of the center search coordinate. If used, lng is required.
    //!   @member int "distance"
    //!    Default is 1000m (distance=1000), max distance is 5000.
    //!   @member float "lng"
    //!    Longitude of the center search coordinate. If used, lat is required.
    //!   @member string|int "foursquare_v2_id"
    //!    Returns a location mapped off of a foursquare v2 api location id.
    //!    If used, you are not required to use lat and lng.
    //!   @member string|int "foursquare_id"
    //!    Returns a location mapped off of a foursquare v1 api location id.
    //!    If used, you are not required to use lat and lng. Note that this
    //!    method is deprecated; you should use the new foursquare IDs with V2
    //!    of their API.
    //!  @endmapping
    //!
    //! @param cb
    mapping search(ParamsArg params, void|Callback cb)
    {
      return _get("search", params, cb);
    }
  }

  //! A generic wrapper around @[Method]. This will query the root of the
  //! API, and can be used to query methods not implemented in this module.
  protected class Any
  {
    inherit Method;

    protected constant METHOD_PATH = "/";

    //! GET data
    //!
    //! @param s
    //!  The path to the Instagram API to query
    //!
    //! @param p
    //!  Parameters to the query
    //!
    //! @param cb
    //!  Async callback
    mixed get(string s, void|ParamsArg p, void|Callback cb)
    {
      return _get(s, p, cb);
    }

    //! POST data
    //!
    //! @param s
    //!  The path to the Instagram API to query
    //!
    //! @param p
    //!  Parameters to the query
    //!
    //! @param cb
    //!  Async callback
    mixed post(string s, void|ParamsArg p, string data, void|Callback cb)
    {
      return _post(s, p, data, cb);
    }

    //! DELETE data
    //!
    //! @param s
    //!  The path to the Instagram API to query
    //!
    //! @param p
    //!  Parameters to the query
    //!
    //! @param cb
    //!  Async callback
    mixed delete(string s, void|ParamsArg p, void|Callback cb)
    {
      return _delete(s, p, cb);
    }

    //! PUT data
    //!
    //! @param s
    //!  The path to the Instagram API to query
    //! @param p
    //!  Parameters to the query
    //! @param cb
    //!  Async callback
    mixed put(string s, void|ParamsArg p, void|Callback cb)
    {
      return _put(s, p, cb);
    }
  }

  /* Internal API */

  //! The URI to the Instagram API
  protected constant API_URI = "https://api.instagram.com/v1";

  //! Singleton @[User] object. Will be instantiated first time requested.
  private Users _users;

  //! Singleton @[Tags] object. Will be instantiated first time requested.
  private Tags  _tags;

  //! Singleton @[Media] object. Will be instantiated first time requested.
  private Media _media;

  //! Singleton @[Comments] object. Will be instantiated first time requested.
  private Comments _comments;

  //! Singleton @[Likes] object. Will be instantiated first time requested.
  private Likes _likes;

  //! Singleton @[Locations] object. Will be instantiated first time requested.
  private Locations _locations;

  //! Singleton @[Any] object. Will be instantiated first time requested.
  private Any _any;
}
