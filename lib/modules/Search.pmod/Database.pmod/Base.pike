//! @class Base class for Search Database implementations

//! Inserts the words of a resource into the database
void insert_words(Standards.URI, array words,
		  void|Linguistics.Language language);

//! Inserts the metadata corresponding to a resource into the database
void insert_metadata(Standards.URI uri, mapping(string:Search.MetadataType),
		     void|Linguistics.Language language);

//! Removes a document from the database.
void remove_words(Standards.URI uri, void|Linguistics.Language language);

//! Removes a document from the database.
void remove_metadata(Standards.URI uri, void|Linguistics.Language language);

//! Look up the language forks present within @variable uri
multiset(Linguistics.Language) lookup_languages(Standards.URI uri);

//! Return a mapping with all the documents containing @variable word.
//! The index part of the mapping contains document URIs.
array(mapping) lookup_word(string word);

//! Return a mapping with all the documents containing one or more of @variable words. 
//! The index part of the mapping contains document URIs.
array(mapping) lookup_words_or(array(string) words);

//! Return a mapping with all the documents containing all of  @variable words
//! The index part of the mapping contains document URIs.
array(mapping) lookup_words_and(array(string) words);

//! Optimize the stored data.
void optimize();
