/*
 * $Id: join.pike,v 1.7 2002/05/23 22:09:36 manual Exp $
 *
 * AutoDoc mk II join script.
 *
 * Usage: pike join.pike --post-process destination.xml files_to_join.xml [...]
 * Usage: pike join.pike destination.xml builddir
 */

mapping sub_cache = ([]);

int main(int n, array(string) args) {

  int post_process = has_value(args, "--post-process");
  args -= ({ "--post-process" });
  if(post_process) {
    int mtime;
    mtime = min( @map(args[2..], lambda(string f) { return file_stat(f)->mtime; } ) );
    Stdio.Stat stat = file_stat(args[1]);
    if(stat && stat->mtime > mtime) return 0;
    return join_files(args[2..], args[1], post_process);
  }

  if(n!=3) {
    write("%s <destination.xml> <builddir>\n", args[0]);
    return 1;
  }

  string builddir = args[2];
  if(builddir[-1]!='/') builddir += "/";
  recurse( builddir, args[1] );
}

void recurse(string builddir, string save_to) {
  array files = ({});
  int mtime;

  // Adding all /*/sub_manual.xml files to the file queue.
  foreach(get_dir(builddir), string fn) {
    if(fn[0]=='.' || (fn[0]=='#' && fn[-1]=='#')) continue;
    Stdio.Stat stat = file_stat(builddir+fn);
    if(!stat->isdir) continue;
    recurse(builddir+fn+"/", builddir+fn+"/sub_manual.xml");

    stat = file_stat(builddir+fn+"/sub_manual.xml");
    if(stat) {
      files += ({ builddir+fn+"/sub_manual.xml" });
      mtime = max(mtime, stat->mtime);
    }
  }

  // Adding all *.xml files to the file queue, except sub_manual.xml.
  werror("Joining in %s\n", builddir);
  foreach(filter(get_dir(builddir), has_suffix, ".xml"), string fn) {
    if(fn=="sub_manual.xml") continue;
    if(fn[0]=='.' || (fn[0]=='#' && fn[-1]=='#')) continue;
    Stdio.Stat stat = file_stat(builddir+fn);
    if(stat->isdir || stat->size < 3) continue;
    files += ({ builddir+fn });
    mtime = max(mtime, stat->mtime);
  }

  Stdio.Stat dstat = file_stat(save_to);
  if(dstat && dstat->mtime > mtime) return;
  int res = join_files(files, save_to, 0);
  if(res) exit(res);
}

object load_tree(string fn) {
  if(sub_cache[fn]) return m_delete(sub_cache, fn);
  return Parser.XML.Tree.parse_file(fn)[0];
}

int(0..1) join_files(array(string) files, string save_to, int(0..1) post_process) {

  if(!sizeof(files)) {
    werror("No content to merge.\n");
    return 0;
  }

  if(sizeof(files)==1) {
   werror("Only one content file present. Copy instead of merge.\n");
    return(!Stdio.cp(files[0], save_to));
  }

  werror("Joining %d file%s...\n", sizeof(files),
	 (sizeof(files)==1?"":"s"));

  werror("Reading %s...\n", files[0]);
  object dest = load_tree(files[0]);

  int fail;

  foreach(files[1..], string filename)
  {    
    object src;
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
    werror("Post processing manual file.\n");
    Tools.AutoDoc.ProcessXML.postProcess(dest);
  }

  if (!fail) {
    werror("\rWriting %s...\n", save_to);
    Stdio.write_file(save_to, dest->html_of_node());
    sub_cache[save_to] = dest;
  }
  return fail;
}
