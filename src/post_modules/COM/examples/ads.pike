constant ADSTYPE_CASE_IGNORE_STRING = 3;

int main(int argc, array(string) argv)
{
  object propList;

  //oWordApp = Com.GetObject("c:\TEMPtest.doc", "Word.Document");
  propList = COM.GetObject("LDAP://ldap.roxen.com/DC=roxen,DC=com");

  propList->GetInfo();
  int count = propList->PropertyCount->_value;
  werror("No of Property Found: %d\n", count);

//   object propEntry = propList->GetPropertyItem("dc",
//                                                ADSTYPE_CASE_IGNORE_STRING);
  
//   foreach (propEntry->Values->_value, object v)
//   {
//     werror("%O", v->CaseIgnoreString->_value);
//   }

  werror("Consts: %O\n", COM.GetConstants(propList));

  for (int i=0; i<count; i++)
  {
    object propEntry = propList->Item(i);
    werror("Consts: %O\n", COM.GetConstants(propEntry));
    werror("Name: %O, ADsType: %O\n", propEntry->Name->_value,
           propEntry->ADsType->_value);

    mixed a = propEntry->Values->_value;
    werror("a: %O\n", a);
    foreach (a || ({ }), object v)
    {
      werror("%d: %s\n", v->ADsType, v->CaseIgnoreString);
      werror("Consts: %O\n", COM.GetConstants(v));
    }
  }
}
