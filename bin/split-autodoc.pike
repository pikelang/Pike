/*
 * $Id: split-autodoc.pike,v 1.1 2001/04/18 13:32:31 jhs Exp $
 *
 * AutoDoc mk II split script.
 *
 * Johan Sundström 2001-04-17
 */

int main(int argc, array(string) argv)
{
  werror("Reading refdoc blob %s...\n", argv[1]);
  string doc = Stdio.read_file(argv[1]);
  werror("Reading structure file %s...\n", argv[2]);
  string struct = Stdio.read_file(argv[2]);
  werror("Splitting to destination directory %s...\n", argv[3]);
  Tools.AutoDoc.ProcessXML.splitIntoDirHier(doc, struct, argv[3]);
}
