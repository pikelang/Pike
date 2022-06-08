/*
 * AutoDoc mk II join script.
 *
 * Usage: pike -x join_autodoc destination.xml builddir
 * Usage: pike -x join_autodoc --post-process dest.xml files_to_join.xml [...]
 */

#pike __REAL_VERSION__

constant description = "Joins AutoDoc extractions.";
mapping sub_cache = ([]);

int verbosity = Tools.AutoDoc.FLAG_NORMAL;

#define TOSTR(X)	#X
#define DEFINETOSTR(X)	TOSTR(X)

protected constant Node = Parser.XML.Tree.SimpleNode;
protected constant SimpleHeaderNode = Parser.XML.Tree.SimpleHeaderNode;
protected constant SimpleRootNode = Parser.XML.Tree.SimpleRootNode;
protected constant SimpleTextNode = Parser.XML.Tree.SimpleTextNode;

class Options
{
  inherit Arg.Options;

  constant help_pre =
    "pike -x join_autodoc <destination.xml> <builddir>\n"
    "pike -x join_autodoc";
  constant help_post = "\t<destination.xml> <files_to_join.xml> [...]";

  Opt help = NoOpt("-h")|NoOpt("--help");
  Opt quiet = NoOpt("-q")|NoOpt("--quiet");
  constant quiet_help = "Quiet mode.";
  Opt verbose = NoOpt("-v")|NoOpt("--verbose");
  constant verbose_help = "Verbose mode.";
  Opt post_process = NoOpt("--post-process");
  constant post_process_help = "Perform post processing (resolve references, etc).";
  Opt pikever = HasOpt("--pike-version")|
    Default(DEFINETOSTR(__MAJOR__) "." DEFINETOSTR(__MINOR__));
  constant pikever_help = "Set the pike version.";
}

int main(int n, array(string) args)
{
  Options options = Options(args);

  int post_process = options->post_process;
  Tools.AutoDoc.Flags flags = Tools.AutoDoc.FLAG_NORMAL;

  if (options->quiet) {
    // quiet.
    verbosity = 0;
  } else if (options->verbose) {
    verbosity++;
  }

  flags = (flags & ~Tools.AutoDoc.FLAG_VERB_MASK)|verbosity;

  if (options->help) {
    return 1;
  }

  args = options[Arg.REST];

  recurse( args[1..], args[0], post_process, flags );
  foreach(values(sub_cache), Node n) {
    n->zap_tree();
  }
}

void recurse(array(string) sources, string save_to,
	     int(0..1) post_process, Tools.AutoDoc.Flags flags)
{
  array files = ({});
  int mtime;

  Stdio.Stat dstat = file_stat(save_to) && file_stat(save_to + ".stamp");
  foreach(sources, string builddir) {
    Stdio.Stat stat = file_stat(builddir);
    if (!stat) {
      werror("File %O not found\n", builddir);
      exit(1);
    }
    if(stat->isdir) {

      if(builddir[-1]!='/') builddir += "/";

      // Adding all /*/.cache.xml files to the file queue.
      foreach(get_dir(builddir), string fn) {
	if(fn[0]=='.' || (fn[0]=='#' && fn[-1]=='#')) continue;
	Stdio.Stat stat = file_stat(builddir+fn);
	if(!stat->isdir) continue;
	recurse(({builddir+fn+"/"}), builddir+fn+"/.cache.xml", 0, flags);

	stat = file_stat(builddir+fn+"/.cache.xml");
	if(stat) {
	  files += ({ builddir+fn+"/.cache.xml" });

#if 0
	  if (dstat && stat->mtime > dstat->mtime) {
	    werror("Rebuilding %O due to %O.\n",
		   save_to, builddir + fn + "/.cache.xml");
	  }
#endif

	  mtime = max(mtime, stat->mtime);
	}
      }

      // Adding all *.xml files to the file queue
      if (verbosity > 0)
	werror("Joining in %s\n", builddir);
      foreach(filter(get_dir(builddir), has_suffix, ".xml"), string fn) {
	if(fn[0]=='.' || (fn[0]=='#' && fn[-1]=='#')) continue;
	Stdio.Stat stat = file_stat(builddir+fn);
	if(stat->isdir || stat->size < 3) continue;
	files += ({ builddir+fn });
#if 0
	if (dstat && stat->mtime > dstat->mtime) {
	  werror("Rebuilding %O due to %O.\n",
		 save_to, builddir + fn);
	}
#endif
	mtime = max(mtime, stat->mtime);
      }
    } else {
      files += ({ builddir });
      mtime = max(mtime, stat->mtime);
    }
  }
  if(dstat && dstat->mtime > mtime) return;
  int res = join_files(files, save_to, post_process, flags);
  if(res) exit(res);
}

Node load_tree(string fn) {
  if(sub_cache[fn]) return m_delete(sub_cache, fn);
  return Parser.XML.Tree.simple_parse_file(fn)->get_first_element();
}

int(0..1) join_files(array(string) files, string save_to,
		     int(0..1) post_process, Tools.AutoDoc.Flags flags)
{
  string|zero post_process_log = UNDEFINED;
  if (post_process) {
    post_process_log = combine_path(save_to, "../resolution.log");
  }
  string data = low_join_files(files, save_to, post_process_log, flags);

  if (!data) return 1;

  // Don't touch the result file unless the content has changed.
  string orig_data = Stdio.read_bytes(save_to);
  if (data != orig_data) {
    if (verbosity > 0)
      werror("\rWriting %s...\n", save_to);
    Stdio.write_file(save_to, data);
  }

  // Touch the stamp file.
  Stdio.write_file(save_to + ".stamp", "");

  return 0;
}

string low_join_files(array(string) files, string save_to,
		      string post_process_log, Tools.AutoDoc.Flags flags)
{
  if(!sizeof(files)) {
    if (verbosity > 1)
      werror("No content to merge.\n");
    return "<?xml version='1.0' encoding='utf-8'?>\n"
      "<autodoc/>\n";
  }

  if(sizeof(files)==1) {
    if (verbosity > 1)
      werror("Only one content file present. Copy instead of merge.\n");
    Node n = m_delete(sub_cache, files[0]);
    if (n) sub_cache[save_to] = n;
    return Stdio.read_bytes(files[0]);
  }

  if (verbosity > 0)
    werror("Joining %d file%s...\n", sizeof(files),
	   (sizeof(files)==1?"":"s"));

  // Attempt to keep the result in a canonic order.
  sort(files);

  Node dest = Parser.XML.Tree.SimpleElementNode("autodoc", ([]));

  int fail;

  foreach(files, string filename)
  {
    Node src;
    if (mixed err = catch {
	if (verbosity > 1)
	  werror("Reading %s...\n", filename);
	src = load_tree( filename );
      }) {
      if (arrayp(err)) {
	throw(err);
      }
      if (stringp(err)) {
	werror("%s: %s", filename, err);
      } else if (err->position) {
	werror("%s %O: %s\n", err->part, err->position, err->message);
      } else {
	werror("%s: %s\n", err->part, err->message);
      }
      fail = 1;
      continue;
    }
    if (!src) {
      werror("\rFailed to read %O\n", filename);
      continue;
    }
    if (verbosity > 1)
      werror("\rMerging with %s...\n", filename);
    if (mixed err = catch {
      Tools.AutoDoc.ProcessXML.mergeTrees(dest, src);
    }) {
      if (arrayp(err)) {
	throw(err);
      }
      if (err->position) {
	werror("%s %O: %s\n", err->part, err->position, err->message);
      } else {
	werror("%s: %s\n", err->part, err->message);
      }
      fail = 1;
    }
    src->zap_tree();
  }

  if(post_process_log) {
    if (verbosity > 0)
      werror("\rPost processing manual file.\n");
    Tools.AutoDoc.ProcessXML.postProcess(dest, post_process_log, flags);
  }

  if (!fail) {
    if (verbosity > 0)
      werror("\rRendering XML...\n");
    SimpleRootNode root = SimpleRootNode();
    root->replace_children(({ SimpleHeaderNode(([ "version":"1.0",
						  "encoding":"utf-8" ])),
			      SimpleTextNode("\n"),
			      dest,
			      SimpleTextNode("\n"),
			   }));
    string res = root->render_xml();
    sub_cache[save_to] = dest;
    return res;
  }
  dest->zap_tree();
  return UNDEFINED;
}
