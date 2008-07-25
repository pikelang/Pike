// Test resolution of all modules.
//
// Extracted from testsuite.in 1.711

int ok=1;

int num_ok;
int num_failed;

// Pike shouldn't produce any warnings on its own modules, so report
// that as errors.
int got_warnings_in_last_test;

class CompileErrorHandler
{
  void compile_warning (string file, int line, string err)
  {
    werror ("test: Erroneous warning: %s:%s: %s\n",
	    master()->trim_file_name (file), line ? (string) line : "-", err);
    got_warnings_in_last_test = 1;
  }

  void compile_error (string file, int line, string err)
  {
    werror ("test: Compilation error: %s:%s: %s\n",
	    master()->trim_file_name (file), line ? (string) line : "-", err);
    got_warnings_in_last_test = 1;
  }
}

void test_resolv(string file, int base_size, object|void handler)
{
#if constant(alarm)
  alarm(1*60);	// 1 minute should be sufficient for this test.
#endif
  string prg = replace( file[base_size+1..sizeof(file)-6],
			([ "/":".", ".pmod":""]) );
  if(prg[sizeof(prg)-7..]==".module")
    prg = prg[..sizeof(prg)-8];
  // write("Resolving %O...\n", prg);
  mixed err;
  got_warnings_in_last_test = 0;
  if((err = catch( (handler||master())->resolv(prg) )) ||
     got_warnings_in_last_test) {
    if (err && (!objectp (err) || !err->is_compilation_error))
      werror("test: Error during compilation of %s: %s\n",
	     prg, describe_backtrace(err));
    num_failed++;
    ok=0;
  } else {
    num_ok++;
  }
}

void test_dir(string dir, int|void base_size, object|void handler)
{
  if (!Stdio.is_dir (dir)) return;
  // write("Testing directory %O...\n", dir);
#if constant(alarm)
  alarm(1*60);	// 1 minute should be sufficient for this test.
#endif
  if(!base_size) base_size=sizeof(dir);
  string prefix = "";
  if (handler) prefix = (handler->ver || "") + " ";
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
    write ("Testing %s%s: %s%*s\r",
	   prefix,
	   stat[1] == -2 ? "dir" : "file",
	   (dir / "/")[-1] + "/" + s,
	   60 - sizeof ((dir / "/")[-1]) - sizeof (s), "");
    if(stat[1]==-2 && has_suffix(file, ".pmod"))
    {
      test_resolv(file, base_size, handler);
      test_dir(file, base_size, handler);
    }
    else if(stat[1]>=0){
      if(has_suffix(file, ".pike") || has_suffix(file, ".pmod") ||
	 has_suffix(file, ".so"))
      {
#if 0
        mixed err=catch { (program)file; };
	if (err) 
        {
	  werror("\ntest: failed to compile %O\n",file);
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
	    werror("\ntest: failed to load %O: %s\n",
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
#if constant(alarm)
  alarm(1*60);	// 1 minute should be sufficient for each part of this test.
#endif

  // Use the old classic way to avoid potential different code paths
  // in the handlers.
  master()->set_inhibit_compile_errors (CompileErrorHandler());

  // Prime the compat_handler_cache.
  master()->get_compilation_handler(0, 0);

  // Note: Get at all versions (including the main version)
  //       by going via the handler cache.
  foreach(Array.uniq(values(master()->compat_handler_cache)), object handler) {
    Array.map(handler->pike_module_path, test_dir, 0,
	      (handler != master())?handler:UNDEFINED);
  }
  write ("%*s\r", 75, "");
  Tools.Testsuite.report_result (num_ok, num_failed);
#if constant(alarm)
  alarm(0);	// Disable any alarms. When running with DMALLOC
  		// the exit code may take quite a while...
#endif
  return !ok;
}
