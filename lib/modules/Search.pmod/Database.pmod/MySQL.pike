// This file is part of Roxen Search
// Copyright © 2000,2001 Roxen IS. All rights reserved.
//
// $Id: MySQL.pike,v 1.48 2001/07/04 20:46:32 nilsson Exp $

inherit .Base;

// Creates the SQL tables we need.

//#define DEBUG
void init_tables()
{
  db->query(
#"create table if not exists uri (id          int unsigned primary key
                                     auto_increment not null,
                         uri         blob not null,
                         uri_md5     varchar(32) binary not null,
                         UNIQUE(uri_md5))"
			 );

  db->query(
#"create table if not exists document (id            int unsigned primary key
			               auto_increment not null,
                         uri_id        int unsigned not null,
                         language char(3) default null,
                         INDEX index_language (language),
                         INDEX index_uri_id (uri_id))"
			 );
  
  db->query("create table if not exists deleted_document (doc_id int unsigned not null)");

  db->query(
#"create table if not exists word_hit (word        varchar(64),
                         first_doc_id   int not null,
            	         hits           mediumblob not null,
                         unique(word(8),first_doc_id))");

  db->query(
#"create table if not exists metadata (doc_id        int not null,
                         name          varchar(32) not null,
            	         value         mediumblob not null,
                         unique(doc_id,name))");

  db->query(
#"create table if not exists field (id    tinyint unsigned primary key not null,
                      name  varchar(127) not null,
                      UNIQUE(name))");

}

// This is the database object that all queries will be made to.
static object db;
static string host;

static int init_done = 0;

static void init_fields()
{
  if(init_done)
    return;

  init_done=1;
  foreach(({"uri"})+Search.get_filter_fields(), string field)
    allocate_field_id(field);
}

void create(string db_url)
{
  db=Sql.sql(host=db_url);
}

string _sprintf()
{
  return sprintf("Search.Database.MySQL(%O)", host);
}


static string to_md5(string url)
{
  object md5 = Crypto.md5();
  md5->update(url);
  return Crypto.string_to_hex(md5->digest());
}

function hash_word = hash;

int get_uri_id(string uri, void|int do_not_create)
{
  string s=sprintf("select id from uri where uri_md5='%s'", to_md5(uri));
  array a=db->query(s);
  if(sizeof(a))
    return (int)a[0]->id;

  if(do_not_create)
    return 0;

  db->query("insert into uri (uri,uri_md5) "
	    "values (%s,%s)",
	    uri, to_md5(uri));
  db->query(s);
  return db->master_sql->insert_id();
}

int get_document_id(string uri, void|string language)
{
  int uri_id=get_uri_id(uri);
  
  string s=sprintf("select id from document where "
		   "uri_id='%d'", uri_id);
  if(language)
    s+=sprintf(" and language='%s'",db->quote(language));

  array a = db->query(s);
  if(sizeof(a))
    return (int)a[0]->id;

  s=sprintf("insert into document (uri_id, language) "
	    "values ('%d',%s)",
	    uri_id,
	    language?("'"+language+"'"):"NULL");

  db->query(s);
  return db->master_sql->insert_id();
}

static mapping(string:int) list_fields_cache;

mapping(string:int) list_fields()
{
  if(list_fields_cache)
    return list_fields_cache;
  init_fields();
  array a=db->query("select name,id from field") + ({ (["name":"body",
							"id": "0"]) });
  return list_fields_cache=mkmapping(a->name, (array(int))a->id);
}

int allocate_field_id(string field)
{
  if(!init_done)
    init_fields();
  if(field=="body")
    return 0;
  db->query("lock tables field write");
  for(int i=1; i<64; i++)
  {
    array a=db->query("select name from field where id=%d",i);
    if(!sizeof(a))
    {
      a=db->query("replace into field (id,name) values (%d,%s)",
		  i, field);
      list_fields_cache=0;
      db->query("unlock tables");
      return i;
    }
  }
  db->query("unlock tables");
  return -1;
}

static mapping field_cache = ([]);
int get_field_id(string field, void|int do_not_create)
{
  // The one special case.
  if(field=="body")      return 0;
  if(field_cache[field]) return field_cache[field];
  
  init_fields();
  string s=sprintf("select id from field where name='%s'",db->quote(field));
  array a=db->query(s);
  if(sizeof(a))
  {
    field_cache[field]=(int)a[0]->id;
    return (int)a[0]->id;
  }

  if(do_not_create)
    return -1;

  return allocate_field_id(field);
}

void remove_field(string field)
{
  init_fields();
  m_delete(field_cache, field);
  list_fields_cache=0;
  db->query("delete from field where name=%s", field);
}

static _WhiteFish.Blobs blobs = _WhiteFish.Blobs();

#define MAXMEM 20*1024*1024


//! Inserts the words of a resource into the database
void insert_words(Standards.URI|string uri, void|string language,
		  string field, array(string) words)
{
  if(!sizeof(words))  return;
  init_fields();

  int doc_id   = get_document_id((string)uri, language);
  int field_id = get_field_id(field);

  blobs->add_words( doc_id, words, field_id, 0 );

  if(blobs->memsize() > MAXMEM)
    sync();
}

void set_metadata(Standards.URI|string uri, void|string language,
			 mapping(string:string) md)
{
  int doc_id;
  if(!intp(uri))
    doc_id = get_document_id((string)uri, language);

  init_fields();

  // Still our one, single special case
  if(md->body)
    md->body = Gz.deflate(6)->deflate(string_to_utf8(md->body[..64000]),
				      Gz.FINISH);

  if(!sizeof(md))
    return 0;

  foreach(indices(md), string ind)
    if(ind!="body")
      md[ind]=string_to_utf8(md[ind]);

  string s=map(Array.transpose( ({ map(indices(md),db->quote),
				   map(values(md), db->quote) }) ),
	       lambda(array a)
	       {
		 return sprintf("(%d,'%s','%s')", doc_id,
				a[0], a[1]);
	       }) * ", ";
  
  db->query("replace into metadata (doc_id, name, value) values "+s);
}

mapping(string:string) get_metadata(int|Standards.URI|string uri,
				    void|string language,
				    void|array(string) wanted_fields)
{
  int doc_id;
  if(intp(uri))
    doc_id=uri;
  else
    doc_id = get_document_id((string)uri, language);
  string s="";
  if(wanted_fields && sizeof(wanted_fields))
    s=" and name IN ('"+map(wanted_fields,db->quote)*"','"+"')";
			      
  array a=db->query("select name,value from metadata where doc_id=%d"+s,
		    doc_id);
  mapping md=mkmapping(a->name,a->value);
  if(md->body)
    md->body=utf8_to_string(Gz.inflate()->inflate(md->body));
  return md;
}

mapping get_uri_and_language(int doc_id)
{
  array a=db->query("select document.language,uri.uri from document,uri "
		    "where uri.id=document.uri_id and document.id=%d",doc_id);
  if(!sizeof(a))
    return 0;

  return (["uri":1,"language":1]) & a[0];
}

static int docs; // DEBUG
void remove_document(string|Standards.URI uri, void|string language)
{
  docs++; // DEBUG


  int uri_id=get_uri_id((string)uri);

  if(!uri_id)
    return;
  array a;
  if(language)
    a=db->query("select id from document where uri_id=%d and "
		"language=%s",uri_id,language);
  else
    a=db->query("select id from document where uri_id=%d",uri_id);

  if(!sizeof(a))
    return;
  
  db->query("delete from document where id in ("+a->id*","+")");
  db->query("insert into deleted_document (doc_id) values "+
	    "("+a->id*"),("+")");
}

static function sync_callback;
void set_sync_callback( function f )
{
  sync_callback = f;
}

static void sync_thread( _WhiteFish.Blobs blobs, int docs )
{
  int s = time();
  int q;
  Sql.Sql db = Sql.Sql( host );
  werror("----------- sync() %4d docs --------------\n", docs);
  do
  {
    [string word, _WhiteFish.Blob b] = blobs->read();
    if( !b )
      break;
    q++;
    string d = b->data();
    int w;
    sscanf( d, "%4c", w );
    mixed err=catch(db->query("insert into word_hit (word,first_doc_id,hits) "
			      "values (%s,%d,%s)", string_to_utf8(word), w, d ));
    if(err)
      werror("%O\n",describe_backtrace(err));
  } while( 1 );

  if( sync_callback )
    sync_callback();
  werror("----------- sync() done %3ds %5dw -------\n", time()-s,q);
}

static object old_thread;
void sync()
{
#if THREADS
  if( old_thread )
    old_thread->wait();
  old_thread = thread_create( sync_thread, blobs, docs );
#else
  sync_thread( blobs, docs );
#endif
  blobs = _WhiteFish.Blobs();
  docs = 0;
}

string get_blob(string word, int num)
{
  array a=db->query("select hits,first_doc_id from word_hit where word=%s "
		    "limit %d,1",
		    string_to_utf8(word), num);

  if(!sizeof(a))
    return 0;

  return a[0]->hits;
}

void clear()
{
  db->query("delete from word_hit");
  db->query("delete from uri");
  db->query("delete from document");
  db->query("delete from deleted_document");
  db->query("delete from metadata");
}



/* The queue does _not_ use the same objects as the database above.
 * The reason is twofold:
 *
 *   1: It's possible to store it in any database
 *
 *   2: Even when it's in the same database, it's accessed in
 *       parallell with the database in a different thread in at least
 *       multiprocess_crawler.pike
 */


class Queue
{
  Sql.Sql db;
  string url, table;
  
  Web.Crawler.Stats stats;
  Web.Crawler.Policy policy;
  Web.Crawler.RuleSet allow, deny;
  
  inherit Web.Crawler.Queue;

  static string to_md5(string url)
  {
    object md5 = Crypto.md5();
    md5->update(url);
    return Crypto.string_to_hex(md5->digest());
  }

  void create( Web.Crawler.Stats _stats,
	       Web.Crawler.Policy _policy,

	       string _url, string _table,

	       void|Web.Crawler.RuleSet _allow,
	       void|Web.Crawler.RuleSet _deny)
  {
    stats = _stats;   policy = _policy;
    allow=_allow;     deny=_deny;
    table = _table;

    db = Sql.Sql( _url );
    perhaps_create_table(  );
  }

  static void perhaps_create_table(  )
  {
    catch {
      db->query(
#"
    create table "+table+#" (
        uri        blob not null,
        uri_md5    char(32) not null default '',
	template   varchar(255) not null default '',
	md5        char(32) not null default '',
	recurse    tinyint not null,
	stage      tinyint not null,
	INDEX uri_ind (uri_md5),
	INDEX stage   (stage)
	)
    "
      );
    };
  }
  
  mapping hascache = ([]);
  static int has_uri( string|Standards.URI uri )
  {
    uri = (string)uri;
    if( sizeof(hascache) > 100000 )  hascache = ([]);
    return hascache[uri]||
      (hascache[uri]=
       sizeof(db->query("select stage from "+table+" where uri_md5=%s",
			to_md5(uri))));
  }

  void add_uri( Standards.URI uri, int recurse, string template, void|int force )
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
      if(search(rpath,reverse(index))==0)
	rpath=rpath[sizeof(index)..];
    r->path=reverse(rpath);
    
    if( force || (check_link(uri, allow, deny) && !has_uri( r ) ))
      db->query( "insert into "+table+
		 " (uri,uri_md5,recurse,template) values (%s,%s,%d,%s)",
		 (string)r, to_md5((string)r), recurse, (template||"") );
  }

  void set_md5( Standards.URI uri, string md5 )
  {
    db->query( "update "+table+
	       " set md5=%s WHERE uri_md5=%s", md5, to_md5((string)uri) );
  }

  mapping(string:mapping(string:string)) extra_data = ([]);
  mapping get_extra( Standards.URI uri )
  {
    if( extra_data[(string)uri] )
      return extra_data[(string)uri];
    array r = db->query( "SELECT md5,recurse,stage,template "
			 "FROM "+table+" WHERE uri_md5=%s", to_md5((string)uri) );
    if( sizeof( r ) )
      return r[0];
  }

  static int empty_count;
  static int retry_count;
  
  // cache, for performance reasons.
  static array possible=({});
  static int p_c;
  
  int|Standards.URI get()
  {
    if(stats->concurrent_fetchers() > policy->max_concurrent_fetchers)
    {
      return -1;
    }

    if( sizeof( possible ) <= p_c )
    {
      p_c = 0;
      possible = db->query( "select * from "+table+" where stage=0 limit 20" );
      extra_data = mkmapping( possible->uri, possible );
      possible = possible->uri;
    }

    while( sizeof( possible ) > p_c )
    {
      empty_count=0;
      if( possible[ p_c ] )
      {
	Standards.URI ur = Standards.URI( possible[p_c++] );

	if( stats->concurrent_fetchers( ur->host ) >
	    policy->max_concurrent_fetchers_per_host )
	{
	  retry_count++;
	  continue; // not this host..
	}
	possible[p_c-1] = 0;
	retry_count=0;
	set_stage( ur, 1 );
	return ur;
      }
      p_c++;
      continue;
    }

    if( stats->concurrent_fetchers() )
    {
      return -1;
    }
    // delay for (quite) a while.
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

  void done( Standards.URI uri,
	     int called )
  {
    if( called )
      set_stage( uri, 2 );
    else
      set_stage( uri, 5 );
  }

  void clear()
  {
    db->query("delete from "+table);
  }


  void clear_stage( int ... stages )
  {
    foreach( stages, int s )
      db->query( "update "+table+" set stage=0 where stage=%d", s );
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
}
