// SQL index database without fragments
// Copyright © 2000, Roxen IS.


// Creates the SQL tables we need.
void create_tables()
{
  catch(db->query("drop table document"));
  catch(db->query("drop table word"));
  catch(db->query("drop table occurance"));
  db->query(
#"create table document (id int unsigned primary key auto_increment,
                         uri varchar(255),
                         title varchar(255),
                         description text,
                         last_indexed timestamp,
                         last_changed int unsigned,
                         size int unsigned,
                         mime_type smallint unsigned)"
			 );
  //We're missing language...
  //domain could be tokenized

  db->query(
#"create table word (word varchar(255),
                     id int unsigned primary key)"
		     );

  db->query(
#"create table occurance (word_id int unsigned,
                          document_id int unsigned,
                          word_position mediumint unsigned,
                          ranking tinyint)"
			  );
}

// This is the database object that all queries will be made to.
object db;
int removed;
void create(string host)
{
  db=Sql.sql(host);
}

// Useful information for crawlers
mapping(string:int) page_stat(string uri)
{
  array res=db->query("SELECT last_changed, last_indexed, size "
		      "FROM document WHERE uri='%s'", uri);
  if(!sizeof(res)) return 0;
  return (mapping(string:int))res[0];
}

int hash_word(string word) {
  string hashed=Crypto.md5()->update(word[..254])->digest();
  return hashed[0]*16777216 +  // 2^24
    hashed[1]*65536 +  // 2^16
    hashed[2]*256 +  // 2^8
    hashed[3];
}

// Insert or update a page in the database.
// title and description is already in words.
void insert_page(string uri, string title, string description, int last_changed,
		 int size, int mime_type, array words)
{
  // Find out our document id
  int doc_id;
  if( catch( doc_id=(int)db->query(sprintf("SELECT id FROM document "
					   "WHERE uri='%s'", uri))[0]->id ))
    doc_id=0;


  int new;
  if(!doc_id)
  {
    db->query(sprintf("INSERT INTO document "
		      "(uri, title, description, last_changed, size, mime_type)"
		      " VALUES ('%s', '%s', '%s', %d, %d, %d)",
		      uri, title, description, last_changed, size, mime_type));
    doc_id = (int)db->query("SELECT id FROM document WHERE uri='%s'", uri)[0]->id;
    new=1;
  }
  else
  {
    // Page was already indexed.
    db->query(sprintf("UPDATE document SET title='%s', description='%s', "
		      "last_changed='%d', size='%d', mime_type='%d'",
		      title, description, last_changed, size, mime_type));
    db->query(sprintf("DELETE FROM occurance WHERE document_id=%d", doc_id));
  }

  // Add word occurances.
  // Should a proper cache be used?
  werror("doc_id: %O\n",doc_id);

  mapping word_ids=([]);
  int word_pos;
  foreach(words, mapping word)
  {
    int word_id;
    string the_word=word->word;
    if(!(word_id=word_ids[the_word])) {
      word_id=hash_word(the_word);
      word_ids[the_word]=word_id;
      array res = db->query("SELECT word FROM word WHERE id="+word_id);
      if(!sizeof(res))
	db->query(sprintf("INSERT INTO word (id,word) VALUES (%d,'%s')",
			  word_id, db->quote(the_word)));
    }
    db->query(sprintf("INSERT INTO occurance (word_id, document_id, word_position, "
	      "ranking) VALUES (%d, %d, %d, %d)", word_id, doc_id,
	      word_pos++, word->rank));
  }

}

void remove_page(string uri)
{
  int doc_id;
  mixed error=catch( doc_id=db->query("SELECT id FROM document WHERE uri='%s'",
				      uri)[0]->id );
  if(error) return;
  db->query("REMOVE FROM document WHERE id=%d", doc_id);
  db->query("REMOVE FROM occurence WHERE document_id=%d", doc_id);
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
    if(!existence) db->query("REMOVE FROM word WHERE word_id="+id);
  }

  db->query("OPTIMIZE TABLE word");
  db->query("OPTIMIZE TABLE occurance");
  db->query("OPTIMIZE TABLE document");
}

array lookup_word(string word) {
  array documents=db->query("SELECT * FROM occurance WHERE word_id=%d",
			    hash_word(word));
  if(sizeof(documents)) return 0;

  //  sort(documents->ranking_type, documents);

  return documents;
}

array lookup_words_or(array(string) words) {
  string sql="SELECT * FROM occurence WHERE ";
  words=map(words, lambda(string in) {
		   return "word_id="+hash_word(in);
		 } );
  sql += words*" && ";

  array documents=db->query(sql);
  if(sizeof(documents)) return 0;

  //  sort(documents->ranking_type, documents);

  return documents;
}

array lookup_words_and(array(string) words) {

  array first_result=({});
  array rest_results=({});
  first_result = lookup_word(words[0]);
  foreach(words[1..], string word) {
    rest_results += ({ lookup_word(word) });
    if(rest_results[-1]==({ 0 }))
      return 0;
    rest_results[-1]=(multiset)rest_results[-1]->uri;
  }
  array results=({});
  foreach(first_result, mapping hit)
    if(!has_value(rest_results[hit->uri], 0))
      results += ({ hit });
  return results;
}
