/*
 * An access-time-based expiration policy manager.
 * by Francesco Chemolli <kinkie@roxen.com>
 *
 * $Id: Timed.pike,v 1.8 2005/01/04 12:29:52 grubba Exp $
 */

//! An access-time-based expiration policy manager.

#pike __REAL_VERSION__

#if defined(CACHE_DEBUG)||defined(CACHE_POLICY_DEBUG)||defined(CACHE_POLICY_TIMED_DEBUG)
#define CACHE_WERR(X...) werror("Cache.Policy.Timed: "+X)
#else
#define CACHE_WERR(X...)
#endif /* CACHE_DEBUG || CACHE_POLICY_DEBUG || CACHE_POLICY_TIMED_DEBUG */

//TODO: use the preciousness somehow.
// idea: expire if (now-atime)*cost < ktime

#define DEFAULT_KTIME 300
private int ktime;

inherit Cache.Policy.Base;

void expire(Cache.Storage.Base storage) {
  CACHE_WERR("Expiring cache\n");
  int now=time(1);
  int limit=now-ktime;
  string key=storage->first();
  while (key) {
    Cache.Data got=storage->get(key,1);
    if (!objectp(got)) {
      //if got==0, there must have been some
      //dependents acting. This is kludgy... :-(
      key=storage->next();
      continue;
    }
    if (got->atime < limit ||
        (got->etime && got->etime < now) ) {
      CACHE_WERR("deleting %s (age: %d, now: %d, etime: %d)\n",
		 key, now - got->atime, 
		 now, got->etime);
      storage->delete(key);
    }
    key=storage->next();
  }
}

void create(void|int instance_ktime) {
  ktime=(instance_ktime?instance_ktime:DEFAULT_KTIME);
}
