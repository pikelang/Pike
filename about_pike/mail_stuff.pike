//And here is some stuff about mailing... 
//Maybe take a look in the manual :-)


void mailit(string file, string subject, array (string) list)
{
   foreach(list, string email)
      Process.popen("mail -n -s '"+subject+"' "+email+" < "+file);
}

//or...

void mailit(string file, string subject, array (string) list)
{
   foreach(list, string victim)
   {
      string command = "mail -n -s '"+subject+"' "+email+" < "+file;
      write(command+"\n");
      Process.popen(command);
   }
}


//or...

import Process;
void mailit(string file, string subject, array (string) list)
{
   string command;
   foreach(list, string victim)
   {
     write((command = "mail -n -s '"+subject+"' "+email+" < "+file)+"\n");
     write(popen(command)); // show the output of the command
   }
}
