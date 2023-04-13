/* -*- mode: Pike; c-basic-offset: 3; -*- */

#pike __REAL_VERSION__

mapping _continents=
([
   "Europe":
   ({ "AD","AL","AT","AX","BA","BE","BG","BY","CH","CZ",
      "DE","DK","EE","ES","FI","FO","FR","GB","GE","GG",
      "GI","GR","HR","HU","IE","IM","IS","IT","JE","LI",
      "LT","LU","LV","MC","MD","ME","MK","MT","NL","NO",
      "PL","PT","RO","RS","RU","SE","SI","SJ","SK","SM",
      "TM","UA","VA","EU"
   }),

   "Africa":
   ({ "AC","AO","BF","BI","BJ","BW","CF","CG","CG","CI",
      "CM","CV","DJ","DZ","EG","EH","ER","ET","GA","GH",
      "GM","GN","GQ","GW","KE","KM","LR","LS","LY","MA",
      "MG","ML","MR","MU","MW","MZ","NA","NE","NG","RW",
      "SC","SD","SH","SL","SN","SO","SS","ST","SZ","TD",
      "TG","TN","TZ","UG","ZA","ZM","ZR","ZW","YT"
   }),

   "Asia":
   ({ "AC","AE","AF","AM","AZ","BD","BH","BN","BT","BV",
      "CN","CX","CY","HK","ID","IL","IO","IN","IQ","IR",
      "JO","JP","KG","KH","KP","KR","KW","KZ","LA","LB",
      "LK","MM","MN","MO","MV","MY","NP","OM","PH","PK",
      "PS","QA","RE","RU","SA","SG","SY","TH","TJ","TL",
      "TR","TW","UZ","VN","YE"
   }),

   "North America":
   ({ "AG","AW","BB","BM","BS","BZ","CA","CR","CU","CW",
      "DM","DO","GD","GL","GT","HN","HT","JM","KN","KY",
      "LC","MF","MQ","MS","MX","NI","PA","PR","SV","TT",
      "US","VC","PM","SX"
   }),

   "South America":
   ({ "AI","AN","AR","BO","BQ","BR","CL","CO","EC","FK",
      "GF","GY","PE","PY","SR","UY","VE","GP","GS","TC",
      "VG","VI"
   }),

   "Oceania":
   ({ "AS","AU","CC","CK","FJ","FM","KI","MH","MP","NC",
      "NR","NU","NZ","PF","PG","PN","PW","SB","TO","TV",
      "VU","WS","GU","NF","TK","UM","WF"
   }),

   "Antarctica":
   ({ "AQ","TF","HM" }),
]);

//!	All known countries.
array(Country) countries=
({
   // AA USER ASSIGNED
   Country("AC","Ascension","SH"), // EXCEPTIONALLY RESERVED
   Country("AD","Andorra","AN"),
   Country("AE","United Arab Emirates","AE"),
   Country("AF","Afghanistan","AF"),
   Country("AG","Antigua and Barbuda","AC"),
   Country("AI","Anguilla","AV"),
   Country("AL","Albania","AL"),
   Country("AM","Armenia","AM"),
   Country("AN","Netherlands Antilles","NL"), // TRANSITIONALLY RESERVED
   Country("AO","Angola","AO"),
   // AP NOT USED
   Country("AQ","Antarctica","AY"),
   Country(0,"Bouvet Island","BV",
           (["_continent":"Antarctica"])),
   Country("AR","Argentina","AR"),
   Country("AS","American Samoa","AQ"),
   Country("AT","Austria","AU"),
   Country("AU","Australia","AS"),
   Country(0,"Ashmore and Cartier Islands","AT",
           (["_continent":"Australia"])),
   Country("AW","Aruba","AA"),
   Country("AX","Aland Islands",0),
   Country("AZ","Azerbaijan","AJ"),
   Country("BA","Bosnia and Herzegovina","BK"),
   Country("BB","Barbados","BB"),
   Country("BD","Bangladesh","BG"),
   Country("BE","Belgium","BE"),
   Country("BF","Burkina Faso","UV"),
   Country("BG","Bulgaria","BU"),
   Country("BH","Bahrain","BA"),
   Country("BI","Burundi","BY"),
   Country("BJ","Benin","BN"),
   Country("BM","Bermuda","BD"),
   Country("BN","Brunei Darussalam","BX",
	   (["aka":({"Brunei"})])),
   Country("BO","Bolivia","BL",(["aka":({"Plurinational State of Bolivia"})])),
   Country("BQ","Bonaire",0,
           (["aka":({"Bonaire, Sint Eustatius and Saba"})])),
   Country("BR","Brazil","BR"),
   Country("BS","Bahamas","BF",(["aka":({"The Bahamas"})])),
   Country("BT","Bhutan","BT"),
   // BU TRANSITIONALLY RESERVED
   Country("BV","Bouvet Island","BV"),
   Country("BW","Botswana","BC"),
   // BX NOT USED
   Country("BY","Belarus","BO"),
   Country("BZ","Belize","BH"),
   Country("CA","Canada","CA"),
   Country("CC","Cocos Islands","CK",
	   (["aka":({"Keeling Islands"})])),
   Country("CD","Congo (Kinshasa)","CG",
	   (["aka":({"Congo, Democratic Republic of the",
		     "Congo (Kinshasa)","Congo"}),
	     "_continent":"Africa"])),
   Country("CF","Central African Republic","CT"),
   Country("CG","Congo","CF",(["aka":({"Congo, Republic of the",
				       "Congo (Brazzaville)"})])),
   Country("CH","Switzerland","SZ"),
   Country("CI","Cote D'Ivoire","IV",(["aka":({"Ivory Coast"})])),
   Country("CK","Cook Islands","CW"),
   Country("CL","Chile","CI"),
   Country("CM","Cameroon","CM"),
   Country("CN","China","CH"),
   Country("CO","Colombia","CO"),
   // CP EXCEPTIONALLY RESERVED
   Country("CR","Costa Rica","CS"),
   Country("CS","Czechoslovakia","LO", // TRANSITIONALLY RESERVED
           (["former":1, "_continent":"Europe"])),
   Country("CU","Cuba","CU"),
   Country("CV","Cape Verde","CV"),
   Country("CW","Curacao","UC"),
   Country("CX","Christmas Island","KT"),
   Country("CY","Cyprus","CY"),
   Country(0,"Akrotiri","AX",(["_continent":"Europe"])),
   Country(0,"Dhekelia","DX",(["_continent":"Europe"])),
   Country("CZ","Czech Republic","EZ"),
   Country("DE","Germany","GM"),
   // DG EXCEPTIONALLY RESERVED
   Country("DJ","Djibouti","DJ"),
   Country("DK","Denmark","DA"),
   Country("DM","Dominica","DO"),
   Country("DO","Dominican Republic","DR"),
   // DY INDETERMINATELY RESERVED
   Country("DZ","Algeria","AG"),
   // EA EXCEPTIONALLY RESERVED
   Country("EC","Ecuador","EC"),
   Country("EE","Estonia","EN"),
   // EF NOT USED
   Country("EG","Egypt","EG"),
   Country("EH","Western Sahara","WI"),
   Country("ER","Eritrea","ER"),
   // EM NOT USED
   // EP NOT USED
   Country("ES","Spain","SP"),
   Country("ET","Ethiopia","ET"),
   Country("EU","European Union",0),
   // EV NOT USED
   // EW INDETERMINATELY RESERVED
   // EZ EXCEPTIONALLY RESERVED
   Country("FI","Finland","FI"),
   Country("FJ","Fiji","FJ"),
   Country("FK","Falkland Islands","FA",(["aka":({"Malvinas"})])),
   // FL INDETERMINATELY RESERVED
   Country("FM","Micronesia","NE",
	   (["aka":({"Federated States of Micronesia",
		     "Micronesia, Federated States of"})])),
   Country("FO","Faroe Islands","FO"),
   Country("FR","France","FR"),
   // FZ EXCEPTIONALY RESERVED
   Country("GA","Gabon","GB"),
   Country("GB","United Kingdom","UK",
           (["aka":
             ({"Great Britain",
               "United Kingdom of Great Britain and Northen Ireland"})])),
   // GC NOT USED
   Country("GD","Grenada","GJ"),
   Country("GE","Georgia","GG"),
   Country("GF","French Guiana","FG"),
   Country("GG","Guernsey",0),
   Country("GH","Ghana","GH"),
   Country("GI","Gibraltar","GI"),
   Country("GL","Greenland","GL"),
   Country("GM","Gambia","GA",
	   (["aka":({"Gambia, The","The Gambia"})])),
   Country("GN","Guinea","GV"),
   Country("GP","Guadeloupe","GP"),
   Country("GQ","Equatorial Guinea","EK"),
   Country("GR","Greece","GR"),
   Country("GS","S. Georgia and S. Sandwich Isls.","SX",
           (["aka":({"South Georgia and the South Sandwich Islands"})])),
   Country("GT","Guatemala","GT"),
   Country("GU","Guam","GQ"),
   Country("GW","Guinea-Bissau","PU"),
   Country("GY","Guyana","GY"),
   Country("HK","Hong Kong","HK"),
   Country("HM","Heard Island and McDonald Islands","HM"),
   Country("HN","Honduras","HO"),
   Country("HR","Croatia","HR",(["aka":({"Hrvatska"})])),
   Country("HT","Haiti","HA"),
   Country("HU","Hungary","HU"),
   // IB NOT USED
   // IC EXCEPTIONALLY RESERVED
   Country("ID","Indonesia","ID"),
   Country("IE","Ireland","EI"),
   Country("IL","Israel","IS",(["aka":({"State of Israel"})])),
   Country("IM","Isle of Man",0),
   Country("IN","India","IN"),
   Country("IO","British Indian Ocean Territory","IO"),
   Country("IQ","Iraq","IZ"),
   Country("IR","Iran","IR",(["aka":({"Islamic Republic of Iran"})])),
   Country("IS","Iceland","IC"),
   Country("IT","Italy","IT"),
   // JA INDETERMINATELY RESERVED
   Country("JE","Jersey","JE"),
   Country("JM","Jamaica","JM"),
   Country("JO","Jordan","JO"),
   Country("JP","Japan","JA"),
   Country("KE","Kenya","KE"),
   Country("KG","Kyrgyzstan","KG"),
   Country("KH","Cambodia","CB"),
   Country("KI","Kiribati","KR"),
   Country("KM","Comoros","CN"),
   Country("KN","Saint Kitts and Nevis","SC"),
   Country("KP","North Korea","KN",
           (["aka":
             ({"Korea, North",
               "Democratic People's Republic of Korea"})])),
   Country("KR","South Korea","KS",(["aka":({"Korea, South",
                                             "Republic of Korea"})])),
   Country("KW","Kuwait","KU"),
   Country("KY","Cayman Islands","CJ"),
   Country("KZ","Kazakhstan","KZ"),
   Country("LA","Laos","LA",(["aka":({"Lao People's Democratic Republic"})])),
   Country("LB","Lebanon","LE"),
   Country("LC","Saint Lucia","ST"),
   // LF INDETERMINATELY RESERVED
   Country("LI","Liechtenstein","LS"),
   Country("LK","Sri Lanka","CE"),
   Country("LR","Liberia","LI"),
   Country("LS","Lesotho","LT"),
   Country("LT","Lithuania","LH"),
   Country("LU","Luxembourg","LU"),
   Country("LV","Latvia","LG"),
   Country("LY","Libya","LY"),
   Country("MA","Morocco","MO"),
   Country("MC","Monaco","MN"),
   Country("MD","Moldova","MD",(["aka":({"Republic of Moldova"})])),
   Country("ME","Montenegro","MW"),
   Country("MF","Saint Martin","RN",(["aka":({"French Saint Martin"})])),
   Country("MG","Madagascar","MA"),
   Country("MH","Marshall Islands","RM"),
   Country("MK","Macedonia","MK",
           (["aka":({"The former Yugoslav Republic of Macedonia",
                     "Republic of Macedonia"})])),
   Country("ML","Mali","ML"),
   Country("MM","Myanmar","BM",
	   (["aka":({"Burma"})])),
   Country("MN","Mongolia","MG"),
   Country("MO","Macao","MC",(["aka":({"Macau"})])),
   Country("MP","Northern Mariana Islands","CQ"),
   Country("MQ","Martinique","MB"),
   Country("MR","Mauritania","MR"),
   Country("MS","Montserrat","MH"),
   Country("MT","Malta","MT"),
   Country("MU","Mauritius","MP"),
   Country("MV","Maldives","MV"),
   Country("MW","Malawi","MI"),
   Country("MX","Mexico","MX"),
   Country("MY","Malaysia","MY"),
   Country("MZ","Mozambique","MZ"),
   Country("NA","Namibia","WA"),
   Country("NC","New Caledonia","NC"),
   Country("NE","Niger","NG"),
   Country("NF","Norfolk Island","NF"),
   Country("NG","Nigeria","NI"),
   Country("NI","Nicaragua","NU"),
   Country("NL","Netherlands","NL"),
   Country("NO","Norway","NO"),
   Country("NP","Nepal","NP"),
   Country("NR","Nauru","NR"),
   Country("NT","Neutral Zone",0, // TRANSITIONALLY RESERVED
           (["_continent":"N/A"])),
   Country("NU","Niue","NE"),
   Country("NZ","New Zealand (Aotearoa)","NZ",
	   (["aka":({"Aotearoa","New Zealand","New Zeeland"})])),
   // OA NOT USED
   Country("OM","Oman","MU"),
   Country("PA","Panama","PM"),
   Country("PE","Peru","PE"),
   Country("PF","French Polynesia","FP"),
   Country("PG","Papua New Guinea","PP"),
   Country("PH","Philippines","RP"),
   // PI INDETERMINATELY RESERVED
   Country("PK","Pakistan","PK"),
   Country("PL","Poland","PL"),
   Country("PM","St. Pierre and Miquelon","SB"),
   Country("PN","Pitcairn","PC",
	   (["aka":({"Pitcairn Islands"})])),
   Country("PR","Puerto Rico","RQ"),
   Country("PS","Palestine",0,(["aka":({"State of Palestine"})])),
   Country("PT","Portugal","PO"),
   Country("PW","Palau","PS"),
   Country("PY","Paraguay","PA"),
   Country("QA","Qatar","QA"),
   // QM-QZ USER ASSIGNED
   // RA-RC INDETERMINATELY RESERVED
   Country("RE","Reunion","RE"),
   // RH-RI INDETERMINATELY RESERVED
   // RL-RN INDETERMINATELY RESERVED
   Country("RO","Romania","RO",(["aka":({"Rumania"})])),
   // RP INDETERMINATELY RESERVED
   Country("RS","Serbia","SR"),
   Country("RU","Russian Federation","RS",
	   (["aka":({"Russia"}),
	     "_continent":"Europe"
	   ])),
   Country("RW","Rwanda","RW"),
   Country("SA","Saudi Arabia","SA"),
   Country("SB","Solomon Islands","BP"),
   Country("SC","Seychelles","SE"),
   Country("SD","Sudan","SU"),
   Country("SE","Sweden","SW"),
   // SF TRANSITIONALLY RESERVED
   Country("SG","Singapore","SN"),
   Country("SH","St. Helena","SH",
           (["aka":({"Saint Helena",
                     "Sait Helena, Ascension and Tristan da Cunha" })])),
   Country("SI","Slovenia","SI"),
   Country("SJ","Svalbard and Jan Mayen Islands",0),
   Country(0,"Svalbard","SV",
           (["_continent":"Europe"])),
   Country(0,"Jan Mayen","JN",
           (["_continent":"Europe"])),
   Country("SK","Slovak Republic","LO",
	   (["aka":({"Slovakia"})])),
   Country("SL","Sierra Leone","SL"),
   Country("SM","San Marino","SM"),
   Country("SN","Senegal","SG"),
   Country("SO","Somalia","SO"),
   Country("SR","Suriname","NS"),
   Country("SS","South Sudan",0),
   Country("ST","Sao Tome and Principe","TP"),
   Country("SU","USSR",0,(["former":1, // EXCEPTIONALLY RESERVED
                           "_continent":"Asia"])),
   Country("SV","El Salvador","ES"),
   Country("SX","Sint Maarten","NN",(["aka":({"Dutch Sint Maarten"})])),
   Country("SY","Syria","SY"),
   Country("SZ","Swaziland","WZ"),
   // TA EXCEPTIONALLY RESERVED
   Country("TC","Turks and Caicos Islands","TK"),
   Country("TD","Chad","CD"),
   Country("TF","French Southern Territories","FS",
           (["aka":({ "French Southern and Antarctic Lands" })])),
   Country("TG","Togo","TO"),
   Country("TH","Thailand","TH"),
   Country("TJ","Tajikistan","TI"),
   Country("TK","Tokelau","TL"),
   Country("TL","Timor-Leste","TT",(["aka":({"East Timor"})])),
   Country("TM","Turkmenistan","TX"),
   Country("TN","Tunisia","TS"),
   Country("TO","Tonga","TN"),
   Country("TP","East Timor",0, // TRANSITIONALLY RESERVED
           (["former":1,"_continent":"Oceania"])),
   Country("TR","Turkey","TU"),
   Country("TT","Trinidad and Tobago","TD"),
   Country("TV","Tuvalu","TV"),
   Country("TW","Taiwan","TW"),
   Country("TZ","Tanzania","TZ",(["aka":({"United Republic of Tanzania"})])),
   Country("UA","Ukraine","UP"),
   Country("UG","Uganda","UG"),
   // UK EXCEPTIONALLY RESERVED
   Country("UM","United States Minor Outlying Islands",0),
   // UN EXCEPTIONALLY RESERVED
   Country("US","USA","US",(["aka":({"United States of America"})])),
   Country("UY","Uruguay","UY"),
   Country("UZ","Uzbekistan","UZ"),
   Country("VA","Vatican City State","VT",
	   (["aka":({"Holy See",
                     "Vatican City"})])),
   Country("VC","Saint Vincent and the Grenadines","VC"),
   Country("VE","Venezuela","VE",
           (["aka":({"bolivarian Republic of Venezuela"})])),
   Country("VG","Virgin Islands (British)","VI"),
   Country("VI","Virgin Islands (U.S.)","VQ"),
   Country("VN","Viet Nam","VM",(["aka":({"Vietnam"})])),
   Country("VU","Vanuatu","NH"),
   Country("WF","Wallis and Futuna Islands","WF"),
   // WG INDETERMINATELY RESERVED
   // WL INDETERMINATELY RESERVED
   // WO NO USED
   Country("WS","Samoa","WS",
	   (["aka":({"Western Samoa"})])),
   // WV INDETERMINATELY RESERVED
   // XA-XZ USER ASSIGNED
   Country("YE","Yemen","YM"),
   Country("YT","Mayotte","MF"),
   Country("YU","Yugoslavia",0, // TRANSITIONALLY RESERVED
           (["former":1, "_continent":"Europe"])),
   // YV INDETERMINATELY RESERVED
   Country("ZA","South Africa","SF"),
   Country("ZM","Zambia","ZA"),
   Country("ZR","Zaire",0, // TRANSITIONALLY RESERVED
           (["former":1,"_continent":"Africa"])),
   Country("ZW","Zimbabwe","ZI"),
   // ZZ USER ASSIGNED
});

//! Country
class Country
{
   string|zero iso2;
   //!    ISO-3166-1 2-character code aka domain name.
   //!
   //! @note
   //!    May be zero in some obscure cases where the
   //!    ISO-3166-1 grouping differs from the FIPS-10
   //!    grouping.

   string|zero fips10;
   //!    FIPS 10-character code;
   //!    "Federal Information Processing Standards 10-4" etc,
   //!    previously used by some goverments in the US.
   //!
   //! @note
   //!    This character code is slightly different from @[iso2],
   //!    and should only be used for compatibility with old code.
   //!
   //! @deprecated iso2
   //!    FIPS 10-4 was withdrawn by NIST 2008-09-02.

   string name;
   array(string) aka=({});
   //!    Country name and also-known-as, if any

   int former=0;
   //!    Flag that is set if this country doesn't exist anymore.
   //!    (eg USSR.)

   //! @decl string continent()
   //!	Returns the continent of the country.
   //! @note
   //!	Some countries are geographically in more then
   //!	one continent; any of the continents might be
   //!	returned then, but probably the continent in which
   //!	the capital is resident - Europe for Russia, for instance.

   string _continent=0;
   string continent()
   {
      if (!_continent)
	 continents(); // load continents
      return _continent;
   }

   protected void create(string|zero _iso2,string _name,string|zero _fips10,
			 mapping|void opts)
   {
      iso2=_iso2;
      name=_name;
      fips10=_fips10;
      if (opts)
      {
	 foreach ( (array)opts, [string what,string val])
	    ::`[]=(what,val);
      }
   }

   //! @decl string cast("string")
   //!	It is possible to cast a country to a string,
   //!	which will be the same as performing
   //!	@expr{country->name;@}.
   protected string cast(string to)
   {
      if (to=="string") return name;
      return UNDEFINED;
   }

   protected string _sprintf(int t)
   {
     return t=='O' && sprintf("%O(%s)", this_program, name);
   }

}

//! @decl Country from_domain(string domain)
//!	Look up a country from a domain name.
//!	Returns zero if the domain doesn't map
//!	to a country. Note that there are some
//!	valid domains that don't:
//!	@dl
//!	  @item INT
//!         International
//!	  @item MIL
//!         US Military
//!	  @item NET
//!         Network
//!	  @item ORG
//!         Non-Profit Organization
//!	  @item ARPA
//!         Old style Arpanet
//!	  @item NATO
//!         Nato field
//!	@enddl
//!
//!	And that US has five domains, Great Britain and france two:
//!	<dl compact>
//!	<dt>EDU   <dd>US Educational
//!	<dt>MIL   <dd>US Military
//!	<dt>GOV   <dd>US Government
//!	<dt>UM	  <dd>US Minor Outlying Islands
//!	<dt>US	  <dd>US
//!	<dt>GB    <dd>Great Britain (UK)
//!	<dt>UK    <dd>United Kingdom
//!	<dt>FR	  <dd>France
//!	<dt>FX	  <dd>France, Metropolitan
//!	<dt>There's also three domains that for convinience maps to US:
//!	<dt>NET   <dd>Network
//!	<dt>ORG   <dd>Organization
//!	<dt>COM   <dd>Commercial
//!	</dl>

protected private mapping _from_domain=0;

Country from_domain(string domain)
{
   if (!_from_domain)
   {
      _from_domain=mkmapping(countries->iso2,countries);
      _from_domain|=
      (["COM":_from_domain->US,
	"EDU":_from_domain->US,
	"MIL":_from_domain->US,
	"GOV":_from_domain->US,
	"NET":_from_domain->US,
	"ORG":_from_domain->US,
        "UK":_from_domain->GB,
	"FX":_from_domain->FR,
      ]);
   }
   return _from_domain[upper_case(domain)];
}

//! @decl Country from_domain(string name)
//!	Look up a country from its name or aka.
//!	The search is case-insensitive but
//!	regards whitespace and interpunctation.

protected private mapping _from_name=0;

Country from_name(string name)
{
   if (!_from_name)
   {
      _from_name=
	 `+(@map( countries,
		  lambda(Country c)
		  {
		     if (c->aka && sizeof(c->aka))
			return
		     ([lower_case(c->name):c])+
			   (mapping)map(
			      c->aka,
			      lambda(string n)
			      {
				 return ({lower_case(n),c});
			      });
		     else
			return ([lower_case(c->name):c]);
		  }));
   }
   return _from_name[lower_case(name)];
}

//! @decl mapping(string:array(Country)) continents()
//!	Gives back a mapping from continent name to
//!	an array of the countries on that continent.
//!
//!	The continents are:
//!	@pre{
//!    	  "Europe"
//!    	  "Africa"
//!    	  "Asia"
//!    	  "North America"
//!    	  "South America"
//!    	  "Oceania"
//!	  "Antarctica"
//!	@}
//! @note
//!	Some countries are considered to be on more than one continent.

protected private mapping _cached_continents;

mapping(string:array(Country)) continents()
{
   if (_cached_continents) return _cached_continents;

   _cached_continents=([]);
   from_domain("se"); // initialize _from_domain
   foreach (_continents;string name;array(string) iso2s)
      _cached_continents[name]=map(
	 iso2s,
	 lambda(string iso2)
         {
            Country co=_from_domain[iso2];
            if (!co) error("%O has unknown country: %O\n",name,iso2);
	    if (!co->_continent) co->_continent=name;
	    return co;
	 });

   return _cached_continents;
}

//! @decl mixed `[](string what)
//! @decl mixed `->(string what)
//!	Convenience functions for getting a country
//!	the name-space way; it looks up whatever it
//!	is in the name- and domain-space and
//!	returns that country if possible:
//!
//! @code
//! > Geography.Countries.se;
//! Result: Country(Sweden)
//! > Geography.Countries.djibouti;
//! Result: Country(Djibouti)
//! > Geography.Countries.com;
//! Result: Country(United States)
//! > Geography.Countries.wallis_and_futuna_islands->iso2;
//! Result: "WF"
//! @endcode

protected Country|function(string:Country)|array(Country)|program
   `->(string what)
{
   return
      ::`[](what) ||
      from_domain(what)||
      from_name(what)||
      from_name(replace(what,"_"," "));
}

function `[] =`->;
