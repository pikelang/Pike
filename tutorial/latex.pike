#include "types.h"
#if __VERSION__ >= 0.6
import ".";
#endif /* __VERSION__ >= 0.6 */
inherit Stdio.File : out;

// Requires
//  ghostscript 4.* or later
//  teTeX 1.0.*
//  transfig 3.2.1

// Todo:
// replace $\\;$ with \hspace ?


object html=.html();

WMML global_data;

string latex = "latex";

// Blue
// #define BORDERCOLOR "0.153,0.13,0.357"
// #define ROWCOLOR "0.867,0.933,1"

// Green
#define BORDERCOLOR "0,0.2,0"
#define ROWCOLOR "0.627,0.878,0.753"

object wcache=.Cache(latex+".wcache");

string SillyCaps(string s)
{
  return Array.map(lower_case(s)/" ",String.capitalize)*" ";
}

array(float) find_line_width(array(SGML) data)
{
  array(string) keys=Array.map(data,Sgml.get_text);
  array(float) ret=rows(wcache, keys);

  if(search(ret, 0) != -1)
  {
    string x=
      "\\begin{titlepage}\n"
      "\\newlength{\\gnapp}\n";
    
    foreach(data, SGML d)
      {
	x+=
	  "\\settowidth{\\gnapp}{"+convert_to_latex(d)+"}\n"
	  "\\message{length=\\number\\gnapp dots}\n";
      }
    
    x+="\\end{titlepage}\n";

    x=package(x);
    
    rm("___tmp.tex");
    Stdio.write_file("___tmp.tex",x);
    string tmp=Process.popen(latex+" '\\scrollmode\\input ___tmp.tex'");
    
    sscanf(tmp,"%{%*slength=%f%}",array lengths);
//	  werror("%O\n",lengths);
//	  werror("%O\n",keys);

    if(sizeof(lengths) != sizeof(keys))
    {
      werror("Latex exection failed.\n");
      werror("%s",tmp);
      exit(1);
    }
    
    for(int e=0;e<sizeof(lengths);e++)
    {
      lengths[e]=lengths[e][0] / 65536; // convert to points
//	      werror("Width of %s is %f\n",key, lengths[e][0]/65536);
    }

    
    wcache->set_many(keys, lengths);
    
    ret=rows(wcache, keys);
  }

  return ret;
}

array(SGML) split_lines(SGML data)
{
  array(SGML) lines=({ ({}) });

  for(int e=0;e<sizeof(data);e++)
  {
    TAG tag=data[e];
    if(objectp(tag))
    {
      switch(tag->tag)
      {
	case "hr":
	  break;
	  
	case "br":
	  lines+=({ ({}) });
	  break;

	case "center":
	case "p":
	  lines+=({ ({}) });
	  if(tag->data) lines+=split_lines(tag->data);
	  lines+=({ ({}) });
	  break;

	case "h1":
	case "h2":
	case "h3":
	case "h4":
	case "h5":
	case "h6":
	  lines+=({ ({}) });
	  foreach(split_lines(tag->data),SGML data)
	    lines+=({ ({ Sgml.Tag(tag->tag,
				  tag->params,
				  tag->pos, 
				  data) }) });
	  lines+=({ ({}) });
	  break;

	case "ex_keyword":
	case "smallcaps":
	case "sub":
	case "sup":
	case "tt":
	case "ex_meta":
	case "i":
	case "b":
	case "a":
	{
	  array tmp=split_lines(tag->data);
	  lines[-1]+= ({ Sgml.Tag(tag->tag,
				  tag->params,
				  tag->pos, 
				  tmp[0]) });
	  foreach(tmp[1..],SGML data)
	    lines+=({ ({ Sgml.Tag(tag->tag,
				  tag->params,
				  tag->pos, 
				  data) }) });
	  break;
	}

	case "link":
	  if(tag->data) 
	  {
	    array tmp=split_lines(tag->data);
	    lines[-1]+=tmp[0];
	    lines+=tmp[1..];
	  }
	  break;

	default:
	  lines[-1]+=({tag});

      }
    }else{
      lines[-1]+=({tag});
    }
  }

  return lines;
}

array(float) find_max_width(array(SGML) datas)
{
  array(array(SGML)) pieces=Array.map(datas,split_lines);
  array(float) widths=find_line_width(pieces * ({}));
  int pos=0;
  array(float) ret=allocate(sizeof(pieces));
  for(int q=0;q<sizeof(pieces);q++)
  {
    int num=sizeof(pieces[q]);
    ret[q]=max(0.0,@widths[pos..pos+num-1]);
    pos+=num;
  }
//  werror("%O\n%O\n%O\n",widths,pieces,ret);
  if(pos != sizeof(widths))
    error("Major internal error!\n");
  return ret;
}


// FIXME: improve this!!!

array(SGML) split_words(SGML data)
{
  array(SGML) words=({ ({}) });

  for(int e=0;e<sizeof(data);e++)
  {
    TAG tag=data[e];
    if(objectp(tag))
    {
      switch(tag->tag)
      {
	case "hr":
	  break;
	  
	case "wbr":
	  words[-1]+=({"-"});
	case "br":
	  words+=({ ({}) });
	  break;

	case "center":
	case "p":
	  words+=({ ({}) });
	  if(tag->data) words+=split_words(tag->data);
	  words+=({ ({}) });
	  break;

	case "h1":
	case "h2":
	case "h3":
	case "h4":
	case "h5":
	case "h6":
	  words+=({ ({}) });
	  foreach(split_words(tag->data),SGML data)
	    words+=({ ({ Sgml.Tag(tag->tag,
				  tag->params,
				  tag->pos, 
				  data) }) });
	  words+=({ ({}) });
	  break;

	case "ex_keyword":
	case "smallcaps":
	case "sub":
	case "sup":
	case "ex_meta":
	case "i":
	case "tt":
	case "b":
	case "a":
	{
	  array tmp=split_words(tag->data);
	  words[-1]+=({ Sgml.Tag(tag->tag,
				  tag->params,
				  tag->pos, 
				  tmp[0]) });
	  foreach(tmp[1..],SGML data)
	    words+=({ ({ Sgml.Tag(tag->tag,
				  tag->params,
				  tag->pos, 
				  data) }) });
	  break;
	}

	case "link":
	  if(tag->data) 
	  {
	    array tmp=split_words(tag->data);
	    words[-1]+=tmp[0];
	    words+=tmp[1..];
	  }
	  break;



	default:
	  words[-1]+=({tag});

      }
    }else{
      array tmp=replace(tag,"\n"," ")/" ";
      words[-1]+=({ tmp[0] });
      foreach(tmp[1..], string word) words+=({ ({word}) });
    }
  }

  return words;
}


array(float) find_min_width(array(SGML) datas)
{
  array(array(SGML)) pieces=Array.map(datas,split_words);
  array(float) widths=find_line_width(pieces * ({}));
  int pos=0;
  array(float) ret=allocate(sizeof(pieces));
  for(int q=0;q<sizeof(pieces);q++)
  {
    int num=sizeof(pieces[q]);
    ret[q]=max(0.0,@widths[pos..pos+num-1]);
    pos+=num;
  }
//  werror("%O\n%O\n%O\n",widths,pieces,ret);
  if(pos != sizeof(widths))
    error("Major internal error!\n");
  return ret;
}

array(string) quote_from=
({"<",">",
  "{","}",
  "µ","&",
  " ","\\",
  "[","]",
  "#","%",
  "$","~",
  "^","_",
});

array(string) quote_to=
({"$<$","$>$",
  "\\{","\\}",
  "$\\mu$","\\&",
  "$\\;$","$\\backslash$",
  "\\symbol{91}","\\symbol{93}",
  "\\#","\\%",
  "\\$","\\symbol{126}",
  "\\symbol{94}","\\_",
});

string low_latex_quote(string text)
{
  return replace( text, quote_from, quote_to );
}


string latex_quote(string text)
{
  if(!strlen(text)) return text;
  if(!pre)
  {
    if(text[-1]=='\n') text[-1]=' ';
    text= ((text/"\n") - ({""})) *"\n";
  }
  text=low_latex_quote(text);
#if 0
  if(space_out_quoting)
  {
    text=replace(text,
		 ({ " - ","-"}),
		 ({ "\\emdash ","\\hyphen "})
      );
  }
#endif
  return text;
}

string quote_label(string s)
{
  string ret="";
  while(sscanf(s,"%[a-zA-Z0-9.:]%c%s",string safe, int char, s)==3)
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
      ]), values(tag)));
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
	switch(tag->tag)
	{
	  case "li": len+=2.0; break;
	}
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

// in points
#define MAX_TABLE_SIZE 345.0

string convert_table(TAG table)
{
  SGML data=table->data;
  int rows,columns,nicer,framed;

  // FIXME, nicer tables not supported (yet)

  // border: lines around each cell
  int border=(int)table->params->border;

  // framed: line around table
  int framed=(int)table->params->framed;

  // extra space between columns
  int spaced;

  // A short table, no page breaks...
  int short=(int)table->params->small;

  if(table->params->nicer)
  {
    nicer=1;
    framed=1;
    border=0;
    spaced=1;
  }
  framed|=border;
  array(float) column_data=allocate(100,1.0);


  int cellpadding=3;
  if(table->params->cellpadding)
    cellpadding=(int)table->params->cellpadding;

  int columns=0;

  class Cell
  {
    TAG tag;
    int cols;

    void create(TAG t, int c) { tag=t; cols=c; }
  };
  
  array(array(Cell)) table=({});

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
	
	array(Cell) row=({});
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
		{
		  int cols=1;

		  if(cell->colspan) 
		    cols=(int)cell->colspan;

		  row+=({ Cell(cell,cols) }) * cols;
		}
	      }
	    }
	  }
	table+=({row});
      }
    }

  columns=max(0,@Array.map(table,sizeof));

  for(int row=0;row<sizeof(table);row++)
  {
    table[row]+=({Cell(Sgml.Tag("td",0,0,({})),1)})*
      ( columns - sizeof(table[row]));
  }

  werror("Converting table %02dx%02d %s.\n",columns,sizeof(table),
	 String.implode_nicely(( nicer? ({ "nicer" }) : ({}) ) +
			       ( framed? ({ "framed" }) : ({}) ) +
			       ( border? ({ "border" }) : ({}) ) +
			       ( spaced? ({ "spaced" }) : ({})) ));
  

  array(SGML) tmp=(table * ({}))->tag->data;

//  werror("%O\n",tmp);

  array(array(float)) maxwidths=find_max_width(tmp)/columns;
  array(array(float)) minwidths=find_min_width(tmp)/columns;


  array(float) column_data=allocate(columns,0.0);
  array(float) column_max=allocate(columns,0.0);
  
  for(int row=0;row<sizeof(table);row++)
  {
    for(int col=0;col<columns;col++)
    {
      maxwidths[row][col]+=cellpadding;
      minwidths[row][col]+=cellpadding;

      column_data[col]+=maxwidths[row][col]/table[row][col]->cols;
      column_max[col]=max(maxwidths[row][col]/table[row][col]->cols,
			  column_max[col]);
    }
  }

#ifdef TABLE_DEBUG
  for(int row=0;row<sizeof(table);row++)
  {
    for(int col=0;col<columns;col++)
      werror("%15s ",Sgml.get_text( table[row][col]->tag->data )[0..14]);
    werror("\n");
    for(int col=0;col<columns;col++)
      werror("%15f ",maxwidths[row][col]);
    werror("\n");
    for(int col=0;col<columns;col++)
      werror("%15f ",minwidths[row][col]);
    werror("\n");
  }
#endif

//  werror("%O %O\n",minwidths,maxwidths);

  array(float) actual_widths=allocate(columns,0.0);
  array(int) fixed=allocate(columns);
  array(int) do_not_shrink=allocate(columns);

  // FIXME: Try to avoid sizes which are close
  // to column_max[e], it makes line breaking look silly
	
  int iteration;
  while(1)
  {
    int adjusted=0;
    float total=0.0;
    float left=MAX_TABLE_SIZE;

// #define TABLE_DEBUG

#ifdef TABLE_DEBUG
  werror("********* Table sizes, iteration %d\n",iteration++);
  for(int e=0;e<columns;e++)
    {
      werror("%2d: data: %8f  actual: %8f fixed: %d  no shrink: %d\n",e,
	     column_data[e],
	     actual_widths[e],
	     fixed[e],
	     do_not_shrink[e]);
    }
  werror("********* Total width: %f\n",`+(0.0,@actual_widths));
#define TD(X) X
#else
#define TD(X)
#endif


    for(int e=0;e<columns;e++)
    {
      if(!fixed[e])
	total+=column_data[e];
      else
	left-=actual_widths[e];
    }

    TD(werror("left=%f\n",left));

    if(total==0.0) break;

    for(int e=0;e<columns;e++)
      if(!fixed[e])
	actual_widths[e]=left * column_data[e]/total;


    // Enlarge if required
    for(int row=0;row<sizeof(table);row++)
    {
      for(int col=0;col<columns;col+=table[row][col]->cols)
      {
	int cols=table[row][col]->cols;
	if(cols > 1) continue;
	
	if(actual_widths[col] < minwidths[row][col])
	{
	  actual_widths[col]=minwidths[row][col];
	  adjusted=1;
	  fixed[col]=1;
	}
      }
    }
    if(adjusted) continue;
    TD(werror("1-column columns are ok\n"));
    
    // Enlarge if required
    for(int row=0;row<sizeof(table);row++)
    {
      for(int col=0;col<columns;col+=table[row][col]->cols)
      {
	int cols=table[row][col]->cols;
	if(cols < 1) continue;
	if(`+(0.0,@actual_widths[col..col+cols-1]) < minwidths[row][col])
	{
	  int num_unfixed=0;
	  int tmp;
	  float size=minwidths[row][col];
	  for(int e=0;e<cols;e++)
	  {
	    if(fixed[col+e])
	    {
	      size-=actual_widths[col+e];
	    }else{
	      num_unfixed++;
	      tmp=e;
	    }
	  }

	  if(num_unfixed==1)
	  {
	    fixed[col+tmp]=1;
	    actual_widths[col+tmp]=size;
	    adjusted=1;
	    continue;
	  }


	  if(do_not_shrink[col] < 40)
	  {
	    for(int e=0;e<cols;e++)
	    {
	      // This might not work if there is no more room in the
	      // page...
	      column_data[col+e]*=1.1;
	      do_not_shrink[cols+e]++;
	      adjusted=1;
	    }
	  }
	}
      }
    }
    
    if(adjusted) continue;
    TD(werror("multi-column columns are ok\n"));
    
    // Shrink columns whenever possible
    for(int col=0;col<columns;col++)
    {
      if(do_not_shrink[col]) continue;
      if(fixed[col]) continue;

      if(actual_widths[col] > column_max[col])
      {
	actual_widths[col]=column_max[col];
	fixed[col]=1;
	adjusted=1;
      }
    }
    
    if(adjusted) continue;
    TD(werror("everything is ok\n"));
    
    break;
  }


#if 0
  werror("********* Table sizes:\n");
  for(int e=0;e<columns;e++)
    {
      werror("%2d: data: %8f  actual: %8f\n",e,
	     column_data[e],
	     actual_widths[e]);
    }
  werror("********* Total width: %f\n",`+(0.0,@actual_widths));
#endif
  
  float total_data=`+(@column_data);

  
  string fmt=(({"l"}) * columns) * (border?"|": (spaced?"l":"") );
  if(framed) fmt="|"+fmt+"|";

  array(string) ret_rows=({});

  in_table++;

  int rownum;

  // FIXME: handle <th>
  foreach(table, array(Cell) row)
    {
      array(string) ltxrow=({});
      int head;

      for(int col=0;col<columns;col+=row[col]->cols)
      {
	string r="";
	int cols=row[col]->cols;
	if(cols > 1) r+="\\multicolumn{"+cols+"}{l}{";
	r+="\\begin{minipage}{"+actual_widths[col]+"pt}\n";

	if(row[col]->tag->tag == "th")
	  head=1;

	switch(row[col]->tag->params->align)
	{
	  // FIXME: might need to add \\ to end of each line
	  case "right":
	    r+="\\begin{flushright}\n";
	    r+=convert_to_latex(row[col]->tag->data);
	    r+="\\end{flushright}\n";
	    break;

	  // FIXME: might need to add \\ to end of each line
	  case "center":
	    r+="\\begin{center}\n";
	    r+=convert_to_latex(row[col]->tag->data);
	    r+="\\end{center}\n";
	    break;

	  default:
	  case "left":
	    r+=convert_to_latex(row[col]->tag->data);
	    break;
	}
	r+="\\end{minipage}";
	if(cols > 1) r+="}";
	ltxrow+=({r});
      }

      string color="";
      string colorend="";
      if(nicer)
      {
	if(head)
	{
	  color="\\rowcolor[rgb]{" BORDERCOLOR "}%%\n";
	  for(int e=0;e<sizeof(ltxrow);e++)
	    ltxrow[e]="\\color[rgb]{1,1,1}%\n"+ltxrow[e];
	  ltxrow[-1]+="\\normalcolor ";
	}else{
	  color=sprintf("\\rowcolor[rgb]{%s}%%\n",(rownum++&1)?"1,1,1":ROWCOLOR);
	}
      }



      ret_rows+=({
	color+
	ltxrow* (spaced?" & $\\;$ & ":" & ")+
	(head ? "\\endhead\n" :  "\\\\\n" )
      });
    }
  in_table--;


  string ret="\n\n";
  int restart_multi;

  if(short)
  {
    ret+="\\begin{tabular}{"+ fmt +"}\n";
  }else{
    if(multicolumn)
    {
      restart_multi=1;
      ret+="\\end{multicols}";
    }
    ret+="\\begin{longtable}{"+ fmt +"}\n";
  }
  if(framed && !border) ret+="\\hline\\endfoot\n";
  if(framed) ret+="\\hline\n";
  ret+=ret_rows * ( border ? "\\hline\n" : "" );
  if(framed) ret+="\\hline\n";

  if(short)
  {
    ret+="\\end{tabular}\n";
  }else{
    ret+="\\end{longtable}\n";
  }

  if(restart_multi)
  {
    ret+="\\begin{multicols}{2}\n";
    multicolumn=1;
  }

  return ret;
}

string convert_gfx(Sgml.Tag tag)
{
  string file;
  float dpi;
  [file,dpi]=Gfx.convert( tag->params,
			  "eepic|eps",
			  300.0,
			  tag->data && Sgml.get_text(tag->data));

  switch( (file/".")[-1] )
  {
    case "tex": return "\\input{"+file+"}";
    default: file="error.eps";

    case "eps":
//      return "\\epsfbox{"+file+"}";
      return "\\includegraphics{"+file+"}";
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

string fix_index_key(string key)
{
  return replace(latex_quote(Html.unquote_param(key)),
		 ".",
		 "\\discretionary{}{}{}.");

}

string low_index_to_latex(INDEX data, string prefix, int indent)
{
//  werror("%O\n",data);
  string ret="";
  foreach(srt(indices(data)-({0})),string key)
    {
      ret+="\\item "+"$\\;$$\\;$$\\;$"*indent;
      
      if(data[key][0])
      {
	ret+=fix_index_key(key);
	if(data[key][0][prefix+key])
	{
	  // FIXME: show all links
	  ret+=", \\pageref{"+quote_label(data[key][0][prefix+key][0])+"}\n";
	}
	ret+="\n";

	foreach(srt(indices(data[key][0])), string key2)
	  {
	    if(key2==prefix+key) continue;
	    ret+="\\item "+"$\\;$$\\;$$\\;$"*(indent+1);
	    ret+=fix_index_key(key2);
	    ret+=", \\pageref{"+quote_label(data[key][0][key2][0])+"}\n";
	}

      }else{
	ret+=fix_index_key(key)+"\n";
      }
	
      if(sizeof(data[key]) > !!data[key][0])
      {
	ret+=low_index_to_latex(data[key],prefix+key+".",indent+1);
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
  ret+="\n\\twocolumn\n\\chapter*{Index}\n\\begin{small}\n";
  
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

	ret+=low_index_to_latex(data[key],"",1);
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
int space_out_quoting;
int multicolumn;

constant FLAG_TABLE=1;
constant FLAG_LIST=2;
constant FLAG_MAN_HEAD=4;


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
	  case "tt":
	    if(flags & FLAG_MAN_HEAD)
	    {
	      space_out_quoting++;
	      ret+="\\begin{Large}\\PikeHead{"+
		convert_to_latex(tag->data)+
		"}\\end{Large} ";
	      space_out_quoting--;
	    }else{
	      ret+="{\\tt "+convert_to_latex(tag->data)+"}";
	    }
	    break;
	    
	  case "ex_keyword":
	  case "b": ret+="\\textbf{"+convert_to_latex(tag->data)+"}"; break;
	  case "ex_meta":

	    // FIXME: support <emph> and <i> correctly.
	  case "i": ret+="\\textit{"+convert_to_latex(tag->data)+"}"; break;
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
	    switch(lower_case(tag->params->title))
	    {
	      case "function":
	      case "method":
	      case "constant":
	      case "variable":
	      case "module":
	      case "class":
	      case "name":
		ret+="\n\n\\pagebreak[0] "+
		  convert_to_latex(tag->data,FLAG_MAN_HEAD)+"\n"
		  "\\hrule\\nopagebreak ";
		break;

	      case "syntax":
		ret+="\\nopagebreak\n\n\\nopagebreak \\parbox{\\linewidth}{%\n"+
		  convert_to_latex(tag->data)+
		  "}\n\\nopagebreak\n";
		break;

	      default:
		ret+="\n\n"+
		  "\\PikeHeaderFont "+latex_quote(SillyCaps(tag->params->title))+"\\normalfont\\normalcolor\\\\\n "+
		  convert_to_latex(tag->data)+
		  "\n";
	    }
	    break;

	  case "chapter":
	    depth++;
	    space_out_quoting++;
	    ret+="\\chapter{"+latex_quote(tag->params->title)+"}\n";
	    space_out_quoting--;
//	    ret+="\\begin{multicols}{2}\n";
//	    multicolumn=1;
	    ret+=convert_to_latex(tag->data);
//	    multicolumn=0;
//	    ret+="\\end{multicols}\n";
	    depth--;
	    break;

	  case "appendix":
	    if(!appendixes)
	    {
	      ret+="\n\\appendix\n";
	      appendixes=1;
	    }
	    depth++;
	    space_out_quoting++;
	    ret+="\\chapter{"+latex_quote(tag->params->title)+"}\n";
	    space_out_quoting--;
	    ret+=convert_to_latex(tag->data);
	    depth--;
	    break;

	  case "section":
	  {
	    int restart;
	    int olddepth=depth;
	    string tmp="";
	    switch(depth)
	    {
	      // For sections outside chapters
	      case 0: depth++; break;
                
	      default:
	      case 3: tmp+="sub";
	      case 2: tmp+="sub";
	      case 1:
	    }
	    depth++;
	    space_out_quoting++;
	    if(multicolumn)
	    {
	      restart=1;
	      multicolumn=0;
	      ret+="\\end{multicols}\n";
	    }
	    ret+="\\"+tmp+"section{"+latex_quote(tag->params->title)+"}";
	    if(restart)
	    {
	      ret+="\\begin{multicols}{2}\n";
	      multicolumn=1;
	    }
	    space_out_quoting--;
	    ret+=convert_to_latex(tag->data);
	    depth=olddepth;
	    break;
	  }

	  case "center":
	  {
	    int restart;

	    if(multicolumn)
	    {
	      restart=1;
	      multicolumn=0;
	      ret+="\\end{multicols}\n";
	    }
	    ret+="\\begin{center}\n";
	    ret+=convert_to_latex(tag->data);
	    ret+="\n\\end{center}\n";
	    if(restart)
	    {
	      ret+="\\begin{multicols}{2}\n";
	      multicolumn=1;
	    }
	    break;
	  }

	  case "class":
	  case "module":
#if 1
	  if(depth)
	  {
	    string tmp="";
	    switch(depth)
	    {
	      // For sections outside chapters
	      case 0: depth++; break;
                
	      default:
	      case 3: tmp+="sub";
	      case 2: tmp+="sub";
	      case 1:
	    }

	    if(tag->params->name)
	    {
	      foreach(tag->params->name/",", string name)
		{
		  sscanf(name," %s",name);
		  ret+="\n\\addcontentsline{toc}{"+tmp+"section}{"+
		    latex_quote(name)+
		"}\n";
		}
	    }
	  }
#endif
	  case "variable":
	  case "constant":
	  case "method":
	  case "function":
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
	      ret+="\\begin{small}{\\startcode\n\n"+
		convert_to_latex(tag->data)+
		"\n}\\end{small}\n";
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
	    // FIXME: this could probably be fixed better with
	    // a parbox...
	    string tmp1=convert_to_latex(tag->data);
	    if(pre) tmp1=replace(tmp1,"\n","\\\\");
	    array(string) tmp=tmp1/"\\\\";

	    for(int e=0;e<sizeof(tmp);e++)
	      tmp[e]="\\indent "+tmp[e];
	    ret+=tmp* (pre ? "\n" : "\\\\");

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
	  case "dd": ret+="\\nopagebreak\n\\item $\\;$$\\;$$\\;$$\\;$\\nopagebreak "; break;

	  case "ex_indent":
	    ret+="$\\;$$\\;$$\\;$$\\;$";
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
	    if(!pre) // FIXME: 
	    {
	      ret+="\n\n";
	      if(tag->data) ret+=convert_to_latex(tag->data)+"\n\n";
	    }
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
		  "* \\marginpar{\\footnotesize{"+
		  replace(latex_quote(href),
			  ({".","/",":"}),
			  ({
			    "\\discretionary{}{}{}.",
			    "\\discretionary{}{}{}/",
			    "\\discretionary{}{}{}:",
			  }))+
		  "}}";
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
	    werror("Latex: unknown tag <%s> (near %s)\n",tag->tag,tag->location()||"");

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
\\input{idonex-fonts.tex}
\\usepackage{epic}
\\usepackage{eepic}
\\usepackage{isolatin1}
\\usepackage{latexsym}  % For $\Box$
\\usepackage{amsmath}
\\usepackage{longtable}
\\usepackage[dvips]{graphicx}
\\usepackage[dvips]{color}  % colors becomes wrong in xdvi, but right in postscript
\\usepackage{colortbl}
\\usepackage{parskip}
\\begin{document}
\\author{wmml to latex}
\\setlength{\\unitlength}{1mm}

{\\catcode`\\^^20=\\active\\catcode`\\^^0d=\\active%
\\global\\def\\startcode{\\catcode`\\^^20=\\active\\def^^20{\\hbox{\\ }}%
\\catcode`\\^^0d=\\active\\def^^0d{\\hskip0pt\\par\\noindent}%
\\parskip=1pt\\tt}}
"+
    x+
    "\\LoadCustomFonts\\end{document}\n";
}

string extention=".tex";

void output(string base, WMML data)
{
  global_data=data;
  string x=convert_to_latex(data->data);

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

