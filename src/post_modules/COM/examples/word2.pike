int main(int argc, array(string) argv)
{
  object oWordApp;

  // Start the word application
  oWordApp = Roxen.COM.CreateObject("Word.Application");

  // Get the Name property from the object
  object name = oWordApp->Name;

  // Print out the name
  werror("name: %s\n", oWordApp->Name);
  
  // Get all constants defined in the applications type library
  mapping consts = Roxen.COM.GetConstants(oWordApp);

  // Set the visible property to make Word show up on screen
  oWordApp->Visible = 1;

  // Get the object in the Document property and call the Add function
  // in the Document object
  oWordApp->Documents->Add();

  // Create some text in the word document
  oWordApp->Selection->TypeText("one");
  oWordApp->Selection->TypeText("two");
  oWordApp->Selection->TypeParagraph();
  oWordApp->Selection->TypeText(name);
  oWordApp->Selection->TypeParagraph();
  oWordApp->Selection->TypeText(name*3);

  sleep(10);

  // Quit word without saving changes
  oWordApp->Quit(consts["wdDoNotSaveChanges"]);
}
