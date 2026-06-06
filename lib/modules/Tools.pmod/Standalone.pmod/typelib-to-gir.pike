
#pike 9.0

constant description =
  "Converts gobject-introspection .typelib files into .gir XML files.";

// Uses the experimental Parser.Typelib module.
#pragma no_experimental_warnings

import Parser.Typelib;

void usage()
{
  Stdio.stdout->write(#"\
Usage:
  pike -c typelib-to-gir [--help] namespace[-version[.typelib]]*
");
}

string find_typelib_file(string fn)
{
  array(string) paths = ({ dirname(fn) });
  string fn_prefix = basename(fn);

  if (paths[0] == "") {
    paths = ({ "." });
#if constant(GI.get_search_path)
    paths += GI.get_search_path();
#endif
  }

  // This prefix order makes "DBus" match "DBus.typelib"
  // before "DBus-1.0.typelib" before "DBusGLib-1.0.typelib".
  foreach(({ fn_prefix + ".typelib", fn_prefix + "-", fn_prefix }), string p) {
    foreach(paths, string path) {
      array(string) d = reverse(sort(get_dir(path) || ({})));
      d = filter(d, has_prefix, p);
      d = filter(d, has_suffix, ".typelib");
      d = map(d, lambda(string f, string path) {
        return combine_path(path, f);
      }, path);
      d = filter(d, Stdio.is_file);
      if (sizeof(d)) return d[0];
    }
  }

  werror("Typelib file %q not found.\n", fn);
  exit(2);
}

int main(int arg, array(string) argv)
{
  foreach(Getopt.find_all_options(argv, ({
    ({ "help", Getopt.NO_ARG, "--help" }),
  })), array(string|int) opt) {
    switch(opt[0]) {
    case "help":
      usage();
      return 0;
    }
  }

  argv = Getopt.get_args(argv)[1..];

  if (!sizeof(argv)) {
    usage();
    return 1;
  }

  foreach(argv, string fn) {
    string file_path = find_typelib_file(fn);

    String.Buffer out_buf = String.Buffer();
    mixed err = catch {
        string data = Stdio.read_bytes(file_path);
        Stdio.FakeFile f = Stdio.FakeFile(data);
        Header header = Header(f);
        if (header->magic != G_IR_MAGIC) {
          werror("File %q is not a typelib file (bad magic).\n", file_path);
          exit(3);
        }

        out_buf->add("<?xml version='1.0'>\n");

        header->render_xml(out_buf);

        Stdio.stdout.write("%s", string_to_utf8(out_buf->get()));
      };
    if (err) {
      werror("%s: Error: Unsupported typelib format: %s",
             file_path, describe_backtrace/*error*/(err));
    }
  }
}
