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
   "America":   ({"Danmarkshavn", "Scoresbysund", "Nuuk", "Thule", "New_York",
                  "Chicago", "North_Dakota/Center", "North_Dakota/New_Salem",
                  "North_Dakota/Beulah", "Denver", "Los_Angeles", "Juneau",
                  "Sitka", "Metlakatla", "Yakutat", "Anchorage", "Nome",
                  "Adak", "Phoenix", "Boise", "Indiana/Indianapolis",
                  "Indiana/Marengo", "Indiana/Vincennes", "Indiana/Tell_City",
                  "Indiana/Petersburg", "Indiana/Knox", "Indiana/Winamac",
                  "Indiana/Vevay", "Kentucky/Louisville",
                  "Kentucky/Monticello", "Detroit", "Menominee", "St_Johns",
                  "Goose_Bay", "Halifax", "Glace_Bay", "Moncton", "Toronto",
                  "Winnipeg", "Regina", "Swift_Current", "Edmonton",
                  "Vancouver", "Dawson_Creek", "Fort_Nelson", "Iqaluit",
                  "Resolute", "Rankin_Inlet", "Cambridge_Bay", "Inuvik",
                  "Whitehorse", "Dawson", "Cancun", "Merida", "Matamoros",
                  "Monterrey", "Mexico_City", "Ciudad_Juarez", "Ojinaga",
                  "Chihuahua", "Hermosillo", "Mazatlan", "Bahia_Banderas",
                  "Tijuana", "Barbados", "Belize", "Costa_Rica", "Havana",
                  "Santo_Domingo", "El_Salvador", "Guatemala",
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
                  "Rio_Branco", "Santiago", "Coyhaique", "Punta_Arenas",
                  "Bogota", "Guayaquil", "Cayenne", "Guyana", "Asuncion",
                  "Lima", "Paramaribo", "Montevideo", "Caracas", "Anguilla",
                  "Antigua", "Argentina/ComodRivadavia", "Aruba", "Atikokan",
                  "Blanc-Sablon", "Cayman", "Coral_Harbour", "Creston",
                  "Curacao", "Dominica", "Ensenada", "Grenada", "Guadeloupe",
                  "Montreal", "Montserrat", "Nassau", "Nipigon",
                  "Pangnirtung", "Port_of_Spain", "Rainy_River", "Rosario",
                  "St_Kitts", "St_Lucia", "St_Thomas", "St_Vincent",
                  "Thunder_Bay", "Tortola",
                  "Yellowknife"}),
   "Pacific":   ({"Fiji", "Gambier", "Marquesas", "Tahiti", "Guam", "Tarawa",
                  "Kanton", "Kiritimati", "Kwajalein", "Kosrae", "Nauru",
                  "Noumea", "Auckland", "Chatham", "Rarotonga", "Niue",
                  "Norfolk", "Palau", "Port_Moresby", "Bougainville",
                  "Pitcairn", "Pago_Pago", "Apia", "Guadalcanal", "Fakaofo",
                  "Tongatapu", "Efate", "Honolulu", "Easter", "Galapagos",
                  "Chuuk", "Enderbury", "Funafuti", "Johnston", "Majuro",
                  "Midway", "Pohnpei", "Saipan", "Wake",
                  "Wallis"}),
   "Antarctica":({"Casey", "Davis", "Mawson", "Troll", "Vostok", "Rothera",
                  "Macquarie", "Palmer", "DumontDUrville", "McMurdo",
                  "Syowa"}),
   "Atlantic":  ({"Cape_Verde", "Faroe", "Azores", "Madeira", "Canary",
                  "Bermuda", "Stanley", "South_Georgia", "Jan_Mayen",
                  "Reykjavik",
                  "St_Helena"}),
   "Indian":    ({"Mauritius", "Chagos", "Maldives", "Antananarivo",
                  "Christmas", "Cocos", "Comoro", "Kerguelen", "Mahe",
                  "Mayotte",
                  "Reunion"}),
   "Europe":    ({"London", "Dublin", "Tirane", "Andorra", "Vienna", "Minsk",
                  "Brussels", "Sofia", "Prague", "Tallinn", "Helsinki",
                  "Paris", "Berlin", "Gibraltar", "Athens", "Budapest",
                  "Rome", "Riga", "Vilnius", "Malta", "Chisinau", "Warsaw",
                  "Lisbon", "Bucharest", "Kaliningrad", "Moscow",
                  "Simferopol", "Astrakhan", "Volgograd", "Saratov", "Kirov",
                  "Samara", "Ulyanovsk", "Belgrade", "Madrid", "Zurich",
                  "Istanbul", "Kyiv", "Amsterdam", "Belfast", "Copenhagen",
                  "Guernsey", "Isle_of_Man", "Jersey", "Ljubljana",
                  "Luxembourg", "Monaco", "Oslo", "Sarajevo", "Skopje",
                  "Stockholm", "Tiraspol", "Uzhgorod", "Vaduz", "Zagreb",
                  "Zaporozhye"}),
   "Africa":    ({"Algiers", "Ndjamena", "Abidjan", "Cairo", "Bissau",
                  "Nairobi", "Monrovia", "Tripoli", "Casablanca", "El_Aaiun",
                  "Maputo", "Windhoek", "Lagos", "Sao_Tome", "Johannesburg",
                  "Khartoum", "Juba", "Tunis", "Ceuta", "Accra",
                  "Addis_Ababa", "Asmara", "Bamako", "Bangui", "Banjul",
                  "Blantyre", "Brazzaville", "Bujumbura", "Conakry", "Dakar",
                  "Dar_es_Salaam", "Djibouti", "Douala", "Freetown",
                  "Gaborone", "Harare", "Kampala", "Kigali", "Kinshasa",
                  "Libreville", "Lome", "Luanda", "Lubumbashi", "Lusaka",
                  "Malabo", "Maseru", "Mbabane", "Mogadishu", "Niamey",
                  "Nouakchott", "Ouagadougou", "Porto-Novo",
                  "Timbuktu"}),
   "Asia":      ({"Kabul", "Yerevan", "Baku", "Dhaka", "Thimphu", "Yangon",
                  "Shanghai", "Urumqi", "Hong_Kong", "Taipei", "Macau",
                  "Nicosia", "Famagusta", "Tbilisi", "Dili", "Kolkata",
                  "Jakarta", "Pontianak", "Makassar", "Jayapura", "Tehran",
                  "Baghdad", "Jerusalem", "Tokyo", "Amman", "Almaty",
                  "Qyzylorda", "Qostanay", "Aqtobe", "Aqtau", "Atyrau",
                  "Oral", "Bishkek", "Seoul", "Pyongyang", "Beirut",
                  "Kuching", "Hovd", "Ulaanbaatar", "Kathmandu", "Karachi",
                  "Gaza", "Hebron", "Manila", "Qatar", "Riyadh", "Singapore",
                  "Colombo", "Damascus", "Dushanbe", "Bangkok", "Ashgabat",
                  "Dubai", "Samarkand", "Tashkent", "Ho_Chi_Minh",
                  "Yekaterinburg", "Omsk", "Barnaul", "Novosibirsk", "Tomsk",
                  "Novokuznetsk", "Krasnoyarsk", "Irkutsk", "Chita",
                  "Yakutsk", "Vladivostok", "Khandyga", "Sakhalin", "Magadan",
                  "Srednekolymsk", "Ust-Nera", "Kamchatka", "Anadyr", "Aden",
                  "Bahrain", "Brunei", "Chongqing", "Hanoi", "Harbin",
                  "Kashgar", "Kuala_Lumpur", "Kuwait", "Muscat", "Phnom_Penh",
                  "Tel_Aviv",
                  "Vientiane"}),
   "Australia": ({"Darwin", "Perth", "Eucla", "Brisbane", "Lindeman",
                  "Adelaide", "Hobart", "Melbourne", "Sydney", "Broken_Hill",
                  "Lord_Howe",
                  "Currie"}),
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
   "+00": ({"Antarctica/Troll", "Atlantic/Azores", "Atlantic/Cape_Verde",
       "Etc/GMT+1", "Africa/El_Aaiun", "Africa/Bissau", "Africa/Casablanca",
       "America/Scoresbysund", "Atlantic/Reykjavik", "Atlantic/Madeira",
       "Atlantic/Canary"}),
   "+0020": ({"Africa/Accra", "Europe/Amsterdam"}),
   "+0030": ({"Africa/Accra", "Africa/Lagos"}),
   "+01": ({"Africa/Casablanca", "Africa/El_Aaiun", "Etc/GMT-1"}),
   "+0120": ({"Europe/Amsterdam"}),
   "+0130": ({"Africa/Lagos", "Africa/Windhoek"}),
   "+02": ({"Africa/Casablanca", "Africa/El_Aaiun", "Antarctica/Troll",
       "Etc/GMT-1", "Etc/GMT-2", "Europe/Ulyanovsk", "Europe/Samara"}),
   "+0220": ({"Europe/Zaporozhye"}),
   "+0230": ({"Africa/Mogadishu", "Africa/Kampala", "Africa/Nairobi",
       "Africa/Windhoek"}),
   "+0245": ({"Africa/Dar_es_Salaam", "Africa/Kampala", "Africa/Nairobi"}),
   "+03": ({"Antarctica/Syowa", "Asia/Aden", "Asia/Amman", "Asia/Baghdad",
       "Asia/Bahrain", "Asia/Damascus", "Asia/Kuwait", "Asia/Qatar",
       "Asia/Riyadh", "Etc/GMT-2", "Etc/GMT-3", "Europe/Istanbul",
       "Europe/Minsk", "Asia/Famagusta", "Europe/Saratov",
       "Europe/Astrakhan", "Europe/Ulyanovsk", "Europe/Kaliningrad",
       "Europe/Samara", "Asia/Yerevan", "Asia/Baku", "Asia/Tbilisi",
       "Asia/Atyrau", "Asia/Oral", "Europe/Kirov", "Europe/Volgograd"}),
   "+0330": ({"Asia/Tehran", "Asia/Bahrain", "Africa/Nairobi"}),
   "+0345": ({"Africa/Nairobi"}),
   "+04": ({"Asia/Amman", "Asia/Baghdad", "Asia/Baku", "Asia/Damascus",
       "Asia/Dubai", "Asia/Muscat", "Asia/Qatar", "Asia/Riyadh",
       "Asia/Tbilisi", "Asia/Yerevan", "Etc/GMT-3", "Etc/GMT-4",
       "Europe/Astrakhan", "Europe/Istanbul", "Europe/Minsk",
       "Europe/Samara", "Europe/Saratov", "Europe/Ulyanovsk", "Indian/Mahe",
       "Indian/Mauritius", "Indian/Reunion", "Europe/Volgograd",
       "Asia/Famagusta", "Europe/Kaliningrad", "Asia/Aqtau", "Asia/Atyrau",
       "Asia/Oral", "Europe/Kirov", "Asia/Aqtobe", "Asia/Qostanay",
       "Asia/Yekaterinburg", "Asia/Qyzylorda", "Asia/Bahrain",
       "Asia/Ashgabat", "Asia/Tehran", "Asia/Kabul", "Asia/Samarkand"}),
   "+0430": ({"Asia/Kabul", "Asia/Tehran"}),
   "+05": ({"Antarctica/Mawson", "Antarctica/Vostok", "Asia/Almaty",
       "Asia/Aqtau", "Asia/Aqtobe", "Asia/Ashgabat", "Asia/Atyrau",
       "Asia/Baku", "Asia/Dubai", "Asia/Dushanbe", "Asia/Oral",
       "Asia/Qostanay", "Asia/Qyzylorda", "Asia/Samarkand", "Asia/Tashkent",
       "Asia/Tbilisi", "Asia/Yekaterinburg", "Asia/Yerevan", "Etc/GMT-4",
       "Etc/GMT-5", "Europe/Astrakhan", "Europe/Samara", "Europe/Saratov",
       "Europe/Ulyanovsk", "Indian/Kerguelen", "Indian/Maldives",
       "Indian/Mauritius", "Europe/Volgograd", "Antarctica/Davis",
       "Indian/Chagos", "Europe/Kirov", "Asia/Omsk", "Asia/Kashgar",
       "Asia/Qatar", "Asia/Karachi", "Asia/Bishkek", "Asia/Tehran",
       "Europe/Moscow", "Asia/Kabul"}),
   "+0530": ({"Asia/Colombo", "Asia/Kabul", "Asia/Thimphu", "Asia/Kathmandu",
       "Asia/Karachi", "Asia/Kolkata", "Asia/Dhaka", "Asia/Kashgar"}),
   "+0545": ({"Asia/Kathmandu"}),
   "+06": ({"Antarctica/Mawson", "Antarctica/Vostok", "Asia/Almaty",
       "Asia/Aqtau", "Asia/Aqtobe", "Asia/Ashgabat", "Asia/Atyrau",
       "Asia/Bishkek", "Asia/Dhaka", "Asia/Dushanbe", "Asia/Omsk",
       "Asia/Oral", "Asia/Qostanay", "Asia/Qyzylorda", "Asia/Samarkand",
       "Asia/Tashkent", "Asia/Thimphu", "Asia/Urumqi", "Asia/Yekaterinburg",
       "Etc/GMT-5", "Etc/GMT-6", "Indian/Chagos", "Indian/Maldives",
       "Asia/Novosibirsk", "Asia/Tomsk", "Asia/Barnaul", "Antarctica/Davis",
       "Asia/Novokuznetsk", "Asia/Colombo", "Asia/Krasnoyarsk", "Asia/Hovd",
       "Asia/Karachi"}),
   "+0630": ({"Asia/Colombo", "Asia/Yangon", "Indian/Cocos", "Asia/Thimphu",
       "Asia/Kathmandu", "Asia/Karachi", "Asia/Dhaka", "Asia/Kolkata"}),
   "+0645": ({"Asia/Kathmandu"}),
   "+07": ({"Antarctica/Davis", "Asia/Bangkok", "Asia/Barnaul",
       "Asia/Bishkek", "Asia/Dhaka", "Asia/Hanoi", "Asia/Ho_Chi_Minh",
       "Asia/Hovd", "Asia/Krasnoyarsk", "Asia/Novokuznetsk",
       "Asia/Novosibirsk", "Asia/Omsk", "Asia/Phnom_Penh", "Asia/Thimphu",
       "Asia/Tomsk", "Asia/Urumqi", "Asia/Vientiane", "Etc/GMT-6",
       "Etc/GMT-7", "Indian/Chagos", "Indian/Christmas", "Asia/Almaty",
       "Asia/Qostanay", "Antarctica/Vostok", "Asia/Qyzylorda",
       "Asia/Yekaterinburg", "Antarctica/Mawson", "Asia/Colombo",
       "Asia/Irkutsk", "Asia/Dushanbe", "Asia/Aqtau", "Asia/Aqtobe",
       "Asia/Atyrau", "Asia/Oral", "Asia/Samarkand", "Asia/Chongqing",
       "Asia/Ulaanbaatar", "Asia/Tashkent", "Asia/Singapore",
       "Asia/Kuala_Lumpur"}),
   "+0720": ({"Asia/Kuala_Lumpur", "Asia/Singapore", "Asia/Jakarta"}),
   "+0730": ({"Asia/Yangon", "Asia/Colombo", "Asia/Kuala_Lumpur",
       "Asia/Singapore", "Asia/Jakarta", "Asia/Pontianak", "Asia/Dhaka",
       "Asia/Brunei", "Asia/Kuching"}),
   "+08": ({"Antarctica/Casey", "Antarctica/Davis", "Asia/Bangkok",
       "Asia/Barnaul", "Asia/Brunei", "Asia/Ho_Chi_Minh", "Asia/Hovd",
       "Asia/Irkutsk", "Asia/Krasnoyarsk", "Asia/Kuala_Lumpur",
       "Asia/Kuching", "Asia/Novokuznetsk", "Asia/Novosibirsk",
       "Asia/Singapore", "Asia/Tomsk", "Asia/Ulaanbaatar", "Etc/GMT-7",
       "Etc/GMT-8", "Antarctica/Vostok", "Asia/Chita", "Asia/Omsk",
       "Asia/Dili", "Asia/Khandyga", "Asia/Yakutsk", "Asia/Vientiane",
       "Asia/Hanoi", "Asia/Jakarta", "Asia/Pontianak", "Asia/Phnom_Penh",
       "Asia/Makassar", "Asia/Ust-Nera"}),
   "+0820": ({"Asia/Singapore", "Asia/Jakarta"}),
   "+0830": ({"Asia/Singapore", "Asia/Harbin", "Asia/Jakarta",
       "Asia/Pontianak", "Asia/Kuching"}),
   "+0845": ({"Australia/Eucla"}),
   "+09": ({"Antarctica/Casey", "Asia/Chita", "Asia/Dili", "Asia/Irkutsk",
       "Asia/Khandyga", "Asia/Kuching", "Asia/Singapore", "Asia/Ulaanbaatar",
       "Asia/Yakutsk", "Etc/GMT-8", "Etc/GMT-9", "Pacific/Palau",
       "Asia/Krasnoyarsk", "Asia/Vladivostok", "Asia/Ho_Chi_Minh",
       "Asia/Macau", "Asia/Ust-Nera", "Asia/Harbin", "Asia/Jakarta",
       "Asia/Pontianak", "Asia/Makassar", "Asia/Kuala_Lumpur",
       "Pacific/Nauru", "Asia/Hanoi", "Asia/Phnom_Penh", "Asia/Vientiane",
       "Asia/Sakhalin", "Pacific/Bougainville", "Pacific/Chuuk",
       "Pacific/Kosrae", "Pacific/Pohnpei", "Asia/Yangon", "Asia/Jayapura",
       "Pacific/Guam", "Pacific/Saipan", "Pacific/Kwajalein",
       "Pacific/Majuro"}),
   "+0930": ({"Asia/Jayapura"}),
   "+0945": ({"Australia/Eucla"}),
   "+10": ({"Antarctica/DumontDUrville", "Asia/Chita", "Asia/Dili",
       "Asia/Khandyga", "Asia/Ust-Nera", "Asia/Vladivostok", "Asia/Yakutsk",
       "Etc/GMT-10", "Etc/GMT-9", "Pacific/Chuuk", "Pacific/Palau",
       "Pacific/Port_Moresby", "Asia/Magadan", "Asia/Sakhalin",
       "Pacific/Bougainville", "Asia/Irkutsk", "Asia/Srednekolymsk",
       "Asia/Macau", "Asia/Jakarta", "Asia/Makassar", "Asia/Pontianak",
       "Asia/Kuching", "Asia/Singapore", "Pacific/Nauru", "Asia/Ho_Chi_Minh",
       "Pacific/Kosrae", "Asia/Yangon", "Asia/Jayapura", "Pacific/Guam",
       "Pacific/Kwajalein", "Pacific/Majuro", "Pacific/Pohnpei"}),
   "+1030": ({"Australia/Lord_Howe", "Asia/Jayapura"}),
   "+11": ({"Asia/Magadan", "Asia/Sakhalin", "Asia/Srednekolymsk",
       "Asia/Ust-Nera", "Asia/Vladivostok", "Etc/GMT-10", "Etc/GMT-11",
       "Pacific/Bougainville", "Pacific/Efate", "Pacific/Guadalcanal",
       "Pacific/Kosrae", "Pacific/Norfolk", "Pacific/Noumea",
       "Pacific/Pohnpei", "Pacific/Port_Moresby", "Antarctica/Casey",
       "Asia/Chita", "Asia/Khandyga", "Asia/Yakutsk", "Asia/Anadyr",
       "Asia/Kamchatka", "Pacific/Kwajalein", "Pacific/Majuro"}),
   "+1112": ({"Pacific/Norfolk"}),
   "+1130": ({"Australia/Lord_Howe", "Pacific/Norfolk", "Pacific/Nauru"}),
   "+12": ({"Asia/Anadyr", "Asia/Kamchatka", "Asia/Magadan", "Asia/Sakhalin",
       "Asia/Srednekolymsk", "Etc/GMT-11", "Etc/GMT-12",
       "Pacific/Bougainville", "Pacific/Efate", "Pacific/Fiji",
       "Pacific/Funafuti", "Pacific/Guadalcanal", "Pacific/Kosrae",
       "Pacific/Kwajalein", "Pacific/Majuro", "Pacific/Nauru",
       "Pacific/Norfolk", "Pacific/Noumea", "Pacific/Tarawa", "Pacific/Wake",
       "Pacific/Wallis", "Antarctica/Casey", "Asia/Ust-Nera",
       "Asia/Vladivostok", "Asia/Khandyga"}),
   "+1212": ({"Pacific/Norfolk"}),
   "+1215": ({"Pacific/Chatham"}),
   "+1220": ({"Pacific/Tongatapu"}),
   "+1230": ({"Pacific/Norfolk", "Pacific/Nauru"}),
   "+1245": ({"Pacific/Chatham"}),
   "+13": ({"Asia/Anadyr", "Asia/Kamchatka", "Etc/GMT-12", "Etc/GMT-13",
       "Pacific/Apia", "Pacific/Fakaofo", "Pacific/Fiji", "Pacific/Kanton",
       "Pacific/Kwajalein", "Pacific/Nauru", "Pacific/Tarawa",
       "Pacific/Tongatapu", "Asia/Magadan", "Asia/Srednekolymsk",
       "Asia/Ust-Nera", "Pacific/Kosrae"}),
   "+1315": ({"Pacific/Chatham"}),
   "+1320": ({"Pacific/Tongatapu"}),
   "+1345": ({"Pacific/Chatham"}),
   "+14": ({"Etc/GMT-13", "Etc/GMT-14", "Pacific/Apia", "Pacific/Fakaofo",
       "Pacific/Kanton", "Pacific/Kiritimati", "Pacific/Tongatapu",
       "Asia/Anadyr"}),
   "+15": ({"Etc/GMT-14", "Pacific/Kiritimati"}),
   "-00": ({"Factory", "Pacific/Enderbury", "Antarctica/Troll",
       "Antarctica/Vostok", "Antarctica/Rothera", "Antarctica/Davis",
       "Antarctica/Casey", "Antarctica/Palmer", "Antarctica/Syowa",
       "America/Rankin_Inlet", "Antarctica/DumontDUrville",
       "Antarctica/McMurdo", "Antarctica/Mawson", "America/Inuvik",
       "Indian/Kerguelen", "Antarctica/Macquarie", "America/Resolute",
       "America/Iqaluit", "Pacific/Kanton", "America/Yellowknife",
       "America/Pangnirtung", "America/Cambridge_Bay"}),
   "-0040": ({"Africa/Freetown"}),
   "-01": ({"America/Noronha", "America/Nuuk", "America/Scoresbysund",
       "Atlantic/Azores", "Atlantic/Cape_Verde", "Atlantic/Jan_Mayen",
       "Atlantic/South_Georgia", "Etc/GMT+1", "Etc/GMT+2", "Africa/El_Aaiun",
       "Africa/Bissau", "Africa/Freetown", "Atlantic/Reykjavik",
       "Atlantic/Madeira", "Africa/Nouakchott", "Africa/Bamako",
       "Africa/Conakry", "Atlantic/Canary", "Africa/Banjul", "Africa/Dakar",
       "Africa/Niamey"}),
   "-0130": ({"America/Paramaribo", "America/Montevideo"}),
   "-0145": ({"America/Guyana"}),
   "-02": ({"America/Araguaina", "America/Argentina/Buenos_Aires",
       "America/Argentina/Catamarca", "America/Argentina/Cordoba",
       "America/Argentina/Jujuy", "America/Argentina/La_Rioja",
       "America/Argentina/Mendoza", "America/Argentina/Rio_Gallegos",
       "America/Argentina/Salta", "America/Argentina/San_Juan",
       "America/Argentina/San_Luis", "America/Argentina/Tucuman",
       "America/Argentina/Ushuaia", "America/Asuncion", "America/Bahia",
       "America/Belem", "America/Cayenne", "America/Coyhaique",
       "America/Fortaleza", "America/Maceio", "America/Miquelon",
       "America/Montevideo", "America/Noronha", "America/Nuuk",
       "America/Paramaribo", "America/Punta_Arenas", "America/Recife",
       "America/Santarem", "America/Sao_Paulo", "America/Scoresbysund",
       "Antarctica/Palmer", "Antarctica/Rothera", "Atlantic/South_Georgia",
       "Atlantic/Stanley", "Etc/GMT+2", "Etc/GMT+3", "America/Guyana",
       "America/Danmarkshavn", "Atlantic/Cape_Verde",
       "America/Argentina/ComodRivadavia", "America/Rosario",
       "Atlantic/Azores"}),
   "-0230": ({"America/Caracas", "America/Paramaribo", "America/Montevideo"}),
   "-0245": ({"America/Guyana"}),
   "-03": ({"America/Araguaina", "America/Argentina/Buenos_Aires",
       "America/Argentina/Catamarca", "America/Argentina/ComodRivadavia",
       "America/Argentina/Cordoba", "America/Argentina/Jujuy",
       "America/Argentina/La_Rioja", "America/Argentina/Mendoza",
       "America/Argentina/Rio_Gallegos", "America/Argentina/Salta",
       "America/Argentina/San_Juan", "America/Argentina/San_Luis",
       "America/Argentina/Tucuman", "America/Argentina/Ushuaia",
       "America/Asuncion", "America/Bahia", "America/Belem",
       "America/Boa_Vista", "America/Campo_Grande", "America/Caracas",
       "America/Cayenne", "America/Coyhaique", "America/Cuiaba",
       "America/Fortaleza", "America/Guyana", "America/La_Paz",
       "America/Maceio", "America/Manaus", "America/Miquelon",
       "America/Montevideo", "America/Paramaribo", "America/Porto_Velho",
       "America/Punta_Arenas", "America/Recife", "America/Rosario",
       "America/Santarem", "America/Santiago", "America/Sao_Paulo",
       "Antarctica/Palmer", "Antarctica/Rothera", "Atlantic/Stanley",
       "Etc/GMT+3", "Etc/GMT+4", "America/Nuuk", "America/Eirunepe",
       "America/Rio_Branco", "America/Danmarkshavn"}),
   "-0330": ({"America/Caracas", "America/Barbados"}),
   "-04": ({"America/Boa_Vista", "America/Bogota", "America/Campo_Grande",
       "America/Caracas", "America/Cuiaba", "America/Eirunepe",
       "America/Guayaquil", "America/Guyana", "America/La_Paz",
       "America/Lima", "America/Manaus", "America/Porto_Velho",
       "America/Rio_Branco", "America/Santiago", "Etc/GMT+4", "Etc/GMT+5",
       "America/Santarem", "America/Argentina/Mendoza",
       "America/Argentina/San_Juan", "America/Argentina/San_Luis",
       "America/Argentina/Catamarca", "America/Argentina/ComodRivadavia",
       "America/Argentina/La_Rioja", "America/Argentina/Rio_Gallegos",
       "America/Argentina/Ushuaia", "America/Argentina/Tucuman",
       "America/Argentina/Cordoba", "America/Argentina/Salta",
       "America/Argentina/Jujuy", "Pacific/Galapagos", "America/Asuncion",
       "America/Argentina/Buenos_Aires", "America/Coyhaique",
       "America/Punta_Arenas", "America/Rosario", "Antarctica/Palmer",
       "Atlantic/Stanley", "America/Cayenne", "America/Montevideo"}),
   "-0430": ({"America/Santo_Domingo", "America/Aruba", "America/Curacao"}),
   "-05": ({"America/Bogota", "America/Eirunepe", "America/Guayaquil",
       "America/Lima", "America/Rio_Branco", "Etc/GMT+5", "Etc/GMT+6",
       "Pacific/Easter", "Pacific/Galapagos", "America/Coyhaique",
       "America/Punta_Arenas", "America/Santiago"}),
   "-0530": ({"America/Belize"}),
   "-06": ({"Etc/GMT+6", "Etc/GMT+7", "Pacific/Easter", "Pacific/Galapagos"}),
   "-0630": ({"Pacific/Pitcairn"}),
   "-07": ({"Etc/GMT+7", "Etc/GMT+8", "Pacific/Pitcairn", "Pacific/Easter"}),
   "-0730": ({"Pacific/Marquesas", "Pacific/Pitcairn"}),
   "-08": ({"Etc/GMT+8", "Etc/GMT+9", "Pacific/Gambier", "Pacific/Pitcairn"}),
   "-0830": ({"Pacific/Marquesas", "Pacific/Rarotonga"}),
   "-0840": ({"Pacific/Kiritimati"}),
   "-09": ({"Etc/GMT+10", "Etc/GMT+9", "Pacific/Gambier",
       "Pacific/Rarotonga", "Pacific/Tahiti", "Pacific/Kiritimati"}),
   "-0920": ({"Pacific/Niue"}),
   "-0930": ({"Pacific/Rarotonga", "Pacific/Apia"}),
   "-0940": ({"Pacific/Kiritimati"}),
   "-10": ({"Etc/GMT+10", "Etc/GMT+11", "Pacific/Niue", "Pacific/Rarotonga",
       "Pacific/Tahiti", "Pacific/Fakaofo", "Pacific/Kanton",
       "Pacific/Kiritimati", "Pacific/Apia", "Pacific/Midway"}),
   "-1020": ({"Pacific/Niue"}),
   "-1030": ({"Pacific/Apia"}),
   "-11": ({"Etc/GMT+11", "Etc/GMT+12", "Pacific/Niue", "Pacific/Fakaofo",
       "Pacific/Kanton", "Pacific/Kwajalein", "Pacific/Apia",
       "Pacific/Midway"}),
   "-12": ({"Etc/GMT+12", "Pacific/Kwajalein", "Pacific/Kanton",
       "Pacific/Enderbury"}),
   "ACDT": ({"Australia/Adelaide", "Australia/Broken_Hill",
       "Australia/Darwin"}),
   "ACST": ({"Australia/Adelaide", "Australia/Broken_Hill",
       "Australia/Darwin"}),
   "ADDT": ({"America/Goose_Bay"}),
   "ADMT": ({"Africa/Addis_Ababa", "Africa/Asmara"}),
   "ADT": ({"America/Barbados", "America/Glace_Bay", "America/Goose_Bay",
       "America/Halifax", "America/Moncton", "America/Thule",
       "Atlantic/Bermuda", "America/Martinique", "America/Anchorage",
       "America/Blanc-Sablon", "America/Puerto_Rico"}),
   "AEDT": ({"Antarctica/Macquarie", "Australia/Brisbane",
       "Australia/Currie", "Australia/Hobart", "Australia/Lindeman",
       "Australia/Melbourne", "Australia/Sydney"}),
   "AEST": ({"Antarctica/Macquarie", "Australia/Brisbane",
       "Australia/Currie", "Australia/Hobart", "Australia/Lindeman",
       "Australia/Melbourne", "Australia/Sydney", "Australia/Lord_Howe",
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
       "America/Blanc-Sablon", "America/Puerto_Rico"}),
   "AST": ({"America/Anguilla", "America/Antigua", "America/Aruba",
       "America/Barbados", "America/Blanc-Sablon", "America/Curacao",
       "America/Dominica", "America/Glace_Bay", "America/Goose_Bay",
       "America/Grenada", "America/Guadeloupe", "America/Halifax",
       "America/Martinique", "America/Moncton", "America/Montserrat",
       "America/Port_of_Spain", "America/Puerto_Rico",
       "America/Santo_Domingo", "America/St_Kitts", "America/St_Lucia",
       "America/St_Thomas", "America/St_Vincent", "America/Thule",
       "America/Tortola", "Atlantic/Bermuda", "America/Grand_Turk",
       "America/Miquelon", "America/Anchorage"}),
   "AT": ({"America/Barbados", "Atlantic/Bermuda"}),
   "AWDT": ({"Australia/Perth"}),
   "AWST": ({"Australia/Perth"}),
   "AWT": ({"America/Glace_Bay", "America/Goose_Bay", "America/Halifax",
       "America/Moncton", "Atlantic/Bermuda", "America/Anchorage",
       "America/Blanc-Sablon", "America/Puerto_Rico"}),
   "BDST": ({"Europe/Belfast", "Europe/Dublin", "Europe/Gibraltar",
       "Europe/Guernsey", "Europe/Isle_of_Man", "Europe/Jersey",
       "Europe/London"}),
   "BDT": ({"America/Adak", "America/Nome"}),
   "BMT": ({"Atlantic/Bermuda", "Africa/Banjul", "Europe/Bucharest",
       "Europe/Chisinau", "Europe/Tiraspol", "Asia/Jakarta", "Asia/Bangkok",
       "Asia/Baghdad", "America/Bogota", "Europe/Zurich", "Europe/Brussels"}),
   "BPT": ({"America/Adak", "America/Nome"}),
   "BST": ({"Europe/Belfast", "Europe/Guernsey", "Europe/Isle_of_Man",
       "Europe/Jersey", "Europe/London", "America/Adak", "America/Nome",
       "Atlantic/Bermuda", "Europe/Dublin", "Europe/Gibraltar",
       "America/La_Paz"}),
   "BWT": ({"America/Adak", "America/Nome"}),
   "CAST": ({"Africa/Juba", "Africa/Khartoum", "Africa/Gaborone"}),
   "CAT": ({"Africa/Blantyre", "Africa/Bujumbura", "Africa/Gaborone",
       "Africa/Harare", "Africa/Juba", "Africa/Khartoum", "Africa/Kigali",
       "Africa/Lubumbashi", "Africa/Lusaka", "Africa/Maputo",
       "Africa/Windhoek"}),
   "CDT": ({"America/Bahia_Banderas", "America/Belize", "America/Chicago",
       "America/Costa_Rica", "America/El_Salvador", "America/Guatemala",
       "America/Havana", "America/Indiana/Knox", "America/Indiana/Tell_City",
       "America/Managua", "America/Matamoros", "America/Menominee",
       "America/Merida", "America/Mexico_City", "America/Monterrey",
       "America/North_Dakota/Beulah", "America/North_Dakota/Center",
       "America/North_Dakota/New_Salem", "America/Ojinaga",
       "America/Rainy_River", "America/Rankin_Inlet", "America/Resolute",
       "America/Tegucigalpa", "America/Winnipeg", "Asia/Chongqing",
       "Asia/Harbin", "Asia/Kashgar", "Asia/Macau", "Asia/Shanghai",
       "Asia/Taipei", "CST6CDT", "America/Indiana/Marengo",
       "America/Kentucky/Louisville", "America/Atikokan",
       "America/Cambridge_Bay", "America/Cancun", "America/Chihuahua",
       "America/Ciudad_Juarez", "America/Indiana/Indianapolis",
       "America/Indiana/Petersburg", "America/Indiana/Vevay",
       "America/Indiana/Vincennes", "America/Indiana/Winamac",
       "America/Iqaluit", "America/Kentucky/Monticello",
       "America/Pangnirtung"}),
   "CEMT": ({"Europe/Berlin", "Europe/Madrid", "Europe/Monaco",
       "Europe/Paris"}),
   "CEST": ({"Africa/Ceuta", "Africa/Tunis", "CET", "Europe/Amsterdam",
       "Europe/Andorra", "Europe/Belgrade", "Europe/Berlin",
       "Europe/Brussels", "Europe/Budapest", "Europe/Copenhagen",
       "Europe/Gibraltar", "Europe/Ljubljana", "Europe/Luxembourg",
       "Europe/Madrid", "Europe/Malta", "Europe/Monaco", "Europe/Oslo",
       "Europe/Paris", "Europe/Prague", "Europe/Rome", "Europe/Sarajevo",
       "Europe/Skopje", "Europe/Stockholm", "Europe/Tirane", "Europe/Vaduz",
       "Europe/Vienna", "Europe/Warsaw", "Europe/Zagreb", "Europe/Zurich",
       "Europe/Vilnius", "Europe/Lisbon", "Africa/Algiers", "Africa/Tripoli",
       "Europe/Athens", "Europe/Chisinau", "Europe/Guernsey",
       "Europe/Jersey", "Europe/Kaliningrad", "Europe/Kyiv", "Europe/Minsk",
       "Europe/Riga", "Europe/Simferopol", "Europe/Sofia", "Europe/Tallinn",
       "Europe/Tiraspol", "Europe/Uzhgorod", "Europe/Zaporozhye"}),
   "CET": ({"Africa/Algiers", "Africa/Ceuta", "Africa/Tunis", "CET",
       "Europe/Amsterdam", "Europe/Andorra", "Europe/Belgrade",
       "Europe/Berlin", "Europe/Brussels", "Europe/Budapest",
       "Europe/Copenhagen", "Europe/Gibraltar", "Europe/Ljubljana",
       "Europe/Luxembourg", "Europe/Madrid", "Europe/Malta", "Europe/Monaco",
       "Europe/Oslo", "Europe/Paris", "Europe/Prague", "Europe/Rome",
       "Europe/Sarajevo", "Europe/Skopje", "Europe/Stockholm",
       "Europe/Tirane", "Europe/Vaduz", "Europe/Vienna", "Europe/Warsaw",
       "Europe/Zagreb", "Europe/Zurich", "Europe/Vilnius", "Europe/Lisbon",
       "Europe/Uzhgorod", "Africa/Tripoli", "Europe/Athens",
       "Europe/Chisinau", "Europe/Guernsey", "Europe/Jersey",
       "Europe/Kaliningrad", "Europe/Kyiv", "Europe/Minsk", "Europe/Riga",
       "Europe/Simferopol", "Europe/Sofia", "Europe/Tallinn",
       "Europe/Tiraspol", "Europe/Zaporozhye"}),
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
   "CPT": ({"America/Belize", "America/Chicago", "America/Indiana/Knox",
       "America/Indiana/Tell_City", "America/Matamoros", "America/Menominee",
       "America/North_Dakota/Beulah", "America/North_Dakota/Center",
       "America/North_Dakota/New_Salem", "America/Ojinaga",
       "America/Rainy_River", "America/Rankin_Inlet", "America/Resolute",
       "America/Winnipeg", "CST6CDT", "America/Atikokan",
       "America/Cambridge_Bay", "America/Indiana/Indianapolis",
       "America/Indiana/Marengo", "America/Indiana/Petersburg",
       "America/Indiana/Vevay", "America/Indiana/Vincennes",
       "America/Indiana/Winamac", "America/Iqaluit",
       "America/Kentucky/Louisville", "America/Kentucky/Monticello",
       "America/Monterrey", "America/Pangnirtung"}),
   "CST": ({"America/Bahia_Banderas", "America/Belize", "America/Chicago",
       "America/Chihuahua", "America/Costa_Rica", "America/El_Salvador",
       "America/Guatemala", "America/Havana", "America/Indiana/Knox",
       "America/Indiana/Tell_City", "America/Managua", "America/Matamoros",
       "America/Menominee", "America/Merida", "America/Mexico_City",
       "America/Monterrey", "America/North_Dakota/Beulah",
       "America/North_Dakota/Center", "America/North_Dakota/New_Salem",
       "America/Ojinaga", "America/Rainy_River", "America/Rankin_Inlet",
       "America/Regina", "America/Resolute", "America/Swift_Current",
       "America/Tegucigalpa", "America/Winnipeg", "Asia/Chongqing",
       "Asia/Harbin", "Asia/Kashgar", "Asia/Macau", "Asia/Shanghai",
       "Asia/Taipei", "CST6CDT", "America/Ciudad_Juarez",
       "America/Cambridge_Bay", "America/Cancun", "America/Atikokan",
       "America/Indiana/Indianapolis", "America/Indiana/Marengo",
       "America/Indiana/Petersburg", "America/Indiana/Vevay",
       "America/Indiana/Vincennes", "America/Indiana/Winamac",
       "America/Iqaluit", "America/Kentucky/Louisville",
       "America/Kentucky/Monticello", "America/Pangnirtung",
       "America/Hermosillo", "America/Mazatlan", "America/Detroit",
       "America/Thunder_Bay"}),
   "CT": ({"Asia/Macau"}),
   "CWT": ({"America/Bahia_Banderas", "America/Belize", "America/Chicago",
       "America/Indiana/Knox", "America/Indiana/Tell_City",
       "America/Matamoros", "America/Menominee", "America/Merida",
       "America/Mexico_City", "America/Monterrey",
       "America/North_Dakota/Beulah", "America/North_Dakota/Center",
       "America/North_Dakota/New_Salem", "America/Ojinaga",
       "America/Rainy_River", "America/Rankin_Inlet", "America/Resolute",
       "America/Winnipeg", "CST6CDT", "America/Atikokan",
       "America/Cambridge_Bay", "America/Cancun", "America/Chihuahua",
       "America/Ciudad_Juarez", "America/Indiana/Indianapolis",
       "America/Indiana/Marengo", "America/Indiana/Petersburg",
       "America/Indiana/Vevay", "America/Indiana/Vincennes",
       "America/Indiana/Winamac", "America/Iqaluit",
       "America/Kentucky/Louisville", "America/Kentucky/Monticello",
       "America/Pangnirtung"}),
   "ChST": ({"Pacific/Guam", "Pacific/Saipan"}),
   "DFT": ({"Europe/Oslo", "Europe/Paris"}),
   "DMT": ({"Europe/Belfast", "Europe/Dublin"}),
   "EAST": ({"Indian/Antananarivo"}),
   "EAT": ({"Africa/Addis_Ababa", "Africa/Asmara", "Africa/Dar_es_Salaam",
       "Africa/Djibouti", "Africa/Kampala", "Africa/Mogadishu",
       "Africa/Nairobi", "Indian/Antananarivo", "Indian/Comoro",
       "Indian/Mayotte", "Africa/Juba", "Africa/Khartoum"}),
   "EDT": ({"America/Detroit", "America/Grand_Turk",
       "America/Indiana/Indianapolis", "America/Indiana/Marengo",
       "America/Indiana/Petersburg", "America/Indiana/Vevay",
       "America/Indiana/Vincennes", "America/Indiana/Winamac",
       "America/Iqaluit", "America/Kentucky/Louisville",
       "America/Kentucky/Monticello", "America/Montreal", "America/Nassau",
       "America/New_York", "America/Nipigon", "America/Pangnirtung",
       "America/Port-au-Prince", "America/Thunder_Bay", "America/Toronto",
       "EST5EDT", "America/Cancun", "America/Coral_Harbour",
       "America/Indiana/Tell_City", "America/Jamaica",
       "America/Santo_Domingo"}),
   "EEST": ({"Africa/Cairo", "Asia/Beirut", "Asia/Famagusta", "Asia/Gaza",
       "Asia/Hebron", "Asia/Nicosia", "EET", "Europe/Athens",
       "Europe/Bucharest", "Europe/Chisinau", "Europe/Helsinki",
       "Europe/Kyiv", "Europe/Riga", "Europe/Sofia", "Europe/Tallinn",
       "Europe/Uzhgorod", "Europe/Vilnius", "Europe/Zaporozhye",
       "Asia/Amman", "Europe/Istanbul", "Europe/Kaliningrad", "Europe/Minsk",
       "Europe/Moscow", "Asia/Damascus", "Europe/Simferopol",
       "Europe/Tiraspol", "Europe/Warsaw"}),
   "EET": ({"Africa/Cairo", "Africa/Tripoli", "Asia/Beirut",
       "Asia/Famagusta", "Asia/Gaza", "Asia/Hebron", "Asia/Nicosia", "EET",
       "Europe/Athens", "Europe/Bucharest", "Europe/Chisinau",
       "Europe/Helsinki", "Europe/Kaliningrad", "Europe/Kyiv", "Europe/Riga",
       "Europe/Sofia", "Europe/Tallinn", "Europe/Uzhgorod", "Europe/Vilnius",
       "Europe/Zaporozhye", "Asia/Amman", "Europe/Istanbul", "Europe/Minsk",
       "Europe/Simferopol", "Europe/Moscow", "Asia/Damascus",
       "Europe/Tiraspol", "Europe/Warsaw"}),
   "EMT": ({"Pacific/Easter"}),
   "EPT": ({"America/Detroit", "America/Grand_Turk",
       "America/Indiana/Indianapolis", "America/Indiana/Marengo",
       "America/Indiana/Petersburg", "America/Indiana/Vevay",
       "America/Indiana/Vincennes", "America/Indiana/Winamac",
       "America/Iqaluit", "America/Kentucky/Louisville",
       "America/Kentucky/Monticello", "America/Montreal", "America/Nassau",
       "America/New_York", "America/Nipigon", "America/Pangnirtung",
       "America/Thunder_Bay", "America/Toronto", "EST5EDT",
       "America/Coral_Harbour", "America/Indiana/Tell_City",
       "America/Jamaica", "America/Santo_Domingo"}),
   "EST": ({"America/Atikokan", "America/Cancun", "America/Cayman",
       "America/Coral_Harbour", "America/Detroit", "America/Grand_Turk",
       "America/Indiana/Indianapolis", "America/Indiana/Marengo",
       "America/Indiana/Petersburg", "America/Indiana/Vevay",
       "America/Indiana/Vincennes", "America/Indiana/Winamac",
       "America/Iqaluit", "America/Jamaica", "America/Kentucky/Louisville",
       "America/Kentucky/Monticello", "America/Montreal", "America/Nassau",
       "America/New_York", "America/Nipigon", "America/Panama",
       "America/Pangnirtung", "America/Port-au-Prince",
       "America/Thunder_Bay", "America/Toronto", "EST", "EST5EDT",
       "America/Resolute", "America/Indiana/Knox",
       "America/Indiana/Tell_City", "America/Rankin_Inlet",
       "America/Cambridge_Bay", "America/Managua", "America/Merida",
       "America/Menominee", "America/Santo_Domingo", "America/Antigua",
       "America/Chicago", "America/Moncton"}),
   "EWT": ({"America/Detroit", "America/Grand_Turk",
       "America/Indiana/Indianapolis", "America/Indiana/Marengo",
       "America/Indiana/Petersburg", "America/Indiana/Vevay",
       "America/Indiana/Vincennes", "America/Indiana/Winamac",
       "America/Iqaluit", "America/Kentucky/Louisville",
       "America/Kentucky/Monticello", "America/Montreal", "America/Nassau",
       "America/New_York", "America/Nipigon", "America/Pangnirtung",
       "America/Thunder_Bay", "America/Toronto", "EST5EDT", "America/Cancun",
       "America/Coral_Harbour", "America/Indiana/Tell_City",
       "America/Jamaica", "America/Santo_Domingo"}),
   "FFMT": ({"America/Martinique"}),
   "FMT": ({"Africa/Freetown", "Atlantic/Madeira"}),
   "GDT": ({"Pacific/Guam", "Pacific/Saipan"}),
   "GMT": ({"Africa/Abidjan", "Africa/Accra", "Africa/Bamako",
       "Africa/Banjul", "Africa/Bissau", "Africa/Conakry", "Africa/Dakar",
       "Africa/Freetown", "Africa/Lome", "Africa/Monrovia",
       "Africa/Nouakchott", "Africa/Ouagadougou", "Africa/Sao_Tome",
       "Africa/Timbuktu", "America/Danmarkshavn", "Atlantic/Reykjavik",
       "Atlantic/St_Helena", "Etc/GMT", "Europe/Belfast", "Europe/Dublin",
       "Europe/Guernsey", "Europe/Isle_of_Man", "Europe/Jersey",
       "Europe/London", "Europe/Gibraltar", "Africa/Malabo", "Africa/Niamey",
       "Europe/Prague", "Africa/Porto-Novo", "Africa/Lagos"}),
   "GST": ({"Pacific/Guam", "Pacific/Saipan"}),
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
   "KMT": ({"Europe/Kyiv", "Europe/Vilnius", "America/Cayman",
       "America/Grand_Turk", "America/Jamaica", "America/St_Vincent"}),
   "KST": ({"Asia/Pyongyang", "Asia/Seoul"}),
   "LMT": ({"Pacific/Niue", "Pacific/Rarotonga", "Asia/Aden", "Asia/Kuwait",
       "Asia/Thimphu", "Asia/Riyadh", "Pacific/Tongatapu", "Asia/Bahrain",
       "Africa/Kigali", "Africa/El_Aaiun", "Asia/Jayapura",
       "Pacific/Galapagos", "Africa/Juba", "Africa/Khartoum", "Asia/Amman",
       "Africa/Dar_es_Salaam", "Africa/Kampala", "Asia/Kashgar",
       "Asia/Urumqi", "Asia/Chongqing", "Asia/Harbin", "Asia/Kuching",
       "Asia/Brunei", "Asia/Yerevan", "Asia/Baku", "Asia/Aqtau", "Asia/Oral",
       "Asia/Atyrau", "Asia/Aqtobe", "Asia/Ashgabat", "Asia/Qostanay",
       "Asia/Qyzylorda", "Asia/Samarkand", "Asia/Dushanbe", "Asia/Tashkent",
       "Asia/Bishkek", "Asia/Almaty", "Asia/Magadan", "Asia/Srednekolymsk",
       "Asia/Anadyr", "Europe/Astrakhan", "Asia/Novokuznetsk",
       "Asia/Vladivostok", "Asia/Kamchatka", "Atlantic/Canary",
       "America/Ensenada", "America/Bahia_Banderas", "America/Chihuahua",
       "America/Ciudad_Juarez", "America/Hermosillo", "America/Mazatlan",
       "America/Mexico_City", "America/Ojinaga", "America/Tijuana",
       "America/Cancun", "America/Matamoros", "America/Merida",
       "America/Monterrey", "Asia/Nicosia", "Asia/Famagusta",
       "America/Tegucigalpa", "Pacific/Nauru", "America/El_Salvador",
       "Asia/Krasnoyarsk", "Europe/Volgograd", "Africa/Tripoli",
       "Asia/Damascus", "Asia/Qatar", "Asia/Dubai", "Asia/Muscat",
       "Asia/Kathmandu", "Asia/Makassar", "Asia/Tomsk", "Asia/Chita",
       "Asia/Yakutsk", "Asia/Khandyga", "Asia/Ust-Nera", "Asia/Novosibirsk",
       "Asia/Barnaul", "Asia/Omsk", "Europe/Kirov", "Europe/Samara",
       "Europe/Saratov", "Europe/Ulyanovsk", "America/Guatemala",
       "America/Thule", "America/Nuuk", "America/Scoresbysund",
       "America/Danmarkshavn", "Asia/Yekaterinburg", "Asia/Tehran",
       "Africa/Accra", "Pacific/Fiji", "America/Eirunepe",
       "America/Rio_Branco", "America/Porto_Velho", "America/Boa_Vista",
       "America/Manaus", "America/Cuiaba", "America/Santarem",
       "America/Campo_Grande", "America/Belem", "America/Araguaina",
       "America/Sao_Paulo", "America/Bahia", "America/Fortaleza",
       "America/Maceio", "America/Recife", "America/Noronha", "Africa/Lagos",
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
       "Africa/Bangui", "Asia/Dili", "America/Barbados", "America/Guyana",
       "Africa/Blantyre", "America/St_Thomas", "America/Tortola",
       "America/Montserrat", "America/Grenada", "America/Dominica",
       "America/Cayenne", "Africa/Djibouti", "Indian/Comoro",
       "Indian/Mayotte", "Indian/Antananarivo", "America/Miquelon",
       "America/Guadeloupe", "Indian/Reunion", "Pacific/Apia",
       "Pacific/Pago_Pago", "America/Paramaribo", "Africa/Maputo",
       "America/Lima", "America/Montevideo", "Africa/Nairobi",
       "Asia/Pontianak", "Asia/Pyongyang", "Asia/Seoul", "Atlantic/Faroe",
       "Atlantic/Reykjavik", "Indian/Mahe", "Indian/Mauritius",
       "Asia/Karachi", "Indian/Chagos", "America/Edmonton", "Asia/Vientiane",
       "Asia/Phnom_Penh", "Asia/Hanoi", "Asia/Ho_Chi_Minh",
       "America/Swift_Current", "America/Regina", "Asia/Sakhalin",
       "Asia/Hovd", "Asia/Ulaanbaatar", "America/Detroit", "Asia/Hong_Kong",
       "Asia/Macau", "Europe/Luxembourg", "Africa/Maseru", "Africa/Lusaka",
       "Africa/Harare", "Africa/Mbabane", "America/Halifax",
       "America/Glace_Bay", "Pacific/Midway", "Pacific/Fakaofo",
       "Pacific/Kiritimati", "Pacific/Pitcairn", "Africa/Ceuta",
       "Europe/Madrid", "Europe/Andorra", "Asia/Kuala_Lumpur",
       "Asia/Singapore", "Asia/Shanghai", "Pacific/Palau", "Pacific/Guam",
       "Pacific/Saipan", "Pacific/Chuuk", "Pacific/Pohnpei",
       "Pacific/Kosrae", "Pacific/Wake", "Pacific/Kwajalein",
       "Pacific/Norfolk", "Pacific/Majuro", "Pacific/Tarawa",
       "Pacific/Funafuti", "Pacific/Wallis", "Africa/Cairo", "Asia/Gaza",
       "Asia/Hebron", "America/Adak", "America/Nome", "America/Anchorage",
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
       "Europe/Kaliningrad", "Africa/Lome", "Europe/Monaco",
       "Africa/Windhoek", "Africa/Johannesburg", "Europe/Bucharest",
       "Europe/Paris", "Africa/Algiers", "Europe/Budapest",
       "Europe/Uzhgorod", "Pacific/Easter", "America/Managua",
       "America/Costa_Rica", "America/Havana", "America/Cayman",
       "America/Guayaquil", "America/Panama", "America/Jamaica",
       "America/Port-au-Prince", "America/Coyhaique", "America/Grand_Turk",
       "America/Punta_Arenas", "America/Santiago", "America/Santo_Domingo",
       "America/La_Paz", "America/Caracas", "Atlantic/Bermuda",
       "America/St_Vincent", "America/Martinique", "America/St_Lucia",
       "Atlantic/Stanley", "America/Asuncion", "Atlantic/South_Georgia",
       "Atlantic/St_Helena", "Europe/Copenhagen", "Africa/Bujumbura",
       "Asia/Baghdad", "Asia/Kabul", "Asia/Dhaka", "Asia/Tokyo",
       "America/Winnipeg", "America/Menominee", "Pacific/Enderbury",
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
       "Europe/Istanbul", "Europe/Tiraspol", "Europe/Kyiv",
       "Europe/Simferopol", "Asia/Tel_Aviv", "Europe/Zaporozhye",
       "Asia/Jerusalem", "Asia/Beirut", "Europe/Moscow", "Asia/Tbilisi",
       "Indian/Maldives", "Asia/Colombo", "Asia/Yangon", "Asia/Bangkok",
       "Asia/Irkutsk", "Pacific/Port_Moresby", "Pacific/Bougainville",
       "Europe/Stockholm", "Europe/Helsinki", "Africa/Addis_Ababa",
       "Africa/Asmara", "Pacific/Auckland", "Pacific/Chatham",
       "Asia/Jakarta", "Europe/Rome", "Asia/Kolkata", "Europe/Zurich",
       "Europe/Prague", "Europe/London", "Europe/Amsterdam"}),
   "LST": ({"Europe/Riga"}),
   "MDST": ({"Europe/Moscow"}),
   "MDT": ({"America/Boise", "America/Cambridge_Bay",
       "America/Ciudad_Juarez", "America/Denver", "America/Edmonton",
       "America/Inuvik", "America/Mazatlan", "America/Yellowknife",
       "MST7MDT", "America/Bahia_Banderas", "America/Chihuahua",
       "America/Hermosillo", "America/Mexico_City", "America/Monterrey",
       "America/North_Dakota/Beulah", "America/North_Dakota/Center",
       "America/North_Dakota/New_Salem", "America/Ojinaga",
       "America/Phoenix", "America/Regina", "America/Swift_Current"}),
   "MEST": ({"MET"}),
   "MET": ({"MET"}),
   "MMT": ({"Africa/Monrovia", "Europe/Moscow", "Indian/Maldives",
       "America/Managua", "Asia/Makassar", "Europe/Minsk",
       "America/Montevideo", "Asia/Colombo", "Asia/Kolkata"}),
   "MPT": ({"America/Boise", "America/Cambridge_Bay",
       "America/Ciudad_Juarez", "America/Denver", "America/Edmonton",
       "America/Inuvik", "America/Yellowknife", "MST7MDT",
       "America/North_Dakota/Beulah", "America/North_Dakota/Center",
       "America/North_Dakota/New_Salem", "America/Ojinaga",
       "America/Phoenix", "America/Regina", "America/Swift_Current"}),
   "MSD": ({"Europe/Tiraspol", "Europe/Kirov", "Europe/Moscow",
       "Europe/Volgograd", "Europe/Simferopol", "Europe/Kaliningrad",
       "Europe/Tallinn", "Europe/Vilnius", "Europe/Chisinau", "Europe/Kyiv",
       "Europe/Minsk", "Europe/Riga", "Europe/Uzhgorod", "Europe/Zaporozhye"}),
   "MSK": ({"Europe/Kirov", "Europe/Moscow", "Europe/Simferopol",
       "Europe/Tiraspol", "Europe/Volgograd", "Europe/Minsk",
       "Europe/Uzhgorod", "Europe/Kaliningrad", "Europe/Tallinn",
       "Europe/Vilnius", "Europe/Chisinau", "Europe/Kyiv", "Europe/Riga",
       "Europe/Zaporozhye"}),
   "MST": ({"America/Boise", "America/Cambridge_Bay",
       "America/Ciudad_Juarez", "America/Creston", "America/Dawson",
       "America/Dawson_Creek", "America/Denver", "America/Edmonton",
       "America/Fort_Nelson", "America/Hermosillo", "America/Inuvik",
       "America/Mazatlan", "America/Phoenix", "America/Whitehorse",
       "America/Yellowknife", "MST", "MST7MDT", "America/Bahia_Banderas",
       "America/Chihuahua", "America/Mexico_City", "America/Monterrey",
       "America/North_Dakota/Beulah", "America/North_Dakota/Center",
       "America/North_Dakota/New_Salem", "America/Ojinaga", "America/Regina",
       "America/Swift_Current", "Europe/Moscow", "America/Ensenada",
       "America/Tijuana"}),
   "MWT": ({"America/Boise", "America/Cambridge_Bay",
       "America/Ciudad_Juarez", "America/Denver", "America/Edmonton",
       "America/Inuvik", "America/Mazatlan", "America/Yellowknife",
       "MST7MDT", "America/Bahia_Banderas", "America/Chihuahua",
       "America/Hermosillo", "America/Mexico_City", "America/Monterrey",
       "America/North_Dakota/Beulah", "America/North_Dakota/Center",
       "America/North_Dakota/New_Salem", "America/Ojinaga",
       "America/Phoenix", "America/Regina", "America/Swift_Current"}),
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
   "PDT": ({"America/Ensenada", "America/Los_Angeles", "America/Tijuana",
       "America/Vancouver", "Asia/Manila", "PST8PDT", "America/Boise",
       "America/Dawson", "America/Dawson_Creek", "America/Fort_Nelson",
       "America/Inuvik", "America/Juneau", "America/Metlakatla",
       "America/Sitka", "America/Whitehorse"}),
   "PKST": ({"Asia/Karachi"}),
   "PKT": ({"Asia/Karachi"}),
   "PLMT": ({"Asia/Hanoi", "Asia/Ho_Chi_Minh", "Asia/Phnom_Penh",
       "Asia/Vientiane"}),
   "PMMT": ({"Pacific/Bougainville", "Pacific/Port_Moresby"}),
   "PMT": ({"America/Paramaribo", "Asia/Pontianak", "Asia/Yekaterinburg",
       "Europe/Monaco", "Africa/Algiers", "Africa/Tunis", "Europe/Paris",
       "Europe/Prague"}),
   "PPMT": ({"America/Port-au-Prince"}),
   "PPT": ({"America/Los_Angeles", "America/Tijuana", "America/Vancouver",
       "PST8PDT", "America/Boise", "America/Dawson", "America/Dawson_Creek",
       "America/Fort_Nelson", "America/Inuvik", "America/Juneau",
       "America/Metlakatla", "America/Sitka", "America/Whitehorse"}),
   "PST": ({"America/Ensenada", "America/Los_Angeles", "America/Tijuana",
       "America/Vancouver", "Asia/Manila", "PST8PDT", "America/Metlakatla",
       "America/Dawson", "America/Whitehorse", "America/Boise",
       "America/Dawson_Creek", "America/Fort_Nelson", "America/Inuvik",
       "America/Juneau", "America/Sitka", "America/Creston"}),
   "PWT": ({"America/Ensenada", "America/Los_Angeles", "America/Tijuana",
       "America/Vancouver", "PST8PDT", "America/Boise", "America/Dawson",
       "America/Dawson_Creek", "America/Fort_Nelson", "America/Inuvik",
       "America/Juneau", "America/Metlakatla", "America/Sitka",
       "America/Whitehorse"}),
   "QMT": ({"America/Guayaquil"}),
   "RMT": ({"Europe/Riga", "Asia/Yangon", "Europe/Rome"}),
   "SAST": ({"Africa/Johannesburg", "Africa/Maseru", "Africa/Mbabane",
       "Africa/Windhoek", "Africa/Gaborone"}),
   "SDMT": ({"America/Santo_Domingo"}),
   "SET": ({"Europe/Stockholm"}),
   "SJMT": ({"America/Costa_Rica"}),
   "SMT": ({"America/Coyhaique", "America/Punta_Arenas", "America/Santiago",
       "Europe/Simferopol", "Atlantic/Stanley", "Asia/Kuala_Lumpur",
       "Asia/Singapore"}),
   "SST": ({"Pacific/Midway", "Pacific/Pago_Pago"}),
   "TBMT": ({"Asia/Tbilisi"}),
   "TMT": ({"Asia/Tehran", "Europe/Tallinn"}),
   "UTC": ({"Etc/UTC"}),
   "WAST": ({"Africa/Ndjamena"}),
   "WAT": ({"Africa/Bangui", "Africa/Brazzaville", "Africa/Douala",
       "Africa/Kinshasa", "Africa/Lagos", "Africa/Libreville",
       "Africa/Luanda", "Africa/Malabo", "Africa/Ndjamena", "Africa/Niamey",
       "Africa/Porto-Novo", "Africa/Windhoek", "Africa/Sao_Tome",
       "Africa/Lubumbashi"}),
   "WEMT": ({"Africa/Ceuta", "Atlantic/Madeira", "Europe/Lisbon",
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
   "ZMT": ({"Africa/Blantyre"}),
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
               -43200:"Pacific/Tarawa",
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
      ([ "test":709984800, // 1992-07-01 10:00:00
         -43200:
            ([ "test":857214000, // 1997-03-01 11:00:00
               -39600:
                  ([ "test":1414245600, // 2014-10-25 14:00:00
                     -39600:"Asia/Srednekolymsk",
                     -36000:"Asia/Magadan",
                  ]),
               -36000:"Asia/Sakhalin",
            ]),
         -39600:
            ([ "test":-1829387596, // 1912-01-12 12:46:44
               -39948:"Pacific/Noumea",
               -39600:"Pacific/Efate",
               -38388:"Pacific/Guadalcanal",
            ]),
         -36000:
            ([ "test":31572000, // 1971-01-01 10:00:00
               -39600:
                  ([ "test":-1583848800, // 1919-10-24 10:00:00
                     -36000:"Australia/Hobart",
                     0:"Antarctica/Macquarie",
                  ]),
               -36000:
                  ([ "test":-2366791928, // 1894-12-31 13:47:52
                     -36292:"Australia/Sydney",
                     -36000:"Australia/Brisbane",
                     -35756:"Australia/Lindeman",
                     -34792:"Australia/Melbourne",
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
                  ([ "test":-885549600, // 1941-12-09 14:00:00
                     -36000:"Pacific/Port_Moresby",
                     -32400:"Pacific/Guam",
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
                  ([ "test":-498063600, // 1954-03-21 09:00:00
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
            ([ "test":-891579600, // 1941-09-30 19:00:00
               -32400:
                  ([ "test":-649954800, // 1949-05-28 09:00:00
                     -32400:"Asia/Taipei",
                     -28800:"Asia/Shanghai",
                  ]),
               -30600:"Asia/Hong_Kong",
               -30000:"Asia/Kuching",
               -28800:
                  ([ "test":-836409600, // 1943-07-01 08:00:00
                     -36000:"Asia/Macau",
                     -32400:
                        ([ "test":-766054800, // 1945-09-22 15:00:00
                           -32400:"Asia/Manila",
                           -28800:"Asia/Makassar",
                        ]),
                     -28800:"Australia/Perth",
                  ]),
            ]),
         -25200:
            ([ "test":171820800, // 1975-06-12 16:00:00
               -28800:"Asia/Pontianak",
               -25200:"Asia/Ho_Chi_Minh",
            ]),
      ]),
   -27000:"Asia/Singapore",
   -25200:
      ([ "test":760035600, // 1994-01-31 17:00:00
         -28800:"Asia/Ulaanbaatar",
         -25200:
            ([ "test":1269716400, // 2010-03-27 19:00:00
               -28800:"Asia/Krasnoyarsk",
               -25200:
                  ([ "test":1255806000, // 2009-10-17 19:00:00
                     -25200:
                        ([ "test":-620812800, // 1950-04-30 16:00:00
                           -28800:"Asia/Hanoi",
                           -27000:"Asia/Jakarta",
                           -25200:"Asia/Bangkok",
                        ]),
                     -18000:"Antarctica/Davis",
                  ]),
               -21600:
                  ([ "test":801648000, // 1995-05-28 08:00:00
                     -28800:
                        ([ "test":1020250800, // 2002-05-01 11:00:00
                           -28800:"Asia/Novokuznetsk",
                           -25200:"Asia/Tomsk",
                        ]),
                     -25200:"Asia/Barnaul",
                  ]),
            ]),
         -21600:"Asia/Novosibirsk",
         0:"Antarctica/Vostok",
      ]),
   -23400:"Asia/Yangon",
   -21600:
      ([ "test":670363200, // 1991-03-30 20:00:00
         -28800:"Asia/Hovd",
         -25200:"Asia/Tashkent",
         -21600:
            ([ "test":1255809600, // 2009-10-17 20:00:00
               -25200:"Asia/Dhaka",
               -21600:"Asia/Urumqi",
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
                  ([ "test":-576135000, // 1951-09-29 18:30:00
                     -18000:"Asia/Karachi",
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
      ([ "test":686102400, // 1991-09-29 00:00:00
         -14400:
            ([ "test":575416800, // 1988-03-26 22:00:00
               -18000:
                  ([ "test":1459033200, // 2016-03-26 23:00:00
                     -14400:"Europe/Astrakhan",
                     -10800:"Europe/Kirov",
                  ]),
               -14400:
                  ([ "test":-1988164200, // 1906-12-31 20:10:00
                     -14400:"Indian/Mauritius",
                     -13272:"Asia/Dubai",
                  ]),
               -10800:
                  ([ "test":1480806000, // 2016-12-03 23:00:00
                     -14400:"Europe/Saratov",
                     -10800:"Europe/Volgograd",
                  ]),
            ]),
         -10800:
            ([ "test":778392000, // 1994-09-01 04:00:00
               -18000:
                  ([ "test":606866400, // 1989-03-25 22:00:00
                     -18000:"Asia/Tbilisi",
                     -10800:"Europe/Samara",
                  ]),
               -14400:
                  ([ "test":-1441163964, // 1924-05-01 20:40:36
                     -10800:"Asia/Baku",
                     -10680:"Asia/Yerevan",
                  ]),
               -10800:"Asia/Qatar",
            ]),
         -7200:"Europe/Ulyanovsk",
      ]),
   -12600:"Asia/Tehran",
   -10800:
      ([ "test":606870000, // 1989-03-25 23:00:00
         -14400:
            ([ "test":646786800, // 1990-06-30 23:00:00
               -14400:
                  ([ "test":1301180400, // 2011-03-26 23:00:00
                     -14400:"Europe/Moscow",
                     -10800:"Asia/Baghdad",
                     -7200:"Europe/Kyiv",
                  ]),
               -10800:
                  ([ "test":-804636000, // 1944-07-03 02:00:00
                     -10800:"Europe/Minsk",
                     -7200:"Europe/Chisinau",
                  ]),
               -7200:"Europe/Simferopol",
            ]),
         -10800:
            ([ "test":622598400, // 1989-09-24 00:00:00
               -10800:
                  ([ "test":-865305900, // 1942-07-31 21:15:00
                     -11212:"Asia/Riyadh",
                     -10800:"Africa/Nairobi",
                  ]),
               -7200:
                  ([ "test":-797637600, // 1944-09-22 02:00:00
                     -10800:"Europe/Tallinn",
                     -7200:"Europe/Riga",
                  ]),
            ]),
         -7200:
            ([ "test":891133200, // 1998-03-29 01:00:00
               -10800:"Europe/Kaliningrad",
               -7200:"Europe/Vilnius",
            ]),
      ]),
   -7200:
      ([ "test":844034400, // 1996-09-29 22:00:00
         -10800:
            ([ "test":267933600, // 1978-06-29 02:00:00
               -10800:
                  ([ "test":347162400, // 1981-01-01 02:00:00
                     -10800:"Europe/Istanbul",
                     -7200:
                        ([ "test":-1686101632, // 1916-07-27 22:26:08
                           -8712:"Asia/Damascus",
                           -7200:"Europe/Athens",
                        ]),
                  ]),
               -7200:
                  ([ "test":291762000, // 1979-03-31 21:00:00
                     -10800:"Europe/Sofia",
                     -7200:
                        ([ "test":-1535938789, // 1921-04-30 22:20:11
                           -7200:"Europe/Helsinki",
                           -6264:"Europe/Bucharest",
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
                        ([ "test":-2109288600, // 1903-02-28 22:30:00
                           -7818:"Africa/Maputo",
                           -7200:
                              ([ "test":-2185409109, // 1900-09-30 21:54:51
                                 -7200:"Africa/Cairo",
                                 -5400:"Africa/Johannesburg",
                              ]),
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
                        ([ "test":-766623600, // 1945-09-16 01:00:00
                           -10800:"Europe/Berlin",
                           -7200:
                              ([ "test":-931726800, // 1940-06-23 03:00:00
                                 -7200:
                                    ([ "test":-728510400, // 1946-12-01 04:00:00
                                       -7200:"Europe/Prague",
                                       -3600:"Europe/Warsaw",
                                    ]),
                                 -3600:
                                    ([ "test":-906768000, // 1941-04-08 00:00:00
                                       -7200:"Europe/Budapest",
                                       -3600:"Europe/Madrid",
                                    ]),
                              ]),
                           -3600:
                              ([ "test":-1693702800, // 1916-04-30 23:00:00
                                 -7200:
                                    ([ "test":-780188400, // 1945-04-12 01:00:00
                                       -7200:"Europe/Brussels",
                                       -3600:"Europe/Vienna",
                                    ]),
                                 -3600:
                                    ([ "test":-777942000, // 1945-05-08 01:00:00
                                       -7200:
                                          ([ "test":-2713915320, // 1883-12-31 22:38:00
                                             -3600:"Europe/Belgrade",
                                             -2996:"Europe/Rome",
                                          ]),
                                       -3600:"Europe/Zurich",
                                    ]),
                                 0:"Europe/Gibraltar",
                              ]),
                           0:
                              ([ "test":-766609200, // 1945-09-16 05:00:00
                                 -3600:"Europe/Paris",
                                 0:"Europe/Andorra",
                              ]),
                        ]),
                  ]),
               -3600:
                  ([ "test":308703600, // 1979-10-13 23:00:00
                     -7200:"Africa/Ndjamena",
                     -3600:
                        ([ "test":-1767226415, // 1913-12-31 23:46:25
                           -3600:"Africa/Tunis",
                           -1800:"Africa/Lagos",
                        ]),
                  ]),
            ]),
         0:"Europe/Lisbon",
      ]),
   0:
      ([ "test":523155600, // 1986-07-31 01:00:00
         -7200:"Africa/Ceuta",
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
                     -3600:"Atlantic/Canary",
                     0:
                        ([ "test":-1830380400, // 1912-01-01 01:00:00
                           0:"Atlantic/Faroe",
                           3600:"Atlantic/Madeira",
                        ]),
                  ]),
            ]),
         0:
            ([ "test":1540699200, // 2018-10-28 04:00:00
               -3600:
                  ([ "test":448243200, // 1984-03-16 00:00:00
                     -3600:"Africa/Casablanca",
                     0:"Africa/Sao_Tome",
                  ]),
               0:
                  ([ "test":-1830383033, // 1912-01-01 00:16:07
                     0:"Antarctica/Troll",
                     968:"Africa/Abidjan",
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
         7200:"America/Nuuk",
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
                  ([ "test":-7617600, // 1969-10-04 20:00:00
                     10800:"Antarctica/Palmer",
                     14400:
                        ([ "test":-740520000, // 1946-07-15 04:00:00
                           10800:"America/Santiago",
                           14400:
                              ([ "test":-2524504580, // 1890-01-01 04:43:40
                                 16965:"America/Punta_Arenas",
                                 17296:"America/Coyhaique",
                              ]),
                        ]),
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
      ([ "test":1283637600, // 2010-09-04 22:00:00
         7200:"America/Miquelon",
         10800:
            ([ "test":590011200, // 1988-09-11 20:00:00
               7200:"America/Goose_Bay",
               10800:
                  ([ "test":495579600, // 1985-09-14 21:00:00
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
                  ([ "test":-1767212472, // 1914-01-01 03:38:48
                     14400:"America/Santarem",
                     16508:"America/Thule",
                  ]),
            ]),
         14400:
            ([ "test":971557200, // 2000-10-14 21:00:00
               10800:
                  ([ "test":86760000, // 1972-10-01 04:00:00
                     10800:"America/Asuncion",
                     14400:
                        ([ "test":-1767212492, // 1914-01-01 03:38:28
                           13460:"America/Cuiaba",
                           14400:"America/Campo_Grande",
                        ]),
                  ]),
               14400:
                  ([ "test":323841600, // 1980-04-06 04:00:00
                     10800:"America/Martinique",
                     14400:
                        ([ "test":-1841256091, // 1911-08-28 03:58:29
                           14400:
                              ([ "test":-788932800, // 1944-12-31 20:00:00
                                 10800:"America/Puerto_Rico",
                                 14400:"America/Barbados",
                              ]),
                           14404:"America/Manaus",
                           14560:"America/Boa_Vista",
                           15336:"America/Porto_Velho",
                           16356:"America/La_Paz",
                        ]),
                  ]),
            ]),
         16200:"America/Caracas",
      ]),
   16200:"America/Santo_Domingo",
   18000:
      ([ "test":1143961200, // 2006-04-02 07:00:00
         14400:
            ([ "test":941320800, // 1999-10-30 22:00:00
               14400:
                  ([ "test":167814000, // 1975-04-27 07:00:00
                     14400:
                        ([ "test":-1402813824, // 1925-07-19 17:29:36
                           14400:
                              ([ "test":-883630800, // 1941-12-31 19:00:00
                                 14400:"America/Toronto",
                                 18000:"America/New_York",
                              ]),
                           18000:
                              ([ "test":-1724083200, // 1915-05-15 08:00:00
                                 18000:"America/Detroit",
                                 19776:"America/Havana",
                              ]),
                           21600:"America/Kentucky/Louisville",
                        ]),
                     18000:"America/Grand_Turk",
                  ]),
               18000:
                  ([ "test":972766800, // 2000-10-28 21:00:00
                     14400:"America/Iqaluit",
                     18000:
                        ([ "test":-386787600, // 1957-09-29 07:00:00
                           18000:
                              ([ "test":-1670483460, // 1917-01-24 16:49:00
                                 18000:"America/Port-au-Prince",
                                 21600:"America/Indiana/Vevay",
                              ]),
                           21600:
                              ([ "test":-463636800, // 1955-04-23 20:00:00
                                 18000:"America/Indiana/Indianapolis",
                                 21600:"America/Indiana/Marengo",
                              ]),
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
                     21600:"America/Resolute",
                  ]),
            ]),
         21600:
            ([ "test":877813200, // 1997-10-25 21:00:00
               14400:"America/Cancun",
               18000:
                  ([ "test":378201600, // 1981-12-26 08:00:00
                     18000:"America/Merida",
                     21600:
                        ([ "test":1001797200, // 2001-09-29 21:00:00
                           18000:
                              ([ "test":-1234807200, // 1930-11-15 06:00:00
                                 21600:"America/Matamoros",
                                 25200:"America/Monterrey",
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
            ([ "test":1669788000, // 2022-11-30 06:00:00
               21600:
                  ([ "test":-1514739601, // 1922-01-01 06:59:59
                     25060:"America/Ojinaga",
                     25460:"America/Chihuahua",
                  ]),
               25200:"America/Ciudad_Juarez",
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
                  ([ "test":941313600, // 1999-10-30 20:00:00
                     18000:"America/Cambridge_Bay",
                     21600:
                        ([ "test":-873828000, // 1942-04-24 06:00:00
                           21600:"America/Edmonton",
                           25200:"America/Bahia_Banderas",
                        ]),
                  ]),
            ]),
         25200:
            ([ "test":-812653140, // 1944-04-01 07:01:00
               21600:"America/Phoenix",
               25200:
                  ([ "test":-1514739601, // 1922-01-01 06:59:59
                     25540:"America/Mazatlan",
                     26632:"America/Hermosillo",
                  ]),
            ]),
      ]),
   28800:
      ([ "test":1446372000, // 2015-11-01 10:00:00
         25200:
            ([ "test":291744000, // 1979-03-31 16:00:00
               25200:
                  ([ "test":-725875200, // 1946-12-31 16:00:00
                     0:"America/Inuvik",
                     28800:"America/Dawson_Creek",
                  ]),
               28800:"America/Fort_Nelson",
            ]),
         28800:
            ([ "test":-608144400, // 1950-09-24 07:00:00
               25200:
                  ([ "test":-2717640000, // 1883-11-18 20:00:00
                     28800:"America/Los_Angeles",
                     29548:"America/Vancouver",
                  ]),
               28800:"America/Tijuana",
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
      ([ "test":1604163600, // 2020-10-31 17:00:00
         25200:"America/Dawson",
         28800:"America/Yakutat",
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
         39600:
            ([ "test":-543069620, // 1952-10-16 11:19:40
               39600:"Pacific/Pago_Pago",
               40800:"Pacific/Niue",
            ]),
      ]),
   43200:
      ([ "test":307627200, // 1979-10-01 12:00:00
         39600:"Pacific/Kanton",
         43200:"Pacific/Kwajalein",
      ]),
]);
