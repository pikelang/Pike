// SQL blob based database
// Copyright © 2000,2001 Roxen IS.
//
// $Id: MySQL.pike,v 1.24 2001/06/05 22:54:24 per Exp $

inherit Search.Database.Base;

// Creates the SQL tables we need.

//#define DEBUG
void recreate_tables()
{
  catch(db->query("drop table uri"));
  catch(db->query("drop table deleted_document"));
  catch(db->query("drop table document"));
  catch(db->query("drop table occurance "));
  catch(db->query("drop table field"));
  catch(db->query("drop table word_hit"));
  catch(db->query("drop table metadata"));
  
  db->query(
#"create table uri      (id          int unsigned primary key auto_increment not null,
                         uri         text not null,
                         uri_md5     char(16) not null,
                         UNIQUE(uri_md5))"
			 );

  db->query(
#"create table document (id            int unsigned primary key auto_increment not null,
                         uri_id        int unsigned not null,
                         language_code char(3),
                         INDEX index_language_code (language_code),
                         INDEX index_uri_id (uri_id))"
			 );
  
  db->query("create table deleted_document (id int unsigned not null)");

  db->query(
#"create table word_hit (word_id        int not null,
                         first_doc_id   int not null,
            	         hits           mediumblob not null,
                         unique(word_id,first_doc_id))");

  db->query(
#"create table metadata (doc_id        int not null,
                         name          varchar(32) not null,
            	         value         mediumblob not null,
                         unique(doc_id,name))");

  db->query(
#"create table field (id    tinyint unsigned primary key auto_increment not null,
                      name  varchar(127) not null,
                      INDEX index_name (name))");
}

// This is the database object that all queries will be made to.
object db;
int removed;

string host;

void create(string _host)
{
  db=Sql.sql(host=_host);
}

string _sprintf()
{
  return sprintf("Search.Database.MySQL(%O)", host);
}

mapping stats()
{
  mapping tmp=([]);
  tmp->words=(int)db->query("select count(*) as c from occurance")[0]->c;
  tmp->documents=(int)db->query("select count(*) as c from document")[0]->c;
  return tmp;
}

string to_md5(string url)
{
  object md5 = Crypto.md5();
  md5->update(url);
  return md5->digest();
}

int hash_word(string word)
{
  return hash(word);
  string hashed=Crypto.md5()->update(word[..254]*16)->digest();
  int c;
  sscanf(hashed,"%4c",c);
  return c;
}


int get_uri_id(string uri, void|int do_not_create)
{
  string s=sprintf("select id from uri where uri_md5='%s'", db->quote(to_md5(uri)));
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

int get_document_id(string uri, void|string language_code)
{
  int uri_id=get_uri_id(uri);
  
  string s=sprintf("select id from document where "
		   "uri_id='%d'", uri_id);
  if(language_code)
    s+=sprintf(" and language_code='%s'",db->quote(language_code));

  array a = db->query(s);
  if(sizeof(a))
    return (int)a[0]->id;

  s=sprintf("insert into document (uri_id, language_code) "
	    "values ('%d',%s)",
	    uri_id,
	    language_code?("'"+language_code+"'"):"NULL");

  db->query(s);
  return db->master_sql->insert_id();
}

mapping field_cache = ([]);
int get_field_id(string field, void|int do_not_create)
{
  if(field=="body")      return 0;
  if(field=="anchor")    return 1;
  if(field_cache[field]) return field_cache[field];
  
  string s=sprintf("select id from field where name='%s'",db->quote(field));
  array a=db->query(s);
  if(sizeof(a))
  {
    field_cache[field]=(int)a[0]->id+1;
    return (int)a[0]->id+1;
  }

  if(do_not_create)
    return 0;

  // FIXME: Write a custom id allocation algorithm
  s=sprintf("insert into field (name) values ('%s')", db->quote(field));
  db->query(s);
  return db->master_sql->insert_id()+1;
}

_WhiteFish.Blobs blobs = _WhiteFish.Blobs();

#define MAXMEM 20*1024*1024

//! Inserts the words of a resource into the database
void insert_words(Standards.URI|string uri, void|string language,
		  string field, array(string) words, void|int link_hash)
{
  if(!sizeof(words))  return;

  int doc_id   = get_document_id((string)uri, language);
  int field_id = get_field_id(field);

  blobs->add_words_hash( doc_id, words, field_id, link_hash );

  if(blobs->memsize() > MAXMEM)
    sync();
}

void set_metadata(Standards.URI|string uri, void|string language,
		  mapping(string:string) md)
{
  int doc_id;
  if(!intp(uri))
    doc_id = get_document_id((string)uri, language);
  if(md->body)
    md->body = Gz.deflate(6)->deflate(string_to_utf8(md->body[..64000]),
				      Gz.FINISH);

  if(!sizeof(md))
    return 0;

  string s=map(Array.transpose( ({ map(indices(md),db->quote),
				   map(values(md), db->quote) }) ),
	       lambda(array a)
	       {
		 return sprintf("(%d,'%s','%s')", doc_id,
				a[0], string_to_utf8(a[1]));
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
  array a=db->query("select document.language_code,uri.uri from document,uri "
		    "where uri.id=document.uri_id and document.id=%d",doc_id);
  if(!sizeof(a))
    return 0;

  return (["uri":1,"language":1]) & a[0];
}

int docs;
void remove_document(string|Standards.URI uri, void|string language)
{
  int uri_id=get_uri_id((string)uri);

  if(!uri_id)
    return;
  array a;
  if(language)
    a=db->query("select id from document where uri_id=%d and "
		"language_code=%s",uri_id,language);
  else
    a=db->query("select id from document where uri_id=%d",uri_id);

  if(!sizeof(a))
    return;
  
  db->query("delete from document where id in ("+a->id*","+")");
  db->query("insert into deleted_document (doc_id) values "+
	    "("+a->id*"),("+")");
  docs++;
}


void sync_thread( _WhiteFish.Blobs blobs, int docs )
{
  int s = time();
  int q;
  Sql.Sql db = Sql.Sql( host );
  werror("----------- sync() %4d docs --------------\n", docs);
  do
  {
    [int i, _WhiteFish.Blob b] = blobs->read();
    q++;
    if( !b )
      break;
    string d = b->data();
    int w;
    sscanf( d, "%4c", w );
    db->query("insert into word_hit (word_id,first_doc_id,hits) "
	      "values (%d,%d,%s)", i, w, d );
  } while( 1 );
  werror("----------- sync() done %3ds %5dw -------\n", time()-s,q);
}

object old_thread;
int sync()
{
#if constant(thread_create)
  if( old_thread )
    old_thread->wait();
  old_thread = thread_create( sync_thread, blobs, docs );
#else
  sync_thread( blobs, docs );
#endif
  blobs = _WhiteFish.Blobs();
  docs = 0;
}

string get_blob(int word_id, int num)
{
  array a=db->query("select hits,first_doc_id from word_hit where word_id=%d "
		    "limit %d,1",
		    word_id, num);

//    if(sizeof(a))
//      werror("get_blob(%d,%d) -> %d bytes, first_doc_id: %s\n",
//  	   word_id,num,sizeof(a[0]->hits),a[0]->first_doc_id);
//    else
//      werror("get_blob(%d,%d) -> EOF\n", word_id, num);

  if(!sizeof(a))
    return 0;

  return a[0]->hits;
}

void zap()
{
  db->query("delete from word_hit");
  db->query("delete from uri");
  db->query("delete from document");
  db->query("delete from field");
}


// Query functions
// -------------------------------------

