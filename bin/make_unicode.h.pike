#! /usr/bin/env pike
#charset utf-8

/*
 * Creates the file pike_unicode.h
 *
 * Henrik Grubbström 2025-10-23
 */

#pragma strict_types

int toint(string val)
{
  sscanf(val, "%d%s", int ret, string suf);
  if (suf && sizeof(suf)) {
    werror("Parse error for value %q.\n", val);
    exit(1);
  }
  return ret;
}

int main(int argc, array(string(8bit)) argv)
{
  if (argc < 2 || argv[1]=="--help" ) {
    werror("Creates the pike_unicode.h file by reading the unicode readme\n"
           "from stdin and outputs it to a file.\n"
           "\n"
           "Usage: make_unicode.h.pike output_file.h\n");
    exit(1);
  }

  string data = Stdio.stdin.read();

  array(string) fields = allocate(3);
  foreach(data/"\n", string line) {
    foreach(({ "Version", " Revision", " Date" }); int i; string prefix) {
      if (has_prefix(line, prefix)) {
        line = line[sizeof(prefix)..];
        fields[i] = String.trim_all_whites(line);
        break;
      }
    }
  }

  if (!fields[0] && !fields[1]) {
    werror("No version information found!\n");
    exit(1);
  }
  if (!fields[2]) {
    werror("No date information found!\n");
    exit(1);
  }
  if (fields[0] && fields[1] && (fields[0] != fields[1])) {
    werror("Inconsistent version of Unicode: %q != %q!\n",
           fields[0], fields[1]);
    exit(1);
  }

  Stdio.File outfile = Stdio.File(argv[1], "wct");

  outfile->
    write(string_to_utf8(sprintf("/*\n"
                                 " * Created by make_unicode.h.pike\n"
                                 " * on %s"
                                 " *\n"
                                 " * Macros containing version information about unicode.\n"
                                 " *\n"
                                 " * Henrik Grubbström 2025-10-23\n"
                                 " */\n\n", ctime(time()))));

  string version = fields[0] || fields[1];
  string date = fields[2];

  array(int) version_segments =
    map(((version/".") + ("0.0.0"/"."))[..2], toint);
  outfile->write("#define PIKE_UNICODE_MAJOR\t%d\n", version_segments[0]);
  outfile->write("#define PIKE_UNICODE_MINOR\t%d\n", version_segments[1]);
  outfile->write("#define PIKE_UNICODE_REVISION\t%d\n", version_segments[2]);
  outfile->write("\n");

  array(int) date_segments = map(date/"-", toint);
  outfile->write("#define PIKE_UNICODE_YEAR\t%d\n", date_segments[0]);
  outfile->write("#define PIKE_UNICODE_MONTH\t%d\n", date_segments[1]);
  outfile->write("#define PIKE_UNICODE_DAY\t%d\n", date_segments[2]);
  exit(0);
}
