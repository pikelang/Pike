#pike __REAL_VERSION__

//! Google authentication classes.

//! Default Google authorization class
class Authorization
{
  inherit Web.Auth.OAuth2.Client;

  //!
  constant OAUTH_AUTH_URI  = "https://accounts.google.com/o/oauth2/auth";

  //!
  constant SCOPE_PROFILE = "profile";

  //!
  constant SCOPE_EMAIL = "email";

  //!
  constant SCOPE_OPENID = "openid";

  //! All valid scopes
  multiset(string) valid_scopes = (<
    SCOPE_PROFILE, SCOPE_EMAIL, SCOPE_OPENID >);

  protected string _scope = SCOPE_PROFILE;
}

//! Google Analytics authorization class
class Analytics
{
  inherit Authorization;

  //! Authentication scopes
  constant SCOPE_RO = "https://www.googleapis.com/auth/analytics.readonly";
  constant SCOPE_RW = "https://www.googleapis.com/auth/analytics";
  constant SCOPE_EDIT = "https://www.googleapis.com/auth/analytics.edit";
  constant SCOPE_MANAGE_USERS =
    "https://www.googleapis.com/auth/analytics.manage.users";
  constant SCOPE_MANAGE_USERS_RO =
    "https://www.googleapis.com/auth/analytics.manage.users.readonly";

  //! All valid scopes
  multiset(string) valid_scopes = (<
    SCOPE_RO, SCOPE_RW, SCOPE_EDIT, SCOPE_MANAGE_USERS,
    SCOPE_MANAGE_USERS_RO >);

  //! Default scope
  protected string _scope = SCOPE_RO;
}

//! Google+ authorization class
class Plus
{
  inherit Authorization;

  //! Authentication scopes
  constant SCOPE_ME = "https://www.googleapis.com/auth/plus.me";
  constant SCOPE_LOGIN = "https://www.googleapis.com/auth/plus.login";

  //! All valid scopes
  protected multiset(string) valid_scopes = (<
    SCOPE_ME, SCOPE_LOGIN, SCOPE_EMAIL >);

  //! Default scope
  protected string _scope = SCOPE_ME;
}
