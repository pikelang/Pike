//! The MIME content types this class can filter.
constant contenttypes = ({ });

//! 
void set_content(string);
array(array(string)) get_anchors();
void add_content(string, int);
array(array) get_filtered_content();
string get_title();
string get_keywords();
string get_description();
