/*
 * Sample class for the Cache.Storage stuff.
 * by Francesco Chemolli <kinkie@roxen.com>
 * (C) 2000 Roxen IS
 *
 * All Storage-related class must MUST implement this method.
 *
 * $Id: Base.pike,v 1.2 2000/09/26 18:59:10 hubbe Exp $
 */

#pike __VERSION__

void expire(Cache.Storage storage) {
  throw("Override this!");
}

