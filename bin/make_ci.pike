#! /usr/bin/env pike

/*
 * $Id: make_ci.pike,v 1.11 2004/04/12 01:11:15 nilsson Exp $
 *
 * Creates the file case_info.h
 *
 * Henrik Grubbström 1999-03-20
 */

#pragma strict_types

#define CIM_NONE	0	/* Case-less */
#define CIM_UPPERDELTA	1	/* Upper-case, delta to lower-case in data */
#define CIM_LOWERDELTA	2	/* Lower-case, -delta to upper-case in data */
#define CIM_CASEBIT	3	/* Some case, case bit in data */
#define CIM_CASEBITOFF	4	/* Same as above, but also offset by data */

int main(int argc, array(string) argv)
{
  int lineno;
  array(array(int)) ci = ({({ 0, CIM_NONE, 0 })});
  int prevchar = 0;

  if (argc < 2 || argv[1]=="--help" ) {
    werror("Creates case info file by reading the unicode database from\n"
	   "stdin and outputs it to a file.\n"
	   "\n"
	   "Usage: make_ci.pike output_file.h\n");
    exit(1);
  }

  string data = Stdio.stdin.read();

  foreach(data/"\n", string line) {
    lineno++;
    line -= "\r";
    array(string) info = line/";";

    if (!sizeof(line)) continue;

    if (sizeof(info) != 15) {
      werror(sprintf("Syntax error on line %d: "
		     "Bad number of fields:%d (expected 15)\n"
		     "%O\n",
		     lineno, sizeof(info), line));
      exit(1);
    }
    int char;
    sscanf(info[0], "%x", char);
#if 1
    // Hardcoded in builtin_functions.c
    if(char && char<='z')
      continue;
#endif
    int mode = CIM_NONE;
    int d;
    if (sizeof(info[13])) {
      // Upper-case char
      mode = CIM_UPPERDELTA;
      sscanf(info[13], "%x", d);
      int delta = d - char;
      if (!(delta & (delta - 1)) && (delta > 0)) {
	if (d & delta) {
	  mode = CIM_CASEBIT;
	} else {
	  mode = CIM_CASEBITOFF;
	}
      }
      d = delta;
    } else if (sizeof(info[14])) {
      // Lower-case char
      mode = CIM_LOWERDELTA;
      sscanf(info[14], "%x", d);
      int delta = char - d;
      if (!(delta & (delta - 1)) && (delta > 0)) {
	if (char & delta) {
	  mode = CIM_CASEBIT;
	} else {
	  mode = CIM_CASEBITOFF;
	}
      }
      d = delta;
    }

    if ((char > prevchar+1) && (ci[-1][1] != CIM_NONE)) {
      // Add a NONE-range.
      ci += ({({ prevchar+1, CIM_NONE, 0 })});
    }

    if ((ci[-1][1] != mode) || (ci[-1][2] != d)) {
      // New range.
      ci += ({({ char, mode, d })});
    }

    prevchar = char;
  }

  array(string) table = allocate(sizeof(ci));

  for (int i = 0; i < sizeof (ci); i++) {
    array(int) info = ci[i];
    if ((info[2] <= -0x8000) || (info[2] > 0x7fff)) {
      error("Case information out of range for shorts: %d\n", info[2]);
    }
    table[i] =
      sprintf("{ 0x%06x, %s, %s0x%04x, },\n",
	      info[0],
	      ({ "CIM_NONE", "CIM_UPPERDELTA", "CIM_LOWERDELTA",
		 "CIM_CASEBIT", "CIM_CASEBITOFF" })[info[1]],
	      (info[2]<0)?"-":"",
	      (info[2]<0)?-info[2]:info[2]);
  }

  Stdio.File outfile = Stdio.File(argv[1], "wct");

  outfile->
    write(sprintf("/*\n"
		  " * Created by\n"
		  " * $Id: make_ci.pike,v 1.11 2004/04/12 01:11:15 nilsson Exp $\n"
		  " * on %s"
		  " *\n"
		  " * Table used for looking up the case of\n"
		  " * Unicode characters.\n"
		  " *\n"
		  " * Henrik Grubbström 1999-03-20\n"
		  " */\n\n", ctime(time())));

  map(table, outfile->write);

  for (lineno=0; lineno<sizeof(ci); lineno++)
    if (ci[lineno][0] > 0xff)
      break;

  outfile->write(sprintf("\n\n#define CASE_INFO_SHIFT0_HIGH 0x%04x\n",
			 lineno));

  exit(0);
}
