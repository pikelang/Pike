/*
 * Generate the JIS X 0213:2000 and JIS X 0213:2004
 * Unicode translation tables for Pike from
 *
 *   http://x0213.org/codetable/jisx0213-2004-std.txt
 *
 * 2011-04-22 Henrik Grubbström
 */

// These are the characters added by JIS X0213/AMMENDMENT-1:2004.
// All of them are in JIS X0213 plane 1.
constant jis_x0213_amd_1_2004 = ([
    0x2e21: 0x4ff1,
    0x2f7e: 0x525d,
    0x4f54: 0x20b9f,
    0x4f7e: 0x541e,
    0x7427: 0x5653,
    0x7e7a: 0x59f8,
    0x7e7b: 0x5c5b,
    0x7e7c: 0x5e77,
    0x7e7d: 0x7626,
    0x7e7e: 0x7e6b,
]);

// These are mapped to half-width, with the full-width in the comment
// in the jisx0213-2004-std.txt file (as of 2011-04-22).
// All of them are in JIS X0213 plane 1.
constant jis_0213_fullwidth = ([
  0x2124: 0xff0c, 0x2125: 0xff0e, 0x2127: 0xff1a, 0x2128: 0xff1b,
  0x2129: 0xff1f, 0x212a: 0xff01, 0x212e: 0xff40, 0x2130: 0xff3e,
  0x2132: 0xff3f, 0x213f: 0xff0f, 0x2140: 0xff3c, 0x2143: 0xff5c,
  0x214a: 0xff08, 0x214b: 0xff09, 0x214e: 0xff3b, 0x214f: 0xff3d,
  0x2150: 0xff5b, 0x2151: 0xff5d, 0x215c: 0xff0b, 0x2161: 0xff1d,
  0x2163: 0xff1c, 0x2164: 0xff1e, 0x2170: 0xff04, 0x2173: 0xff05,
  0x2174: 0xff03, 0x2175: 0xff06, 0x2176: 0xff0a, 0x2177: 0xff20,
  0x222f: 0xff07, 0x2230: 0xff02, 0x2231: 0xff0d, 0x2232: 0xff5e,
  0x2330: 0xff10, 0x2331: 0xff11, 0x2332: 0xff12, 0x2333: 0xff13,
  0x2334: 0xff14, 0x2335: 0xff15, 0x2336: 0xff16, 0x2337: 0xff17,
  0x2338: 0xff18, 0x2339: 0xff19, 0x2341: 0xff21, 0x2342: 0xff22,
  0x2343: 0xff23, 0x2344: 0xff24, 0x2345: 0xff25, 0x2346: 0xff26,
  0x2347: 0xff27, 0x2348: 0xff28, 0x2349: 0xff29, 0x234a: 0xff2a,
  0x234b: 0xff2b, 0x234c: 0xff2c, 0x234d: 0xff2d, 0x234e: 0xff2e,
  0x234f: 0xff2f, 0x2350: 0xff30, 0x2351: 0xff31, 0x2352: 0xff32,
  0x2353: 0xff33, 0x2354: 0xff34, 0x2355: 0xff35, 0x2356: 0xff36,
  0x2357: 0xff37, 0x2358: 0xff38, 0x2359: 0xff39, 0x235a: 0xff3a,
  0x2361: 0xff41, 0x2362: 0xff42, 0x2363: 0xff43, 0x2364: 0xff44,
  0x2365: 0xff45, 0x2366: 0xff46, 0x2367: 0xff47, 0x2368: 0xff48,
  0x2369: 0xff49, 0x236a: 0xff4a, 0x236b: 0xff4b, 0x236c: 0xff4c,
  0x236d: 0xff4d, 0x236e: 0xff4e, 0x236f: 0xff4f, 0x2370: 0xff50,
  0x2371: 0xff51, 0x2372: 0xff52, 0x2373: 0xff53, 0x2374: 0xff54,
  0x2375: 0xff55, 0x2376: 0xff56, 0x2377: 0xff57, 0x2378: 0xff58,
  0x2379: 0xff59, 0x237a: 0xff5a,
]);

int convert_to_9494(int jis)
{
  return ((jis >> 8) - 0x21)*94 + (jis & 0xff) - 0x21;
}

// Extra combining characters.
array(mapping(int:int)) combiners = ({ ([]), ([]) });

array(array(int)) parse_jisx_file(string path)
{
  Stdio.File f = Stdio.File();
  if (!f->open(path, "r")) return UNDEFINED;

  array(array(int)) result = allocate(2, allocate)(94*94, 0xfffd);
  foreach(f->line_iterator(1);;string line) {
    line = (line/"#")[0];
    array(string) fields = map(line/"\t", String.trim_all_whites) - ({ "" });
    if (sizeof(fields) < 2) continue;
    int plane;
    if (has_prefix(fields[0], "3-")) {
      plane = 0;
    } else if (has_prefix(fields[0], "4-")) {
      plane = 1;
    } else {
      werror("Unsupported JIS prefix: %O\n", fields[0][..1]);
      exit(2);
    }
    sscanf(fields[0][2..], "%0x", int jis);
    int ucs = !plane && jis_0213_fullwidth[jis];
    int combiner;
    if (!ucs) {
      if (!has_prefix(fields[1], "U+")) {
	werror("Unsupported Unicode prefix: %O\n", fields[1][..1]);
	exit(3);
      }
      sscanf(fields[1][2..], "%0x+%0x", ucs, combiner);
    }
    int pike_9494 = convert_to_9494(jis);

    result[plane][pike_9494] = ucs;
    if (combiner) {
      combiners[plane][pike_9494] = combiner;
    }
  }
  return result;
}

void generate_table(Stdio.File out, string name,
		    array(int) table9494, int|void plane)
{
  array(int) unistrs = ({});
  out->write("const UNICHAR map_%s[] = {", name);
  foreach(table9494; int i; int ucs) {
    if (!(i & 0x7)) out->write("\n ");
    if (ucs > 0xffff) {
      // Convert to utf16.
      ucs -= 0x00010000;
      unistrs += ({ 0xd800 | (ucs >> 10),
		    0xdc00 | (ucs & 0x3ff),
		    0x0000 });
      ucs = 0xd800 + sizeof(unistrs) - 3;
    } else if (combiners[plane][i]) {
      unistrs += ({ ucs, combiners[plane][i], 0x0000 });
      ucs = 0xd800 + sizeof(unistrs) - 3;
    }
    out->write(" 0x%04x,", ucs);
  }
  if (sizeof(unistrs)) {
    out->write("\n");
    foreach(unistrs; int i; int ucs) {
      if (!(i & 0x7)) out->write("\n ");
      out->write(" 0x%04x,", ucs);
    }
  }
  out->write(" };\n");
}

int main(int argc, array(string) argv)
{
  if (sizeof(argv) != 3) {
    werror("Bad number of arguments.\n");
    exit(5);
  }

  array(array(int)) jisx0213_2004 = parse_jisx_file(argv[1]);

  if (!jisx0213_2004) {
    werror("No jisx0213-2004-std.txt file found.\n");
    exit(1);
  }

  Stdio.File out = Stdio.File();
  if (!out->open(argv[2], "wct")) {
    werror("Failed to open output file %O: %s(%d)\n",
	   argv[2], strerror(out->errno()), out->errno());
    exit(4);
  }

  // Generate the tables.
  generate_table(out, "JIS_X0213_2004_1", jisx0213_2004[0]);
  foreach(jis_x0213_amd_1_2004; int jis_char;) {
    jisx0213_2004[0][convert_to_9494(jis_char)] = 0xfffd;
  }
  generate_table(out, "JIS_X0213_2000_1", jisx0213_2004[0]);
  generate_table(out, "JIS_X0213_2000_2", jisx0213_2004[1], 1);
  out->close();
  return 0;
}
