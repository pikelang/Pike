#include "types.h"
#if __VERSION__ >= 0.6
import ".";
#endif /* __VERSION__ >= 0.6 */
inherit Stdio.File : out;

object html=.html();

string low_latex_quote(string text)
{
  return replace( text,
		 ({"<",">",
		     "{","}",
		     "µ","&",
		     " ","\\",
		     "[","]",
		     "#","%",
		     "$","~",
		     "^","_",
		     }),
		 ({"$<$","$>$",
		     "\\{","\\}",
		     "$\\mu$","\\&",
		     "\\verb+ +","$\\backslash$",
		     "\\verb+[+","\\verb+]+",

		     "\\#","\\%",
		     "\\$","\\verb+~+",
		     "\\verb+^+","\\_",
		     }) );
}

string latex_quote(string text)
{
  return low_latex_quote( pre ? text : ((text/"\n") - ({""})) *"" );
}


float weighted_strlen(string tag)
{
  return strlen(tag)+
    `+(0.0,@rows(([
      'i':-0.3,
      'm': 0.2,
      'w': 0.2,
      '@': 0.2,
      ' ':-0.5,
      '\n':-1.0,
      ]), indices(tag)));
}

float aproximate_length(SGML data)
{
  float len=0.0;
  if(!data) return 0.0;
  foreach(data, TAG tag)
    {
      if(stringp(tag))
      {
	len+=weighted_strlen(tag);
      }else{
	len+=aproximate_length(tag->data);
      }
    }
  return len;
}

string convert_table(TAG table)
{
  SGML data=table->data;
  int rows,columns;
  int border=(int)table->params->border;
  array(float) column_data=allocate(100,1.0);

  foreach(data, TAG tag)
    {
      if(stringp(tag))
      {
	string tmp=replace(tag,({" "," ","\t","\n"}),({"","","",""}));
	if(strlen(tmp))
	{
	  werror("Warning: Skipping string %s inside table!\n",tag);
	}
      }else{
	if(tag->tag != "tr")
	{
	  werror("Tag in table is not <tr>, it is %s! (near %s)\n",tag->tag,tag->location());
	  continue;
	}
	
	rows++;
	int row_cells=0;
	foreach(tag->data, TAG cell)
	  {
	    if(stringp(cell))
	    {
	      string tmp=replace(cell,({" "," ","\t","\n"}),({"","","",""}));
	      if(strlen(tmp))
	      {
		werror("Warning: Skipping string %s inside table!\n",cell);
	      }
	    }else{
	      switch(cell->tag)
	      {
		default:
		  werror("Tag inside table row is not <td> or <th>, it is %s! (near %s)\n",cell->tag,cell->location());
		  break;

		case "anchor":
		  // FIXME!!! Anchors should be moved outside of
		  // the table!!
		  break;

		case "th":
		case "td":
		  column_data[row_cells]+=aproximate_length(cell->data);
		  row_cells++;
	      }
	    }
	  }
	if(row_cells > columns)
	  columns=row_cells;
      }
    }

  column_data=column_data[..columns-1];
  for(int e=0;e<sizeof(column_data);e++) column_data[e]+=20.0*rows;

  float total_data=`+(@column_data);

  string fmt=(border ? "|" : "") + ("l"+(border ? "|" : ""))*columns;

  string ret="\n\n\\begin{longtable}{"+ fmt +"}\n";

  if(border) ret+="\\hline \\\\\n";

  foreach(data, TAG tag)
    {
      int c=0;
      if(stringp(tag)) continue;
      if(tag->tag != "tr") continue;
      array(string) row=({});
      foreach(tag->data, TAG cell)
	{
	  if(stringp(cell)) continue;
	  if(cell->tag != "td" && cell->tag!="th") continue;
	  row+=({
	    "\\begin{minipage}{"+(  column_data[c] / total_data ) +" \\linewidth}\n"+
	    convert_to_latex(cell->data)+
	    "\\end{minipage}"
	  });
	  c++;
	}
      ret+=row*" & "+"\\\\\n";
      if(border) ret+="\\hline\n";
    }

  ret+="\\end{longtable}\n";

  return ret;
}

string convert_image_to_latex(TAG tag)
{
  mapping params=tag->params;
  if(params->xfig)
    params->src=params->xfig+".fig";

  if(params->src && (params->src/".")[-1]=="fig")
  {
    rm("___tmp.latex");
    werror("Converting fig->latex");
    Process.create_process( ({ "fig2dev", "-L","latex",params->src,"___tmp.latex" }) )->wait();
    string ret=Stdio.read_file("___tmp.latex");
    rm("___tmp.latex");
    return ret;
  }else{
    string file=Wmml.image_to_eps(tag,600.0);
    return "\\epsfbox{"+file+"}";
  }
}

int depth;
int appendixes;
int pre;

constant FLAG_TABLE=1;
constant FLAG_LIST=2;


string convert_to_latex(SGML data, void|int flags)
{
  if(!data) return "";
  string ret="";
  foreach(data, TAG tag)
    {
      if(stringp(tag))
      {
	ret+=latex_quote(tag);
      }
      else if(objectp(tag))
      {
	switch(tag->tag)
	{
	  case "smallcaps":
	    ret+="{\\sc "+convert_to_latex(tag->data)+"}";
	    break;
	  case "tt": ret+="{\\tt "+convert_to_latex(tag->data)+"}"; break;
	  case "ex_keyword":
	  case "b": ret+="{\\bf "+convert_to_latex(tag->data)+"}"; break;
	  case "ex_meta":
	  case "i": ret+="\\emph{"+convert_to_latex(tag->data)+"}"; break;
	  case "sub": ret+="$^{"+convert_to_latex(tag->data)+"}$"; break;
	  case "sup": ret+="$_{"+convert_to_latex(tag->data)+"}$"; break;
	  case "h1":
	    ret+="\\begin{huge} "+convert_to_latex(tag->data)+"\\end{huge}";
	    break;
	  case "h2":
	    ret+="\\begin{Large} "+convert_to_latex(tag->data)+"\\end{Large}";
	    break;
	  case "h3":
	    ret+=convert_to_latex(tag->data);
	    break;
	  case "h4":
	    ret+="\\begin{small} "+convert_to_latex(tag->data)+"\\end{small}";
	    break;
	  case "h5":
	  case "h6":
	    ret+="\\begin{tiny} "+convert_to_latex(tag->data)+"\\end{tiny}";
	    break;

	  case "wbr":
	    ret+="\-";
	    break;

	  case "ex_identifier":
	  case "ex_comment":
	    ret+=convert_to_latex(tag->data);
	    break;

	  case "man_title":
	    // FIXME encaps?
	    // FIXME indentation
	    ret+="\n\n"+latex_quote(tag->params->title)+"\\\\\n "+
	      convert_to_latex(tag->data)+
	      "\n";
	    break;

	  case "chapter":
	    depth++;
	    ret+="\\cleardoublepage \\section{"+
	      latex_quote(tag->params->title)+"}\n"+
	      convert_to_latex(tag->data);
	    depth--;
	    break;

	  case "appendix":
	    if(!appendixes)
	    {
	      ret+="\n\\appendix\n";
	      appendixes=1;
	    }
	    depth++;
	    ret+="\\cleardoublepage \\section{"+
	      latex_quote(tag->params->title)+"}\n"+
	      convert_to_latex(tag->data);
	    depth--;
	    break;

	  case "section":
	    string tmp="";
	    switch(depth)
	    {
	      default:
	      case 2: tmp+="sub";
	      case 1: tmp+="sub";
	      case 0:
	    }
	    depth++;
	    ret+="\\"+tmp+"section{"+latex_quote(tag->params->title)+"}"+
	      convert_to_latex(tag->data);
	    depth--;
	    break;

	  case "center":
	    ret+="\\begin{center}\n"+
	      convert_to_latex(tag->data)+
	      "\n\\end{center}\n";
	    break;

	  case "firstpage":
	  case "variable":
	  case "constant":
	  case "method":
	  case "function":
	  case "class":
	  case "module":
            // FIXME: but how?
	    ret+=convert_to_latex(tag->data);
	    break;

	  case "pre":
	    if(pre)
	    {
	      ret+=convert_to_latex(tag->data);
	    }else{	
	      pre=1;
	      // FIXME: we will have to remove all tags inside <pre>
	      ret+="{\\startcode\n\n"+
		convert_to_latex(tag->data)+
		"\n}\n";
	      pre=0;
	    }
	    break;

	  case "encaps":
            // FIXME: Manual should not really use <encaps>
	    ret+=convert_to_latex(tag->data);
	    break;

	  case "bloackquote":
	  case "example":
	  {
	    array(string) tmp=
	      convert_to_latex(tag->data)/"\\\\";
	    for(int e=0;e<sizeof(tmp);e++)
	      tmp[e]="\\indent "+tmp[e];
	    ret+=tmp*"\\\\";
	    break;
	  }

	  case "ex_string":
	    // Make all spaces nonbreakable
	    ret+=convert_to_latex(tag->data);
	    break;

	  case "dl":
	    ret+="\\begin{list}{}{} \\item "+
	      convert_to_latex(tag->data)+
	      "\n\\end{list}\n\n";
	    break;
	    
	  case "dt": ret+="\n\\item "; break;
	  case "dd": ret+="\n\\item \\verb+  +"; break;

	  case "ex_indent":
	    ret+="\\verb+  +";
	    break;
	  case "ex_br":
	  case "br":
	    int e=strlen(ret)-1;
	    while(e>=0 && ret[e]==' ') e--;
	    if(e<0 || ret[e]=='\n')
	      ret=ret[..e]+"\n";
	    else
	      ret+="\\\\";
	    break;

	  case "p":
	    ret+="\n\n"+convert_to_latex(tag->data)+"\n\n";
	    break;

	  case "hr":
	    if(tag->params->newline)
	      ret+="\\pagebreak\n";
	    else
	      ret+="\n\n\\begin{tabular}{p{\\linewidth}} \\\\ \\hline \\end{tabular}\n\n";
	    break;

	  case "table":
	    ret+=convert_table(tag);
	    break;

	  case "a":
	    if(tag->params->href)
	    {
	      string href=tag->params->href;
	      if((lower_case(href)/":")[0] == "mailto")
	      {
		ret+=convert_to_latex(tag->data)+
		  latex_quote(" <"+(href/":")[1..]*":"+">");
	      }else{
		ret+=
		  convert_to_latex(tag->data)+
		  "* \\marginpar{"+
		  replace(latex_quote(href),
			  ({".","/",":"}),
			  ({
			    ".\\discretionary{}{}{}",
			    "/\\discretionary{}{}{}",
			    ":\\discretionary{}{}{}",
			  }))+
		  "}";
	      }
	    }else{
	      ret+=convert_to_latex(tag->data);
	    }
	    break;

	  case "link":
	    // FIXME
//	    if(tag->params->to)
//	    {
//	      ret+=
//		convert_to_latex(tag->data)+
//		"\\marginpar{See "+latex_quote(tag->params->to)+"}";
//	    }else{
	      ret+=convert_to_latex(tag->data);
//	    }
	    break;


	  case "ul":
	    ret+="\\begin{itemize} "+
	      convert_to_latex(tag->data,FLAG_LIST)+
	      "\\end{itemize} ";
	    break;

	  case "li":  // FIXME??
	    if(flags & FLAG_LIST)
	      ret+="\\item ";
	    else
	      ; // FIXME, insert filled circle
	    break;


	  case "ol":
	    ret+="\\begin{enumerate} "+
	      convert_to_latex(tag->data,FLAG_LIST)+
	      "\\end{enumerate} ";
	    break;

	  case "anchor":
//          FIXME labels causes tex stack overflow!!
//	    ret+="\\label{"+latex_quote(tag->params->name)+"}";
	    ret+=convert_to_latex(tag->data);
	    break;

	  case "ref":
	    // FIXME: must find out what type of object we are
	    // referencing!!
	    ret+="\\ref{"+tag->params->to+"}";
	    break;


	  case "arguments":
	  case "aargdesc":
	  case "adesc":
	    // FIXME FIXME FIXME
	    break;

	  case "aarg":
	    ret+="{\\tt "+convert_to_latex(tag->data)+"}\\\\";
	    break;

	  case "data_description":
	    ret+=convert_to_latex( html->data_description(tag->params,
							  tag->pos,
							  tag->data,
							  tag) );
	    break;

// #endif

	  case "exercise":
	  case "exercises":
	    // FIXME: 
	    ret+=convert_to_latex(tag->data);
	    break;

	  case "image":
	    ret+="\\epsfbox{"+Wmml.image_to_eps(tag,75.0)+"}";
	    break;

	  case "illustration":
	    ret+="\\epsfbox{"+Wmml.illustration_to_eps(tag,75.0)+"}";
	    break;

	  default:
	    werror("Latex: unknown tag <%s> (near %s)\n",tag->tag,tag->location());

	    if(tag->data)
	      ret+=convert_to_latex(tag->data);
	}
	    
      }
      else
      {
	throw(({"Tag or contents has illegal type: "+sprintf("%O\n",tag),
		  backtrace()}));
      }
    }
  return ret;
}


void output(string base, WMML data)
{
  string x=convert_to_latex(data->data);

  x=replace(x,
	    "\\verb+  +\\verb+  +",
	    "\\verb+    +"
	    );

  x=#"
\\documentclass[twoside,a4paper]{book}
\\usepackage{isolatin1}
\\usepackage{latexsym}  % For $\Box$
\\usepackage{amsmath}
\\usepackage{longtable}
\\usepackage{epsfig}
\\begin{document}
\\author{html2latex}\n
\\setlength{\\unitlength}{1mm}\n

{\\catcode`\\^^20=\\active\\catcode`\\^^0d=\\active%
\\global\\def\\startcode{\\catcode`\\^^20=\\active\\def^^20{\\hbox{\\ }}%
\\catcode`\\^^0d=\\active\\def^^0d{\\hskip0pt\\par\\noindent}%
\\parskip=1pt\\tt}}
"+
    x+
    "\\end{document}\n";
  rm(base+".tex");
  Stdio.write_file(base+".tex",x);
}
