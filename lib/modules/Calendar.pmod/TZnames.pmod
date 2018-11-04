#charset utf-8
// ----------------------------------------------------------------
// Timezone names
//
// This file is created half-manually. It is read and updated
// by mkrules.pike.
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
         combine_path(__DIR__,"tzdata/zone.tab")) - "\r"));
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
   "America":   ({"Adak", "Anchorage", "Anguilla", "Antigua", "Araguaina",
                  "Argentina/Buenos_Aires", "Argentina/Catamarca",
                  "Argentina/ComodRivadavia", "Argentina/Cordoba",
                  "Argentina/Jujuy", "Argentina/La_Rioja",
                  "Argentina/Mendoza", "Argentina/Rio_Gallegos",
                  "Argentina/Salta", "Argentina/San_Juan",
                  "Argentina/San_Luis", "Argentina/Tucuman",
                  "Argentina/Ushuaia", "Aruba", "Asuncion", "Atikokan",
                  "Bahia", "Bahia_Banderas", "Barbados", "Belem", "Belize",
                  "Blanc-Sablon", "Boa_Vista", "Bogota", "Boise",
                  "Cambridge_Bay", "Campo_Grande", "Cancun", "Caracas",
                  "Cayenne", "Cayman", "Chicago", "Chihuahua",
                  "Coral_Harbour", "Costa_Rica", "Creston", "Cuiaba",
                  "Curacao", "Danmarkshavn", "Dawson", "Dawson_Creek",
                  "Denver", "Detroit", "Dominica", "Edmonton", "Eirunepe",
                  "El_Salvador", "Ensenada", "Fort_Nelson", "Fortaleza",
                  "Glace_Bay", "Godthab", "Goose_Bay", "Grand_Turk",
                  "Grenada", "Guadeloupe", "Guatemala", "Guayaquil", "Guyana",
                  "Halifax", "Havana", "Hermosillo", "Indiana/Indianapolis",
                  "Indiana/Knox", "Indiana/Marengo", "Indiana/Petersburg",
                  "Indiana/Tell_City", "Indiana/Vevay", "Indiana/Vincennes",
                  "Indiana/Winamac", "Inuvik", "Iqaluit", "Jamaica", "Juneau",
                  "Kentucky/Louisville", "Kentucky/Monticello", "La_Paz",
                  "Lima", "Los_Angeles", "Maceio", "Managua", "Manaus",
                  "Martinique", "Matamoros", "Mazatlan", "Menominee",
                  "Merida", "Metlakatla", "Mexico_City", "Miquelon",
                  "Moncton", "Monterrey", "Montevideo", "Montreal",
                  "Montserrat", "Nassau", "New_York", "Nipigon", "Nome",
                  "Noronha", "North_Dakota/Beulah", "North_Dakota/Center",
                  "North_Dakota/New_Salem", "Ojinaga", "Panama",
                  "Pangnirtung", "Paramaribo", "Phoenix", "Port-au-Prince",
                  "Port_of_Spain", "Porto_Velho", "Puerto_Rico",
                  "Punta_Arenas", "Rainy_River", "Rankin_Inlet", "Recife",
                  "Regina", "Resolute", "Rio_Branco", "Rosario", "Santarem",
                  "Santiago", "Santo_Domingo", "Sao_Paulo", "Scoresbysund",
                  "Sitka", "St_Johns", "St_Kitts", "St_Lucia", "St_Thomas",
                  "St_Vincent", "Swift_Current", "Tegucigalpa", "Thule",
                  "Thunder_Bay", "Tijuana", "Toronto", "Tortola", "Vancouver",
                  "Whitehorse", "Winnipeg", "Yakutat", "Yellowknife"}),
   "Pacific":   ({"Apia", "Auckland", "Bougainville", "Chatham", "Chuuk",
                  "Easter", "Efate", "Enderbury", "Fakaofo", "Fiji",
                  "Funafuti", "Galapagos", "Gambier", "Guadalcanal", "Guam",
                  "Honolulu", "Johnston", "Kiritimati", "Kosrae", "Kwajalein",
                  "Majuro", "Marquesas", "Midway", "Nauru", "Niue", "Norfolk",
                  "Noumea", "Pago_Pago", "Palau", "Pitcairn", "Pohnpei",
                  "Port_Moresby", "Rarotonga", "Saipan", "Tahiti", "Tarawa",
                  "Tongatapu", "Wake", "Wallis"}),
   "Antarctica":({"Casey", "Davis", "DumontDUrville", "Macquarie", "Mawson",
                  "McMurdo", "Palmer", "Rothera", "Syowa", "Troll",
                  "Vostok"}),
   "Atlantic":  ({"Azores", "Bermuda", "Canary", "Cape_Verde", "Faroe",
                  "Jan_Mayen", "Madeira", "Reykjavik", "South_Georgia",
                  "St_Helena", "Stanley"}),
   "Indian":    ({"Antananarivo", "Chagos", "Christmas", "Cocos", "Comoro",
                  "Kerguelen", "Mahe", "Maldives", "Mauritius", "Mayotte",
                  "Reunion"}),
   "Europe":    ({"Amsterdam", "Andorra", "Astrakhan", "Athens", "Belfast",
                  "Belgrade", "Berlin", "Brussels", "Bucharest", "Budapest",
                  "Chisinau", "Copenhagen", "Dublin", "Gibraltar", "Guernsey",
                  "Helsinki", "Isle_of_Man", "Istanbul", "Jersey",
                  "Kaliningrad", "Kiev", "Kirov", "Lisbon", "Ljubljana",
                  "London", "Luxembourg", "Madrid", "Malta", "Minsk",
                  "Monaco", "Moscow", "Oslo", "Paris", "Prague", "Riga",
                  "Rome", "Samara", "Sarajevo", "Saratov", "Simferopol",
                  "Skopje", "Sofia", "Stockholm", "Tallinn", "Tirane",
                  "Tiraspol", "Ulyanovsk", "Uzhgorod", "Vaduz", "Vienna",
                  "Vilnius", "Volgograd", "Warsaw", "Zagreb", "Zaporozhye",
                  "Zurich"}),
   "Africa":    ({"Abidjan", "Accra", "Addis_Ababa", "Algiers", "Asmara",
                  "Bamako", "Bangui", "Banjul", "Bissau", "Blantyre",
                  "Brazzaville", "Bujumbura", "Cairo", "Casablanca", "Ceuta",
                  "Conakry", "Dakar", "Dar_es_Salaam", "Djibouti", "Douala",
                  "El_Aaiun", "Freetown", "Gaborone", "Harare",
                  "Johannesburg", "Juba", "Kampala", "Khartoum", "Kigali",
                  "Kinshasa", "Lagos", "Libreville", "Lome", "Luanda",
                  "Lubumbashi", "Lusaka", "Malabo", "Maputo", "Maseru",
                  "Mbabane", "Mogadishu", "Monrovia", "Nairobi", "Ndjamena",
                  "Niamey", "Nouakchott", "Ouagadougou", "Porto-Novo",
                  "Sao_Tome", "Timbuktu", "Tripoli", "Tunis", "Windhoek"}),
   "Asia":      ({"Aden", "Almaty", "Amman", "Anadyr", "Aqtau", "Aqtobe",
                  "Ashgabat", "Atyrau", "Baghdad", "Bahrain", "Baku",
                  "Bangkok", "Barnaul", "Beirut", "Bishkek", "Brunei",
                  "Chita", "Choibalsan", "Chongqing", "Colombo", "Damascus",
                  "Dhaka", "Dili", "Dubai", "Dushanbe", "Famagusta", "Gaza",
                  "Hanoi", "Harbin", "Hebron", "Ho_Chi_Minh", "Hong_Kong",
                  "Hovd", "Irkutsk", "Jakarta", "Jayapura", "Jerusalem",
                  "Kabul", "Kamchatka", "Karachi", "Kashgar", "Kathmandu",
                  "Khandyga", "Kolkata", "Krasnoyarsk", "Kuala_Lumpur",
                  "Kuching", "Kuwait", "Macau", "Magadan", "Makassar",
                  "Manila", "Muscat", "Nicosia", "Novokuznetsk",
                  "Novosibirsk", "Omsk", "Oral", "Phnom_Penh", "Pontianak",
                  "Pyongyang", "Qatar", "Qyzylorda", "Riyadh", "Sakhalin",
                  "Samarkand", "Seoul", "Shanghai", "Singapore",
                  "Srednekolymsk", "Taipei", "Tashkent", "Tbilisi", "Tehran",
                  "Tel_Aviv", "Thimphu", "Tokyo", "Tomsk", "Ulaanbaatar",
                  "Urumqi", "Ust-Nera", "Vientiane", "Vladivostok", "Yakutsk",
                  "Yangon", "Yekaterinburg", "Yerevan"}),
   "Australia": ({"Adelaide", "Brisbane", "Broken_Hill", "Currie", "Darwin",
                  "Eucla", "Hobart", "Lindeman", "Lord_Howe", "Melbourne",
                  "Perth", "Sydney"}),
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
   "": ({
     "Europe/Amsterdam",
     "Europe/Moscow",
   }),
   "%s": ({
     "Europe/Belfast",
     "Europe/Guernsey",
     "Europe/Isle_of_Man",
     "Europe/Jersey",
   }),
   "+00": ({
     "America/Scoresbysund",
     "Antarctica/Troll",
     "Atlantic/Azores",
     "Atlantic/Madeira",
     "Atlantic/Reykjavik",
   }),
   "+0020": ({
     "Africa/Accra",
     "Europe/Amsterdam",
   }),
   "+01": ({
     "Africa/Freetown",
     "Atlantic/Madeira",
     "Etc/GMT-1",
   }),
   "+0120": ({
     "Europe/Amsterdam",
   }),
   "+0130": ({
     "Africa/Windhoek",
   }),
   "+02": ({
     "Antarctica/Troll",
     "Etc/GMT-2",
     "Europe/Samara",
     "Europe/Ulyanovsk",
   }),
   "+0220": ({
     "Europe/Zaporozhye",
   }),
   "+0230": ({
     "Africa/Kampala",
     "Africa/Mogadishu",
     "Africa/Nairobi",
   }),
   "+0245": ({
     "Africa/Dar_es_Salaam",
     "Africa/Kampala",
     "Africa/Nairobi",
   }),
   "+03": ({
     "Antarctica/Syowa",
     "Asia/Aden",
     "Asia/Atyrau",
     "Asia/Baghdad",
     "Asia/Bahrain",
     "Asia/Baku",
     "Asia/Famagusta",
     "Asia/Kuwait",
     "Asia/Oral",
     "Asia/Qatar",
     "Asia/Riyadh",
     "Asia/Tbilisi",
     "Asia/Yerevan",
     "Etc/GMT-3",
     "Europe/Astrakhan",
     "Europe/Istanbul",
     "Europe/Kaliningrad",
     "Europe/Kirov",
     "Europe/Minsk",
     "Europe/Samara",
     "Europe/Saratov",
     "Europe/Ulyanovsk",
     "Europe/Volgograd",
   }),
   "+0330": ({
     "Asia/Tehran",
   }),
   "+04": ({
     "Asia/Aqtau",
     "Asia/Aqtobe",
     "Asia/Ashgabat",
     "Asia/Atyrau",
     "Asia/Baghdad",
     "Asia/Bahrain",
     "Asia/Baku",
     "Asia/Dubai",
     "Asia/Kabul",
     "Asia/Muscat",
     "Asia/Oral",
     "Asia/Qatar",
     "Asia/Qyzylorda",
     "Asia/Samarkand",
     "Asia/Tbilisi",
     "Asia/Tehran",
     "Asia/Yekaterinburg",
     "Asia/Yerevan",
     "Etc/GMT-4",
     "Europe/Astrakhan",
     "Europe/Istanbul",
     "Europe/Kirov",
     "Europe/Samara",
     "Europe/Saratov",
     "Europe/Ulyanovsk",
     "Europe/Volgograd",
     "Indian/Mahe",
     "Indian/Mauritius",
     "Indian/Reunion",
   }),
   "+0430": ({
     "Asia/Kabul",
     "Asia/Tehran",
   }),
   "+05": ({
     "Antarctica/Davis",
     "Antarctica/Mawson",
     "Asia/Almaty",
     "Asia/Aqtau",
     "Asia/Aqtobe",
     "Asia/Ashgabat",
     "Asia/Atyrau",
     "Asia/Baku",
     "Asia/Bishkek",
     "Asia/Dushanbe",
     "Asia/Karachi",
     "Asia/Kashgar",
     "Asia/Omsk",
     "Asia/Oral",
     "Asia/Qyzylorda",
     "Asia/Samarkand",
     "Asia/Tashkent",
     "Asia/Tbilisi",
     "Asia/Tehran",
     "Asia/Yekaterinburg",
     "Asia/Yerevan",
     "Etc/GMT-5",
     "Europe/Astrakhan",
     "Europe/Kirov",
     "Europe/Moscow",
     "Europe/Samara",
     "Europe/Saratov",
     "Europe/Ulyanovsk",
     "Europe/Volgograd",
     "Indian/Chagos",
     "Indian/Kerguelen",
     "Indian/Maldives",
     "Indian/Mauritius",
   }),
   "+0530": ({
     "Asia/Colombo",
     "Asia/Dhaka",
     "Asia/Karachi",
     "Asia/Kashgar",
     "Asia/Kathmandu",
     "Asia/Thimphu",
   }),
   "+0545": ({
     "Asia/Kathmandu",
   }),
   "+06": ({
     "Antarctica/Mawson",
     "Antarctica/Vostok",
     "Asia/Almaty",
     "Asia/Aqtau",
     "Asia/Aqtobe",
     "Asia/Ashgabat",
     "Asia/Atyrau",
     "Asia/Barnaul",
     "Asia/Bishkek",
     "Asia/Colombo",
     "Asia/Dhaka",
     "Asia/Dushanbe",
     "Asia/Hovd",
     "Asia/Krasnoyarsk",
     "Asia/Novokuznetsk",
     "Asia/Novosibirsk",
     "Asia/Omsk",
     "Asia/Oral",
     "Asia/Qyzylorda",
     "Asia/Samarkand",
     "Asia/Tashkent",
     "Asia/Thimphu",
     "Asia/Tomsk",
     "Asia/Urumqi",
     "Asia/Yekaterinburg",
     "Etc/GMT-6",
     "Indian/Chagos",
   }),
   "+0630": ({
     "Asia/Colombo",
     "Asia/Dhaka",
     "Asia/Karachi",
     "Asia/Kolkata",
     "Asia/Yangon",
     "Indian/Cocos",
   }),
   "+07": ({
     "Antarctica/Davis",
     "Asia/Almaty",
     "Asia/Bangkok",
     "Asia/Barnaul",
     "Asia/Bishkek",
     "Asia/Choibalsan",
     "Asia/Chongqing",
     "Asia/Dhaka",
     "Asia/Dushanbe",
     "Asia/Hanoi",
     "Asia/Ho_Chi_Minh",
     "Asia/Hovd",
     "Asia/Irkutsk",
     "Asia/Krasnoyarsk",
     "Asia/Kuala_Lumpur",
     "Asia/Novokuznetsk",
     "Asia/Novosibirsk",
     "Asia/Omsk",
     "Asia/Phnom_Penh",
     "Asia/Qyzylorda",
     "Asia/Singapore",
     "Asia/Tashkent",
     "Asia/Tomsk",
     "Asia/Ulaanbaatar",
     "Asia/Vientiane",
     "Etc/GMT-7",
     "Indian/Christmas",
   }),
   "+0720": ({
     "Asia/Jakarta",
     "Asia/Kuala_Lumpur",
     "Asia/Singapore",
   }),
   "+0730": ({
     "Asia/Brunei",
     "Asia/Jakarta",
     "Asia/Kuala_Lumpur",
     "Asia/Kuching",
     "Asia/Pontianak",
     "Asia/Singapore",
   }),
   "+08": ({
     "Antarctica/Casey",
     "Asia/Barnaul",
     "Asia/Brunei",
     "Asia/Chita",
     "Asia/Choibalsan",
     "Asia/Dili",
     "Asia/Hanoi",
     "Asia/Ho_Chi_Minh",
     "Asia/Hovd",
     "Asia/Irkutsk",
     "Asia/Jakarta",
     "Asia/Khandyga",
     "Asia/Krasnoyarsk",
     "Asia/Kuala_Lumpur",
     "Asia/Kuching",
     "Asia/Makassar",
     "Asia/Manila",
     "Asia/Novokuznetsk",
     "Asia/Novosibirsk",
     "Asia/Phnom_Penh",
     "Asia/Pontianak",
     "Asia/Singapore",
     "Asia/Tomsk",
     "Asia/Ulaanbaatar",
     "Asia/Ust-Nera",
     "Asia/Vientiane",
     "Asia/Yakutsk",
     "Etc/GMT-8",
   }),
   "+0820": ({
     "Asia/Kuching",
   }),
   "+0830": ({
     "Asia/Harbin",
   }),
   "+0845": ({
     "Australia/Eucla",
   }),
   "+09": ({
     "Asia/Chita",
     "Asia/Choibalsan",
     "Asia/Dili",
     "Asia/Hanoi",
     "Asia/Harbin",
     "Asia/Ho_Chi_Minh",
     "Asia/Irkutsk",
     "Asia/Jakarta",
     "Asia/Jayapura",
     "Asia/Khandyga",
     "Asia/Kuala_Lumpur",
     "Asia/Kuching",
     "Asia/Makassar",
     "Asia/Manila",
     "Asia/Phnom_Penh",
     "Asia/Pontianak",
     "Asia/Sakhalin",
     "Asia/Singapore",
     "Asia/Ulaanbaatar",
     "Asia/Ust-Nera",
     "Asia/Vientiane",
     "Asia/Vladivostok",
     "Asia/Yakutsk",
     "Asia/Yangon",
     "Etc/GMT-9",
     "Pacific/Bougainville",
     "Pacific/Nauru",
     "Pacific/Palau",
     "Pacific/Saipan",
   }),
   "+0930": ({
     "Asia/Jayapura",
   }),
   "+0945": ({
     "Australia/Eucla",
   }),
   "+10": ({
     "Antarctica/DumontDUrville",
     "Asia/Chita",
     "Asia/Choibalsan",
     "Asia/Khandyga",
     "Asia/Magadan",
     "Asia/Sakhalin",
     "Asia/Srednekolymsk",
     "Asia/Ust-Nera",
     "Asia/Vladivostok",
     "Asia/Yakutsk",
     "Etc/GMT-10",
     "Pacific/Bougainville",
     "Pacific/Chuuk",
     "Pacific/Port_Moresby",
     "Pacific/Saipan",
   }),
   "+1030": ({
     "Australia/Lord_Howe",
   }),
   "+11": ({
     "Antarctica/Casey",
     "Antarctica/Macquarie",
     "Asia/Anadyr",
     "Asia/Kamchatka",
     "Asia/Khandyga",
     "Asia/Magadan",
     "Asia/Sakhalin",
     "Asia/Srednekolymsk",
     "Asia/Ust-Nera",
     "Asia/Vladivostok",
     "Australia/Lord_Howe",
     "Etc/GMT-11",
     "Pacific/Bougainville",
     "Pacific/Efate",
     "Pacific/Guadalcanal",
     "Pacific/Kosrae",
     "Pacific/Kwajalein",
     "Pacific/Majuro",
     "Pacific/Norfolk",
     "Pacific/Noumea",
     "Pacific/Pohnpei",
   }),
   "+1112": ({
     "Pacific/Norfolk",
   }),
   "+1130": ({
     "Australia/Lord_Howe",
     "Pacific/Nauru",
     "Pacific/Norfolk",
   }),
   "+12": ({
     "Asia/Anadyr",
     "Asia/Kamchatka",
     "Asia/Magadan",
     "Asia/Sakhalin",
     "Asia/Srednekolymsk",
     "Asia/Ust-Nera",
     "Etc/GMT-12",
     "Pacific/Efate",
     "Pacific/Fiji",
     "Pacific/Funafuti",
     "Pacific/Kosrae",
     "Pacific/Kwajalein",
     "Pacific/Majuro",
     "Pacific/Nauru",
     "Pacific/Noumea",
     "Pacific/Tarawa",
     "Pacific/Wake",
     "Pacific/Wallis",
   }),
   "+1215": ({
     "Pacific/Chatham",
   }),
   "+1220": ({
     "Pacific/Tongatapu",
   }),
   "+1230": ({
     "Pacific/Norfolk",
   }),
   "+1245": ({
     "Pacific/Chatham",
   }),
   "+13": ({
     "Asia/Anadyr",
     "Asia/Kamchatka",
     "Etc/GMT-13",
     "Pacific/Apia",
     "Pacific/Enderbury",
     "Pacific/Fakaofo",
     "Pacific/Fiji",
     "Pacific/Tongatapu",
   }),
   "+1345": ({
     "Pacific/Chatham",
   }),
   "+14": ({
     "Asia/Anadyr",
     "Etc/GMT-14",
     "Pacific/Apia",
     "Pacific/Kiritimati",
     "Pacific/Tongatapu",
   }),
   "-00": ({
     "America/Cambridge_Bay",
     "America/Inuvik",
     "America/Iqaluit",
     "America/Pangnirtung",
     "America/Rankin_Inlet",
     "America/Resolute",
     "America/Yellowknife",
     "Antarctica/Casey",
     "Antarctica/Davis",
     "Antarctica/DumontDUrville",
     "Antarctica/Macquarie",
     "Antarctica/Mawson",
     "Antarctica/McMurdo",
     "Antarctica/Palmer",
     "Antarctica/Rothera",
     "Antarctica/Syowa",
     "Antarctica/Troll",
     "Antarctica/Vostok",
     "Factory",
     "Indian/Kerguelen",
   }),
   "-0020": ({
     "Africa/Freetown",
   }),
   "-01": ({
     "Africa/Bamako",
     "Africa/Banjul",
     "Africa/Bissau",
     "Africa/Conakry",
     "Africa/Dakar",
     "Africa/El_Aaiun",
     "Africa/Freetown",
     "Africa/Niamey",
     "Africa/Nouakchott",
     "America/Noronha",
     "America/Scoresbysund",
     "Atlantic/Azores",
     "Atlantic/Canary",
     "Atlantic/Cape_Verde",
     "Atlantic/Jan_Mayen",
     "Atlantic/Madeira",
     "Atlantic/Reykjavik",
     "Etc/GMT+1",
   }),
   "-0130": ({
     "America/Montevideo",
   }),
   "-02": ({
     "America/Araguaina",
     "America/Argentina/Buenos_Aires",
     "America/Argentina/Catamarca",
     "America/Argentina/ComodRivadavia",
     "America/Argentina/Cordoba",
     "America/Argentina/Jujuy",
     "America/Argentina/La_Rioja",
     "America/Argentina/Mendoza",
     "America/Argentina/Rio_Gallegos",
     "America/Argentina/Salta",
     "America/Argentina/San_Juan",
     "America/Argentina/San_Luis",
     "America/Argentina/Tucuman",
     "America/Argentina/Ushuaia",
     "America/Bahia",
     "America/Belem",
     "America/Danmarkshavn",
     "America/Fortaleza",
     "America/Godthab",
     "America/Maceio",
     "America/Miquelon",
     "America/Montevideo",
     "America/Noronha",
     "America/Recife",
     "America/Rosario",
     "America/Sao_Paulo",
     "America/Scoresbysund",
     "Antarctica/Palmer",
     "Atlantic/Azores",
     "Atlantic/Cape_Verde",
     "Atlantic/South_Georgia",
     "Atlantic/Stanley",
     "Etc/GMT+2",
   }),
   "-0230": ({
     "America/Montevideo",
   }),
   "-03": ({
     "America/Araguaina",
     "America/Argentina/Buenos_Aires",
     "America/Argentina/Catamarca",
     "America/Argentina/ComodRivadavia",
     "America/Argentina/Cordoba",
     "America/Argentina/Jujuy",
     "America/Argentina/La_Rioja",
     "America/Argentina/Mendoza",
     "America/Argentina/Rio_Gallegos",
     "America/Argentina/Salta",
     "America/Argentina/San_Juan",
     "America/Argentina/San_Luis",
     "America/Argentina/Tucuman",
     "America/Argentina/Ushuaia",
     "America/Asuncion",
     "America/Bahia",
     "America/Belem",
     "America/Boa_Vista",
     "America/Campo_Grande",
     "America/Cayenne",
     "America/Cuiaba",
     "America/Danmarkshavn",
     "America/Fortaleza",
     "America/Godthab",
     "America/Guyana",
     "America/Maceio",
     "America/Manaus",
     "America/Miquelon",
     "America/Montevideo",
     "America/Paramaribo",
     "America/Porto_Velho",
     "America/Punta_Arenas",
     "America/Recife",
     "America/Rosario",
     "America/Santarem",
     "America/Santiago",
     "America/Sao_Paulo",
     "Antarctica/Palmer",
     "Antarctica/Rothera",
     "Atlantic/Stanley",
     "Etc/GMT+3",
   }),
   "-0330": ({
     "America/Montevideo",
     "America/Paramaribo",
   }),
   "-0345": ({
     "America/Guyana",
   }),
   "-04": ({
     "America/Argentina/Buenos_Aires",
     "America/Argentina/Catamarca",
     "America/Argentina/ComodRivadavia",
     "America/Argentina/Cordoba",
     "America/Argentina/Jujuy",
     "America/Argentina/La_Rioja",
     "America/Argentina/Mendoza",
     "America/Argentina/Rio_Gallegos",
     "America/Argentina/Salta",
     "America/Argentina/San_Juan",
     "America/Argentina/San_Luis",
     "America/Argentina/Tucuman",
     "America/Argentina/Ushuaia",
     "America/Asuncion",
     "America/Boa_Vista",
     "America/Bogota",
     "America/Campo_Grande",
     "America/Caracas",
     "America/Cayenne",
     "America/Cuiaba",
     "America/Eirunepe",
     "America/Guayaquil",
     "America/Guyana",
     "America/La_Paz",
     "America/Lima",
     "America/Manaus",
     "America/Montevideo",
     "America/Porto_Velho",
     "America/Punta_Arenas",
     "America/Rio_Branco",
     "America/Rosario",
     "America/Santarem",
     "America/Santiago",
     "Antarctica/Palmer",
     "Atlantic/Stanley",
     "Etc/GMT+4",
   }),
   "-0430": ({
     "America/Aruba",
     "America/Caracas",
     "America/Curacao",
     "America/Santo_Domingo",
   }),
   "-05": ({
     "America/Bogota",
     "America/Eirunepe",
     "America/Guayaquil",
     "America/Lima",
     "America/Punta_Arenas",
     "America/Rio_Branco",
     "America/Santiago",
     "Etc/GMT+5",
     "Pacific/Easter",
     "Pacific/Galapagos",
   }),
   "-0530": ({
     "America/Belize",
   }),
   "-06": ({
     "Etc/GMT+6",
     "Pacific/Easter",
     "Pacific/Galapagos",
   }),
   "-07": ({
     "Etc/GMT+7",
     "Pacific/Easter",
   }),
   "-08": ({
     "Etc/GMT+8",
     "Pacific/Pitcairn",
   }),
   "-0830": ({
     "Pacific/Pitcairn",
   }),
   "-09": ({
     "Etc/GMT+9",
     "Pacific/Gambier",
   }),
   "-0930": ({
     "Pacific/Marquesas",
     "Pacific/Rarotonga",
   }),
   "-10": ({
     "Etc/GMT+10",
     "Pacific/Apia",
     "Pacific/Kiritimati",
     "Pacific/Midway",
     "Pacific/Rarotonga",
     "Pacific/Tahiti",
   }),
   "-1030": ({
     "Pacific/Rarotonga",
   }),
   "-1040": ({
     "Pacific/Kiritimati",
   }),
   "-11": ({
     "Etc/GMT+11",
     "Pacific/Apia",
     "Pacific/Enderbury",
     "Pacific/Fakaofo",
     "Pacific/Midway",
     "Pacific/Niue",
   }),
   "-1120": ({
     "Pacific/Niue",
   }),
   "-1130": ({
     "Pacific/Apia",
     "Pacific/Niue",
   }),
   "-12": ({
     "Etc/GMT+12",
     "Pacific/Enderbury",
     "Pacific/Kwajalein",
   }),
   "ACDT": ({
     "Australia/Adelaide",
     "Australia/Broken_Hill",
     "Australia/Darwin",
   }),
   "ACST": ({
     "Australia/Adelaide",
     "Australia/Broken_Hill",
     "Australia/Darwin",
   }),
   "ADDT": ({
     "America/Goose_Bay",
     "America/Pangnirtung",
   }),
   "ADMT": ({
     "Africa/Addis_Ababa",
     "Africa/Asmara",
   }),
   "ADT": ({
     "America/Anchorage",
     "America/Barbados",
     "America/Blanc-Sablon",
     "America/Glace_Bay",
     "America/Goose_Bay",
     "America/Halifax",
     "America/Martinique",
     "America/Moncton",
     "America/Pangnirtung",
     "America/Puerto_Rico",
     "America/Thule",
     "Atlantic/Bermuda",
   }),
   "AEDT": ({
     "Antarctica/Macquarie",
     "Australia/Brisbane",
     "Australia/Currie",
     "Australia/Hobart",
     "Australia/Lindeman",
     "Australia/Melbourne",
     "Australia/Sydney",
   }),
   "AEST": ({
     "Antarctica/Macquarie",
     "Australia/Brisbane",
     "Australia/Broken_Hill",
     "Australia/Currie",
     "Australia/Hobart",
     "Australia/Lindeman",
     "Australia/Lord_Howe",
     "Australia/Melbourne",
     "Australia/Sydney",
   }),
   "AHDT": ({
     "America/Adak",
     "America/Anchorage",
   }),
   "AHPT": ({
     "America/Adak",
     "America/Anchorage",
   }),
   "AHST": ({
     "America/Adak",
     "America/Anchorage",
   }),
   "AHWT": ({
     "America/Adak",
     "America/Anchorage",
   }),
   "AKDT": ({
     "America/Anchorage",
     "America/Juneau",
     "America/Metlakatla",
     "America/Nome",
     "America/Sitka",
     "America/Yakutat",
   }),
   "AKPT": ({
     "America/Anchorage",
     "America/Juneau",
     "America/Metlakatla",
     "America/Nome",
     "America/Sitka",
     "America/Yakutat",
   }),
   "AKST": ({
     "America/Anchorage",
     "America/Juneau",
     "America/Metlakatla",
     "America/Nome",
     "America/Sitka",
     "America/Yakutat",
   }),
   "AKWT": ({
     "America/Anchorage",
     "America/Juneau",
     "America/Metlakatla",
     "America/Nome",
     "America/Sitka",
     "America/Yakutat",
   }),
   "AMT": ({
     "Africa/Asmara",
     "America/Asuncion",
     "Europe/Amsterdam",
     "Europe/Athens",
   }),
   "APT": ({
     "America/Anchorage",
     "America/Blanc-Sablon",
     "America/Glace_Bay",
     "America/Goose_Bay",
     "America/Halifax",
     "America/Moncton",
     "America/Pangnirtung",
     "America/Puerto_Rico",
     "Atlantic/Bermuda",
   }),
   "AST": ({
     "America/Anchorage",
     "America/Anguilla",
     "America/Antigua",
     "America/Aruba",
     "America/Barbados",
     "America/Blanc-Sablon",
     "America/Curacao",
     "America/Dominica",
     "America/Glace_Bay",
     "America/Goose_Bay",
     "America/Grand_Turk",
     "America/Grenada",
     "America/Guadeloupe",
     "America/Halifax",
     "America/Martinique",
     "America/Miquelon",
     "America/Moncton",
     "America/Montserrat",
     "America/Pangnirtung",
     "America/Port_of_Spain",
     "America/Puerto_Rico",
     "America/Santo_Domingo",
     "America/St_Kitts",
     "America/St_Lucia",
     "America/St_Thomas",
     "America/St_Vincent",
     "America/Thule",
     "America/Tortola",
     "Atlantic/Bermuda",
   }),
   "AWDT": ({
     "Australia/Perth",
   }),
   "AWST": ({
     "Australia/Perth",
   }),
   "AWT": ({
     "America/Anchorage",
     "America/Blanc-Sablon",
     "America/Glace_Bay",
     "America/Goose_Bay",
     "America/Halifax",
     "America/Moncton",
     "America/Pangnirtung",
     "America/Puerto_Rico",
     "Atlantic/Bermuda",
   }),
   "BDST": ({
     "Europe/Dublin",
     "Europe/Gibraltar",
     "Europe/London",
   }),
   "BDT": ({
     "America/Adak",
     "America/Nome",
   }),
   "BMT": ({
     "Africa/Banjul",
     "America/Barbados",
     "America/Bogota",
     "Asia/Baghdad",
     "Asia/Bangkok",
     "Asia/Jakarta",
     "Europe/Brussels",
     "Europe/Bucharest",
     "Europe/Chisinau",
     "Europe/Tiraspol",
     "Europe/Zurich",
   }),
   "BPT": ({
     "America/Adak",
     "America/Nome",
   }),
   "BST": ({
     "America/Adak",
     "America/La_Paz",
     "America/Nome",
     "Europe/Belfast",
     "Europe/Dublin",
     "Europe/Gibraltar",
     "Europe/Guernsey",
     "Europe/Isle_of_Man",
     "Europe/Jersey",
     "Europe/London",
   }),
   "BWT": ({
     "America/Adak",
     "America/Nome",
   }),
   "CAST": ({
     "Africa/Gaborone",
     "Africa/Juba",
     "Africa/Khartoum",
   }),
   "CAT": ({
     "Africa/Blantyre",
     "Africa/Bujumbura",
     "Africa/Gaborone",
     "Africa/Harare",
     "Africa/Juba",
     "Africa/Khartoum",
     "Africa/Kigali",
     "Africa/Lubumbashi",
     "Africa/Lusaka",
     "Africa/Maputo",
     "Africa/Windhoek",
   }),
   "CDDT": ({
     "America/Rankin_Inlet",
     "America/Resolute",
   }),
   "CDT": ({
     "America/Atikokan",
     "America/Bahia_Banderas",
     "America/Belize",
     "America/Cambridge_Bay",
     "America/Cancun",
     "America/Chicago",
     "America/Chihuahua",
     "America/Costa_Rica",
     "America/El_Salvador",
     "America/Guatemala",
     "America/Havana",
     "America/Indiana/Indianapolis",
     "America/Indiana/Knox",
     "America/Indiana/Marengo",
     "America/Indiana/Petersburg",
     "America/Indiana/Tell_City",
     "America/Indiana/Vevay",
     "America/Indiana/Vincennes",
     "America/Indiana/Winamac",
     "America/Iqaluit",
     "America/Kentucky/Louisville",
     "America/Kentucky/Monticello",
     "America/Managua",
     "America/Matamoros",
     "America/Menominee",
     "America/Merida",
     "America/Mexico_City",
     "America/Monterrey",
     "America/North_Dakota/Beulah",
     "America/North_Dakota/Center",
     "America/North_Dakota/New_Salem",
     "America/Ojinaga",
     "America/Pangnirtung",
     "America/Rainy_River",
     "America/Rankin_Inlet",
     "America/Resolute",
     "America/Tegucigalpa",
     "America/Winnipeg",
     "Asia/Chongqing",
     "Asia/Harbin",
     "Asia/Kashgar",
     "Asia/Macau",
     "Asia/Shanghai",
     "Asia/Taipei",
     "CST6CDT",
   }),
   "CE%sT": ({
     "Europe/Guernsey",
     "Europe/Jersey",
     "Europe/Ljubljana",
     "Europe/Sarajevo",
     "Europe/Skopje",
     "Europe/Tiraspol",
     "Europe/Vaduz",
     "Europe/Zagreb",
   }),
   "CEMT": ({
     "Europe/Berlin",
     "Europe/Madrid",
     "Europe/Monaco",
     "Europe/Paris",
   }),
   "CEST": ({
     "Africa/Algiers",
     "Africa/Ceuta",
     "Africa/Tripoli",
     "Africa/Tunis",
     "CET",
     "Europe/Amsterdam",
     "Europe/Andorra",
     "Europe/Athens",
     "Europe/Belgrade",
     "Europe/Berlin",
     "Europe/Brussels",
     "Europe/Budapest",
     "Europe/Chisinau",
     "Europe/Copenhagen",
     "Europe/Gibraltar",
     "Europe/Kaliningrad",
     "Europe/Kiev",
     "Europe/Lisbon",
     "Europe/Ljubljana",
     "Europe/Luxembourg",
     "Europe/Madrid",
     "Europe/Malta",
     "Europe/Minsk",
     "Europe/Monaco",
     "Europe/Oslo",
     "Europe/Paris",
     "Europe/Prague",
     "Europe/Riga",
     "Europe/Rome",
     "Europe/Sarajevo",
     "Europe/Simferopol",
     "Europe/Skopje",
     "Europe/Sofia",
     "Europe/Stockholm",
     "Europe/Tallinn",
     "Europe/Tirane",
     "Europe/Uzhgorod",
     "Europe/Vienna",
     "Europe/Vilnius",
     "Europe/Warsaw",
     "Europe/Zagreb",
     "Europe/Zaporozhye",
     "Europe/Zurich",
   }),
   "CET": ({
     "Africa/Algiers",
     "Africa/Casablanca",
     "Africa/Ceuta",
     "Africa/Tripoli",
     "Africa/Tunis",
     "CET",
     "Europe/Amsterdam",
     "Europe/Andorra",
     "Europe/Athens",
     "Europe/Belgrade",
     "Europe/Berlin",
     "Europe/Brussels",
     "Europe/Budapest",
     "Europe/Chisinau",
     "Europe/Copenhagen",
     "Europe/Gibraltar",
     "Europe/Kaliningrad",
     "Europe/Kiev",
     "Europe/Lisbon",
     "Europe/Ljubljana",
     "Europe/Luxembourg",
     "Europe/Madrid",
     "Europe/Malta",
     "Europe/Minsk",
     "Europe/Monaco",
     "Europe/Oslo",
     "Europe/Paris",
     "Europe/Prague",
     "Europe/Riga",
     "Europe/Rome",
     "Europe/Sarajevo",
     "Europe/Simferopol",
     "Europe/Skopje",
     "Europe/Sofia",
     "Europe/Stockholm",
     "Europe/Tallinn",
     "Europe/Tirane",
     "Europe/Uzhgorod",
     "Europe/Vaduz",
     "Europe/Vienna",
     "Europe/Vilnius",
     "Europe/Warsaw",
     "Europe/Zagreb",
     "Europe/Zaporozhye",
     "Europe/Zurich",
   }),
   "CMT": ({
     "America/Argentina/Buenos_Aires",
     "America/Argentina/Catamarca",
     "America/Argentina/ComodRivadavia",
     "America/Argentina/Cordoba",
     "America/Argentina/Jujuy",
     "America/Argentina/La_Rioja",
     "America/Argentina/Mendoza",
     "America/Argentina/Rio_Gallegos",
     "America/Argentina/Salta",
     "America/Argentina/San_Juan",
     "America/Argentina/San_Luis",
     "America/Argentina/Tucuman",
     "America/Argentina/Ushuaia",
     "America/Caracas",
     "America/La_Paz",
     "America/Panama",
     "America/Rosario",
     "America/St_Lucia",
     "Europe/Chisinau",
     "Europe/Copenhagen",
     "Europe/Tiraspol",
   }),
   "CPT": ({
     "America/Atikokan",
     "America/Cambridge_Bay",
     "America/Chicago",
     "America/Indiana/Indianapolis",
     "America/Indiana/Knox",
     "America/Indiana/Marengo",
     "America/Indiana/Petersburg",
     "America/Indiana/Tell_City",
     "America/Indiana/Vevay",
     "America/Indiana/Vincennes",
     "America/Indiana/Winamac",
     "America/Iqaluit",
     "America/Kentucky/Louisville",
     "America/Kentucky/Monticello",
     "America/Matamoros",
     "America/Menominee",
     "America/Monterrey",
     "America/North_Dakota/Beulah",
     "America/North_Dakota/Center",
     "America/North_Dakota/New_Salem",
     "America/Pangnirtung",
     "America/Rainy_River",
     "America/Rankin_Inlet",
     "America/Resolute",
     "America/Winnipeg",
     "CST6CDT",
   }),
   "CST": ({
     "America/Atikokan",
     "America/Bahia_Banderas",
     "America/Belize",
     "America/Cambridge_Bay",
     "America/Cancun",
     "America/Chicago",
     "America/Chihuahua",
     "America/Costa_Rica",
     "America/Detroit",
     "America/El_Salvador",
     "America/Guatemala",
     "America/Havana",
     "America/Hermosillo",
     "America/Indiana/Indianapolis",
     "America/Indiana/Knox",
     "America/Indiana/Marengo",
     "America/Indiana/Petersburg",
     "America/Indiana/Tell_City",
     "America/Indiana/Vevay",
     "America/Indiana/Vincennes",
     "America/Indiana/Winamac",
     "America/Iqaluit",
     "America/Kentucky/Louisville",
     "America/Kentucky/Monticello",
     "America/Managua",
     "America/Matamoros",
     "America/Mazatlan",
     "America/Menominee",
     "America/Merida",
     "America/Mexico_City",
     "America/Monterrey",
     "America/North_Dakota/Beulah",
     "America/North_Dakota/Center",
     "America/North_Dakota/New_Salem",
     "America/Ojinaga",
     "America/Pangnirtung",
     "America/Rainy_River",
     "America/Rankin_Inlet",
     "America/Regina",
     "America/Resolute",
     "America/Swift_Current",
     "America/Tegucigalpa",
     "America/Thunder_Bay",
     "America/Winnipeg",
     "Asia/Chongqing",
     "Asia/Harbin",
     "Asia/Kashgar",
     "Asia/Macau",
     "Asia/Shanghai",
     "Asia/Taipei",
     "CST6CDT",
   }),
   "CWT": ({
     "America/Atikokan",
     "America/Bahia_Banderas",
     "America/Cambridge_Bay",
     "America/Cancun",
     "America/Chicago",
     "America/Chihuahua",
     "America/Indiana/Indianapolis",
     "America/Indiana/Knox",
     "America/Indiana/Marengo",
     "America/Indiana/Petersburg",
     "America/Indiana/Tell_City",
     "America/Indiana/Vevay",
     "America/Indiana/Vincennes",
     "America/Indiana/Winamac",
     "America/Iqaluit",
     "America/Kentucky/Louisville",
     "America/Kentucky/Monticello",
     "America/Matamoros",
     "America/Menominee",
     "America/Merida",
     "America/Mexico_City",
     "America/Monterrey",
     "America/North_Dakota/Beulah",
     "America/North_Dakota/Center",
     "America/North_Dakota/New_Salem",
     "America/Ojinaga",
     "America/Pangnirtung",
     "America/Rainy_River",
     "America/Rankin_Inlet",
     "America/Resolute",
     "America/Winnipeg",
     "CST6CDT",
   }),
   "ChST": ({
     "Pacific/Guam",
     "Pacific/Saipan",
   }),
   "DFT": ({
     "Europe/Oslo",
     "Europe/Paris",
   }),
   "DMT": ({
     "Europe/Belfast",
     "Europe/Dublin",
   }),
   "E%sT": ({
     "America/Coral_Harbour",
     "America/Montreal",
   }),
   "EAST": ({
     "Indian/Antananarivo",
   }),
   "EAT": ({
     "Africa/Addis_Ababa",
     "Africa/Asmara",
     "Africa/Dar_es_Salaam",
     "Africa/Djibouti",
     "Africa/Juba",
     "Africa/Kampala",
     "Africa/Khartoum",
     "Africa/Mogadishu",
     "Africa/Nairobi",
     "Indian/Antananarivo",
     "Indian/Comoro",
     "Indian/Mayotte",
   }),
   "EDDT": ({
     "America/Iqaluit",
   }),
   "EDT": ({
     "America/Cancun",
     "America/Detroit",
     "America/Grand_Turk",
     "America/Indiana/Indianapolis",
     "America/Indiana/Marengo",
     "America/Indiana/Petersburg",
     "America/Indiana/Tell_City",
     "America/Indiana/Vevay",
     "America/Indiana/Vincennes",
     "America/Indiana/Winamac",
     "America/Iqaluit",
     "America/Jamaica",
     "America/Kentucky/Louisville",
     "America/Kentucky/Monticello",
     "America/Montreal",
     "America/Nassau",
     "America/New_York",
     "America/Nipigon",
     "America/Pangnirtung",
     "America/Port-au-Prince",
     "America/Santo_Domingo",
     "America/Thunder_Bay",
     "America/Toronto",
     "EST5EDT",
   }),
   "EE%sT": ({
     "Europe/Tiraspol",
   }),
   "EEST": ({
     "Africa/Cairo",
     "Asia/Amman",
     "Asia/Beirut",
     "Asia/Damascus",
     "Asia/Famagusta",
     "Asia/Gaza",
     "Asia/Hebron",
     "Asia/Nicosia",
     "EET",
     "Europe/Athens",
     "Europe/Bucharest",
     "Europe/Chisinau",
     "Europe/Helsinki",
     "Europe/Istanbul",
     "Europe/Kaliningrad",
     "Europe/Kiev",
     "Europe/Minsk",
     "Europe/Moscow",
     "Europe/Riga",
     "Europe/Simferopol",
     "Europe/Sofia",
     "Europe/Tallinn",
     "Europe/Tiraspol",
     "Europe/Uzhgorod",
     "Europe/Vilnius",
     "Europe/Warsaw",
     "Europe/Zaporozhye",
   }),
   "EET": ({
     "Africa/Cairo",
     "Africa/Tripoli",
     "Asia/Amman",
     "Asia/Beirut",
     "Asia/Damascus",
     "Asia/Famagusta",
     "Asia/Gaza",
     "Asia/Hebron",
     "Asia/Nicosia",
     "EET",
     "Europe/Athens",
     "Europe/Bucharest",
     "Europe/Chisinau",
     "Europe/Helsinki",
     "Europe/Istanbul",
     "Europe/Kaliningrad",
     "Europe/Kiev",
     "Europe/Minsk",
     "Europe/Moscow",
     "Europe/Riga",
     "Europe/Simferopol",
     "Europe/Sofia",
     "Europe/Tallinn",
     "Europe/Tiraspol",
     "Europe/Uzhgorod",
     "Europe/Vilnius",
     "Europe/Warsaw",
     "Europe/Zaporozhye",
   }),
   "EMT": ({
     "Pacific/Easter",
   }),
   "EPT": ({
     "America/Detroit",
     "America/Grand_Turk",
     "America/Indiana/Indianapolis",
     "America/Indiana/Marengo",
     "America/Indiana/Petersburg",
     "America/Indiana/Tell_City",
     "America/Indiana/Vevay",
     "America/Indiana/Vincennes",
     "America/Indiana/Winamac",
     "America/Iqaluit",
     "America/Jamaica",
     "America/Kentucky/Louisville",
     "America/Kentucky/Monticello",
     "America/Nassau",
     "America/New_York",
     "America/Nipigon",
     "America/Pangnirtung",
     "America/Santo_Domingo",
     "America/Thunder_Bay",
     "America/Toronto",
     "EST5EDT",
   }),
   "EST": ({
     "America/Antigua",
     "America/Atikokan",
     "America/Cambridge_Bay",
     "America/Cancun",
     "America/Cayman",
     "America/Chicago",
     "America/Coral_Harbour",
     "America/Detroit",
     "America/Grand_Turk",
     "America/Indiana/Indianapolis",
     "America/Indiana/Knox",
     "America/Indiana/Marengo",
     "America/Indiana/Petersburg",
     "America/Indiana/Tell_City",
     "America/Indiana/Vevay",
     "America/Indiana/Vincennes",
     "America/Indiana/Winamac",
     "America/Iqaluit",
     "America/Jamaica",
     "America/Kentucky/Louisville",
     "America/Kentucky/Monticello",
     "America/Managua",
     "America/Menominee",
     "America/Merida",
     "America/Moncton",
     "America/Montreal",
     "America/Nassau",
     "America/New_York",
     "America/Nipigon",
     "America/Panama",
     "America/Pangnirtung",
     "America/Port-au-Prince",
     "America/Rankin_Inlet",
     "America/Resolute",
     "America/Santo_Domingo",
     "America/Thunder_Bay",
     "America/Toronto",
     "EST",
     "EST5EDT",
   }),
   "EWT": ({
     "America/Cancun",
     "America/Detroit",
     "America/Grand_Turk",
     "America/Indiana/Indianapolis",
     "America/Indiana/Marengo",
     "America/Indiana/Petersburg",
     "America/Indiana/Tell_City",
     "America/Indiana/Vevay",
     "America/Indiana/Vincennes",
     "America/Indiana/Winamac",
     "America/Iqaluit",
     "America/Jamaica",
     "America/Kentucky/Louisville",
     "America/Kentucky/Monticello",
     "America/Nassau",
     "America/New_York",
     "America/Nipigon",
     "America/Pangnirtung",
     "America/Santo_Domingo",
     "America/Thunder_Bay",
     "America/Toronto",
     "EST5EDT",
   }),
   "FFMT": ({
     "America/Martinique",
   }),
   "FMT": ({
     "Africa/Freetown",
     "Atlantic/Madeira",
   }),
   "GMT": ({
     "Africa/Abidjan",
     "Africa/Accra",
     "Africa/Bamako",
     "Africa/Banjul",
     "Africa/Bissau",
     "Africa/Conakry",
     "Africa/Dakar",
     "Africa/Freetown",
     "Africa/Lome",
     "Africa/Malabo",
     "Africa/Monrovia",
     "Africa/Niamey",
     "Africa/Nouakchott",
     "Africa/Ouagadougou",
     "Africa/Porto-Novo",
     "Africa/Sao_Tome",
     "Africa/Timbuktu",
     "America/Danmarkshavn",
     "Atlantic/Reykjavik",
     "Atlantic/St_Helena",
     "Etc/GMT",
     "Europe/Belfast",
     "Europe/Dublin",
     "Europe/Gibraltar",
     "Europe/Guernsey",
     "Europe/Isle_of_Man",
     "Europe/Jersey",
     "Europe/London",
     "Europe/Prague",
   }),
   "GST": ({
     "Pacific/Guam",
   }),
   "HDT": ({
     "America/Adak",
     "Pacific/Honolulu",
   }),
   "HKST": ({
     "Asia/Hong_Kong",
   }),
   "HKT": ({
     "Asia/Hong_Kong",
   }),
   "HMT": ({
     "America/Havana",
     "Asia/Dhaka",
     "Asia/Kolkata",
     "Atlantic/Azores",
     "Europe/Helsinki",
   }),
   "HPT": ({
     "America/Adak",
   }),
   "HST": ({
     "America/Adak",
     "HST",
     "Pacific/Honolulu",
     "Pacific/Johnston",
   }),
   "HWT": ({
     "America/Adak",
   }),
   "IDDT": ({
     "Asia/Gaza",
     "Asia/Hebron",
     "Asia/Jerusalem",
     "Asia/Tel_Aviv",
   }),
   "IDT": ({
     "Asia/Gaza",
     "Asia/Hebron",
     "Asia/Jerusalem",
     "Asia/Tel_Aviv",
   }),
   "IMT": ({
     "Asia/Irkutsk",
     "Europe/Istanbul",
     "Europe/Sofia",
   }),
   "IST": ({
     "Asia/Gaza",
     "Asia/Hebron",
     "Asia/Jerusalem",
     "Asia/Kolkata",
     "Asia/Tel_Aviv",
     "Europe/Belfast",
     "Europe/Dublin",
   }),
   "JDT": ({
     "Asia/Tokyo",
   }),
   "JMT": ({
     "Asia/Jerusalem",
     "Asia/Tel_Aviv",
     "Atlantic/St_Helena",
   }),
   "JST": ({
     "Asia/Hong_Kong",
     "Asia/Pyongyang",
     "Asia/Seoul",
     "Asia/Taipei",
     "Asia/Tokyo",
   }),
   "KDT": ({
     "Asia/Seoul",
   }),
   "KMT": ({
     "America/Cayman",
     "America/Grand_Turk",
     "America/Jamaica",
     "America/St_Vincent",
     "Europe/Kiev",
     "Europe/Vilnius",
   }),
   "KST": ({
     "Asia/Pyongyang",
     "Asia/Seoul",
   }),
   "LMT": ({
     "Africa/Abidjan",
     "Africa/Accra",
     "Africa/Addis_Ababa",
     "Africa/Algiers",
     "Africa/Asmara",
     "Africa/Bamako",
     "Africa/Bangui",
     "Africa/Banjul",
     "Africa/Bissau",
     "Africa/Blantyre",
     "Africa/Brazzaville",
     "Africa/Bujumbura",
     "Africa/Cairo",
     "Africa/Casablanca",
     "Africa/Ceuta",
     "Africa/Conakry",
     "Africa/Dakar",
     "Africa/Dar_es_Salaam",
     "Africa/Djibouti",
     "Africa/Douala",
     "Africa/El_Aaiun",
     "Africa/Freetown",
     "Africa/Gaborone",
     "Africa/Harare",
     "Africa/Johannesburg",
     "Africa/Juba",
     "Africa/Kampala",
     "Africa/Khartoum",
     "Africa/Kigali",
     "Africa/Kinshasa",
     "Africa/Lagos",
     "Africa/Libreville",
     "Africa/Lome",
     "Africa/Luanda",
     "Africa/Lubumbashi",
     "Africa/Lusaka",
     "Africa/Malabo",
     "Africa/Maputo",
     "Africa/Maseru",
     "Africa/Mbabane",
     "Africa/Mogadishu",
     "Africa/Monrovia",
     "Africa/Nairobi",
     "Africa/Ndjamena",
     "Africa/Niamey",
     "Africa/Nouakchott",
     "Africa/Ouagadougou",
     "Africa/Porto-Novo",
     "Africa/Sao_Tome",
     "Africa/Timbuktu",
     "Africa/Tripoli",
     "Africa/Tunis",
     "Africa/Windhoek",
     "America/Adak",
     "America/Anchorage",
     "America/Anguilla",
     "America/Antigua",
     "America/Araguaina",
     "America/Argentina/Buenos_Aires",
     "America/Argentina/Catamarca",
     "America/Argentina/ComodRivadavia",
     "America/Argentina/Cordoba",
     "America/Argentina/Jujuy",
     "America/Argentina/La_Rioja",
     "America/Argentina/Mendoza",
     "America/Argentina/Rio_Gallegos",
     "America/Argentina/Salta",
     "America/Argentina/San_Juan",
     "America/Argentina/San_Luis",
     "America/Argentina/Tucuman",
     "America/Argentina/Ushuaia",
     "America/Aruba",
     "America/Asuncion",
     "America/Atikokan",
     "America/Bahia",
     "America/Bahia_Banderas",
     "America/Barbados",
     "America/Belem",
     "America/Belize",
     "America/Blanc-Sablon",
     "America/Boa_Vista",
     "America/Bogota",
     "America/Boise",
     "America/Campo_Grande",
     "America/Cancun",
     "America/Caracas",
     "America/Cayenne",
     "America/Cayman",
     "America/Chicago",
     "America/Chihuahua",
     "America/Coral_Harbour",
     "America/Costa_Rica",
     "America/Creston",
     "America/Cuiaba",
     "America/Curacao",
     "America/Danmarkshavn",
     "America/Dawson",
     "America/Dawson_Creek",
     "America/Denver",
     "America/Detroit",
     "America/Dominica",
     "America/Edmonton",
     "America/Eirunepe",
     "America/El_Salvador",
     "America/Ensenada",
     "America/Fort_Nelson",
     "America/Fortaleza",
     "America/Glace_Bay",
     "America/Godthab",
     "America/Goose_Bay",
     "America/Grand_Turk",
     "America/Grenada",
     "America/Guadeloupe",
     "America/Guatemala",
     "America/Guayaquil",
     "America/Guyana",
     "America/Halifax",
     "America/Havana",
     "America/Hermosillo",
     "America/Indiana/Indianapolis",
     "America/Indiana/Knox",
     "America/Indiana/Marengo",
     "America/Indiana/Petersburg",
     "America/Indiana/Tell_City",
     "America/Indiana/Vevay",
     "America/Indiana/Vincennes",
     "America/Indiana/Winamac",
     "America/Jamaica",
     "America/Juneau",
     "America/Kentucky/Louisville",
     "America/Kentucky/Monticello",
     "America/La_Paz",
     "America/Lima",
     "America/Los_Angeles",
     "America/Maceio",
     "America/Managua",
     "America/Manaus",
     "America/Martinique",
     "America/Matamoros",
     "America/Mazatlan",
     "America/Menominee",
     "America/Merida",
     "America/Metlakatla",
     "America/Mexico_City",
     "America/Miquelon",
     "America/Moncton",
     "America/Monterrey",
     "America/Montevideo",
     "America/Montreal",
     "America/Montserrat",
     "America/Nassau",
     "America/New_York",
     "America/Nipigon",
     "America/Nome",
     "America/Noronha",
     "America/North_Dakota/Beulah",
     "America/North_Dakota/Center",
     "America/North_Dakota/New_Salem",
     "America/Ojinaga",
     "America/Panama",
     "America/Paramaribo",
     "America/Phoenix",
     "America/Port-au-Prince",
     "America/Port_of_Spain",
     "America/Porto_Velho",
     "America/Puerto_Rico",
     "America/Punta_Arenas",
     "America/Rainy_River",
     "America/Recife",
     "America/Regina",
     "America/Rio_Branco",
     "America/Rosario",
     "America/Santarem",
     "America/Santiago",
     "America/Santo_Domingo",
     "America/Sao_Paulo",
     "America/Scoresbysund",
     "America/Sitka",
     "America/St_Johns",
     "America/St_Kitts",
     "America/St_Lucia",
     "America/St_Thomas",
     "America/St_Vincent",
     "America/Swift_Current",
     "America/Tegucigalpa",
     "America/Thule",
     "America/Thunder_Bay",
     "America/Tijuana",
     "America/Toronto",
     "America/Tortola",
     "America/Vancouver",
     "America/Whitehorse",
     "America/Winnipeg",
     "America/Yakutat",
     "Asia/Aden",
     "Asia/Almaty",
     "Asia/Amman",
     "Asia/Anadyr",
     "Asia/Aqtau",
     "Asia/Aqtobe",
     "Asia/Ashgabat",
     "Asia/Atyrau",
     "Asia/Baghdad",
     "Asia/Bahrain",
     "Asia/Baku",
     "Asia/Bangkok",
     "Asia/Barnaul",
     "Asia/Beirut",
     "Asia/Bishkek",
     "Asia/Brunei",
     "Asia/Chita",
     "Asia/Choibalsan",
     "Asia/Chongqing",
     "Asia/Colombo",
     "Asia/Damascus",
     "Asia/Dhaka",
     "Asia/Dili",
     "Asia/Dubai",
     "Asia/Dushanbe",
     "Asia/Famagusta",
     "Asia/Gaza",
     "Asia/Hanoi",
     "Asia/Harbin",
     "Asia/Hebron",
     "Asia/Ho_Chi_Minh",
     "Asia/Hong_Kong",
     "Asia/Hovd",
     "Asia/Irkutsk",
     "Asia/Jakarta",
     "Asia/Jayapura",
     "Asia/Jerusalem",
     "Asia/Kabul",
     "Asia/Kamchatka",
     "Asia/Karachi",
     "Asia/Kashgar",
     "Asia/Kathmandu",
     "Asia/Khandyga",
     "Asia/Kolkata",
     "Asia/Krasnoyarsk",
     "Asia/Kuala_Lumpur",
     "Asia/Kuching",
     "Asia/Kuwait",
     "Asia/Macau",
     "Asia/Magadan",
     "Asia/Makassar",
     "Asia/Manila",
     "Asia/Muscat",
     "Asia/Nicosia",
     "Asia/Novokuznetsk",
     "Asia/Novosibirsk",
     "Asia/Omsk",
     "Asia/Oral",
     "Asia/Phnom_Penh",
     "Asia/Pontianak",
     "Asia/Pyongyang",
     "Asia/Qatar",
     "Asia/Qyzylorda",
     "Asia/Riyadh",
     "Asia/Sakhalin",
     "Asia/Samarkand",
     "Asia/Seoul",
     "Asia/Shanghai",
     "Asia/Singapore",
     "Asia/Srednekolymsk",
     "Asia/Taipei",
     "Asia/Tashkent",
     "Asia/Tbilisi",
     "Asia/Tehran",
     "Asia/Tel_Aviv",
     "Asia/Thimphu",
     "Asia/Tokyo",
     "Asia/Tomsk",
     "Asia/Ulaanbaatar",
     "Asia/Urumqi",
     "Asia/Ust-Nera",
     "Asia/Vientiane",
     "Asia/Vladivostok",
     "Asia/Yakutsk",
     "Asia/Yangon",
     "Asia/Yekaterinburg",
     "Asia/Yerevan",
     "Atlantic/Azores",
     "Atlantic/Bermuda",
     "Atlantic/Canary",
     "Atlantic/Cape_Verde",
     "Atlantic/Faroe",
     "Atlantic/Madeira",
     "Atlantic/Reykjavik",
     "Atlantic/South_Georgia",
     "Atlantic/St_Helena",
     "Atlantic/Stanley",
     "Australia/Adelaide",
     "Australia/Brisbane",
     "Australia/Broken_Hill",
     "Australia/Currie",
     "Australia/Darwin",
     "Australia/Eucla",
     "Australia/Hobart",
     "Australia/Lindeman",
     "Australia/Lord_Howe",
     "Australia/Melbourne",
     "Australia/Perth",
     "Australia/Sydney",
     "Europe/Amsterdam",
     "Europe/Andorra",
     "Europe/Astrakhan",
     "Europe/Athens",
     "Europe/Belfast",
     "Europe/Belgrade",
     "Europe/Berlin",
     "Europe/Brussels",
     "Europe/Bucharest",
     "Europe/Budapest",
     "Europe/Chisinau",
     "Europe/Copenhagen",
     "Europe/Dublin",
     "Europe/Gibraltar",
     "Europe/Guernsey",
     "Europe/Helsinki",
     "Europe/Isle_of_Man",
     "Europe/Istanbul",
     "Europe/Jersey",
     "Europe/Kaliningrad",
     "Europe/Kiev",
     "Europe/Kirov",
     "Europe/Lisbon",
     "Europe/Ljubljana",
     "Europe/London",
     "Europe/Luxembourg",
     "Europe/Madrid",
     "Europe/Malta",
     "Europe/Minsk",
     "Europe/Monaco",
     "Europe/Moscow",
     "Europe/Oslo",
     "Europe/Paris",
     "Europe/Prague",
     "Europe/Riga",
     "Europe/Rome",
     "Europe/Samara",
     "Europe/Sarajevo",
     "Europe/Saratov",
     "Europe/Simferopol",
     "Europe/Skopje",
     "Europe/Sofia",
     "Europe/Stockholm",
     "Europe/Tallinn",
     "Europe/Tirane",
     "Europe/Tiraspol",
     "Europe/Ulyanovsk",
     "Europe/Uzhgorod",
     "Europe/Vaduz",
     "Europe/Vienna",
     "Europe/Vilnius",
     "Europe/Volgograd",
     "Europe/Warsaw",
     "Europe/Zagreb",
     "Europe/Zaporozhye",
     "Europe/Zurich",
     "Indian/Antananarivo",
     "Indian/Chagos",
     "Indian/Christmas",
     "Indian/Cocos",
     "Indian/Comoro",
     "Indian/Mahe",
     "Indian/Maldives",
     "Indian/Mauritius",
     "Indian/Mayotte",
     "Indian/Reunion",
     "Pacific/Apia",
     "Pacific/Auckland",
     "Pacific/Bougainville",
     "Pacific/Chatham",
     "Pacific/Chuuk",
     "Pacific/Easter",
     "Pacific/Efate",
     "Pacific/Enderbury",
     "Pacific/Fakaofo",
     "Pacific/Fiji",
     "Pacific/Funafuti",
     "Pacific/Galapagos",
     "Pacific/Gambier",
     "Pacific/Guadalcanal",
     "Pacific/Guam",
     "Pacific/Honolulu",
     "Pacific/Kiritimati",
     "Pacific/Kosrae",
     "Pacific/Kwajalein",
     "Pacific/Majuro",
     "Pacific/Marquesas",
     "Pacific/Midway",
     "Pacific/Nauru",
     "Pacific/Niue",
     "Pacific/Norfolk",
     "Pacific/Noumea",
     "Pacific/Pago_Pago",
     "Pacific/Palau",
     "Pacific/Pitcairn",
     "Pacific/Pohnpei",
     "Pacific/Port_Moresby",
     "Pacific/Rarotonga",
     "Pacific/Saipan",
     "Pacific/Tahiti",
     "Pacific/Tarawa",
     "Pacific/Tongatapu",
     "Pacific/Wake",
     "Pacific/Wallis",
   }),
   "LST": ({
     "Europe/Riga",
   }),
   "MDDT": ({
     "America/Cambridge_Bay",
     "America/Inuvik",
     "America/Yellowknife",
   }),
   "MDST": ({
     "Europe/Moscow",
   }),
   "MDT": ({
     "America/Bahia_Banderas",
     "America/Boise",
     "America/Cambridge_Bay",
     "America/Chihuahua",
     "America/Denver",
     "America/Edmonton",
     "America/Hermosillo",
     "America/Inuvik",
     "America/Mazatlan",
     "America/North_Dakota/Beulah",
     "America/North_Dakota/Center",
     "America/North_Dakota/New_Salem",
     "America/Ojinaga",
     "America/Phoenix",
     "America/Regina",
     "America/Swift_Current",
     "America/Yellowknife",
     "MST7MDT",
   }),
   "MEST": ({
     "MET",
   }),
   "MET": ({
     "MET",
   }),
   "MMT": ({
     "Africa/Monrovia",
     "America/Managua",
     "America/Montevideo",
     "Asia/Colombo",
     "Asia/Kolkata",
     "Asia/Makassar",
     "Europe/Minsk",
     "Europe/Moscow",
     "Indian/Maldives",
   }),
   "MPT": ({
     "America/Boise",
     "America/Cambridge_Bay",
     "America/Denver",
     "America/Edmonton",
     "America/Inuvik",
     "America/North_Dakota/Beulah",
     "America/North_Dakota/Center",
     "America/North_Dakota/New_Salem",
     "America/Ojinaga",
     "America/Phoenix",
     "America/Regina",
     "America/Swift_Current",
     "America/Yellowknife",
     "MST7MDT",
   }),
   "MSD": ({
     "Europe/Chisinau",
     "Europe/Kaliningrad",
     "Europe/Kiev",
     "Europe/Minsk",
     "Europe/Moscow",
     "Europe/Riga",
     "Europe/Simferopol",
     "Europe/Tallinn",
     "Europe/Tiraspol",
     "Europe/Uzhgorod",
     "Europe/Vilnius",
     "Europe/Zaporozhye",
   }),
   "MSK": ({
     "Europe/Chisinau",
     "Europe/Kaliningrad",
     "Europe/Kiev",
     "Europe/Minsk",
     "Europe/Moscow",
     "Europe/Riga",
     "Europe/Simferopol",
     "Europe/Tallinn",
     "Europe/Tiraspol",
     "Europe/Uzhgorod",
     "Europe/Vilnius",
     "Europe/Zaporozhye",
   }),
   "MST": ({
     "America/Bahia_Banderas",
     "America/Boise",
     "America/Cambridge_Bay",
     "America/Chihuahua",
     "America/Creston",
     "America/Dawson_Creek",
     "America/Denver",
     "America/Edmonton",
     "America/Ensenada",
     "America/Fort_Nelson",
     "America/Hermosillo",
     "America/Inuvik",
     "America/Mazatlan",
     "America/Mexico_City",
     "America/North_Dakota/Beulah",
     "America/North_Dakota/Center",
     "America/North_Dakota/New_Salem",
     "America/Ojinaga",
     "America/Phoenix",
     "America/Regina",
     "America/Swift_Current",
     "America/Tijuana",
     "America/Yellowknife",
     "Europe/Moscow",
     "MST",
     "MST7MDT",
   }),
   "MWT": ({
     "America/Bahia_Banderas",
     "America/Boise",
     "America/Cambridge_Bay",
     "America/Chihuahua",
     "America/Denver",
     "America/Edmonton",
     "America/Hermosillo",
     "America/Inuvik",
     "America/Mazatlan",
     "America/North_Dakota/Beulah",
     "America/North_Dakota/Center",
     "America/North_Dakota/New_Salem",
     "America/Ojinaga",
     "America/Phoenix",
     "America/Regina",
     "America/Swift_Current",
     "America/Yellowknife",
     "MST7MDT",
   }),
   "NDDT": ({
     "America/Goose_Bay",
     "America/St_Johns",
   }),
   "NDT": ({
     "America/Adak",
     "America/Goose_Bay",
     "America/Nome",
     "America/St_Johns",
   }),
   "NFT": ({
     "Europe/Oslo",
     "Europe/Paris",
   }),
   "NPT": ({
     "America/Adak",
     "America/Goose_Bay",
     "America/Nome",
     "America/St_Johns",
   }),
   "NST": ({
     "America/Adak",
     "America/Goose_Bay",
     "America/Nome",
     "America/St_Johns",
     "Europe/Amsterdam",
   }),
   "NWT": ({
     "America/Adak",
     "America/Goose_Bay",
     "America/Nome",
     "America/St_Johns",
   }),
   "NZDT": ({
     "Antarctica/McMurdo",
     "Pacific/Auckland",
   }),
   "NZMT": ({
     "Antarctica/McMurdo",
     "Pacific/Auckland",
   }),
   "NZST": ({
     "Antarctica/McMurdo",
     "Pacific/Auckland",
   }),
   "P%sT": ({
     "America/Ensenada",
   }),
   "PDDT": ({
     "America/Dawson",
     "America/Inuvik",
     "America/Whitehorse",
   }),
   "PDT": ({
     "America/Boise",
     "America/Dawson",
     "America/Dawson_Creek",
     "America/Fort_Nelson",
     "America/Inuvik",
     "America/Juneau",
     "America/Los_Angeles",
     "America/Metlakatla",
     "America/Sitka",
     "America/Tijuana",
     "America/Vancouver",
     "America/Whitehorse",
     "PST8PDT",
   }),
   "PKST": ({
     "Asia/Karachi",
   }),
   "PKT": ({
     "Asia/Karachi",
   }),
   "PLMT": ({
     "Asia/Hanoi",
     "Asia/Ho_Chi_Minh",
     "Asia/Phnom_Penh",
     "Asia/Vientiane",
   }),
   "PMMT": ({
     "Pacific/Bougainville",
     "Pacific/Port_Moresby",
   }),
   "PMT": ({
     "Africa/Algiers",
     "Africa/Tunis",
     "America/Paramaribo",
     "Asia/Pontianak",
     "Asia/Yekaterinburg",
     "Europe/Monaco",
     "Europe/Paris",
     "Europe/Prague",
   }),
   "PPMT": ({
     "America/Port-au-Prince",
   }),
   "PPT": ({
     "America/Boise",
     "America/Dawson",
     "America/Dawson_Creek",
     "America/Fort_Nelson",
     "America/Inuvik",
     "America/Juneau",
     "America/Los_Angeles",
     "America/Metlakatla",
     "America/Sitka",
     "America/Tijuana",
     "America/Vancouver",
     "America/Whitehorse",
     "PST8PDT",
   }),
   "PST": ({
     "America/Bahia_Banderas",
     "America/Boise",
     "America/Creston",
     "America/Dawson",
     "America/Dawson_Creek",
     "America/Ensenada",
     "America/Fort_Nelson",
     "America/Hermosillo",
     "America/Inuvik",
     "America/Juneau",
     "America/Los_Angeles",
     "America/Mazatlan",
     "America/Metlakatla",
     "America/Sitka",
     "America/Tijuana",
     "America/Vancouver",
     "America/Whitehorse",
     "PST8PDT",
   }),
   "PWT": ({
     "America/Boise",
     "America/Dawson",
     "America/Dawson_Creek",
     "America/Fort_Nelson",
     "America/Inuvik",
     "America/Juneau",
     "America/Los_Angeles",
     "America/Metlakatla",
     "America/Sitka",
     "America/Tijuana",
     "America/Vancouver",
     "America/Whitehorse",
     "PST8PDT",
   }),
   "QMT": ({
     "America/Guayaquil",
   }),
   "RMT": ({
     "Asia/Yangon",
     "Europe/Riga",
     "Europe/Rome",
   }),
   "S": ({
     "Europe/Amsterdam",
     "Europe/Moscow",
   }),
   "SAST": ({
     "Africa/Gaborone",
     "Africa/Johannesburg",
     "Africa/Maseru",
     "Africa/Mbabane",
     "Africa/Windhoek",
   }),
   "SDMT": ({
     "America/Santo_Domingo",
   }),
   "SET": ({
     "Europe/Stockholm",
   }),
   "SJMT": ({
     "America/Costa_Rica",
   }),
   "SMT": ({
     "America/Punta_Arenas",
     "America/Santiago",
     "Asia/Kuala_Lumpur",
     "Asia/Singapore",
     "Atlantic/Stanley",
     "Europe/Simferopol",
   }),
   "SST": ({
     "Pacific/Pago_Pago",
   }),
   "TBMT": ({
     "Asia/Tbilisi",
   }),
   "TMT": ({
     "Asia/Tehran",
     "Europe/Tallinn",
   }),
   "UCT": ({
     "Etc/UCT",
   }),
   "UTC": ({
     "Etc/UTC",
   }),
   "WAST": ({
     "Africa/Ndjamena",
   }),
   "WAT": ({
     "Africa/Bangui",
     "Africa/Brazzaville",
     "Africa/Douala",
     "Africa/Kinshasa",
     "Africa/Lagos",
     "Africa/Libreville",
     "Africa/Luanda",
     "Africa/Malabo",
     "Africa/Ndjamena",
     "Africa/Niamey",
     "Africa/Porto-Novo",
     "Africa/Sao_Tome",
     "Africa/Windhoek",
   }),
   "WEMT": ({
     "Africa/Ceuta",
     "Atlantic/Madeira",
     "Europe/Lisbon",
     "Europe/Madrid",
     "Europe/Monaco",
     "Europe/Paris",
   }),
   "WEST": ({
     "Africa/Algiers",
     "Africa/Casablanca",
     "Africa/Ceuta",
     "Africa/El_Aaiun",
     "Atlantic/Azores",
     "Atlantic/Canary",
     "Atlantic/Faroe",
     "Atlantic/Madeira",
     "Europe/Brussels",
     "Europe/Lisbon",
     "Europe/Luxembourg",
     "Europe/Madrid",
     "Europe/Monaco",
     "Europe/Paris",
     "WET",
   }),
   "WET": ({
     "Africa/Algiers",
     "Africa/Casablanca",
     "Africa/Ceuta",
     "Africa/El_Aaiun",
     "Atlantic/Azores",
     "Atlantic/Canary",
     "Atlantic/Faroe",
     "Atlantic/Madeira",
     "Europe/Andorra",
     "Europe/Brussels",
     "Europe/Lisbon",
     "Europe/Luxembourg",
     "Europe/Madrid",
     "Europe/Monaco",
     "Europe/Paris",
     "WET",
   }),
   "WIB": ({
     "Asia/Jakarta",
     "Asia/Pontianak",
   }),
   "WIT": ({
     "Asia/Jayapura",
   }),
   "WITA": ({
     "Asia/Makassar",
     "Asia/Pontianak",
   }),
   "WMT": ({
     "Europe/Vilnius",
     "Europe/Warsaw",
   }),
   "YDDT": ({
     "America/Dawson",
     "America/Whitehorse",
   }),
   "YDT": ({
     "America/Anchorage",
     "America/Dawson",
     "America/Juneau",
     "America/Nome",
     "America/Sitka",
     "America/Whitehorse",
     "America/Yakutat",
   }),
   "YPT": ({
     "America/Anchorage",
     "America/Dawson",
     "America/Juneau",
     "America/Nome",
     "America/Sitka",
     "America/Whitehorse",
     "America/Yakutat",
   }),
   "YST": ({
     "America/Anchorage",
     "America/Dawson",
     "America/Juneau",
     "America/Nome",
     "America/Sitka",
     "America/Whitehorse",
     "America/Yakutat",
   }),
   "YWT": ({
     "America/Anchorage",
     "America/Dawson",
     "America/Juneau",
     "America/Nome",
     "America/Sitka",
     "America/Whitehorse",
     "America/Yakutat",
   }),
]);

// this is used by the timezone expert system,
// that uses localtime (or whatever) to figure out the *correct* timezone

// note that at least Red Hat 6.2 has an error in the time database,
// so a lot of Europeean (Paris, Brussels) times get to be Luxembourg /Mirar

mapping timezone_expert_tree=
([ "test":57801600, // 1971-11-01 00:00:00
   -36000:
   ([ "test":688490994, // 1991-10-26 15:29:54
      -32400:"Asia/Vladivostok",
      -36000:
      ([ "test":-441849600, // 1956-01-01 00:00:00
	 -32400:
	 ({
	    "Pacific/Saipan",
	    "Pacific/Yap",
	 }),
	 0:"Antarctica/DumontDUrville",
	 -36000:
	 ({
	    "Pacific/Guam",
	    "Pacific/Truk",
	    "Pacific/Port_Moresby",
	 }),]),
      -37800:"Australia/Lord_Howe",]),
   3600:
   ([ "test":157770005, // 1975-01-01 01:00:05
      3600:"Africa/El_Aaiun",
      0:"Africa/Bissau",]),
   36000:
   ([ "test":9979204, // 1970-04-26 12:00:04
      36000:
      ([ "test":-725846400, // 1947-01-01 00:00:00
	 37800:"Pacific/Honolulu",
	 36000:
	 ([ "test":-1830384000, // 1912-01-01 00:00:00
	    36000:"Pacific/Fakaofo",
	    35896:"Pacific/Tahiti",]),]),
      32400:"America/Anchorage",]),
   -19800:
   ([ "test":832962604, // 1996-05-24 18:30:04
      -23400:"Asia/Colombo",
      -20700:"Asia/Katmandu",
      -19800:"Asia/Calcutta",
      -21600:"Asia/Thimbu",]),
   43200:
   ([ "test":307627204, // 1979-10-01 12:00:04
      43200:"Pacific/Kwajalein",
      39600:"Pacific/Enderbury",]),
   10800:
   ([ "test":323845205, // 1980-04-06 05:00:05
      10800:
      ([ "test":511149594, // 1986-03-14 01:59:54
	 14400:"Antarctica/Palmer",
	 10800:
	 ([ "test":-189388800, // 1964-01-01 00:00:00
	    14400:"America/Cayenne",
	    10800:
	    ([ "test":-1767225600, // 1914-01-01 00:00:00
	       8572:"America/Maceio",
	       11568:"America/Araguaina",
	       9240:"America/Fortaleza",
	       11636:"America/Belem",]),
	    7200:"America/Sao_Paulo",]),
	 7200:
	 ([ "test":678319194, // 1991-06-30 21:59:54
	    14400:
	    ([ "test":686721605, // 1991-10-06 04:00:05
	       14400:"America/Mendoza",
	       10800:"America/Jujuy",]),
	    10800:
	    ([ "test":678337204, // 1991-07-01 03:00:04
	       7200:"America/Catamarca",
	       10800:"America/Cordoba",]),
	    7200:
	    ([ "test":938901595, // 1999-10-02 21:59:55
	       10800:"America/Rosario",
	       7200:"America/Buenos_Aires",]),]),]),
      14400:"America/Santiago",
      7200:"America/Godthab",]),
   -12600:"Asia/Tehran",
   2670:"Africa/Monrovia",
   -28800:
   ([ "test":690314404, // 1991-11-16 18:00:04
      -25200:"Asia/Irkutsk",
      -28800:
      ([ "test":515520004, // 1986-05-03 16:00:04
	 -28800:
	 ([ "test":9315004, // 1970-04-18 19:30:04
	    -28800:
	    ([ "test":133977605, // 1974-03-31 16:00:05
	       -28800:
	       ([ "test":259344004, // 1978-03-21 16:00:04
		  -32400:"Asia/Manila",
		  -28800:
		  ([ "test":-788918400, // 1945-01-01 00:00:00
		     -28800:"Asia/Brunei",
		     -32400:
		     ([ "test":-770601600, // 1945-08-01 00:00:00
			-32400:"Asia/Kuching",
			-28800:"Asia/Ujung_Pandang",]),
		     0:"Antarctica/Casey",]),]),
	       -32400:"Asia/Taipei",]),
	    -32400:
	    ([ "test":72214195, // 1972-04-15 19:29:55
	       -32400:"Asia/Macao",
	       -28800:"Asia/Hong_Kong",]),]),
	 -32400:"Asia/Shanghai",]),
      -32400:"Australia/Perth",]),
   -45000:"Pacific/Auckland",
   34200:"Pacific/Marquesas",
   -21600:
   ([ "test":515520004, // 1986-05-03 16:00:04
      -25200:
      ([ "test":717526795, // 1992-09-26 16:59:55
	 -25200:"Asia/Almaty",
	 -21600:
	 ([ "test":683582405, // 1991-08-30 20:00:05
	    -21600:"Asia/Tashkent",
	    -18000:"Asia/Bishkek",]),
	 -18000:"Asia/Dushanbe",]),
      -28800:"Asia/Hovd",
      -32400:"Asia/Urumqi",
      -21600:
      ([ "test":670363205, // 1991-03-30 20:00:05
	 -18000:"Asia/Omsk",
	 -21600:
	 ([ "test":-504921600, // 1954-01-01 00:00:00
	    -21600:"Asia/Dacca",
	    0:"Antarctica/Mawson",]),]),]),
   41400:"Pacific/Niue",
   -14400:
   ([ "test":606866404, // 1989-03-25 22:00:04
      -10800:
      ([ "test":606866394, // 1989-03-25 21:59:54
	 -10800:
	 ([ "test":-1609459200, // 1919-01-01 00:00:00
	    -12368:"Asia/Qatar",
	    -12140:"Asia/Bahrain",]),
	 -14400:"Europe/Samara",]),
      -14400:
      ([ "test":-2019686400, // 1906-01-01 00:00:00
	 -14060:"Asia/Muscat",
	 -13800:"Indian/Mauritius",
	 -13272:"Asia/Dubai",
	 -13308:"Indian/Mahe",
	 -13312:"Indian/Reunion",]),
      -18000:
      ([ "test":796172395, // 1995-03-25 22:59:55
	 -10800:"Asia/Yerevan",
	 -14400:"Asia/Baku",
	 -18000:"Asia/Tbilisi",]),]),
   18000:
   ([ "test":104914805, // 1973-04-29 07:00:05
      14400:
      ([ "test":9961194, // 1970-04-26 06:59:54
	 14400:"America/Havana",
	 18000:
	 ([ "test":9961204, // 1970-04-26 07:00:04
	    14400:
	    ([ "test":126687605, // 1974-01-06 07:00:05
	       18000:
	       ([ "test":162370804, // 1975-02-23 07:00:04
		  18000:
		  ([ "test":-210556800, // 1963-05-01 00:00:00
		     18000:"America/Nassau",
		     14400:"America/Montreal",]),
		  14400:
		  ([ "test":199263605, // 1976-04-25 07:00:05
		     14400:"America/Louisville",
		     18000:"America/Indiana/Marengo",]),]),
	       14400:"America/New_York",]),
	    18000:"America/Detroit",]),]),
      18000:
      ([ "test":514969195, // 1986-04-27 06:59:55
	 14400:
	 ([ "test":421217995, // 1983-05-08 04:59:55
	    14400:"America/Grand_Turk",
	    18000:"America/Port-au-Prince",]),
	 21600:"Pacific/Galapagos",
	 18000:
	 ([ "test":954658805, // 2000-04-02 07:00:05
	    14400:
	    ([ "test":9961204, // 1970-04-26 07:00:04
	       18000:"America/Nipigon",
	       14400:"America/Thunder_Bay",]),
	    18000:
	    ([ "test":9961204, // 1970-04-26 07:00:04
	       18000:
	       ([ "test":136364405, // 1974-04-28 07:00:05
		  18000:
		  ([ "test":704782804, // 1992-05-02 05:00:04
		     18000:
		     ([ "test":536475605, // 1987-01-01 05:00:05
			14400:"America/Lima",
			18000:
			([ "test":-1830384000, // 1912-01-01 00:00:00
			   18432:"America/Cayman",
			   18840:"America/Guayaquil",
			   16272:"America/Porto_Acre",
			   18000:"America/Panama",]),]),
		     14400:"America/Bogota",]),
		  14400:"America/Jamaica",]),
	       14400:
	       ([ "test":41410805, // 1971-04-25 07:00:05
		  14400:"America/Indiana/Vevay",
		  18000:"America/Indianapolis",]),]),
	    21600:"America/Iqaluit",]),]),
      21600:"America/Menominee",]),
   -7200:
   ([ "test":291761995, // 1979-03-31 20:59:55
      -7200:
      ([ "test":323816404, // 1980-04-05 21:00:04
	 -7200:
	 ([ "test":386125194, // 1982-03-28 00:59:54
	    -7200:
	    ([ "test":767746795, // 1994-04-30 22:59:55
	       -7200:
	       ([ "test":10364394, // 1970-04-30 22:59:54
		  -7200:
		  ([ "test":10364404, // 1970-04-30 23:00:04
		     -7200:
		     ([ "test":142379994, // 1974-07-06 21:59:54
			-10800:"Asia/Damascus",
			-7200:
			([ "test":142380004, // 1974-07-06 22:00:04
			   -10800:
			   ([ "test":828655204, // 1996-04-04 22:00:04
			      -10800:"Asia/Gaza",
			      -7200:"Asia/Jerusalem",]),
			   -7200:
			   ([ "test":-820540800, // 1944-01-01 00:00:00
			      -7200:
			      ([ "test":-2114380800, // 1903-01-01 00:00:00
				 -7216:"Africa/Kigali",
				 -7200:
				 ({
				    "Africa/Bujumbura",
				    "Africa/Lubumbashi",
				 }),
				 -7452:"Africa/Harare",
				 -6788:"Africa/Lusaka",
				 -7464:"Africa/Mbabane",
				 -7820:"Africa/Maputo",
				 -8400:"Africa/Blantyre",]),
			      -10800:
			      ([ "test":-852076800, // 1943-01-01 00:00:00
				 -10800:"Africa/Johannesburg",
				 -7200:
				 ([ "test":-2114380800, // 1903-01-01 00:00:00
				    -6600:"Africa/Maseru",
				    -7200:"Africa/Gaborone",]),]),
			      -3600:"Europe/Athens",]),]),]),
		     -10800:"Africa/Cairo",]),
		  -10800:"Africa/Khartoum",]),
	       -10800:
	       ([ "test":354686404, // 1981-03-29 04:00:04
		  -7200:
		  ([ "test":108165594, // 1973-06-05 21:59:54
		     -10800:"Asia/Beirut",
		     -7200:"Asia/Amman",]),
		  -10800:"Europe/Helsinki",]),
	       -3600:"Africa/Windhoek",]),
	    -10800:"Asia/Nicosia",
	    -3600:"Africa/Tripoli",]),
	 -10800:
	 ([ "test":291762005, // 1979-03-31 21:00:05
	    -7200:"Europe/Bucharest",
	    -10800:"Europe/Sofia",]),
	 -3600:"Europe/Berlin",]),
      -10800:"Europe/Istanbul",
      -3600:
      ([ "test":-810086400, // 1944-05-01 00:00:00
	 -3600:"Europe/Monaco",
	 -7200:"Europe/Paris",]),]),
   -37800:
   ([ "test":384280204, // 1982-03-06 16:30:04
      -37800:"Australia/Broken_Hill",
      -34200:"Australia/Adelaide",]),
   25200:
   ([ "test":954665994, // 2000-04-02 08:59:54
      25200:
      ([ "test":9968404, // 1970-04-26 09:00:04
	 25200:
	 ([ "test":73472404, // 1972-04-30 09:00:04
	    25200:
	    ([ "test":325674005, // 1980-04-27 09:00:05
	       25200:
	       ([ "test":28795, // 1970-01-01 07:59:55
		  28800:
		  ([ "test":923216404, // 1999-04-04 09:00:04
		     21600:"America/Mazatlan",
		     25200:"America/Hermosillo",]),
		  25200:"America/Phoenix",]),
	       21600:"America/Yellowknife",]),
	    21600:"America/Edmonton",]),
	 21600:
	 ([ "test":126694804, // 1974-01-06 09:00:04
	    25200:"America/Boise",
	    21600:"America/Denver",]),]),
      18000:"America/Cambridge_Bay",
      21600:"America/Swift_Current",]),
   -30600:"Asia/Harbin",
   9000:"America/Montevideo",
   32400:
   ([ "test":325681205, // 1980-04-27 11:00:05
      32400:"Pacific/Gambier",
      25200:"America/Dawson",
      28800:"America/Yakutat",]),
   -46800:
   ([ "test":938782794, // 1999-10-01 12:59:54
      -43200:"Asia/Anadyr",
      -46800:"Pacific/Tongatapu",]),
   -23400:"Asia/Rangoon",
   39600:
   ([ "test":436291204, // 1983-10-29 16:00:04
      32400:"America/Adak",
      28800:"America/Nome",
      39600:
      ([ "test":-631152000, // 1950-01-01 00:00:00
	 39600:"Pacific/Midway",
	 41400:
	 ([ "test":-1861920000, // 1911-01-01 00:00:00
	    41216:"Pacific/Apia",
	    40968:"Pacific/Pago_Pago",]),]),]),
   16200:"America/Santo_Domingo",
   -39600:
   ([ "test":625593594, // 1989-10-28 15:59:54
      -43200:"Pacific/Efate",
      -39600:
      ([ "test":849366004, // 1996-11-30 15:00:04
	 -39600:
	 ([ "test":670345204, // 1991-03-30 15:00:04
	    -36000:"Asia/Magadan",
	    -39600:
	    ([ "test":-1830384000, // 1912-01-01 00:00:00
	       -38388:"Pacific/Guadalcanal",
	       -39600:"Pacific/Ponape",]),]),
	 -43200:"Pacific/Noumea",]),
      -36000:
      ([ "test":636480005, // 1990-03-03 16:00:05
	 -36000:
	 ([ "test":719942404, // 1992-10-24 16:00:04
	    -39600:
	    ([ "test":89136004, // 1972-10-28 16:00:04
	       -39600:"Australia/Sydney",
	       -36000:"Australia/Lindeman",]),
	    -36000:"Australia/Brisbane",]),
	 -39600:
	 ([ "test":5673595, // 1970-03-07 15:59:55
	    -36000:"Australia/Melbourne",
	    -39600:"Australia/Hobart",]),]),]),
   -16200:"Asia/Kabul",
   0:
   ([ "test":512528405, // 1986-03-30 01:00:05
      -7200:"Africa/Ceuta",
      0:
      ([ "test":57722395, // 1971-10-31 01:59:55
	 0:
	 ([ "test":141264004, // 1974-06-24 00:00:04
	    0:
	    ([ "test":-862617600, // 1942-09-01 00:00:00
	       0:
	       ([ "test":-63158400, // 1968-01-01 00:00:00
		  0:
		  ([ "test":-915148800, // 1941-01-01 00:00:00
		     0:
		     ([ "test":-1830384000, // 1912-01-01 00:00:00
			968:"Africa/Abidjan",
			364:"Africa/Ouagadougou",
			0:"UTC", // "Africa/Lome",
			724:"Africa/Timbuktu",
			2192:"Africa/Sao_Tome",]),
		     3600:"Africa/Dakar",]),
		  3600:"Atlantic/Reykjavik",]),
	       1200:"Africa/Freetown",
	       3600:
	       ([ "test":-189388800, // 1964-01-01 00:00:00
		  0:
		  ([ "test":-312940800, // 1960-02-01 00:00:00
		     3600:
		     ([ "test":-299894400, // 1960-07-01 00:00:00
			3600:"Africa/Nouakchott",
			0:"Africa/Bamako",]),
		     0:"Africa/Conakry",]),
		  3600:"Africa/Banjul",]),
	       1368:"Atlantic/St_Helena",
	       -1200:"Africa/Accra",]),
	    -3600:"Africa/Casablanca",]),
	 -3600:
	 ([ "test":-718070400, // 1947-04-01 00:00:00
	    -3600:"Europe/Dublin",
	    0:
	    ([ "test":-1704153600, // 1916-01-01 00:00:00
	       1521:"Europe/Belfast",
	       0:"Europe/London",]),]),]),
      3600:"Atlantic/Azores",
      -3600:
      ([ "test":354675605, // 1981-03-29 01:00:05
	 0:"Africa/Algiers",
	 -3600:
	 ([ "test":323827205, // 1980-04-06 00:00:05
	    -3600:"Atlantic/Canary",
	    0:"Atlantic/Faeroe",]),]),]),
   -32400:
   ([ "test":547570804, // 1987-05-09 15:00:04
      -28800:"Asia/Dili",
      -32400:
      ([ "test":670352405, // 1991-03-30 17:00:05
	 -28800:"Asia/Yakutsk",
	 -32400:
	 ([ "test":-283996800, // 1961-01-01 00:00:00
	    -28800:"Asia/Pyongyang",
	    -32400:
	    ({
	       "Pacific/Palau",
	       "Asia/Tokyo",
	    }),
	    -34200:"Asia/Jayapura",]),]),
      -36000:"Asia/Seoul",]),
   30600:"Pacific/Pitcairn",
   -25200:
   ([ "test":671558395, // 1991-04-13 15:59:55
      -25200:
      ([ "test":-410227200, // 1957-01-01 00:00:00
	 -25200:
	 ([ "test":-2019686400, // 1906-01-01 00:00:00
	    -24624:"Asia/Vientiane",
	    -25180:"Asia/Phnom_Penh",
	    -25200:"Indian/Christmas",
	    -24124:"Asia/Bangkok",
	    -25600:"Asia/Saigon",]),
	 -27000:"Asia/Jakarta",
	 0:"Antarctica/Davis",]),
      -28800:"Asia/Chungking",
      -32400:"Asia/Ulaanbaatar",
      -21600:
      ([ "test":738140405, // 1993-05-23 07:00:05
	 -25200:"Asia/Krasnoyarsk",
	 -21600:"Asia/Novosibirsk",]),]),
   38400:"Pacific/Kiritimati",
   7200:
   ([ "test":354675604, // 1981-03-29 01:00:04
      7200:
      ([ "test":-1167609600, // 1933-01-01 00:00:00
	 7200:"Atlantic/South_Georgia",
	 3600:"America/Noronha",]),
      3600:"Atlantic/Cape_Verde",
      0:"America/Scoresbysund",]),
   37800:"Pacific/Rarotonga",
   -18000:
   ([ "test":670366805, // 1991-03-30 21:00:05
      -25200:"Asia/Samarkand",
      -28800:"Asia/Kashgar",
      -14400:"Asia/Yekaterinburg",
      -21600:"Asia/Ashkhabad",
      -18000:
      ([ "test":828212405, // 1996-03-30 19:00:05
	 -14400:"Asia/Aqtau",
	 -18000:
	 ([ "test":-599616000, // 1951-01-01 00:00:00
	    -17640:"Indian/Maldives",
	    -19800:"Asia/Karachi",
	    -18000:"Indian/Kerguelen",]),
	 -21600:"Asia/Aqtobe",]),]),
   14400:
   ([ "test":796802394, // 1995-04-02 05:59:54
      10800:"Atlantic/Stanley",
      14400:
      ([ "test":733903204, // 1993-04-04 06:00:04
	 10800:
	 ([ "test":9957604, // 1970-04-26 06:00:04
	    14400:
	    ([ "test":73461604, // 1972-04-30 06:00:04
	       14400:
	       ([ "test":136360805, // 1974-04-28 06:00:05
		  10800:"Atlantic/Bermuda",
		  14400:"America/Thule",]),
	       10800:"America/Glace_Bay",]),
	    10800:"America/Halifax",]),
	 14400:
	 ([ "test":234943204, // 1977-06-12 06:00:04
	    14400:
	    ([ "test":323841605, // 1980-04-06 04:00:05
	       14400:
	       ([ "test":86760004, // 1972-10-01 04:00:04
		  14400:
		  ([ "test":-788918400, // 1945-01-01 00:00:00
		     14400:
		     ([ "test":-1861920000, // 1911-01-01 00:00:00
			14404:"America/Manaus",
			15584:"America/St_Thomas",
			14764:"America/Port_of_Spain",
			15336:"America/Porto_Velho",
			15052:"America/St_Kitts",
			15136:"America/Anguilla",
			15508:"America/Tortola",
			14640:"America/St_Lucia",
			14820:"America/Grenada",
			14736:"America/Dominica",
			13460:"America/Cuiaba",
			16356:"America/La_Paz",
			14560:"America/Boa_Vista",
			14932:"America/Montserrat",
			14696:"America/St_Vincent",
			14768:"America/Guadeloupe",]),
		     10800:"America/Puerto_Rico",
		     12600:"America/Goose_Bay",
		     16200:
		     ([ "test":-1830384000, // 1912-01-01 00:00:00
			16064:"America/Caracas",
			16544:"America/Curacao",
			16824:"America/Aruba",]),
		     18000:"America/Antigua",]),
		  10800:"America/Asuncion",]),
	       10800:"America/Martinique",]),
	    10800:"America/Barbados",]),]),
      7200:"America/Miquelon",
      18000:"America/Pangnirtung",]),
   21600:
   ([ "test":891763194, // 1998-04-05 07:59:54
      14400:"America/Cancun",
      25200:"America/Chihuahua",
      21600:
      ([ "test":9964805, // 1970-04-26 08:00:05
	 25200:"Pacific/Easter",
	 21600:
	 ([ "test":136368004, // 1974-04-28 08:00:04
	    21600:
	    ([ "test":325670404, // 1980-04-27 08:00:04
	       21600:
	       ([ "test":828864004, // 1996-04-07 08:00:04
		  21600:
		  ([ "test":123919195, // 1973-12-05 05:59:55
		     21600:
		     ([ "test":123919205, // 1973-12-05 06:00:05
			21600:
			([ "test":547020004, // 1987-05-03 06:00:04
			   18000:
			   ([ "test":-1546300800, // 1921-01-01 00:00:00
			      20932:"America/Tegucigalpa",
			      21408:"America/El_Salvador",]),
			   21600:"America/Regina",]),
			18000:"America/Belize",]),
		     18000:"America/Guatemala",]),
		  18000:"America/Mexico_City",]),
	       18000:"America/Rankin_Inlet",]),
	    18000:"America/Rainy_River",]),
	 18000:
	 ([ "test":126691205, // 1974-01-06 08:00:05
	    21600:"America/Winnipeg",
	    18000:"America/Chicago",]),]),
      18000:
      ([ "test":9964805, // 1970-04-26 08:00:05
	 21600:
	 ([ "test":288770405, // 1979-02-25 06:00:05
	    21600:"America/Managua",
	    18000:"America/Costa_Rica",]),
	 18000:"America/Indiana/Knox",]),]),
   -41400:
   ([ "test":294323404, // 1979-04-30 12:30:04
      -41400:"Pacific/Norfolk",
      -43200:"Pacific/Nauru",]),
   13500:"America/Guyana",
   -34200:"Australia/Darwin",
   28800:
   ([ "test":452080795, // 1984-04-29 09:59:55
      32400:"America/Juneau",
      28800:
      ([ "test":9972004, // 1970-04-26 10:00:04
	 28800:
	 ([ "test":199274404, // 1976-04-25 10:00:04
	    25200:"America/Tijuana",
	    28800:"America/Whitehorse",]),
	 25200:
	 ([ "test":126698404, // 1974-01-06 10:00:04
	    28800:"America/Vancouver",
	    25200:"America/Los_Angeles",]),]),
      25200:"America/Dawson_Creek",
      21600:"America/Inuvik",]),
   -10800:
   ([ "test":646786804, // 1990-06-30 23:00:04
      -10800:
      ([ "test":909277204, // 1998-10-25 01:00:04
	 -10800:
	 ([ "test":670395594, // 1991-03-31 04:59:54
	    -10800:
	    ([ "test":670395604, // 1991-03-31 05:00:04
	       -10800:
	       ([ "test":-662688000, // 1949-01-01 00:00:00
		  -9000:"Africa/Mogadishu",
		  -11212:"Asia/Riyadh",
		  -10800:
		  ([ "test":-499824000, // 1954-03-01 00:00:00
		     -10800:
		     ([ "test":-1861920000, // 1911-01-01 00:00:00
			-10856:"Indian/Mayotte",
			-9320:
			({
			   "Africa/Asmera",
			   "Africa/Addis_Ababa",
			}),
			-10356:"Africa/Djibouti",
			-10384:"Indian/Comoro",]),
		     -14400:"Indian/Antananarivo",]),
		  -10848:"Asia/Aden",
		  -11516:"Asia/Kuwait",
		  0:"Antarctica/Syowa",
		  -9900:
		  ([ "test":-725846400, // 1947-01-01 00:00:00
		     -10800:"Africa/Dar_es_Salaam",
		     -9000:"Africa/Kampala",
		     -9900:"Africa/Nairobi",]),]),
	       -7200:"Europe/Tiraspol",]),
	    -7200:"Europe/Moscow",]),
	 -7200:
	 ([ "test":686102394, // 1991-09-28 23:59:54
	    -7200:
	    ([ "test":670374004, // 1991-03-30 23:00:04
	       -10800:"Europe/Zaporozhye",
	       -7200:"Europe/Kaliningrad",]),
	    -10800:
	    ([ "test":686102404, // 1991-09-29 00:00:04
	       -7200:
	       ([ "test":701820004, // 1992-03-28 22:00:04
		  -7200:"Europe/Riga",
		  -10800:"Europe/Minsk",]),
	       -10800:"Europe/Tallinn",]),]),
	 -3600:"Europe/Vilnius",]),
      -7200:
      ([ "test":796168805, // 1995-03-25 22:00:05
	 -7200:"Europe/Kiev",
	 -10800:"Europe/Chisinau",
	 -14400:"Europe/Simferopol",]),
      -14400:"Asia/Baghdad",
      -3600:"Europe/Uzhgorod",]),
   -27000:
   ({
      "Asia/Kuala_Lumpur",
      "Asia/Singapore",
   }),
   12600:
   ([ "test":465449405, // 1984-10-01 03:30:05
      10800:"America/Paramaribo",
      12600:"America/St_Johns",]),
   -43200:
   ([ "test":686671204, // 1991-10-05 14:00:04
      -43200:
      ([ "test":920123995, // 1999-02-27 13:59:55
	 -43200:
	 ([ "test":-31536000, // 1969-01-01 00:00:00
	    -43200:
	    ({
	       "Pacific/Tarawa",
	       "Pacific/Funafuti",
	       "Pacific/Wake",
	       "Pacific/Wallis",
	    }),
	    -39600:"Pacific/Majuro",]),
	 -46800:"Pacific/Fiji",
	 -39600:"Pacific/Kosrae",]),
      -46800:"Antarctica/McMurdo",
      -39600:"Asia/Kamchatka",
      39600:"Pacific/Auckland",]),
   -3600:
   ([ "test":433299604, // 1983-09-25 01:00:04
      -7200:"Europe/Tirane",
      0:
      ([ "test":717555604, // 1992-09-27 01:00:04
	 -3600:"Europe/Lisbon",
	 0:"Atlantic/Madeira",]),
      -3600:
      ([ "test":481078795, // 1985-03-31 00:59:55
	 -3600:
	 ([ "test":481078805, // 1985-03-31 01:00:05
	    -3600:
	    ([ "test":308703605, // 1979-10-13 23:00:05
	       -3600:
	       ([ "test":231202804, // 1977-04-29 23:00:04
		  -7200:"Africa/Tunis",
		  -3600:
		  ([ "test":-220924800, // 1963-01-01 00:00:00
		     -3600:
		     ([ "test":-347155200, // 1959-01-01 00:00:00
			-3600:
			([ "test":-1861920000, // 1911-01-01 00:00:00
			   -3668:"Africa/Brazzaville",
			   -2268:"Africa/Libreville",
			   -2328:"Africa/Douala",
			   -4460:"Africa/Bangui",
			   -3124:"Africa/Luanda",
			   -628:"Africa/Porto-Novo",
			   -816:"Africa/Lagos",
			   -3600:"Africa/Kinshasa",]),
			0:"Africa/Niamey",]),
		     0:"Africa/Malabo",]),]),
	       -7200:"Africa/Ndjamena",]),
	    -7200:
	    ([ "test":354675605, // 1981-03-29 01:00:05
	       -3600:
	       ([ "test":386125205, // 1982-03-28 01:00:05
		  -3600:
		  ([ "test":417574804, // 1983-03-27 01:00:04
		     -7200:"Europe/Belgrade",
		     -3600:"Europe/Andorra",]),
		  -7200:"Europe/Gibraltar",]),
	       -7200:
	       ([ "test":228877205, // 1977-04-03 01:00:05
		  -3600:
		  ([ "test":291776404, // 1979-04-01 01:00:04
		     -3600:
		     ([ "test":323830795, // 1980-04-06 00:59:55
			-3600:
			([ "test":323830805, // 1980-04-06 01:00:05
			   -3600:
			   ([ "test":-683856000, // 1948-05-01 00:00:00
			      -3600:
			      ([ "test":-917827200, // 1940-12-01 00:00:00
				 -7200:"Europe/Zurich",
				 -3600:"Europe/Vaduz",]),
			      -7200:"Europe/Vienna",]),
			   -7200:
			   ([ "test":12956404, // 1970-05-30 23:00:04
			      -3600:
			      ([ "test":-681177600, // 1948-06-01 00:00:00
				 -3600:
				 ([ "test":-788918400, // 1945-01-01 00:00:00
				    -3600:"Europe/Stockholm",
				    -7200:"Europe/Oslo",]),
				 -7200:"Europe/Copenhagen",]),
			      -7200:"Europe/Rome",]),]),
			-7200:
			([ "test":323827195, // 1980-04-05 23:59:55
			   -7200:"Europe/Malta",
			   -3600:"Europe/Budapest",]),]),
		     -7200:
		     ([ "test":-652320000, // 1949-05-01 00:00:00
			-3600:"Europe/Madrid",
			-7200:"Europe/Prague",]),]),
		  -7200:
		  ([ "test":-788918400, // 1945-01-01 00:00:00
		     -7200:"Europe/Amsterdam",
		     -3600:
		     ([ "test":-1283472000, // 1929-05-01 00:00:00
			-3600:"Europe/Luxembourg",
			0:"Europe/Brussels",]),]),]),]),]),
	 -7200:"Europe/Warsaw",]),]),]);
