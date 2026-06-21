#pike 9.0

#pragma no_experimental_warnings

import Parser.Typelib;

int main(int arg, array(string) argv)
{
#if constant(GI.get_search_path)
  string out_dir = ".";
  if (arg > 1) {
    out_dir = argv[1];
  }
  if (!Stdio.is_dir(out_dir)) mkdir(out_dir);

  foreach(sort(GI.get_search_path()), string path) {
    foreach(sort(get_dir(path) || ({})), string fn) {
      if (!has_suffix(fn, ".typelib")) continue;

      string file_path = combine_path(path, fn);
      Stdio.Stat file_st = file_stat(file_path);
      if (!file_st || !file_st->isreg) continue;
      string out_path =
        combine_path(out_dir, fn[..<sizeof(".typelib")] + ".autodoc.xml");
      string csum_st = "";
      Stdio.Stat out_st = file_stat(out_path);
      if (out_st && out_st->isreg) {
        // There's a file since earlier. Check the stamp.
        out_st = file_stat(out_path + ".stamp");
        if (out_st && (out_st->mtime > file_st->mtime)) continue;
        csum_st = Stdio.read_bytes(out_path + ".stamp");
      }

      String.Buffer out_buf = String.Buffer();
      mixed err = catch {
          string data = Stdio.read_bytes(file_path);
          Stdio.FakeFile f = Stdio.FakeFile(data);
          Header header = Header(f);
          if (header->magic != G_IR_MAGIC) {
            werror("File %q is not a typelib file (bad magic).\n", file_path);
            continue;
          }

          out_buf->add("<?xml version='1.0'?>\n");

          header->render_autodoc(out_buf);

          string out_data = string_to_utf8(out_buf->get());
          string csum_data = String.string2hex(Crypto.SHA1.hash(out_data));
          if (csum_data != csum_st) {
            Stdio.write_file(out_path, out_data);
          }

          // Always bump the stamp.
          Stdio.write_file(out_path + ".stamp", csum_data);
        };
      if (err) {
        werror("%s: Error: Unsupported typelib format: %s",
               file_path, describe_backtrace/*error*/(err));
      }
    }
  }
  // sort(namespaces);
  //werror("Namespaces: %O\n", namespaces);
#else
  werror("GI module not available.\n");
#endif
  return 0;
}
