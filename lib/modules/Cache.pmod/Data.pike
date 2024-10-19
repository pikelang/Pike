/*
 * Base storage-object for the cache system
 * by Francesco Chemolli <kinkie@roxen.com>
 */

//! Base stored object for the cache system.

#pike __REAL_VERSION__

int atime=0; //! last-access time.
int ctime=0; //! creation-time
int etime=0; //! expiry-time (if supplied). 0 otherwise
float cost=1.0; //! relative preciousness scale

void touch() { //used by the Storage Manager mainly.
  atime=time(1);
}

//! expire_time is relative and in seconds.
protected void create(void|mixed value, void|int expire_time,
		      void|float preciousness) {
  atime=ctime=time(1);
}

int size() {} //! A method in order to allow for lazy computation.
              //! Used by some Policy Managers

mixed data() {} //! A method in order to allow for lazy computation


#define DEFAULT_SIZE 2048

//! Attempts a wild guess of an object's size.
//! It's left here as a common utility. Some classes won't even need it.
int recursive_low_size(mixed whatfor) {
  if (stringp(whatfor)) return sizeof(whatfor);
  if (intp(whatfor)) return SIZEOF_INT; //NOTE: not true for bignums...
  if (floatp(whatfor)) return SIZEOF_FLOAT;
  if (programp(whatfor) || objectp(whatfor) ||
      functionp(whatfor)) return DEFAULT_SIZE;
  // only composite types ahead
  array(mixed) iter;
  int size=sizeof(whatfor);
  if (arrayp(whatfor)) iter=whatfor;
  if (mappingp(whatfor)) iter=indices(whatfor)+values(whatfor);
  if (multisetp(whatfor)) iter=indices(whatfor);
  foreach(iter,mixed tmp) {
    size+=recursive_low_size(tmp);
  }
  return size;
}
