#pike __REAL_VERSION__
#if constant(Regexp.PCRE1)
inherit Tools.Shoot.TagRemoveSscanf;

constant name="Tag removal u. Regexp.PCRE1";
int n=200;

Regexp.PCRE1 pcre=Regexp.PCRE1.Studied("<[^>]+>");

string tagremove(string line)
{
   return pcre->replace(line,"");
}

#endif /* constant(Regexp.PCRE1) */
