int get_uri_id(string uri);
int get_document_id(string uri, void|string language_code);
int get_field_id(string field, void|int do_not_create);
mapping get_uri_and_language(int doc_id);

void insert_words(Standards.URI|string uri,
		  void|string language,
		  string field,
		  array(string) words,
		  void|int link_hash);

void set_metadata(Standards.URI|string uri,
		  void|string language,
		  mapping(string:string) md);

mapping(string:string) get_metadata(Standards.URI|string uri,
				    void|string language,
				    void|array(string) wanted_fields);

void remove_document(string|Standards.URI uri, string language);

int sync();
string get_blob(int word_id, int num);
int hash_word(string word);
