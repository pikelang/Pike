// This file is part of Roxen Search
// Copyright © 2000,2001 Roxen IS. All rights reserved.
//
// $Id: MySQL.pike,v 1.76 2004/03/02 16:57:59 stewa Exp $

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
                         language varchar(255) default null,
                         INDEX index_language (language),
                         INDEX index_uri_id (uri_id))"
				       ); //FIXME: Remove index_language?
  
  db->query("create table if not exists deleted_document (doc_id int unsigned not null)");

  db->query(
#"create table if not exists word_hit (word        varchar(64) binary not null,
                         first_doc_id   int not null,
            	         hits           mediumblob not null,
                         index index_word (word))");

  db->query(
#"create table if not exists metadata (doc_id        int not null,
                         name          varchar(32) not null,
            	         value         mediumblob not null,
                         index index_doc_id(doc_id))");

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
  foreach(({"uri","path1","path2"})+Search.get_filter_fields(), string field)
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
  md5->update( string_to_utf8( url ) );
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
	    string_to_utf8( uri ), to_md5(uri));
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
  array a =db->query("select id from field where name=%s", field);
  if(sizeof(a))
    return (int)a[0]->id;
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

void safe_remove_field(string field)
{
  if( search(({"uri"})+Search.get_filter_fields(), field) == -1 )
    remove_field( field );
}

static _WhiteFish.Blobs blobs = _WhiteFish.Blobs();

#define MAXMEM 3*1024*1024


int size()
{
  return blobs->memsize();
}

//! Inserts the words of a resource into the database
void insert_words(Standards.URI|string uri, void|string language,
		  string field, array(string) words)
{
  if(!sizeof(words))  return;
  init_fields();

  int doc_id   = get_document_id((string)uri, language);
  int field_id = get_field_id(field);
  
  blobs->add_words( doc_id, words, field_id );
  
  if(blobs->memsize() > MAXMEM)
    sync();
}

array(string) expand_word_glob(string g, void|int max_hits)
{
  g = replace( string_to_utf8(g), ({ "*", "?" }), ({ "%", "_" }) );
  if(max_hits)
    return db->query("select distinct word from word_hit where word like %s limit %d",
		     g, max_hits)->word;
  else
    return db->query("select distinct word from word_hit where word like %s",g)->word;
}

void remove_metadata(Standards.URI|string uri, void|string language)
{
  int doc_id;
  if(!intp(uri))
    doc_id = get_document_id((string)uri, language);
  db->query("delete from metadata where doc_id = %d", doc_id);
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
  {
    if(sizeof(md->body))
      md->body = Unicode.normalize( Unicode.split_words_and_normalize( md->body ) * " ", "C");
    md->body = Gz.deflate(6)->deflate(string_to_utf8(md->body[..64000]),
				      Gz.FINISH);
  }

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

static string make_fields_sql(void|array(string) wanted_fields)
{
  if(wanted_fields && sizeof(wanted_fields))
    return " and name IN ('"+map(wanted_fields,db->quote)*"','"+"')";
  else
    return "";
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
  array a=db->query("select name,value from metadata where doc_id=%d"+
		    make_fields_sql(wanted_fields),
		    doc_id);
  mapping md=mkmapping(a->name,a->value);
  if(md->body)
    md->body=Gz.inflate()->inflate(md->body);

  foreach(indices(md), string field)
    md[field] = utf8_to_string(md[field]);

  return md;
}

mapping(int:string) get_special_metadata(array(int) doc_ids,
					  string wanted_field)
{
  array a=db->query("select doc_id,value from metadata where doc_id IN ("+
		    ((array(string))doc_ids)*","+") and name = %s",
		    wanted_field);

  return mkmapping( (array(int))a->doc_id, a->value);
}

mapping get_uri_and_language(int|array(int) doc_id)
{
  if(arrayp(doc_id))
  {
    array a=db->query("select document.id,document.language, uri.uri from document, uri "
		      "where uri.id=document.uri_id and document.id IN ("+
		      ((array(string))doc_id)*","+")");
    return mkmapping( (array(int))a->id, a );
  }
  else
  {
    array a=db->query("select document.language,uri.uri from document,uri "
		      "where uri.id=document.uri_id and document.id=%d",doc_id);
    if(!sizeof(a))
      return 0;
    
    return (["uri":1,"language":1]) & a[0];
  }
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
#ifdef SEARCH_DEBUG  
  werror("----------- sync() %4d docs --------------\n", docs);
#endif  
  do
  {
    [string word, _WhiteFish.Blob b] = blobs->read();
    if( !b )
      break;
    q++;
    string d = b->data();
    int w;
    sscanf( d, "%4c", w );
    db->query("insert into word_hit (word,first_doc_id,hits) "
	      "values (%s,%d,%s)", string_to_utf8(word), w, d );
 } while( 1 );

  if( sync_callback )
    sync_callback();
#ifdef SEARCH_DEBUG
  werror("----------- sync() done %3ds %5dw -------\n", time()-s,q);
#endif
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

#ifdef SEARCH_DEBUG
mapping times = ([ ]);
#endif

string get_blob(string word, int num, void|mapping(string:mapping(int:string)) blobcache)
{
  if(String.width(word) > 8)
    word = string_to_utf8(word);
  if(blobcache[word] && blobcache[word][num])
    return blobcache[word][num];

#ifdef SEARCH_DEBUG
  int t0 = gethrtime();
#endif
  
  array a=db->query("select hits,first_doc_id from word_hit where word=%s "
		    "order by first_doc_id limit %d,10",
		    word, num);

#ifdef SEARCH_DEBUG
  int t1 = gethrtime()-t0;
  times[word] += t1;
  werror("word: %O  time accum: %.2f ms   delta_t: %.2f\n", word, times[word]/1000.0, t1/1000.0);
#endif
  
  blobcache[word] = ([]);
  if(!sizeof(a))
  {
#ifdef SEARCH_DEBUG
    times[word] = 0;
#endif
    return 0;
  }

  foreach(a, mapping m)
    blobcache[word][num++] = m->hits;

  return a[0]->hits;
}

array(int) get_deleted_documents()
{
  return (array(int))db->query("select doc_id from deleted_document "
			       "order by doc_id")->doc_id;
}


mapping(string|int:int) get_language_stats()
{
  array a=db->query("select count(id) as c,language from document group by language");
  return mkmapping( a->language, a->c);
}

int get_num_words()
{
  return (int)(db->query("select count(distinct word) as c from word_hit") +
	       ({ (["c": 0]) }))[0]->c;
}

int get_database_size()
{
  int size;
  foreach(db->query("show table status"), mapping table)
    size += (int)table->Data_length + (int)table->Index_length;
  return size;
}

int get_num_deleted_documents()
{
  return (int)db->query("select count(*) as c from deleted_document")[0]->c;

}

static string my_denormalize(string in)
{
  return Unicode.normalize(utf8_to_string(in), "C");
}

array(array) get_most_common_words(void|int count)
{
  array a =
    db->query("select word, sum(length(hits))/5 as c from word_hit "
	      "group by word order by c desc limit %d", count||10);

  if(!sizeof(a))
    return ({ });
  else
    return Array.transpose( ({ map(a->word, my_denormalize),
			       (array(int))a->c }) );
}

void clear()
{
  db->query("delete from word_hit");
  db->query("delete from uri");
  db->query("delete from document");
  db->query("delete from deleted_document");
  db->query("delete from metadata");
}
