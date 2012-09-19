#pike __REAL_VERSION__

#if constant (___Mysql)

inherit ___Mysql;

#else  // !___Mysql
constant this_program_does_not_exist = 1;
#endif
