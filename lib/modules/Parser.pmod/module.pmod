//
// $Id$

#pike __REAL_VERSION__

// inherit Parser._parser;

constant HTML = Parser._parser.HTML;
constant _RCS = Parser._parser._RCS;

static int(0..0) return_zero() {return 0;}
static HTML xml_parser =
  lambda() {
    HTML p = HTML();
    p->lazy_entity_end (1);
    p->match_tag (0);
    p->xml_tag_syntax (3);
    p->add_quote_tag ("!--", return_zero, "--");
    p->add_quote_tag ("![CDATA[", return_zero, "]]");
    p->add_quote_tag ("?", return_zero, "?");
    return p;
  }();

//! Returns a @[Parser.HTML] initialized for parsing XML. It has all
//! the flags set properly for XML syntax and callbacks to ignore
//! comments, CDATA blocks and unknown PI tags, but it has no
//! registered tags and doesn't decode any entities.
HTML get_xml_parser()
{
  return xml_parser->clone();
}

constant html_entities=
([
  // Basic Latin
   "quot":"\x22",
   "amp":"\x26",
   "apos":"\x27",
   "lt":"\x3C",
   "gt":"\x3E",

   // Latin Extended-A
   "OElig":"\x152",
   "oelig":"\x153",
   "Scaron":"\x160",
   "scaron":"\x161",
   "Yuml":"\x178",

   // Spacing Modifier Letters
   "circ":"\x2C6",
   "tilde":"\x2DC",

   // General Punctuation
   "ensp":"\x2002",
   "emsp":"\x2003",
   "thinsp":"\x2009",
   "zwnj":"\x200C",
   "zwj":"\x200D",
   "lrm":"\x200E",
   "rlm":"\x200F",
   "ndash":"\x2013",
   "mdash":"\x2014",
   "lsquo":"\x2018",
   "rsquo":"\x2019",
   "sbquo":"\x201A",
   "ldquo":"\x201C",
   "rdquo":"\x201D",
   "bdquo":"\x201E",
   "dagger":"\x2020",
   "Dagger":"\x2021",
   "permil":"\x2030",
   "lsaquo":"\x2039",
   "rsaquo":"\x203A",
   "euro":"\x20AC",

   // Latin 1
   "nbsp":"\xA0",
   "iexcl":"\xA1",
   "cent":"\xA2",
   "pound":"\xA3",
   "curren":"\xA4",
   "yen":"\xA5",
   "brvbar":"\xA6",
   "sect":"\xA7",
   "uml":"\xA8",
   "copy":"\xA9",
   "ordf":"\xAA",
   "laquo":"\xAB",
   "not":"\xAC",
   "shy":"\xAD",
   "reg":"\xAE",
   "macr":"\xAF",
   "deg":"\xB0",
   "plusmn":"\xB1",
   "sup2":"\xB2",
   "sup3":"\xB3",
   "acute":"\xB4",
   "micro":"\xB5",
   "para":"\xB6",
   "middot":"\xB7",
   "cedil":"\xB8",
   "sup1":"\xB9",
   "ordm":"\xBA",
   "raquo":"\xBB",
   "frac14":"\xBC",
   "frac12":"\xBD",
   "frac34":"\xBE",
   "iquest":"\xBF",
   "Agrave":"\xC0",
   "Aacute":"\xC1",
   "Acirc":"\xC2",
   "Atilde":"\xC3",
   "Auml":"\xC4",
   "Aring":"\xC5",
   "AElig":"\xC6",
   "Ccedil":"\xC7",
   "Egrave":"\xC8",
   "Eacute":"\xC9",
   "Ecirc":"\xCA",
   "Euml":"\xCB",
   "Igrave":"\xCC",
   "Iacute":"\xCD",
   "Icirc":"\xCE",
   "Iuml":"\xCF",
   "ETH":"\xD0",
   "Ntilde":"\xD1",
   "Ograve":"\xD2",
   "Oacute":"\xD3",
   "Ocirc":"\xD4",
   "Otilde":"\xD5",
   "Ouml":"\xD6",
   "times":"\xD7",
   "Oslash":"\xD8",
   "Ugrave":"\xD9",
   "Uacute":"\xDA",
   "Ucirc":"\xDB",
   "Uuml":"\xDC",
   "Yacute":"\xDD",
   "THORN":"\xDE",
   "szlig":"\xDF",
   "agrave":"\xE0",
   "aacute":"\xE1",
   "acirc":"\xE2",
   "atilde":"\xE3",
   "auml":"\xE4",
   "aring":"\xE5",
   "aelig":"\xE6",
   "ccedil":"\xE7",
   "egrave":"\xE8",
   "eacute":"\xE9",
   "ecirc":"\xEA",
   "euml":"\xEB",
   "igrave":"\xEC",
   "iacute":"\xED",
   "icirc":"\xEE",
   "iuml":"\xEF",
   "eth":"\xF0",
   "ntilde":"\xF1",
   "ograve":"\xF2",
   "oacute":"\xF3",
   "ocirc":"\xF4",
   "otilde":"\xF5",
   "ouml":"\xF6",
   "divide":"\xF7",
   "oslash":"\xF8",
   "ugrave":"\xF9",
   "uacute":"\xFA",
   "ucirc":"\xFB",
   "uuml":"\xFC",
   "yacute":"\xFD",
   "thorn":"\xFE",
   "yuml":"\xFF",

   // Latin Extended-B
   "fnof":"\x192",

   // Greek
   "Alpha":"\x391",
   "Beta":"\x392",
   "Gamma":"\x393",
   "Delta":"\x394",
   "Epsilon":"\x395",
   "Zeta":"\x396",
   "Eta":"\x397",
   "Theta":"\x398",
   "Iota":"\x399",
   "Kappa":"\x39A",
   "Lambda":"\x39B",
   "Mu":"\x39C",
   "Nu":"\x39D",
   "Xi":"\x39E",
   "Omicron":"\x39F",
   "Pi":"\x3A0",
   "Rho":"\x3A1",
   "Sigma":"\x3A3",
   "Tau":"\x3A4",
   "Upsilon":"\x3A5",
   "Phi":"\x3A6",
   "Chi":"\x3A7",
   "Psi":"\x3A8",
   "Omega":"\x3A9",
   "alpha":"\x3B1",
   "beta":"\x3B2",
   "gamma":"\x3B3",
   "delta":"\x3B4",
   "epsilon":"\x3B5",
   "zeta":"\x3B6",
   "eta":"\x3B7",
   "theta":"\x3B8",
   "iota":"\x3B9",
   "kappa":"\x3BA",
   "lambda":"\x3BB",
   "mu":"\x3BC",
   "nu":"\x3BD",
   "xi":"\x3BE",
   "omicron":"\x3BF",
   "pi":"\x3C0",
   "rho":"\x3C1",
   "sigmaf":"\x3C2",
   "sigma":"\x3C3",
   "tau":"\x3C4",
   "upsilon":"\x3C5",
   "phi":"\x3C6",
   "chi":"\x3C7",
   "psi":"\x3C8",
   "omega":"\x3C9",
   "thetasym":"\x3D1",
   "upsih":"\x3D2",
   "piv":"\x3D6",

   // General Punctuation
   "bull":"\x2022",
   "hellip":"\x2026",
   "prime":"\x2032",
   "Prime":"\x2033",
   "oline":"\x203E",
   "frasl":"\x2044",

   // Letterlike symbols
   "weierp":"\x2118",
   "image":"\x2111",
   "real":"\x211C",
   "trade":"\x2122",
   "alefsym":"\x2135",

   // Arrows
   "larr":"\x2190",
   "uarr":"\x2191",
   "rarr":"\x2192",
   "darr":"\x2193",
   "harr":"\x2194",
   "crarr":"\x21B5",
   "lArr":"\x21D0",
   "uArr":"\x21D1",
   "rArr":"\x21D2",
   "dArr":"\x21D3",
   "hArr":"\x21D4",

   // Mathematical Operators
   "forall":"\x2200",
   "part":"\x2202",
   "exist":"\x2203",
   "empty":"\x2205",
   "nabla":"\x2207",
   "isin":"\x2208",
   "notin":"\x2209",
   "ni":"\x220B",
   "prod":"\x220F",
   "sum":"\x2211",
   "minus":"\x2212",
   "lowast":"\x2217",
   "radic":"\x221A",
   "prop":"\x221D",
   "infin":"\x221E",
   "ang":"\x2220",
   "and":"\x2227",
   "or":"\x2228",
   "cap":"\x2229",
   "cup":"\x222A",
   "int":"\x222B",
   "there4":"\x2234",
   "sim":"\x223C",
   "cong":"\x2245",
   "asymp":"\x2248",
   "ne":"\x2260",
   "equiv":"\x2261",
   "le":"\x2264",
   "ge":"\x2265",
   "sub":"\x2282",
   "sup":"\x2283",
   "nsub":"\x2284",
   "sube":"\x2286",
   "supe":"\x2287",
   "oplus":"\x2295",
   "otimes":"\x2297",
   "perp":"\x22A5",
   "sdot":"\x22C5",

   // Misecellaneous Technical
   "lceil":"\x2308",
   "rceil":"\x2309",
   "lfloor":"\x230A",
   "rfloor":"\x230B",
   "lang":"\x2329",
   "rang":"\x232A",

   // Geometric Shapes
   "loz":"\x25CA",

   // Miscellaneous Symbols
   "spades":"\x2660",
   "clubs":"\x2663",
   "hearts":"\x2665",
   "diams":"\x2666",
]);

string decode_numeric_xml_entity (string chref)
//! Decodes the numeric XML entity @[chref], e.g. @tt{"&#x34;"@} and
//! returns the character as a string. @[chref] is the name part of
//! the entity, i.e. without the leading '&' and trailing ';'. Returns
//! zero if @[chref] isn't on a recognized form or if the character
//! number is too large to be represented in a string.
{
  if (sizeof (chref) && chref[0] == '#')
    if ((<"#x", "#X">)[chref[..1]]) {
      if (sscanf (chref, "%*2s%x%*c", int c) == 2)
	// A cast gives a proper error if the integer is too large;
	// sprintf("%c", c) can (currently) wrap and produce negative
	// character values.
	catch {return (string) ({c});};
    }
    else
      if (sscanf (chref, "%*c%d%*c", int c) == 2)
	catch {return (string) ({c});};
  return 0;
}

//! @decl HTML html_entity_parser()
//! @decl string parse_html_entities(string in)
//! @decl HTML html_entity_parser(int noerror)
//! @decl string parse_html_entities(string in,int noerror)
//!	Parse any HTML entities in the string to unicode characters.
//!	Either return a complete parser (to build on or use) or parse
//!	a string. Throw an error if there is an unrecognized entity in
//!	the string if noerror is not set.
//! @note
//!	Currently using XHTML 1.0 tables.

static HTML entityparser =
  lambda () {
    HTML p=HTML();
    p->add_entities (html_entities);

    p->_set_entity_callback(
      lambda(HTML p,string ent)
      {
	string chr = decode_numeric_xml_entity (p->tag_name());
	if (!chr)
	  error ("Cannot decode character entity reference %O.\n", p->current());
	return ({chr});
      });

    p->lazy_entity_end(1);

    return p;
  }();

static HTML entityparser_noerror =
  lambda () {
     HTML p=entityparser->clone();

     p->_set_entity_callback(
	lambda(HTML p,string ent)
	{
	   string chr = decode_numeric_xml_entity (p->tag_name());
	   if (!chr)
	      return 0;
	   return ({chr});
	});

    return p;
  }();

HTML html_entity_parser(void|int noerror)
{
   return (noerror?entityparser_noerror:entityparser)->clone();
}

string parse_html_entities(string in,void|int noerror)
{
   return html_entity_parser(noerror)->finish(in)->read();
}

static mapping(string:string) rev_html_entities;

string encode_html_entities(string raw)
{
  if (!rev_html_entities) {
    rev_html_entities = mkmapping(values(html_entities)[*][0],
				  ("&"+indices(html_entities)[*])[*]+";");
  }
  string res = "";
  while (sizeof(raw)) {
    string tmp;
    int c;
    if (sscanf(raw, "%[!#-%(-;=?-~\241-\377]%c%s", tmp, c, raw) > 1) {
      string enc;
      if (!(enc = rev_html_entities[c])) {
	enc = sprintf("&#%d;", c);
      }
      res += tmp + enc;
    } else {
      return res + raw;
    }
  }
  return res;
}
