#pike __REAL_VERSION__
inherit Tools.Shoot.TagRemoveSscanf;

constant name="Tag removal using a loop";
int n=50;

string tagremove(string line)
{
   string out="";
   string c;
   int stop=0;
   for(int i=0; i<sizeof(line);i++)
   {
      c = line[i..i];
      if(c == "<") stop =1;
      if(c == ">") stop =0;
      if(stop == 0) out += c;
   }
   return out;
}
