
//! Implements a stateless Digest implementation with the MD5
//! algorithm. To add an actual user lookup overload @[get_password]
//! or @[get_hashed_password]. Hashed passwords must be hashed with
//! the digest MD5 hash, e.g. by calling hash(user, realm, password).
//!
//! @example
//! class Auth {
//!   inherit Protocols.HTTP.Authentication.DigestMD;
//!   Concurrent.Future get_password(string user) {
//!     Promise p = Concurrent.Promise();
//!     if( user == "bob" )
//!       return p->success("builder");
//!     return p->failure(sprintf("No user %O", user));
//!   }
//! }
//!
//! Auth auth = Auth("apps@@pike.org");
//! Concurrent.Future authenticate(Protocols.HTTP.Server.Request req) {
//!   Concurrent.Future authenticated = Concurrent.Promise();
//!   auth->authenticate(req->request_headers->authorization,
//!                      req->request_method, request->not_query)
//!     ->then(lambda(string user) {
//!         authenticated->success(user);
//!       },
//!       lambda(string reason) {
//!         authenticated->failure(reason);
//!         string c = auth->challenge();
//!         request->response_and_finish( ([ "error":401,
//!                                          "extra_heads" : ([
//!                                            "WWW-Authenticate":c,
//!                                        ]) ]) );
//!       });
//!   return authenticated;
//! }
//! @endexample
class DigestMD5 (string realm)
{

  // FIXME: We have no bookkeeping of server generated nonces. Idea:
  // sign them so we don't have to keep track of them. Send signature
  // in opaque value.

  constant algorithm = "MD5";
  string hash(string ... args) {
    return sprintf("%032x", Crypto.MD5.hash( args * ":" ));
  }

  //! Creates a challenge header value for the WWW-Authenticate header
  //! in 401 responses.
  string challenge() {
    // MD5 has a digest size of 16 bytes. Go for the nearest multiple
    // of 3 above to utilize base64 lenght fully.
    return sprintf("Digest realm=\"%s\", qop=\"auth\", "
                   "nonce=\"%s\", algorithm=" + algorithm,
                   realm, MIME.encode_base64(random_string(18)));
  }

  //! Function intended to be overloaded that returns a future that
  //! will resolve to the given users password.
  //! @seealso
  //! @[get_hashed_password]
  Concurrent.Future get_password(string user) {
    return Concurrent.reject(sprintf("No user %O", user));
  }

  //! Function intended to be overloaded that returns a future that
  //! will resolved to the given users hashed password. Overloading
  //! this function will prevent @[get_password] from being called.
  Concurrent.Future get_hashed_password(string user, string nonce, string cnonce) {
    Concurrent.Promise ret = Concurrent.Promise();
    get_password(user)->then(lambda(string password) {
        if( !password )
          ret->failure( sprintf("No user %O", user) );
        else {
          // algorithm MD5-sess
          // return MD5(MD5(username, realm, password), nonce, cnonce);
          ret->success( hash(user, realm, password) );
        }
      },
      lambda(string msg) {
        ret->failure(msg);
      });
    return ret->future();
  }

  //! Split client generated Authorization header into its parts.
  mapping(string:string) split_response(string hdr) {
    mapping parts = ([]);
    while( sizeof(hdr) ) {
      hdr = String.trim_all_whites(hdr);

      string name;
      if( sscanf(hdr, "%s=%s", name, hdr)!=2 ) {

        // Ignore unknown tokens. (RFC 2617 3.2.1 auth-param)
        if( sscanf(hdr, "%s,%s", name, hdr)==2 )
          continue;

        return parts;
      }
      hdr = String.trim_all_whites(hdr);

      string value;
      if( !sizeof(hdr) ) return parts;
      if( hdr[0]=='\"' ) {
        if( sscanf(hdr, "\"%s\"%s", value, hdr)!=2 )
          return parts;
        hdr = String.trim_all_whites(hdr);
        if( sizeof(hdr) && hdr[0]==',' )
          hdr = hdr[1..];
      }
      else {
        sscanf(hdr, "%s,%s", value, hdr);
        value = String.trim_all_whites(value);
      }

      // Warn on overwrite?
      parts[name] = value;
    }
    return parts;
  }

  //! Authenticate a request.
  //!
  //! @param string hdr
  //!   The value of the Authorization header. Zero is acceptable, but
  //!   will produce an unconditional rejection.
  //!
  //! @param string method
  //!   This is the HTTP method used, typically "GET" or "POST".
  //!
  //! @param string path
  //!   This is the path of the request.
  Concurrent.Future authenticate(string hdr, string method, string path) {

    if(!hdr) return Concurrent.reject("Missing authenticate header");
    if(!method) return Concurrent.reject("Missing method");
    if(!path) return Concurrent.reject("Missing path");

    if( sscanf(hdr, "Digest %s", hdr)!=1 )
      return Concurrent.reject("Not a Digest header.");

    mapping resp = split_response(hdr);

    if( realm != resp->realm )
      return Concurrent.reject("Realms do not match");

    if( resp->qop && resp->qop!="auth" )
      return Concurrent.reject("Unsupported qop");

    string nonce = resp->nonce;
    if( !nonce )
      return Concurrent.reject("Missing nonce");

    if( !resp->response )
      return Concurrent.reject("Missing response");

    string username = resp->username;
    if( !username )
      return Concurrent.reject("No username key.");

    Concurrent.Promise p = Concurrent.Promise();
    get_hashed_password(username, nonce, resp->cnonce)->then(lambda(string ha1) {
        if( !ha1 )
          return p->failure("Failed to resolve hashed password.");

        // on qop = auth-int
        // MD5(method, digestURI, MD5(entityBody));
        string ha2 = hash(method, path);

        if( (< "auth", "auth-int" >)[resp->qop] ) {
          if( !resp->nc )
            return p->failure("Missing nc key");
          if( !resp->cnonce )
            return p->failure("Missing cnonce");
          string h = hash( ha1, nonce, resp->nc, resp->cnonce, resp->qop,
                          ha2 );
          if( h == resp->response ) {
            p->success(username);
          }
          else {
            p->failure("Wrong password");
          }
        }
        else {
          string h = hash( ha1, nonce, ha2 );
          return h == resp->response;
        }

      },
      lambda(string msg) {
        p->failure(msg);
      });

    return p->future();
  }
}
