#pike __REAL_VERSION__

//! This module implements EXIF (Exchangeable image file format for Digital Still Cameras) 2.1
//! parsing.

// $Id: EXIF.pmod,v 1.11 2002/09/24 20:22:56 nilsson Exp $
//  Johan Schön <js@roxen.com>, July 2001.
//  Based on Exiftool by Robert F. Tobler <rft@cg.tuwien.ac.at>.
//
// Some URLs:
// http://www.ba.wakwak.com/~tsuruzoh/Computer/Digicams/exif-e.html
// http://www.pima.net/standards/it10/PIMA15740/exif.htm

void add_field(mapping m, string field, mapping alts, mixed index)
{
  if(alts[index])
    m[field]=alts[index];
}

mapping canon_d30_multi0(array(int) data)
{
  mapping res=([]);
  add_field(res, "CanonMacroMode",
	    ([1: "Macro", 2: "Normal"]),
	    data[1]);
  
  if(data[2])
    res->CanonSelfTimerLength = sprintf("%.1f s", data[2]/10.0);

  add_field(res, "CanonQuality",
	    ([2: "Normal", 3: "Fine", 4: "Superfine"]),
	    data[3]);

  add_field(res, "CanonFlashMode",
	    ([0: "Flash not fired",
	     1: "Auto",
	     2: "On",
	     3: "Red-eye reduction",
	     4: "Slow synchro",
	     5: "Auto + red-eye reduction",
	     6: "On + red-eye reduction",
	     16: "External flash", ]),
	    data[4]);

  add_field(res, "CanonDriveMode",
	    ([0:"Single", 1:"Continuous"]),
	    data[5]);

  add_field(res, "CanonFocusMode",
	    ([ 0: "One-Shot",
	     1: "AI Servo",
	     2: "AI Focus",
	     3: "MF",
	     4: "Single",
	     5: "Continuous",
	     6: "MF", ]),
	    data[7]);

  add_field(res, "CanonImageSize",
	    ([0: "Large", 1: "Medium", 2: "Small"]),
	    data[10]);

  add_field(res, "CanonShootingMode",
	    ([ 0: "Full Auto",
	     1: "Manual",
	     2: "Landscape",
	     3: "Fast Shutter",
	     4: "Slow Shutter",
	     5: "Night",
	     6: "B&W",
	     7: "Sepia",
	     8: "Portrait",
	     9: "Sports",
	     10: "Macro / Close-Up",
	     11: "Pan Focus"]),
	    data[11]);

  add_field(res, "CanonDigitalZoom",
	    ([0: "None", 1: "2X", 2: "4X"]),
	    data[12]);

  add_field(res, "CanonContrast",
	    ([0xffff:"Low", 0x0000:"Normal", 0x0001: "High"]),
	    data[13]);
  add_field(res, "CanonSaturation",
	    ([0xffff:"Low", 0x0000:"Normal", 0x0001: "High"]),
	    data[14]);
  add_field(res, "CanonSharpness",
	    ([0xffff:"Low", 0x0000:"Normal", 0x0001: "High"]),
	    data[15]);

  add_field(res, "CanonISO",
	    ([15: "Auto", 16: "50", 17: "100", 18: "200", 19: "400"]),
	    data[16]);

  add_field(res, "CanonMeteringMode",
	    ([3: "Evaluative", 4: "Partial", 5: "Center-weighted"]),
	    data[17]);

  add_field(res, "CanonFocusType",
	    ([0: "Manual", 1: "Auto", 3: "Close-up (macro)",
	     8: "Locked (pan mode)"]),
	    data[18]);

  add_field(res, "CanonAutoFocusPointSelected",
	    ([0x3000: "None (MF)",
	     0x3001: "Auto-selected",
	     0x3002: "Right",
	     0x3003: "Center",
	     0x3004: "Left"]),
	    data[19]);

  add_field(res, "CanonExposureMode",
	    ([0: "Easy shooting",
	     1: "Program",
	     2: "Tv-priority",
	     3: "Av-priority",
	     4: "Manual",
	     5: "A-DEP"]),
	    data[20]);

  float unit=(float)data[25];
  res->CanonLensFocalLengthRange=sprintf("%.0f-%.0f mm", data[24]/unit, data[23]/unit);

  add_field(res, "CanonFlashActivity",
	    ([0: "Did not fire", 1: "Fired"]), data[28]);

  array flashdetails=({});
  if(data[29] & 1<<14)
    flashdetails+=({"External E-TTL"});
  if(data[29] & 1<<13)
    flashdetails+=({"Internal flash"});
  if(data[29] & 1<<11)
    flashdetails+=({"FP sync used"});
  if(data[29] & 1<<7)
    flashdetails+=({"Rear-curtain sync used"});
  if(data[29] & 1<<4)
    flashdetails+=({"FP sync enabled"});

  if(sizeof(flashdetails))
    res->CanonFlashDetails=flashdetails*", ";
  else
    res->CanonFlashDetails="No flash";
  return res;
}

mapping canon_d30_multi1(array(int) data)
{
  mapping res=([]);

  add_field(res, "CanonWhiteBalance",
    ([ 0:"Auto",
       1:"Sunny",
       2:"Cloudy",
       3:"Tungsten",
       4:"Flourescent",
       5:"Flash",
       6:"Custom"]), data[7]);

  res->CanonBurstSequenceNumber=(string)data[9];
//    res->CanonAutoFocusPoint=sprintf("%b",data[14]);
  mapping flashbias=
    ([ 0xffc0: "-2 EV",
     0xffcc: "-1.67 EV",
     0xffd0: "-1.50 EV",
     0xffd4: "-1.33 EV",
     0xffe0: "-1 EV",
     0xffec: "-0.67 EV",
     0xfff0: "-0.50 EV",
     0xfff4: "-0.33 EV",
     0x0000: "±0 EV",
     0x000c: "+0.33 EV",
     0x0010: "+0.50 EV",
     0x0014: "+0.67 EV",
     0x0020: "+1 EV",
     0x002c: "+1.33 EV",
     0x0030: "+1.50 EV",
     0x0034: "+1.67 EV",
     0x0040: "+2 EV" ]);
  if(flashbias[data[15]])
    res->CanonFlashBias=flashbias[data[15]];
  res->CanonSubjectDistance=sprintf("%.2f",data[19]/100.0);
  return res;
}

mapping NIKON_990_MAKERNOTE = ([
  0x0001:	({"MN_0x0001",	    	    	0, ([]) }),
  0x0002:	({"MN_ISOSetting",   	    	0, ([]) }),
  0x0003:	({"MN_ColorMode",    	    	0, ([]) }),
  0x0004:	({"MN_Quality",	    	    	0, ([]) }),
  0x0005:	({"MN_Whitebalance", 	    	0, ([]) }),
  0x0006:	({"MN_ImageSharpening",	    	0, ([]) }),
  0x0007:	({"MN_FocusMode",    	    	0, ([]) }),
  0x0008:	({"MN_FlashSetting", 	    	0, ([]) }),
  0x000A:	({"MN_0x0008",	    	    	0, ([]) }),
  0x000F:	({"MN_ISOSelection", 	    	0, ([]) }),
  0x0080:	({"MN_ImageAdjustment",	    	0, ([]) }),
  0x0082:	({"MN_AuxiliaryLens",	    	0, ([]) }),
  0x0085:	({"MN_ManualFocusDistance",  	"FLOAT"}),
  0x0086:	({"MN_DigitalZoomFactor",    	"FLOAT"}),
  0x0088:	({"MN_AFFocusPosition",	    	"MAP",
		    ([ "\00\00\00\00": "Center",
		       "\00\01\00\00": "Top",
		       "\00\02\00\00": "Bottom",
		       "\00\03\00\00": "Left",
		       "\00\04\00\00": "Right",
		    ])  }),
  0x0010:	({"MN_DataDump",     	    	0, ([]) }),
]);

mapping CANON_D30_MAKERNOTE = ([
  0x0001:       ({"MN_Multi0",                  "CUSTOM", canon_d30_multi0 }),
  0x0004:       ({"MN_Multi1",                  "CUSTOM", canon_d30_multi1 }),
  0x0006:	({"MN_ImageType",	    	0, ([]) }),
  0x0007:	({"MN_FirmwareVersion",         0, ([]) }),
  0x0008:	({"MN_ImageNumber", 	    	0, ([]) }),
  0x0009:	({"MN_OwnerName", 	    	0, ([]) }),
  0x000C:	({"MN_CameraSerialNumber",  	0, ([]) }),
]);

mapping TAG_INFO = ([
  0x00fe:	({"NewSubFileType",  	    	0, ([]) }),
  0x0100:	({"ImageWidth",      	    	0, ([]) }),
  0x0101:	({"ImageLength",     	    	0, ([]) }),
  0x0102:	({"BitsPerSample",   	    	0, ([]) }),
  0x0103:	({"Compression",     	    	0, ([]) }),
  0x0106:	({"PhotometricInterpretation",	0, ([]) }),
  0x010e:	({"ImageDescription", 	    	0, ([]) }),
  0x010f:	({"Make",    	    	    	0, ([]) }),
  0x0110:	({"Model",   	    	    	0, ([]) }),
  0x0111:	({"StripOffsets",    	    	0, ([]) }),
  0x0112:	({"Orientation",     	    	0, ([]) }),
  0x0115:	({"SamplesPerPixel", 	    	0, ([]) }),
  0x0116:	({"RowsPerStrip",    	    	0, ([]) }),
  0x0117:	({"StripByteCounts", 	    	0, ([]) }),
  0x011a:	({"XResolution",     	    	0, ([]) }),
  0x011b:	({"YResolution",     	    	0, ([]) }),
  0x011c:	({"PlanarConfiguration",     	0, ([]) }),
  0x0128:	({"ResolutionUnit",  	    	"MAP",
		  ([ 1:	"Not Absolute",
		     2:	"Inch",
		     3:	"Centimeter", ]) }),
  0x0131:	({"Software",	    	    	0, ([]) }),
  0x0132:	({"DateTime",	    	    	0, ([]) }),
  0x013b:	({"Artist",  	    	    	0, ([]) }),
  0x0142:	({"TileWidth",	    	    	0, ([]) }),
  0x0143:	({"TileLength",	    	    	0, ([]) }),
  0x0144:	({"TileOffsets",     	    	0, ([]) }),
  0x0145:	({"TileByteCounts",  	    	0, ([]) }),
  0x014a:	({"SubIFDs", 	    	    	0, ([]) }),
  0x015b:	({"JPEGTables",	    	    	0, ([]) }),
  0x0201:	({"JPEGInterchangeFormat",   	0, ([]) }), // from EXIFread
  0x0202:	({"JPEGInterchangeFormatLength", 0, ([]) }), // from EXIFread
  0x0211:	({"YCbCrCoefficients",	    	0, ([]) }),
  0x0212:	({"YCbCrSubSampling",	    	0, ([]) }),
  0x0213:	({"YCbCrPositioning",	    	0, ([]) }),
  0x0214:	({"ReferenceBlackWhite",     	0, ([]) }),
  0x828f:	({"BatteryLevel",    	    	0, ([]) }),
  0x8298:	({"Copyright",	    	    	0, ([]) }),
  0x829a:	({"ExposureTime",    	    	"EXPOSURE", ([]) }),
  0x829d:	({"FNumber", 	    	    	"FLOAT", ([]) }),
  0x83bb:	({"IPTC_NAA",	    	    	0, ([]) }),
  0x8773:	({"InterColorProfile",	    	0, ([]) }),
  0x8822:	({"ExposureProgram", 	    	"MAP",
		    ([ 0:	"Unidentified",
		       1:	"Manual",
		       2:	"Program Normal",
		       3:	"Aperture Priority",
		       4:	"Shutter Priority",
		       5:	"Program Creative",
		       6:	"Program Action",
		       7:	"Portrait Mode",
		       8:	"Landscape Mode", ]) }),
  0x8824:	({"SpectralSensitivity",     	0, ([]) }),
  0x8825:	({"GPSInfo", 	    	    	0, ([]) }),
  0x8827:	({"ISOSpeedRating", 	    	0, ([]) }),
  0x8828:	({"OECF",    	    	    	0, ([]) }),
  0x8829:	({"Interlace",	    	    	0, ([]) }),
  0x882a:	({"TimeZoneOffset",  	    	0, ([]) }),
  0x882b:	({"SelfTimerMode",   	    	0, ([]) }),
  0x8769:	({"ExifOffset",	    	    	0, ([]) }), // from EXIFread
  0x9000:       ({"ExifVersion",     	    	0, ([]) }), // from EXIFread
  0x9003:       ({"DateTimeOriginal",	    	0, ([]) }),
  0x9004:       ({"DateTimeDigitized",	    	0, ([]) }), // from EXIFread
  0x9101:       ({"ComponentsConfiguration", 	0, ([]) }), // from EXIFread
  0x9102:	({"CompressedBitsPerPixel",  	0, ([]) }),
  0x9201:	({"ShutterSpeedValue",	    	0, ([]) }),
  0x9202:	({"ApertureValue",   	    	0, ([]) }),
  0x9203:	({"BrightnessValue", 	    	0, ([]) }),
  0x9204:	({"ExposureBiasValue",	    	"BIAS", ([]) }),
  0x9205:	({"MaxApertureValue",	    	"FLOAT", ([]) }),
  0x9206:	({"SubjectDistance", 	    	0, ([]) }),
  0x9207:	({"MeteringMode",    	    	"MAP",
		  ([  0:		    "Unidentified",
		      1:		    "Average",
		      2:		    "CenterWeightedAverage",
		      3:		    "Spot",
		      4:		    "MultiSpot",
		      5:                    "Matrix", ]) }),
  0x9208:	({"LightSource",     	    	0, ([]) }),
  0x9209:	({"Flash",   	    	    	"MAP",
		  ([  0:		"no",
		      1:		"fired",
		      5:		"fired (?)", // no return sensed
		      7:		"fired (!)", // return sensed
		      9:		"fill fired",
		      13:		"fill fired (?)",
		      15:		"fill fired (!)",
		      16:		"off",
		      24:		"auto off",
		      25:		"auto fired",
		      29:		"auto fired (?)",
		      31:		"auto fired (!)",
		      32:		"not available", ]) }),
  0x920a:	({"FocalLength",     	    	"FLOAT", ([]) }),
  0x920b:	({"FlashEnergy",     	    	0, ([]) }),
  0x920c:	({"SpatialFrequencyResponse",	0, ([]) }),
  0x920d:	({"Noise",   	    	    	0, ([]) }),
  0x920e:	({"FocalPlaneXResolution",   	0, ([]) }),
  0x920f:	({"FocalPlaneYResolution",   	0, ([]) }),
  0x9210:	({"FocalPlaneResolutionUnit",	0, ([]) }),
  0x9211:	({"ImageNumber",     	    	0, ([]) }),
  0x9212:	({"SecurityClassification",  	0, ([]) }),
  0x9213:	({"ImageHistory",    	    	0, ([]) }),
  0x9214:	({"SubjectLocation", 	    	0, ([]) }),
  0x9215:	({"ExposureIndex",   	    	0, ([]) }),
  0x9216:	({"TIFF_EPStandardID",	    	0, ([]) }),
  0x9217:	({"SensingMethod",   	    	0, ([]) }),
  0x927c:       ({"MakerNote",	    	    	"MAKE_MODEL",
		    ([
		      "NIKON_E990":  ({"TAGS", NIKON_990_MAKERNOTE}),
		      "NIKON":  ({"TAGS", NIKON_990_MAKERNOTE}),
		      "Canon":  ({"TAGS", CANON_D30_MAKERNOTE}),
		      "": ({ 0,([]) }) ]) }),
  0x9286:       ({"UserComment",                0, ([]) }),
  0xa000:       ({"FlashPixVersion",	       	0, ([]) }),
  0xa001:       ({"ColorSpace",	    	    	0, ([]) }),
  0xa002:       ({"ImageWidth",	    	    	0, ([]) }), // guess by rft
  0xa003:       ({"ImageHeight",     	    	0, ([]) }), // guess by rft
]);

mapping TAG_TYPE_INFO =
             ([1:	({"BYTE",	1}),
	       2:	({"ASCII",	1}),
	       3:	({"SHORT",	2}),
	       4:	({"LONG",	4}),
	       5:	({"RATIO",	8}),
	       6:	({"SBYTE",	1}),
	       7:	({"UNDEF",	1}),
	       8:	({"SSHORT",	2}),
	       9:	({"SLONG",	4}),
	       10:	({"SRATIO",	8}),
	       11:	({"FLOAT",	4}),
	       12:	({"DOUBLE",	8}),
	     ]);

int short_value(string str, string order)
{
  if(order=="MM")
    return (str[0]<<8)|str[1];
  else
    return (str[1]<<8)|str[0];
}

int long_value(string str, string order)
{
  if(order=="MM")
    return (str[0]<<24)|(str[1]<<16)|(str[2]<<8)|str[3];
  else
    return (str[3]<<24)|(str[2]<<16)|(str[1]<<8)|str[0];
}

void exif_seek(Stdio.File file, int offset, int exif_offset)
{
  file->seek(offset+exif_offset);
}

string format_bytes(string str)
{
  return str;
}

mapping parse_tag(Stdio.File file, mapping tags, mapping exif_info,
		  int exif_offset, string order)
{
  int tag_id=short_value(file->read(2), order);
  int tag_type=short_value(file->read(2), order);
  int tag_count=long_value(file->read(4), order);
  string make,model;

  [string tag_name, string tag_format, mapping|function tag_map]=
    exif_info[tag_id] || ({ sprintf("Tag0x%x",tag_id), 0, ([]) });

  if(tag_format=="MAKE_MODEL")
  {
    make=tags["Make"];
    model=tags["Model"];

    if(tag_map[make+"_"+model])
      [tag_format,tag_map]=tag_map[make+"_"+model];
    else if(tag_map[make])
      [tag_format,tag_map]=tag_map[make];
    else
      [tag_format,tag_map]=tag_map[""];
  }
  
  [string tag_type_name, int tag_type_len] = 
     TAG_TYPE_INFO[tag_type];

  int tag_len=tag_type_len * tag_count;
  
  int pos=file->tell();
  if(tag_len>4)
    exif_seek(file, long_value(file->read(4), order), exif_offset);

  if(tag_type==1 || tag_type==6 || tag_type==7)
  {
    if(tag_count==1)
      tags[tag_name]=(string)file->read(1)[0];
    else if(tag_format == "TAGS")
    {
      int num_entries=short_value(file->read(2), order);
      for(int i=0; i<num_entries; i++)
	tags|=parse_tag(file, tags, tag_map, exif_offset, order);
    }
    else
    {
      string str=file->read(tag_count);
      if(tag_format=="MAP")
	if(tag_map[str])
	  tags[tag_name]=tag_map[str];
	else
	{
	  make = tags["Make"];
	  model = tags["Model"];
	  tags[tag_name]= tag_map[make+"_"+model] || tag_map[make] || format_bytes(str);
	}
      else
	tags[tag_name]=format_bytes(str);
    }
  }

  if(tag_type==2) // ASCII
    tags[tag_name]=String.trim_whites(file->read(tag_count-1))-"\0";
  
  if(tag_type==3 || tag_type==8) // (S)SHORT
  {
    array a=allocate(tag_count);
    for(int i=0; i<tag_count; i++)
      a[i]=short_value(file->read(2), order);
    
    if(tag_format=="MAP")
      for(int i=0; i<tag_count; i++)
	if(tag_map[a[i]])
	  tags[tag_name]=tag_map[a[i]];
	else
	{
	  make = tags["Make"];
	  model = tags["Model"];
	  tags[tag_name]= tag_map[make+"_"+model] || tag_map[make] || (string)a[i];
	}
    else if(tag_format=="CUSTOM")
      tags|=tag_map(a);
    else
      tags[tag_name]=(array(string))a*", ";
  }
  
  if(tag_type==4 || tag_type==9) // (S)LONG
    for(int i=0;i<tag_count; i++)
      tags[tag_name]=(string)long_value(file->read(4), order);

  if(tag_type==5 || tag_type==10) // (S)RATIONAL
  {
    for(int i=0;i<tag_count; i++)
    {
      int long1=long_value(file->read(4), order);
      int long2=long_value(file->read(4), order);
      string val;
      switch(tag_format)
      {
  	case "BIAS":
  	  if(long1>0)
  	    val=sprintf("+%3.1f", long1*1.0/long2);
  	  else if(long1 < 0)
  	    val=sprintf("-%3.1f", -long1*1.0/long2);
  	  else
  	    val = "±0.0";
  	  break;
  	  
  	case "FLOAT":
  	  if(long2==0)
  	    tags[tag_name]="0";
  	  else
  	    tags[tag_name]=sprintf("%g", long1*1.0/long2);
  	  break;
  	  
  	case "EXPOSURE":
  	  tags[tag_name] = sprintf("%d/%d", long1,long2);
	  break;
//  	  break;
//    	  if(long1>=long2)
//    	    tags[tag_name]=sprintf("%g", long1*1.0/long2);
//    	  else
//    	    if(long1 > 1)
//    	      tags[tag_name]=sprintf("1/%d",  ((2*long2+long1)/(2*long1)));
//    	    else
//    	      tags[tag_name]=sprintf("1/%d", long1);
//    	  break;
  	  
  	default:
  	  tags[tag_name] = sprintf("%.2f", (float)long1/(float)long2);
  	  break;
      }
    }
  }

  file->seek(pos+4);
  return tags;
}

//! Retrieve the EXIF properties of the given file.
//! @param file
//!   The file containing wanted EXIF properties. Either a filename or a
//!   Stdio.File object.
//! @returns
//!   A mapping with all found EXIF properties.
mapping get_properties(Stdio.File|string file)
{
  int exif_offset=12;
  if(!objectp(file))
    file=Stdio.File(file, "rb");

  string skip=file->read(12);  // skip the jpeg header

  if (skip[strlen(skip)-6..]!="Exif\0\0")
  {
     skip=file->read(100);
     int z=search(skip,"Exif\0\0");
     if (z==-1) return ([]); // no exif header?
     exif_offset=z+18;
     file->seek(z+18);
  }

  string order=file->read(2);  // tiff order magic
  string code=file->read(2);   // tiff magic 42
  mapping tags=([]);

  if(sizeof(code)==2 && short_value(code, order)==42)
  {
    int offset=long_value(file->read(4), order);
    while(offset>0)
    {
      exif_seek(file,offset,exif_offset);
      int num_entries=short_value(file->read(2), order);
      for(int i=0; i<num_entries; i++)
	tags|=parse_tag(file, tags, TAG_INFO, exif_offset, order);

      offset=long_value(file->read(4), order);

      if(offset == 0)
	if(tags["ExifOffset"])
	{
	  offset=(int)tags["ExifOffset"];
	  m_delete(tags, "ExifOffset");
	}
    }
  }
  else
    tags=([]);

  return tags;
}
