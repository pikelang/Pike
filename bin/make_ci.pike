/*
 * $Id: make_ci.pike,v 1.2 1999/03/20 02:56:34 grubba Exp $
 *
 * Creates the file case_info.h
 *
 * Henrik Grubbström 1999-03-20
 */

#define CIM_NONE	0	/* Case-less */
#define CIM_UPPER	1	/* Upper-case, lower-case in data */
#define CIM_LOWER	2	/* Lower-case, upper-case in data */
#define CIM_CASEBIT	3	/* Some case, case bit in data */
#define CIM_CASEBITOFF	4	/* Same as above, but also offset by data */

int main(int argc, array(string) argv)
{
  int lineno;
  array(array(int)) ci = ({({ 0, CIM_NONE, 0 })});

  string data = Stdio.stdin.read();

  foreach(data/"\r\n", string line) {
    lineno++;
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
    sscanf(info[0], "%04x", char);
    int mode = CIM_NONE;
    int d;
    if (sizeof(info[13])) {
      // Upper-case char
      mode = CIM_UPPER;
      sscanf(info[13], "%04x", d);
      int delta = d - char;
      if (!(delta & (delta - 1)) && (delta > 0)) {
	if (d & delta) {
	  mode = CIM_CASEBIT;
	} else {
	  mode = CIM_CASEBITOFF;
	}
	d = delta;
      }
    } else if (sizeof(info[14])) {
      // Lower-case char
      mode = CIM_LOWER;
      sscanf(info[14], "%04x", d);
      int delta = char - d;
      if (!(delta & (delta - 1)) && (delta > 0)) {
	if (char & delta) {
	  mode = CIM_CASEBIT;
	} else {
	  mode = CIM_CASEBITOFF;
	}
	d = delta;
      }
    }

    if ((ci[-1][1] != mode) || (ci[-1][2] != d)) {
      // New range.
      ci += ({({ char, mode, d })});
    }
  }

  write(sprintf("/*\n"
		" * Created by $Id: make_ci.pike,v 1.2 1999/03/20 02:56:34 grubba Exp $ on %s"
		" *\n"
		" * Table used for looking up the case of\n"
		" * Unicode characters.\n"
		" *\n"
		" * Henrik Grubbström 1999-03-20\n"
		" */\n\n", ctime(time())));

  foreach(ci, array(int) info) {
    write(sprintf("{ 0x%04x, %s, 0x%04x, },\n",
		  info[0],
		  ({ "CIM_NONE", "CIM_UPPER", "CIM_LOWER",
		     "CIM_CASEBIT", "CIM_CASEBITOFF" })[info[1]],
		  info[2]));
  }

  exit(0);
}
