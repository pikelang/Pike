#include "global.h"
#include "stralloc.h"
#include "global.h"
RCSID("$Id: whitefish.c,v 1.3 2001/05/22 13:29:21 per Exp $");
#include "pike_macros.h"
#include "interpret.h"
#include "program.h"
#include "program_id.h"
#include "object.h"
#include "operators.h"

#include "config.h"

#include "whitefish.h"
#include "resultset.h"
#include "blob.h"

/* must be included last */
#include "module_magic.h"


static void f_do_query_merge( INT32 args )
/*! @decl ResultSet do_query_merge( array(int) musthave,          @
 *!                          array(int) field_coefficients,       @
 *!                          array(int) proximity_coefficients,   @
 *!                          function(int:string) blobfeeder,     @
 *!			     int field)
 *!       @[musthave]
 *!       
 *!          Arrays of word ids. Note that the order is significant
 *!          for the ranking.
 *!
 *!       @[field_coefficients]
 *!
 *!       An array of ranking coefficients for the different fields. 
 *!       In the range of [0x0000-0xffff]. The array (always) has 68
 *!       elements:
 *!
 *!	  Index        Coefficient for field
 *!	  -----        ---------------------
 *!	  0..63        Special field 0..63
 *!	  64           Large body text
 *!	  65           Medium body text
 *!	  66           Small body text
 *!	  67           Anchor
 *!
 *!       @[proximity_coefficients]
 *!
 *!         An array of ranking coefficients for the different
 *!         proximity categories. Always has 8 elements, in the range
 *!         of [0x0000-0xffff].
 *!	 
 *!	 Index       Meaning
 *!	 -----       -------
 *!      0           spread: 0 (Perfect hit)
 *!	 1           spread: 1-5
 *!	 2           spread: 6-10                                 
 *!	 3           spread: 11-20
 *!	 4           spread: 21-40
 *!	 5           spread: 41-80
 *!	 6           spread: 81-160
 *!	 7           spread: 161-
 *!
 *!	 The 'spread' value should be defined somehow.
 *!	 
 *!     @[blobfeeder]
 *!     
 *!      This function returns a Pike string containing the word hits
 *!	 for a certain word_id. Call repeatedly until it returns 0.
 *!
 *!     @[field]
 *!
 *!         Selects what fields to match words in. Should be -1 to
 *!         match any field. 
 *!       
 *!	  Index        Field
 *!	  -----        ---------------------
 *!	  -1           Any field
 *!	  0..63        Special field 0..63
 *!	  64           Body text
 */
{
  int proximity_coefficients[8];
  int field_cofficients[68];

  Blob *blobs;
}

void pike_module_init(void)
{
  init_resultset_program();

  add_function( "do_query_merge", f_do_query_merge,
		"function(array(int),array(int),array(int)"
		",function(int:string),int",
		0 );
}

void pike_module_exit(void)
{
  exit_resultset_program();
}
