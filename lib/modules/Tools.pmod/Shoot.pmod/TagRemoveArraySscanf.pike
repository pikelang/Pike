#pike __REAL_VERSION__
inherit Tools.Shoot.TagRemoveSscanf;

constant name="Tag removal u. array_sscanf";

int n=300;

string tagremove(string line)
{
  return column(array_sscanf(data,"%{%s<%*s>%}%{%s%}")*({}),0)*"";
}
