//! Graph sub-module for drawing bars.
// These functions were written by Henrik "Hedda" Wallin (hedda@roxen.com)
// Create_bars can draw normal bars, sumbars and normalized sumbars.

#pike __REAL_VERSION__

#include "graph.h"

inherit .create_graph;

#ifdef BG_DEBUG
# define START_DEBUG(X) diagram_data->bg_timers[(X)]=gethrtime()
# define STOP_DEBUG(X) diagram->data->bg_timers[(X)]=gethrtime()-diagram_data->bg_timers[(X)]
#else
# define START_DEBUG(X)
# define STOP_DEBUG(X)
#endif

mapping(string:mixed) create_bars(mapping(string:mixed) diagram_data)
{

#ifdef BG_DEBUG
  diagram_data->bg_timers = ([]);
#endif

  //Supports only xsize>=100

  int si=diagram_data->fontsize;
 
  //Fix defaultcolors!
  setinitcolors(diagram_data);

  Image.Image barsdiagram;

  START_DEBUG("init_bg");
  init_bg(diagram_data);
  STOP_DEBUG("init_bg");

  barsdiagram=diagram_data->image;

  START_DEBUG("set_legend_size");
  set_legend_size(diagram_data);
  STOP_DEBUG("set_legend_size");

  //write("ysize:"+diagram_data->ysize+"\n");
  diagram_data->ysize-=diagram_data->legend_size;
  //write("ysize:"+diagram_data->ysize+"\n");
  
  //Calculate biggest and smallest datavalues
  START_DEBUG("init");
  init(diagram_data);
  STOP_DEBUG("init");

  //Calculate how many and how big the texts are.
  START_DEBUG("space");
  if (!(diagram_data->xspace))
  {
    //Initiate the distance between the texts on the x-axis (Or X-axis?)
    float range=(diagram_data->xmaxvalue-
		 diagram_data->xminvalue);
    float space=pow(10.0, floor(log(range/3.0)/log(10.0)));
    if (range/space>5.0)
    {
      if(range/(2.0*space)>5.0)
      {
	space=space*5.0;
      }
      else
	space=space*2.0;
    }
    else
      if (range/space<2.5)
	space*=0.5;
    diagram_data->xspace=space;      
  }
  if (!(diagram_data->yspace))
  {
    //Initiate the distance between the texts on the y-axis (Or X-axis?)
    float range=(diagram_data->ymaxvalue-
		 diagram_data->yminvalue);
    float space=pow(10.0, floor(log(range/3.0)/log(10.0)));
    if (range/space>5.0)
    {
      if(range/(2.0*space)>5.0)
	space *= 5.0;
      else
	space *= 2.0;
    }
    else
      if (range/space<2.5)
	space *= 0.5;
    diagram_data->yspace=space;      
  }
  STOP_DEBUG("space");

  START_DEBUG("text");
  float start;
  start=diagram_data->xminvalue+diagram_data->xspace/2.0;
  diagram_data->values_for_xnames=allocate(sizeof(diagram_data->xnames));
  for(int i=0; i<sizeof(diagram_data->xnames); i++)
    diagram_data->values_for_xnames[i]=start+start*2*i;

  if (!(diagram_data->values_for_ynames))
  {
    if ((diagram_data->yspace<LITET)
	&& (diagram_data->yspace>-LITET))
      error( "Very bad error because yspace is zero!\n" );
    float start;
    start=diagram_data->yminvalue;
    start=diagram_data->yspace*ceil((start)/diagram_data->yspace);
    diagram_data->values_for_ynames=({start});
    while(start < diagram_data->ymaxvalue) {
      diagram_data->values_for_ynames+=({start+=diagram_data->yspace});
    }
  }
  
  function fun;
  if (diagram_data->eng)
    fun=diagram_eng;
  else
    fun=diagram_neng;
  
  //Draw the text if it does not exist
  if (!(diagram_data->ynames))
    if (diagram_data->eng||diagram_data->neng)
    {
      diagram_data->ynames=
	allocate(sizeof(diagram_data->values_for_ynames));
      array(mixed) v=diagram_data->values_for_ynames;
      mixed m=diagram_data->ymaxvalue;
      mixed mi=diagram_data->yminvalue;
      for(int i=0; i<sizeof(v); i++)
	if (abs(v[i]*1000)<max(m, abs(mi)))
	  diagram_data->ynames[i]="0";
	else	
	  diagram_data->ynames[i]=
	    fun((float)(v[i]));
    }
    else
    {
      diagram_data->ynames=
	allocate(sizeof(diagram_data->values_for_ynames));
      
      for(int i=0; i<sizeof(diagram_data->values_for_ynames); i++)
	diagram_data->ynames[i]=
	  no_end_zeros((string)(diagram_data->values_for_ynames[i]));
    }
  
  
  if (!(diagram_data->xnames))
  {
    diagram_data->xnames=
      allocate(sizeof(diagram_data->values_for_xnames));
      
    for(int i=0; i<sizeof(diagram_data->values_for_xnames); i++)
      diagram_data->xnames[i]=
	no_end_zeros((string)(diagram_data->values_for_xnames[i]));
  }


  //Draw the text-images
  //calculate xmaxynames, ymaxynames xmaxxnames ymaxxnames
  create_text(diagram_data);
  si=diagram_data->fontsize;
  STOP_DEBUG("text");

  //Create the label-texts for the X-axis
  object labelimg;
  string label;
  int labelx=0;
  int labely=0;
  if (diagram_data->labels)
  {
    if (diagram_data->labels[2] && sizeof(diagram_data->labels[2]))
      label=diagram_data->labels[0]+" ["+diagram_data->labels[2]+"]"; //Xstorhet
    else
      label=diagram_data->labels[0];

    object notext=GETFONT(xaxisfont);
    if ((label!="")&&(label!=0))
      labelimg=notext
	->write(UNICODE(label,diagram_data->encoding))
	->scale(0,diagram_data->labelsize);
    else
      labelimg=Image.Image(diagram_data->labelsize,diagram_data->labelsize);

    if (labelimg->xsize()<1)
      labelimg=Image.Image(diagram_data->labelsize,diagram_data->labelsize);

    if (labelimg->xsize()>
	diagram_data->xsize/2)
      labelimg=labelimg->scale(diagram_data->xsize/2, 0);

    labely=diagram_data->labelsize;
    labelx=labelimg->xsize();
  }
  else
    diagram_data->labelsize=0;

  labely+=write_name(diagram_data);

  int ypos_for_xaxis; //Distance from bottom
  int xpos_for_yaxis; //Distance from right

  //Decide where the graph can be drawn
  diagram_data->ystart=(int)ceil(diagram_data->linewidth);
  diagram_data->ystop=diagram_data->ysize-
    (int)ceil(diagram_data->linewidth+si)-labely;
  if (((float)diagram_data->yminvalue>-LITET)&&
      ((float)diagram_data->yminvalue<LITET))
    diagram_data->yminvalue=0.0;

  if (diagram_data->yminvalue<0)
  {
    //Calculate position for the x-axis. 
    //If this doesn't work: Draw the y-axis at right or left
    // and recalculate diagram_data->ystart
    ypos_for_xaxis=((-diagram_data->yminvalue)
		    * (diagram_data->ystop - diagram_data->ystart))
      /	(diagram_data->ymaxvalue-diagram_data->yminvalue)
      + diagram_data->ystart;
      
    int minpos;
    minpos=max(labely, diagram_data->ymaxxnames)+si/2;
    if (minpos>ypos_for_xaxis)
    {
      ypos_for_xaxis=minpos;
      diagram_data->ystart=ypos_for_xaxis+
	diagram_data->yminvalue*(diagram_data->ystop-ypos_for_xaxis)/
	(diagram_data->ymaxvalue);
    }
    else
    {
      int maxpos;
      maxpos=diagram_data->ysize-
	(int)ceil(diagram_data->linewidth+si*2)
	- labely;
      if (maxpos<ypos_for_xaxis)
      {
	ypos_for_xaxis=maxpos;
	diagram_data->ystop=ypos_for_xaxis
	  + diagram_data->ymaxvalue*(ypos_for_xaxis-diagram_data->ystart)
	  / (0-diagram_data->yminvalue);
      }
    }
  }
  else
    if (diagram_data->yminvalue==0.0)
    {
      // Place the x-axis and diagram_data->ystart at bottom.  
      diagram_data->ystop=diagram_data->ysize
	- (int)ceil(diagram_data->linewidth+si)-labely;
      ypos_for_xaxis=max(labely, diagram_data->ymaxxnames)+si/2;
      diagram_data->ystart=ypos_for_xaxis;
    }
    else
    {
      //Place the x-axis at bottom and diagram_data->ystart a
      //Little bit higher
      diagram_data->ystop=diagram_data->ysize
	- (int)ceil(diagram_data->linewidth+si)-labely;
      ypos_for_xaxis=max(labely, diagram_data->ymaxxnames)+si/2;
      diagram_data->ystart=ypos_for_xaxis+si*2;
    }

  //Calculate position for the y-axis
  diagram_data->xstart=(int)ceil(diagram_data->linewidth);
  diagram_data->xstop=diagram_data->xsize-
    (int)ceil(diagram_data->linewidth)-max(si,labelx+si/2)-
    diagram_data->xmaxxnames/2;
  if (((float)diagram_data->xminvalue>-LITET)&&
      ((float)diagram_data->xminvalue<LITET))
    diagram_data->xminvalue=0.0;
  
  if (diagram_data->xminvalue<0)
  {
    //Calculate position for the y-axis. 
    //If this doesn't work: Draw the y-axis at right or left
    // and recalculate diagram_data->xstart
    xpos_for_yaxis=((-diagram_data->xminvalue)
		    * (diagram_data->xstop-diagram_data->xstart))
      /	(diagram_data->xmaxvalue-diagram_data->xminvalue)
      + diagram_data->xstart;
      
    int minpos;
    minpos=diagram_data->xmaxynames+si/2;
    if (minpos>xpos_for_yaxis)
    {
      xpos_for_yaxis=minpos;
      diagram_data->xstart=xpos_for_yaxis+
	diagram_data->xminvalue*(diagram_data->xstop-xpos_for_yaxis)/
	(diagram_data->ymaxvalue);
    }
    else
    {
      int maxpos;
      maxpos=diagram_data->xsize-
	(int)ceil((float)diagram_data->linewidth+si*2+labelx);
      if (maxpos<xpos_for_yaxis)
      {
	xpos_for_yaxis=maxpos;
	diagram_data->xstop=xpos_for_yaxis+
	  diagram_data->xmaxvalue*(xpos_for_yaxis-diagram_data->xstart)/
	  (0-diagram_data->xminvalue);
      }
    }
  }
  else
    if (diagram_data->xminvalue==0.0)
    {
      //Place the y-axis at left (?) and diagram_data->xstart at
      // the same place
      diagram_data->xstop=diagram_data->xsize-
	(int)ceil(diagram_data->linewidth)-max(si,labelx+si/2)-
	diagram_data->xmaxxnames/2;
      xpos_for_yaxis=diagram_data->xmaxynames+si/2+2;
      diagram_data->xstart=xpos_for_yaxis+si/2;
    }
    else
    {
      //Place the y-axis at left (?) and diagram_data->xstart a
      // little bit more right (?)
      diagram_data->xstop=diagram_data->xsize-
	(int)ceil(diagram_data->linewidth)-max(si,labelx+si/2)-
	diagram_data->xmaxxnames/2;
      xpos_for_yaxis=diagram_data->xmaxynames+si/2;
      diagram_data->xstart=xpos_for_yaxis+si*2;
    }

  //Calculate some shit
  float xstart=(float)diagram_data->xstart;
  float xmore=(-xstart+diagram_data->xstop)/
    (diagram_data->xmaxvalue-diagram_data->xminvalue);
  float ystart=(float)diagram_data->ystart;
  float ymore=(-ystart+diagram_data->ystop)/
    (diagram_data->ymaxvalue-diagram_data->yminvalue);
  
  START_DEBUG("draw_grid");
  draw_grid(diagram_data, xpos_for_yaxis, ypos_for_xaxis,
	     xmore, ymore, xstart, ystart, (float) si);
  STOP_DEBUG("draw_grid");

  //???
  int farg=0;
 
  START_DEBUG("draw_values");
  if (diagram_data->type=="sumbars")
  {
    int s=diagram_data->datasize;
    float barw=diagram_data->xspace*xmore/3.0;
    for(int i=0; i<s; i++)
    {
      int j=0;
      float x,y;
      x=xstart+(diagram_data->xspace/2.0+diagram_data->xspace*i)*
	xmore;
      
      y=-(-diagram_data->yminvalue)*ymore+
	diagram_data->ysize-ystart;	 
      float start=y;
      
      foreach(column(diagram_data->data, i), float|string d)
      {
	if (d==VOIDSYMBOL)
	  d=0.0;
	y-=d*ymore;
	
	barsdiagram->setcolor(@(diagram_data->datacolors[j++]));
	
	barsdiagram->polygone(
			      ({x-barw, y 
				, x+barw, y, 
				x+barw, start
				, x-barw, start
			      }));  
	/*   barsdiagram->setcolor(0,0,0);
	     draw(barsdiagram, 0.5, 
	     ({
	     x-barw, start,
	     x-barw, y 
	     , x+barw, y, 
	     x+barw, start
	     
	     })
	     );
	*/
	start=y;
      }
    }
  }
  else
  if (diagram_data->subtype=="line")
    if (diagram_data->drawtype=="linear")
      foreach(diagram_data->data, array(float|string) d)
      {
	array(float|string) l=allocate(sizeof(d)*2);
	for(int i=0; i<sizeof(d); i++)
	  if (d[i]==VOIDSYMBOL)
	  {
	    l[i*2]=VOIDSYMBOL;
	    l[i*2+1]=VOIDSYMBOL;
	  }
	  else
	  {
	    l[i*2]=xstart+(diagram_data->xspace/2.0+
			   diagram_data->xspace*i)
	      * xmore;
	    l[i*2+1]=-(d[i]-diagram_data->yminvalue)*ymore+
	      diagram_data->ysize-ystart;	  
	  }
	  
	  //Draw Ugly outlines
	  if ((diagram_data->backdatacolors)&&
	      (diagram_data->backlinewidth))
	  {
	    barsdiagram->setcolor(@(diagram_data->backdatacolors[farg]));
	    draw(barsdiagram, diagram_data->backlinewidth,l,
		 diagram_data->xspace );
	  }

	  barsdiagram->setcolor(@(diagram_data->datacolors[farg++]));
	  draw(barsdiagram, diagram_data->graphlinewidth,l);
      }
    else
      error( "\""+diagram_data->drawtype
	     + "\" is an unknown bars-diagram drawtype!\n" );
  else
    if (diagram_data->subtype=="box")
      if (diagram_data->drawtype=="2D")
      {
	int s=sizeof(diagram_data->data);
	float barw=diagram_data->xspace*xmore/1.5;
	float dnr=-barw/2.0+ barw/s/2.0;
	barw/=s;
	barw/=2.0;
	farg=-1;
	float yfoo=(float)(diagram_data->ysize-ypos_for_xaxis);
	//"draw_values":3580,
	foreach(diagram_data->data, array(float|string) d)
	{
	  farg++;
	  
	  for(int i=0; i<sizeof(d); i++)
	    if (d[i]!=VOIDSYMBOL)
	    {
	      float x,y;
	      x=xstart+(diagram_data->xspace/2.0+diagram_data->xspace*i)*
		xmore;
	      y=-(d[i]-diagram_data->yminvalue)*ymore+
		diagram_data->ysize-ystart;	 
	      barsdiagram->setcolor(@(diagram_data->datacolors[farg]));
	      
	      barsdiagram->polygone(
				    ({x-barw+dnr, y 
				      , x+barw+dnr, y, 
				      x+barw+dnr, yfoo
				      , x-barw+dnr, yfoo
				    })); 
	      /*  barsdiagram->setcolor(0,0,0);		  
		  draw(barsdiagram, 0.5, 
		  ({x-barw+dnr, y 
		  , x+barw+dnr, y, 
		  x+barw+dnr, diagram_data->ysize-ypos_for_xaxis
		  , x-barw+dnr,diagram_data->ysize- ypos_for_xaxis,
		  x-barw+dnr, y 
		  }));*/
	    }
	  dnr+=barw*2.0;
	}   
      }
      else
	error( "\""+diagram_data->drawtype
	       + "\" is an unknown bars-diagram drawtype!\n" );
    else
      error( "\""+diagram_data->subtype
	     +"\" is an unknown bars-diagram subtype!\n" );
  STOP_DEBUG("draw_values");

  //Draw X and Y-axis
  barsdiagram->setcolor(@(diagram_data->axcolor));

  //Draw the X-axis
  if ((diagram_data->xminvalue<=LITET)&&
      (diagram_data->xmaxvalue>=-LITET))
    barsdiagram->
      polygone(make_polygon_from_line(diagram_data->linewidth, 
				      ({
					xpos_for_yaxis,
					diagram_data->ysize- ypos_for_xaxis,
					diagram_data->xsize-
					diagram_data->linewidth-labelx/2,  
					diagram_data->ysize-ypos_for_xaxis
				      }), 
				      1, 1)[0]);
  else
    if (diagram_data->xmaxvalue<-LITET)
    {
      barsdiagram
	->polygone(make_polygon_from_line(
                     diagram_data->linewidth, 
		     ({
		       xpos_for_yaxis,
		       diagram_data->ysize-ypos_for_xaxis,
		       
		       xpos_for_yaxis-4.0/3.0*si, 
		       diagram_data->ysize-ypos_for_xaxis,
		       
		       xpos_for_yaxis-si, 
		       diagram_data->ysize-ypos_for_xaxis-si/2.0,
		       xpos_for_yaxis-si/1.5,
		       diagram_data->ysize-ypos_for_xaxis+si/2.0,
		       
		       xpos_for_yaxis-si/3.0,
		       diagram_data->ysize-ypos_for_xaxis,
		       
		       diagram_data->xsize-diagram_data->linewidth
		       - labelx/2, 
		       diagram_data->ysize-ypos_for_xaxis
		     }), 1, 1)[0]);
    }
    else
      if (diagram_data->xminvalue>LITET)
	{
	barsdiagram
	  ->polygone(make_polygon_from_line(
                       diagram_data->linewidth, 
		       ({
			 xpos_for_yaxis,
			 diagram_data->ysize- ypos_for_xaxis,
			 
			 xpos_for_yaxis+si/3.0, 
			 diagram_data->ysize-ypos_for_xaxis,
			 
			 xpos_for_yaxis+si/1.5, 
			 diagram_data->ysize-ypos_for_xaxis-si/2.0,
			 xpos_for_yaxis+si, 
			 diagram_data->ysize-ypos_for_xaxis+si/2.0,
			 
			 xpos_for_yaxis+4.0/3.0*si, 
			 diagram_data->ysize-ypos_for_xaxis,
			 
			 diagram_data->xsize-diagram_data->linewidth
			 - labelx/2, 
			 diagram_data->ysize-ypos_for_xaxis
		       }), 1, 1)[0]);
      }
  
  //Draw arrow on the X-axis
  if (diagram_data->subtype=="line")
    barsdiagram->polygone( ({
      diagram_data->xsize-diagram_data->linewidth/2-(float)si-labelx/2,
      diagram_data->ysize-ypos_for_xaxis-(float)si/4.0,
      
      diagram_data->xsize-diagram_data->linewidth/2-labelx/2,
      diagram_data->ysize-ypos_for_xaxis,
      
      diagram_data->xsize-diagram_data->linewidth/2-(float)si-labelx/2,
      diagram_data->ysize-ypos_for_xaxis+(float)si/4.0
    })
			   );  
  
  //Draw Y-axis
  if ((diagram_data->yminvalue<=LITET)&&
      (diagram_data->ymaxvalue>=-LITET))
  {
    if  ((diagram_data->yminvalue<=LITET)&&
	 (diagram_data->yminvalue>=-LITET)) 
      barsdiagram->
	polygone(make_polygon_from_line(diagram_data->linewidth, 
					({
					  xpos_for_yaxis,
					  diagram_data->ysize-ypos_for_xaxis,
					  xpos_for_yaxis,
					  si+labely
					  }), 1, 1)[0]);
    else
      barsdiagram->
	polygone(make_polygon_from_line(diagram_data->linewidth, 
					({
					  xpos_for_yaxis,
					  diagram_data->ysize
					  - diagram_data->linewidth,
					  
					  xpos_for_yaxis,
					  si+labely
					  }), 1, 1)[0]);
    
  }
  else
    if (diagram_data->ymaxvalue<-LITET)
    {
      barsdiagram->
	polygone(make_polygon_from_line(
                   diagram_data->linewidth, 
		   ({
		     xpos_for_yaxis,
		     diagram_data->ysize-diagram_data->linewidth,
		     
		     xpos_for_yaxis,
		     diagram_data->ysize-ypos_for_xaxis+si*4.0/3.0,
		     
		     xpos_for_yaxis-si/2.0,
		     diagram_data->ysize-ypos_for_xaxis+si,
		     
		     xpos_for_yaxis+si/2.0,
		     diagram_data->ysize-ypos_for_xaxis+si/1.5,
		     
		     xpos_for_yaxis,
		     diagram_data->ysize-ypos_for_xaxis+si/3.0,
		     
		     xpos_for_yaxis,
		     si+labely
		   }), 1, 1)[0]);
    }
    else
      if (diagram_data->yminvalue>LITET)
      {
	barsdiagram->
	  polygone(make_polygon_from_line(
		     diagram_data->linewidth, 
		     ({
		       xpos_for_yaxis,
		       diagram_data->ysize-diagram_data->linewidth,
		       
		       xpos_for_yaxis,
		       diagram_data->ysize-ypos_for_xaxis-si/3.0,
		       
		       xpos_for_yaxis-si/2.0,
		       diagram_data->ysize-ypos_for_xaxis-si/1.5,
		       
		       xpos_for_yaxis+si/2.0,
		       diagram_data->ysize-ypos_for_xaxis-si,
		       
		       xpos_for_yaxis,
		       diagram_data->ysize-ypos_for_xaxis-si*4.0/3.0,
		       
		       xpos_for_yaxis,
		       si+labely
		     }), 1, 1)[0]);
      }
    
  //Draw arrow
  barsdiagram->
    polygone( ({
      xpos_for_yaxis-(float)si/4.0,
      diagram_data->linewidth/2.0+(float)si+labely,
				      
      xpos_for_yaxis,
      diagram_data->linewidth/2.0+labely,
	
      xpos_for_yaxis+(float)si/4.0,
      diagram_data->linewidth/2.0+(float)si+labely
    }) ); 

  //Write the text on the X-axis
  int s=sizeof(diagram_data->xnamesimg);

  START_DEBUG("text_on_axis");
  for(int i=0; i<s; i++)
    if ((diagram_data->values_for_xnames[i]<diagram_data->xmaxvalue)&&
	(diagram_data->values_for_xnames[i]>diagram_data->xminvalue))
    {
      barsdiagram->paste_alpha_color(
                    diagram_data->xnamesimg[i], 
		    @(diagram_data->textcolor), 
		    (int)floor((diagram_data->values_for_xnames[i]
				- diagram_data->xminvalue)*xmore+xstart
			       - diagram_data->xnamesimg[i]->xsize()/2), 
		    (int)floor(diagram_data->ysize-ypos_for_xaxis+si/4.0));
    }

  //Write the text on the Y-axis
  s=min(sizeof(diagram_data->ynamesimg), 
	sizeof(diagram_data->values_for_ynames));
  for(int i=0; i<s; i++)
    if ((diagram_data->values_for_ynames[i]<=diagram_data->ymaxvalue)&&
	(diagram_data->values_for_ynames[i]>=diagram_data->yminvalue))
    {
      barsdiagram->setcolor(@diagram_data->textcolor);
      barsdiagram->paste_alpha_color(
                     diagram_data->ynamesimg[i], 
		     @(diagram_data->textcolor), 
		     (int)floor(xpos_for_yaxis-
				si/4.0-diagram_data->linewidth-
				diagram_data->ynamesimg[i]->xsize()),
		     (int)floor(-(diagram_data->values_for_ynames[i]-
				  diagram_data->yminvalue)
				*ymore+diagram_data->ysize-ystart
				-
				diagram_data->ymaxynames/2));
      
      barsdiagram->setcolor(@diagram_data->axcolor);
      barsdiagram->
	polygone(make_polygon_from_line(
                   diagram_data->linewidth, 
		   ({
		     xpos_for_yaxis-si/4,
		     (-(diagram_data->values_for_ynames[i]
			- diagram_data->yminvalue)*ymore
		      + diagram_data->ysize-ystart),
		     
		     xpos_for_yaxis+si/4,
		     (-(diagram_data->values_for_ynames[i]
			- diagram_data->yminvalue)*ymore
		      + diagram_data->ysize-ystart)
		   }), 1, 1)[0]);
    }


  //Write labels ({xstorhet, ystorhet, xenhet, yenhet})
  if (diagram_data->labelsize)
  {
    barsdiagram
      ->paste_alpha_color(labelimg, 
			  @(diagram_data->labelcolor), 
			  diagram_data->xsize-labelx
			  - (int)ceil((float)diagram_data->linewidth),
			  diagram_data->ysize
			  - (int)ceil((float)(ypos_for_xaxis-si/2)));
      
    string label;
    int x;
    int y;

    if (diagram_data->labels[3] && sizeof(diagram_data->labels[3]))
      label=diagram_data->labels[1]+" ["+diagram_data->labels[3]+"]"; //Yquantity
    else
      label=diagram_data->labels[1];
    object notext=GETFONT(yaxisfont);
    if ((label!="")&&(label!=0))
      labelimg=notext
	->write(label)->scale(0,diagram_data->labelsize);
    else
      labelimg=Image.Image(diagram_data->labelsize,diagram_data->labelsize);
    
    if (labelimg->xsize()<1)
      labelimg=Image.Image(diagram_data->labelsize,diagram_data->labelsize);
    
    if (labelimg->xsize()>
	diagram_data->xsize)
      labelimg=labelimg->scale(diagram_data->xsize, 0);
      
    x=max(2,((int)floor((float)xpos_for_yaxis)-labelimg->xsize()/2));
    x=min(x, barsdiagram->xsize()-labelimg->xsize());
      
    y=0; 
      
    if (label && sizeof(label))
      barsdiagram->paste_alpha_color(labelimg, 
				     @(diagram_data->labelcolor), 
				     x,
				     2+labely-labelimg->ysize());
  }
  STOP_DEBUG("text_on_axis");

  diagram_data->ysize-=diagram_data->legend_size;
  diagram_data->image=barsdiagram;

  return diagram_data;
}

