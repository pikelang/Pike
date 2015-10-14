#pike __REAL_VERSION__

//! This class is used to OAuth2 authenticate against LinkedIn

inherit .OAuth2.Client;

//! @ignore
constant ACCESS_TOKEN_PARAM_NAME = "oauth2_access_token";
//! @endignore

constant OAUTH_AUTH_URI = "https://www.linkedin.com/uas/oauth2/authorization";
constant OAUTH_TOKEN_URI = "https://www.linkedin.com/uas/oauth2/accessToken";

//! Adds the @tt{state@} parameter to the request which will have the value
//! of a random string
protected constant STATE = 1;

//! Default scope to use if none is set explicitly
protected constant DEFAULT_SCOPE = SCOPE_R_BASIC;

enum Scopes {
  SCOPE_R_BASIC        = "r_basicprofile",
  SCOPE_R_NETWORK      = "r_network",
  SCOPE_RW_GROUPS      = "rw_groups",
  SCOPE_R_FULLPROFILE  = "r_fullprofile",
  SCOPE_R_CONTACTINFO  = "r_contactinfo",
  SCOPE_W_MESSAGES     = "w_messages",
  SCOPE_R_EMAILADDRESS = "r_emailaddress",
  SCOPE_RW_NUS         = "rw_nus"
};

protected multiset(string) valid_scopes = (<
  SCOPE_R_BASIC,
  SCOPE_R_NETWORK,
  SCOPE_RW_GROUPS,
  SCOPE_R_FULLPROFILE,
  SCOPE_R_CONTACTINFO,
  SCOPE_W_MESSAGES,
  SCOPE_R_EMAILADDRESS,
  SCOPE_RW_NUS >);

protected string _scope = SCOPE_R_BASIC;
