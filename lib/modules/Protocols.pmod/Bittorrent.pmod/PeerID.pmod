//
// Originally by nolar
// Translated to Pike and expanded by Mirar
//

#pike __REAL_VERSION__

/**
 * Decodes the given peerid, returning an identification string.
 */  

string identify_peer(string peerid)
{
   string shadow = peerid[..0];
   if (shadow=="S") 
   {
      if (peerid[8] == 45) 
      {
	 string version = peerid[1..3];
	 string name = "Shadow "
	    +version[..2]/1*".";
	 return name;
      }
        
      if (peerid[8] == 0) 
      {  // is next Burst version still using this?
	 string name = "Shadow "
	    +peerid[1..3]/1*".";
	 return name;
      }
   }
      
   string azureus = peerid[1..2];
   if (azureus=="AZ") 
   {
      string version = peerid[3..3+4-1];
      string name = "Azureus "
	 +version[..3]/1*".";
      return name;
   }
      
   string old_azureus = peerid[5..5+7-1];
   if (old_azureus=="Azureus") return "Azureus 2.0.3.2";
      
   string upnp = peerid[..0];
   if (upnp=="U") 
   {
      if (peerid[8] == 45) 
      {
	 string version = peerid[1..3];
	 string name = "UPnP "
	    +((array(string))version[..2])*".";
	 return name;
      }  
   }
      
   string bitcomet = peerid[..0+4-1];
   if (bitcomet=="exbc") 
   {
      string name = "BitComet "
	 +peerid[4]+"."
	 +peerid[5]/10
	 +peerid[5]%10;
      return name;
   }
      
   string turbobt = peerid[..0+7-1];
   if (turbobt=="turbobt") 
      return "TurboBT " + peerid[7..7+5-1];
  
   string libtorrent = peerid[1..2];
   if (libtorrent=="LT") 
   {
      string version = peerid[3..3+4-1];
      string name = "LibTorrent "
	 +version[..3]/1*".";
      return name;
   }
      
   string btfans = peerid[4..4+6-1];
   if (btfans=="btfans") return "SimpleBT";
      
   string xantorrent = peerid[..0+10-1];
   if (xantorrent=="DansClient") return "XanTorrent";

   if (peerid[..1]=="Pi")
   {
      int c;
      sscanf(peerid[4..5],"%2c",c);
      object d=Calendar.Day(c+2452991);
      return sprintf("Pike Protocols.Torrent %d.%d %4d/%02d/%02d",
		     peerid[2],peerid[3],
		     d->year_no(),
		     d->month_no(),
		     d->month_day());
   }
      
   int allZero = peerid[..11]=="\0"*12;
   
   if (allZero)
   {
      if ((peerid[12] == 97) && (peerid[13] == 97)) 
	 return "Experimental 3.2.1b2";

      if ((peerid[12] == 0) && (peerid[13] == 0)) 
	 return "Experimental 3.1";

      return "Generic";
   }

   werror("unknown client: %O\n",peerid);   

   return "unknown";
}
