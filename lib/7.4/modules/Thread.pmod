#pike 7.5

inherit Thread;

#if constant(thread_create)

//! The Mutex destruct behaviour in Pike 7.4 and below is different
//! from 7.6 in that destructing a mutex destroys the key that is
//! locking it.
optional constant Mutex=__builtin.mutex_compat_7_4;

#endif /* constant(thread_create) */
