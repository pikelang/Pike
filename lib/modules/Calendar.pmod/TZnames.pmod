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
                  "Pyongyang", "Qatar", "Qostanay", "Qyzylorda", "Riyadh",
                  "Sakhalin", "Samarkand", "Seoul", "Shanghai", "Singapore",
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
     "Africa/Casablanca",
     "Africa/El_Aaiun",
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
     "Africa/Casablanca",
     "Africa/El_Aaiun",
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
     "Asia/Qostanay",
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
     "Asia/Qostanay",
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
     "Asia/Qostanay",
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
     "Asia/Macau",
     "Asia/Makassar",
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
     "Pacific/Chuuk",
     "Pacific/Guam",
     "Pacific/Kosrae",
     "Pacific/Kwajalein",
     "Pacific/Majuro",
     "Pacific/Nauru",
     "Pacific/Palau",
     "Pacific/Pohnpei",
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
     "Asia/Macau",
     "Asia/Magadan",
     "Asia/Sakhalin",
     "Asia/Srednekolymsk",
     "Asia/Ust-Nera",
     "Asia/Vladivostok",
     "Asia/Yakutsk",
     "Etc/GMT-10",
     "Pacific/Bougainville",
     "Pacific/Chuuk",
     "Pacific/Kosrae",
     "Pacific/Kwajalein",
     "Pacific/Majuro",
     "Pacific/Pohnpei",
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
   "CT": ({
     "Asia/Macau",
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
   "GDT": ({
     "Pacific/Guam",
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
     "Pacific/Honolulu",
   }),
   "HST": ({
     "America/Adak",
     "HST",
     "Pacific/Honolulu",
     "Pacific/Johnston",
   }),
   "HWT": ({
     "America/Adak",
     "Pacific/Honolulu",
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
     "Asia/Manila",
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
     "Asia/Qostanay",
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
     "Asia/Manila",
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
     "Asia/Manila",
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
     "Africa/Ceuta",
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
     "Africa/Ceuta",
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
