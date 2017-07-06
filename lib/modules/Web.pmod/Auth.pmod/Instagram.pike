#pike __REAL_VERSION__

//! This class is used to OAuth2 authenticate agains Instagram

inherit .OAuth2.Client;

//! Instagram authorization URI
constant OAUTH_AUTH_URI  = "https://api.instagram.com/oauth/authorize";

//! Instagram request access token URI
constant OAUTH_TOKEN_URI = "https://api.instagram.com/oauth/access_token";

constant SCOPE_BASIC = "basic";
constant SCOPE_COMMENTS = "comments";
constant SCOPE_RELATIONSHIPS = "relationships";
constant SCOPE_LIKES = "likes";

//! Valid Instagram scopes
protected multiset(string) valid_scopes = (<
  SCOPE_BASIC, SCOPE_COMMENTS, SCOPE_RELATIONSHIPS, SCOPE_LIKES >);

//! Default scope
protected string _scope = SCOPE_BASIC;
