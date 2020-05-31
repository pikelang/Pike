// ----------------------------------------------------------------
// Timezone names
//
// this file is created half-manually
// ----------------------------------------------------------------

//! module Calendar
//! submodule TZnames
//!	This module contains listnings of available timezones,
//!	in some different ways

#pike __REAL_VERSION__

//! method string _zone_tab()
//! method array(array) zone_tab()
//!	This returns the raw respectively parsed zone tab file
//!	from the timezone data files.
//!
//!	The parsed format is an array of zone tab line arrays,
//!	<pre>
//!	({ string country_code,
//!	   string position,
//!	   string zone_name,
//!	   string comment })
//!	</pre>
//!
//!	To convert the position to a Geography.Position, simply
//!	feed it to the constructor.

protected string raw_zone_tab=0;
string _zone_tab()
{
   return raw_zone_tab ||
      (raw_zone_tab = ( master()->master_read_file(
         master()->combine_path_with_cwd(__FILE__,"..","tzdata/zone.tab")) - "\r"));
}

protected array(array(string)) parsed_zone_tab=0;
array(array(string)) zone_tab()
{
   return parsed_zone_tab ||
      (parsed_zone_tab=
       map(_zone_tab()/"\n",
	   lambda(string s)
	   {
	      if (s=="" || s[0]=='#')
		 return 0;
	      else
	      {
		 array v=s/"\t";
		 if (sizeof(v)==3) return v+=({""});
		 else return v;
	      }
	   })
	   -({0}));
}

//! method array(string) zonenames()
//!	This reads the zone.tab file and returns name of all
//!	standard timezones, like "Europe/Belgrade".

protected array(string) zone_names=0;
array(string) zonenames()
{
   return zone_names || (zone_names=column(zone_tab(),2));
}

//! constant mapping(string:array(string)) zones
//!	This constant is a mapping that can be
//!	used to loop over to get all the region-based
//!	timezones.
//!
//!	It looks like this:
//!	<pre>
//!	([ "America": ({ "Los_Angeles", "Chicago", <i>[...]</i> }),
//!	   "Europe":  ({ "Stockholm", <i>[...]</i> }),
//!        <i>[...]</i> }),
//!	</pre>
//!
//!	Please note that loading all the timezones can take some
//!	time, since they are generated and compiled on the fly.

mapping zones =
([
   "America":   ({"Anguilla", "Antigua", "Argentina/ComodRivadavia", "Aruba",
                  "Cayman", "Coral_Harbour", "Dominica", "Ensenada",
                  "Grenada", "Guadeloupe", "Montreal", "Montserrat",
                  "Rosario", "St_Kitts", "St_Lucia", "St_Thomas",
                  "St_Vincent", "Tortola", "Danmarkshavn", "Scoresbysund",
                  "Godthab", "Thule", "New_York", "Chicago",
                  "North_Dakota/Center", "North_Dakota/New_Salem",
                  "North_Dakota/Beulah", "Denver", "Los_Angeles", "Juneau",
                  "Sitka", "Metlakatla", "Yakutat", "Anchorage", "Nome",
                  "Adak", "Phoenix", "Boise", "Indiana/Indianapolis",
                  "Indiana/Marengo", "Indiana/Vincennes", "Indiana/Tell_City",
                  "Indiana/Petersburg", "Indiana/Knox", "Indiana/Winamac",
                  "Indiana/Vevay", "Kentucky/Louisville",
                  "Kentucky/Monticello", "Detroit", "Menominee", "St_Johns",
                  "Goose_Bay", "Halifax", "Glace_Bay", "Moncton",
                  "Blanc-Sablon", "Toronto", "Thunder_Bay", "Nipigon",
                  "Rainy_River", "Atikokan", "Winnipeg", "Regina",
                  "Swift_Current", "Edmonton", "Vancouver", "Dawson_Creek",
                  "Fort_Nelson", "Creston", "Pangnirtung", "Iqaluit",
                  "Resolute", "Rankin_Inlet", "Cambridge_Bay", "Yellowknife",
                  "Inuvik", "Whitehorse", "Dawson", "Cancun", "Merida",
                  "Matamoros", "Monterrey", "Mexico_City", "Ojinaga",
                  "Chihuahua", "Hermosillo", "Mazatlan", "Bahia_Banderas",
                  "Tijuana", "Nassau", "Barbados", "Belize", "Costa_Rica",
                  "Havana", "Santo_Domingo", "El_Salvador", "Guatemala",
                  "Port-au-Prince", "Tegucigalpa", "Jamaica", "Martinique",
                  "Managua", "Panama", "Puerto_Rico", "Miquelon",
                  "Grand_Turk", "Argentina/Buenos_Aires", "Argentina/Cordoba",
                  "Argentina/Salta", "Argentina/Tucuman",
                  "Argentina/La_Rioja", "Argentina/San_Juan",
                  "Argentina/Jujuy", "Argentina/Catamarca",
                  "Argentina/Mendoza", "Argentina/San_Luis",
                  "Argentina/Rio_Gallegos", "Argentina/Ushuaia", "La_Paz",
                  "Noronha", "Belem", "Santarem", "Fortaleza", "Recife",
                  "Araguaina", "Maceio", "Bahia", "Sao_Paulo", "Campo_Grande",
                  "Cuiaba", "Porto_Velho", "Boa_Vista", "Manaus", "Eirunepe",
                  "Rio_Branco", "Santiago", "Punta_Arenas", "Bogota",
                  "Curacao", "Guayaquil", "Cayenne", "Guyana", "Asuncion",
                  "Lima", "Paramaribo", "Port_of_Spain", "Montevideo",
                  "Caracas"}),
   "Pacific":   ({"Fiji", "Gambier", "Marquesas", "Tahiti", "Guam", "Tarawa",
                  "Enderbury", "Kiritimati", "Majuro", "Kwajalein", "Chuuk",
                  "Pohnpei", "Kosrae", "Nauru", "Noumea", "Auckland",
                  "Chatham", "Rarotonga", "Niue", "Norfolk", "Palau",
                  "Port_Moresby", "Bougainville", "Pitcairn", "Pago_Pago",
                  "Apia", "Guadalcanal", "Fakaofo", "Tongatapu", "Funafuti",
                  "Wake", "Efate", "Wallis", "Johnston", "Midway", "Saipan",
                  "Honolulu", "Easter", "Galapagos"}),
   "Antarctica":({"Casey", "Davis", "Mawson", "DumontDUrville", "Syowa",
                  "Troll", "Vostok", "Rothera", "Macquarie", "McMurdo",
                  "Palmer"}),
   "Atlantic":  ({"Cape_Verde", "Jan_Mayen", "St_Helena", "Faroe",
                  "Reykjavik", "Azores", "Madeira", "Canary", "Bermuda",
                  "Stanley", "South_Georgia"}),
   "Indian":    ({"Mauritius", "Reunion", "Mahe", "Kerguelen", "Chagos",
                  "Maldives", "Christmas", "Cocos", "Antananarivo", "Comoro",
                  "Mayotte"}),
   "Europe":    ({"Belfast", "Guernsey", "Isle_of_Man", "Jersey", "Ljubljana",
                  "Sarajevo", "Skopje", "Tiraspol", "Vaduz", "Zagreb",
                  "London", "Dublin", "Tirane", "Andorra", "Vienna", "Minsk",
                  "Brussels", "Sofia", "Prague", "Copenhagen", "Tallinn",
                  "Helsinki", "Paris", "Berlin", "Gibraltar", "Athens",
                  "Budapest", "Rome", "Riga", "Vilnius", "Luxembourg",
                  "Malta", "Chisinau", "Monaco", "Amsterdam", "Oslo",
                  "Warsaw", "Lisbon", "Bucharest", "Kaliningrad", "Moscow",
                  "Simferopol", "Astrakhan", "Volgograd", "Saratov", "Kirov",
                  "Samara", "Ulyanovsk", "Belgrade", "Madrid", "Stockholm",
                  "Zurich", "Istanbul", "Kiev", "Uzhgorod", "Zaporozhye"}),
   "Africa":    ({"Algiers", "Ndjamena", "Abidjan", "Cairo", "Accra",
                  "Bissau", "Nairobi", "Monrovia", "Tripoli", "Casablanca",
                  "El_Aaiun", "Maputo", "Windhoek", "Lagos", "Sao_Tome",
                  "Johannesburg", "Khartoum", "Juba", "Tunis", "Addis_Ababa",
                  "Asmara", "Bamako", "Bangui", "Banjul", "Blantyre",
                  "Brazzaville", "Bujumbura", "Conakry", "Dakar",
                  "Dar_es_Salaam", "Djibouti", "Douala", "Freetown",
                  "Gaborone", "Harare", "Kampala", "Kigali", "Kinshasa",
                  "Libreville", "Lome", "Luanda", "Lubumbashi", "Lusaka",
                  "Malabo", "Maseru", "Mbabane", "Mogadishu", "Niamey",
                  "Nouakchott", "Ouagadougou", "Porto-Novo", "Timbuktu",
                  "Ceuta"}),
   "Asia":      ({"Kabul", "Yerevan", "Baku", "Dhaka", "Thimphu", "Brunei",
                  "Yangon", "Shanghai", "Urumqi", "Hong_Kong", "Taipei",
                  "Macau", "Nicosia", "Famagusta", "Tbilisi", "Dili",
                  "Kolkata", "Jakarta", "Pontianak", "Makassar", "Jayapura",
                  "Tehran", "Baghdad", "Jerusalem", "Tokyo", "Amman",
                  "Almaty", "Qyzylorda", "Qostanay", "Aqtobe", "Aqtau",
                  "Atyrau", "Oral", "Bishkek", "Seoul", "Pyongyang", "Beirut",
                  "Kuala_Lumpur", "Kuching", "Hovd", "Ulaanbaatar",
                  "Choibalsan", "Kathmandu", "Karachi", "Gaza", "Hebron",
                  "Manila", "Qatar", "Riyadh", "Singapore", "Colombo",
                  "Damascus", "Dushanbe", "Bangkok", "Ashgabat", "Dubai",
                  "Samarkand", "Tashkent", "Ho_Chi_Minh", "Aden", "Bahrain",
                  "Chongqing", "Hanoi", "Harbin", "Kashgar", "Kuwait",
                  "Muscat", "Phnom_Penh", "Tel_Aviv", "Vientiane",
                  "Yekaterinburg", "Omsk", "Barnaul", "Novosibirsk", "Tomsk",
                  "Novokuznetsk", "Krasnoyarsk", "Irkutsk", "Chita",
                  "Yakutsk", "Vladivostok", "Khandyga", "Sakhalin", "Magadan",
                  "Srednekolymsk", "Ust-Nera", "Kamchatka", "Anadyr"}),
   "Australia": ({"Darwin", "Perth", "Eucla", "Brisbane", "Lindeman",
                  "Adelaide", "Hobart", "Currie", "Melbourne", "Sydney",
                  "Broken_Hill", "Lord_Howe"}),
]);

// this is used to dwim timezone

//! constant mapping(string:array(string)) abbr2zones
//!	This mapping is used to look up abbreviation
//!	to the possible regional zones.
//!
//!	It looks like this:
//!	<pre>
//!	([ "CET": ({ "Europe/Stockholm", <i>[...]</i> }),
//!	   "CST": ({ "America/Chicago", "Australia/Adelaide", <i>[...]</i> }),
//!        <i>[...]</i> }),
//!	</pre>
//!
//!	Note this: Just because it's noted "CST" doesn't mean it's a
//!	unique timezone. There is about 7 *different* timezones that
//!	uses "CST" as abbreviation; not at the same time,
//!	though, so the DWIM routines checks this before
//!	it's satisfied. Same with some other timezones.
//!
//!     For most timezones, there is a number of region timezones that for the
//!     given time are equal. This is because region timezones include rules
//!     about local summer time shifts and possible historic shifts.
//!
//!	The <ref>YMD.parse</ref> functions can handle timezone abbreviations
//!	by guessing.

mapping abbr2zones =
([
   "": ({"Europe/Amsterdam", "Europe/Moscow"}),
   "%s": ({"Europe/Belfast", "Europe/Guernsey", "Europe/Isle_of_Man",
       "Europe/Jersey"}),
   "+00": ({"Africa/Casablanca", "Africa/El_Aaiun", "America/Scoresbysund",
       "Antarctica/Troll", "Atlantic/Azores", "Atlantic/Madeira",
       "Atlantic/Reykjavik"}),
   "+0020": ({"Africa/Accra", "Europe/Amsterdam"}),
   "+01": ({"Africa/Casablanca", "Africa/El_Aaiun", "Etc/GMT-1",
       "Africa/Freetown", "Atlantic/Madeira"}),
   "+0120": ({"Europe/Amsterdam"}),
   "+0130": ({"Africa/Windhoek"}),
   "+02": ({"Antarctica/Troll", "Etc/GMT-2", "Europe/Ulyanovsk",
       "Europe/Samara"}),
   "+0220": ({"Europe/Zaporozhye"}),
   "+0230": ({"Africa/Mogadishu", "Africa/Kampala", "Africa/Nairobi"}),
   "+0245": ({"Africa/Dar_es_Salaam", "Africa/Nairobi", "Africa/Kampala"}),
   "+03": ({"Antarctica/Syowa", "Asia/Aden", "Asia/Baghdad", "Asia/Bahrain",
       "Asia/Kuwait", "Asia/Qatar", "Asia/Riyadh", "Etc/GMT-3",
       "Europe/Istanbul", "Europe/Kirov", "Europe/Minsk", "Europe/Volgograd",
       "Asia/Famagusta", "Europe/Saratov", "Europe/Astrakhan",
       "Europe/Ulyanovsk", "Europe/Kaliningrad", "Europe/Samara",
       "Asia/Yerevan", "Asia/Baku", "Asia/Tbilisi", "Asia/Atyrau",
       "Asia/Oral"}),
   "+0330": ({"Asia/Tehran"}),
   "+04": ({"Asia/Baghdad", "Asia/Baku", "Asia/Dubai", "Asia/Muscat",
       "Asia/Tbilisi", "Asia/Yerevan", "Etc/GMT-4", "Europe/Astrakhan",
       "Europe/Samara", "Europe/Saratov", "Europe/Ulyanovsk",
       "Europe/Volgograd", "Indian/Mahe", "Indian/Mauritius",
       "Indian/Reunion", "Europe/Kirov", "Asia/Aqtau", "Asia/Atyrau",
       "Asia/Oral", "Asia/Aqtobe", "Asia/Qostanay", "Asia/Yekaterinburg",
       "Asia/Qyzylorda", "Asia/Bahrain", "Asia/Qatar", "Asia/Ashgabat",
       "Asia/Tehran", "Europe/Istanbul", "Asia/Kabul", "Asia/Samarkand"}),
   "+0430": ({"Asia/Kabul", "Asia/Tehran"}),
   "+05": ({"Antarctica/Mawson", "Asia/Aqtau", "Asia/Aqtobe",
       "Asia/Ashgabat", "Asia/Atyrau", "Asia/Baku", "Asia/Dushanbe",
       "Asia/Oral", "Asia/Qyzylorda", "Asia/Samarkand", "Asia/Tashkent",
       "Asia/Yekaterinburg", "Asia/Yerevan", "Etc/GMT-5", "Indian/Kerguelen",
       "Indian/Maldives", "Indian/Mauritius", "Antarctica/Davis",
       "Europe/Samara", "Asia/Qostanay", "Asia/Tbilisi", "Indian/Chagos",
       "Asia/Almaty", "Asia/Omsk", "Europe/Astrakhan", "Europe/Kirov",
       "Europe/Ulyanovsk", "Europe/Saratov", "Europe/Volgograd",
       "Asia/Kashgar", "Asia/Karachi", "Asia/Bishkek", "Asia/Tehran",
       "Europe/Moscow"}),
   "+0530": ({"Asia/Colombo", "Asia/Thimphu", "Asia/Kathmandu",
       "Asia/Karachi", "Asia/Dhaka", "Asia/Kashgar"}),
   "+0545": ({"Asia/Kathmandu"}),
   "+06": ({"Antarctica/Vostok", "Asia/Almaty", "Asia/Bishkek", "Asia/Dhaka",
       "Asia/Omsk", "Asia/Qostanay", "Asia/Thimphu", "Asia/Urumqi",
       "Etc/GMT-6", "Indian/Chagos", "Asia/Qyzylorda", "Asia/Novosibirsk",
       "Asia/Tomsk", "Asia/Barnaul", "Asia/Yekaterinburg",
       "Asia/Novokuznetsk", "Antarctica/Mawson", "Asia/Colombo",
       "Asia/Aqtobe", "Asia/Atyrau", "Asia/Aqtau", "Asia/Oral",
       "Asia/Krasnoyarsk", "Asia/Dushanbe", "Asia/Samarkand", "Asia/Hovd",
       "Asia/Ashgabat", "Asia/Tashkent"}),
   "+0630": ({"Asia/Yangon", "Indian/Cocos", "Asia/Colombo", "Asia/Dhaka",
       "Asia/Karachi", "Asia/Kolkata"}),
   "+07": ({"Antarctica/Davis", "Asia/Bangkok", "Asia/Barnaul", "Asia/Dhaka",
       "Asia/Hanoi", "Asia/Ho_Chi_Minh", "Asia/Hovd", "Asia/Krasnoyarsk",
       "Asia/Novokuznetsk", "Asia/Novosibirsk", "Asia/Phnom_Penh",
       "Asia/Tomsk", "Asia/Vientiane", "Etc/GMT-7", "Indian/Christmas",
       "Asia/Omsk", "Asia/Almaty", "Asia/Qyzylorda", "Asia/Irkutsk",
       "Asia/Bishkek", "Asia/Dushanbe", "Asia/Chongqing", "Asia/Choibalsan",
       "Asia/Ulaanbaatar", "Asia/Tashkent", "Asia/Kuala_Lumpur",
       "Asia/Singapore"}),
   "+0720": ({"Asia/Kuala_Lumpur", "Asia/Singapore", "Asia/Jakarta"}),
   "+0730": ({"Asia/Kuala_Lumpur", "Asia/Singapore", "Asia/Jakarta",
       "Asia/Pontianak", "Asia/Brunei", "Asia/Kuching"}),
   "+08": ({"Antarctica/Casey", "Asia/Brunei", "Asia/Choibalsan",
       "Asia/Hovd", "Asia/Irkutsk", "Asia/Kuala_Lumpur", "Asia/Kuching",
       "Asia/Singapore", "Asia/Ulaanbaatar", "Etc/GMT-8", "Asia/Chita",
       "Asia/Krasnoyarsk", "Asia/Novokuznetsk", "Asia/Dili", "Asia/Khandyga",
       "Asia/Yakutsk", "Asia/Barnaul", "Asia/Novosibirsk", "Asia/Tomsk",
       "Asia/Ho_Chi_Minh", "Asia/Vientiane", "Asia/Hanoi", "Asia/Jakarta",
       "Asia/Pontianak", "Asia/Phnom_Penh", "Asia/Makassar", "Asia/Ust-Nera"}),
   "+0820": ({"Asia/Kuching"}),
   "+0830": ({"Asia/Harbin"}),
   "+0845": ({"Australia/Eucla"}),
   "+09": ({"Asia/Chita", "Asia/Choibalsan", "Asia/Dili", "Asia/Khandyga",
       "Asia/Ulaanbaatar", "Asia/Yakutsk", "Etc/GMT-9", "Pacific/Palau",
       "Asia/Irkutsk", "Asia/Vladivostok", "Asia/Macau", "Asia/Ust-Nera",
       "Pacific/Saipan", "Asia/Harbin", "Asia/Jakarta", "Asia/Makassar",
       "Asia/Pontianak", "Asia/Kuala_Lumpur", "Asia/Kuching",
       "Asia/Singapore", "Pacific/Nauru", "Asia/Hanoi", "Asia/Ho_Chi_Minh",
       "Asia/Phnom_Penh", "Asia/Vientiane", "Asia/Sakhalin",
       "Pacific/Bougainville", "Pacific/Chuuk", "Pacific/Kosrae",
       "Pacific/Pohnpei", "Asia/Yangon", "Asia/Jayapura", "Pacific/Guam",
       "Pacific/Kwajalein", "Pacific/Majuro"}),
   "+0930": ({"Asia/Jayapura"}),
   "+0945": ({"Australia/Eucla"}),
   "+10": ({"Antarctica/DumontDUrville", "Asia/Ust-Nera", "Asia/Vladivostok",
       "Etc/GMT-10", "Pacific/Chuuk", "Pacific/Port_Moresby", "Asia/Magadan",
       "Asia/Sakhalin", "Pacific/Bougainville", "Asia/Chita",
       "Asia/Khandyga", "Asia/Yakutsk", "Pacific/Saipan",
       "Asia/Srednekolymsk", "Asia/Choibalsan", "Asia/Macau",
       "Pacific/Kosrae", "Pacific/Kwajalein", "Pacific/Majuro",
       "Pacific/Pohnpei"}),
   "+1030": ({"Australia/Lord_Howe"}),
   "+11": ({"Antarctica/Macquarie", "Asia/Magadan", "Asia/Sakhalin",
       "Asia/Srednekolymsk", "Australia/Lord_Howe", "Etc/GMT-11",
       "Pacific/Bougainville", "Pacific/Efate", "Pacific/Guadalcanal",
       "Pacific/Kosrae", "Pacific/Norfolk", "Pacific/Noumea",
       "Pacific/Pohnpei", "Antarctica/Casey", "Asia/Ust-Nera",
       "Asia/Vladivostok", "Asia/Khandyga", "Asia/Anadyr", "Asia/Kamchatka",
       "Pacific/Kwajalein", "Pacific/Majuro"}),
   "+1112": ({"Pacific/Norfolk"}),
   "+1130": ({"Pacific/Norfolk", "Pacific/Nauru", "Australia/Lord_Howe"}),
   "+12": ({"Asia/Anadyr", "Asia/Kamchatka", "Etc/GMT-12", "Pacific/Efate",
       "Pacific/Fiji", "Pacific/Funafuti", "Pacific/Kwajalein",
       "Pacific/Majuro", "Pacific/Nauru", "Pacific/Norfolk",
       "Pacific/Noumea", "Pacific/Tarawa", "Pacific/Wake", "Pacific/Wallis",
       "Asia/Magadan", "Asia/Srednekolymsk", "Asia/Ust-Nera",
       "Pacific/Kosrae", "Asia/Sakhalin"}),
   "+1215": ({"Pacific/Chatham"}),
   "+1220": ({"Pacific/Tongatapu"}),
   "+1230": ({"Pacific/Norfolk"}),
   "+1245": ({"Pacific/Chatham"}),
   "+13": ({"Etc/GMT-13", "Pacific/Apia", "Pacific/Enderbury",
       "Pacific/Fakaofo", "Pacific/Fiji", "Pacific/Tongatapu", "Asia/Anadyr",
       "Asia/Kamchatka"}),
   "+1345": ({"Pacific/Chatham"}),
   "+14": ({"Etc/GMT-14", "Pacific/Apia", "Pacific/Kiritimati",
       "Pacific/Tongatapu", "Asia/Anadyr"}),
   "-00": ({"Factory", "Antarctica/Troll", "Antarctica/Rothera",
       "Antarctica/Davis", "Antarctica/Casey", "Antarctica/Palmer",
       "Antarctica/Vostok", "Antarctica/Syowa", "America/Rankin_Inlet",
       "Antarctica/DumontDUrville", "Antarctica/McMurdo",
       "Antarctica/Mawson", "America/Inuvik", "Indian/Kerguelen",
       "Antarctica/Macquarie", "America/Resolute", "America/Iqaluit",
       "America/Yellowknife", "America/Pangnirtung", "America/Cambridge_Bay"}),
   "-0020": ({"Africa/Freetown"}),
   "-01": ({"America/Scoresbysund", "Atlantic/Azores", "Atlantic/Cape_Verde",
       "Atlantic/Jan_Mayen", "Etc/GMT+1", "Africa/El_Aaiun", "Africa/Bissau",
       "Africa/Freetown", "America/Noronha", "Atlantic/Madeira",
       "Atlantic/Reykjavik", "Africa/Banjul", "Africa/Nouakchott",
       "Africa/Bamako", "Africa/Conakry", "Atlantic/Canary", "Africa/Dakar",
       "Africa/Niamey"}),
   "-0130": ({"America/Montevideo"}),
   "-02": ({"America/Argentina/Buenos_Aires", "America/Argentina/Cordoba",
       "America/Argentina/Tucuman", "America/Godthab", "America/Miquelon",
       "America/Montevideo", "America/Noronha", "America/Sao_Paulo",
       "Atlantic/South_Georgia", "Etc/GMT+2", "America/Argentina/Jujuy",
       "America/Argentina/San_Luis", "America/Scoresbysund",
       "Atlantic/Cape_Verde", "America/Araguaina",
       "America/Argentina/Catamarca", "America/Argentina/ComodRivadavia",
       "America/Argentina/La_Rioja", "America/Argentina/Mendoza",
       "America/Argentina/Rio_Gallegos", "America/Argentina/Salta",
       "America/Argentina/San_Juan", "America/Argentina/Ushuaia",
       "America/Bahia", "America/Belem", "America/Danmarkshavn",
       "America/Fortaleza", "America/Maceio", "America/Recife",
       "America/Rosario", "Antarctica/Palmer", "Atlantic/Azores",
       "Atlantic/Stanley"}),
   "-0230": ({"America/Montevideo"}),
   "-03": ({"America/Araguaina", "America/Argentina/Buenos_Aires",
       "America/Argentina/Catamarca", "America/Argentina/ComodRivadavia",
       "America/Argentina/Cordoba", "America/Argentina/Jujuy",
       "America/Argentina/La_Rioja", "America/Argentina/Mendoza",
       "America/Argentina/Rio_Gallegos", "America/Argentina/Salta",
       "America/Argentina/San_Juan", "America/Argentina/San_Luis",
       "America/Argentina/Tucuman", "America/Argentina/Ushuaia",
       "America/Asuncion", "America/Bahia", "America/Belem",
       "America/Campo_Grande", "America/Cayenne", "America/Cuiaba",
       "America/Fortaleza", "America/Godthab", "America/Maceio",
       "America/Miquelon", "America/Montevideo", "America/Paramaribo",
       "America/Punta_Arenas", "America/Recife", "America/Rosario",
       "America/Santarem", "America/Santiago", "America/Sao_Paulo",
       "Antarctica/Palmer", "Antarctica/Rothera", "Atlantic/Stanley",
       "Etc/GMT+3", "America/Guyana", "America/Danmarkshavn",
       "America/Boa_Vista", "America/Manaus", "America/Porto_Velho"}),
   "-0330": ({"America/Paramaribo", "America/Montevideo"}),
   "-0345": ({"America/Guyana"}),
   "-04": ({"America/Asuncion", "America/Boa_Vista", "America/Bogota",
       "America/Campo_Grande", "America/Caracas", "America/Cuiaba",
       "America/Guayaquil", "America/Guyana", "America/La_Paz",
       "America/Lima", "America/Manaus", "America/Porto_Velho",
       "America/Santiago", "Etc/GMT+4", "America/Eirunepe",
       "America/Rio_Branco", "America/Santarem", "America/Argentina/Mendoza",
       "America/Argentina/San_Juan", "America/Argentina/San_Luis",
       "America/Argentina/Catamarca", "America/Argentina/ComodRivadavia",
       "America/Argentina/La_Rioja", "America/Argentina/Rio_Gallegos",
       "America/Argentina/Ushuaia", "America/Argentina/Tucuman",
       "America/Argentina/Cordoba", "America/Argentina/Salta",
       "America/Argentina/Jujuy", "America/Argentina/Buenos_Aires",
       "America/Punta_Arenas", "America/Rosario", "Antarctica/Palmer",
       "Atlantic/Stanley", "America/Cayenne", "America/Montevideo"}),
   "-0430": ({"America/Caracas", "America/Santo_Domingo", "America/Aruba",
       "America/Curacao"}),
   "-05": ({"America/Bogota", "America/Eirunepe", "America/Guayaquil",
       "America/Lima", "America/Rio_Branco", "Etc/GMT+5", "Pacific/Easter",
       "Pacific/Galapagos", "America/Punta_Arenas", "America/Santiago"}),
   "-0530": ({"America/Belize"}),
   "-06": ({"Etc/GMT+6", "Pacific/Easter", "Pacific/Galapagos"}),
   "-07": ({"Etc/GMT+7", "Pacific/Easter"}),
   "-08": ({"Etc/GMT+8", "Pacific/Pitcairn"}),
   "-0830": ({"Pacific/Pitcairn"}),
   "-09": ({"Etc/GMT+9", "Pacific/Gambier"}),
   "-0930": ({"Pacific/Marquesas", "Pacific/Rarotonga"}),
   "-10": ({"Etc/GMT+10", "Pacific/Rarotonga", "Pacific/Tahiti",
       "Pacific/Kiritimati", "Pacific/Apia", "Pacific/Midway"}),
   "-1030": ({"Pacific/Rarotonga"}),
   "-1040": ({"Pacific/Kiritimati"}),
   "-11": ({"Etc/GMT+11", "Pacific/Midway", "Pacific/Niue",
       "Pacific/Fakaofo", "Pacific/Enderbury", "Pacific/Apia"}),
   "-1120": ({"Pacific/Niue"}),
   "-1130": ({"Pacific/Niue", "Pacific/Apia"}),
   "-12": ({"Etc/GMT+12", "Pacific/Kwajalein", "Pacific/Enderbury"}),
   "ACDT": ({"Australia/Adelaide", "Australia/Broken_Hill",
       "Australia/Darwin"}),
   "ACST": ({"Australia/Adelaide", "Australia/Broken_Hill",
       "Australia/Darwin"}),
   "ADDT": ({"America/Goose_Bay", "America/Pangnirtung"}),
   "ADMT": ({"Africa/Addis_Ababa", "Africa/Asmara"}),
   "ADT": ({"America/Barbados", "America/Glace_Bay", "America/Goose_Bay",
       "America/Halifax", "America/Moncton", "America/Thule",
       "Atlantic/Bermuda", "America/Martinique", "America/Anchorage",
       "America/Blanc-Sablon", "America/Pangnirtung", "America/Puerto_Rico"}),
   "AEDT": ({"Australia/Brisbane", "Australia/Currie", "Australia/Hobart",
       "Australia/Lindeman", "Australia/Melbourne", "Australia/Sydney",
       "Antarctica/Macquarie"}),
   "AEST": ({"Australia/Brisbane", "Australia/Currie", "Australia/Hobart",
       "Australia/Lindeman", "Australia/Melbourne", "Australia/Sydney",
       "Australia/Lord_Howe", "Antarctica/Macquarie",
       "Australia/Broken_Hill"}),
   "AHDT": ({"America/Adak", "America/Anchorage"}),
   "AHPT": ({"America/Adak", "America/Anchorage"}),
   "AHST": ({"America/Adak", "America/Anchorage"}),
   "AHWT": ({"America/Adak", "America/Anchorage"}),
   "AKDT": ({"America/Anchorage", "America/Juneau", "America/Metlakatla",
       "America/Nome", "America/Sitka", "America/Yakutat"}),
   "AKPT": ({"America/Anchorage", "America/Juneau", "America/Metlakatla",
       "America/Nome", "America/Sitka", "America/Yakutat"}),
   "AKST": ({"America/Anchorage", "America/Juneau", "America/Metlakatla",
       "America/Nome", "America/Sitka", "America/Yakutat"}),
   "AKWT": ({"America/Anchorage", "America/Juneau", "America/Metlakatla",
       "America/Nome", "America/Sitka", "America/Yakutat"}),
   "AMT": ({"Europe/Amsterdam", "America/Asuncion", "Europe/Athens",
       "Africa/Asmara"}),
   "APT": ({"America/Glace_Bay", "America/Goose_Bay", "America/Halifax",
       "America/Moncton", "Atlantic/Bermuda", "America/Anchorage",
       "America/Blanc-Sablon", "America/Pangnirtung", "America/Puerto_Rico"}),
   "AST": ({"America/Anguilla", "America/Antigua", "America/Aruba",
       "America/Barbados", "America/Blanc-Sablon", "America/Curacao",
       "America/Dominica", "America/Glace_Bay", "America/Goose_Bay",
       "America/Grenada", "America/Guadeloupe", "America/Halifax",
       "America/Martinique", "America/Moncton", "America/Montserrat",
       "America/Port_of_Spain", "America/Puerto_Rico",
       "America/Santo_Domingo", "America/St_Kitts", "America/St_Lucia",
       "America/St_Thomas", "America/St_Vincent", "America/Thule",
       "America/Tortola", "Atlantic/Bermuda", "America/Grand_Turk",
       "America/Miquelon", "America/Anchorage", "America/Pangnirtung"}),
   "AWDT": ({"Australia/Perth"}),
   "AWST": ({"Australia/Perth"}),
   "AWT": ({"America/Glace_Bay", "America/Goose_Bay", "America/Halifax",
       "America/Moncton", "Atlantic/Bermuda", "America/Anchorage",
       "America/Blanc-Sablon", "America/Pangnirtung", "America/Puerto_Rico"}),
   "BDST": ({"Europe/Dublin", "Europe/Gibraltar", "Europe/London"}),
   "BDT": ({"America/Adak", "America/Nome"}),
   "BMT": ({"Africa/Banjul", "America/Barbados", "Europe/Bucharest",
       "Europe/Chisinau", "Europe/Tiraspol", "Asia/Jakarta", "Asia/Bangkok",
       "Asia/Baghdad", "America/Bogota", "Europe/Zurich", "Europe/Brussels"}),
   "BPT": ({"America/Adak", "America/Nome"}),
   "BST": ({"Europe/Belfast", "Europe/Guernsey", "Europe/Isle_of_Man",
       "Europe/Jersey", "Europe/London", "America/Adak", "America/Nome",
       "Europe/Dublin", "Europe/Gibraltar", "America/La_Paz"}),
   "BWT": ({"America/Adak", "America/Nome"}),
   "CAST": ({"Africa/Juba", "Africa/Khartoum", "Africa/Gaborone"}),
   "CAT": ({"Africa/Blantyre", "Africa/Bujumbura", "Africa/Gaborone",
       "Africa/Harare", "Africa/Khartoum", "Africa/Kigali",
       "Africa/Lubumbashi", "Africa/Lusaka", "Africa/Maputo",
       "Africa/Windhoek", "Africa/Juba"}),
   "CDDT": ({"America/Rankin_Inlet", "America/Resolute"}),
   "CDT": ({"America/Bahia_Banderas", "America/Belize", "America/Chicago",
       "America/Costa_Rica", "America/El_Salvador", "America/Guatemala",
       "America/Havana", "America/Indiana/Knox", "America/Indiana/Tell_City",
       "America/Managua", "America/Matamoros", "America/Menominee",
       "America/Merida", "America/Mexico_City", "America/Monterrey",
       "America/North_Dakota/Beulah", "America/North_Dakota/Center",
       "America/North_Dakota/New_Salem", "America/Rainy_River",
       "America/Rankin_Inlet", "America/Resolute", "America/Tegucigalpa",
       "America/Winnipeg", "Asia/Chongqing", "Asia/Harbin", "Asia/Kashgar",
       "Asia/Macau", "Asia/Shanghai", "Asia/Taipei", "CST6CDT",
       "America/Indiana/Marengo", "America/Kentucky/Louisville",
       "America/Atikokan", "America/Cambridge_Bay", "America/Cancun",
       "America/Chihuahua", "America/Indiana/Indianapolis",
       "America/Indiana/Petersburg", "America/Indiana/Vevay",
       "America/Indiana/Vincennes", "America/Indiana/Winamac",
       "America/Iqaluit", "America/Kentucky/Monticello", "America/Ojinaga",
       "America/Pangnirtung"}),
   "CE%sT": ({"Europe/Ljubljana", "Europe/Sarajevo", "Europe/Skopje",
       "Europe/Vaduz", "Europe/Zagreb", "Europe/Guernsey", "Europe/Jersey",
       "Europe/Tiraspol"}),
   "CEMT": ({"Europe/Berlin", "Europe/Madrid", "Europe/Monaco",
       "Europe/Paris"}),
   "CEST": ({"Africa/Ceuta", "Africa/Tunis", "CET", "Europe/Amsterdam",
       "Europe/Andorra", "Europe/Belgrade", "Europe/Berlin",
       "Europe/Brussels", "Europe/Budapest", "Europe/Copenhagen",
       "Europe/Gibraltar", "Europe/Luxembourg", "Europe/Madrid",
       "Europe/Malta", "Europe/Monaco", "Europe/Oslo", "Europe/Paris",
       "Europe/Prague", "Europe/Rome", "Europe/Stockholm", "Europe/Tirane",
       "Europe/Vienna", "Europe/Warsaw", "Europe/Zurich", "Europe/Vilnius",
       "Europe/Lisbon", "Africa/Algiers", "Africa/Tripoli", "Europe/Athens",
       "Europe/Chisinau", "Europe/Kaliningrad", "Europe/Kiev",
       "Europe/Minsk", "Europe/Riga", "Europe/Simferopol", "Europe/Sofia",
       "Europe/Tallinn", "Europe/Uzhgorod", "Europe/Zaporozhye",
       "Europe/Ljubljana", "Europe/Sarajevo", "Europe/Skopje",
       "Europe/Zagreb"}),
   "CET": ({"Africa/Algiers", "Africa/Ceuta", "Africa/Tunis", "CET",
       "Europe/Amsterdam", "Europe/Andorra", "Europe/Belgrade",
       "Europe/Berlin", "Europe/Brussels", "Europe/Budapest",
       "Europe/Copenhagen", "Europe/Gibraltar", "Europe/Luxembourg",
       "Europe/Madrid", "Europe/Malta", "Europe/Monaco", "Europe/Oslo",
       "Europe/Paris", "Europe/Prague", "Europe/Rome", "Europe/Stockholm",
       "Europe/Tirane", "Europe/Vienna", "Europe/Warsaw", "Europe/Zurich",
       "Europe/Vilnius", "Europe/Lisbon", "Europe/Uzhgorod",
       "Europe/Ljubljana", "Europe/Sarajevo", "Europe/Skopje",
       "Europe/Zagreb", "Africa/Tripoli", "Europe/Athens", "Europe/Chisinau",
       "Europe/Kaliningrad", "Europe/Kiev", "Europe/Minsk", "Europe/Riga",
       "Europe/Simferopol", "Europe/Sofia", "Europe/Tallinn",
       "Europe/Zaporozhye"}),
   "CMT": ({"America/La_Paz", "America/Argentina/Buenos_Aires",
       "America/Argentina/Catamarca", "America/Argentina/ComodRivadavia",
       "America/Argentina/Cordoba", "America/Argentina/Jujuy",
       "America/Argentina/La_Rioja", "America/Argentina/Mendoza",
       "America/Argentina/Rio_Gallegos", "America/Argentina/Salta",
       "America/Argentina/San_Juan", "America/Argentina/San_Luis",
       "America/Argentina/Tucuman", "America/Argentina/Ushuaia",
       "America/Rosario", "Europe/Chisinau", "Europe/Tiraspol",
       "America/Caracas", "America/St_Lucia", "America/Panama",
       "Europe/Copenhagen"}),
   "CPT": ({"America/Chicago", "America/Indiana/Knox",
       "America/Indiana/Tell_City", "America/Matamoros", "America/Menominee",
       "America/North_Dakota/Beulah", "America/North_Dakota/Center",
       "America/North_Dakota/New_Salem", "America/Rainy_River",
       "America/Rankin_Inlet", "America/Resolute", "America/Winnipeg",
       "CST6CDT", "America/Atikokan", "America/Cambridge_Bay",
       "America/Indiana/Indianapolis", "America/Indiana/Marengo",
       "America/Indiana/Petersburg", "America/Indiana/Vevay",
       "America/Indiana/Vincennes", "America/Indiana/Winamac",
       "America/Iqaluit", "America/Kentucky/Louisville",
       "America/Kentucky/Monticello", "America/Monterrey",
       "America/Pangnirtung"}),
   "CST": ({"America/Bahia_Banderas", "America/Belize", "America/Chicago",
       "America/Costa_Rica", "America/El_Salvador", "America/Guatemala",
       "America/Havana", "America/Indiana/Knox", "America/Indiana/Tell_City",
       "America/Managua", "America/Matamoros", "America/Menominee",
       "America/Merida", "America/Mexico_City", "America/Monterrey",
       "America/North_Dakota/Beulah", "America/North_Dakota/Center",
       "America/North_Dakota/New_Salem", "America/Rainy_River",
       "America/Rankin_Inlet", "America/Regina", "America/Resolute",
       "America/Swift_Current", "America/Tegucigalpa", "America/Winnipeg",
       "Asia/Chongqing", "Asia/Harbin", "Asia/Kashgar", "Asia/Macau",
       "Asia/Shanghai", "Asia/Taipei", "CST6CDT", "America/Cambridge_Bay",
       "America/Chihuahua", "America/Ojinaga", "America/Cancun",
       "America/Atikokan", "America/Indiana/Indianapolis",
       "America/Indiana/Marengo", "America/Indiana/Petersburg",
       "America/Indiana/Vevay", "America/Indiana/Vincennes",
       "America/Indiana/Winamac", "America/Iqaluit",
       "America/Kentucky/Louisville", "America/Kentucky/Monticello",
       "America/Pangnirtung", "America/Hermosillo", "America/Mazatlan",
       "America/Detroit", "America/Thunder_Bay"}),
   "CT": ({"Asia/Macau"}),
   "CWT": ({"America/Bahia_Banderas", "America/Chicago",
       "America/Indiana/Knox", "America/Indiana/Tell_City",
       "America/Matamoros", "America/Menominee", "America/Merida",
       "America/Mexico_City", "America/Monterrey",
       "America/North_Dakota/Beulah", "America/North_Dakota/Center",
       "America/North_Dakota/New_Salem", "America/Rainy_River",
       "America/Rankin_Inlet", "America/Resolute", "America/Winnipeg",
       "CST6CDT", "America/Atikokan", "America/Cambridge_Bay",
       "America/Cancun", "America/Chihuahua", "America/Indiana/Indianapolis",
       "America/Indiana/Marengo", "America/Indiana/Petersburg",
       "America/Indiana/Vevay", "America/Indiana/Vincennes",
       "America/Indiana/Winamac", "America/Iqaluit",
       "America/Kentucky/Louisville", "America/Kentucky/Monticello",
       "America/Ojinaga", "America/Pangnirtung"}),
   "ChST": ({"Pacific/Guam", "Pacific/Saipan"}),
   "DFT": ({"Europe/Oslo", "Europe/Paris"}),
   "DMT": ({"Europe/Belfast", "Europe/Dublin"}),
   "E%sT": ({"America/Montreal", "America/Coral_Harbour"}),
   "EAST": ({"Indian/Antananarivo"}),
   "EAT": ({"Africa/Addis_Ababa", "Africa/Asmara", "Africa/Dar_es_Salaam",
       "Africa/Djibouti", "Africa/Juba", "Africa/Kampala",
       "Africa/Mogadishu", "Africa/Nairobi", "Indian/Antananarivo",
       "Indian/Comoro", "Indian/Mayotte", "Africa/Khartoum"}),
   "EDDT": ({"America/Iqaluit"}),
   "EDT": ({"America/Detroit", "America/Grand_Turk",
       "America/Indiana/Indianapolis", "America/Indiana/Marengo",
       "America/Indiana/Petersburg", "America/Indiana/Vevay",
       "America/Indiana/Vincennes", "America/Indiana/Winamac",
       "America/Iqaluit", "America/Kentucky/Louisville",
       "America/Kentucky/Monticello", "America/Nassau", "America/New_York",
       "America/Nipigon", "America/Pangnirtung", "America/Port-au-Prince",
       "America/Thunder_Bay", "America/Toronto", "EST5EDT", "America/Cancun",
       "America/Indiana/Tell_City", "America/Jamaica", "America/Montreal",
       "America/Santo_Domingo"}),
   "EE%sT": ({"Europe/Tiraspol"}),
   "EEST": ({"Africa/Cairo", "Asia/Amman", "Asia/Beirut", "Asia/Damascus",
       "Asia/Famagusta", "Asia/Gaza", "Asia/Hebron", "Asia/Nicosia", "EET",
       "Europe/Athens", "Europe/Bucharest", "Europe/Chisinau",
       "Europe/Helsinki", "Europe/Kiev", "Europe/Riga", "Europe/Sofia",
       "Europe/Tallinn", "Europe/Uzhgorod", "Europe/Vilnius",
       "Europe/Zaporozhye", "Europe/Istanbul", "Europe/Kaliningrad",
       "Europe/Minsk", "Europe/Moscow", "Europe/Simferopol",
       "Europe/Tiraspol", "Europe/Warsaw"}),
   "EET": ({"Africa/Cairo", "Africa/Tripoli", "Asia/Amman", "Asia/Beirut",
       "Asia/Damascus", "Asia/Famagusta", "Asia/Gaza", "Asia/Hebron",
       "Asia/Nicosia", "EET", "Europe/Athens", "Europe/Bucharest",
       "Europe/Chisinau", "Europe/Helsinki", "Europe/Kaliningrad",
       "Europe/Kiev", "Europe/Riga", "Europe/Sofia", "Europe/Tallinn",
       "Europe/Uzhgorod", "Europe/Vilnius", "Europe/Zaporozhye",
       "Europe/Istanbul", "Europe/Minsk", "Europe/Moscow",
       "Europe/Simferopol", "Europe/Tiraspol", "Europe/Warsaw"}),
   "EMT": ({"Pacific/Easter"}),
   "EPT": ({"America/Detroit", "America/Grand_Turk",
       "America/Indiana/Indianapolis", "America/Indiana/Marengo",
       "America/Indiana/Petersburg", "America/Indiana/Vevay",
       "America/Indiana/Vincennes", "America/Indiana/Winamac",
       "America/Iqaluit", "America/Kentucky/Louisville",
       "America/Kentucky/Monticello", "America/Nassau", "America/New_York",
       "America/Nipigon", "America/Pangnirtung", "America/Thunder_Bay",
       "America/Toronto", "EST5EDT", "America/Indiana/Tell_City",
       "America/Jamaica", "America/Santo_Domingo"}),
   "EST": ({"America/Atikokan", "America/Cancun", "America/Cayman",
       "America/Coral_Harbour", "America/Detroit", "America/Grand_Turk",
       "America/Indiana/Indianapolis", "America/Indiana/Marengo",
       "America/Indiana/Petersburg", "America/Indiana/Vevay",
       "America/Indiana/Vincennes", "America/Indiana/Winamac",
       "America/Iqaluit", "America/Jamaica", "America/Kentucky/Louisville",
       "America/Kentucky/Monticello", "America/Nassau", "America/New_York",
       "America/Nipigon", "America/Panama", "America/Pangnirtung",
       "America/Port-au-Prince", "America/Thunder_Bay", "America/Toronto",
       "EST", "EST5EDT", "America/Resolute", "America/Indiana/Knox",
       "America/Indiana/Tell_City", "America/Rankin_Inlet",
       "America/Cambridge_Bay", "America/Managua", "America/Merida",
       "America/Menominee", "America/Montreal", "America/Santo_Domingo",
       "America/Antigua", "America/Chicago", "America/Moncton"}),
   "EWT": ({"America/Detroit", "America/Grand_Turk",
       "America/Indiana/Indianapolis", "America/Indiana/Marengo",
       "America/Indiana/Petersburg", "America/Indiana/Vevay",
       "America/Indiana/Vincennes", "America/Indiana/Winamac",
       "America/Iqaluit", "America/Kentucky/Louisville",
       "America/Kentucky/Monticello", "America/Nassau", "America/New_York",
       "America/Nipigon", "America/Pangnirtung", "America/Thunder_Bay",
       "America/Toronto", "EST5EDT", "America/Cancun",
       "America/Indiana/Tell_City", "America/Jamaica",
       "America/Santo_Domingo"}),
   "FFMT": ({"America/Martinique"}),
   "FMT": ({"Africa/Freetown", "Atlantic/Madeira"}),
   "GDT": ({"Pacific/Guam"}),
   "GMT": ({"Africa/Abidjan", "Africa/Accra", "Africa/Bamako",
       "Africa/Banjul", "Africa/Bissau", "Africa/Conakry", "Africa/Dakar",
       "Africa/Lome", "Africa/Monrovia", "Africa/Nouakchott",
       "Africa/Ouagadougou", "Africa/Sao_Tome", "Africa/Timbuktu",
       "America/Danmarkshavn", "Atlantic/Reykjavik", "Atlantic/St_Helena",
       "Etc/GMT", "Europe/Belfast", "Europe/Dublin", "Europe/Guernsey",
       "Europe/Isle_of_Man", "Europe/Jersey", "Europe/London",
       "Africa/Freetown", "Europe/Gibraltar", "Africa/Malabo",
       "Africa/Niamey", "Europe/Prague", "Africa/Porto-Novo"}),
   "GST": ({"Pacific/Guam"}),
   "HDT": ({"America/Adak", "Pacific/Honolulu"}),
   "HKST": ({"Asia/Hong_Kong"}),
   "HKT": ({"Asia/Hong_Kong"}),
   "HKWT": ({"Asia/Hong_Kong"}),
   "HMT": ({"Asia/Dhaka", "America/Havana", "Europe/Helsinki",
       "Atlantic/Azores", "Asia/Kolkata"}),
   "HPT": ({"America/Adak", "Pacific/Honolulu"}),
   "HST": ({"America/Adak", "HST", "Pacific/Honolulu", "Pacific/Johnston"}),
   "HWT": ({"America/Adak", "Pacific/Honolulu"}),
   "IDDT": ({"Asia/Jerusalem", "Asia/Tel_Aviv", "Asia/Gaza", "Asia/Hebron"}),
   "IDT": ({"Asia/Jerusalem", "Asia/Tel_Aviv", "Asia/Gaza", "Asia/Hebron"}),
   "IMT": ({"Asia/Irkutsk", "Europe/Istanbul", "Europe/Sofia"}),
   "IST": ({"Asia/Jerusalem", "Asia/Kolkata", "Asia/Tel_Aviv",
       "Europe/Dublin", "Asia/Gaza", "Asia/Hebron", "Europe/Belfast"}),
   "JDT": ({"Asia/Tokyo"}),
   "JMT": ({"Atlantic/St_Helena", "Asia/Jerusalem", "Asia/Tel_Aviv"}),
   "JST": ({"Asia/Tokyo", "Asia/Hong_Kong", "Asia/Taipei", "Asia/Seoul",
       "Asia/Pyongyang", "Asia/Manila"}),
   "KDT": ({"Asia/Seoul"}),
   "KMT": ({"Europe/Kiev", "Europe/Vilnius", "America/Cayman",
       "America/Grand_Turk", "America/Jamaica", "America/St_Vincent"}),
   "KST": ({"Asia/Pyongyang", "Asia/Seoul"}),
   "LMT": ({"Asia/Aden", "Asia/Kuwait", "Asia/Thimphu", "Asia/Riyadh",
       "Africa/Kigali", "Africa/El_Aaiun", "Asia/Jayapura",
       "Pacific/Galapagos", "Africa/Juba", "Africa/Khartoum", "Asia/Amman",
       "Africa/Dar_es_Salaam", "Atlantic/Bermuda", "Africa/Kampala",
       "Africa/Nairobi", "Asia/Kashgar", "Asia/Urumqi", "Asia/Chongqing",
       "Asia/Harbin", "Asia/Kuching", "Asia/Brunei", "Asia/Yerevan",
       "Asia/Baku", "Asia/Aqtau", "Asia/Oral", "Asia/Atyrau", "Asia/Aqtobe",
       "Asia/Ashgabat", "Asia/Qostanay", "Asia/Qyzylorda", "Asia/Samarkand",
       "Asia/Dushanbe", "Asia/Tashkent", "Asia/Bishkek", "Asia/Almaty",
       "Asia/Magadan", "Asia/Srednekolymsk", "Asia/Anadyr",
       "Europe/Astrakhan", "Asia/Novokuznetsk", "America/Barbados",
       "Asia/Vladivostok", "Asia/Kamchatka", "Atlantic/Canary",
       "America/Ensenada", "America/Tijuana", "America/Bahia_Banderas",
       "America/Chihuahua", "America/Hermosillo", "America/Mazatlan",
       "America/Mexico_City", "America/Ojinaga", "America/Cancun",
       "America/Matamoros", "America/Merida", "America/Monterrey",
       "Asia/Nicosia", "Asia/Famagusta", "America/Tegucigalpa",
       "Pacific/Nauru", "America/El_Salvador", "Asia/Krasnoyarsk",
       "Europe/Volgograd", "Africa/Tripoli", "Asia/Damascus", "Asia/Bahrain",
       "Asia/Qatar", "Asia/Dubai", "Asia/Muscat", "Asia/Kathmandu",
       "Asia/Makassar", "Asia/Tomsk", "Asia/Chita", "Asia/Yakutsk",
       "Asia/Khandyga", "Asia/Ust-Nera", "Asia/Novosibirsk", "Asia/Barnaul",
       "Asia/Omsk", "Africa/Lagos", "Europe/Kirov", "Europe/Samara",
       "Europe/Saratov", "Europe/Ulyanovsk", "America/Guatemala",
       "Africa/Accra", "America/Thule", "America/Godthab",
       "America/Scoresbysund", "America/Danmarkshavn", "Asia/Yekaterinburg",
       "Asia/Tehran", "Pacific/Fiji", "America/Guyana", "America/Eirunepe",
       "America/Rio_Branco", "America/Porto_Velho", "America/Boa_Vista",
       "America/Manaus", "America/Cuiaba", "America/Santarem",
       "America/Campo_Grande", "America/Belem", "America/Araguaina",
       "America/Sao_Paulo", "America/Bahia", "America/Fortaleza",
       "America/Maceio", "America/Recife", "America/Noronha",
       "Europe/Tirane", "Africa/Casablanca", "Europe/Guernsey",
       "Pacific/Tahiti", "Pacific/Marquesas", "Pacific/Gambier",
       "Pacific/Guadalcanal", "America/Belize", "America/Nassau",
       "America/Anguilla", "America/St_Kitts", "America/Antigua",
       "America/Port_of_Spain", "America/Aruba", "America/Curacao",
       "Pacific/Noumea", "Pacific/Efate", "Atlantic/Cape_Verde",
       "Africa/Dakar", "Africa/Banjul", "Africa/Nouakchott", "Africa/Bissau",
       "Africa/Conakry", "Africa/Bamako", "Africa/Abidjan",
       "Africa/Timbuktu", "Africa/Ouagadougou", "Africa/Sao_Tome",
       "Europe/Lisbon", "Africa/Niamey", "Africa/Porto-Novo",
       "Africa/Malabo", "Africa/Libreville", "Africa/Douala",
       "Africa/Luanda", "Africa/Ndjamena", "Africa/Brazzaville",
       "Africa/Bangui", "Asia/Dili", "America/St_Thomas", "America/Tortola",
       "America/Montserrat", "America/Grenada", "America/Dominica",
       "America/Cayenne", "Africa/Djibouti", "Indian/Comoro",
       "Indian/Mayotte", "Indian/Antananarivo", "America/Guadeloupe",
       "Indian/Reunion", "America/Miquelon", "Pacific/Apia",
       "Pacific/Pago_Pago", "America/Paramaribo", "America/Lima",
       "America/Montevideo", "Asia/Pontianak", "Asia/Pyongyang",
       "Asia/Seoul", "Atlantic/Faroe", "Atlantic/Reykjavik",
       "Indian/Mauritius", "Asia/Karachi", "Indian/Chagos",
       "America/Edmonton", "Asia/Vientiane", "Asia/Phnom_Penh", "Asia/Hanoi",
       "Asia/Ho_Chi_Minh", "Indian/Mahe", "America/Swift_Current",
       "America/Regina", "Asia/Sakhalin", "Asia/Hovd", "Asia/Ulaanbaatar",
       "Asia/Choibalsan", "America/Detroit", "Asia/Hong_Kong", "Asia/Macau",
       "Europe/Luxembourg", "Africa/Maseru", "Africa/Lusaka",
       "Africa/Harare", "Africa/Mbabane", "Africa/Maputo", "Africa/Blantyre",
       "America/Halifax", "America/Glace_Bay", "Pacific/Midway",
       "Pacific/Fakaofo", "Pacific/Enderbury", "Pacific/Niue",
       "Pacific/Rarotonga", "Pacific/Kiritimati", "Pacific/Pitcairn",
       "Africa/Ceuta", "Europe/Madrid", "Europe/Andorra",
       "Asia/Kuala_Lumpur", "Asia/Singapore", "Asia/Shanghai",
       "Pacific/Palau", "Pacific/Guam", "Pacific/Saipan", "Pacific/Chuuk",
       "Pacific/Pohnpei", "Pacific/Kosrae", "Pacific/Wake",
       "Pacific/Kwajalein", "Pacific/Norfolk", "Pacific/Majuro",
       "Pacific/Tarawa", "Pacific/Funafuti", "Pacific/Wallis",
       "Pacific/Tongatapu", "Africa/Cairo", "Asia/Gaza", "Asia/Hebron",
       "America/Adak", "America/Nome", "America/Anchorage",
       "America/Yakutat", "America/Sitka", "America/Juneau",
       "America/Metlakatla", "America/Dawson", "America/Whitehorse",
       "Indian/Cocos", "Asia/Manila", "America/Puerto_Rico", "Europe/Jersey",
       "Africa/Kinshasa", "Africa/Lubumbashi", "Pacific/Honolulu",
       "Asia/Taipei", "Australia/Perth", "Australia/Eucla", "Europe/Athens",
       "Australia/Currie", "Australia/Hobart", "Indian/Christmas",
       "Australia/Darwin", "Australia/Adelaide", "Australia/Broken_Hill",
       "Australia/Melbourne", "Australia/Sydney", "Australia/Lord_Howe",
       "America/Rainy_River", "America/Atikokan", "America/Thunder_Bay",
       "America/Nipigon", "America/Toronto", "Europe/Oslo",
       "Australia/Lindeman", "Australia/Brisbane", "America/Rosario",
       "America/Argentina/Rio_Gallegos", "America/Argentina/Mendoza",
       "America/Argentina/San_Juan", "America/Argentina/Ushuaia",
       "America/Argentina/ComodRivadavia", "America/Argentina/La_Rioja",
       "America/Argentina/San_Luis", "America/Argentina/Catamarca",
       "America/Argentina/Salta", "America/Argentina/Jujuy",
       "America/Argentina/Tucuman", "America/Argentina/Cordoba",
       "America/Argentina/Buenos_Aires", "Europe/Vaduz", "Europe/Malta",
       "Africa/Mogadishu", "Europe/Berlin", "Europe/Vienna",
       "Europe/Kaliningrad", "Africa/Lome", "Africa/Windhoek",
       "Africa/Johannesburg", "Europe/Bucharest", "Europe/Paris",
       "Africa/Algiers", "Europe/Monaco", "Europe/Budapest",
       "Europe/Uzhgorod", "Pacific/Easter", "America/Managua",
       "America/Costa_Rica", "America/Havana", "America/Cayman",
       "America/Guayaquil", "America/Panama", "America/Jamaica",
       "America/Port-au-Prince", "America/Grand_Turk",
       "America/Punta_Arenas", "America/Santiago", "America/Santo_Domingo",
       "America/La_Paz", "America/Caracas", "America/St_Vincent",
       "America/Martinique", "America/St_Lucia", "Atlantic/Stanley",
       "America/Asuncion", "Atlantic/South_Georgia", "Atlantic/St_Helena",
       "Europe/Copenhagen", "Africa/Bujumbura", "Asia/Baghdad", "Asia/Kabul",
       "Asia/Dhaka", "Asia/Tokyo", "America/Winnipeg", "America/Menominee",
       "Africa/Gaborone", "America/Bogota", "America/Vancouver",
       "America/Fort_Nelson", "America/Dawson_Creek", "America/Creston",
       "America/Coral_Harbour", "America/Montreal", "America/Goose_Bay",
       "America/Blanc-Sablon", "America/St_Johns", "Atlantic/Azores",
       "Atlantic/Madeira", "Europe/Ljubljana", "Europe/Zagreb",
       "Europe/Sarajevo", "Europe/Belgrade", "Europe/Skopje",
       "America/Moncton", "America/Boise", "America/Los_Angeles",
       "America/Denver", "America/North_Dakota/Beulah",
       "America/North_Dakota/Center", "America/North_Dakota/New_Salem",
       "America/Phoenix", "America/Chicago", "America/Indiana/Indianapolis",
       "America/Indiana/Knox", "America/Indiana/Marengo",
       "America/Indiana/Petersburg", "America/Indiana/Tell_City",
       "America/Indiana/Vevay", "America/Indiana/Vincennes",
       "America/Indiana/Winamac", "America/Kentucky/Louisville",
       "America/Kentucky/Monticello", "America/New_York",
       "Europe/Isle_of_Man", "Africa/Freetown", "Africa/Monrovia",
       "Africa/Tunis", "Europe/Dublin", "Europe/Belfast", "Europe/Gibraltar",
       "Europe/Brussels", "Europe/Warsaw", "Europe/Sofia", "Europe/Riga",
       "Europe/Tallinn", "Europe/Vilnius", "Europe/Minsk", "Europe/Chisinau",
       "Europe/Istanbul", "Europe/Tiraspol", "Europe/Kiev",
       "Europe/Simferopol", "Asia/Tel_Aviv", "Europe/Zaporozhye",
       "Asia/Jerusalem", "Asia/Beirut", "Europe/Moscow", "Asia/Tbilisi",
       "Indian/Maldives", "Asia/Colombo", "Asia/Yangon", "Asia/Bangkok",
       "Asia/Irkutsk", "Pacific/Port_Moresby", "Pacific/Bougainville",
       "Europe/Stockholm", "Europe/Helsinki", "Africa/Addis_Ababa",
       "Africa/Asmara", "Pacific/Auckland", "Pacific/Chatham",
       "Asia/Jakarta", "Europe/Rome", "Asia/Kolkata", "Europe/Zurich",
       "Europe/Prague", "Europe/London", "Europe/Amsterdam"}),
   "LST": ({"Europe/Riga"}),
   "MDDT": ({"America/Cambridge_Bay", "America/Inuvik",
       "America/Yellowknife"}),
   "MDST": ({"Europe/Moscow"}),
   "MDT": ({"America/Boise", "America/Cambridge_Bay", "America/Chihuahua",
       "America/Denver", "America/Edmonton", "America/Inuvik",
       "America/Mazatlan", "America/Ojinaga", "America/Yellowknife",
       "MST7MDT", "America/Bahia_Banderas", "America/Hermosillo",
       "America/North_Dakota/Beulah", "America/North_Dakota/Center",
       "America/North_Dakota/New_Salem", "America/Phoenix", "America/Regina",
       "America/Swift_Current"}),
   "MEST": ({"MET"}),
   "MET": ({"MET"}),
   "MMT": ({"Africa/Monrovia", "Europe/Moscow", "Indian/Maldives",
       "America/Managua", "Asia/Makassar", "Europe/Minsk",
       "America/Montevideo", "Asia/Colombo", "Asia/Kolkata"}),
   "MPT": ({"America/Boise", "America/Cambridge_Bay", "America/Denver",
       "America/Edmonton", "America/Inuvik", "America/Ojinaga",
       "America/Yellowknife", "MST7MDT", "America/North_Dakota/Beulah",
       "America/North_Dakota/Center", "America/North_Dakota/New_Salem",
       "America/Phoenix", "America/Regina", "America/Swift_Current"}),
   "MSD": ({"Europe/Tiraspol", "Europe/Moscow", "Europe/Simferopol",
       "Europe/Kaliningrad", "Europe/Tallinn", "Europe/Vilnius",
       "Europe/Chisinau", "Europe/Kiev", "Europe/Minsk", "Europe/Riga",
       "Europe/Uzhgorod", "Europe/Zaporozhye"}),
   "MSK": ({"Europe/Moscow", "Europe/Simferopol", "Europe/Tiraspol",
       "Europe/Minsk", "Europe/Uzhgorod", "Europe/Kaliningrad",
       "Europe/Tallinn", "Europe/Vilnius", "Europe/Chisinau", "Europe/Kiev",
       "Europe/Riga", "Europe/Zaporozhye"}),
   "MST": ({"America/Boise", "America/Cambridge_Bay", "America/Chihuahua",
       "America/Creston", "America/Dawson_Creek", "America/Denver",
       "America/Edmonton", "America/Fort_Nelson", "America/Hermosillo",
       "America/Inuvik", "America/Mazatlan", "America/Ojinaga",
       "America/Phoenix", "America/Yellowknife", "MST", "MST7MDT",
       "America/Bahia_Banderas", "America/North_Dakota/Beulah",
       "America/North_Dakota/Center", "America/North_Dakota/New_Salem",
       "America/Regina", "America/Swift_Current", "Europe/Moscow",
       "America/Ensenada", "America/Mexico_City", "America/Tijuana"}),
   "MWT": ({"America/Boise", "America/Cambridge_Bay", "America/Chihuahua",
       "America/Denver", "America/Edmonton", "America/Inuvik",
       "America/Mazatlan", "America/Ojinaga", "America/Yellowknife",
       "MST7MDT", "America/Bahia_Banderas", "America/Hermosillo",
       "America/North_Dakota/Beulah", "America/North_Dakota/Center",
       "America/North_Dakota/New_Salem", "America/Phoenix", "America/Regina",
       "America/Swift_Current"}),
   "NDDT": ({"America/Goose_Bay", "America/St_Johns"}),
   "NDT": ({"America/St_Johns", "America/Adak", "America/Goose_Bay",
       "America/Nome"}),
   "NFT": ({"Europe/Oslo", "Europe/Paris"}),
   "NPT": ({"America/St_Johns", "America/Adak", "America/Goose_Bay",
       "America/Nome"}),
   "NST": ({"America/St_Johns", "America/Adak", "America/Goose_Bay",
       "America/Nome", "Europe/Amsterdam"}),
   "NWT": ({"America/St_Johns", "America/Adak", "America/Goose_Bay",
       "America/Nome"}),
   "NZDT": ({"Antarctica/McMurdo", "Pacific/Auckland"}),
   "NZMT": ({"Antarctica/McMurdo", "Pacific/Auckland"}),
   "NZST": ({"Antarctica/McMurdo", "Pacific/Auckland"}),
   "P%sT": ({"America/Ensenada"}),
   "PDDT": ({"America/Dawson", "America/Inuvik", "America/Whitehorse"}),
   "PDT": ({"America/Dawson", "America/Los_Angeles", "America/Tijuana",
       "America/Vancouver", "America/Whitehorse", "Asia/Manila", "PST8PDT",
       "America/Boise", "America/Dawson_Creek", "America/Fort_Nelson",
       "America/Inuvik", "America/Juneau", "America/Metlakatla",
       "America/Sitka"}),
   "PKST": ({"Asia/Karachi"}),
   "PKT": ({"Asia/Karachi"}),
   "PLMT": ({"Asia/Hanoi", "Asia/Ho_Chi_Minh", "Asia/Phnom_Penh",
       "Asia/Vientiane"}),
   "PMMT": ({"Pacific/Bougainville", "Pacific/Port_Moresby"}),
   "PMT": ({"America/Paramaribo", "Asia/Pontianak", "Asia/Yekaterinburg",
       "Europe/Paris", "Africa/Algiers", "Africa/Tunis", "Europe/Monaco",
       "Europe/Prague"}),
   "PPMT": ({"America/Port-au-Prince"}),
   "PPT": ({"America/Dawson", "America/Los_Angeles", "America/Tijuana",
       "America/Vancouver", "America/Whitehorse", "PST8PDT", "America/Boise",
       "America/Dawson_Creek", "America/Fort_Nelson", "America/Inuvik",
       "America/Juneau", "America/Metlakatla", "America/Sitka"}),
   "PST": ({"America/Dawson", "America/Los_Angeles", "America/Tijuana",
       "America/Vancouver", "America/Whitehorse", "Asia/Manila", "PST8PDT",
       "America/Metlakatla", "America/Ensenada", "America/Bahia_Banderas",
       "America/Hermosillo", "America/Mazatlan", "America/Boise",
       "America/Dawson_Creek", "America/Fort_Nelson", "America/Inuvik",
       "America/Juneau", "America/Sitka", "America/Creston"}),
   "PWT": ({"America/Dawson", "America/Los_Angeles", "America/Tijuana",
       "America/Vancouver", "America/Whitehorse", "PST8PDT", "America/Boise",
       "America/Dawson_Creek", "America/Fort_Nelson", "America/Inuvik",
       "America/Juneau", "America/Metlakatla", "America/Sitka"}),
   "QMT": ({"America/Guayaquil"}),
   "RMT": ({"Europe/Riga", "Asia/Yangon", "Europe/Rome"}),
   "S": ({"Europe/Amsterdam", "Europe/Moscow"}),
   "SAST": ({"Africa/Johannesburg", "Africa/Maseru", "Africa/Mbabane",
       "Africa/Windhoek", "Africa/Gaborone"}),
   "SDMT": ({"America/Santo_Domingo"}),
   "SET": ({"Europe/Stockholm"}),
   "SJMT": ({"America/Costa_Rica"}),
   "SMT": ({"America/Punta_Arenas", "America/Santiago", "Europe/Simferopol",
       "Atlantic/Stanley", "Asia/Kuala_Lumpur", "Asia/Singapore"}),
   "SST": ({"Pacific/Pago_Pago"}),
   "TBMT": ({"Asia/Tbilisi"}),
   "TMT": ({"Asia/Tehran", "Europe/Tallinn"}),
   "UTC": ({"Etc/UTC"}),
   "WAST": ({"Africa/Ndjamena"}),
   "WAT": ({"Africa/Bangui", "Africa/Brazzaville", "Africa/Douala",
       "Africa/Kinshasa", "Africa/Lagos", "Africa/Libreville",
       "Africa/Luanda", "Africa/Malabo", "Africa/Ndjamena", "Africa/Niamey",
       "Africa/Porto-Novo", "Africa/Windhoek", "Africa/Sao_Tome"}),
   "WEMT": ({"Atlantic/Madeira", "Europe/Lisbon", "Africa/Ceuta",
       "Europe/Madrid", "Europe/Monaco", "Europe/Paris"}),
   "WEST": ({"Atlantic/Canary", "Atlantic/Faroe", "Atlantic/Madeira",
       "Europe/Lisbon", "WET", "Atlantic/Azores", "Africa/Algiers",
       "Africa/Ceuta", "Europe/Luxembourg", "Europe/Madrid", "Europe/Monaco",
       "Europe/Paris", "Europe/Brussels"}),
   "WET": ({"Atlantic/Canary", "Atlantic/Faroe", "Atlantic/Madeira",
       "Europe/Lisbon", "WET", "Atlantic/Azores", "Africa/Algiers",
       "Africa/Ceuta", "Europe/Luxembourg", "Europe/Madrid", "Europe/Monaco",
       "Europe/Paris", "Europe/Andorra", "Europe/Brussels"}),
   "WIB": ({"Asia/Jakarta", "Asia/Pontianak"}),
   "WIT": ({"Asia/Jayapura"}),
   "WITA": ({"Asia/Makassar", "Asia/Pontianak"}),
   "WMT": ({"Europe/Vilnius", "Europe/Warsaw"}),
   "YDDT": ({"America/Dawson", "America/Whitehorse"}),
   "YDT": ({"America/Anchorage", "America/Dawson", "America/Juneau",
       "America/Nome", "America/Sitka", "America/Whitehorse",
       "America/Yakutat"}),
   "YPT": ({"America/Anchorage", "America/Dawson", "America/Juneau",
       "America/Nome", "America/Sitka", "America/Whitehorse",
       "America/Yakutat"}),
   "YST": ({"America/Anchorage", "America/Dawson", "America/Juneau",
       "America/Nome", "America/Sitka", "America/Whitehorse",
       "America/Yakutat"}),
   "YWT": ({"America/Anchorage", "America/Dawson", "America/Juneau",
       "America/Nome", "America/Sitka", "America/Whitehorse",
       "America/Yakutat"}),
]);

// this is used by the timezone expert system,
// that uses localtime (or whatever) to figure out the *correct* timezone

// note that at least Red Hat 6.2 has an error in the time database,
// so a lot of Europeean (Paris, Brussels) times get to be Luxembourg /Mirar

mapping timezone_expert_tree =
([ "test":63086400, // 1972-01-01 04:00:00
   -46800:
      ([ "test":386420400, // 1982-03-31 11:00:00
         -46800:"Pacific/Tongatapu",
         -43200:"Asia/Anadyr",
      ]),
   -45900:"Pacific/Chatham",
   -43200:
      ([ "test":915105600, // 1998-12-31 12:00:00
         -46800:
            ([ "test":-1709985344, // 1915-10-25 12:04:16
               -43200:"Pacific/Fiji",
               -41400:"Pacific/Auckland",
            ]),
         -43200:
            ([ "test":670341600, // 1991-03-30 14:00:00
               -43200:
                  ([ "test":-818067600, // 1944-01-29 15:00:00
                     -43200:
                        ([ "test":-2177496920, // 1900-12-31 11:44:40
                           -43200:"Pacific/Wallis",
                           -43012:"Pacific/Funafuti",
                           -41524:"Pacific/Tarawa",
                           -39988:"Pacific/Wake",
                        ]),
                     -39600:"Pacific/Majuro",
                  ]),
               -39600:"Asia/Kamchatka",
            ]),
         -39600:"Pacific/Kosrae",
      ]),
   -41400:
      ([ "test":152029800, // 1974-10-26 14:30:00
         -45000:"Pacific/Norfolk",
         -41400:"Pacific/Nauru",
      ]),
   -39600:
      ([ "test":1270389600, // 2010-04-04 14:00:00
         -43200:
            ([ "test":1414245600, // 2014-10-25 14:00:00
               -39600:"Asia/Srednekolymsk",
               -36000:"Asia/Magadan",
            ]),
         -39600:
            ([ "test":857214000, // 1997-03-01 11:00:00
               -43200:"Pacific/Noumea",
               -39600:
                  ([ "test":-907408800, // 1941-03-31 14:00:00
                     -39600:
                        ([ "test":-1829387596, // 1912-01-12 12:46:44
                           -39600:"Pacific/Efate",
                           -38388:"Pacific/Guadalcanal",
                        ]),
                     -32400:"Pacific/Pohnpei",
                     0:"Antarctica/Macquarie",
                  ]),
               -36000:"Asia/Sakhalin",
            ]),
         -36000:
            ([ "test":31572000, // 1971-01-01 10:00:00
               -39600:"Australia/Hobart",
               -36000:
                  ([ "test":-1680508800, // 1916-09-30 16:00:00
                     -39600:"Australia/Currie",
                     -36000:
                        ([ "test":-2366791928, // 1894-12-31 13:47:52
                           -36292:"Australia/Sydney",
                           -36000:"Australia/Brisbane",
                           -35756:"Australia/Lindeman",
                           -34792:"Australia/Melbourne",
                        ]),
                  ]),
            ]),
      ]),
   -37800:
      ([ "test":-2364110060, // 1895-01-31 14:45:40
         -36000:"Australia/Broken_Hill",
         -32400:"Australia/Adelaide",
      ]),
   -36000:
      ([ "test":489061800, // 1985-07-01 10:30:00
         -39600:"Asia/Vladivostok",
         -37800:"Australia/Lord_Howe",
         -36000:
            ([ "test":1419696000, // 2014-12-27 16:00:00
               -39600:"Pacific/Bougainville",
               -36000:
                  ([ "test":-802256400, // 1944-07-30 15:00:00
                     -36000:
                        ([ "test":-885549600, // 1941-12-09 14:00:00
                           -36000:"Pacific/Port_Moresby",
                           -32400:"Pacific/Guam",
                        ]),
                     -32400:"Pacific/Chuuk",
                     0:"Antarctica/DumontDUrville",
                  ]),
            ]),
      ]),
   -34200:"Australia/Darwin",
   -32400:
      ([ "test":354963600, // 1981-04-01 09:00:00
         -43200:"Asia/Ust-Nera",
         -36000:
            ([ "test":1072947600, // 2004-01-01 09:00:00
               -36000:"Asia/Khandyga",
               -32400:
                  ([ "test":1414252800, // 2014-10-25 16:00:00
                     -32400:"Asia/Yakutsk",
                     -28800:"Asia/Chita",
                  ]),
            ]),
         -32400:
            ([ "test":1439564400, // 2015-08-14 15:00:00
               -32400:
                  ([ "test":-498128400, // 1954-03-20 15:00:00
                     -34200:"Asia/Jayapura",
                     -32400:
                        ([ "test":-2587712400, // 1887-12-31 15:00:00
                           -32400:"Asia/Tokyo",
                           -32276:"Pacific/Palau",
                        ]),
                     -30600:"Asia/Seoul",
                  ]),
               -30600:"Asia/Pyongyang",
            ]),
         -28800:"Asia/Dili",
      ]),
   -31500:"Australia/Eucla",
   -28800:
      ([ "test":1255802400, // 2009-10-17 18:00:00
         -39600:"Antarctica/Casey",
         -32400:"Asia/Irkutsk",
         -28800:
            ([ "test":-2056692850, // 1904-10-29 16:25:50
               -28800:
                  ([ "test":-649954800, // 1949-05-28 09:00:00
                     -32400:
                        ([ "test":-766224000, // 1945-09-20 16:00:00
                           -32400:"Asia/Macau",
                           -28800:"Asia/Taipei",
                        ]),
                     -28800:
                        ([ "test":-794221200, // 1944-10-31 15:00:00
                           -32400:"Asia/Shanghai",
                           -28800:
                              ([ "test":-836409600, // 1943-07-01 08:00:00
                                 -32400:"Asia/Manila",
                                 -28800:"Australia/Perth",
                              ]),
                        ]),
                  ]),
               -28656:"Asia/Makassar",
               -27580:"Asia/Brunei",
               -27402:"Asia/Hong_Kong",
               -26480:"Asia/Kuching",
            ]),
         -25200:
            ([ "test":171820800, // 1975-06-12 16:00:00
               -28800:"Asia/Pontianak",
               -25200:"Asia/Ho_Chi_Minh",
            ]),
      ]),
   -27000:
      ([ "test":-2177477725, // 1900-12-31 17:04:35
         -24925:"Asia/Singapore",
         -24406:"Asia/Kuala_Lumpur",
      ]),
   -25200:
      ([ "test":670359600, // 1991-03-30 19:00:00
         -36000:"Asia/Choibalsan",
         -32400:"Asia/Ulaanbaatar",
         -25200:
            ([ "test":1255806000, // 2009-10-17 19:00:00
               -25200:
                  ([ "test":-2004073404, // 1906-06-30 16:56:36
                     -25632:"Asia/Jakarta",
                     -25590:"Asia/Hanoi",
                     -25200:"Indian/Christmas",
                     -24124:"Asia/Bangkok",
                  ]),
               -18000:"Antarctica/Davis",
            ]),
         -21600:
            ([ "test":801648000, // 1995-05-28 08:00:00
               -28800:
                  ([ "test":1020250800, // 2002-05-01 11:00:00
                     -28800:
                        ([ "test":1269716400, // 2010-03-27 19:00:00
                           -28800:"Asia/Krasnoyarsk",
                           -21600:"Asia/Novokuznetsk",
                        ]),
                     -25200:"Asia/Tomsk",
                  ]),
               -25200:
                  ([ "test":738144000, // 1993-05-23 08:00:00
                     -28800:"Asia/Barnaul",
                     -25200:"Asia/Novosibirsk",
                  ]),
            ]),
      ]),
   -23400:
      ([ "test":-873268200, // 1942-04-30 17:30:00
         -32400:"Asia/Yangon",
         -23400:"Indian/Cocos",
      ]),
   -21600:
      ([ "test":670363200, // 1991-03-30 20:00:00
         -28800:"Asia/Hovd",
         -25200:"Asia/Tashkent",
         -21600:
            ([ "test":1255809600, // 2009-10-17 20:00:00
               -25200:"Asia/Dhaka",
               -21600:
                  ([ "test":-1325483420, // 1927-12-31 18:09:40
                     -21600:"Asia/Urumqi",
                     0:"Antarctica/Vostok",
                  ]),
               -18000:
                  ([ "test":684363600, // 1991-09-08 21:00:00
                     -21600:"Antarctica/Mawson",
                     -18000:"Asia/Dushanbe",
                  ]),
            ]),
         -18000:
            ([ "test":683625600, // 1991-08-31 08:00:00
               -21600:
                  ([ "test":1301169600, // 2011-03-26 20:00:00
                     -25200:"Asia/Omsk",
                     -21600:"Asia/Almaty",
                  ]),
               -18000:"Asia/Bishkek",
            ]),
      ]),
   -19800:
      ([ "test":832962600, // 1996-05-24 18:30:00
         -23400:"Asia/Colombo",
         -21600:"Asia/Thimphu",
         -20700:"Asia/Kathmandu",
         -19800:"Asia/Kolkata",
      ]),
   -18000:
      ([ "test":922568400, // 1999-03-27 21:00:00
         -21600:
            ([ "test":695768400, // 1992-01-18 21:00:00
               -21600:"Asia/Qyzylorda",
               -18000:"Indian/Chagos",
               -14400:
                  ([ "test":370720800, // 1981-09-30 18:00:00
                     -21600:
                        ([ "test":1099170000, // 2004-10-30 21:00:00
                           -21600:"Asia/Qostanay",
                           -18000:"Asia/Aqtobe",
                        ]),
                     -18000:"Asia/Yekaterinburg",
                  ]),
            ]),
         -18000:
            ([ "test":354913200, // 1981-03-31 19:00:00
               -21600:
                  ([ "test":370720800, // 1981-09-30 18:00:00
                     -21600:"Asia/Samarkand",
                     -18000:"Asia/Ashgabat",
                  ]),
               -18000:
                  ([ "test":-631152000, // 1950-01-01 00:00:00
                     -19800:"Asia/Karachi",
                     -18000:"Indian/Kerguelen",
                     -17640:"Indian/Maldives",
                  ]),
            ]),
         -14400:
            ([ "test":354913200, // 1981-03-31 19:00:00
               -21600:"Asia/Oral",
               -18000:
                  ([ "test":-1247544000, // 1930-06-20 20:00:00
                     -18000:"Asia/Aqtau",
                     -10800:"Asia/Atyrau",
                  ]),
            ]),
      ]),
   -16200:"Asia/Kabul",
   -14400:
      ([ "test":575416800, // 1988-03-26 22:00:00
         -18000:
            ([ "test":687916800, // 1991-10-20 00:00:00
               -14400:
                  ([ "test":670374000, // 1991-03-30 23:00:00
                     -14400:
                        ([ "test":1459033200, // 2016-03-26 23:00:00
                           -14400:"Europe/Astrakhan",
                           -10800:"Europe/Kirov",
                        ]),
                     -7200:"Europe/Samara",
                  ]),
               -10800:
                  ([ "test":778392000, // 1994-09-01 04:00:00
                     -18000:"Asia/Tbilisi",
                     -14400:
                        ([ "test":-1441163964, // 1924-05-01 20:40:36
                           -10800:"Asia/Baku",
                           -10680:"Asia/Yerevan",
                        ]),
                  ]),
               -7200:"Europe/Ulyanovsk",
            ]),
         -14400:
            ([ "test":-2006653308, // 1906-05-31 20:18:12
               -14400:"Indian/Mahe",
               -13800:"Indian/Mauritius",
               -13312:"Indian/Reunion",
               -13272:"Asia/Dubai",
            ]),
         -10800:
            ([ "test":76190400, // 1972-05-31 20:00:00
               -14400:
                  ([ "test":1480806000, // 2016-12-03 23:00:00
                     -14400:"Europe/Saratov",
                     -10800:"Europe/Volgograd",
                  ]),
               -10800:"Asia/Qatar",
            ]),
      ]),
   -12600:"Asia/Tehran",
   -10800:
      ([ "test":670374000, // 1991-03-30 23:00:00
         -14400:"Europe/Zaporozhye",
         -10800:
            ([ "test":686102400, // 1991-09-29 00:00:00
               -14400:"Asia/Baghdad",
               -10800:
                  ([ "test":-719636812, // 1947-03-13 20:53:08
                     -10800:"Asia/Riyadh",
                     -9900:"Africa/Nairobi",
                     0:"Antarctica/Syowa",
                  ]),
               -7200:"Europe/Kiev",
            ]),
         -7200:
            ([ "test":606870000, // 1989-03-25 23:00:00
               -14400:
                  ([ "test":646786800, // 1990-06-30 23:00:00
                     -14400:"Europe/Moscow",
                     -10800:
                        ([ "test":-804636000, // 1944-07-03 02:00:00
                           -10800:"Europe/Minsk",
                           -7200:"Europe/Chisinau",
                        ]),
                     -7200:"Europe/Simferopol",
                  ]),
               -10800:
                  ([ "test":-797637600, // 1944-09-22 02:00:00
                     -10800:"Europe/Tallinn",
                     -7200:"Europe/Riga",
                  ]),
               -7200:
                  ([ "test":891133200, // 1998-03-29 01:00:00
                     -10800:"Europe/Kaliningrad",
                     -7200:"Europe/Vilnius",
                  ]),
            ]),
         -3600:"Europe/Uzhgorod",
      ]),
   -7200:
      ([ "test":844034400, // 1996-09-29 22:00:00
         -10800:
            ([ "test":354672000, // 1981-03-29 00:00:00
               -14400:"Europe/Istanbul",
               -10800:
                  ([ "test":-1535938789, // 1921-04-30 22:20:11
                     -7200:"Europe/Helsinki",
                     -6264:"Europe/Bucharest",
                  ]),
               -7200:
                  ([ "test":291762000, // 1979-03-31 21:00:00
                     -10800:"Europe/Sofia",
                     -7200:
                        ([ "test":-1686101632, // 1916-07-27 22:26:08
                           -8712:"Asia/Damascus",
                           -7200:"Europe/Athens",
                        ]),
                  ]),
            ]),
         -7200:
            ([ "test":1220220000, // 2008-08-31 22:00:00
               -10800:
                  ([ "test":904618800, // 1998-09-01 03:00:00
                     -10800:
                        ([ "test":-1641003640, // 1917-12-31 21:39:20
                           -8624:"Asia/Amman",
                           -8148:"Asia/Famagusta",
                           -8008:"Asia/Nicosia",
                           -7200:
                              ([ "test":-2840149254, // 1879-12-31 21:39:06
                                 -8440:"Asia/Jerusalem",
                                 -7200:"Asia/Beirut",
                              ]),
                        ]),
                     -7200:
                        ([ "test":1509483600, // 2017-10-31 21:00:00
                           -10800:"Africa/Juba",
                           -7200:"Africa/Khartoum",
                        ]),
                  ]),
               -7200:
                  ([ "test":1269640860, // 2010-03-26 22:01:00
                     -10800:
                        ([ "test":1219978800, // 2008-08-29 03:00:00
                           -10800:"Asia/Hebron",
                           -7200:"Asia/Gaza",
                        ]),
                     -7200:
                        ([ "test":-2109291020, // 1903-02-28 21:49:40
                           -7200:
                              ([ "test":-2185409109, // 1900-09-30 21:54:51
                                 -7820:"Africa/Maputo",
                                 -7200:"Africa/Cairo",
                              ]),
                           -5400:"Africa/Johannesburg",
                        ]),
                  ]),
               -3600:"Africa/Windhoek",
            ]),
         -3600:"Africa/Tripoli",
      ]),
   -3600:
      ([ "test":212544000, // 1976-09-26 00:00:00
         -7200:"Europe/Tirane",
         -3600:
            ([ "test":481078800, // 1985-03-31 01:00:00
               -7200:
                  ([ "test":102387600, // 1973-03-31 01:00:00
                     -7200:"Europe/Malta",
                     -3600:
                        ([ "test":-1855958961, // 1911-03-10 23:50:39
                           -5040:"Europe/Warsaw",
                           -3600:
                              ([ "test":-766623600, // 1945-09-16 01:00:00
                                 -10800:"Europe/Berlin",
                                 -7200:
                                    ([ "test":-728510400, // 1946-12-01 04:00:00
                                       -7200:"Europe/Prague",
                                       -3600:
                                          ([ "test":-781045200, // 1945-04-02 03:00:00
                                             -7200:"Europe/Oslo",
                                             -3600:"Europe/Budapest",
                                          ]),
                                    ]),
                                 -3600:
                                    ([ "test":-777942000, // 1945-05-08 01:00:00
                                       -7200:
                                          ([ "test":-797972400, // 1944-09-18 05:00:00
                                             -7200:
                                                ([ "test":-781045200, // 1945-04-02 03:00:00
                                                   -7200:"Europe/Copenhagen",
                                                   -3600:"Europe/Belgrade",
                                                ]),
                                             -3600:
                                                ([ "test":-935179200, // 1940-05-14 04:00:00
                                                   -7200:"Europe/Luxembourg",
                                                   -3600:"Europe/Rome",
                                                ]),
                                          ]),
                                       -3600:
                                          ([ "test":-781052400, // 1945-04-02 01:00:00
                                             -7200:"Europe/Vienna",
                                             -3600:
                                                ([ "test":-1692496800, // 1916-05-14 22:00:00
                                                   -7200:"Europe/Stockholm",
                                                   -3600:"Europe/Zurich",
                                                ]),
                                          ]),
                                    ]),
                              ]),
                           -1172:"Europe/Amsterdam",
                           -561:"Europe/Paris",
                           0:
                              ([ "test":-766609200, // 1945-09-16 05:00:00
                                 -7200:"Europe/Madrid",
                                 -3600:
                                    ([ "test":-934668000, // 1940-05-20 02:00:00
                                       -7200:"Europe/Brussels",
                                       -3600:
                                          ([ "test":-2486680172, // 1891-03-14 23:30:28
                                             -561:"Europe/Monaco",
                                             0:"Europe/Gibraltar",
                                          ]),
                                    ]),
                                 0:"Europe/Andorra",
                              ]),
                        ]),
                  ]),
               -3600:
                  ([ "test":308703600, // 1979-10-13 23:00:00
                     -7200:"Africa/Ndjamena",
                     -3600:
                        ([ "test":-1855958961, // 1911-03-10 23:50:39
                           -3600:"Africa/Tunis",
                           -816:"Africa/Lagos",
                        ]),
                  ]),
            ]),
         0:"Europe/Lisbon",
      ]),
   0:
      ([ "test":357523200, // 1981-05-01 00:00:00
         -3600:
            ([ "test":246240000, // 1977-10-21 00:00:00
               -3600:
                  ([ "test":309747600, // 1979-10-26 01:00:00
                     -3600:
                        ([ "test":-1691962479, // 1916-05-21 02:25:21
                           -3600:"Europe/London",
                           -2079:"Europe/Dublin",
                        ]),
                     0:"Africa/Algiers",
                  ]),
               0:
                  ([ "test":323827200, // 1980-04-06 00:00:00
                     -3600:
                        ([ "test":-768445200, // 1945-08-25 23:00:00
                           0:"Atlantic/Madeira",
                           3600:"Atlantic/Canary",
                        ]),
                     0:"Atlantic/Faroe",
                  ]),
            ]),
         0:
            ([ "test":1540699200, // 2018-10-28 04:00:00
               -3600:
                  ([ "test":448243200, // 1984-03-16 00:00:00
                     -3600:
                        ([ "test":504918000, // 1985-12-31 23:00:00
                           -3600:"Africa/Ceuta",
                           0:"Africa/Casablanca",
                        ]),
                     0:"Africa/Sao_Tome",
                  ]),
               0:
                  ([ "test":-1956609120, // 1908-01-01 01:28:00
                     0:"Antarctica/Troll",
                     52:"Africa/Accra",
                     968:"Africa/Abidjan",
                     3600:"Atlantic/Reykjavik",
                  ]),
            ]),
         10800:"Antarctica/Rothera",
      ]),
   2670:"Africa/Monrovia",
   3600:
      ([ "test":1540699200, // 2018-10-28 04:00:00
         -3600:"Africa/El_Aaiun",
         0:"Africa/Bissau",
         3600:"Atlantic/Azores",
      ]),
   7200:
      ([ "test":653522400, // 1990-09-16 22:00:00
         0:"America/Scoresbysund",
         3600:"Atlantic/Cape_Verde",
         7200:
            ([ "test":-2524512832, // 1890-01-01 02:26:08
               7200:"Atlantic/South_Georgia",
               7780:"America/Noronha",
            ]),
      ]),
   10800:
      ([ "test":1086058800, // 2004-06-01 03:00:00
         0:"America/Danmarkshavn",
         7200:"America/Godthab",
         10800:
            ([ "test":132096600, // 1974-03-09 21:30:00
               5400:"America/Montevideo",
               7200:
                  ([ "test":667947600, // 1991-03-02 21:00:00
                     7200:"America/Argentina/Buenos_Aires",
                     10800:"America/Argentina/Jujuy",
                     14400:
                        ([ "test":-2372096592, // 1894-10-31 04:16:48
                           15408:"America/Argentina/Cordoba",
                           15700:"America/Argentina/Salta",
                        ]),
                  ]),
               10800:
                  ([ "test":971560800, // 2000-10-14 22:00:00
                     7200:
                        ([ "test":972165600, // 2000-10-21 22:00:00
                           7200:
                              ([ "test":1318734000, // 2011-10-16 03:00:00
                                 7200:
                                    ([ "test":-195447600, // 1963-10-22 21:00:00
                                       7200:"America/Sao_Paulo",
                                       10800:"America/Bahia",
                                    ]),
                                 10800:"America/Araguaina",
                              ]),
                           10800:
                              ([ "test":-1767217028, // 1914-01-01 02:22:52
                                 9240:"America/Fortaleza",
                                 10800:"America/Maceio",
                              ]),
                        ]),
                     10800:
                        ([ "test":-1767217224, // 1914-01-01 02:19:36
                           10800:"America/Recife",
                           11636:"America/Belem",
                           14400:"America/Cayenne",
                        ]),
                  ]),
            ]),
         14400:
            ([ "test":667796400, // 1991-03-01 03:00:00
               7200:
                  ([ "test":667947600, // 1991-03-02 21:00:00
                     7200:
                        ([ "test":1085886000, // 2004-05-30 03:00:00
                           10800:"America/Argentina/Rio_Gallegos",
                           14400:"America/Argentina/Ushuaia",
                        ]),
                     14400:
                        ([ "test":1087099200, // 2004-06-13 04:00:00
                           10800:"America/Argentina/Tucuman",
                           14400:"America/Argentina/Catamarca",
                        ]),
                  ]),
               10800:
                  ([ "test":-740520000, // 1946-07-15 04:00:00
                     0:"Antarctica/Palmer",
                     10800:"America/Santiago",
                     14400:"America/Punta_Arenas",
                  ]),
               14400:
                  ([ "test":637380000, // 1990-03-14 02:00:00
                     10800:
                        ([ "test":1085972400, // 2004-05-31 03:00:00
                           10800:"America/Argentina/La_Rioja",
                           14400:"America/Argentina/San_Juan",
                        ]),
                     14400:
                        ([ "test":636498000, // 1990-03-03 21:00:00
                           7200:"America/Argentina/San_Luis",
                           14400:"America/Argentina/Mendoza",
                        ]),
                  ]),
            ]),
      ]),
   12600:
      ([ "test":465449400, // 1984-10-01 03:30:00
         9000:"America/St_Johns",
         10800:"America/Paramaribo",
      ]),
   13500:"America/Guyana",
   14400:
      ([ "test":941320800, // 1999-10-30 22:00:00
         7200:"America/Miquelon",
         10800:
            ([ "test":590011200, // 1988-09-11 20:00:00
               7200:"America/Goose_Bay",
               10800:
                  ([ "test":136360800, // 1974-04-28 06:00:00
                     10800:
                        ([ "test":-2131646412, // 1902-06-15 03:59:48
                           14400:"America/Glace_Bay",
                           15264:"America/Halifax",
                           15558:"Atlantic/Bermuda",
                           18000:"America/Moncton",
                        ]),
                     14400:"Atlantic/Stanley",
                  ]),
               14400:
                  ([ "test":-1206389360, // 1931-10-10 03:50:40
                     10800:
                        ([ "test":971557200, // 2000-10-14 21:00:00
                           10800:
                              ([ "test":-1767212492, // 1914-01-01 03:38:28
                                 13460:"America/Cuiaba",
                                 14400:"America/Campo_Grande",
                              ]),
                           14400:"America/Boa_Vista",
                        ]),
                     14400:
                        ([ "test":86760000, // 1972-10-01 04:00:00
                           10800:"America/Asuncion",
                           14400:"America/Thule",
                        ]),
                  ]),
            ]),
         14400:
            ([ "test":1214280000, // 2008-06-24 04:00:00
               10800:"America/Santarem",
               14400:
                  ([ "test":323841600, // 1980-04-06 04:00:00
                     10800:"America/Martinique",
                     14400:
                        ([ "test":-1826738653, // 1912-02-12 04:35:47
                           14309:"America/Barbados",
                           14400:
                              ([ "test":-2713896692, // 1884-01-01 03:48:28
                                 14400:"America/Blanc-Sablon",
                                 15865:"America/Puerto_Rico",
                              ]),
                           14404:"America/Manaus",
                           14764:"America/Port_of_Spain",
                           15336:"America/Porto_Velho",
                           16200:"America/Curacao",
                           16356:"America/La_Paz",
                        ]),
                  ]),
               16200:"America/Caracas",
            ]),
         18000:"America/Pangnirtung",
      ]),
   16200:"America/Santo_Domingo",
   18000:
      ([ "test":1143961200, // 2006-04-02 07:00:00
         14400:
            ([ "test":167814000, // 1975-04-27 07:00:00
               14400:
                  ([ "test":1136091600, // 2006-01-01 05:00:00
                     14400:"America/Havana",
                     18000:
                        ([ "test":-923288400, // 1940-09-28 19:00:00
                           14400:
                              ([ "test":-883630800, // 1941-12-31 19:00:00
                                 14400:
                                    ([ "test":-2366736148, // 1895-01-01 05:17:32
                                       18000:"America/Toronto",
                                       21184:"America/Nipigon",
                                    ]),
                                 18000:"America/New_York",
                              ]),
                           18000:
                              ([ "test":-1893434400, // 1910-01-01 06:00:00
                                 18000:"America/Thunder_Bay",
                                 18570:"America/Nassau",
                                 21600:"America/Detroit",
                              ]),
                           21600:
                              ([ "test":-273729600, // 1961-04-29 20:00:00
                                 18000:"America/Indiana/Marengo",
                                 21600:"America/Kentucky/Louisville",
                              ]),
                        ]),
                  ]),
               18000:
                  ([ "test":972766800, // 2000-10-28 21:00:00
                     14400:
                        ([ "test":-865296000, // 1942-08-01 00:00:00
                           14400:"America/Iqaluit",
                           18000:"America/Grand_Turk",
                        ]),
                     18000:
                        ([ "test":-386787600, // 1957-09-29 07:00:00
                           18000:
                              ([ "test":-1670483460, // 1917-01-24 16:49:00
                                 18000:"America/Port-au-Prince",
                                 21600:"America/Indiana/Vevay",
                              ]),
                           21600:"America/Indiana/Indianapolis",
                        ]),
                  ]),
            ]),
         18000:
            ([ "test":1214283600, // 2008-06-24 05:00:00
               14400:
                  ([ "test":-1767209328, // 1914-01-01 04:31:12
                     16768:"America/Eirunepe",
                     18000:"America/Rio_Branco",
                  ]),
               18000:
                  ([ "test":-1946918424, // 1908-04-22 05:19:36
                     17776:"America/Bogota",
                     18000:"America/Panama",
                     18430:"America/Jamaica",
                     18516:"America/Lima",
                     18840:"America/Guayaquil",
                     21600:"America/Atikokan",
                  ]),
            ]),
         21600:
            ([ "test":1194123600, // 2007-11-03 21:00:00
               14400:
                  ([ "test":1173556800, // 2007-03-10 20:00:00
                     18000:"America/Indiana/Winamac",
                     21600:"America/Indiana/Vincennes",
                  ]),
               18000:
                  ([ "test":104914800, // 1973-04-29 07:00:00
                     18000:"America/Indiana/Tell_City",
                     21600:"America/Menominee",
                  ]),
               21600:"Pacific/Galapagos",
            ]),
      ]),
   21600:
      ([ "test":1194123600, // 2007-11-03 21:00:00
         14400:
            ([ "test":972766800, // 2000-10-28 21:00:00
               14400:"America/Kentucky/Monticello",
               18000:"America/Indiana/Petersburg",
            ]),
         18000:
            ([ "test":1136052000, // 2005-12-31 18:00:00
               18000:
                  ([ "test":-195066000, // 1963-10-27 07:00:00
                     21600:"America/Indiana/Knox",
                     25200:"Pacific/Easter",
                  ]),
               21600:
                  ([ "test":-704937600, // 1947-08-31 00:00:00
                     0:"America/Rankin_Inlet",
                     18000:
                        ([ "test":-1067832000, // 1936-02-29 20:00:00
                           18000:"America/Chicago",
                           21600:"America/Winnipeg",
                        ]),
                     21600:
                        ([ "test":-880214400, // 1942-02-09 08:00:00
                           0:"America/Resolute",
                           18000:"America/Rainy_River",
                        ]),
                  ]),
            ]),
         21600:
            ([ "test":902008800, // 1998-08-01 22:00:00
               18000:
                  ([ "test":377935200, // 1981-12-23 06:00:00
                     18000:
                        ([ "test":407653200, // 1982-12-02 05:00:00
                           18000:"America/Cancun",
                           21600:"America/Merida",
                        ]),
                     21600:
                        ([ "test":1001797200, // 2001-09-29 21:00:00
                           18000:
                              ([ "test":-1514743201, // 1922-01-01 05:59:59
                                 24000:"America/Matamoros",
                                 24076:"America/Monterrey",
                              ]),
                           21600:"America/Mexico_City",
                        ]),
                  ]),
               21600:
                  ([ "test":105084000, // 1973-05-01 06:00:00
                     18000:"America/Managua",
                     21600:
                        ([ "test":-1822500432, // 1912-04-01 05:52:48
                           20173:"America/Costa_Rica",
                           20932:"America/Tegucigalpa",
                           21408:"America/El_Salvador",
                           21600:"America/Belize",
                           21724:"America/Guatemala",
                           25200:"America/Regina",
                        ]),
                  ]),
            ]),
         25200:
            ([ "test":-1514739601, // 1922-01-01 06:59:59
               25060:"America/Ojinaga",
               25460:"America/Chihuahua",
            ]),
      ]),
   25200:
      ([ "test":1289073600, // 2010-11-06 20:00:00
         18000:
            ([ "test":719956800, // 1992-10-24 20:00:00
               18000:"America/North_Dakota/Center",
               21600:
                  ([ "test":1067112000, // 2003-10-25 20:00:00
                     18000:"America/North_Dakota/New_Salem",
                     21600:"America/North_Dakota/Beulah",
                  ]),
            ]),
         21600:
            ([ "test":129114000, // 1974-02-03 09:00:00
               21600:
                  ([ "test":70909200, // 1972-03-31 17:00:00
                     21600:"America/Swift_Current",
                     25200:
                        ([ "test":-1577948400, // 1919-12-31 17:00:00
                           25200:"America/Denver",
                           28800:"America/Boise",
                        ]),
                  ]),
               25200:
                  ([ "test":-661539600, // 1949-01-14 07:00:00
                     25200:
                        ([ "test":941313600, // 1999-10-30 20:00:00
                           18000:"America/Cambridge_Bay",
                           21600:
                              ([ "test":-1998663968, // 1906-09-01 07:33:52
                                 0:"America/Yellowknife",
                                 25200:"America/Edmonton",
                              ]),
                        ]),
                     28800:"America/Bahia_Banderas",
                  ]),
            ]),
         25200:
            ([ "test":-1627833600, // 1918-06-02 08:00:00
               21600:"America/Phoenix",
               25200:"America/Creston",
               25540:"America/Mazatlan",
               26632:"America/Hermosillo",
            ]),
      ]),
   28800:
      ([ "test":1446372000, // 2015-11-01 10:00:00
         25200:
            ([ "test":83962800, // 1972-08-29 19:00:00
               25200:
                  ([ "test":536428800, // 1986-12-31 16:00:00
                     25200:"America/Dawson_Creek",
                     28800:"America/Fort_Nelson",
                  ]),
               28800:"America/Inuvik",
            ]),
         28800:
            ([ "test":-686073600, // 1948-04-05 08:00:00
               25200:
                  ([ "test":-1222963200, // 1931-04-01 08:00:00
                     25200:"America/Tijuana",
                     28800:"America/Los_Angeles",
                  ]),
               28800:"America/Vancouver",
               32400:"America/Whitehorse",
            ]),
         32400:
            ([ "test":325620000, // 1980-04-26 18:00:00
               28800:
                  ([ "test":438966000, // 1983-11-29 15:00:00
                     28800:"America/Metlakatla",
                     32400:"America/Sitka",
                  ]),
               32400:"America/Juneau",
            ]),
      ]),
   30600:"Pacific/Pitcairn",
   32400:
      ([ "test":120582000, // 1973-10-27 15:00:00
         28800:
            ([ "test":315504000, // 1979-12-31 16:00:00
               28800:"America/Dawson",
               32400:"America/Yakutat",
            ]),
         32400:"Pacific/Gambier",
      ]),
   34200:"Pacific/Marquesas",
   36000:
      ([ "test":436294800, // 1983-10-29 17:00:00
         28800:"America/Anchorage",
         36000:
            ([ "test":-1155436200, // 1933-05-21 21:30:00
               36000:"Pacific/Tahiti",
               37800:"Pacific/Honolulu",
            ]),
      ]),
   37800:"Pacific/Rarotonga",
   38400:"Pacific/Kiritimati",
   39600:
      ([ "test":1325242800, // 2011-12-30 11:00:00
         -50400:"Pacific/Apia",
         -46800:"Pacific/Fakaofo",
         32400:"America/Nome",
         36000:"America/Adak",
         39600:"Pacific/Pago_Pago",
      ]),
   41400:"Pacific/Niue",
   43200:
      ([ "test":307627200, // 1979-10-01 12:00:00
         39600:"Pacific/Enderbury",
         43200:"Pacific/Kwajalein",
      ]),
]);
