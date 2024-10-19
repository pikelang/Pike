#charset iso-8859-1


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
	  "to %d entries (currently %d/%d).\n", consts*110/100, allocated,
	  consts);
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

protected int test_year(string str, string match, string err)
{
  int y;
  sscanf(str, match, y);
  if(gmtime(time())->year+1900 != y) {
    write("The year in the copyright message in %s\nneeds an update.\n", err);
    return 0;
  }
  return 1;
}

int test_master_year() {
  return test_year( Stdio.read_file("lib/master.pike.in"),
                    "%*s 1994-%d Link�ping", "master.pike.in");
}

int test_install_year() {
  return test_year( Stdio.read_file("bin/install.pike"),
                    "%*s 1994-%d IDA", "install.pike");
}

int test_legal_year() {
  return test_year( Tools.Legal.Copyright.get_latest_pike(),
                    "%*s 2002-%d,", "Tools.Legal.Copyright");
}

int test_charset_table(string t) {
  array names;
  foreach( Stdio.read_file("src/modules/_Charset/"+t)/"\n", string line ) {
    if( !names ) {
      if( has_value(line, "charset_map") )
        names = ({});
      continue;
    }
    if( sscanf(line, "  { \"%s\", ", string name) )
      names += ({ name });
  }

  constant path = "lib/modules/Charset.pmod/module.pmod";
  string code = Stdio.read_file(path);
  if(!code)
  {
    write("Can't load %O\n", path);
    return 0;
  }
  sscanf(code, "%*sstring|zero normalize(%s return out;\n}", code);
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
    foreach( sort(copy_value(names)); int i; string name)
      if( name != names[i] )
      {
        write("Problem begins at %O, should be %O\n", names[i], name);
        break;
      }
    status = 0;
  }

  return status;
}

int test_unicode() {
  string readme = Protocols.HTTP.
    get_url_data("http://ftp.unicode.org/Public/UNIDATA/ReadMe.txt");
  int a,b,c;
  sscanf(readme, "%*sVersion %d.%d.%d", a,b,c);
  int x,y,z;
  sscanf(Stdio.read_file("src/UnicodeData-ReadMe.txt"),
         "%*sVersion %d.%d.%d", x,y,z);
  if( a!=x || b!=y || c!=z ) {
    write("Unicode database (%d.%d.%d) behind unicode.org (%d.%d.%d).\n", x,y,z,a,b,c);
    return 0;
  }
  return 1;
}

int test_realpike() {
  int status = 1;

  multiset whitelist = (<
    "lib/modules/Calendar.pmod/mkexpert.pike",
  >);

  // bin and tools shouldn't really be #pike-ified, since they
  // should run with the pike it is bundled with.
  foreach( ({ "lib", /* "bin", "tools" */ }), string dir)
    foreach(Filesystem.Traversion(dir); string path; string file)
      if(has_suffix(file, ".pike") || has_suffix(file, ".pmod"))
      {
        string f = Stdio.read_file(path+file);
        if(!f) { write("Unable to read %O\n", path+file); continue; }
        if(!whitelist[path+file] && !has_value(f,"#pike")) {
	  write("%s%s is missing a #pike directive.\n", path,file);
	  status = 0;
	}
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
    write("%d %d\n%d %d\n%d %d\n", v[0], __REAL_MAJOR__,
          v[1], __REAL_MINOR__, v[2], __REAL_BUILD__);
    exit(1);
  }

  write("\nRelease checks for "+version()+"\n\n");
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

void test_fncases(void|string dir) {
  dir = dir||".";
  array d = get_dir(dir);
  multiset d2 = (<>);
  foreach(d, string fn) {
    if( d2[lower_case(fn)] )
      write("%O with different cases in %O.\n", fn, dir);
    if( Stdio.is_dir(dir+"/"+fn) )
      test_fncases(dir+"/"+fn);
    d2[lower_case(fn)]=1;
  }
}

// Rationale: The DEBUG symbol should be reserved for user code.
void test_generic_debug(string dir)
{
  foreach(get_dir(dir), string fn) {
    string p = dir + "/" + fn;
    if (Stdio.is_dir(p))
      test_generic_debug(p);
    else if (has_suffix(p, ".pike") || has_suffix(p, ".pmod")) {
      string data = Stdio.read_bytes(p);
      if (cpp("#define DEBUG\n" + data, p) !=
	  cpp("#undef DEBUG\n" + data, p))
	write("%O has a dependency on the DEBUG symbol being defined.\n", p);
    }
  }
}

void test_manifest()
{
  mapping entries = ([]);

  string section;
  array files = ({});
  string description = "";

  void record()
  {
    if( sizeof(description) )
    {
      foreach(files, string file)
      {
        if( !has_value(file, "(autogenerated)") )
          entries[section][file] = description;
      }
      files = ({});
      description = "";
    }
  }

  foreach(Stdio.File("src/MANIFEST")->line_iterator(); int no; string line)
  {
    string pad;
    sscanf(line, "%[ ]%s", pad, line);
    switch(sizeof(pad))
    {
    case 0:
      if( line=="" ) break;

      record();

      section = line;
      if( entries[section] )
        write("Section %O repeated in MANIFEST line %d.\n", section, no);
      entries[section] = ([]);
      break;
    case 2:
      string file = line;
      sscanf(file, "%s/", file);

      if( !section ) {
        write("File without section in MANIFEST line %d.\n", no);
        break;
      }
      if( entries[section][file] || has_value(files, file) )
        write("File %O repeated in MANIFEST line %d.\n", file, no);
      record();
      files += ({ file });
      break;
    case 4:
      if( !section || !file ) {
        write("Description without file/section in MANIFEST line %d.\n", no);
        break;
      }
      description += line + "\n";
      break;
    default:
      write("Wrong indentation in MANIFEST line %d.\n", no);
      break;
    }
  }
  record();

  mapping all = ([
    ".gitignore" : "ignored",
  ]);
  foreach(Stdio.File("src/.gitignore")->line_iterator();; string line)
    all[line[1..]] = "ignored";
  foreach(entries;; mapping files)
  {
    all += files;
    foreach(files; string file; string desc)
    {
      if( !Stdio.exist(combine_path("src",file)) )
        write("Reference missing file %O.\n", file);
    }
  }
  foreach(get_dir("src");; string file)
  {
    if( file[-1]=='~' ) continue;
    if( !all[file] )
    {
      write("File %O not described in MANIFEST.\n", file);
    }
  }
}

void main(int args) {
  if(args>1) {
    write("This program checks various aspects of the Pike tree\n"
	  "before a release.\n");
    exit(0);
  }
  cd(combine_path(__FILE__,"../.."));

  assert_version();
  test_manifest();

  test_constants();
  test_copyright();
  test_copying();
  test_master_year();
  test_install_year();
  test_charset_table("tables.c");
  test_charset_table("misc.c");
  test_unicode();
  // FIXME: Consider verifying that lib/modules/Calendar.pmod/tzdata
  //        is up to date.
  test_realpike();
  test_version();
  test_generic_debug("lib/modules");

  test_fncases("src");
  test_fncases("lib");
  test_fncases("bin");
  test_fncases("man");
  test_fncases("tools");
  test_fncases("packaging");
  test_fncases("refdoc");

}
