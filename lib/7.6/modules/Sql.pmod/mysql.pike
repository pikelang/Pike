/*
 * $Id: mysql.pike,v 1.2 2004/09/29 13:36:08 per Exp $
 *
 * Glue for the old broken Mysql-module which fetched all rows at once always
 */

#pike 7.7

inherit Sql.mysql;

#if constant(thread_create)
Thread.Mutex gmutex = Thread.Mutex();
#define LOCK() object lock = gmutex->lock()
#define UNLOCK() lock=0;
#else
#define LOCK()
#define UNLOCK()
#endif

int|object big_query(mixed ... args) {
  LOCK();
  object pre_res = ::big_query(@args);
  program p = master()->get_compilation_handler(7,6)->resolv("Mysql.mysql_result");
  mixed res = pre_res && p(pre_res);
  UNLOCK(); // Avoid any possible tail-recursion optimizations by not returning directly above.

  return res;
}

array query( mixed ... args ) 
{
    LOCK();
    mixed res= ::query(@args);
    UNLOCK(); // Avoid any possible tail-recursion optimizations by not returning directly above.

    return res;
}
