struct cache_entry *aap_cache_lookup(char *s, int len, 
				 char *h, int hlen,
				 struct cache *c, int nl, 
				 struct cache_entry **p, 
				 int *hv);


void aap_free_cache_entry(struct cache *c, struct cache_entry *e,
		      struct cache_entry *prev, int b);

void simple_aap_free_cache_entry(struct cache *c, struct cache_entry *e);

void aap_cache_insert(struct cache_entry *ce, struct cache *c);

void aap_clean_cache();

extern struct cache *first_cache;


