/*
 * $Id: join_autodoc.pike,v 1.16 2004/07/08 15:33:56 grubba Exp $
 *
 * AutoDoc mk II join script.
 *
 * Usage: pike -x join_autodoc destination.xml builddir
 * Usage: pike -x join_autodoc --post-process dest.xml files_to_join.xml [...]
 */

#pike __REAL_VERSION__

constant description = "Joins AutoDoc extractions.";
mapping sub_cache = ([]);

int verbosity = 2;

static constant Node = Parser.XML.Tree.SimpleNode;

int main(int n, array(string) args) {

  int post_process = has_value(args, "--post-process");
  args -= ({ "--post-process" });

  if (has_value(args, "-q")) {
    // quiet.
    verbosity = 0;
    args -= ({ "-q" });
  } else if (has_value(args, "--quiet")) {
    // quiet.
    verbosity = 0;
    args -= ({ "--quiet" });
  } 

  if(sizeof(args)<3) {
    write("pike -x %s <destination.xml> <builddir>\n", args[0]);
    write("pike -x %s --post-process <dest.xml> files_to_join.xml [...]\n",
	  args[0]);
    return 1;
  }

  recurse( args[2..], args[1], post_process );
}

void recurse(array(string) sources, string save_to, int post_process) {
  array files = ({});
  int mtime;

  foreach(sources, string builddir) {
    Stdio.Stat stat = file_stat(builddir);
    if(stat->isdir) {

      if(builddir[-1]!='/') builddir += "/";

      // Adding all /*/.cache.xml files to the file queue.
      foreach(get_dir(builddir), string fn) {
	if(fn[0]=='.' || (fn[0]=='#' && fn[-1]=='#')) continue;
	Stdio.Stat stat = file_stat(builddir+fn);
	if(!stat->isdir) continue;
	recurse(({builddir+fn+"/"}), builddir+fn+"/.cache.xml", 0);
	
	stat = file_stat(builddir+fn+"/.cache.xml");
	if(stat) {
	  files += ({ builddir+fn+"/.cache.xml" });
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
	mtime = max(mtime, stat->mtime);
      }
    } else {
      files += ({ builddir });
      mtime = max(mtime, stat->mtime);
    }
  }
  Stdio.Stat dstat = file_stat(save_to);
  if(dstat && dstat->mtime > mtime) return;
  int res = join_files(files, save_to, post_process);
  if(res) exit(res);
}

Node load_tree(string fn) {
  if(sub_cache[fn]) return m_delete(sub_cache, fn);
  return Parser.XML.Tree.simple_parse_file(fn)[0];
}

int(0..1) join_files(array(string) files, string save_to, int(0..1) post_process) {

  if(!sizeof(files)) {
    if (verbosity > 1)
      werror("No content to merge.\n");
    return 0;
  }

  if(sizeof(files)==1) {
    if (verbosity > 1)
      werror("Only one content file present. Copy instead of merge.\n");
    return !Stdio.cp(files[0], save_to);
  }

  if (verbosity > 0)
    werror("Joining %d file%s...\n", sizeof(files),
	   (sizeof(files)==1?"":"s"));

  if (verbosity > 1)
    werror("Reading %s...\n", files[0]);
  Node dest = load_tree(files[0]);

  int fail;

  foreach(files[1..], string filename)
  {    
    Node src;
    if (mixed err = catch {
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
  }

  if(post_process) {
    if (verbosity > 0)
      werror("Post processing manual file.\n");
    Tools.AutoDoc.ProcessXML.postProcess(dest);
  }

  if (!fail) {
    if (verbosity > 0)
      werror("\rWriting %s...\n", save_to);
    Stdio.write_file(save_to, dest->html_of_node());
    sub_cache[save_to] = dest;
  }
  return fail;
}
