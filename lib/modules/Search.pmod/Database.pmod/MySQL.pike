// SQL index database without fragments
// Copyright © 2000, Roxen IS.
//
// $Id: MySQL.pike,v 1.8 2001/03/18 05:19:46 js Exp $

inherit Search.Database.Base;

// Creates the SQL tables we need.

void create_tables()
{
  catch(db->query("drop table uri"));
  catch(db->query("drop table document"));
  catch(db->query("drop table field"));
  catch(db->query("drop table occurance"));
  db->query(
#"create table uri      (id          int unsigned primary key auto_increment not null,
                         uri_first   varchar(235),
                         uri_rest    text not null,
                         uri_md5     char(16) not null default '',
                         UNIQUE(uri_md5),
                         INDEX index_uri_first (uri_first(235))"
			 );

  db->query(
#"create table document (id            int unsigned primary key auto_increment not null,
                         uri_id        int unsigned not null,
                         language_code char(3) not null,
                         INDEX index_language_code (language_code),
                         INDEX index_uri_id (uri_id))"
			 );
  
  db->query(
#"create table occurance (word_id      int unsigned not null,
                          document_id  int unsigned not null,
                          offset       mediumint unsigned not null,
                          field_id     tinyint unsigned not null
                          INDEX index_offset (offset),
                          INDEX index_field_id (field_id),
                          INDEX index_word_id (word_id),
                          INDEX index_document_id (document_id))");

  db->query(
#"create table field (id    tinyint unsigned primary_key auto_increment not null,
                      name  varchar(127),
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
  return sprintf("Search.Database.MySQL( %s )", host);
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


// Takes about 50 us, i.e. not much. :)
int hash_word(string word)
{
  string hashed=Crypto.md5()->update(word[..254])->digest();
  return hashed[0]*16777216 +  // 2^24
    hashed[1]*65536 +  // 2>^16
    hashed[2]*256 +  // 2^8
    hashed[3];
}

int wc=0;

array(string) split_uri(string in)
{
  return ({ in[..218], in[219..] });
}


int find_or_create_uri_id(string uri)
{
  string s=sprintf("select id from uri where uri_md5='%s'", to_md5(uri));
  array a=db->query(s);
  if(sizeof(a))
    return (int)a[0]->id;

  s=sprintf("insert into uri (uri_first,uri_rest,uri_md5) "
	    "values ('%s','%s','%s')",
	    @map(uri, db->quote), to_md5(uri));
  db->query(s);
  return db->master_sql->insert_id();
}

int find_or_create_document_id(string uri, void|string language_code)
{
  int uri_id=find_or_create_uri_id(string uri);
  
  string s=sprintf("select id from document where "
		   "uri_id='%d'", uri_id);
  if(language)
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

//! Inserts the words of a resource into the database
void insert_words(Standards.URI|string uri, void|string language,
		  string field, array words)
{
  int doc_id=find_or_create_document_id((string)uri, language);
  int field_id=find_or_create_field_id(field);

  mapping word_ids=([]);
  int offset;
  string s="insert into occurance (word_id, document_id, offset, "
           "field_id) values ";
  
  foreach(words, mapping word)
  {
    int word_id;
    string the_word=word->word;
    if(!(word_id=word_ids[the_word]))
      word_id=hash_word(the_word);
    s+=sprintf("(%d,%d,%d,%d),",
	       word_id, doc_id, offset++, field_id);
    wc++;
  }
  if(sizeof(words))
    db->query(s[..sizeof(s)-2]);
}

void remove_document(string uri)
{
}


class Node
{
  mapping build_sql_components(mapping(string:int) ref);
}

class GroupNode
{
  inherit Node;

  static privage string build_sql_part(string what, array params, string delimit)
  {
    return sprintf("%-7s %s\n",what,Array.uniq(params)*delimit);
  }

  string build_sql(void|int limit, void|mapping(string:array(string)) extra_select_items)
  {
    mapping ref=(["ref":0]);
    if(!extra_select_items) extra_select_items=([]);
    mapping sub_res=build_sql_components(ref);
    
    array tmp=({});
    foreach(indices(extra_select_items), string tablename)
      tmp+=map(extra_select_items[tablename],
	       lambda(string field){return tablename+"."+field;});
    if(sizeof(sub_res->ranking)
       tmp+=({sub_res->ranking*" +\n       "+"as ranking"});
    
    string res="";
    res+=build_sql_part("SELECT",
			({ "distinct document.id as doc_id",
			   "concat(uri.uri_first,uri.uri_rest) as doc_uri"})+tmp,
			",\n       ");

    res+=build_sql_part("FROM",
			sub_res->from+indices(extra_select_items)+({"document", "uri"}),
			",\n       ");

    tmp=({});
    if(ref->ref>1)
    {
      tmp=allocate(ref->ref-1);
      for(int i=1;i<ref->ref; i++)
	tmp[i-1]=sprintf("t0.doc_id=t%d.doc_id",i);
      tmp+=({"t0.doc_id=document.id"})
    }
    
    res+=build_sql_part("WHERE",
			({"document.uri_id=uri.id",sub_res->where})+tmp,
			" and\n       ");

    res+="group by document.id";

    if(sizeof(sub_res->ranking)
       res+=" order by ranking desc";

    if(limit)
      res+=sprintf(" limit %d",limit);
    return res+"\n";
  }
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
  
  mapping build_sql_components(mapping(string:int) ref)
  {
    array(mapping) children_tmp=children->build_sql_components(ref);
    return ([ "from": Array.flatten(children_tmp->from),
	      "ranking": Array.flatten(children_tmp->ranking),
	      "where": "("+children_tmp->where*" AND\n       "+")"]);
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
  
  mapping build_sql_components(mapping(string:int) ref)
  {
    array(mapping) children_tmp=children->build_sql_components(ref);
    return ([ "from": Array.flatten(children_tmp->from),
	      "ranking": Array.flatten(children_tmp->ranking),
	      "where": "("+children_tmp->where*" OR\n       "+")"]);
  }
}

class Not(Node child)
{
  inherit GroupNode;

  mapping build_sql_components(mapping(string:int) ref)
  {
    mapping child_tmp=child->build_sql_components(ref);
    child_tmp->where="not "+child_tmp->where;
    return child_tmp;
  }
}

class Contains(string field, string word)
{
  inherit LeafNode;

  mapping build_sql_components(mapping(string:int) ref)
  {
    return (["from": ({"field", sprintf("occurance t%d",ref->ref)}),
	     "ranking": ({/*sprintf("sum((t%d.tf * t%d.idf * t%d.idf)/document.length)",
			    ref->ref,ref->ref,ref->ref)*/}),
	     "where":  sprintf("t%d.field_id=field.id and field.name='%s' and t%d.word='%s'",
			       ref->ref,db->quote(field),ref->ref++,hash_word(word)) ]);
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
  
  mapping build_sql_components(mapping(string:int) ref, void|int window_size)
  {
    int real_window_size=window_size||1;
    if(sizeof(words)<2)
      return Contains(field, @words)->build_sql_components(ref);
    
    array(string) from=({}), where=({}), ranking=({});
    int i=0;
    
    where+=({ sprintf("t%d.field_id=field.id",ref->ref) });
    if(arrayp(field))
      where+=({ sprintf("field.name in (%s)'",
			map(field, lambda(string f)
				   {
				     return sprintf("'%s'",db->quote(f));
				   }) * ", ") });
    else
      where+=({ sprintf("field.name='%s'", db->quote(field)) });
    
    foreach(words, string word)
    {
//       ranking+=({sprintf("sum((document.tf * t%d.idf * t%d.idf)/document.length)",
// 			 ref->ref,ref->ref)});
      from+=({ sprintf("occurance t%d",ref->ref) });
      where+= ({ sprintf("t%d.word='%s'",ref->ref,hash_word(word)) });
      if(i++<sizeof(words)-1)
	where+=({ sprintf("t%d.offset-t%d.offset = "+real_window_size,
			  ref->ref+1, ref->ref) });
      ref->ref++;
    }
    return ([ "from": ({"field"})+from,
              "ranking":ranking,
	      "where": "("+where*" and\n       "+")" ]);
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

  mapping build_sql_components(mapping(string:int) ref)
  {
    return ([ "where": "uri.uri_first like '"+db->quote(prefix)+"'", 
	      "ranking": ({ }),
	      "from": ({ }) ]);
  }
}

class Language(/*Linguistics.Language*/string language)
{
  inherit LeafNode;

  mapping build_sql_components(mapping(string:int) ref)
  {
    return ([ "where": "document.language_code ='"+(string)language+"'",
	      "ranking": ({ }),
	      "from": ({ }) ]);
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


// int main()
// {
//   write(And(Or(Contains("body","foo"), Contains("body","bar"),Phrase("title","tjo","hej")),
// 	    Phrase("title","roxen","internet","software"))->build_sql());
// }
 
