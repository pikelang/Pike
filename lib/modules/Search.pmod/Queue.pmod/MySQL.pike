#pike __REAL_VERSION__

inherit .Base;

//! @[Search] crawler state stored in a @[Mysql] database.

string url, table;

protected Thread.Local _db = Thread.Local();
Sql.Sql `db()
{
  // NB: We need to have a thread local connection,
  //     since the status functions may get called
  //     from some other thread while we're busy
  //     performing sql queries elsewhere.
  Sql.Sql ret = _db->get();
  if (ret && !ret->ping()) return ret;
  return _db->set(Sql.Sql( url ));
}

Web.Crawler.Stats stats;
Web.Crawler.Policy policy;
Web.Crawler.RuleSet allow, deny;

protected string to_md5(string url)
{
  return String.string2hex(Crypto.MD5.hash(string_to_utf8(url)));
}

//! @param _url
//!   @[Sql.Sql] URL for the database to store the queue.
//!
//! @param _table
//!   @[Sql.Sql] table name to store the queue in.
//!
//!   If the table doesn't exist it will be created.
protected void create( Web.Crawler.Stats _stats,
		       Web.Crawler.Policy _policy,

		       string _url, string _table,

		       void|Web.Crawler.RuleSet _allow,
		       void|Web.Crawler.RuleSet _deny)
{
  stats = _stats;   policy = _policy;
  allow=_allow;     deny=_deny;
  table = _table;
  url = _url;

  perhaps_create_table(  );
}

protected void perhaps_create_table(  )
{
  db->query(
#"
    create table IF NOT EXISTS "+table+#" (
        uri        blob not null,
        uri_md5    char(32) not null default '',
	template   varchar(255) not null default '',
	md5        char(32) not null default '',
	recurse    tinyint not null,
	stage      tinyint not null,
	UNIQUE(uri_md5),
	INDEX stage   (stage),
	INDEX uri     (uri(255))
	)
    ");
  if (!((multiset)db->query("SHOW INDEX FROM " + table)->Key_name)["uri"]) {
    db->query("ALTER TABLE " + table +
	      "  ADD INDEX uri (uri(255))");
  }
}

protected mapping hascache = ([]);

void clear_cache()
{
  hascache = ([]);
}

protected int has_uri( string|Standards.URI uri )
{
  uri = (string)uri;
  if( sizeof(hascache) > 100000 )  hascache = ([]);
  return hascache[uri]||
    (hascache[uri]=
     sizeof(db->query("select stage from "+table+" where uri_md5=%s",
		      to_md5(uri))));
}

void add_uri( Standards.URI uri, int recurse, string|zero template,
	      void|int force )
{
  // The language is encoded in the fragment.
  Standards.URI r = Standards.URI( (string)uri );
  if( r->query )         r->query = normalize_query( r->query );
  if(r->query && !strlen(r->query))  r->query = 0;

    // Remove any trailing index filename

  string rpath=reverse(r->path);
  // FIXME: Make these configurable?
  foreach( ({"index.xml", "index.html", "index.htm"}),
	   string index)
    if(search(rpath,reverse(index))==0 && rpath[sizeof(index)]=='/')
      rpath=rpath[sizeof(index)..];
  r->path=reverse(rpath);

  r->path = combine_path(r->path);

  if( force || check_link(uri, allow, deny) )
  {
    if(has_uri(r))
    {
      // FIXME:
      // Race condition:
      // If a url is forced to be indexed *while* it's being indexed,
      // and it's changed since the indexing started, setting the stage
      // to 0 here might be worthless, since it could be overwritten before
      // it's fetched again.
      if(force) {
	set_stage(r, 0);
	set_recurse(r, recurse);
      }
    }
    else
      // There's a race condition between the select query in has_uri()
      // and this query, so we ignore duplicate key errors from MySQL
      // by using the "ignore" keyword.
      db->query( "insert ignore into "+table+
		 " (uri,uri_md5,recurse,template) values (%s,%s,%d,%s)",
		 string_to_utf8((string)r),
		 to_md5((string)r), recurse, (template||"") );
  }
}

void set_md5( Standards.URI uri, string md5 )
{
  if( extra_data[(string)uri] )
    extra_data[(string)uri]->md5 = md5;
  db->query( "update "+table+
	     " set md5=%s WHERE uri_md5=%s", md5, to_md5((string)uri) );
}

void set_recurse( Standards.URI uri, int recurse )
{
  if( extra_data[(string)uri] )
    extra_data[(string)uri]->recurse = (string)recurse;
  db->query( "update "+table+
	     " set recurse=%d WHERE uri_md5=%s", recurse, to_md5((string)uri));
}

mapping(string:mapping(string:string)) extra_data = ([]);
mapping get_extra( Standards.URI uri )
{
  if( extra_data[(string)uri] )
    return extra_data[(string)uri] || ([ ]);
  array r = db->query( "SELECT md5,recurse,stage,template "
		       "FROM "+table+" WHERE uri_md5=%s", to_md5((string)uri) );
  return (sizeof(r) && r[0]) || ([ ]);
}

protected int empty_count;
protected int retry_count;

// cache, for performance reasons.
protected array possible=({});
protected int p_c;

int|Standards.URI get()
{
  if(stats->concurrent_fetchers() > policy->max_concurrent_fetchers)
    return -1;

  if( sizeof( possible ) <= p_c )
  {
    p_c = 0;
    possible = db->query( "select * from "+table+" where stage=0 limit 20" );
    extra_data = mkmapping( map(possible->uri,utf8_to_string), possible );
    possible = map(possible->uri,utf8_to_string);
  }

  while( sizeof( possible ) > p_c )
  {
    empty_count=0;
    if( possible[ p_c ] )
    {
      Standards.URI uri = Standards.URI( possible[p_c++] );

      if( stats->concurrent_fetchers( uri->host ) >
	  policy->max_concurrent_fetchers_per_host )
      {
	retry_count++;
	continue; // not this host..
      }
      possible[p_c-1] = 0;
      retry_count=0;
      set_stage( uri, 1 );
      return uri;
    }
    p_c++;
    continue;
  }

  if( stats->concurrent_fetchers() )
  {
    return -1;
  }

  // This is needed for the following race condition scenario:
  //   1.  The queue contains one page
  //   2.  The crawler indexes the page
  //   3a. In thread/process A, document filtering and fetching is done, and
  //       links are found
  //   3b. In thread/process B, queue->get() returns 0 since the queue doesn't contain
  //       any more pages to crawl.
  //
  // The workaround is to wait 40 cycles (i.e. 4 seconds) after fetching the last page.
  if( empty_count++ > 40 )
  {
    if( num_with_stage( 2 ) || num_with_stage( 3 ) )
    {
      empty_count=0;
      return -1;
    }
    return 0;
  }
  return -1;
}

array(Standards.URI) get_uris(void|int stage)
{
  array uris = ({});
  if (stage)
    uris = db->query( "select * from "+table+" where stage=%d", stage );
  else
    uris = db->query( "select * from "+table );
  uris = map(uris->uri, utf8_to_string);
  uris = map(uris, Standards.URI);

  return uris;
}

//! @returns
//!   Returns an array with all URI schemes currently used in the queue.
array(string) get_schemes()
{
  // FIXME: Consider using SUBSTRING_INDEX().
  array(string) schemes =
    db->query("SELECT DISTINCT"
	      "       SUBSTRING(uri, 1, 20) AS scheme"
	      "  FROM "+table)->scheme;
  schemes = map(schemes,
		lambda(string s) {
		  return (s/":")[0];
		});
  return Array.uniq(sort(schemes));
}

void put(string|array(string)|Standards.URI|array(Standards.URI) uri)
{
  if(arrayp(uri))
  {
    foreach(uri, string|object _uri)
      put(_uri);
    return;
  }
  if(!objectp(uri))
    uri=Standards.URI(uri);

  add_uri( uri, 1, 0 );
}

void clear()
{
  hascache = ([ ]);
  db->query("delete from "+table);
}

void remove_uri(string|Standards.URI uri)
{
  hascache[(string)uri]=0;
  db->query("delete from "+table+" where uri_md5=%s", to_md5((string)uri));
}

void remove_uri_prefix(string|Standards.URI uri)
{
  string uri_string = (string)uri;
  foreach(indices(hascache), string _uri)
    if(has_prefix(_uri, uri_string))
      hascache[_uri]=0;

  db->query("delete from "+table+" where uri like '" + db->quote(uri_string) + "%%'");
}

void clear_stage( int ... stages )
{
  foreach( stages, int s )
    db->query( "update "+table+" set stage=0 where stage=%d", s );
}

void remove_stage (int stage)
{
  db->query( "delete from "+table+" where stage=%d", stage );
}

void clear_md5( int ... stages )
{
  foreach( stages, int s )
    db->query( "update "+table+" set md5='' where stage=%d", s );
}

int num_with_stage( int ... stage )
{
  return (int)
    db->query( "select COUNT(*) as c from "+table+" where stage IN (%s)",
	       ((array(string))stage)*"," )[ 0 ]->c;
}

void set_stage( Standards.URI uri,
		int stage )
{
  db->query( "update "+table+" set stage=%d where uri_md5=%s",stage,
	     to_md5((string)uri));
}

//! @returns
//!   Returns the current stage for the specified URI.
//!
//! @seealso
//!   @[set_stage()]
int get_stage( Standards.URI uri )
{
  array a = db->query( "select stage from "+table+" where uri_md5=%s", to_md5((string)uri));
  if(sizeof(a))
    return (int)a[0]->stage;
  else
    return -1;
}

//! Reset the stage to @expr{0@} (zero) for all URIs with the specified
//! @[uri_prefix]. If no @[uri_prefix] is specified reset the stage for
//! all URIs.
void reset_stage(string|void uri_prefix)
{
  if (uri_prefix) {
    db->query("UPDATE " + table + " SET stage = 0"
	      " WHERE SUBSTRING(uri, 1, " + sizeof(uri_prefix) + ") = %s",
	      uri_prefix);
  } else {
    db->query("UPDATE " + table + " SET stage = 0");
  }
}
