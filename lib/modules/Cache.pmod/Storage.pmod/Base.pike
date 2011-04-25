/*
 * Storage Manager prototype.
 * by Francesco Chemolli <kinkie@roxen.com>
 *
 * $Id$
 *
 * All storage managers must provide these methods.
 */

//! Base class for cache storage managers.
//!
//! All @[Cache.Storage] managers must provide these methods.

#pike __REAL_VERSION__

#define T() error( "Override this\n" );

//! These two functions are an iterator over the cache. There is an
//! internal cursor, which is reset by each call to @[first()]. Subsequent
//! calls to @[next()] will iterate over all the contents of the cache.
//!
//! These functions are not meant to be exported to the user, but are
//! solely for the policy managers' benefit.
int(0..0)|string first() {
  T();
}
int(0..0)|string next() {
  T();
}

//! Data-object creation is performed here if necessary, or in @[get()]
//! depending on the backend.
//!
//! This allows the storage managers to have their own
//! data class implementation.
void set(string key, mixed value,
         void|int max_life, void|float preciousness, 
         void|multiset(string) dependants) {
  T();
}

//! Fetch some data from the cache synchronously.
//! @note
//!   Be careful, as with some storage managers it might block the calling
//!   thread for some time.
int(0..0)|Cache.Data get(string key, void|int(0..1) notouch) {
  T();
}

//! Fetch some data from the cache asynchronously.
//!
//! @[callback()] will get as first argument @[key], and as second
//! argument @tt{0@} (cache miss) or an @[Cache.Data] object, plus any
//! additional argument that the user may have supplied.
void aget(string key,
          function(string,int(0..0)|Cache.Data,mixed ...:void) callback,
          mixed ... extra_callback_args) {
  T();
}

//! Delete the entry specified by @[key] from the cache (if present).
//!
//! If @[hard] is @tt{1@}, some backends may force a @[destruct()]
//! on the deleted value.
//!
//! Dependants (if present) are automatically deleted.
//!
//! @returns
//!   Returns the deleted entry.
mixed delete(string key, void|int(0..1) hard) {
  T();
}
