import ".";

static string flat(string s)
{
   return replace(lower_case(s),
		  "àáâãäåæçèéêëìíîïðñòóôõöøùúûüýþÿ'- "/1,
		  "aaaaaaeceeeeiiiidnoooooouuuuyty"/1+({""})*3);
}

static class _language_base
{
   inherit Ruleset.Language;

   static mapping events_translate=0;

   string translate_event(string name)
   {
      if (events_translate) return events_translate[name]||name;
      return name;
   }
}

static string roman_number(int m)
{
  string res="";
  if (m<0) return "["+m+"]";
  if (m==0) return "O";
  if (m>100000) return "["+m+"]";
  while (m>999) { res+="M"; m-=1000; }
  if (m>899) { res+="CM"; m-=900; }
  else if (m>499) { res+="D"; m-=500; }
  else if (m>399) { res+="CD"; m-=400; }
  while (m>99) { res+="C"; m-=100; }
  if (m>89) { res+="XC"; m-=90; }
  else if (m>49) { res+="L"; m-=50; }
  else if (m>39) { res+="XL"; m-=40; }
  while (m>9) { res+="X"; m-=10; }
  if (m>8) return res+"IX";
  else if (m>4) { res+="V"; m-=5; }
  else if (m>3) return res+"IV";
  while (m) { res+="I"; m--; }
  return res;
}

static class _ymd_base
{
   inherit _language_base;

   static mapping(int:string) month_n2s;
   static mapping(int:string) month_n2ss;
   static mapping(string:int) month_s2n;
   static mapping(int:string) week_day_n2s;
   static mapping(int:string) week_day_n2ss;
   static mapping(string:int) week_day_s2n;

   string month_name_from_number(int n)
   {
      return month_n2s[n];
   }

   string month_shortname_from_number(int n)
   {
      return month_n2ss[n];
   }

   int month_number_from_name(string name)
   {
      int j=(month_s2n[lower_case(name)]);
      if (!j) error("no such month of year: %O\n",name);
      return j;
   }

   string week_day_name_from_number(int n)
   {
      return week_day_n2s[n];
   }

   string week_day_shortname_from_number(int n)
   {
      return week_day_n2ss[n];
   }

   int week_day_number_from_name(string name)
   {
      int j=(week_day_s2n[lower_case(name)]);
      if (!j) error("no such day of week: %O\n",name);
      return j;
   }

   string week_name_from_number(int n)
   {
      return sprintf("w%d",n);
   }

   int week_number_from_name(string s)
   {
      int w;
      if (sscanf(s,"w%d",w)) return w;
      if (sscanf(s,"%d",w)) return w;
      return 0;
   }

   string year_name_from_number(int y)
   {
      if (y<1) return sprintf("%d BC",1-y);
      return (string)y;
   }

   int year_number_from_name(string name)
   {
      int y;
      string x;
      if (sscanf(name,"%d%s",y,x)==1 || 
	  x=="") 
	 return y>=0?y:y+1; // "-1" == integer year 0
      switch (x)
      {
	 case "AD": case " AD": return y; 
	 case "BC": case " BC": return -y+1;
	 default:
	    error("Can't understand year.\n");
      }
   }

   string month_day_name_from_number(int d,int mnd)
   {
      return (string)d;
   }

// gregorian defaults

   string gregorian_week_day_name_from_number(int n)
   {
      return week_day_n2s[(n+5)%7+1];
   }

   string gregorian_week_day_shortname_from_number(int n)
   {
      return week_day_n2ss[(n+5)%7+1];
   }

   int gregorian_week_day_number_from_name(string name)
   {
      int j=(week_day_s2n[lower_case(name)]);
      if (!j) error("no such day of week: %O\n",name);
      return j%7+1;
   }

   string gregorian_year_name_from_number(int y)
   {
      if (y<1) return sprintf("%d BC",1-y);
      return sprintf("%d AD",y);
   }


// discordian defaults

   string discordian_month_name_from_number(int n)
   {
      return ({0,"Chaos","Discord","Confusion","Bureaucracy","The Aftermath"})[n];
   }

   string discordian_month_shortname_from_number(int n)
   {
      return ({0,"Chs","Dsc","Cfn","Bcy","Afm"})[n];
   }

   int discordian_month_number_from_name(string name)
   {
      return (["chaos":1,"discord":2,"confusion":3,
	       "bureaucracy":4,"the aftermath":5,
	       "chs":1,"dsc":2,"cfn":3,"bcy":4,"afm":5])
	 [lower_case(name)];
   }

   string discordian_week_day_shortname_from_number(int n)
   {
      return ({0,"SM","BT","PD","PP","SO","ST"})[n];
   }

   string discordian_week_day_name_from_number(int n)
   {
      return ({0,"Sweetmorn","Boomtime","Pungenday","Prickle-Prickle",
	       "Setting Orange","St. Tib's day"})[n];
   }

   int discordian_week_day_number_from_name(string name)
   {
      return (["sweetmorn":1,"boomtime":2,"pungenday":3,"prickle-prickle":4,
	       "setting orange":5,"st. tib's day":6,
	       "prickleprickle":4,"setting":5,"orange":5,"tib":6,"tibs":6,
	       "sttib":6,"sttibs":6,"saint tib's day":6,
	       "sm":1,"bt":2,"pd":3,"pp":4,"so":5,"st":6])
	 [lower_case(name)];
   }

   string discordian_week_name_from_number(int n)
   {
      return "w"+n;
   }

   string discordian_year_name_from_number(int y)
   {
      return (string)y;
   }

// coptic defaults

   string coptic_month_name_from_number(int n)
   {
      return ({0,"Tout","Baba","Hator","Kiahk","Toba",
	       "Amshir","Baramhat","Baramouda","Pakho",
	       "Paona","Epep","Mesra","Nasie"})[n];
   }

   string coptic_month_shortname_from_number(int n)
   {
      return ({0,"Tou","Bab","Hat","Kia","Tob",
	       "Ams","Bar","Bar","Pak",
	       "Pao","Epe","Mes","Nas"})[n];
   }

   int coptic_month_number_from_name(string name)
   {
      return (["tout":1,"baba":2,"hator":3,"kiahk":4,"toba":5,
	       "amshir":6,"baramhat":7,"baramouda":8,"pakho":9,
	       "paona":10,"epep":11,"mesra":12,"nasie":13,
	       "tou":1,"bab":2,"hat":3,"kia":4,"tob":5,
	       "ams":6,"bar":7,"bar":8,"pak":9,
	       "pao":10,"epe":11,"mes":12,"nas":13])
	 [lower_case(name)];
   }

   string coptic_year_name_from_number(int y)
   {
      return (string)y;
   }

   int x;

// islamic defaults

   array(string) islamic_months=
   ",Muharram,Safar,Rebîu'l-awwal,Rebîul-âchir,"
   "Djumâda'l-ûla,Djumâda'l-âchira,Redjeb,Shaabân,Ramadân,"
   "Schawwâl,Dhu'l-káada,Dhu'l-Hiddja"/",";
   array(string) islamic_shortmonths= // help! :)
   ",Muharram,Safar,Rebîu'l-awwal,Rebîul-âchir,"
   "Djumâda'l-ûla,Djumâda'l-âchira,Redjeb,Shaabân,Ramadân,"
   "Schawwâl,Dhu'l-káada,Dhu'l-Hiddja"/",";
   mapping islamic_backmonth=0;
   array(string) islamic_shortweekdays=
   ",aha,ith,thu,arb,kha,dsc,sab"/",";
   array(string) islamic_weekdays=
   ",ahad,ithnain,thulâthâ,arbiâ,khamîs,dschuma,sabt"/",";
   mapping islamic_backweekday=0;

   string islamic_month_name_from_number(int n)
   {
      return islamic_months[n];
   }

   string islamic_month_shortname_from_number(int n)
   {
      return islamic_shortmonths[n];
   }

   int islamic_month_number_from_name(string name)
   {
      if (!islamic_backmonth)
      {
	 islamic_backmonth=
	    mkmapping(map(islamic_months[1..],flat),
		      enumerate(12,1,1))|
	    mkmapping(map(islamic_months[1..],flat),
		      enumerate(12,1,1))|
	 (["rabi1":2,
	   "rabi2":3,
	   "djumada1":4,
	   "djumada2":5]);
      }
      
      return islamic_backmonth[`-(flat(name),"-","'"," ")];
   }

   string islamic_week_day_name_from_number(int n)
   {
      return "jaum el "+islamic_weekdays[n];
   }

   string islamic_week_day_shortname_from_number(int n)
   {
      return islamic_shortweekdays[n];
   }

   int islamic_week_day_number_from_name(string name)
   {
      if (!islamic_backweekday)
      {
	 islamic_backweekday=
	    mkmapping(map(map(islamic_weekdays[1..],flat),`-,"'","-"),
		      enumerate(7,1,1))|
	    mkmapping(map(map(islamic_weekdays[1..],flat),`-,"'","-"),
		      enumerate(7,1,1));
      }
      
      sscanf(name,"jaum el %s",name);
      return islamic_backweekday[`-(flat(name),"-","'")];
   }


   string islamic_week_name_from_number(int n)
   {
      return "w"+n;
   }

   string islamic_year_name_from_number(int y)
   {
      if (y<1) return sprintf("%d BH",1-y);
      return sprintf("%d AH",y);
   }
}

// ----------------------------------------------------------------

// this sets up the mappings from the arrays

#define SETUPSTUFF							\
      month_n2s=mkmapping(enumerate(12,1,1),month_names);		\
      month_n2ss= 							\
	 mkmapping(enumerate(12,1,1),map(month_names,predef::`[],0,2));	\
      month_s2n=							\
	 mkmapping(map(map(month_names,predef::`[],0,2),flat),	\
		   enumerate(12,1,1))					\
	 | mkmapping(map(month_names,flat),enumerate(12,1,1));	\
      week_day_n2s= mkmapping(enumerate(7,1,1),week_day_names);		\
      week_day_n2ss= mkmapping(enumerate(7,1,1),			\
			       map(week_day_names,predef::`[],0,2));	\
      week_day_s2n= 							\
	 mkmapping(map(map(week_day_names,predef::`[],0,2),flat),	\
		   enumerate(7,1,1))					\
	 | mkmapping(map(week_day_names,flat),enumerate(7,1,1))

// ----------------------------------------------------------------
// now the real classes:

// this should probably be called UK_en or something:

constant cENGLISH=cISO;
class cISO
{
   inherit _ymd_base;
   
   constant month_names=
   ({"January","February","March","April","May","June","July","August",
     "September","October","November","December"});

   constant week_day_names=
   ({"Monday","Tuesday","Wednesday","Thursday","Friday","Saturday","Sunday"});

   void create()
   {
      SETUPSTUFF;
   }

   string week_name_from_number(int n)
   {
      return sprintf("w%d",n);
   }

   int week_number_from_name(string s)
   {
      int w;
      if (sscanf(s,"w%d",w)) return w;
      if (sscanf(s,"%d",w)) return w;
      return 0;
   }

   string year_name_from_number(int y)
   {
      if (y<1) return sprintf("%d BC",1-y);
      return (string)y;
   }
};

// swedish (note: all name as cLANG where LANG is in caps)

constant cSE_SV=cSWEDISH;
class cSWEDISH
{
   inherit _ymd_base;

   static private constant month_names=
   ({"januari","februari","mars","april","maj","juni","juli","augusti",
     "september","oktober","november","december"});

   static private constant week_day_names=
   ({"måndag","tisdag","onsdag","torsdag",
     "fredag","lördag","söndag"});

   static mapping events_translate=
   ([
      "New Year's Day":		"Nyårsdagen",
      "Epiphany":		"Trettondag jul",
      "King's Nameday":		"H K M Konungens namnsdag",
      "Candlemas":		"Kyndelsmässodagen",
      "St. Valentine":		"Alla hjärtans dag",
      "Int. Women's Day":	"Internationella kvinnodagen",
      "Crown Princess' Nameday":"H K M Kronprinsessans namnsdag",
      "Waffle Day":		"Våffeldagen",
      "Annunciation":		"Marie bebådelsedag",
      "Labor Day":		"Första maj",
      "Sweden's Flag Day":	"Svenska flaggans dag",
      "St. John the Baptist":	"Johannes Döpares dag",
      "Crown Princess' Birthday":"H K M Kronprinsessans födelsedag",
      "Queen's Nameday":	"H K M Drottningens namnsdag",
      "UN Day":			"FN-dagen",
      "All saints Day":		"Allhelgonadagen",
      "King's Nameday":		"H K M Konungens namnsdag",
      "King's Birthday":	"H K M Konungens födelsedag",
      "St. Lucy":		"Luciadagen",
      "Queen's Birthday":	"H K M Drottningens födelsedag",
      "Christmas Eve":		"Julafton",
      "Christmas Day":		"Juldagen",
      "St. Stephen":		"Annandagen",
      "New Year's Eve":		"Nyårsafton",
      "Midsummer's Eve":	"Midsommarafton",
      "Midsummer's Day":	"Midsommardagen",
      "All Saints Day":		"Allhelgonadagen",

      "Fat Tuesday":		"Fettisdagen",
      "Palm Sunday":		"Palmsöndagen",
      "Good Friday":		"Långfredagen",
      "Easter Eve":		"Påskafton",
      "Easter":			"Påskdagen",
      "Easter Monday":		"Annandag påsk",
      "Ascension":		"Kristi himmelsfärd",
      "Pentecost Eve":		"Pingstafton",
      "Pentecost":		"Pingst",
      "Pentecost Monday":	"Annandag pingst",
      "Advent 1":		"Första advent",
      "Advent 2":		"Andra advent",
      "Advent 3":		"Tredje advent",
      "Advent 4":		"Fjärde advent",
      "Mother's Day":		"Mors dag",
      "Father's Day":		"Fars dag",

      "Summer Solstice":	"Sommarsolstånd",
      "Winter Solstice":	"Vintersolstånd",
      "Spring Equinox":		"Vårdagjämning",
      "Autumn Equinox":		"Höstdagjämning",

// not translated:
//	"Halloween"
//	"Alla helgons dag"
//	"Valborgsmässoafton"
   ]);

   void create()
   {
      SETUPSTUFF;
   }

   string week_name_from_number(int n)
   {
      return sprintf("v%d",n);
   }

   int week_number_from_name(string s)
   {
      if (sscanf(s,"v%d",int w)) return w;
      return ::week_number_from_name(s);
   }

   string year_name_from_number(int y)
   {
      if (y<1) return sprintf("%d fk",1-y);
      return (string)y;
   }
}

// austrian
// Martin Baehr <mbaehr@email.archlab.tuwien.ac.at>

class cAUSTRIAN
{
   inherit _ymd_base;

   static private constant month_names=
      ({"jänner","feber","märz","april","mai","juni","juli","august",
        "september","oktober","november","dezember"});

   static private constant week_day_names=
      ({"montag","dienstag","mittwoch","donnerstag",
        "freitag","samstag","sonntag"});

   void create()
   {
      SETUPSTUFF;
   }
}

// Welsh

class cWELSH
{
   inherit _ymd_base;

   static private constant month_names=
   ({"ionawr","chwefror","mawrth","ebrill","mai","mehefin",
     "gorffenaf","awst","medi","hydref","tachwedd","rhagfyr"});

   static private constant week_day_names=
   ({"Llun","Mawrth","Mercher","Iau","Gwener","Sadwrn","Sul"});

   string week_day_name_from_number(int n)
   {
      return "dydd "+::week_day_name_from_number(n);
   }

   int week_day_number_from_name(string name)
   {
      sscanf(name,"dydd %s",name);
      return week_day_number_from_name(name);
   }

   void create()
   {
      SETUPSTUFF;
   }
}

// Spanish
// Julio César Gázquez <jgazquez@dld.net>

class cSPANISH
{
   inherit _ymd_base;

   static private constant month_names=
   ({"enero","febrero","marzo","abril","mayo","junio",
     "julio","agosto","setiembre","octubre","noviembre","diciembre"});

   static private constant week_day_names=
   ({"lunes","martes","miércoles","jueves",
     "viernes","sábado","domingo"});

// contains argentina for now
   static mapping events_translate=
   ([
      "Epiphany":"Día de Reyes", // Epifania
      "Malvinas Day":"Día de las Malvinas",
      "Labor Day":"Aniversario de la Revolución",
      "Soberany's Day":"Día de la soberania",
      "Flag's Day":"Día de la bandera",
      "Independence Day":"Día de la independencia",
      "Assumption Day":"Día de la asunción", // ?
      "Gral San Martín decease":
      "Aniversario del fallecimiento del Gral. San Martin",
      "Race's Day":"Día de la Raza",
      "All Saints Day":"Día de todos los santos",
      "Immaculate Conception":"Inmaculada Concepción",
      "Christmas Day":"Natividad del Señor",
      "New Year's Day":"Año Nuevo",
      "Holy Thursday":"Jueves Santo",
      "Good Friday":"Viernes Santo",
      "Holy Saturday":"Sãbado de gloria",
      "Easter":"Domingo de resurrección",
      "Corpus Christi":"Corpus Christi"
   ]);

   void create()
   {
      SETUPSTUFF;
   }
}

// Hungarian
// Csongor Fagyal <concept@conceptonline.hu>

class cHUNGARIAN
{
   inherit _ymd_base;

   static private constant month_names=
   ({"Január","Február","Március","Április","Május","Június",
     "Július","August","September","October","November","December"});

   static private constant week_day_names=
   ({"Hétfo","Kedd","Szerda","Csütörtkök","Péntek","Szombat","Vasárnap"});

// contains argentina for now
   static mapping events_translate=
   ([
      "New Year's Day":"Úb év ünnepe",
      "1848 Revolution Day":"Az 'Az 1848-as Forradalom Napja",
      "Holiday of Labor":"A munka ünnepe",
      "Constitution Day":"Az alkotmány ünnepe",
      "'56 Revolution Day":"Az '56-os Forradalom ünnepe",
      "Easter":"Húsvét",
      "Easter monday":"Húsvét",
      "Whitsunday":"Pünkösd",
      "Whitmonday":"Pünkösd",
      "Christmas":"Christmas",
   ]);

   void create()
   {
      SETUPSTUFF;
   }
}

// Modern Latin

class cLATIN
{
   inherit _ymd_base;

   static array(string) month_names=
   ({"Ianuarius", "Februarius", "Martius", "Aprilis", "Maius", "Iunius", 
     "Iulius", "Augustus", "September", "October", "November", "December" });

   static private constant week_day_names=
   ({"lunae","Martis","Mercurii","Jovis","Veneris","Saturni","solis"});

   string week_day_name_from_number(int n)
   {
      return ::week_day_name_from_number(n)+" dies";
   }

   int week_day_number_from_name(string name)
   {
      sscanf(name,"%s dies",name);
      return week_day_number_from_name(name);
   }

   string gregorian_year_name_from_number(int y)
   {
      return year_name_from_number(y);
   }

   string year_name_from_number(int y)
   {
      if (y<1) return sprintf("%d BC",1-y); // ?
      return sprintf("anno ab Incarnatione Domini %s",roman_number(y));
   }

   void create()
   {
      SETUPSTUFF;
   }
}

// Roman latin

class cROMAN
{
   inherit cLATIN;

   static array(string) month_names=
   ({"Ianuarius", "Februarius", "Martius", "Aprilis", "Maius", "Iunius",
     "Quintilis", // Iulius
     "Sextilis",  // Augustus
     "September", "October", "November", "December"
   });

   string year_name_from_number(int y)
   {
      return sprintf("%s ab urbe condita",roman_number(y+752));
   }
   
   string month_day_name_from_number(int d,int mnd)
   {
// this is not really correct, I've seen but
// i can't find it - they did something like 4 from the start of the
// months, 19 from the end of the month.
      return roman_number(d); 
   }
}




// ----------------------------------------------------------------

// find & compile language

static mapping _cache=([]);

Ruleset.Language `[](string lang)
{
   lang=upper_case(lang);
   Ruleset.Language l=_cache[lang];
   if (l) return l;
   program cl=::`[]("c"+lang);

   if (!cl) { return ([])[0]; }

   l=_cache[lang]=cl();
   
   return l;
}
