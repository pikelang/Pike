/*
 * $Id: split-autodoc.pike,v 1.2 2001/04/18 14:03:52 jhs Exp $
 *
 * AutoDoc mk II split script.
 *
 * Johan Sundström 2001-04-17
 */

import Tools.AutoDoc;

int main(int argc, array(string) argv)
{
  werror("Reading refdoc blob %s...\n", argv[1]);
  string doc = Stdio.read_file(argv[1]);
  werror("Reading structure file %s...\n", argv[2]);
  string struct = Stdio.read_file(argv[2]);
  werror("Splitting to destination directory %s...\n", argv[3]);

  mixed error;
  if(error = catch(ProcessXML.splitIntoDirHier(doc, struct, argv[3])))
  {
    if(_typeof( error ) <= AutoDocError)
      werror("%O\n", error);
    else
      throw( error );
    exit( 1 );
  }
}
