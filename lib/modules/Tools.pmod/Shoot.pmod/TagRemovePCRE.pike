#pike __REAL_VERSION__
inherit Tools.Shoot.TagRemoveSscanf;

constant name="Tag removal u. Regexp.PCRE";
int n=2000;

Regexp.PCRE pcre=Regexp.PCRE.Studied("<[^>]>");

string tagremove(string line)
{
   return pcre->replace(line,"");
}

