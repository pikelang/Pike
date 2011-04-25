/*
 * Storage Manager prototype.
 * by Francesco Chemolli <kinkie@roxen.com>
 *
 * $Id$
 *
 * All storage managers must provide these methods.
 */

#pike __REAL_VERSION__

#define T() error( "Override this\n" );

// The next two functions are an iterator over the cache. There is an
// internal cursor, which is reset by each call to first(). Subsequent
// calls to next() will iterate over all the contents of the cache.
// These functions are not meant to be exported to the user, but are
// solely for the policy managers' benefit.
int(0..0)|string first() {
  T();
}

int(0..0)|string next() {
  T();
}

// Guess what these do?
// Data-object creation is performed here if necessary, or in get() depending
// on the backend. This allows the storage managers to have their own
// Data class implementation.
void set(string key, mixed value,
         void|int max_life, void|float preciousness, 
         void|multiset(string) dependants) {
  T();
}

// fetches some data from the cache synchronously.
// be careful, as with some storage managers it might block the calling
// thread for some time.
int(0..0)|Cache.Data get(string key) {
  T();
}

// fetches some data from the cache asynchronously.
// the callback will get as first argument the key, and as second
// argument 0 (cache miss) or an Cache.Data object, plus any
// additional argument that the user may have supplied.
void aget(string key,
          function(string,int(0..0)|Cache.Data,mixed ...:void) callback,
          mixed ... extra_callback_args) {
  T();
}

// deletes some entry from the cache.
// returns the deleted entry.
// if hard==1, some backends may force a destruct() on the deleted value
// dependants (if present) are automatically deleted.
void delete(string key, void|int(0..1) hard) {
  T();
}
