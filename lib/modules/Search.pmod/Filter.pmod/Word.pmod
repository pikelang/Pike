// This file is part of Roxen Search
// Copyright © 2001 Roxen IS. All rights reserved.
//
// $Id: Word.pmod,v 1.12 2004/08/07 15:27:00 js Exp $

inherit .HTML;

constant contenttypes = ({ "application/msword", "application/vnd.ms-word" });
constant fields = ({ "body", "title", "keywords"});

.Output filter(Standards.URI uri, string|Stdio.File data, string content_type)
{
  .Output res=.Output();

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
  string bin = combine_path(getcwd(), "modules/search/bin/wvWare");
  string cwd = combine_path(getcwd(), "modules/search/bin");
  string xml = combine_path(getcwd(), "modules/search/pike-modules/"
				      "Search.pmod/Filter.pmod/wvHtml.xml");
  string doc = combine_path(getcwd(), fn);
  mixed err = catch
  {
    //  Wait for process exit since rm() may otherwise fail
    text = my_popen( ({ bin, "-1", "-c", "utf-8", "-x", xml, doc }), cwd, 1);
  };
  if(!rm(fn))
    werror("Search: Failed to remove temporary file: %s\n", fn);
  if(err)
    throw(err);
  
  return ::filter(uri, text, "text/html", ([]), "utf-8");
}
