// ----------------------------------------------------------------
// Timezone names
//
// this file is created half-manually
// ----------------------------------------------------------------

//!	This module is a mapping of the names of 
//!	all the geographical (political) 
//!	based timezones. It looks mainly like
//!	@pre{
//!	(["Europe":({"Stockholm","Paris",...}),
//!       "America":({"Chicago","Panama",...}),
//!	  ...
//!     ])
//!     @}
//!
//!	It is mainly there for easy and reliable ways
//!	of making user interfaces to select timezone.
//!
//!	The Posix and standard timezones (like CET, PST8PDT, etc)
//!	are not listed.

#pike __REAL_VERSION__

//! @decl string _zone_tab()
//! @decl array(array) zone_tab()
//!	These return the raw respectively parsed zone tab file 
//!	from the timezone data files.
//!
//!	The parsed format is an array of zone tab line arrays,
//!	@code{
//!	({ string country_code, 
//!	   string position, 
//!	   string zone_name, 
//!	   string comment })
//!     @}
//!
//!	To convert the position to a @[Geography.Position], simply
//!	feed it to the constructor.

static string raw_zone_tab=0;
string _zone_tab()
{
   return raw_zone_tab ||
      (raw_zone_tab = Stdio.read_bytes(
	 combine_path(__FILE__,"..","tzdata/zone.tab")));
}

static array(array(string)) parsed_zone_tab=0;
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

//! @decl array(string) zonenames()
//!	This reads the zone.tab file and returns name of all
//!	standard timezones, like @tt{"Europe/Belgrade"@}.

static array(string) zone_names=0;
array(string) zonenames()
{
   return zone_names || (zone_names=column(zone_tab(),2));
}

//!	This constant is a mapping that can be
//!	used to loop over to get all the region-based
//!	timezones. 
//!
//!	It looks like this:
//!	@code{
//!	([ "America": ({ "Los_Angeles", "Chicago", <i>[...]</i> }),
//!	   "Europe":  ({ "Stockholm", <i>[...]</i> }),
//!        <i>[...]</i> }),
//!	@}
//!
//!	Please note that loading all the timezones can take some 
//!	time, since they are generated and compiled on the fly.
mapping(string:array(string)) zones =
([
   "America":   ({"Scoresbysund","Godthab","Thule","New_York","Chicago",    
                  "Denver","Los_Angeles","Juneau","Yakutat","Anchorage",    
                  "Nome","Adak","Phoenix","Boise","Indianapolis",           
                  "Indiana/Marengo","Indiana/Knox","Indiana/Vevay",           
                  "Louisville","Detroit","Menominee","St_Johns","Goose_Bay",
                  "Halifax","Glace_Bay","Montreal","Thunder_Bay","Nipigon", 
                  "Rainy_River","Winnipeg","Regina","Swift_Current",         
                  "Edmonton","Vancouver","Dawson_Creek","Pangnirtung",       
                  "Iqaluit","Rankin_Inlet","Cambridge_Bay","Yellowknife",    
                  "Inuvik","Whitehorse","Dawson","Cancun","Mexico_City",    
                  "Chihuahua","Hermosillo","Mazatlan","Tijuana","Anguilla", 
                  "Antigua","Nassau","Barbados","Belize","Cayman",          
                  "Costa_Rica","Havana","Dominica","Santo_Domingo",          
                  "El_Salvador","Grenada","Guadeloupe","Guatemala",          
                  "Port-au-Prince","Tegucigalpa","Jamaica","Martinique",     
                  "Montserrat","Managua","Panama","Puerto_Rico","St_Kitts", 
                  "St_Lucia","Miquelon","St_Vincent","Grand_Turk","Tortola",
                  "St_Thomas","Buenos_Aires","Rosario","Cordoba","Jujuy",   
                  "Catamarca","Mendoza","Aruba","La_Paz","Noronha","Belem",
                  "Fortaleza","Araguaina","Maceio","Sao_Paulo","Cuiaba",    
                  "Porto_Velho","Boa_Vista","Manaus","Porto_Acre","Santiago",
                  "Bogota","Curacao","Guayaquil","Cayenne","Guyana",        
                  "Asuncion","Lima","Paramaribo","Port_of_Spain",            
                  "Montevideo","Caracas"}),                    
   "Pacific":   ({"Rarotonga","Fiji","Gambier","Marquesas","Tahiti","Guam",
                  "Tarawa","Enderbury","Kiritimati","Saipan","Majuro",      
                  "Kwajalein","Yap","Truk","Ponape","Kosrae","Nauru",      
                  "Noumea","Auckland","Chatham","Niue","Norfolk","Palau",  
                  "Port_Moresby","Pitcairn","Pago_Pago","Apia","Guadalcanal",
                  "Fakaofo","Tongatapu","Funafuti","Johnston","Midway",     
                  "Wake","Efate","Wallis","Honolulu","Easter","Galapagos"}),
   "Antarctica":({"Casey","Davis","Mawson","DumontDUrville","Syowa",        
                  "Palmer","McMurdo"}),                        
   "Atlantic":  ({"Cape_Verde","St_Helena","Faeroe","Reykjavik","Jan_Mayen",
                  "Azores","Madeira","Canary","Bermuda","Stanley",          
                  "South_Georgia"}),                            
   "Indian":    ({"Comoro","Antananarivo","Mauritius","Mayotte","Reunion",  
                  "Mahe","Kerguelen","Chagos","Maldives","Christmas",       
                  "Cocos"}),                                    
   "Europe":    ({"London","Belfast","Dublin","Tirane","Andorra","Vienna", 
                  "Minsk","Brussels","Sofia","Prague","Copenhagen",         
                  "Tallinn","Helsinki","Paris","Berlin","Gibraltar",        
                  "Athens","Budapest","Rome","Riga","Vaduz","Vilnius",     
                  "Luxembourg","Malta","Chisinau","Tiraspol","Monaco",      
                  "Amsterdam","Oslo","Warsaw","Lisbon","Bucharest",         
                  "Kaliningrad","Moscow","Samara","Madrid","Stockholm",     
                  "Zurich","Istanbul","Kiev","Uzhgorod","Zaporozhye",       
                  "Simferopol","Belgrade"}),                   
   "Africa":    ({"Algiers","Luanda","Porto-Novo","Gaborone","Ouagadougou", 
                  "Bujumbura","Douala","Bangui","Ndjamena","Kinshasa",      
                  "Lubumbashi","Brazzaville","Abidjan","Djibouti","Cairo",  
                  "Malabo","Asmera","Addis_Ababa","Libreville","Banjul",    
                  "Accra","Conakry","Bissau","Nairobi","Maseru","Monrovia",
                  "Tripoli","Blantyre","Bamako","Timbuktu","Nouakchott",    
                  "Casablanca","El_Aaiun","Maputo","Windhoek","Niamey",     
                  "Lagos","Kigali","Sao_Tome","Dakar","Freetown",           
                  "Mogadishu","Johannesburg","Khartoum","Mbabane",           
                  "Dar_es_Salaam","Lome","Tunis","Kampala","Lusaka",        
                  "Harare","Ceuta"}),                          
   "Asia":      ({"Kabul","Yerevan","Baku","Bahrain","Dacca","Thimbu",     
                  "Brunei","Rangoon","Phnom_Penh","Harbin","Shanghai",      
                  "Chungking","Urumqi","Kashgar","Hong_Kong","Taipei",      
                  "Macao","Nicosia","Tbilisi","Dili","Calcutta","Jakarta", 
                  "Ujung_Pandang","Jayapura","Tehran","Baghdad","Jerusalem",
                  "Tokyo","Amman","Almaty","Aqtobe","Aqtau","Bishkek",     
                  "Seoul","Pyongyang","Kuwait","Vientiane","Beirut",        
                  "Kuala_Lumpur","Kuching","Hovd","Ulaanbaatar","Katmandu", 
                  "Muscat","Karachi","Gaza","Manila","Qatar","Riyadh",     
                  "Singapore","Colombo","Damascus","Dushanbe","Bangkok",    
                  "Ashkhabad","Dubai","Samarkand","Tashkent","Saigon",      
                  "Aden","Yekaterinburg","Omsk","Novosibirsk","Krasnoyarsk",
                  "Irkutsk","Yakutsk","Vladivostok","Magadan","Kamchatka",  
                  "Anadyr"}),                                   
   "Australia": ({"Darwin","Perth","Brisbane","Lindeman","Adelaide",        
                  "Hobart","Melbourne","Sydney","Broken_Hill","Lord_Howe"}),
]);

// this is used to dwim timezone

//!	This mapping is used to look up abbreviation
//!	to the possible regional zones. 
//!
//!	It looks like this:
//!	@code{
//!	([ "CET": ({ "Europe/Stockholm", @i{[...]@} }),
//!	   "CST": ({ "America/Chicago", "Australia/Adelaide", @i{[...]@} }),
//!        @i{[...]@} }),
//!	@}
//!
//!	Note this: Just because it's noted @tt{"CST"@} doesn't mean it's a
//!	unique timezone. There are about 7 *different* timezones that
//!	use @tt{"CST"@} as abbreviation; not at the same time,
//!	though, so the DWIM routines checks this before
//!	it's satisfied. Same with some other timezones. 
//!
//!     For most timezones, there is a number of region timezones that for the
//!     given time are equal. This is because region timezones include rules
//!     about local summer time shifts and possible historic shifts.
//!
//!	The @[YMD.parse] functions can handle timezone abbreviations
//!	by guessing.
mapping(string:array(string)) abbr2zones=
([
   "ACST": ({"America/Porto_Acre"}),                                           
   "ACT": ({"America/Porto_Acre"}),                                            
   "ADDT": ({"America/Pangnirtung"}),                                          
   "ADMT": ({"Africa/Asmera", "Africa/Addis_Ababa"}),                          
   "ADT": ({"Atlantic/Bermuda", "Asia/Baghdad", "America/Thule",               
       "America/Goose_Bay", "America/Halifax", "America/Glace_Bay",       
       "America/Pangnirtung", "America/Barbados", "America/Martinique"}), 
   "AFT": ({"Asia/Kabul"}),                                                    
   "AHDT": ({"America/Anchorage"}),                                            
   "AHST": ({"America/Anchorage"}),                                            
   "AHWT": ({"America/Anchorage"}),                                            
   "AKDT": ({"America/Juneau", "America/Yakutat", "America/Anchorage",         
       "America/Nome"}),                                                 
   "AKST": ({"Asia/Aqtobe", "America/Juneau", "America/Yakutat",               
       "America/Anchorage", "America/Nome"}),                            
   "AKT": ({"Asia/Aqtobe"}),                                                   
   "AKTST": ({"Asia/Aqtobe"}),                                                 
   "AKWT": ({"America/Juneau", "America/Yakutat", "America/Anchorage",         
       "America/Nome"}),                                                 
   "ALMST": ({"Asia/Almaty"}),                                                 
   "ALMT": ({"Asia/Almaty"}),                                                  
   "AMST": ({"Asia/Yerevan", "America/Cuiaba", "America/Porto_Velho",          
       "America/Boa_Vista", "America/Manaus"}),                          
   "AMT": ({"Europe/Athens", "Europe/Amsterdam", "Asia/Yerevan",               
       "Africa/Asmera", "America/Cuiaba", "America/Porto_Velho",          
       "America/Boa_Vista", "America/Manaus", "America/Asuncion"}),       
   "ANAMT": ({"Asia/Anadyr"}),                                                 
   "ANAST": ({"Asia/Anadyr"}),                                                 
   "ANAT": ({"Asia/Anadyr"}),                                                  
   "ANT": ({"America/Aruba", "America/Curacao"}),                              
   "AQTST": ({"Asia/Aqtobe", "Asia/Aqtau"}),                                   
   "AQTT": ({"Asia/Aqtobe", "Asia/Aqtau"}),                                    
   "ARST": ({"Antarctica/Palmer", "America/Buenos_Aires", "America/Rosario",   
       "America/Cordoba", "America/Jujuy", "America/Catamarca",          
       "America/Mendoza"}),                                              
   "ART": ({"Antarctica/Palmer", "America/Buenos_Aires", "America/Rosario",    
       "America/Cordoba", "America/Jujuy", "America/Catamarca",           
       "America/Mendoza"}),                                               
   "ASHST": ({"Asia/Ashkhabad"}),                                              
   "ASHT": ({"Asia/Ashkhabad"}),                                               
   "AST": ({"Atlantic/Bermuda", "Asia/Bahrain", "Asia/Baghdad", "Asia/Kuwait", 
       "Asia/Qatar", "Asia/Riyadh", "Asia/Aden", "America/Thule",         
       "America/Goose_Bay", "America/Halifax", "America/Glace_Bay",       
       "America/Pangnirtung", "America/Anguilla", "America/Antigua",      
       "America/Barbados", "America/Dominica", "America/Santo_Domingo",   
       "America/Grenada", "America/Guadeloupe", "America/Martinique",     
       "America/Montserrat", "America/Puerto_Rico", "America/St_Kitts",   
       "America/St_Lucia", "America/Miquelon", "America/St_Vincent",      
       "America/Tortola", "America/St_Thomas", "America/Aruba",           
       "America/Curacao", "America/Port_of_Spain"}),                      
   "AWT": ({"America/Puerto_Rico"}),                                           
   "AZOST": ({"Atlantic/Azores"}),                                             
   "AZOT": ({"Atlantic/Azores"}),                                              
   "AZST": ({"Asia/Baku"}),                                                    
   "AZT": ({"Asia/Baku"}),                                                     
   "BAKST": ({"Asia/Baku"}),                                                   
   "BAKT": ({"Asia/Baku"}),                                                    
   "BDT": ({"Asia/Dacca", "America/Nome", "America/Adak"}),                    
   "BEAT": ({"Africa/Nairobi", "Africa/Mogadishu", "Africa/Kampala"}),         
   "BEAUT": ({"Africa/Nairobi", "Africa/Dar_es_Salaam", "Africa/Kampala"}),    
   "BMT": ({"Europe/Brussels", "Europe/Chisinau", "Europe/Tiraspol",           
       "Europe/Bucharest", "Europe/Zurich", "Asia/Baghdad",               
       "Asia/Bangkok", "Africa/Banjul", "America/Barbados",               
       "America/Bogota"}),                                                
   "BNT": ({"Asia/Brunei"}),                                                   
   "BORT": ({"Asia/Ujung_Pandang", "Asia/Kuching"}),                           
   "BOST": ({"America/La_Paz"}),                                               
   "BOT": ({"America/La_Paz"}),                                                
   "BRST": ({"America/Belem", "America/Fortaleza", "America/Araguaina",        
       "America/Maceio", "America/Sao_Paulo"}),                          
   "BRT": ({"America/Belem", "America/Fortaleza", "America/Araguaina",         
       "America/Maceio", "America/Sao_Paulo"}),                           
   "BST": ({"Europe/London", "Europe/Belfast", "Europe/Dublin",                
       "Europe/Gibraltar", "Pacific/Pago_Pago", "Pacific/Midway",         
       "America/Nome", "America/Adak"}),                                  
   "BTT": ({"Asia/Thimbu"}),                                                   
   "BURT": ({"Asia/Dacca", "Asia/Rangoon", "Asia/Calcutta"}),                  
   "BWT": ({"America/Nome", "America/Adak"}),                                  
   "CANT": ({"Atlantic/Canary"}),                                              
   "CAST": ({"Africa/Gaborone", "Africa/Khartoum"}),                           
   "CAT": ({"Africa/Gaborone", "Africa/Bujumbura", "Africa/Lubumbashi",        
       "Africa/Blantyre", "Africa/Maputo", "Africa/Windhoek",             
       "Africa/Kigali", "Africa/Khartoum", "Africa/Lusaka",               
       "Africa/Harare", "America/Anchorage"}),                            
   "CCT": ({"Indian/Cocos"}),                                                  
   "CDDT": ({"America/Rankin_Inlet"}),                                         
   "CDT": ({"Asia/Harbin", "Asia/Shanghai", "Asia/Chungking", "Asia/Urumqi",   
       "Asia/Kashgar", "Asia/Taipei", "Asia/Macao", "America/Chicago",    
       "America/Indianapolis", "America/Indiana/Marengo",                 
       "America/Indiana/Knox", "America/Indiana/Vevay",                   
       "America/Louisville", "America/Menominee", "America/Rainy_River",  
       "America/Winnipeg", "America/Pangnirtung", "America/Iqaluit",      
       "America/Rankin_Inlet", "America/Cambridge_Bay", "America/Cancun", 
       "America/Mexico_City", "America/Chihuahua", "America/Belize",      
       "America/Costa_Rica", "America/Havana", "America/El_Salvador",     
       "America/Guatemala", "America/Tegucigalpa", "America/Managua"}),   
   "CEST": ({"Europe/Tirane", "Europe/Andorra", "Europe/Vienna",               
       "Europe/Minsk", "Europe/Brussels", "Europe/Sofia",                
       "Europe/Prague", "Europe/Copenhagen", "Europe/Tallinn",           
       "Europe/Berlin", "Europe/Gibraltar", "Europe/Athens",             
       "Europe/Budapest", "Europe/Rome", "Europe/Riga", "Europe/Vaduz",  
       "Europe/Vilnius", "Europe/Luxembourg", "Europe/Malta",            
       "Europe/Chisinau", "Europe/Tiraspol", "Europe/Monaco",            
       "Europe/Amsterdam", "Europe/Oslo", "Europe/Warsaw",               
       "Europe/Lisbon", "Europe/Kaliningrad", "Europe/Madrid",           
       "Europe/Stockholm", "Europe/Zurich", "Europe/Kiev",               
       "Europe/Uzhgorod", "Europe/Zaporozhye", "Europe/Simferopol",      
       "Europe/Belgrade", "Africa/Algiers", "Africa/Tripoli",            
       "Africa/Tunis", "Africa/Ceuta"}),                                 
   "CET": ({"Europe/Tirane", "Europe/Andorra", "Europe/Vienna", "Europe/Minsk",
       "Europe/Brussels", "Europe/Sofia", "Europe/Prague",                
       "Europe/Copenhagen", "Europe/Tallinn", "Europe/Berlin",            
       "Europe/Gibraltar", "Europe/Athens", "Europe/Budapest",            
       "Europe/Rome", "Europe/Riga", "Europe/Vaduz", "Europe/Vilnius",    
       "Europe/Luxembourg", "Europe/Malta", "Europe/Chisinau",            
       "Europe/Tiraspol", "Europe/Monaco", "Europe/Amsterdam",            
       "Europe/Oslo", "Europe/Warsaw", "Europe/Lisbon",                   
       "Europe/Kaliningrad", "Europe/Madrid", "Europe/Stockholm",         
       "Europe/Zurich", "Europe/Kiev", "Europe/Uzhgorod",                 
       "Europe/Zaporozhye", "Europe/Simferopol", "Europe/Belgrade",       
       "Africa/Algiers", "Africa/Tripoli", "Africa/Casablanca",           
       "Africa/Tunis", "Africa/Ceuta"}),                                  
   "CGST": ({"America/Scoresbysund"}),                                         
   "CGT": ({"America/Scoresbysund"}),                                          
   "CHDT": ({"America/Belize"}),                                               
   "CHUT": ({"Asia/Chungking"}),                                               
   "CJT": ({"Asia/Tokyo"}),                                                    
   "CKHST": ({"Pacific/Rarotonga"}),                                           
   "CKT": ({"Pacific/Rarotonga"}),                                             
   "CLST": ({"Antarctica/Palmer", "America/Santiago"}),                        
   "CLT": ({"Antarctica/Palmer", "America/Santiago"}),                         
   "CMT": ({"Europe/Copenhagen", "Europe/Chisinau", "Europe/Tiraspol",         
       "America/St_Lucia", "America/Buenos_Aires", "America/Rosario",     
       "America/Cordoba", "America/Jujuy", "America/Catamarca",           
       "America/Mendoza", "America/Caracas"}),                            
   "COST": ({"America/Bogota"}),                                               
   "COT": ({"America/Bogota"}),                                                
   "CST": ({"Asia/Harbin", "Asia/Shanghai", "Asia/Chungking", "Asia/Urumqi",   
       "Asia/Kashgar", "Asia/Taipei", "Asia/Macao", "Asia/Jayapura",      
       "Australia/Darwin", "Australia/Adelaide", "Australia/Broken_Hill", 
       "America/Chicago", "America/Indianapolis",                         
       "America/Indiana/Marengo", "America/Indiana/Knox",                 
       "America/Indiana/Vevay", "America/Louisville", "America/Detroit",  
       "America/Menominee", "America/Rainy_River", "America/Winnipeg",    
       "America/Regina", "America/Swift_Current", "America/Pangnirtung",  
       "America/Iqaluit", "America/Rankin_Inlet", "America/Cambridge_Bay",
       "America/Cancun", "America/Mexico_City", "America/Chihuahua",      
       "America/Hermosillo", "America/Mazatlan", "America/Belize",        
       "America/Costa_Rica", "America/Havana", "America/El_Salvador",     
       "America/Guatemala", "America/Tegucigalpa", "America/Managua"}),   
   "CUT": ({"Europe/Zaporozhye"}),                                             
   "CVST": ({"Atlantic/Cape_Verde"}),                                          
   "CVT": ({"Atlantic/Cape_Verde"}),                                           
   "CWT": ({"America/Chicago", "America/Indianapolis",                         
       "America/Indiana/Marengo", "America/Indiana/Knox",                 
       "America/Indiana/Vevay", "America/Louisville",                     
       "America/Menominee"}),                                             
   "CXT": ({"Indian/Christmas"}),                                              
   "DACT": ({"Asia/Dacca"}),                                                   
   "DAVT": ({"Antarctica/Davis"}),                                             
   "DDUT": ({"Antarctica/DumontDUrville"}),                                    
   "DMT": ({"Europe/Belfast", "Europe/Dublin"}),                               
   "DUSST": ({"Asia/Dushanbe"}),                                               
   "DUST": ({"Asia/Dushanbe"}),                                                
   "EASST": ({"Pacific/Easter"}),                                              
   "EAST": ({"Indian/Antananarivo", "Pacific/Easter"}),                        
   "EAT": ({"Indian/Comoro", "Indian/Antananarivo", "Indian/Mayotte",          
       "Africa/Djibouti", "Africa/Asmera", "Africa/Addis_Ababa",          
       "Africa/Nairobi", "Africa/Mogadishu", "Africa/Khartoum",           
       "Africa/Dar_es_Salaam", "Africa/Kampala"}),                        
   "ECT": ({"Pacific/Galapagos", "America/Guayaquil"}),                        
   "EDDT": ({"America/Iqaluit"}),                                              
   "EDT": ({"America/New_York", "America/Indianapolis",                        
       "America/Indiana/Marengo", "America/Indiana/Vevay",                
       "America/Louisville", "America/Detroit", "America/Montreal",       
       "America/Thunder_Bay", "America/Nipigon", "America/Pangnirtung",   
       "America/Iqaluit", "America/Cancun", "America/Nassau",             
       "America/Santo_Domingo", "America/Port-au-Prince",                 
       "America/Jamaica", "America/Grand_Turk"}),                         
   "EEMT": ({"Europe/Minsk", "Europe/Chisinau", "Europe/Tiraspol",             
       "Europe/Kaliningrad", "Europe/Moscow"}),                          
   "EEST": ({"Europe/Minsk", "Europe/Sofia", "Europe/Tallinn",                 
       "Europe/Helsinki", "Europe/Athens", "Europe/Riga",                
       "Europe/Vilnius", "Europe/Chisinau", "Europe/Tiraspol",           
       "Europe/Warsaw", "Europe/Bucharest", "Europe/Kaliningrad",        
       "Europe/Moscow", "Europe/Istanbul", "Europe/Kiev",                
       "Europe/Uzhgorod", "Europe/Zaporozhye", "Asia/Nicosia",           
       "Asia/Amman", "Asia/Beirut", "Asia/Gaza", "Asia/Damascus",        
       "Africa/Cairo"}),                                                 
   "EET": ({"Europe/Minsk", "Europe/Sofia", "Europe/Tallinn",                  
       "Europe/Helsinki", "Europe/Athens", "Europe/Riga",                 
       "Europe/Vilnius", "Europe/Chisinau", "Europe/Tiraspol",            
       "Europe/Warsaw", "Europe/Bucharest", "Europe/Kaliningrad",         
       "Europe/Moscow", "Europe/Istanbul", "Europe/Kiev",                 
       "Europe/Uzhgorod", "Europe/Zaporozhye", "Europe/Simferopol",       
       "Asia/Nicosia", "Asia/Amman", "Asia/Beirut", "Asia/Gaza",          
       "Asia/Damascus", "Africa/Cairo", "Africa/Tripoli"}),               
   "EGST": ({"America/Scoresbysund"}),                                         
   "EGT": ({"Atlantic/Jan_Mayen", "America/Scoresbysund"}),                    
   "EHDT": ({"America/Santo_Domingo"}),                                        
   "EST": ({"Australia/Brisbane", "Australia/Lindeman", "Australia/Hobart",    
       "Australia/Melbourne", "Australia/Sydney", "Australia/Broken_Hill",
       "Australia/Lord_Howe", "America/New_York", "America/Chicago",      
       "America/Indianapolis", "America/Indiana/Marengo",                 
       "America/Indiana/Knox", "America/Indiana/Vevay",                   
       "America/Louisville", "America/Detroit", "America/Menominee",      
       "America/Montreal", "America/Thunder_Bay", "America/Nipigon",      
       "America/Pangnirtung", "America/Iqaluit", "America/Cancun",        
       "America/Antigua", "America/Nassau", "America/Cayman",             
       "America/Santo_Domingo", "America/Port-au-Prince",                 
       "America/Jamaica", "America/Managua", "America/Panama",            
       "America/Grand_Turk"}),                                            
   "EWT": ({"America/New_York", "America/Indianapolis",                        
       "America/Indiana/Marengo", "America/Indiana/Vevay",                
       "America/Louisville", "America/Detroit", "America/Jamaica"}),      
   "FFMT": ({"America/Martinique"}),                                           
   "FJST": ({"Pacific/Fiji"}),                                                 
   "FJT": ({"Pacific/Fiji"}),                                                  
   "FKST": ({"Atlantic/Stanley"}),                                             
   "FKT": ({"Atlantic/Stanley"}),                                              
   "FMT": ({"Atlantic/Madeira", "Africa/Freetown"}),                           
   "FNST": ({"America/Noronha"}),                                              
   "FNT": ({"America/Noronha"}),                                               
   "FRUST": ({"Asia/Bishkek"}),                                                
   "FRUT": ({"Asia/Bishkek"}),                                                 
   "GALT": ({"Pacific/Galapagos"}),                                            
   "GAMT": ({"Pacific/Gambier"}),                                              
   "GBGT": ({"America/Guyana"}),                                               
   "GEST": ({"Asia/Tbilisi"}),                                                 
   "GET": ({"Asia/Tbilisi"}),                                                  
   "GFT": ({"America/Cayenne"}),                                               
   "GHST": ({"Africa/Accra"}),                                                 
   "GILT": ({"Pacific/Tarawa"}),                                               
   "GMT": ({"Atlantic/St_Helena", "Atlantic/Reykjavik", "Europe/London",       
       "Europe/Belfast", "Europe/Dublin", "Europe/Gibraltar",             
       "Africa/Porto-Novo", "Africa/Ouagadougou", "Africa/Abidjan",       
       "Africa/Malabo", "Africa/Banjul", "Africa/Accra", "Africa/Conakry",
       "Africa/Bissau", "Africa/Monrovia", "Africa/Bamako",               
       "Africa/Timbuktu", "Africa/Nouakchott", "Africa/Niamey",           
       "Africa/Sao_Tome", "Africa/Dakar", "Africa/Freetown",              
       "Africa/Lome"}),                                                   
   "GST": ({"Atlantic/South_Georgia", "Asia/Bahrain", "Asia/Muscat",           
       "Asia/Qatar", "Asia/Dubai", "Pacific/Guam"}),                      
   "GYT": ({"America/Guyana"}),                                                
   "HADT": ({"America/Adak"}),                                                 
   "HART": ({"Asia/Harbin"}),                                                  
   "HAST": ({"America/Adak"}),                                                 
   "HAWT": ({"America/Adak"}),                                                 
   "HDT": ({"Pacific/Honolulu"}),                                              
   "HKST": ({"Asia/Hong_Kong"}),                                               
   "HKT": ({"Asia/Hong_Kong"}),                                                
   "HMT": ({"Atlantic/Azores", "Europe/Helsinki", "Asia/Dacca",                
       "Asia/Calcutta", "America/Havana"}),                               
   "HOVST": ({"Asia/Hovd"}),                                                   
   "HOVT": ({"Asia/Hovd"}),                                                    
   "HST": ({"Pacific/Johnston", "Pacific/Honolulu"}),                          
   "HWT": ({"Pacific/Honolulu"}),                                              
   "ICT": ({"Asia/Phnom_Penh", "Asia/Vientiane", "Asia/Bangkok",               
       "Asia/Saigon"}),                                                   
   "IDDT": ({"Asia/Jerusalem", "Asia/Gaza"}),                                  
   "IDT": ({"Asia/Jerusalem", "Asia/Gaza"}),                                   
   "IHST": ({"Asia/Colombo"}),                                                 
   "IMT": ({"Europe/Sofia", "Europe/Istanbul", "Asia/Irkutsk"}),               
   "IOT": ({"Indian/Chagos"}),                                                 
   "IRKMT": ({"Asia/Irkutsk"}),                                                
   "IRKST": ({"Asia/Irkutsk"}),                                                
   "IRKT": ({"Asia/Irkutsk"}),                                                 
   "IRST": ({"Asia/Tehran"}),                                                  
   "IRT": ({"Asia/Tehran"}),                                                   
   "ISST": ({"Atlantic/Reykjavik"}),                                           
   "IST": ({"Atlantic/Reykjavik", "Europe/Belfast", "Europe/Dublin",           
       "Asia/Dacca", "Asia/Thimbu", "Asia/Calcutta", "Asia/Jerusalem",    
       "Asia/Katmandu", "Asia/Karachi", "Asia/Gaza", "Asia/Colombo"}),    
   "JAYT": ({"Asia/Jayapura"}),                                                
   "JMT": ({"Atlantic/St_Helena", "Asia/Jerusalem"}),                          
   "JST": ({"Asia/Rangoon", "Asia/Dili", "Asia/Ujung_Pandang", "Asia/Tokyo",   
       "Asia/Kuala_Lumpur", "Asia/Kuching", "Asia/Manila",                
       "Asia/Singapore", "Pacific/Nauru"}),                               
   "KART": ({"Asia/Karachi"}),                                                 
   "KAST": ({"Asia/Kashgar"}),                                                 
   "KDT": ({"Asia/Seoul"}),                                                    
   "KGST": ({"Asia/Bishkek"}),                                                 
   "KGT": ({"Asia/Bishkek"}),                                                  
   "KMT": ({"Europe/Vilnius", "Europe/Kiev", "America/Cayman",                 
       "America/Jamaica", "America/St_Vincent", "America/Grand_Turk"}),   
   "KOST": ({"Pacific/Kosrae"}),                                               
   "KRAMT": ({"Asia/Krasnoyarsk"}),                                            
   "KRAST": ({"Asia/Krasnoyarsk"}),                                            
   "KRAT": ({"Asia/Krasnoyarsk"}),                                             
   "KST": ({"Asia/Seoul", "Asia/Pyongyang"}),                                  
   "KUYMT": ({"Europe/Samara"}),                                               
   "KUYST": ({"Europe/Samara"}),                                               
   "KUYT": ({"Europe/Samara"}),                                                
   "KWAT": ({"Pacific/Kwajalein"}),                                            
   "LHST": ({"Australia/Lord_Howe"}),                                          
   "LINT": ({"Pacific/Kiritimati"}),                                           
   "LKT": ({"Asia/Colombo"}),                                                  
   "LPMT": ({"America/La_Paz"}),                                               
   "LRT": ({"Africa/Monrovia"}),                                               
   "LST": ({"Europe/Riga"}),                                                   
   "M": ({"Europe/Moscow"}),                                                   
   "MADST": ({"Atlantic/Madeira"}),                                            
   "MAGMT": ({"Asia/Magadan"}),                                                
   "MAGST": ({"Asia/Magadan"}),                                                
   "MAGT": ({"Asia/Magadan"}),                                                 
   "MALT": ({"Asia/Kuala_Lumpur", "Asia/Singapore"}),                          
   "MART": ({"Pacific/Marquesas"}),                                            
   "MAWT": ({"Antarctica/Mawson"}),                                            
   "MDDT": ({"America/Cambridge_Bay", "America/Yellowknife",                   
       "America/Inuvik"}),                                               
   "MDST": ({"Europe/Moscow"}),                                                
   "MDT": ({"America/Denver", "America/Phoenix", "America/Boise",              
       "America/Regina", "America/Swift_Current", "America/Edmonton",     
       "America/Cambridge_Bay", "America/Yellowknife", "America/Inuvik",  
       "America/Chihuahua", "America/Hermosillo", "America/Mazatlan"}),   
   "MHT": ({"Pacific/Majuro", "Pacific/Kwajalein"}),                           
   "MMT": ({"Indian/Maldives", "Europe/Minsk", "Europe/Moscow", "Asia/Rangoon",
       "Asia/Ujung_Pandang", "Asia/Colombo", "Pacific/Easter",            
       "Africa/Monrovia", "America/Managua", "America/Montevideo"}),      
   "MOST": ({"Asia/Macao"}),                                                   
   "MOT": ({"Asia/Macao"}),                                                    
   "MPT": ({"Pacific/Saipan"}),                                                
   "MSK": ({"Europe/Minsk", "Europe/Tallinn", "Europe/Riga", "Europe/Vilnius", 
       "Europe/Chisinau", "Europe/Kiev", "Europe/Uzhgorod",               
       "Europe/Zaporozhye", "Europe/Simferopol"}),                        
   "MST": ({"Europe/Moscow", "America/Denver", "America/Phoenix",              
       "America/Boise", "America/Regina", "America/Swift_Current",        
       "America/Edmonton", "America/Dawson_Creek",                        
       "America/Cambridge_Bay", "America/Yellowknife", "America/Inuvik",  
       "America/Mexico_City", "America/Chihuahua", "America/Hermosillo",  
       "America/Mazatlan", "America/Tijuana"}),                           
   "MUT": ({"Indian/Mauritius"}),                                              
   "MVT": ({"Indian/Maldives"}),                                               
   "MWT": ({"America/Denver", "America/Phoenix", "America/Boise"}),            
   "MYT": ({"Asia/Kuala_Lumpur", "Asia/Kuching"}),                             
   "NCST": ({"Pacific/Noumea"}),                                               
   "NCT": ({"Pacific/Noumea"}),                                                
   "NDT": ({"America/Nome", "America/Adak", "America/St_Johns",                
       "America/Goose_Bay"}),                                             
   "NEGT": ({"America/Paramaribo"}),                                           
   "NFT": ({"Pacific/Norfolk"}),                                               
   "NMT": ({"Pacific/Norfolk"}),                                               
   "NOVMT": ({"Asia/Novosibirsk"}),                                            
   "NOVST": ({"Asia/Novosibirsk"}),                                            
   "NOVT": ({"Asia/Novosibirsk"}),                                             
   "NPT": ({"Asia/Katmandu"}),                                                 
   "NRT": ({"Pacific/Nauru"}),                                                 
   "NST": ({"Europe/Amsterdam", "Pacific/Pago_Pago", "Pacific/Midway",         
       "America/Nome", "America/Adak", "America/St_Johns",                
       "America/Goose_Bay"}),                                             
   "NUT": ({"Pacific/Niue"}),                                                  
   "NWT": ({"America/Nome", "America/Adak"}),                                  
   "NZDT": ({"Antarctica/McMurdo"}),                                           
   "NZHDT": ({"Pacific/Auckland"}),                                            
   "NZST": ({"Antarctica/McMurdo", "Pacific/Auckland"}),                       
   "OMSMT": ({"Asia/Omsk"}),                                                   
   "OMSST": ({"Asia/Omsk"}),                                                   
   "OMST": ({"Asia/Omsk"}),                                                    
   "PDDT": ({"America/Inuvik", "America/Whitehorse", "America/Dawson"}),       
   "PDT": ({"America/Los_Angeles", "America/Juneau", "America/Boise",          
       "America/Vancouver", "America/Dawson_Creek", "America/Inuvik",     
       "America/Whitehorse", "America/Dawson", "America/Tijuana"}),       
   "PEST": ({"America/Lima"}),                                                 
   "PET": ({"America/Lima"}),                                                  
   "PETMT": ({"Asia/Kamchatka"}),                                              
   "PETST": ({"Asia/Kamchatka"}),                                              
   "PETT": ({"Asia/Kamchatka"}),                                               
   "PGT": ({"Pacific/Port_Moresby"}),                                          
   "PHOT": ({"Pacific/Enderbury"}),                                            
   "PHST": ({"Asia/Manila"}),                                                  
   "PHT": ({"Asia/Manila"}),                                                   
   "PKT": ({"Asia/Karachi"}),                                                  
   "PMDT": ({"America/Miquelon"}),                                             
   "PMMT": ({"Pacific/Port_Moresby"}),                                         
   "PMST": ({"America/Miquelon"}),                                             
   "PMT": ({"Antarctica/DumontDUrville", "Europe/Prague", "Europe/Paris",      
       "Europe/Monaco", "Africa/Algiers", "Africa/Tunis",                 
       "America/Panama", "America/Paramaribo"}),                          
   "PNT": ({"Pacific/Pitcairn"}),                                              
   "PONT": ({"Pacific/Ponape"}),                                               
   "PPMT": ({"America/Port-au-Prince"}),                                       
   "PST": ({"Pacific/Pitcairn", "America/Los_Angeles", "America/Juneau",       
       "America/Boise", "America/Vancouver", "America/Dawson_Creek",      
       "America/Inuvik", "America/Whitehorse", "America/Dawson",          
       "America/Hermosillo", "America/Mazatlan", "America/Tijuana"}),     
   "PWT": ({"Pacific/Palau", "America/Los_Angeles", "America/Juneau",          
       "America/Boise", "America/Tijuana"}),                              
   "PYST": ({"America/Asuncion"}),                                             
   "PYT": ({"America/Asuncion"}),                                              
   "QMT": ({"America/Guayaquil"}),                                             
   "RET": ({"Indian/Reunion"}),                                                
   "RMT": ({"Atlantic/Reykjavik", "Europe/Rome", "Europe/Riga",                
       "Asia/Rangoon"}),                                                  
   "S": ({"Europe/Moscow"}),                                                   
   "SAMMT": ({"Europe/Samara"}),                                               
   "SAMST": ({"Europe/Samara", "Asia/Samarkand"}),                             
   "SAMT": ({"Europe/Samara", "Asia/Samarkand", "Pacific/Pago_Pago",           
       "Pacific/Apia"}),                                                 
   "SAST": ({"Africa/Maseru", "Africa/Windhoek", "Africa/Johannesburg",        
       "Africa/Mbabane"}),                                               
   "SBT": ({"Pacific/Guadalcanal"}),                                           
   "SCT": ({"Indian/Mahe"}),                                                   
   "SDMT": ({"America/Santo_Domingo"}),                                        
   "SGT": ({"Asia/Singapore"}),                                                
   "SHEST": ({"Asia/Aqtau"}),                                                  
   "SHET": ({"Asia/Aqtau"}),                                                   
   "SJMT": ({"America/Costa_Rica"}),                                           
   "SLST": ({"Africa/Freetown"}),                                              
   "SMT": ({"Atlantic/Stanley", "Europe/Stockholm", "Europe/Simferopol",       
       "Asia/Phnom_Penh", "Asia/Vientiane", "Asia/Kuala_Lumpur",          
       "Asia/Singapore", "Asia/Saigon", "America/Santiago"}),             
   "SRT": ({"America/Paramaribo"}),                                            
   "SST": ({"Pacific/Pago_Pago", "Pacific/Midway"}),                           
   "SVEMT": ({"Asia/Yekaterinburg"}),                                          
   "SVEST": ({"Asia/Yekaterinburg"}),                                          
   "SVET": ({"Asia/Yekaterinburg"}),                                           
   "SWAT": ({"Africa/Windhoek"}),                                              
   "SYOT": ({"Antarctica/Syowa"}),                                             
   "TAHT": ({"Pacific/Tahiti"}),                                               
   "TASST": ({"Asia/Samarkand", "Asia/Tashkent"}),                             
   "TAST": ({"Asia/Samarkand", "Asia/Tashkent"}),                              
   "TBIST": ({"Asia/Tbilisi"}),                                                
   "TBIT": ({"Asia/Tbilisi"}),                                                 
   "TBMT": ({"Asia/Tbilisi"}),                                                 
   "TFT": ({"Indian/Kerguelen"}),                                              
   "TJT": ({"Asia/Dushanbe"}),                                                 
   "TKT": ({"Pacific/Fakaofo"}),                                               
   "TMST": ({"Asia/Ashkhabad"}),                                               
   "TMT": ({"Europe/Tallinn", "Asia/Tehran", "Asia/Ashkhabad"}),               
   "TOST": ({"Pacific/Tongatapu"}),                                            
   "TOT": ({"Pacific/Tongatapu"}),                                             
   "TPT": ({"Asia/Dili"}),                                                     
   "TRST": ({"Europe/Istanbul"}),                                              
   "TRT": ({"Europe/Istanbul"}),                                               
   "TRUT": ({"Pacific/Truk"}),                                                 
   "TVT": ({"Pacific/Funafuti"}),                                              
   "ULAST": ({"Asia/Ulaanbaatar"}),                                            
   "ULAT": ({"Asia/Ulaanbaatar"}),                                             
   "URUT": ({"Asia/Urumqi"}),                                                  
   "UYHST": ({"America/Montevideo"}),                                          
   "UYT": ({"America/Montevideo"}),                                            
   "UZST": ({"Asia/Samarkand", "Asia/Tashkent"}),                              
   "UZT": ({"Asia/Samarkand", "Asia/Tashkent"}),                               
   "VET": ({"America/Caracas"}),                                               
   "VLAMT": ({"Asia/Vladivostok"}),                                            
   "VLAST": ({"Asia/Vladivostok"}),                                            
   "VLAT": ({"Asia/Vladivostok"}),                                             
   "VUST": ({"Pacific/Efate"}),                                                
   "VUT": ({"Pacific/Efate"}),                                                 
   "WAKT": ({"Pacific/Wake"}),                                                 
   "WARST": ({"America/Jujuy", "America/Mendoza"}),                            
   "WART": ({"America/Jujuy", "America/Mendoza"}),                             
   "WAST": ({"Africa/Ndjamena", "Africa/Windhoek"}),                           
   "WAT": ({"Africa/Luanda", "Africa/Porto-Novo", "Africa/Douala",             
       "Africa/Bangui", "Africa/Ndjamena", "Africa/Kinshasa",             
       "Africa/Brazzaville", "Africa/Malabo", "Africa/Libreville",        
       "Africa/Banjul", "Africa/Conakry", "Africa/Bissau",                
       "Africa/Bamako", "Africa/Nouakchott", "Africa/El_Aaiun",           
       "Africa/Windhoek", "Africa/Niamey", "Africa/Lagos", "Africa/Dakar",
       "Africa/Freetown"}),                                               
   "WEST": ({"Atlantic/Faeroe", "Atlantic/Azores", "Atlantic/Madeira",         
       "Atlantic/Canary", "Europe/Brussels", "Europe/Luxembourg",        
       "Europe/Monaco", "Europe/Lisbon", "Europe/Madrid",                
       "Africa/Algiers", "Africa/Casablanca", "Africa/Ceuta"}),          
   "WET": ({"Atlantic/Faeroe", "Atlantic/Azores", "Atlantic/Madeira",          
       "Atlantic/Canary", "Europe/Andorra", "Europe/Brussels",            
       "Europe/Luxembourg", "Europe/Monaco", "Europe/Lisbon",             
       "Europe/Madrid", "Africa/Algiers", "Africa/Casablanca",            
       "Africa/El_Aaiun", "Africa/Ceuta"}),                               
   "WFT": ({"Pacific/Wallis"}),                                                
   "WGST": ({"America/Godthab"}),                                              
   "WGT": ({"America/Godthab"}),                                               
   "WMT": ({"Europe/Vilnius", "Europe/Warsaw"}),                               
   "WST": ({"Antarctica/Casey", "Pacific/Apia", "Australia/Perth"}),           
   "YAKMT": ({"Asia/Yakutsk"}),                                                
   "YAKST": ({"Asia/Yakutsk"}),                                                
   "YAKT": ({"Asia/Yakutsk"}),                                                 
   "YAPT": ({"Pacific/Yap"}),                                                  
   "YDDT": ({"America/Whitehorse", "America/Dawson"}),                         
   "YDT": ({"America/Yakutat", "America/Whitehorse", "America/Dawson"}),       
   "YEKMT": ({"Asia/Yekaterinburg"}),                                          
   "YEKST": ({"Asia/Yekaterinburg"}),                                          
   "YEKT": ({"Asia/Yekaterinburg"}),                                           
   "YERST": ({"Asia/Yerevan"}),                                                
   "YERT": ({"Asia/Yerevan"}),                                                 
   "YST": ({"America/Yakutat", "America/Whitehorse", "America/Dawson"}),       
   "YWT": ({"America/Yakutat"}),                                               
   "___": ({"Antarctica/Casey", "Antarctica/Davis", "Antarctica/Mawson",       
       "Antarctica/DumontDUrville", "Antarctica/Syowa",                   
       "Antarctica/Palmer", "Antarctica/McMurdo", "Indian/Kerguelen"}),   
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
      -39600:"Asia/Kamchatka",]),
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


