/* Compatibility (We really need a better way to do this..) */

mapping tmp;
mixed `->(string s)
{
  if(s=="_module_value")
  {
    if(!tmp)
    {
      tmp=([]);
      tmp->sql=(program)"Sql";
    }
    return tmp;
  }
}
