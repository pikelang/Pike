#pike __REAL_VERSION__

/* $Id$
 *
 * Keeps track of global data for an SSL server,
 * such as preferred encryption algorithms and session cache.
 */

//! Keeps the state that is shared by all SSL-connections for
//! one server (or one port). It includes policy configuration, a server
//! certificate, the server's private key(s), etc. It also includes the
//! session cache.

inherit "constants";

//! The server's private key
object rsa;

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
object long_rsa;
object short_rsa;

object dsa;  /* Servers dsa key */
object dh_params; /* Parameters for dh keyexchange */

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
object active_sessions;  /* Queue of pairs (time, id), in cronological order */
mapping(string:object) session_cache;

int session_number; /* Incremented for each session, and used when constructing the
		     * session id */

void forget_old_sessions()
{
  int t = time() - session_lifetime;
  array pair;
  while ( (pair = active_sessions->peek())
	  && (pair[0] < t))
    m_delete (session_cache, active_sessions->get()[1]);
}

//! Lookup a session identifier in the cache. Returns the
//! corresponding session, or zero if it is not found or caching is
//! disabled.
object lookup_session(string id)
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
object new_session()
{
  object s = SSL.session();
  s->identity = (use_cache) ? sprintf("%4cPikeSSL3%4c",
				      time(), session_number++) : "";
  return s;
}

//! Add a session to the cache (if caching is enabled).
void record_session(object s)
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
  werror(sprintf("SSL.context->purge_session: %O\n", s->identity || ""));
#endif
  if (s->identity)
    m_delete (session_cache, s->identity);
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
