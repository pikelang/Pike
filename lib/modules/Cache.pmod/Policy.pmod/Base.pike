/*
 * Sample class for the Cache.Storage stuff.
 * by Francesco Chemolli <kinkie@roxen.com>
 * (C) 2000 Roxen IS
 *
 * All Storage-related class must MUST implement this method.
 *
 * $Id: Base.pike,v 1.1 2000/07/02 20:14:39 kinkie Exp $
 */

void expire(Cache.Storage storage) {
  throw("Override this!");
}

