// SQL index database without fragments
// Copyright © 2000, Roxen IS.
//
// $Id: MySQL.pike,v 1.13 2001/04/05 10:26:27 js Exp $

// inherit Search.Database.Base;

// Creates the SQL tables we need.

void create_tables()
{
  catch(db->query("drop table uri"));
  catch(db->query("drop table document"));
  catch(db->query("drop table occurance"));
  catch(db->query("drop table field"));
  
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
                         language_code char(3) not null,
                         INDEX index_language_code (language_code),
                         INDEX index_uri_id (uri_id))"
			 );
  
  db->query(
#"create table occurance (word         int unsigned not null,
                          document_id  int unsigned not null,
                          offset       mediumint unsigned not null,
                          field_id     tinyint unsigned not null,
                          INDEX index_offset (offset),
                          INDEX index_field_id (field_id),
                          INDEX index_word (word),
                          INDEX index_document_id (document_id))");

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
  return sprintf("Search.Database.MySQL(%s)", host);
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


// Useful information for crawlers
mapping(string:int) page_stat(string uri)
{
//   array res=db->query("SELECT last_changed, last_indexed, size "
// 		      "FROM document WHERE uri_md5='%s'", to_md5(uri));
//   if(!sizeof(res)) return 0;
//   return (mapping(string:int))res[0];
}


int hash_word(string word)
{
  string hashed=Crypto.md5()->update(word[..254])->digest();
  return hashed[0]*16777216 +  // 2^24
    hashed[1]*65536 +  // 2>^16
    hashed[2]*256 +  // 2^8
    hashed[3];
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

int find_or_create_field_id(string field)
{
  string s=sprintf("select id from field where name='%s'",db->quote(field));
  array a=db->query(s);
  if(sizeof(a))
    return (int)a[0]->id;

  s=sprintf("insert into field (name) values ('%s')", db->quote(field));
  db->query(s);
  return db->master_sql->insert_id();
}

//! Inserts the words of a resource into the database
void insert_words(Standards.URI|string uri, void|string language,
		  string field, array(string) words)
{
  int doc_id=find_or_create_document_id((string)uri, language);
  int field_id=find_or_create_field_id(field);

  mapping word_ids=([]);
  int offset;
  string s="insert into occurance (word, document_id, offset, "
           "field_id) values ";
  
  foreach(words, string word)
    s+=sprintf("(%d,%d,%d,%d),",
	       hash_word(word), doc_id, offset++, field_id);

  if(sizeof(words))
    db->query(s[..sizeof(s)-2]);
}

void remove_document(string uri)
{
}


string create_temp_table(Sql.sql db, int num)
{
  db->query("create temporary table t"+num+"("+
	    "document_id int not null, "
	    "ranking float not null, "
	    "key(document_id))");
  return "t"+num;
}

class Node
{
}

class GroupNode
{
  inherit Node;
}

class LeafNode
{
  inherit Node;
}

class And
{
  inherit GroupNode;
  
  array(Node) children;
  void create(Node ... _children)
  {
    children=_children;
  }
  
  string do_sql(Sql.sql db, mapping ref)
  {
    string temp_table=create_temp_table(db,ref->ref++);
    array tables=children->do_sql(db, ref);
    
    // FIXME: Optimization for NOT nodes
    
    // AND(t0,t1,t2,NOT(t3))
    //   join t0,t1,t2
    //   sub t3
    //
    // select t0.document_id as document_id, MIN(t0.ranking,t1.ranking,t2.ranking) as ranking
    // from t0,t1,t2
    // where t0.document_id=t1.document_id and t0.document_id=t2.document_id
    string s=sprintf("insert into %s (document_id,ranking) "
		     "select %s.document_id as document_id, MIN(%s) as ranking "
		     "from %s where %s",
		     temp_table,
		     tables[0],
		     map(tables,`+,".ranking")*",",
		     tables*",",
		     map(tables[1..],`+,sprintf("=%s.document_id",tables[0])) * " and ");
    foreach(tables, string table)
      db->query("drop table "+table);

    return temp_table;
  }
}

class Or
{
  inherit GroupNode;

  array(Node) children;
  void create(Node ... _children)
  {
    children=_children;
  }
  
  string do_sql(Sql.sql db, mapping ref)
  {
    // FIXME: Optimization for NOT nodes (?)
    string temp_table=create_temp_table(db,ref->ref++);
    array tables=children->do_sql(db, ref);

    foreach(tables, string table)
      db->query("insert into "+temp_table+" (document_id, ranking) "
		"select document_id,ranking from "+table);

    string temp_table2=create_temp_table(db,ref->ref++);

    db->query("insert into "+temp_table2+" (document_id, ranking) "
	      "select document_id,max(ranking) from "+temp_table+
	      " group by document_id");

    db->query("drop table "+temp_table);
    
    return temp_table2;
  }
}

class Not(Node child)
{
  inherit GroupNode;
  
  string do_sql(Sql.sql db, mapping ref)
  {
    child->do_sql(db,ref);
  }
}

class Contains(string field, string word)
{
  inherit LeafNode;

  void do_sql(Sql.sql db, mapping ref)
  {
    string temp_table=create_temp_table(db,ref->ref++);
    db->query("insert into "+temp_table+" (document_id,ranking) "
	      "select document_id, count(document_id) as ranking "
	      "from occurance where word=%s group by document_id",hash_word(wword));
  }
}


class Phrase
{
  inherit LeafNode;
  string|array(string) field;
  array(string) words;
  void create(string|array(string) _field, string ... _words)
  {
    field=_field;
    words=_words;
  }

  string create_temp_offset_table(Sql.sql db, int num)
  {
      db->query("create temporary table to"+num+"("+
	    "offset int not null, "
	    "key(document_id), key(offset))");
      return "to"+num;
  }
  
  string do_sql(Sql.sql db, mapping ref, void|int window_size)
  {
    if(sizeof(words)<2)
      return Contains(field, @words)->do_sql(db,ref);
    int real_window_size=window_size||1;

    array(string) offset_tables=({});
    string temp_table;
    foreach(words, string word)
    {
      int i=0;
      offset_tables+=({ temp_table=create_temp_offset_table(db,i++) });
      db->query("insert into "+temp_table+" (document_id,offset) "
		"select occurance.document_id, occurance.offset "
		"from occurance where occurance.word=%d",hash_word(word));
    }
    
    temp_table=create_temp_table(db,ref->ref++);

    string s=sprintf(" select %s.document_id, 0.0 as ranking from %s"
		     "where",offset_table[0], offset_tables*", ");

    for(int i=1; i++; i<sizeof(offset_tables)-1)
      s+=sprintf("%s.document_id=%s.document_id and %s.offset-%s.offset=%d ",
		 offset_tables[0], offset_tables[i],
		 offset_tables[i], offset_tables[i]-1,
		 real_window_size);
    
    s+=sprintf(" group by %s.document_id", offset_tables[0]);
    db->query(s);
    
    foreach(offset_tables, string offset_table)
      db->query("drop table "+offset_table);
    return temp_table;
  }
}


class Near
{
  inherit LeafNode;
  
  string|array(string) field;
  array(string) words;
  int window_size;
  void create(string|array(string) _field, int _window_size, string ... _words)
  {
    window_size=_window_size,
    field=_field;
    words=_words;
  }

  mapping build_sql_components(mapping(string:int) ref)
  {
    return Phrase(field,@words)->build_sql_components(ref,window_size);
  }
}

class URIPrefix(string prefix)
{
  inherit LeafNode;
  
  string do_sql(Sql.sql db, mapping ref)
  {
    string temp_table=create_temp_table(db,ref->ref++);
    db->query("insert into "+temp_table+" (document_id,ranking) "
	      "select document.id as document_id, 0.0 as ranking "
	      "from document,uri where document.uri_id=uri.id and uri.first like %s",prefix);
    return temp_table;
  }
}

class Language(string language)
{
  inherit LeafNode;

  string do_sql(Sql.sql db, mapping ref)
  {
    string temp_table=create_temp_table(db,ref->ref++);
    db->query("insert into "+temp_table+" (document_id,ranking) "
	      "select id as document_id, 0.0 as ranking "
	      "from document where language_code=%s",language);
    return temp_table;
  }
}

class FloatOP(string field, string op, float value)
{
  inherit LeafNode;
}

class IntegerOP(string field, string op, float value)
{
  inherit LeafNode;
}

class DateOP(string field, string op, string date)
{
  inherit LeafNode;
}

string foo()
{
  return And(Contains("body","tjo"), Phrase("body","bar","gazonk"))->build_sql();
}

// int main()
// {
//   write(And(Or(Contains("body","foo"), Contains("body","bar"),Phrase("title","tjo","hej")),
// 	    Phrase("title","roxen","internet","software"))->build_sql());
// }
 
