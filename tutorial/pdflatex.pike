#include "types.h"
#if __VERSION__ >= 0.6
import ".";
#endif /* __VERSION__ >= 0.6 */

// Requires:
//  ghostscript 5.10 or later
//  teTeX 1.0.*
//  transfig 3.2.1

inherit "latex";

string extention=".pdftex";

string packages=
#"\\usepackage{isolatin1}
\\usepackage{latexsym}  % For $\Box$
\\usepackage{amsmath}
\\usepackage{longtable}
\\usepackage[pdftex]{graphicx}
\\usepackage[pdftex]{color}
\\usepackage{colortbl}
";

string convert_gfx(TAG tag)
{
  string file;
  float dpi;
  [file,dpi]=Gfx.convert( tag->params,
			  "png|jpg|pdf",
			  300.0,
			  tag->data && Sgml.get_text(tag->data));
  
  if(!file) return "\\{Large Error, no file}\n";

  switch( (file/".")[-1] )
  {
//    case "tex": return "\\input{"+file+"}";
    case "tpi":
    case "tex":
    case "pdf":
      return "\\includegraphics{"+file+"}";

    default:
      return "\\{Huge error, wrong extention}";

    case "png":
    case "jpg":
//      return "\\epsfbox{"+file+"}";
      
      return sprintf("\\pdfimageresolution=%d\n\\includegraphics{%s}",
		     (int)dpi,
		     file);
  }
}

Sgml.Tag illustration(object o,void|mapping options)
{
  return Sgml.Tag("image",(["src":Gfx.mkpng(o,options)]),0);
}

Sgml.Tag illustration_jpeg(object o,void|mapping options)
{
  return Sgml.Tag("image",(["src":Gfx.mkjpg(o,options)]),0);
}

string package(string x)
{
  return #"
\\pdfoutput=1
\\relax
\\documentclass[twoside,a4paper]{book}
"+packages+
#"\\begin{document}
\\author{wmml2pdflatex}
\\setlength{\\unitlength}{1mm}

{\\catcode`\\^^20=\\active\\catcode`\\^^0d=\\active%
\\global\\def\\startcode{\\catcode`\\^^20=\\active\\def^^20{\\hbox{\\ }}%
\\catcode`\\^^0d=\\active\\def^^0d{\\hskip0pt\\par\\noindent}%
\\parskip=1pt\\tt}}
"+
    x+
    "\\end{document}\n";
}
