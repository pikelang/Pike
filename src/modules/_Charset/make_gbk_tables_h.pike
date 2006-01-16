//
// $Id: make_gbk_tables_h.pike,v 1.1 2006/01/16 18:47:51 grubba Exp $
//
// Create lookup tables for GBK.
//
// 2006-01-16 Henrik Grubbström
//

import Parser.XML.Tree;

#if !constant(Parser.XML.Tree.SimpleNode)
// Old Pike 7.2 or older.
// Fall back to using fully linked nodes.
#define SimpleNode Node
#define simple_parse_input(X)	parse_input(X)
#endif /* !constant(Parser.XML.Tree.SimpleNode) */

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

  // Indexed by [ch[0]-0x81][ch[1]]
  array(array(int)) dec_table = allocate(126, allocate)(256);
  foreach(encodings; int u; string b) {
    if (!b) {
      continue;
    }
    array(string) a = b/" ";
    if (sizeof(a) != 2) continue;
    array(int) byte_pair = array_sscanf(b, "%x %x");
    if ((byte_pair[0] < 0x81) || (byte_pair[0] > 0xfe)) {
      werror("Unexpected byte pair: 0x%02x 0x%02x\n", @byte_pair);
      return 1;
    }
    if (dec_table[byte_pair[0] - 0x81][byte_pair[1]]) {
      werror("Multiple definitions for byte pair 0x%02x 0x%02x: "
	     "U+%06x and U+%06x\n",
	     @byte_pair,
	     dec_table[byte_pair[0] - 0x81][byte_pair[1]],
	     u);
      return 1;
    }
    dec_table[byte_pair[0] - 0x81][byte_pair[1]] = u;
  }
  array(int) dec_offsets = allocate(126);
  foreach(dec_table; int no; array(int) tab) {
    int i;
    while ((i < sizeof(tab)) && !tab[i]) i++;
    dec_offsets[no] = i;
    int j = sizeof(tab)-1;
    while ((j >= i) && !tab[j]) j--;
    dec_table[no] = tab[i..j];
  }
  int tab_no = 0x81;
  string code = sprintf("/* Generated automatically by\n"
			" * %s\n"
			" * Do not edit.\n"
			" */\n"
			"\n"
			"/* Multi-char decoding table for GBK/GB18030\n"
			" * generated from %s version %s.\n"
			" */\n"
			"%{%s\n%}"
			"static const struct multichar_table GBK[] = {\n"
			"%{%s\n%}"
			"};\n"
			"\n",
			"$Id: make_gbk_tables_h.pike,v 1.1 2006/01/16 18:47:51 grubba Exp $",
			chmap->get_attributes()->id || "UNKNOWN",
			chmap->get_attributes()->version || "UNKNOWN",
			map(dec_table,
			    lambda(array(int) tab) {
			      return sprintf("static const UNICHAR "
					     "map_GBK_%02X[] = {\n"
					     "%{ %s\n%}"
					     "};",
					     tab_no++,
					     map(tab/8.0,
						 lambda(array(int) chars) {
						   return
						     sprintf("%{ 0x%04x,%}",
							     chars);
						 }));
			    }),
			((tab_no = 0x81),
			 map(dec_offsets,
			     lambda(int off) {
			       return sprintf("  { %d, sizeof(map_GBK_%02X)/"
					      "sizeof(UNICHAR)+%d, "
					      "map_GBK_%02X },",
					      off, tab_no, off-1, tab_no++);
			     })));
  //write(code);
  Stdio.write_file(argv[2], code);
  return 0;
}