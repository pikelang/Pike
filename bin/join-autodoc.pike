/*
 * $Id: join-autodoc.pike,v 1.1 2001/04/18 13:16:12 jhs Exp $
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
  foreach(argv[3..], string filename)
  {
    object src = Parser.XML.Tree.parse_file( filename )[0];
    werror("\rMerging with %s...\n", filename);
    Tools.AutoDoc.ProcessXML.mergeTrees(dest, src);
  }

  werror("\rWriting %s...\n", save_to);
  Stdio.write_file(save_to, dest->html_of_node());
}
