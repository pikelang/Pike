/*
 * A multiple-policies expiration policy manager.
 * by Francesco Chemolli <kinkie@roxen.com>
 * (C) 2000 Roxen IS
 *
 * $Id: Multiple.pike,v 1.1 2000/07/02 20:14:47 kinkie Exp $
 */

inherit Cache.Policy.Base;
private array(Cache.Policy.Base) my_policies;

void expire (Cache.Storage storage) {
  foreach(my_policies, object policy) {
    policy->expire(storage);
  }
}

void create(Cache.Policy.Base ... policies) {
  my_policies=policies;
}
