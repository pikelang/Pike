// SQL index database without fragments
// Copyright © 2000, Roxen IS.


// Creates the SQL tables we need.
void create_tables() {

  db->query(
#"CREATE TABLE document (id INT UNSIGNED PRIMARY KEY AUTO_INCREMENT,
                         uri VARCHAR(255),
                         title VARCHAR(255),
                         description TEXT,
                         last_indexed TIMESTAMP,
                         last_changed INT UNSIGNED,
                         size INT UNSIGNED,
                         mime_type SMALLINT UNSIGNED)"
			 );
  //We're missing language...
  //domain could be tokenized

  db->query(
#"CREATE TABLE word (word VARCHAR(255),
                     id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY)"
		     );

  db->query(
#"CREATE TABLE occurance (word_id INT UNSIGNED,
                          document_id INT UNSIGNED,
                          word_position MEDIUMINT UNSIGNED,
                          ranking TINYINT)"
			  );
}

// This is the database object that all queries will be made to.
object db;
int removed;
void create(string host) {
  db=Sql.sql(host);
}

// Useful information for crawlers
mapping(string:int) page_stat(string uri) {

  array res=db->query("SELECT last_changed, last_indexed, size FROM document WHERE uri='%s'", uri);
  if(!sizeof(res)) return 0;
  return (mapping(string:int))res[0];
}

// Insert or update a page in the database.
// title and description is already in words.
void insert_page(string uri, string title, string description, int last_changed,
		 int size, int mime_type, array words) {

  // Find out our document id
  int doc_id;
  if( catch( doc_id=(int)db->query(sprintf("SELECT id FROM document WHERE uri='%s'", uri))[0]->id ))
    doc_id=0;


  int new;
  if(!doc_id) {
    db->query(sprintf("INSERT INTO document (uri, title, description, last_changed, size, mime_type)"
	      " VALUES ('%s', '%s', '%s', %d, %d, %d)", uri, title, description, last_changed,
	      size, mime_type));
    doc_id = (int)db->query("SELECT id FROM document WHERE uri='%s'", uri)[0]->id;
    new=1;
  }
  else {
    // Page was already indexed.
    db->query(sprintf("UPDATE document SET title='%s', description='%s', last_changed='%d', size='%d', "
	      "mime_type='%d'", title, description, last_changed, size, mime_type));
    db->query(sprintf("DELETE FROM occurance WHERE document_id=%d", doc_id));
  }

  // Add word occurances.
  // Should a proper cache be used?
  werror("doc_id: %O\n",doc_id);

  mapping word_ids=([]);
  int word_pos;
  foreach(words, mapping word) {
    int word_id;
    string the_word=word->word[..254];
    if(!(word_id=word_ids[the_word])) {
      if(catch( word_id=(int)(db->query(sprintf("SELECT id FROM word WHERE word='%s'", the_word))[0]->id))) {
	db->query(sprintf("INSERT INTO word (word) VALUES ('%s')", the_word)); // VARCHAR(255)
	word_id=(int)(db->query(sprintf("SELECT id FROM word WHERE word='%s'", the_word))[0]->id);
      }
      word_ids[the_word]=word_id;
    }
    db->query(sprintf("INSERT INTO occurance (word_id, document_id, word_position, "
	      "ranking) VALUES (%d, %d, %d, %d)", word_id, doc_id,
	      word_pos++, word->rank));
  }

}

void remove_page(string uri) {
  int doc_id;
  mixed error=catch( doc_id=db->query("SELECT id FROM document WHERE uri='%s'", uri)[0]->id );
  if(error) return;
  db->query("REMOVE FROM document WHERE id=%d", doc_id);
  db->query("REMOVE FROM occurence WHERE document_id=%d", doc_id);
  removed++;
}

void garbage_collect() {
  removed=0;
  
  for(int word_id = db->query("SELECT MAX(id) FROM word")[0]["MAX(id)"];
      word_id; word_id--) {
    int existence = sizeof(db->query("SELECT COUNT(id) FROM occurance WHERE word_id=%d",
				     word_id));
    if(!existence) db->query("REMOVE FROM word WHERE word_id=%d", word_id);
  }

  db->query("OPTIMIZE TABLE word");
  db->query("OPTIMIZE TABLE occurance");
  db->query("OPTIMIZE TABLE document");
}

array search_word(string word) {
  array temp=db->query("SELECT id FROM word WHERE word='%s'", word[..254]);
  if(!sizeof(temp)) return 0;
  int word_id=temp[0]->id;

  array documents=db->query("SELECT * FROM occurance WHERE word_id=%d", word_id);
  if(sizeof(documents)) return 0;

  //  sort(documents->ranking_type, documents);

  return documents;
}
