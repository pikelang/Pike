#pike __REAL_VERSION__
inherit Tools.Shoot.TagRemoveSscanf;

constant name="Tag removal u. search";
int n=200;

string tagremove(string line)
{
   String.Buffer out=String.Buffer();
   int pos=0,i;
   while ((i=search(line,"<",pos))!=-1)
   {
      out+=line[pos..i-1];
      i=search(line,">",i);
      if (i==-1) { pos=strlen(line); break; }
      pos=i+1;
   }
   out+=line[pos..];
   return (string)out;
}
