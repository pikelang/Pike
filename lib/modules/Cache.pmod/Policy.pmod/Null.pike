/*
 * Null policy-manager for the generic Caching system
 * by Francesco Chemolli <kinkie@roxen.com>
 *
 * $Id: Null.pike,v 1.5 2003/01/16 14:35:58 grubba Exp $
 *
 * This is a policy manager that doesn't actually expire anything.
 * It is useful in multilevel and/or network-based caches.
 */

#pike __REAL_VERSION__

void expire (Cache.Storage.Base storage) {
  /* empty */
}
