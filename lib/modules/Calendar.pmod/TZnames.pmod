/* -*- mode: Pike; c-basic-offset: 3; -*- */

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
array(array(string))|zero zone_tab()
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
                  "Glace_Bay", "Goose_Bay", "Grand_Turk", "Grenada",
                  "Guadeloupe", "Guatemala", "Guayaquil", "Guyana", "Halifax",
                  "Havana", "Hermosillo", "Indiana/Indianapolis",
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
                  "North_Dakota/New_Salem", "Nuuk", "Ojinaga", "Panama",
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
                  "Honolulu", "Johnston", "Kanton", "Kiritimati", "Kosrae",
                  "Kwajalein", "Majuro", "Marquesas", "Midway", "Nauru",
                  "Niue", "Norfolk", "Noumea", "Pago_Pago", "Palau",
                  "Pitcairn", "Pohnpei", "Port_Moresby", "Rarotonga",
                  "Saipan", "Tahiti", "Tarawa", "Tongatapu", "Wake",
                  "Wallis"}),
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
                  "Kaliningrad", "Kirov", "Kyiv", "Lisbon", "Ljubljana",
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
   "+0030": ({
     "Africa/Accra",
     "Africa/Lagos",
   }),
   "+01": ({
     "Africa/Casablanca",
     "Africa/El_Aaiun",
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
     "Asia/Bahrain",
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
     "Pacific/Norfolk",
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
     "Pacific/Fakaofo",
     "Pacific/Fiji",
     "Pacific/Kanton",
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
     "Pacific/Enderbury",
     "Pacific/Kanton",
   }),
   "-0040": ({
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
     "America/Maceio",
     "America/Miquelon",
     "America/Montevideo",
     "America/Noronha",
     "America/Nuuk",
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
     "America/Guyana",
     "America/Maceio",
     "America/Manaus",
     "America/Miquelon",
     "America/Montevideo",
     "America/Nuuk",
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
     "America/Barbados",
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
     "Pacific/Fakaofo",
     "Pacific/Kanton",
     "Pacific/Midway",
     "Pacific/Niue",
   }),
   "-1120": ({
     "Pacific/Niue",
   }),
   "-1130": ({
     "Pacific/Apia",
   }),
   "-12": ({
     "Etc/GMT+12",
     "Pacific/Enderbury",
     "Pacific/Kanton",
     "Pacific/Kwajalein",
   }),
   "A%sT": ({
     "America/Blanc-Sablon",
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
   "AT": ({
     "America/Barbados",
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
     "America/Bogota",
     "Asia/Baghdad",
     "Asia/Bangkok",
     "Asia/Jakarta",
     "Atlantic/Bermuda",
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
     "Atlantic/Bermuda",
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
   "C%sT": ({
     "America/Atikokan",
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
     "Europe/Amsterdam",
     "Europe/Copenhagen",
     "Europe/Guernsey",
     "Europe/Jersey",
     "Europe/Ljubljana",
     "Europe/Luxembourg",
     "Europe/Monaco",
     "Europe/Oslo",
     "Europe/Sarajevo",
     "Europe/Skopje",
     "Europe/Stockholm",
     "Europe/Tiraspol",
     "Europe/Vaduz",
     "Europe/Zagreb",
   }),
   "CEMT": ({
     "Europe/Berlin",
     "Europe/Madrid",
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
     "Europe/Kyiv",
     "Europe/Lisbon",
     "Europe/Ljubljana",
     "Europe/Luxembourg",
     "Europe/Madrid",
     "Europe/Malta",
     "Europe/Minsk",
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
     "Europe/Kyiv",
     "Europe/Lisbon",
     "Europe/Ljubljana",
     "Europe/Luxembourg",
     "Europe/Madrid",
     "Europe/Malta",
     "Europe/Minsk",
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
     "America/Belize",
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
     "America/Bahia_Banderas",
     "America/Belize",
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
     "America/Nassau",
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
     "Europe/Kyiv",
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
     "Europe/Kyiv",
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
     "Africa/Lagos",
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
   "HKWT": ({
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
     "Europe/Kyiv",
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
     "America/Nuuk",
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
     "Europe/Kirov",
     "Europe/Kyiv",
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
     "Europe/Kyiv",
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
     "Europe/Kyiv",
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
     "America/Dawson",
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
     "America/Whitehorse",
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
     "Africa/Lubumbashi",
     "Africa/Malabo",
     "Africa/Ndjamena",
     "Africa/Niamey",
     "Africa/Porto-Novo",
     "Africa/Sao_Tome",
     "Africa/Windhoek",
   }),
   "WE%sT": ({
     "Europe/Luxembourg",
     "Europe/Monaco",
   }),
   "WEMT": ({
     "Africa/Ceuta",
     "Atlantic/Madeira",
     "Europe/Lisbon",
     "Europe/Madrid",
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
   "ZMT": ({
     "Africa/Blantyre",
   }),
]);

// this is used by the timezone expert system,
// that uses localtime (or whatever) to figure out the *correct* timezone

// note that at least Red Hat 6.2 has an error in the time database,
// so a lot of Europeean (Paris, Brussels) times get to be Luxembourg /Mirar

mapping timezone_expert_tree =
([ "test":60404400, // 1971-12-01 03:00:00
   -46800:
      ([ "test":528381000, // 1986-09-29 12:30:00
         -46800:"Pacific/Tongatapu",
         -43200:"Asia/Anadyr",
      ]),
   -45900:"Pacific/Chatham",
   -43200:
      ([ "test":1092402000, // 2004-08-13 13:00:00
         -46800:"Asia/Kamchatka",
         -43200:
            ([ "test":-1726831672, // 1915-04-13 12:32:08
               -43200:
                  ([ "test":-2177496366, // 1900-12-31 11:53:54
                     -43200:"Pacific/Wallis",
                     -43012:"Pacific/Funafuti",
                     -41524:"Pacific/Tarawa",
                     -39988:"Pacific/Wake",
                  ]),
               -42944:"Pacific/Fiji",
               -41400:"Pacific/Auckland",
               -32400:"Pacific/Majuro",
            ]),
         -39600:"Pacific/Kosrae",
      ]),
   -41400:
      ([ "test":157473000, // 1974-12-28 14:30:00
         -45000:"Pacific/Norfolk",
         -41400:"Pacific/Nauru",
      ]),
   -39600:
      ([ "test":702867600, // 1992-04-10 01:00:00
         -43200:
            ([ "test":1079182800, // 2004-03-13 13:00:00
               -39600:
                  ([ "test":1437836400, // 2015-07-25 15:00:00
                     -39600:"Asia/Srednekolymsk",
                     -36000:"Asia/Magadan",
                  ]),
               -36000:"Asia/Sakhalin",
            ]),
         -39600:
            ([ "test":-2003439184, // 1906-07-08 01:06:56
               -40396:"Pacific/Efate",
               -39948:"Pacific/Noumea",
               -39600:"Pacific/Pohnpei",
               -38388:"Pacific/Guadalcanal",
            ]),
         -36000:
            ([ "test":-31543200, // 1968-12-31 22:00:00
               -39600:
                  ([ "test":1278072000, // 2010-07-02 12:00:00
                     -39600:"Antarctica/Macquarie",
                     -36000:"Australia/Hobart",
                  ]),
               -36000:
                  ([ "test":725763600, // 1992-12-31 01:00:00
                     -39600:
                        ([ "test":-2365452024, // 1895-01-16 01:59:36
                           -36292:"Australia/Sydney",
                           -36000:"Australia/Lindeman",
                           -34792:"Australia/Melbourne",
                        ]),
                     -36000:"Australia/Brisbane",
                  ]),
            ]),
      ]),
   -37800:
      ([ "test":-2339530630, // 1895-11-12 02:22:50
         -36000:"Australia/Broken_Hill",
         -32400:"Australia/Adelaide",
      ]),
   -36000:
      ([ "test":420639300, // 1983-05-01 12:15:00
         -39600:"Asia/Vladivostok",
         -37800:"Australia/Lord_Howe",
         -36000:
            ([ "test":-491245200, // 1954-06-08 07:00:00
               -36000:
                  ([ "test":1435474800, // 2015-06-28 07:00:00
                     -39600:"Pacific/Bougainville",
                     -36000:
                        ([ "test":-786445200, // 1945-01-29 15:00:00
                           -36000:
                              ([ "test":-843903000, // 1943-04-05 14:30:00
                                 -36000:"Pacific/Port_Moresby",
                                 -32400:"Pacific/Guam",
                              ]),
                           -32400:"Pacific/Chuuk",
                        ]),
                  ]),
               0:"Antarctica/DumontDUrville",
            ]),
      ]),
   -34200:"Australia/Darwin",
   -32400:
      ([ "test":512654400, // 1986-03-31 12:00:00
         -43200:"Asia/Ust-Nera",
         -36000:
            ([ "test":1187051400, // 2007-08-14 00:30:00
               -39600:"Asia/Khandyga",
               -36000:
                  ([ "test":1436634000, // 2015-07-11 17:00:00
                     -32400:"Asia/Yakutsk",
                     -28800:"Asia/Chita",
                  ]),
            ]),
         -32400:
            ([ "test":1482505200, // 2016-12-23 15:00:00
               -32400:
                  ([ "test":-381467700, // 1957-11-29 20:45:00
                     -34200:"Asia/Jayapura",
                     -32400:
                        ([ "test":-2382598738, // 1894-07-01 15:01:02
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
      ([ "test":1324791000, // 2011-12-25 05:30:00
         -39600:"Antarctica/Casey",
         -32400:"Asia/Irkutsk",
         -28800:
            ([ "test":-2117087397, // 1902-11-30 16:10:03
               -28800:
                  ([ "test":-708089400, // 1947-07-25 12:30:00
                     -32400:
                        ([ "test":-1597651372, // 1919-05-17 15:57:08
                           -32400:"Asia/Shanghai",
                           -28800:"Asia/Taipei",
                        ]),
                     -28800:
                        ([ "test":-815315400, // 1944-03-01 11:30:00
                           -32400:"Asia/Manila",
                           -28800:"Australia/Perth",
                        ]),
                  ]),
               -28656:"Asia/Makassar",
               -27580:"Asia/Brunei",
               -27402:"Asia/Hong_Kong",
               -27250:"Asia/Macau",
               -26480:"Asia/Kuching",
            ]),
         -25200:
            ([ "test":369892800, // 1981-09-21 04:00:00
               -28800:"Asia/Pontianak",
               -25200:"Asia/Ho_Chi_Minh",
            ]),
      ]),
   -27000:
      ([ "test":-2177477466, // 1900-12-31 17:08:54
         -24925:"Asia/Singapore",
         -24406:"Asia/Kuala_Lumpur",
      ]),
   -25200:
      ([ "test":544167000, // 1987-03-31 05:30:00
         -36000:"Asia/Choibalsan",
         -32400:"Asia/Ulaanbaatar",
         -28800:
            ([ "test":910949400, // 1998-11-13 09:30:00
               -25200:
                  ([ "test":1144983600, // 2006-04-14 03:00:00
                     -28800:
                        ([ "test":1285441200, // 2010-09-25 19:00:00
                           -28800:"Asia/Krasnoyarsk",
                           -25200:"Asia/Novokuznetsk",
                        ]),
                     -25200:"Asia/Tomsk",
                  ]),
               -21600:
                  ([ "test":769896000, // 1994-05-25 20:00:00
                     -28800:"Asia/Barnaul",
                     -25200:"Asia/Novosibirsk",
                  ]),
            ]),
         -25200:
            ([ "test":1262028600, // 2009-12-28 19:30:00
               -25200:
                  ([ "test":-1927825497, // 1908-11-29 04:55:03
                     -25632:"Asia/Jakarta",
                     -25590:"Asia/Hanoi",
                     -25200:"Indian/Christmas",
                     -24124:"Asia/Bangkok",
                  ]),
               -18000:"Antarctica/Davis",
            ]),
      ]),
   -23400:
      ([ "test":-825839100, // 1943-10-31 16:15:00
         -32400:"Asia/Yangon",
         -23400:"Indian/Cocos",
      ]),
   -21600:
      ([ "test":897467400, // 1998-06-10 08:30:00
         -28800:"Asia/Hovd",
         -25200:
            ([ "test":1357716600, // 2013-01-09 07:30:00
               -25200:"Asia/Omsk",
               -21600:"Asia/Almaty",
            ]),
         -21600:
            ([ "test":-440640000, // 1956-01-15 00:00:00
               -21600:
                  ([ "test":-538672500, // 1952-12-06 08:45:00
                     -21600:
                        ([ "test":-719388000, // 1947-03-16 18:00:00
                           -23400:"Asia/Dhaka",
                           -21600:
                              ([ "test":-1286515510, // 1929-03-26 18:34:50
                                 -21600:"Asia/Urumqi",
                                 -18000:"Asia/Bishkek",
                              ]),
                        ]),
                     0:"Antarctica/Mawson",
                  ]),
               0:"Antarctica/Vostok",
            ]),
         -18000:
            ([ "test":670384800, // 1991-03-31 02:00:00
               -25200:"Asia/Tashkent",
               -21600:"Asia/Dushanbe",
            ]),
      ]),
   -19800:
      ([ "test":839614500, // 1996-08-09 18:15:00
         -23400:"Asia/Colombo",
         -21600:"Asia/Thimphu",
         -20700:"Asia/Kathmandu",
         -19800:"Asia/Kolkata",
      ]),
   -18000:
      ([ "test":695770200, // 1992-01-18 21:30:00
         -21600:"Asia/Qyzylorda",
         -18000:
            ([ "test":362817000, // 1981-07-01 06:30:00
               -21600:"Asia/Samarkand",
               -18000:
                  ([ "test":836215200, // 1996-07-01 10:00:00
                     -21600:"Indian/Chagos",
                     -18000:
                        ([ "test":-603643500, // 1950-11-15 09:15:00
                           -19800:"Asia/Karachi",
                           -18000:"Indian/Kerguelen",
                           -17640:"Indian/Maldives",
                        ]),
                  ]),
            ]),
         -14400:
            ([ "test":370722600, // 1981-09-30 18:30:00
               -21600:
                  ([ "test":638614800, // 1990-03-28 09:00:00
                     -21600:
                        ([ "test":1114948800, // 2005-05-01 12:00:00
                           -21600:"Asia/Qostanay",
                           -18000:"Asia/Aqtobe",
                        ]),
                     -18000:"Asia/Oral",
                  ]),
               -18000:
                  ([ "test":378585000, // 1981-12-30 18:30:00
                     -21600:
                        ([ "test":851504400, // 1996-12-25 09:00:00
                           -18000:"Asia/Atyrau",
                           -14400:"Asia/Aqtau",
                        ]),
                     -18000:
                        ([ "test":670384800, // 1991-03-31 02:00:00
                           -21600:"Asia/Ashgabat",
                           -18000:"Asia/Yekaterinburg",
                        ]),
                  ]),
            ]),
      ]),
   -16200:"Asia/Kabul",
   -14400:
      ([ "test":848104200, // 1996-11-16 00:30:00
         -18000:"Asia/Tbilisi",
         -14400:
            ([ "test":670372200, // 1991-03-30 22:30:00
               -14400:
                  ([ "test":-1988163954, // 1906-12-31 20:14:06
                     -14400:"Indian/Mauritius",
                     -13312:"Indian/Reunion",
                     -13308:"Indian/Mahe",
                     -13272:"Asia/Dubai",
                  ]),
               -10800:
                  ([ "test":638618400, // 1990-03-28 10:00:00
                     -18000:
                        ([ "test":763608600, // 1994-03-14 01:30:00
                           -14400:"Asia/Baku",
                           -10800:"Asia/Yerevan",
                        ]),
                     -14400:"Europe/Samara",
                  ]),
            ]),
         -10800:
            ([ "test":1510743600, // 2017-11-15 11:00:00
               -14400:
                  ([ "test":683076600, // 1991-08-24 23:30:00
                     -14400:
                        ([ "test":1469919600, // 2016-07-30 23:00:00
                           -14400:"Europe/Astrakhan",
                           -10800:"Europe/Saratov",
                        ]),
                     -10800:"Europe/Ulyanovsk",
                  ]),
               -10800:
                  ([ "test":325803600, // 1980-04-28 21:00:00
                     -14400:
                        ([ "test":1574850600, // 2019-11-27 10:30:00
                           -14400:"Europe/Volgograd",
                           -10800:"Europe/Kirov",
                        ]),
                     -10800:"Asia/Qatar",
                  ]),
            ]),
      ]),
   -12600:"Asia/Tehran",
   -10800:
      ([ "test":646799400, // 1990-07-01 02:30:00
         -14400:
            ([ "test":1357727400, // 2013-01-09 10:30:00
               -14400:"Europe/Moscow",
               -10800:"Asia/Baghdad",
               -7200:
                  ([ "test":-825894000, // 1943-10-31 01:00:00
                     -10800:"Europe/Zaporozhye",
                     -3600:"Europe/Kiev",
                  ]),
            ]),
         -10800:
            ([ "test":626880600, // 1989-11-12 13:30:00
               -10800:
                  ([ "test":1076634000, // 2004-02-13 01:00:00
                     -10800:
                        ([ "test":-792471356, // 1944-11-20 21:04:04
                           -11212:"Asia/Riyadh",
                           -10800:"Africa/Nairobi",
                           0:"Antarctica/Syowa",
                        ]),
                     -7200:
                        ([ "test":656173800, // 1990-10-17 14:30:00
                           -10800:"Europe/Minsk",
                           -7200:"Europe/Chisinau",
                        ]),
                  ]),
               -7200:
                  ([ "test":996314400, // 2001-07-28 10:00:00
                     -10800:
                        ([ "test":965088000, // 2000-08-01 00:00:00
                           -10800:"Europe/Kaliningrad",
                           -7200:"Europe/Riga",
                        ]),
                     -7200:
                        ([ "test":898783200, // 1998-06-25 14:00:00
                           -10800:"Europe/Tallinn",
                           -7200:"Europe/Vilnius",
                        ]),
                  ]),
            ]),
         -7200:"Europe/Simferopol",
         -3600:"Europe/Uzhgorod",
      ]),
   -7200:
      ([ "test":639876600, // 1990-04-11 23:30:00
         -10800:
            ([ "test":1301230800, // 2011-03-27 13:00:00
               -10800:
                  ([ "test":-2015372062, // 1906-02-19 22:25:38
                     -8148:"Asia/Famagusta",
                     -8008:"Asia/Nicosia",
                     -7200:"Europe/Sofia",
                     -6264:"Europe/Bucharest",
                     -5989:"Europe/Helsinki",
                     -5692:"Europe/Athens",
                  ]),
               -7200:
                  ([ "test":1285414230, // 2010-09-25 11:30:30
                     -10800:
                        ([ "test":368033400, // 1981-08-30 15:30:00
                           -10800:"Europe/Istanbul",
                           -7200:"Asia/Damascus",
                        ]),
                     -7200:
                        ([ "test":1220099400, // 2008-08-30 12:30:00
                           -10800:
                              ([ "test":-1913206832, // 1909-05-17 09:39:28
                                 -8440:"Asia/Jerusalem",
                                 -7200:"Asia/Hebron",
                              ]),
                           -7200:"Asia/Gaza",
                        ]),
                  ]),
            ]),
         -7200:
            ([ "test":1560805200, // 2019-06-17 21:00:00
               -10800:
                  ([ "test":1627905600, // 2021-08-02 12:00:00
                     -10800:
                        ([ "test":-2035462972, // 1905-07-02 09:37:08
                           -8624:"Asia/Amman",
                           -7200:"Asia/Beirut",
                        ]),
                     -7200:"Africa/Juba",
                  ]),
               -7200:
                  ([ "test":1228714200, // 2008-12-08 05:30:00
                     -10800:"Africa/Khartoum",
                     -7200:
                        ([ "test":653749200, // 1990-09-19 13:00:00
                           -10800:"Africa/Cairo",
                           -7200:
                              ([ "test":-853115400, // 1942-12-19 23:30:00
                                 -10800:
                                    ([ "test":-2458171812, // 1892-02-07 22:29:48
                                       -5400:"Africa/Johannesburg",
                                       -4104:"Africa/Windhoek",
                                    ]),
                                 -7200:"Africa/Maputo",
                              ]),
                        ]),
                  ]),
            ]),
         -3600:"Africa/Tripoli",
      ]),
   -3600:
      ([ "test":312116400, // 1979-11-22 11:00:00
         -7200:"Africa/Ndjamena",
         -3600:
            ([ "test":299764800, // 1979-07-02 12:00:00
               -7200:
                  ([ "test":402327000, // 1982-10-01 13:30:00
                     -7200:"Europe/Tirane",
                     -3600:
                        ([ "test":-2123583138, // 1902-09-16 11:47:42
                           -5040:"Europe/Warsaw",
                           -3600:
                              ([ "test":-724888800, // 1947-01-12 02:00:00
                                 -7200:"Europe/Prague",
                                 -3600:
                                    ([ "test":-2403521942, // 1893-11-01 11:00:58
                                       -3600:"Europe/Rome",
                                       -3484:"Europe/Malta",
                                    ]),
                              ]),
                           -1476:"Europe/Luxembourg",
                           -1172:"Europe/Amsterdam",
                           -561:
                              ([ "test":-1855181361, // 1911-03-19 23:50:39
                                 -561:"Europe/Monaco",
                                 0:"Europe/Paris",
                              ]),
                           0:
                              ([ "test":-1653764400, // 1917-08-06 05:00:00
                                 -7200:"Europe/Brussels",
                                 0:"Europe/Madrid",
                              ]),
                        ]),
                  ]),
               -3600:
                  ([ "test":331347600, // 1980-07-02 01:00:00
                     -7200:
                        ([ "test":-766967400, // 1945-09-12 01:30:00
                           -10800:"Europe/Berlin",
                           -7200:
                              ([ "test":-232754400, // 1962-08-17 02:00:00
                                 -7200:"Europe/Oslo",
                                 -3600:"Europe/Budapest",
                              ]),
                           -3600:
                              ([ "test":-780616800, // 1945-04-07 02:00:00
                                 -7200:
                                    ([ "test":-2410174671, // 1893-08-16 11:02:09
                                       -3600:"Europe/Vienna",
                                       -3020:"Europe/Copenhagen",
                                    ]),
                                 -3600:"Europe/Stockholm",
                              ]),
                        ]),
                     -3600:
                        ([ "test":392943600, // 1982-06-14 23:00:00
                           -7200:
                              ([ "test":362923200, // 1981-07-02 12:00:00
                                 -7200:"Europe/Zurich",
                                 -3600:"Europe/Gibraltar",
                              ]),
                           -3600:
                              ([ "test":-1898424281, // 1909-11-04 11:55:19
                                 -3600:"Europe/Belgrade",
                                 -815:"Africa/Lagos",
                                 -561:"Africa/Tunis",
                                 0:"Europe/Andorra",
                              ]),
                        ]),
                  ]),
            ]),
         0:"Europe/Lisbon",
      ]),
   0:
      ([ "test":964310400, // 2000-07-23 00:00:00
         -7200:"Africa/Ceuta",
         -3600:
            ([ "test":277993800, // 1978-10-23 12:30:00
               -3600:
                  ([ "test":10243800, // 1970-04-29 13:30:00
                     -3600:
                        ([ "test":-709466400, // 1947-07-09 14:00:00
                           -7200:"Europe/London",
                           -3600:"Europe/Dublin",
                        ]),
                     0:"Africa/Algiers",
                  ]),
               0:
                  ([ "test":331389000, // 1980-07-02 12:30:00
                     -3600:
                        ([ "test":-773888400, // 1945-06-23 23:00:00
                           -3600:"Atlantic/Madeira",
                           3600:"Atlantic/Canary",
                        ]),
                     0:"Atlantic/Faroe",
                  ]),
            ]),
         0:
            ([ "test":1311467400, // 2011-07-24 00:30:00
               -7200:"Antarctica/Troll",
               -3600:"Africa/Casablanca",
               0:
                  ([ "test":1530536400, // 2018-07-02 13:00:00
                     -3600:"Africa/Sao_Tome",
                     0:
                        ([ "test":-818640900, // 1944-01-22 23:45:00
                           -1800:"Africa/Accra",
                           0:"Africa/Abidjan",
                           3600:"Atlantic/Reykjavik",
                        ]),
                  ]),
            ]),
         10800:"Antarctica/Rothera",
      ]),
   2670:"Africa/Monrovia",
   3600:
      ([ "test":178030800, // 1975-08-23 13:00:00
         0:"Africa/Bissau",
         3600:
            ([ "test":315797400, // 1980-01-04 01:30:00
               0:"Africa/El_Aaiun",
               3600:"Atlantic/Azores",
            ]),
      ]),
   7200:
      ([ "test":955110600, // 2000-04-07 12:30:00
         0:"America/Scoresbysund",
         3600:"Atlantic/Cape_Verde",
         7200:
            ([ "test":-2145865326, // 1902-01-01 14:17:54
               7200:"Atlantic/South_Georgia",
               7780:"America/Noronha",
            ]),
      ]),
   10800:
      ([ "test":1086579000, // 2004-06-07 03:30:00
         0:"America/Danmarkshavn",
         7200:"America/Nuuk",
         10800:
            ([ "test":129158100, // 1974-02-03 21:15:00
               5400:"America/Montevideo",
               7200:
                  ([ "test":687326400, // 1991-10-13 04:00:00
                     7200:"America/Argentina/Jujuy",
                     10800:"America/Argentina/Buenos_Aires",
                     14400:
                        ([ "test":-2372096446, // 1894-10-31 04:19:14
                           15408:"America/Argentina/Cordoba",
                           15700:"America/Argentina/Salta",
                        ]),
                  ]),
               10800:
                  ([ "test":971863200, // 2000-10-18 10:00:00
                     7200:
                        ([ "test":-192414600, // 1963-11-26 23:30:00
                           7200:"America/Sao_Paulo",
                           10800:
                              ([ "test":-1767216694, // 1914-01-01 02:28:26
                                 9240:"America/Fortaleza",
                                 9244:"America/Bahia",
                                 10800:"America/Maceio",
                                 11568:"America/Araguaina",
                              ]),
                        ]),
                     10800:
                        ([ "test":-919153382, // 1940-11-15 15:36:58
                           10800:
                              ([ "test":-1767215594, // 1914-01-01 02:46:46
                                 10800:"America/Recife",
                                 11636:"America/Belem",
                              ]),
                           14400:"America/Cayenne",
                        ]),
                  ]),
            ]),
         14400:
            ([ "test":667872000, // 1991-03-02 00:00:00
               7200:
                  ([ "test":677939400, // 1991-06-26 12:30:00
                     10800:
                        ([ "test":1085972400, // 2004-05-31 03:00:00
                           10800:"America/Argentina/Rio_Gallegos",
                           14400:"America/Argentina/Ushuaia",
                        ]),
                     14400:
                        ([ "test":1087401600, // 2004-06-16 16:00:00
                           10800:"America/Argentina/Tucuman",
                           14400:"America/Argentina/Catamarca",
                        ]),
                  ]),
               10800:
                  ([ "test":1496577600, // 2017-06-04 12:00:00
                     10800:
                        ([ "test":-435708000, // 1956-03-12 02:00:00
                           0:"Antarctica/Palmer",
                           14400:"America/Punta_Arenas",
                        ]),
                     14400:"America/Santiago",
                  ]),
               14400:
                  ([ "test":636939000, // 1990-03-08 23:30:00
                     7200:"America/Argentina/San_Luis",
                     10800:
                        ([ "test":1086015600, // 2004-05-31 15:00:00
                           10800:"America/Argentina/La_Rioja",
                           14400:"America/Argentina/San_Juan",
                        ]),
                     14400:"America/Argentina/Mendoza",
                  ]),
            ]),
      ]),
   12600:
      ([ "test":892773000, // 1998-04-17 00:30:00
         9000:"America/St_Johns",
         10800:"America/Paramaribo",
      ]),
   13500:"America/Guyana",
   14400:
      ([ "test":972162000, // 2000-10-21 21:00:00
         7200:"America/Miquelon",
         10800:
            ([ "test":304934400, // 1979-08-31 08:00:00
               10800:
                  ([ "test":110448000, // 1973-07-02 08:00:00
                     10800:
                        ([ "test":-646933500, // 1949-07-02 08:15:00
                           9000:"America/Goose_Bay",
                           10800:"America/Halifax",
                           14400:"America/Glace_Bay",
                        ]),
                     14400:
                        ([ "test":-331358400, // 1959-07-02 20:00:00
                           10800:"America/Moncton",
                           14400:"Atlantic/Bermuda",
                        ]),
                  ]),
               14400:
                  ([ "test":779963400, // 1994-09-19 08:30:00
                     10800:
                        ([ "test":458080200, // 1984-07-07 20:30:00
                           10800:"Atlantic/Stanley",
                           14400:"America/Thule",
                        ]),
                     14400:
                        ([ "test":110388600, // 1973-07-01 15:30:00
                           10800:"America/Asuncion",
                           14400:
                              ([ "test":-1767212316, // 1914-01-01 03:41:24
                                 13460:"America/Cuiaba",
                                 14400:"America/Campo_Grande",
                              ]),
                        ]),
                  ]),
            ]),
         14400:
            ([ "test":1338183000, // 2012-05-28 05:30:00
               10800:"America/Santarem",
               14400:
                  ([ "test":331399800, // 1980-07-02 15:30:00
                     10800:
                        ([ "test":-1325448091, // 1928-01-01 03:58:29
                           14309:"America/Barbados",
                           14400:"America/Martinique",
                        ]),
                     14400:
                        ([ "test":-1825918745, // 1912-02-21 16:20:55
                           14400:
                              ([ "test":-2473466014, // 1891-08-14 22:06:26
                                 14400:"America/Blanc-Sablon",
                                 15865:"America/Puerto_Rico",
                              ]),
                           14404:"America/Manaus",
                           14560:"America/Boa_Vista",
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
      ([ "test":1158759000, // 2006-09-20 13:30:00
         14400:
            ([ "test":110462400, // 1973-07-02 12:00:00
               14400:
                  ([ "test":162781200, // 1975-02-28 01:00:00
                     14400:
                        ([ "test":-47329200, // 1968-07-02 05:00:00
                           14400:
                              ([ "test":-78926400, // 1967-07-02 12:00:00
                                 14400:"America/New_York",
                                 18000:"America/Kentucky/Louisville",
                              ]),
                           18000:"America/Indiana/Marengo",
                        ]),
                     18000:
                        ([ "test":-1938148750, // 1908-08-01 17:20:50
                           18000:"America/Toronto",
                           18570:"America/Nassau",
                           19776:"America/Havana",
                           21600:"America/Detroit",
                        ]),
                  ]),
               18000:
                  ([ "test":15750000, // 1970-07-02 07:00:00
                     14400:
                        ([ "test":-15768000, // 1969-07-02 12:00:00
                           14400:
                              ([ "test":-377713800, // 1958-01-12 07:30:00
                                 18000:"America/Indiana/Vevay",
                                 21600:"America/Indiana/Indianapolis",
                              ]),
                           18000:"America/Thunder_Bay",
                        ]),
                     18000:
                        ([ "test":957043800, // 2000-04-29 21:30:00
                           14400:
                              ([ "test":-901753200, // 1941-06-05 01:00:00
                                 14400:"America/Nipigon",
                                 18000:"America/Grand_Turk",
                              ]),
                           18000:
                              ([ "test":-1267889730, // 1929-10-28 08:24:30
                                 0:"America/Iqaluit",
                                 18000:"America/Port-au-Prince",
                              ]),
                        ]),
                  ]),
            ]),
         18000:
            ([ "test":1299169800, // 2011-03-03 16:30:00
               14400:
                  ([ "test":-1767209080, // 1914-01-01 04:35:20
                     16768:"America/Eirunepe",
                     18000:"America/Rio_Branco",
                  ]),
               18000:
                  ([ "test":1209902400, // 2008-05-04 12:00:00
                     14400:
                        ([ "test":1183840200, // 2007-07-07 20:30:00
                           14400:"America/Indiana/Winamac",
                           18000:"America/Indiana/Vincennes",
                        ]),
                     18000:
                        ([ "test":-1942728354, // 1908-06-09 17:14:06
                           17776:"America/Bogota",
                           18000:"America/Panama",
                           18430:"America/Jamaica",
                           18516:"America/Lima",
                           18840:"America/Guayaquil",
                           21600:"America/Atikokan",
                        ]),
                  ]),
               21600:
                  ([ "test":-45095400, // 1968-07-28 01:30:00
                     18000:"America/Indiana/Tell_City",
                     21600:"America/Menominee",
                  ]),
            ]),
         21600:"Pacific/Galapagos",
      ]),
   21600:
      ([ "test":1342508400, // 2012-07-17 07:00:00
         14400:
            ([ "test":1058364000, // 2003-07-16 14:00:00
               14400:"America/Kentucky/Monticello",
               18000:"America/Indiana/Petersburg",
            ]),
         18000:
            ([ "test":141609600, // 1974-06-28 00:00:00
               18000:
                  ([ "test":-144891000, // 1965-05-30 00:30:00
                     18000:"America/Chicago",
                     21600:
                        ([ "test":-218673000, // 1963-01-27 01:30:00
                           18000:"America/Indiana/Knox",
                           21600:
                              ([ "test":-901753200, // 1941-06-05 01:00:00
                                 18000:"America/Rainy_River",
                                 21600:"America/Winnipeg",
                              ]),
                        ]),
                  ]),
               21600:
                  ([ "test":392794200, // 1982-06-13 05:30:00
                     18000:
                        ([ "test":979439400, // 2001-01-14 02:30:00
                           18000:
                              ([ "test":1167834600, // 2007-01-03 14:30:00
                                 18000:"America/Resolute",
                                 21600:"America/Rankin_Inlet",
                              ]),
                           21600:
                              ([ "test":654831000, // 1990-10-02 01:30:00
                                 18000:"America/Cancun",
                                 21600:"America/Merida",
                              ]),
                        ]),
                     21600:
                        ([ "test":583804800, // 1988-07-02 00:00:00
                           18000:
                              ([ "test":-1530522000, // 1921-07-02 15:00:00
                                 24000:"America/Matamoros",
                                 24076:"America/Monterrey",
                              ]),
                           21600:"America/Mexico_City",
                        ]),
                  ]),
            ]),
         21600:
            ([ "test":1076844600, // 2004-02-15 11:30:00
               18000:"Pacific/Easter",
               21600:
                  ([ "test":133421400, // 1974-03-25 05:30:00
                     18000:"America/Managua",
                     21600:
                        ([ "test":-1581660034, // 1919-11-18 17:59:26
                           19800:"America/Belize",
                           20173:"America/Costa_Rica",
                           20932:"America/Tegucigalpa",
                           21408:"America/El_Salvador",
                           21600:"America/Guatemala",
                           25200:"America/Regina",
                        ]),
                  ]),
               25200:
                  ([ "test":-1530518400, // 1921-07-02 16:00:00
                     25060:"America/Ojinaga",
                     25460:"America/Chihuahua",
                  ]),
            ]),
      ]),
   25200:
      ([ "test":1304852400, // 2011-05-08 11:00:00
         18000:
            ([ "test":1168716600, // 2007-01-13 19:30:00
               21600:
                  ([ "test":893534400, // 1998-04-25 20:00:00
                     18000:"America/North_Dakota/Center",
                     21600:"America/North_Dakota/New_Salem",
                  ]),
               25200:
                  ([ "test":-330755400, // 1959-07-09 19:30:00
                     25200:"America/North_Dakota/Beulah",
                     28800:"America/Bahia_Banderas",
                  ]),
            ]),
         21600:
            ([ "test":-47345400, // 1968-07-02 00:30:00
               21600:
                  ([ "test":127659600, // 1974-01-17 13:00:00
                     21600:"America/Denver",
                     25200:"America/Boise",
                  ]),
               25200:
                  ([ "test":973083600, // 2000-11-01 13:00:00
                     18000:"America/Cambridge_Bay",
                     21600:"America/Swift_Current",
                     25200:
                        ([ "test":-1551600784, // 1920-10-31 15:46:56
                           0:"America/Yellowknife",
                           25200:"America/Edmonton",
                        ]),
                  ]),
               28800:"America/Mazatlan",
            ]),
         25200:
            ([ "test":-75470400, // 1967-08-11 12:00:00
               21600:"America/Phoenix",
               25200:"America/Creston",
               28800:"America/Hermosillo",
            ]),
      ]),
   28800:
      ([ "test":1576071000, // 2019-12-11 13:30:00
         25200:
            ([ "test":187853400, // 1975-12-15 05:30:00
               25200:"America/Dawson_Creek",
               28800:
                  ([ "test":425968200, // 1983-07-02 04:30:00
                     21600:"America/Inuvik",
                     25200:"America/Fort_Nelson",
                  ]),
            ]),
         28800:
            ([ "test":53652600, // 1971-09-13 23:30:00
               25200:
                  ([ "test":-2715760426, // 1883-12-10 14:06:14
                     28800:"America/Los_Angeles",
                     29548:"America/Vancouver",
                  ]),
               28800:
                  ([ "test":-183025800, // 1964-03-14 15:30:00
                     28800:"America/Tijuana",
                     32400:"America/Whitehorse",
                  ]),
            ]),
         32400:
            ([ "test":333482400, // 1980-07-26 18:00:00
               25200:
                  ([ "test":437634000, // 1983-11-14 05:00:00
                     28800:"America/Metlakatla",
                     32400:"America/Sitka",
                  ]),
               28800:"America/Juneau",
            ]),
      ]),
   30600:"Pacific/Pitcairn",
   32400:
      ([ "test":1021564800, // 2002-05-16 16:00:00
         25200:"America/Dawson",
         28800:"America/Yakutat",
         32400:"Pacific/Gambier",
      ]),
   34200:"Pacific/Marquesas",
   36000:
      ([ "test":202397400, // 1976-05-31 13:30:00
         32400:"America/Anchorage",
         36000:
            ([ "test":-933831000, // 1940-05-29 18:30:00
               36000:"Pacific/Tahiti",
               37800:"Pacific/Honolulu",
            ]),
      ]),
   37800:"Pacific/Rarotonga",
   38400:"Pacific/Kiritimati",
   39600:
      ([ "test":1325205000, // 2011-12-30 00:30:00
         -50400:"Pacific/Apia",
         32400:"America/Nome",
         36000:"America/Adak",
         39600:
            ([ "test":1341021600, // 2012-06-30 02:00:00
               -46800:"Pacific/Fakaofo",
               39600:"Pacific/Pago_Pago",
            ]),
      ]),
   41400:"Pacific/Niue",
   43200:
      ([ "test":526780800, // 1986-09-11 00:00:00
         39600:"Pacific/Enderbury",
         43200:"Pacific/Kwajalein",
      ]),
]);
