// This file is part of Roxen Search
// Copyright © 2000,2001 Roxen IS. All rights reserved.
//
// $Id: PDF.pike,v 1.1 2001/06/28 12:16:43 js Exp $

// Filter for text/plain

inherit Search.Filter.Base;

constant contenttypes = ({ "application/pdf" });
constant fields = ({ "body","title", "keywords"});

Output filter(Standards.URI uri, string|Stdio.File data, string content_type)
{
  Output res=Output();

  if(objectp(data))
    data=data->read();

  string s=Process.popen(sprintf(""));

  string fn=tmp_filename();
  object f=Stdio.File(fn,"wcb");
  f->write(file);
  f->close();
  
  string text=Process.popen(combine_path(__FILE__, "bin/xpdf/pdftotext")+" "+fn+" -");
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
      
  res->fields->body=body;

  return res;  
}

string _sprintf()
{
  return "Search.Filter.PDF";
}
