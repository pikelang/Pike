/*
 * A RAM-based storage manager.
 * by Francesco Chemolli <kinkie@roxen.com>
 *
 * $Id$
 *
 * This storage manager provides the means to save data to memory.
 * In this manager I'll add reference documentation as comments to
 * interfaces. It will be organized later in a more comprehensive format
 *
 * Settings will be added later.
 */

#pike __REAL_VERSION__

class Data {

  inherit Cache.Data;
  
  int _size=0;
  mixed _data=0;
  multiset(string) _deps;
  
  void create(void|mixed value, void|int abs_expire_time, 
              void|float preciousness,
              void|multiset(string) dependants) {
    _data=value;
    atime=ctime=time(1);
    if (abs_expire_time) etime=abs_expire_time;
    if (preciousness) cost=preciousness;
    if (dependants) _deps=dependants;
  }
  
  int size() {
    if (_size) return _size;
    return (_size=recursive_low_size(_data));
  }
  
  mixed data() {
    return _data;
  }
  
}

inherit Cache.Storage.Base;

private mapping(string:mixed) data=([]);


/*
 * First an iterator over the contents.
 * Since accesses to the data are not serialized to increase efficiency
 * there are a few guidelines that must be followed in order to maintain
 * consistency.
 *
 * First off, only one entity must be using the iterator at one time.
 * That's not as bad as it seems, as the only entity needing to enumerate
 * the entries in cache is the expiration policy manager, and there can
 * be only one for each storage manager.
 *
 * The enumerator over the cache is initialized by a call to first().
 * Subsequent calls to next() return following entries in the cache.
 * While it is guarranteed that each and all entries in the cache 
 * will be iterated upon by the enumerator, their order is NOT guarranteed
 * to remain consistent across calls.
 */

// these are used by the enumerator. While entries might be deleted while
// enumerating, it won't bite us.
private array(string) iter=0;
private int current=0;

int(0..0)|string first() {
  iter=indices(data);
  current=0;
  return next();
}

int(0..0)|string next() {
  if (iter && current < sizeof(iter) && data[iter[current]])
    return iter[current++];
  iter=0;
  return 0;
}

/*
 * Guess what these do?
 * I leave the data-object creation here, so that the storage manager
 * can choose whatever data-class it pleases
 */
void set(string key, mixed value,
         void|int absolute_expire, 
         void|float preciousness,
         void|multiset(string) dependants) {
  data[key]=Data(value,absolute_expire,preciousness,dependants);
}

// fetches some data from the cache. If notouch is set, don't touch the
// data from the cache (meant to be used by the storage manager only)
int(0..0)|Cache.Data get(string key, void|int notouch) {
  mixed tmp;
  tmp=data[key];
  if (!notouch && tmp) tmp->touch();
  return tmp;
}

void aget(string key, 
          function(string,int(0..0)|Cache.Data:void) callback) {
  mixed rv=get(key);
  callback(key,rv);
}

void delete(string key, void|int(0..1) hard) {
  object(Cache.Data) rv=data[key];
  if (!rv) return;
  multiset deps=rv->_deps;
  
  //need to destruct this first, or we might infinite-loop recursing
  //through the dependants (think of a bug listing a value depending
  //on itself, or even worse, circularly.
  if (hard) {
    destruct(rv->value());
  }
  m_delete(data,key);

  if (deps) {
    foreach((array)(deps), string dep) {
      delete(dep,hard);
    }
  }
  return 0;
}

