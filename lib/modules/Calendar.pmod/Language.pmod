#pike __REAL_VERSION__

import ".";

static string flat(string s)
{
   return replace(lower_case(s),
		  "àáâãäåæçèéêëìíîïðñòóôõöøùúûüýþÿ'- "/1,
		  "aaaaaaeceeeeiiiidnoooooouuuuyty"/1+({""})*3);
}

static class _language_base
{
   inherit Rule.Language;

   static mapping events_translate=0;

   string translate_event(string name)
   {
      if (events_translate) return events_translate[name]||name;
      return name;
   }
}

static string roman_number(int m)
{
  if      (m<0)      return "["+m+"]";
  if      (m==0)     return "O";
  if      (m>100000) return "["+m+"]";
  return String.int2roman(m);
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
      int j=(month_s2n[name])
	 || (month_s2n[flat(name)]);
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
      int j=(week_day_s2n[name])
	 || (week_day_s2n[flat(name)]);
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
      int j=(week_day_s2n[flat(name)]);
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
	 [flat(name)];
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
	 [flat(name)];
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
	 [flat(name)];
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

#define SETUPSTUFF2							\
      month_n2s=mkmapping(enumerate(12,1,1),month_names);		\
      month_n2ss= 							\
	 mkmapping(enumerate(12,1,1),short_month_names);	\
      month_s2n=							\
	 mkmapping(map(short_month_names,flat),	\
		   enumerate(12,1,1))					\
	 | mkmapping(map(month_names,flat),enumerate(12,1,1));	\
      week_day_n2s= mkmapping(enumerate(7,1,1),week_day_names);		\
      week_day_n2ss= mkmapping(enumerate(7,1,1),			\
			       short_week_day_names);	\
      week_day_s2n= 							\
	 mkmapping(map(short_week_day_names,flat),	\
		   enumerate(7,1,1))					\
	 | mkmapping(map(week_day_names,flat),enumerate(7,1,1))




// ========================================================================



// now the real classes:

// this should probably be called UK_en or something:

constant cENGLISH=cISO;
constant cENG=cISO;
constant cEN=cISO;
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
constant cSV=cSWEDISH;
constant cSWE=cSWEDISH;
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
      "National Day":		"Nationaldagen",
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
// source: Martin Baehr <mbaehr@email.archlab.tuwien.ac.at>

constant cDE_AT=cAUSTRIAN; // this is a german dialect, appearantly
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
// source: book

constant cCY=cWELSH;
constant cCYM=cWELSH;
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

constant cES=cSPANISH;
constant cSPA=cSPANISH;
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

// portugese
// source: Sérgio Araújo <sergio@projecto-oasis.cx>

constant cPT=cPORTUGESE; // Portugese (Brasil)
constant cPOR=cPORTUGESE;
class cPORTUGESE
{
   inherit _ymd_base;

   static private constant month_names=
   ({
      "Janeiro",
      "Fevereiro",
      "Março",
      "Abril",
      "Maio",
      "Junho",
      "Julho",
      "Agosto",
      "Setembro",
      "Outubro",
      "Novembro",
      "Dezembro",
   });

   static private constant week_day_names=
   ({
      "Segunda-feira", // -feira is removed for the short version
      "Terça-feira",   // don't know how it's used
      "Quarta-feira",
      "Quinta-feira",
      "Sexta-feira",
      "Sábado",
      "Domingo",
   });

// contains argentina for now
   static mapping events_translate=
   ([
      "New Year's Day":"Ano Novo",
      "Good Friday":"Sexta-Feira Santa",
      "Liberty Day":"Dia da Liberdade",
      "Labor Day":"Dia do Trabalhador",
      "Portugal Day":"Dia de Portugal",
      "Corpus Christi":"Corpo de Deus",
      "Assumption Day":"Assunção",
      "Republic Day":"Implantação da República",
      "All Saints Day":"Todos-os-Santos",
      "Restoration of the Independence":"Restauração da Independência",
      "Immaculate Conception":"Imaculada Conceição",
      "Christmas":"Natal"
   ]);

   void create()
   {
      SETUPSTUFF;
   }
}

// Hungarian
// Csongor Fagyal <concept@conceptonline.hu>

constant cHU=cHUNGARIAN;
constant cHUN=cHUNGARIAN;
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
// source: book

constant cLA=cLATIN;
constant cLAT=cLATIN;
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
// source: calendar FAQ + book

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

   int year_number_from_name(string name)
   {
      int y;
      sscanf(name,"%d",y);
      return y-752;
   }
   
   string month_day_name_from_number(int d,int mnd)
   {
// this is not really correct, I've seen but
// i can't find it - they did something like 4 from the start of the
// months, 19 from the end of the month.
      return roman_number(d); 
   }
}


// source: anonymous unix locale file

constant cKL=cGREENLANDIC; // Greenlandic 
constant cKAL=cGREENLANDIC; 
class cGREENLANDIC
{
   inherit _ymd_base;

   static private constant month_names=
   ({
      "januari",
      "februari",
      "martsi",
      "aprili",
      "maji",
      "juni",
      "juli",
      "augustusi",
      "septemberi",
      "oktoberi",
      "novemberi",
      "decemberi",
   });

   static private constant week_day_names=
   ({
      "ataasinngorneq",
      "marlunngorneq",
      "pingasunngorneq",
      "sisamanngorneq",
      "tallimanngorneq",
      "arfininngorneq",
      "sabaat",
   });

   void create() { SETUPSTUFF; }
}


// source: anonymous unix locale file

constant cIS=cICELANDIC; // Icelandic 
constant cISL=cICELANDIC;
class cICELANDIC
{
   inherit _ymd_base;

   static private constant month_names=
   ({
      "Januar",
      "Februar",
      "Mars",
      "April",
      "Mai",
      "Juni",
      "Juli",
      "Agust",
      "September",
      "Oktober",
      "November",
      "Desember",
   });

   static private constant week_day_names=
   ({
      "Manudagur",
      "Tridjudagur",
      "Midvikudagur",
      "Fimmtudagur",
      "Föstudagur",
      "Laugardagur",
      "Sunnudagur",
   });

   void create() { SETUPSTUFF; }
}


// source: anonymous unix locale file

constant cFA=cPERSIAN; // Persian (Iran)
constant cFAS=cPERSIAN;
class cPERSIAN
{
   inherit _ymd_base;

   static private constant month_names=
   ({
      "zanwyh",            // <zj><a+><n+><w+><yf><h+>
      "fwrwyh",            // <f+><w+><r+><w+><yf><h+>
      "mars",
      "awryl",
      "mh",
      "zwyn",              // <zj><w+><yH><n+>
      "zwyyh",
      "awt",
      "sptambr",
      "aktbr",
      "nwambr",
      "dsambr",
   });

   static private constant week_day_names=
   ({
      "dwsnbh",
      "shzsnbh",
      "tharsnbh",
      "pngzsnbh",
      "gmeh",
      "snbh",
      "ykzsnbh",           // <yf><kf><zwnj><sn><n+><b+><h+>
   });

   void create() { SETUPSTUFF; }
}


// source: anonymous unix locale file

constant cAF=cAFRIKAANS; // Afrikaans (South Africa)
constant cAFR=cAFRIKAANS; 
class cAFRIKAANS
{
   inherit _ymd_base;

   static private constant month_names=
   ({
      "Januarie",
      "Februarie",
      "Maart",
      "April",
      "Mei",
      "Junie",
      "Julie",
      "Augustus",
      "September",
      "Oktober",
      "November",
      "Desember",
   });

   static private constant week_day_names=
   ({
      "Maandag",
      "Dinsdag",
      "Woensdag",
      "Donderdag",
      "Vrydag",
      "Saterdag",
      "Sondag",
   });

   void create() { SETUPSTUFF; }
}


// source: anonymous unix locale file

constant cGA=cIRISH; // Irish Gaelic 
constant cGLE=cIRISH;
class cIRISH
{
   inherit _ymd_base;

   static private constant month_names=
   ({
      "Eanáir",
      "Feabhra",
      "Márta",
      "Aibreán",
      "Mí na Bealtaine",
      "Meith",
      "Iúil",
      "Lúnasa",
      "Meán Fómhair",
      "Deireadh Fómhair",
      "Mí na Samhna",
      "Mí na Nollag",
   });

   static private constant short_month_names=
   ({
      "Ean","Fea","Már","Aib","Bea","Mei","Iúi","Lún","MFó","DFó","Sam","Nol"
   });

   static private constant week_day_names=
   ({
      "Dé Luain",
      "Dé Máirt",
      "Dé Céadaoin",
      "Déardaoin",
      "Dé hAoine",
      "Dé Sathairn",
      "Dé Domhnaigh",
   });

   static private constant short_week_day_names=
   ({
      "Lua","Mai","Céa","Déa","Aoi","Sat", "Dom",
   });

   void create() { SETUPSTUFF2; }
}


// source: anonymous unix locale file

constant cEU=cBASQUE; // Basque (Spain)
constant cEUS=cBASQUE;
class cBASQUE
{
   inherit _ymd_base;

   static private constant month_names=
   ({
      "urtarrila",
      "otsaila",
      "martxoa",
      "apirila",
      "maiatza",
      "ekaina",
      "uztaila",
      "abuztua",
      "iraila",
      "urria",
      "azaroa",
      "abendua",
   });

   static private constant week_day_names=
   ({
      "astelehena",
      "asteartea",
      "asteazkena",
      "osteguna",
      "ostirala",
      "larunbata",
      "igandea",
   });

   void create() { SETUPSTUFF; }
}


// source: anonymous unix locale file

constant cNO=cNORWEGIAN; // Norwegian 
constant cNOR=cNORWEGIAN;
class cNORWEGIAN
{
   inherit _ymd_base;

   static private constant month_names=
   ({
      "januar",
      "februar",
      "mars",
      "april",
      "mai",
      "juni",
      "juli",
      "august",
      "september",
      "oktober",
      "november",
      "desember",
   });

   static private constant week_day_names=
   ({
      "mandag",
      "tirsdag",
      "onsdag",
      "torsdag",
      "fredag",
      "lørdag",
      "søndag",
   });

   void create() { SETUPSTUFF; }
}


// source: anonymous unix locale file



constant cNL=cDUTCH; // Dutch
constant cNLD=cDUTCH;
class cDUTCH
{
   inherit _ymd_base;

   static private constant month_names=
   ({
      "januari",
      "februari",
      "maart",
      "april",
      "mei",
      "juni",
      "juli",
      "augustus",
      "september",
      "oktober",
      "november",
      "december",
   });

   static private constant week_day_names=
   ({
      "maandag",
      "dinsdag",
      "woensdag",
      "donderdag",
      "vrijdag",
      "zaterdag",
      "zondag",
   });

   void create() { SETUPSTUFF; }
}


// source: anonymous unix locale file

constant cPL=cPOLISH; // Polish 
constant cPOL=cPOLISH;
class cPOLISH
{
   inherit _ymd_base;

   static private constant month_names=
   ({
      "styczen",           // <s><t><y><c><z><e><n'>
      "luty",
      "marzec",
      "kwiecien",
      "maj",
      "czerwiec",
      "lipiec",
      "sierpien",
      "wrzesien",
      "pazdziernik",
      "listopad",
      "grudzien",
   });

   static private constant week_day_names=
   ({
      "poniedzialek",      // <p><o><n><i><e><d><z><i><a><l/><e><k>
      "wtorek",
      "sroda",
      "czwartek",
      "piatek",
      "sobota",
      "niedziela",
   });

   void create() { SETUPSTUFF; }
}

class cPOLISH_UNICODE
{
   inherit _ymd_base;

   static private constant month_names=
   ({
      "stycze\x0143",           // <s><t><y><c><z><e><n'>
      "luty",
      "marzec",
      "kwiecien",
      "maj",
      "czerwiec",
      "lipiec",
      "sierpien",
      "wrzesien",
      "pazdziernik",
      "listopad",
      "grudzien",
   });

   static private constant week_day_names=
   ({
      "poniedzia\x0142""ek",      // <p><o><n><i><e><d><z><i><a><l/><e><k>
      "wtorek",
      "sroda",
      "czwartek",
      "piatek",
      "sobota",
      "niedziela",
   });

   void create() { SETUPSTUFF; }
}


// source: Cile Ekici (@ Roxen IS)

constant cTR=cTURKISH; // Turkish
constant cTUR=cTURKISH; // Turkish
class cTURKISH
{
   inherit _ymd_base;

   static private constant month_names=
   ({
      "Ocak",
      "Subat",
      "Mart",
      "Nisan",
      "Mayis",
      "Haziran",
      "Temmuz",
      "Agustos",
      "Eylül",
      "Ekim",
      "Kasim",
      "Aralik",
   });

   static private constant week_day_names=
   ({
      "Pazartesi",
      "Sali",              // <S><a><l><i.>
      "Çarsamba",
      "Persembe",
      "Cuma",
      "Cumartesi",
      "Pazar",
   });

   void create() { SETUPSTUFF; }
}

constant cTR_UNICODE=cTURKISH_UNICODE; // Turkish
constant cTUR_UNICODE=cTURKISH_UNICODE; // Turkish
class cTURKISH_UNICODE
{
   inherit _ymd_base;

   static private constant month_names=
   ({
      "Ocak",
      "\015e""ubat",    // S-cedilla
      "Mart",
      "Nisan",
      "May\x0131""s",   // i-no-dot
      "Haziran",
      "Temmuz",
      "A\x01e5""ustos", // line over g
      "Eylül",
      "Ekim",
      "Kas\x0131""m",   // i-no-dot
      "Aral\x0131""k",  // i-no-dot
   });

   static private constant week_day_names=
   ({
      "Pazartesi",
      "Sal\x0131",      // <S><a><l><i.> i without dot
      "Çarsamba",
      "Per\x015f""embe",// s-cedilla
      "Cuma",
      "Cumartesi",
      "Pazar",
   });

   void create() { SETUPSTUFF; }
}

// source: anonymous unix locale file



constant cDE=cGERMAN; // German 
constant cDEU=cGERMAN;
class cGERMAN
{
   inherit _ymd_base;

   static private constant month_names=
   ({
      "Januar",
      "Februar",
      "März",
      "April",
      "Mai",
      "Juni",
      "Juli",
      "August",
      "September",
      "Oktober",
      "November",
      "Dezember",
   });

   static private constant week_day_names=
   ({
      "Montag",
      "Dienstag",
      "Mittwoch",
      "Donnerstag",
      "Freitag",
      "Samstag",
      "Sonntag",
   });

   void create() { SETUPSTUFF; }
}


// source: anonymous unix locale file

constant cLV=cLATVIAN; // Latvian 
constant cLAV=cLATVIAN;
class cLATVIAN
{
   inherit _ymd_base;

   static private constant month_names=
   ({
      "janvaris",          // <j><a><n><v><a-><r><i><s>
      "februaris",
      "marts",
      "aprilis",           // <a><p><r><i-><l><i><s>
      "maijs",
      "junijs",
      "julijs",
      "augusts",
      "septembris",
      "oktobris",
      "novembris",
      "decembris",
   });

   static private constant week_day_names=
   ({
      "pirmdiena",
      "otrdiena",
      "tresdiena",
      "ceturtdiena",
      "piektdiena",
      "sestdiena",
      "svetdiena",         // <s><v><e-><t><d><i><e><n><a>
   });

   void create() { SETUPSTUFF; }
}

constant cLV_UNICODE=cLATVIAN_UNICODE;
constant cLAV_UNICODE=cLATVIAN_UNICODE;
class cLATVIAN_UNICODE
{
   inherit _ymd_base;

   static private constant month_names=
   ({
      "janv\x0101""ris",          // <j><a><n><v><a-><r><i><s>
      "februaris",
      "marts",
      "apr\x012b""lis",           // <a><p><r><i-><l><i><s>
      "maijs",
      "junijs",
      "julijs",
      "augusts",
      "septembris",
      "oktobris",
      "novembris",
      "decembris",
   });

   static private constant week_day_names=
   ({
      "pirmdiena",
      "otrdiena",
      "tresdiena",
      "ceturtdiena",
      "piektdiena",
      "sestdiena",
      "sv\x0113""tdiena",         // <s><v><e-><t><d><i><e><n><a>
   });

   void create() { SETUPSTUFF; }
}


// source: anonymous unix locale file

constant cFI=cFINNISH; // Finnish 
constant cFIN=cFINNISH;
class cFINNISH
{
   inherit _ymd_base;

   static private constant month_names=
   ({
      "tammikuu",
      "helmikuu",
      "maaliskuu",
      "huhtikuu",
      "toukokuu",
      "kesäkuu",
      "heinäkuu",
      "elokuu",
      "syyskuu",
      "lokakuu",
      "marraskuu",
      "joulukuu",
   });

   static private constant week_day_names=
   ({
      "maanantai",
      "tiistai",
      "keskiviikko",
      "torstai",
      "perjantai",
      "lauantai",
      "sunnuntai",
   });

   void create() { SETUPSTUFF; }
}


// source: anonymous unix locale file

constant cLT=cLITHUANIAN; // Lithuanian
constant cLIT=cLITHUANIAN;
class cLITHUANIAN
{
   inherit _ymd_base;

   static private constant month_names=
   ({
      "sausio",
      "vasario",
      "kovo",
      "balandzio",
      "geguzes",           // <g><e><g><u><z<><e.><s>
      "birzelio",
      "liepos",
      "rugpjucio",         // <r><u><g><p><j><u-><c<><i><o>
      "rugsejo",
      "spalio",
      "lapkricio",
      "gruodzio",
   });

   static private constant week_day_names=
   ({
      "Pirmadienis",
      "Antradienis",
      "Treciadienis",
      "Ketvirtadienis",
      "Penktadienis",
      "Sestadienis",       // <S<><e><s<><t><a><d><i><e><n><i><s>
      "Sekmadienis",
   });

   void create() { SETUPSTUFF; }
}

class cLITHUANIAN_UNICODE
{
   inherit _ymd_base;

   static private constant month_names=
   ({
      "sausio",
      "vasario",
      "kovo",
      "balandzio",
      "geguz\x0117""s",           // <g><e><g><u><z<><e.><s>
      "birzelio",
      "liepos",
      "rugpj\x016b\x010d""io",         // <r><u><g><p><j><u-><c<><i><o>
      "rugsejo",
      "spalio",
      "lapkricio",
      "gruodzio",
   });

   static private constant week_day_names=
   ({
      "Pirmadienis",
      "Antradienis",
      "Treciadienis",
      "Ketvirtadienis",
      "Penktadienis",
      "\x0160""estadienis",       // <S<><e><s<><t><a><d><i><e><n><i><s>
      "Sekmadienis",
   });

   void create() { SETUPSTUFF; }
}


// source: anonymous unix locale file

constant cET=cESTONIAN; // Estonian 
constant cEST=cESTONIAN;
class cESTONIAN
{
   inherit _ymd_base;

   static constant month_names=
   ({
      "jaanuar",
      "veebruar",
      "märts",
      "aprill",
      "mai",
      "juuni",
      "juuli",
      "august",
      "september",
      "oktoober",
      "november",
      "detsember",
   });

   static constant week_day_names=
   ({
      "esmaspäev",
      "teisipäev",
      "kolmapäev",
      "neljapäev",
      "reede",
      "laupäev",
      "pühapäev",          // <p><u:><h><a><p><a:><e><v>
   });

   void create() { SETUPSTUFF; }
}

constant cGL=cGALICIAN; // Galician (Spain)
constant cGLG=cGALICIAN;
class cGALICIAN
{
   inherit _ymd_base;

   static private constant month_names=
   ({
      "Xaneiro",
      "Febreiro",
      "Marzo",
      "Abril",
      "Maio",
      "Xuño",
      "Xullo",
      "Agosto",
      "Setembro",
      "Outubro",
      "Novembro",
      "Decembro",
   });

   static private constant week_day_names=
   ({
      "Luns",
      "Martes",
      "Mércores",
      "Xoves",
      "Venres",
      "Sabado",
      "Domingo",
   });

   void create() { SETUPSTUFF; }
}

constant cID=cINDONESIAN; 
class cINDONESIAN
{
   inherit _ymd_base;

   static private constant month_names=
   ({
      "Januari",
      "Pebruari",
      "Maret",
      "April",
      "Mei",
      "Juni",
      "Juli",
      "Agustus",
      "September",
      "Oktober",
      "November",
      "Desember",
   });

   static private constant week_day_names=
   ({
      "Senin",
      "Selasa",
      "Rabu",
      "Kamis",
      "Jumat",
      "Sabtu",
      "Minggu",   
   });

   void create() { SETUPSTUFF; }
}

constant cFR=cFRENCH; // French
constant cFRA=cFRENCH;
class cFRENCH
{
   inherit _ymd_base;

   static private constant month_names=
   ({
      "janvier",
      "février",
      "mars",
      "avril",
      "mai",
      "juin",
      "juillet",
      "aoû",
      "septembre",
      "octobre",
      "novembre",
      "décembre",
   });

   static private constant week_day_names=
   ({
      "lundi",
      "mardi",
      "mercredi",
      "jeudi",
      "vendredi",
      "samedi",
      "dimanche",
   });

   void create() { SETUPSTUFF; }
}

constant cIT=cITALIAN; // Italian
constant cITA=cITALIAN;
class cITALIAN
{
   inherit _ymd_base;

   static private constant month_names=
   ({
      "gennaio",
      "febbraio",
      "marzo",
      "aprile",
      "maggio",
      "giugno",
      "luglio",
      "agosto",
      "settembre",
      "ottobre",
      "novembre",
      "dicembre",
   });

   static private constant week_day_names=
   ({
      "lunedi",    // swizz italian: "lunedì" - should I care?
      "martedi",
      "mercoledi",
      "giovedi",
      "venerdi",
      "sabato",
      "domenica",
   });

   void create() { SETUPSTUFF; }
}

constant cCA=cCATALAN; // Catalan (Catalonia)
constant cCAT=cCATALAN;
class cCATALAN
{
   inherit _ymd_base;

   static private constant month_names=
   ({
      "gener",
      "febrer",
      "març",
      "abril",
      "maig",
      "juny",
      "juliol",
      "agost",
      "setembre",
      "octubre",
      "novembre",
      "decembre",
   });

   static private constant week_day_names=
   ({
      "dilluns",
      "dimarts",
      "dimecres",
      "dijous",
      "divendres",
      "dissabte",
      "diumenge",
   });

   void create() { SETUPSTUFF; }
}

constant cSL=cSLOVENIAN; // Slovenian
constant cSLV=cSLOVENIAN;
class cSLOVENIAN
{
   inherit _ymd_base;

   static constant month_names=
   ({
      "januar",
      "februar",
      "marec",
      "april",
      "maj",
      "juni",
      "juli",
      "avgust",
      "september",
      "oktober",
      "november",
      "december",
   });

   static constant week_day_names=
   ({
      "ponedeljek",
      "torek",
      "sreda",
      "cetrtek",    // <c<><e><t><r><t><e><k>
      "petek",
      "sobota",
      "nedelja",
   });

   void create() { SETUPSTUFF; }
}

class cSLOVENIAN_UNICODE
{
   inherit cSLOVENIAN;

   static constant week_day_names=
   ({
      "ponedeljek",
      "torek",
      "sreda",
      "\x010d""etrtek",    // <c<><e><t><r><t><e><k>
      "petek",
      "sobota",
      "nedelja",
   });

   void create() { SETUPSTUFF; }
}

constant cFO=cFAROESE; // Faroese 
constant cFAO=cFAROESE;
class cFAROESE
{
   inherit _ymd_base;

   static private constant month_names=
   ({
      "januar",
      "februar",
      "mars",
      "april",
      "mai",
      "juni",
      "juli",
      "august",
      "september",
      "oktober",
      "november",
      "desember",
   });

   static private constant week_day_names=
   ({
      "manadagur",
      "týsdagur",
      "mikudagur",
      "hosdagur",
      "friggjadagur",
      "leygardagur",
      "sunnudagur",
   });

   void create() { SETUPSTUFF; }
}

constant cRO=cROMANIAN; // Romanian
constant cRON=cROMANIAN;
class cROMANIAN
{
   inherit _ymd_base;

   static constant month_names=
   ({
      "Ianuarie",
      "Februarie",
      "Martie",
      "Aprilie",
      "Mai",
      "Iunie",
      "Iulie",
      "August",
      "Septembrie",
      "Octombrie",
      "Noiembrie",
      "Decembrie",
   });

   static constant week_day_names=
   ({
      "Luni",
      "Marti",             // <M><A><R><T,><I>
      "Miercuri",
      "Joi",
      "Vineri",
      "Simbata",                // <S><I/>><M><B><A(><T><A(>
      "Duminica",          // <D><U><M><I><N><I><C><A(>
   });

   void create() { SETUPSTUFF; }
}

class cROMANIAN_UNICODE
{
   inherit cROMANIAN;

   static constant week_day_names=
   ({
      "Luni",
      "Mar\x0163""i",             // <M><A><R><T,><I>
      "Miercuri",
      "Joi",
      "Vineri",
      "Sîmb\x0103""t\x0103",                // <S><I/>><M><B><A(><T><A(>
      "Duminic\x0103",          // <D><U><M><I><N><I><C><A(>
   });

   void create() { SETUPSTUFF; }
}

constant cHR=cCROATIAN; // Croatian
constant cHRV=cCROATIAN; // Croatian
class cCROATIAN
{
   inherit _ymd_base;

   static private constant month_names=
   ({
      "Sijecanj",          // <S><i><j><e><c<><a><n><j>
      "Veljaca",           // <V><e><l><j><a><c<><a>
      "Ozujak",            // <O><z<><u><j><a><k>
      "Travanj",
      "Svibanj",
      "Lipanj",
      "Srpanj",
      "Kolovoz",
      "Rujan",
      "Listopad",
      "Studeni",
      "Prosinac",
   });

   static private constant week_day_names=
   ({
      "Ponedjeljak",
      "Utorak",
      "Srijeda",
      "Cetvrtak",          // <C<><e><t><v><r><t><a><k>
      "Petak",
      "Subota",
      "Nedjelja",
   });

   void create() { SETUPSTUFF; }
}

class cCROATIAN_UNICODE
{
   inherit _ymd_base;

   static private constant month_names=
   ({
      "Sije\415anj",
      "Velja\415a",
      "O\576ujak",
      "Travanj",
      "Svibanj",
      "Lipanj",
      "Srpanj",
      "Kolovoz",
      "Rujan",
      "Listopad",
      "Studeni",
      "Prosinac",
   });

   static private constant week_day_names=
   ({
      "Ponedjeljak",
      "Utorak",
      "Srijeda",
      "\414etvrtak",
      "Petak",
      "Subota",
      "Nedjelja",
   });

   void create() { SETUPSTUFF; }
}

constant cDA=cDANISH; // Danish 
constant cDAN=cDANISH;
class cDANISH
{
   inherit _ymd_base;

   static private constant month_names=
   ({
      "januar",
      "februar",
      "marts",
      "april",
      "maj",
      "juni",
      "juli",
      "august",
      "september",
      "oktober",
      "november",
      "december",
   });

   static private constant week_day_names=
   ({
      "mandag",
      "tirsdag",
      "onsdag",
      "torsdag",
      "fredag",
      "lørdag",
      "søndag",
   });

   static mapping events_translate=
   ([
      "2nd day of Christmas"		    :"2. juledag",			      
      "Ascension"			    :"Kristi himmelfart",		      
      "Birthday of Queen Margrethe II"	    :"Dronning Margrethes fødselsdag",   
      "Carnival"			    :"Fastelavn",			      
      "Christmas Day"			    :"1. juledag",			      
      "Christmas Eve"			    :"Juleaften",			      
      "Constitution Day"		    :"Grundlovsdag",		      
      "Easter Monday"			    :"2. påskedag",			      
      "Easter"                              :"Påske",			      
      "Good Friday"			    :"Langfredag",
      "Great Prayer Day"		    :"Store bededag",		      
      "Holy Thursday"			    :"Skærtorsdag",		      
      "Labor Day"			    :"1. Maj",			      
      "New Year's Day"			    :"Nytårsdag",			      
      "New Year's Eve"			    :"Nytårsaften",			      
      "Palm Sunday"			    :"Palme søndag",		      
      "Pentecost Monday"		    :"2. Pinsedag",			      
      "Pentecost"			    :"Pinsedag",			      
      "Prince Fredrik's Birthday"	    :"Kronprins Fredriks fødselsdag",    
      "Epiphany"			    :"Hellig trekongers dag",
      "Epiphany Eve" 			    :"Hellig trekongers aften",
   ]);

   void create() { SETUPSTUFF; }
}

constant cSR=cSERBIAN; // Serbian (Yugoslavia)
constant cSRP=cSERBIAN;
class cSERBIAN
{
   inherit _ymd_base;

   static constant month_names=
   ({
      "januar",
      "februar",
      "mart",
      "april",
      "maj",
      "jun",
      "jul",
      "avgust",
      "septembar",
      "oktobar",
      "novembar",
      "decembar",
   });

   static constant week_day_names=
   ({
      "ponedeljak",
      "utorak",
      "sreda",
      "cetvrtak",
      "petak",
      "subota",
      "nedelja",
   });

   void create() { SETUPSTUFF; }
}


class cSERBIAN_UNICODE
{
   inherit cSERBIAN;

   static constant week_day_names=
   ({
      "ponedeljak",
      "utorak",
      "sreda",
      "\415etvrtak",
      "petak",
      "subota",
      "nedelja",
   });

   void create() { SETUPSTUFF; }
}

// ----------------------------------------------------------------

// find & compile language

static mapping _cache=([]);

Rule.Language `[](string lang)
{
   lang=upper_case(lang);
   Rule.Language l=_cache[lang];
   if (l) return l;
   program cl=::`[]("c"+lang);
   
// if unicode doesn't exist, try without
   if (!cl && sscanf(lang,"%s_UNICODE",lang))
       cl=::`[]("c"+lang);

   if (!cl) { return UNDEFINED; }

   l=_cache[lang]=cl();
   
   return l;
}


