// SQL index database without fragments
// Copyright © 2000, Roxen IS.

inherit Search.Database.Base;

// Creates the SQL tables we need.
void create_tables()
{
  catch(db->query("drop table document"));
  catch(db->query("drop table word"));
  catch(db->query("drop table occurance"));
  db->query(
#"create table document (id int unsigned primary key auto_increment not null,
                         uri text not null,
                         uri_md5 char(32) not null,
                         title varchar(255),
                         description text,
                         last_indexed timestamp,
                         last_changed int unsigned,
                         size int unsigned,
                         mime_type smallint unsigned,
                         unique(uri_md5),
                         key(id,uri_md5))"
			 );
  //We're missing language...
  //domain could be tokenized

  db->query(
#"create table word (word varchar(255),
                     id int unsigned primary key)"
		     );

  db->query(
#"create table occurance (word_id int unsigned not null,
                          document_id int unsigned not null,
                          word_position mediumint unsigned not null,
                          ranking tinyint not null,
                          key(word_id,document_id))"
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
  string digest=md5->digest();
  string res="";
  foreach(values(digest), int c)
    res+=sprintf("%02x",c);
  return res;
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
	      "(uri, uri_md5, title, description, last_changed, size, mime_type)"
	      " VALUES ('%s', '%s', '%s', '%s', %s, %s, %s)",
	      uri, to_md5(uri), title, description, last_changed, size, mime_type);
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


mapping(string:Search.Document) make_document_mapping(array(mapping) mysql_result)
{
  array a= map(mysql_result,
	       lambda(mapping m)
	       {
		 Search.Document tmp=Search.Document();
		 tmp->title=m->title;
		 tmp->description=m->description;
		 tmp->last_changed=(int)m->last_changed;
		 tmp->uri=m->uri;
		 return tmp;
	       });
  return mkmapping(a->uri,a);
}

mapping(string:Search.Document) lookup_word(string word)
{
  array documents=db->query("SELECT document.* FROM document, occurance WHERE word_id=%s"
			    " and occurance.document_id=document.id",
			    hash_word(word));
  return make_document_mapping(documents);
}

mapping(string:Search.Document) lookup_words_or(array(string) words)
{
  string sql="SELECT * FROM occurance WHERE ";
  words=map(words, lambda(string in) {
		   return "word_id="+hash_word(in);
		 } );
  sql += words*" || ";

  array documents=db->query(sql);
  return make_document_mapping(documents);
}

mapping(string:Search.Document) lookup_words_and(array(string) words)
{
//   array first_result=({});
//   array rest_results=({});
  
//   first_result = lookup_word(words[0]);
//   foreach(words[1..], string word)
//   {
//     rest_results += ({ lookup_word(word) });
//     if(rest_results[-1]==({ 0 }))
//       return 0;
//     rest_results[-1]=(multiset)rest_results[-1]->uri;
//   }
  
//   array results=({});
//   foreach(first_result, mapping hit)
//     if(!has_value(rest_results[hit->uri], 0))
//       results += ({ hit });
//   return make_document_mapping(documents);
}
