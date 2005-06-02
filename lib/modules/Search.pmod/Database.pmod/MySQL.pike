// This file is part of Roxen Search
// Copyright © 2000,2001 Roxen IS. All rights reserved.
//
// $Id: MySQL.pike,v 1.84 2005/06/02 16:27:22 mast Exp $

inherit .Base;

// Creates the SQL tables we need.

//#define SEARCH_DEBUG

#if constant (Sql.Sql)
// 7.6 and later.
#define SQL Sql.Sql
#else
#define SQL Sql.sql
#endif


static
{
// This is the database object that all queries will be made to.
  SQL db;
  string host;
  mapping options;
  string mergefile_path;
  int mergefile_counter = 0;
  int init_done = 0;
};

void create(string db_url, void|mapping _options)
{
  db=SQL(host=db_url);
  options = _options || ([]);
  mergefile_path = options->mergefiles;
  
  if(!mergefile_path)
    mergefile_path = "/tmp/";

  if(options->mergefiles)
    foreach(get_mergefiles(), string fn)
      rm(fn);
}

string _sprintf()
{
  return sprintf("Search.Database.MySQL(%O,%O)", host, mergefile_path);
}

// ----------------------------------------------
// Database initialization
// ----------------------------------------------

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
                         primary key (word,first_doc_id))");

  db->query(
#"create table if not exists lastmodified(doc_id     int not null primary key,
                                          at         int not null,
					  index index_at(at))");

  db->query(
#"create table if not exists link(from_id  int not null,
                                  to_id    int not null,
  				  index index_from(from_id),
  				  index index_to(to_id))");
  
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

void clear()
{
  db->query("delete from word_hit");
  db->query("delete from uri");
  db->query("delete from document");
  db->query("delete from deleted_document");
  db->query("delete from metadata");
  db->query("delete from lastmodified");
}


// ----------------------------------------------
// Utility functions
// ----------------------------------------------

static array(string) get_mergefiles()
{
  return map(glob("mergefile*.dat", get_dir(mergefile_path) || ({ })),
	     lambda(string s) { return combine_path(mergefile_path, s);});
}

static string to_md5(string url)
{
#if constant(Crypto.md5) && constant(Crypto.string_to_hex)
  return Crypto.string_to_hex( Crypto.md5()->
			       update( string_to_utf8(url) )->digest() );
#else
  return String.string2hex( Crypto.MD5.hash( string_to_utf8(url) ) );
#endif
}


// ----------------------------------------------
// Document handling
// ----------------------------------------------

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

  db->query("insert into document (uri_id, language) "
	    "values (%d,"+(language?"%s":"NULL")+")", 
	    uri_id, language);
  return db->master_sql->insert_id();
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
  db->query("insert delayed into deleted_document (doc_id) values "+
	    "("+a->id*"),("+")");
}

static Search.ResultSet deleted_documents = Search.ResultSet();
static int deleted_max, deleted_count;
Search.ResultSet get_deleted_documents()
{
  // FIXME: Make something better

  array a = db->query("select max(doc_id) as m, count(*) as c from deleted_document");
  int max_id = (int)a[0]->m;
  int count = (int)a[0]->c;

  if(max_id==deleted_max && count == deleted_count)
    return deleted_documents;
  else
  {
    array ids =  (array(int))db->query("select doc_id from deleted_document "
				       "order by doc_id")->doc_id;
    deleted_count = count;
    deleted_max = max_id;
    return deleted_documents = Search.ResultSet(ids);
  }
}
  
// ----------------------------------------------
// Field handling
// ----------------------------------------------

static mapping(string:int) list_fields_cache;

static void init_fields()
{
  if(init_done)
    return;

  init_done=1;
  foreach(({"uri","path1", "path2"})+Search.get_filter_fields(), string field)
    allocate_field_id(field);
}

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
  mixed err = catch {
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
    };
  mixed unlock_err = catch (db->query("unlock tables"));
  if (err) throw (err);
  if (unlock_err) throw (unlock_err);
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
  if( search(({"uri","path1","path2"})+Search.get_filter_fields(), field) == -1 )
    remove_field( field );
}

// ----------------------------------------------
// Word/blob handling
// ----------------------------------------------

static _WhiteFish.Blobs blobs = _WhiteFish.Blobs();

#define MAXMEM 64*1024*1024

void insert_words(Standards.URI|string uri, void|string language,
		  string field, array(string) words)
{
  if(!sizeof(words))  return;
  init_fields();

  int doc_id   = get_document_id((string)uri, language);
  int field_id = get_field_id(field);
  
  blobs->add_words( doc_id, words, field_id );
  
  if(blobs->memsize() > MAXMEM)
    if(options->mergefiles)
      mergefile_sync();
    else
      sync();
}

array(string) expand_word_glob(string g, void|int max_hits)
{
  g = replace( string_to_utf8(g), ({ "*", "?" }), ({ "%", "_" }) );
  if(max_hits)
    return map(db->query("select distinct word from word_hit where word like %s limit %d",
			 g, max_hits)->word,utf8_to_string);
  else
    return map(db->query("select distinct word from word_hit where word like %s",g)->word,utf8_to_string);
}

static int blobs_per_select = 40;

string get_blob(string word, int num, void|mapping(string:mapping(int:string)) blobcache)
{
  word = string_to_utf8( word );
  if(blobcache[word] && blobcache[word][num])
    return blobcache[word][num];
  if( blobcache[word] && blobcache[word][-1] )
  {
#ifdef SEARCH_DEBUG
    times[word] = 0;
#endif
    return 0;
  }
#ifdef SEARCH_DEBUG
  int t0 = gethrtime();
#endif
  
  array a=db->query("select hits,first_doc_id from word_hit where word=%s "
		    "order by first_doc_id limit %d,%d",
		    word, num, blobs_per_select);
#ifdef SEARCH_DEBUG
  int t1 = gethrtime()-t0;
  times[word] += t1;
  werror("word: %O  time accum: %.2f ms   delta_t: %.2f\n", word, times[word]/1000.0, t1/1000.0);
#endif
  
  blobcache[word] = ([]);
  if( sizeof( a ) < blobs_per_select )
    blobcache[word][-1]="";
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


// ----------------------------------------------
// Metadata handling
// ----------------------------------------------

void remove_metadata(Standards.URI|string uri, void|string language)
{
  int doc_id;
  if(!intp(uri))
    doc_id = get_document_id((string)uri, language);
  db->query("delete from metadata where doc_id = %d", doc_id);
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

// ----------------------------------------------
// Date stuff
// ----------------------------------------------

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
  
  db->query("replace delayed into metadata (doc_id, name, value) values "+s);
}

void set_lastmodified(Standards.URI|string uri,
		      void|string language,
		      int when)
{
  int doc_id   = get_document_id((string)uri, language);
  db->query("replace into lastmodified (doc_id, at) values (%d,%d)", doc_id, when);
}

int get_lastmodified(Standards.URI|string|array(Standards.URI|string) uri, void|string language)
{
  int doc_id   = get_document_id((string)uri, language);
  array q = db->query("select at from lastmodified where doc_id=%d", doc_id);
  if( sizeof( q ) )
      return (int)q[0]->at;
}

void randomize_dates()
{
  foreach(db->query("select id from document")->id, string id)
    db->query("replace into lastmodified (doc_id,at) values (%s,%d)",
	      id,
	      random(365*24*3600)+time()-365*24*3600);
    
}

static
{
  _WhiteFish.DateSet dateset_cache;
  int dateset_cache_max_doc_id = -1;
  
  int get_max_doc_id()
  {
    array a = db->query("select doc_id from lastmodified order by doc_id desc limit 1");
    if(!sizeof(a))
      return 0;
    else
      return (int)a[0]->doc_id;
  }
};

_WhiteFish.DateSet get_global_dateset()
{
  int max_doc_id = get_max_doc_id();
  if(max_doc_id == dateset_cache_max_doc_id)
    return dateset_cache;
  else
  {
    array a = db->query("select doc_id,at from lastmodified where "
			"doc_id > %d order by doc_id asc", dateset_cache_max_doc_id);
    
    dateset_cache_max_doc_id = max_doc_id;
    if(!dateset_cache)
      dateset_cache = _WhiteFish.DateSet();
    dateset_cache->add_many( (array(int))a->doc_id,
			     (array(int))a->at );
    return dateset_cache;
  }
}

// ----------------------------------------------
// Link handling
// ----------------------------------------------

void add_links(Standards.URI|string uri,
	       void|string language,
	       array(Standards.URI|string) links)
{
  if(!links || !sizeof(links))
    return;
  
  int doc_id = get_document_id((string)uri, language);
  
  array(int) to_ids = map(links,
			  lambda(Standards.URI|string uri)
			  {
			    return get_document_id( (string)uri, language);
			  });

  string res =
    "replace into link (from_id, to_id) values " +
    map(to_ids,
	lambda(int to_id)
	{
	  return sprintf("(%d, %d)", doc_id, to_id);
	}) * ", ";
  db->query(res);
}

void remove_links(Standards.URI|string uri,
		  void|string language)
{
  int doc_id = get_document_id((string)uri, language);

  db->query("delete from link where from_id=%d", doc_id);
}

array(int) get_broken_links()
{
  db->query("select 'Not yet done :-)'");
}

// ----------------------------------------------
// Sync stuff
// ----------------------------------------------

static function sync_callback;
void set_sync_callback( function f )
{
  sync_callback = f;
}

int max_blob_size = 512*1024;

static array(array(int|string)) split_blobs(int blob_size, string blob,
					    int max_blob_size)
{
  /*
    +-----------+----------+---------+---------+---------+
    | docid: 32 | nhits: 8 | hit: 16 | hit: 16 | hit: 16 |...
    +-----------+----------+---------+---------+---------+
  */
  
  int ptr = blob_size;
  int start = 0, end=0;
  array blobs = ({});

  while( end+5 < sizeof(blob) )
  {
    while(end+5 < sizeof(blob) && blob_size < (max_blob_size-517))
    {
      int l = 4 + 1 + 2*blob[end+4];
      end += l;
      blob_size += l;
    }
    string me = blob[start..end-1];
    if( sizeof( blobs ) )
      blobs += ({ ({array_sscanf(me,"%4c")[0],me}) });
    else
      blobs += ({ ({0,me}) });
    start = end;
    blob_size=0;
  }

  return blobs;
}

static void store_to_db( void|string mergedfilename )
{
  Search.MergeFile mergedfile;

  if(mergedfilename)
    mergedfile = Search.MergeFile(Stdio.File(mergedfilename, "r"));
  
  int s = time();
  int q;
  Sql.Sql db = Sql.Sql( host );
#ifdef SEARCH_DEBUG  
  werror("----------- sync() %4d docs --------------\n", docs);
#endif  
  db->query("LOCK TABLES word_hit LOW_PRIORITY WRITE");

  mixed err = catch {
  String.Buffer multi_query = String.Buffer();

  do
  {
    string word, blob;
    if(mergedfilename)
    {
      array a = mergedfile->get_next_word_blob();
      if( !a )
	break;
      [word, blob] = a;
    }
    else
    {
      [word, blob] = blobs->read();
      if(!word)
	break;
      word = string_to_utf8(word);
    }

    q++;

    db->query("UNLOCK TABLES");
    db->query("LOCK TABLES word_hit LOW_PRIORITY WRITE");

    int first_doc_id;
    array old=db->query("SELECT first_doc_id,length(hits) as l "+
			"FROM word_hit WHERE word=%s ORDER BY first_doc_id",
			word );
    if( sizeof( old ) )
    {
      int blob_size = (int)old[-1]->l;
      int last_doc_id = (int)old[-1]->first_doc_id;
      if(blob_size+sizeof(blob) > max_blob_size)
      {
	array blobs = split_blobs(blob_size, blob, max_blob_size);
	blob = blobs[0][1];
	foreach(blobs[1..], array blob_pair)
	  db->query("INSERT INTO word_hit "
		    "(word,first_doc_id,hits) "
		    "VALUES (%s,%d,%s)", word, @blob_pair);
      }
      db->query("UPDATE word_hit SET hits=CONCAT(hits,%s) "+
		"WHERE word=%s and first_doc_id=%d", blob, word, last_doc_id);
    }
    else
    {
      if(sizeof(blob) > max_blob_size)
      {
	array blobs = split_blobs(0, blob, max_blob_size);
	foreach(blobs, array blob_pair)
	  db->query("INSERT INTO word_hit "
		    "(word,first_doc_id,hits) "
		    "VALUES (%s,%d,%s)", word, @blob_pair);
      }
      else
      {
	sscanf( blob, "%4c", first_doc_id );
	string qu = "('"+db->quote(word)+"',"+
	  first_doc_id+",'"+db->quote(blob)+"')";
	if( sizeof(multi_query) + sizeof(qu) > 900*1024 )
	  db->query( multi_query->get());
	if( sizeof(multi_query) )
	  multi_query->add( ",",qu );
	else
	  multi_query->add( "INSERT INTO word_hit "
			    "(word,first_doc_id,hits) VALUES ",
			    qu );
      }
    }
  } while( 1 );

  if( sizeof( multi_query ) )
    db->query( multi_query->get());

  };				// catch
  mixed unlock_err = catch (db->query("UNLOCK TABLES"));
  if (err) throw (err);
  if (unlock_err) throw (unlock_err);
  
  if( sync_callback )
    sync_callback();
  
  if(mergedfilename)
  {
    mergedfile->close();
    rm(mergedfilename);
  }
#ifdef SEARCH_DEBUG
  werror("----------- sync() done %3ds %5dw -------\n", time()-s,q);
#endif
}

static string get_mergefilename()
{
  return combine_path(mergefile_path,
		      sprintf("mergefile%03d.dat", mergefile_counter));
}

static void mergefile_sync()
{
#ifdef SEARCH_DEBUG  
  System.Timer t = System.Timer();
  werror("----------- mergefile_sync() %4d docs --------------\n", docs);
#endif  
  Search.MergeFile mergefile = Search.MergeFile(
    Stdio.File(get_mergefilename(), "wct"));

  mergefile->write_blobs(blobs);

  if( sync_callback )
    sync_callback();
#ifdef SEARCH_DEBUG
  werror("----------- mergefile_sync() done %.3f s  %2.1f MB -------\n",
	 t->get(),
	 file_stat(get_mergefilename())->size/(1024.0*1024.0));
#endif

  mergefile_counter++;
  blobs = _WhiteFish.Blobs();
}

static string merge_mergefiles(array(string) mergefiles)
{
#ifdef SEARCH_DEBUG  
  werror("merge_mergefiles( %s )\n", mergefiles*", ");
#endif
  if(sizeof(mergefiles)==1)
    return mergefiles[0];

  if(sizeof(mergefiles)>2)
  {
    int pivot = sizeof(mergefiles)/2;
    return merge_mergefiles( ({ merge_mergefiles(mergefiles[..pivot-1] ),
				merge_mergefiles(mergefiles[pivot..] ) }) );
  }

  // Else: actually merge two mergefiles

  string mergedfile_fn = get_mergefilename();
  mergefile_counter++;

  Search.MergeFile mergedfile =
    Search.MergeFile(Stdio.File(mergedfile_fn, "wct"));

  System.Timer t = System.Timer();
  mergedfile->merge_mergefiles(Search.MergeFile(Stdio.File(mergefiles[0], "r")),
			       Search.MergeFile(Stdio.File(mergefiles[1], "r")));

#ifdef SEARCH_DEBUG  
  werror("Merging %s (%.1f MB) took %.1f s\n",
	 mergedfile_fn, file_stat(mergedfile_fn)->size/(1024.0*1024.0),
	 t->get());
#endif

  rm(mergefiles[0]);
  rm(mergefiles[1]);
  return mergedfile_fn;
}

void sync()
{
  if(options->mergefiles)
  {
    mergefile_sync();
    store_to_db(merge_mergefiles(sort(get_mergefiles())));
  }
  else
  {
    store_to_db();
    blobs = _WhiteFish.Blobs();
  }
  docs = 0;
}

#ifdef SEARCH_DEBUG
mapping times = ([ ]);
#endif


// ----------------------------------------------
// Statistics
// ----------------------------------------------

int memsize()
{
  return blobs->memsize();
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
