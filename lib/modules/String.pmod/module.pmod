#pike __REAL_VERSION__
#pragma strict_types

constant Bootstring = __builtin.bootstring;
constant Buffer = __builtin.Buffer;
constant Iterator = __builtin.string_iterator;
constant Replace = __builtin.multi_string_replace;
constant SingleReplace = __builtin.single_string_replace;
constant SplitIterator = __builtin.string_split_iterator;

constant count = __builtin.string_count;
constant filter_non_unicode = string_filter_non_unicode;
constant hex2string = __builtin.hex2string;
constant int2char = predef::int2char;
constant int2hex = predef::int2hex;
constant normalize_space = __builtin.string_normalize_space;
constant range = __builtin.string_range;
constant secure = __builtin.string_secure;
constant status = __builtin.string_status;
constant trim = __builtin.string_trim;
/* deprecated */ constant trim_all_whites = __builtin.string_trim;
constant trim_whites = __builtin.string_trim_whites;
constant width = __builtin.string_width;

constant __HAVE_SPRINTF_STAR_MAPPING__ = 1;
constant __HAVE_SPRINTF_NEGATIVE_F__ = 1;

#if constant(predef::string2hex)
/* NB: This module is used from the precompiler, and may thus be used
 *     with older versions of Pike.
 */
constant string2hex = predef::string2hex;
#else
/* NB: Intentionally uses integer ranges instead of 7bit/8bit
 *     for improved backward compat.
 */
string(0..127) string2hex(string(0..255) s)
{
  return sprintf("%{%02x%}", (array(int(0..255)))s);
}
#endif

//! This function implodes a list of words to a readable string, e.g.
//! @expr{({"straw","berry","pie"})@} becomes
//! @expr{"straw, berry and pie"@}. If the separator is omitted, the
//! default is @expr{"and"@}. If the words are numbers they are
//! converted to strings first.
//!
//! @seealso
//! @[`*()]
//!
string implode_nicely(array(string|int|float) foo, string separator="and")
{
  array(string) bar = (array(string))foo;
  switch(sizeof(foo))
  {
  case 0: return "";
  case 1: return bar[0];
  default: return bar[..<1] * ", " + " " + [string]separator + " " + bar[-1];
  }
}

//! Convert the first character in @[str] to upper case, and return the
//! new string.
//!
//! @seealso
//! @[lower_case()], @[upper_case()]
//!
string capitalize(string str)
{
  return upper_case(str[0..0])+str[1..];
}

//! Convert the first character in each word (separated by spaces) in
//! @[str] to upper case, and return the new string.
//!
string sillycaps(string str)
{
  return Array.map(str/" ",capitalize)*" ";
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

//! Find the longest common prefix from an array of strings.
//!
string common_prefix(array(string) strs)
{
  if(!sizeof(strs))
    return "";

  strs = Array.uniq(sort(strs));

  if (sizeof(strs) == 1)
    return strs[0];

  string strs0 = strs[0];
  string strsn = strs[-1];

  int n;

  int sz = min(sizeof(strs0), sizeof(strsn));

  for(n = 0; n < sz; n++)
    if(strs0[n] != strsn[n])
      return strs0[0..n-1];

  // NB: strs0 MUST be the short string (aka prefix) in this case.
  return strs0;
}

// Do a fuzzy matching between two different strings and return a
// "similarity index". The higher, the closer the strings match.
protected int low_fuzzymatch(string str1, string str2)
{
  int fuzz;
  fuzz = 0;
  while(sizeof(str1) && sizeof(str2))
  {
    int offset, length;

    /* Now we will look for the first character of str1 in str2 */
    if((offset = search(str2, str1[0..0])) != -1)
    {
      string tmp2 = str2[offset..];
      /* Ok, so we have found one character, let's check how many more */
      string tmp1 = str1;
      length = 1;
      while(1)
      {
        //*(++tmp1)==*(++tmp2) && *tmp1
        if(length < sizeof(tmp1) && length < sizeof(tmp2) &&
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
    if(sizeof(str1))
      str1 = str1[1..];
  }
  return fuzz;
}

//! This function compares two strings using a fuzzy matching
//! routine. The higher the resulting value, the better the strings match.
//!
//! @seealso
//! @[Array.diff()], @[Array.diff_compare_table()]
//! @[Array.diff_longest_sequence()]
//!
int(0..100) fuzzymatch(string a, string b)
{
  int fuzz;

  if(a == b)
    return 100;

  fuzz = low_fuzzymatch(a, b);
  fuzz += low_fuzzymatch(b, a);
  fuzz = fuzz*100/[int(1..)](sizeof(a)+sizeof(b));

  return [int(0..100)]fuzz;
}

//! This function calculates the Levenshtein distance between two strings a
//! and b. The Levenshtein distance describes the minimal number of character
//! additions, removals or substitutions to apply to convert a to b.
//!
//! Mathematically, the Levenshtein distance between two strings a, b is given
//! by lev_a,b(|a|,|b|) where
//!
//! lev_a,b(i, j) == max(i, j), if min(i, j) == 0
//! lev_a,b(i, j) == min( lev_a,b(i, j-1)+1,
//!                       lev_a,b(i-1, j)+1,
//!                       lev_a,b(i-1, j-1) + a_i!=b_j ), else
//!
//! Note that the first element in the minimum corresponds to inserting a
//! character to a (or deleting a character from b), the second to deleting a
//! character from a and the third to match or mismatch, depending on whether
//! the respective characters are equal.
//!
//! Example: For example, the Levenshtein distance between "pike" and
//! "bikes" is 2, since the following two edits change one into the other,
//! and there is no way to do it with fewer than two edits:
//! - "pike" -> "bike" (substitute "p" with "b")
//! - "bike" -> "bikes" (add "s" at the end)
//!
//! Note that the cost to compute the Levenshtein distance is roughly
//! proportional to the product of the two string lengths. So this function
//! is usually used to aid in fuzzy string matching, when at least one of the
//! strings is short.
int levenshtein_distance(string a, string b)
{
    // The __builtin implementation takes care about a==b, a==0, b==0 and the
    // algorithm if both strings are 8bit wide.
    int res;
    if ((res = __builtin.levenshtein_distance(a, b)) != -1)
        return res;

    // Let ai be a sub-string of a with length i and bj be a sub-string of b
    // with length j. Then let lev_i the row
    //     lev_i = ( lev_ai,bj(|ai|,j) for j=0..|b| ).
    // Then we can calculate the next row
    //     lev_i+1 = ( lev_ai+1,bj(|ai|+1,j) for j=0..|b| )
    // from the above formula.

    // Initialize lev_i for the empty string a0, i.e., |a0| = 0.
    // You need |bj| = j edit operations (deleting each character from bj) to
    // get from bj to a0.
    // Thus lev_0[j] = j for j=0..|b|:
    int(0..2147483647) len = strlen(b);

    if( len > 2147483646 )
        error("Too large string.\n");

    ++len;
    array(int) lev_i = enumerate(len);
    for (int i = 0; i < strlen(a); i++)
    {
        // To calculate the next row (i+1): copy lev_i:
        array(int) lev_p = copy_value(lev_i);

        // First element of lev_i+1 is lev_ai+1,bj(|ai|+1,0):
        // the edit distance is deleting (i+1) chars from a to match empty b:
        lev_i[0] = i + 1;

        // Use the mathematical formula to fill in the rest of the row:
        for (int j = 0; j < strlen(b); j++)
        {
            int cost = (a[i] != b[j]);
            lev_i[j + 1] = min(lev_i[j] + 1,
                               lev_p[j + 1] + 1,
                               lev_p[j] + cost);
        }
    }

    // Now the Levenshtein distance is the last element in the last row:
    return lev_i[strlen(b)];
}

//! Returns the soundex value of @[word] according to
//! the original Soundex algorithm, patented by Margaret O´Dell
//! and Robert C. Russel in 1918. The method is based on the phonetic
//! classification of sounds by how they are made. It was only intended
//! for hashing of english surnames, and even at that it isn't that
//! much of a help.
string soundex(string word) {
  word = upper_case(word);
  string first = word[0..0];
  word = word[1..] - "A" - "E" - "H" - "I" - "O" - "U" - "W" - "Y";
  word = replace(word, ([ "B":"1", "F":"1", "P":"1", "V":"1",
			  "C":"2", "G":"2", "J":"2", "K":"2",
			  "Q":"2", "S":"2", "X":"2", "Z":"2",
			  "D":"3", "T":"3",
			  "L":"4",
			  "M":"5", "N":"5",
			  "R":"6" ]) );
  word = replace(word, ({"11", "22", "33", "44", "55", "66" }),
		 ({"1", "2", "3", "4", "5", "6", }));
  word+="000";
  return first + word[..2];
}

//! Converts the provided integer to a roman integer (i.e. a string).
//! @throws
//!   Throws an error if @[m] is outside the range 0 to 10000.
string int2roman(int m)
{
  string res="";
  if (m>10000||m<0)
    error("Can not represent values outside the range 0 to 10000.\n");
  while (m>999) { res+="M"; m-=1000; }
  if (m>899) { res+="CM"; m-=900; }
  else if (m>499) { res+="D"; m-=500; }
  else if (m>399) { res+="CD"; m-=400; }
  while (m>99) { res+="C"; m-=100; }
  if (m>89) { res+="XC"; m-=90; }
  else if (m>49) { res+="L"; m-=50; }
  else if (m>39) { res+="XL"; m-=40; }
  while (m>9) { res+="X"; m-=10; }
  if (m>8) return res+"IX";
  else if (m>4) { res+="V"; m-=5; }
  else if (m>3) return res+"IV";
  while (m) { res+="I"; m--; }
  return res;
}

protected constant prefix = ({ "bytes", "kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB" });

//! Returns the size as a memory size string with suffix,
//! e.g. 43210 is converted into "42.2 kB". To be correct
//! to the latest standards it should really read "42.2 KiB",
//! but we have chosen to keep the old notation for a while.
//! The function knows about the quantifiers kilo, mega, giga,
//! tera, peta, exa, zetta and yotta.
string int2size( int size )
{
  if(size<0)
    return "-" + int2size(-size);
  if(size==1) return "1 byte";
  int oldmask = -1;
  int mask = -1024;
  int(0..) no;
  int(0..) val = [int(0..)]size;
  while (size & mask) {
    if (++no == sizeof(prefix))
      return sprintf("%d %s", val, prefix[-1]);
    oldmask = mask;
    mask <<= 10;
    val >>= 10;
  }

  int decimal;
  if (!(decimal = (size & ~oldmask))) return sprintf("%d %s", val, prefix[no]);

  // Convert the decimal to base 10.
  // NB: no >= 1 since otherwise ~oldmask == ~-1 == 0 and we return above.
  decimal += 1<<[int(6..)](no*10 - 4);	// Rounding.
  decimal *= 5;
  decimal >>= [int(9..)](no*10 - 1);

  if (decimal == 10) {
    val++;
    decimal = 0;
  }

  return sprintf("%d.%d %s", val, decimal, prefix[no]);
}

//! Expands tabs in a string to ordinary spaces, according to common
//! tabulation rules.
string expand_tabs(string s, int(1..) tab_width = 8,
		   string tab = " ",
		   string space = " ",
		   string newline = "\n")
{
  return map(s/"\n", line_expand_tab, tab_width, space, tab) * newline;
}

// the \n splitting is done in our caller for speed improvement
protected string line_expand_tab(string line, int(1..) tab_width,
				 string space, string tab)
{
  string result = "";
  int(0..) col, next_tab_stop;
  while(sizeof(line))
  {
    sscanf(line, "%[ \t\n]%[^ \t\n]%s", string ws, string chunk, line);
    foreach(ws; ; int ch)
      switch(ch)
      {
	case '\t':
	  int(0..) num_spaces = [int(0..)](tab_width - (col % tab_width));
	  next_tab_stop = col + num_spaces;
	  result += tab * num_spaces;
	  col = next_tab_stop;
	  break;

	case ' ':
	  result += space;
	  col++;
	  break;
      }
    result += chunk;
    col += sizeof(chunk);
  }
  return result;
}

//! Gives the actual number of bits needed to represent every
//! character in the string. Unlike @[width] that only looks at the
//! allocated string width, @[bits] actually looks at the maximum used
//! character and delivers a more precise answer than just 8, 16, or
//! 32 bits. The empty string results in 0.
int(0..) bits(string data) {
  if(data=="") return 0;
  return Gmp.mpz(String.range(data)[1])->size();
}
