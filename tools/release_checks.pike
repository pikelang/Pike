

// Ok, so this test is stupid
int test_constants() {
  int consts = sizeof(all_constants());
  int allocated;
  sscanf(Stdio.read_file("src/constants.c"), "%*sallocate_mapping(%d)",
			 allocated);
  // Aim for 10% overallocation to allow for adding of a few extra constants
  // without penalty.
  if(allocated < consts*105/100) {
    // Overallocating by less than 5%.
    write("Consider increasing the size of the builtin_constants mapping "
	  "to %d entries (currently %d).\n", consts*110/100, allocated);
  } else if (allocated > consts*115/100) {
    // Overallocating by more than 15% seems excessive.
    write("Consider decreasing the size of the builtin_constants mapping "
	  "to %d entries (currently %d).\n", consts*110/100, allocated);
  }
  return 1;
}

int test_copyright() {
  if(Stdio.read_file("COPYRIGHT")!=Tools.Legal.Copyright.get_text()) {
    write("COPYRIGHT needs to be updated.\n");
    return 0;
  }
  return 1;
}

int test_copying() {
  if(Stdio.read_file("COPYING")!=Tools.Legal.License.get_text()) {
    write("COPYRIGHT needs to be updated.\n");
    return 0;
  }
  return 1;
}

int test_master_year() {
  int y;
  sscanf(Stdio.read_file("lib/master.pike.in"),
	 "%*s 1994-%d Linköping", y);
  if(gmtime(time())->year+1900 != y) {
    write("The year in the copyright message in master.pike.in\n"
	  "needs an update.\n");
    return 0;
  }
  return 1;
}

int test_install_year() {
  int y;
  sscanf(Stdio.read_file("bin/install.pike"),
	 "%*s 1994-%d IDA", y);
  if(gmtime(time())->year+1900 != y) {
    write("The year in the copyright message in install.pike\n"
	  "needs an update.\n");
    return 0;
  }
  return 1;
}

int test_charset_table(string t) {
  array names = ({});
  foreach( Stdio.read_file("src/modules/_Charset/"+t)/"\n", string line )
    if( sscanf(line, "  { \"%s\", ", string name) )
      names += ({ name });

  string code = Stdio.read_file("src/modules/_Charset/module.pmod.in");
  sscanf(code, "%*sstring normalize(%s return out;\n}", code);
  code = "string normalize("+code+" return out;\n}\n";
  function normalize = compile_string(code)()->normalize;

  int status = 1;

  foreach(names, string name)
    if(name!=normalize(name)) {
      write("%O is nor correctly normalized into %O in %s.\n",
	    name, normalize(name), t);
      status = 0;
    }

  if( !equal(names, sort(copy_value(names))) ) {
    write("Incorrect sorting order in %s.\n", t);
    status = 0;
  }

  return status;
}

int test_unicode() {
  string readme = Protocols.HTTP.
    get_url_data("http://ftp.unicode.org/Public/UNIDATA/ReadMe.txt");
  int a,b,c;
  sscanf(readme, "Version %d.%d.%d", a,b,c);
  int x,y,z;
  sscanf(Stdio.read_file("src/UnicodeData-ReadMe.txt"),
	 "Version %d.%d.%d", x,y,z);
  if( a!=x || b!=y || c!=z ) {
    write("Unicode database out of sync with unicode.org.\n");
    return 0;
  }
  return 1;
}

int test_realpike() {
  int status = 1;

  // bin and tools shouldn't really be #pike-ified, since they
  // should run with the pike it is bundled with.
  foreach( ({ "lib", /* "bin", "tools" */ }), string dir)
    foreach(Filesystem.Traversion(dir); string path; string file)
      if(has_suffix(file, ".pike") || has_suffix(file, ".pmod"))
	if(!has_value(Stdio.read_file(path+file),"#pike")) {
	  write("%s%s is missing a #pike directive.\n", path,file);
	  status = 0;
	}
  foreach(Filesystem.Traversion("src"); string path; string file)
    if(file=="module.pmod.in" &&
       !has_value(Stdio.read_file(path+file),"#pike")) {
      write("%s%s is missing a #pike directive.\n", path,file);
      status = 0;
    }
  return status;
}

array(int) read_version() {
  array r = array_sscanf(Stdio.read_file("src/version.h"),
			 "%*sPIKE_MAJOR_VERSION %d%*s"
			 "PIKE_MINOR_VERSION %d%*s"
			 "PIKE_BUILD_VERSION %d");
  if(!r || sizeof(r)!=3)
    error("Couldn't parse version.h\n");
  return r;
}

void assert_version() {
  array v = read_version();
  if( v[0]!=__REAL_MAJOR__ || v[1]!=__REAL_MINOR__ ||
      v[2]!=__REAL_BUILD__ ) {
    write("You must be running the Pike you want to test.\n");
    exit(1);
  }
}

int test_version() {
  int status=1;
  array v = read_version();
  array t = array_sscanf(Stdio.read_file("ANNOUNCE"),
			 "%*sPIKE %d.%d ANNOUNCEMENT");

  if(t[0]!=v[0] || t[1]!=v[1]) {
    write("Wrong Pike version in ANNOUNCE.\n");
    status = 0;
  }

  t = array_sscanf(Stdio.read_file("man/pike.1"),
		   "%*s.nr mj %d%*s.nr mn %d");
  if(t[0]!=v[0] || t[1]!=v[1]) {
    write("Wrong Pike version in man/pike.1.\n");
    status = 0;
  }
  return status;
}

void main(int args) {
  if(args>1) {
    write("This program checks various aspects of the Pike tree\n"
	  "before a release.\n");
    exit(0);
  }
  cd(combine_path(__FILE__,"../.."));

  assert_version();

  test_constants();
  test_copyright();
  test_copying();
  test_master_year();
  test_install_year();
  test_charset_table("tables.c");
  test_charset_table("misc.c");
  test_unicode();
  test_realpike();
  test_version();
}
