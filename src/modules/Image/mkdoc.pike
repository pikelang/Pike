#include <stdio.h>

mapping parse=([]);

/*

module : mapping <- moduleM
	"desc" : text
	"classes" : mapping <- classM
		class : mapping
			"desc" : text
			"methods" : array of mappings <- methodM
				"methods" : list of method names
				"decl" : array of textlines of declarations
				"desc" : text
				"returns" : textline
				"see also" : array of references 
				"note" : text
				"args" : mapping
					arg : mapping <- argM
						"type" : textline
						"desc" : text
		
*/

mapping moduleM, classM, methodM, argM;

int main()
{
   string doc=stdin->read(10000000);
   
}
