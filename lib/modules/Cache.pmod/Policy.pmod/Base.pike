/*
 * Sample class for the Cache.Storage stuff.
 * by Francesco Chemolli <kinkie@roxen.com>
 * (C) 2000 Roxen IS
 *
 * All Storage-related class must MUST implement this method.
 *
 * $Id: Base.pike,v 1.3 2000/09/28 03:38:29 hubbe Exp $
 */

#pike __REAL_VERSION__

void expire(Cache.Storage storage) {
  throw("Override this!");
}

