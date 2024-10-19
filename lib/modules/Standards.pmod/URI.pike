#charset iso-8859-1
#pike __REAL_VERSION__

//! This class implements URI parsing and resolving of relative references to
//! absolute form, as defined in @rfc{2396@} and @rfc{3986@}.

// Implemented by Johan Sundstr�m and Johan Sch�n.

#pragma strict_types

//! Scheme component of URI
string|zero scheme;

//! Authority component of URI (formerly called net_loc, from @rfc{2396@}
//! known as authority)
string|zero authority;

//! Path component of URI. May be empty, but not undefined.
string path = "";

//! Query component of URI. May be 0 if not present.
string|zero query;

//! The fragment part of URI. May be 0 if not present.
string|zero fragment;

//! Certain classes of URI (e.g. URL) may have these defined
string|zero host, user, password;

//! If no port number is present in URI, but the scheme used has a
//! default port number, this number is put here.
int port;

//! The base URI object, if present
this_program|zero base_uri;

// URI hacker docs:
// This string is the raw uri the object was instantiated from in the
// first place. We save it here for the sole purpose of being able to
// replace the base URI, hence also needing to reresolve all of our
// properties with respect to that change.
string raw_uri = "";

#ifdef STANDARDS_URI_DEBUG
#define DEBUG(X, Y ...) werror("Standards.URI: "+X+"\n", Y)
#else
#define DEBUG(X, Y ...)
#endif

// FIXME: What about decoding of Percent-Encoding (RFC3986 2.1)?
//        cf pct-encoded in the functions below.

// Parse authority component (according to RFC 1738, � 3.1)
// Updated to RFC 3986 $ 3.2.
// NOTE: Censors the userinfo from the @[authority] variable.
protected void parse_authority()
{
  string host_port = [string]authority;
  // authority   = [ userinfo "@" ] host [ ":" port ]
  array(string) a = host_port/"@";
  if (sizeof(a) > 1) {
    host_port = a[-1];
    string userinfo = a[..<1] * "@";
    // userinfo    = *( unreserved / pct-encoded / sub-delims / ":" )
    sscanf(userinfo, "%[^:]:%s", user, password); // user info present
    DEBUG("parse_authority(): user=%O, password=%O", user, password);
  }
  if(scheme)
    port = Protocols.Ports.tcp[scheme]; // Set a good default � la RFC 1700
  // host        = IP-literal / IPv4address / reg-name
  if (has_prefix(host_port, "[")) {
    // IP-literal = "[" ( IPv6address / IPvFuture  ) "]"
    // IPvFuture  = "v" 1*HEXDIG "." 1*( unreserved / sub-delims / ":" )
    sscanf(host_port, "[%s]%*[:]%d", host, port);
  } else {
    // IPv4address = dec-octet "." dec-octet "." dec-octet "." dec-octet
    // reg-name    = *( unreserved / pct-encoded / sub-delims )
    sscanf(host_port, "%[^:]%*[:]%d", host, port);
  }
  DEBUG("parse_authority(): host=%O, port=%O", host, port);
}

// Inherit all properties except raw_uri and base_uri from the URI uri. :-)
protected void inherit_properties(this_program uri)
{
  sprintf_cache = ([]);
  authority = uri->authority;
  scheme = uri->scheme;
  user = uri->user; password = uri->password;
  host = uri->host; query = uri->query;
  port = uri->port;
  path = uri->path; fragment = uri->fragment;
}

//! Compare this URI to something, in a canonical way.
//! @param something
//!   Compare the URI to this
protected int `==(mixed something)
{
    if( !objectp( something ) || object_program(something) < this_program )
        return false;
    Standards.URI other = [object(Standards.URI)]something;
    // For protocols with host/port/user/password we do lower_case on
    // the host when comparing, and use the port according to RFC 2396
    // section 6.
    return
        ((host
          && other->host
          && lower_case(other->host) == lower_case(host)
          && other->port == port
          && other->user == user
          && other->password == password)
         || (other->authority == authority))
        && other->path == path
        && other->query == query
        && other->scheme == scheme
        && other->fragment == fragment;
}

string combine_uri_path(string base, string rel, int(0..1)|void is_abs_path)
{
  string buf = rel;
  array(string) segments = ({});

  // RFC 2396, �5.2.5:
  //    If the path component begins with a slash character ("/"),
  //    then the reference is an absolute-path and we skip to step 7.
  //
  // NB: The RFC does not take into account that even absolute
  //     paths may contain segments of ".." and ".", and this
  //     function may get called by external code that wants
  //     to get rid of them. We simply ignore the base URI's
  //     path if rel is absolute.
  if (!has_prefix(rel, "/")) {
    // RFC 2396, �5.2.6:
    // a) All but the last segment of the base URI's path component is
    //    copied to the buffer.  In other words, any characters after the
    //    last (right-most) slash character, if any, are excluded.
    segments = base/"/";
    if ((sizeof(segments) > 1) || is_abs_path) {
      buf = segments[..<1]*"/"+"/";
    } else {
      buf = "";
    }

    // b) The reference's path component is appended to the buffer string.
    buf += rel;
  }
  segments = buf / "/";

  // c) All occurrences of "./", where "." is a complete path segment,
  //    are removed from the buffer string.
  array(string|zero) tmp = segments;
  for(int i=0; i<sizeof(tmp)-1; i++)
    if(tmp[i]==".")
      tmp[i]=0;

  segments = [array(string)](tmp - ({ 0 }));

  // d) If the buffer string ends with "." as a complete path segment,
  //    that "." is removed.
  if(segments[-1]==".")
    segments=segments[..<1]+({""});

  // e) All occurrences of "<segment>/../", where <segment> is a
  //    complete path segment not equal to "..", are removed from the
  //    buffer string.  Removal of these path segments is performed
  //    iteratively, removing the leftmost matching pattern on each
  //    iteration, until no matching pattern remains.
  int found_pattern;
  do
  {
    found_pattern=0;
    if(sizeof(segments)<3)
      continue;
    for(int i=0; i<sizeof(segments)-2; i++)
    {
      if(segments[i]!=".." && segments[i]!="" && segments[i+1]=="..")
      {
	segments = segments[..i-1]+segments[i+2..];
	found_pattern=1;
	continue;
      }
    }

  } while(found_pattern);

  // f) If the buffer string ends with "<segment>/..", where <segment>
  //    is a complete path segment not equal to "..", that
  //    "<segment>/.." is removed.
  if(sizeof(segments)>=2)
    if(segments[-2]!=".." && segments[-1]=="..")
      segments=segments[..<2]+({""});

  // g) If the resulting buffer string still begins with one or more
  //    complete path segments of "..", then the reference is
  //    considered to be in error.  Implementations may handle this
  //    error by retaining these components in the resolved path (i.e.,
  //    treating them as part of the final URI), by removing them from
  //    the resolved path (i.e., discarding relative levels above the
  //    root), or by avoiding traversal of the reference.
  segments -= ({ ".." });
  return segments * "/";
}


//! @decl void reparse_uri()
//! @decl void reparse_uri(URI base_uri)
//! @decl void reparse_uri(string base_uri)
//! Reparse the URI with respect to a new base URI. If
//! no base_uri was supplied, the old base_uri is thrown away.
//! The resolving is performed according to the guidelines
//! outlined by @rfc{2396@}, Uniform Resource Identifiers (URI): Generic Syntax.
//! @param base_uri
//!   Set the new base URI to this.
//! @throws
//!   An exception is thrown if the @[uri] is a relative URI or only a
//!   fragment, and missing a @[base_uri].
void reparse_uri(this_program|string|void base_uri)
{
  string uri = raw_uri;
  if(stringp(base_uri))
  {
    DEBUG("cloning base URI %O", base_uri);
    this::base_uri = this_program([string]base_uri); // create a new URI object
  }
  else
    this::base_uri = [object(this_program)]base_uri;

  // RFC 2396, �5.2:
  // 1) The URI reference is parsed into the potential four components and
  //    fragment identifier, as described in Section 4.3.
  // URI         = scheme ":" hier-part [ "?" query ] [ "#" fragment ]

  // 2) If the path component is empty and the scheme, authority, and
  //    query components are undefined, then it is a reference to the
  //    current document and we are done.  Otherwise, the reference URI's
  //    query and fragment components are defined as found (or not found)
  //    within the URI reference and not inherited from the base URI.
  //    (Doing this at once saves us some useless parsing efforts.)
  if((!uri || uri == "") && this::base_uri)
  {
    DEBUG("Path is empty -- Inherit entire base URI "
	  "as per RFC 2396, �5.2 step 2. Done!");
    inherit_properties(this::base_uri);
    return;
  }

  // Parse fragment identifier
  // fragment    = *( pchar / "/" / "?" )
  // pchar       = unreserved / pct-encoded / sub-delims / ":" / "@"
  if( sscanf(uri, "%s#%s", uri, fragment)==2 )
  {
    DEBUG("Found fragment %O", fragment);
    if( !sizeof(uri) )
    {
      DEBUG("Fragment only. Using entire base URI, except fragment.");
      if( !this::base_uri )
        error("fragment only URI lacking base URI.\n");
      string f = [string]fragment;
      inherit_properties(this::base_uri);
      fragment = f;
      return;
    }
  }

  // Parse scheme
  // scheme      = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
  if(sscanf(uri, "%[A-Za-z0-9+.-]:%s", scheme, uri) < 2)
  {
    scheme = 0;
    if(!this::base_uri)
	error("Standards.URI: got a relative URI (no scheme) lacking a base_uri!\n");
  } else {
    /* RFC 3986 �3.1
     *
     * An implementation should accept uppercase letters as equivalent
     * to lowercase in scheme names (e.g., allow "HTTP" as well as
     * "http") for the sake of robustness but should only produce
     * lowercase scheme names for consistency.
     */
    scheme = lower_case([string]scheme);
  }
  DEBUG("Found scheme %O", scheme);

  // DWIM for "www.cnn.com" style input, when parsed in the context of
  // base "http://".
  if( !has_prefix(uri, "//") && !scheme && this::base_uri->?scheme &&
      this::base_uri->authority &&
      !sizeof(this::base_uri->authority) &&
      !sizeof(this::base_uri->path))
  {
    DEBUG("DWIM authority: %O\n", uri);
    uri = "//"+uri;
  }

  // Parse authority/login
  //
  // hier-part    = "//" authority path-abempty / path-absolute
  //                / path-rootless / path-empty
  if(sscanf(uri, "//%[^/]%s", authority, uri))
  {
    DEBUG("Found authority %O", authority);
    int q = search(authority, "?", search(authority, "@")+1);
    if (q >= 0) {
      // There's a question mark in the host and port section
      // of the authority. This happens when the path is empty
      // and there's a query part afterwards.
      // Example: http://foo?bar
      uri = authority[q..] + uri;
      authority = authority[..q-1];
      DEBUG("Adjusted authority %O", authority);
    }
  }

  // Parse query information
  // query       = *( pchar / "/" / "?" )
  // pchar       = unreserved / pct-encoded / sub-delims / ":" / "@"
  sscanf(uri, "%s?%s", uri, query);
  DEBUG("Found query %O", query);

  // Parse path:
  // pchar       = unreserved / pct-encoded / sub-delims / ":" / "@"
  if ((uri == "") && !scheme && !authority && (this::base_uri)) {
    // Empty path.
    path = this::base_uri->path;
  } else {
    path = uri;
  }
  DEBUG("Found path %O", path);

  // 3) If the scheme component is defined, indicating that the reference
  //    starts with a scheme name, then the reference is interpreted as an
  //    absolute URI and we are done.  Otherwise, the reference URI's
  //    scheme is inherited from the base URI's scheme component.
  if(scheme)
  {
    if(authority)
      parse_authority();

    DEBUG("Scheme found! RFC 2396, �5.2, step 3 "
	  "says we're absolute. Done!");
    return;
  }
  scheme = this::base_uri->scheme;
  DEBUG("Inherited scheme %O from base URI", scheme);

  if(authority)
    parse_authority();


  // 4) If the authority component is defined, then the reference is a
  //	network-path and we skip to step 7.  Otherwise, the reference
  //	URI's authority is inherited from the base URI's authority
  //	component, which will also be undefined if the URI scheme does not
  //	use an authority component.
  if(!authority || !sizeof(authority))
  {
    authority = this::base_uri->authority;
    DEBUG("Inherited authority %O from base URI", authority);
    if (authority)
      parse_authority();

    // 5) If the path component begins with a slash character ("/"), then
    //    the reference is an absolute-path and we skip to step 7.
    //
    // FIXME: What if it contains "." or ".." segments?
    // cf combine_uri_path() above.
    if(!has_prefix(path, "/"))
    {

      // 6) If this step is reached, then we are resolving a relative-path
      //    reference.  The relative path needs to be merged with the base
      //    URI's path.  Although there are many ways to do this, we will
      //    describe a simple method using a separate string buffer.

      DEBUG("Combining base path %O with path %O => %O",
	    this::base_uri->path, path,
	    combine_uri_path(this::base_uri->path, path,
			     !!this::base_uri->authority));
      path = combine_uri_path(this::base_uri->path, path,
			      !!this::base_uri->authority);

    }
  }

  // 7) The resulting URI components, including any inherited from the
  //    base URI, are recombined to give the absolute form of the URI reference.
  //    (Reassembly is done at cast-to-string/sprintf() time)
}


//! @decl void create(URI uri)
//! @decl void create(URI uri, URI base_uri)
//! @decl void create(URI uri, string base_uri)
//! @decl void create(string uri)
//! @decl void create(string uri, URI base_uri)
//! @decl void create(string uri, string base_uri)
//! @param base_uri
//!   When supplied, will root the URI a the given location. This is
//!   needed to correctly verify relative URIs, but may be left out otherwise.
//!   If left out, and uri is a relative URI, an error is thrown.
//! @param uri
//!   When uri is another URI object, the created
//!   URI will inherit all properties of the supplied uri
//!   except, of course, for its base_uri.
//! @throws
//!   An exception is thrown if the @[uri] is a relative URI or only a
//!   fragment, and missing a @[base_uri].
protected void create(this_program|string uri,
		      this_program|string|void base_uri)
{
  DEBUG("create(%O, %O) called!", uri, base_uri);
  sprintf_cache = ([]);
  if(stringp(uri))
    raw_uri = [string]uri; // Keep for future runs of reparse_uri after a base_uri change
  else if(objectp(uri)) // If uri is 0, we want to inherit from the base_uri.
    raw_uri = uri->raw_uri;

  reparse_uri(base_uri);
}

//! Assign a new value to a property of URI
//! @param property
//!   When any of the following properties are used, properties that
//!   depend on them are recalculated: user, password, host, port, authority, base_uri.
//! @param value
//!   The value to assign to @[property]
protected mixed `->=(string property, mixed value) {
  return `[]=(property, value);
}
protected mixed `[]=(string property, mixed value)
{
  DEBUG("`[]=(%O, %O)", property, value);
  sprintf_cache = ([]);
  switch(property)
  {
    case "user":
    case "password":
    case "host":
      if(!stringp(value) && value!=0)
	error("%s value not string.\n", property);
    case "port":
      ::`[]=(property, value);
      authority = (user ? user + (password ? ":" + password : "") + "@" : "") +
	(host?(has_value(host, ":")?("["+host+"]"):host):"") +
	(port != Protocols.Ports.tcp[scheme] ? ":" + port : "");
	return value;

    case "authority":
      if(!stringp(value) && value!=0)
	error("authority value not string.\n");
      authority = [string]value;
      parse_authority(); // Set user, password, host and port accordingly
      return value;

    case "base_uri":
      if(!stringp(value) && value!=0 && !objectp(value))
	error("base_uri value neither object nor string.\n");
      reparse_uri([object(this_program)|string]value);
      return base_uri;

    case "query":
      variables = 0;
      ::`[]=(property, value);
      return value;

    case "scheme":
      /* RFC 3986 �3.1
       *
       * An implementation should accept uppercase letters as equivalent
       * to lowercase in scheme names (e.g., allow "HTTP" as well as
       * "http") for the sake of robustness but should only produce
       * lowercase scheme names for consistency.
       */
      if(!stringp(value) && value!=0)
	error("scheme value not string.\n");
      value = lower_case([string]value);

      // FALL_THROUGH
    default:
      ::`[]=(property, value); // Set and return the new value
      return value;
  }
}

//! When cast to string, return the URI (in a canonicalized form).
//! When cast to mapping, return a mapping with scheme, authority,
//! user, password, host, port, path, query, fragment, raw_uri,
//! base_uri as documented above.
protected string|mapping cast(string to)
{
  switch(to)
  {
    case "string":
      return _sprintf('s');
    case "mapping":
      array(string) i = ({ "scheme", "authority", "user", "password",
			   "host", "port",
			   "path", "query", "fragment",
			   "raw_uri", "base_uri",  });
      return mkmapping(i, rows(this, i));
  }
  return UNDEFINED;
}

//! Returns path and query part of the URI if present.
string get_path_query()
{
  return (path||"") + (query ? "?" + query : "");
}

protected mapping(string:string)|zero variables;

//! Returns the query variables as a @expr{mapping(string:string)@}.
mapping(string:string) get_query_variables() {
  if( variables ) return [mapping]variables;
  if(!query) return ([]);

  variables = ([]);
  foreach( query/"&";; string pair )
  {
    if( sscanf( pair, "%s=%s", string var, string val )==2 )
      variables[var] = val;
    else
      variables[pair] = 0;
  }

  return [mapping]variables;
}

//! Sets the query variables from the provided mapping.
void set_query_variables(mapping(string:string) vars) {
  sprintf_cache = ([]);
  variables = vars;
  if(!sizeof(vars))
    query = 0;
  else
  {
    query = "";
    foreach( vars; string var; string val )
    {
      if( sizeof(query) )
        query += "&";
      query += var;
      if( val )
        query += "=" + val;
    }
  }
}

//! Adds the provided query variable to the already existing ones.
//! Will overwrite an existing variable with the same name.
void add_query_variable(string name, string value) {
  set_query_variables(get_query_variables()+([name:value]));
}

//! Appends the provided set of query variables with the already
//! existing ones. Will overwrite all existing variables with the same
//! names.
void add_query_variables(mapping(string:string) vars) {
  set_query_variables(get_query_variables()|vars);
}


// HTTP stuff

// RFC 1738, 2.2. URL Character Encoding Issues
protected constant url_non_corresponding = enumerate(0x21) +
  enumerate(0x81,1,0x7f);
// RFC 3986 2.2: gen-delims
protected constant url_gen_delims = ({
  ':', '/', '?', '#', '[', ']', '@',
});
// RFC 3986 2.2: sub-delims
protected constant url_sub_delims = ({
  '!', '$', '&', '\'', '(', ')',
  '*', '+', ',', ';', '=',
});
// RFC 3986 2.2: reserved
protected constant url_reserved = url_gen_delims + url_sub_delims;
protected constant rfc1738_url_unsafe = ({
  // RFC 1738 5: punctuation
  '%', '"',
  // RFC 1738 5: national
  '{', '}', '|', '\\', '^', '~', '`',
}) - ({
  /* RFC 3986 2.3:
   *   For consistency, percent-encoded octets in the ranges of ALPHA
   *   (%41-%5A and %61-%7A), DIGIT (%30-%39), hyphen (%2D), period
   *   (%2E), underscore (%5F), or tilde (%7E) should not be created
   *   by URI producers and, when found in a URI, should be decoded to
   *   their corresponding unreserved characters by URI normalizers.
   */
  '~',
});

// Encode these chars
protected constant url_chars = url_reserved + rfc1738_url_unsafe +
  url_non_corresponding;
protected constant url_from = sprintf("%c", url_chars[*]);
protected constant url_to   = sprintf("%%%02x", url_chars[*]);

string http_encode(string in)
{
  // We shouldn't really have to soft case here. Bug(ish) in constant
  // type generation...
  return replace(in, url_from, url_to);
}

//! Return the query part, coded according to @rfc{1738@}, or zero.
string|zero get_http_query() {
  return query;
}

//! Return the path and query part of the URI, coded according to
//! @rfc{1738@}.
string get_http_path_query() {
  string|zero q = get_http_query();
  return http_encode(((path||"")/"/")[*])*"/" + (q?"?"+q:"");
}

protected int __hash() { return hash_value(_sprintf('x')); }

private mapping(int:string) sprintf_cache = ([]);
protected string _sprintf(int how, mapping|void args)
{
  if( how == 't' ) return "Standards.URI";
  if( string res = sprintf_cache[how] )
      return res;
  string|zero _host = host;
  if(how == 'x' && _host)
      _host = lower_case([string]_host);
  string getstring = (path||"") + (query ? "?" + query : "");
  if(args && args->flag_left)
      return getstring;
  string look =
      (scheme?(scheme + ":"):"") +
      (authority ?
       "//" +
       (user ? user + (password ? ":" + password : "") + "@" : "") +
       (_host?(has_value(_host, ":")?("["+_host+"]"):_host):"") +
       (port != Protocols.Ports.tcp[scheme] ? ":" + port : "") : ("")) +
      getstring +
      (fragment ? "#" + fragment : "");

  if(how == 'O')
      look=sprintf("URI(%q)", look);
  return sprintf_cache[how]=look;
}

// Master codec API function. Allows for serialization with
// encode_value.
mapping(string:string|int|this_program) _encode()
{
#define P(X) #X:X
    return ([
        P(scheme),
        P(authority),
        P(path),
        P(query),
        P(fragment),
        P(host),
        P(user),
        P(password),
        P(port),
        P(base_uri),
        P(raw_uri),
        // variables is only a cache
    ]);
#undef P
}

// Master codec API function. Allows for deserialization with
// decode_value.
void _decode(mapping(string:mixed) m)
{
    foreach(m; string index; mixed value)
      ::`[]=(index, value);
}

#if 0
// Not used yet.
string quote(string s)
{
  return replace(s,
		 ({ "\000", "\001", "\002", "\003", "\004", "\005", "\006",
		    "\007", "\010", "\011", "\012", "\013", "\014", "\015",
		    "\016", "\017", "\020", "\021", "\022", "\023", "\024",
		    "\025", "\026", "\027", "\030", "\031", "\032", "\033",
		    "\034", "\035", "\036", "\037", "\200", "\201", "\202",
		    "\203", "\204", "\205", "\206", "\207", "\210", "\211",
		    "\212", "\213", "\214", "\215", "\216", "\217", "\220",
		    "\221", "\222", "\223", "\224", "\225", "\226", "\227",
		    "\230", "\231", "\232", "\233", "\234", "\235", "\236",
		    "\237", " ", "%", "'", "\"" }),
		 ({ "%00", "%01", "%02", "%03", "%04", "%05", "%06", "%07",
		    "%08", "%09", "%0A", "%0B", "%0C", "%0D", "%0E", "%0F",
		    "%10", "%11", "%12", "%13", "%14", "%15", "%16", "%17",
		    "%18", "%19", "%1A", "%1B", "%1C", "%1D", "%1E", "%1F",
		    "%80", "%81", "%82", "%83", "%84", "%85", "%86", "%87",
		    "%88", "%89", "%8A", "%8B", "%8C", "%8D", "%8E", "%8F",
		    "%90", "%91", "%92", "%93", "%94", "%95", "%96", "%97",
		    "%98", "%99", "%9A", "%9B", "%9C", "%9D", "%9E", "%9F",
		    "%20", "%25", "%27", "%22"}));
}
#endif
