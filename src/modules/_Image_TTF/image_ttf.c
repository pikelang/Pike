/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: image_ttf.c,v 1.51 2004/04/14 12:12:50 grubba Exp $
*/

#include "config.h"

#include "global.h"
RCSID("$Id: image_ttf.c,v 1.51 2004/04/14 12:12:50 grubba Exp $");
#include "module.h"

#ifdef HAVE_LIBTTF
#if defined(HAVE_FREETYPE_FREETYPE_H) && defined(HAVE_FREETYPE_FTXKERN_H)

#include <freetype/freetype.h>
#include <freetype/ftxkern.h>

#else /* !HAVE_FREETYPE_FREETYPE_H || !HAVE_FREETYPE_FTXKERN_H */

#include <freetype.h>
#include <ftxkern.h>

#endif /* HAVE_FREETYPE_FREETYPE_H && HAVE_FREETYPE_FTXKERN_H */
#endif /* HAVE_LIBTTF */

#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "threads.h"
#include "array.h"
#include "mapping.h"
#include "pike_error.h"
#include "stralloc.h"
#include "builtin_functions.h"
#include "operators.h"

#ifdef HAVE_LIBTTF
#include "../Image/image.h"


#ifdef DYNAMIC_MODULE
static struct program *image_program=NULL;
#else
extern struct program *image_program;
/* Image module is probably linked static too. */
#endif

static TT_Engine engine;

#endif /* HAVE_LIBTTF */


#define sp Pike_sp

static struct pike_string *param_baseline;
static struct pike_string *param_quality;

#ifdef HAVE_LIBTTF

/*
**! module Image
**! submodule TTF
**!
**!	This module adds TTF (Truetype font) capability
**!	to the Image module.
**!
**! note
**!	This module needs the <tt>libttf</tt> "Freetype" library
**!
*/

void my_tt_error(char *where,char *extra,int err)
{
   char *errc="Unknown";

   if (err==TT_Err_Invalid_Face_Handle)
      errc="TT_Err_Invalid_Face_Handle";
   else if (err==TT_Err_Invalid_Instance_Handle)
      errc="TT_Err_Invalid_Instance_Handle";
   else if (err==TT_Err_Invalid_Glyph_Handle)
      errc="TT_Err_Invalid_Glyph_Handle";
   else if (err==TT_Err_Invalid_CharMap_Handle)
      errc="TT_Err_Invalid_CharMap_Handle";
   else if (err==TT_Err_Invalid_Result_Address)
      errc="TT_Err_Invalid_Result_Address";
   else if (err==TT_Err_Invalid_Glyph_Index)
      errc="TT_Err_Invalid_Glyph_Index";
   else if (err==TT_Err_Invalid_Argument)
      errc="TT_Err_Invalid_Argument";
   else if (err==TT_Err_Could_Not_Open_File)
      errc="TT_Err_Could_Not_Open_File";
   else if (err==TT_Err_File_Is_Not_Collection)
      errc="TT_Err_File_Is_Not_Collection";
   else if (err==TT_Err_Table_Missing)
      errc="TT_Err_Table_Missing";
   else if (err==TT_Err_Invalid_Horiz_Metrics)
      errc="TT_Err_Invalid_Horiz_Metrics";
   else if (err==TT_Err_Invalid_CharMap_Format)
      errc="TT_Err_Invalid_CharMap_Format";
   else if (err==TT_Err_Invalid_PPem)
      errc="TT_Err_Invalid_PPem";
   else if (err==TT_Err_Invalid_File_Format)
      errc="TT_Err_Invalid_File_Format";
   else if (err==TT_Err_Invalid_Engine)
      errc="TT_Err_Invalid_Engine";
   else if (err==TT_Err_Too_Many_Extensions)
      errc="TT_Err_Too_Many_Extensions";
   else if (err==TT_Err_Extensions_Unsupported)
      errc="TT_Err_Extensions_Unsupported";
   else if (err==TT_Err_Invalid_Extension_Id)
      errc="TT_Err_Invalid_Extension_Id";
   else if (err==TT_Err_Max_Profile_Missing)
      errc="TT_Err_Max_Profile_Missing";
   else if (err==TT_Err_Header_Table_Missing)
      errc="TT_Err_Header_Table_Missing";
   else if (err==TT_Err_Horiz_Header_Missing)
      errc="TT_Err_Horiz_Header_Missing";
   else if (err==TT_Err_Locations_Missing)
      errc="TT_Err_Locations_Missing";
   else if (err==TT_Err_Name_Table_Missing)
      errc="TT_Err_Name_Table_Missing";
   else if (err==TT_Err_CMap_Table_Missing)
      errc="TT_Err_CMap_Table_Missing";
   else if (err==TT_Err_Hmtx_Table_Missing)
      errc="TT_Err_Hmtx_Table_Missing";
   else if (err==TT_Err_OS2_Table_Missing)
      errc="TT_Err_OS2_Table_Missing";
   else if (err==TT_Err_Post_Table_Missing)
      errc="TT_Err_Post_Table_Missing";
   else if (err==TT_Err_Out_Of_Memory)
      errc="TT_Err_Out_Of_Memory";
   else if (err==TT_Err_Invalid_File_Offset)
      errc="TT_Err_Invalid_File_Offset";
   else if (err==TT_Err_Invalid_File_Read)
      errc="TT_Err_Invalid_File_Read";
   else if (err==TT_Err_Invalid_Frame_Access)
      errc="TT_Err_Invalid_Frame_Access";
   else if (err==TT_Err_Too_Many_Points)
      errc="TT_Err_Too_Many_Points";
   else if (err==TT_Err_Too_Many_Contours)
      errc="TT_Err_Too_Many_Contours";
   else if (err==TT_Err_Invalid_Composite)
      errc="TT_Err_Invalid_Composite";
   else if (err==TT_Err_Too_Many_Ins)
      errc="TT_Err_Too_Many_Ins";
   else if (err==TT_Err_Invalid_Opcode)
      errc="TT_Err_Invalid_Opcode";
   else if (err==TT_Err_Too_Few_Arguments)
      errc="TT_Err_Too_Few_Arguments";
   else if (err==TT_Err_Stack_Overflow)
      errc="TT_Err_Stack_Overflow";
   else if (err==TT_Err_Code_Overflow)
      errc="TT_Err_Code_Overflow";
   else if (err==TT_Err_Bad_Argument)
      errc="TT_Err_Bad_Argument";
   else if (err==TT_Err_Divide_By_Zero)
      errc="TT_Err_Divide_By_Zero";
   else if (err==TT_Err_Storage_Overflow)
      errc="TT_Err_Storage_Overflow";
   else if (err==TT_Err_Cvt_Overflow)
      errc="TT_Err_Cvt_Overflow";
   else if (err==TT_Err_Invalid_Reference)
      errc="TT_Err_Invalid_Reference";
   else if (err==TT_Err_Invalid_Distance)
      errc="TT_Err_Invalid_Distance";
   else if (err==TT_Err_Interpolate_Twilight)
      errc="TT_Err_Interpolate_Twilight";
   else if (err==TT_Err_Debug_OpCode)
      errc="TT_Err_Debug_OpCode";
   else if (err==TT_Err_ENDF_In_Exec_Stream)
      errc="TT_Err_ENDF_In_Exec_Stream";
   else if (err==TT_Err_Out_Of_CodeRanges)
      errc="TT_Err_Out_Of_CodeRanges";
   else if (err==TT_Err_Nested_DEFS)
      errc="TT_Err_Nested_DEFS";
   else if (err==TT_Err_Invalid_CodeRange)
      errc="TT_Err_Invalid_CodeRange";
   else if (err==TT_Err_Invalid_Displacement)
      errc="TT_Err_Invalid_Displacement";
   else if (err==TT_Err_Nested_Frame_Access)
      errc="TT_Err_Nested_Frame_Access";
   else if (err==TT_Err_Invalid_Cache_List)
      errc="TT_Err_Invalid_Cache_List";
   else if (err==TT_Err_Could_Not_Find_Context)
      errc="TT_Err_Could_Not_Find_Context";
   else if (err==TT_Err_Unlisted_Object)
      errc="TT_Err_Unlisted_Object";
   else if (err==TT_Err_Raster_Pool_Overflow)
      errc="TT_Err_Raster_Pool_Overflow";
   else if (err==TT_Err_Raster_Negative_Height)
      errc="TT_Err_Raster_Negative_Height";
   else if (err==TT_Err_Raster_Invalid_Value)
      errc="TT_Err_Raster_Invalid_Value";
   else if (err==TT_Err_Raster_Not_Initialized)
      errc="TT_Err_Raster_Not_Initialized";
   Pike_error("%s: %sFreeType error 0x%03x (%s)\n",
	 where,extra,err,errc);
}

/*
**! method object `()(string filename)
**! method object `()(string filename,mapping options)
**!
**!	Makes a new TTF Face object.
**!
**! returns 0 if failed.
**!
**! arg string filename
**!	The filename of the <tt>TTF</tt> font or the <tt>TTC</tt>
**!	font collection.
**!
**! arg mapping options
**!	<pre>
**!	advanced options:
**!	    "face":int
**!             If opening a font collection,  '<tt>.TTC</tt>',
**!		this is used to get other fonts than the first.
**!	</pre>
**!
*/

struct image_ttf_face_struct
{
   TT_Face face;
};

struct image_ttf_faceinstance_struct
{
   TT_Instance instance;
   struct object *faceobj;
   int load_flags;
   int baseline,height;  /* "baseline" position */
   int trans;            /* translation of position (ypos) in 26.6*/
};

static struct program *image_ttf_face_program=NULL;
static struct program *image_ttf_faceinstance_program=NULL;

static void image_ttf_make(INT32 args)
{
   int col=0, i=0;
   struct object *o;
   TT_Error res;
   TT_Face face;

   if (sp[-args].type!=T_STRING)
      Pike_error("Image.TTF(): illegal argument 1\n");

   res=TT_Open_Collection(engine, sp[-args].u.string->str, col, &face);
   if (res) my_tt_error("Image.TTF()","",res);
   while(! TT_Load_Kerning_Table( face, (TT_UShort)(i++) ) );

   pop_n_elems(args);

   o=clone_object(image_ttf_face_program,0);
   ((struct image_ttf_face_struct*)o->storage)->face=face;

   push_object(o);
}

/*
**! class Face
**!
**!	This represents instances of TTF Faces.
*/

#define THISOBJ (Pike_fp->current_object)
#define THISf ((struct image_ttf_face_struct*)get_storage(THISOBJ,image_ttf_face_program))
#define THISi ((struct image_ttf_faceinstance_struct*)get_storage(THISOBJ,image_ttf_faceinstance_program))

static void image_ttf_face_exit(struct object *o)
{
  if(THISf)
    TT_Close_Face(THISf->face);
}

static void image_ttf_faceinstance_exit(struct object *o)
{
  if (THISi) {
    if(THISi->faceobj->prog)
      TT_Done_Instance(THISi->instance);
    free_object(THISi->faceobj);
  }
}

#ifdef TTF_DEBUG_INFO
/*
**! method mapping properties()
**!     This gives back a structure of the face's properties.
**!	Most of this stuff is information you can skip.
**!
**!	The most interesting item to look at may be
**!	<tt>->num_Faces</tt>, which describes the number of faces
**!	in a <tt>.TTC</tt> font collection.
**!
**! returns a mapping of a lot of properties
*/

static void image_ttf_face_properties(INT32 args)
{
   int res;
   TT_Face_Properties prop;

   pop_n_elems(args);

   res=TT_Get_Face_Properties(THISf->face,&prop);
   if (res) my_tt_error("Image.TTF.Face->properties()","",res);

   push_text("num_Glyphs");   push_int(prop.num_Glyphs);
   push_text("max_Points");   push_int(prop.max_Points);
   push_text("max_Contours");   push_int(prop.max_Contours);
   push_text("num_Faces");   push_int(prop.num_Faces);

   push_text("header");
   if (prop.header)
   {
      push_text("Table_Version"); push_int(prop.header->Table_Version);
      push_text("Font_Revision"); push_int(prop.header->Font_Revision);
      push_text("CheckSum_Adjust"); push_int(prop.header->CheckSum_Adjust);
      push_text("Magic_Number"); push_int(prop.header->Magic_Number);
      push_text("Flags"); push_int(prop.header->Flags);
      push_text("Units_Per_EM"); push_int(prop.header->Units_Per_EM);
      push_text("Created");
      push_int(prop.header->Created[0]);
      push_int(prop.header->Created[1]);
      f_aggregate(2);
      push_text("Modified");
      push_int(prop.header->Modified[0]);
      push_int(prop.header->Modified[1]);
      f_aggregate(2);
      push_text("xMin"); push_int(prop.header->xMin);
      push_text("yMin"); push_int(prop.header->yMin);
      push_text("xMax"); push_int(prop.header->xMax);
      push_text("yMax"); push_int(prop.header->yMax);
      push_text("Mac_Style"); push_int(prop.header->Mac_Style);
      push_text("Lowest_Rec_PPEM"); push_int(prop.header->Lowest_Rec_PPEM);
      push_text("Font_Direction"); push_int(prop.header->Font_Direction);
      push_text("Index_To_Loc_Format");
      push_int(prop.header->Index_To_Loc_Format);
      push_text("Glyph_Data_Format");
      push_int(prop.header->Glyph_Data_Format);
      f_aggregate_mapping(17*2);
   }
   else push_int(0);

   push_text("horizontal");
   if (prop.horizontal)
   {
      push_text("Version"); push_int(prop.horizontal->Version);
      push_text("Ascender"); push_int(prop.horizontal->Ascender);
      push_text("Descender"); push_int(prop.horizontal->Descender);
      push_text("Line_Gap"); push_int(prop.horizontal->Line_Gap);
      push_text("advance_Width_Max"); push_int(prop.horizontal->advance_Width_Max);
      push_text("min_Left_Side_Bearing"); push_int(prop.horizontal->min_Left_Side_Bearing);
      push_text("min_Right_Side_Bearing"); push_int(prop.horizontal->min_Right_Side_Bearing);
      push_text("xMax_Extent"); push_int(prop.horizontal->xMax_Extent);
      push_text("caret_Slope_Rise"); push_int(prop.horizontal->caret_Slope_Rise);
      push_text("caret_Slope_Run"); push_int(prop.horizontal->caret_Slope_Run);
      push_text("metric_Data_Format"); push_int(prop.horizontal->metric_Data_Format);
      push_text("number_Of_HMetrics"); push_int(prop.horizontal->number_Of_HMetrics);
      f_aggregate_mapping(13*2);
   }
   else push_int(0);

   push_text("os2");
   if (prop.os2)
   {
      push_text("version"); push_int(prop.os2->version);
      push_text("xAvgCharWidth"); push_int(prop.os2->xAvgCharWidth);
      push_text("usWeightClass"); push_int(prop.os2->usWeightClass);
      push_text("usWidthClass"); push_int(prop.os2->usWidthClass);
      push_text("fsType"); push_int(prop.os2->fsType);
      push_text("ySubscriptXSize"); push_int(prop.os2->ySubscriptXSize);
      push_text("ySubscriptYSize"); push_int(prop.os2->ySubscriptYSize);
      push_text("ySubscriptXOffset"); push_int(prop.os2->ySubscriptXOffset);
      push_text("ySubscriptYOffset"); push_int(prop.os2->ySubscriptYOffset);
      push_text("ySuperscriptXSize"); push_int(prop.os2->ySuperscriptXSize);
      push_text("ySuperscriptYSize"); push_int(prop.os2->ySuperscriptYSize);
      push_text("ySuperscriptXOffset"); push_int(prop.os2->ySuperscriptXOffset);
      push_text("ySuperscriptYOffset"); push_int(prop.os2->ySuperscriptYOffset);
      push_text("yStrikeoutSize"); push_int(prop.os2->yStrikeoutSize);
      push_text("yStrikeoutPosition"); push_int(prop.os2->yStrikeoutPosition);
      push_text("sFamilyClass"); push_int(prop.os2->sFamilyClass);

      push_text("panose");
      push_string(make_shared_binary_string(prop.os2->panose,10));

      push_text("ulUnicodeRange1"); push_int(prop.os2->ulUnicodeRange1);
      push_text("ulUnicodeRange2"); push_int(prop.os2->ulUnicodeRange2);
      push_text("ulUnicodeRange3"); push_int(prop.os2->ulUnicodeRange3);
      push_text("ulUnicodeRange4"); push_int(prop.os2->ulUnicodeRange4);

      push_text("achVendID");
      push_string(make_shared_binary_string(prop.os2->achVendID,4));

      push_text("fsSelection"); push_int(prop.os2->fsSelection);
      push_text("usFirstCharIndex"); push_int(prop.os2->usFirstCharIndex);
      push_text("usLastCharIndex"); push_int(prop.os2->usLastCharIndex);
      push_text("sTypoAscender"); push_int(prop.os2->sTypoAscender);
      push_text("sTypoDescender"); push_int(prop.os2->sTypoDescender);
      push_text("sTypoLineGap"); push_int(prop.os2->sTypoLineGap);
      push_text("usWinAscent"); push_int(prop.os2->usWinAscent);
      push_text("usWinDescent"); push_int(prop.os2->usWinDescent);
      push_text("ulCodePageRange1"); push_int(prop.os2->ulCodePageRange1);
      push_text("ulCodePageRange2"); push_int(prop.os2->ulCodePageRange2);

      f_aggregate_mapping(32*2);
   }
   else push_int(0);

   push_text("postscript");
   if (prop.postscript)
   {
      push_text("FormatType"); push_int(prop.postscript->FormatType);
      push_text("italicAngle"); push_int(prop.postscript->italicAngle);
      push_text("underlinePosition"); push_int(prop.postscript->underlinePosition);
      push_text("underlineThickness"); push_int(prop.postscript->underlineThickness);
      push_text("isFixedPitch"); push_int(prop.postscript->isFixedPitch);
      push_text("minMemType42"); push_int(prop.postscript->minMemType42);
      push_text("maxMemType42"); push_int(prop.postscript->maxMemType42);
      push_text("minMemType1"); push_int(prop.postscript->minMemType1);
      push_text("maxMemType1"); push_int(prop.postscript->maxMemType1);
      f_aggregate_mapping(9*2);
   }
   else push_int(0);

   push_text("hdmx");
   if (prop.hdmx)
   {
      int i;

      push_text("version"); push_int(prop.hdmx->version);
      push_text("num_records"); push_int(prop.hdmx->num_records);
      push_text("records");

      for (i=0; i<prop.hdmx->num_records; i++)
      {
	 push_text("ppem"); push_int(prop.hdmx->records[i].ppem);
	 push_text("max_width"); push_int(prop.hdmx->records[i].max_width);
	 /*	 push_text("widths"); push_int(prop.hdmx->records[i].widths);*/
	 f_aggregate_mapping(2*2);
      }
      f_aggregate(prop.hdmx->num_records);

      f_aggregate_mapping(3*2);
   }
   else push_int(0);

   f_aggregate_mapping(9*2);
}
#endif /* TTF_DEBUG_INFO */

/*
**! method object flush()
**!     This flushes all cached information.
**!	Might be used to save memory - the face
**!	information is read back from disk upon need.
**!
**! returns the object being called
*/

static void image_ttf_face_flush(INT32 args)
{
   int res;
   pop_n_elems(args);

   if ((res=TT_Flush_Face(THISf->face)))
      my_tt_error("Image.TTF.Face->flush()","",res);

   ref_push_object(THISOBJ);
}

/*
**! method mapping names()
**! method array(array) _names()
**!     Gives back the names or the complete name-list
**!	of this face.
**!
**!     The result from <ref>names</ref>() is a mapping,
**!	which has any or all of these indices:
**!	<pre>
**!	"copyright":   ("Copyright the Foo Corporation...bla bla")
**!	"family":      ("My Little Font")
**!	"style":       ("Bold")
**!	"full":	       ("Foo: My Little Font: 1998")
**!	"expose":      ("My Little Font Bold")
**!	"version":     ("June 1, 1998; 1.00, ...")
**!	"postscript":  ("MyLittleFont-Bold")
**!	"trademark":   ("MyLittleFont is a registered...bla bla")
**!	</pre>
**!
**!	This is extracted from the information from _names(),
**!	and fit into pike-strings using unicode or iso-8859-1,
**!	if possible.
**!
**!     The result from <ref>_names</ref>() is a
**!	matrix, on this form: <pre>
**!	     ({ ({ int platform, encoding, language, id, string name }),
**!	        ({ int platform, encoding, language, id, string name }),
**!             ...
**!	     })
**!     </pre>
**!
**! returns the name as a mapping to string or the names as a matrix
**!
**! note:
**!	To use the values from <ref>_names</ref>(),
**!	check the TrueType standard documentation.
*/

static void image_ttf_face__names(INT32 args)
{
   int ns,res;
   TT_UShort i;
   TT_Face face=THISf->face;
   pop_n_elems(args);

   if ((ns=TT_Get_Name_Count(face))==-1)
      Pike_error("Image.TTF.Face->names(): Illegal face handler\n");

   for (i=0; i<ns; i++)
   {
      unsigned short platformID,encodingID,languageID,nameID;
      TT_UShort length;
      char *stringPtr;

      if ((res=TT_Get_Name_ID(face,i,
			      &platformID,&encodingID,&languageID,&nameID)))
	 my_tt_error("Image.TTF.Face->names()","TT_Get_Name_ID: ",res);

      push_int(platformID);
      push_int(encodingID);
      push_int(languageID);
      push_int(nameID);

      if ((res=TT_Get_Name_String(face,i,&stringPtr,&length)))
	 my_tt_error("Image.TTF.Face->names()","TT_Get_Name_String: ",res);

      push_string(make_shared_binary_string(stringPtr,length));

      f_aggregate(5);
   }
   f_aggregate(ns);
}

static void image_ttf_face_names(INT32 args)
{
   int n,i;
   int has[8]={0,0,0,0,0,0,0,0}; /* iso8859=20, unicode=30, any=1 */
   char *hasname[8]={"copyright","family","style","full",
		     "expose","version","postscript","trademark"};
   struct array *a,*b;

   image_ttf_face__names(args);

   if (sp[-1].type!=T_ARRAY)
      Pike_error("Image.TTF.Face->names(): internal error, weird _names()\n");

   a=sp[-1].u.array;

   n=0;
   for (i=0; i<a->size; i++)
   {
      int ihas=1;
      int what;
      b=a->item[i].u.array;

      what=b->item[3].u.integer;
      if (what>=8 || what<0) continue; /* weird */
      switch (b->item[0].u.integer*100+b->item[1].u.integer)
      {
	 case 301: /* M$:  unicode */
	 case 300: /* M$:  unicode (?) */
	    ihas=30;
	    break;
	 case 202: /* ISO: iso-8859-1 */
	    ihas=20;
	    break;
      }
      if (ihas<has[what]) continue; /* worse */

      push_text(hasname[what]);

      if (ihas==30) /* unicode, M$ but weird enough correct byteorder */
      {
	 ptrdiff_t n = b->item[4].u.string->len/2;
	 struct pike_string *ps=begin_wide_shared_string(n,1);
	 p_wchar1 *d=STR1(ps);
	 p_wchar0 *s=STR0(b->item[4].u.string);
	 while (n--) *(d++)=((p_wchar1)s[0]<<8)|(p_wchar1)s[1],s+=2;
	 push_string(end_shared_string(ps));
      }
      else
	 push_svalue(b->item+4);

      n++;
   }
   f_aggregate_mapping(n*2);
   stack_swap();
   pop_stack();
}

/*
**! method object `()()
**!     This instantiates the face for normal usage -
**!	to convert font data to images.
**!
**! returns a <ref>Image.TTF.FaceInstance</ref> object.
*/

static void image_ttf_face_make(INT32 args)
{
   pop_n_elems(args);

   ref_push_object(THISOBJ);
   push_object(clone_object(image_ttf_faceinstance_program,1));
}

/*
**! class FaceInstance
**!     This is the instance of a face, with geometrics,
**!	encodings and stuff.
**!
**! method void create(object face)
**!	creates a new Instance from a face.
*/

static void ttf_instance_setc(struct image_ttf_face_struct *face_s,
			      struct image_ttf_faceinstance_struct *face_i,
			      int towhat,
			      char *where)
{
   TT_Face_Properties prop;
   TT_Instance_Metrics metr;
   int res;
   int resol;

   if ((res=TT_Get_Face_Properties(face_s->face,&prop)))
      my_tt_error(where,"TT_Get_Face_Properties",res);

   resol=58; /* should be 72, but glyphs fit using this value */
      /*
         (int)((72*(prop.horizontal->Ascender+
		    prop.horizontal->Descender)/
		(float)prop.horizontal->Ascender));
*/

   if ((res=TT_Set_Instance_Resolutions(face_i->instance,
					(TT_UShort)resol, (TT_UShort)resol)))
      my_tt_error("Image.TTF.FaceInstance()",
		  "TT_Set_Instance_Resolutions: ",res);

   if ((res=TT_Get_Instance_Metrics(face_i->instance,&metr)))
      my_tt_error(where,"TT_Get_Instance_Metrics",res);

   if ((res=TT_Set_Instance_CharSize(face_i->instance,towhat)))
      my_tt_error(where,"TT_Set_Instance_CharSize: ",res);

   face_i->baseline=
      DOUBLE_TO_INT(((double)(towhat/64.0+towhat/640.0)*
		     prop.horizontal->Ascender)/
		    (prop.horizontal->Ascender - prop.horizontal->Descender));

   face_i->height= (towhat/64 + towhat/640);

   face_i->trans = ~63 &
      (32 +
       DOUBLE_TO_INT(64*((towhat/64.0+towhat/640.0)*
			 prop.horizontal->Ascender)/
		     (prop.horizontal->Ascender-prop.horizontal->Descender)));
}

static void image_ttf_faceinstance_create(INT32 args)
{
   struct image_ttf_face_struct *face_s = NULL;
   struct image_ttf_faceinstance_struct *face_i=THISi;
   int res;

   if (!args)
      Pike_error("Image.TTF.FaceInstance(): too few arguments\n");

   if (sp[-args].type!=T_OBJECT ||
       !(face_s=(struct image_ttf_face_struct*)
	 get_storage(sp[-args].u.object,image_ttf_face_program)))
      Pike_error("Image.TTF.FaceInstance(): illegal argument 1\n");

   if ((res=TT_New_Instance(face_s->face,&(face_i->instance))))
      my_tt_error("Image.TTF.FaceInstance()","TT_New_Instance: ",res);

   face_i->load_flags = TTLOAD_SCALE_GLYPH|TTLOAD_HINT_GLYPH;
   add_ref(face_i->faceobj=sp[-args].u.object);

   ttf_instance_setc(face_s,face_i,32*64,"Image.TTF.FaceInstance()");
}

static void image_ttf_faceinstance_set_height(INT32 args)
{
   struct image_ttf_face_struct *face_s;
   struct image_ttf_faceinstance_struct *face_i=THISi;
   int h=0;

   if (!args)
      Pike_error("Image.TTF.FaceInstance->set_height(): missing arguments\n");

   if (sp[-args].type==T_INT)
      h = sp[-args].u.integer*64;
   else if (sp[-args].type==T_FLOAT)
      h = DOUBLE_TO_INT(sp[-args].u.float_number*64);
   else
      Pike_error("Image.TTF.FaceInstance->set_height(): illegal argument 1\n");
   if (h<1) h=1;

   if (!(face_s=(struct image_ttf_face_struct*)
	 get_storage(THISi->faceobj,image_ttf_face_program)))
      Pike_error("Image.TTF.FaceInstance->write(): lost Face\n");

   ttf_instance_setc(face_s,face_i,h,"Image.TTF.FaceInstance->set_height()");

   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

static void ttf_translate_8bit(TT_CharMap charMap,
			       unsigned char *what,
			       int **dest,
			       int len,
			       int base)
{
   int i;

   dest[0]=(int*)xalloc(len*sizeof(int));

   THREADS_ALLOW();
   for (i=0; i<len; i++)
      dest[0][i]=TT_Char_Index(charMap, (TT_UShort)(what[i]+base));
   THREADS_DISALLOW();
}


static void ttf_translate_16bit(TT_CharMap charMap,
				unsigned short *what,
				int **dest,
				int len,
				int base)
{
   int i;

   dest[0]=(int*)xalloc(len*sizeof(int));

   THREADS_ALLOW();
   for (i=0; i<len; i++)
      dest[0][i] = TT_Char_Index(charMap, (TT_UShort)(what[i] + base));
   THREADS_DISALLOW();
}

static void ttf_get_nice_charmap(TT_Face face,
				 TT_CharMap *charMap,
				 char *where)
{
   int n,i,res,got=-1,best=-1;
   if (-1==(n=TT_Get_CharMap_Count(face)))
      Pike_error("%s: illegal face handle\n",where);

   for (i=0; i<n; i++)
   {
      int ihas=0;
      TT_UShort platformID, encodingID;

      if ((res=TT_Get_CharMap_ID(face, (TT_UShort)i,
				 &platformID, &encodingID)))
	 my_tt_error(where,"TT_Get_CharMap_ID: ",res);

      switch (platformID*100+encodingID)
      {
	 case 301: /* M$:  unicode */
	 case 300: /* M$:  unicode (?) */
	    ihas=30;
	    break;
	 case 202: /* ISO: iso-8859-1 */
	    ihas=20;
	    break;
       default:
            ihas = 1;
	    break;
      }

      if (ihas>got)
      {
	 best=i;
	 got=ihas;
      }
   }
   if (got==-1)
      Pike_error("%s: no charmaps at all\n",where);

   if ((res=TT_Get_CharMap(face, (TT_UShort)best, charMap)))
      my_tt_error(where,"TT_Get_CharMap: ",res);
}

static void ttf_please_translate_8bit(TT_Face face,
				      struct pike_string *s,
				      int **dest,int *dlen,
				      int base,
				      char *where)
{
   TT_CharMap charMap;

   ttf_get_nice_charmap(face,&charMap,where);
   (*dlen) = DO_NOT_WARN(s->len);
   ttf_translate_8bit(charMap,(unsigned char*)(s->str),dest,*dlen,base);
}

static void ttf_please_translate_16bit(TT_Face face,
				      struct pike_string *s,
				      int **dest,int *dlen,
				      int base,
				      char *where)
{
   TT_CharMap charMap;

   ttf_get_nice_charmap(face,&charMap,where);
   (*dlen) = DO_NOT_WARN(s->len);
   ttf_translate_16bit(charMap,(unsigned short*)(s->str),dest,*dlen,base);
}


static void image_ttf_faceinstance_ponder(INT32 args)
{
   int *sstr;
   int len,i,res,base=0;
   struct image_ttf_face_struct *face_s;
   struct image_ttf_faceinstance_struct *face_i=THISi;

   int xmin=1000,xmax=-1000,pos=0;

   if (!(face_s=(struct image_ttf_face_struct*)
	 get_storage(THISi->faceobj,image_ttf_face_program)))
      Pike_error("Image.TTF.FaceInstance->ponder(): lost Face\n");

   if (args && sp[-1].type==T_INT)
   {
      base=sp[-1].u.integer;
      args--;
      pop_stack();
   }

   if (sp[-args].type!=T_STRING)
      Pike_error("Image.TTF.FaceInstance->ponder(): illegal argument 1\n");

   switch( sp[-args].u.string->size_shift )
   {
    case 0:
      ttf_please_translate_8bit(face_s->face,
                                sp[-args].u.string,&sstr,&len,base,
                                "Image.TTF.FaceInstance->ponder()");
      break;
    case 1:
      ttf_please_translate_16bit(face_s->face,
                                 sp[-args].u.string,&sstr,&len,base,
                                 "Image.TTF.FaceInstance->ponder()");
      break;
    default:
     Pike_error("Too wide string for truetype\n");
   }
   pop_n_elems(args);

   for (i=0; i<len; i++)
   {
      TT_Glyph glyph;
      TT_Glyph_Metrics metrics;
      int ind;

      ind=sstr[i];
/*       fprintf(stderr,"glyph: %d\n",ind); */

      if ((res=TT_New_Glyph(face_s->face,&glyph)))
	 my_tt_error("Image.TTF.FaceInstance->ponder()","TT_New_Glyph: ",res);

      if ((res=TT_Load_Glyph(face_i->instance, glyph,
			     (TT_UShort)ind, (TT_UShort)face_i->load_flags)))
	 my_tt_error("Image.TTF.FaceInstance->ponder()","TT_Load_Glyph: ",res);

      if ((res=TT_Get_Glyph_Metrics(glyph,&metrics)))
	 my_tt_error("Image.TTF.FaceInstance->ponder()",
		     "TT_Get_Glyph_Metrics: ",res);

      if (pos+metrics.bbox.xMin<xmin) xmin=pos+metrics.bbox.xMin;
      if (pos+metrics.bbox.xMax>xmax) xmax=pos+metrics.bbox.xMax;
      pos+=metrics.advance; /* insert kerning stuff here */

/*       fprintf(stderr,"bbox: (%f,%f)-(%f,%f)\n", */
/* 	      metrics.bbox.xMin/64.0, */
/* 	      metrics.bbox.yMin/64.0, */
/* 	      metrics.bbox.xMax/64.0, */
/* 	      metrics.bbox.yMax/64.0); */

/*       fprintf(stderr,"BearingX: %f\n",metrics.bearingX/64.0); */
/*       fprintf(stderr,"BearingY: %f\n",metrics.bearingY/64.0); */
/*       fprintf(stderr,"advance: %f\n",metrics.advance/64.0); */

/*       fprintf(stderr,"\n"); */
   }

   free(sstr);

/*    fprintf(stderr,"xmin: %f\n",xmin/64.0); */
/*    fprintf(stderr,"xmax: %f\n",xmax/64.0); */

   ref_push_object(THISOBJ);
}

static int find_kerning( TT_Kerning kerning, int c1, int c2 )
{
  int j;
/*   fprintf(stderr, "kern: %d %d\n", c1, c2); */
  for(j=0; j<kerning.nTables; j++)
  {
    if( (kerning.tables[j].coverage & 0x0f) != 1 ) /* We don't want it  */
      continue;
    switch(kerning.tables[j].format)
    {
     case 2: /* The smart one */
       {
	 int ind;
	 int lclass=0, rclass=0;
	 TT_Kern_2 table = kerning.tables[j].t.kern2;
	 ind = c1 - table.leftClass.firstGlyph;
	 if(ind >= table.leftClass.nGlyphs || ind < 0)
	   continue;
	 lclass = table.leftClass.classes[ ind ];
	 ind = c2 - table.rightClass.firstGlyph;
	 if(ind >= table.rightClass.nGlyphs || ind < 0)
	   continue;
	 rclass = table.rightClass.classes[ ind ];

	 return table.array[ rclass+lclass ];
       }
     case 0: /* The microsoft one */
       {
	 TT_Kern_0 table = kerning.tables[j].t.kern0;
	 int a=0,b=table.nPairs-1;

	 while (a<b)
	 {
	    int i=(a+b)/2;
	    if(table.pairs[i].left == c1)
	    {
	       if(table.pairs[i].right == c2)
	       {
		  /* fprintf(stderr, "found kerning\n"); */
		  return table.pairs[i].value;
	       }
	       else if(table.pairs[i].right < c2)
		  a=i+1;
	       else
		  b=i-1;
	    } else if(table.pairs[i].left < c1)
	       a=i+1;
	    else
	       b=i-1;
	 }
       }
       break;
     default:
       fprintf(stderr, "Warning: Unknown kerning table format %d\n",
	       kerning.tables[j].format);
    }
  }
  return 0;
}

static void image_ttf_faceinstance_write(INT32 args)
{
   int **sstr;
   int *slen;
   int i,res=0,base=0,a;
   struct image_ttf_face_struct *face_s;
   struct image_ttf_faceinstance_struct *face_i=THISi;
   TT_CharMap charMap;
   TT_Kerning kerning;
   int has_kerning = 0;
   char *errs=NULL;
   int scalefactor=0;
   int xmin=1000,xmax=-1000,pos=0,ypos;
   int width,height,mod;

   unsigned char* pixmap;
   int maxcharwidth = 0;

   if (!(face_s=(struct image_ttf_face_struct*)
	 get_storage(THISi->faceobj,image_ttf_face_program)))
      Pike_error("Image.TTF.FaceInstance->write(): lost Face\n");

   if(!TT_Get_Kerning_Directory( face_s->face, &kerning ))
   {
     TT_Instance_Metrics metrics;
/*      fprintf(stderr, "has kerning!\n"); */
     has_kerning = 1;
     if(TT_Get_Instance_Metrics( face_i->instance, &metrics ))
       Pike_error("Nope. No way.\n");
     scalefactor = metrics.x_scale;
/*      fprintf(stderr, "offset=%d\n", (int)metrics.x_scale); */
   }

   if (args && sp[-1].type==T_INT)
   {
      base=sp[-1].u.integer;
      args--;
      pop_stack();
   }

   if (!args)
   {
      push_text("");
      args=1;
   }

   ttf_get_nice_charmap(face_s->face,&charMap,
			"Image.TTF.FaceInstance->write()");

   sstr=alloca(args*sizeof(int*));
   slen=alloca(args*sizeof(int));

   /* first pass: figure out total bounding box */

   for (a=0; a<args; a++)
   {
     char *errs=NULL;
     TT_Glyph_Metrics metrics;

      if (sp[a-args].type!=T_STRING)
	 Pike_error("Image.TTF.FaceInstance->write(): illegal argument %d\n",a+1);

      switch(sp[a-args].u.string->size_shift)
      {
       case 0:
	 ttf_translate_8bit(charMap,(unsigned char*)sp[a-args].u.string->str,
			    sstr+a,
			    DO_NOT_WARN(slen[a]=sp[a-args].u.string->len),
			    base);
	 break;
       case 1:
	 ttf_translate_16bit(charMap,(unsigned short*)sp[a-args].u.string->str,
			     sstr+a,
			     DO_NOT_WARN(slen[a]=sp[a-args].u.string->len),
			     base);
	 break;
       case 2:
         free( sstr );
         free( slen );
	 Pike_error("Too wide string for truetype\n");
	 break;
      }

      pos=0;
      for (i=0; i<slen[a]; i++)
      {
	 TT_Glyph glyph;
	 int ind;

	 ind=sstr[a][i];
/* 	 fprintf(stderr,"glyph: %d\n",ind); */

	 if ((res=TT_New_Glyph(face_s->face,&glyph)))
	    { errs="TT_New_Glyph: "; break; }

	 if ((res=TT_Load_Glyph(face_i->instance, glyph, (TT_UShort)ind,
				(TT_UShort)face_i->load_flags)))
	    { errs="TT_Load_Glyph: "; break; }

	 if ((res=TT_Get_Glyph_Metrics(glyph,&metrics)))
	    { errs="TT_Get_Glyph_Metrics: "; break; }

	 if (pos+metrics.bbox.xMin<xmin)
	   xmin=pos+metrics.bbox.xMin;
	 if (pos+metrics.bbox.xMax>xmax)
	   xmax=pos+metrics.bbox.xMax;

	 if((metrics.bbox.xMax-(metrics.bbox.xMin<0?metrics.bbox.xMin:0))
	    >maxcharwidth)
	   maxcharwidth =
	     (metrics.bbox.xMax-(metrics.bbox.xMin<0?metrics.bbox.xMin:0));

	 pos+=metrics.advance;
	 if(has_kerning && i<slen[a]-1)
	 {
	   int kern = find_kerning( kerning, ind, sstr[a][i+1] );
	   pos += DOUBLE_TO_INT(kern * (scalefactor/65535.0));
	 }
	 if ((res=TT_Done_Glyph(glyph)))
	    { errs="TT_Done_Glyph: "; break; }
      }
      pos -= metrics.advance;
      pos += metrics.bbox.xMax-metrics.bbox.xMin;
      if (pos>xmax)
	xmax=pos;
      if (errs)
      {
	 for (i=0; i<a; i++) free(sstr[i]);
	 my_tt_error("Image.TTF.FaceInstance->write()",errs,res);
      }
   }

   pop_n_elems(args);

/*    fprintf(stderr,"xmin=%f xmax=%f\n",xmin/64.0,xmax/64.0); */

   xmin&=~63;
   width=((xmax-xmin+63)>>6)+4;
   height=face_i->height*args;
   mod=(4-(maxcharwidth&3))&3;
   if (width<1) width=1;

   if ((pixmap=malloc((maxcharwidth+mod)*face_i->height)))
   {
      /* second pass: write the stuff */

      TT_Raster_Map rastermap;
      struct object *o;
      struct image *img;
      rgb_group *d;


      rastermap.rows=face_i->height;
      rastermap.cols=rastermap.width=maxcharwidth+mod;
      rastermap.flow=TT_Flow_Down;
      rastermap.bitmap=pixmap;
      rastermap.size=rastermap.cols*rastermap.rows;

      ypos=0;

/*       fprintf(stderr,"rastermap.rows=%d cols=%d width=%d\n", */
/* 	      rastermap.rows,rastermap.cols,rastermap.width); */


      push_int(width);
      push_int(height);
      o=clone_object(image_program,2);
      img=(struct image*)get_storage(o,image_program);
      d=img->img;

      for (a=0; a<args; a++)
      {
         pos=-xmin;
	 for (i=0; i<slen[a]; i++)
	 {
    	    int sw, xp;
	    TT_Glyph glyph;
	    TT_Glyph_Metrics metrics;
	    int ind, x, y;

	    ind=sstr[a][i];
/* 	    fprintf(stderr,"glyph: %d\n",ind); */


	    if ((res=TT_New_Glyph(face_s->face,&glyph)))
	       { errs="TT_New_Glyph: "; break; }

	    if ((res=TT_Load_Glyph(face_i->instance, glyph, (TT_UShort)ind,
				   (TT_UShort)face_i->load_flags)))
	       { errs="TT_Load_Glyph: "; break; }

	    if ((res=TT_Get_Glyph_Metrics(glyph,&metrics)))
	       { errs="TT_Get_Glyph_Metrics: "; break; }

	    MEMSET(pixmap,0,rastermap.size);

	    if ((res=TT_Get_Glyph_Pixmap(glyph,
					 &rastermap,
					 -metrics.bbox.xMin
                                         /*+pos%64*/,
					 face_i->height*64-
					 face_i->trans)))
	       { errs="TT_Get_Glyph_Pixmap: "; break; }


	    sw = metrics.bbox.xMax-(metrics.bbox.xMin<0?metrics.bbox.xMin:0);
	    /* Copy source pixmap to destination image object. */
	    for(y=0; y<face_i->height; y++)
	    {
	      unsigned int s;
	      unsigned char * source = pixmap+rastermap.width*y;
	      rgb_group *dt=d+(ypos+y)*width+(xp=(metrics.bbox.xMin+pos)/64);

	      for(x=0; x<sw && xp<width; x++,xp++,source++,dt++)
		if(xp<0 || !(s = *source))
		  continue;
		else if((s=dt->r+s) < 256)
		  dt->r=dt->g=dt->b=s;
		else
		  dt->r=dt->g=dt->b=255;
	    }

	    pos+=metrics.advance;
/* 	    if(metrics.bbox.xMin < 0) */
/* 	      pos += metrics.bbox.xMin; */
	    if(has_kerning && i<slen[a]-1)
	    {
	      int kern = find_kerning( kerning, sstr[a][i], sstr[a][i+1] );
	      pos += DOUBLE_TO_INT(kern * (scalefactor/65535.0));
/* 	      fprintf(stderr, "Adjusted is %d\n", */
/* 		      (int)(kern * (scalefactor/65535.0))); */
	    }

	    if ((res=TT_Done_Glyph(glyph)))
	       { errs="TT_Done_Glyph: "; break; }
	 }
	 if (errs)
	 {
	    for (a=0; a<args; a++) free(sstr[a]);
	    free(pixmap);
	    free_object(o);
	    my_tt_error("Image.TTF.FaceInstance->write()",errs,res);
	 }
	 ypos+=face_i->height;
      }
      free(pixmap);
      push_object(o);
   }
   else
   {
      Pike_error("Image.TTF.FaceInstance->write(): out of memory\n");
   }

   for (a=0; a<args; a++)
      free(sstr[a]);
}

static void image_ttf_faceinstance_face(INT32 args)
{
   pop_n_elems(args);
   ref_push_object(THISi->faceobj);
}

#endif /* HAVE_LIBTTF */

/*** module init & exit & stuff *****************************************/


PIKE_MODULE_EXIT
{
   free_string(param_baseline);
   free_string(param_quality);

#ifdef HAVE_LIBTTF
   TT_Done_FreeType(engine);
   if (image_ttf_faceinstance_program) {
     free_program(image_ttf_faceinstance_program);
     image_ttf_faceinstance_program = NULL;
   }
   if (image_ttf_face_program) {
     free_program(image_ttf_face_program);
     image_ttf_face_program = NULL;
   }
#endif /* HAVE_LIBTTF */
}

PIKE_MODULE_INIT
{
#ifdef HAVE_LIBTTF
   unsigned char palette[5]={0,64,128,192,255};
   TT_Error errcode;
#endif /* HAVE_LIBTTF */

   param_baseline=make_shared_string("baseline");
   param_quality=make_shared_string("quality");

#ifdef HAVE_LIBTTF
   /* First check that we actually can initialize the FreeType library. */
   if ((errcode = TT_Init_FreeType(&engine))) {
#ifdef PIKE_DEBUG
     fprintf(stderr, "TT_Init_FreeType() failed with code 0x%03lx!\n",
	     (unsigned long)errcode);
#endif /* PIKE_DEBUG */
     return;
   }

   TT_Set_Raster_Gray_Palette(engine, palette);
   TT_Init_Kerning_Extension( engine );

#ifdef DYNAMIC_MODULE
   push_text("Image.Image");
   SAFE_APPLY_MASTER("resolv",1);
   if (sp[-1].type==T_PROGRAM)
      image_program=program_from_svalue(sp-1);
   pop_stack();
#endif /* DYNAMIC_MODULE */

   if (image_program)
   {
      /* function(string,void|mapping(string:int):object) */
  ADD_FUNCTION("`()",image_ttf_make,tFunc(tStr tOr(tVoid,tMap(tStr,tInt)),tObj),0);

      /* make face program */

      start_new_program();
      ADD_STORAGE(struct image_ttf_face_struct);

#ifdef TTF_DEBUG_INFO
      /* function(:mapping) */
  ADD_FUNCTION("properties",image_ttf_face_properties,tFunc(tNone,tMapping),0);
#endif /* TTF_DEBUG_INFO */

      /* function(:object) */
  ADD_FUNCTION("flush",image_ttf_face_flush,tFunc(tNone,tObj),0);
      /* function(:mapping(string:string)) */
  ADD_FUNCTION("names",image_ttf_face_names,tFunc(tNone,tMap(tStr,tStr)),0);
      /* function(:array(array)) */
  ADD_FUNCTION("_names",image_ttf_face__names,tFunc(tNone,tArr(tArray)),0);

      /* function(:object) */
  ADD_FUNCTION("`()",image_ttf_face_make,tFunc(tNone,tObj),0);

      set_exit_callback(image_ttf_face_exit);
      image_ttf_face_program=end_program();

      /* make face instance program */

      start_new_program();
      ADD_STORAGE(struct image_ttf_faceinstance_struct);

      /* function(object:void) */
  ADD_FUNCTION("create",image_ttf_faceinstance_create,tFunc(tObj,tVoid),0);
      /* function(string:object) */
  ADD_FUNCTION("ponder",image_ttf_faceinstance_ponder,tFunc(tStr,tObj),0);
      /* function(string...:object) */
  ADD_FUNCTION("write",image_ttf_faceinstance_write,tFuncV(tNone,tStr,tObj),0);
      /* function(:object) */
  ADD_FUNCTION("face",image_ttf_faceinstance_face,tFunc(tNone,tObj),0);
      /* function(int:object) */
  ADD_FUNCTION("set_height",image_ttf_faceinstance_set_height,tFunc(tInt,tObj),0);

      set_exit_callback(image_ttf_faceinstance_exit);
      image_ttf_faceinstance_program=end_program();
   }
#endif /* HAVE_LIBTTF */
}
