#pike __REAL_VERSION__
constant name="Tag removal u. Regexp.PCRE";

#if constant(Regexp.PCRE)
inherit Tools.Shoot.TagRemoveSscanf;

int n=2000;

Regexp.PCRE pcre=Regexp.PCRE.Studied("<[^>]>");

string tagremove(string line)
{
   return pcre->replace(line,"");
}

#else /* !constant(regexp.PCRE) */

void perform()
{
  exit(1);
}

#endif /* constant(Regexp.PCRE) */
