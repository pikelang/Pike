#pike __REAL_VERSION__
inherit Tools.Shoot.TagRemoveSscanf;

constant name="Tag removal u. array_sscanf";
int n=300;

void perform()
{
   string out;
   for (int i=0; i<n; i++)
      out=column(array_sscanf(data,"%{%s<%*s>%}")[0],0)*"";
}

string present_n(int ntot,int nruns,float tseconds,float useconds,int memusage)
{
   return sprintf("%.0f /s",ntot/useconds);
}
