/*
 * A multiple-policies expiration policy manager.
 * by Francesco Chemolli <kinkie@roxen.com>
 *
 * $Id: Multiple.pike,v 1.5 2003/01/16 14:35:58 grubba Exp $
 */

#pike __REAL_VERSION__

inherit Cache.Policy.Base;
private array(Cache.Policy.Base) my_policies;

void expire (Cache.Storage.Base storage) {
  foreach(my_policies, object policy) {
    policy->expire(storage);
  }
}

void create(Cache.Policy.Base ... policies) {
  my_policies=policies;
}
