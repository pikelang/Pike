/*
 * $Id: join-autodoc.pike,v 1.8 2001/08/11 22:09:27 nilsson Exp $
 *
 * AutoDoc mk II join script.
 *
 * Johan Sundström 2001-04-17
 *
 * Usage: pike join-autodoc.pike destination.xml files_to_join.xml [...]
 */

int main(int argc, array(string) argv)
{
  int post_process = has_value(argv, "--post-process");
  argv -= ({ "--post-process" });

  string save_to = argv[1];
  array(string) files = argv[2..];

  files = filter(files, lambda(string fn) {
			  Stdio.Stat st = file_stat(fn);
			  return st->size != 0 && st->size != 37;
			});

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

  werror("Reading %s...\n", argv[2]);
  object dest = Parser.XML.Tree.parse_file(files[0])[0];

  int fail;

  foreach(files[1..], string filename)
  {    
    object src;
    if (mixed err = catch {
      src = Parser.XML.Tree.parse_file( filename )[0];
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
  }
  return fail;
}
