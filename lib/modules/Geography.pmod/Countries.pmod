#pike __REAL_VERSION__

mapping _continents=
([ 
   ","  "Europe":
   ({ "AD","AL","AT","BA","BE","BG","BY","CH","CZ",
      "DE","DK","EE","ES","FI","FO","FR","GB","GE","GI",
      "GR","HR","HU","IE","IS","IT","JE","LI","LT","LU",
      "LV","MC","MD","MK","MT","NL","NO","PL","PT","RO",
      "RU","SE","SI","SJ","SK","SM","TM","UA","VA","YU",}),

   "Africa": 
   ({ "AC","AO","BF","BI","BJ","BW","CF","CG","CG","CI",
      "CM","CV","DJ","DZ","EG","EH","ER","ET","GA","GH",
      "GM","GN","GQ","GW","KE","KM","LR","LS","LY",
      "MA","MG","ML","MR","MU","MW","MZ","NA","NE",
      "NG","RW","SC","SD","SH","SL","SN","SO","ST",
      "SZ","TD","TG","TN","TZ","UG","ZA","ZM","ZR","ZW",
      "YT"
   }),

   "Asia":
   ({ "AC","AE","AF","AM","AZ","BD","BH","BN","BT","BV","CN","CX",
      "CY","HK","ID","IL","IO","IN","IQ","IR","JO","JP",
      "KG","KH","KP","KR","KW","KZ","LA","LB","LK",
      "MM","MN","MO","MV","MY","NP","OM","PH","PK","RE",
      "QA","SA","SG","SY","TH","TJ","TR","TW","UZ",
      "VN","YE","RU"}),

   "North America":
   ({ "AG","AW","BB","BM","BS","BZ","CA","CR",
      "CU","DM","DO","GD","GL","GT","HN","HT","JM","KN",
      "KY","LC","MQ","MS","MX","NI","PA","PR","SV","TT",
      "US","VC","PM"}),

   "South America":
   ({"AI","AN","AR","BO","BR","CL","CO","EC","FK","GF",
     "GY","PE","PY","SR","UY","VE","GP","GS","TC","VG","VI"}),

   "Oceania":
   ({"AS","AU","CC","CK","FJ","FM","KI","MH","MP","NC","NR","NU",
     "NZ","PF","PG","PN","PW","SB","TO","TV","VU","WS","GU","NF",
     "TK","TP","WF"}),

   "Antarctica":
   ({"AQ","TF","HM"}),
]);

//!	All known countries.
array(Country) countries=
({
   Country("AC","Ascension","SH"), // saint helena?
   Country("AD","Andorra","AN"),
   Country("AE","United Arab Emirates","AE"),
   Country("AF","Afghanistan","AF"),
   Country("AG","Antigua and Barbuda","AC"),
   Country("AI","Anguilla","AV"),
   Country("AL","Albania","AL"),
   Country("AM","Armenia","AM"),
   Country("AN","Netherlands Antilles","NL"),
   Country("AO","Angola","AO"),
   Country("AQ","Antarctica","AY"),
   Country("AR","Argentina","AR"),
   Country("AS","American Samoa","WS"),
   Country("AT","Austria","AU"),
   Country("AU","Australia","AS"),
   Country("AW","Aruba","AA"),
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
   Country("BO","Bolivia","BL"),
   Country("BR","Brazil","BR"),
   Country("BS","Bahamas","BF",(["aka":({"The Bahamas"})])),
   Country("BT","Bhutan","BT"),
   Country("BV","Bouvet Island","BV"),
   Country("BW","Botswana","BC"),
   Country("BY","Belarus","BO"),
   Country("BZ","Belize","BH"),
   Country("CA","Canada","CA"),
   Country("CC","Cocos Islands","CK",
	   (["aka":({"Keeling Islands"})])),
   Country("CF","Central African Republic","CT"),
   Country("CG","Congo","CF",(["aka":({"Congo, Republic of the",
				       "Congo (Brazzaville)"})])),
   Country("??","Congo (Kinshasa)","CG",
	   (["aka":({"Congo, Democratic Republic of the",
		     "Congo (Kinshasa)","Congo"}),
	     "_continent":"Africa"])),
   Country("CH","Switzerland","SZ"),
   Country("CI","Cote D'Ivoire","IV",(["aka":({"Ivory Coast"})])),
   Country("CK","Cook Islands","CW"),
   Country("CL","Chile","CI"),
   Country("CM","Cameroon","CM"),
   Country("CN","China","CH"),
   Country("CO","Colombia","CO"),
   Country("CR","Costa Rica","CS"),
   Country("CS","Czechoslovakia","LO",(["former":1,
				   "_continent":"Europe"])),
   Country("CU","Cuba","CU"),
   Country("CV","Cape Verde","CV"),
   Country("CX","Christmas Island","KT"),
   Country("CY","Cyprus","CY"),
   Country("CZ","Czech Republic","EZ"),
   Country("DE","Germany","GM"),
   Country("DJ","Djibouti","DJ"),
   Country("DK","Denmark","DA"),
   Country("DM","Dominica","DO"),
   Country("DO","Dominican Republic","DR"),
   Country("DZ","Algeria","AG"),
   Country("EC","Ecuador","EC"),
   Country("EE","Estonia","EN"),
   Country("EG","Egypt","EG"),
   Country("EH","Western Sahara","WI"),
   Country("ER","Eritrea","ER"),
   Country("ES","Spain","SP"),
   Country("ET","Ethiopia","ET"),
   Country("FI","Finland","FI"),
   Country("FJ","Fiji","FJ"),
   Country("FK","Falkland Islands","FA",(["aka":({"Malvinas"})])),
   Country("FM","Micronesia","NE",
	   (["aka":({"Federated States of Micronesia",
		     "Micronesia, Federated States of"})])),
   Country("FO","Faroe Islands","FO"),
   Country("FR","France","FR"),
   Country("GA","Gabon","GB"),
   Country("GB","United Kingdom","UK",(["aka":({"Great Britain"})])),
   Country("GD","Grenada","GJ"),
   Country("GE","Georgia","GG"),
   Country("GF","French Guiana","FG"),
   Country("GH","Ghana","GH"),
   Country("GI","Gibraltar","GI"),
   Country("GL","Greenland","GL"),
   Country("GM","Gambia","GA",
	   (["aka":({"Gambia, The","The Gambia"})])),
   Country("GN","Guinea","GV"),
   Country("GP","Guadeloupe","GP"),
   Country("GQ","Equatorial Guinea","EK"),
   Country("GR","Greece","GR"),
   Country("GS","S. Georgia and S. Sandwich Isls.","SX"),
   Country("GT","Guatemala","GT"),
   Country("GU","Guam","GQ"),
   Country("GW","Guinea-Bissau","PU"),
   Country("GY","Guyana","GY"),
   Country("HK","Hong Kong","HK"),
   Country("HM","Heard and McDonald Islands","HM"),
   Country("HN","Honduras","HO"),
   Country("HR","Croatia","HR",(["aka":({"Hrvatska"})])),
   Country("HT","Haiti","HA"),
   Country("HU","Hungary","HU"),
   Country("ID","Indonesia","ID"),
   Country("IE","Ireland","EI"),
   Country("IL","Israel","IS",(["aka":({"State of Israel"})])),
   Country("IN","India","IN"),
   Country("IO","British Indian Ocean Territory","IO"),
   Country("IQ","Iraq","IZ"),
   Country("IR","Iran","IR"),
   Country("IS","Iceland","IC"),
   Country("IT","Italy","IT"),
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
   Country("KP","North Korea","KN",(["aka":({"Korea, North"})])),
   Country("KR","South Korea","KS",(["aka":({"Korea, South"})])),
   Country("KW","Kuwait","KU"),
   Country("KY","Cayman Islands","CJ"),
   Country("KZ","Kazakhstan","KZ"),
   Country("LA","Laos","LA"),
   Country("LB","Lebanon","LE"),
   Country("LC","Saint Lucia","ST"),
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
   Country("MD","Moldova","MD"),
   Country("MG","Madagascar","MA"),
   Country("MH","Marshall Islands","RM"),
   Country("MK","Macedonia","MK"),
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
   Country("NT","Neutral Zone","??",(["_continent":"N/A"])),
   Country("NU","Niue","NE"),
   Country("NZ","New Zealand (Aotearoa)","NZ",
	   (["aka":({"Aotearoa","New Zealand","New Zeeland"})])),
   Country("OM","Oman","MU"),
   Country("PA","Panama","PM"),
   Country("PE","Peru","PE"),
   Country("PF","French Polynesia","FP"),
   Country("PG","Papua New Guinea","PP"),
   Country("PH","Philippines","RP"),
   Country("PK","Pakistan","PK"),
   Country("PL","Poland","PL"),
   Country("PM","St. Pierre and Miquelon","SB"),
   Country("PN","Pitcairn","PC",
	   (["aka":({"Pitcairn Islands"})])),
   Country("PR","Puerto Rico","RQ"),
   Country("PT","Portugal","PO"),
   Country("PW","Palau","PS"),
   Country("PY","Paraguay","PA"),
   Country("QA","Qatar","QA"),
   Country("RE","Reunion","RE"),
   Country("RO","Romania","RO",(["aka":({"Rumania"})])),
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
   Country("SG","Singapore","SN"),
   Country("SH","St. Helena","SH", // including ascension?
	   (["aka":({"Saint Helena"})])),
   Country("SI","Slovenia","SI"),
   Country("SJ","Svalbard and Jan Mayen Islands","??"),
   Country("??","Svalbard","SV"),
   Country("??","Jan Mayen","JN"),
   Country("SK","Slovak Republic","LO",
	   (["aka":({"Slovakia"})])),
   Country("SL","Sierra Leone","SL"),
   Country("SM","San Marino","SM"),
   Country("SN","Senegal","SG"),
   Country("SO","Somalia","SO"),
   Country("SR","Suriname","NS"),
   Country("ST","Sao Tome and Principe","TP"),
   Country("SU","USSR","??",(["former":1,
			      "_continent":"Asia"])),
   Country("SV","El Salvador","ES"),
   Country("SY","Syria","SY"),
   Country("SZ","Swaziland","WZ"),
   Country("TC","Turks and Caicos Islands","??"),
   Country("TD","Chad","CD"),
   Country("TF","French Southern Territories","??"),
   Country("TG","Togo","TO"),
   Country("TH","Thailand","TH"),
   Country("TJ","Tajikistan","TI"),
   Country("TK","Tokelau","TL"),
   Country("TM","Turkmenistan","TX"),
   Country("TN","Tunisia","TS"),
   Country("TO","Tonga","TN"),
   Country("TP","East Timor","??"), // indonesia?
   Country("TR","Turkey","TU"),
   Country("TT","Trinidad and Tobago","TD"),
   Country("TV","Tuvalu","TV"),
   Country("TW","Taiwan","TW"),
   Country("TZ","Tanzania","TZ"),
   Country("UA","Ukraine","UP"),
   Country("UG","Uganda","UG"),
   Country("US","United States","US"),
   Country("UY","Uruguay","UY"),
   Country("UZ","Uzbekistan","UZ"),
   Country("VA","Vatican City State (Holy See)","VT",
	   (["aka":({"Holy See",
		     "Holy See (Vatican City)"})])),
   Country("VC","Saint Vincent and the Grenadines","VC"),
   Country("VE","Venezuela","VE"),
   Country("VG","Virgin Islands (British)","VI"),
   Country("VI","Virgin Islands (U.S.)","VQ"),
   Country("VN","Viet Nam","VM",(["aka":({"Vietnam"})])),
   Country("VU","Vanuatu","NH"),
   Country("WF","Wallis and Futuna Islands","WF"),
   Country("WS","Samoa","WS",
	   (["aka":({"Western Samoa"})])),
   Country("??","American Samoa","AQ"),
   Country("YE","Yemen","YM"),
   Country("YT","Mayotte","MF"),
   Country("YU","Yugoslavia","??",
	   (["aka":({"Serbia and Montenegro"})])),
   Country("??","Montenegro","MW",
	   (["_continent":"Europe"])),
   Country("??","Serbia","SR",
	   (["_continent":"Europe"])),
   Country("ZA","South Africa","SF"),
   Country("ZM","Zambia","ZA"),
   Country("ZR","Zaire","??"),
   Country("ZW","Zimbabwe","ZI"),
});

//! Country
class Country
{
   string iso2; // iso-2-character-code aka domain name
   //!    ISO 2-character code aka domain name

   string fips10; // iso-2-character-code aka domain name
   //!    FIPS 10-character code; 
   //!    "Federal Information Processing Standards 10-3" etc,
   //!    used by some goverments in the US.

   string name;
   array(string) aka=({});
   //!    Country name and as-known-as, if any

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

   void create(string _iso2,string _name,string _fips10,
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
   string cast(string to)
   {
      if (to[..5]=="string") return name;
      error("can't cast to %O\n",to);
   }

   string _sprintf(int t)
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

static private mapping _from_domain=0;

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
	"UM":_from_domain->US,
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

static private mapping _from_name=0;

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

static private mapping _cached_continents;

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

#if 1
   foreach (countries,Country co)
      if (!co->_continent)
	 werror("no continent: %O %O\n",co->iso2,co);
#endif

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

Country|function(string:Country)|array(Country)|program
   `->(string what)
{
   return
      ::`[](what) ||
      from_domain(what)||
      from_name(what)||
      from_name(replace(what,"_"," "));
}

function `[] =`->;
