#pike __REAL_VERSION__

constant description = "Makes a high granularity Pike feature list.";

void f(string sym, void|string name) {
  int x = !zero_type(all_constants()[sym]) ||
    !zero_type(master()->resolv(sym));
  write(" [%s] %s\n", (x?"X":" "), name||sym);
}

#define F(X) f(#X)

#define N() write("\n");

void builtin() {

  write("Compilation options\n");
  f("Debug.reset_dmalloc", "DEBUG_MALLOC");
  f("__leak", "PIKE_DEBUG");
  f("get_profiling_info", "PROFILING");
  f("load_module", "USE_DYNAMIC_MODULES");

  write("\nBuiltin functions\n");
  F(__leak); // PIKE_DEBUG marker
  F(alarm);
  F(chown);
  F(chroot);
  F(cleargroups);
  F(closelog);
  F(exece);
  F(filesystem_stat);
  F(fork);
  F(get_all_users);
  F(get_all_groups);
  F(get_groups_for_user);
  F(get_profiling_info); // PROFILING marker
  F(getegid);
  F(geteuid);
  F(getgid);
  F(getgrgid);
  F(getgrnam);
  F(getgroups);
  F(gethostbyaddr);
  F(gethostbyname);
  F(gethostname);
  F(getpgrp);
  F(getppid);
  F(getpwnam);
  F(getpwuid);
  F(getsid);
  F(getuid);
  F(gmtime);
  F(hardlink);
  F(initgroups);
  F(innetgr);
  F(kill);
  F(load_module); // USE_DYNAMIC_MODULES marker
  F(localtime);
  F(mktime);
  F(openlog);
  F(readlink);
  F(resolvepath);
  F(setegid);
  F(seteuid);
  F(setgid);
  F(setgroups);
  F(setpgrp);
  F(setresgid);
  F(setresuid);
  F(setsid);
  F(setuid);
  F(symlink);
  F(syslog);
  F(thread_set_concurrency); // UNIX_THREADS
  F(ualarm);
  F(uname);
  F(utime);

  write("\nDebug\n");
  F(Debug.assembler_debug);
  F(Debug.compiler_trace);
  F(Debug.debug);
  F(Debug.describe);
  F(Debug.dmalloc_set_name);
  F(Debug.dump_backlog);
  F(Debug.gc_set_watch);
  F(Debug.list_open_fds);
  F(Debug.locate_references);
  F(Debug.optimizer_debug);
  F(Debug.reset_dmalloc); // DEBUG_MALLOC marker

  write("\nDVB\n");
  f("DVB.dvb","DVB");

  write("\nGdbm\n");
  f("Gdbm.gdbm","Gdbm");

  write("\nGettext\n");
  f("Gettext.gettext","Gettext");
  F(Gettext.LC_MESSAGES);

  write("\nGmp\n");
  f("Gmp.mpz","Gmp");

  write("\nGz\n");
  f("Gz.crc32","Gz");

  write("\nPike\n");
  F(Pike.Security.get_user);

  write("\nProcess\n");
  f("kill","Process.Process->kill");
  F(Process.TraceProcess); // HAVE_PTRACE

  write("\nStdio\n");
  F(Stdio.DN_ACCESS);
  F(Stdio.DN_CREATE);
  F(Stdio.DN_MODIFY);
  F(Stdio.DN_MULTISHOT);
  F(Stdio.DN_RENAME);
  // __HAVE_OOB__
  // __OOB__
  // __HAVE_CONNECT_UNIX__
  // __HAVE_OPEN_PT__
  F(Stdio.UDP.MSG_PEEK);
  F(Stdio.SOCK.STREAM);
  F(Stdio.SOCK.DGRAM);
  F(Stdio.SOCK.SEQPACKET);
  F(Stdio.SOCK.RAW);
  F(Stdio.SOCK.RDM);
  F(Stdio.SOCK.PACKET);
  F(Stdio.sendfile); // _REENTRANT marker

  write("\nSystem\n");
  F(System.dumpable);
  F(System.endgrent);
  F(System.endpwent);
  F(System.get_netinfo_property);
  F(System.getgrent);
  F(System.getitimer);
  F(System.getpwent);
  F(System.getrlimit);
  F(System.getrlimits);
  F(System.gettimeofday);
  F(System.nanosleep);
  F(System.rdtsc);
  F(System.sleep);
  F(System.setgrent);
  F(System.setitimer);
#if constant(System.setitimer);
  write("     System.setitimer types: ");
  array itimer_types = ({});
#define ITIMER(X) \
  if(!zero_type(System.ITIMER_##X)) itimer_types += ({ #X })
  ITIMER(REAL);
  ITIMER(VIRTUAL);
  ITIMER(PROF);
  write( itimer_types*", " + "\n" );
#endif
  F(System.setpwent);
  F(System.setrlimit);
  F(System.usleep);
  // System.CPU_TIME_IS_THREAD_LOCAL
  // System.Memory.shmat
  // System.Memory.PAGE_SIZE
  // System.Memory.PAGE_SHIFT
  // System.Memory.PAGE_MASK
  // System.Memory.__MMAP__
}



int main() {
  builtin();
  return 0;
}
