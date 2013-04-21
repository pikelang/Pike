/*
 * Representation of a pike parse tree.
 *
 * $Id$
 *
 * Henrik Grubbström 2002-08-28
 */

#include "global.h"

#include "config.h"

#include "svalue.h"
#include "object.h"
#include "interpret.h"
#include "operators.h"
#include "mapping.h"
#include "interpret.h"
#include "builtin_functions.h"

/*! @module Tokenizer
 */

/*! @class Node
 *!
 *!   Basic parse tree node.
 *!
 *! @seealso
 *!   @[UnaryNode], @[BinaryNode]
 */

static struct program *parse_node_program = NULL;

struct parse_node
{
  struct object *parent, *car, *cdr;
  /*! @decl Node parent
   *!
   *! Pointer to parent node.
   *!
   *! Used for traversing the parsetree without spending stack.
   */

  struct mapping *misc;
  /*! @decl mapping(string:mixed) misc
   *!
   *! Mapping that may contain some of the following information:
   *!
   *! @mapping
   *!   @member mixed "line_info"
   *!      Line number information.
   *!
   *!   @member mixed "type_info"
   *!      Expression type information.
   *!
   *!   @member mixed "value"
   *!      Expression value.
   *! @endmapping
   */

  int node_type;
  /* Node type information.
   *
   * NOTE: Is not available directly to the Pike layer.
   */
};

static void init_parse_node(struct object *o)
{
  struct parse_node *n = (struct parse_node *)(Pike_fp->current_storage);

  n->parent = n->car = n->cdr = NULL;

  n->misc = NULL;

  n->node_type = -1;
}

static void exit_parse_node(struct object *o)
{
  struct parse_node *n = (struct parse_node *)(Pike_fp->current_storage);

  if (n->parent) {
    free_object(n->parent);
    n->parent = NULL;
  }
  if (n->car) {
    free_object(n->car);
    n->car = NULL;
  }
  if (n->cdr) {
    free_object(n->cdr);
    n->cdr = NULL;
  }
  if (n->misc) {
    free_mapping(n->misc);
    n->misc = NULL;
  }
  n->node_type = -1;
}

static int nodename_id_no = -1;
/*! @decl constant nodename
 *!
 *!   Descriptive name of the node type.
 */

static void parse_node__sprintf(INT32 args)
{
  pop_n_elems(args);
  Pike_sp->type = PIKE_T_INT;	/* Safety... */
  low_object_index_no_free(Pike_sp++, Pike_fp->current_object, nodename_id_no);
}

/*! @endclass
 */

/*! @class UnaryNode
 */

static struct program *parse_node1_program = NULL;

/*! @decl Node car
 *!
 *!   Child node.
 */

/*! @endclass
 */

/*! @class BinaryNode
 */

static struct program *parse_node2_program = NULL;

/*! @decl Node cdr
 *!
 *!   Second child node.
 */

/*! @endclass
 */

/*! @endmodule
 */


void parsetree_module_init(void)
{
  int car_id, cdr_id;

  /* Node */
  {
    start_new_program();
    ADD_STORAGE(struct parse_node);

    ADD_FUNCTION("_sprintf", parse_node__sprintf,
		 tFunc(tNone, tStr), ID_STATIC);

    push_constant_text("Tokenizer.Node");
    nodename_id_no = simple_add_constant("nodename", Pike_sp-1, 0);
    pop_stack();

    map_variable("parent", "object(this_program)", 0,
		 OFFSETOF(parse_node, parent), PIKE_T_OBJECT);
    map_variable("misc", "mapping(string:mixed)", 0,
		 OFFSETOF(parse_node, misc), PIKE_T_MAPPING);

    /* NOTE: These two aren't visible in Node, but are
     * made visible after inheriting.
     */
    car_id = map_variable("car", "object", ID_STATIC|ID_PRIVATE,
			  OFFSETOF(parse_node, car), PIKE_T_OBJECT);
    cdr_id = map_variable("cdr", "object", ID_STATIC|ID_PRIVATE,
			  OFFSETOF(parse_node, cdr), PIKE_T_OBJECT);

    set_init_callback(init_parse_node);
    set_exit_callback(exit_parse_node);

    parse_node_program = end_program();
    add_program_constant("Node", parse_node_program, 0);
  }

  /* UnaryNode */
  {
    start_new_program();
    low_inherit(parse_node_program, NULL, -1, 0, 0, NULL);

    push_constant_text("Tokenizer.UnaryNode");
    simple_add_constant("nodename", Pike_sp-1, 0);
    pop_stack();

    /* Make the car field visible... */
    Pike_compiler->new_program->identifier_references[car_id].id_flags &=
      ~(ID_STATIC|ID_PRIVATE|ID_HIDDEN);

    parse_node1_program = end_program();
    add_program_constant("UnaryNode", parse_node1_program, 0);
  }

  /* BinaryNode */
  {
    start_new_program();
    low_inherit(parse_node1_program, NULL, -1, 0, 0, NULL);

    push_constant_text("Tokenizer.BinaryNode");
    simple_add_constant("nodename", Pike_sp-1, 0);
    pop_stack();

    /* Make the cdr field visible... */
    Pike_compiler->new_program->identifier_references[cdr_id].id_flags &=
      ~(ID_STATIC|ID_PRIVATE|ID_HIDDEN);

    parse_node2_program = end_program();
    add_program_constant("BinaryNode", parse_node2_program, 0);
  }
}

void parsetree_module_exit(void) 
{
  if (parse_node2_program) {
    free_program(parse_node2_program);
    parse_node2_program = NULL;
  }
  if (parse_node1_program) {
    free_program(parse_node1_program);
    parse_node1_program = NULL;
  }
  if (parse_node_program) {
    free_program(parse_node_program);
    parse_node_program = NULL;
  }
}
