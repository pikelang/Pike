/*
 * $Id: join-autodoc.pike,v 1.2 2001/04/21 15:05:57 grubba Exp $
 *
 * AutoDoc mk II join script.
 *
 * Johan Sundström 2001-04-17
 *
 * Usage: pike join-autodoc.pike destination.xml files_to_join.xml [...]
 */

int main(int argc, array(string) argv)
{
  int files = sizeof( argv )-2;
  string save_to = argv[1];
  werror("Joining %d file%s...\n", files, (files==1?"":"s"));

  werror("Reading %s...\n", argv[2]);
  object dest = Parser.XML.Tree.parse_file(argv[2])[0];

  int fail;

  foreach(argv[3..], string filename)
  {
    object src = Parser.XML.Tree.parse_file( filename )[0];
    werror("\rMerging with %s...\n", filename);
    if (mixed err = catch {
      Tools.AutoDoc.ProcessXML.mergeTrees(dest, src);
    }) {
      if (err->position) {
	werror("%s %O: %s\n", err->part, err->position, err->message);
      } else {
	werror("%s: %s\n", err->part, err->message);
      }
      fail = 1;
    }
  }
  werror("\rWriting %s...\n", save_to);
  Stdio.write_file(save_to, dest->html_of_node());

  return fail;
}
