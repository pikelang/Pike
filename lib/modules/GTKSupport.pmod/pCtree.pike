#pike __REAL_VERSION__

inherit GTK.Ctree:ctree;

class Node
{
   private static program Prog=object_program(this_object());

   private static GTK.CTreeNode node;

   void create(object _node)
   {
      node=_node;
   }

   object(GTK.CTreeNode) get_node()
   {
      return node;
   }

   Node collapse()
   {
      ctree::collapse(node);
      return this;
   }
   Node collapse_recursive()
   {
      ctree::collapse_recursive(node);
      return this;
   }
   Node collapse_to_depth(int depth)
   {
      ctree::collapse_to_depth(node,depth);
      return this;
   }
   Node expand()
   {
      ctree::expand(node);
      return this;
   }
   Node expand_recursive()
   {
      ctree::expand_recursive(node);
      return this;
   }
   Node expand_to_depth(int depth )
   {
      ctree::expand_to_depth(node,depth);
      return this;
   }

   Node insert_sibling(array(string)|void columns,
		       void|int is_leaf,void|int expanded)
   {
      return this_program(ctree::insert_node(0,node,columns,is_leaf,expanded));
   }

   Node insert_child(array(string)|void columns,
		     void|int is_leaf,void|int expanded)
   {
      return this_program(ctree::insert_node(node,0,columns,is_leaf,expanded));
   }

   int is_ancestor(Node what)
   {
      return !!ctree::is_ancestor(node,what);
   }

   int is_viewable()
   {
      return !!ctree::is_viewable(node);
   }

   int is_visible()
   {
      return !!ctree::node_is_visible(node);
   }

   Node last()
   {
      return this_program(ctree::last(node));
   }

   Node move_new_parent(Node parent)
   {
      ctree::move(node,parent,0);
      return this;
   }

   Node move_new_sibling(Node sibling)
   {
      ctree::move(node,0,sibling);
      return this;
   }

   GTK.Style get_cell_style(int cell)
   {
      return ctree::node_get_cell_style(node,cell);
   }

   GTK.Style get_style()
   {
      return ctree::node_get_row_style(node);
   }

   Node set_style(GTK.Style style)
   {
      ctree::node_set_row_style(node,style);
      return node;
   }

   int get_cell_type(int cell)
   {
      return !!ctree::node_get_cell_type(node,cell);
   }

   Node set_background(GDK.Color color)
   {
      ctree::node_set_background(node,color);
      return this;
   }

   Node set_foreground(GDK.Color color)
   {
      ctree::node_set_foreground(node, color);
      return this;
   }

   Node set_cell_style(int column,GTK.Style style)
   {
      ctree::node_set_cell_style(node, column, style);
      return this;
   }

   int get_selectable()
   {
      return !!ctree::node_get_selectable(node);
   }

   Node set_selectable(int yes)
   {
      ctree::node_set_selectable(node,yes);
      return this;
   }

   Node moveto(int column,void|float row_align,float|void col_align)
   {
      ctree::node_moveto(node,column,row_align||0.0,col_align||0.0);
      return this;
   }

   string get_text(int column)
   {
      return ctree::node_get_text(node,column);
   }

   Node set_text(int column,string text)
   {
      ctree::node_set_text(node,column,text);
      return this;
   }

   array(string) get_texts()
   {
      return Array.map(indices(allocate(ctree::get_columns())),get_text);
   }

   Node set_texts(array(string) what)
   {
      foreach (indices(what),int i) set_text(i,what[i]);
      return this;
   }

   mapping get_pixmap( int column )
   {
      return node_get_pixmap(node,column);
   }

   mapping get_pixtext( int column )
   {
      return node_get_pixtext(node,column);
   }

   Node set_pixmap(int column, GDK.Pixmap pixmap,void|object(GDK.Bitmap) mask )
   {
      ctree::node_set_pixmap(node,column,pixmap,mask);
      return this;
   }

   Node set_pixtext( int column, string text, int spacing,
		     GDK.Pixmap pixmap, object(GDK.Bitmap)|void mask)
   {
      ctree::node_set_pixtext(node,column,text,spacing,pixmap,mask);
      return this;
   }

   Node set_data(object z)
   {
      ctree::node_set_row_data(node,z);
      return this;
   }

   object get_data()
   {
      return ctree::node_get_row_data(node);
   }

   Node set_info(string text, int spacing,
		 object(GDK.Pixmap)|void pixmap_closed,
		 object(GDK.Bitmap)|void mask_closed,
		 object(GDK.Pixmap)|void pixmap_opened,
		 object(GDK.Bitmap)|void mask_opened,
		 int is_leaf, int expanded )
   {
      ctree::set_node_info(node,text,spacing,pixmap_closed,
			   mask_closed,pixmap_opened,mask_opened,
			   is_leaf,expanded);
      return this;
   }

   Node set_shift(int column,int vertical,int horizontal)
   {
      ctree::node_set_shift(node,column,vertical,horizontal);
      return this;
   }

   void remove()
   {
      ctree::remove_node(node);
      node=0;
   }

   Node select()
   {
      ctree::select(node);
      return this;
   }

   Node select_recursive()
   {
      ctree::select_recursive(node);
      return this;
   }

   Node sort()
   {
      ctree::sort_node(node);
      return this;
   }

   Node sort_recursive()
   {
      ctree::sort_recursive(node);
      return this;
   }

   Node toggle_expansion()
   {
      ctree::toggle_expansion(node);
      return this;
   }

   Node toggle_expansion_recursive()
   {
      ctree::toggle_expansion_recursive(node);
      return this;
   }

   Node unselect()
   {
      ctree::unselect(node);
      return this;
   }

   Node unselect_recursive()
   {
      ctree::unselect_recursive(node);
      return this;
   }

   int find(Node what)
   {
      return (int)ctree::find(what->get_node(),node);
   }

   Node find_by_data(object data)
   {
      return this_program(ctree::find_by_row_data(data,node));
   }

   mixed `[](string z)
   {
      return ::`[](z) || (node && node[z]);
   }

   mixed `->(string z)
   {
      return `[](z);
   }

   array(string) _indices()
   {
      return indices(this)+(node?indices(node):({}));
   }

   array _values()
   {
      return Array.map(_indices(),`[]);
   }

   Node find_node_ptr()
   {
      return this_program(ctree::find_node_ptr(node));
   }
}

Node root=Node(0);

Node node_nth(int i)
{
   return Node(ctree::node_nth(i));
}

static void collapse() {} /* (node); */
static void collapse_recursive() {} /* (node); */
static void collapse_to_depth() {} /* (node,depth); */
static void expand() {} /* (node); */
static void expand_recursive() {} /* (node); */
static void expand_to_depth() {} /* (node,depth); */
static void insert_node() {} /* (0,node,columns,is_leaf,expanded)); */
static void is_ancestor() {} /* (node,what); */
static void is_viewable() {} /* (node); */
static void node_is_visable() {} /* (node); */
static void last() {} /* (node)); */
static void move() {} /* (node,parent,0); */
static void node_get_cell_style() {} /* (node,cell); */
static void node_get_row_style() {} /* (node); */
static void node_set_row_style() {} /* (node,style); */
static void node_get_cell_type() {} /* (node,cell); */
static void set_background() {} /* (node,color); */
static void node_set_foreground() {} /* (node, color); */
static void node_set_cell_style() {} /* (node, style); */
static void node_get_selectable() {} /* (node); */
static void node_set_selectable() {} /* (node,yes); */
static void node_moveto() {} /* (node,column,row_align||0.0,col_align||0.0); */
static void node_get_text() {} /* (node,column); */
static void node_set_text() {} /* (node,column,text); */
static void columns() {} /* ())),get_text); */
static void node_set_pixmap() {} /* (node,column,pixmap,mask); */
static void node_set_pixtext() {} /* (node,column,text,spacing,pixmap,mask); */
static void node_set_row_data() {} /* (node,z); */
static void node_get_row_data() {} /* (node); */
static void set_node_info() {} /* (node,text,spacing,pixmap_closed, */
static void set_node_shift() {} /* (node,column,vertical,horizontal); */
static void remove_node() {} /* (node); */
static void select() {} /* (node); */
static void select_recursive() {} /* (node); */
static void sort_node() {} /* (node); */
static void sort_recursive() {} /* (node); */
static void toggle_expansion() {} /* (node); */
static void toggle_expansion_recursive() {} /* (node); */
static void unselect_expansion() {} /* (node); */
static void unselect_expansion_recursive() {} /* (node); */
static void find() {} /* (what->get_node(),node); */
static void find_by_row_data() {} /* (what->get_node(),node)); */
