/* $Id: context.pike,v 1.10 2000/04/13 19:08:07 per Exp $
 *
 * Keeps track of global data for an SSL server,
 * such as preferred encryption algorithms and session cache.
 */

inherit "constants";

int auth_level;

object rsa;  /* Servers private key */

/* These temporary keys, of non-zero, are used for the
 * ServerKeyExchange message */
object long_rsa;
object short_rsa;

object dsa;  /* Servers dsa key */
object dh_params; /* Parameters for dh keyexchange */

function(int:string) random; /* Random number generator */

/* Chain of X509.v3 certificates
 * Senders certificate first, root certificate last .*/
array(string) certificates; 

/* For client authentication */
array(string) authorities; /* List of authorities distinguished names */

array(int) preferred_auth_methods =
({ AUTH_rsa_sign });

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

array(int) preferred_compressors =
({ COMPRESSION_null });

constant Session = SSL.session;
constant Queue = ADT.Queue;

int use_cache = 1;
int session_lifetime = 600; /* Time to remember a session, in seconds */

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
    session_cache[active_sessions->get()[1]] = 0;
}

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

object new_session()
{
  object s = Session();
  s->identity = (use_cache) ? sprintf("%4cPikeSSL3%4c",
				      time(), session_number++) : "";
  return s;
}

void record_session(object s)
{
  if (use_cache && s->identity)
  {
    active_sessions->put( ({ time(), s->identity }) );
    session_cache[s->identity] = s;
  }
}

void purge_session(object s)
{
#ifdef SSL3_DEBUG
  werror(sprintf("SSL.context->purge_session: '%s'\n", s->identity || ""));
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
  active_sessions = Queue();
  session_cache = ([ ]);
  /* Backwards compatibility */
  rsa_mode();
}
