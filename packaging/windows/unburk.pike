
string read(Stdio.File f) {
  int size;
  sscanf(f->read(4), "%4c", size);
  return f->read(size);
}

int main(int c, array(string) args) {

  if(c<2) exit(1, "No burk file argument.\n");

  Stdio.File f = Stdio.File(args[1]);

  string cmd;
  while(1) {
    cmd = f->read(1);
    if(!sizeof(cmd)) break;
    switch(cmd) {

    case "w":
    case "e":
    case "s":
    case "D":
      read(f);
      break;

    case "d":
      mkdir(read(f));
      break;

    case "f":
      string fn = read(f);
      string file = read(f);

      if( has_value(fn, "piketmp/bin") || has_value(fn, "piketmp/lib") ||
	  has_value(fn, "piketmp/src") || has_suffix(fn, ".pmod") ||
	  has_suffix(fn, ".h") ||
	  (< "dynamic_module_makefile", "ANNOUNCE", "COPYING",
	     "COPYRIGHT", "master.pike", "pike.syms", "specs" >)[fn] )
	file = replace(file, "\n", "\r\n");

      Stdio.write_file(fn, file);
      break;

    case "q":
      return 0;

    default:
      exit(1, "Unknown symbol '%s'\n", cmd);
    }
  }
}
