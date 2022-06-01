#pike __REAL_VERSION__

//! This module contains various HTTP Authentication implements for
//! both server and client use. A Client implementation would
//! typically call the @[make_authenticator] method with the incoming
//! WWW-Authenticate header to get a @[Client] object. For each HTTP
//! request the auth() method of the object can be called to get an
//! appropriate Authorization header.
//!
//! Server code should create an authentication class and inherit the
//! concrete authentication scheme implementation. To add an actual
//! user lookup, overload @[get_password] or
//! @[get_hashed_password]. Hashed passwords must be hashed with the
//! scheme appropriate digest.
//!
//! @example
//! class Auth {
//!   inherit Protocols.HTTP.Authentication.DigestMD5Server;
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
//!   auth->auth(req->request_headers->authorization,
//!              req->request_method, request->not_query)
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

//! Split client generated Authorization header into its parts.
mapping(string:string) split_header(string hdr) {
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
      if( !sscanf(hdr, "%s,%s", value, hdr) ) {
        value = hdr;
        hdr = "";
      }
      value = String.trim_all_whites(value);
    }

    // Warn on overwrite?
    parts[name] = value;
  }
  return parts;
}

// Abstract class for hash algorithm.
class Digest {
  protected extern function(string(8bit):string(8bit)) hash_function;

  // Perform hashing of the given strings.
  string(7bit) hash(string(8bit) ... args) {
    return sprintf("%032x", hash_function( args * ":" ));
  }
}

class DigestMD5 {
  protected function(string(8bit):string(8bit)) hash_function=Crypto.MD5.hash;
  constant algorithm = "MD5";
}

class DigestSHA256 {
  protected function(string(8bit):string(8bit))
    hash_function=Crypto.SHA256.hash;
  constant algorithm = "SHA-256";
}

class DigestSHA512256 {
  protected string(8bit) hash_function(string(8bit) data) {
    return Crypto.SHA512.hash(data)[..31];
  }
  constant algorithm = "SHA-512-256";
}

//! Abstract HTTP Digest implementation.
class DigestServer
{
  inherit Digest;
  protected Crypto.MAC.State hmac;

  //! The current realm of the authentication.
  string realm;

  //! @param realm
  //!   The realm to be authenticated.
  //!
  //! @param key
  //!   If this key is set all challanges are verified against
  //!   signature using this key. The key can be any 8-bit string, but
  //!   should be the same across multiple instances on the same
  //!   domain, and over time.
  protected void create(void|string(8bit) realm, void|string(8bit) key) {
    this::realm = realm;
    if( key )
      hmac = Crypto.SHA1.HMAC(key);
  }

  protected string(7bit) make_mac(string nonce) {
    return MIME.encode_base64(hmac(nonce))[..<1];
  }

  //! Creates a challenge header value for the WWW-Authenticate header
  //! in 401 responses.
  string(7bit) challenge() {
    // MD5 has a digest size of 16 bytes. Go for the nearest multiple
    // of 3 above to utilize base64 lenght fully.
    string nonce = MIME.encode_base64(random_string(18));
    string ret = sprintf("Digest realm=\"%s\", qop=\"auth\", "
                         "nonce=\"%s\", algorithm=" + this->algorithm,
                         realm, nonce);
    if( hmac )
      ret += sprintf(", opaque=\"%s\"", make_mac(nonce));

    return ret;
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
  Concurrent.Future get_hashed_password(string user) {
    Concurrent.Promise ret = Concurrent.Promise();
    get_password(user)->then(lambda(string password) {
        if( !password )
          ret->failure( sprintf("No user %O", user) );
        else {
          ret->success( hash(user, realm, password) );
        }
      },
      lambda(string msg) {
        ret->failure(msg);
      });
    return ret->future();
  }

  //! Authenticate a request.
  //!
  //! @param hdr
  //!   The value of the Authorization header. Zero is acceptable, but
  //!   will produce an unconditional rejection.
  //!
  //! @param method
  //!   This is the HTTP method used, typically "GET" or "POST".
  //!
  //! @param path
  //!   This is the path of the request.
  Concurrent.Future auth(string hdr, string method, string path) {

    if(!hdr) return Concurrent.reject("Missing authenticate header");
    if(!method) return Concurrent.reject("Missing method");
    if(!path) return Concurrent.reject("Missing path");

    if( sscanf(hdr, "Digest %s", hdr)!=1 )
      return Concurrent.reject("Not a Digest header.");

    mapping resp = split_header(hdr);

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

    // A more sophisticated implementation would do this check after
    // user lookup and have the server respond with stale="true".
    if( hmac && resp->opaque && make_mac(nonce) != resp->opaque )
      return Concurrent.reject("Wrong nonce used.");

    Concurrent.Promise p = Concurrent.Promise();
    get_hashed_password(username)->then(lambda(string ha1) {
        if( !ha1 )
          return p->failure("Failed to resolve hashed password.");

        if( has_suffix(this->algorithm, "-sess") ) {
          ha1 = hash(ha1, nonce, resp->cnonce);
        }

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

//! HTTP Digest server implementation using MD5.
class DigestMD5Server {
  inherit DigestServer;
  inherit DigestMD5;
}

//! Implements the session version "MD5-sess" of the MD5 HTTP Digest
//! authentication. Used identically to @[DigestMD5Server].
class DigestMD5sessServer
{
  inherit DigestMD5Server;
  constant algorithm = "MD5-sess";
}

//! HTTP Digest server implementation using SHA256.
class DigestSHA256Server {
  inherit DigestServer;
  inherit DigestSHA256;
}

//! Implements the session version "SHA256-sess" of the SHA256 HTTP
//! Digest authentication. Used identically to @[DigestSHA256Server].
class DigestSHA256sessServer
{
  inherit DigestSHA256Server;
  constant algorithm = "SHA-256-sess";
}

//! HTTP Digest server implementation using SHA512/256.
class DigestSHA512256Server {
  inherit DigestServer;
  inherit DigestSHA512256;
}

//! Implements the session version "SHA-512-256-sess" of the SHA512/256 HTTP
//! Digest authentication. Used identically to @[DigestSHA512256Server].
class DigestSHA512256sessServer
{
  inherit DigestSHA512256Server;
  constant algorithm = "SHA-512-256-sess";
}

//! Abstract Client class.
class Client {
  protected void create(string hdr, string user, string password,
                        void|string realm);
  string(7bit) auth(string method, string path);
}

//! Create an authenticator for a server responding with the given
//! HTTP authentication header. Currently only works for one realm.
//!
//! @param hdrs
//!   The WWW-Authenticate HTTP header or headers.
//!
//! @param user
//!   The username to use.
//!
//! @param password
//!   The plaintext password.
//!
//! @param realm
//!   Optionally the realm the user and password is valid in. If
//!   omitted, the authentication will happen in whatever realm the
//!   server is presenting.
Client make_authenticator(string|array(string) hdrs,
                          string user, string password,
                          void|string realm) {
  if( !arrayp(hdrs) )
    hdrs = ({ hdrs });

  foreach(hdrs;; string hdr) {
    string type;
    sscanf(hdr, "%s%*[ ]%s", type, hdr);
    mapping auth = split_header(hdr);
    switch(type) {
    case "Basic":
      return BasicClient(auth, user, password, realm);

    case "Digest":
      if( auth->qop ) {
        // No implementation of auth-int yet.
        int found = 0;
        foreach(auth->qop/",";; string qop) {
          if( String.trim(qop)=="auth" ) {
            found = 1;
            break;
          }
        }
        if( !found ) continue;
      }
      switch( auth->algorithm ) {
      case 0:
      case "MD5":
        return DigestMD5Client(auth, user, password, realm);
      case "SHA-256":
        return DigestSHA256Client(auth, user, password, realm);
      case "SHA-512-256":
        return DigestSHA512256Client(auth, user, password, realm);
      }
    }
  }
}

//! HTTP Basic authentication client.
class BasicClient {

  string realm;
  string creds;

  protected void create(mapping hdr, string user, string password,
                        void|string realm) {
    if( !hdr || !user || !password )
      error("Missing argument.\n");

    if( !hdr->realm )
      error("Missing realm header argument.\n");
    if( realm && hdr->realm != realm )
      error("Realm mismatch %O!=%O\n", realm, hdr->realm);
    this::realm = hdr->realm;

    if( has_value(user, ":") )
      error("Illegal user name");
    creds = MIME.encode_base64(user+":"+password, 1);
  }

  string(7bit) auth(string method, string path) {
    return "Basic "+creds;
  }
}

//! Abstract HTTP Digest authentication client.
class DigestClient {
  inherit Digest;

  string user;
  string realm;
  string ha1;
  string nonce;
  string qop;
  string opaque;

  int counter = 1;
  int userhash = 0;

  protected void create(mapping hdr, string user, string password,
                        void|string realm) {
    if( !hdr || !user || !password )
      error("Missing argument.\n");

    if( !hdr->realm )
      error("Missing realm header argument.\n");
    if( realm && hdr->realm != realm )
      error("Realm mismatch %O!=%O\n", realm, hdr->realm);

    this::realm = realm = hdr->realm;

    nonce = hdr->nonce;
    qop = hdr->qop;
    opaque = hdr->opaque;

    if( hdr->charset) {
      if( hdr->charset!="UTF-8" )
        error("Unknown charset %O\n", hdr->charset);
      user = string_to_utf8(user);
    }

    ha1 = hash(user, realm, password);

    if( hdr->userhash=="true" ) {
      user = hash(user, realm);
      userhash = 1;
    }
    this::user = user;
  }

  // Should we quote the parameter. RFC 7616 section 3.5 lists some
  // parameters that MUST and some that MUST NOT be quoted.
  protected int(0..1) should_quote(string name, string value) {
    if( (< "nextonce", "rspauth", "cnonce" >)[name] )
      return 1;
    if( (< "qop", "nc" >)[name] )
      return 0;
    return has_value(value, ' ') || has_value(value, ',');
  }

  string(7bit)|zero auth(string method, string path, string|void cnonce) {

    if( !method || !path )
      return 0;

    string ha2 = hash(method, path);

    mapping response = ([
      "realm" : realm,
      "nonce" : nonce,
      "username" : user,
      "uri" : path,
      "algorithm" : this->algorithm,
    ]);
    if( opaque )
      response->opaque = opaque;
    if( userhash )
      response->userhash = "true";

    if( (< "auth", "auth-int" >)[qop] ) {
      // RFC 7616 3.5. nc MUST be exactly 8 characters.
      response->nc = sprintf("%08d", counter++);
      response->cnonce = cnonce || MIME.encode_base64(random_string(6));
      response->response = hash(ha1,nonce,response->nc,response->cnonce,qop,ha2);
      response->qop = qop;
    }
    else
      response->response = hash(ha1, nonce, ha2);

    String.Buffer buf = String.Buffer();
    foreach(response; string name; string value) {
      if( !sizeof(buf) )
        buf->add("Digest ");
      else
        buf->add(",");
      if( should_quote(name, value) )
        buf->add(name, "=\"", value, "\"");
      else
        buf->add(name, "=", value);
    }
    return (string)buf;
  }
}

//! HTTP Digest authentication client using MD5.
class DigestMD5Client {
  inherit DigestMD5;
  inherit DigestClient;
}

//! HTTP Digest authentication client using SHA256.
class DigestSHA256Client {
  inherit DigestSHA256;
  inherit DigestClient;
}

//! HTTP Digest authentication client using SHA512/256.
class DigestSHA512256Client {
  inherit DigestSHA512256;
  inherit DigestClient;
}
