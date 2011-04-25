/*
 * Null policy-manager for the generic Caching system
 * by Francesco Chemolli <kinkie@roxen.com>
 *
 * $Id$
 *
 * This is a policy manager that doesn't actually expire anything.
 * It is useful in multilevel and/or network-based caches.
 */

#pike __REAL_VERSION__

void expire (Cache.Storage.Base storage) {
  /* empty */
}
