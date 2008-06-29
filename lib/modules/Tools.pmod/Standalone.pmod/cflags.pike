#! /usr/bin/env pike
// -*- pike -*- $Id: cflags.pike,v 1.1 2008/06/29 11:20:05 agehall Exp $
#pike __REAL_VERSION__

constant description = "CFLAGS for C module compilation";

int main(int argc, array(string) argv) {
  write("%s\n", master()->cflags||"");
}
