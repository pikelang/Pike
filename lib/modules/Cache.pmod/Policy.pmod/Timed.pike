/*
 * An access-time-based expiration policy manager.
 * by Francesco Chemolli <kinkie@roxen.com>
 * (C) 2000 Roxen IS
 *
 * $Id: Timed.pike,v 1.1 2000/07/02 20:15:10 kinkie Exp $
 */

//TODO: use the preciousness somehow.
// idea: expire if (now-atime)*cost < ktime

#define DEFAULT_KTIME 300
private int ktime;

inherit Cache.Policy.Base;

void expire(Cache.Storage storage) {
  werror("Expiring cache\n");
  int now=time(1);
  int limit=now-ktime;
  string key=storage->first();
  while (key) {
    Cache.Data got=storage->get(key,1);
    werror(sprintf("examining '%s' (age: %d, e: %d, v: %f)\n",
                   key, now-got->atime, got->etime, got->cost));
    if (got->atime < limit ||
        (got->etime && got->etime < now) ) {
      werror("deleting\n");
      storage->delete(key);
    }
    key=storage->next();
  }
}

void create(void|int instance_ktime) {
  ktime=(instance_ktime?instance_ktime:DEFAULT_KTIME);
}
