#!/usr/local/bin/pike

/* $Id: sendfiletest.pike,v 1.1 1999/10/14 20:52:01 grubba Exp $ */

void done(int sent)
{
  rm("conftest.Makefile");
  exit(0);
}

int main(int argc, array(string) argv)
{
  object from = Stdio.File();
  object to = Stdio.File();

  if (!from->open("Makefile", "r")) {
    werror("Failed to open \"Makefile\" for reading!\n");
    exit(1);
  }
  if (!from->open("conftest.Makefile", "cwt")) {
    werror("Failed to open \"conftest.Makefile\" for writing!\n");
    exit(1);
  }
  mixed err = catch {
    if (!Stdio.sendfile(0, from, 0, -1, 0, to, done)) {
      werror("Stdio,sendfile() failed!\n");
      exit(1);
    }
    return(-1);
  };
  catch {
    werror("Stdio.sendfile() failed!\n"
	   "%s\n",
	   describe_backtrace(err));
  };
  exit(1);
}
