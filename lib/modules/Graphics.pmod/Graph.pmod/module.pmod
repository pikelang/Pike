#pike __REAL_VERSION__

#include "graph.h"

private inherit .polyline;
private inherit .create_graph;
private inherit .create_bars;
private inherit .create_pie;

//! This function sets all unset elements in diagram_data to its
//! default value as well as performing some simple sanity checks.
//! This function is called from within @[pie], @[bars], @[sumbars],
//! @[line], @[norm] and @[graph].
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
    if(zero_type(diagram_data[i]))
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

//! @fixme
//!   Document this function
Image.Image pie(mapping(string:mixed) diagram_data)
{
  diagram_data = diagram_data + ([]);
  check_mapping(diagram_data, "pie");
  return create_pie(diagram_data)->image;
} 

//! @fixme
//!   Document this function
Image.Image bars(mapping(string:mixed) diagram_data)
{
  diagram_data = diagram_data + ([]);
  check_mapping(diagram_data, "bars");
  return create_bars(diagram_data)->image;
} 

//! @fixme
//!   Document this function
Image.Image sumbars(mapping(string:mixed) diagram_data)
{ 
  diagram_data = diagram_data + ([]);
  check_mapping(diagram_data, "sumbars");
  return create_bars(diagram_data)->image;
} 

//! @fixme
//!   Document this function
Image.Image line(mapping(string:mixed) diagram_data)
{
  diagram_data = diagram_data + ([]);
  check_mapping(diagram_data, "line");
  return create_bars(diagram_data)->image;
} 

//! @fixme
//!   Document this function
Image.Image norm(mapping(string:mixed) diagram_data)
{
  diagram_data = diagram_data + ([]);
  check_mapping(diagram_data, "norm");
  return create_bars(diagram_data)->image;
} 

//! @fixme
//!   Document this function
Image.Image graph(mapping(string:mixed) diagram_data)
{
  diagram_data = diagram_data + ([]);
  check_mapping(diagram_data, "graph");
  return create_graph(diagram_data)->image;
} 
