// SQL index database without fragments
// Copyright © 2000, Roxen IS.

inherit Search.Database.Base;

// Creates the SQL tables we need.
// FIXME: last_changed and last_indexed should be in the same format. (fix here or in query?)
// FIXME: last changed and last_indexed should be named modified and indexed.
void create_tables()
{
  catch(db->query("drop table document"));
  catch(db->query("drop table word"));
  catch(db->query("drop table occurance"));
  db->query(
#"create table uri      (id int unsigned primary key auto_increment not null,
                         uri_first varchar(235),
                         uri_rest text not null,
                         uri_md5 char(16) not null default '',
                         protocol_id tinyint,
                         UNIQUE(uri_md5),
                         INDEX index_uri_first (uri_first(235))"
			 );

  db->query(
#"create table language (id smallint unsigned primary key auto_increment not null,
                         language_code char(3) not null)"
                         );
  db->query(
#"create table document (id int unsigned primary key auto_increment not null,
                         uri_id int unsigned not null,
                         title varchar(255),
                         description text,
                         last_indexed timestamp,
                         last_changed int unsigned,
                         size int unsigned,
                         mime_type smallint unsigned,
                         language_id smallint unsigned)"
			 );
  
  db->query(
#"create table mime_type (id smallint unsigned primary_key auto_increment not null,
                          name varchar(127))"
                          );
                          
  db->query(
#"create table word (word varchar(255),
                     id int unsigned primary key)"
		     );

  db->query(
#"create table occurance (word_id int unsigned not null,
                          document_id int unsigned not null,
                          word_position mediumint unsigned not null,
                          ranking tinyint not null,
                          INDEX index_word_id (word_id),
                          INDEX index_document_id (document_id))"
			  );
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
  array res=db->query("SELECT last_changed, last_indexed, size "
		      "FROM document WHERE uri_md5='%s'", to_md5(uri));
  if(!sizeof(res)) return 0;
  return (mapping(string:int))res[0];
}


// Takes about 50 us, i.e. not much. :)
int hash_word(string word) {
  string hashed=Crypto.md5()->update(word[..254])->digest();
  return hashed[0]*16777216 +  // 2^24
    hashed[1]*65536 +  // 2^16
    hashed[2]*256 +  // 2^8
    hashed[3];
}

int wc=0;

array(string) split_uri(string in)
{
  return ({ in[..218], in[219..] });
}

// Insert or update a page in the database.
// title and description is already in words.
void insert_page(string uri, string title, string description, int last_changed,
		 int size, int mime_type, array words)
{
  // Find out our document id
  int doc_id;
  if( catch( doc_id=(int)db->query("SELECT id FROM document "
				   "WHERE uri_md5='%s'", to_md5(uri))[0]->id ))
    doc_id=0;


  int new;
  if(!doc_id)
  {
    db->query("INSERT INTO document "
	      "(uri_first, uri_rest, uri_md5, title, description, last_changed, size, mime_type)"
	      " VALUES ('%s', '%s', '%s', '%s', '%s', %s, %s, %s)",
	      @split_uri(uri), to_md5(uri), title, description, last_changed, size, mime_type);
    werror("[%s] ",uri);
    doc_id = db->master_sql->insert_id();
    new=1;
  }
  else
  {
    // Page was already indexed.
    db->query("UPDATE document SET title='%s', description='%s', "
	      "last_changed='%s', size='%s', mime_type='%s' WHERE id=%s",
	      title, description, last_changed, size, mime_type, doc_id);
    db->query("DELETE FROM occurance WHERE document_id=%s", doc_id);
  }

  // Add word occurances.
  // Should a proper cache be used?
  werror("doc_id: %O\n",doc_id);

  mapping word_ids=([]);
  int word_pos;
  string s="INSERT INTO occurance (word_id, document_id, word_position, "
           "ranking) VALUES ";
  foreach(words, mapping word)
  {
    int word_id;
    string the_word=word->word;
    if(!(word_id=word_ids[the_word])) {
      word_id=hash_word(the_word);
      //       word_ids[the_word]=word_id;
      //       array res = db->query("SELECT word FROM word WHERE id="+word_id);
      //       if(!sizeof(res))
      // 	db->query("INSERT INTO word (id,word) VALUES (%s,'%s')",
      // 		  word_id, the_word);
    }
    s+=sprintf("(%d,%d,%d,%d),",
	       word_id, doc_id, word_pos++, word->rank);
    wc++;
  }
  if(sizeof(words))
    db->query(s[..sizeof(s)-2]);
}

void remove_page(string uri)
{
  int doc_id;
  mixed error=catch( doc_id=db->query("SELECT id FROM document WHERE uri_md5='%s'",
				      to_md5(uri) )[0]->id );
  if(error) return;
  db->query("REMOVE FROM document WHERE id=%s", doc_id);
  db->query("REMOVE FROM occurence WHERE document_id=%s", doc_id);
  removed++;
}

void garbage_collect()
{
  removed=0;

  array ids;
  if( catch( ids = db->query("SELECT id FROM word")->id ))
    return;

  foreach(ids, string id) {
    int existence = sizeof(db->query("SELECT id FROM occurance WHERE word_id="+id+" LIMIT 1"));
//     if(!existence) db->query("REMOVE FROM word WHERE word_id="+id);
  }

//   db->query("OPTIMIZE TABLE word");
  db->query("OPTIMIZE TABLE occurance");
  db->query("OPTIMIZE TABLE document");
}

void optimize()
{
  garbage_collect();
}


class Node
{
  mapping build_sql_components(mapping(string:int) ref);
}

class GroupNode
{
  inherit Node;

  string build_sql(void|int limit, void|mapping(string:array(string)) extra_select_items)
  {
    mapping ref=(["ref":0]);
    mapping sub_res=build_sql_components(ref);
    if(!sizeof(sub_res->ranking))
      sub_res->ranking=({"0.0"});
    string res="";
    res+="select distinct t0.doc_id as doc_id,\n       "+
      sub_res->ranking*" +\n       "+" as ranking\n";

    int has_extra_comma=0;
    if(extra_select_items)
    {
      foreach(indices(extra_select_items), string tablename)
      {
	if(sizeof(extra_select_items[tablename]))
	{
	  res+=
	    "       "+
	    map(extra_select_items[tablename],
		lambda(string field){return tablename+"."+field;})*", "+
	    ",\n";
	  has_extra_comma=1;
	}
      }
      if(has_extra_comma)
	res=res[..sizeof(res)-3]+"\n";
    }
    res+="from   ";

    if(extra_select_items)
      res+=indices(extra_select_items)*", ";

    res+=sub_res->from*", " + "\n";

    res+="where  "+sub_res->where;
    if(ref->ref>1)
      res+=" and";
    res+="\n";

    if(ref->ref>1)
    {
      array tmp=allocate(ref->ref-1);
      for(int i=1;i<ref->ref; i++)
	tmp[i-1]=sprintf("t0.doc_id=t%d.doc_id",i);
      res+="       "+tmp*" and ";
      res+="\n";
    }

    res+="group by t0.doc_id order by ranking desc";

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
    return (["from": ({sprintf("occurance_%s t%d",field,ref->ref)}),
	     "ranking": ({/*sprintf("sum((t%d.tf * t%d.idf * t%d.idf)/document.length)",
			    ref->ref,ref->ref,ref->ref)*/}),
	     "where":  sprintf("t%d.word='%s'",ref->ref++,word /*FIXME: quote*/) ]);
  }
}

class Phrase
{
  inherit LeafNode;

  string field;
  array(string) words;
  void create(string _field, string ... _words)
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
    foreach(words, string word)
    {
//       ranking+=({sprintf("sum((document.tf * t%d.idf * t%d.idf)/document.length)",
// 			 ref->ref,ref->ref)});
      from+=({ sprintf("occurance_%s t%d",field,ref->ref) });
      where+= ({ sprintf("t%d.word='%s'",ref->ref,word) });  /*FIXME: quote*/
      if(i++<sizeof(words)-1)
	where+=({ sprintf("t%d.offset-t%d.offset = "+real_window_size,
			  ref->ref+1, ref->ref) });
      ref->ref++;
    }
    return ([ "from": from,
              "ranking":ranking,
	      "where": "("+where*" and\n       "+")" ]);
  }
}

class Near
{
  inherit LeafNode;
  
  string field;
  array(string) words;
  int window_size;
  void create(string _field, int _window_size, string ... _words)
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
