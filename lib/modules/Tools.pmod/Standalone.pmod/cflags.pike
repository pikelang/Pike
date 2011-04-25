#! /usr/bin/env pike
// -*- pike -*- $Id$
#pike __REAL_VERSION__

constant description = "CFLAGS for C module compilation";

int main(int argc, array(string) argv) {
  write("%s\n", master()->cflags||"");
}
