#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name="Tag removal using sscanf";
constant data=
   "abc<def>ghi<jkl>mno<prq>stu<vwx>yzå"
   "ABC<DEF>GHI<JKL>MNO<PRQ>STU<VWX>YZÅ"
   "abc1<def1>ghi1<jkl1>mno1<prq1>stu1<vwx1>yzå1"
   "ABC1<DEF1>GHI1<JKL1>MNO1<PRQ1>STU1<VWX1>YZÅ1"
   "abc2<def2>ghi2<jkl2>mno2<prq2>stu2<vwx2>yzå2"
   "ABC2<DEF2>GHI2<JKL2>MNO2<PRQ2>STU2<VWX2>YZÅ2"
   "abc3<def3>ghi3<jkl3>mno3<prq3>stu3<vwx3>yzå3"
   "ABC3<DEF3>GHI3<JKL3>MNO3<PRQ3>STU3<VWX3>YZÅ3"
   "abc4<def4>ghi4<jkl4>mno4<prq4>stu4<vwx4>yzå4"
   "ABC4<DEF4>GHI4<JKL4>MNO4<PRQ4>STU4<VWX4>YZÅ4"
   "abc5<def5>ghi5<jkl5>mno5<prq5>stu5<vwx5>yzå5"
   "ABC5<DEF5>GHI5<JKL5>MNO5<PRQ5>STU5<VWX5>YZÅ5"
   "abc6<def6>ghi6<jkl6>mno6<prq6>stu6<vwx6>yzå6"
   "ABC6<DEF6>GHI6<JKL6>MNO6<PRQ6>STU6<VWX6>YZÅ6"
   *100;
int n=20;

string tagremove(string line)
{
   string out="";
   string in=line;
   string tmp;
   while (sscanf(in,"%s<%*s>%s",tmp,in)) out+=tmp;
   return out;
}

void perform()
{
   string out;
   for (int i=0; i<n; i++)
      out=tagremove(data);
}

string present_n(int ntot,int nruns,float tseconds,float useconds,int memusage)
{
   return sprintf("%.0f /s",ntot/useconds);
}
