/*
 * Sample class for the Cache.Storage stuff.
 * by Francesco Chemolli <kinkie@roxen.com>
 *
 * All Storage-related class must MUST implement this method.
 *
 * $Id: Base.pike,v 1.5 2003/01/16 14:35:58 grubba Exp $
 */

#pike __REAL_VERSION__

void expire(Cache.Storage.Base storage) {
  throw("Override this!");
}

