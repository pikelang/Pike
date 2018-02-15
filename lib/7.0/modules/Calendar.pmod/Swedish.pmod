// by Mirar 

#pike 7.0

inherit Calendar.ISO:ISO;

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
   inherit ISO::Week;

   string name()
   {
      return "v"+(string)this->number();
   }
}

class Year
{
   inherit ISO::Year;

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

      // insert test for year here
      if (!(a=namedays_cache[this->leap()+" "+this->leap_day()]))
      {
      // insert test for year here
	 a=namedays_1993;

	 if (this->leap())
	 {
	    a=a[..this->leap_day()-1]+
	       Array.map(allocate(this->leap()),
			 lambda(int x) { return ({}); })+
	       a[this->leap_day()..];
	 }

	 namedays_cache[this->leap()+" "+this->leap_day()]=a;
      }

      return _namedays=a;
   }

   object nameday(string name)
   {
      if (!_nameday_lookup
	 && !(_nameday_lookup=
	      namedays_lookup_cache[this->leap()+" "+this->leap_day()]))
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
      
      if (zero_type(_nameday_lookup[lower_case(name)])) return 0;
      return this->day(_nameday_lookup[lower_case(name)]);
   }
}

class Day
{
   inherit ISO::Day;

   array(string) names()
   {
      return this->year()->namedays()[this->year_day()];
   }
}

// --- namnsdagar, data -------------------------------------------------

mapping namedays_cache=([]);
mapping namedays_lookup_cache=([]);

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
   ({"William","Bill"}), ({"Stella","Stellan"}), ({"Brynolf","Sigyn"}),
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
