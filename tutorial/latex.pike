#include "types.h"
#if __VERSION__ >= 0.6
import ".";
#endif /* __VERSION__ >= 0.6 */
inherit Stdio.File : out;

object html=.html();

WMML global_data;

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
  return low_latex_quote( pre ? text : ((text/"\n") - ({""})) *"\n" );
}

string quote_label(string s)
{
  string ret="";
  while(sscanf(s,"%[_a-zA-Z0-9.:]%c%s",string safe, int char, s)==3)
    ret+=sprintf("%s-%02x",safe,char);
  ret+=s;
  return ret;
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

string mkref(string label)
{
  if(!global_data->links[label])
  {
    return "\\pageref{"+quote_label(label)+"}";
  }
  switch(global_data->links[label]->tag)
  {
    case "chapter":
    case "section":
    case "appendix":
      return global_data->links[label]->tag+" \\ref{"+quote_label(label)+"}";

    default:
      return "\\pageref{"+quote_label(label)+"}";
  }
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

  in_table++;
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
  in_table--;

  ret+="\\end{longtable}\n";

  return ret;
}

string convert_gfx(Sgml.Tag tag)
{
  string file=Gfx.convert( tag->params,
			   "eps",
			   300.0,
			   tag->data && Sgml.get_text(tag->data));

  switch( (file/".")[-1] )
  {
    case "tex": return "\\input{"+file+"}";
    case "eps": return "\\epsfbox{"+file+"}";
    default: return "\\epsfbox{error.eps}";
  }
}

string *srt(string *x)
{
  string *y=allocate(sizeof(x));
  for(int e=0;e<sizeof(y);e++)
  {
    y[e]=lower_case(x[e]);
    sscanf(y[e],"%*[ ,./_]%s",y[e]);
  }
  sort(y,x);
  return x;
}

string low_index_to_latex(INDEX data, string prefix, string indent)
{
//  werror("%O\n",data);
  string ret="";
  foreach(srt(indices(data)-({0})),string key)
    {
      ret+="\\item \\verb+"+indent+"+";
      
      if(data[key][0])
      {
	ret+=latex_quote(Html.unquote_param(key));
	if(data[key][0][prefix+key])
	{
	  // FIXME: show all links
	  ret+=", \\pageref{"+quote_label(data[key][0][prefix+key][0])+"}\n";
	}
	ret+="\n";

	foreach(srt(indices(data[key][0])), string key2)
	  {
	    if(key2==prefix+key) continue;
	    ret+="\\item \\verb+"+indent+"  +";
	    ret+=latex_quote(Html.unquote_param(key2));
	    ret+=", \\pageref{"+quote_label(data[key][0][key2][0])+"}\n";
	}

      }else{
	ret+=latex_quote(Html.unquote_param(key))+"\n";
      }
	
      if(sizeof(data[key]) > !!data[key][0])
      {
	ret+=low_index_to_latex(data[key],prefix+key+".",indent+"  ");
      }
    }

  return ret;
}


// FIXME:
// End chapter / appendix!!
// Too long symbols, fix linebreaking or do not use
// two-column mode...
string index_to_latex(INDEX foo)
{
  string ret="";
  ret+="\n\\twocolumn[\\begin{Huge}Index\\end{Huge}]\n\\begin{small}\n";
  
//  ret+="\\begin{Huge}Index\\end{Huge}\n\n";

  INDEX data=Wmml.group_index(foo);
  data=Wmml.group_index_by_character(data);

  ret+="\\begin{list}{}{}\n";

  foreach(srt(indices(data)-({0})),string key)
    {
      if(sizeof(data[key]) > !!data[key][0])
      {
	ret+="\n\\item \\begin{large}"+
	  latex_quote(key)+
	  "\\end{large}\n";

	ret+=low_index_to_latex(data[key],"","  ");
      }
    }
  ret+="\\end{list}\n";
  ret+="\n\\end{small}\n\\onecolumn\n";
  return ret;
}


int depth;
int appendixes;
int pre;
int in_table;

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
	    ret+="\\chapter{"+
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
	    ret+="\\chapter{"+
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

	  case "variable":
	  case "constant":
	  case "method":
	  case "function":
	  case "class":
	  case "module":
            // FIXME: but how?
	    ret+=convert_to_latex(tag->data);
	    break;

	  case "firstpage":
	    ret+="\\begin{titlepage}\n"+
	      convert_to_latex(tag->data)+
	      "\n\\end{titlepage}\n";
	    break;

	  case "table-of-contents":
	    ret+="\\tableofcontents \\newpage\n";
	    break;

	  case "index":
	  {
	    ret+=index_to_latex(global_data->index_data);

	    break;
	  }

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
	    ret+="\\begin{scshape}"+convert_to_latex(tag->data)+
	      "\\end{scshape}";
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
	    if(pre)
	    {
	      ret+="\n";
	    }else{
	      int e=strlen(ret)-1;
	      while(e>=0 && ret[e]==' ') e--;
	      if(e<0 || ret[e]=='\n')
		ret=ret[..e]+"\n";
	      else
		ret+="\\\\";
	    }
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
			    "\\discretionary{}{}{}.",
			    "\\discretionary{}{}{}/",
			    "\\discretionary{}{}{}:",
			  }))+
		  "}";
	      }
	    }else{
	      ret+=convert_to_latex(tag->data);
	    }
	    break;

	  case "link":
	    // FIXME
	    // (la)tex doesn't like having too many links on one
	    // page. We need to not do this for see-also links.
	    // To do this we need to put hints in the concrete wmml
//	    if(tag->params->to && !in_table)
//	    {
//	      ret+=
//		convert_to_latex(tag->data)+
//		"\\marginpar{See \\ref{"+
//		quote_label(tag->params->to)+"} \\pageref{"+
//		quote_label(tag->params->to)+"}.}";
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
	      ret="$\\bullet$ ";
	    break;

	  case "ol":
	    ret+="\\begin{enumerate} "+
	      convert_to_latex(tag->data,FLAG_LIST)+
	      "\\end{enumerate} ";
	    break;

	  case "anchor":
//          FIXME labels causes tex stack overflow!!
	    ret+="\\label{"+quote_label(tag->params->name)+"}";
	    ret+=convert_to_latex(tag->data);
	    break;

	  case "ref":
	    // FIXME: must find out what type of object we are
	    // referencing!!
	    
	    ret+=mkref(tag->params->to);
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

	  case "img": // FIXME ??
	  case "illustration":
	  case "image":
	    ret+=convert_gfx(tag);
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


string package(string x)
{
  return #"
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
}

string extention=".tex";

void output(string base, WMML data)
{
  global_data=data;
  string x=convert_to_latex(data->data);

  x=replace(x,
	    "\\verb+  +\\verb+  +",
	    "\\verb+    +"
	    );

  x=package(x);

  rm(base+extention);
  Stdio.write_file(base+extention,x);
  array(string) lines=x/"\n";
  array(int) linenum=indices("x"*sizeof(lines));
  array(int) lenghts=sort(Array.map(lines,strlen),linenum,lines);

  werror("Longest line is line %d (%d characters).\n",linenum[-1],lenghts[-1]);
}


Sgml.Tag illustration(object o,void|mapping options)
{
  return Sgml.Tag("image",(["src":Gfx.mkeps(o,options)]),0);
}

