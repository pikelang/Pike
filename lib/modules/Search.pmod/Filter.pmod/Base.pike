// This file is part of Roxen Search
// Copyright © 2001 Roxen IS. All rights reserved.
//
// $Id: Base.pike,v 1.16 2004/08/07 15:26:59 js Exp $

//! The MIME content types this class can filter.
constant contenttypes = ({ });

constant tmp_filename = Search.TmpFile.tmp_filename;

//!
.Output filter(Standards.URI uri, string|Stdio.File data,
	      string content_type, mixed ... more);

string my_popen(array(string) args, string|void cwd, int|void wait_for_exit)
  // A smarter version of Process.popen: No need to quote arguments.
{    
  Stdio.File pipe0 = Stdio.File();
  Stdio.File pipe1 = pipe0->pipe(Stdio.PROP_IPC);
  if(!pipe1)
    if(!pipe1) error("my_popen failed (couldn't create pipe).\n");
  mapping setup = ([ "env":getenv(), "stdout":pipe1 ]);
  if (cwd)
    setup["cwd"] = cwd;
  Process.create_process proc = Process.create_process(args, setup);
  pipe1->close();
  string result = pipe0->read();
  if(!result)
    error("my_popen failed with error "+pipe0->errno()+".\n");
  pipe0->close();
  if (wait_for_exit)
    proc->wait();
  return result;
}
