#include "global.h"
RCSID("$id: $");

#include "config.h"

#if !defined(HAVE_LIBJPEG)
#undef HAVE_JPEGLIB_H
#endif

#ifdef HAVE_JPEGLIB_H

#include "jpeglib.h"

#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "threads.h"
#include "array.h"
#include "error.h"
#include "threads.h"

#include "../Image/image.h"

/*
**! module Image
**! submodule JPEG
*/

void my_exit_error(struct jpeg_common_struct *cinfo)
{
}

void my_emit_message(struct jpeg_common_struct *cinfo,int msg_level)
{
}

void my_output_message(struct jpeg_common_struct *cinfo)
{
}

void my_format_message(struct jpeg_common_struct *cinfo,char *buffer)
{
}

void my_reset_error_mgr(struct jpeg_common_struct *cinfo)
{
}

/*
**! method object decode
*/

static void image_jpeg_decode(INT32 args)
{
   struct jpeg_decompress_struct cinfo;
   struct jpeg_error_mgr jerr;
   
   struct jpeg_error_mgr my_error_mgr=
   { my_exit_error,
     my_emit_message,
     my_output_message,
     my_format_message,
     my_reset_error_mgr
   };
   
}

#endif /* HAVE_JPEGLIB_H */


void pike_module_exit(void)
{
   add_function("decode",image_jpeg_decode,
		"function(string:object)",0);
}

void pike_module_init(void)
{
#ifdef HAVE_JPEGLIB_H
#endif /* HAVE_JPEGLIB_H */
}
