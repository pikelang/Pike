#pike 7.5

inherit Thread;

#if constant(thread_create)
optional constant Mutex=__builtin.mutex_compat_7_4;
#endif /* constant(thread_create) */
