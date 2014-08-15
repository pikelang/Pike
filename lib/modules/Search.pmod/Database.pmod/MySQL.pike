#pike __REAL_VERSION__

inherit .Base;

// Creates the SQL tables we need.

//#define SEARCH_DEBUG

#define DB_MAX_WORD_SIZE 64

protected
{
// This is the database object that all queries will be made to.
  Sql.Sql db;
  string host;
  mapping options;
  string mergefile_path;
  int mergefile_counter = 0;
  int init_done = 0;
};

void create(string db_url, void|mapping _options)
{
  db=Sql.Sql(host=db_url);
  options = _options || ([]);
  mergefile_path = options->mergefiles;
  
  if(!mergefile_path)
    mergefile_path = "/tmp/";

  if(options->mergefiles)
    foreach(get_mergefiles(), string fn)
      rm(fn);
}

#ifdef SEARCH_DEBUG
void destroy()
{
  if (blobs_dirty)
    werror("Search.Database.MySQL: WARNING: Forgot to sync before "
	   "abandoning db object?\n");
}
#endif

string _sprintf()
{
  return sprintf("Search.Database.MySQL(%O,%O)", host, mergefile_path);
}


//  Support for old- and new-style padded blobs must be determined at
//  runtime. This is because the format must be compatible with whatever
//  high-level Search module currently available, specifically the compactor.
int cache_supports_padded_blobs = -1;

int supports_padded_blobs()
{
  if (cache_supports_padded_blobs < 0) {
    mixed compactor_class = master()->resolv("Search.Process.Compactor");
    if (compactor_class && compactor_class->supports_padded_blobs)
      cache_supports_padded_blobs = 1;
    else
      cache_supports_padded_blobs = 0;
  }
  return cache_supports_padded_blobs;
}


// ----------------------------------------------
// Database initialization
// ----------------------------------------------

void init_tables()
{
  int use_padded_blobs = supports_padded_blobs();
  
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
  
  db->query("create table if not exists deleted_document (doc_id int unsigned not null primary key)");



  
  db->query(
#"create table if not exists word_hit (word        varchar("+DB_MAX_WORD_SIZE+#") binary not null,
                         first_doc_id   int not null, " +
            (use_padded_blobs ? #"
                         used_len       int not null,
                         real_len       int not null, " : "") + #"
            	         hits           mediumblob not null,
                         primary key (word,first_doc_id))");
  
  int has_padded_blobs_fields =
    sizeof(db->query("DESCRIBE word_hit used_len"));
  if (use_padded_blobs && !has_padded_blobs_fields) {
    //  Add used_len and real_len to older tables
    werror("Search: Upgrading '%s.word_hit' table to support padded blobs.\n",
	   (host / "/")[-1]);
    db->query("ALTER TABLE word_hit "
	      " ADD COLUMN used_len INT NOT NULL "
	      "      AFTER first_doc_id, "
	      " ADD COLUMN real_len INT NOT NULL "
	      "      AFTER used_len");
    db->query("UPDATE word_hit "
	      "   SET used_len = LENGTH(hits), real_len = LENGTH(hits)");
  } else if (!use_padded_blobs && has_padded_blobs_fields) {
    //  Newer database format found in a context where we don't expect it.
    //  In order to not misinterpret or even write incorrect records we
    //  must drop the extra fields. (Trying to set the supports flag here
    //  will not survive new instances of the MySQL object.)
    werror("Search: Downgrading '%s.word_hit' table to remove padded blobs.\n",
	   (host / "/")[-1]);
    db->query("UPDATE word_hit "
	      "   SET hits = LEFT(hits, used_len) "
	      " WHERE used_len < real_len");
    db->query("ALTER TABLE word_hit "
	      " DROP COLUMN used_len, "
	      " DROP COLUMN real_len");
  }

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

protected array(string) get_mergefiles()
{
  return map(glob("mergefile*.dat", get_dir(mergefile_path) || ({ })),
	     lambda(string s) { return combine_path(mergefile_path, s);});
}

protected string to_md5(string url)
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

int get_document_id(string uri, void|string language, void|int do_not_create)
{
  int uri_id=get_uri_id(uri, do_not_create);

  if (!uri_id)
    return 0;
  
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

void remove_uri(string|Standards.URI uri)
{
  db->query("delete from uri where uri_md5=%s", to_md5((string)uri));
}

void remove_uri_prefix(string|Standards.URI uri)
{
  string uri_string = (string)uri;
  db->query("delete from uri where uri like '" + db->quote(uri_string) + "%%'");
}

#ifdef SEARCH_DEBUG
protected int docs;
protected int blobs_dirty;
#endif

void remove_document(string|Standards.URI uri, void|string language)
{
#ifdef SEARCH_DEBUG
  docs++;
#endif

  int uri_id=get_uri_id((string)uri, 1);

  if(!uri_id)
    return;
  array a;
  if(language) {
    //  Need to remove this particular language fork as well as any
    //  non-language version of the document (since they are mutually
    //  exclusive).
    //
    //  Note however that a document with several language forks where
    //  one fork is removed will keep that entry since we cannot know
    //  which entries that are garbage and hence leave them in place.
    //  It is up to the query filter to only show valid forks.
    a=db->query("select id from document where uri_id=%d and "
		"(language=%s OR language IS NULL)", uri_id, language);
  } else {
    //  This also deletes any past language-specific forks
    a=db->query("select id from document where uri_id=%d",uri_id);
  }

  if(!sizeof(a))
    return;
  
  db->query("delete from document where id in ("+a->id*","+")");
  db->query("insert delayed into deleted_document (doc_id) values "+
	    "("+a->id*"),("+")");
}

void remove_document_prefix(string|Standards.URI uri)
{
  array a =
    db->query("SELECT document.id AS id"
	      "  FROM document, uri "
	      " WHERE document.uri_id=uri.id "
	      "   AND uri.uri like '" + db->quote(uri) + "%%'");
  if(!sizeof(a))
    return;

  array ids = a->id;
#ifdef SEARCH_DEBUG
  docs += sizeof(ids);
#endif
  db->query("DELETE FROM document "
	    " WHERE id IN (" + (ids * ",") + ")");
  db->query("INSERT DELAYED INTO deleted_document "
	    "(doc_id) VALUES (" + (ids * "),(") + ")");
}

protected Search.ResultSet deleted_documents = Search.ResultSet();
protected int deleted_max, deleted_count;
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

Search.ResultSet get_all_documents()
{
  array ids =
    (array(int)) db->query("SELECT id FROM document ORDER BY id")->id;
  return Search.ResultSet(ids);
}



// ----------------------------------------------
// Field handling
// ----------------------------------------------

protected mapping(string:int) list_fields_cache;

protected void init_fields()
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

protected mapping field_cache = ([]);
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

protected _WhiteFish.Blobs blobs = _WhiteFish.Blobs();

#define MAXMEM 64*1024*1024

void insert_words(Standards.URI|string uri, void|string language,
		  string field, array(string) words)
{
  // Remove long words that won't fit into the database.
  words = filter(words, lambda (string word)
			{ return sizeof(string_to_utf8(word)) <= DB_MAX_WORD_SIZE; });
      
  if(!sizeof(words))  return;
  init_fields();

  int doc_id   = get_document_id((string)uri, language);
  int field_id = get_field_id(field);
  
  blobs->add_words( doc_id, words, field_id );
#ifdef SEARCH_DEBUG
  blobs_dirty = 1;
#endif
  
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


int get_padded_blob_length(int used_len)
{
  //  Suggest a padded length based on current length. We'll use this
  //  strategy:
  //
  //    - no blobs smaller than 64 bytes
  //    - blobs grow 25% rounded up to nearest 64 bytes
  int new_len = (((used_len >> 2) + used_len) | 63) + 1;
  return min(new_len, max_blob_size);
}


protected int blobs_per_select = 40;

string get_blob(string word, int num,
		void|mapping(string:mapping(int:string)) blobcache)
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

  int use_padded_blobs = supports_padded_blobs();
  array a =
    db->query("  SELECT hits, first_doc_id " +
	      (use_padded_blobs ? ", used_len, real_len " : "") +
	      "    FROM word_hit "
	      "   WHERE word = %s "
	      "ORDER BY first_doc_id "
	      "   LIMIT %d,%d",
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

  foreach(a, mapping m) {
    if (use_padded_blobs) {
      //  Each blob may be padded with trailing space to reduce fragmentation.
      //  The feeder requesting the data will however not understand that so
      //  we cut it off. In the unlikely event that real_len isn't the true
      //  length we take care of that as well (this would indicate something
      //  fishy in the writing of padded blobs).
      int used_len = (int) m->used_len;
      int real_len = (int) m->real_len;
      if ((used_len < real_len) || (real_len != sizeof(m->hits)))
	m->hits = m->hits[..(used_len - 1)];
    }
    
    blobcache[word][num++] = m->hits;
  }

  return a[0]->hits;
}


// ----------------------------------------------
// Metadata handling
// ----------------------------------------------

void remove_metadata(Standards.URI|string uri, void|string language)
{
  int doc_id;
  if(!intp(uri))
    doc_id = get_document_id((string)uri, language, 1);
  db->query("delete from metadata where doc_id = %d", doc_id);
}

protected string make_fields_sql(void|array(string) wanted_fields)
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
#if constant(Gz)
  if(md->body)
    md->body=Gz.inflate()->inflate(md->body);
#endif

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
#if constant(Gz)
    md->body = Gz.deflate(6)->deflate(string_to_utf8(md->body[..64000]),
				      Gz.FINISH);
#endif
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

protected
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

protected
{
  _WhiteFish.DateSet publ_dateset_cache;
  int publ_dateset_cache_max_doc_id = -1;
};

_WhiteFish.DateSet get_global_publ_dateset()
{
  int max_doc_id = get_max_doc_id();
  if(max_doc_id == publ_dateset_cache_max_doc_id)
    return publ_dateset_cache;
  else
  {
    array(mapping(string:mixed)) a = 
      db->query("SELECT doc_id, value FROM metadata "
		" WHERE name = 'publish-time' "
		"   AND doc_id > %d ORDER BY doc_id ASC",
		publ_dateset_cache_max_doc_id);

    publ_dateset_cache_max_doc_id = max_doc_id;
    if(!publ_dateset_cache)
      publ_dateset_cache = _WhiteFish.DateSet();
    publ_dateset_cache->add_many( (array(int))a->doc_id,
				  (array(int))a->value );
    return publ_dateset_cache;
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
  int doc_id = get_document_id((string)uri, language, 1);

  db->query("delete from link where from_id=%d", doc_id);
}

array(int) get_broken_links()
{
  db->query("select 'Not yet done :-)'");
}

// ----------------------------------------------
// Sync stuff
// ----------------------------------------------

protected function sync_callback;
void set_sync_callback( function f )
{
  sync_callback = f;
}

//  The maximum blob size on disk must be at least big enough to hold as
//  many entries that can be found in a single document. This is needed so
//  a split blob doesn't get the same docid in separate records.
//
//  We can get at most 255 occurrences of the same word from each document,
//  and if all of those are the same word AND the update takes place
//  incrementally we'll write [ docid | nhits | hit ] for every occurrence,
//  i.e. 7 bytes every time. Minimum blob size is therefore 1785 bytes.
constant max_blob_size = 512 * 1024;


protected array(array(int|string)) split_blobs(int blob_size, string blob,
					    int max_blob_size)
{
  /*
    +-----------+----------+---------+---------+---------+
    | docid: 32 | nhits: 8 | hit: 16 | hit: 16 | hit: 16 |...
    +-----------+----------+---------+---------+---------+
  */
  
  sscanf(blob, "%4c", int first_doc_id);
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
    if (sizeof(me))
      sscanf(me, "%4c", first_doc_id);
    blobs += ({ ({ first_doc_id, me }) });
    start = end;
    blob_size=0;
  }

  return blobs;
}

protected void store_to_db( void|string mergedfilename )
{
  Search.MergeFile mergedfile;

  if(mergedfilename)
    mergedfile = Search.MergeFile(Stdio.File(mergedfilename, "r"));

  int use_padded_blobs = supports_padded_blobs();
  
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
      
      //  Blob hits are grouped by docid but not sorted internally. We need
      //  to store in sorted form since the hit analysis depend on it. The
      //  data() method in Blob performs sorting so instantiate a temp blob
      //  just to access this.
      blob = _WhiteFish.Blob(blob)->data();
    }

    q++;

    //  Don't unlock and lock every word to reduce overhead
    if (q % 32 == 0) {
      db->query("UNLOCK TABLES");
      db->query("LOCK TABLES word_hit LOW_PRIORITY WRITE");
    }
    
    //  NOTE: Concatenation of hits info is strictly speaking not correct in
    //  the general case since we may have the same docid repeated. In practice
    //  the only code path that adds words also invalidates the old docid and
    //  gets a fresh one.

    void add_padded_blobs(string word, array new_blobs)
    {
      //  Write all blobs except the last one that should be padded
      foreach (new_blobs[..<1], array new_blob_pair) {
	[int first_doc_id, string blob] = new_blob_pair;
	int new_used_len = sizeof(blob);
	db->query("INSERT INTO word_hit "
		  "            (word, first_doc_id, used_len, real_len, hits)"
		  "     VALUES (%s, %d, %d, %d, %s)",
		  word, first_doc_id, new_used_len, new_used_len, blob);
      }
      
      //  Write final blob with padding
      [int first_doc_id, string blob] = new_blobs[-1];
      int new_used_len = sizeof(blob);
      int new_real_len = get_padded_blob_length(new_used_len);
      int space_count = new_real_len - new_used_len;
      db->query("INSERT INTO word_hit "
		"            (word, first_doc_id, used_len, real_len, hits)"
		"     VALUES (%s, %d, %d, %d, CONCAT(%s, SPACE(%d)))",
		word, first_doc_id, new_used_len, new_real_len,
		blob, space_count);
    };
    
    void add_oldstyle_blobs(string word, array new_blobs)
    {
      //  Write all blobs as new entries
      foreach (new_blobs, array new_blob_pair) {
	[int first_doc_id, string blob] = new_blob_pair;
	db->query("INSERT INTO word_hit "
		  "            (word, first_doc_id, hits)"
		  "     VALUES (%s, %d, %s)",
		  word, first_doc_id, blob);
      }
    };
    
    //  We only care about the most recent blob for the given word so look
    //  for the highest document ID.
    int first_doc_id;
    array old;
    if (use_padded_blobs) {
      old = db->query("  SELECT first_doc_id, used_len, real_len "
		      "    FROM word_hit "
		      "   WHERE word=%s "
		      "ORDER BY first_doc_id DESC "
		      "   LIMIT 1", word);
    } else {
      old = db->query("  SELECT first_doc_id, LENGTH(hits) AS used_len "
		      "    FROM word_hit "
		      "   WHERE word=%s "
		      "ORDER BY first_doc_id DESC "
		      "   LIMIT 1", word);
    }

    if (sizeof(old)) {
      int used_len = (int) old[-1]->used_len;
      int real_len = use_padded_blobs ? ((int) old[-1]->real_len) : used_len;
      int first_doc_id = (int) old[-1]->first_doc_id;
      
      //  Can the new blob fit in the existing padding space?
      //
      //  NOTE: This is never true for old-style blobs.
      if (real_len - used_len >= sizeof(blob)) {
	//  Yes, update in place
	db->query(" UPDATE word_hit "
		  "    SET hits = INSERT(hits, %d, %d, %s), "
		  "        used_len = %d "
		  "  WHERE word = %s "
		  "    AND first_doc_id = %d",
		  used_len + 1, sizeof(blob), blob,
		  used_len + sizeof(blob),
		  word, first_doc_id);
      } else if (used_len + sizeof(blob) <= max_blob_size) {
	//  The old blob can grow to accomodate the new data without
	//  exceeding the maximum blob size.
	if (use_padded_blobs) {
	  //  Make sure we make room for new padding for future use
	  int new_used_len = used_len + sizeof(blob);
	  int new_real_len = get_padded_blob_length(new_used_len);
	  int space_count = new_real_len - new_used_len;
	  db->query("UPDATE word_hit "
		    "   SET hits = INSERT(hits, %d, %d, CONCAT(%s, SPACE(%d))),"
		    "       used_len = %d, "
		    "       real_len = %d "
		    " WHERE word = %s "
		    "   AND first_doc_id = %d",
		    used_len + 1, sizeof(blob) + space_count, blob, space_count,
		    new_used_len,
		    new_real_len,
		    word, first_doc_id);
	} else {
	  //  Append blob data to old record
	  db->query("UPDATE word_hit "
		    "   SET hits = CONCAT(hits, %s) "
		    " WHERE word = %s "
		    "   AND first_doc_id = %d",
		    blob, word, first_doc_id);
	}
      } else {
	//  Need to split blobs
	array new_blobs = split_blobs(used_len, blob, max_blob_size);
	blob = new_blobs[0][1];
	
	if (use_padded_blobs) {
	  //  Write the first chunk at the end of the existing blob and remove
	  //  any left-over padding by giving a sufficiently bigger blob size
	  //  as third parameter compared to the actual data.
	  int new_used_len = used_len + sizeof(blob);
	  db->query("UPDATE word_hit "
		    "   SET hits = INSERT(hits, %d, %d, %s), "
		    "       used_len = %d, "
		    "       real_len = %d "
		    " WHERE word = %s "
		    "   AND first_doc_id = %d",
		    used_len + 1, sizeof(blob) + max_blob_size, blob,
		    new_used_len,
		    new_used_len,
		    word, first_doc_id);
	} else {
	  //  Write the first chunk at the end of the existing blob
	  db->query("UPDATE word_hit "
		    "   SET hits = CONCAT(hits, %s) "
		    " WHERE word = %s "
		    "   AND first_doc_id = %d",
		    blob, word, first_doc_id);
	}
	
	//  Write remaining ones
	if (use_padded_blobs)
	  add_padded_blobs(word, new_blobs[1..]);
	else
	  add_oldstyle_blobs(word, new_blobs[1..]);
      }
    } else {
      //  No existing entries so create new blobs
      if (sizeof(blob) > max_blob_size) {
	//  Blobs must be split in several records
	array new_blobs = split_blobs(0, blob, max_blob_size);
	if (use_padded_blobs)
	  add_padded_blobs(word, new_blobs);
	else
	  add_oldstyle_blobs(word, new_blobs);
      } else {
	//  Queue writing of single blob
	sscanf(blob, "%4c", first_doc_id);
	string new_query;
	if (use_padded_blobs) {
	  int new_used_len = sizeof(blob);
	  int new_real_len = get_padded_blob_length(new_used_len);
	  int space_count = new_real_len - new_used_len;
	  new_query =
	    sprintf("('%s', %d, %d, %d, CONCAT('%s', SPACE(%d)))",
		    db->quote(word), first_doc_id,
		    new_used_len, new_real_len,
		    db->quote(blob), space_count);
	} else {
	  new_query =
	    sprintf("('%s', %d, '%s')",
		    db->quote(word), first_doc_id, db->quote(blob));
	}
	
	//  If aggregated query is too big we run the old one now
	if (sizeof(multi_query) + sizeof(new_query) > 900 * 1024)
	  db->query(multi_query->get());
	
	//  Append to delayed query
	if (!sizeof(multi_query)) {
	  multi_query->add("INSERT INTO word_hit ",
			   (use_padded_blobs ?
			    " (word, first_doc_id, used_len, real_len, hits) " :
			    " (word, first_doc_id, hits) "),
			   "VALUES ",
			   new_query);
	} else {
	  multi_query->add(",", new_query);
	}
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
  
#ifdef SEARCH_DEBUG
  blobs_dirty = 0;
#endif
}

protected string get_mergefilename()
{
  return combine_path(mergefile_path,
		      sprintf("mergefile%03d.dat", mergefile_counter));
}

protected void mergefile_sync()
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

protected string merge_mergefiles(array(string) mergefiles)
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
#ifdef SEARCH_DEBUG
  docs = 0;
#endif
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

protected string my_denormalize(string in)
{
  return Unicode.normalize(utf8_to_string(in), "C");
}

array(array) get_most_common_words(void|int count)
{
  array a =
    db->query("   SELECT word, " +
	      (supports_padded_blobs() ?
	       "         SUM(used_len) / 5 AS c " :
	       "         SUM(LENGTH(hits)) / 5 AS c ") +
	      "     FROM word_hit "
	      " GROUP BY word "
	      " ORDER BY c DESC "
	      "    LIMIT %d", count || 10);

  if(!sizeof(a))
    return ({ });
  else
    return Array.transpose( ({ map(a->word, my_denormalize),
			       (array(int))a->c }) );
}

void list_url_by_prefix(string url_prefix, function(string:void) cb)
{
  Sql.sql_result q =
    db->big_query("SELECT uri "
		  "  FROM uri "
		  " WHERE uri LIKE '"+db->quote(url_prefix)+"%'");
  for(;;) {
    array row = q->fetch_row();
    if(!row)
      break;
    cb(row[0]);
  }
}
