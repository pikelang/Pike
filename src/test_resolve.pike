// Test resolution of all modules.
//
// Extracted from testsuite.in 1.711

int ok=1;

int num_ok;
int num_failed;

void test_resolv(string file, int base_size, object|void handler)
{
  string prg = replace( file[base_size+1..sizeof(file)-6],
			([ "/":".", ".pmod":""]) );
  if(prg[sizeof(prg)-7..]==".module")
    prg = prg[..sizeof(prg)-8];
  // werror("Resolving %O...\n", prg);
  mixed err;
  if(err = catch( (handler||master())->resolv(prg) ) ) {
    werror("test: failed to peek at %O: %s\n",
	   prg, describe_error(err));
    num_failed++;
    ok=0;
  } else {
    num_ok++;
  }
}

void test_dir(string dir, int|void base_size, object|void handler)
{
  if (!Stdio.is_dir (dir)) return;
  // werror("Testing directory %O...\n", dir);
  if(!base_size) base_size=sizeof(dir);
  array(string) files = get_dir(dir);
  // Ensure that .so files are loaded before .pike and .pmod files.
  // Otherwise their loading errors will be hidden.
  sort(reverse(files[*]), files);
  files = reverse(files);
  foreach(files, string s)
  {
    switch(s)
    {
#if !constant(GTK.Window)
      case "GTKSupport.pmod":
      case "PV.pike":
      case "pv.pike":
#endif
        continue; // These modules cannot be tested properly by this test
    }
    string file=combine_path(dir,s);
    mixed stat=file_stat(file);
    if(!stat) continue;
    if(stat[1]==-2 && has_suffix(file, ".pmod"))
    {
      test_resolv(file, base_size, handler);
      test_dir(file, base_size, handler);
    }
    else if(stat[1]>=0){
      // DEBUG
      if (lower_case(uname()->sysname||"") == "osf1") {
        werror("Testing file %O...\n", file);
      }
      if(has_suffix(file, ".pike") || has_suffix(file, ".pmod") ||
	 has_suffix(file, ".so"))
      {
#if 0
        mixed err=catch { (program)file; };
	if (err) 
        {
	  werror("test: failed to compile %O\n",file);
          ok=0;
	  num_failed++;
          continue;
        }
#endif
	if (has_suffix(file, ".so")) {
	  if (!master()->programs[file]) {
	    // Clear any negative cache entry.
	    m_delete(master()->programs, file);
	  }
#if constant(load_module)
	  // Load .so files by hand to make sure
	  // loading errors aren't hidden.
	  mixed err;
	  if (err = catch{
	    program ret = load_module(file);
	    master()->programs[file] = ret;
	  }) {
	    werror("test: failed to load %O: %s\n",
		   file, describe_error(err));
	    num_failed++;
	    ok=0;
	  } else
#endif /* constant(load_module) */
	  {
	    // "XX" pads suffix to 4 characters.
	    test_resolv(file+"XX", base_size, handler);
	  }
	} else {
	  test_resolv(file, base_size, handler);
	}
      }
    }
  }
}

int main()
{
  Array.map(master()->pike_module_path,test_dir);
  // FIXME: Forward compatibility?
  foreach(({"0.6","7.0","7.2","7.4"}),string ver) {
    object handler = master()->get_compilation_handler(@(array(int))(ver/"."));
    Array.map(handler->pike_module_path,test_dir,0,handler);
  }

  // Make a report.
  write("Loaded ok: %d\n", num_ok);
  if (num_failed) {
    write("Failed: %d\n", num_failed);
  }

  return !ok;
}
