import Calendar;

// verified with calendars
string iso_days=#"
1993-12-31 Fri 1993w52
1994-01-01 Sat 1993w52
1994-12-31 Sat 1994w52
1995-01-01 Sun 1994w52
1995-12-31 Sun 1995w52
1996-01-01 Mon 1996w01
1996-12-31 Tue 1997w01
1997-01-01 Wed 1997w01
1997-12-31 Wed 1998w01
1998-01-01 Thu 1998w01
1998-12-31 Thu 1998w53
1999-01-01 Fri 1998w53
1999-12-31 Fri 1999w52
2000-01-01 Sat 1999w52
2000-12-31 Sun 2000w52
2001-01-01 Mon 2001w01"

// calculated
#"
2003-12-31 Wed 2004w01 
2004-01-01 Thu 2004w01 
";

int errs=0;

int main()
{
   foreach (iso_days/"\n"-({""});;string s)
   {
      sscanf(s,"%d-%d-%d %s %dw%d",
	     int y,int m,int d,string wd,int wy,int w);
      ISO.Day d1=ISO.Day(y,m,d);
      ISO.Day d2=ISO.dwim_day(y+"-"+m+"-"+d);
      ISO.Day d3=ISO.Week(wy,w)->day(wd);
      
      if (d1!=d2 ||
	  d2!=d3)
      {
	 werror("Day differ:\n"
		"   source: %O\n"
		"   Day(%d,%d,%d): %O\n"
		"   dwim_day(%O): %O\n"
		"   Week(%d,%d)->day(%O): %O\n",
		s,
		y,m,d,d1,
		y+"-"+m+"-"+d,d2,
		wy,w,wd,d3);
	 errs++;
	 continue;
      }
#define WDIFF(A,B)							\
      ((A)->week_no()!=(B)->week_no() ||				\
       (A)->week()!=(B)->week() ||					\
       (A)->week()->year()!=(B)->week()->year())
      if (WDIFF(d1,d2) || WDIFF(d2,d3) ||
	  d1->week_no()!=w ||
	  d1->week()->year_no()!=wy)
      {
	 werror("Week differ:\n"
		"   source: %O\n"
		"   Day(%d,%d,%d): %O: %O (%d,%d)\n"
		"   dwim_day(%O): %O: %O (%d,%d)\n"
		"   Week(%d,%d)->day(%O): %O: %O (%d,%d)\n",
		s,
		y,m,d,d1,d1->week(),d1->week_no(),d1->week()->year_no(),
		y+"-"+m+"-"+d,d2,
		   d2->week(),d2->week_no(),d2->week()->year_no(),
		wy,w,wd,d3,d3->week(),d3->week_no(),d3->week()->year_no()
		);
	 errs++;
      }
   }

   return !errs;
}
