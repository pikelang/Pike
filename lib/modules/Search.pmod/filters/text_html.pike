// Filter for text/plain
// Copyright © 2000, Roxen IS.

#include "../types.h"

constant contenttypes = ({ "text/html" });

constant whtspaces = ({ "\n", "\r", "\t" });
constant interpunc = ({ ".", ",", ";", ":", "-", "_", "!", "\"", "?",
			"\\", "(", ")", "{", "}", "[", "]" });

inline string normalize(string text) {
  return replace(text,
		 Roxen.replace_entities+whtspaces+interpunc,
		 Roxen.replace_values+({" "})*sizeof(whtspaces+interpunc));
}

inline string remove_slash(string bar) {
  return replace(bar, "/", " ");
}

class Filter {
  array(string) content=({});
  array(int) context=({});
  array(int) offset=({});

  string title;

  void set_content(string c) {

    // Lazy preprocess...
    c=parse_html(normalize(c), ([]), (["script":""]) );

    int counter=0;
    int p_con=T_NONE;
    int next;
    int zero;
    int parse_atomic;
    while (counter<sizeof(c)) {
      next=search(c, "<", counter);
      if(next==-1) {
	content+=({ remove_slash(c[counter+1..]) });
	context+=({ p_con });
	offset+=({ counter });
	break;
      }

      if(counter!=next) {
	content+=({ remove_slash(c[counter+1..next-1]) });
	context+=({ p_con });
	offset+=({ counter });
      }

      next++;

      // Alter context?
      zero=0;
      string c4=lower_case(c[next..next+4]);
      string c2=c4[..1];
      if(c4=="title")
	p_con=T_TITLE;
      else if(c2=="h1")
	p_con=T_H1;
      else if(c2=="h2")
	p_con=T_H2;
      else if(c2=="h3")
	p_con=T_H3;
      else if(c2=="h4")
	p_con=T_H4;
      else if(c2=="h5")
	p_con=T_H5;
      else if(c2=="h6")
	p_con=T_H6;
      else if(c2=="th")
	p_con=T_TH;
      else if(c[next+1]=='>' || c[next+1]=='/' || c[next+1]==' ') {
	if(c[next]=='b' || c[next]=='B')
	  p_con=T_B;
	else if(c[next]=='a' || c[next]=='A')
	  p_con=T_A;
	else if(c[next]=='i' || c[next]=='I')
	  p_con=T_I;
	else
	  zero=1;
      }
      else
	zero=1;

      // Keep context?
      parse_atomic=0;
      if(zero) {
	if(c2=="br")
	  zero=0;
	else if(c4[..2]=="img") {
	  zero=0;
	  parse_atomic=1;
	}
	else if(c4[..3]=="meta")
	  parse_atomic=2;
      }

      if(zero) p_con=T_NONE;

      counter=search(c, ">", next);
      if(counter==-1) break;
      if(!parse_atomic) continue;
      if(parse_atomic==1) {
	string alt, img=lower_case(c[next..counter]);
	if(sscanf(img, "%*salt=\"%s\"", alt)==2 ||
	   sscanf(img, "%*salt='%s'", alt)==2 ||
	   sscanf(img, "%*salt=%s ", alt)==2 )

	  {
	    content+=({ remove_slash(alt) });
	    context+=({ T_ALT });
	    offset+=({ next }); // Not true
	  }
	continue;
      }
      if(parse_atomic==2) {
	string name, cont, meta=lower_case(c[next..counter]);
	if(sscanf(meta, "%*sname=\"%s\"", name)!=2 &&
	   sscanf(meta, "%*sname='%s'", name)!=2 &&
	   sscanf(meta, "%*sname=%s ", name)!=2)
	  continue;
	if(sscanf(meta, "%*scontent=\"%s\"", cont)!=2 &&
	   sscanf(meta, "%*scontent='%s'", cont)!=2 &&
	   sscanf(meta, "%*scontent=%s ", cont)!=2)
	  continue;
	if(name=="description") {
	  content+=({ remove_slash(cont) });
	  context+=({ T_DESC });
	  offset+=({ next }); // Not true
	  continue;
	}
	if(name=="keywords") {
	  content+=({ remove_slash(cont) });
	  context+=({ T_KEYWORDS });
	  offset+=({ next }); // Not true
	  continue;
	}

      }
    }
  }

  void add_content(string c, int t) {
    content+=({ (c) });
    context+=({ t });
    offset+=({ T_NONE });
  }
			    
  array(array) get_filtered_content() {
    return ({ content, context, offset });
  }

  array get_anchors() { return 0; }
  string get_title() { 
    int pos=search(context, T_TITLE);
    if (pos==-1) return "";
    return content[pos];
  }
  string get_keywords() {
    int pos=search(context, T_KEYWORDS);
    if (pos==-1) return "";
    return content[pos];
  }
  string get_description() {
    int pos=search(context, T_DESC);
    if (pos==-1) return "";
    return "";
  }
}
