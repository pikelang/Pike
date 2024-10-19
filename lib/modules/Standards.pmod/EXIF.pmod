#charset iso-8859-1
#pike __REAL_VERSION__

//! This module implements EXIF (Exchangeable image file format for
//! Digital Still Cameras) 2.2 parsing.

//  Johan Sch�n <js@roxen.com>, July 2001.
//  Based on Exiftool by Robert F. Tobler <rft@cg.tuwien.ac.at>.
//
// Some URLs:
// http://www.exif.org/
// http://www.dicasoft.de/casiomn.htm
// http://www.dalibor.cz/minolta/


protected void add_field(mapping m, string field,
		      mapping|array alts,
		      array(int) from, int index)
{
  if(index>=sizeof(from))
    return;
  index = from[index];
  if(mappingp(alts)) {
    if(alts[index])
      m[field]=alts[index];
    else
      m["failed "+field]=index;
  }
  else {
    if(index<sizeof(alts))
      m[field]=alts[index];
    else
      m["failed "+field]=index;
  }
}

string components_config(string data)
{
  mapping val = ([ 00: "",
		   01: "Y",
		   02: "Cb",
		   03: "Cr",
		   04: "R",
		   05: "G",
		   06: "B" ]);
  string res = "";
  foreach(data/"", string b)
    res += val[b[0]] || b;
  return res;
}

#define SIZETEST(X) if(sizeof(data)<(X)+1) return res
protected mapping canon_multi0(array(int) data)
{
  mapping res=([]);

  add_field(res, "CanonMacroMode",
	    ([1: "Macro", 2: "Normal"]),
	    data, 1);

  SIZETEST(2);
  if(data[2])
    res->CanonSelfTimerLength = sprintf("%.1f s", data[2]/10.0);

  add_field(res, "CanonQuality",
	    ([2: "Normal", 3: "Fine", 4: "Superfine", 5: "Superfine"]),
	    data, 3);

  add_field(res, "CanonFlashMode",
	    ([0: "Flash not fired",
	     1: "Auto",
	     2: "On",
	     3: "Red-eye reduction",
	     4: "Slow synchro",
	     5: "Auto + red-eye reduction",
	     6: "On + red-eye reduction",
	     16: "External flash", ]),
	    data, 4);

  add_field(res, "CanonDriveMode",
	    ({"Single", "Continuous"}),
	    data, 5);

  add_field(res, "CanonFocusMode",
	    ({ "One-Shot", "AI Servo",
	       "AI Focus", "MF", "Single",
	       "Continuous", "MF" }),
	    data, 7);

  add_field(res, "CanonImageSize",
	    ({ "Large", "Medium", "Small" }),
	    data, 10);

  add_field(res, "CanonShootingMode",
	    ({ "Full Auto", "Manual",
	       "Landscape", "Fast Shutter",
	       "Slow Shutter", "Night",
	       "B&W", "Sepia", "Portrait",
	       "Sports", "Macro / Close-Up",
	       "Pan Focus" }),
	    data, 11);

  add_field(res, "CanonDigitalZoom",
	    ({ "None", "2X", "4X" }),
	    data, 12);

  add_field(res, "CanonContrast",
	    ([0xffff:"Low", 0x0000:"Normal", 0x0001: "High"]),
	    data, 13);

  add_field(res, "CanonSaturation",
	    ([0xffff:"Low", 0x0000:"Normal", 0x0001: "High"]),
	    data, 14);

  add_field(res, "CanonSharpness",
	    ([0xffff:"Low", 0x0000:"Normal", 0x0001: "High"]),
	    data, 15);

  add_field(res, "CanonISO",
	    ([ 0:"100", 1:"200", 2:"400",
	       15:"Auto", 16:"50", 17:"100", 18:"200", 19:"400"]),
	    data, 16);

  add_field(res, "CanonMeteringMode",
	    ([ 0:"Central importance", 1:"Spot",
	       3:"Evaluative", 4:"Partial", 5:"Center-weighted" ]),
	    data, 17);

  add_field(res, "CanonFocusType",
	    ([0: "Manual", 1: "Auto", 3: "Close-up (macro)",
	     8: "Locked (pan mode)"]),
	    data, 18);

  add_field(res, "CanonAutoFocusPointSelected",
	    ([0x3000: "None (MF)",
	     0x3001: "Auto-selected",
	     0x3002: "Right",
	     0x3003: "Center",
	     0x3004: "Left"]),
	    data, 19);

  add_field(res, "CanonExposureMode",
	    ({ "Easy shooting", "Program",
	       "Tv-priority", "Av-priority",
	       "Manual", "A-DEP" }),
	    data, 20);

  SIZETEST(25);
  float unit=(float)data[25];
  res->CanonLensFocalLengthRange=sprintf("%.0f-%.0f mm", data[24]/unit,
					 data[23]/unit);

  add_field(res, "CanonFlashActivity",
	    ({ "Did not fire", "Fired" }), data, 28);

  SIZETEST(29);
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

  if (sizeof(data)>32)
  {
     add_field(res, "CanonFocusMode",
	       ({ "Single", "Continuous" }), data, 32);
  }

  return res;
}

protected mapping canon_multi1(array(int) data) {
  mapping res = ([]);

  SIZETEST(1);
  if(data[1]) res->CanonFocalLength = data[1]/32.0;
  return res;
}

protected mapping canon_multi3(array(int) data)
{
  mapping res=([]);

  SIZETEST(6);
  if(data[6]) {
    float v = 0.0;
    if(data[6]<65)
      v = v/32.0;
    else
      v = (0x10000-data[6])/32.0;
    res->CanonFlashBias = sprintf("%s%1.2f EV", (v>0?"+":""), v);
  }

  add_field(res, "CanonWhiteBalance",
    ({ "Auto", "Sunny", "Cloudy",
       "Tungsten", "Flourescent", "Flash",
       "Custom" }), data, 7);

  SIZETEST(9);
  res->CanonBurstSequenceNumber=(string)data[9];

  SIZETEST(10);
  res->CanonOpticalZoon=(string)data[10];

  SIZETEST(14);
  if(data[14]) {
    res->CanonAutoFocusPoint = ([
      0:"Right",
      1:"Center",
      2:"Left" ])[ data[14]&3 ];
    res->CanonAutoFocusPoints = (string)( data[14]>>11 );
  }

  add_field(res, "CanonFlashBias",
	    ([ 0xffc0: "-2 EV",
	       0xffcc: "-1.67 EV",
	       0xffd0: "-1.50 EV",
	       0xffd4: "-1.33 EV",
	       0xffe0: "-1 EV",
	       0xffec: "-0.67 EV",
	       0xfff0: "-0.50 EV",
	       0xfff4: "-0.33 EV",
	       0x0000: "�0 EV",
	       0x000c: "+0.33 EV",
	       0x0010: "+0.50 EV",
	       0x0014: "+0.67 EV",
	       0x0020: "+1 EV",
	       0x002c: "+1.33 EV",
	       0x0030: "+1.50 EV",
	       0x0034: "+1.67 EV",
	       0x0040: "+2 EV" ]),
	    data, 15);

  SIZETEST(19);
  res->CanonSubjectDistance=sprintf("%.2f",data[19]/100.0);
  return res;
}

protected mapping canon_multi4(array(int) data) {
  mapping res = ([]);
  add_field(res, "CanonStichDirection",
	    ({ "Left to right", "Right to left", "Bottom to top", "Top to bottom" }),
	    data, 5);
  return res;
}

protected mapping CANON_D30_MAKERNOTE = ([
  0x0001:       ({"MN_Multi0",                  "CUSTOM", canon_multi0 }),
  0x0002:       ({"MN_Multi1",                  "CUSTOM", canon_multi1 }),
  0x0004:       ({"MN_Multi3",                  "CUSTOM", canon_multi3 }),
  0x0005:       ({"MN_Multi4",                  "CUSTOM", canon_multi4 }),
  0x0006:	({"MN_ImageType",	    	}),
  0x0007:	({"MN_FirmwareVersion",         }),
  0x0008:	({"MN_ImageNumber", 	    	}),
  0x0009:	({"MN_OwnerName", 	    	}),
  0x000C:	({"MN_CameraSerialNumber",  	}),
  0x000D:       ({"MN_CameraInfo",              }),
  0x000F:       ({"MN_CustomFunctions",         }),
  0x0010:       ({"MN_ModelID",                 }),
]);

protected mapping NIKON_D_MAKERNOTE = ([
  0x0001: ({ "MN_FirmwareVersion" }),
  0x0002: ({ "MN_ISO" }),
  0x0004: ({ "MN_Quality" }),
  0x0005: ({ "MN_WhiteBalance" }),
  0x0006: ({ "MN_Sharpening" }),
  0x0007: ({ "MN_FocusMode" }),
  0x0008: ({ "MN_FlashSetting" }),
  0x0009: ({ "MN_FlashMode" }),
  0x000b: ({ "MN_WhiteBalanceFine" }),
  0x000c: ({ "MN_WhiteBalandeRBCoefficients" }),
  0x0012: ({ "MN_FlashCompensation" }),
  0x0013: ({ "MN_ISO2" }),
  0x0081: ({ "MN_ToneCompensation" }),
  0x0083: ({ "MN_LensType", "MAP",
	     ([ "0" : "AF non D",
		"1" : "Manual",
		"2" : "AF-D/AF-S",
		"6" : "AF-D G",
		"10" : "AF-D VR",
	     ]) }),
  0x0084: ({ "MN_Lens" }),
  0x0087: ({ "MN_FlashUsed", "MAP",
	     ([ "0" : "Flash not fired",
		"4" : "Unknown",
		"7" : "External",
		"9" : "On camera",
	     ]) }),
  0x008c: ({ "MN_ContrastCurve" }),
  0x008d: ({ "MN_ColorMode" }),
  0x0090: ({ "MN_LightType" }),
  0x0092: ({ "MN_Hue" }),
  0x0e01: ({ "MN_CaptureEditorData" }),
]);

protected mapping|string nikon_D70_makernote(string data, mapping real_tags) {
  object f = Stdio.FakeFile(data);
  if(f->read(10)!="Nikon\0\2\20\0\0") return data;

  mapping tags = low_get_properties(f, NIKON_D_MAKERNOTE, 0);
  if(!tags) return data;

  foreach(tags; string name; mixed value)
    if(has_prefix(name, "MN_"))
      real_tags[name] = m_delete(tags, name);
  if(sizeof(tags))
    return tags;
  return UNDEFINED;
}

protected mapping nikon_iso(array(int) data) {
  mapping res=([]);

  SIZETEST(1);
  if(data[1]) res->NikonISOSetting = "ISO"+data[1];
  return res;
}

// NIKON 990, D1 (more?)
protected mapping NIKON_990_MAKERNOTE = ([
  0x0001:	({"MN_0x0001",	    	    	}),
  0x0002:	({"MN_ISOSetting",   	    	"CUSTOM", nikon_iso }),
  0x0003:	({"MN_ColorMode",    	    	}),
  0x0004:	({"MN_Quality",	    	    	}),
  0x0005:	({"MN_Whitebalance", 	    	}),
  0x0006:	({"MN_ImageSharpening",	    	}),
  0x0007:	({"MN_FocusMode",    	    	}),
  0x0008:	({"MN_FlashSetting", 	    	}),
  0x000A:	({"MN_0x000A",	    	    	}),
  0x000F:	({"MN_ISOSelection", 	    	}),
  0x0080:	({"MN_ImageAdjustment",	    	}),
  0x0082:	({"MN_AuxiliaryLens",	    	}),
  0x0085:	({"MN_ManualFocusDistance",  	"FLOAT" }),
  0x0086:	({"MN_DigitalZoomFactor",    	"FLOAT" }),
  0x0088:	({"MN_AFFocusPosition",	    	"MAP",
		    ([ "\00\00\00\00": "Center",
		       "\00\01\00\00": "Top",
		       "\00\02\00\00": "Bottom",
		       "\00\03\00\00": "Left",
		       "\00\04\00\00": "Right",
		    ])  }),
  0x0094:       ({"MN_Saturation", "MAP",
		  ([ -3 : "B&W",
		     -2 : "-2",
		     -1 : "-1",
		     0 : "0",
		     1 : "1",
		     2 : "2"
		  ]) }),
  0x0095:       ({"MN_NoiseReduction",          }),
  0x0010:	({"MN_DataDump",     	    	}),
]);

protected mapping sanyo_specialmode(array(int) data) {
  mapping res = ([]);

  add_field(res, "SanyoSpecialMode",
	    ([ 0:"Normal",
	       2:"Fast",
	       3:"Panorama" ]), data, 0);

  SIZETEST(1);
  if(data[1]) res->SanyoSpecialModeSequence = (string)data[1];

  add_field(res, "SanyoSpecialModeDirection",
            ([ 0:"Non-panoramic",
	       1:"Left to right",
	       2:"Right to left",
	       3:"Bottom to top",
	       4:"Top to bottom" ]), data, 2);
  return res;
}

protected mapping sanyo_jpegquality(array(int) data) {
  mapping res=([]);

  int r = data[0]&0xff;
  add_field(res, "SanyoJPEGQualityResolution",
	    ({ "Very low", "Low", "Medium low", "Medium", "Medium high",
	       "High", "Very High", "Super high" }), ({ r }), 0);

  r = (data[0]&0xff00)>>8;
  add_field(res, "SanyoJPEGQualityDetail",
	    ({ "Normal", "Fine", "Super Fine" }), ({ r }), 0);
  return res;
}

protected mapping SANYO_MAKERNOTE = ([
  0x00ff:       ({"MN_StartOffset",             }),
  0x0100:       ({"MN_JPEGThumbnail",           }),
  0x0200:       ({"MN_SpecialMode",             "CUSTOM", sanyo_specialmode }),
  0x0201:       ({"MN_JPEGQuality",             "CUTSOM", sanyo_jpegquality }),
  0x0202:       ({"MN_Macro",                   "MAP",
		  ([ 0:"Normal",
		     1:"Macro",
		     2:"View",
		     3:"Manual" ]) }),
  0x0204:       ({"MN_DigitalZoom",             }),
  0x0207:       ({"MN_SoftwareRelease",         }),
  0x0208:       ({"MN_PictInfo",                }),
  0x0209:       ({"MN_CameraID",                }),
  0x020e:       ({"MN_SequentialShotMethod",    "MAP",
		  ([ 0:"None",
		     1:"Standard",
		     2:"Best",
		     3:"Adjust exposure" ]) }),
  0x020f:       ({"MN_WideRange",               "MAP", ([ 0:"Off", 1:"On" ]) }),
  0x0210:       ({"MN_ColourAdjustmentMode",    "MAP", ([ 0:"Off", 1:"On" ]) }),
  0x0213:       ({"MN_QuickShot",               "MAP", ([ 0:"Off", 1:"On" ]) }),
  0x0214:       ({"MN_SelfTimer",               "MAP", ([ 0:"Off", 1:"On" ]) }),
  0x0216:       ({"MN_VoiceMemo",               "MAP", ([ 0:"Off", 1:"On" ]) }),
  0x0217:       ({"MN_RecordShutterRelease",    "MAP",
		  ([ 0:"Record whilst held",
		     1:"Press to start. Press to stop." ]) }),
  0x0218:       ({"MN_FlickerReduce",           "MAP", ([ 0:"Off", 1:"On" ]) }),
  0x0219:       ({"MN_OpticalZoom",             "MAP", ([ 0:"Disabled", 1:"Enabled" ]) }),
  0x021b:       ({"MN_DigitalZoom",             "MAP", ([ 0:"Disabled", 1:"Enabled" ]) }),
  0x021d:       ({"MN_LightSourceSpecial",      "MAP", ([ 0:"Off", 1:"On" ]) }),
  0x021e:       ({"MN_Resaved",                 "MAP", ([ 0:"No", 1:"Yes" ]) }),
  0x021f:       ({"MN_SceneSelect",             "MAP",
		  ([ 0:"Off",
		     1:"Sport",
		     2:"TV",
		     3:"Night",
		     4:"User 1",
		     5:"User 2" ]) }),
  0x0223:       ({"MN_ManualFocalDistance",     }),
  0x0224:       ({"MN_SequentialShotInterval",  "MAP",
		  ([ 0:"5 frames/s",
		     1:"10 frames/s",
		     2:"15 frames/s",
		     3:"20 frames/s" ]) }),
  0x0225:       ({"MN_FlashMode",               "MAP",
		  ([ 0:"Auto",
		     1:"Force",
		     2:"Disabled",
		     3:"Red eye" ]) }),
  0x0e00:       ({"MN_PrintIMFlags",            }),
  0x0f00:       ({"MN_DataDump",                }),
]);

protected mapping OLYMPUS_MAKERNOTE = ([
  0x0100:       ({"MN_JPEGThumbnail"            }),
  0x0200:       ({"MN_SpecialMode",             "CUSTOM", sanyo_specialmode }),
  0x0201:       ({"MN_JPEGQuality",             "MAP",
		  ([ 0:"SQ",
		     1:"HQ",
		     2:"SHQ" ]) }),
  0x0202:       ({"MN_Macro",                   "MAP", ([ 0:"No", 1:"Yes" ]) }),
  0x0204:       ({"MN_DigitalZoom",             "MAP", ([ 0:"1x", 1:"2x" ]) }),
  0x0207:       ({"MN_SoftwareRelease",         }),
  0x0208:       ({"MN_PictInfo",                }),
  0x0209:       ({"MN_CameraID",                }),
  0x0f00:       ({"MN_DataDump",                }),
]);

// Nikon E700/E800/E900/E900S/E910/E950
protected mapping NIKON_MAKERNOTE = ([
  0x0002:       ({"MN_0x0002",                   }),
  0x0003:       ({"MN_Quality",                 "MAP",
		  ([ 1:"VGA Basic",
		     2:"VGA Normal",
		     3:"VGA Fine",
		     4:"SXGA Basic",
		     5:"SXGA Normal",
		     6:"SXGA Fine" ]) }),
  0x0004:       ({"MN_ColorMode",               "MAP", ([ 1:"Color", 2:"Monochrome" ]) }),
  0x0005:       ({"MN_ImageAdjustment",         "MAP",
		  ([ 0:"Normal",
		     1:"Bright+",
		     2:"Bright-",
		     3:"Contrast+",
		     4:"Contrast-" ]) }),
  0x0006:       ({"MN_CCDSensitivity",          "MAP",
		  ([ 0:"ISO80",
		     1:"ISO160",
		     4:"ISO320",
		     5:"ISO100" ]) }),
  0x0007:       ({"MN_WhiteBalance",            "MAP",
		  ([ 0:"Auto",
		     1:"Preset",
		     2:"Daylight",
		     3:"Incandescense",
		     4:"Flourescense",
		     5:"Cloudy",
		     6:"SpeedLight" ]) }),
  0x0008:       ({"MN_Focus",                   }),
  0x0009:       ({"MN_0x0009",                  }),
  0x000a:       ({"MN_DigitalZoom",             }),
  0x000b:       ({"MN_Converter",               "MAP", ([ 1:"Fisheye converter" ]) }),
  0x0f00:       ({"MN_0x0f00",                  }),
]);

protected mapping CASIO_MAKERNOTE = ([
  0x0001:       ({"MN_RecordingMode",           "MAP",
		  ([ 1:"Single Shutter",
		     2:"Panorama",
		     7:"Panorama",
		     3:"Night Scene",
		     10:"Night Scene",
		     4:"Portrait",
		     15:"Portrait",
		     5:"Landscape",
		     16:"Landscape" ]) }),
  0x0002:       ({"MN_Quality",                 "MAP",
		  ([ 1:"Economy",
		     2:"Normal",
		     3:"Fine" ]) }),
  0x0003:       ({"MN_FocusingMode",            "MAP",
		  ([ 2:"Macro",
		     3:"Auto Focus",
		     4:"Manual Focus",
		     5:"Infinity" ]) }),
  0x0004:       ({"MN_FlashMode",               "MAP",
		  ([ 1:"Auto",
		     2:"On",
		     3:"Off",
		     4:"Red Eye Reduction" ]) }),
  0x0005:       ({"MN_FlashIntensity",          "MAP",
		  ([ 11:"Weak",
		     13:"Normal",
		     15:"Strong" ]) }),
  0x0006:       ({"MN_ObjectDistance",          }),
  0x0007:       ({"MN_WhiteBalance",            "MAP",
		  ([ 1:"Auto",
		     2:"Tungsten",
		     3:"Daylight",
		     4:"Flourescent",
		     5:"Shade",
		     129:"Manual" ]) }),
  0x000a:       ({"MN_DigitalZoom",             "MAP",
		  ([ 0x10000:"Off",
		     0x10001:"2X Digital Zoom" ]) }),
  0x000b:       ({"MN_Sharpness",               "MAP",
		  ([ 0:"Normal",
		     1:"Soft",
		     2:"Hard" ]) }),
  0x000c:       ({"MN_Contrast",                "MAP",
		  ([ 0:"Normal",
		     1:"Low",
		     2:"High" ]) }),
  0x000d:       ({"MN_Saturation",              "MAP",
		  ([ 0:"Normal",
		     1:"Low",
		     2:"High" ]) }),
  0x0014:       ({"MN_CCDSensitivity",          "MAP",
		  ([ 64:"Normal",
		     125:"+1.0",
		     250:"+2.0",
		     244:"+3.0",
		     80:"Normal",
		     100:"High", ]) }),
]);

protected mapping FUJIFILM_MAKERNOTE = ([
  0x0000:       ({"MN_Version",                 }),
  0x1000:       ({"MN_Quality",                 }),
  0x1001:       ({"MN_Sharpness",               "MAP",
		  ([ 1:"Soft",
		     2:"Soft",
		     3:"Normal",
		     4:"Hard",
		     5:"Hard" ]) }),
  0x1002:       ({"MN_WhiteBalance",            "MAP",
		  ([ 0:"Auto",
		     256:"Daylight",
		     512:"Cloudy",
		     768:"DaylightColor-fluorescence",
		     769:"DaywhiteColor-fluorescence",
		     770:"White-fluorescence",
		     1024:"Incandenscence",
		     3840:"Custom white balance" ]) }),
  0x1003:       ({"MN_Saturation",              "MAP",
		  ([ 0:"Normal",
		     256:"High",
		     512:"Low" ]) }),
  0x1004:       ({"MN_Contrast",                "MAP",
		  ([ 0:"Normal",
		     256:"High",
		     512:"Low" ]) }),
  0x1010:       ({"MN_FlashMode",               "MAP",
		  ([ 0:"Auto",
		     1:"On",
		     2:"Off",
		     3:"Red-eye reduction" ]) }),
  0x1011:       ({"MN_FlashStrength",           }),
  0x1020:       ({"MN_Macro",                   "MAP", ([ 0:"Off", 1:"On" ]) }),
  0x1021:       ({"MN_FocusMode",               "MAP", ([ 0:"Auto focus", 1:"Manual focus" ]) }),
  0x1030:       ({"MN_SlowSync",                "MAP", ([ 0:"Off", 1:"On" ]) }),
  0x1031:       ({"MN_PictureMode",             "MAP",
		  ([ 0:"Auto",
		     1:"Portrait scene",
		     2:"Landscape scene",
		     4:"Sports scene",
		     5:"Night scene",
		     6:"Program AE",
		     256:"Aperture prior AE",
		     512:"Shutter prior AE",
		     768:"Manual exposure" ]) }),
  0x1300:       ({"MN_BlurWarning",             "MAP", ([ 0:"No", 1:"Yes" ]) }),
  0x1301:       ({"MN_FocusWarning",            "MAP", ([ 0:"No", 1:"Yes" ]) }),
  0x1302:       ({"MN_AEWarning",               "MAP", ([ 0:"No", 1:"Yes" ]) }),
]);

protected mapping GPS_TAGS = ([
  0x0000: ({"GPSVersionID"}),
  0x0001: ({"GPSLatitudeRef"}),
  0x0002: ({"GPSLatitude"}),
  0x0003: ({"GPSLongitudeRef"}),
  0x0004: ({"GPSLongitude"}),
  0x0005: ({"GPSAltitudeRef"}),
  0x0006: ({"GPSAltitude"}),
  0x0007: ({"GPSTimeStamp"}),
  0x0008: ({"GPSSatellites"}),
  0x0009: ({"GPSStatus"}),
  0x000A: ({"GPSMeasureMode"}),
  0x000B: ({"GPSDOP"}),
  0x000C: ({"GPSSpeedRef"}),
  0x000D: ({"GPSSpeed"}),
  0x000E: ({"GPSTrackRef"}),
  0x000F: ({"GPSTrack"}),
  0x0010: ({"GPSImgDirectionRef"}),
  0x0011: ({"GPSImgDirection"}),
  0x0012: ({"GPSMapDatum"}),
  0x0013: ({"GPSDestLatitudeRef"}),
  0x0014: ({"GPSDestLatitude"}),
  0x0015: ({"GPSDestLongitudeRef"}),
  0x0016: ({"GPSDestLongitude"}),
  0x0017: ({"GPSDestBearingRef"}),
  0x0018: ({"GPSDestBearing"}),
  0x0019: ({"GPSDestDistanceRef"}),
  0x001A: ({"GPSDestDistance"}),
]);

protected mapping TAG_INFO = ([
  0x0001:       ({"InteroperabilityIndex",      }),
  0x0002:       ({"InteroperabilityVersion",    }),
  0x00fe:	({"NewSubFileType",  	    	}),
  0x0100:	({"ImageWidth",      	    	}),
  0x0101:	({"ImageLength",     	    	}),
  0x0102:	({"BitsPerSample",   	    	}),
  0x0103:	({"Compression",     	    	}),
  0x0106:	({"PhotometricInterpretation",	}),
  0x010a:       ({"FillOrder",                  }),
  0x010d:       ({"DocumentName",               }),
  0x010e:	({"ImageDescription", 	    	}),
  0x010f:	({"Make",    	    	    	}),
  0x0110:	({"Model",   	    	    	}),
  0x0111:	({"StripOffsets",    	    	}),
  0x0112:	({"Orientation",     	    	}),
  0x0115:	({"SamplesPerPixel", 	    	}),
  0x0116:	({"RowsPerStrip",    	    	}),
  0x0117:	({"StripByteCounts", 	    	}),
  0x011a:	({"XResolution",     	    	}),
  0x011b:	({"YResolution",     	    	}),
  0x011c:	({"PlanarConfiguration",     	}),
  0x0128:	({"ResolutionUnit",  	    	"MAP",
		  ([ 1:	"Not Absolute",
		     2:	"Inch",
		     3:	"Centimeter", ]) }),
  0x012d:       ({"TransferFunction",           }),
  0x0131:	({"Software",	    	    	}),
  0x0132:	({"DateTime",	    	    	}),
  0x013b:	({"Artist",  	    	    	}),
  0x013c:       ({"HostComputer",               }),
  0x013e:       ({"WhitePoint",                 }),
  0x013f:       ({"PrimaryChromaticities",      }),
  0x0142:	({"TileWidth",	    	    	}),
  0x0143:	({"TileLength",	    	    	}),
  0x0144:	({"TileOffsets",     	    	}),
  0x0145:	({"TileByteCounts",  	    	}),
  0x014a:	({"SubIFDs", 	    	    	}),
  0x0156:       ({"TransferRange",              }),
  0x015b:	({"JPEGTables",	    	    	}),
  0x0200:       ({"JPEGProc",                   }),
  0x0201:	({"JPEGInterchangeFormat",   	}), // from EXIFread
  0x0202:	({"JPEGInterchangeFormatLength", }), // from EXIFread
  0x0211:	({"YCbCrCoefficients",	    	}),
  0x0212:	({"YCbCrSubSampling",	    	}),
  0x0213:	({"YCbCrPositioning",	    	}),
  0x0214:	({"ReferenceBlackWhite",     	}),
  0x1000:       ({"RelatedImageFileFormat",     }),
  0x1001:       ({"RelatedImageWidth",          }),
  0x1002:       ({"RelatedImageLength",         }),
  0x828d:       ({"CFARepeatPatternDim",        }),
  0x828f:	({"BatteryLevel",    	    	}),
  0x8298:	({"Copyright",	    	    	}),
  0x829a:	({"ExposureTime",    	    	"EXPOSURE" }),
  0x829d:	({"FNumber", 	    	    	"FLOAT" }),
  0x83bb:	({"IPTC_NAA",	    	    	}),
  0x8769:       ({"ExifOffset",                 }),
  0x8773:	({"InterColorProfile",	    	}),
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
  0x8824:	({"SpectralSensitivity",     	}),
  0x8825:	({"GPSInfo", 	    	    	}),
  0x8827:	({"ISOSpeedRating", 	    	}),
  0x8828:	({"OECF",    	    	    	}),
  0x8829:	({"Interlace",	    	    	}),
  0x882a:	({"TimeZoneOffset",  	    	}),
  0x882b:	({"SelfTimerMode",   	    	}),
  0x9000:       ({"ExifVersion",     	    	}), // from EXIFread
  0x9003:       ({"DateTimeOriginal",	    	}),
  0x9004:       ({"DateTimeDigitized",	    	}), // from EXIFread
  0x9101:       ({"ComponentsConfiguration", 	"CUSTOM", components_config }),
  0x9102:	({"CompressedBitsPerPixel",  	}),
  0x9201:	({"ShutterSpeedValue",	    	}),
  0x9202:	({"ApertureValue",   	    	}),
  0x9203:	({"BrightnessValue", 	    	}),
  0x9204:	({"ExposureBiasValue",	    	"BIAS" }),
  0x9205:	({"MaxApertureValue",	    	"FLOAT" }),
  0x9206:	({"SubjectDistance", 	    	}),
  0x9207:	({"MeteringMode",    	    	"MAP",
		  ([  0:		    "Unidentified",
		      1:		    "Average",
		      2:		    "CenterWeightedAverage",
		      3:		    "Spot",
		      4:		    "MultiSpot",
		      5:                    "Pattern",
		      6:                    "Partial",
		      255:                  "Other", ]) }),
  0x9208:	({"LightSource",     	    	"MAP",
		  ([  0:                    "Unknown",
		      1:                    "Daylight",
		      2:                    "Fluorescent",
		      3:                    "Tungsten",
		      4:                    "Flash",
		      9:                    "Fine weather",
		      10:                   "Cloudy weather",
		      11:                   "Shade",
		      12:                   "Daylight fluorescent",
		      13:                   "Day white fluorescent",
		      14:                   "Cool white fluorescent",
		      15:                   "White fluorescent",
		      17:                   "Standard light A",
		      18:                   "Standard light B",
		      19:                   "Standard light C",
		      20:                   "D55",
		      21:                   "D65",
		      22:                   "D75",
		      23:                   "D50",
		      24:                   "ISO studio tungsten",
		      255:                  "Other light source", ]) }),
  0x9209:	({"Flash",   	    	    	"MAP",
		  ([  0:		"no",
		      1:		"fired",
		      5:		"fired (?)", // no return sensed
		      7:		"fired (!)", // return sensed
		      9:		"fill fired",
		      13:		"fill fired (?)",
		      15:		"fill fired (!)",
		      16:		"fill no",
		      24:		"auto off",
		      25:		"auto fired",
		      29:		"auto fired (?)",
		      31:		"auto fired (!)",
		      32:		"not available",
		      65:               "red-eye fired",
		      69:               "red-eye fired (?)",
		      71:               "red-eye fired (!)",
		      73:               "red-eye fill fired",
		      77:               "red-eye fill fired (?)",
		      79:               "red-eye fill fired (!)",
		      89:               "red-eye auto fired",
		      93:               "red-eye auto fired (?)",
		      95:               "red-eye auto fired (!)", ]) }),
  0x920a:	({"FocalLength",     	    	"FLOAT" }),
  0x920b:	({"FlashEnergy",     	    	}),
  0x920c:	({"SpatialFrequencyResponse",	}),
  0x920d:	({"Noise",   	    	    	}),
  0x920e:	({"FocalPlaneXResolution",   	}),
  0x920f:	({"FocalPlaneYResolution",   	}),
  0x9210:	({"FocalPlaneResolutionUnit",	}),
  0x9211:	({"ImageNumber",     	    	}),
  0x9212:	({"SecurityClassification",  	}),
  0x9213:	({"ImageHistory",    	    	}),
  0x9214:	({"SubjectArea", 	    	}),
  0x9215:	({"ExposureIndex",   	    	}),
  0x9216:	({"TIFF_EPStandardID",	    	}),
  0x9217:	({"SensingMethod",   	    	}),
  0x927c:       ({"MakerNote",	    	    	"MAKE_MODEL",
		    ([
		      "NIKON_E990":  ({"TAGS", NIKON_990_MAKERNOTE}),
		      "NIKON":  ({"TAGS", NIKON_990_MAKERNOTE}),
		      "NIKON CORPORATION": ({"CUSTOM", nikon_D70_makernote}),
		      "Canon":  ({"TAGS", CANON_D30_MAKERNOTE}),
		      "FUJIFILM": ({"TAGS", FUJIFILM_MAKERNOTE}),
		      "OLYMPUS DIGITAL CAMERA": ({"TAGS", OLYMPUS_MAKERNOTE}),
		      "SANYO DIGITAL CAMERA": ({"TAGS", SANYO_MAKERNOTE}),
		      "CASIO": ({"TAGS", CASIO_MAKERNOTE}),

		      // FIXME: Reverse engineer these.
		      "FUJIFILM_FinePix2400Zoom": ({ 0, ([]) }),
		      "FUJIFILM_FinePix4700 ZOOM": ({ 0, ([]) }),

		      "": ({ 0, ([]) }) ]) }),
  0x9286:       ({"UserComment",                }),
  0x9290:       ({"SubSecTime",                 }),
  0x9291:       ({"SubSecTimeOriginal",         }),
  0x9292:       ({"SubSecTimeDigitized",        }),
  0xa000:       ({"FlashPixVersion",	       	}),
  0xa001:       ({"ColorSpace",	    	    	}),
  0xa002:       ({"PixelXDimension",   	    	}),
  0xa003:       ({"PixelYDimension",   	    	}),
  0xa004:       ({"RelatedSoundFile",           }),
  0xa005:       ({"ExifInterabilityOffset",     }),
  0xa20b:       ({"FlashEnergy",                }),
  0xa20c:       ({"SpatialFrequencyResponse",   }),
  0xa20e:       ({"FocalPlaneXResolution",      }),
  0xa20f:       ({"FocalPlaneYResolution",      }),
  0xa210:       ({"FocalPlaneResolutionUnit",   }),
  0xa214:       ({"SubjectLocation",            }),
  0xa215:       ({"ExposureIndex",              }),
  0xa217:       ({"SensingMethod",              }),
  0xa300:       ({"FileSource",                 }),
  0xa301:       ({"SceneType",                  }),
  0xa302:       ({"CFAPattern",                 }),
  0xa401:       ({"CustomRendered",             }),
  0xa402:       ({"ExposureMode",               "MAP",
		  ([  0:                "Auto exposure",
		      1:                "Manual exposure",
		      2:                "Auto bracket", ]) }),
  0xa403:       ({"WhiteBalance",               "MAP",
		  ([  0:                "Auto",
		      1:                "Manual", ]) }),
  0xa404:       ({"DigitalZoomRatio",           "FLOAT" }),
  0xa405:       ({"FocalLengthIn35mmFilm",      }),
  0xa406:       ({"SceneCaptureType",           "MAP",
		  ([  0:                "Standard",
		      1:                "Landscape",
		      2:                "Portrait",
		      3:                "Night scene", ]) }),
  0xa407:       ({"GainControl",                "MAP",
		  ([  0:                "None",
		      1:                "Low gain up",
		      2:                "High gain up",
		      3:                "Low gain down",
		      4:                "High gain down", ]) }),
  0xa408:       ({"Contrast",                   "MAP",
		  ([  0:                "Normal",
		      1:                "Soft",
		      2:                "Hard", ]) }),
  0xa409:       ({"Saturation",                 "MAP",
		  ([  0:                "Normal",
		      1:                "Low saturation",
		      2:                "Hight saturation", ]) }),
  0xa40a:       ({"Sharpness",                  "MAP",
		  ([  0:                "Normal",
		      1:                "Soft",
		      2:                "Hard", ]) }),
  0xa40b:       ({"DeviceSettingDescription",   }),
  0xa40c:       ({"SubjectDistanceRange",       "MAP",
		  ([  0:                "Unknown",
		      1:                "Macro",
		      2:                "Close view",
		      3:                "Distant view", ]) }),
  0xa420:       ({"ImageUniqueID",              "ASCII" }),
]);

protected mapping TAG_TYPE_INFO =
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

protected string format_bytes(string str)
{
  return str;
}

protected mapping parse_tag(TIFF file, mapping tags, mapping exif_info,
                            int discard_unknown)
{
  // JEITA CP-3451 4.6.2:
  //
  //   Each of the 12-byte field Interoperability consists
  //   of the following four elements respectively.
  //
  //     Bytes 0-1	Tag
  //     Bytes 2-3	Type
  //     Bytes 4-7	Count
  //     Bytes 8-11	Value Offset
  int tag_id=file->read_short();
  int tag_type=file->read_short();
  int tag_count=file->read_long();
  string make,model;

  // Attempt to fix files with incorrect byteorder.
  if(!TAG_TYPE_INFO[tag_type]) {
    tag_type = Int.swap_word(tag_type);
    if(!TAG_TYPE_INFO[tag_type]) return tags;
    tag_id = Int.swap_word(tag_id);
    tag_count = Int.swap_long(tag_count);
  }

  [string tag_type_name, int tag_type_len] =
     TAG_TYPE_INFO[tag_type];

  int tag_len=tag_type_len * tag_count;

  // Using the tag id, now look up how we should interpret the
  // data. Note that the tag format is our own invention: MAKE_MODEL,
  // TAGS, MAP, CUSTOM, BIAS, FLOAT, EXPOSURE.
  string tag_name, tag_format;
  mapping|function tag_map;
  array temp = exif_info[tag_id];
  if(!temp || !sizeof(temp)) {
    if( discard_unknown )
    {
      file->read(4);
      return tags;
    }
    tag_name = sprintf("%s0x%04x", (exif_info->prefix||"Tag"), tag_id);
    tag_format = 0;
    tag_map = ([]);
  }
  else {
    tag_name = temp[0];
    tag_format = sizeof(temp)>1?temp[1]:0;
    tag_map = sizeof(temp)>2?temp[2]:([]);
  }

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

  int pos=file->tell();
  if(tag_len>4)
    file->exif_seek(file->read_long());

  switch(tag_type)
  {
  case 1: // BYTE
  case 6: // SBYTE
  case 7: // UNDEF
    if(tag_count==1)
      tags[tag_name]=(string)file->read(1)[0];
    else if(tag_format == "TAGS")
    {
      int num_entries=file->read_short();
      for(int i=0; i<num_entries; i++) {
	catch {
          tags|=parse_tag(file, tags, tag_map, discard_unknown);
	};
      }
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
      else if(tag_format=="CUSTOM") {
	mixed res = ([function]tag_map)(str,tags);
	if(!undefinedp(res))
	  tags[tag_name]=res;
      }
      else
	tags[tag_name]=format_bytes(str);
    }
    break;

  case 2: // ASCII
    tags[tag_name]=String.trim_whites(file->read(max(tag_count-1, 0)))-"\0";
    break;

  case 3: // SHORT
  case 8: // SSHORT
    {
      if(tag_count>0xffff) return ([]); // Impossible amount of tags.
      array a=allocate(tag_count);

      for(int i=0; i<tag_count; i++)
        a[i]=file->read_short();

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
        tags|= ([function]tag_map)(a);
      else
        tags[tag_name]=(array(string))a*", ";
    }
    break;

  case 4: // LONG
  case 9: // SLONG
    for(int i=0;i<tag_count; i++)
      tags[tag_name]=(string)file->read_long();
    break;

  case 5: // RATIONAL
  case 10: // SRATIONAL
    for(int i=0;i<tag_count; i++)
    {
      int long1=file->read_long();
      int long2=file->read_long();
      if (!long2) {
	if (long1 < 0) {
	  tags[tag_name] = "-Inf";
	} else {
	  tags[tag_name] = "Inf";
	}
	continue;
      }
      switch(tag_format)
      {
      case "BIAS":
        if(long1>0)
          tags[tag_name] = sprintf("+%3.1f", long1*1.0/long2);
        else if(long1 < 0)
          tags[tag_name] = sprintf("-%3.1f", -long1*1.0/long2);
        else
          tags[tag_name] = "0.0";
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
    break;
  }

  file->seek(pos+4);
  return tags;
}

protected class TIFF
{
  protected int start, order;
  protected Stdio.BlockFile file;

  protected int read_number(int size)
  {
    string data = file->read(size);
    if(sizeof(data)!=size) error("End of file.\n");
    int ret;
    if(order)
      sscanf(data, "%-"+size+"c", ret);
    else
      sscanf(data, "%"+size+"c", ret);
    return ret;
  }

  int read_short() { return read_number(2); }
  int read_long() { return read_number(4); }

  protected void create(Stdio.BlockFile f)
  {
    file = f;
    start = f->tell();
    string o = f->read(2);

    switch(o)
    {
    case "MM": order = 0; break; // big endian
    case "II": order = 1; break; // little endian
    default: file = 0; return;
    }

    // TIFF magic number
    if(read_short() != 42)
      file = 0;
  }

  int is_valid() { return !!file; }

  string read(int len) { return file->read(len); }
  int tell() { return file->tell(); }
  void seek(int position) { file->seek(position); }

  void exif_seek(int position) { file->seek(start+position); }
}

protected int read_marker(Stdio.BlockFile f)
{
  string x;
  do {
    x = f->read(1);
    if(!sizeof(x)) return 0;
  } while( x == "\xff" );
  return x[0];
}

//! Parses and returns all EXIF properties.
//! @param file
//!   A JFIF file positioned at the start.
//! @param tags
//!   An optional list of tags to process. If given, all unknown tags
//!   will be ignored.
//! @returns
//!   A mapping with all found EXIF properties.
mapping(string:mixed) get_properties(Stdio.BlockFile file, void|mapping tags)
{
 loop: while(1)
  {
    int marker = read_marker(file);
    switch(marker)
    {
    case 0: // read_marker EOF
    case 0xd9: // End of Image
    case 0xda: // Start of Scan
      return ([]);
    case 0xd8: // Start of Image
      continue;
    default:
      int size;
      sscanf(file->read(2), "%2c", size);
      if (marker == 0xe1) {
        //  There could be other app sections tagged with 0xE1, not just EXIF
        if (file->read(6) == "Exif\0\0") {
          file = Stdio.FakeFile(file->read(size - 2 - 6));
          break loop;
        } else {
          //  Undo last read
          file->seek(file->tell() - 6);
        }
      }
      file->read(size-2);
      continue;
    }
  }

  int discard_unknown = 0;
  if( tags )
    discard_unknown = 1;
  else
    tags = TAG_INFO;

  return low_get_properties(file, tags, discard_unknown) || ([]);
}

protected int parse_ifd(TIFF tiff, mapping tags, mapping exif_info,
			int discard_unknown)
{
  // JEITA CP-3451 4.6.2:
  //   The IFD used in this standard consists of a 2-byte count
  //   (number of fields), 12-byte field Interoperability arrays,
  //   and 4-byte offset to the next IFD, in conformance with
  //   TIFF Rev. 6.0.
  int num_entries=tiff->read_short();
  if (!num_entries) return 0;

  for(int i=0; i<num_entries; i++)
    parse_tag(tiff, tags, exif_info, discard_unknown);

  return 1;
}

protected mapping|zero low_get_properties(Stdio.BlockFile file,
                                          mapping exif_info,
                                          int discard_unknown)
{
  TIFF tiff = TIFF(file);
  if(!tiff->is_valid()) return 0;

  mapping ret = ([]);
  multiset seent = (<>);

  int offset=tiff->read_long();
  seent[offset]=1;
  while(offset>0)
  {
    // Parse the IFD at offset.
    tiff->exif_seek(offset);

    if (!parse_ifd(tiff, ret, exif_info, discard_unknown)) {
      // Seen in the wild:
      //   IFD chain terminated by an empty IFD without next field at EOTIFF.
      break;
    }

    offset = tiff->read_long();

    if( seent[offset] ) break;	// Circular IFD loop.
    seent[offset]=1;
  }

  // There can be more info at the ExifOffset (aka Exif IFD).
  if (offset = (int)m_delete(ret, "ExifOffset")) {
    tiff->seek(offset);
    parse_ifd(tiff, ret, exif_info, discard_unknown);
  }

  return ret;
}
