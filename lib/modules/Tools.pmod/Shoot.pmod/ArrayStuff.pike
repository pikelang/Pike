#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name="Array & String Juggling";

void perform()
{
   array ia=allocate(100,random);
   array is=((array(string))
	     ((ia*200)(10000)));
   string s=is*" ";
   int sum=0,x;
   ia=({});
   while (sscanf(s,"%d %s",x,s)==2) sum+=x,ia+=({x});
}
