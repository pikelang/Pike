// This file is part of Roxen Search
// Copyright © 2001 - 2009, Roxen IS. All rights reserved.
//
// $Id: PDF.pmod,v 1.17 2009/05/25 18:26:52 mast Exp $

// Filter for application/pdf

inherit .HTML;

constant contenttypes = ({ "application/pdf" });

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
  string bin = combine_path(getcwd(), "modules/search/bin/pdftohtml");
  string cwd = combine_path(getcwd(), "modules/search/bin");
  string doc = combine_path(getcwd(), fn);
  mixed err = catch {
    //  Wait for process exit since rm() may otherwise fail
    text = my_popen(({ bin, "-noframes", "-i", "-stdout", doc}), cwd, 1);
  };
  if(!rm(fn))
    werror("Search: Failed to remove temporary file: %s\n", fn);
  if(err)
    throw(err);


  res = ::filter(uri, text, "text/html", ([]));

  if (res->fields->title == combine_path(getcwd(), fn))
    res->fields->title = "";

  return res;
}
