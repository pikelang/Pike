//
// $Id: make_gb18030_h.pike,v 1.4 2006/01/15 15:24:10 grubba Exp $
//
// Create lookup tables and code for GB18030.
//
// 2006-01-13 Henrik Grubbström
//

import Parser.XML.Tree;

#if !constant(Parser.XML.Tree.SimpleNode)
// Old Pike 7.2 or older.
// Fall back to using fully linked nodes.
#define SimpleNode Node
#define simple_parse_input(X)	parse_input(X)
#endif /* !constant(Parser.XML.Tree.SimpleNode) */

int decode_4bytes(array(string) hex_bytes)
{
  int i;
  int c;
  sscanf(hex_bytes[0], "%02x", c);
  i = c - 0x81;
  sscanf(hex_bytes[1], "%02x", c);
  i *= 10;
  i += c - 0x30;
  sscanf(hex_bytes[2], "%02x", c);
  i *= 126;
  i += c - 0x81;
  sscanf(hex_bytes[3], "%02x", c);
  i *= 10;
  return i + c - 0x30;
}

int main(int argc, array(string) argv)
{
  // First argument is the file retrieved from
  //   http://dev.icu-project.org/cgi-bin/viewcvs.cgi/*checkout*/charset/data/xml/gb-18030-2000.xml
  string xml_data = Stdio.read_bytes(argv[1]);

  SimpleNode root = simple_parse_input(xml_data);

  SimpleNode chmap = root->get_first_element("characterMapping");
  if (!chmap) {
    werror("No characterMapping!\n");
    return 1;
  }
  write("Analyzing character mapping tables for %s version %s...\n",
	chmap->get_attributes()->id || "UNKNOWN",
	chmap->get_attributes()->version || "UNKNOWN");

  array(SimpleNode) assignments = chmap->get_elements("assignments");
  if (!sizeof(assignments)) {
    werror("No character assignments defined!\n");
    return 1;
  }
  array(string) encodings = allocate(65536);
  foreach(assignments, SimpleNode ass_list) {
    foreach(ass_list->get_elements("a"), SimpleNode assignment) {
      mapping(string:string) attrs = assignment->get_attributes();
      if (!attrs->u || !attrs->b) {
	werror("a-node %O lacks u and/or b attribute(s)!\n",
	       assignment);
	return 1;
      }
      int u;
      if (!sscanf(attrs->u, "%04x", u)) {
	werror("Failed to parse u attribute (%O) for node %O.\n",
	       attrs->u, assignment);
	return 1;
      }
      if (encodings[u] && encodings[u] != attrs->b) {
	werror("Multiple encodings for U+%04x: %O != %O.\n",
	       u, encodings[u], attrs->b);
	return 1;
      }
      encodings[u] = attrs->b;
    }
  }

  // ulow, uhigh, index||bytes	Where either i = u - ulow + index,
  //                          	or byte_pair = bytes[(u-ulow)*2..(u-ulow)*2+1].
  array(array(int|array(int))) enc_table = ({
    ({ 0x80, 0x80, 0 }),
  });

  // index, offset	Where u = i - index + offset;
  array(array(int)) dec_table = ({
    ({ 0, 0x80 }),
  });
  foreach(encodings; int u; string b) {
    if (!b) {
      continue;
    }
    array(string) a = b/" ";
    if (sizeof(a) == 4) {
      int i = decode_4bytes(a);
      if (((i - dec_table[-1][0]) + dec_table[-1][1]) != u) {
	// New entry needed.
	dec_table += ({ ({ i, u }) });
      }
      if (!intp(enc_table[-1][2]) ||
	  ((u - enc_table[-1][0] + enc_table[-1][2]) != i)) {
	enc_table += ({ ({ u, u, i }) });
      } else {
	enc_table[-1][1] = u;
      }
    } else if (u >= 0x80) {
      array(int) byte_pair = array_sscanf(b, "%x %x");
      if (intp(enc_table[-1][2]) ||
	  enc_table[-1][1] != u-1) {
	enc_table += ({ ({ u, u, byte_pair }) });
      } else {
	enc_table[-1][1] = u;
	enc_table[-1][2] += byte_pair;
      }
      // FIXME: Generate GB2312/GBK here?
      continue;
    }
  }
#if 0
  werror("enc_table: ({\n"
	 "%{  0x%06x, 0x%06x, X\n%}"
	 "});\n",
	 enc_table);
#endif /* 0 */
  foreach(assignments, SimpleNode ass_list) {
    foreach(ass_list->get_elements("range"), SimpleNode range) {
      mapping(string:string) attrs = range->get_attributes();
      if (!attrs->uFirst || !attrs->bFirst || !attrs->bLast) {
	werror("range-node %O lacks uFirst and/or bFirst attribute(s)!\n",
	       range);
	return 1;
      }
      if (attrs->bMin != "81 30 81 30" || attrs->bMax != "FE 39 FE 39") {
	werror("Unexpected coding method for range %O\n"
	       "Attributes: %O\n",
	       range, attrs);
	return 1;
      }
      int uFirst;
      if (!sscanf(attrs->uFirst, "%x", uFirst)) {
	werror("Failed to parse uFirst attribute (%O) for node %O.\n",
	       attrs->uFirst, range);
	return 1;
      }
      int bFirst = decode_4bytes(attrs->bFirst/" ");
      int bLast = decode_4bytes(attrs->bLast/" ");
      int i;
      for(i = 0; i < sizeof(dec_table); i++) {
	if (dec_table[i][0] > bFirst) {
	  if (dec_table[i][0] < bLast) {
	    werror("Overlapping ranges!\n");
	    return 1;
	  }
	  break;
	}
      }
      dec_table = dec_table[..i-1] + ({ ({ bFirst, uFirst }) }) +
	dec_table[i..];
      int uLast;
      if (!sscanf(attrs->uLast, "%x", uLast)) {
	werror("Failed to parse uLast attribute (%O) for node %O.\n",
	       attrs->uLast, range);
	return 1;
      }
      for(i = 0; i < sizeof(enc_table); i++) {
	if (enc_table[i][0] > uFirst) {
	  if (enc_table[i][0] < uLast) {
	    werror("Overlapping ranges!\n"
		   "  enc_table.ulow: 0x%06x\n"
		   "  uLast:          0x%06x\n",
		   enc_table[i][0], uLast);
	    return 1;
	  }
	  if (enc_table[i-1][1] >= uFirst) {
	    werror("Overlapping ranges!\n"
		   "  enc_table.uhi: 0x%06x\n"
		   "  uFirst:        0x%06x\n",
		   enc_table[i-1][1], uFirst);
	    return 1;
	  }
	  break;
	}
      }
      enc_table = enc_table[..i-1] + ({ ({ uFirst, uLast, bFirst }) }) +
	enc_table[i..];
    }
  }

  int ind = 0;
  array(int) enc_bytes = map(enc_table,
			     lambda(array(int|array(int)) enc_info) {
			       array(int) res;
			       if (intp(res = enc_info[2])) return ({});
			       enc_info[2] = ~ind;
			       ind += sizeof(res);
			       return res;
			     }) * ({});
  //werror("enc_bytes: %O\n", enc_bytes);
  string code = sprintf("/* Generated automatically by\n"
			" * %s\n"
			" * Do not edit.\n"
			" */\n"
			"\n"
			"/* Decoding table for %s version %s. */\n"
			"static const struct gb18030_info {\n"
			"  p_wchar2 index;\n"
			"  p_wchar2 ucode;\n"
			"} gb18030_info[] = {\n"
			"%{  { 0x%06x, 0x%06x, },\n%}"
			"  /* Sentinel */\n"
			"  { 0x7fffffff, 0 },\n"
			"};\n"
			"\n"
			"#define NUM_GB18030_INFO %d\n"
			"\n"
			"static p_wchar2 gb18030_to_unicode(p_wchar2 i)\n"
			"{\n"
			"  static int last_j;\n"
#if 0
			"  fprintf(stderr, \"gb18030_to_unicode(0x%%06x)\",\n"
			"          i);\n"
#endif /* 0 */
			"  if (gb18030_info[last_j].index > i) {\n"
			"    int jlo = 0, jhi = last_j, jmid;\n"
			"    while (jlo < (jmid = (jlo + jhi)/2)) {\n"
			"      if (gb18030_info[jmid].index <= i) {\n"
			"        jlo = jmid;\n"
			"      } else {\n"
			"        jhi = jmid;\n"
			"      }\n"
			"    }\n"
			"    last_j = jlo;\n"
			"  } else if (gb18030_info[last_j+1].index <= i) {\n"
			"    int jlo = last_j + 1, jhi = NUM_GB18030_INFO;\n"
			"    int jmid;\n"
			"    while (jlo < (jmid = (jlo + jhi)/2)) {\n"
			"      if (gb18030_info[jmid].index <= i) {\n"
			"        jlo = jmid;\n"
			"      } else {\n"
			"        jhi = jmid;\n"
			"      }\n"
			"    }\n"
			"    last_j = jlo;\n"
			"  }\n"
			"#ifdef PIKE_DEBUG\n"
			"  if (i < gb18030_info[last_j].index) {\n"
			"    Pike_fatal(\"GB18030: Bad index: \"\n"
			"               \"0x%%06x > 0x%%06x (j:%%d)\\n\",\n"
			"               gb18030_info[last_j].index, i,\n"
			"               last_j);\n"
			"  } else if (i >= gb18030_info[last_j+1].index) {\n"
			"    Pike_fatal(\"GB18030: Bad index: \"\n"
			"               \"0x%%06x <= 0x%%06x (j:%%d)\\n\",\n"
			"               gb18030_info[last_j+1].index, i,\n"
			"               last_j);\n"
			"  }\n"
			"#endif /* PIKE_DEBUG */\n"
#if 0
			"  fprintf(stderr, \" j: %%d, index: 0x%%06x,\"\n"
			"          \" ucode: 0x%%06x\\n\",\n"
			"          last_j, gb18030_info[last_j].index,\n"
			"          gb18030_info[last_j].ucode);\n"
#endif /* 0 */
			"  return i - gb18030_info[last_j].index +\n"
			"    gb18030_info[last_j].ucode;\n"
			"}\n"
			"\n"
			"/* Encoding tables for %s version %s. */\n"
			"static const p_wchar0 gb18030e_bytes[] = {\n"
			"%s\n"
			"};\n"
			"\n"
			"static const struct gb18030e_info {\n"
			"  p_wchar2 ulow;\n"
			"  p_wchar2 uhigh;\n"
			"  INT32 index;\n"
			"    /* If positive: gb18030 index.\n"
			"     * If negative: ~index in gb18030e_bytes\n"
			"     */\n"
			"} gb18030e_info[] = {\n"
			"%{  { 0x%06x, 0x%06x, %d },\n%}"
			"  /* Sentinel */\n"
			"  { 0x7fffffff, 0x7fffffff, 0 },\n"
			"};\n"
			"\n"
			"#define NUM_GB18030E_INFO %d\n"
			"\n"
			"static const struct gb18030e_info * const\n"
			"  get_gb18030e_info(p_wchar2 u)\n"
			"{\n"
			"  static int last_j;\n"
			"\n"
			"  if (u < 0x80) return NULL;\n"
			"\n"
#if 0
			"  fprintf(stderr, \"get_gb18030e_info(0x%%06x)\",\n"
			"          u);\n"
#endif /* 0 */
			"  if (gb18030e_info[last_j].ulow > u) {\n"
			"    int jlo = 0, jhi = last_j, jmid;\n"
			"    while (jlo < (jmid = (jlo + jhi)/2)) {\n"
			"      if (gb18030e_info[jmid].ulow <= u) {\n"
			"        jlo = jmid;\n"
			"      } else {\n"
			"        jhi = jmid;\n"
			"      }\n"
			"    }\n"
			"    last_j = jlo;\n"
			"  } else if (gb18030e_info[last_j+1].ulow <= u) {\n"
			"    int jlo = last_j + 1, jhi = NUM_GB18030E_INFO;\n"
			"    int jmid;\n"
			"    while (jlo < (jmid = (jlo + jhi)/2)) {\n"
			"      if (gb18030e_info[jmid].ulow <= u) {\n"
			"        jlo = jmid;\n"
			"      } else {\n"
			"        jhi = jmid;\n"
			"      }\n"
			"    }\n"
			"    last_j = jlo;\n"
			"  }\n"
			"#ifdef PIKE_DEBUG\n"
			"  if (u < gb18030e_info[last_j].ulow) {\n"
			"    Pike_fatal(\"GB18030: Bad ulow: \"\n"
			"               \"0x%%06x > 0x%%06x (j:%%d)\\n\",\n"
			"               gb18030e_info[last_j].ulow, u,\n"
			"               last_j);\n"
			"  } else if (u >= gb18030e_info[last_j+1].ulow) {\n"
			"    Pike_fatal(\"GB18030: Bad ulow: \"\n"
			"               \"0x%%06x <= 0x%%06x (j:%%d)\\n\",\n"
			"               gb18030e_info[last_j+1].ulow, u,\n"
			"               last_j);\n"
			"  }\n"
			"#endif /* PIKE_DEBUG */\n"
#if 0
			"  fprintf(stderr, \" j: %%d, ulow: 0x%%06x,\"\n"
			"          \" uhigh: 0x%06x index: %d\\n\",\n"
			"          last_j, gb18030e_info[last_j].ulow,\n"
			"          gb18030e_info[last_j].uhigh,\n"
			"          gb18030e_info[last_j].index);\n"
#endif /* 0 */
			"  if (gb18030e_info[last_j].uhigh < u) return NULL;\n"
			"  return gb18030e_info + last_j;\n"
			"}\n"
			"\n",
			"$Id: make_gb18030_h.pike,v 1.4 2006/01/15 15:24:10 grubba Exp $",
			chmap->get_attributes()->id || "UNKNOWN",
			chmap->get_attributes()->version || "UNKNOWN",
			dec_table,
			sizeof(dec_table),

			chmap->get_attributes()->id || "UNKNOWN",
			chmap->get_attributes()->version || "UNKNOWN",
			map(enc_bytes/8.0,
			    lambda(array(int) bytes) {
			      return sprintf(" %{ 0x%02x,%}\n", bytes);
			    }) * "",
			enc_table,
			sizeof(enc_table));
  //write(code);
  Stdio.write_file(argv[2], code);
  return 0;
}