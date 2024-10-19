#charset iso-8859-1

#pike __REAL_VERSION__

// necessary translation (in order)
array(array(string|array(string))) elite_short =
({
  ({" you are"," your"}),
  ({" you'r"," your"}),
  ({" what the"," wt"}),
  ({" wt fuck"," wt\1001"}),
  ({" wt\1001?"," wt\1001!"}),
  ({" download"," d/l"}),
  ({" upload"," u/l"}),
  ({"picture","pic"}),
  ({" pornography"," pr0n"}),
  ({" porn"," pr0n"}),
  ({"cool","kewl"}),
  ({" elite "," 1337 "}),
  ({"qu","kw"}),
  ({" too "," 2 "}),
  ({" too."," 2."}),
  ({" too!"," 2!"}),

  ({"japanese",".jp"}),
  ({"japan",".jp"}),
  ({"swedish",".se"}),
  ({"sweden",".se"}),
  ({"chinese",".cn"}),
  ({"china",".cn"}),
  ({"france",".fr"}),
});

// optional recursive translation (in order)
array(array(string|array(string))) elite_trans =
({
  ({"you",({"u","j00"})}),
  ({" are "," r "}),
  ({" ok "," k "}),
  ({" any "," ne "}),
  ({"dude","dood"}),
  ({"newbie","noob"}),
  ({"fuck","\1001-"}),
  ({"girl","gurl"}),
  ({"choose","pick"}),

  ({"one","1"}),
  ({"two","2"}),
  ({"ate","8"}),
  ({"twi","2"}),
  ({" to"," 2"}),
  ({"ight","8t"}),
  ({"ite","eet"}),
  ({"four","4"}),
  ({"fore","4"}),
  ({"for","4"}),

  ({"ea","ee"}),
  ({"nn","n"}),
  ({"ff","\1002h"}),
  ({"f","\1002h"}),
  ({"cks",({"xor"})}),
  ({"cks",({"x","xx","xz"})}),
  ({"cs ",({"kz ","cz "})}),
  ({"ks",({"x","xz"})}),
  ({"cs",({"x","xz"})}),
  ({"ck ",({"xer "})}),
  ({"ck",({"k","cc"})}),
  ({"ers ",({"ors "})}),
  ({"er ",({"or "})}),
  ({"ed ",({"edz "})}),
  ({"me ",({"mez "})}),
  ({" el",({" l"})}),
  ({"s ",({"z "})}),
  ({"s!",({"z!"})}),
  ({"s.",({"z."})}),
  ({"s,",({"z,"})}),
  ({"s",({"z"})}),
  ({". ","! "}),
  ({"! ","!! "}),
  ({"? ",({"?? ","!? "})}),
});

// optional one-time character translation
mapping(string:array(string)) elite_char =
([
  "a":({"4","@"}),
  "b":({"8"}),
  "c":({"("}),
  "d":({"|)","|]","c|"}),
  "e":({"3"}),
  "f":({}),
  "g":({"6"}),
  "h":({"|-|"}),
  "i":({"1","|"}),
  "j":({}),
  "k":({"|<"}),
  "l":({"1","|_"}),
  "m":({"|V|","|\\/|"}),
  "n":({"|\\|"}),
  "o":({"0","()"}),
  "p":({"I=","p"}),
  "q":({}),
  "r":({"|~"}),
  "s":({"5"}),
  "t":({"+","7","-|-"}),
  "u":({"|_|","(_)"}),
  "v":({"\\/"}),
  "w":({"\\/\\/"}),
  "x":({"><"}),
  "y":({}),
  "z":({}),

  "0":({"()","O"}),
  "1":({"|","l"}),
  "2":({}),
  "3":({"E"}),
  "4":({}),
  "5":({"S"}),
  "6":({"G"}),
  "7":({}),
  "8":({}),
  "9":({}),
]);

// 8 bit variants
mapping(string:array(string)) elite_char8 =
([
  "!":"�!"/1,
  "?":"�"/1,
  "0":"�"/1,
  "1":"�"/1,
  "2":"�"/1,
  "3":"�"/1,
  "a":"������"/1,
  "b":"��"/1,
  "c":"碩"/1,
  "d":"�"/1,
  "e":"����"/1,
  "f":"�"/1,
  "i":"����"/1,
  "n":"�"/1,
  "o":"�������"/1,
  "p":"�"/1,
  "r":"�"/1,
  "s":"�"/1,
  "u":"�����"/1,
  "x":"�"/1,
  "y":"���"/1,
  "A":"�����Ū"/1,
  "B":"��"/1,
  "C":"Ǣ�"/1,
  "D":"�"/1,
  "E":"����"/1,
  "F":"�"/1,
  "I":"���� "/1,
  "N":"�"/1,
  "O":"�����غ"/1,
  "P":"�"/1,
  "R":"�"/1,
  "S":"�"/1,
  "U":"���ܵ"/1,
  "X":"�"/1,
  "Y":"���"/1,
]);

//! Translates one word to 1337. The optional
//! argument leetp is the maximum percentage of
//! leetness (100=max leet, 0=no leet).
//! elite_word only do character-based translation,
//! for instance from "k" to "|<", but no language
//! translation (no "cool" to "kewl").
string elite_word(string in, int(0..100) leetp=50, int(0..2) eightbit=0)
{
  if (!leetp)
    return replace(in,"\1001\1002\1003"/1,"fpl"/1);

  array v;
  switch (eightbit)
  {
  case 0:
    v=rows(elite_char,
	   lower_case(in)/1);
    break;
  case 2:
    v=rows(elite_char8,
	   lower_case(in)/1);
    break;
  case 1:
    v=map(in/1,
	  lambda(string s)
	    {
	      return
		( elite_char[s]||({}) ) |
		( elite_char8[s]||({}) );
	    });
    break;
  default:
    error("argument 3: illegal value (expected eightbit 0..2)\n");
  }


  multiset leet=(<>);
  multiset unleet=(<>);
  foreach (v;int i;array(string) d)
    if (!d || !sizeof(d)) unleet[i]=1;
    else leet[i]=1;

  // lower leet level to target leetness
  while (100*sizeof(leet)/sizeof(in)>leetp)
  {
    int z=((array)leet)[random(sizeof(leet))];
    leet[z]=0;
    unleet[z]=1;
  }

  string res="";
  foreach (v;int i;array(string) d)
    if (leet[i])
    {
      res+=d[random(sizeof(d))];
    }
    else
    {
      string s=in[i..i];
      if ( (<"\1001","\1002">)[s] )
	s=replace(s,"\1001\1002\1003"/1,"fpl"/1);

      if (leetp>50) // add random caps
	res+=(i&1?upper_case:lower_case)(s);
      else
	res+=s;
    }

  return res;
}

//! Translates a string to 1337. The optional
//! argument leetp is the maximum percentage of
//! leetness (100=max leet, 0=no leet).
//!
//! The translation is performed in three steps,
//! first the necessary elite translations (picture -> pic,
//! cool->kewl etc), then optional translations
//! (ok->k, dude->dood, -ers -> -orz), then
//! calls elite_word on the resulting words.
string elite_string(string in, int(0..100) leetp=50, int(0..1) eightbit=0)
{
  in=" "+in+" ";
  foreach (elite_short;;[string what,array(string)|string dest])
  {
    string res="";
    int i;
    while ((i=search(lower_case(in),what))!=-1)
    {
      if (arrayp(dest)) dest=dest[random(sizeof(dest))];
      res+=in[..i-1]+dest;
      in=in[i+sizeof(what)..];
    }
    in=res+in;
  }

  in=" "+in+" ";
  foreach (elite_trans;;[string what,array(string)|string dest])
  {
    string res="";
    int i;
    while ((i=search(lower_case(in),what))!=-1)
    {
      string r;
      if (dest && random(100)<leetp)
      {
	if (!arrayp(dest)) r=dest;
	else r=dest[random(sizeof(dest))];
      }
      else
	r=what;
      res+=in[..i-1]+r;
      in=in[i+sizeof(what)..];
    }
    in=res+in;
  }

  in=map(in/" "-({""}),elite_word,leetp,eightbit)*" ";

  return in;
}
