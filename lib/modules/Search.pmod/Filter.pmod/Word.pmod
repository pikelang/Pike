// This file is part of Roxen Search
// Copyright © 2001 Roxen IS. All rights reserved.
//
// $Id: Word.pmod,v 1.8 2001/11/19 13:33:15 js Exp $

inherit Search.Filter.HTML;

constant contenttypes = ({ "application/msword", "application/vnd.ms-word" });
constant fields = ({ "body", "title", "keywords"});

Output filter(Standards.URI uri, string|Stdio.File data, string content_type)
{
  Output res=Output();

  if(objectp(data))
    data=data->read();

  string fn = tmp_filename();
  object f = Stdio.File(fn, "wct");
  int r = f->write(data);
  f->close();
  if(r != sizeof(data))
    error("Failed to write data for %O (returned %O, not %O)\n",
	  fn, r, sizeof(data));
  
  string text;
  mixed err = catch
  {
    text = my_popen(({ "modules/search/bin/wvWare",
		       "-c", "utf-8",
		       "-x", "modules/search/pike-modules/Search.pmod/Filter.pmod/wvHtml.xml",
		       fn }));
  };
  if(!rm(fn))
    werror("Search: Failed to remove temporary file: %s\n", fn);
  if(err)
    throw(err);
  
  return ::filter(uri, text, "text/html", ([]), "utf-8");
}

string _sprintf()
{
  return "Search.Filter.Word";
}
