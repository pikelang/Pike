/*
 * Storage Manager prototype.
 * by Francesco Chemolli <kinkie@roxen.com>
 * (C) 2000 Roxen IS
 *
 * $Id: Base.pike,v 1.1 2000/07/02 20:15:24 kinkie Exp $
 *
 * All storage managers must provide these methods.
 */

#define T() throw( ({"override this", backtrace()}))

int(0..0)|string first() {
  T();
}

int(0..0)|string next() {
  T();
}

/*
 * Guess what these do?
 * I leave the data-object creation here, so that the storage manager
 * can choose whatever data-class it pleases
 */
Cache.Data|int(0..0) set(string key, mixed value,
                         void|int max_life, void|float preciousness) {
  T();
}

//fetches some data from the cache synchronously.
//be careful, as with some storage managers it might block the calling
//thread for some time.
int(0..0)|Cache.Data get(string key) {
  T();
}

//fetches some data from the cache asynchronously.
//the callback will get as first argument the key, and as second
//argument 0 (cache miss) or an Cache.Data object.
void aget(string key,
          function(string,int(0..0)|Cache.Data:void) callback) {
  T();
}

Cache.Data|int(0..0) delete(string key, void|int(0..1) hard) {
  T();
}
