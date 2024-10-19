#pike __REAL_VERSION__

constant description = "Makes a high granularity Pike feature list.";

void item(string name, int(0..1) check) {
  write(" [%s] %s\n", (check?"X":" "), name);
}

void test_ipv6()
{
  write("\nIPv6\n");

  Stdio.Port port = Stdio.Port();
  int check;
  mixed err = catch {
      // The following throws if the address isn't supported.
      check = port->bind(0, 0, "::");
    };
  item("IPv6 addresses",
       !err || !has_prefix(describe_error(err), "Invalid address"));

  // The following fails on Linux machines which haven't
  // been configured for IPv6 (EAFNOSUPPORT).
  item("IPv6 binding", check);

  if (check) {
    // The following fails on Solaris machines which haven't
    // been configured for IPv6 (ENETUNREACH).
    if( port->query_address() )
    {
      int portno = array_sscanf(port->query_address(), "%*s %d")[0];
      check = Stdio.File()->connect("::1", portno);
    }
  }
  item("IPv6 connecting", check);
}

void f(string sym, void|string name) {
  int x = has_index(all_constants(), sym) ||
    !undefinedp(master()->resolv(sym));
  item(name||sym, x);
}

void m(string sym) {
  f(sym, (sym/".")[..String.count(sym,".")-1]*".");
}

#define F(X) f(#X)
#define M(X) m(#X)
#define I(X) item(#X, !!(X))

int main(int num, array(string) args) {

  if( has_value(args, "--help") )
    exit(0, "This tools lists different invarient points of Pike\n"
         "and how this installation is configured.\n");

  write("Compilation options\n");
  f("Debug.reset_dmalloc", "DEBUG_MALLOC");
  f("Debug.HAVE_DEBUG", "PIKE_DEBUG");
  f("get_profiling_info", "PROFILING");
  f("load_module", "USE_DYNAMIC_MODULES");

  write("\nRuntime options\n");
  mapping(string:string|int) rt = Pike.get_runtime_info();
  item(sprintf("%d-bit ABI", rt->abi), 1);
  if (!(<"default">)[rt->bytecode_method]) {
    item(sprintf("Machine code: %s", rt->bytecode_method), 1);
  } else {
    item("Machine code", 0);
  }

  test_ipv6();

  write("\nBuiltin functions\n");
  F(_leak); // PIKE_DEBUG marker
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
  F(thread_set_concurrency); // UNIX_THREADS
  F(ualarm);
  F(uname);
  F(utime);

  write("\nBz2\n");
  M(Bz2.Deflate);

  write("\nCOM\n");
  M(COM.com);

  write("\nCrypto\n");
  F(Crypto.AES.CMAC);
  F(Crypto.AES.EAX);
  F(Crypto.AES.GCM);
  F(Crypto.Blowfish);
  F(Crypto.Camellia);
  F(Crypto.ChaCha20);
  F(Crypto.ECC.Curve);
  F(Crypto.ECC.Curve25519);
  F(Crypto.ECC.SECP_192R1);
  F(Crypto.ECC.SECP_224R1);
  F(Crypto.ECC.SECP_256R1);
  F(Crypto.ECC.SECP_384R1);
  F(Crypto.ECC.SECP_521R1);
  F(Crypto.GOST94);
  F(Crypto.IDEA);
  F(Crypto.RIPEMD160);
  F(Crypto.SALSA20);
  F(Crypto.SALSA20R12);
  F(Crypto.Serpent);
  F(Crypto.SHA224);
  F(Crypto.SHA384);
  F(Crypto.SHA512);
  F(Crypto.SHA3_512);

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
  M(DVB.dvb);

  write("\nFFmpeg\n");
  M(_Ffmpeg.list_codecs);

  write("\nFuse\n");
  M(Fuse.Operations);

  write("\nGdbm\n");
  M(Gdbm.gdbm);

  write("\nGettext\n");
  M(Gettext.gettext);
  F(Gettext.LC_MESSAGES);

  write("\nGL\n");
  M(GL.glGet);
  M(GL.GLSL.glCreateShader);

  write("\nGLUT\n");
  M(GLUT.glutGet);

  write("\nGmp\n");
  M(Gmp.mpz);
  F(Gmp.mpf);
  f("Gmp.MPF_IS_IEEE", "Gmp.mpf [IEEE]");
  F(Gmp.mpq);
  F(Gmp.mpz);


#if 0
  write("\nGnome\n");
  // FIXME: Gnome.pmod looks buggy.
  // require gnome
  // require docklets
  // require HAVE_SAVE_SESSION_SIGNAL
  // require HAVE_PANEL_PIZEL_SIZE
  // require HAVE_PANEL_DRAW_SIGNAL
  // require HAVE_APPLET_QUEUE_RESIZE
#endif

  write("\nGnome2\n");
  F(Gnome2.Canvas);
  F(Gnome2.Client);

  write("\nGSSAPI\n");
  M(GSSAPI.Context);

  write("\nGTK2\n");
  M(GTK2.gtk_init);
  F(GTK2.Databox);
  F(GTK2.GladeXML);
  F(GTK2.HandleBox);

  write("\nGz\n");
  M(Gz.crc32);

  write("\nImage\n");
  M(Image.FreeType.Face);
  M(Image.GIF.decode);
  M(Image.JPEG.decode);
  f("Image.JPEG.FLIP_H", "JPEG transforms");
  M(Image.PNG.decode);
  M(Image.SVG.decode);
  M(Image.TIFF.decode);
  f("Image.TTF->`()","Image.TTF"); // FIXME: Does this work? Fix _Image.Fonts
  M(Image.WebP.decode);
  M(Image.XFace.decode);

  write("\nJava\n");
  M(Java.jvm);
  f("Java.NATIVE_METHODS", "Java native methods");

  write("\nKerberos\n");
  M(Kerberos.Context);

  write("\nMath\n");
  M(Math.Transforms.FFT);
  F(Math.LMatrix);

  write("\nMIME\n");
  M(MIME.Message);

  write("\nMsql\n");
  M(Msql.version);
#if 0 // if Msql version is >2
  item("Msql.msql->affected_rows",1);
  item("Msql.msql->list_index",1);
#endif

  write("\nMysql\n");
  M(Mysql.mysql);
  object mysql_obj = master()->resolv("Mysql");
  // Classic:	"MySQL (Copyright Abandoned)/3.23.49"
  // Mysql GPL:	"MySQL Community Server (GPL)/5.5.30"
  // MariaDB:	"MySQL (Copyright Abandoned)/5.5.0"
  string license = "Unknown";
  string version = "Unknown";
  string client_info = mysql_obj[?"client_info"] && mysql_obj["client_info"]();
  if (client_info) {
    sscanf(client_info, "%*s(%s)%*s/%s", license, version);
  }
  item("Version: " + version, !!client_info);
  item("License: " + license, !!client_info);
  mysql_obj = master()->resolv("Mysql.mysql");
  int mysql_db_fun = mysql_obj && mysql_obj->MYSQL_NO_ADD_DROP_DB;
  item("Mysql.mysql->create_db", mysql_db_fun);
  item("Mysql.mysql->drop_db", mysql_db_fun);
  item("Mysql SSL support", mysql_obj && mysql_obj->CLIENT_SSL);

  write("\nNettle\n");
  M(Nettle.Yarrow);
  F(Nettle.bcrypt_hash);
  // F(Nettle.IDEA_Info); // Expose as Crypto.IDEA

  write("\nOdbc\n");
  M(Odbc.odbc);

  write("\nOracle\n");
  M(Oracle.NULL);

  // PDF

  // Perl

  // Pipe

  write("\nProcess\n");
  f("kill","Process.Process->kill");
  F(Process.TraceProcess); // HAVE_PTRACE

  write("\nProtocols\n");
  M(Protocols.DNS_SD.Service);

  write("\nPostgres\n");
  M(Postgres.postgres);
  M(_PGsql.PGsql);

  write("\nRandom\n");
  F(Random.Hardware);

  write("\nRegexp\n");
  f("_Regexp_PCRE._pcre", "Regexp.PCRE");
  f("_Regexp_PCRE.Widestring", "PCRE wide string support");

  write("\nSANE\n");
  M(SANE.list_scanners);

  write("\nSDL\n");
  M(SDL.init);
  F(SDL.Joystick);
  F(SDL.Music);
  F(SDL.open_audio);	/* Aka SDL_mixer */

  write("\nSQLite\n");
  M(SQLite.library_version);

  write("\nStandards\n");
  M(Standards.JSON.encode);

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
  I(Stdio.File()->peek);
  I(Stdio.File()->sync);
  I(Stdio.File()->connect_unix);
  I(Stdio.File()->proxy);
  I(Stdio.File()->lock);
  I(Stdio.File()->trylock);
  I(Stdio.File()->openat);
  I(Stdio.File()->statat);
  I(Stdio.File()->unlinkat);
  I(Stdio.File()->openpt);
  I(Stdio.File()->grantpt);
  item("Stdio.File()->tcgetattr/tcsetattr/tcsendbrak/tcflush",
       !!Stdio.File()->tcgetattr);
  I(Stdio.File()->set_keepalive);
  I(Stdio.File()->notify);
  item("Stdio.File()->listxattr/setxattr/getxattr/removexattr",
       !!Stdio.File()->listxattr);

  write("\nsybase\n");
  M(sybase.sybase);

  write("\nSystem\n");
  M(System.FSEvents.EventStream);
  M(System.Inotify._Instance);
  M(System.Wnotify.EventPoller);
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
  if(!undefinedp(System.ITIMER_##X)) itimer_types += ({ #X })
  ITIMER(REAL);
  ITIMER(VIRTUAL);
  ITIMER(PROF);
  write( itimer_types*", " + "\n" );
#endif
  F(System.setpwent);
  F(System.setrlimit);
  F(System.syslog);
  F(System.usleep);
  F(System.FSEvents.EventStream);
  F(System.Inotify);
  // System.CPU_TIME_IS_THREAD_LOCAL
  // System.Memory.shmat
  // System.Memory.PAGE_SIZE
  // System.Memory.PAGE_SHIFT
  // System.Memory.PAGE_MASK
  // System.Memory.__MMAP__

  write("\nVCDiff\n");
  F(VCDiff.Decoder);
  F(VCDiff.Encoder);

  write("\nWeb\n");
  M(Web.Sass.Compiler);

  write("\nYp\n");
  M(Yp.default_domain);

  write("\nZXID\n");
  M(ZXID.Configuration);

  return 0;
}
