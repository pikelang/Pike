//! Null policy-manager for the generic Caching system
//! 
//! This is a policy manager that doesn't actually expire anything.
//! It is useful in multilevel and/or network-based caches.
//!
//! @thanks
//!   Thanks to Francesco Chemolli <kinkie@@roxen.com> for the contribution.

#pike __REAL_VERSION__

//! This is an expire function that does nothing.
void expire (Cache.Storage.Base storage) {
  /* empty */
}
