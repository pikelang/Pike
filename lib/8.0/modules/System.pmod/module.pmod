#require constant(System.RegGetValue)
#pike 8.1

/*!   Get a single value from the register (COMPAT).
 *!
 *!   Pike 7.6 compatibility implementation of @[System.RegGetValue()].
 *!   The difference being that this function throws errors when
 *!   keys are missing.
 *!
 *! @note
 *!   This function is only available on Win32 systems.
 *!
 *! @seealso
 *!   @[RegGetKeyNames_76()], @[RegGetValues_76()], @[System.RegGetValue()]
 */
string|int|array(string) RegGetValue_76(int hkey, string key,
                                        string index)
{
  string|int|array(string) ret = System.RegGetValue(hkey, key, index);
  if(undefinedp(ret)) error("RegQueryValueEx: File not found.\n");
  return ret;
}

/*!   Get a list of value key names from the register (COMPAT).
 *!
 *!   Pike 7.6 compatibility implementation of @[System.RegGetKeyNames()].
 *!   The difference being that this function throws errors when
 *!   keys are missing.
 *!
 *! @note
 *!   This function is only available on Win32 systems.
 *!
 *! @seealso
 *!   @[System.RegGetValue()], @[RegGetValues_76()], @[System.RegGetKeyNames()]
 */
array(string) RegGetKeyNames_76(int hkey, string key)
{
  array(string) ret = System.RegGetKeyNames(hkey, key);
  if(undefinedp(ret)) error("RegGetKeyNames[RegOpenKeyEx]: File not found.\n");
  return ret;
}

/*!   Get multiple values from the register (COMPAT).
 *!
 *!   Pike 7.6 compatibility implementation of @[System.RegGetValues()].
 *!   The difference being that this function throws errors when
 *!   keys are missing.
 *!
 *! @note
 *!   This function is only available on Win32 systems.
 *!
 *! @seealso
 *!   @[RegGetValue_76()], @[RegGetKeyNames_76()], @[System.RegGetValues()]
 */
mapping(string:string|int|array(string)) RegGetValues_76(int hkey, string key)
{
  mapping(string:string|int|array(string)) ret = System.RegGetValues(hkey, key);
  if(undefinedp(ret)) error("RegOpenKeyEx: File not found.\n");
  return ret;
}
