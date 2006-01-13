//
// $Id: make_gb18030_h.pike,v 1.1 2006/01/13 19:56:33 grubba Exp $
//
// Create lookup tables and code for GB18030.
//
// 2006-01-13 Henrik Grubbström
//

import Parser.XML.Tree;

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

  // FIXME: For future use.
  array(array(int)) enc_table = ({});

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
    } else {
      // FIXME: Generate GB2312/GBK here?
      continue;
    }
  }
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
    }
  }
  string code = sprintf("/* Generated automaticlly by\n"
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
			"  if (gb18030_info[last_j].index > i) {\n"
			"    int jlo = 0, jhi = last_j;\n"
			"    while (jlo + 1 < jhi) {\n"
			"      int jmid = (jlo + jhi)/2;\n"
			"      if (gb18030_info[jmid].index <= i) {\n"
			"        jlo = jmid;\n"
			"      } else {\n"
			"        jhi = jmid;\n"
			"      }\n"
			"    }\n"
			"    last_j = jlo;\n"
			"  } else if (gb18030_info[last_j+1].index < i) {\n"
			"    int jlo = last_j + 1, jhi = NUM_GB18030_INFO;\n"
			"    while (jlo + 1 < jhi) {\n"
			"      int jmid = (jlo + jhi)/2;\n"
			"      if (gb18030_info[jmid].index <= i) {\n"
			"        jlo = jmid;\n"
			"      } else {\n"
			"        jhi = jmid;\n"
			"      }\n"
			"    }\n"
			"    last_j = jlo;\n"
			"  }\n"
			"  return i - gb18030_info[last_j].index +\n"
			"    gb18030_info[last_j].ucode;\n"
			"}\n"
			"\n",
			"$Id: make_gb18030_h.pike,v 1.1 2006/01/13 19:56:33 grubba Exp $",
			chmap->get_attributes()->id || "UNKNOWN",
			chmap->get_attributes()->version || "UNKNOWN",
			dec_table,
			sizeof(dec_table)+1);
  //write(code);
  Stdio.write_file(argv[2], code);
  return 0;
}