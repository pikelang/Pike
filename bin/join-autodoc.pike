/*
 * $Id: join-autodoc.pike,v 1.7 2001/07/26 17:43:32 nilsson Exp $
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
  int files = sizeof( argv )-2;
  string save_to = argv[1];
  werror("Joining %d file%s...\n", files, (files==1?"":"s"));

  werror("Reading %s...\n", argv[2]);
  //trace(5);
  object dest = Parser.XML.Tree.parse_file(argv[2])[0];

  int fail;

  foreach(argv[3..], string filename)
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
