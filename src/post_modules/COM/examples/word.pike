int main(int argc, array(string) argv)
{
  object oWordApp;

  //oWordApp = Com.GetObject("c:\TEMPtest.doc", "Word.Document");
  oWordApp = Roxen.COM.CreateObject("Word.Application");

  werror(sprintf("name: %O\n", oWordApp->Name));
  werror(sprintf("name: %s\n", oWordApp->Name));
  
  object name = oWordApp->Name;
  mapping consts = Roxen.COM.GetConstants(oWordApp);

  oWordApp->Visible = 1;
  oWordApp->Documents->Add();
  oWordApp->Selection->TypeText("one");
  oWordApp->Selection->TypeText("two");
  oWordApp->Selection->TypeParagraph();
  oWordApp->Selection->TypeText(name);
  oWordApp->Selection->TypeParagraph();
  oWordApp->Selection->TypeText(name*3);
  sleep(10);
  oWordApp->Quit(consts["wdPromptToSaveChanges"]);
}
