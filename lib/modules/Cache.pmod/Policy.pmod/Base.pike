/*
 * Sample class for the Cache.Storage stuff.
 * by Francesco Chemolli <kinkie@roxen.com>
 *
 * All Storage-related class must MUST implement this method.
 *
 * $Id$
 */

#pike __REAL_VERSION__

void expire(Cache.Storage storage) {
  throw("Override this!");
}

