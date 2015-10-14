#pike __REAL_VERSION__

//! OAuth client class.
//!
//! @note
//!  This class is of no use by it self. It's intended to be inherited by
//!  classes that uses OAuth authorization.

#include "oauth.h"

import ".";

//#define assert_cache() if (!cache) return UNDEFINED

//! The consumer object.
protected Consumer consumer;

//! The token object.
protected Token token;

//! The endpoint to send request for a request token.
protected string request_token_url;

//! The endpoint to send request for an access token.
protected string access_token_url;

//! The enpoint to redirect to when authorize an application.
protected string user_auth_url;

// ! Data cache object
//protected DataCache cache;

//! Create a new @[Client].
//!
//! @note
//!  This class must be inherited
protected void create(Consumer consumer, Token token)
{
  this::consumer = consumer;
  this::token    = token;
}

//! Returns the @[consumer].
Consumer get_consumer()
{
  return consumer;
}

//! Returns the @[token].
Token get_token()
{
  return token;
}

//! @decl void set_token(Token token)
//! @decl void set_token(string token_key, string token_secret)
//!
//! Set the Token.
//!
//! @param key
//!  Either a @[Token] object or a token key.
//! @param secret
//!  The token secret if @[key] is a token key.
void set_token(Token|string key, void|string secret)
{
  token = stringp(key) ? Token(key, secret) : key;
}

//! Returns the url for requesting a request token.
string get_request_token_url()
{
  return request_token_url;
}

//! Returns the url for requesting an access token.
string get_access_token_url()
{
  return access_token_url;
}

//! Returns the url for authorizing an application.
string get_user_auth_url()
{
  return user_auth_url;
}

#if 0
// Generates a cache key.
string get_cache_key(string key)
{
  return String.string2hex(Crypto.MD5.hash(consumer->secret + key));
}

// Tries to find the entry @[key] in the cache.
mixed get_cache(string key)
{
  assert_cache();
  return cache->get(get_cache_key(key));
}

// Add to cache.
void set_cache(string key, string value, void|int maxlife)
{
  assert_cache();
  cache->set(get_cache_key(key), value, maxlife);
}

// Delete the entry with key @[key] from the cache.
void delete_cache(string key)
{
  assert_cache();
  cache->delete(get_cache_key(key));
}

// Returns the cache object.
DataCache get_cache_obj()
{
  assert_cache();
  return cache;
}

// Abstract class for cache handling.
class DataCache
{
  protected mixed cache;

  protected void create() {}

  // Return cache item with key @[key].
  mixed get(string key);

  // Set cache.
  //
  // @param maxlife
  //  Number of seconds the cache should live.
  void set(string key, mixed value, void|int maxlife);

  // Delete item with key @[key] from the cache
  void delete(string key);
}
#endif
