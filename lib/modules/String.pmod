#define BEGIN 32

constant count=__builtin.string_count;
constant width=__builtin.string_width;

/*
 * Implode an array of strings to an english 'list'
 * ie. ({"foo","bar","gazonk"}) beomces "foo, bar and gazonk"
 */
string implode_nicely(array(string|int|float) foo, string|void and)
{
  if(!and) and="and";
  foo=(array(string))foo;
  switch(sizeof(foo))
  {
  case 0: return "";
  case 1: return foo[0];
  default: return foo[0..sizeof(foo)-2]*", "+" "+and+" "+foo[-1];
  }
}

string capitalize(string s)
{
  return upper_case(s[0..0])+s[1..sizeof(s)];
}

string sillycaps(string s)
{
  return Array.map(s/" ",capitalize)*" ";
}

string strmult(string str, int num)
{
#if 1
  num*=strlen(str);
  while(strlen(str) < num) str+=str;
  return str[0..num-1];
#endif
#if 0
  return sprintf("%~n",str,strlen(str)*num);
#endif
}

/*
 * string common_prefix(array(string) strs)
 * {
 *   if(!sizeof(strs))
 *     return "";
 *  
 *   for(int n = 0; n < sizeof(strs[0]); n++)
 *     for(int i = 1; i < sizeof(strs); i++)
 * 	if(sizeof(strs[i]) <= n || strs[i][n] != strs[0][n])
 * 	  return strs[0][0..n-1];
 *
 *   return strs[0];
 * }
 *
 * This function is a slightly optimised version based on the code
 * above (which is far more suitable for an implementation in C).
 */
string common_prefix(array(string) strs)
{
  if(!sizeof(strs))
    return "";

  string strs0 = strs[0];
  int n, i;
  
  catch
  {
    for(n = 0; n < sizeof(strs0); n++)
      for(i = 1; i < sizeof(strs); i++)
	if(strs[i][n] != strs0[n])
	  return strs0[0..n-1];
  };

  return strs0[0..n-1];
}

class String_buffer {
  string *buffer=allocate(BEGIN);
  int ptr=0;
  
  static void fix()
    {
      string tmp=buffer*"";
      buffer=allocate(strlen(tmp)/128+BEGIN);
      buffer[0]=tmp;
      ptr=1;
    }
  
  string get_buffer()
    {
      if(ptr != 1) fix();
      return buffer[0];
    }
  
  void append(string s)
    {
      if(ptr==sizeof(buffer)) fix();
      buffer[ptr++]=s;
    }
  
  mixed cast(string to)
    {
      if(to=="string") return get_buffer();
      return 0;
    }
  
  void flush()
    {
      buffer=allocate(BEGIN);
      ptr=0;
    }
};


// Do a fuzzy matching between two different strings and return a
// "similarity index". The higher, the closer the strings match.

static int low_fuzzymatch(string str1, string str2)
{
  string tmp1, tmp2;
  int offset, length;
  int fuzz;
  fuzz = 0;
  while(strlen(str1) && strlen(str2))
  {
    /* Now we will look for the first character of tmp1 in tmp2 */
    if((offset = search(str2, str1[0..0])) != -1)
    {
      tmp2 = str2[offset..];
      /* Ok, so we have found one character, let's check how many more */
      tmp1 = str1;
      length = 1;
      while(1)
      {
        //*(++tmp1)==*(++tmp2) && *tmp1
        if(length < strlen(tmp1) && length < strlen(tmp2) &&
           tmp1[length] == tmp2[length])
          length++;
        else
          break;
      }
      if(length >= offset)
      {
        fuzz += length;
        str1 = str1[length..];
        str2 = str2[length + offset..];
        continue;
      }
    }
    if(strlen(str1))
      str1 = str1[1..];
  }
  return fuzz;
}

int fuzzymatch(string a, string b)
{
  int fuzz;

  if(a == b)
  {
    fuzz = 100;
  } else {
    fuzz = low_fuzzymatch(a, b);
    fuzz += low_fuzzymatch(b, a);
    fuzz = fuzz*100/(strlen(a)+strlen(b));
  }

  return fuzz;
}
