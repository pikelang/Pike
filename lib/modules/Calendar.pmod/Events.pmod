import ".";
import Event;

program|Event `[](string s)
{
   return ::`[](s) || magic_event(s);
}
function `-> = `[];

Event|Namedays|Magic_Index magic_event(string s)
{
   s=replace(s,"_"," ");
   Event e;
   if ( (e=made_events[s]) ) return e;
   array a=events[s];
   if (a)
      return made_events[s]=a[0](@a[1..]);

   if (sizeof(glob(s+"/*",indices(events))))
      return made_events[s]=Magic_Index(s);
   return ([])[0];
}

mapping made_events=([]);

class Magic_Index
{
   string base;
   void create(string s) { base=s+"/"; }
   Event|Namedays|Magic_Index `[](string s) { return magic_event(base+s); }
   Event|Namedays|Magic_Index `->(string s) { return `[](s); }

   array(string) _indices()
   {
      return map(glob(base+"*",indices(events)),
		 lambda(string s) { return array_sscanf(s,base+"%[^/]")[0]; });
   }

   array(Event|Namedays|Magic_Index) _values()
   {
      return map(_indices(),`[]);
   }

   string _sprintf(int t) 
   { return (t!='O')?0:sprintf("Event index(%O)",base); }
}

#if 1

mapping(string:SuperEvent) made_countries=([]);

SuperEvent country(string name)
{
   if (made_countries[name]) return made_countries[name];

   array z=country_events[name];
   if (!z) return 0;

   array(Event) ze=({});
   mapping(Event:multiset(string)) zf=([]);
// reuse those multisets
   mapping(string:multiset) flags=(["":0,"h":(<"h">),"f":(<"f">)]);
   flags->hf=flags->fh=(<"h","f">);

   foreach (z,string id)
      if (id[0]=='+')
      {
	 Event e=country(id[1..]);
	 if (!e) werror("internal error: Missing country %O, can't inherit\n",
			id[1..]);
	 else
	    ze=e->events|ze, zf=e->flags|zf;
      }
      else if (id[0]=='-')
      {
	 Event e=magic_event(id[1..]);
	 if (!zf[e]) 
	    werror("internal error: Missing event %O, can't remove\n",id[1..]);
	 m_delete(zf,e);
	 ze-=({e});
      }
      else
      {
	 string fs="";
	 sscanf(id,"%s|%s",id,fs);
	 Event e=magic_event(id);
	 if (!e) werror("internal error: Missing event %O\n",id);
	 else ze|=({e});
	 if (fs!="") zf[e]=flags[fs]||mkmultiset( (array)fs );
      }
   return made_countries[name]=SuperEvent(ze,zf);
}
#endif

// ----------------------------------------------------------------
// event definitions
// ----------------------------------------------------------------

#define FIXED(NAME,MD,MN) \
	({Gregorian_Fixed,NAME,MD,MN})
#define FIX_M(NAME,MD,MN,N) \
	({Gregorian_Fixed,NAME,MD,MN,N})

// relative week day (n: 1=first following, 2=2nd, -1=previous, -2=2nd prev)
#define DWDR(NAME,MD,MN,WD,N) \
	({Monthday_Weekday_Relative,NAME,MD,MN,WD,N})

// relative week day, inclusive
#define DWDRI(NAME,MD,MN,WD,N) \
	({Monthday_Weekday_Relative,NAME,MD,MN,WD,N,1})

#define EASTER_REL(NAME,OFFSET) \
	({Easter_Relative,NAME,OFFSET})

#define ORTH_EASTER_REL(NAME,OFFSET) \
	({Orthodox_Easter_Relative,NAME,OFFSET})

#define J_FIXED(NAME,MD,MN) \
	({Julian_Fixed,NAME,MD,MN})
#define J_FIX_M(NAME,MD,MN,N) \
	({Julian_Fixed,NAME,MD,MN,N})

#define DUMMY ({NullEvent})

constant events=([
// ----------------------------------------------------------------
// standard
   "monday":	({Weekday,1}),
   "tuesday":	({Weekday,2}),
   "wednesday":	({Weekday,3}),
   "thursday":	({Weekday,4}),
   "friday":	({Weekday,5}),
   "saturday":	({Weekday,6}),
   "sunday":	({Weekday,7}),

// ----------------------------------------------------------------------
// global events
// ----------------------------------------------------------------------

   "new year":		FIXED("New Year's Day",     1, 1),
   "new year 2d":	FIX_M("New Year",           1, 1, 2),
   "unity":             FIXED("Unity Day",         22, 2),
   "womens":		FIXED("Int. Women's Day",   8, 3),
   "arab league":       FIXED("Arab League Day",   22, 3),
   "anzac":             FIXED("ANZAC Day",         25, 4),
   "labor":		FIXED("Labor Day", 	    1, 5),
   "labor 2":		FIXED("2nd day of Labor",   2, 5),
   "may day":           FIXED("May Day",            1, 5),
   "africa":            FIXED("Africa Day",        25, 5),
   "perseid meteor shower":FIXED("Perseid Meteor Shower",11,8),
   "columbus":          FIXED("Columbus Day",      12,10),
   "un":		FIXED("UN Day",	   	   24,10),
   "halloween":         FIXED("Halloween",         31,10),
   "armistice":         FIXED("Armistice Day",     11,11),
   "family":            FIXED("Family Day",        25,12),
   "kwanzaa":           FIXED("Kwanzaa",           26,12),
   "new years eve":	FIXED("New Year's Eve",    31,12),

   "mardi gras":        EASTER_REL("Mardi Gras",-47),
   "mardi gras 2":      EASTER_REL("Mardi Gras",-46),

   "mothers":          	DWDRI("Mother's Day",	1,5,7,2),
   "fathers":          	DWDRI("Father's Day",	1,6,7,3),

// ----------------------------------------------------------------------
// religious events
// ----------------------------------------------------------------------

// common (catholic) christianity

   "c/circumcision":    FIXED("Feast of the Circumcision",1,1),
   "c/epiphany":	FIXED("Epiphany",        6, 1),
   "c/presentation":	FIXED("Presentation",    2, 2),
   "c/candlemas":       FIXED("Candlemas",       2, 2),
   "c/annunciation":	FIXED("Annunciation",   25, 3),
   "c/transfiguration": FIXED("Transfiguration", 6, 8),
   "c/assumption":      FIXED("Assumption Day", 15, 8),
   "c/nativity of mary":FIXED("Nativity of Mary",8, 9),
   "c/all saints":	FIXED("All Saints Day",  1,11),
   "c/all souls":	FIXED("All Souls Day",   2,11),
   "c/immaculate conception":FIXED("Immaculate Conception",8,12),
   "c/christmas eve":	FIXED("Christmas Eve",  24,12),
   "c/christmas":	FIXED("Christmas Day",  25,12),
   "c/christmas2d":	FIX_M("Christmas",      25,12,2),
   "c/boxing":		FIXED("Boxing Day",     26,12),
   "c/childermas":      FIXED("Childermas",     28,12),

   "c/shrove sunday":   EASTER_REL("Shrove Sunday",   -49),
   "c/shrove monday":   EASTER_REL("Shrove Monday",   -48),
   "c/rose monday":     EASTER_REL("Rose Monday",     -48), // aka
   "c/shrove tuesday":  EASTER_REL("Shrove Tuesday",  -47),
   "c/fat tuesday":     EASTER_REL("Fat Tuesday",     -47), // aka
   "c/ash wednesday":   EASTER_REL("Ash wednesday",   -46),
   "c/palm sunday":     EASTER_REL("Palm Sunday",      -7),
   "c/holy thursday":   EASTER_REL("Holy Thursday",    -3),
   "c/maundy thursday": EASTER_REL("Maundy Thursday",  -3), // aka
   "c/holy friday":     EASTER_REL("Holy Friday",      -2), 
   "c/good friday":     EASTER_REL("Good Friday",      -2), // aka
   "c/holy saturday":   EASTER_REL("Holy Saturday",    -1), 
   "c/easter eve":      EASTER_REL("Easter Eve",       -1), // aka
   "c/easter":          EASTER_REL("Easter",            0),
   "c/easter monday":   EASTER_REL("Easter Monday",     1),
   "c/rogation sunday": EASTER_REL("Rogation Sunday",  35),
   "c/ascension":       EASTER_REL("Ascension",        39),
   "c/pentecost eve":   EASTER_REL("Pentecost Eve",    48),
   "c/pentecost":       EASTER_REL("Pentecost",        49),
   "c/whitsunday":      EASTER_REL("Whitsunday",       49), // aka
   "c/pentecost monday":EASTER_REL("Pentecost Monday",50),
   "c/whitmonday":      EASTER_REL("Whitmonday",       50), // aka
   "c/trinity":         EASTER_REL("Trinity",          56),
   "c/corpus christi":  EASTER_REL("Corpus Christi",   60),

   "c/advent 1":        DWDR ("Advent 1",24,12,7,-4),
   "c/advent 2":        DWDR ("Advent 2",24,12,7,-3),
   "c/advent 3":        DWDR ("Advent 3",24,12,7,-2),
   "c/advent 4":        DWDR ("Advent 4",24,12,7,-1),

// other

   "catholic church/baptism of christ":
                                 FIXED("Feast of the Baptism of Christ",13,1),
   "catholic church/christ the king":
                                 DWDRI("Feast of Christ the King",1,10,7,-1),
   "catholic church/gaurdian angles":
                                 FIXED("Feast of the Gaurdian Angles",2,10),
   "catholic church/transfiguration":FIXED("Transfiguration",6,8),

// orthodox

// I don't know which of these are/will be needed:
   "orthodox/new year":         J_FIXED("Orthodox New Year",1,1),
   "orthodox/circumcision":    	J_FIXED("Feast of the Circumcision",1,1),
   "orthodox/epiphany":		J_FIXED("Epiphany",        6, 1),
   "orthodox/presentation":	J_FIXED("Presentation",    2, 2),
   "orthodox/candlemas":       	J_FIXED("Candlemas",       2, 2),
   "orthodox/annunciation":	J_FIXED("Annunciation",   25, 3),
   "orthodox/transfiguration": 	J_FIXED("Transfiguration", 6, 8),
   "orthodox/assumption":      	J_FIXED("Assumption Day", 15, 8),
   "orthodox/nativity of mary":	J_FIXED("Nativity of Mary",8, 9),
   "orthodox/immaculate conception":J_FIXED("Immaculate Conception",8,12),
   "orthodox/christmas eve":	J_FIXED("Christmas Eve",  24,12),
   "orthodox/christmas":	J_FIXED("Christmas Day",  25,12),
   "orthodox/christmas2d":	J_FIX_M("Christmas",      25,12,2),
   "orthodox/boxing":		J_FIXED("Boxing Day",     26,12),
   "orthodox/childermas": 	J_FIXED("Childermas",     28,12),

   "orthodox/triodon":		ORTH_EASTER_REL("Triodon",-70),
   "orthodox/sat. of souls":	ORTH_EASTER_REL("Sat. of Souls",-57),
   "orthodox/meat fare":	ORTH_EASTER_REL("Meat Fare",-56),
   "orthodox/2nd sat. of souls":ORTH_EASTER_REL("2nd Sat. of Souls",-50),
   "orthodox/lent begins":	ORTH_EASTER_REL("Lent Begins",-48),
   "orthodox/saints/theodore":	ORTH_EASTER_REL("St. Theodore",-43),
   "orthodox/orthodoxy":	ORTH_EASTER_REL("Sunday of Orthodoxy",-42),
   "orthodox/lazarus":		ORTH_EASTER_REL("Saturday of Lazarus",-8),
   "orthodox/palm sunday":	ORTH_EASTER_REL("Palm Sunday",-7),
   "orthodox/good friday":	ORTH_EASTER_REL("Good Friday",-2),
   "orthodox/holy saturday":	ORTH_EASTER_REL("Holy Saturday",-1),
   "orthodox/easter":		ORTH_EASTER_REL("Easter",0),
   "orthodox/easter monday":	ORTH_EASTER_REL("Easter Monday",1),
   "orthodox/ascension":	ORTH_EASTER_REL("Ascension",39),
   "orthodox/sat. of souls":	ORTH_EASTER_REL("Sat. of Souls",48),
   "orthodox/pentecost":	ORTH_EASTER_REL("Pentecost",49),
   "orthodox/pentecost monday":	ORTH_EASTER_REL("Pentecost Monday",50),
   "orthodox/all saints":	ORTH_EASTER_REL("All Saints",56),

   "bahai/ascension of abdul-baha":FIXED("Ascension of 'Abdu'l-Baha",28,11),
   "bahai/ascension of bahaullah":FIXED("Ascension of Baha'u'llah",29,5),
   "bahai/birth of bab":         FIXED("Birth of Bab",20,10),
   "bahai/birth of bahaullah":   FIXED("Birth of Baha'u'llah",12,11),
   "bahai/day of the covenant":  FIXED("Day of the Covenant",27,11),
   "bahai/declaration of bab":   FIXED("Declaration of B\341b",23,5),
   "bahai/festival of ridvan":   FIXED("Festival of Ridv\341n",21,4),
   "bahai/martyrdom of bab":     FIXED("Martyrdom of Bab",9,7),
   "bahai/naw-ruz":              FIXED("Naw-R\372z",21,3),

// temporary
   "saints/basil":      FIXED("St. Basil",1,1),
   "saints/mary mother":FIXED("Mary, Mother of God",1,1),
   "saints/devote":     FIXED("St. Devote",27,1),
   "saints/blaise":     FIXED("St. Blaise",3,2),
   "saints/valentine":	FIXED("St. Valentine",14,2),
   "saints/joseph":	FIXED("St. Joseph",19,3),
   "saints/joseph the worker":FIXED("St. Joseph the Worker",1,5),
   "saints/john the baptist": FIXED("St. John the Baptist",24,6),
   "saints/peter":	FIXED("St. Peter",29,6),
   "saints/paul":	FIXED("St. Paul",29,6),
   "saints/cyril & methodius":FIXED("St Cyril & Methodius",5,7),
   "saints/demetrios":  FIXED("St. Demetrios",26,10),
   "saints/lucy": 	FIXED("St. Lucy",13,12),
   "saints/stephen":	FIXED("St. Stephen",26,12), // aka boxing day
   "saints/patrick":	FIXED("St. Patrick",17,3),
   "saints/david":      FIXED("St. David",1,3),

// ---------------------------------------------------------------------
// historic and political
// ---------------------------------------------------------------------

#if 0
// this should be the julian dates...
   "roman festivals/agonalia":   FIXED("Agonalia",9,1),
   "roman festivals/ambarvalia": FIXED("Ambarvalia",29,5),
   "roman festivals/carmentalia":FIX_M("Carmentalia",11,1,5),
   "roman festivals/cerealia":   FIXED("Cerealia",19,4),
   "roman festivals/equiria":    FIXED("Equiria",14,3),
   "roman festivals/faunalia":   FIXED("Faunalia",13,2),
   "roman festivals/feralia":    FIXED("Feralia",18,2),
   "roman festivals/festival of anna perenna":
                                 FIXED("festival of Anna Perenna",15,3),
   "roman festivals/festival of the lares pr\346stites":
                               FIXED("festival of the Lares Pr\346stites",1,5),
   "roman festivals/floria":     FIXED("Floria",28,4),
   "roman festivals/fools":      FIXED("Feast of Fools",17,2),
   "roman festivals/fordicidia": FIXED("Fordicidia",15,4),
// "roman festivals/fornicalia": February,
   "roman festivals/lemuria":    FIX_M("Lemuria",9,5,5),
   "roman festivals/liberalia":  FIXED("Liberalia",17,3),
   "roman festivals/ludi apollinares":FIXED("Ludi Apollinares",5,7),
   "roman festivals/ludi consualia":FIXED("Ludi Consualia",21,8),
   "roman festivals/ludi martiales":FIXED("Ludi Martiales",12,5),
   "roman festivals/ludi merceruy":FIXED("Ludi Merceruy",15,5),
   "roman festivals/lupercalia": FIXED("Lupercalia",15,2),
   "roman festivals/magalesia":  FIX_M("Magalesia",4,4,7),
   "roman festivals/matralia":   FIXED("Matralia",11,6),
   "roman festivals/matronalia": FIXED("Matronalia",1,3),
   "roman festivals/nemoralia":  FIXED("Nemoralia",13,8),
   "roman festivals/neptunalia": FIXED("Neptunalia",23,7),
   "roman festivals/paganalia":  FIX_M("Paganalia",24,1,3),
   "roman festivals/parentalia": FIX_M("Parentalia",13,2,9),
   "roman festivals/parilia":    FIXED("Parilia",21,4),
   "roman festivals/portunalia": FIXED("Portunalia",17,8),
   "roman festivals/quinquatrus minusculoe":
                                 FIXED("Quinquatrus Minusculoe",13,6),
   "roman festivals/quinquatrus[quinquatria?]":
                                 FIX_M("Quinquatrus[Quinquatria?]",19,3,3),
   "roman festivals/quirinalia": FIXED("Quirinalia",17,2),
   "roman festivals/regifugium": FIXED("Regifugium",24,2),
   "roman festivals/robigalia":  FIXED("Robigalia",25,4),
// "roman festivals/semo sanctus":occurred in June,
   "roman festivals/terminalia": FIXED("Terminalia",23,2),
   "roman festivals/the vestalia":FIXED("The Vestalia",9,6),
   "roman festivals/tubilustrium":FIXED("Tubilustrium",23,3),
   "roman festivals/vinalia":    FIXED("Vinalia",23,4),
   "roman festivals/vinalia rustica":FIXED("Vinalia Rustica",19,8),
   "roman festivals/volcanalia": FIXED("Volcanalia",23,8),
#endif

   "british commonwealth/august bank holiday":
                                 DWDRI("August Bank Holiday",1,8,1,1),
   "british commonwealth/empire":FIXED("Empire Day",24,5),
   "british commonwealth/prince of wales":
                                 FIXED("Prince of Wales' Birthday",14,11),
   "british commonwealth/queens":FIXED("Queen's Birthday",4,6),

   "caribbean/caricom":          DWDRI("CARICOM Day",1,7,1,1),
   "caribbean/emancipation":     DWDRI("Emancipation Day",1,8,1,1),
   "caribbean/schoelcher":       FIXED("Schoelcher Day",21,7),

// chinese calendar

   "chinese/new year":	         DUMMY,

// ----------------------------------------------------------------
// verified
// ----------------------------------------------------------------

   "argentina/revolution":	FIXED("Revolution Anniversary",25,5),
   "argentina/soberanys":	FIXED("Soberany's Day",10,6),
   "argentina/flag":		FIXED("Flag's Day",20,6),
   "argentina/independence":    FIXED("Independence Day",9,7),
   "argentina/gral san martin": FIXED("Gral San Martín decease",17,8),
   "argentina/race":            FIXED("Race's Day",12,10),
   "argentina/malvinas":         FIXED("Malvinas Day",2,4),  

   "sweden/kings birthday":     FIXED("King's Birthday",30,4),
   "sweden/kings nameday 2":    FIXED("King's Nameday",28,1),
   "sweden/crown princess nameday": FIXED("Crown Princess' Nameday",12,3),
   "sweden/crown princess birthtday":FIXED("Crown Princess' Birthday",14,7),
   "sweden/flag":               FIXED("Sweden's Flag Day",6,6),
   "sweden/kings nameday":      FIXED("King's Nameday",6,11),
   "sweden/nobel":              FIXED("Nobel Day",10,12),
   "sweden/queens birthday":    FIXED("Queen's Birthday",23,12),
   "sweden/queens nameday":     FIXED("Queen's Nameday",8,8),
   "sweden/valborg eve":        FIXED("Valborgsmässoafton",30,4),
   "sweden/waffle":             FIXED("Waffle Day",25,3),
   "sweden/all saints":         DWDRI("Alla helgons dag",1,11,6,1),
   "sweden/midsummers eve":     DWDRI("Midsummer's Eve",20,6,5,1),
   "sweden/midsummer":          DWDRI("Midsummer's Day",21,6,6,1),
   "sweden/mothers":           	DWDR  ("Mother's Day",	1,6,7,-1),
   "sweden/fathers":           	DWDRI("Father's Day",	1,11,7,2),

   "russia/state sovereignity":  FIXED("State Sovereignity Day",12,6),
   "russia/victory":             FIXED("Victory Day",9,5),
   "russia/valpurgis":           FIXED("Valpurgis Night",1,5),
   "russia/warriors":            FIXED("Warriors Day",23,2),
   "russia/teachers":            FIXED("Teachers Day",1,10),

// ----------------------------------------------------------------
// not verified
// ----------------------------------------------------------------

   "afghanistan/afghan new year":FIXED("Afghan New Year",21,3),
   "afghanistan/childrens":      FIXED("Children's Day",30,8),
   "afghanistan/independence":   FIXED("Independence Day",27,5),
// "afghanistan/jeshyn-afghans": August [28-31?],
   "afghanistan/mothers":        FIXED("Mother's Day",14,6),
   "afghanistan/national assembly":DWDRI("National Assembly Day",1,9,3,2),
   "afghanistan/revolution":     FIXED("Revolution Day",27,4),
   "afghanistan/workers":        FIXED("Workers' Day",1,5),

   "albania/army":               FIXED("Army Day",10,7),
   "albania/independence":       FIXED("Independence Day",28,11),
   "albania/liberation":         FIXED("Liberation Day",29,11),
   "albania/proclamation of the republic":
                                FIXED("Proclamation of the Republic Day",11,1),

   "algeria/independence":       FIXED("Independence Day",3,7),
   "algeria/national":           FIXED("National Day",19,6),
   "algeria/revolution":         FIXED("Revolution Day",1,11),

   "american baptist/roger williams":FIXED("Roger Williams Day",5,2),

   "american samoa/flag":        FIXED("Flag Day",17,4),

   "andorra/national feast":     FIXED("National Feast Day",8,9),

   "anglican church/name of jesus":FIXED("Feast of the Name of Jesus",7,8),

   "angola/armed forces":     	 FIXED("Armed Forces Day",1,8),
   "angola/independence":     	 FIXED("Independence Day",11,11),
   "angola/mpla foundation":  	 FIXED("MPLA Foundation Day",10,12),
   "angola/national heros":   	 FIXED("National Hero's Day",17,9),
   "angola/national holiday": 	 FIXED("National Holiday",4,2),
   "angola/pioneers":         	 FIXED("Pioneers' Day",1,12),
   "angola/victory":          	 FIXED("Victory Day",27,3),
   "angola/workers":          	 FIXED("Workers' Day",1,5),
   "angola/youth":            	 FIXED("Youth Day",14,4),
   "angola/martyrs":FIXED("Day of Martyrs of the Colonial Repression",4,1),

   "anguilla/anguilla":          FIXED("Anguilla Day",1,6),

   "antigua and barbuda/independence":FIXED("Independence Day",1,11),

   "armenia/christmas":          FIXED("Christmas",6,1),
   "armenia/martyrs":            FIXED("Martyrs' Day",24,4),
   "armenia/republic":           FIXED("Republic Day",28,5),

   "aruba/aruba flag":           FIXED("Aruba Flag Day",18,3),

   "australia/australia":        FIXED("Australia Day",26,1),
   "australia/eight hour":       FIXED("Eight Hour Day",11,10),
   "australia/foundation":       FIXED("Foundation Day",7,6),
   "australia/hobert regatta":   FIXED("Hobert Regatta Day",9,2),
   "australia/proclamation":     FIXED("Proclamation Day",28,12),
   "australia/recreation":       FIXED("Recreation Day",1,11),
   "australia/second new years": FIXED("Second New Year's Day",2,1),
   "australia/show":             DWDRI("Show Day",1,11,2,1),
   "australia/summer bank holiday":DWDRI("Bank Holiday",1,8,1,4), // ?
   "australia/spring bank holiday":DWDRI("Bank Holiday",1,3,1,4), // ?

   "austria/national":           FIXED("National Day",15,5),
   "austria/national holiday":   FIXED("National Holiday",26,10),
   "austria/national holiday of austria":
                                 FIXED("National Holiday of Austria",26,10),
   "austria/republic":           FIXED("Republic Day",12,11),
   "austria/second republic":    FIXED("Second Republic Day",27,4),

   "bahamas/fox hill":           DWDRI("Fox Hill Day",1,8,2,2),
   "bahamas/independence":       FIXED("Independence Day",10,7),
   "bahamas/labor":              DWDRI("Labour Day",1,6,5,1),

   "bahrain/national of bahrain":FIXED("National Day of Bahrain",16,12),

   "bangladesh/bengali new year":FIXED("Bengali New Year",14,4),
   "bangladesh/independence":    FIXED("Independence Day",26,3),
   "bangladesh/national mourning":FIXED("National Mourning Day",21,2),
   "bangladesh/revolution":      FIXED("Revolution Day",7,11),
   "bangladesh/victory":         FIXED("Victory Day",16,12),

   "barbados/errol barrow":      FIXED("Errol Barrow Day",21,1),
   "barbados/independence":      FIXED("Independence Day",30,11),
   "barbados/united nations":    FIXED("United Nations Day",7,10),

   "belgium/dynasty":            FIXED("Dynasty Day",15,11),
   "belgium/independence":       FIX_M("Independence Day",21,7,2),
   "belgium/kings":              FIXED("King's Birthday",15,11),

   "belize/baron bliss":         FIXED("Baron Bliss Day",9,3),
   "belize/commonwealth":        FIXED("Commonwealth Day",24,5),
   "belize/garifuna":            FIXED("Garifuna Day",19,11),
   "belize/independence":        FIXED("Independence Day",21,9),
   "belize/national":            FIXED("National Day",10,9),
   "belize/pan american":        FIXED("Pan American Day",12,10),
   "belize/saint georges cay":   FIXED("St. George's Cay Day",10,9),

   "benin/armed forces":         FIXED("Armed Forces Day",26,10),
   "benin/harvest":              FIXED("Harvest Day",31,12),
   "benin/martyrs":              FIXED("Martyrs' Day",16,1),
   "benin/national":             FIXED("National Day",30,11),
   "benin/workers":              FIXED("Workers' Day",1,5),
   "benin/youth":                FIXED("Youth Day",1,4),

   "bermuda/bermuda":            FIXED("Bermuda Day",24,5),
   "bermuda/peppercorn":         FIXED("Peppercorn Day",23,4),
   "bermuda/remembrance":        FIXED("Remembrance Day",11,11),
   "bermuda/somers":             FIXED("Somers Day",28,7),

   "bhutan/kings":               FIXED("King's Birthday",11,11),
   "bhutan/national of bhutan":  FIXED("National Day of Bhutan",17,12),

// "bolivia/alacitas":           occurs in January,
   "bolivia/independence":       FIXED("Independence Day",6,8),
   "bolivia/la paz":             FIXED("La Paz Day",16,7),
   "bolivia/martyrs":            FIXED("Martyrs' Day",21,7),
   "bolivia/memorial":           FIXED("Memorial Day",23,3),
   "bolivia/national":           FIXED("National Day",9,4),

   "bosnia and herzegovina/labors":FIX_M("Labor Days",1,5,2),

   "botswana/botswana":          FIXED("Botswana Day",30,9),
   "botswana/presidents":        FIXED("President's Day",24,5),

   "brazil/brasilia":            FIXED("Brasilia Day",21,4),
   "brazil/discovery":           FIXED("Discovery Day",22,4),
   "brazil/independence":        FIXED("Independence Day",7,9),
   "brazil/proclamation of the republic":
                               FIXED("Proclamation of the Republic Day",15,11),
   "brazil/republic":            FIXED("Republic Day",15,11),
   "brazil/sao paulo":           FIXED("Sao Paulo Anniversary",25,1),
   "brazil/tiradentes":          FIXED("Tiradentes Day",21,4),

// "british virgin islands/sovereigns observance":occurs in June,
   "british virgin islands/territory":FIXED("Territory Day",1,7),

   "brunei/national":            FIXED("National Day",23,2),
   "brunei/royal brunei armed forces":
                                 FIXED("Royal Brunei Armed Forces Day",1,6),
   "brunei/sultans":             FIXED("Sultan's Birthday",15,7),

   "bulgaria/babin den":         FIXED("Babin Den",20,1),
   "bulgaria/botev":             FIXED("Botev Day",20,5),
   "bulgaria/education":         FIXED("Education Day",24,5),
   "bulgaria/liberation":        FIXED("Liberation Day",3,3),
   "bulgaria/national":          FIXED("National Day",3,3),
   "bulgaria/national holiday":  FIX_M("National Holiday",9,9,2),
   "bulgaria/viticulturists":    FIXED("Viticulturists' Day",14,2),

   "burkina faso/anniversary of 1966 coup":
                                 FIXED("Anniversary of 1966 Coup",3,1),
   "burkina faso/labor":         FIXED("Labour Day",1,5),
   "burkina faso/national":      FIXED("National Day",4,8),
   "burkina faso/republic":      FIXED("Republic Day",11,12),
   "burkina faso/youth":         FIXED("Youth Day",30,11),

   "burundi/anniversary of the revolution":
                                 FIXED("Anniversary of the Revolution",30,11),
   "burundi/independence":       FIXED("Independence Day",1,7),
   "burundi/murder of the hero": FIXED("Murder of the Hero Day",13,10),
   "burundi/victory of uprona":  FIXED("Victory of Uprona Day",18,9),

   "cambodia/ancestors":         FIXED("Feast of the Ancestors",22,9),
// "cambodia/chaul chhnam":      occurs in April,
   "cambodia/day of hatred":     FIXED("Day of Hatred",20,5),
   "cambodia/independence":      FIXED("Independence Day",17,4),
   "cambodia/khmer republic constitution":
                                 FIXED("Khmer Republic Constitution Day",12,5),
   "cambodia/memorial":          FIXED("Memorial Day",28,6),
   "cambodia/national":          FIXED("National Day",7,1),
// "cambodia/visak bauchea":     Vesak Day,

   "cameroon/commemoration of national institutions":
                      FIXED("Commemoration of National Institutions Day",10,5),
   "cameroon/constitution":      FIXED("Constitution Day",20,5),
   "cameroon/human rights":      FIXED("Human Rights Day",10,12),
   "cameroon/independence":      FIXED("Independence Day",1,1),
   "cameroon/unification":       FIXED("Unification Day",1,10),
   "cameroon/youth":             FIXED("Youth Day",11,2),

   "canada/civic holiday":       DWDRI("Civic Holiday",1,8,1,1),
   "canada/discovery":           DWDRI("Discovery Day",24,6,1,0),
   "canada/dominion":            FIXED("Dominion Day",1,7),
   "canada/klondike gold discovery":
    			         DWDR("Klondike Gold Discovery Day",18,8,5,-1),
   "canada/labor":               DWDRI("Labor Day",1,9,1,1),
   "canada/remembrance":         FIXED("Remembrance Day",11,11),
// "canada/thanksgiving":
//             usually in early October, and determined by annual proclamation,
   "canada/victoria":            FIXED("Victoria Day",24,5),

   "cape verde/independence":    FIXED("Independence Day",5,7),
   "cape verde/national":        FIXED("National Day",12,9),
   "cape verde/national heroes": FIXED("National Heroes' Day",20,1),

   "caribbean/caricom":          DWDRI("CARICOM Day",1,7,1,1),
   "caribbean/emancipation":     DWDRI("Emancipation Day",1,8,1,1),
   "caribbean/schoelcher":       FIXED("Schoelcher Day",21,7),

   "central african republic/first government":
                                 FIXED("anniversary of first government",15,5),
   "central african republic/republic":
                 FIXED("Anniversary of the Proclamation of the Republic",1,12),
   "central african republic/day after republic":
                 FIXED("Day after Republic Day",2,12),
   "central african republic/boganda":FIXED("Boganda Day",29,3),
   "central african republic/independence":FIXED("Independence Day",13,8),
   "central african republic/national holiday":FIXED("national holiday",11,11),
   "central african republic/the after republic":
                                 FIXED("the day after Republic Day",2,12),

   "central america/independence":FIXED("Independence Day",15,9),

   "chad/creation of the union of central african states":
              FIXED("Creation of the Union of Central African States Day",2,4),
   "chad/independence":          FIXED("Independence Day",11,8),
   "chad/national":              FIXED("National Day",11,1),
   "chad/republic":              FIXED("Republic Day",28,11),
   "chad/traditional independence []":
                                 FIXED("Traditional Independence Day []",11,8),

   "chile/armed forces":         FIXED("Armed Forces Day",19,9),
   "chile/independence":         FIXED("Independence Day",18,9),
   "chile/independence2":        FIX_M("Independence Day",18,9,2),
   "chile/national holiday":     FIXED("National Holiday",11,9),
   "chile/navy":                 FIXED("Navy Day",21,5),

// "china/army":
//                 or <i>National Liberation Army Festival,</i> August 1, 1927,
   "china/ccps":                 FIXED("The CCP's Birthday",1,7),
   "china/childrens":            FIXED("Children's Day",1,6),
   "china/national":             FIXED("National Day",1,10),
   "china/teachers":             FIXED("Teacher's Day",1,9),
   "china/tree planting":        FIXED("Tree Planting Day",1,4),
   "china/youth":                FIXED("Youth Day",4,5),

   "columbia/battle of boyaca":  FIXED("Battle of Boyaca Day",7,8),
   "columbia/cartagena independence":FIXED("Cartagena Independence Day",11,11),
   "columbia/independence":      FIXED("Independence Day",20,7),
   "columbia/thanksgiving":      FIXED("Thanksgiving Day",5,6),

   "comoros/anniversary of president abdallahs assassination":
              FIXED("Anniversary of President Abdallah's Assassination",27,11),
   "comoros/independence":       FIXED("Independence Day",6,7),

   "congo/day before army":      FIXED("day before Army Day",16,11),
   "congo/day before independence":FIXED("day before Independence Day",29,6),
   "congo/day before national":  FIXED("day before national day",23,11),
   "congo/independence":         FIXED("Independence Day",15,8),
   "congo/martyr of independence":FIXED("Martyr of Independence Day",4,1),

   "corsica/napoleons":          FIXED("Napoleon's Day",15,8),

   "costa rica/battle of rivas": FIXED("Battle of Rivas Day",11,4),
// "costa rica/guanacaste":      ,
   "costa rica/independence":    FIXED("Independence Day",15,9),

   "croatia/national holiday":   FIXED("National Holiday",22,6),
   "croatia/republic":           FIXED("Republic Day",30,5),

// "cuba/beginning of independence wars":commemorates Fidel Castro's guerrilla,
   "cuba/independence":          FIXED("Independence Day",20,5),
   "cuba/liberation":            FIXED("Liberation Day",1,1),
   "cuba/revolution":            FIXED("Revolution Day",26,7),
   "cuba/wars of independence":  FIXED("Wars of Independence Day",10,10),

   "cyprus/green monday":        EASTER_REL("Green Monday",-41),
   "cyprus/kataklysmos":         FIXED("Kataklysmos",7,6),
   "cyprus/peace and freedom":   FIXED("Peace and Freedom Day",20,7),
   "cyprus/submersion of the holy cross":
                        FIXED("Feast of the Submersion of the Holy Cross",6,1),
   "cyprus/communal resistance": FIXED("Communal Resistance Day",1,8),
   "cyprus/greek national":      FIXED("Greek National Day",28,10),
   "cyprus/name day":            FIXED("Name Day",19,1),

   "czech republic/death of jan hus":FIXED("Death of Jan Hus",6,7),
   "czech republic/introduction of christianity":
                                 FIXED("Introduction of Christianity",5,7),

   "denmark/birthday of queen margrethe ii":
                                 FIXED("Birthday of Queen Margrethe II",16,4),
   "denmark/common prayer":      EASTER_REL("Common Prayer Day",26),
   "denmark/constitution":       FIXED("Constitution Day",5,6),
   "denmark/fjortende februar":  FIXED("Fjortende Februar",14,2),
   "denmark/liberation":         FIXED("Liberation Day",5,5),
// "denmark/public holidays":    ,
   "denmark/valdemars":          FIXED("Valdemar's Day",15,6),

   "djibouti/independence feast":FIXED("Independence Feast Day",27,6),
   "djibouti/workers":           FIXED("Workers' Day",1,5),

   "dominica/community service": FIXED("Community Service Day",4,11),
   "dominica/emancipation":      DWDRI("Emancipation Day",1,8,1,1),
   "dominica/independence":      FIXED("Independence Day",3,11),

   "ecuador/battle of pichincha ":FIXED("Battle of Pichincha  Day",24,5),
// "ecuador/cuenca independence":
//                            commemorates the declaration of independence for,
   "ecuador/guayaquils independence":
                                 FIXED("Guayaquil's Independence Day",9,10),
// "ecuador/independence":       : August 10, 1809,
   "ecuador/simon bolivars":     FIXED("Simon Bolivar's Birthday",24,7),

   "egypt/armed forces":         FIXED("Armed Forces Day",6,10),
   "egypt/evacuation":           FIXED("Evacuation Day",18,6),
   "egypt/leylet en-nuktah":     FIXED("Leylet en-Nuktah",17,6),
   "egypt/revolution":           FIXED("Revolution Day",23,7),
   "egypt/sham al-naseem":       FIXED("Sham al-Naseem",21,3),
   "egypt/sinai liberation":     FIXED("Sinai Liberation Day",25,4),
   "egypt/suez":                 FIXED("Suez Day",24,10),
   "egypt/victory":              FIXED("Victory Day",23,12),

   "el salvador/first call for independence":
                                 FIXED("First Call for Independence Day",5,11),
   "el salvador/independence":   FIXED("Independence Day",15,9),
   "el salvador/san salvador festival":FIX_M("San Salvador Festival",4,8,3),
   "el salvador/schoolteachers": FIXED("Schoolteachers Day",22,6),

   "equatorial guinea/armed forces":FIXED("Armed Forces Day",3,8),
   "equatorial guinea/constitution":FIXED("Constitution Day",15,8),
   "equatorial guinea/independence":FIXED("Independence Day",12,10),
   "equatorial guinea/oau":      FIXED("OAU DAy",25,5),
   "equatorial guinea/presidents":FIXED("President's Birthday",5,6),

   "estonia/midsummer": 	 FIXED("Midsummer Day",24,6),
   "estonia/victory":            FIXED("Victory Day",23,6),

   "ethiopia/battle of aduwa":   FIXED("Battle of Aduwa Day",2,3),
   "ethiopia/buhe":              FIXED("Buhe",19,8),
   "ethiopia/epiphany":          FIXED("Epiphany",19,1),
   "ethiopia/ethiopian new year":FIXED("Ethiopian New Year",11,9),
   "ethiopia/genna":             FIXED("Genna",7,1),
   "ethiopia/liberation":        FIXED("Liberation Day",5,5),
   "ethiopia/martyrs":           FIXED("Martyrs' Day",19,2),
   "ethiopia/meskel":            FIXED("Meskel",28,9),
   "ethiopia/patriots victory":  FIXED("Patriots' Victory Day",6,4),
   "ethiopia/popular revolution commemoration":
                            FIXED("Popular Revolution Commemoration Day",12,9),
   "ethiopia/revolution":        FIXED("Revolution Day",12,9),
   "ethiopia/victory of adowa":  FIXED("Victory of Adowa",2,3),

   "falkland islands/anniversary of the battle of the falkland islands":
               FIXED("Anniversary of the Battle of the Falkland Islands",8,12),
   "falkland islands/liberation":FIXED("Liberation Day",14,6),

   "fiji/bank holiday":          DWDRI("Bank Holiday",1,8,1,1),
// "fiji/deed of cession":       occurs near October 7,
// "fiji/fiji2"                  on or near October 10,
   "fiji/fiji":                  DWDRI("Fiji Day",1,8,1,2),
   "fiji/prince charles":        DWDRI("Prince Charles Day",1,11,1,3),
   "fiji/queens":                DWDRI("Queen's Birthday",1,6,1,2),

   "finland/alexis kivi":        FIXED("Alexis Kivi Day",10,10),
   "finland/flag":               FIXED("Flag Day",4,6),
   "finland/flag of the army":   FIXED("Flag Day of the Army",19,5),
// "finland/helsinki":           ,
   "finland/independence":       FIXED("Independence Day",6,12),
   "finland/kalevala":           FIXED("Kalevala Day",28,2),
// "finland/midsummers eve":     WDREL("Midsummer's Eve","midsummers",5,-1),
   "finland/runebergs":          FIXED("Runeberg's Day",5,2),
   "finland/snellman":           FIXED("Snellman Day",12,5),
   "finland/all saints":         DWDRI("All Saints Day",1,11,6,1),
   "finland/midsummers eve":     DWDRI("Midsummer's Eve",24,6,5,0),
   "finland/midsummer":          DWDRI("Midsummer's Day",25,6,6,0),

   "france/ascension":           FIXED("Ascension Day",12,5),
   "france/bastille":            FIXED("Bastille Day",14,7),
   "france/d-day observance":    FIXED("D-Day Observance",6,6),
   "france/fathers":             DWDRI("Father's Day",1,6,7,2),
   "france/fete des saintes-maries":FIXED("F\352te des Saintes-Maries",24,5),
   "france/fete nationale":      FIXED("F\352te Nationale",22,9),
   "france/liberation":          FIXED("Liberation Day",8,5),
   "france/marseillaise":        FIXED("Marseillaise Day",30,7),
   "france/night watch":         FIXED("Night Watch",13,7),


   "french polynesia/armistice": FIXED("Armistice Day",8,5),
   "french polynesia/chinese new year":FIXED("Chinese New Year",6,2),
   "french polynesia/missionaries arrival commemoration":
                               FIXED("Missionaries Arrival Commemoration",5,3),

   "gabon/constitution":         FIXED("Constitution Day",19,2),
   "gabon/independence":         FIXED("Independence Day",17,8),
   "gabon/renovation":           FIXED("Renovation Day",12,3),

   "germany/day of repentance":  FIXED("Day of Repentance",18,11),
   "germany/day of the workers": FIXED("Day of the Workers",1,5),
   "germany/foundation":         FIXED("Foundation Day",7,10),
   "germany/national":           FIXED("National Day",23,5),
// "germany/oktoberfest":        occurs in Munich in September/October,
   "germany/unity":              FIXED("Unity Day",3,10),
   "germany/waldchestag":        EASTER_REL("Waldchestag",53),

// "ghana/aboakyer":             occurs in April or May,
   "ghana/homowo":               FIXED("Homowo",1,8),
   "ghana/independence":         FIXED("Independence Day",6,3),
   "ghana/liberation":           FIXED("Liberation Day",24,2),
   "ghana/republic":             FIXED("Republic Day",1,7),
   "ghana/revolution":           FIXED("Revolution Day",31,12),
   "ghana/third republic":       FIXED("Third Republic Day",24,9),
   "ghana/uprising":             FIXED("Uprising Day",4,6),

   "gibraltar/commonwealth":     FIXED("Commonwealth Day",13,3),
// "gibraltar/late summer bank holiday":occurs in the end of August,
   "gibraltar/spring bank holiday":FIXED("Spring Bank Holiday",25,6),

   "greece/clean monday":        EASTER_REL("Clean Monday",-41),
// "greece/dumb week":           the week preceding Holy Week,
   "greece/independence":        FIXED("Independence Day",25,3),
   "greece/flower festival":
                                 FIXED("Flower Festival",1,5),
   "greece/midwifes":            FIXED("Midwife's Day",8,1),
   "greece/ochi":                FIXED("Ochi Day",28,10),
   "greece/saint basils":        FIXED("Saint Basil's Day",1,1),
   "greece/dodecanese accession":FIXED("Dodecanese Accession Day",7,3),
   "greece/liberation of xanthi":FIXED("Liberation of Xanthi",4,10),

   "gregorian calendar /gregorian calendar":
                                 FIXED("Gregorian Calendar Day",24,2),

   "grenada/independence":       FIXED("Independence Day",7,2),
   "grenada/national holiday":   FIXED("National Holiday",13,3),
   "grenada/thanksgiving":       FIXED("Thanksgiving Day",25,10),

   "guadeloupe/ascension":       FIXED("Ascension Day",12,5),

   "guam/discovery":             FIXED("Discovery Day",6,3),
   "guam/guam discovery":        DWDRI("Guam Discovery Day",1,3,1,1),
   "guam/liberation":            FIXED("Liberation Day",21,7),

   "guatemala/army":             FIXED("Army Day",30,6),
   "guatemala/bank employees":   FIXED("Bank Employees' Day",1,7),
   "guatemala/independence":     FIXED("Independence Day",15,9),
   "guatemala/revolution":       FIXED("Revolution Day",30,6),

   "guinea/anniversary of cmrn": FIXED("Anniversary of CMRN",3,4),
   "guinea/anniversary of womens revolt":
                                 FIXED("Anniversary of Women's Revolt",27,8),
   "guinea/day of 1970 invasion":FIXED("Day of 1970 Invasion",22,11),
   "guinea/independence":        FIXED("Independence Day",2,10),
   "guinea/referendum":          FIXED("Referendum Day",28,9),

   "guinea-bissau/colonization martyrs":FIXED("Colonization Martyr's Day",3,8),
   "guinea-bissau/independence": FIXED("Independence Day",24,9),
   "guinea-bissau/mocidade":     FIXED("Mocidade Day",1,12),
   "guinea-bissau/national":     FIXED("National Day",12,9),
   "guinea-bissau/national heroes":FIXED("National Heroes Day",20,1),
   "guinea-bissau/readjustment movement":
                                 FIXED("Readjustment Movement Day",14,11),

   "guyana/freedom":             FIXED("Freedom Day",1,8),
   "guyana/independence":        FIXED("Independence Day",26,5),
   "guyana/republic":            FIXED("Republic Day",23,2),

   "haiti/independence":	 FIXED("Independence Day",1,1),
   "haiti/ancestry":		 FIXED("Ancestry Day",2,1),
   "haiti/toussaint":		 FIXED("Toussain L'Ouverture Day",7,4),
   "haiti/pan american":	 FIXED("Pan-American Day",14,4),
   "haiti/labor":		 FIXED("Agriculture and Labor Day",1,5),
   "haiti/flag":		 FIXED("Flag & University Day",18,5),
   "haiti/sovereignty":		 FIXED("National Sovereignty",22,5),
   "haiti/agwe":		 FIXED("Day of Agwe",4,7),
   "haiti/papa ogou":		 FIXED("Day of Papa Ogou",25,7),
   "haiti/dessalines":           FIXED("Dessalines Day",17,10),
   "haiti/discovery":            FIXED("Discovery Day",5,12),
   "haiti/vertieres":            FIXED("Verti\350res Day",18,11),

   "honduras/armed forces":      FIXED("Armed Forces Day",21,10),
   "honduras/independence":      FIXED("Independence Day",15,9),
   "honduras/morazan":           FIXED("Moraz\341n Day",3,10),
   "honduras/pan american":      FIXED("Pan American Day",14,4),
   "honduras/thanksgiving":      FIXED("Thanksgiving Day",15,3),

   "hong kong/birthday of confucious":FIXED("Birthday of Confucious",22,9),
   "hong kong/birthday of pak tai":FIXED("Birthday of Pak Tai",3,4),
// "hong kong/chung yeung festival":[9th day of 9th month],
   "hong kong/half-year":        FIXED("Half-Year Day",1,7),
   "hong kong/liberation":       DWDR("Liberation Day",1,9,1,-1),

   "hungary/constitution":       FIXED("Constitution Day",20,8),
   "hungary/revolution1848":     FIXED("1848 Revolution Day",15,3),
   "hungary/revolution56":       FIXED("'56 Revolution Day",23,10),
   "hungary/labor":              FIXED("Holiday of Labor",1,5),

   "iceland/independence":       FIXED("Independence Day",17,6),

   "india/indian independence":  FIXED("Indian Independence Day",15,8),
   "india/mahatma gandhi jayanti":FIXED("Mahatma Gandhi Jayanti",2,10),
   "india/mahatma gandhi martyrdom":FIXED("Mahatma Gandhi Martyrdom Day",30,1),
   "india/republic":             FIXED("Republic Day",26,1),

// "indonesia/balinese new year":held every 210 days,
   "indonesia/independence":     FIXED("Independence Day",17,8),
   "indonesia/kartini":          FIXED("Kartini Day",21,4),

   "iran/constitution":          FIXED("Constitution Day",5,8),
   "iran/islamic republic":      FIXED("Islamic Republic Day",1,4),
   "iran/martyrdom of imam ali": FIXED("Martyrdom of Imam Ali",14,7),
   "iran/no ruz":                FIXED("No Ruz",21,3),
   "iran/oil nationalization":   FIXED("Oil Nationalization Day",20,3),
   "iran/revolution":            FIXED("Revolution Day",11,2),
   "iran/revolution2":           FIXED("Revolution Day",2,4),

   "iraq/14 ramadan revolution": FIXED("14 Ramadan Revolution Day",8,2),
   "iraq/army":                  FIXED("Army Day",6,1),
   "iraq/july revolution":       FIXED("July Revolution Day",17,7),
   "iraq/republic":              FIXED("Republic Day",14,7),

   "ireland/sheelahs":           FIXED("Sheelah's Day",18,3),

   "israel/balfour declaration": FIXED("Balfour Declaration Day",2,11),
// "israel/hebrew university":   occurs near April 1,
// "israel/holocaust memorial":  Nisan 27,
// "israel/independence":        Iyar 5,
   "israel/jerusalem reunification":FIXED("Jerusalem Reunification Day",12,5),
// "israel/national":            occurs in May,

   "italy/anniversary of the republic":
                                 FIXED("Anniversary of the Republic",2,6),
   "italy/befana":               FIXED("Befana Day",6,1),
   "italy/day of conciliation":  FIXED("Day of Conciliation",11,2),
   "italy/festa del redentore":  DWDRI("Festa del Redentore",1,7,7,3),
   "italy/joust of the quintana":DWDRI("Joust of the Quintana",1,8,7,1),
   "italy/liberation":           FIXED("Liberation Day",25,4),
   "italy/natale di roma":       FIXED("Natale di Roma",21,4),
   "italy/palio del golfo":      DWDRI("Palio Del Golfo",1,8,7,2),
// "italy/saint marks":          occurs in April,
   "italy/santo stefano":        FIXED("Santo Stefano",26,12),

   "ivory coast/independence":   FIXED("Independence Day",7,12),
   "ivory coast/national holiday of the ivory coast":
                             FIXED("National Holiday of the Ivory Coast",7,12),

// "jainism/jain payushan":      began on 9/2/94 and 8/23/95,
// "jainism/mahavir jayanti":    Vaisakha 13,

   "jamaica/heroes":             DWDRI("Heroes' Day",1,10,1,3),
   "jamaica/independence":       DWDRI("Independence Day",1,8,1,1),
   "jamaica/labor":              FIXED("Labour Day",23,5),

   "japan/adults":               FIXED("Adults Day",15,1),
// "japan/autumnal equinox":     [sep 22nd or 23rd],
   "japan/bean-throwing festival":FIXED("Bean-Throwing Festival",4,2),
// "japan/black ship":           near July 15,
// "japan/bon":                  occurs between July 13 and 15,
   "japan/childrens":            FIXED("Children's Day",5,5),
   "japan/childrens protection": FIXED("Children's Protection Day",17,4),
   "japan/constitution":         FIXED("Constitution Day",3,5),
   "japan/culture":              FIXED("Culture Day",3,11),
   "japan/emperors":             FIXED("Emperor's Birthday",23,12),
   "japan/empire":               FIXED("Empire Day",11,2),
   "japan/gion matsuri":         FIX_M("Gion Matsuri",16,2,9),
   "japan/greenery":             FIXED("Greenery Day",29,4),
// "japan/health":               <i>Sports Day,</i> October 10,
   "japan/hina matsuri":         FIXED("Hina Matsuri",3,3),
   "japan/hiroshima peace festival":FIXED("Hiroshima Peace Festival",6,8),
   "japan/hollyhock festival":   FIXED("Hollyhock Festival",15,5),
   "japan/jidai matsuri":        FIXED("Jidai Matsuri",22,10),
   "japan/kakizome":             FIXED("Kakizome",2,1),
   "japan/kambutsue":            FIXED("Kambutsue",8,4),
   "japan/kite battles of hamamatsu":FIX_M("Kite Battles of Hamamatsu",3,5,3),
   "japan/labor thanksgiving":   FIXED("Labor Thanksgiving Day",23,11),
   "japan/martyr":               FIXED("Martyr Day",5,2),
// "japan/memorial":             ,
   "japan/memorial to broken dolls":FIXED("Memorial to Broken Dolls Day",3,6),
   "japan/national foundation":  FIXED("National Foundation Day",11,2),
// "japan/peoples holiday":      [near may 4?],
   "japan/respect for the aged": FIXED("Respect for the Aged Day",15,9),
   "japan/sanno matsuri":        FIXED("Sanno Matsuri",15,6),
// "japan/shichigosan":          The name means "seven-five-three",
   "japan/shigoto hajime":       FIXED("Shigoto Hajime",2,1),
   "japan/tanabata":             FIXED("Tanabata",7,7),
   "japan/tango-no-sekku":       FIXED("Tango-no-sekku",5,5),
// "japan/vernal equinox":       [mar 20 or mar 21st],

   "jordan/arbor":               FIXED("Arbor Day",15,1),
   "jordan/coronation":          FIXED("Coronation Day",11,8),
   "jordan/independence":        FIXED("Independence Day",25,5),
   "jordan/king hussein":        FIXED("King Hussein Day",14,11),

   "kazakhstan/nauryz meyrami":	 FIXED("Nauryz Meyrami",22,3),
   "kazakhstan/peoples unity":	 FIXED("People's Unity Day",1,5),
   "kazakhstan/victory":	 FIXED("Victory Day",9,5),
   "kazakhstan/national flag":	 FIXED("National Flag",24,8),
   "kazakhstan/constitution":	 FIXED("Constitution Day",30,8),
   "kazakhstan/republic":	 FIXED("Republic Day",25,10),
   "kazakhstan/independence":	 FIXED("Independence Day",16,12),

   "kenya/independence":         FIXED("Independence Day",12,12),
   "kenya/kenyatta":             FIXED("Kenyatta Day",20,10),
   "kenya/madaraka":             FIXED("Madaraka Day",1,6),

   "kiribati/independence":      FIXED("Independence Day",12,7),
   "kiribati/youth":             FIXED("Youth Day",4,8),

   "south korea/folklore":       FIXED("Folklore Day",3,2),
   "south korea/taeborum":	 FIX_M("Taeborum",20,2,3),
   "south korea/independence":	 FIXED("Independence Movement Day",1,3),
   "south korea/labor":		 FIXED("Labor Day",10,3),
   "south korea/arbor":		 FIXED("Arbor Day",5,4),
   "south korea/childrens":	 FIXED("Children's Day",5,5),
   "south korea/buddha":	 FIXED("Buddha's Birthday",22,5),
   "south korea/memorial":	 FIXED("Memorial Day",6,6),
   "south korea/constitution":	 FIXED("Constitution Day",17,7),
   "south korea/liberation":	 FIXED("Liberation Day",15,8),
   "south korea/republic":	 FIXED("Republic Day",15,8),
   "south korea/thanksgiving":	 FIX_M("Thanksgiving",20,9,3),
   "south korea/armed forces":	 FIXED("Armed Forces Day",1,10),
   "south korea/foundation":     FIXED("Foundation Day",3,10),
   "south korea/alphabet":       FIXED("Alphabet Day",9,10),

   "kuwait/independence":        FIXED("Independence Day",19,6),
   "kuwait/national":            FIXED("National Day",25,2),

// "laos/army":                  occurs around March 24,
   "laos/constitution":          FIXED("Constitution Day",11,5),
   "laos/independence":          FIXED("Independence Day",19,7),
   "laos/memorial":              FIXED("Memorial Day",15,8),
   "laos/pathet lao":            FIXED("Pathet Lao Day",6,1),
   "laos/republic":              FIXED("Republic Day",2,12),

   "latvia/independence":        FIXED("Independence Day",18,11),
   "latvia/midsummer festival":  FIX_M("Midsummer Festival",23,6,2),

   "lebanon/evacuation":         FIXED("Evacuation Day",31,12),
   "lebanon/independence":       FIXED("Independence Day",22,11),
   "lebanon/martyrs":            FIXED("Martyrs' Day",6,5),
   "lebanon/saint maron":        FIXED("Saint Maron Day",9,2),

   "lesotho/family":             DWDRI("Family Day",1,7,1,1),
   "lesotho/independence":       FIXED("Independence Day",4,10),
   "lesotho/kings":              FIXED("King's Birthday",2,5),
   "lesotho/moshoeshoes":        FIXED("Moshoeshoe's Day",12,3),
   "lesotho/national holiday":   FIXED("National Holiday",28,1),
   "lesotho/national sports":    FIXED("National Sports Day",6,10),
   "lesotho/national tree planting":FIXED("National Tree Planting Day",21,3),

   "liberia/armed forces":       FIXED("Armed Forces Day",11,2),
   "liberia/decoration":         FIXED("Decoration Day",13,3),
   "liberia/fast and prayer":    FIXED("Fast and Prayer Day",11,4),
   "liberia/flag":               FIXED("Flag Day",24,8),
   "liberia/independence":       FIXED("Independence Day",26,7),
   "liberia/j j roberts":        FIXED("J. J. Robert's Birthday",15,3),
   "liberia/literacy":           FIXED("Literacy Day",14,2),
   "liberia/matilda newport":    FIXED("Matilda Newport Day",1,12),
   "liberia/memorial":           FIXED("Memorial Day",12,11),
   "liberia/national redemption":FIXED("National Redemption Day",12,4),
   "liberia/pioneers":           FIXED("Pioneers' Day",7,1),
   "liberia/president tubmans":  FIXED("President Tubman's Birthday",29,11),
   "liberia/thanksgiving":       DWDRI("Thanksgiving Day",1,11,4,1),
   "liberia/unification and integration":
                                 FIXED("Unification and Integration Day",14,5),

   "libya/evacuation":           FIXED("Evacuation Day",28,3),
   "libya/expulsion of the fascist settlers":
                           FIXED("Expulsion of the Fascist Settlers Day",7,10),
   "libya/kings":                FIXED("King's Birthday",12,3),
   "libya/national":             FIXED("National Day",1,9),
   "libya/revolution":           FIXED("Revolution Day",1,9),
   "libya/sanusi army":          FIXED("Sanusi Army Day",9,8),
// "libya/troop withdrawal":     near June 30,

   "liechtenstein/birthday of prince franz-josef ii":
                               FIXED("Birthday of Prince Franz-Josef II",16,8),
   "liechtenstein/national":     FIXED("National Day",15,8),
   "liechtenstein/bank holiday": FIXED("Bank Holiday",2,1),

   "lithuania/coronation":       FIXED("Coronation Day",6,7),
   "lithuania/independence":     FIXED("Independence Day",16,2),
   "lithuania/mourning":         FIXED("Mourning",1,11),

   "luxembourg/burgsonndeg":     FIXED("Burgsonndeg",28,2),
   "luxembourg/grand duchess":   FIXED("Grand Duchess' Birthday",23,1),
   "luxembourg/liberation":      FIXED("Liberation Day",9,9),
   "luxembourg/national":        FIXED("National Day",23,6),

   "macao/anniversary of the portugese revolution":
                         FIXED("Anniversary of the Portugese Revolution",25,4),
   "macao/battle of july 13":    FIXED("Feast of the Battle of July 13",13,7),
// "macao/hungry ghosts":        occurs in August,
   "macao/procession of our lady of fatima":
                                FIXED("Procession of Our Lady of Fatima",13,5),
   "macao/republic":             FIXED("Republic Day",5,10),

   "madagascar/commemoration":   FIXED("Commemoration Day",29,3),
   "madagascar/independence":    FIXED("Independence Day",26,6),
   "madagascar/republic":        FIXED("Republic Day",30,12),

   "malawi/august holiday":      FIXED("August Holiday",6,8),
   "malawi/kamuzu":              FIXED("Kamuzu Day",14,5),
   "malawi/martyrs":             FIXED("Martyrs' Day",3,3),
   "malawi/mothers":             FIXED("Mother's Day",17,10),
   "malawi/national tree-planting":FIXED("National Tree-Planting Day",21,12),
   "malawi/republic":            FIXED("Republic Day",6,7),

// "malaysia/additional public holiday in kuala lumpur":City Day (Feb 1),
   "malaysia/kings":             FIXED("King's Birthday",3,6),
   "malaysia/malaysia":          FIXED("Malaysia Day",31,8),

   "maldives/fisheries":         FIXED("Fisheries Day",10,12),
   "maldives/independence":      FIXED("Independence Day",26,7),
   "maldives/national":          FIXED("National Day",7,1),
   "maldives/republic":          FIXED("Republic Day",11,11),
   "maldives/victory":           FIXED("Victory Day",3,11),

   "mali/army":                  FIXED("Army Day",20,1),
   "mali/independence":          FIXED("Independence Day",22,9),
   "mali/liberation":            FIXED("Liberation Day",19,11),

   "malta/freedom":              FIXED("Freedom Day",31,3),
   "malta/independence":         FIXED("Independence Day",21,9),
   "malta/memorial of 1919 riot":FIXED("Memorial of 1919 riot",7,6),
   "malta/republic":             FIXED("Republic Day",13,12),

   "marshall islands/proclamation of the republic of marshall islands":
                 FIXED("Proclamation of the Republic of Marshall Islands",1,5),

   "martinique/ascension":       FIXED("Ascension Day",12,5),

   "mauritania/independence":    FIXED("Independence Day",28,11),

   "mauritius/independence":     FIXED("Independence Day",12,3),

   "mexico/birthday of benito juarez":
                                 FIXED("Birthday of Benito Ju\341rez",21,3),
   "mexico/cinco de mayo":       FIXED("Cinco de Mayo",5,5),
   "mexico/constitution":        FIXED("Constitution Day",5,2),
   "mexico/day of mourning":     FIXED("Day of Mourning",17,7),
   "mexico/dia de la raza":      FIXED("Dia de la Raza",12,10),
   "mexico/holy cross":          FIXED("Holy Cross Day",3,5),
   "mexico/independence":        FIXED("Independence Day",16,9),
   "mexico/night of the radishes":FIXED("Night of the Radishes",23,12),
   "mexico/our lady of guadalupe":
                                 FIXED("Feast of Our Lady of Guadalupe",12,12),
   "mexico/posadass":            FIX_M("Posadas Days",16,12,9),
   "mexico/presidential message":FIXED("Presidential Message Day",1,9),
   "mexico/revolution":          FIXED("Revolution Day",20,11),
   "mexico/san marc\363s":       FIXED("San Marc\363s Day",25,4),

// "micronesia/national holiday":,

   "monaco/monaco national festival":FIXED("Monaco National Festival",19,11),

   "mongolia/national":          FIXED("National Day",11,7),
   "mongolia/republic":          FIXED("Republic Day",26,11),

   "montserrat/festival":        FIXED("Festival Day",31,12),
   "montserrat/liberation":      FIXED("Liberation Day",23,11),

   "mormonism/founding of the mormon church":
                                 FIXED("Founding of the Mormon Church",6,4),

   "morocco/green march":        FIXED("Green March",6,11),
   "morocco/independence":       FIXED("Independence Day",2,3),
   "morocco/oued ed-dahab":      FIXED("Oued ed-Dahab Day",14,8),
   "morocco/throne":             FIXED("Throne Day",3,3),

   "mozambique/armed forces":    FIXED("Armed Forces Day",25,9),
   "mozambique/heroes":          FIXED("Heroes' Day",3,2),
   "mozambique/independence":    FIXED("Independence Day",25,6),
   "mozambique/lusaka agreement":FIXED("Lusaka Agreement Day",7,9),
   "mozambique/universal fraternity":FIXED("Universal Fraternity Day",1,1),
   "mozambique/womens":          FIXED("Women's Day",7,4),
   "mozambique/workers":         FIXED("Workers' Day",1,5),

   "myanmar/armed forces":       FIXED("Armed Forces Day",27,3),
// "myanmar/burmese new year":   near April 17,
   "myanmar/independence":       FIXED("Independence Day",4,1),
   "myanmar/martyrs":            FIXED("Martyrs' Day",19,7),
   "myanmar/national":           FIXED("National Day",10,12),
   "myanmar/peasants":           FIXED("Peasants' Day",2,3),
   "myanmar/resistance":         FIXED("Resistance Day",27,3),
   "myanmar/union":              FIXED("Union Day",12,2),

   "namibia/casinga":            FIXED("Casinga Day",4,5),
   "namibia/day of goodwill":    FIXED("Day of Goodwill",7,10),
   "namibia/family":             FIXED("Family Day",26,12),
   "namibia/heroes":             FIXED("Heroes' Day",26,8),
   "namibia/independence":       FIXED("Independence Day",21,3),
   "namibia/workers":            FIXED("Workers' Day",1,5),

   "nauru/angam":                FIXED("Angam Day",27,10),
   "nauru/independence":         FIXED("Independence Day",31,1),

   "nepal/birthday of king birendra":FIXED("Birthday of King Birendra",28,12),
// "nepal/buddha jayanti":       occurs in May,
   "nepal/constitution":         FIXED("Constitution Day",16,12),
   "nepal/democracy":            FIXED("Democracy Day",18,2),
   "nepal/independence":         FIXED("Independence Day",21,12),
   "nepal/kings":                FIXED("King's Birthday",28,12),
   "nepal/national unity":       FIXED("National Unity Day",11,1),
   "nepal/queens":               FIXED("Queen's Birthday",8,11),
   "nepal/tij":                  FIXED("Tij Day",8,8),

   "netherlands/beggars":        FIXED("Beggar's Day",11,11),
   "netherlands/independence":   FIXED("Independence Day",25,7),
   "netherlands/liberation":     FIXED("Liberation Day",5,5),
   "netherlands/queens":         FIXED("Queen's Day",30,4),
   "netherlands/sinterklaas":    FIXED("Sinterklaas",6,12),

   "netherlands antilles/bonaire":FIXED("Bonaire Day",6,9),
   "netherlands antilles/cura\347ao":FIXED("Cura\347ao Day",2,7),
   "netherlands antilles/queens":FIXED("Queen's Birthday",30,4),
   "netherlands antilles/saba":  FIXED("Saba Day",6,12),
   "netherlands antilles/saint eustatius":FIXED("St Eustatius Day",16,11),
   "netherlands antilles/saint maarten":FIXED("St Maarten Day",11,11),
   
   "new caledonia/bridge":	 FIXED("Bridge Day",2,1),

   "new zealand/labor":          DWDR("Labor Day",1,11,1,-1),
   "new zealand/queens":         FIXED("Queen's Birthday",4,6),
   "new zealand/waitangi":       FIXED("Waitangi Day",6,2),

   "nicaragua/air force":        FIXED("Air Force Day",1,2),
   "nicaragua/army":             FIXED("Army Day",27,5),
   "nicaragua/fiesta":           FIXED("Fiesta Day",1,8),
   "nicaragua/independence":     FIXED("Independence Day",15,9),
   "nicaragua/revolution":       FIXED("Revolution Day",19,7),
   "nicaragua/san jacinto":      FIXED("San Jacinto Day",14,9),

   "niger/independence":         FIXED("Independence Day",3,8),
   "niger/national":             FIXED("National Day",15,4),
   "niger/republic":             FIXED("Republic Day",18,12),

   "nigeria/childrens":          FIXED("Children's Day",27,5),
   "nigeria/harvest festival":   FIXED("Harvest Festival",12,10),
   "nigeria/national":           FIXED("National Day",1,10),
// "nigeria/odum titun":         occurs in the winter,
// "nigeria/odun kekere":        occurs at the end of Ramadan,

   "northern mariana islands/commonwealth":FIXED("Commonwealth Day",3,1),
   "northern mariana islands/presidents":FIXED("Presidents Day",13,2),

   "north korea/kim jong-il":	 FIXED("Kim Jong-il's Birthday",16,2),
   "north korea/kim il-sun":     FIXED("Kim Il-Sun's Birthday",15,4),
   "north korea/armed forces":   FIXED("Armed Forces Day",25,4),
   "north korea/chilsok":        FIXED("Ch'ilsok",28,7),
   "north korea/liberation":     FIXED("Anniversary of Liberation",15,8),
   "north korea/independence":   FIXED("Independence Day",9,9),
   "north korea/party foundation":FIXED("Party Foundation Day",10,10),
   "north kroea/constutution":  FIXED("Anniversary of the Constitution",27,12),

   "norway/constitution":        FIXED("Constitution Day",17,5),
   "norway/olsok eve festival":  FIXED("Olsok Eve Festival",29,7),
   "norway/tyvendedagen":        FIXED("Tyvendedagen",13,1),

   "oman/national":              FIXED("National Day",23,7),
   "oman/national of oman":      FIXED("National Day of Oman",18,11),
   "oman/national2":             FIXED("National Day",18,11),
   "oman/sultans":               FIXED("Sultan's Birthday",19,11),

   "pakistan/birthday of quaid-i-azam":FIXED("Birthday of Quaid-i-Azam",25,12),
   "pakistan/defense":           FIXED("Defense Day",6,9),
   "pakistan/independence":      FIXED("Independence Day",14,8),
   "pakistan/iqbal":             FIXED("Iqbal Day",9,11),
   "pakistan/jinnah":            FIXED("Jinnah Day",11,9),
   "pakistan/pakistan":          FIXED("Pakistan Day",23,3),

   "panama/constitution":        FIXED("Constitution Day",1,3),
   "panama/day of mourning":     FIXED("Day of Mourning",9,1),
   "panama/festival of the black christ":
                                 FIXED("Festival of the Black Christ",21,10),
   "panama/flag":                FIXED("Flag Day",4,11),
   "panama/foundation of panama city":FIXED("Foundation of Panama City",15,8),
   "panama/independence":        FIXED("Independence Day",3,11),
   "panama/independence from spain":FIXED("Independence from Spain",28,11),
   "panama/mothers":             FIXED("Mothers' Day",8,12),
   "panama/national anthem":     FIXED("National Anthem Day",1,11),
   "panama/revolution":          FIXED("Revolution Anniversary Day",11,10),
   "panama/uprising of los santos":FIXED("Uprising of Los Santos",10,11),

   "papua new guinea/independence":FIXED("Independence Day",16,9),
   "papua new guinea/remembrance":FIXED("Remembrance Day",23,7),

   "paraguay/battle of boquer\363n":FIXED("Battle of Boquer\363n Day",29,9),
   "paraguay/constitution":      FIXED("Constitution Day",25,8),
   "paraguay/day of the race":   FIXED("Day of the Race",12,10),
   "paraguay/founding of the city of asunci\363n":
                             FIXED("Founding of the City of Asunci\363n",15,8),
   "paraguay/heroes":            FIXED("Heroes' Day",1,3),
   "paraguay/independence":      FIX_M("Independence Day",14,5,2),
   "paraguay/peace of chaco":    FIXED("Peace of Chaco Day",12,6),
   "paraguay/virgin of caacupe": FIXED("Virgin of Caacupe",8,12),

   "peru/combat of angamos":     FIXED("Combat of Angamos",8,10),
   "peru/independence":          FIX_M("Independence Day",28,7,2),
   "peru/inti raymi fiesta":     FIXED("Inti Raymi Fiesta",24,6),
   "peru/santa rosa de lima":    FIXED("Santa Rosa de Lima",30,8),

   "philippines/araw ng kagitingan":FIXED("Araw ng Kagitingan",6,5),
   "philippines/barangay":       FIXED("Barangay Day",11,9),
   "philippines/bataan":         FIXED("Bataan Day",9,4),
   "philippines/bonifacio":      FIXED("Bonifacio Day",30,11),
   "philippines/christ the king":FIXED("Feast of Christ the King",24,10),
   "philippines/constitution":   FIXED("Constitution Day",17,1),
   "philippines/freedom":        FIXED("Freedom Day",25,2),
   "philippines/independence":   FIXED("Independence Day",12,6),
   "philippines/misa de gallo":  FIX_M("Misa de Gallo",16,12,9),
   "philippines/national heroes":FIXED("National Heroes' Day",27,8),
   "philippines/philippine-american friendship":
                               FIXED("Philippine-American Friendship Day",4,7),
   "philippines/rizal":          FIXED("Rizal Day",30,12),
   "philippines/thanksgiving":   FIXED("Thanksgiving Day",21,9),

   "portugal/cam\365es memorial":FIXED("Cam\365es Memorial Day",10,6),
   "portugal/christmas lisbon also observes the st anthony":
         FIXED("Christmas. Lisbon also observes the feast of St Anthony",13,6),
   "portugal/day of the dead":   FIXED("Day of the Dead",2,11),
   "portugal/liberty":           FIXED("Liberty Day",25,4),
   "portugal/portugal":          FIXED("Portugal Day",10,6),
   "portugal/republic":          FIXED("Republic Day",5,10),
   "portugal/restoration of the independence":
                                 FIXED("Restoration of the Independence",1,12),

   "puerto rico/barbosa":        FIXED("Barbosa Day",27,7),
   "puerto rico/birthday of eugenio maria de hostos":
                             FIXED("Birthday of Eugenio Maria de Hostos",11,1),
   "puerto rico/constitution":   FIXED("Constitution Day",25,7),
   "puerto rico/de diego":       FIXED("De Diego Day",16,4),
   "puerto rico/discovery":      FIXED("Discovery Day",19,11),
   "puerto rico/emancipation":   FIXED("Emancipation Day",22,3),
   "puerto rico/memorial":       FIXED("Memorial Day",28,5),
   "puerto rico/mu\361oz rivera":FIXED("Mu\361oz Rivera Day",17,7),
   "puerto rico/ponce de leon":  FIXED("Ponce de Leon Day",12,8),
   "puerto rico/saint johns":    FIXED("St John's Day",24,6),
   "puerto rico/san juan":       FIXED("San Juan Day",24,6),

   "qatar/anniversary of the amirs accession":
                             FIXED("Anniversary of the Amir's Accession",22,2),
   "qatar/independence":         FIXED("Independence Day",3,9),

   "romania/liberation":         FIXED("Liberation Day",23,8),
   "romania/national":           FIXED("National Day",1,12),
   "romania/public holiday":     FIX_M("Public Holiday",23,8,2),

   "rwanda/armed forces":        FIXED("Armed Forces Day",26,10),
   "rwanda/democracy":           FIXED("Democracy Day",28,1),
   "rwanda/independence":        FIXED("Independence Day",1,7),
   "rwanda/kamarampaka":         FIXED("Kamarampaka Day",25,9),
   "rwanda/peace and unity":     FIXED("Peace and Unity Day",5,7),

   "saint kitts and nevis/carnival":FIX_M("Carnival",31,12,3),
   "saint kitts and nevis/independence":FIXED("Independence Day",19,9),
   "saint kitts and nevis/prince of wales":
                                 FIXED("Prince of Wales' Birthday",14,11),

   "saint lucia/discovery":      FIXED("Discovery Day",13,12),
   "saint lucia/independence":   FIXED("Independence Day",22,2),
   "saint lucia/thanksgiving":   DWDRI("Thanksgiving Day",1,10,1,1),

   "saint vincent and the grenadines/independence":
                                 FIXED("Independence Day",27,10),
   "saint vincent and the grenadines/saint vincent and the grenadines":
                            FIXED("Saint Vincent and the Grenadines Day",22,1),

   "san marino/anniversary of the arengo":
                                 FIXED("Anniversary of the Arengo",25,3),
   "san marino/fall of fascism": FIXED("Fall of Fascism Day",28,7),
   "san marino/investiture of the new captains regent":
                           FIXED("Investiture of the New Captains Regent",1,4),
   "san marino/investiture of the new captains-regent":
                          FIXED("Investiture of the New Captains-Regent",1,10),
   "san marino/liberation":      FIXED("Liberation Day",5,2),
   "san marino/national":        FIXED("National Day",1,4),
   "san marino/san marino":      FIXED("San Marino Day",3,9),

   "sao tome and principe/farmers":FIXED("Farmers' Day",30,9),
   "sao tome and principe/independence":FIXED("Independence Day",12,7),
   "sao tome and principe/martyrs":FIXED("Martyrs' Day",4,2),
   "sao tome and principe/transitional government":
                                 FIXED("Transitional Government Day",21,12),

   "saudi arabia/national":      FIXED("National Day",12,9),
   "saudi arabia/national of saudi arabia":
                                 FIXED("National Day of Saudi Arabia",23,9),

   "senegal/african community":  FIXED("African Community Day",14,7),
   "senegal/independence":       FIXED("Independence Day",4,4),

   "seychelles/independence":    FIXED("Independence Day",29,6),
   "seychelles/liberation":      FIXED("Liberation Day",5,6),

   "sierra leone/independence":  FIXED("Independence Day",27,4),
   "sierra leone/republic":      FIXED("Republic Day",19,4),

// "singapore/birthday of the monkey god":
//                               occurs in February and September/October,
// "singapore/birthday of the saint of the poor":occurs in March,
   "singapore/independence":     FIXED("Independence Day",16,9),
   "singapore/labor":            FIXED("Labour Day",1,5),
// "singapore/mooncake festival":occurs in September,
   "singapore/national holiday": FIXED("National Holiday",9,8),
// "singapore/vesak":            occurs in May,

   "slovakia/independence":	 FIXED("Independence Day",1,1),
   "slovakia/constitution":      FIXED("Constitution Day",1,9),
   "slovakia/day of the slav apostles":FIXED("Day of the Slav Apostles",5,7),
   "slovakia/liberation":        FIXED("Liberation Day",8,5),
   "slovakia/reconciliation":    FIXED("Reconciliation Day",1,11),
   "slovakia/slovak national uprising":
                                 FIXED("Slovak National Uprising Day",29,8),

   "slovenia/culture":		 FIXED("Culture Day",8,2),
   "slovenia/national resistance":FIXED("National Resistance Day",27,4),
   "slovenia/national":		 FIXED("National Day",25,6),
   "slovenia/peoples uprising":  FIXED("People's Uprising",25,7),
   "slovenia/reformation":	 FIXED("Reformation Day",31,10),
   "slovenia/remembrance":	 FIXED("Remembrance Day",1,11),
   "slovenia/independence":	 FIXED("Independence Day",26,12),

   "solomon islands/independence":FIXED("Independence Day",7,7),

   "somalia/foundation of the republic":
                                 FIXED("Foundation of the Republic Day",1,7),
   "somalia/independence":       FIXED("Independence Day",26,6),
   "somalia/revolution":         FIX_M("Revolution Anniversary",21,10,2),

   "south africa/day of the vow":FIXED("Day of the Vow",16,12),
   "south africa/family":        DWDRI("Family Day",1,7,1,2),
   "south africa/kruger":        FIXED("Kruger Day",10,10),
   "south africa/republic":      FIXED("Republic Day",31,5),
   "south africa/settlers":      DWDRI("Settlers' Day",1,9,1,1),
   "south africa/van riebeeck":  FIXED("Van Riebeeck Day",6,4),
   "south africa/workers":       FIXED("Workers' Day",1,5),

   "soviet union/anniversary of the october socialist revolution":
               FIX_M("Anniversary of the October Socialist Revolution",7,11,2),
   "soviet union/victory":       FIXED("Victory Day",9,5),

   "spain/dia de la toma": FIXED("Dia de la Toma",5,1),
   "spain/constitution":         FIXED("Constitution Day",6,12),
   "spain/fiesta de san fermin": FIXED("Fiesta de San Fermin",7,7),
   "spain/fiesta del arbol":     FIXED("Fiesta del Arbol",26,3),
   "spain/grenada":              FIXED("Grenada Day",2,1),
   "spain/hispanidad":           FIXED("Hispanidad Day",12,10),
   "spain/king juan carlos saints":FIXED("King Juan Carlos' Saint's Day",24,6),
   "spain/labor":                FIXED("Labor Day",18,7),
   "spain/national":             FIXED("National Day",12,10),
   "spain/national holiday of spain":FIXED("National Holiday of Spain",12,10),
   "spain/queen isabella":       FIXED("Queen Isabella Day",22,4),
   "spain/saint james":          FIXED("St James Day",25,7),
   "spain/saint joseph the workman":FIXED("St Joseph the Workman",1,5),
// "spain/tomatina":             late August,

// "sri lanka/bandaranaike memorial":[9/26/96],
// "sri lanka/hadji festival":   [4/29/96],
   "sri lanka/independence":     FIXED("Independence Day",4,2),
// "sri lanka/kandy perahera":   occurs on a new moon in July,
   "sri lanka/national heroes":  FIXED("National Heroes Day",22,5),
   "sri lanka/republic":         FIXED("Republic Day",22,5),
   "sri lanka/sinhala and tamil new year":
                                 FIXED("Sinhala and Tamil New Year",14,4),
// "sri lanka/thai pongal":      occurs in January,
// "sri lanka/vesak festival":   occurs in May,

   "sudan/decentralization":     FIXED("Decentralization Day",1,7),
   "sudan/independence":         FIXED("Independence Day",1,1),
   "sudan/national":             FIXED("National Day",25,5),
   "sudan/unity":                FIXED("Unity Day",3,3),
   "sudan/uprising":             FIXED("Uprising Day",6,4),

   "suriname/independence":      FIXED("Independence Day",25,11),
   "suriname/national union":    FIXED("National Union Day",1,7),
   "suriname/revolution":        FIXED("Revolution Day",25,2),

   "swaziland/commonwealth":     DWDRI("Commonwealth Day",1,3,1,2),
   "swaziland/flag":             FIXED("Flag Day",25,4),
// "swaziland/incwala":          December or January,
   "swaziland/kings":            FIXED("King's Birthday",22,7),
   "swaziland/reed dance":       DWDRI("Reed Dance Day",1,7,1,2),
   "swaziland/somhlolo":         FIXED("Somhlolo Day",6,9),

   "switzerland/berchtolds":     FIXED("Berchtold's Day",2,1),
   "switzerland/glarus festival":DWDRI("Glarus Festival",1,4,4,1),
   "switzerland/homstrom":       DWDRI("Homstrom",1,2,7,1),
   "switzerland/independence":   FIXED("Independence Day",1,8),
   "switzerland/may eve":        FIXED("May Day Eve",30,4),

   "syria/beginning of october war":FIXED("Beginning of October War",6,10),
   "syria/evacuation":           FIXED("Evacuation Day",17,4),
   "syria/national":             FIXED("National Day",16,11),
   "syria/revolution":           FIXED("Revolution Day",8,3),
   "syria/union":                FIXED("Union Day",1,9),

   "taiwan/birthday of confucious":FIXED("Birthday of Confucious",28,9),
// "taiwan/birthday of matsu":   occurs in April or May,
   "taiwan/buddha bathing festival":FIXED("Buddha Bathing Festival",8,4),
   "taiwan/chiang kai-shek":     FIXED("Chiang Kai-shek Day",1,11),
   "taiwan/childrens":           FIXED("Children's Day",4,4),
   "taiwan/constitution":        FIXED("Constitution Day",25,12),
   "taiwan/dragon boat festival":FIXED("Dragon Boat Festival",8,6),
   "taiwan/founding of the republic of china":
                              FIX_M("Founding of the Republic of China",1,1,2),
// "taiwan/lantern festival":    occurs on the fifteenth day of the first moon,
   "taiwan/martyrs":             FIXED("Martyrs' Day",29,3),
   "taiwan/national":            FIXED("National Day",10,10),
   "taiwan/restoration":         FIXED("Restoration Day",25,10),
   "taiwan/sun yat-sen":         FIXED("Sun Yat-sen Day",12,11),
   "taiwan/youth":               FIXED("Youth Day",29,3),

   "tanzania/chama cha mapinduzi":FIXED("Chama Cha Mapinduzi Day",5,2),
   "tanzania/heroes":            FIXED("Heroes' Day",1,9),
   "tanzania/independence":      FIXED("Independence Day",9,12),
   "tanzania/naming":            FIXED("Naming Day",29,10),
   "tanzania/saba saba":         FIXED("Saba Saba Day",7,7),
   "tanzania/sultans":           FIXED("Sultan's Birthday",26,8),
   "tanzania/union":             FIXED("Union Day",26,4),
   "tanzania/zanzibar revolution":FIXED("Zanzibar Revolution Day",12,1),

   "thailand/asalapha bupha":    FIXED("Asalapha Bupha Day",28,7),
   "thailand/chakri":            FIXED("Chakri Day",6,4),
   "thailand/chulalongkorn":     FIXED("Chulalongkorn Day",23,10),
   "thailand/constitution":      FIXED("Constitution Day",10,12),
   "thailand/coronation":        FIXED("Coronation Day",5,5),
   "thailand/harvest festival":  FIXED("Harvest Festival Day",11,5),
   "thailand/kings":             FIXED("King's Birthday",5,12),
// "thailand/loy krathong festival":occurs in November,
// "thailand/makha bucha":       full moon in first lunar month,
   "thailand/queens":            FIXED("Queen's Birthday",12,8),
   "thailand/songkran":          FIX_M("Songkran",13,4,3),
// "thailand/state ploughing ceremony":occurs in May [7th?],
// "thailand/visakha bucha":     full moon of the sixth lunar month,

   "gambia/independence":        FIXED("Independence Day",18,2),
   "gambia/moslem new year":     FIXED("Moslem New Year",7,10),

   "togo/anniversary of the failed attack on lome":
                     FIXED("Anniversary of the failed attack on Lom\351",24,9),
   "togo/economic liberation":   FIXED("Economic Liberation Day",24,1),
   "togo/independence":          FIXED("Independence Day",27,4),
   "togo/liberation":            FIXED("Liberation Day",13,1),
   "togo/martyrs of pya":        FIXED("Martyrs of Pya",21,6),
   "togo/victory":               FIXED("Victory Day",24,4),

   "tonga/constitution":         FIXED("Constitution Day",4,11),
   "tonga/emancipation":         FIXED("Emancipation Day",4,6),
   "tonga/king tupou":           FIXED("King Tupou Day",4,12),
   "tonga/kings":                FIXED("King's Birthday",4,7),
   "tonga/princes":              FIXED("Prince's Birthday",4,5),

   "trinidad and tobago/discovery":FIXED("Discovery Day",1,8),
   "trinidad and tobago/emancipation":FIXED("Emancipation Day",1,8),
   "trinidad and tobago/independence":FIXED("Independence Day",31,8),
   "trinidad and tobago/labor":  FIXED("Labor Day",19,6),
   "trinidad and tobago/republic":FIXED("Republic Day",24,9),

   "tunisia/accession":          FIXED("Accession Day",7,11),
   "tunisia/bourguibas":         FIXED("Bourguiba's Day",3,8),
   "tunisia/evacuation":         FIXED("Evacuation Day",15,10),
   "tunisia/independence":       FIXED("Independence Day",20,3),
   "tunisia/independence recognition":
                                 FIXED("Independence Recognition Day",20,7),
   "tunisia/martyrs":            FIXED("Martyrs' Day",9,4),
   "tunisia/memorial":           FIXED("Memorial Day",3,9),
   "tunisia/national holiday of tunisia":
                                 FIXED("National Holiday of Tunisia",1,6),
   "tunisia/national revolution":FIXED("National Revolution Day",18,1),
   "tunisia/republic":           FIXED("Republic Day",25,7),
   "tunisia/tree festival":      FIXED("Tree Festival Day",9,11),
   "tunisia/womens":             FIXED("Women's Day",13,8),
   "tunisia/youth":              FIXED("Youth Day",21,3),

   "turkey/freedom and constitution":
                                 FIXED("Freedom and Constitution Day",27,5),
   "turkey/hidrellez":           FIXED("Hidrellez",6,5),
   "turkey/independence":        FIXED("Independence Day",29,10),
   "turkey/youth and sports":    FIXED("Youth and Sports Day",19,5),
   "turkey/ataturk commemoration": FIXED("Atatürk Commemoration",19,5), // ?
   "turkey/national sovereignty":FIXED("National Sovereignty",23,4),
   "turkey/childrens":           FIXED("Children's day",23,4),
   "turkey/navy and merchant marine":FIXED("Navy and Merchant Marine Day",1,7),
   "turkey/rumis":               FIXED("Rumi's Birthday",17,12),
   "turkey/spring":              FIXED("Spring Day",1,5),
   "turkey/victory":             FIXED("Victory Day",30,8),

   "turks and caicos islands/columbus":FIXED("Columbus Day",10,10),
   "turks and caicos islands/emancipation":FIXED("Emancipation Day",1,8),
   "turks and caicos islands/human rights":
                                 FIXED("International Human Rights Day",24,10),
   "turks and caicos islands/jags mccartney memorial":
                                 FIXED("J.A.G.S. McCartney Memorial Day",6,6),

   "tuvalu/tuvalu":              FIX_M("Tuvalu Day",1,10,2),

   "uganda/heroes":              FIXED("Heroes Day",9,6),
   "uganda/independence":        FIXED("Independence Day",9,10),
   "uganda/martyrs":             FIXED("Martyr's Day",3,6),
   "uganda/nrm/nra victorys":    FIXED("NRM/NRA Victory Celebrations",26,1),
   "uganda/republic":            FIXED("Republic Day",8,9),

   "ukraine/taras shevchenko":   FIXED("Taras Shevchenko Day",9,3),
   "ukraine/ukrainian":          FIXED("Ukrainian Day",22,1),
   "ukraine/ukrainian independence":FIXED("Ukrainian Independence Day",24,8),
   "ukraine/victory":            FIXED("Victory Day",9,3),

   "united arab emirates/accession of the ruler of abu dhabi":
                              FIXED("Accession of the Ruler of Abu Dhabi",6,8),
   "united arab emirates/national":FIXED("National Day",2,12),

   "uruguay/childrens":          FIXED("Chilren's Day",5,1),
   "uruguay/artigas":            FIXED("Artigas Day",19,6),
   "uruguay/battle of las piedras":FIXED("Battle of Las Piedras",18,5),
   "uruguay/blessing of the waters":FIXED("Blessing of the Waters",8,12),
   "uruguay/constitution":       FIXED("Constitution Day",18,7),
   "uruguay/independence":       FIXED("Independence Day",25,8),
   "uruguay/landing of the 33 patriots":
                                 FIXED("Landing of the 33 Patriots Day",19,4),

   "united kingdom/bannockburn": FIXED("Bannockburn Day",24,6),
   "united kingdom/battle of britain":FIXED("Battle of Britain Day",15,9),
   "united kingdom/burns":       FIXED("Burns Day",25,1),
   "united kingdom/day after":   FIXED("The Day After",2,1),
   "united kingdom/guy fawkes":  FIXED("Guy Fawkes Day",5,11),
   "united kingdom/handsel monday":DWDRI("Handsel Monday",1,1,1,1),
   "united kingdom/highland games":DWDR("Highland Games",1,10,6,-1),
   "united kingdom/lammas":      FIXED("Lammas Day",1,8),
   "united kingdom/lord mayors": FIXED("Lord Mayor's Day",9,11),
   "united kingdom/mothering sunday":EASTER_REL("Mothering Sunday",-21), // ?
   "united kingdom/new year":    FIX_M("New Year's Day",1,1,2),
   "united kingdom/oak apple":   FIXED("Oak Apple Day",29,5),
   "united kingdom/orangemans":  FIXED("Orangeman's Day",12,7),
   "united kingdom/pancake tuesday": EASTER_REL("Fat Tuesday",-47),
   "united kingdom/queens":      DWDRI("Queen's Birthday",1,6,4,2),
   "united kingdom/woman peerage":FIXED("Woman Peerage Day",30,1),
   "united kingdom/spring bank holiday":DWDRI("Spring Bank Holiday",1,5,1,4),
   "united kingdom/late summer bank holiday":
                                 DWDRI("Late Summer Bank Holiday",1,8,1,4),
   "united kingdom/orangeman":   FIXED("Orangeman's Day",12,7),
   "united kingdom/scottish new year":FIXED("Scottish New Year",2,1),
   "united kingdom/victoria":DWDRI("Victoria Day",1,5,1,3),
   "united kingdom/autumn holiday":DWDRI("Autumn Holiday",1,9,1,3),


   "us/appomattox":              FIXED("Appomattox Day",9,4),
   "us/armed forces":            DWDRI("Armed Forces Day",1,5,6,3),
   "us/bill of rights":          FIXED("Bill of Rights Day",15,12),
   "us/carnation":               FIXED("Carnation Day",29,1),
   "us/citizenship":             FIXED("Citizenship Day",17,9),
   "us/columbus":                DWDRI("Columbus Day",1,10,1,2),
   "us/election":                DWDRI("Election Day",1,11,1,1),
   "us/flag":                    FIXED("Flag Day",14,6),
   "us/forefathers":             FIXED("Forefathers' Day",21,12),
   "us/inauguration":            FIXED("Inauguration Day",20,1),
   "us/independence":            FIXED("Independence Day",4,7),
   "us/iwo jima":                FIXED("Iwo Jima Day",23,2),
   "us/jefferson davis":         FIXED("Jefferson Davis Day",3,6),
   "us/kosciuszko":              FIXED("Kosciuszko Day",4,2),
   "us/labor":                   DWDRI("Labor Day",1,9,1,1),
   "us/lincolns":                FIXED("Lincoln's Birthday",12,2),
   "us/martin luther king":      FIXED("Martin Luther King Day",15,1),
   "us/memorial":                DWDRI("Memorial Day",30,5,1,-1),
   "us/national freedom":        FIXED("National Freedom Day",1,2),
   "us/navy":                    FIXED("Navy Day",27,10),
   "us/patriots":                DWDRI("Patriot's Day",1,4,1,3),
   "us/robert e lee":            FIXED("Robert E. Lee Day",19,1),
   "us/thomas jeffersons":       FIXED("Thomas Jefferson's Birthday",13,4),
   "us/twelfth night":	         FIXED("Twelfth Night",5,1),
   "us/trivia":                  FIXED("Trivia Day",4,1),
   "us/veterans":                FIXED("Veteran's Day",11,11),
   "us/vietnam":                 FIXED("Vietnam Day",27,1),
   "us/washingtons":             DWDRI("Washington's Birthday",1,2,1,3),
   "us/washington":              DWDRI("Washington Day",1,2,1,3),
   "us/washington-lincoln":      DWDRI("Washington-Lincoln Day",1,2,1,3),
   "us/presidents":              DWDRI("Presidents Day",1,2,1,3),
   "us/womens equality":         FIXED("Women's Equality Day",26,8),
   "us/thanksgiving":            DWDRI("Thanksgiving Day",1,11,4,4),
   "us/day after thanksgiving":  DWDRI("Day after Thanksgiving Day",2,11,5,4),

   "us/alabama/alabama admission":  FIXED("Alabama Admission Day",14,12),
   "us/alabama/confederate memorial":DWDR("Confederate Memorial Day",1,5,1,-1),
   "us/alabama/jefferson davis":    DWDRI("Jefferson Davis Day",1,6,1,1),
   "us/alabama/robert e lee":       DWDRI("Robert E. Lee Day",1,1,1,3),

   "us/alaska/alaska":              DWDRI("Alaska Day",1,10,1,3),
   "us/alaska/alaska admission":    FIXED("Alaska Admission Day",3,1),
   "us/alaska/flag":                FIXED("Flag Day",4,7),
   "us/alaska/sewards":             FIXED("Seward's Day",30,3),

   "us/arizona/american family":    DWDRI("American Family Day",1,8,7,1),
   "us/arizona/arborn":             FIXED("Arbor Day (northern Arizona)",1,1),
   "us/arizona/arbors":          DWDR ("Arbor Day (southern Arizona)",1,2,5,1),
   "us/arizona/arizona admission":  FIXED("Arizona Admission Day",14,2),
   "us/arizona/lincoln":            DWDRI("Lincoln Day",1,2,1,2),

   "us/arkansas/arkansas admission":FIXED("Arkansas Admission Day",15,6),
   "us/arkansas/general douglas macarthur":
                                 FIXED("General Douglas MacArthur Day",26,1),
// "us/arkansas/jefferson davis":   ,
   "us/arkansas/world war ii memorial":FIXED("World War II Memorial Day",14,8),

   "us/california/arbor":           FIXED("Arbor Day",7,3),
   "us/california/california admission":FIXED("California Admission Day",9,9),

   "us/colorado/arbor":             DWDRI("Arbor Day",1,4,5,3),
   "us/colorado/colorado":          DWDRI("Colorado Day",1,8,1,1),

   "us/connecticut/connecticut ratification":
                                 FIXED("Connecticut Ratification Day",9,1),
   "us/delaware/arbor":             FIXED("Arbor Day",22,4),
   "us/delaware/delaware":          FIXED("Delaware Day",7,12),
   "us/delaware/lincolns":          DWDRI("Lincoln's Birthday",1,2,1,1),
   "us/delaware/memorial":          FIXED("Memorial Day",30,5),
   "us/delaware/separation":        FIXED("Separation Day",15,6),
   "us/delaware/swedish colonial":  FIXED("Swedish Colonial Day",29,3),

   "us/florida/arbor":              DWDRI("Arbor Day",1,1,5,3),
   "us/florida/confederate memorial":FIXED("Confederate Memorial Day",26,4),
   "us/florida/farmers":            DWDRI("Farmer's Day",1,10,1,2),
   "us/florida/florida admission":  FIXED("Florida Admission Day",3,3),
   "us/florida/pascua florida":     FIXED("Pascua Florida Day",2,4),
   "us/florida/susan b anthony":    FIXED("Susan B. Anthony Day",15,2),

   "us/georgia/confederate memorial":FIXED("Confederate Memorial Day",26,4),
   "us/georgia/georgia":            FIXED("Georgia Day",12,2),

   "us/hawaii/flag":                FIXED("Flag Day",4,7),
   "us/hawaii/hawaii statehood":    DWDRI("Hawaii Statehood Day",1,8,5,3),
   "us/hawaii/kamehameha":          FIXED("Kamehameha Day",11,6),
   "us/hawaii/kuhio":               FIXED("Kuhio Day",26,3),
   "us/hawaii/lei":                 FIXED("Lei Day",1,5),
   "us/hawaii/wesak flower festival":DWDRI("Wesak Flower Festival",1,4,7,1),

   "us/idaho/idaho admission":      FIXED("Idaho Admission Day",3,7),
   "us/idaho/idaho pioneer":        FIXED("Idaho Pioneer Day",15,6),

   "us/illinois/illinois admission":FIXED("Illinois Admission Day",3,12),
   "us/illinois/memorial":          FIXED("Memorial Day",30,5),

   "us/indiana/indiana admission":  FIXED("Indiana Admission Day",11,12),
   "us/indiana/primary election":   DWDRI("Primary Election Day",2,5,2,1),
   "us/indiana/vincennes":          FIXED("Vincennes Day",24,2),

   "us/iowa/bird":                  FIXED("Bird Day",21,3),
   "us/iowa/independence sunday":   DWDR ("Independence Sunday",4,7,7,-1),
   "us/iowa/iowa admission":        FIXED("Iowa Admission Day",28,12),

   "us/kansas/kansas":              FIXED("Kansas Day",29,1),

   "us/kentucky/franklin d roosevelt":FIXED("Franklin D. Roosevelt Day",30,1),
   "us/kentucky/kentucky statehood":FIXED("Kentucky Statehood Day",1,6),

   "us/louisiana/huey p long":      FIXED("Huey P. Long Day",30,8),
   "us/louisiana/jackson":          FIXED("Jackson Day",8,1),
   "us/louisiana/louisiana admission":FIXED("Louisiana Admission Day",30,4),

   "us/maine/battleship":           FIXED("Battleship Day",15,2),
   "us/maine/maine admission":      FIXED("Maine Admission Day",3,3),

   "us/maryland/defenders":         FIXED("Defenders' Day",12,9),
   "us/maryland/john hanson":       FIXED("John Hanson Day",14,4),
   "us/maryland/maryland":          FIXED("Maryland Day",25,3),
   "us/maryland/maryland admission":FIXED("Maryland Admission Day",28,4),
   "us/maryland/maryland ratification":FIXED("Maryland Ratification Day",14,1),
   "us/maryland/memorial":          FIXED("Memorial Day",30,5),
   "us/maryland/national anthem":   FIXED("National Anthem Day",14,9),
   "us/maryland/repudiation":       FIXED("Repudiation Day",23,11),

   "us/massachusetts/bunker hill":  FIXED("Bunker Hill Day",17,6),
   "us/massachusetts/childrens":    DWDRI("Children's Day",1,6,7,2),
   "us/massachusetts/evacuation":   FIXED("Evacuation Day",17,3),
   "us/massachusetts/john f kennedy":DWDR("John F. Kennedy Day",1,12,7,-1),
   "us/massachusetts/lafayette":    FIXED("Lafayette Day",20,5),
   "us/massachusetts/liberty tree": FIXED("Liberty Tree Day",14,8),
   "us/massachusetts/massachusetts ratification":
                                 FIXED("Massachusetts Ratification Day",6,2),
   "us/massachusetts/spanish-american war memorial":
                               FIXED("Spanish-American War Memorial Day",15,2),
   "us/massachusetts/student government":DWDRI("Student Government Day",1,4,5,1),
   "us/massachusetts/susan b anthony":FIXED("Susan B. Anthony Day",26,8),
   "us/massachusetts/teachers":     DWDRI("Teacher's Day",1,6,7,1),

   "us/michigan/memorial":          DWDRI("Memorial Day",1,5,1,3),
   "us/michigan/michigan":          FIXED("Michigan Day",26,1),

   "us/minnesota/american family":  DWDRI("American Family Day",1,8,7,1),
   "us/minnesota/minnesota":        FIXED("Minnesota Day",11,5),

   "us/mississippi/confederate memorial":
                       DWDRI("Confederate Memorial Day",30,4,1,-1),
   "us/mississippi/jefferson davis":DWDRI("Jefferson Davis Day",1,6,1,1),
   "us/mississippi/robert elee":    DWDRI("Robert E.Lee Day",1,1,1,3),

   "us/missouri/missouri admission":FIXED("Missouri Admission Day",10,8),
   "us/missouri/truman":            FIXED("Truman Day",8,5),

   "us/montana/election":           DWDRI("Election Day",1,11,2,1),

   "us/nebraska/arbor":             FIXED("Arbor Day",22,4),
   "us/nebraska/nebraska state":    FIXED("Nebraska State Day",1,3),

   "us/nevada/nevada":              FIXED("Nevada Day",31,10),

   "us/new hampshire/fast":         DWDRI("Fast Day",1,4,1,4),
   "us/new hampshire/memorial":     FIXED("Memorial Day",30,5),
   "us/new hampshire/new hampshire admission":
                                    FIXED("New Hampshire Admission Day",21,6),

   "us/new jersey/new jersey admission":
	                            FIXED("New Jersey Admission Day",18,12),

   "us/new mexico/arbor":           DWDRI("Arbor Day",1,3,5,2),
   "us/new mexico/memorial":        FIXED("Memorial Day",25,5),

   "us/new york/audubon":           DWDRI("Audubon Day",1,4,5,2),
   "us/new york/flag":              DWDRI("Flag Day",1,6,7,2),
   "us/new york/martin luther king":DWDRI("Martin Luther King Day",1,1,7,3),
   "us/new york/new york ratification":FIXED("New York Ratification Day",26,7),
   "us/new york/verrazano":         FIXED("Verrazano Day",17,4),

   "us/north carolina/confederate memorial":
                                 FIXED("Confederate Memorial Day",10,5),
   "us/north carolina/halifax resolutions":
                                 FIXED("Halifax Resolutions Day",12,4),
   "us/north carolina/mecklenburg": FIXED("Mecklenburg Day",20,5),

   "us/north dakota/north dakota admission":
                                 FIXED("North Dakota Admission Day",2,11),

   "us/ohio/martin luther king":    DWDRI("Martin Luther King Day",1,1,1,3),
   "us/ohio/ohio admission":        FIXED("Ohio Admission Day",1,3),

   "us/oklahoma/bird":              FIXED("Bird Day",1,5),
   "us/oklahoma/cherokee strip":    FIXED("Cherokee Strip Day",16,9),
   "us/oklahoma/indian":            FIXED("Indian Day",12,8),
   "us/oklahoma/oklahoma":          FIXED("Oklahoma Day",22,4),
// "oklahoma/oklahoma heritage week":week of November 16,
   "us/oklahoma/oklahoma historical":FIXED("Oklahoma Historical Day",10,10),
   "us/oklahoma/oklahoma statehood":FIXED("Oklahoma Statehood Day",16,11),
   "us/oklahoma/senior citizens":   FIXED("Senior Citizens Day",9,6),
   "us/oklahoma/will rogers":       FIXED("Will Rogers Day",4,11),
// "oklahoma/youth":                on first day of Spring,

   "us/oregon/lincolns":            DWDRI("Lincoln's Birthday",1,2,1,1),
   "us/oregon/oregon statehood":    FIXED("Oregon Statehood Day",14,2),

   "us/pennsylvania/barry":         FIXED("Barry Day",13,9),
   "us/pennsylvania/charter":       FIXED("Charter Day",4,3),
   "us/pennsylvania/pennsylvania admission":
                                 FIXED("Pennsylvania Admission Day",12,12),

   "us/rhode island/arbor":         DWDRI("Arbor Day",1,5,5,2),
   "us/rhode island/rhode island admission":
                                 FIXED("Rhode Island Admission Day",29,5),
   "us/rhode island/rhode island independence":
                                 FIXED("Rhode Island Independence Day",4,5),
   "us/rhode island/victory":       DWDRI("Victory Day",1,8,1,2),

   "us/south carolina/confederate memorial":
                                 FIXED("Confederate Memorial Day",10,5),
   "us/south carolina/south carolina admission":
                                 FIXED("South Carolina Admission Day",23,5),

   "us/south dakota/memorial":      FIXED("Memorial Day",30,5),
   "us/south dakota/pioneers":      DWDRI("Pioneers' Day",1,10,1,2),
   "us/south dakota/south dakota admission":
                                 FIXED("South Dakota Admission Day",2,11),

   "us/tennessee/confederate memorial":FIXED("Confederate Memorial Day",3,6),
   "us/tennessee/tennesse statehood":FIXED("Tennesse Statehood Day",1,6),

   "us/texas/alamo":                FIXED("Alamo Day",6,3),
   "us/texas/austin":               FIXED("Austin Day",3,11),
   "us/texas/confederate heroes":   FIXED("Confederate Heroes Day",19,1),
   "us/texas/juneteenth":           FIXED("Juneteenth",19,6),
   "us/texas/lyndon b johnsons":    FIXED("Lyndon B. Johnson's Birthday",27,8),
   "us/texas/san jacinto":          FIXED("San Jacinto Day",21,4),
   "us/texas/texas admission":      FIXED("Texas Admission Day",29,12),
   "us/texas/texas independence":   FIXED("Texas Independence Day",2,3),
   "us/texas/texas pioneers":       FIXED("Texas Pioneers' Day",12,8),

   "us/utah/arbor":                 DWDR("Arbor Day",1,5,5,-1),
   "us/utah/pioneer":               FIXED("Pioneer Day",24,7),
   "us/utah/utah admission":        FIXED("Utah Admission Day",4,1),

   "us/vermont/bennington battle":  FIXED("Bennington Battle Day",16,8),
   "us/vermont/memorial":           FIXED("Memorial Day",30,5),
   "us/vermont/town meeting":       DWDRI("Town Meeting Day",1,3,2,1),
   "us/vermont/vermont":            FIXED("Vermont Day",4,3),

   "us/virginia/cape henry":        FIXED("Cape Henry Day",26,4),
   "us/virginia/confederate memorial":FIXED("Confederate Memorial Day",30,5),
   "us/virginia/crater":            FIXED("Crater Day",30,7),
// "virginia/jack jouett":       occurs in June,
   "us/virginia/jamestown":         FIXED("Jamestown Day",13,5),
   "us/virginia/lee-jackson":       DWDRI("Lee-Jackson Day",1,1,1,3),
   "us/virginia/royalist fast":     FIXED("Royalist Fast Day",30,1),
   "us/virginia/virginia ratification":FIXED("Virginia Ratification Day",25,6),

   "us/washington/washington admission":
   	                               FIXED("Washington Admission Day",11,11),

   "us/washington dc/arbor":        DWDRI("Arbor Day",1,4,5,3),

   "us/west virginia/west virginia":FIXED("West Virginia Day",20,6),

   "us/wisconsin/primary election": DWDRI("Primary Election Day",1,9,2,1),
   "us/wisconsin/wisconsin":        FIXED("Wisconsin Day",29,5),

   "us/wyoming/arbor":              DWDR("Arbor Day",1,5,1,-1),
   "us/wyoming/primary election":   DWDRI("Primary Election Day",1,9,2,2),
   "us/wyoming/wyoming":            FIXED("Wyoming Day",10,12),
   "us/wyoming/wyoming statehood":  FIXED("Wyoming Statehood Day",10,7),

   "vanuatu/constitution":       FIXED("Constitution Day",5,10),
   "vanuatu/independence":       FIXED("Independence Day",30,7),

   "vatican city/world peace":   FIXED("World Peace Day",1,1),
   "vatican city/lateranensi pacts":
	FIXED("Anniversary of Lateranensi Pacts",11,2),
   "vatican city/pope election":    
        FIXED("Anniversary of the Pope's election",16,10),

   
   "vatican city/anniversary of the beginning of the john paul ii pontificate":
   FIXED("Anniversary of the Beginning of the John Paul II Pontificate",22,10),
   "vatican city/john paul ii namesday":FIXED("John Paul II Namesday",4,11),

   "venezuela/battle of carabobo":FIXED("Battle of Carabobo Day",24,6),
   "venezuela/bolivars":         FIXED("Bolivar's Birthday",24,7),
   "venezuela/civil servants":   FIXED("Civil Servants' Day",4,9),
   "venezuela/declaration of independence":
                                 FIXED("Declaration of Independence",19,4),
   "venezuela/independence":     FIXED("Independence Day",5,7),
   "venezuela/teachers":         FIXED("Teacher's Day",15,1),

   "vietnam/day of the nation":  FIXED("Day of the Nation",2,9),
   "vietnam/emperor-founder hung vuongs":
                                 FIXED("Emperor-Founder Hung Vuong's Day",7,4),
   "vietnam/founding of the communist party":
                                 FIXED("Founding of the Communist Party",3,2),
   "vietnam/independence":       FIXED("Independence Day",14,6),
   "vietnam/liberation of saigon":FIXED("Liberation of Saigon",30,4),
// "vietnam/tet nguyen dan":     occurs in January of February,
// "vietnam/thanh minh":         between April 5 and 20,

   "virgin islands/danish west indies emancipation":
                              FIXED("Danish West Indies Emancipation Day",3,7),
   "virgin islands/hurricane supplication":
                                 DWDRI("Hurricane Supplication Day",1,7,1,4),
   "virgin islands/hurricane thanksgiving":
                                 DWDRI("Hurricane Thanksgiving Day",1,10,1,3),
   "virgin islands/liberty":     DWDRI("Liberty Day",1,11,1,1),
   "virgin islands/nicole robin":FIXED("Nicole Robin Day",4,8),
   "virgin islands/organic act": DWDRI("Organic Act Day",1,6,1,3),
   "virgin islands/transfer":    DWDR("Transfer Day",1,4,1,-1),

   "western samoa/arbor":        DWDRI("Arbor Day",1,11,5,1),
   "western samoa/independence": FIXED("Independence Celebration Day",1,6),
   "western samoa/independence2":FIX_M("Independence Celebration",1,6,3),
   "western samoa/white sunday": DWDRI("White Sunday",1,10,7,2),

   "yemen/corrective movement":  FIXED("Corrective Movement Anniversary",13,6),
   "yemen/national":             FIXED("National Day",14,10),

   "yugoslavia/constitution":    FIXED("Constitution Day (Serbia)",28,3),
   "yugoslavia/national":	 FIXED("National Day",27,4),
   "yugoslavia/victory":	 FIXED("Victory Day",9,5),
   "yugoslavia/freedom fighters":FIXED("Freedom Fighters' Day",4,7),
   "yugoslavia/freedom serbia":  FIXED("Freedom Rising Day (Serbia)",7,7),
   "yugoslavia/freedom montenegro":
                            FIXED("Freedom Rising Day (Montenegro)",14,7),
   "yugoslavia/republic":	 FIXED("Republic Day",29,11),

   "zaire/armed forces":         FIXED("Armed Forces Day",17,11),
   "zaire/constitution":         FIXED("Constitution Day",24,6),
   "zaire/day of the martyrs for independence":
                              FIXED("Day of the Martyrs for Independence",4,1),
   "zaire/independence":         FIXED("Independence Day",30,6),
   "zaire/mpr":                  FIXED("MPR Day",20,5),
   "zaire/naming":               FIXED("Naming Day",27,10),
   "zaire/new regime":           FIXED("New Regime Day",24,11),
   "zaire/parents":              FIXED("Parent's Day",1,8),
   "zaire/presidents":           FIXED("President's Day",14,10),
   "zaire/youth/presidents":     FIXED("Youth Day/President's Birthday",14,10),

   "zambia/farmers":             DWDRI("Farmers' Day",1,8,1,1),
   "zambia/heroes":              DWDRI("Heroes' Day",1,7,1,1),
   "zambia/independence":        FIXED("Independence Day",24,10),
   "zambia/unity":               DWDRI("Unity Day",1,7,2,1),
   "zambia/youth":               DWDRI("Youth Day",1,8,1,2),
   "zambia/youth2":              DWDRI("Youth Day",1,3,6,2),

   "zimbabwe/heroess":           FIX_M("Heroes' Days",11,8,2),
   "zimbabwe/independence":      FIXED("Independence Day",18,4),
   "zimbabwe/workers":           FIXED("Workers' Day",1,5),

// ----------------------------------------------------------------
// namedays
// ----------------------------------------------------------------

   "namedays/sweden":({.Namedays.Sweden}),
   "namedays/hungary":({.Namedays.Hungary}),
]);

// ----------------------------------------------------------------------
// country information
// ----------------------------------------------------------------------

mapping country_events=
([
// ----------------------------------------------------------------------
// verified
// ----------------------------------------------------------------------

   "sweden":
   ({
      "halloween",
      "labor|hf",
      "new years eve",
      "new year|hf",
      "un|f",
      "womens",

      "c/all saints",
      "c/annunciation",
      "c/ascension|h",
      "c/christmas eve",
      "c/christmas|hf",
      "c/easter monday|h",
      "c/easter eve",
      "c/easter|hf",
      "c/epiphany|h",
      "c/fat tuesday",
      "c/good friday|h",
      "c/palm sunday|h",
      "c/pentecost monday|h",
      "c/pentecost eve",
      "c/pentecost|hf",
      "c/candlemas",
      "c/advent 1|h",
      "c/advent 2|h",
      "c/advent 3|h",
      "c/advent 4|h",
   
      "saints/stephen|h",  // annandag jul
      "saints/john the baptist",
      "saints/lucy",
      "saints/valentine",

      "sweden/all saints|h",
      "sweden/crown princess birthtday|f",
      "sweden/crown princess nameday|f",
      "sweden/flag|f",
      "sweden/nobel|f",
      "sweden/kings birthday|f",
      "sweden/kings nameday 2|f",
      "sweden/kings nameday|f",
      "sweden/midsummers eve",
      "sweden/midsummer|hf",
      "sweden/queens birthday|f",
      "sweden/queens nameday|f",
      "sweden/valborg eve",
      "sweden/waffle",
      "sweden/mothers",
      "sweden/fathers",

      "namedays/sweden",
      "sunday|h",
   }),

// argentina
// source: Julio César Gázquez

   "argentina":
 // "argentina/race" ?
 // "carnival [shrove tuesday?]" ?
 // "death of general san martin" ?
   ({ 
      "new year|h",
      "labor|h",
	 
      "argentina/revolution|h",
      "argentina/soberanys|h",	
      "argentina/flag|h",		
      "argentina/independence|h",    
      "argentina/gral san martin|h", 
      "argentina/race|h",            
      "argentina/malvinas|h",         // ?
	 
      "c/epiphany",
      "c/good friday|h",
      "c/easter|h",
      "c/christmas|h",
      "c/immaculate conception|h",
      "c/assumption",  
      "c/all saints",       
      "c/corpus christi",
      "c/holy thursday",   
      "c/holy saturday",   

      "sunday|h",
   }),

// hungary
// source: Csongor Fagyal <concept@conceptonline.hu>

   "hungary":
   ({
      "new year|h",
      "hungary/revolution1848|h",
      "hungary/labor|h",
      "hungary/constitution|h",
      "hungary/revolution56|h",
      "c/christmas2d|h",
      "c/easter|h",
      "c/easter monday|h",
      "c/pentecost|h",
      "c/pentecost monday|h",
      "sunday|h",
      "namedays/hungary",
   }),

// russia
// source: Michael Schvryev

   "russia":
   ({ "orthodox/christmas|h",
      "labor|h",
      "new year|h",
      "orthodox/easter|h",
      "russia/state sovereignity|h",
      "russia/victory|h",
      "russia/valpurgis|h",
      "russia/warriors|h",
      "russia/teachers|h",
      "womens|h" }),

// ----------------------------------------------------------------------
// not verified
// ----------------------------------------------------------------------

   "afghanistan":
 // "afghanistan/jeshyn-afghans" ?
 // "deliverance" ?
 // "eid al-qorban" ?
 // "first of ramadan the sabbath is on friday" ?
 // "independence festival" ?
 // "national holidays are eid al-fetr" ?
 // "prophets" ?
 // "pushtunistan" ?
 // "sunday are normal works" ?
 // "with work ceasing on noon thursday saturday" ?
 // "yom ashura" ?
   ({ "afghanistan/afghan new year",
      "afghanistan/childrens",
      "afghanistan/independence",
      "afghanistan/mothers",
      "afghanistan/national assembly",
      "afghanistan/revolution",
      "afghanistan/workers" }),

   "albania":
   ({ "albania/army",
      "albania/independence",
      "albania/liberation",
      "albania/proclamation of the republic",
      "new year 2d",
      "womens" }),

   "algeria":
 // "ashora" ?
 // "first of ramadan" ?
 // "id al-adha" ?
 // "id al-fitr" ?
 // "islamic new year" ?
 // "leilat al-meiraj" ?
 // "mouloud" ?
   ({ "algeria/independence",
      "algeria/national",
      "algeria/revolution",
      "labor",
      "new year" }),

   "american baptist":
   ({ "american baptist/roger williams" }),

   "american samoa":
   ({ "+us",
      "american samoa/flag",
      "c/christmas",
      "labor",
      "new year" }),

   "andorra":
   ({ "andorra/national feast",
      "c/boxing",
      "c/christmas",
      "c/epiphany",
      "c/easter monday",
      "c/good friday",
      "new year" }),

   "anglican church":
   ({ "anglican church/name of jesus" }),

   "angola":
 // "yet are generally treated as public holidays these are youth" ?
   ({ "angola/armed forces",
      "angola/independence",
      "angola/mpla foundation",
      "angola/national heros",
      "angola/national holiday",
      "angola/pioneers",
      "angola/victory",
      "angola/workers",
      "angola/youth",
      "angola/martyrs",
      "family",
      "new year" }),

   "anguilla":
 // "august monday" ?
 // "august thursday" ?
 // "constitution" ?
 // "queens official" ?
 // "separation" ?
 // "spring bank holiday" ?
   ({ "anguilla/anguilla",
      "c/christmas",
      "c/easter monday",
      "c/good friday",
      "c/whitmonday",
      "labor",
      "new year" }),

   "antigua and barbuda":
 // "carnival" ?
   ({ "antigua and barbuda/independence",
      "c/boxing",
      "c/christmas",
      "c/easter monday",
      "c/good friday",
      "c/whitmonday",
      "caribbean/caricom",
      "labor",
      "new year" }),

   "armenia":
 // "armenia/Easter (Orthodox)" ?
   ({ "armenia/christmas",
      "armenia/martyrs",
      "armenia/republic" }),

   "aruba":
 // "lenten carnival" ?
   ({ "aruba/aruba flag",
      "british commonwealth/queens",
      "c/ascension",
      "c/christmas",
      "c/easter",
      "c/whitmonday",
      "labor",
      "new year" }),

   "australia":
   ({ "australia/australia|h",
      "australia/eight hour",
      "new year|h",
      "c/good friday|h",
      "c/easter monday|h",
      "c/easter|h",
      "anzac|h",
      "united kingdom/queens|h",
      "c/christmas|h",
      "c/boxing|h" }),

   "australia/new south wales":
   ({ "+australia",
      "australia/summer bank holiday|h"}),
   "australia/northern territory":
   ({ "+australia",
      "labor|h" }),
   "australia/queensland":
   ({ "+australia",
//    "australia/labor_oct|h", 
      "australia/summer bank holiday|h"}),
   "australia/south australia":
   ({ "+australia",
      "australia/proclamation|h",
      "labor|h", }),
   "australia/tasmania":
   ({ "+australia",
      "australia/recreation",
      "australia/hobert regatta|h",
//    "australia/labor_mar|h",
      "australia/spring bank holiday|h" }),
   "australia/victoria":
   ({ "+australia",
      "australia/show|h",
      "australia/second new years",
//    "australia/labor_mar|h",
//    "australia/cup|h",
      "australia/spring bank holiday|h"}),
   "australia/western australia":
   ({ "+australia",
      "australia/foundation|h",
      "australia/show|h",
//    "australia/labor_mar|h",
      "australia/spring bank holiday|h" }),

   "austria":
   ({ "c/all saints",
      "austria/national",
      "austria/national holiday",
      "austria/national holiday of austria",
      "austria/republic",
      "austria/second republic",
      "c/ascension",
      "c/assumption",
      "c/christmas",
      "c/corpus christi",
      "c/easter monday",
      "c/immaculate conception",
      "c/whitmonday",
      "c/epiphany",
      "labor",
      "new year" }),

   "bahamas":
   ({ "bahamas/fox hill",
      "bahamas/independence",
      "bahamas/labor",
      "c/christmas",
      "c/easter monday",
      "c/good friday",
      "c/whitmonday",
      "caribbean/emancipation",
      "columbus",
      "labor",
      "new year" }),

   "bahrain":
 // "ashoura" ?
 // "first of muharram" ?
 // "first of ramadan" ?
 // "id al-adha" ?
 // "id al-fitr" ?
 // "leilat al-meiraj" ?
 // "mouloud" ?
 // "national" ?
   ({ "bahrain/national of bahrain",
      "new year" }),

   "bangladesh":
 // "buddha purnima" ?
 // "durga puja" ?
 // "eid-ul-azha" ?
 // "eid-ul-fitr" ?
 // "jamaat-ul-bida" ?
 // "janmastami" ?
 // "martyrs" ?
 // "shab-e-barat" ?
 // "shab-e-qudr" ?
 // "yom ashura" ?
   ({ "bangladesh/bengali new year",
      "bangladesh/independence",
      "bangladesh/national mourning",
      "bangladesh/revolution",
      "bangladesh/victory",
      "c/christmas",
      "may day" }),

   "barbados":
 // "kadooment" ?
   ({ "barbados/errol barrow",
      "barbados/independence",
      "barbados/united nations",
      "c/christmas",
      "c/easter monday",
      "c/good friday",
      "c/whitmonday",
      "labor",
      "new year" }),
   
   "belgium":
   ({ "c/all saints",
      "armistice",
      "belgium/dynasty",
      "belgium/independence",
      "belgium/kings",
      "c/all souls",
      "c/ascension",
      "c/assumption",
      "c/boxing",
      "c/christmas",
      "c/easter monday",
      "c/whitmonday",
      "labor",
      "new year" }),

   "belize":
   ({ "belize/baron bliss",
      "belize/commonwealth",
      "belize/garifuna",
      "belize/independence",
      "belize/national",
      "belize/pan american",
      "belize/saint georges cay",
      "c/boxing",
      "c/christmas",
      "c/easter monday",
      "c/good friday",
      "c/holy saturday",
      "labor",
      "new year" }),

   "benin":
 // "id al-adha" ?
 // "id al-fitr" ?
   ({ "c/all saints",
      "benin/armed forces",
      "benin/harvest",
      "benin/martyrs",
      "benin/national",
      "benin/workers",
      "benin/youth",
      "c/ascension",
      "c/assumption",
      "c/christmas",
      "c/easter monday",
      "c/good friday",
      "c/whitmonday",
      "new year" }),

   "bermuda":
 // "bermuda/Labor Day (Sep)" ?
 // "cup match" ?
 // "queens official" ?
   ({ "bermuda/bermuda",
      "bermuda/peppercorn",
      "bermuda/remembrance",
      "bermuda/somers",
      "c/christmas",
      "c/good friday",
      "new year" }),

   "bhutan":
 // "buddhist holidays" ?
 // "dussehra" ?
   ({ "bhutan/kings",
      "bhutan/national of bhutan" }),

   "bolivia":
 // "also regional public holidays" ?
 // "bolivia/alacitas" ?
 // "bolivia/la paz" ?
 // "carnival " ?
 // "christmas there" ?
 // "shrove tue)" ?
   ({ "bolivia/independence",
      "bolivia/martyrs",
      "bolivia/memorial",
      "bolivia/national",
      "c/all souls",
      "c/corpus christi",
      "c/good friday",
      "labor",
      "new year" }),

   "bosnia and herzegovina":
 // "july 27" ?
 // "november 25" ?
   ({ "bosnia and herzegovina/labors",
      "new year" }),

   "botswana":
 // "christmas multiples" ?
 // "given for most of these holidays" ?
 // "national holidays" ?
   ({ "botswana/botswana",
      "botswana/presidents",
      "c/ascension",
      "c/easter",
      "new year" }),

   "brazil":
 // "carnival" ?
 // "christmas sao paulo has the additional regional public holiday sao paulo" ?
 // "national holidays" ?
 // "saint sebastians feast" ?
   ({ "brazil/brasilia",
      "brazil/discovery",
      "brazil/independence",
      "brazil/proclamation of the republic",
      "brazil/republic",
      "brazil/sao paulo",
      "brazil/tiradentes",
      "c/all souls",
      "c/corpus christi",
      "c/good friday",
      "labor",
      "new year" }),

   "british virgin islands":
 // "august holiday " ?
 // "british virgin islands/sovereigns observance" ?
 // "commonwealth" ?
 // "queens official" ?
 // "saint ursulas" ?
 // "tue" ?
 // "wed)" ?
   ({ "british commonwealth/prince of wales",
      "british virgin islands/territory",
      "c/christmas",
      "c/easter monday",
      "c/good friday",
      "c/whitmonday",
      "new year" }),

   "brunei":
 // "beginning of ramadan" ?
 // "brunei/Hari Raya Haji (Feast of the Sacrifice)" ?
 // "brunei/Hari Raya Puasa (end of Ramadan)" ?
 // "brunei/Hizrah (Islamic New Year)" ?
 // "brunei/Isra Mera (Leilat al-Meiraj)" ?
 // "brunei/Memperingati Nuzul Al-Quran (Revelation of the Koran)" ?
 // "chinese new year" ?
 // "hari mouloud" ?
   ({ "brunei/national",
      "brunei/royal brunei armed forces",
      "brunei/sultans",
      "c/christmas",
      "new year" }),

   "bulgaria":
 // "national holidays" ?
   ({ "bulgaria/babin den",
      "bulgaria/botev",
      "bulgaria/education",
      "bulgaria/liberation",
      "bulgaria/national",
      "bulgaria/national holiday",
      "bulgaria/viticulturists",
      "c/christmas",
      "labor",
      "new year 2d" }),

   "burkina faso":
 // "id al-adha" ?
 // "id al-fitr" ?
 // "mouloud" ?
   ({ "c/all saints",
      "burkina faso/anniversary of 1966 coup",
      "burkina faso/labor",
      "burkina faso/national",
      "burkina faso/republic",
      "burkina faso/youth",
      "c/ascension",
      "c/assumption",
      "c/christmas",
      "c/easter monday",
      "c/whitmonday",
      "new year" }),

   "burundi":
 // "umugamuro" ?
   ({ "c/all saints",
      "burundi/anniversary of the revolution",
      "burundi/independence",
      "burundi/murder of the hero",
      "burundi/victory of uprona",
      "c/ascension",
      "c/assumption",
      "c/christmas",
      "c/easter monday",
      "labor",
      "new year" }),

   "cambodia":
 // "cambodia/chaul chhnam" ?
 // "cambodia/visak bauchea" ?
 // "chaul chhnam" ?
   ({ "cambodia/ancestors",
      "cambodia/day of hatred",
      "cambodia/independence",
      "cambodia/khmer republic constitution",
      "cambodia/memorial",
      "cambodia/national",
      "labor" }),

   "cameroon":
   ({ "c/all saints",
      "c/ascension",
      "c/assumption",
      "c/christmas",
      "c/easter monday",
      "c/good friday",
      "c/pentecost monday",
      "cameroon/commemoration of national institutions",
      "cameroon/constitution",
      "cameroon/human rights",
      "cameroon/independence",
      "cameroon/unification",
      "cameroon/youth",
      "labor" }),

   "canada":
 // "also public holidays specific to each province" ?
 // "canada/Labor Day (Sep.)" ?
 // "canada/discovery" ?
 // "canada/thanksgiving" ?
 // "christmas there" ?
 // "thanksgiving" ?
   ({ "c/easter monday",
      "c/good friday",
      "canada/civic holiday",
      "canada/dominion",
      "canada/klondike gold discovery",
      "canada/labor",
      "canada/remembrance",
      "canada/victoria",
      "new year" }),

   "cape verde":
   ({ "c/all saints",
      "c/assumption",
      "c/christmas",
      "cape verde/independence",
      "cape verde/national",
      "cape verde/national heroes",
      "labor",
      "new year" }),

   "caribbean":
   ({ "caribbean/caricom",
      "caribbean/emancipation",
      "caribbean/schoelcher" }),

   "cayman islands":
 // "constitution" ?
 // "discovery" ?
 // "queens official" ?
 // "remembrance" ?
   ({ "c/ash wednesday",
      "c/christmas",
      "c/easter monday",
      "c/good friday",
      "new year" }),

   "central african republic":
 // "national holidays" ?
   ({ "c/all saints",
      "c/ascension",
      "c/assumption",
      "c/christmas",
      "c/easter monday",
      "c/pentecost",
      "central african republic/first government",
      "central african republic/republic",
      "central african republic/boganda",
      "central african republic/day after republic",
      "central african republic/independence",
      "central african republic/national holiday",
      "labor",
      "new year" }),

   "central america":
   ({ "central america/independence" }),

   "chad":
 // "aid el kebir" ?
 // "aid el sequir" ?
 // "mouloud el nebi" ?
   ({ "africa",
      "c/all saints",
      "c/ascension",
      "c/assumption",
      "c/christmas",
      "c/easter monday",
      "c/pentecost",
      "chad/creation of the union of central african states",
      "chad/independence",
      "chad/national",
      "chad/republic",
      "chad/traditional independence []",
      "labor",
      "new year" }),

   "chile":
   ({ "saints/paul",
      "saints/peter",
      "c/all saints",
      "c/ascension",
      "c/assumption",
      "c/christmas",
      "c/corpus christi",
      "c/good friday",
      "c/immaculate conception",
      "chile/armed forces",
      "chile/independence",
      "chile/independence2",
      "chile/national holiday",
      "chile/navy",
      "columbus",
      "labor",
      "new year" }),

   "china":
 // "army" ?
 // "china/army" ?
 // "chinese new year" ?
 // "dragon boat festival" ?
 // "lantern festival" ?
 // "mid-autumn festival" ?
 // "qing ming" ?
 // "seventh eve" ?
   ({ "china/ccps",
      "china/childrens",
      "china/national",
      "china/teachers",
      "china/tree planting",
      "china/youth",
      "labor",
      "new year 2d",
      "womens" }),

   "columbia":
 // "battle of boyacaday" ?
 // "national holidays" ?
 // "sacred heart" ?
 // "saint josephs" ?
   ({ "saints/paul",
      "saints/peter",
      "c/all saints",
      "c/ascension",
      "c/assumption",
      "c/christmas",
      "c/corpus christi",
      "c/good friday",
      "c/holy thursday",
      "c/immaculate conception",
      "columbia/battle of boyaca",
      "columbia/cartagena independence",
      "columbia/independence",
      "columbia/thanksgiving",
      "columbus",
      "c/epiphany",
      "labor",
      "new year" }),

   "comoros":
 // "ashoura" ?
 // "beginning of ramadan" ?
 // "id al-adha" ?
 // "id al-fitr" ?
 // "islamic new year" ?
 // "leilat al-meiraj" ?
 // "mouloud" ?
   ({ "comoros/anniversary of president abdallahs assassination",
      "comoros/independence" }),

   "congo":
 // "national holidays" ?
   ({ "c/all saints",
      "c/ascension",
      "c/assumption",
      "c/christmas",
      "c/easter monday",
      "c/pentecost",
      "congo/day before army",
      "congo/day before independence",
      "congo/day before national",
      "congo/independence",
      "congo/martyr of independence",
      "new year" }),

   "corsica":
   ({ "corsica/napoleons" }),

   "costa rica":
 // "costa rica/guanacaste" ?
   ({ "costa rica/battle of rivas",
      "costa rica/independence" }),

   "croatia":
   ({ "c/assumption",
      "c/christmas",
      "croatia/national holiday",
      "croatia/republic",
      "c/epiphany",
      "labor",
      "new year" }),

   "cuba":
 // "cuba/beginning of independence wars" ?
   ({ "cuba/independence",
      "cuba/liberation",
      "cuba/revolution",
      "cuba/wars of independence",
      "labor" }),

   "cyprus/greece":
   ({ "new year",
      "c/epiphany",
      "cyprus/name day",
      "cyprus/green monday",
      "greece/independence",
      "c/good friday",
      "c/holy saturday",
      "c/easter monday",
      "cyprus/greek national",
      "c/christmas",
      "c/boxing",
      "cyprus/kataklysmos",
      "cyprus/submersion of the holy cross" }),

   "cyprus/turkey":
//    "id al-fitr"
//    "id al-adha"
//    "birth of the prophet"
   ({ "new year",
      "turkey/national sovereignty",
      "turkey/childrens",
      "labor",
      "turkey/youth and sports",
      "cyprus/communal resistance",
      "turkey/victory",
      "turkey/independence",
      "cyprus/kataklysmos",
      "cyprus/peace and freedom",
      "cyprus/submersion of the holy cross" }),

   "czech republic":
   ({ "c/christmas",
      "saints/stephen",
      "c/christmas eve",
      "c/easter monday",
      "czech republic/death of jan hus",
      "czech republic/introduction of christianity",
      "labor",
      "new year" }),

   "denmark":
 // "denmark/public holidays" ?
   ({ "denmark/birthday of queen margrethe ii",
      "denmark/common prayer",
      "denmark/constitution",
      "denmark/fjortende februar",
      "denmark/liberation",
      "denmark/valdemars" }),

   "djibouti":
 // "id al-adha" ?
 // "id al-fitr" ?
 // "independence" ?
 // "mawloud" ?
 // "muharram" ?
   ({ "c/christmas",
      "djibouti/independence feast",
      "djibouti/workers",
      "new year" }),

   "dominica":
 // "carnival" ?
   ({ "c/christmas",
      "c/easter monday",
      "c/good friday",
      "c/whitmonday",
      "c/epiphany",
      "dominica/community service",
      "dominica/emancipation",
      "dominica/independence",
      "labor",
      "new year" }),

   "ecuador":
 // "battle of pichincha" ?
 // "carnival" ?
 // "cuenca independence" ?
 // "ecuador/cuenca independence" ?
 // "ecuador/independence" ?
 // "founding of quito" ?
 // "guayaquil independence" ?
 // "independence" ?
 // "simsn bolmvars" ?
   ({ "c/all saints",
      "c/all souls",
      "c/christmas",
      "c/good friday",
      "c/holy thursday",
      "columbus",
      "ecuador/battle of pichincha ",
      "ecuador/guayaquils independence",
      "ecuador/simon bolivars",
      "c/epiphany",
      "labor",
      "new year" }),

   "egypt":
 // "id al-adha" ?
 // "id al-fitr" ?
 // "islamic new year" ?
 // "leilat al-meiraj" ?
 // "mouled al nabi" ?
   ({ "egypt/armed forces",
      "egypt/evacuation",
      "egypt/leylet en-nuktah",
      "egypt/revolution",
      "egypt/sham al-naseem",
      "egypt/sinai liberation",
      "egypt/suez",
      "egypt/victory",
      "labor",
      "new year",
      "unity" }),

   "el salvador":
 // "el salvador/Discovery of America (Columbus Day)" ?
   ({ "c/all souls",
      "c/christmas",
      "c/corpus christi",
      "c/easter",
      "el salvador/first call for independence",
      "el salvador/independence",
      "el salvador/san salvador festival",
      "el salvador/schoolteachers",
      "labor",
      "new year" }),

   "equatorial guinea":
   ({ "c/christmas",
      "c/easter",
      "c/good friday",
      "equatorial guinea/armed forces",
      "equatorial guinea/constitution",
      "equatorial guinea/independence",
      "equatorial guinea/oau",
      "equatorial guinea/presidents",
      "labor",
      "new year" }),

   "estonia":
 // "independence" ?
   ({ "c/christmas",
      "c/good friday",
      "estonia/midsummer",
      "estonia/victory",
      "labor",
      "new year" }),

   "ethiopia":
 // "birthday of mohammed" ?
 // "id al-adha" ?
 // "id al-fitr" ?
   ({ "c/good friday",
      "ethiopia/battle of aduwa",
      "ethiopia/buhe",
      "ethiopia/epiphany",
      "ethiopia/ethiopian new year",
      "ethiopia/genna",
      "ethiopia/liberation",
      "ethiopia/martyrs",
      "ethiopia/meskel",
      "ethiopia/patriots victory",
      "ethiopia/popular revolution commemoration",
      "ethiopia/revolution",
      "ethiopia/victory of adowa",
      "labor" }),

   "falkland islands":
 // "bank holiday" ?
   ({ "british commonwealth/queens",
      "c/christmas",
      "c/good friday",
      "falkland islands/anniversary of the battle of the falkland islands",
      "falkland islands/liberation",
      "new year" }),

   "fiji":
 // "diwali" ?
 // "fiji/deed of cession" ?
 // "fiji/fiji2" ?
 // "prophets" ?
   ({ "c/boxing",
      "c/christmas",
      "c/easter",
      "c/easter eve",
      "c/easter monday",
      "c/good friday",
      "fiji/bank holiday",
      "fiji/fiji",
      "fiji/prince charles",
      "fiji/queens",
      "new year" }),

   "finland":
 // "finland/helsinki" ?
   ({ "c/boxing",
      "c/christmas",
      "c/christmas eve",
      "c/easter monday",
      "c/good friday",
      "c/epiphany",
      "c/pentecost eve",
      "finland/alexis kivi",
      "finland/flag",
      "finland/flag of the army",
      "finland/independence",
      "finland/kalevala",
      "finland/midsummers eve",
      "finland/runebergs",
      "finland/snellman",
      "finland/all saints",
      "finland/midsummer",
      "may day",
      "new year" }),

   "france":
   ({ "c/all saints",
      "armistice",
      "c/all souls",
      "c/ash wednesday",
      "c/assumption",
      "c/christmas",
      "c/easter monday",
      "c/good friday",
      "c/pentecost",
      "france/ascension",
      "france/bastille",
      "france/d-day observance",
      "france/fathers",
      "france/fete des saintes-maries",
      "france/fete nationale",
      "france/liberation",
      "france/marseillaise",
      "france/night watch",
      "new year" }),

   "french guiana":
 // "bastille" ?
   ({ "c/all saints",
      "armistice",
      "c/all souls",
      "c/ascension",
      "c/ash wednesday",
      "c/assumption",
      "c/christmas",
      "c/easter monday",
      "c/good friday",
      "c/pentecost",
      "new year" }),

   "french polynesia":
   ({ "french polynesia/armistice",
      "french polynesia/chinese new year",
      "french polynesia/missionaries arrival commemoration",
      "may day" }),

   "gabon":
 // "id al-adha" ?
 // "id al-fitr" ?
 // "mouloud" ?
   ({ "c/all saints",
      "c/christmas",
      "c/easter monday",
      "c/whitmonday",
      "gabon/constitution",
      "gabon/independence",
      "gabon/renovation",
      "labor",
      "new year" }),

   "gambia":
 // "id al-fitr" ?
 // "id al-kabir" ?
 // "mouloud" ?
   ({ "c/assumption",
      "c/christmas",
      "gambia/independence",
      "gambia/moslem new year",
      "labor",
      "new year" }),

   "germany":
 // "christmas the following religious holidays" ?
 // "germany/oktoberfest" ?
 // "observed in certain l\344nder only epiphany" ?
   ({ "c/all saints",
      "c/ascension",
      "c/assumption",
      "c/christmas",
      "c/corpus christi",
      "c/easter monday",
      "c/good friday",
      "c/whitmonday",
      "c/epiphany",
      "germany/day of repentance",
      "germany/day of the workers",
      "germany/foundation",
      "germany/national",
      "germany/unity",
      "germany/waldchestag",
      "labor",
      "new year",
      "unity" }),

   "ghana":
 // "ghana/aboakyer" ?
   ({ "c/boxing",
      "c/easter eve",
      "c/easter monday",
      "c/good friday",
      "ghana/homowo",
      "ghana/independence",
      "ghana/liberation",
      "ghana/republic",
      "ghana/revolution",
      "ghana/third republic",
      "ghana/uprising",
      "labor",
      "new year" }),

   "gibraltar":
 // "gibraltar/late summer bank holiday" ?
 // "late summer bank holiday" ?
 // "queens official" ?
   ({ "c/boxing",
      "c/christmas",
      "c/easter monday",
      "c/good friday",
      "gibraltar/commonwealth",
      "gibraltar/spring bank holiday",
      "may day",
      "new year" }),

   "greece":
 // "greece/dumb week" ?
   ({ "orthodox/assumption",
      "orthodox/easter",
      "orthodox/easter monday",
      "orthodox/good friday",
      "orthodox/holy saturday",
      "orthodox/epiphany",
      "orthodox/christmas",
      "greece/clean monday",
      "greece/independence",
      "greece/flower festival",
      "greece/midwifes",
      "greece/ochi",       
      "greece/dodecanese accession", // rhodes
      "saints/demetrios",            // thessaloniki
      "greece/liberation of xanthi", // xanthi
      "saints/basil",      
      "labor",
      "new year" }),

   "gregorian calendar ":
   ({ "gregorian calendar /gregorian calendar" }),

   "grenada":
   ({ "c/boxing",
      "c/christmas",
      "c/corpus christi",
      "c/easter monday",
      "c/good friday",
      "c/whitmonday",
      "caribbean/emancipation",
      "grenada/independence",
      "grenada/national holiday",
      "grenada/thanksgiving",
      "labor",
      "new year" }),

   "guadeloupe":
 // "bastille" ?
 // "schoelcher" ?
   ({ "c/all saints",
      "armistice",
      "c/all souls",
      "c/ash wednesday",
      "c/assumption",
      "c/christmas",
      "c/easter monday",
      "c/good friday",
      "c/pentecost",
      "caribbean/schoelcher",
      "guadeloupe/ascension",
      "new year" }),

   "guam":
 // "george washingtons" ?
 // "memorial" ?
 // "thanksgiving" ?
 // "us independence" ?
   ({ "c/christmas",
      "guam/discovery",
      "guam/guam discovery",
      "guam/liberation",
      "labor",
      "new year" }),

   "guatemala":
   ({ "c/all saints",
      "c/assumption",
      "c/christmas",
      "c/holy friday",
      "c/holy saturday",
      "c/holy thursday",
      "guatemala/army",
      "guatemala/bank employees",
      "guatemala/independence",
      "guatemala/revolution",
      "labor",
      "new year" }),

   "guinea":
 // "id al-fitr" ?
 // "mouloud" ?
   ({ "c/all saints",
      "c/christmas",
      "c/easter",
      "guinea/anniversary of cmrn",
      "guinea/anniversary of womens revolt",
      "guinea/day of 1970 invasion",
      "guinea/independence",
      "guinea/referendum",
      "labor",
      "new year" }),

   "guinea-bissau":
 // "guinea-bissau/Korit\351(Id al-Fitr)" ?
 // "guinea-bissau/Tabaski (Id al-Adha)" ?
   ({ "c/christmas",
      "guinea-bissau/colonization martyrs",
      "guinea-bissau/independence",
      "guinea-bissau/mocidade",
      "guinea-bissau/national",
      "guinea-bissau/national heroes",
      "guinea-bissau/readjustment movement",
      "labor",
      "new year" }),

   "guyana":
 // "diwali" ?
 // "guyana/Carribean Day (June)" ?
 // "guyana/Indian Heritage Day (May)" ?
 // "guyana/Youman Nabi (Mouloud)" ?
 // "holi" ?
 // "id al-adha" ?
 // "id al-fitr" ?
   ({ "c/boxing",
      "c/christmas",
      "c/easter monday",
      "c/good friday",
      "guyana/freedom",
      "guyana/independence",
      "guyana/republic",
      "labor",
      "new year" }),

   "haiti":
   ({ "haiti/independence",
      "haiti/ancestry",
      "mardi gras",
      "mardi gras 2",
      "haiti/toussaint",
      "haiti/pan american",
      "c/good friday",
      "haiti/labor",
      "haiti/flag",
      "haiti/sovereignty",
      "c/corpus christi",
      "haiti/agwe",
      "haiti/papa ogou",
      "c/assumption",
      "haiti/dessalines",
      "un",
      "c/all saints",
      "c/all souls",
      "haiti/discovery",
      "haiti/vertieres",
      "c/christmas" }),

   "honduras":
   ({ "c/christmas",
      "c/good friday",
      "c/holy thursday",
      "columbus",
      "honduras/armed forces",
      "honduras/independence",
      "honduras/morazan",
      "honduras/thanksgiving",
      "labor",
      "new year",
      "honduras/pan american" }),

   "hong kong":
 // "hong kong/chung yeung festival" ?
   ({ "hong kong/birthday of confucious",
      "hong kong/birthday of pak tai",
      "hong kong/half-year",
      "hong kong/liberation" }),

   "iceland":
   ({ "iceland/independence" }),

   "india":
   ({ "india/indian independence",
      "india/mahatma gandhi jayanti",
      "india/mahatma gandhi martyrdom",
      "india/republic" }),

   "indonesia":
 // "id al-adha" ?
 // "id al-fitr" ?
 // "indonesia/balinese new year" ?
 // "islamic new year" ?
 // "leilat al-meiraj" ?
 // "mouloud" ?
   ({ "c/ascension",
      "c/christmas",
      "c/good friday",
      "indonesia/independence",
      "indonesia/kartini",
      "new year" }),

   "iran":
 // "ashoura" ?
 // "id al-adha" ?
 // "id al-fitr" ?
 // "leilat al-meiraj" ?
 // "mouloud" ?
   ({ "iran/constitution",
      "iran/islamic republic",
      "iran/martyrdom of imam ali",
      "iran/no ruz",
      "iran/oil nationalization",
      "iran/revolution",
      "iran/revolution2" }),

   "iraq":
 // "ashoura" ?
 // "id al-adha" ?
 // "id al-fitr" ?
 // "islamic new year" ?
 // "leilat al-meiraj" ?
 // "mouloud" ?
   ({ "iraq/14 ramadan revolution",
      "iraq/army",
      "iraq/july revolution",
      "iraq/republic",
      "new year" }),

   "ireland":
 // "august" ?
 // "bank holidays on mondays in june" ?
 // "october" ?
 // "saint patricks" ?
   ({ "c/christmas",
      "saints/stephen",
      "c/easter monday",
      "c/good friday",
      "ireland/sheelahs",
      "new year" }),

   "israel":
 // "holocaust memorial" ?
 // "independence" ?
 // "israel/hebrew university" ?
 // "israel/holocaust memorial" ?
 // "israel/independence" ?
 // "israel/national" ?
 // "lasts)" ?
 // "passover " ?
 // "rosh hashanah" ?
 // "shavuot" ?
 // "simhat torah" ?
 // "succot" ?
 // "yom kippur" ?
   ({ "israel/balfour declaration",
      "israel/jerusalem reunification" }),

   "italy":
 // "italy/saint marks" ?
   ({ "c/christmas",
      "c/epiphany",
      "italy/anniversary of the republic",
      "italy/befana",
      "italy/day of conciliation",
      "italy/festa del redentore",
      "italy/joust of the quintana",
      "italy/liberation",
      "italy/natale di roma",
      "italy/palio del golfo",
      "italy/santo stefano" }),

   "ivory coast":
 // "id al-adha" ?
 // "id al-fitr" ?
   ({ "c/all saints",
      "c/ascension",
      "c/assumption",
      "c/christmas",
      "c/easter monday",
      "c/good friday",
      "c/whitmonday",
      "ivory coast/independence",
      "ivory coast/national holiday of the ivory coast",
      "labor",
      "new year" }),

   "jamaica":
   ({ "c/ash wednesday",
      "c/christmas",
      "c/easter monday",
      "c/good friday",
      "jamaica/heroes",
      "jamaica/independence",
      "jamaica/labor",
      "labor",
      "new year" }),

   "japan":
 // "autumn equinox" ?
 // "health sports" ?
 // "japan/autumnal equinox" ?
 // "japan/black ship" ?
 // "japan/bon" ?
 // "japan/health" ?
 // "japan/memorial" ?
 // "japan/peoples holiday" ?
 // "japan/shichigosan" ?
 // "japan/vernal equinox" ?
 // "vernal equinox" ?
   ({ "japan/adults",
      "japan/bean-throwing festival",
      "japan/childrens",
      "japan/childrens protection",
      "japan/constitution",
      "japan/culture",
      "japan/emperors",
      "japan/empire",
      "japan/gion matsuri",
      "japan/greenery",
      "japan/hina matsuri",
      "japan/hiroshima peace festival",
      "japan/hollyhock festival",
      "japan/jidai matsuri",
      "japan/kakizome",
      "japan/kambutsue",
      "japan/kite battles of hamamatsu",
      "japan/labor thanksgiving",
      "japan/martyr",
      "japan/memorial to broken dolls",
      "japan/national foundation",
      "japan/respect for the aged",
      "japan/sanno matsuri",
      "japan/shigoto hajime",
      "japan/tanabata",
      "japan/tango-no-sekku",
      "new year" }),

   "jordan":
 // "id al-adha" ?
 // "id al-fitr" ?
 // "islamic new year" ?
 // "leilat al-meiraj" ?
 // "mouloud" ?
   ({ "arab league",
      "jordan/arbor",
      "jordan/coronation",
      "jordan/independence",
      "jordan/king hussein" }),

   "kazakhstan":
   ({ "new year 2d",
      "womens",
      "kazakhstan/nauryz meyrami",
      "kazakhstan/peoples unity",
      "kazakhstan/victory",
      "kazakhstan/national flag",
      "kazakhstan/constitution",
      "kazakhstan/republic",
      "kazakhstan/independence"}),

   "kenya":
 // "id al-fitr" ?
   ({ "c/boxing",
      "c/christmas",
      "c/easter monday",
      "c/good friday",
      "kenya/independence",
      "kenya/kenyatta",
      "kenya/madaraka",
      "labor",
      "new year" }),

   "kiribati":
   ({ "c/christmas",
      "c/easter",
      "kiribati/independence",
      "kiribati/youth",
      "new year" }),

   "kuwait":
 // "id al-adha" ?
 // "id al-fitr" ?
 // "islamic new year" ?
 // "leilat al-meiraj" ?
 // "mouloud" ?
 // "start of ramadan" ?
   ({ "kuwait/independence",
      "kuwait/national",
      "new year" }),

   "laos":
 // "chinese new year" ?
 // "laos/army" ?
   ({ "labor",
      "laos/constitution",
      "laos/independence",
      "laos/memorial",
      "laos/pathet lao",
      "laos/republic" }),

   "latvia":
 // "indpendence" ?
   ({ "c/christmas",
      "c/good friday",
      "latvia/independence",
      "latvia/midsummer festival",
      "new year",
      "new years eve" }),

   "lebanon":
 // "ashoura" ?
 // "chrismas" ?
 // "id al-adha" ?
 // "id al-fitr" ?
 // "islamic new year" ?
 // "leilat al-meiraj" ?
 // "mouloud" ?
 // "orthodox easter" ?
   ({ "c/all saints",
      "arab league",
      "c/ascension",
      "c/assumption",
      "c/easter",
      "lebanon/evacuation",
      "lebanon/independence",
      "lebanon/martyrs",
      "lebanon/saint maron",
      "new year" }),

   "lesotho":
   ({ "c/ascension",
      "c/boxing",
      "c/christmas",
      "c/easter monday",
      "c/good friday",
      "family",
      "lesotho/family",
      "lesotho/independence",
      "lesotho/kings",
      "lesotho/moshoeshoes",
      "lesotho/national holiday",
      "lesotho/national sports",
      "lesotho/national tree planting",
      "new year" }),

   "liberia":
 // "fast" ?
 // "integration" ?
 // "prayer" ?
 // "unification" ?
   ({ "c/christmas",
      "c/good friday",
      "liberia/armed forces",
      "liberia/decoration",
      "liberia/fast and prayer",
      "liberia/flag",
      "liberia/independence",
      "liberia/j j roberts",
      "liberia/literacy",
      "liberia/matilda newport",
      "liberia/memorial",
      "liberia/national redemption",
      "liberia/pioneers",
      "liberia/president tubmans",
      "liberia/thanksgiving",
      "liberia/unification and integration",
      "new year" }),

   "libya":
 // "ashoura" ?
 // "expulsion of fascist settlers" ?
 // "id al-adha" ?
 // "id al-fitr" ?
 // "islamic new year" ?
 // "leilat al-meiraj" ?
 // "libya/troop withdrawal" ?
 // "mouloud" ?
   ({ "libya/evacuation",
      "libya/expulsion of the fascist settlers",
      "libya/kings",
      "libya/national",
      "libya/revolution",
      "libya/sanusi army" }),

   "liechtenstein":
 // "saint josephs" ?
   ({ "c/all saints",
      "saints/stephen",
      "c/ascension",
      "c/assumption",
      "c/christmas",
      "c/corpus christi",
      "c/easter",
      "c/good friday",
      "c/immaculate conception",
      "c/nativity of mary",
      "c/whitmonday",
      "c/epiphany",
      "labor",
      "liechtenstein/bank holiday",
      "liechtenstein/birthday of prince franz-josef ii",
      "liechtenstein/national",
      "new year" }),

   "lithuania":
 // "national of hope" ?
   ({ "c/christmas",
      "lithuania/coronation",
      "lithuania/independence",
      "lithuania/mourning",
      "new year" }),

   "luxembourg":
   ({ "c/all saints",
      "c/boxing",
      "c/christmas",
      "c/easter monday",
      "luxembourg/burgsonndeg",
      "luxembourg/grand duchess",
      "luxembourg/liberation",
      "luxembourg/national",
      "new year 2d" }),

   "macao":
 // "chinese mid-autumn festival" ?
 // "chinese new year" ?
 // "ching ming" ?
 // "dragon boat festival" ?
 // "festival of ancestors" ?
 // "macao/hungry ghosts" ?
 // "national of china" ?
 // "portugal" ?
 // "portuguese republic" ?
 // "restoration of the independence" ?
 // "saint johns" ?
 // "winter solstice" ?
   ({ "c/all souls",
      "c/christmas",
      "c/good friday",
      "c/immaculate conception",
      "labor",
      "macao/anniversary of the portugese revolution",
      "macao/battle of july 13",
      "macao/procession of our lady of fatima",
      "macao/republic",
      "new year" }),

   "madagascar":
   ({ "c/all saints",
      "c/ascension",
      "c/christmas",
      "c/easter monday",
      "c/good friday",
      "c/whitsunday",
      "labor",
      "madagascar/commemoration",
      "madagascar/independence",
      "madagascar/republic",
      "new year" }),

   "malawi":
 // "but usually jul 5-7)" ?
   ({ "c/boxing",
      "c/christmas",
      "c/easter monday",
      "c/good friday",
      "malawi/august holiday",
      "malawi/kamuzu",
      "malawi/martyrs",
      "malawi/mothers",
      "malawi/national tree-planting",
      "malawi/republic",
      "mothers",
      "new year" }),

   "malaysia":
 // "chinese new year" ?
 // "diwali " ?
 // "each state has its own public holidays" ?
 // "malaysia/Hari Raya Haji (Id al-Adha)" ?
 // "malaysia/Hari Raya Puasa (Id al-Fitr)" ?
 // "malaysia/additional public holiday in kuala lumpur" ?
 // "mouloud in addition" ?
 // "sarawak)" ?
 // "vesak" ?
   ({ "c/christmas",
      "labor",
      "malaysia/kings",
      "malaysia/malaysia" }),

   "maldives":
 // "id al-adha" ?
 // "id al-fitr" ?
 // "islamic new year" ?
 // "mouloud" ?
   ({ "maldives/fisheries",
      "maldives/independence",
      "maldives/national",
      "maldives/republic",
      "maldives/victory" }),

   "mali":
 // "mali/Korit\351(Id al-Fitr)" ?
 // "mali/Tabaski (Id al-Adha)" ?
 // "mouloud" ?
   ({ "africa",
      "c/christmas",
      "c/easter monday",
      "labor",
      "mali/army",
      "mali/independence",
      "mali/liberation",
      "new year" }),

   "malta":
 // "malta/Feast of Our Lady of Victories (Nativity of Mary)" ?
 // "saint josephs" ?
 // "saint pauls shipwreck" ?
   ({ "saints/paul",
      "saints/peter",
      "c/assumption",
      "c/christmas",
      "c/good friday",
      "c/immaculate conception",
      "labor",
      "malta/freedom",
      "malta/independence",
      "malta/memorial of 1919 riot",
      "malta/republic",
      "new year" }),

   "marshall islands":
   ({ "marshall islands/proclamation of the republic of marshall islands" }),

   "martinique":
 // "bastille" ?
 // "lundi-gras" ?
 // "mardi-gras" ?
   ({ "c/all saints",
      "armistice",
      "c/all souls",
      "c/ash wednesday",
      "c/assumption",
      "c/christmas",
      "c/easter monday",
      "c/good friday",
      "c/pentecost",
      "martinique/ascension",
      "new year" }),

   "mauritania":
 // "islamic new year" ?
 // "leilat al-meiraj" ?
 // "mauritania/Korit\351(Id al-Fitr)" ?
 // "mauritania/Tabaski (Id al-Adha)" ?
 // "mouloud" ?
   ({ "africa",
      "labor",
      "mauritania/independence",
      "new year" }),

   "mauritius":
 // "diwali" ?
 // "ganesh chaturi" ?
 // "id al-fitr" ?
 // "maha shivaratree" ?
 // "mauritius/Chinese Spring Festival (Chinese New Year)" ?
 // "mauritius/Ougadi (Hindu)" ?
 // "thaipusam" ?
   ({ "c/all saints",
      "c/christmas",
      "c/good friday",
      "labor",
      "mauritius/independence",
      "new year 2d" }),

   "mexico":
 // "birthday of benito juarez easter" ?
 // "but not public holidays" ?
 // "christmas in addition" ?
 // "mexico/Day of the Dead (All Souls' Day)" ?
 // "widely celebrated" ?
   ({ "c/christmas",
      "labor",
      "mexico/birthday of benito juarez",
      "mexico/cinco de mayo",
      "mexico/constitution",
      "mexico/day of mourning",
      "mexico/dia de la raza",
      "mexico/holy cross",
      "mexico/independence",
      "mexico/night of the radishes",
      "mexico/our lady of guadalupe",
      "mexico/posadass",
      "mexico/presidential message",
      "mexico/revolution",
      "mexico/san marc\363s",
      "new year" }),

   "micronesia":
 // "micronesia/national holiday" ?
   ({  }),

   "monaco":
 // "monaco/F\352te du Travail (Labor Day)" ?
   ({ "c/all saints",
      "c/ascension",
      "c/assumption",
      "c/christmas",
      "c/easter monday",
      "c/immaculate conception",
      "c/whitmonday",
      "monaco/monaco national festival",
      "new year",
      "saints/devote" }),

   "mongolia":
 // "mongolia/Tsagaan Sar (lunar new year)" ?
   ({ "mongolia/national",
      "mongolia/republic",
      "new year",
      "womens" }),

   "montserrat":
 // "august monday" ?
 // "queens official" ?
 // "saint patricks" ?
   ({ "c/boxing",
      "c/christmas",
      "c/easter monday",
      "c/good friday",
      "c/whitmonday",
      "labor",
      "montserrat/festival",
      "montserrat/liberation",
      "new year" }),

   "mormonism":
   ({ "mormonism/founding of the mormon church" }),

   "morocco":
 // "ashoura" ?
 // "first of ramadan" ?
 // "islamic new year" ?
 // "morocco/Eid el Kebir (Id al-Adha)" ?
 // "morocco/Eid el Seghir (Id al-Fitr)" ?
 // "mouloud" ?
   ({ "labor",
      "morocco/green march",
      "morocco/independence",
      "morocco/oued ed-dahab",
      "morocco/throne",
      "new year" }),

   "mozambique":
   ({ "family",
      "mozambique/armed forces",
      "mozambique/heroes",
      "mozambique/independence",
      "mozambique/lusaka agreement",
      "mozambique/universal fraternity",
      "mozambique/womens",
      "mozambique/workers",
      "new year" }),

   "myanmar":
 // "burmese new year" ?
 // "diwali" ?
 // "full moon of kason" ?
 // "full moon of tabaung" ?
 // "full moon of thadingyut" ?
 // "full moon of waso" ?
 // "id al-adha" ?
 // "karen new year" ?
 // "myanmar/Water Festival (Maha Thingyan)" ?
 // "myanmar/burmese new year" ?
 // "tazaundaing festival" ?
   ({ "c/christmas",
      "labor",
      "myanmar/armed forces",
      "myanmar/independence",
      "myanmar/martyrs",
      "myanmar/national",
      "myanmar/peasants",
      "myanmar/resistance",
      "myanmar/union" }),

   "namibia":
 // "human rights" ?
   ({ "africa",
      "c/ascension",
      "c/christmas",
      "c/easter",
      "family",
      "namibia/casinga",
      "namibia/day of goodwill",
      "namibia/family",
      "namibia/heroes",
      "namibia/independence",
      "namibia/workers",
      "new year" }),

   "nauru":
   ({ "c/christmas",
      "c/easter",
      "nauru/angam",
      "nauru/independence",
      "new year" }),

   "nepal":
 // "diwali" ?
 // "holi" ?
 // "indra jatra" ?
 // "nepal/Baishakh Purnima (Buddha's birthday)" ?
 // "nepal/Dasain (Durga Puja)" ?
 // "nepal/Martyrs' Day (near Jan 30)" ?
 // "nepal/Navabarsha (Baisakhi)" ?
 // "nepal/buddha jayanti" ?
 // "ramnavami" ?
 // "shiva ratri" ?
 // "united nations" ?
   ({ "nepal/birthday of king birendra",
      "nepal/constitution",
      "nepal/democracy",
      "nepal/independence",
      "nepal/kings",
      "nepal/national unity",
      "nepal/queens",
      "nepal/tij",
      "womens" }),

   "netherlands":
   ({ "c/ascension",
      "c/christmas",
      "c/easter monday",
      "c/good friday",
      "c/whitmonday",
      "netherlands/beggars",
      "netherlands/independence",
      "netherlands/liberation",
      "netherlands/queens",
      "netherlands/sinterklaas",
      "new year" }),

   "netherlands antilles":
 // "christmas additional public holidays: lenten carnival in cura\347ao" ?
 // "whitmonday in st maarten" ?
   ({ "c/ascension",
      "c/easter monday",
      "c/good friday",
      "labor",
      "netherlands antilles/bonaire",
      "netherlands antilles/cura\347ao",
      "netherlands antilles/queens",
      "netherlands antilles/saba",
      "netherlands antilles/saint eustatius",
      "netherlands antilles/saint maarten",
      "new year" }),

   "new caledonia":
 // "bastille" ?
 // "liberation" ?
   ({ "armistice",
      "c/ascension",
      "c/christmas",
      "c/easter monday",
      "c/whitmonday",
      "labor",
      "new year",
      "new caledonia/bridge",
   }),

   "new zealand":
   ({ "anzac",
      "c/boxing",
      "c/christmas",
      "c/easter monday",
      "c/good friday",
      "labor",
      "new year 2d",
      "new zealand/labor",
      "new zealand/queens",
      "new zealand/waitangi" }),

   "nicaragua":
   ({ "c/all souls",
      "c/christmas",
      "c/easter",
      "c/holy friday",
      "c/holy thursday",
      "labor",
      "new year",
      "nicaragua/air force",
      "nicaragua/army",
      "nicaragua/fiesta",
      "nicaragua/independence",
      "nicaragua/revolution",
      "nicaragua/san jacinto" }),

   "niger":
 // "id al-adha" ?
 // "id al-fitr" ?
 // "islamic new year" ?
 // "mouloud" ?
   ({ "c/christmas",
      "c/easter monday",
      "labor",
      "new year",
      "niger/independence",
      "niger/national",
      "niger/republic" }),

   "nigeria":
 // "id al-fitr" ?
 // "mouloud" ?
 // "nigeria/Id al-Kabir (Id al-Adha)" ?
 // "nigeria/odum titun" ?
 // "nigeria/odun kekere" ?
   ({ "c/boxing",
      "c/christmas",
      "c/easter monday",
      "c/good friday",
      "new year",
      "nigeria/childrens",
      "nigeria/harvest festival",
      "nigeria/national" }),

   "northern mariana islands":
 // "memorial" ?
 // "thanksgiving" ?
 // "us independence" ?
   ({ "c/christmas",
      "labor",
      "new year",
      "northern mariana islands/commonwealth",
      "northern mariana islands/presidents" }),

   "north korea":
   ({"new year 2d",
//       "north korea/lunar new year",
     "north korea/kim jong-il",
     "womens",
     "north korea/kim il-sun",
     "north korea/armed forces",
     "may day",
     "north korea/chilsok",
     "north korea/liberation",
     "north korea/independence",
     "north korea/party foundation",
     "north kroea/constutution"}),
	
   "norway":
   ({ "c/ascension",
      "c/christmas",
      "c/easter monday",
      "c/good friday",
      "c/holy thursday",
      "c/whitmonday",
      "may day",
      "new year",
      "norway/constitution",
      "norway/olsok eve festival",
      "norway/tyvendedagen" }),

   "oman":
 // "ashoura" ?
 // "first of ramadan" ?
 // "id al-adha" ?
 // "id al-adha" ?
 // "id al-fitr" ?
 // "id al-fitr" ?
 // "islamic new year" ?
 // "islamic new year" ?
 // "leilat al-meiraj" ?
 // "mouloud" ?
 // "prophets" ?
   ({ "oman/national",
      "oman/national of oman",
      "oman/national2",
      "oman/sultans" }),

   "pakistan":
 // "ashoura" ?
 // "first of ramadan" ?
 // "id al-adha optional holidays for christians" ?
 // "id al-fitr" ?
 // "islamic new year" ?
 // "pakistan/Eid-i-Milad-un-Nabi (Mouloud)" ?
   ({ "c/boxing",
      "c/christmas",
      "c/easter monday",
      "c/good friday",
      "labor",
      "pakistan/birthday of quaid-i-azam",
      "pakistan/defense",
      "pakistan/independence",
      "pakistan/iqbal",
      "pakistan/jinnah",
      "pakistan/pakistan" }),

   "panama":
 // "christmas in panama city" ?
 // "panama/Carnival (Shrove Tuesday)" ?
   ({ "c/all souls",
      "c/good friday",
      "labor",
      "mothers",
      "new year",
      "panama/constitution",
      "panama/day of mourning",
      "panama/festival of the black christ",
      "panama/flag",
      "panama/foundation of panama city",
      "panama/independence",
      "panama/independence from spain",
      "panama/mothers",
      "panama/national anthem",
      "panama/revolution",
      "panama/uprising of los santos" }),

   "papua new guinea":
   ({ "british commonwealth/queens",
      "c/boxing",
      "c/christmas",
      "c/easter monday",
      "c/good friday",
      "new year",
      "papua new guinea/independence",
      "papua new guinea/remembrance" }),

   "paraguay":
   ({ "c/all saints",
      "c/ascension",
      "c/christmas",
      "c/corpus christi",
      "c/good friday",
      "c/holy thursday",
      "labor",
      "new year",
      "paraguay/battle of boquer\363n",
      "paraguay/constitution",
      "paraguay/day of the race",
      "paraguay/founding of the city of asunci\363n",
      "paraguay/heroes",
      "paraguay/independence",
      "paraguay/peace of chaco",
      "paraguay/virgin of caacupe",
      "saints/blaise" }),

   "peru":
 // "peasants" ?
   ({ "saints/paul",
      "saints/peter",
      "c/all saints",
      "c/christmas",
      "c/good friday",
      "c/holy thursday",
      "c/immaculate conception",
      "labor",
      "new year",
      "peru/combat of angamos",
      "peru/independence",
      "peru/inti raymi fiesta",
      "peru/santa rosa de lima" }),

   "philippines":
   ({ "c/all saints",
      "c/christmas",
      "c/good friday",
      "c/maundy thursday",
      "labor",
      "new year",
      "new years eve",
      "philippines/araw ng kagitingan",
      "philippines/barangay",
      "philippines/bataan",
      "philippines/bonifacio",
      "philippines/christ the king",
      "philippines/constitution",
      "philippines/freedom",
      "philippines/independence",
      "philippines/misa de gallo",
      "philippines/national heroes",
      "philippines/philippine-american friendship",
      "philippines/rizal",
      "philippines/thanksgiving" }),

   "portugal":
 // "porto observes the st john the baptist" ?
 // "portugal/Carnival (Shrove Tuesday)" ?
   ({ "c/all saints",
      "c/assumption",
      "c/corpus christi",
      "c/good friday",
      "c/immaculate conception",
      "labor",
      "new year",
      "portugal/cam\365es memorial",
      "portugal/christmas lisbon also observes the st anthony",
      "portugal/day of the dead",
      "portugal/liberty",
      "portugal/portugal",
      "portugal/republic",
      "portugal/restoration of the independence" }),

   "puerto rico":
 // "martin luther king" ?
 // "us independence" ?
 // "us thanksgiving" ?
 // "veterans" ?
 // "washington-lincoln" ?
   ({ "c/christmas",
      "c/good friday",
      "columbus",
      "c/epiphany",
      "labor",
      "new year",
      "puerto rico/barbosa",
      "puerto rico/birthday of eugenio maria de hostos",
      "puerto rico/constitution",
      "puerto rico/de diego",
      "puerto rico/discovery",
      "puerto rico/emancipation",
      "puerto rico/memorial",
      "puerto rico/mu\361oz rivera",
      "puerto rico/ponce de leon",
      "puerto rico/saint johns",
      "puerto rico/san juan" }),

   "qatar":
 // "first of ramadan" ?
 // "id al-adha" ?
 // "id al-fitr" ?
 // "islamic new year" ?
 // "leilat al-meiraj" ?
   ({ "qatar/anniversary of the amirs accession",
      "qatar/independence" }),

   "romania":
   ({ "c/christmas",
      "c/easter monday",
      "c/good friday",
      "labor",
      "new year 2d",
      "romania/liberation",
      "romania/national",
      "romania/public holiday" }),

   "rwanda":
 // "peace" ?
   ({ "c/all saints",
      "c/ascension",
      "c/assumption",
      "c/christmas",
      "c/easter monday",
      "c/whitmonday",
      "labor",
      "new year",
      "rwanda/armed forces",
      "rwanda/democracy",
      "rwanda/independence",
      "rwanda/kamarampaka",
      "rwanda/peace and unity",
      "unity" }),

   "saint kitts and nevis":
 // "august monday" ?
   ({ "british commonwealth/queens",
      "c/christmas",
      "c/easter monday",
      "c/good friday",
      "c/whitmonday",
      "labor",
      "saint kitts and nevis/carnival",
      "saint kitts and nevis/independence",
      "saint kitts and nevis/prince of wales" }),

   "saint lucia":
 // "carnival" ?
   ({ "british commonwealth/august bank holiday",
      "british commonwealth/queens",
      "c/boxing",
      "c/christmas",
      "c/corpus christi",
      "c/easter monday",
      "c/good friday",
      "c/whitmonday",
      "labor",
      "new year 2d",
      "saint lucia/discovery",
      "saint lucia/independence",
      "saint lucia/thanksgiving" }),

   "saint pierre and miquelon":
   ({ "new year 2d",
      "mardi gras" }),

   "saint vincent and the grenadines":
 // "carnival" ?
 // "grenadines" ?
 // "saint vincent" ?
   ({ "c/boxing",
      "c/christmas",
      "c/easter monday",
      "c/good friday",
      "c/whitmonday",
      "caribbean/caricom",
      "caribbean/emancipation",
      "labor",
      "new year",
      "saint vincent and the grenadines/independence",
      "saint vincent and the grenadines/saint vincent and the grenadines" }),

   "san marino":
   ({ "c/all saints",
      "saints/stephen",
      "c/all souls",
      "c/assumption",
      "c/christmas",
      "c/corpus christi",
      "c/easter monday",
      "c/immaculate conception",
      "c/epiphany",
      "labor",
      "new year",
      "san marino/anniversary of the arengo",
      "san marino/fall of fascism",
      "san marino/investiture of the new captains regent",
      "san marino/investiture of the new captains-regent",
      "san marino/liberation",
      "san marino/national",
      "san marino/san marino" }),

   "sao tome and principe":
 // "sao tomeand principe/Armed Forces Day (1st week in Sep)" ?
   ({ "family",
      "sao tome and principe/farmers",
      "sao tome and principe/independence",
      "sao tome and principe/martyrs",
      "sao tome and principe/transitional government" }),

   "saudi arabia":
 // "ashoura" ?
 // "id al-adha" ?
 // "id al-fitr" ?
 // "islamic new year" ?
 // "leilat al-meirag" ?
 // "mouloud" ?
   ({ "saudi arabia/national",
      "saudi arabia/national of saudi arabia" }),

   "senegal":
 // "mouloud" ?
 // "senegal/Korit\351 (Id al-Fitr)" ?
 // "senegal/Tabaski (Id al-Adha)" ?
   ({ "c/all saints",
      "c/ascension",
      "c/assumption",
      "c/christmas",
      "c/easter monday",
      "c/good friday",
      "c/whitmonday",
      "labor",
      "new year",
      "senegal/african community",
      "senegal/independence" }),

   "seychelles":
   ({ "c/all saints",
      "c/assumption",
      "c/christmas",
      "c/corpus christi",
      "c/easter",
      "c/immaculate conception",
      "labor",
      "new year 2d",
      "seychelles/independence",
      "seychelles/liberation" }),

   "sierra leone":
 // "id al-adha" ?
 // "id al-fitr" ?
 // "mouloud" ?
   ({ "c/boxing",
      "c/christmas",
      "c/easter monday",
      "c/good friday",
      "new year",
      "sierra leone/independence",
      "sierra leone/republic" }),

   "singapore":
 // "chinese new year" ?
 // "singapore/Deepavali (Diwali)" ?
 // "singapore/Hari Raya Haji (Id al-Adha)" ?
 // "singapore/Hari Raya Puasa (Id al-Fitr)" ?
 // "singapore/birthday of the monkey god" ?
 // "singapore/birthday of the saint of the poor" ?
 // "singapore/mooncake festival" ?
 // "singapore/vesak" ?
 // "vesak" ?
   ({ "c/christmas",
      "c/good friday",
      "new year",
      "singapore/independence",
      "singapore/labor",
      "singapore/national holiday" }),

   "slovakia":
   ({ "c/christmas eve",
      "c/christmas2d",
      "c/easter monday",
      "c/all saints",
      "c/good friday",
      "c/epiphany",
      "saints/cyril & methodius",
      "may day",
      "new year",
      "slovakia/independence",
      "slovakia/constitution",
      "slovakia/day of the slav apostles",
      "slovakia/liberation",
      "slovakia/reconciliation",
      "slovakia/slovak national uprising" }),

   "slovenia":
   ({ "new year 2d",
      "slovenia/culture",
      "c/easter",
      "c/easter monday",
      "slovenia/national resistance",
      "labor",
      "labor 2",
      "c/pentecost",
      "slovenia/national",
      "slovenia/peoples uprising",
      "c/assumption",
      "slovenia/reformation",
      "c/all saints",
      "slovenia/remembrance",
      "c/christmas",
      "slovenia/independence",}),

   "solomon islands":
   ({ "british commonwealth/queens",
      "c/boxing",
      "c/christmas",
      "c/easter monday",
      "c/good friday",
      "c/holy saturday",
      "c/whitmonday",
      "new year",
      "solomon islands/independence" }),

   "somalia":
 // "ashoura" ?
 // "id al-adha" ?
 // "id al-fitr" ?
 // "mouloud" ?
   ({ "labor",
      "new year",
      "somalia/foundation of the republic",
      "somalia/independence",
      "somalia/revolution" }),

   "south africa":
   ({ "c/ascension",
      "c/boxing",
      "c/christmas",
      "c/easter monday",
      "c/good friday",
      "new year",
      "south africa/day of the vow",
      "south africa/family",
      "south africa/kruger",
      "south africa/republic",
      "south africa/settlers",
      "south africa/van riebeeck",
      "south africa/workers" }),

   "south korea":
   ({ "new year 2d",
      "south korea/folklore",
      "chinese/new year",
      "south korea/taeborum",
      "south korea/independence",
      "south korea/labor",
      "south korea/arbor",
      "south korea/childrens",
      "south korea/buddha",
      "south korea/memorial",
      "south korea/constitution",
      "south korea/liberation",
      "south korea/republic",
      "south korea/thanksgiving",
      "south korea/armed forces",
      "south korea/foundation",
      "south korea/alphabet",
      "c/christmas" }),

   "soviet union":
   ({ "soviet union/anniversary of the october socialist revolution",
      "soviet union/victory" }),


   "spain":
 // "also observed" ?
 // "madrid only)" ?
 // "palma de mallorca)" ?
 // "palma de mallorca) local holidays" ?
 // "saint isidros " ?
 // "spain/tomatina" ?
   ({ "c/all saints",
      "c/assumption",
      "c/boxing",
      "c/christmas",
      "c/corpus christi",
      "c/easter monday",
      "c/good friday",
      "c/immaculate conception",
      "c/maundy thursday",
      "c/epiphany",
      "new year",
      "spain/dia de la toma",
      "spain/constitution",
      "spain/fiesta de san fermin",
      "spain/fiesta del arbol",
      "spain/grenada",
      "spain/hispanidad",
      "spain/king juan carlos saints",
      "spain/labor",
      "spain/national",
      "spain/national holiday of spain",
      "spain/queen isabella",
      "spain/saint james",
      "spain/saint joseph the workman" }),

   "sri lanka":
 // "there is a monthy full moon" ?
 // "diwali" ?
 // "holy prophets" ?
 // "sinhala" ?
 // "sri lanka/bandaranaike memorial" ?
 // "sri lanka/hadji festival" ?
 // "sri lanka/kandy perahera" ?
 // "sri lanka/thai pongal" ?
 // "sri lanka/vesak festival" ?
 // "tamil new year" ?
 // "tamil thai pongal" ?
   ({ "c/good friday",
      "c/christmas",	
      "may day",
      "new year",
      "sri lanka/independence",
      "sri lanka/national heroes",
      "sri lanka/republic",
      "sri lanka/sinhala and tamil new year" }),

   "sudan":
 // "id al-adha" ?
 // "id al-fitr" ?
 // "islamic new year" ?
 // "mououd" ?
 // "sudan/Sham an-Nassim (Coptic Easter Monday)" ?
   ({ "c/christmas",
      "new year",
      "sudan/decentralization",
      "sudan/independence",
      "sudan/national",
      "sudan/unity",
      "sudan/uprising" }),

   "suriname":
 // "id al-fitr" ?
 // "suriname/Phagwa (Holi)" ?
   ({ "c/boxing",
      "c/christmas",
      "c/easter monday",
      "c/good friday",
      "labor",
      "new year",
      "suriname/independence",
      "suriname/national union",
      "suriname/revolution" }),

   "swaziland":
 // "incwala" ?
 // "national flag" ?
 // "swaziland/incwala" ?
 // "united nations" ?
   ({ "c/ascension",
      "c/boxing",
      "c/christmas",
      "c/easter monday",
      "c/good friday",
      "new year",
      "swaziland/commonwealth",
      "swaziland/flag",
      "swaziland/kings",
      "swaziland/reed dance",
      "swaziland/somhlolo" }),

   "switzerland":
   ({ "c/ascension",
      "c/boxing",
      "c/christmas",
      "c/easter monday",
      "c/good friday",
      "c/whitmonday",
      "c/epiphany",
      "labor",
      "new year",
      "switzerland/berchtolds",
      "switzerland/glarus festival",
      "switzerland/homstrom",
      "switzerland/independence",
      "switzerland/may eve" }),

   "syria":
 // "egypts revolution" ?
 // "greek orthodox easter" ?
 // "id al-adha" ?
 // "id al-fitr" ?
 // "islamic new year" ?
 // "leilat al-meiraj" ?
 // "mouloud" ?
   ({ "c/christmas",
      "new year",
      "syria/beginning of october war",
      "syria/evacuation",
      "syria/national",
      "syria/revolution",
      "syria/union",
      "unity" }),

   "taiwan":
 // "chinese new year" ?
 // "ching ming festival" ?
 // "taiwan/birthday of matsu" ?
 // "taiwan/lantern festival" ?
   ({ "taiwan/birthday of confucious",
      "taiwan/buddha bathing festival",
      "taiwan/chiang kai-shek",
      "taiwan/childrens",
      "taiwan/constitution",
      "taiwan/dragon boat festival",
      "taiwan/founding of the republic of china",
      "taiwan/martyrs",
      "taiwan/national",
      "taiwan/restoration",
      "taiwan/sun yat-sen",
      "taiwan/youth" }),

   "tanzania":
 // "id al-fitr" ?
 // "mouloud" ?
 // "tanzania/Id al-Hajj (Id al-Adha)" ?
   ({ "c/christmas",
      "c/easter monday",
      "c/good friday",
      "labor",
      "tanzania/chama cha mapinduzi",
      "tanzania/heroes",
      "tanzania/independence",
      "tanzania/naming",
      "tanzania/saba saba",
      "tanzania/sultans",
      "tanzania/union",
      "tanzania/zanzibar revolution" }),

   "thailand":
 // "buddhist lent" ?
 // "makha bucha" ?
 // "thailand/loy krathong festival" ?
 // "thailand/makha bucha" ?
 // "thailand/state ploughing ceremony" ?
 // "thailand/visakha bucha" ?
 // "visakha bucha" ?
   ({ "new year",
      "new years eve",
      "thailand/asalapha bupha",
      "thailand/chakri",
      "thailand/chulalongkorn",
      "thailand/constitution",
      "thailand/coronation",
      "thailand/harvest festival",
      "thailand/kings",
      "thailand/queens",
      "thailand/songkran" }),

   "togo":
 // "id al-fitr" ?
 // "national liberation" ?
 // "togo/Tabaski (Id al-Adha)" ?
   ({ "c/all saints",
      "c/ascension",
      "c/assumption",
      "c/christmas",
      "c/easter monday",
      "c/whitmonday",
      "labor",
      "new year",
      "togo/anniversary of the failed attack on lome",
      "togo/economic liberation",
      "togo/independence",
      "togo/liberation",
      "togo/martyrs of pya",
      "togo/victory" }),

   "tonga":
   ({ "anzac",
      "c/boxing",
      "c/christmas",
      "c/easter monday",
      "c/good friday",
      "new year",
      "tonga/constitution",
      "tonga/emancipation",
      "tonga/king tupou",
      "tonga/kings",
      "tonga/princes" }),

   "trinidad and tobago":
 // "carnival" ?
 // "diwali" ?
   ({ "c/christmas",
      "c/corpus christi",
      "c/easter",
      "c/whitmonday",
      "new year",
      "trinidad and tobago/discovery",
      "trinidad and tobago/emancipation",
      "trinidad and tobago/independence",
      "trinidad and tobago/labor",
      "trinidad and tobago/republic" }),

   "tunisia":
 // "evacuaton" ?
 // "tunisia/Aid El Kebir (Id al-Adha)" ?
 // "tunisia/Aid El Seghir (Id al-Fitr)" ?
   ({ "labor",
      "new year",
      "tunisia/accession",
      "tunisia/bourguibas",
      "tunisia/evacuation",
      "tunisia/independence",
      "tunisia/independence recognition",
      "tunisia/martyrs",
      "tunisia/memorial",
      "tunisia/national holiday of tunisia",
      "tunisia/national revolution",
      "tunisia/republic",
      "tunisia/tree festival",
      "tunisia/womens",
      "tunisia/youth",
      "womens" }),

   "turkey":
 // "ataturk memorial" ?
 // "childrens" ?
 // "national sovereignty" ?
 // "turkey/Kurban Bayram (Id al-Adha)" ?
 // "turkey/Seker Bayram (Id al-Fitr)" ?
 // "youth & sports" ?
   ({ "new year",
      "turkey/ataturk commemoration",
      "turkey/youth and sports",
      "turkey/freedom and constitution",
      "turkey/hidrellez",
      "turkey/independence",
      "turkey/childrens",
      "turkey/national sovereignty",
      "turkey/navy and merchant marine",
      "turkey/rumis",
      "turkey/spring",
      "turkey/victory" }),

   "turks and caicos islands":
 // "commonwealth" ?
 // "national heroes" ?
 // "national youth" ?
 // "queens official" ?
   ({ "c/christmas",
      "c/easter monday",
      "c/good friday",
      "new year",
      "turks and caicos islands/columbus",
      "turks and caicos islands/emancipation",
      "turks and caicos islands/human rights",
      "turks and caicos islands/jags mccartney memorial" }),

   "tuvalu":
 // "commonwealth" ?
 // "national childrens" ?
   ({ "british commonwealth/prince of wales",
      "british commonwealth/queens",
      "c/boxing",
      "c/christmas",
      "c/easter",
      "new year",
      "tuvalu/tuvalu" }),

   "uganda":
 // "id al-adha" ?
 // "id al-fitr" ?
   ({ "c/christmas",
      "c/easter",
      "c/good friday",
      "labor",
      "new year",
      "uganda/heroes",
      "uganda/independence",
      "uganda/martyrs",
      "uganda/nrm/nra victorys",
      "uganda/republic" }),

   "ukraine":
   ({ "c/christmas",
      "labor",
      "new year",
      "orthodox/christmas",
      "ukraine/taras shevchenko",
      "ukraine/ukrainian",
      "ukraine/ukrainian independence",
      "ukraine/victory",
      "womens" }),

   "united arab emirates":
 // "id al-adha" ?
 // "id al-fitr" ?
 // "islamic new year" ?
 // "leilat al-meiraj" ?
 // "mouloud" ?
 // "ramadan 1" ?
   ({ "c/christmas",
      "new year",
      "united arab emirates/accession of the ruler of abu dhabi",
      "united arab emirates/national" }),

   "uruguay":
   ({ "c/all souls",
      "c/christmas",
      "columbus",
      "c/epiphany",
      "labor",
      "new year",
      "uruguay/artigas",
      "uruguay/battle of las piedras",
      "uruguay/blessing of the waters",
      "uruguay/constitution",
      "uruguay/independence",
      "uruguay/landing of the 33 patriots",
      "uruguay/childrens"}),

   "united kingdom":
   ({ "new year|h",
      "c/good friday|h",
      "c/easter monday|h",
      "c/christmas|h",
      "c/boxing|h",
      "may day|h",
      "united kingdom/spring bank holiday|h",
      "united kingdom/late summer bank holiday|h",
      "united kingdom/lammas",
      "united kingdom/queens",
      "united kingdom/burns", }),

   "united kingdom/england":
   ({ "+united kingdom",
      "united kingdom/guy fawkes",
      "united kingdom/oak apple",
      "united kingdom/battle of britain",
      "united kingdom/lord mayors",
      "united kingdom/mothering sunday",
      "united kingdom/woman peerage",
      "united kingdom/pancake tuesday" }),

   "united kingdom/scotland":
   ({ "+united kingdom",
      "united kingdom/bannockburn",
      "united kingdom/day after",
      "united kingdom/handsel monday",
      "united kingdom/highland games",
      "united kingdom/scottish new year|h",
      "united kingdom/victoria|h",
      "united kingdom/autumn holiday|h" }),
   
   "united kingdom/wales":
   ({ "+united kingdom",
      "saints/david|h" }),

   "united kingdom/northern ireland":
   ({ "+united kingdom",
      "united kingdom/orangeman|h",
      "saints/patrick" }),

   "us":
   ({ "new year|h",
      "c/christmas",
      "us/columbus|h",
      "mardi gras",
      "us/appomattox",
      "us/armed forces",
      "us/bill of rights",
      "us/carnation",
      "us/citizenship",
      "us/election",
      "us/flag|f",
      "us/forefathers",
      "us/inauguration",
      "us/independence|h",
      "us/iwo jima",
      "us/jefferson davis",
      "us/kosciuszko",
      "us/labor|h",
      "us/lincolns",
      "us/martin luther king",
      "us/memorial",
      "us/national freedom",
      "us/navy",
      "us/patriots",
      "us/robert e lee",
      "us/thanksgiving",
      "us/thomas jeffersons",
      "us/trivia",
      "us/twelfth night",
      "us/veterans|h",
      "us/vietnam",
      "us/washingtons|h",
      "us/womens equality" }),

   "us/alabama":
   ({ "+us",
      "us/alabama/alabama admission|h",
      "us/alabama/confederate memorial|h",
      "us/alabama/jefferson davis|h",
      "us/alabama/robert e lee|h",
      "columbus|h",
      "mardi gras|h",
      "us/thomas jeffersons|h" }),

   "us/alaska":
   ({ "+us",
      "us/alaska/alaska|h",
      "us/alaska/alaska admission|h",
      "-us/independence",
      "us/alaska/flag|fh",
      "us/alaska/sewards|h",
      "us/lincolns|h",
      "us/memorial|h" }),

   "us/arizona":
   ({ "+us",
      "us/arizona/american family|h",
      "us/arizona/arbors|h",
      "us/arizona/arborn|h",
      "us/arizona/arizona admission|h",
      "us/arizona/lincoln|h",
      "us/columbus|h",
      "fathers|h",
      "mothers|h",
      "us/memorial|h",
      "-us/washingtons",
      "us/washington|h" }),

   "us/arkansas":
   ({ "+us",
      "us/arkansas/arkansas admission|h",
      "us/arkansas/general douglas macarthur|h",
      "us/arkansas/world war ii memorial|h",
      "us/columbus|h",
      "us/election|h",
      "us/jefferson davis|h",
      "us/memorial|h",
      "us/robert e lee|h" }),

   "us/california":
   ({ "+us",
      "us/california/california admission|h",
      "us/columbus|h",
      "us/california/arbor|h",
      "us/election|h",
      "us/lincolns|h",
      "us/memorial|h", }),

   "us/colorado":
   ({ "+us",
      "us/colorado/arbor|h",
      "us/colorado/colorado|h",
      "columbus|h",
      "us/election|h",
      "us/lincolns|h",
      "us/memorial|h", }),

   "us/connecticut":
   ({ "+us",
      "c/good friday|h",
      "us/columbus|h",
      "us/connecticut/connecticut ratification|h",
      "us/lincolns|h",
      "us/martin luther king|h",
      "us/memorial|h", }),

   "us/delaware":
   ({ "+us",
      "c/good friday|h",
      "us/columbus|h",
      "us/delaware/arbor|h",
      "us/delaware/delaware|h",
      "us/delaware/lincolns|h",
      "us/delaware/memorial|h",
      "us/delaware/separation|h",
      "us/delaware/swedish colonial|h",
      "us/election|h" }),

   "us/florida":
   ({ "+us",
      "c/christmas|h",
      "c/good friday|h",
      "us/columbus|h",
      "us/florida/arbor|h",
      "us/florida/confederate memorial|h",
      "us/florida/farmers|h",
      "us/florida/florida admission|h",
      "us/florida/pascua florida|h",
      "us/florida/susan b anthony|h",
      "us/election|h",
      "us/jefferson davis|h",
      "us/lincolns|h",
      "us/martin luther king|h",
      "us/memorial|h",
      "us/robert e lee|h",}),

   "us/georgia":
 // "us/robert elee" ?
   ({ "+us",
      "us/columbus|h",
      "us/georgia/confederate memorial|h",
      "us/georgia/georgia|h",
      "us/jefferson davis|h",
      "us/memorial|h" }),

   "us/hawaii":
 // "us/hawaii/Discoverers' Day (Us/Columbus Day)" ?
   ({ "+us",
      "c/good friday|h",
      "us/hawaii/flag|fh",
      "us/hawaii/hawaii statehood|h",
      "us/hawaii/kamehameha|h",
      "us/hawaii/kuhio|h",
      "us/hawaii/lei|h",
      "us/hawaii/wesak flower festival|h",
      "us/election|h",
      "us/memorial|h",
      "us/presidents|h" }),

   "us/idaho":
   ({ "+us",
      "us/columbus|h",
      "us/idaho/idaho admission|h",
      "us/idaho/idaho pioneer|h",
      "us/election|h",
      "us/memorial|h" }),

   "us/illinois":
   ({ "+us",
      "us/columbus|h",
      "us/day after thanksgiving|h",
      "us/illinois/illinois admission|h",
      "us/illinois/memorial|h",
      "us/election|h",
      "us/lincolns|h",
      "us/martin luther king|h" }),

   "us/indiana":
   ({ "+us",
      "c/good friday|h",
      "us/columbus|h",
      "us/indiana/indiana admission|h",
      "us/indiana/primary election|h",
      "us/indiana/vincennes|h",
      "us/election|h",
      "us/lincolns|h",
      "us/memorial|h" }),

   "us/iowa":
   ({ "+us",
      "us/iowa/bird|h",
      "us/iowa/independence sunday|h",
      "us/iowa/iowa admission|h",
      "us/lincolns|h",
      "us/memorial|h" }),

   "us/kansas":
   ({ "+us",
      "us/columbus|h",
      "us/kansas/kansas|h",
      "us/lincolns|h",
      "us/memorial|h" }),

   "us/kentucky":
   ({ "+us",
      "us/columbus|h",
      "us/kentucky/franklin d roosevelt|h",
      "us/kentucky/kentucky statehood|h",
      "us/election|h",
      "us/jefferson davis|h",
      "us/lincolns|h",
      "us/martin luther king|h",
      "us/memorial|h",
      "us/robert e lee|h" }),

   "us/louisiana":
   ({ "+us",
      "c/all saints|h",
      "c/good friday|h",
      "us/columbus|h",
      "us/louisiana/huey p long|h",
      "us/louisiana/jackson|h",
      "us/louisiana/louisiana admission|h",
      "mardi gras|h",
      "new year|h",
      "us/election|h",
      "us/jefferson davis|h",
      "us/martin luther king|h",
      "us/memorial|h",
      "us/robert e lee|h" }),

   "us/maine":
   ({ "+us",
      "us/columbus|h",
      "us/maine/battleship|h",
      "us/maine/maine admission|h",
      "us/memorial|h",
      "us/patriots|h" }),

   "us/maryland":
   ({ "+us",
      "c/good friday|h",
      "us/columbus|h",
      "us/maryland/defenders|h",
      "us/maryland/john hanson|h",
      "us/maryland/maryland|h",
      "us/maryland/maryland admission|h",
      "us/maryland/maryland ratification|h",
      "us/maryland/memorial|h",
      "us/maryland/national anthem|h",
      "us/maryland/repudiation|h",
      "us/election|h",
      "us/lincolns|h",
      "us/martin luther king|h" }),

   "us/massachusetts":
   ({ "+us",
      "us/columbus|h",
      "us/massachusetts/bunker hill|h",
      "us/massachusetts/childrens|h",
      "us/massachusetts/evacuation|h",
      "us/massachusetts/john f kennedy|h",
      "us/massachusetts/lafayette|h",
      "us/massachusetts/liberty tree|h",
      "us/massachusetts/massachusetts ratification|h",
      "us/massachusetts/spanish-american war memorial|h",
      "us/massachusetts/student government|h",
      "us/massachusetts/susan b anthony|h",
      "us/massachusetts/teachers|h",
      "us/martin luther king|h",
      "us/memorial|h",
      "us/patriots|h" }),

   "us/michigan":
   ({ "+us",
      "us/michigan/memorial|h",
      "us/michigan/michigan|h",
      "us/martin luther king|h" }),

   "us/minnesota":
   ({ "+us",
      "us/columbus|h",
      "us/minnesota/american family|h",
      "us/minnesota/minnesota|h",
      "us/memorial|h" }),

   "us/mississippi":
   ({ "+us",
      "us/mississippi/confederate memorial|h",
      "us/mississippi/jefferson davis|h",
      "us/mississippi/robert elee|h" }),

   "us/missouri":
   ({ "+us",
      "us/columbus|h",
      "us/missouri/missouri admission|h",
      "us/missouri/truman|h",
      "us/election|h",
      "us/lincolns|h",
      "us/memorial|h" }),

   "us/montana":
   ({ "+us",
      "us/columbus|h",
      "us/montana/election|h",
      "us/lincolns|h",
      "us/memorial|h" }),

   "us/nebraska":
   ({ "+us",
      "us/columbus|h",
      "us/day after thanksgiving|h",
      "us/nebraska/arbor|h",
      "us/nebraska/nebraska state|h",
      "us/memorial|h",
      "us/presidents|h" }),

   "us/nevada":
   ({ "+us",
      "us/nevada/nevada|h",
      "us/memorial|h" }),

   "us/new hampshire":
   ({ "+us",
      "us/columbus|h",
      "us/day after thanksgiving|h",
      "us/new hampshire/fast|h",
      "us/new hampshire/memorial|h",
      "us/new hampshire/new hampshire admission|h",
      "us/election|h" }),

   "us/new jersey":
   ({ "+us",
      "c/good friday|h",
      "us/columbus|h",
      "us/new jersey/new jersey admission|h",
      "us/election|h",
      "us/lincolns|h",
      "us/martin luther king|h",
      "us/memorial|h" }),

   "us/new mexico":
   ({ "+us",
      "us/columbus|h",
      "us/new mexico/arbor|h",
      "us/new mexico/memorial|h",
      "us/lincolns|h" }),

   "us/new york":
   ({ "+us",
      "us/columbus|h",
      "us/new york/audubon|h",
      "us/new york/flag|fh",
      "us/new york/martin luther king|h",
      "us/new york/new york ratification|h",
      "us/new york/verrazano|h",
      "us/election|h",
      "us/lincolns|h",
      "us/memorial|h" }),

   "us/north carolina":
   ({ "+us",
      "c/easter monday|h",
      "us/north carolina/confederate memorial|h",
      "us/north carolina/halifax resolutions|h",
      "us/north carolina/mecklenburg|h",
      "us/memorial|h",
      "us/robert e lee|h" }),

   "us/north dakota":
   ({ "+us",
      "c/good friday|h",
      "us/north dakota/north dakota admission|h",
      "us/memorial|h" }),

   "us/ohio":
   ({ "+us",
      "us/columbus|h",
      "us/ohio/martin luther king|h",
      "us/ohio/ohio admission|h",
      "us/memorial|h",
      "-us/washingtons",
      "us/washington-lincoln|h" }),

   "us/oklahoma":
 // "us/oklahoma/oklahoma heritage week" ?
 // "us/oklahoma/youth" ?
   ({ "+us",
      "us/columbus|h",
      "mothers|h",
      "us/oklahoma/bird|h",
      "us/oklahoma/cherokee strip|h",
      "us/oklahoma/indian|h",
      "us/oklahoma/oklahoma|h",
      "us/oklahoma/oklahoma historical|h",
      "us/oklahoma/oklahoma statehood|h",
      "us/oklahoma/senior citizens|h",
      "us/oklahoma/will rogers|h",
      "us/election|h",
      "us/memorial|h",
      "us/thomas jeffersons|h" }),

   "us/oregon":
   ({ "+us",
      "us/oregon/lincolns|h",
      "us/oregon/oregon statehood|h",
      "us/memorial|h" }),

   "us/pennsylvania":
   ({ "+us",
      "c/good friday|h",
      "us/columbus|h",
      "us/pennsylvania/barry|h",
      "us/pennsylvania/charter|h",
      "us/pennsylvania/pennsylvania admission|h",
      "us/election|h",
      "us/flag|fh",
      "us/martin luther king|h",
      "us/memorial|h",
      "us/presidents|h" }),

   "us/rhode island":
   ({ "+us",
      "us/columbus|h",
      "us/rhode island/arbor|h",
      "us/rhode island/rhode island admission|h",
      "us/rhode island/rhode island independence|h",
      "us/rhode island/victory|h",
      "us/election|h",
      "us/memorial|h" }),

   "us/south carolina":
   ({ "+us",
      "us/south carolina/confederate memorial|h",
      "us/south carolina/south carolina admission|h",
      "us/election|h",
      "us/jefferson davis|h",
      "us/martin luther king|h",
      "us/robert e lee|h" }),

   "us/south dakota":
   ({ "+us",
      "us/south dakota/memorial|h",
      "us/south dakota/pioneers|h",
      "us/south dakota/south dakota admission|h",
      "us/presidents|h" }),

   "us/tennessee":
   ({ "+us",
      "c/good friday|h",
      "us/columbus|h",
      "us/tennessee/confederate memorial|h",
      "us/tennessee/tennesse statehood|h",
      "us/election|h",
      "us/memorial|h" }),

   "us/texas":
   ({ "+us",
      "us/columbus|h",
      "us/texas/alamo|h",
      "us/texas/austin|h",
      "us/texas/confederate heroes|h",
      "us/texas/juneteenth|h",
      "us/texas/lyndon b johnsons|h",
      "us/texas/san jacinto|h",
      "us/texas/texas admission|h",
      "us/texas/texas independence|h",
      "us/texas/texas pioneers|h",
      "us/election|h",
      "us/memorial|h" }),

   "us/utah":
   ({ "+us",
      "us/columbus|h",
      "us/lincolns|h",
      "us/memorial|h",
      "us/utah/arbor|h",
      "us/utah/pioneer|h",
      "us/utah/utah admission|h" }),

   "us/vermont":
   ({ "+us",
      "us/columbus|h",
      "us/lincolns|h",
      "us/vermont/bennington battle|h",
      "us/vermont/memorial|h",
      "us/vermont/town meeting|h",
      "us/vermont/vermont|h" }),

   "us/virginia":
 // "us/virginia/jack jouett" ?
   ({ "+us",
      "us/columbus|h",
      "us/election|h",
      "us/memorial|h",
      "us/virginia/cape henry|h",
      "us/virginia/confederate memorial|h",
      "us/virginia/crater|h",
      "us/virginia/jamestown|h",
      "us/virginia/lee-jackson|h",
      "us/virginia/royalist fast|h",
      "us/virginia/virginia ratification|h" }),

   "us/washington":
   ({ "+us",
      "us/lincolns|h",
      "us/memorial|h",
      "us/washington/washington admission|h" }),

// ?
   "us/washington dc":
   ({ "+us",
      "us/columbus|h",
      "us/election|h",
      "us/memorial|h",
      "us/washington dc/arbor|h" }),

   "us/west virginia":
   ({ "+us",
      "us/columbus|h",
      "us/election|h",
      "us/lincolns|h",
      "us/memorial|h",
      "-us/washingtons",
      "us/washington|h",
      "us/west virginia/west virginia|h" }),

   "us/wisconsin":
   ({ "+us",
      "c/good friday|h",
      "us/columbus|h",
      "us/election|h",
      "us/memorial|h",
      "-us/washingtons",
      "us/washington-lincoln|h",
      "us/wisconsin/primary election|h",
      "us/wisconsin/wisconsin|h" }),

   "us/wyoming":
 // "us/nellie tayloe ross" ?
   ({ "+us",
      "us/columbus|h",
      "us/election|h",
      "us/memorial|h",
      "-us/washingtons",
      "us/washington-lincoln|h",
      "us/wyoming/arbor|h",
      "us/wyoming/primary election|h",
      "us/wyoming/wyoming|h",
      "us/wyoming/wyoming statehood" }),

   "vanuatu":
   ({ "c/ascension",
      "c/assumption",
      "c/christmas",
      "c/easter",
      "labor",
      "new year",
      "unity",
      "vanuatu/constitution",
      "vanuatu/independence" }),

   "vatican city":
   ({ "saints/mary mother",
      "new year",
      "vatican city/world peace",
      "c/epiphany",
      "vatican city/lateranensi pacts",
      "saints/joseph",
      "c/holy thursday",
      "c/good friday",
      "c/easter",
      "saints/joseph the worker",
      "c/ascension",
      "c/pentecost",
      "c/corpus christi",
      "saints/peter",
      "saints/paul",
      "c/assumption",
      "vatican city/pope election",
      "c/all saints",
      "c/all souls",
      "c/immaculate conception",
      "c/christmas",
      "saints/stephen" }),

   "venezuela":
 // "carnival" ?
 // "additional bank holidays" ?
 // "saint josephs" ?
   ({ "saints/paul",
      "saints/peter",
      "c/all saints",
      "c/ascension",
      "c/assumption",
      "c/christmas",
      "c/easter",
      "c/immaculate conception",
      "columbus",
      "c/epiphany",
      "labor",
      "new year",
      "new years eve",
      "venezuela/battle of carabobo",
      "venezuela/bolivars",
      "venezuela/civil servants",
      "venezuela/declaration of independence",
      "venezuela/independence",
      "venezuela/teachers" }),

   "vietnam":
 // "tet nguyen dan" ?
 // "vietnam/tet nguyen dan" ?
 // "vietnam/thanh minh" ?
   ({ "labor",
      "new year",
      "vietnam/day of the nation",
      "vietnam/emperor-founder hung vuongs",
      "vietnam/founding of the communist party",
      "vietnam/independence",
      "vietnam/liberation of saigon" }),

   "virgin islands":
 // "martin luther king" ?
 // "memorial" ?
 // "presidents" ?
 // "three kings" ?
 // "us independence" ?
 // "us thanksgiving" ?
 // "veterans" ?
   ({ "c/christmas",
      "c/easter",
      "columbus",
      "labor",
      "new year",
      "virgin islands/danish west indies emancipation",
      "virgin islands/hurricane supplication",
      "virgin islands/hurricane thanksgiving",
      "virgin islands/liberty",
      "virgin islands/nicole robin",
      "virgin islands/organic act",
      "virgin islands/transfer" }),

   "western samoa":
 // "national womens" ?
   ({ "anzac",
      "c/boxing",
      "c/christmas",
      "c/easter",
      "c/whitmonday",
      "new year 2d",
      "western samoa/arbor",
      "western samoa/independence",
      "western samoa/independence2",
      "western samoa/white sunday" }),

   "yemen":
 // "ashoura" ?
 // "first of ramadan" ?
 // "id al-adha" ?
 // "id al-fitr" ?
 // "leilat al-meiraj" ?
 // "mouloud" ?
 // "muharram" ?
   ({ "labor",
      "new year",
      "womens",
      "yemen/corrective movement",
      "yemen/national" }),

   "yugoslavia":
   ({ "new year 2d",
      "orthodox/christmas",
      "orthodox/new year",
      "yugoslavia/constitution", // serbia
      "yugoslavia/national",
      "labor",
      "yugoslavia/victory",
      "yugoslavia/freedom fighters",
      "yugoslavia/freedom serbia", // serbia
      "yugoslavia/freedom montenegro", // montenegro
      "yugoslavia/republic" }),

   "zaire":
   ({ "c/christmas",
      "labor",
      "new year",
      "zaire/armed forces",
      "zaire/constitution",
      "zaire/day of the martyrs for independence",
      "zaire/independence",
      "zaire/mpr",
      "zaire/naming",
      "zaire/new regime",
      "zaire/parents",
      "zaire/presidents",
      "zaire/youth/presidents" }),

   "zambia":
   ({ "africa",
      "c/christmas",
      "c/easter",
      "labor",
      "new year",
      "unity",
      "zambia/farmers",
      "zambia/heroes",
      "zambia/independence",
      "zambia/unity",
      "zambia/youth",
      "zambia/youth2" }),

   "zimbabwe":
   ({ "africa",
      "c/christmas",
      "c/easter",
      "new year",
      "zimbabwe/heroess",
      "zimbabwe/independence",
      "zimbabwe/workers" }),
]);
