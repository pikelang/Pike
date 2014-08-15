#pike __REAL_VERSION__

#include "graph.h"

protected inherit .create_pie;

//! This function sets all unset elements in diagram_data to its
//! default value as well as performing some simple sanity checks.
//! This function is called from within @[pie], @[bars], @[sumbars],
//! @[line], @[norm] and @[graph].
//!
//! @param diagram_data
//!   @mapping
//!     @member mixed "drawtype"
//!       Default: "linear"
//!
//!       Will be set to "2D" for pie below
//!       Only "linear" works for now.
//!     @member mixed "tone"
//!       Default: 0
//!
//!       If present a Pie-chart will be toned.
//!     @member mixed "3Ddepth"
//!       Default: 10
//!
//!       How much 3D-depth a graph will have in pixels Default is 10.
//!     @member array(array(float)) "data"
//!       Default: ({({1.0}), ({2.0}), ({4.0})})
//!
//!       Will be set to something else with graph
//!       An array of arrays. Each array describing a data-set.
//!       The graph-function however should be fed with an array
//!       of arrays with X,Y-pairs.
//!       Example:
//!       ({({1.0, 2.0, 3.0}),({1.2, 2.2, 3.8})}) draws stuff in yellow
//!       with the values (1.0, 2.0, 3.0), and (1.2, 2.2, 3.8) in blue.
//!     @member array(string) "labels"
//!       Default: 0
//!
//!       Should have four elements
//!       ({xquantity, yquantity, xunit, yunit}). The strings will
//!       be written on the axes.
//!     @member array(string) "xnames"
//!       Default: 0
//!
//!       An array(string) with the text that will be written under
//!       the X-axis. This should be the same size as sizeof(data).
//!     @member array(string) "ynames"
//!       Default: 0
//!
//!       An array(string) with the text that will be written to
//!       the left of the Y-axis.
//!     @member mixed "fontsize"
//!       Default: 10
//!
//!       The size of the text. Default is 10.
//!     @member mixed "graphlinewidth"
//!       Default: 1.0
//!
//!       Width of the lines that draws data in Graph and line.
//!       Default is 1.0
//!     @member mixed "labelsize"
//!       Default: same as fontsize
//!
//!       The size of the text for labels.
//!     @member mixed "legendfontsize"
//!       Default: same as fontsize
//!
//!       The size of the text for the legend.
//!     @member mixed "legend_texts"
//!       Default: 0
//!
//!       The texts that will be written the legend.
//!     @member array(float) "values_for_xnames"
//!       Default: 0
//!
//!       An array(float) that describes where the ynames should
//!       be placed. The numbers are like the data-numbers.
//!       Default is equally distributed.
//!     @member array(float) "values_for_ynames"
//!       Default: 0
//!
//!       An array(float) that describes where the ynames should
//!       be placed. The numbers are like the data-numbers.
//!       Default is equally distributed.
//!     @member int "xsize"
//!       Default: 100
//!
//!       X-size of the graph in pixels.
//!     @member int "ysize"
//!       Default: 100
//!
//!       Y-size of the graph in pixels.
//!     @member mixed "image"
//!       Default: 0
//!
//!       An image that the graph will be drawn on.
//!     @member mixed "legendcolor"
//!       Default: 0
//!
//!       The color of the text in the legend. Default is?
//!     @member mixed "legendimage"
//!       Default: 0
//!
//!       I have no idea.
//!     @member mixed "bgcolor"
//!       Default: 0
//!
//!       The bakground-color. If the the background is a image
//!       this color is used for antialias the texts.
//!     @member mixed "gbimage"
//!       Default: 0
//!
//!       Some sort of image...
//!     @member mixed "axcolor"
//!       Default: ({0,0,0})
//!
//!       The color of the axis.
//!     @member mixed "datacolors"
//!       Default: 0
//!
//!       An array of colors for the datasets.
//!     @member mixed "backdatacolors"
//!       Default: 0
//!
//!       An array of color that do something...
//!     @member mixed "textcolor"
//!       Default: ({0,0,0})
//!
//!       Color of the text. Default is black.
//!     @member mixed "labelcolor"
//!       Default: 0
//!
//!       Color of the labeltexts.
//!     @member string "orient"
//!       Default: "hor"
//!
//!       Can be "hor" or "vert". Orientation of the graph.
//!     @member mixed "linewidth"
//!       Default: 0
//!
//!       Width of lines (the axis and their like).
//!     @member mixed "backlinewidth"
//!       Default: 0
//!
//!       Width of the outline-lines. Default is 0.
//!     @member mixed "vertgrid"
//!       Default: 0
//!
//!       If the vertical grid should be present.
//!     @member mixed "horgrid"
//!       Default: 0
//!
//!       If the horizontal grid should be present.
//!     @member mixed "gridwidth"
//!       Default: 0
//!
//!       Width of the grid. Default is linewidth/4.
//!     @member mixed "rotate"
//!       Default: 0
//!
//!       How much a the Pie in a Pie-shart should be rotated in degrees.
//!     @member mixed "center"
//!       Default: 0
//!
//!       Makes the first Pie-slice be centered.
//!     @member mixed "bw"
//!       Default: 0
//!
//!       Draws the graph black and white.
//!     @member mixed "eng"
//!       Default: 0
//!
//!       Writes the numbers in eng format.
//!     @member mixed "neng"
//!       Default: 0
//!
//!       Writes the numbers in engformat except for 0.1 < x < 1.0
//!     @member mixed "xmin"
//!       Default: 0
//!
//!       Where the X-axis should start. This will be overrided
//!       by datavalues.
//!     @member mixed "ymin"
//!       Default: 0
//!
//!       Where the Y-axis should start. This will be overridden
//!       by datavalues.
//!     @member mixed "name"
//!       Default: 0
//!
//!       A string with the name of the graph that will be written
//!       at top of the graph.
//!     @member mixed "namecolor"
//!       Default: 0
//!
//!       The color of the name.
//!     @member mixed "font"
//!       Default: Image.Font()
//!
//!       The font that will be used.
//!     @member mixed "gridcolor"
//!       Default: ({0,0,0}
//!
//!       The color of the grid. Default is black.
//!   @endmapping
mapping(string:mixed) check_mapping(mapping(string:mixed) diagram_data, 
				    string type)
{

  //This maps from mapping entry to defaulvalue
  //This may be optimized, but I don't think the zeros mather. 
  //I just wanted all indices to be here. But this will not be
  //Updated so it is a bad idea in the long run.
  mapping md=
    ([
      "type":1, //This is already checked
      "subtype":1, //This is already checked
      "drawtype":"linear", //Will be set to "2D" for pie below
      "tone":0,
      "3Ddepth":10,
      "data":({({1.0}), ({2.0}), 
	       ({4.0})}), //Will be set to something else with graph
      "labels":0, //array(string) ({xquantity, yquantity, xunit, yunit})
      "xnames":0, //array(string) ?
      "ynames":0, //array(string) ?
      "fontsize":10,
      "graphlinewidth":1.0,
      "labelsize":0, //Default is set somewhere else
      "legendfontsize":0,
      "legend_texts":0,
      "values_for_xnames":0,
      "values_for_ynames":0,
      //xmaxvalue, xminvalue, ymaxvalue, yminvalue;
      "xsize":100,
      "ysize":100,
      "image":0,
      "legendcolor":0,
      "legendimage":0,
      "bgcolor":0,
      "gbimage":0,
      "axcolor":({0,0,0}),
      "datacolors":0,
      "backdatacolors":0,
      "textcolor":({0,0,0}),
      "labelcolor":0,
      "orient":"hor",
      "linewidth":0,
      "backlinewidth":0,
      "vertgrid":0,
      "horgrid":0,
      "gridwidth":0,
      "rotate":0,
      "center":0,
      "bw":0,
      "eng":0,
      "neng":0,
      "xmin":0,
      "ymin":0,
      "name":0,
      "namecolor":0,
      "font":Image.Font(),
      "gridcolor":({0,0,0})
    ]);

  foreach( md; string i; mixed v)
    if(!has_index(diagram_data, i))
      diagram_data[i]=v;

  switch(type)
  {
  case "pie":
    diagram_data->type = "pie";
    diagram_data->subtype="pie";
    break;
  case "bars":
    diagram_data->type = "bars";
    diagram_data->subtype = "box";
    m_delete( diagram_data, "drawtype" );
    break;
  case "line":
    diagram_data->type = "bars";
    diagram_data->subtype = "line";
    break;
  case "norm":
    diagram_data->type = "sumbars";
    diagram_data->subtype = "norm";
    break;
  case "graph":
    diagram_data->type = "graph";
    diagram_data->subtype = "line";
    break;
  case "sumbars":
    diagram_data->type = "sumbars";
    break;
  default:
    error("Wrong type %O given to check_mapping.\n", type);
  }

  return diagram_data;
}

//! @decl Image.Image pie(mapping(string:mixed) diagram_data)
//! @decl Image.Image bars(mapping(string:mixed) diagram_data)
//! @decl Image.Image sumbars(mapping(string:mixed) diagram_data)
//! @decl Image.Image line(mapping(string:mixed) diagram_data)
//! @decl Image.Image norm(mapping(string:mixed) diagram_data)
//! @decl Image.Image graph(mapping(string:mixed) diagram_data)
//!   Generate a graph of the specified type. See @[check_mapping]
//!   for an explanation of diagram_data.

Image.Image pie(mapping(string:mixed) diagram_data)
{
  diagram_data = diagram_data + ([]);
  check_mapping(diagram_data, "pie");
  return create_pie(diagram_data)->image;
} 

Image.Image bars(mapping(string:mixed) diagram_data)
{
  diagram_data = diagram_data + ([]);
  check_mapping(diagram_data, "bars");
  return create_bars(diagram_data)->image;
} 

Image.Image sumbars(mapping(string:mixed) diagram_data)
{ 
  diagram_data = diagram_data + ([]);
  check_mapping(diagram_data, "sumbars");
  return create_bars(diagram_data)->image;
} 

Image.Image line(mapping(string:mixed) diagram_data)
{
  diagram_data = diagram_data + ([]);
  check_mapping(diagram_data, "line");
  return create_bars(diagram_data)->image;
} 

Image.Image norm(mapping(string:mixed) diagram_data)
{
  diagram_data = diagram_data + ([]);
  check_mapping(diagram_data, "norm");
  return create_bars(diagram_data)->image;
} 

Image.Image graph(mapping(string:mixed) diagram_data)
{
  diagram_data = diagram_data + ([]);
  check_mapping(diagram_data, "graph");
  return create_graph(diagram_data)->image;
}
