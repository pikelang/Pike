#pike __REAL_VERSION__

//! This class is used to OAuth2 authenticate against Github

inherit .OAuth2.Client;

constant OAUTH_AUTH_URI  = "https://github.com/login/oauth/authorize";
constant OAUTH_TOKEN_URI = "https://github.com/login/oauth/access_token";

enum Scopes {
  SCOPE_REPO = "repo",
  SCOPE_GIST = "gist",
  SCOPE_USER = "user",
  SCOPE_USER_EMAIL = "user:email",
  SCOPE_USER_FOLLOW = "user:follow",
  SCOPE_PUBLIC_REPO = "public_repo",
  SCOPE_REPO_DEPLOYMENT = "repo_deployment",
  SCOPE_REPO_STATUS = "repo:status",
  SCOPE_DELETE_REPO = "delete_repo",
  SCOPE_NOTIFICATIONS = "notifications",
  SCOPE_READ_REPO_HOOK = "read:repo_hook",
  SCOPE_WRITE_REPO_HOOK = "write:repo_hook",
  SCOPE_ADMIN_REPO_HOOK = "admin:repo_hook",
  SCOPE_READ_ORG = "read:org",
  SCOPE_WRITE_ORG = "write:org",
  SCOPE_ADMIN_ORG = "admin:org",
  SCOPE_READ_PUBLIC_KEY = "read:public_key",
  SCOPE_WRITE_PUBLIC_KEY = "write:public_key",
  SCOPE_ADMIN_PUBLIC_KEY = "admin:public_key"
};

protected multiset(string) valid_scopes = (<
  SCOPE_REPO,
  SCOPE_GIST,
  SCOPE_USER,
  SCOPE_USER_EMAIL,
  SCOPE_USER_FOLLOW,
  SCOPE_PUBLIC_REPO,
  SCOPE_REPO_DEPLOYMENT,
  SCOPE_REPO_STATUS,
  SCOPE_DELETE_REPO,
  SCOPE_NOTIFICATIONS,
  SCOPE_READ_REPO_HOOK,
  SCOPE_WRITE_REPO_HOOK,
  SCOPE_ADMIN_REPO_HOOK,
  SCOPE_READ_ORG,
  SCOPE_WRITE_ORG,
  SCOPE_ADMIN_ORG,
  SCOPE_READ_PUBLIC_KEY,
  SCOPE_WRITE_PUBLIC_KEY,
  SCOPE_ADMIN_PUBLIC_KEY >);
