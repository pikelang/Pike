/*
  Author: Pontus Östlund <https://profiles.google.com/poppanator>

  Permission to copy, modify, and distribute this source for any legal
  purpose granted as long as my name is still attached to it. More
  specifically, the GPL, LGPL and MPL licenses apply to this software.
*/

inherit Auth.OAuth.Authentication;

//! The endpoint to send request for a request token
constant REQUEST_TOKEN_URL = "https://api.twitter.com/oauth/request_token";

//! The endpoint to send request for an access token
constant ACCESS_TOKEN_URL = "https://api.twitter.com/oauth/access_token";

//! The enpoint to redirect to when authenticating an application
constant USER_AUTH_URL = "https://api.twitter.com/oauth/authenticate";
