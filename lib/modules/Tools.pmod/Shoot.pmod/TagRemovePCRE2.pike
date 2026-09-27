#pike __REAL_VERSION__
#if constant(Regexp.PCRE2)
inherit Tools.Shoot.TagRemoveSscanf;

constant name="Tag removal u. Regexp.PCRE2";
int n=200;

Regexp.PCRE2 pcre=Regexp.PCRE2.Studied("<[^>]+>");

string tagremove(string line)
{
   return pcre->replace(line,"");
}

#endif /* constant(Regexp.PCRE2) */
