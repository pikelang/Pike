inherit Calendar.ISO;

void create()
{
   month_names=
      ({"januari","februari","mars","april","maj","juni","juli","augusti",
	"september","oktober","november","december"});

   week_day_names=
      ({"måndag","tisdag","onsdag","torsdag",
	"fredag","lördag","söndag"});
}

class Week
{
   inherit Calendar.ISO.Week;

   string name()
   {
      return "v"+(string)this->number();
   }
}

class Year
{
   inherit Calendar.ISO.Year;

   array(array(string)) _namedays;
   mapping(string:int) _nameday_lookup;

   string name()
   {
      if (this->number()<=0) 
	 return (string)(1-this->number())+" fk";
      return (string)this->number();
   }

   array(array(string)) namedays()
   {
      if (_namedays) return _namedays;

      array(array(string)) a;

      if (!(a=namedays_cache[nameday_group(this)]))
      {
	 a=nameday_list(this);

	 if (this->leap())
	 {
	    a=a[..this->leap_day()-1]+
	       Array.map(allocate(this->leap()),
			 lambda(int x) { return ({}); })+
	       a[this->leap_day()..];
	 }

	 namedays_cache[nameday_group(this)]=a;
      }

      return _namedays=a;
   }

   object nameday(string name)
   {
      if (!_nameday_lookup
	 && !(_nameday_lookup=
	      namedays_lookup_cache[nameday_group(this)]))
      {
	 mapping m=([]);
	 int i;
	 foreach (this->namedays(),array a)
	 {
	    foreach (a,string name) m[lower_case(name)]=i;
	    i++;
	 }
	 _nameday_lookup =
	    namedays_lookup_cache[this->leap()+" "+this->leap_day()] = m;
      }

      return this->day(_nameday_lookup[lower_case(name)]);
   }
}

class Day
{
   inherit Calendar.ISO.Day;

   array(string) names()
   {
      return this->year()->namedays()[this->year_day()];
   }
}

// --- swedish namedays, data and functions ------------------------------

mapping namedays_cache=([]);
mapping namedays_lookup_cache=([]);

string nameday_group(object year)
{
   return (year->number()<1993)+":"+
          (year->leap())+":"+
          (year->leap_day());
}

array(array(string)) nameday_list(object year)
{
   if (year->number()<1993)
      return namedays_1986;
   else
      return namedays_1993;
}

/**

Name database from alma-1.0, 
http://www.lysator.liu.se/~tab/alma-1.0.tar.gz

Permission to use from Kent Engström, 1998-01-28

 **/


array(array(string)) namedays_1993= 
({ ({}), ({"Svea","Sverker"}), ({"Alfred","Alfrida"}),
   ({"Rut","Ritva"}), ({"Hanna","Hannele"}), ({"Baltsar","Kasper"}),
   ({"August","Augusta"}), ({"Erland","Erhard"}), ({"Gunnar","Gunder"}),
   ({"Sigurd","Sigmund"}), ({"Hugo","Hagar"}), ({"Frideborg","Fridolf"}),
   ({"Knut"}), ({"Felix","Felicia"}), ({"Laura","Liv"}),
   ({"Hjalmar","Hervor"}), ({"Anton","Tony"}), ({"Hilda","Hildur"}),
   ({"Henrik","Henry"}), ({"Fabian","Sebastian"}), ({"Agnes","Agneta"}),
   ({"Vincent","Veine"}), ({"Emilia","Emilie"}), ({"Erika","Eira"}),
   ({"Paul","Pål"}), ({"Bodil","Boel"}), ({"Göte","Göta"}),
   ({"Karl","Karla"}), ({"Valter","Vilma"}), ({"Gunhild","Gunilla"}),
   ({"Ivar","Joar"}), ({"Max","Magda"}), ({"Marja","Mia"}),
   ({"Disa","Hjördis"}), ({"Ansgar","Anselm"}), ({"Lisa","Elise"}),
   ({"Dorotea","Dora"}), ({"Rikard","Dick"}), ({"Berta","Berthold"}),
   ({"Fanny","Betty"}), ({"Egon","Egil"}), ({"Yngve","Ingolf"}),
   ({"Evelina","Evy"}), ({"Agne","Agnar"}), ({"Valentin","Tina"}),
   ({"Sigfrid","Sigbritt"}), ({"Julia","Jill"}),
   ({"Alexandra","Sandra"}), ({"Frida","Fritz"}), ({"Gabriella","Ella"}),
   ({"Rasmus","Ruben"}), ({"Hilding","Hulda"}), ({"Marina","Marlene"}),
   ({"Torsten","Torun"}), ({"Mattias","Mats"}), ({"Sigvard","Sivert"}),
   ({"Torgny","Torkel"}), ({"Lage","Laila"}), ({"Maria","Maja"}),
   ({"Albin","Inez"}), ({"Ernst","Erna"}), ({"Gunborg","Gunvor"}),
   ({"Adrian","Ada"}), ({"Tora","Tor"}), ({"Ebba","Ebbe"}),
   ({"Isidor","Doris"}), ({"Siv","Saga"}), ({"Torbjörn","Ambjörn"}),
   ({"Edla","Ethel"}), ({"Edvin","Elon"}), ({"Viktoria","Viktor"}),
   ({"Greger","Iris"}), ({"Matilda","Maud"}), ({"Kristofer","Christel"}),
   ({"Herbert","Gilbert"}), ({"Gertrud","Görel"}), ({"Edvard","Eddie"}),
   ({"Josef","Josefina"}), ({"Joakim","Kim"}), ({"Bengt","Benny"}),
   ({"Viking","Vilgot"}), ({"Gerda","Gert"}), ({"Gabriel","Rafael"}),
   ({"Mary","Marion"}), ({"Emanuel","Manne"}), ({"Ralf","Raymond"}),
   ({"Elma","Elmer"}), ({"Jonas","Jens"}), ({"Holger","Reidar"}),
   ({"Ester","Estrid"}), ({"Harald","Halvar"}), ({"Gunnel","Gun"}),
   ({"Ferdinand","Florence"}), ({"Irene","Irja"}), ({"Nanna","Nanny"}),
   ({"Vilhelm","Willy"}), ({"Irma","Mimmi"}), ({"Vanja","Ronja"}),
   ({"Otto","Ottilia"}), ({"Ingvar","Ingvor"}), ({"Ulf","Ylva"}),
   ({"Julius","Gillis"}), ({"Artur","Douglas"}), ({"Tiburtius","Tim"}),
   ({"Olivia","Oliver"}), ({"Patrik","Patricia"}), ({"Elias","Elis"}),
   ({"Valdemar","Volmar"}), ({"Olaus","Ola"}), ({"Amalia","Amelie"}),
   ({"Annika","Anneli"}), ({"Allan","Alida"}), ({"Georg","Göran"}),
   ({"Vega","Viveka"}), ({"Markus","Mark"}), ({"Teresia","Terese"}),
   ({"Engelbrekt","Enok"}), ({"Ture Tyko"}), ({"Kennet","Kent"}),
   ({"Mariana","Marianne"}), ({"Valborg","Maj"}), ({"Filip","Filippa"}),
   ({"John","Jack"}), ({"Monika","Mona"}), ({"Vivianne","Vivan"}),
   ({"Marit","Rita"}), ({"Lilian","Lilly"}), ({"Åke","Ove"}),
   ({"Jonatan","Gideon"}), ({"Elvira","Elvy"}), ({"Märta","Märit"}),
   ({"Charlotta","Lotta"}), ({"Linnea","Nina"}), ({"Lillemor","Lill"}),
   ({"Sofia","Sonja"}), ({"Hilma","Hilmer"}), ({"Nore","Nora"}),
   ({"Erik","Jerker"}), ({"Majken","Majvor"}), ({"Karolina","Lina"}),
   ({"Konstantin","Conny"}), ({"Henning","Hemming"}),
   ({"Desiree","Renee"}), ({"Ivan","Yvonne"}), ({"Urban","Ursula"}),
   ({"Vilhelmina","Helmy"}), ({"Blenda","Beda"}),
   ({"Ingeborg","Borghild"}), ({"Jean","Jeanette"}),
   ({"Fritiof","Frej"}), ({"Isabella","Isa"}), ({"Rune","Runa"}),
   ({"Rutger","Roger"}), ({"Ingemar","Gudmar"}),
   ({"Solveig","Solbritt"}), ({"Bo","Boris"}), ({"Gustav","Gösta"}),
   ({"Robert","Robin"}), ({"Eivor","Elaine"}), ({"Petra","Petronella"}),
   ({"Kerstin","Karsten"}), ({"Bertil","Berit"}), ({"Eskil","Esbjörn"}),
   ({"Aina","Eila"}), ({"Håkan","Heidi"}), ({"Margit","Mait"}),
   ({"Axel","Axelina"}), ({"Torborg","Torvald"}), ({"Björn","Bjarne"}),
   ({"Germund","Jerry"}), ({"Linda","Linn"}), ({"Alf","Alva"}),
   ({"Paulina","Paula"}), ({"Adolf","Adela"}), ({"Johan","Jan"}),
   ({"David","Salomon"}), ({"Gunni","Jim"}), ({"Selma","Herta"}),
   ({"Leo","Leopold"}), ({"Petrus","Peter"}), ({"Elof","Leif"}),
   ({"Aron","Mirjam"}), ({"Rosa","Rosita"}), ({"Aurora","Adina"}),
   ({"Ulrika","Ulla"}), ({"Melker","Agaton"}), ({"Ronald","Ronny"}),
   ({"Klas","Kaj"}), ({"Kjell","Tjelvar"}), ({"Jörgen","Örjan"}),
   ({"Anund","Gunda"}), ({"Eleonora","Ellinor"}), ({"Herman","Hermine"}),
   ({"Joel","Judit"}), ({"Folke","Odd"}), ({"Ragnhild","Ragnvald"}),
   ({"Reinhold","Reine"}), ({"Alexis","Alice"}), ({"Fredrik","Fred"}),
   ({"Sara","Sally"}), ({"Margareta","Greta"}), ({"Johanna","Jane"}),
   ({"Magdalena","Madeleine"}), ({"Emma","Emmy"}),
   ({"Kristina","Stina"}), ({"Jakob","James"}), ({"Jesper","Jessika"}),
   ({"Marta","Moa"}), ({"Botvid","Seved"}), ({"Olof","Olle"}),
   ({"Algot","Margot"}), ({"Elin","Elna"}), ({"Per","Pernilla"}),
   ({"Karin","Kajsa"}), ({"Tage","Tanja"}), ({"Arne","Arnold"}),
   ({"Ulrik","Alrik"}), ({"Sixten","Sölve"}), ({"Dennis","Donald"}),
   ({"Silvia","Sylvia"}), ({"Roland","Roine"}), ({"Lars","Lorentz"}),
   ({"Susanna","Sanna"}), ({"Klara","Clary"}), ({"Hillevi","Gullvi"}),
   ({"William","Bill"}), ({"Stella","Stefan"}), ({"Brynolf","Sigyn"}),
   ({"Verner","Veronika"}), ({"Helena","Lena"}), ({"Magnus","Måns"}),
   ({"Bernhard","Bernt"}), ({"Jon","Jonna"}), ({"Henrietta","Henny"}),
   ({"Signe","Signhild"}), ({"Bartolomeus","Bert"}),
   ({"Lovisa","Louise"}), ({"Östen","Ejvind"}), ({"Rolf","Rudolf"}),
   ({"Gurli","Gull"}), ({"Hans","Hampus"}), ({"Albert","Albertina"}),
   ({"Arvid","Vidar"}), ({"Samuel","Sam"}), ({"Justus","Justina"}),
   ({"Alfhild","Alfons"}), ({"Gisela","Glenn"}), ({"Harry","Harriet"}),
   ({"Sakarias","Esaias"}), ({"Regina","Roy"}), ({"Alma","Ally"}),
   ({"Anita","Anja"}), ({"Tord","Tove"}), ({"Dagny","Daniela"}),
   ({"Tyra","Åsa"}), ({"Sture","Styrbjörn"}), ({"Ida","Ellida"}),
   ({"Sigrid","Siri"}), ({"Dag","Daga"}), ({"Hildegard","Magnhild"}),
   ({"Alvar","Orvar"}), ({"Fredrika","Carita"}), ({"Agda","Agata"}),
   ({"Ellen","Elly"}), ({"Maurits","Morgan"}), ({"Tekla","Tea"}),
   ({"Gerhard","Gert"}), ({"Kåre","Tryggve"}), ({"Einar","Enar"}),
   ({"Dagmar","Rigmor"}), ({"Lennart","Leonard"}),
   ({"Mikael","Mikaela"}), ({"Helge","Helny"}), ({"Ragnar","Ragna"}),
   ({"Ludvig","Louis"}), ({"Evald","Osvald"}), ({"Frans","Frank"}),
   ({"Bror","Bruno"}), ({"Jenny","Jennifer"}), ({"Birgitta","Britta"}),
   ({"Nils","Nelly"}), ({"Ingrid","Inger"}), ({"Helmer","Hadar"}),
   ({"Erling","Jarl"}), ({"Valfrid","Ernfrid"}), ({"Birgit","Britt"}),
   ({"Manfred","Helfrid"}), ({"Hedvig","Hedda"}), ({"Fingal","Finn"}),
   ({"Antonia","Annette"}), ({"Lukas","Matteus"}), ({"Tore","Torleif"}),
   ({"Sibylla","Camilla"}), ({"Birger","Börje"}), ({"Marika","Marita"}),
   ({"Sören","Severin"}), ({"Evert","Eilert"}), ({"Inga","Ingvald"}),
   ({"Amanda","My"}), ({"Sabina","Ina"}), ({"Simon","Simone"}),
   ({"Viola","Vivi"}), ({"Elsa","Elsie"}), ({"Edit","Edgar"}),
   ({"Andre","Andrea"}), ({"Tobias","Toini"}), ({"Hubert","Diana"}),
   ({"Uno","Unn"}), ({"Eugen","Eugenia"}), ({"Gustav Adolf"}),
   ({"Ingegerd","Ingela"}), ({"Vendela","Vanda"}), ({"Teodor","Ted"}),
   ({"Martin","Martina"}), ({"Mårten"}), ({"Konrad","Kurt"}),
   ({"Kristian","Krister"}), ({"Emil","Mildred"}), ({"Katja","Nadja"}),
   ({"Edmund","Gudmund"}), ({"Naemi","Nancy"}), ({"Pierre","Percy"}),
   ({"Elisabet","Lisbeth"}), ({"Pontus","Pia"}), ({"Helga","Olga"}),
   ({"Cecilia","Cornelia"}), ({"Klemens","Clarence"}),
   ({"Gudrun","Runar"}), ({"Katarina","Carina"}), ({"Linus","Love"}),
   ({"Astrid","Asta"}), ({"Malte","Malkolm"}), ({"Sune","Synnöve"}),
   ({"Anders","Andreas"}), ({"Oskar","Ossian"}), ({"Beata","Beatrice"}),
   ({"Lydia","Carola"}), ({"Barbro","Barbara"}), ({"Sven","Svante"}),
   ({"Nikolaus","Niklas"}), ({"Angelika","Angela"}),
   ({"Virginia","Vera"}), ({"Anna","Annie"}), ({"Malin","Malena"}),
   ({"Daniel","Dan"}), ({"Alexander","Alex"}), ({"Lucia"}),
   ({"Sten","Stig"}), ({"Gottfrid","Gotthard"}), ({"Assar","Astor"}),
   ({"Inge","Ingemund"}), ({"Abraham","Efraim"}), ({"Isak","Rebecka"}),
   ({"Israel","Moses"}), ({"Tomas","Tom"}), ({"Natanael","Natalia"}),
   ({"Adam"}), ({"Eva"}), ({}), ({"Stefan","Staffan"}),
   ({"Johannes","Hannes"}), ({"Abel","Set"}), ({"Gunlög","Åslög"}),
   ({"Sylvester"}), });


array(array(string)) namedays_1986=
({ ({"Svea","Sverker"}), ({"Alfred","Alfrida"}), ({"Rut","Ritva"}),
   ({"Hanna","Hannele"}), ({"Baltsar","Kasper"}), ({"August","Augusta"}),
   ({"Erland","Erhard"}), ({"Gunnar","Gunder"}), ({"Sigurd","Sigmund"}),
   ({"Hugo","Hagar"}), ({"Frideborg","Fridolf"}), ({"Knut"}),
   ({"Felix","Felicia"}), ({"Laura","Liv"}), ({"Hjalmar","Hervor"}),
   ({"Anton","Tony"}), ({"Hilda","Hildur"}), ({"Henrik","Henry"}),
   ({"Fabian","Sebastian"}), ({"Agnes","Agneta"}), ({"Vincent","Veine"}),
   ({"Emilia","Emilie"}), ({"Erika","Eira"}), ({"Paul","Pål"}),
   ({"Bodil","Boel"}), ({"Göte","Göta"}), ({"Karl","Karla"}),
   ({"Valter","Vilma"}), ({"Gunhild","Gunilla"}), ({"Ivar","Joar"}),
   ({"Max","Magda"}), ({"Marja","Mia"}), ({"Disa","Hjördis"}),
   ({"Ansgar","Anselm"}), ({"Lisa","Elise"}), ({"Dorotea","Dora"}),
   ({"Rikard","Ricky","Rigmor"}), ({"Berta","Bert","Bertram"}),
   ({"Fanny","Sanny","Sonny"}), ({"Eugenia","Egon","Eira"}),
   ({"Yngve","Yvette","Yvonne"}), ({"Evelina","Elaine","Evelyn"}),
   ({"Agne","Alin","Alina"}), ({"Valentin","Valentina","Vally"}),
   ({"Sigfrid","Sigbert","Sigbritt"}), ({"Julia","Juliana","Juliette"}),
   ({"Alexandra","Sandor","Sandra"}), ({"Frida","Fride","Frode"}),
   ({"Gabriella","Ella","Elna"}), ({"Hulda","Haldis","Haldo"}),
   ({"Hilding","Hildeborg","Hildemar"}), ({"Martina","Tim","Tina"}),
   ({"Torsten","Toivo","Torun"}), ({"Mattias","Matti","Mats"}),
   ({"Sigvard","Sigvald","Sigvor"}), ({"Torgny","Torvald"}),
   ({"Lage","Laila","Lave"}), ({"Maria","Marie","Mary"}),
   ({"Albin","Alba","Alban"}), ({"Ernst","Erna","Ernfrid"}),
   ({"Gunborg","Gunbritt","Gunvald"}), ({"Adrian","Adrienne","Astor"}),
   ({"Tora","Toini","Tor"}), ({"Ebba","Ebon","Evonne"}),
   ({"Ottilia","Petra","Petronella"}), ({"Filippa","Gunlög","Åslög"}),
   ({"Torbjörn","Torben","Torgun"}), ({"Edla","Edling","Ethel"}),
   ({"Edvin","Diana","Edna"}), ({"Viktoria","Vibeke","Viking"}),
   ({"Greger","Grels","Greta"}), ({"Matilda","Maud","Moa"}),
   ({"Kristofer","Christel","Christer"}), ({"Herbert","Herta","Hervor"}),
   ({"Gertrud","Gertie","Gölin"}), ({"Edvard","Eda","Eddie"}),
   ({"Josef","James","Janet"}), ({"Joakim","Jockum","Kim"}),
   ({"Bengt","Bengta","Benita"}), ({"Viktor","Vimar","Våge"}),
   ({"Gerda","Anngerd","Gerd"}), ({"Gabriel","Gabrielle","Gunni"}),
   ({"Mary","Marion"}), ({"Emanuel","Emanuella","Immanuel"}),
   ({"Rudolf","Rode","Rudi"}), ({"Malkolm","Elma","Elmer"}),
   ({"Jonas","Jon","Jonna"}), ({"Holger","Olga"}),
   ({"Ester","Estrid","Vasti"}), ({"Harald","Hadar","Hardy"}),
   ({"Gudmund","Gudmar","Gunder"}), ({"Ferdinand","Gunvi","Gunvor"}),
   ({"Ambrosius","Irene","Irina"}), ({"Nanna","Nancy","Nanny"}),
   ({"Vilhelm","William","Willy"}), ({"Ingemund","Ingemo","Irma"}),
   ({"Hemming","Heimer","Helmut"}), ({"Otto","Orvar","Ottar"}),
   ({"Ingvar","Ingvald","Ingvor"}), ({"Ulf","Ylva","Yrsa"}),
   ({"Julius","Gillis"}), ({"Artur","Aldor","Atle"}),
   ({"Tiburtius","Ellen","Elly"}), ({"Olivia","Oliver","Ove"}),
   ({"Patrik","Patricia","Percy"}), ({"Elias","Elis","Elise"}),
   ({"Valdemar","Valdis","Volmar"}), ({"Olavus Petri","Olaus Petri"}),
   ({"Amalia","Amelie","Amy"}), ({"Anselm","Annevi","Annvor"}),
   ({"Albertina","Alida","Allan"}), ({"Georg","Georgina","Jörgen"}),
   ({"Vega","Viggo","Viveka"}), ({"Markus","Marika","Mark"}),
   ({"Teresia","Terese","Tessy"}), ({"Engelbrekt","Engelbert","Enok"}),
   ({"Ture","Turid","Tuve"}), ({"Tyko","Toralf","Torulf"}),
   ({"Mariana","Marianne","Marina"}), ({"Valborg","Maj","Maja"}),
   ({"Filip","Åsa","Åse"}), ({"Göta","Görel","Götmar"}),
   ({"Monika","Majne","Mona"}), ({"Gotthard","Gotthild","Gotty"}),
   ({"Sigmund","Sigge"}), ({"Gustava","Gullvi","Gullbritt"}),
   ({"Åke","Åge","Ågot"}), ({"Jonathan","John","Johnny"}),
   ({"Esbjörn","Elvy","Essy"}), ({"Märta","Meta","Märit"}),
   ({"Charlotta","Charlotte","Lotta"}), ({"Linnea","Linn","Lis"}),
   ({"Halvard","Hallvor","Halvar"}), ({"Sofia","Sia","Sofie"}),
   ({"Hilma","Helvi","Hilmer"}), ({"Rebecka","Renee","Rosita"}),
   ({"Erik","Erk","Jerker"}), ({"Alrik","Alda","Altea"}),
   ({"Karolina","Carola","Caroline"}),
   ({"Konstantin","Conny","Konstatia"}), ({"Henning","Henny","Pål"}),
   ({"Desideria","Dennis","Desiree"}), ({"Ragnvald","Ragnvi","Ragnvor"}),
   ({"Urban","Una","Uno"}), ({"Vilhelmina","Vilma","Vilmar"}),
   ({"Blenda","Beda","Britten"}), ({"Ingeborg","Ingabritt","Ingbritt"}),
   ({"Kristi Himmelsfärdsdag"}), ({"Baltsar","Bill","Billy"}),
   ({"Fritjof","Majny","Majvi"}), ({"Isabella","Iris","Isa"}),
   ({"Nikodemus","Nina","Ninni"}), ({"Rutger","Runa","Rune"}),
   ({"Ingemar","Ingar","Ingmarie"}), ({"Holmfrid","Helfrid","Helfrida"}),
   ({"Bo","Bodil","Boel"}), ({"Danmarks grundlagsdag"}),
   ({"Gustav","Gusten","Gösta"}), ({"Robert","Robin","Ruben"}),
   ({"Salomon","Sally"}), ({"Börje","Belinda","Björg"}),
   ({"Svante","Sante","Sjunne"}), ({"Bertil","Berit","Berthold"}),
   ({"Eskil","Eje","Evan"}), ({"Aina","Aino","Roine"}),
   ({"Håkan","Hakon","Hakvin"}), ({"Justina","Jim","Jimmy"}),
   ({"Axel","Axelia","Axelina"}), ({"Torborg","Torhild","Toril"}),
   ({"Björn","Bjarne","Björne"}), ({"Germund","Jerry","Jill"}),
   ({"Flora","Florence","Florentin"}), ({"Alf","Alvin","Alvina"}),
   ({"Paulina","Pamela","Paulette"}), ({"Adolf","Adolfina","Ally"}),
   ({"Johan","Jan"}), ({"David","Davida","Daisy"}),
   ({"Rakel","Rafael","Ralf"}), ({"Selma","Selim","Selmer"}),
   ({"Leo","Lola","Liselott"}), ({"Petrus","Peter","Petter"}),
   ({"Elof","Elvira","Viran"}), ({"Aron","Arent","Arild"}),
   ({"Rosa","Rose","Rosemarie"}), ({"Aurora","Andre","Aurelia"}),
   ({"Ulrika","Ellika","Ulla"}), ({"Melker","Marja","Mirjam"}),
   ({"Esaias","Elisiv","Esse"}), ({"Klas","Claudia","Klaus"}),
   ({"Kjell","Kajsa","Kettil"}), ({"Götilda","Göran","Jörn"}),
   ({"Anund","Anita","Ante"}), ({"Eleonora","Eleonor","Ellinor"}),
   ({"Herman","Hanne","Hermine"}), ({"Joel","Joar","Jorunn"}),
   ({"Folke","Fale","Fylgia"}), ({"Ragnhild","Ragni","Runo"}),
   ({"Reinhold","Reine","Reino"}), ({"Alexis","Alex","Alice"}),
   ({"Fredrik","Fred","Freddy"}), ({"Sara","Charles","Saga"}),
   ({"Margareta","Margit","Margret"}), ({"Johanna","Jean","Jeanette"}),
   ({"Magdalena","Magda","Madeleine"}), ({"Emma","Elena","Emmy"}),
   ({"Kristina","Kerstin","Kristin"}), ({"Jakob","Jack"}),
   ({"Jesper","Jessika","Jessie"}), ({"Marta","Marit","Marita"}),
   ({"Botvid","Reidar","Reidun"}), ({"Olof","Ola","Olle"}),
   ({"Algot","Margot","Vilgot"}), ({"Elin","Elon","Elina"}),
   ({"Per","Peder","Pernilla"}), ({"Karin","Karen","Kåre"}),
   ({"Tage","Tanja","Truls"}), ({"Arne","Arna","Arnevi"}),
   ({"Ulrik","Unn","Unni"}), ({"Sixten","Säve","Sölve"}),
   ({"Arnold","Annika","Annmari"}), ({"Sylvia","Silja","Silvia"}),
   ({"Roland","Ronald","Ronny"}), ({"Lars","Lasse","Lorentz"}),
   ({"Susanna","Sanna","Susanne"}), ({"Klara","Clarence","Clary"}),
   ({"Hillevi","Hilja","Irja"}), ({"Ebbe","Eberhard","Efraim"}),
   ({"Stella","Estelle","Stellan"}), ({"Brynolf","Benjamin","Benny"}),
   ({"Verner","Verna","Veronika"}), ({"Helena","Helen","Helny"}),
   ({"Magnus","Mogens","Måns"}), ({"Bernhard","Berna","Bernt"}),
   ({"Josefina","Josefin"}), ({"Henrietta","Harriet","Harry"}),
   ({"Signe","Signar","Signy"}), ({"Bartolomeus","Carita","Rita"}),
   ({"Lovisa","Louis","Louise"}), ({"Östen","Ejvind","Öjvind"}),
   ({"Rolf","Raoul","Rasmus"}), ({"Augustin","Gusti","Gurli"}),
   ({"Hans","Hampus","Hasse"}), ({"Albert","Albrekt","Aste"}),
   ({"Arvid","Arvida","Vidar"}), ({"Samuel","Sam","Solveig"}),
   ({"Justus","Jane","Judit"}), ({"Alfhild","Alfons","Arja"}),
   ({"Moses","Molly","My"}), ({"Adela","Adele","Adin"}),
   ({"Sakarias","Siv","Sivert"}), ({"Regina","Gilbert","Gisela"}),
   ({"Alma","Adils","Almar"}), ({"Augusta","Gunda","Gunde"}),
   ({"Tord","Tordis","Torgil"}), ({"Dagny","Dag","Daga"}),
   ({"Tyra","Tyr"}), ({"Ambjörn","Stig","Styrbjörn"}),
   ({"Ida","Idar","Vida"}), ({"Sigrid","Siri","Solbritt"}),
   ({"Eufemia","Cornelia","Cornelius"}),
   ({"Hildegard","Hilbert","Hildebrand"}), ({"Alvar","Alva","Alve"}),
   ({"Fredrika","Frej","Freja"}), ({"Agda","Jan","Jannika"}),
   ({"Matteus","Majbritt","Majlis"}), ({"Maurits","Marlene","Moritz"}),
   ({"Tekla","Trond","Tryggve"}), ({"Gerhard","Gert","Glenn"}),
   ({"Signhild","Sanfrid","Signhild"}), ({"Enar","Einar","Eja"}),
   ({"Dagmar","Donald","Douglas"}), ({"Lennart","Lena","Leonard"}),
   ({"Mikael","Majken","Mikaela"}), ({"Helge","Heidi","Härje"}),
   ({"r","Ragna","Ragne"}), ({"Ludvig","Levi","Liv"}),
   ({"Evald","Eila","Eilert"}), ({"Frans","Franciska","Frank"}),
   ({"Bror","Brage","Bruno"}), ({"Jenny","Jennifer","Jens"}),
   ({"Birgitta","Birgit","Britt"}), ({"Nils","Nilla","Nelly"}),
   ({"Ingrid","Inger","Ingolf"}), ({"Helmer","Helmina","Helmy"}),
   ({"Erling","Elvin","Elvina"}), ({"Valfrid","Ina","Inez"}),
   ({"Teofil","Terje","tjelvar"}), ({"Manfred","Mandor","Manne"}),
   ({"Hedvig","Hartvig","Hedda"}), ({"Fingal","Finn","Flemming"}),
   ({"Antonietta","Anette","Tony"}), ({"Lukas","Lillemor","Lilly"}),
   ({"Tore","Bojan Borghild"}), ({"Sibylla","Camilla","Kasper"}),
   ({"Birger","Brita","Britta"}), ({"Seved","Sigvid","Ursula"}),
   ({"Sören","Severin"}), ({"Evert","Eivor","Elving"}),
   ({"Inga","Ingalill","Ingert"}), ({"Amanda","Manda","Mandy"}),
   ({"Sabina","Sebastian","Sussy"}), ({"Simon","Simeon","Simone"}),
   ({"Viola","Vivi","Vivianne"}), ({"Elsa","Elsie","Ilse"}),
   ({"Edit","Edgar","Edor"}), ({"Tobias","Tova","Tove"}),
   ({"Hubert","Raymond","Roy"}), ({"Sverker","Nora","Nore"}),
   ({"Eugen","Ebert","Egil"}), ({"Gustav Adolf","Gull","Gulli"}),
   ({"Ingegerd","Ingel","Ingela"}), ({"Vendela","Vanda","Ville"}),
   ({"Teodor","Tea","Ted"}), ({"Martin Luther","Mait","Martin"}),
   ({"Mårten","Marion","Morgan"}), ({"Konrad","Kuno","Kurt"}),
   ({"Kristian","Karsten","Kersti"}), ({"Emil","Milly","Mimmi"}),
   ({"Leopold","Leif","Lilian"}), ({"Edmund","Elida","Elisa"}),
   ({"Napoleon","Naemi","Naima"}), ({"Magnhild","Magna","Magne"}),
   ({"Elisabet","Lisa","Lisbeth"}), ({"Pontus","Polly","Povel"}),
   ({"Helga","Helle","Hilde"}), ({"Cecilia","Cilla","Cissi"}),
   ({"Klemens","Ketty","Kitty"}), ({"Gudrun","Gullan","Gullvor"}),
   ({"Katarina","Carina","Katrin"}), ({"Torkel","Torleif"}),
   ({"Astrid","Asta","Astri"}), ({"Malte","Malvina","Mia"}),
   ({"Sune","Sonja","Synnöve"}), ({"Anders","Andrea","Andreas"}),
   ({"Oskar","Ole","Ossian"}), ({"Beata","Beatrice","Betty"}),
   ({"Lydia","Linda","Love"}), ({"Barbro","Barbara","Boris"}),
   ({"Sven","Svend","Svenning"}), ({"Nikolaus","Niklas","nikolina"}),
   ({"Agaton","Angela","Angelika"}), ({"Virginia","Vera","Vesta"}),
   ({"Anna","Ann","Annie"}), ({"Malin","Majvor","Malena"}),
   ({"Daniel","Dan","Daniela"}), ({"Alexander","Pia","Pierre"}),
   ({"Lucia","Lisen","Lisette"}), ({"Sten","Stina","Sture"}),
   ({"Gottfrid","Kaj","Kajsa"}), ({"Assar","Odd","Osvald"}),
   ({"Inge","Ilona","Irmeli"}), ({"Abraham","Abdon","Gideon"}),
   ({"Isak","Isidor","Isidora"}), ({"Israel","Gina","Gitte"}),
   ({"Tomas","Tom","Tommy"}), ({"Natanael","Natalia","Natan"}),
   ({"Adam","Ada","Adina"}), ({"Eva","Evita","Evy"}),
   ({"Stefan","Staffan","Stefanie"}), ({"Johannes","Hannes","Johan"}),
   ({"Abel","Abbe"}), ({"Set","Viva","Vivari"}),
   ({"Sylvester","Sylve","Sylvi"})});
