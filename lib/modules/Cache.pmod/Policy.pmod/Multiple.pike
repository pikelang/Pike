//! A multiple-policies expiration policy manager.
//!
//! @thanks
//!   Thanks to Francesco Chemolli <kinkie@roxen.com> for the contribution.

#pike __REAL_VERSION__

inherit Cache.Policy.Base;
private array(Cache.Policy.Base) my_policies;

//! This expire function calls the expire functions
//! in all of the sub-policies in turn.
void expire (Cache.Storage.Base storage) {
  foreach(my_policies, object policy) {
    policy->expire(storage);
  }
}

//!
void create(Cache.Policy.Base ... policies) {
  my_policies=policies;
}
