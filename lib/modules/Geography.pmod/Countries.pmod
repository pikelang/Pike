#pike __REAL_VERSION__

mapping _continents=
([ 
   "Europe":
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
   Country("AC","Ascension"),
   Country("AD","Andorra"),
   Country("AE","United Arab Emirates"),
   Country("AF","Afghanistan"),
   Country("AG","Antigua and Barbuda"),
   Country("AI","Anguilla"),
   Country("AL","Albania"),
   Country("AM","Armenia"),
   Country("AN","Netherlands Antilles"),
   Country("AO","Angola"),
   Country("AQ","Antarctica"),
   Country("AR","Argentina"),
   Country("AS","American Samoa"),
   Country("AT","Austria"),
   Country("AU","Australia"),
   Country("AW","Aruba"),
   Country("AZ","Azerbaijan"),
   Country("BA","Bosnia and Herzegovina"),
   Country("BB","Barbados"),
   Country("BD","Bangladesh"),
   Country("BE","Belgium"),
   Country("BF","Burkina Faso"),
   Country("BG","Bulgaria"),
   Country("BH","Bahrain"),
   Country("BI","Burundi"),
   Country("BJ","Benin"),
   Country("BM","Bermuda"),
   Country("BN","Brunei Darussalam",
	   (["aka":({"Brunei"})])),
   Country("BO","Bolivia"),
   Country("BR","Brazil"),
   Country("BS","Bahamas"),
   Country("BT","Bhutan"),
   Country("BV","Bouvet Island"),
   Country("BW","Botswana"),
   Country("BY","Belarus"),
   Country("BZ","Belize"),
   Country("CA","Canada"),
   Country("CC","Cocos Islands",(["aka":({"Keeling Islands"})])),
   Country("CF","Central African Republic"),
   Country("CG","Congo",(["aka":({"Congo, Republic of the",
				  "Congo, Democratic Republic of the"})])),
   Country("CH","Switzerland"),
   Country("CI","Cote D'Ivoire",(["aka":({"Ivory Coast"})])),
   Country("CK","Cook Islands"),
   Country("CL","Chile"),
   Country("CM","Cameroon"),
   Country("CN","China"),
   Country("CO","Colombia"),
   Country("CR","Costa Rica"),
   Country("CS","Czechoslovakia",(["former":1,
				   "_continent":"Europe"])),
   Country("CU","Cuba"),
   Country("CV","Cape Verde"),
   Country("CX","Christmas Island"),
   Country("CY","Cyprus"),
   Country("CZ","Czech Republic"),
   Country("DE","Germany"),
   Country("DJ","Djibouti"),
   Country("DK","Denmark"),
   Country("DM","Dominica"),
   Country("DO","Dominican Republic"),
   Country("DZ","Algeria"),
   Country("EC","Ecuador"),
   Country("EE","Estonia"),
   Country("EG","Egypt"),
   Country("EH","Western Sahara"),
   Country("ER","Eritrea"),
   Country("ES","Spain"),
   Country("ET","Ethiopia"),
   Country("FI","Finland"),
   Country("FJ","Fiji"),
   Country("FK","Falkland Islands",(["aka":({"Malvinas"})])),
   Country("FM","Micronesia",(["aka":({"Federated States of Micronesia",
				       "Micronesia, Federated States of"})])),
   Country("FO","Faroe Islands"),
   Country("FR","France"),
   Country("GA","Gabon"),
   Country("GB","United Kingdom",(["aka":({"Great Britain"})])),
   Country("GD","Grenada"),
   Country("GE","Georgia"),
   Country("GF","French Guiana"),
   Country("GH","Ghana"),
   Country("GI","Gibraltar"),
   Country("GL","Greenland"),
   Country("GM","Gambia",
	   (["aka":({"Gambia, The"})])),
   Country("GN","Guinea"),
   Country("GP","Guadeloupe"),
   Country("GQ","Equatorial Guinea"),
   Country("GR","Greece"),
   Country("GS","S. Georgia and S. Sandwich Isls."),
   Country("GT","Guatemala"),
   Country("GU","Guam"),
   Country("GW","Guinea-Bissau"),
   Country("GY","Guyana"),
   Country("HK","Hong Kong"),
   Country("HM","Heard and McDonald Islands"),
   Country("HN","Honduras"),
   Country("HR","Croatia",(["aka":({"Hrvatska"})])),
   Country("HT","Haiti"),
   Country("HU","Hungary"),
   Country("ID","Indonesia"),
   Country("IE","Ireland"),
   Country("IL","Israel"),
   Country("IN","India"),
   Country("IO","British Indian Ocean Territory"),
   Country("IQ","Iraq"),
   Country("IR","Iran"),
   Country("IS","Iceland"),
   Country("IT","Italy"),
   Country("JE","Jersey"),
   Country("JM","Jamaica"),
   Country("JO","Jordan"),
   Country("JP","Japan"),
   Country("KE","Kenya"),
   Country("KG","Kyrgyzstan"),
   Country("KH","Cambodia"),
   Country("KI","Kiribati"),
   Country("KM","Comoros"),
   Country("KN","Saint Kitts and Nevis"),
   Country("KP","North Korea",(["aka":({"Korea, North"})])),
   Country("KR","South Korea",(["aka":({"Korea, South"})])),
   Country("KW","Kuwait"),
   Country("KY","Cayman Islands"),
   Country("KZ","Kazakhstan"),
   Country("LA","Laos"),
   Country("LB","Lebanon"),
   Country("LC","Saint Lucia"),
   Country("LI","Liechtenstein"),
   Country("LK","Sri Lanka"),
   Country("LR","Liberia"),
   Country("LS","Lesotho"),
   Country("LT","Lithuania"),
   Country("LU","Luxembourg"),
   Country("LV","Latvia"),
   Country("LY","Libya"),
   Country("MA","Morocco"),
   Country("MC","Monaco"),
   Country("MD","Moldova"),
   Country("MG","Madagascar"),
   Country("MH","Marshall Islands"),
   Country("MK","Macedonia"),
   Country("ML","Mali"),
   Country("MM","Myanmar",
	   (["aka":({"Burma"})])),
   Country("MN","Mongolia"),
   Country("MO","Macao",(["aka":({"Macau"})])),
   Country("MP","Northern Mariana Islands"),
   Country("MQ","Martinique"),
   Country("MR","Mauritania"),
   Country("MS","Montserrat"),
   Country("MT","Malta"),
   Country("MU","Mauritius"),
   Country("MV","Maldives"),
   Country("MW","Malawi"),
   Country("MX","Mexico"),
   Country("MY","Malaysia"),
   Country("MZ","Mozambique"),
   Country("NA","Namibia"),
   Country("NC","New Caledonia"),
   Country("NE","Niger"),
   Country("NF","Norfolk Island"),
   Country("NG","Nigeria"),
   Country("NI","Nicaragua"),
   Country("NL","Netherlands"),
   Country("NO","Norway"),
   Country("NP","Nepal"),
   Country("NR","Nauru"),
   Country("NT","Neutral Zone",(["_continent":"N/A"])),
   Country("NU","Niue"),
   Country("NZ","New Zealand (Aotearoa)",
	   (["aka":({"Aotearoa","New Zealand","New Zeeland"})])),
   Country("OM","Oman"),
   Country("PA","Panama"),
   Country("PE","Peru"),
   Country("PF","French Polynesia"),
   Country("PG","Papua New Guinea"),
   Country("PH","Philippines"),
   Country("PK","Pakistan"),
   Country("PL","Poland"),
   Country("PM","St. Pierre and Miquelon"),
   Country("PN","Pitcairn"),
   Country("PR","Puerto Rico"),
   Country("PT","Portugal"),
   Country("PW","Palau"),
   Country("PY","Paraguay"),
   Country("QA","Qatar"),
   Country("RE","Reunion"),
   Country("RO","Romania"),
   Country("RU","Russian Federation",
	   (["aka":({"Russia"}),
	     "_continent":"Europe"
	   ])),
   Country("RW","Rwanda"),
   Country("SA","Saudi Arabia"),
   Country("SB","Solomon Islands"),
   Country("SC","Seychelles"),
   Country("SD","Sudan"),
   Country("SE","Sweden"),
   Country("SG","Singapore"),
   Country("SH","St. Helena",
	   (["aka":({"Saint Helena"})])),
   Country("SI","Slovenia"),
   Country("SJ","Svalbard and Jan Mayen Islands"),
   Country("SK","Slovak Republic",
	   (["aka":({"Slovakia"})])),
   Country("SL","Sierra Leone"),
   Country("SM","San Marino"),
   Country("SN","Senegal"),
   Country("SO","Somalia"),
   Country("SR","Suriname"),
   Country("ST","Sao Tome and Principe"),
   Country("SU","USSR",(["former":1,
			 "_continent":"Asia"])),
   Country("SV","El Salvador"),
   Country("SY","Syria"),
   Country("SZ","Swaziland"),
   Country("TC","Turks and Caicos Islands"),
   Country("TD","Chad"),
   Country("TF","French Southern Territories"),
   Country("TG","Togo"),
   Country("TH","Thailand"),
   Country("TJ","Tajikistan"),
   Country("TK","Tokelau"),
   Country("TM","Turkmenistan"),
   Country("TN","Tunisia"),
   Country("TO","Tonga"),
   Country("TP","East Timor"),
   Country("TR","Turkey"),
   Country("TT","Trinidad and Tobago"),
   Country("TV","Tuvalu"),
   Country("TW","Taiwan"),
   Country("TZ","Tanzania"),
   Country("UA","Ukraine"),
   Country("UG","Uganda"),
   Country("US","United States"),
   Country("UY","Uruguay"),
   Country("UZ","Uzbekistan"),
   Country("VA","Vatican City State (Holy See)",
	   (["aka":({"Holy See",
		     "Holy See (Vatican City)"})])),
   Country("VC","Saint Vincent and the Grenadines"),
   Country("VE","Venezuela"),
   Country("VG","Virgin Islands (British)"),
   Country("VI","Virgin Islands (U.S.)"),
   Country("VN","Viet Nam",(["aka":({"Vietnam"})])),
   Country("VU","Vanuatu"),
   Country("WF","Wallis and Futuna Islands"),
   Country("WS","Samoa"),
   Country("YE","Yemen"),
   Country("YT","Mayotte"),
   Country("YU","Yugoslavia",(["aka":({"Serbia and Montenegro"})])),
   Country("ZA","South Africa"),
   Country("ZM","Zambia"),
   Country("ZR","Zaire"),
   Country("ZW","Zimbabwe"),
});

//! Country
class Country
{
   //!    ISO 2-character code aka domain name
   string iso2; // iso-2-character-code aka domain name

   //! @decl string name
   //! @decl array(string) aka
   //!    Country name and also-known-as, if any

   string name;
   array(string) aka=({});

   //!    Flag that is set if this country doesn't exist anymore.
   //!    (eg USSR.)
   int former=0;

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

   void create(string _iso2,string _name,mapping|void opts)
   {
      iso2=_iso2;
      name=_name;
      if (opts)
      {
	 foreach ( (array)opts, [string what,string val])
	    ::`[]=(what,val);
      }
   }

   //! @decl string cast("string")
   //!	It is possible to cast a country to a string,
   //!	which will be the same as performing 
   //!	@code{country->name;@}.
   string cast(string to)
   {
      if (to[..5]=="string") return name;
      error("can't cast to %O\n",to);
   }

   string _sprintf(int t)
   {
      if (t=='O')
	 return "Country("+name+")";
      return 0;
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
//!	And that USA has six domains, Great Britain and france two:
//!	@dl
//!	  @item COM
//!         US Commercial
//!	  @item EDU
//!         US Educational
//!	  @item MIL
//!         US Military
//!	  @item GOV
//!         US Government
//!	  @item UM
//!         US Minor Outlying Islands
//!	  @item GB
//!         Great Britain (UK)
//!	  @item UK
//!         United Kingdom
//!	  @item FR
//!         France
//!	  @item FX
//!	    France, Metropolitan
//!	@enddl

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
//!     @pre{
//! > Geography.Countries.se;    
//! Result: Country(Sweden)
//! > Geography.Countries.djibouti;
//! Result: Country(Djibouti)
//! > Geography.Countries.com;     
//! Result: Country(United States)
//! > Geography.Countries.wallis_and_futuna_islands->iso2;
//! Result: "WF"
//! 	@}

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
