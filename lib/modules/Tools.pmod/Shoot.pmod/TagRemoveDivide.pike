#pike __REAL_VERSION__
inherit Tools.Shoot.TagRemoveSscanf;

constant name="Tag removal u. division";
int n=200;

string tagremove(string line)
{
   String.Buffer out=String.Buffer();
   foreach(line/"<";; string tmp) out+=(tmp/">")[-1];
   return (string)out;
}
