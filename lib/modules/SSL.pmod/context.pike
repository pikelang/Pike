//
// $Id: context.pike,v 1.19 2003/01/27 15:16:31 nilsson Exp $

#pike __REAL_VERSION__
#pragma strict_types

//! Keeps the state that is shared by all SSL-connections for
//! one server (or one port). It includes policy configuration, a server
//! certificate, the server's private key(s), etc. It also includes the
//! session cache.

import .Constants;

//! The server's private key
Crypto.rsa rsa;

//! Temporary, non-certified, private keys, used with a
//! server_key_exchange message. The rules are as follows:
//!
//! If the negotiated cipher_suite has the "exportable" property, and
//! short_rsa is not zero, send a server_key_exchange message with the
//! (public part of) the short_rsa key.
//!
//! If the negotiated cipher_suite does not have the exportable
//! property, and long_rsa is not zero, send a server_key_exchange
//! message with the (public part of) the long_rsa key.
//!
//! Otherwise, dont send any server_key_exchange message.
Crypto.rsa long_rsa;
Crypto.rsa short_rsa;

//! Servers dsa key.
Crypto.dsa dsa;

//! Parameters for dh keyexchange.
.Cipher.DHParameters dh_params;

//! Used to generate random cookies for the hello-message. If we use
//! the RSA keyexchange method, and this is a server, this random
//! number generator is not used for generating the master_secret.
function(int:string) random;

//! The server's certificate, or a chain of X509.v3 certificates, with the
//! server's certificate first and root certificate last.
array(string) certificates;

//! For client authentication. Used only if auth_level is AUTH_ask or
//! AUTH_require.
array(int) preferred_auth_methods =
({ AUTH_rsa_sign });

//! Cipher suites we want the server to support, best first.
array(int) preferred_suites;

//! Set @[preferred_suites] to RSA based methods.
void rsa_mode()
{
#ifdef SSL3_DEBUG
  werror("SSL.context: rsa_mode()\n");
#endif
  preferred_suites = ({
#ifndef WEAK_CRYPTO_40BIT
    SSL_rsa_with_idea_cbc_sha,
    SSL_rsa_with_rc4_128_sha,
    SSL_rsa_with_rc4_128_md5,
    SSL_rsa_with_3des_ede_cbc_sha,
    SSL_rsa_with_des_cbc_sha,
#endif /* !WEAK_CRYPTO_40BIT (magic comment) */
    SSL_rsa_export_with_rc4_40_md5,
    SSL_rsa_with_null_sha,
    SSL_rsa_with_null_md5,
  });
}

//! Set @[preferred_suites] to DSS based methods.
void dhe_dss_mode()
{
#ifdef SSL3_DEBUG
  werror("SSL.context: dhe_dss_mode()\n");
#endif
  preferred_suites = ({
#ifndef WEAK_CRYPTO_40BIT
    SSL_dhe_dss_with_3des_ede_cbc_sha,
    SSL_dhe_dss_with_des_cbc_sha,
#endif /* !WEAK_CRYPTO_40BIT (magic comment) */
    SSL_dhe_dss_export_with_des40_cbc_sha,
  });
}

//! Always ({ COMPRESSION_null })
array(int) preferred_compressors =
({ COMPRESSION_null });

//! Non-zero to enable cahing of sessions
int use_cache = 1;

//! Sessions are removed from the cache when they are older than this
//! limit (in seconds). Sessions are also removed from the cache if a
//! connection using the session dies unexpectedly.
int session_lifetime = 600;

/* Session cache */
ADT.Queue active_sessions;  /* Queue of pairs (time, id), in cronological order */
mapping(string:.session) session_cache;

int session_number; /* Incremented for each session, and used when constructing the
		     * session id */

// Remove sessions older than @[session_lifetime] from the session cache.
void forget_old_sessions()
{
  int t = time() - session_lifetime;
  array pair;
  while ( (pair = [array]active_sessions->peek())
	  && (pair[0] < t))
    session_cache[([array(string)]active_sessions->get())[1]] = 0;
}

//! Lookup a session identifier in the cache. Returns the
//! corresponding session, or zero if it is not found or caching is
//! disabled.
.session lookup_session(string id)
{
  if (use_cache)
  {
    forget_old_sessions();
    return session_cache[id];
  }
  else
    return 0;
}

//! Create a new session.
.session new_session()
{
  .session s = .session();
  s->identity = (use_cache) ? sprintf("%4cPikeSSL3%4c",
				      time(), session_number++) : "";
  return s;
}

//! Add a session to the cache (if caching is enabled).
void record_session(.session s)
{
  if (use_cache && s->identity)
  {
    active_sessions->put( ({ time(), s->identity }) );
    session_cache[s->identity] = s;
  }
}

//! Remove a session from the cache.
void purge_session(object s)
{
#ifdef SSL3_DEBUG
  werror("SSL.context->purge_session: %O\n", s->identity || "");
#endif
  if (s->identity)
    session_cache[s->identity] = 0;
  /* There's no need to remove the id from the active_sessions queue */
}

void create()
{
#ifdef SSL3_DEBUG
  werror("SSL.context->create\n");
#endif
  active_sessions = ADT.Queue();
  session_cache = ([ ]);
  /* Backwards compatibility */
  rsa_mode();
}
