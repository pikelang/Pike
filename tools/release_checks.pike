

// Ok, so this test is silly
int test_constants() {
  int consts = Process.system("bin/pike -e "
			      "\"return sizeof(all_constants())-200\"")+200;
  int allocated;
  sscanf(Stdio.read_file("src/constants.c"), "%*sallocate_mapping(%d)",
			 allocated);
  if(consts>allocated) {
    write("bultin_constants mapping in constants.c needs to be at least %d\n"
	  "entries big.\n", allocated);
    return 0;
  }
  if(allocated-consts>10)
    write("builtin_constants mapping in constants.c should perhaps be \n"
	  "reduced. Allocated: %d, Used: %d\n", allocated, consts);
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
  foreach(Filesystem.Traversion("lib/modules"); string path; string file)
    if(has_suffix(file, ".pike") || has_suffix(file, ".pmod"))
      if(!has_value(Stdio.read_file(path+file),"#pike __REAL_VERSION__")) {
	write("%s%s is missing #pike __REAL_VERSION__.\n", path,file);
	status = 0;
      }
  foreach(Filesystem.Traversion("src"); string path; string file)
    if(file=="module.pmod.in" &&
       !has_value(Stdio.read_file(path+file),"#pike __REAL_VERSION__")) {
      write("%s%s is missing #pike __REAL_VERSION__.\n", path,file);
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

  test_constants();
  test_copyright();
  test_copying();
  test_master_year();
  test_charset_table("tables.c");
  test_charset_table("misc.c");
  test_unicode();
  test_realpike();
}
