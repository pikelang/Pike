// This file is part of Roxen Search
// Copyright © 2001 Roxen IS. All rights reserved.
//
// $Id: PDF.pmod,v 1.3 2001/07/12 22:45:20 nilsson Exp $

// Filter for application/pdf

inherit Search.Filter.Base;

constant contenttypes = ({ "application/pdf" });
constant fields = ({ "body", "title", "keywords" });

Output filter(Standards.URI uri, string|Stdio.File data, string content_type)
{
  Output res=Output();

  if(objectp(data))
    data=data->read();

  string fn=tmp_filename();
  object f=Stdio.File(fn,"wcb");
  f->write(data);
  f->close();
  
  string text=Process.popen(combine_path(__FILE__, "../bin/xpdf/pdftotext")+" "+fn+" -");
  rm(fn);

  string md="", body="";
  array a=text/"\n----------";

  if(sizeof(a)>1)
  {
    md=a[0];
    body=a[1];
  }

  string field,value="";
  foreach(md/"\n", string md_line)
    if(sscanf(md_line,"%s: %s",field,value)==2)
      res->fields[field]=value;
      
  res->fields->body=get_paragraphs(body)*"\n\n";

  werror(res->fields->body);

  return res;  
}


string _sprintf()
{
  return "Search.Filter.PDF";
}


// Functions for paragraph exctration.

private array(array(string)) split_blocks(array(string) lines) {
  array blocks = ({});
  array block = ({});
  foreach(lines, string line) {
    if(String.trim_all_whites(line)=="") {
      if(sizeof(block)) blocks += ({ block });
      block = ({});
      continue;
    }
    block += ({ line });
  }
  if(sizeof(block)) blocks += ({ block });
  return blocks;
}

private array(string) rotate_r(array(string) lines) {
  if(!sizeof(lines))
    return ({});

  int m = max( @map(lines, sizeof) );
  lines = map(lines, lambda(string row) { return row + " "*(m-sizeof(row)); });
  array newlines = ({});
  for(int i; i<m; i++) {
    string row="";
    foreach(lines, string line)
      row += line[i..i];
    newlines += ({ row });
  }
  return newlines;
}

private array(string) rotate_l(array(string) lines) {
  if(!sizeof(lines))
    return ({});

  int m = max( @map(lines, sizeof) );
  lines = map(lines, lambda(string row) { return row + " "*(m-sizeof(row)); });
  array newlines = ({});
  for(int i=m-1; i>-1; i--) {
    string row="";
    foreach(lines, string line)
      row += line[i..i];
    newlines = ({ row }) + newlines;
  }
  return newlines;
}

private array(string) f_vblocks(array(string) lines) {
  array vblocks = split_blocks(rotate_r(lines));

  if(sizeof(vblocks)==0)
    return vblocks;
  if(sizeof(vblocks)==1)
    return ({ lines*"\n" });

  array res = ({});
  foreach(vblocks, array l)
    res += f_hblocks( rotate_l(l) );
  return res;
}

private array(string) f_hblocks(array(string) lines) {
  array hblocks = split_blocks(lines);

  if(sizeof(hblocks)==0)
    return hblocks;
  if(sizeof(hblocks)==1)
    return ({ lines*"\n" });

  array res = ({});
  foreach(hblocks, array l)
    res += f_vblocks(l);
  return res;
}

private array(string) get_paragraphs(string text) {
  array lines = text / "\n";
  array vblocks = f_vblocks(lines);
  array hblocks = f_hblocks(lines);

  if(sizeof(vblocks)>sizeof(hblocks))
    return vblocks;

  return hblocks;
}
