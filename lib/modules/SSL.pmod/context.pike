/* context.pike
 *
 * Keeps track of global data for an SSL server,
 * such as preferred encryption algorithms and session cache.
 */

inherit "constants";

int auth_level;

object rsa;  /* Servers private key */

function(int:string) random; /* Random number generator */

/* Chain of X509.v3 certificates
 * Senders certificate first, root certificate last .*/
array(string) certificates; 

/* For client authentication */
array(string) authorities; /* List of authorities distinguished names */

array(int) preferred_auth_methods =
({ AUTH_rsa_sign });

array(int) preferred_suites =
({ SSL_rsa_with_idea_cbc_sha,
   SSL_rsa_with_rc4_128_sha,
   SSL_rsa_with_rc4_128_md5,
   SSL_rsa_with_3des_ede_cbc_sha,
   SSL_rsa_with_des_cbc_sha,
//   SSL_rsa_export_with_rc4_40_md5,
   SSL_rsa_with_null_sha,
   SSL_rsa_with_null_md5
});

array(int) preferred_compressors =
({ COMPRESSION_null });

constant Session = (program) "session";
constant Queue = (program) "queue";

int use_cache = 1;
int session_lifetime = 600; /* Time to remember a session, in seconds */

/* Session cache */
object active_sessions;  /* Queue of pairs (time, id), in cronological order */
mapping(string:object(Session)) session_cache;

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
  s->identity = (use_cache) ? sprintf("%4c%4c", time(), session_number++) : "";
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
  if (s->identity)
    session_cache[s->identity] = 0;
  /* There's no need to remove the id from the active_sessions queue */
}

void create()
{
  active_sessions = Queue();
  session_cache = ([ ]);
}
