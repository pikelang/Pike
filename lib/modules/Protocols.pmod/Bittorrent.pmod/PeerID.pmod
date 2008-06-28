//
// Originally by nolar
// Translated to Pike and expanded by Mirar
//

#pike __REAL_VERSION__

//! Decodes the given @[peerid], returning an identification string
//! for the client software. Assumes the @[peerid] string is exactly
//! 20 characters long.
string identify_peer(string peerid)
{
  string id = azureus_style(peerid);
  if( id ) return id;

  id = shadow_style(peerid);
  if( id ) return id;

  string old_azureus = peerid[5..5+7-1];
  if (old_azureus=="Azureus") return "Azureus 2.0.3.2";

   string bitcomet = peerid[..0+4-1];
   if (bitcomet=="exbc") 
   {
      string name = "BitComet "
	 +peerid[4]+"."
	 +peerid[5]/10
	 +peerid[5]%10;
      return name;
   }

   if( has_prefix(peerid, "XBT") )
     return "XBT " + peerid[3..5]/1*"." + (peerid[6]=='d' ? " (debug)":"");

   if( has_prefix(peerid, "OP") )
     return "Opera " + peerid[2..5];

   if( sscanf(peerid, "-ML%s-", id) )
     return "MLdonkey "+id;

   if( has_prefix(peerid, "-BOW") )
     return "Bits on Wheels"; // Next three are version, A0C == 1.0.6

   id = bram_style(peerid);
   if( id ) return id;

   if( has_prefix(peerid, "AZ2500BT") )
     return "BitTyrant 1.1";

   if( peerid[0]==0 && peerid[2..3]=="BS" )
     return "BitSpirit " + peerid[1];

   if( peerid[2..3]=="RS" )
     return "Rufus " + peerid[..1];

   if( has_prefix(peerid, "-G3") )
     return "G3 Torrent";

   if( has_prefix(peerid, "FG") )
     return "FlashGet";

   if( has_prefix(peerid, "AP") && peerid[-1]=='-' )
     return "AllPeers";

   if ( has_prefix(peerid, "turbobt") )
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

   if( has_prefix(peerid, "BLZ") )
   {
     int version;
     sscanf(peerid, "BLZ%3d", version);
     return "Blizzard " + version;
   }

#ifdef TORRENT_DEBUG
   werror("unknown client: %O\n",peerid);   
#endif
   return "unknown";
}

protected string azureus_style(string peerid)
{
  if( peerid[0]!='-' || peerid[7]!='-' ) return 0;
  string id = peerid[1..2];
  int ver;
  if( sscanf(peerid[3..6], "%d%*1s", ver)!= 1 ) return 0;

  if( id=="AZ" )
    return "Azureus " + peerid[3..6]/1*".";

  id = ([
    "AG" : "Ares",
    "A~" : "Ares",
    "AR" : "Arctic",
    "AT" : "Artemis",
    "AX" : "BitPump",
    "AZ" : "Azureus",
    "BB" : "BitBuddy",
    "BC" : "BitComet",
    "BF" : "Bitflu",
    "BG" : "BTG",
    "BR" : "BitRocket",
    "BS" : "BTSlave",
    "BX" : "~Bittorrent X",
    "CD" : "Enhanced CTorrent",
    "CT" : "CTorrent",
    "DE" : "DelugeTorrent",
    "DP" : "Propagate Data Client",
    "EB" : "EBit",
    "ES" : "electric sheep",
    "FC" : "FileCroc",
    "FT" : "FoxTorrent",
    "GS" : "GSTorrent",
    "HL" : "Halite",
    "HN" : "Hydranode",
    "KG" : "KGet",
    "KT" : "KTorrent",
    "LH" : "LH-ABC",
    "LP" : "Lphant",
    "LT" : "libtorrent",
    "lt" : "libTorrent",
    "LW" : "LimeWire",
    "MO" : "MonoTorrent",
    "MP" : "MooPolice",
    "MR" : "Miro",
    "MT" : "MoonlightTorrent",
    "NE" : "BT Next Evolution",
    "NX" : "Net Transport",
    "OT" : "OmegaTorrent",
    "PD" : "Pando",
    "qB" : "qBittorrent",
    "QD" : "QQDownload",
    "QT" : "Qt 4 Torrent example",
    "RT" : "Retriever",
    "S~" : "Shareaza alpha/beta",
    "SB" : "~Swiftbit",
    "SS" : "SwarmScope",
    "ST" : "SymTorrent",
    "st" : "sharktorrent",
    "SZ" : "Shareaza",
    "TN" : "TorrentDotNET",
    "TR" : "Transmission",
    "TS" : "Torrentstorm",
    "TT" : "TuoTu",
    "UL" : "uLeecher!",
    "UT" : "µTorrent",
    "VG" : "Vagaa",
    "WT" : "BitLet",
    "WY" : "FireTorrent",
    "XL" : "Xunlei",
    "XT" : "XanTorrent",
    "XX" : "Xtorrent",
    "ZT" : "ZipTorrent",
  ])[id];
  if( !id ) return 0;

  return id + " " + ver;
}

protected string shadow_style(string peerid)
{
  if( peerid[4..6]!="---" ) return 0;
  string id = ([
    'A' : "ABC",
    'O' : "Osprey Permaseed",
    'Q' : "BTQueue",
    'R' : "Tribler",
    'S' : "Shadow",
    'T' : "BitTornado",
    'U' : "UPnP NAT Bit Torrent",
  ])[peerid[0]];
  if( !id ) return 0;

  return id + " " + (array(string))shadow_int( ((array)peerid[1..3])[*] )*".";
}

protected int shadow_int(int c)
{
  if( c>='0' && c<='9' ) return c-'0';
  if( c>='A' && c<='Z' ) return c-'A'+10;
  if( c>='a' && c<='z' ) return c-'a'+36;
  if( c=='.' ) return 62;
  if( c=='-' ) return 63;
  return -1;
}

protected string bram_style(string peerid)
{
  int a,b,c;
  if( sscanf(peerid, "%*1s%d-%d-%d-", a,b,c)!=4 ) return 0;

  string id = ([
    'M' : "Mainline",
    'Q' : "Queen Bee",
  ])[peerid[0]];
  if( !id ) return 0;

  return sprintf("%s %d.%d.%d", id, a,b,c);
}
