/*
 * Null policy-manager for the generic Caching system
 * by Francesco Chemolli <kinkie@roxen.com>
 *
 * $Id: Null.pike,v 1.4 2002/01/15 22:31:24 nilsson Exp $
 *
 * This is a policy manager that doesn't actually expire anything.
 * It is useful in multilevel and/or network-based caches.
 */

#pike __REAL_VERSION__

void expire (Cache.Storage storage) {
  /* empty */
}
