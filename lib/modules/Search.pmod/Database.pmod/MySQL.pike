// SQL blob based database
// Copyright © 2000,2001 Roxen IS.
//
// $Id: MySQL.pike,v 1.16 2001/05/25 20:55:34 js Exp $

// inherit Search.Database.Base;

// Creates the SQL tables we need.

//#define DEBUG
void create_tables()
{
  catch(db->query("drop table uri"));
  catch(db->query("drop table document"));
  catch(db->query("drop table occurance "));
  catch(db->query("drop table field"));
  catch(db->query("drop table word_hit"));
  
  db->query(
#"create table uri      (id          int unsigned primary key auto_increment not null,
                         uri_first   varchar(235) not null,
                         uri_rest    text not null,
                         uri_md5     char(16) not null,
                         UNIQUE(uri_md5),
                         INDEX index_uri_first (uri_first(235)))"
			 );

  db->query(
#"create table document (id            int unsigned primary key auto_increment not null,
                         uri_id        int unsigned not null,
                         language_code char(3),
                         INDEX index_language_code (language_code),
                         INDEX index_uri_id (uri_id))"
			 );
  

  db->query(
#"create table word_hit (word_id        int not null,
                          first_doc_id   int not null,
            	          hits           mediumblob not null,
                          unique(word_id,first_doc_id))");

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

array(string) split_uri(string in)
{
  return ({ in[..218], in[219..] });
}


int find_or_create_uri_id(string uri)
{
  string s=sprintf("select id from uri where uri_md5='%s'", db->quote(to_md5(uri)));
  array a=db->query(s);
  if(sizeof(a))
    return (int)a[0]->id;

  s=sprintf("insert into uri (uri_first,uri_rest,uri_md5) "
	    "values ('%s','%s','%s')",
	    @map(split_uri(uri), db->quote), db->quote(to_md5(uri)));
  db->query(s);
  return db->master_sql->insert_id();
}

int find_or_create_document_id(string uri, void|string language_code)
{
  int uri_id=find_or_create_uri_id(uri);
  
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
int find_or_create_field_id(string field)
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

  s=sprintf("insert into field (name) values ('%s')", db->quote(field));
  db->query(s);
  return db->master_sql->insert_id()+1;
}


mapping blobs=([]);

#define MAXMEM 8*1024*1024

//! Inserts the words of a resource into the database
void insert_words(Standards.URI|string uri, void|string language,
		  string field, array(string) words, void|int link_hash)
{
  if(!sizeof(words))
    return;
  werror("field: %O  num words: %d\n",field,sizeof(words));
  int doc_id=find_or_create_document_id((string)uri, language);
  int field_id;
  field_id=find_or_create_field_id(field);

  mapping word_ids=([]);
  int offset;
  float f;
  foreach(words, string word)
  {
    int hsh=hash_word(word);
    if(!blobs[hsh])
      blobs[hsh]=_WhiteFish.Blob();
    blobs[hsh]->add(doc_id, field_id, offset++, link_hash);
  }
  
  int memsize=`+(0, @values(blobs)->memsize());
  werror("memsize: %d  num: %d\n", memsize, sizeof(blobs));
  
  if(memsize> MAXMEM)
    sync();
}

void remove_document(string uri)
{
}


int sync()
{
  werror("----------- sync() --------------\n");
  foreach(indices(blobs), int word_id)
  {
    werror(".");
    array a=db->query("select hits from word_hit where word_id=%d",word_id);
    if(sizeof(a))
      blobs[word_id]->merge(`+("",@a->hits));
    
    db->query("replace into word_hit (word_id,first_doc_id,hits) "
	      "values (%d,%d,%s)", word_id, 0, blobs[word_id]->data());
  }
  blobs=([]);
}

string get_blob(int word_id, int num)
{
  if(num) return 0;

  array a=db->query("select hits from word_hit where word_id=%d",
		    word_id);

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

