/*
 * An LRU, size-constrained expiration policy manager.
 * by Francesco Chemolli <kinkie@roxen.com>
 *
 * $Id$
 */

#pike __REAL_VERSION__

inherit Cache.Policy.Base;
//watermarks
int max_size=0; //in bytes
int min_size=0;

//used for the "candidate-for-removal" array
#define KEY 0
#define SIZE 1

void expire (Cache.Storage storage) {
  ADT.Priority_queue removables=ADT.Priority_queue();
  Cache.Data got;
  mixed tmp;
  int now=time(1);
  int current_size=0; //in bytes. Should I use kb maybe?

  werror("expiring cache\n");
  string key=storage->first();
  while (key) {
    got=storage->get(key,1);
    werror("examining: %s (age: %d, size: %d). Current size is %d\n",
           key,now-(got->atime), got->size(), current_size);
    if (tmp=(got->etime) && tmp < now) { //explicit expiration
      werror("expired\n");
      storage->delete(key);
      key=storage->next();
      continue;
    }
    current_size+=got->size();
    removables->push( got->atime, ({ key, got->size() })  );
    if (current_size > max_size) {
      array candidate;
      while (current_size > min_size) {
        candidate=removables->pop();
        werror("deleting %s (size: %d)\n",candidate[KEY],candidate[SIZE]);
        storage->delete(candidate[KEY]);
        current_size-=candidate[SIZE];
      }
    }
    key=storage->next();
  }
}

void create (int max, void|int min) {
  max_size=max;
  if (min)
    min_size=min;
  else if (zero_type(min)) //not specified
    min_size=max_size/2;
}
