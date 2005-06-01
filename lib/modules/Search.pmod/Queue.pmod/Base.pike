inherit Web.Crawler.Queue;

void add_uri( Standards.URI uri, int recurse, string template, void|int force );

void set_md5( Standards.URI uri, string md5 );
void set_recurse( Standards.URI uri, int recurse );
mapping get_extra( Standards.URI uri );
  
int|Standards.URI get();
array(Standards.URI) get_uris(void|int stage);
void put(string|array(string)|Standards.URI|array(Standards.URI) uri);
void clear();
void clear_stage( int ... stages );
void clear_md5( int ... stages );
int num_with_stage( int ... stage );
void set_stage( Standards.URI uri, int stage );
void remove_uri(string|Standards.URI uri);
void clear_cache();
