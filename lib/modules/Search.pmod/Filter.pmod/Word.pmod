// This file is part of Roxen Search
// Copyright © 2001 Roxen IS. All rights reserved.
//
// $Id: Word.pmod,v 1.6 2001/09/03 21:42:10 marcus Exp $

inherit Search.Filter.HTML;

constant contenttypes = ({ "application/msword", "application/vnd.ms-word" });
constant fields = ({ "body", "title", "keywords"});

#if constant(PIKE_MODULE_RELOC)
#define RELFILE(n) combine_path(getcwd(), master()->relocate_module(__FILE__), "../"n)
#else
#define RELFILE(n) combine_path(getcwd(), __FILE__, "../"n)
#endif

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
  
  string text = my_popen(({ RELPATH("../../../bin/wvWare"),
			    "-c", "utf-8",
			    "-x", RELPATH("wvHtml.xml"),
			    fn }));
  
  if(!rm(fn))
    werror("Search: Failed to remove temporary file: %s\n", fn);
  
  return ::filter(uri, text, "text/html", ([]), "utf-8");
}

string _sprintf()
{
  return "Search.Filter.Word";
}
