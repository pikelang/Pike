/*
 * Null policy-manager for the generic Caching system
 * by Francesco Chemolli <kinkie@roxen.com>
 * (C) 2000 Roxen IS
 *
 * $Id: Null.pike,v 1.1 2000/08/07 17:56:07 kinkie Exp $
 *
 * This is a policy manager that doesn't actually expire anything.
 * It is useful in multilevel and/or network-based caches.
 */

void expire (Cache.Storage storage) {
  /* empty */
}
