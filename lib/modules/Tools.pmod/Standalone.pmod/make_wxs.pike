/*
 * $Id: make_wxs.pike,v 1.6 2005/01/06 21:08:26 grubba Exp $
 *
 * Make a Wix modules source XML file from an existing set of
 * directories or files.
 *
 * 2004-11-02 Henrik Grubbström
 */

int main(int argc, array(string) argv)
{
  string base_guid = Standards.UUID.make_version(-1)->str();
  string version_str = "1.0";
  string id;
  string descr;
  string manufacturer;
  string comments;
  string service;

  foreach(Getopt.find_all_options(argv, ({
    ({"--guid", Getopt.HAS_ARG, ({"-g", "--guid"})}),
    ({"--version", Getopt.MAY_HAVE_ARG, ({"-v", "--version"})}),
    ({"--id", Getopt.HAS_ARG, ({"-i", "--id", "--identifier"})}),
    ({"--description", Getopt.HAS_ARG, ({"-d", "--descr", "--description"})}),
    ({"--manufacturer", Getopt.HAS_ARG, ({"-m", "--manufacturer"})}),
    ({"--comments", Getopt.HAS_ARG, ({"-c", "--comment", "--comments"})}),
    ({"--service", Getopt.HAS_ARG, ({"-s", "--service"})}),
  })), array(string) opt) {
    switch(opt[0]) {
    case "--guid":
      base_guid = Standards.UUID.UUID(opt[1])->str();
      break;
    case "--version":
      if (stringp(opt[1])) {
	version_str = opt[1];
      } else {
	write("$Revision: 1.6 $\n");
	exit(0);
      }
      break;
    case "--id":
      id = opt[1];
      break;
    case "--description":
      descr = opt[1];
      break;
    case "--manufacturer":
      manufacturer = opt[1];
      break;
    case "--comments":
      comments = opt[1];
      break;
    case "--service":
      service = opt[1];
      break;
    }
  }

  if (!id) {
    werror("No identifier specified.\n");
    exit(1);
  }

  argv = Getopt.get_args(argv);

  string version_guid =
    Standards.UUID.make_version3(base_guid, version_str)->str();
  Standards.XML.Wix.Directory root =
    Standards.XML.Wix.Directory("SourceDir",
				Standards.UUID.UUID(version_guid)->encode(),
				"TARGETDIR");

  string last_file;

  foreach(argv[1..], string src) {
    src = replace(src, "\\", "/");
    src = replace(src, "\"", "");	// FIXME: Remove junk
    array(string) seg;
    string dest;
    if (sizeof(seg = (src/":")) > 1) {
      // The destination name may be specified with
      //   <dest>:<src>
      dest = seg[0];
      src = seg[1..]*":";
    }
    Stdio.Stat st = file_stat(src);
    if (!st) {
      werror("Failed to find %O\n", src);
      exit(1);
    } if (st->isdir) {
      root->recurse_install_directory(dest||".", src);
    } else {
      root->install_file(dest||((src/"/")[-1]), src);
      last_file = dest;
    }
  }

  root->set_sources();

  write(Standards.XML.Wix.get_module_xml(root, id, version_str,
					 manufacturer, descr, version_guid,
					 comments)->render_xml());
}