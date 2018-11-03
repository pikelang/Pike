#pike __REAL_VERSION__

//! Twitter authentication class

inherit Web.Auth.OAuth.Authentication;

//! The endpoint to send request for a request token
constant REQUEST_TOKEN_URL = "https://api.twitter.com/oauth/request_token";

//! The endpoint to send request for an access token
constant ACCESS_TOKEN_URL = "https://api.twitter.com/oauth/access_token";

//! The enpoint to redirect to when authenticating an application
constant USER_AUTH_URL = "https://api.twitter.com/oauth/authenticate";
