// Filter for text/html
// Copyright © 2000,2001 Roxen IS.

inherit Search.Filter.Base;

constant contenttypes = ({ "text/html" });

string _sprintf()
{
  return "Search.Filter.HTML";
}

static string fix_entities(string text)
{
  return replace(text,
		 replace_entities,
		 replace_values);
}

Output filter(Standards.URI uri, string|Stdio.File data,
	      string content_type)
{
  Output res=Output();

  if(objectp(data))
    data=data->read();

  String.Buffer databuf=String.Buffer();
  Parser.HTML parser = Parser.HTML();

  parser->case_insensitive_tag(1);

  parser->add_container("title",
			lambda(string t, mapping m, string c)
			{
			  res->fields->title=fix_entities(c);
			});

  parser->add_tag("meta",
		  lambda(string t, mapping m)
		  {
		    switch(m->name)
		    {
		    case "description": 
		      res->fields->description=fix_entities(m->contents||"");
		      break;
		    case "keywords":
		      res->fields->keywords=fix_entities(m->contents||"");
		      break;
		    }
		  });

  parser->add_container("A",
			lambda(string t, mapping m, string c)
			{
			  if( m->href )
			    res->links+=({fix_entities(m->href)});
			});
  
  parser->_set_data_callback(lambda(object p, string data)
			     {
			       databuf->add(data);
			     });

   array(string) ignore_tags=({"noindex","script","style","no-index",});

    foreach(ignore_tags, string ignore_tag)
      parser->add_container(ignore_tag, "");
    
  res->fields->title="";
  res->fields->description="";
  res->fields->keywords="";

  int h = gethrtime();
  parser->feed(data);
  parser->finish();
  werror("parse: %dms\n", (gethrtime()-h)/1000 );

  h = gethrtime();
  res->fields->body=fix_entities(databuf->get());
  werror("fix_entities: %dms\n", (gethrtime()-h)/1000 );

  h = gethrtime();
  res->fix_relative_links(uri);
  werror("fix_relative_links: %dms\n", (gethrtime()-h)/1000 );
  return res;
}

mapping num =
  mkmapping( map(Array.enumerate(255),
		 lambda(int in){ return "&#"+in+";"; }),
	     map(Array.enumerate(255),
		 lambda(int in){ return sprintf("%c", in); }) ) +
  mkmapping( map(Array.enumerate(255),
		 lambda(int in){ return sprintf("&#x%x;", in); }),
	     map(Array.enumerate(255),
		 lambda(int in){ return sprintf("%c", in); }) ) +
  mkmapping( map(Array.enumerate(255),
		 lambda(int in){ return sprintf("&#x%X;", in); }),
	     map(Array.enumerate(255),
		 lambda(int in){ return sprintf("%c", in); }) );

constant iso88591
=([ "&nbsp;":   " ",
    "&iexcl;":  "¡",
    "&cent;":   "¢",
    "&pound;":  "£",
    "&curren;": "¤",
    "&yen;":    "¥",
    "&brvbar;": "¦",
    "&sect;":   "§",
    "&uml;":    "¨",
    "&copy;":   "©",
    "&ordf;":   "ª",
    "&laquo;":  "«",
    "&not;":    "¬",
    "&shy;":    "­",
    "&reg;":    "®",
    "&macr;":   "¯",
    "&deg;":    "°",
    "&plusmn;": "±",
    "&sup2;":   "²",
    "&sup3;":   "³",
    "&acute;":  "´",
    "&micro;":  "µ",
    "&para;":   "¶",
    "&middot;": "·",
    "&cedil;":  "¸",
    "&sup1;":   "¹",
    "&ordm;":   "º",
    "&raquo;":  "»",
    "&frac14;": "¼",
    "&frac12;": "½",
    "&frac34;": "¾",
    "&iquest;": "¿",
    "&Agrave;": "À",
    "&Aacute;": "Á",
    "&Acirc;":  "Â",
    "&Atilde;": "Ã",
    "&Auml;":   "Ä",
    "&Aring;":  "Å",
    "&AElig;":  "Æ",
    "&Ccedil;": "Ç",
    "&Egrave;": "È",
    "&Eacute;": "É",
    "&Ecirc;":  "Ê",
    "&Euml;":   "Ë",
    "&Igrave;": "Ì",
    "&Iacute;": "Í",
    "&Icirc;":  "Î",
    "&Iuml;":   "Ï",
    "&ETH;":    "Ð",
    "&Ntilde;": "Ñ",
    "&Ograve;": "Ò",
    "&Oacute;": "Ó",
    "&Ocirc;":  "Ô",
    "&Otilde;": "Õ",
    "&Ouml;":   "Ö",
    "&times;":  "×",
    "&Oslash;": "Ø",
    "&Ugrave;": "Ù",
    "&Uacute;": "Ú",
    "&Ucirc;":  "Û",
    "&Uuml;":   "Ü",
    "&Yacute;": "Ý",
    "&THORN;":  "Þ",
    "&szlig;":  "ß",
    "&agrave;": "à",
    "&aacute;": "á",
    "&acirc;":  "â",
    "&atilde;": "ã",
    "&auml;":   "ä",
    "&aring;":  "å",
    "&aelig;":  "æ",
    "&ccedil;": "ç",
    "&egrave;": "è",
    "&eacute;": "é",
    "&ecirc;":  "ê",
    "&euml;":   "ë",
    "&igrave;": "ì",
    "&iacute;": "í",
    "&icirc;":  "î",
    "&iuml;":   "ï",
    "&eth;":    "ð",
    "&ntilde;": "ñ",
    "&ograve;": "ò",
    "&oacute;": "ó",
    "&ocirc;":  "ô",
    "&otilde;": "õ",
    "&ouml;":   "ö",
    "&divide;": "÷",
    "&oslash;": "ø",
    "&ugrave;": "ù",
    "&uacute;": "ú",
    "&ucirc;":  "û",
    "&uuml;":   "ü",
    "&yacute;": "ý",
    "&thorn;":  "þ",
    "&yuml;":   "ÿ",
]);

constant international
=([ "&OElig;":  "\x0152",
    "&oelig;":  "\x0153",
    "&Scaron;": "\x0160",
    "&scaron;": "\x0161",
    "&Yuml;":   "\x0178",
    "&circ;":   "\x02C6",
    "&tilde;":  "\x02DC",
    "&ensp;":   "\x2002",
    "&emsp;":   "\x2003",
    "&thinsp;": "\x2009",
    "&zwnj;":   "\x200C",
    "&zwj;":    "\x200D",
    "&lrm;":    "\x200E",
    "&rlm;":    "\x200F",
    "&ndash;":  "\x2013",
    "&mdash;":  "\x2014",
    "&lsquo;":  "\x2018",
    "&rsquo;":  "\x2019",
    "&sbquo;":  "\x201A",
    "&ldquo;":  "\x201C",
    "&rdquo;":  "\x201D",
    "&bdquo;":  "\x201E",
    "&dagger;": "\x2020",
    "&Dagger;": "\x2021",
    "&permil;": "\x2030",
    "&lsaquo;": "\x2039",
    "&rsaquo;": "\x203A",
    "&euro;":   "\x20AC",
]);

constant symbols
=([ "&fnof;":     "\x0192",
    "&thetasym;": "\x03D1",
    "&upsih;":    "\x03D2",
    "&piv;":      "\x03D6",
    "&bull;":     "\x2022",
    "&hellip;":   "\x2026",
    "&prime;":    "\x2032",
    "&Prime;":    "\x2033",
    "&oline;":    "\x203E",
    "&frasl;":    "\x2044",
    "&weierp;":   "\x2118",
    "&image;":    "\x2111",
    "&real;":     "\x211C",
    "&trade;":    "\x2122",
    "&alefsym;":  "\x2135",
    "&larr;":     "\x2190",
    "&uarr;":     "\x2191",
    "&rarr;":     "\x2192",
    "&darr;":     "\x2193",
    "&harr;":     "\x2194",
    "&crarr;":    "\x21B5",
    "&lArr;":     "\x21D0",
    "&uArr;":     "\x21D1",
    "&rArr;":     "\x21D2",
    "&dArr;":     "\x21D3",
    "&hArr;":     "\x21D4",
    "&forall;":   "\x2200",
    "&part;":     "\x2202",
    "&exist;":    "\x2203",
    "&empty;":    "\x2205",
    "&nabla;":    "\x2207",
    "&isin;":     "\x2208",
    "&notin;":    "\x2209",
    "&ni;":       "\x220B",
    "&prod;":     "\x220F",
    "&sum;":      "\x2211",
    "&minus;":    "\x2212",
    "&lowast;":   "\x2217",
    "&radic;":    "\x221A",
    "&prop;":     "\x221D",
    "&infin;":    "\x221E",
    "&ang;":      "\x2220",
    "&and;":      "\x2227",
    "&or;":       "\x2228",
    "&cap;":      "\x2229",
    "&cup;":      "\x222A",
    "&int;":      "\x222B",
    "&there4;":   "\x2234",
    "&sim;":      "\x223C",
    "&cong;":     "\x2245",
    "&asymp;":    "\x2248",
    "&ne;":       "\x2260",
    "&equiv;":    "\x2261",
    "&le;":       "\x2264",
    "&ge;":       "\x2265",
    "&sub;":      "\x2282",
    "&sup;":      "\x2283",
    "&nsub;":     "\x2284",
    "&sube;":     "\x2286",
    "&supe;":     "\x2287",
    "&oplus;":    "\x2295",
    "&otimes;":   "\x2297",
    "&perp;":     "\x22A5",
    "&sdot;":     "\x22C5",
    "&lceil;":    "\x2308",
    "&rceil;":    "\x2309",
    "&lfloor;":   "\x230A",
    "&rfloor;":   "\x230B",
    "&lang;":     "\x2329",
    "&rang;":     "\x232A",
    "&loz;":      "\x25CA",
    "&spades;":   "\x2660",
    "&clubs;":    "\x2663",
    "&hearts;":   "\x2665",
    "&diams;":    "\x2666",
]);

constant greek
= ([ "&Alpha;":   "\x391",
     "&Beta;":    "\x392",
     "&Gamma;":   "\x393",
     "&Delta;":   "\x394",
     "&Epsilon;": "\x395",
     "&Zeta;":    "\x396",
     "&Eta;":     "\x397",
     "&Theta;":   "\x398",
     "&Iota;":    "\x399",
     "&Kappa;":   "\x39A",
     "&Lambda;":  "\x39B",
     "&Mu;":      "\x39C",
     "&Nu;":      "\x39D",
     "&Xi;":      "\x39E",
     "&Omicron;": "\x39F",
     "&Pi;":      "\x3A0",
     "&Rho;":     "\x3A1",
     "&Sigma;":   "\x3A3",
     "&Tau;":     "\x3A4",
     "&Upsilon;": "\x3A5",
     "&Phi;":     "\x3A6",
     "&Chi;":     "\x3A7",
     "&Psi;":     "\x3A8",
     "&Omega;":   "\x3A9",
     "&alpha;":   "\x3B1",
     "&beta;":    "\x3B2",
     "&gamma;":   "\x3B3",
     "&delta;":   "\x3B4",
     "&epsilon;": "\x3B5",
     "&zeta;":    "\x3B6",
     "&eta;":     "\x3B7",
     "&theta;":   "\x3B8",
     "&iota;":    "\x3B9",
     "&kappa;":   "\x3BA",
     "&lambda;":  "\x3BB",
     "&mu;":      "\x3BC",
     "&nu;":      "\x3BD",
     "&xi;":      "\x3BE",
     "&omicron;": "\x3BF",
     "&pi;":      "\x3C0",
     "&rho;":     "\x3C1",
     "&sigmaf;":  "\x3C2",
     "&sigma;":   "\x3C3",
     "&tau;":     "\x3C4",
     "&upsilon;": "\x3C5",
     "&phi;":     "\x3C6",
     "&chi;":     "\x3C7",
     "&psi;":     "\x3C8",
     "&omega;":   "\x3C9",
]);

array replace_entities = indices( iso88591 )+indices( international )+
  indices( symbols )+indices( greek )+indices( num )+
  ({"&lt;","&gt;","&amp;","&quot;","&apos;"});

array replace_values = values( iso88591 )+values( international )+
  values( symbols )+values( greek )+values( num )+({"<",">","&","\"","\'"});
