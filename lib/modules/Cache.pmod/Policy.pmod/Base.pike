/*
 * Sample class for the Cache.Storage stuff.
 * by Francesco Chemolli <kinkie@roxen.com>
 *
 * All Storage-related class must MUST implement this method.
 *
 * $Id: Base.pike,v 1.4 2002/01/15 22:31:24 nilsson Exp $
 */

#pike __REAL_VERSION__

void expire(Cache.Storage storage) {
  throw("Override this!");
}

