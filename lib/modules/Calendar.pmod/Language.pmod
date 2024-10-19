/* -*- mode: Pike; c-basic-offset: 3; -*- */

#charset iso-8859-1
#pike __REAL_VERSION__

protected string flat(string s)
{
   return replace(lower_case(s),
		  "�������������������������������'- "/1,
		  "aaaaaaeceeeeiiiidnoooooouuuuyty"/1+({""})*3);
}

protected class _language_base
{
   inherit Calendar.Rule.Language;

   protected mapping events_translate=0;

   string translate_event(string name)
   {
      if (events_translate) return events_translate[name]||name;
      return name;
   }
}

protected string roman_number(int m)
{
  if      (m<0)      return "["+m+"]";
  if      (m==0)     return "O";
  if      (m>100000) return "["+m+"]";
  return String.int2roman(m);
}

protected class _ymd_base
{
   inherit _language_base;

   protected mapping(int:string) month_n2s;
   protected mapping(int:string) month_n2ss;
   protected mapping(string:int) month_s2n;
   protected mapping(int:string) week_day_n2s;
   protected mapping(int:string) week_day_n2ss;
   protected mapping(string:int) week_day_s2n;

   string|zero name()
   {
     //  Perform the inverse operation to the `[] method below. We don't
     //  care about normalizing the name among several aliases.
     string cls = function_name(object_program(this));
     if (has_prefix(cls, "c"))
       return lower_case(cls[1..]);
     return 0;
   }

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
   ",Muharram,Safar,Reb�u'l-awwal,Reb�ul-�chir,"
   "Djum�da'l-�la,Djum�da'l-�chira,Redjeb,Shaab�n,Ramad�n,"
   "Schaww�l,Dhu'l-k�ada,Dhu'l-Hiddja"/",";
   array(string) islamic_shortmonths= // help! :)
   ",Muharram,Safar,Reb�u'l-awwal,Reb�ul-�chir,"
   "Djum�da'l-�la,Djum�da'l-�chira,Redjeb,Shaab�n,Ramad�n,"
   "Schaww�l,Dhu'l-k�ada,Dhu'l-Hiddja"/",";
   mapping islamic_backmonth=0;
   array(string) islamic_shortweekdays=
   ",aha,ith,thu,arb,kha,dsc,sab"/",";
   array(string) islamic_weekdays=
   ",ahad,ithnain,thul�th�,arbi�,kham�s,dschuma,sabt"/",";
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

//badi defaults (baha'i calendar)

   string badi_month_name_from_number(int n)
   {
     // Ayy�m-i-H� is not a month but the period of 4-5 days before the last
     // month. it is here at 0 to distinguish it from regular months.
     return ({ "Ayy�m-i-H�", "Bah�", "Jal�l", "Jam�l", "'Azamat", "N�r",
               "Rahmat", "Kalim�t", "Kam�l", "Asm�", "'Izzat", "Mash�yyat",
               "'Ilm", "Qudrat", "Qawl", "Mas�'il", "Sharaf", "Sult�n", "Mulk",
               "'Al�" })[n];
   }

   string badi_month_shortname_from_number(int n)
   {
     // i have no idea how to abbreviate these, am just guessing here
     return ({ "Ah", "Bh", "Jl", "Jm", "Az", "Nr", "Rh", "Kl", "Km", "Am", "Iz",
                  "Msh", "Ilm", "Qd", "Qw", "Ms", "Shr", "Sl", "Ml", "Al"})[n];
   }

   int badi_month_number_from_name(string n)
   {
     return ([ "Bah�":1,       "Baha":1,       "Bh":1,      "Bh":1,
               "Jal�l":2,      "Jalal":2,      "Jll":2,     "Jl":2,
               "Jam�l":3,      "Jamal":3,      "Jml":3,     "Jm":3,
               "'Azamat":4,    "Azamat":4,     "Azmt":4,    "Az":4,
               "N�r":5,        "Nur":5,        "Nr":5,      "Nr":5,
               "Rahmat":6,     "Rahmat":6,     "Rhmt":6,    "Rh":6,
               "Kalim�t":7,    "Kalimat":7,    "Klmt":7,    "Kl":7,
               "Kam�l":8,      "Kamal":8,      "Kml":8,     "Km":8,
               "Asm�":9,       "Asma":9,       "Am":9,      "Am":9,
               "'Izzat":10,    "Izzat":10,     "Izzt":10,   "Iz":10,
               "Mash�yyat":11, "Mashiyyat":11, "Mshyyt":11, "Msh":11,
               "'Ilm":12,      "Ilm":12,       "Ilm":12,    "Ilm":12,
               "Qudrat":13,    "Qudrat":13,    "Qdrt":13,   "Qd":13,
               "Qawl":14,      "Qawl":14,      "Qwl":14,    "Qw":14,
               "Mas�'il":15,   "Masail":15,    "Msl":15,    "Ms":15,
               "Sharaf":16,    "Sharaf":16,    "Shrf":16,   "Shr":16,
               "Sult�n":17,    "Sultan":17,    "Sltn":17,   "Sl":17,
               "Mulk":18,      "Mulk":18,      "Mlk":18,    "Ml":18,
               "'Al�":19,      "Ala":19,       "Al":19,     "Al":19,
             ])[n];
   }

   string badi_month_day_name_from_number(int n)
   {
     return sprintf("%d", (n>19?n-19:n));
   }

   string badi_month_day_longname_from_number(int n)
   {
     // month day names are the same as month names
     array names= ({ "Bah�", "Jal�l", "Jam�l", "'Azamat", "N�r", "Rahmat",
                     "Kalim�t", "Kam�l", "Asm�", "'Izzat", "Mash�yyat", "'Ilm",
                     "Qudrat", "Qawl", "Mas�'il", "Sharaf", "Sult�n", "Mulk",
                     "'Al�", "", "", "", "", "" });
     // not sure if the number should be included here
     return sprintf("%d (%s)", (n>19?n-19:n), names[n-1]);
   }

   string badi_week_day_shortname_from_number(int n)
   {
     // i have no idea how to abbreviate these, am just guessing here
     // see badi_week_day_number_from_name for more guesses
     return ({ 0,  "Jl", "Jm", "Km", "Fd", "Id",
                   "Ij", "Iq" })[n];
   }

   int badi_week_day_number_from_name(string n)
   {
     return([ "Jal�l":1,    "Jalal":1,    "Jal":1, "Jl":1, "Jll":1,
              "Jam�l":2,    "Jamal":2,    "Jam":2, "Jm":2, "Jml":2,
              "Kam�l":3,    "Kamal":3,    "Kam":3, "Km":3, "Kml":3,
              "Fid�l":4,    "Fidal":4,    "Fid":4, "Fd":4, "Fdl":4,
              "'Id�l":5,    "Idal":5,     "'Id":5, "Id":5, "'Idl":5,
              "Istijl�l":6, "Istijlal":6, "Itj":6, "Ij":6, "Istjll":6,
              "Istiql�l":7, "Istiqlal":7, "Itq":7, "Iq":7, "Istqll":7,
            ])[n];
   }

   string badi_week_day_name_from_number(int n)
   {
     return({ 0,  "Jal�l", "Jam�l", "Kam�l", "Fid�l", "'Id�l",
                  "Istijl�l", "Istiql�l" })[n];
   }

   string badi_year_name_from_number(int y)
   {
     array vahid=({ "Alif", "B�", "Ab", "D�l", "B�b", "V�v", "Abad", "J�d",
                    "Bah�", "Hubb", "Bahh�j", "Jav�b", "Ahad", "Vahh�b",
                    "Vid�d", "Bad�", "Bah�", "Abh�", "V�hid" });
     if (y<1)
       return sprintf("%d BB",1-y);                // ? before Baha'i?
     return sprintf("%d BE (%s)", y, vahid[y%19-1]); // Baha'i Era
     // V�hid is a cycle of 19years with each year having a name.
     // there is also the period of Kull-i-Shay which is 19 cycles of V�hid
   }

   int badi_year_number_from_name(string name)
   {
      mapping vahid=([ "Alif":1,    "Alif":1,
                       "B�":2,      "Ba":2,
                       "Ab":3,      "Ab":3,
                       "D�l":4,     "Dal":4,
                       "B�b":5,     "Bab":5,
                       "V�v":6,     "Vav":6,
                       "Abad":7,    "Abad":7,
                       "J�d":8,     "Jad":8,
                       "Bah�":9,    "Baha":9,
                       "Hubb":10,   "Hubb":10,
                       "Bahh�j":11, "Bahhaj":11,
                       "Jav�b":12,  "Javab":12,
                       "Ahad":13,   "Ahad":13,
                       "Vahh�b":14, "Vahhab":14,
                       "Vid�d":15,  "Vidad":15,
                       "Bad�":16,   "Badi":16,
                       "Bah�":17,   "Bahi":17,
                       "Abh�":18,   "Abha":18,
                       "V�hid":19,  "Vahid":19,
                    ]);

      if(vahid[name])
        return vahid[name];

      int y;
      string x;
      if (sscanf(name,"%*s%d%*[ ]B%[BE]%*s",y,x)==1 || x=="")
	 return y>=0?y:y+1; // "-1" == integer year 0
      if(stringp(x))
        x-=" ";
      switch (x)
      {
	 case "E": return y;
	 case "B": return -y+1;
	 default:
	    error("Can't understand year.\n");
      }
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

class cISO
{
   inherit _ymd_base;

   constant month_names=
   ({"January","February","March","April","May","June","July","August",
     "September","October","November","December"});

   constant week_day_names=
   ({"Monday","Tuesday","Wednesday","Thursday","Friday","Saturday","Sunday"});

   protected void create()
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
constant cENGLISH=cISO;
constant cENG=cISO;
constant cEN=cISO;

// swedish (note: all name as cLANG where LANG is in caps)

class cSWEDISH
{
   inherit _ymd_base;

   protected private constant month_names=
   ({"januari","februari","mars","april","maj","juni","juli","augusti",
     "september","oktober","november","december"});

   protected private constant week_day_names=
   ({"m�ndag","tisdag","onsdag","torsdag",
     "fredag","l�rdag","s�ndag"});

   protected mapping events_translate=
   ([
      "New Year's Day":		"Ny�rsdagen",
      "Epiphany":		"Trettondag jul",
      "King's Nameday":		"H K M Konungens namnsdag",
      "Candlemas":		"Kyndelsm�ssodagen",
      "St. Valentine":		"Alla hj�rtans dag",
      "Int. Women's Day":	"Internationella kvinnodagen",
      "Crown Princess' Nameday":"H K M Kronprinsessans namnsdag",
      "Waffle Day":		"V�ffeldagen",
      "Annunciation":		"Marie beb�delsedag",
      "Labor Day":		"F�rsta maj",
      "Sweden's Flag Day":	"Svenska flaggans dag",
      "National Day":		"Nationaldagen",
      "St. John the Baptist":	"Johannes D�pares dag",
      "Crown Princess' Birthday":"H K M Kronprinsessans f�delsedag",
      "Queen's Nameday":	"H K M Drottningens namnsdag",
      "UN Day":			"FN-dagen",
      "All saints Day":		"Allhelgonadagen",
      "King's Nameday":		"H K M Konungens namnsdag",
      "King's Birthday":	"H K M Konungens f�delsedag",
      "St. Lucy":		"Luciadagen",
      "Queen's Birthday":	"H K M Drottningens f�delsedag",
      "Christmas Eve":		"Julafton",
      "Christmas Day":		"Juldagen",
      "St. Stephen":		"Annandagen",
      "New Year's Eve":		"Ny�rsafton",
      "Midsummer's Eve":	"Midsommarafton",
      "Midsummer's Day":	"Midsommardagen",
      "All Saints Day":		"Allhelgonadagen",

      "Fat Tuesday":		"Fettisdagen",
      "Palm Sunday":		"Palms�ndagen",
      "Good Friday":		"L�ngfredagen",
      "Easter Eve":		"P�skafton",
      "Easter":			"P�skdagen",
      "Easter Monday":		"Annandag p�sk",
      "Ascension":		"Kristi himmelsf�rd",
      "Pentecost Eve":		"Pingstafton",
      "Pentecost":		"Pingst",
      "Pentecost Monday":	"Annandag pingst",
      "Advent 1":		"F�rsta advent",
      "Advent 2":		"Andra advent",
      "Advent 3":		"Tredje advent",
      "Advent 4":		"Fj�rde advent",
      "Mother's Day":		"Mors dag",
      "Father's Day":		"Fars dag",

      "Summer Solstice":	"Sommarsolst�nd",
      "Winter Solstice":	"Vintersolst�nd",
      "Spring Equinox":		"V�rdagj�mning",
      "Autumn Equinox":		"H�stdagj�mning",

// not translated:
//	"Halloween"
//	"Alla helgons dag"
//	"Valborgsm�ssoafton"
   ]);

   protected void create()
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
constant cSE_SV=cSWEDISH;
constant cSV=cSWEDISH;
constant cSWE=cSWEDISH;

// austrian
// source: Martin Baehr <mbaehr@email.archlab.tuwien.ac.at>

class cAUSTRIAN
{
   inherit _ymd_base;

   protected private constant month_names=
      ({"j�nner","feber","m�rz","april","mai","juni","juli","august",
        "september","oktober","november","dezember"});

   protected private constant week_day_names=
      ({"montag","dienstag","mittwoch","donnerstag",
        "freitag","samstag","sonntag"});

   protected void create()
   {
      SETUPSTUFF;
   }
}
constant cDE_AT=cAUSTRIAN; // this is a german dialect, appearantly

// Welsh
// source: book

class cWELSH
{
   inherit _ymd_base;

   protected private constant month_names=
   ({"ionawr","chwefror","mawrth","ebrill","mai","mehefin",
     "gorffenaf","awst","medi","hydref","tachwedd","rhagfyr"});

   protected private constant week_day_names=
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

   protected void create()
   {
      SETUPSTUFF;
   }
}
constant cCY=cWELSH;
constant cCYM=cWELSH;

// Spanish
// Julio C�sar G�zquez <jgazquez@dld.net>

class cSPANISH
{
   inherit _ymd_base;

   protected private constant month_names=
   ({"enero","febrero","marzo","abril","mayo","junio",
     "julio","agosto","septiembre","octubre","noviembre","diciembre"});

   protected private constant week_day_names=
   ({"lunes","martes","mi�rcoles","jueves",
     "viernes","s�bado","domingo"});

// contains argentina for now
   protected mapping events_translate=
   ([
      "Epiphany":"D�a de Reyes", // Epifania
      "Malvinas Day":"D�a de las Malvinas",
      "Labor Day":"Aniversario de la Revoluci�n",
      "Soberany's Day":"D�a de la soberania",
      "Flag's Day":"D�a de la bandera",
      "Independence Day":"D�a de la independencia",
      "Assumption Day":"D�a de la asunci�n", // ?
      "Gral San Mart�n decease":
      "Aniversario del fallecimiento del Gral. San Martin",
      "Race's Day":"D�a de la Raza",
      "All Saints Day":"D�a de todos los santos",
      "Immaculate Conception":"Inmaculada Concepci�n",
      "Christmas Day":"Natividad del Se�or",
      "New Year's Day":"A�o Nuevo",
      "Holy Thursday":"Jueves Santo",
      "Good Friday":"Viernes Santo",
      "Holy Saturday":"S�bado de gloria",
      "Easter":"Domingo de resurrecci�n",
      "Corpus Christi":"Corpus Christi"
   ]);

   protected void create()
   {
      SETUPSTUFF;
   }
}
constant cES=cSPANISH;
constant cSPA=cSPANISH;

// portugese
// source: S�rgio Ara�jo <sergio@projecto-oasis.cx>

class cPORTUGESE
{
   inherit _ymd_base;

   protected private constant month_names=
   ({
      "Janeiro",
      "Fevereiro",
      "Mar�o",
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

   protected private constant week_day_names=
   ({
      "Segunda-feira", // -feira is removed for the short version
      "Ter�a-feira",   // don't know how it's used
      "Quarta-feira",
      "Quinta-feira",
      "Sexta-feira",
      "S�bado",
      "Domingo",
   });

// contains argentina for now
   protected mapping events_translate=
   ([
      "New Year's Day":"Ano Novo",
      "Good Friday":"Sexta-Feira Santa",
      "Liberty Day":"Dia da Liberdade",
      "Labor Day":"Dia do Trabalhador",
      "Portugal Day":"Dia de Portugal",
      "Corpus Christi":"Corpo de Deus",
      "Assumption Day":"Assun��o",
      "Republic Day":"Implanta��o da Rep�blica",
      "All Saints Day":"Todos-os-Santos",
      "Restoration of the Independence":"Restaura��o da Independ�ncia",
      "Immaculate Conception":"Imaculada Concei��o",
      "Christmas":"Natal"
   ]);

   protected void create()
   {
      SETUPSTUFF;
   }
}
constant cPT=cPORTUGESE; // Portugese (Brasil)
constant cPOR=cPORTUGESE;

// Hungarian
// Csongor Fagyal <concept@conceptonline.hu>

constant cHU=cHUNGARIAN;
constant cHUN=cHUNGARIAN;
class cHUNGARIAN
{
   inherit _ymd_base;

   protected private constant month_names=
   ({"Janu�r","Febru�r","M�rcius","�prilis","M�jus","J�nius",
     "J�lius","August","September","October","November","December"});

   protected private constant week_day_names=
   ({"H�tfo","Kedd","Szerda","Cs�t�rtk�k","P�ntek","Szombat","Vas�rnap"});

// contains argentina for now
   protected mapping events_translate=
   ([
      "New Year's Day":"�b �v �nnepe",
      "1848 Revolution Day":"Az 'Az 1848-as Forradalom Napja",
      "Holiday of Labor":"A munka �nnepe",
      "Constitution Day":"Az alkotm�ny �nnepe",
      "'56 Revolution Day":"Az '56-os Forradalom �nnepe",
      "Easter":"H�sv�t",
      "Easter monday":"H�sv�t",
      "Whitsunday":"P�nk�sd",
      "Whitmonday":"P�nk�sd",
      "Christmas":"Christmas",
   ]);

   protected void create()
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

   protected array(string) month_names=
   ({"Ianuarius", "Februarius", "Martius", "Aprilis", "Maius", "Iunius",
     "Iulius", "Augustus", "September", "October", "November", "December" });

   protected private constant week_day_names=
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

   protected void create()
   {
      SETUPSTUFF;
   }
}

// Roman latin
// source: calendar FAQ + book

class cROMAN
{
   inherit cLATIN;

   protected array(string) month_names=
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

   protected private constant month_names=
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

   protected private constant week_day_names=
   ({
      "ataasinngorneq",
      "marlunngorneq",
      "pingasunngorneq",
      "sisamanngorneq",
      "tallimanngorneq",
      "arfininngorneq",
      "sabaat",
   });

   protected void create() { SETUPSTUFF; }
}


// source: anonymous unix locale file

constant cIS=cICELANDIC; // Icelandic
constant cISL=cICELANDIC;
class cICELANDIC
{
   inherit _ymd_base;

   protected private constant month_names=
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

   protected private constant week_day_names=
   ({
      "Manudagur",
      "Tridjudagur",
      "Midvikudagur",
      "Fimmtudagur",
      "F�studagur",
      "Laugardagur",
      "Sunnudagur",
   });

   protected void create() { SETUPSTUFF; }
}


// source: anonymous unix locale file

constant cFA=cPERSIAN; // Persian (Iran)
constant cFAS=cPERSIAN;
class cPERSIAN
{
   inherit _ymd_base;

   protected private constant month_names=
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

   protected private constant week_day_names=
   ({
      "dwsnbh",
      "shzsnbh",
      "tharsnbh",
      "pngzsnbh",
      "gmeh",
      "snbh",
      "ykzsnbh",           // <yf><kf><zwnj><sn><n+><b+><h+>
   });

   protected void create() { SETUPSTUFF; }
}


// source: anonymous unix locale file

constant cAF=cAFRIKAANS; // Afrikaans (South Africa)
constant cAFR=cAFRIKAANS;
class cAFRIKAANS
{
   inherit _ymd_base;

   protected private constant month_names=
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

   protected private constant week_day_names=
   ({
      "Maandag",
      "Dinsdag",
      "Woensdag",
      "Donderdag",
      "Vrydag",
      "Saterdag",
      "Sondag",
   });

   protected void create() { SETUPSTUFF; }
}


// source: anonymous unix locale file

constant cGA=cIRISH; // Irish Gaelic
constant cGLE=cIRISH;
class cIRISH
{
   inherit _ymd_base;

   protected private constant month_names=
   ({
      "Ean�ir",
      "Feabhra",
      "M�rta",
      "Aibre�n",
      "M� na Bealtaine",
      "Meith",
      "I�il",
      "L�nasa",
      "Me�n F�mhair",
      "Deireadh F�mhair",
      "M� na Samhna",
      "M� na Nollag",
   });

   protected private constant short_month_names=
   ({
      "Ean","Fea","M�r","Aib","Bea","Mei","I�i","L�n","MF�","DF�","Sam","Nol"
   });

   protected private constant week_day_names=
   ({
      "D� Luain",
      "D� M�irt",
      "D� C�adaoin",
      "D�ardaoin",
      "D� hAoine",
      "D� Sathairn",
      "D� Domhnaigh",
   });

   protected private constant short_week_day_names=
   ({
      "Lua","Mai","C�a","D�a","Aoi","Sat", "Dom",
   });

   protected void create() { SETUPSTUFF2; }
}


// source: anonymous unix locale file

constant cEU=cBASQUE; // Basque (Spain)
constant cEUS=cBASQUE;
class cBASQUE
{
   inherit _ymd_base;

   protected private constant month_names=
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

   protected private constant week_day_names=
   ({
      "astelehena",
      "asteartea",
      "asteazkena",
      "osteguna",
      "ostirala",
      "larunbata",
      "igandea",
   });

   protected void create() { SETUPSTUFF; }
}


// source: anonymous unix locale file

constant cNO=cNORWEGIAN; // Norwegian
constant cNOR=cNORWEGIAN;
class cNORWEGIAN
{
   inherit _ymd_base;

   protected private constant month_names=
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

   protected private constant week_day_names=
   ({
      "mandag",
      "tirsdag",
      "onsdag",
      "torsdag",
      "fredag",
      "l�rdag",
      "s�ndag",
   });

   protected void create() { SETUPSTUFF; }
}


// source: anonymous unix locale file



constant cNL=cDUTCH; // Dutch
constant cNLD=cDUTCH;
class cDUTCH
{
   inherit _ymd_base;

   protected private constant month_names=
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

   protected private constant week_day_names=
   ({
      "maandag",
      "dinsdag",
      "woensdag",
      "donderdag",
      "vrijdag",
      "zaterdag",
      "zondag",
   });

   protected void create() { SETUPSTUFF; }
}


// source: anonymous unix locale file

constant cPL=cPOLISH; // Polish
constant cPOL=cPOLISH;
class cPOLISH
{
   inherit _ymd_base;

   protected private constant month_names=
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

   protected private constant week_day_names=
   ({
      "poniedzialek",      // <p><o><n><i><e><d><z><i><a><l/><e><k>
      "wtorek",
      "sroda",
      "czwartek",
      "piatek",
      "sobota",
      "niedziela",
   });

   protected void create() { SETUPSTUFF; }
}

class cPOLISH_UNICODE
{
   inherit _ymd_base;

   protected private constant month_names=
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

   protected private constant week_day_names=
   ({
      "poniedzia\x0142""ek",      // <p><o><n><i><e><d><z><i><a><l/><e><k>
      "wtorek",
      "sroda",
      "czwartek",
      "piatek",
      "sobota",
      "niedziela",
   });

   protected void create() { SETUPSTUFF; }
}


// source: Cile Ekici (@ Roxen IS)

constant cTR=cTURKISH; // Turkish
constant cTUR=cTURKISH; // Turkish
class cTURKISH
{
   inherit _ymd_base;

   protected private constant month_names=
   ({
      "Ocak",
      "Subat",
      "Mart",
      "Nisan",
      "Mayis",
      "Haziran",
      "Temmuz",
      "Agustos",
      "Eyl�l",
      "Ekim",
      "Kasim",
      "Aralik",
   });

   protected private constant week_day_names=
   ({
      "Pazartesi",
      "Sali",              // <S><a><l><i.>
      "�arsamba",
      "Persembe",
      "Cuma",
      "Cumartesi",
      "Pazar",
   });

   protected void create() { SETUPSTUFF; }
}

constant cTR_UNICODE=cTURKISH_UNICODE; // Turkish
constant cTUR_UNICODE=cTURKISH_UNICODE; // Turkish
class cTURKISH_UNICODE
{
   inherit _ymd_base;

   protected private constant month_names=
   ({
      "Ocak",
      "\015e""ubat",    // S-cedilla
      "Mart",
      "Nisan",
      "May\x0131""s",   // i-no-dot
      "Haziran",
      "Temmuz",
      "A\x01e5""ustos", // line over g
      "Eyl�l",
      "Ekim",
      "Kas\x0131""m",   // i-no-dot
      "Aral\x0131""k",  // i-no-dot
   });

   protected private constant week_day_names=
   ({
      "Pazartesi",
      "Sal\x0131",      // <S><a><l><i.> i without dot
      "�arsamba",
      "Per\x015f""embe",// s-cedilla
      "Cuma",
      "Cumartesi",
      "Pazar",
   });

   protected void create() { SETUPSTUFF; }
}

// source: anonymous unix locale file



constant cDE=cGERMAN; // German
constant cDEU=cGERMAN;
class cGERMAN
{
   inherit _ymd_base;

   protected private constant month_names=
   ({
      "Januar",
      "Februar",
      "M�rz",
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

   protected private constant week_day_names=
   ({
      "Montag",
      "Dienstag",
      "Mittwoch",
      "Donnerstag",
      "Freitag",
      "Samstag",
      "Sonntag",
   });

   protected void create() { SETUPSTUFF; }
}


// source: anonymous unix locale file

constant cLV=cLATVIAN; // Latvian
constant cLAV=cLATVIAN;
class cLATVIAN
{
   inherit _ymd_base;

   protected private constant month_names=
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

   protected private constant week_day_names=
   ({
      "pirmdiena",
      "otrdiena",
      "tresdiena",
      "ceturtdiena",
      "piektdiena",
      "sestdiena",
      "svetdiena",         // <s><v><e-><t><d><i><e><n><a>
   });

   protected void create() { SETUPSTUFF; }
}

constant cLV_UNICODE=cLATVIAN_UNICODE;
constant cLAV_UNICODE=cLATVIAN_UNICODE;
class cLATVIAN_UNICODE
{
   inherit _ymd_base;

   protected private constant month_names=
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

   protected private constant week_day_names=
   ({
      "pirmdiena",
      "otrdiena",
      "tresdiena",
      "ceturtdiena",
      "piektdiena",
      "sestdiena",
      "sv\x0113""tdiena",         // <s><v><e-><t><d><i><e><n><a>
   });

   protected void create() { SETUPSTUFF; }
}


// source: anonymous unix locale file

constant cFI=cFINNISH; // Finnish
constant cFIN=cFINNISH;
class cFINNISH
{
   inherit _ymd_base;

   protected private constant month_names=
   ({
      "tammikuu",
      "helmikuu",
      "maaliskuu",
      "huhtikuu",
      "toukokuu",
      "kes�kuu",
      "hein�kuu",
      "elokuu",
      "syyskuu",
      "lokakuu",
      "marraskuu",
      "joulukuu",
   });

   protected private constant week_day_names=
   ({
      "maanantai",
      "tiistai",
      "keskiviikko",
      "torstai",
      "perjantai",
      "lauantai",
      "sunnuntai",
   });

   protected void create() { SETUPSTUFF; }
}


// source: anonymous unix locale file

constant cLT=cLITHUANIAN; // Lithuanian
constant cLIT=cLITHUANIAN;
class cLITHUANIAN
{
   inherit _ymd_base;

   protected private constant month_names=
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

   protected private constant week_day_names=
   ({
      "Pirmadienis",
      "Antradienis",
      "Treciadienis",
      "Ketvirtadienis",
      "Penktadienis",
      "Sestadienis",       // <S<><e><s<><t><a><d><i><e><n><i><s>
      "Sekmadienis",
   });

   protected void create() { SETUPSTUFF; }
}

class cLITHUANIAN_UNICODE
{
   inherit _ymd_base;

   protected private constant month_names=
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

   protected private constant week_day_names=
   ({
      "Pirmadienis",
      "Antradienis",
      "Treciadienis",
      "Ketvirtadienis",
      "Penktadienis",
      "\x0160""estadienis",       // <S<><e><s<><t><a><d><i><e><n><i><s>
      "Sekmadienis",
   });

   protected void create() { SETUPSTUFF; }
}


// source: anonymous unix locale file

constant cET=cESTONIAN; // Estonian
constant cEST=cESTONIAN;
class cESTONIAN
{
   inherit _ymd_base;

   protected constant month_names=
   ({
      "jaanuar",
      "veebruar",
      "m�rts",
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

   protected constant week_day_names=
   ({
      "esmasp�ev",
      "teisip�ev",
      "kolmap�ev",
      "neljap�ev",
      "reede",
      "laup�ev",
      "p�hap�ev",          // <p><u:><h><a><p><a:><e><v>
   });

   protected void create() { SETUPSTUFF; }
}

constant cGL=cGALICIAN; // Galician (Spain)
constant cGLG=cGALICIAN;
class cGALICIAN
{
   inherit _ymd_base;

   protected private constant month_names=
   ({
      "Xaneiro",
      "Febreiro",
      "Marzo",
      "Abril",
      "Maio",
      "Xu�o",
      "Xullo",
      "Agosto",
      "Setembro",
      "Outubro",
      "Novembro",
      "Decembro",
   });

   protected private constant week_day_names=
   ({
      "Luns",
      "Martes",
      "M�rcores",
      "Xoves",
      "Venres",
      "Sabado",
      "Domingo",
   });

   protected void create() { SETUPSTUFF; }
}

constant cID=cINDONESIAN;
class cINDONESIAN
{
   inherit _ymd_base;

   protected private constant month_names=
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

   protected private constant week_day_names=
   ({
      "Senin",
      "Selasa",
      "Rabu",
      "Kamis",
      "Jumat",
      "Sabtu",
      "Minggu",
   });

   protected void create() { SETUPSTUFF; }
}

constant cFR=cFRENCH; // French
constant cFRA=cFRENCH;
class cFRENCH
{
   inherit _ymd_base;

   protected private constant month_names=
   ({
      "janvier",
      "f�vrier",
      "mars",
      "avril",
      "mai",
      "juin",
      "juillet",
      "ao�",
      "septembre",
      "octobre",
      "novembre",
      "d�cembre",
   });

   protected private constant week_day_names=
   ({
      "lundi",
      "mardi",
      "mercredi",
      "jeudi",
      "vendredi",
      "samedi",
      "dimanche",
   });

   protected void create() { SETUPSTUFF; }
}

constant cIT=cITALIAN; // Italian
constant cITA=cITALIAN;
class cITALIAN
{
   inherit _ymd_base;

   protected private constant month_names=
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

   protected private constant week_day_names=
   ({
      "lunedi",    // swizz italian: "luned�" - should I care?
      "martedi",
      "mercoledi",
      "giovedi",
      "venerdi",
      "sabato",
      "domenica",
   });

   protected void create() { SETUPSTUFF; }
}

constant cCA=cCATALAN; // Catalan (Catalonia)
constant cCAT=cCATALAN;
class cCATALAN
{
   inherit _ymd_base;

   protected private constant month_names=
   ({
      "gener",
      "febrer",
      "mar�",
      "abril",
      "maig",
      "juny",
      "juliol",
      "agost",
      "setembre",
      "octubre",
      "novembre",
      "desembre",
   });

   protected private constant week_day_names=
   ({
      "dilluns",
      "dimarts",
      "dimecres",
      "dijous",
      "divendres",
      "dissabte",
      "diumenge",
   });

   protected void create() { SETUPSTUFF; }
}

constant cSL=cSLOVENIAN; // Slovenian
constant cSLV=cSLOVENIAN;
class cSLOVENIAN
{
   inherit _ymd_base;

   protected constant month_names=
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

   protected constant week_day_names=
   ({
      "ponedeljek",
      "torek",
      "sreda",
      "cetrtek",    // <c<><e><t><r><t><e><k>
      "petek",
      "sobota",
      "nedelja",
   });

   protected void create() { SETUPSTUFF; }
}

class cSLOVENIAN_UNICODE
{
   inherit cSLOVENIAN;

   protected constant week_day_names=
   ({
      "ponedeljek",
      "torek",
      "sreda",
      "\x010d""etrtek",    // <c<><e><t><r><t><e><k>
      "petek",
      "sobota",
      "nedelja",
   });

   protected void create() { SETUPSTUFF; }
}

constant cFO=cFAROESE; // Faroese
constant cFAO=cFAROESE;
class cFAROESE
{
   inherit _ymd_base;

   protected private constant month_names=
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

   protected private constant week_day_names=
   ({
      "manadagur",
      "t�sdagur",
      "mikudagur",
      "hosdagur",
      "friggjadagur",
      "leygardagur",
      "sunnudagur",
   });

   protected void create() { SETUPSTUFF; }
}

constant cRO=cROMANIAN; // Romanian
constant cRON=cROMANIAN;
class cROMANIAN
{
   inherit _ymd_base;

   protected constant month_names=
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

   protected constant week_day_names=
   ({
      "Luni",
      "Marti",             // <M><A><R><T,><I>
      "Miercuri",
      "Joi",
      "Vineri",
      "Simbata",                // <S><I/>><M><B><A(><T><A(>
      "Duminica",          // <D><U><M><I><N><I><C><A(>
   });

   protected void create() { SETUPSTUFF; }
}

class cROMANIAN_UNICODE
{
   inherit cROMANIAN;

   protected constant week_day_names=
   ({
      "Luni",
      "Mar\x0163""i",             // <M><A><R><T,><I>
      "Miercuri",
      "Joi",
      "Vineri",
      "S�mb\x0103""t\x0103",                // <S><I/>><M><B><A(><T><A(>
      "Duminic\x0103",          // <D><U><M><I><N><I><C><A(>
   });

   protected void create() { SETUPSTUFF; }
}

constant cHR=cCROATIAN; // Croatian
constant cHRV=cCROATIAN; // Croatian
class cCROATIAN
{
   inherit _ymd_base;

   protected private constant month_names=
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

   protected private constant week_day_names=
   ({
      "Ponedjeljak",
      "Utorak",
      "Srijeda",
      "Cetvrtak",          // <C<><e><t><v><r><t><a><k>
      "Petak",
      "Subota",
      "Nedjelja",
   });

   protected void create() { SETUPSTUFF; }
}

class cCROATIAN_UNICODE
{
   inherit _ymd_base;

   protected private constant month_names=
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

   protected private constant week_day_names=
   ({
      "Ponedjeljak",
      "Utorak",
      "Srijeda",
      "\414etvrtak",
      "Petak",
      "Subota",
      "Nedjelja",
   });

   protected void create() { SETUPSTUFF; }
}

constant cDA=cDANISH; // Danish
constant cDAN=cDANISH;
class cDANISH
{
   inherit _ymd_base;

   protected private constant month_names=
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

   protected private constant week_day_names=
   ({
      "mandag",
      "tirsdag",
      "onsdag",
      "torsdag",
      "fredag",
      "l�rdag",
      "s�ndag",
   });

   protected mapping events_translate=
   ([
      "2nd day of Christmas"		    :"2. juledag",
      "Ascension"			    :"Kristi himmelfart",
      "Birthday of Queen Margrethe II"	    :"Dronning Margrethes f�dselsdag",
      "Carnival"			    :"Fastelavn",
      "Christmas Day"			    :"1. juledag",
      "Christmas Eve"			    :"Juleaften",
      "Constitution Day"		    :"Grundlovsdag",
      "Easter Monday"			    :"2. p�skedag",
      "Easter"                              :"P�ske",
      "Good Friday"			    :"Langfredag",
      "Great Prayer Day"		    :"Store bededag",
      "Holy Thursday"			    :"Sk�rtorsdag",
      "Labor Day"			    :"1. Maj",
      "New Year's Day"			    :"Nyt�rsdag",
      "New Year's Eve"			    :"Nyt�rsaften",
      "Palm Sunday"			    :"Palme s�ndag",
      "Pentecost Monday"		    :"2. Pinsedag",
      "Pentecost"			    :"Pinsedag",
      "Prince Fredrik's Birthday"	    :"Kronprins Fredriks f�dselsdag",
      "Epiphany"			    :"Hellig trekongers dag",
      "Epiphany Eve" 			    :"Hellig trekongers aften",
   ]);

   protected void create() { SETUPSTUFF; }
}

constant cSR=cSERBIAN; // Serbian (Yugoslavia)
constant cSRP=cSERBIAN;
class cSERBIAN
{
   inherit _ymd_base;

   protected constant month_names=
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

   protected constant week_day_names=
   ({
      "ponedeljak",
      "utorak",
      "sreda",
      "cetvrtak",
      "petak",
      "subota",
      "nedelja",
   });

   protected void create() { SETUPSTUFF; }
}


class cSERBIAN_UNICODE
{
   inherit cSERBIAN;

   protected constant week_day_names=
   ({
      "ponedeljak",
      "utorak",
      "sreda",
      "\415etvrtak",
      "petak",
      "subota",
      "nedelja",
   });

   protected void create() { SETUPSTUFF; }
}


constant cEL=cGREEK_UNICODE; //Greek Ellinika
constant cGR=cGREEK_UNICODE;
class cGREEK_UNICODE
{
   inherit _ymd_base;

   private constant month_names=
   ({
     "\u0399\u03b1\u03bd\u03bf\u03c5\u03ac\u03c1\u03b9\u03bf\u03c2",
     "\u03a6\u03b5\u03b2\u03c1\u03bf\u03c5\u03ac\u03c1\u03b9\u03bf\u03c2",
     "\u039c\u03ac\u03c1\u03c4\u03b9\u03bf\u03c2",
     "\u0391\u03c0\u03c1\u03af\u03bb\u03b9\u03bf\u03c2",
     "\u039c\u03ac\u03b9\u03bf\u03c2",
     "\u0399\u03bf\u03cd\u03bd\u03b9\u03bf\u03c2",
     "\u0399\u03bf\u03cd\u03bb\u03b9\u03bf\u03c2",
     "\u0391\u03cd\u03b3\u03bf\u03c5\u03c3\u03c4\u03bf\u03c2",
     "\u03a3\u03b5\u03c0\u03c4\u03ad\u03bc\u03b2\u03c1\u03b9\u03bf\u03c2",
     "\u039f\u03ba\u03c4\u03ce\u03b2\u03c1\u03b9\u03bf\u03c2",
     "\u039d\u03bf\u03ad\u03bc\u03b2\u03c1\u03b9\u03bf\u03c2",
     "\u0394\u03b5\u03ba\u03ad\u03bc\u03b2\u03c1\u03b9\u03bf\u03c2",
   });

   private constant week_day_names=
   ({
     "\u0394\u03b5\u03c5\u03c4\u03ad\u03c1\u03b1",
     "\u03a4\u03c1\u03af\u03c4\u03b7",
     "\u03a4\u03b5\u03c4\u03ac\u03c1\u03c4\u03b7",
     "\u03a0\u03ad\u03bc\u03c0\u03c4\u03b7",
     "\u03a0\u03b1\u03c1\u03b1\u03c3\u03ba\u03b5\u03c5\u03ae",
     "\u03a3\u03ac\u03b2\u03b2\u03b1\u03c4\u03bf",
     "\u039a\u03c5\u03c1\u03b9\u03b1\u03ba\u03ae",
   });

   protected void create() { SETUPSTUFF; }
}


// ----------------------------------------------------------------

// find & compile language

protected mapping _cache=([]);

protected Calendar.Rule.Language `[](string lang)
{
   lang=upper_case(lang);
   Calendar.Rule.Language l=_cache[lang];
   if (l) return l;
   program cl=::`[]("c"+lang);

// if unicode doesn't exist, try without
   if (!cl && sscanf(lang,"%s_UNICODE",lang))
       cl=::`[]("c"+lang);

   if (!cl) { return UNDEFINED; }

   l=_cache[lang]=cl();

   return l;
}
