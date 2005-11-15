//! Graph sub-module for drawing pie-charts.
// $Id: create_pie.pike,v 1.11 2005/11/15 00:43:17 nilsson Exp $
//
// These functions were written by Henrik "Hedda" Wallin (hedda@roxen.com)
// Create_pie can draw pie charts in different forms.

#pike __REAL_VERSION__

#include "graph.h"

inherit .create_bars;

mapping(string:mixed) create_pie(mapping(string:mixed) diagram_data)
{
  //Only tested with xsize>=100
  int si=diagram_data["fontsize"];

  string where_is_ax;

  Image.Image piediagram;

  init_bg(diagram_data);
  piediagram=diagram_data["image"];
  setinitcolors(diagram_data);

  set_legend_size(diagram_data);

  diagram_data["ysize"]-=diagram_data["legend_size"];
  
  //Do the standard init (The init function is in create_graph)
  init(diagram_data);

  //Initiate values
  int|void  size=diagram_data["xsize"];
  array(int|float) numbers=diagram_data["data"][0];
  void | array(string) names=diagram_data["xnames"];
  void|int twoD=diagram_data["drawtype"]=="2D";
  void|array(array(int)) colors=diagram_data["datacolors"];
  array(int)bg=diagram_data["bgcolor"];
  array(int)fg=diagram_data["textcolor"];
  int tone=diagram_data["tone"];

  for(int i; i<sizeof(numbers); i++)
    if ((float)numbers[i]<0.0)
      numbers[i]*=-1.0;
  



  //////////////////////



  
  array(object) text;
  object notext;
  int ymaxtext;
  int xmaxtext;

  int imysize;
  int imxsize;
  array(float) arr=allocate(802);
  array(float) arr2=allocate(802);
  array(float) arr3=allocate(802);
  array(float) arrplus=allocate(802);
  array(float) arrpp=allocate(802);

  int yc;
  int xc;
  int xr;
  int yr;

  mixed sum;
  int sum2;

  array(int) pnumbers=allocate(sizeof(numbers));
  array(int) order=indices(numbers);

  int edge_nr=0;



  //create the text objects
  if (names)
    text=allocate(sizeof(names));
   
  if (diagram_data["3Ddepth"]>diagram_data["ysize"]/5)
    diagram_data["3Ddepth"]=diagram_data["ysize"]/5;
  
  notext=GETFONT(xnamesfont);
  if (names)
    if (notext)
      for(int i=0; i<sizeof(names); i++)
	{
	  if ((names[i]!=0) && (names[i]!=""))
	    text[i]=notext
	      ->write(UNICODE((string)(names[i]),diagram_data["encoding"]))
	      ->scale(0,diagram_data["fontsize"]);
	  else
	    text[i]=Image.Image(diagram_data["fontsize"],
				diagram_data["fontsize"]);

	  if (text[i]->xsize()<1)
	    text[i]=Image.Image(diagram_data["fontsize"],
				diagram_data["fontsize"]);

	  if (text[i]->xsize()>diagram_data["xsize"]/5+diagram_data["3Ddepth"])
	    text[i]=text[i]->scale((int)diagram_data["xsize"]/5, 0);

	  if (text[i]->ysize()>diagram_data["ysize"]/5-diagram_data["3Ddepth"])
	    text[i]=text[i]->scale(0, (int)diagram_data["ysize"]/5-
				   diagram_data["3Ddepth"]);
	  
	  if (xmaxtext<(text[i]->xsize()))
	    xmaxtext=text[i]->xsize();
	  if (ymaxtext<(text[i]->ysize()))
	    ymaxtext=text[i]->ysize();
	  
	}
    else
      error("Missing font or similar error!\n");

  int nameheight=write_name(diagram_data);

  //Some calculations
  if (twoD)
    {
      xc=diagram_data["xsize"]/2;
      yc=diagram_data["ysize"]/2+nameheight/2;
      xr=(int)min(xc-xmaxtext-ymaxtext-1-diagram_data["linewidth"], 
		  yc-2*ymaxtext-
		  1-diagram_data["linewidth"]-nameheight);
      yr=xr;
    }
  else
    {
      xc=diagram_data["xsize"]/2;
      yc=diagram_data["ysize"]/2-diagram_data["3Ddepth"]/2+nameheight/2;
      yr=(int)(min(xc-xmaxtext-ymaxtext-1-diagram_data["3Ddepth"]/2, 
		   yc-2*ymaxtext-1-nameheight)
	       -diagram_data["linewidth"]);
      xr=(int)(min(xc-xmaxtext-ymaxtext-1, 
		   yc+diagram_data["3Ddepth"]/2-
		   2*ymaxtext-1-nameheight)-
	       diagram_data["linewidth"]);
    }
  float w=diagram_data["linewidth"];

  if (xr<2)
    error("Image to small for this pie-diagram.\n"
	  "Try smaller font or bigger image!\n");

  //initiate the 0.25*% for different numbers:
  //Ex: If numbers is ({3,3}) pnumbers will be ({200, 200}) 
  sum=`+(@ numbers);
  int i;

  if (sum>LITET)
    {
      for(int i=0; i<sizeof(numbers); i++)
	{
	  float t=(float)(numbers[i]*400)/sum;
	  pnumbers[i]=(int)floor(t);
	  numbers[i]=t-floor(t);
	}
      
      
      //Now the rests are in the numbers-array
      //We now make sure that the sum of pnumbers is 400.
      sort(numbers, order);
      sum2=`+(@ pnumbers);
      i=sizeof(pnumbers);
      while(sum2<400)
	{
	  pnumbers[order[--i]]++;
	  sum2++;
	}  
    }
  else
    if (sizeof(numbers)>1)
      {
	for(int i=0; i<sizeof(numbers); i++)
	  pnumbers[i]=(int)floor(400.0/(float)sizeof(numbers));
	int j=400-`+(@pnumbers);
	for(int i=0; i<j; i++)
	  pnumbers[i]++;
      }
    else
      pnumbers=({400});
  
  //Initiate the piediagram!
  float FI=0;
  if (diagram_data["center"])
    {
      //If to great center integer is given, module is used. 
      // Center should not be greater than sizeof(data[0]).
      diagram_data["center"]%=(1+sizeof(numbers));
      FI=(400-`+(0,@pnumbers[0..diagram_data["center"]-2])
	-pnumbers[diagram_data["center"]-1]*0.5)*2.0*PI/400.0;
    }
  else
    if (diagram_data["rotate"])
      FI=((float)(diagram_data["rotate"])*2.0*PI/360.0)%(2*PI);
  float most_down=yc+yr+w;
  float most_right=xc+xr+w;
  float most_left=xc-xr-w;

  for(int i=0; i<401; i++)
    {
      arr[2*i]=xc+xr*sin((i*2.0*PI/400.0)+FI);
      arr[1+2*i]=yc+yr*sin(-PI/2+i*2.0*PI/400.0+FI);
      arr2[2*i]=xc+(xr+w)*sin((i*2.0*PI/400.0));
      arr2[2*i+1]=yc+(w+yr)*sin(-PI/2+i*2.0*PI/400.0);
      arr3[2*i]=xc+(xr+w)*sin((-i*2.0*PI/400.0+FI));
      arr3[2*i+1]=yc+(w+yr)*sin(-PI/2-i*2.0*PI/400.0+FI);
    }

  //Draw the slices
  if (sizeof(diagram_data["datacolors"])>
      sizeof(diagram_data["data"][0]))
    diagram_data["datacolors"]=diagram_data["datacolors"]
      [0..sizeof(diagram_data["data"][0])-1];
  
  int t=sizeof(diagram_data["datacolors"]);

  float miniwxr;
  float miniwyr;

  if (!twoD)
    {
      array arrfoo=copy_value(arr2);
      miniwxr=max((w+xr)/2,(w+xr)-diagram_data["3Ddepth"]/10);
      miniwyr=max((w+yr)/2,(w+yr)-diagram_data["3Ddepth"]/10);
      for(int i=200; i<604; i+=2)
	{
	  arrfoo[i]=xc+miniwxr*sin((i*PI/400.0));
	  arrfoo[i+1]=yc+miniwyr*sin(-PI/2+i*PI/400.0)+
	    diagram_data["3Ddepth"];
	}
      //arrfoo[i]=arr2[i]+diagram_data["3Ddepth"];
      for(int i=0; i<401; i++)
	{
	  arrplus[2*i]=xc+(xr+w)*sin((i*2.0*PI/400.0)+FI);
	  arrplus[1+2*i]=yc+(w+yr)*sin(-PI/2+i*2.0*PI/400.0+FI);
	  arrpp[2*i]=xc+miniwxr*sin((-i*2.0*PI/400.0)+FI);
	  arrpp[1+2*i]=yc+miniwyr*sin(-PI/2-i*2.0*PI/400.0+FI)+
	    diagram_data["3Ddepth"];
	}
      object skugg;
      skugg=Image.Image(piediagram->xsize(),piediagram->ysize(), 255,255,255);
      object foo;
      foo=Image.Image(piediagram->xsize(),piediagram->ysize(), 255,255,255);
      skugg->tuned_box(xc,yc-yr-1,xc+xr+1,1+yc+yr+diagram_data["3Ddepth"],  
		       ({			 
			 ({255,255,255}),
			 ({100,100,100}),
			 ({255,255,255}),
			 ({100,100,100})		      
		       }));
      skugg->tuned_box(xc-xr-1,yc-yr-1,xc,1+yc+yr+diagram_data["3Ddepth"],  
		       ({			 
			 ({100,100,100}),
			 ({255,255,255}),
			 ({100,100,100}),
			 ({255,255,255})
		       }));
      skugg->polyfill(arr2[600..801]+({xc,0,0,0}));
      skugg->polyfill(
		      arr2[..201]
		      +
		      ({piediagram->xsize()-1,0,xc,0})
		      );
      skugg->polyfill(
		      arr2[200..201]+
		      arrfoo[200..401]
		      +
		      ({xc,piediagram->ysize()-1,
			piediagram->xsize()-1,piediagram->ysize()-1})
		      );
      skugg->polyfill(
		      arrfoo[400..601]+
		      arr2[600..601]+
		      ({0,piediagram->ysize()-1,xc,piediagram->ysize()-1
		      })
		      );
      
      edge_nr=0;
      for(i=0; i<t; i++)
	{
	  piediagram->setcolor(
			       @diagram_data["datacolors"][i]
			       );
	  
	  if (pnumbers[i])
	    {
	      piediagram->
		polygone(
			 (arrplus[2*edge_nr..2*(edge_nr+pnumbers[i]+2)+1]
			  +
			   arrpp[800-2*(edge_nr+pnumbers[i]+2)..
				801-2*edge_nr])
			 );
	    }
	  edge_nr+=pnumbers[i];
	}
      piediagram=piediagram*skugg;

      piediagram->setcolor(0,0,0);
      piediagram->polyfill(
			   make_polygon_from_line(diagram_data["linewidth"],
						  ({
						    xc+(xr+w/2.0), 
						    yc})+
						  arrfoo[200..
							601]+
						  ({xc-(xr+w/2.0), 
						    yc}),
						  0,1)[0]);
    }
    
  edge_nr=0;
  for(i=0; i<t; i++)
    {
      piediagram=piediagram->setcolor(@diagram_data["datacolors"][i]);
      if (pnumbers[i])
	piediagram=piediagram->polyfill(({(float)xc,(float)yc})+
					arr[2*edge_nr..2*(edge_nr+pnumbers[i]+2)+1]);
      edge_nr+=pnumbers[i];
    }
  


  edge_nr=pnumbers[0];

  //black borders
  if (diagram_data["linewidth"]>LITET)
    {
      piediagram->setcolor(@diagram_data["axcolor"]);
      piediagram->polygone(
				      make_polygon_from_line(diagram_data["linewidth"],
							     ({
							       xc,
							       yc,
							       arr[0],
							       arr[1]
							     })
							     ,
							     0, 1)[0]
				      );

      for(int i=1; i<sizeof(pnumbers); i++)
	{
	  piediagram->
	    polygone(
		     make_polygon_from_line(diagram_data["linewidth"],
					    ({xc
					      ,yc,
					      arr[2*(edge_nr)],
					      arr[2*(edge_nr)+1]
					    })
					    ,
					    0, 1)[0]
		     );
	  
	  edge_nr+=pnumbers[i];
	}
      piediagram->polygone(arr[..401]
			   +arr3[400..]);
      piediagram->polygone(arr[400..]
			   +arr3[..401]);
    }
  
  piediagram->setcolor(255,255,255);
  
  //And now some shading!
  if (!twoD)
    {
      object below;
      array(int) b=({70,70,70});
      array(int) a=({0,0,0});
      
      
      object tbild;

      int imxsize=piediagram->xsize(); //diagram_data["xsize"];
      int imysize=piediagram->ysize(); //diagram_data["ysize"]+diagram_data["legendsize"];

      if(tone)
	{
	  
	  
	  tbild=Image.Image(imxsize, imysize, 255, 255, 255)->
	    tuned_box(0, 0 , 1, imysize,
		      ({a,a,b,b}));
	  tbild=tbild->paste(tbild->copy(0,0,0, imysize), 1, 0);
	  tbild=tbild->paste(tbild->copy(0,0,1, imysize), 2, 0);
	  tbild=tbild->paste(tbild->copy(0,0,3, imysize), 4, 0);
	  tbild=tbild->paste(tbild->copy(0,0,7, imysize), 8, 0);
	  tbild=tbild->paste(tbild->copy(0,0,15, imysize), 16, 0);
	  if (imxsize>32)
	    tbild=tbild->paste(tbild->copy(0,0,31, imysize), 32, 0);
      
	  if (imxsize>64)
	    tbild->
	      paste(tbild->copy(0,0,63, imysize), 64, 0);
	  if (imxsize>128)
	    tbild=tbild->paste(tbild->copy(0,0,128, imysize), 128, 0);
	  if (imxsize>256)
	    tbild=tbild->paste(tbild->copy(0,0,256, imysize), 256, 0);
	  if (imxsize>512)
	    tbild=tbild->paste(tbild->copy(0,0,512, imysize), 512, 0);
	  piediagram+=tbild;
	}

      //Vertical lines below
      edge_nr=(int)(FI*200.0/PI+0.5);
      piediagram->setcolor(0,0,0);
      for(int i=0; i<sizeof(pnumbers); i++)
	{
	  if (sin(-PI/2+edge_nr*2.0*PI/400.0)>0)
	    {
	      
	      float x1=xc+(xr+w/2.0)*sin((edge_nr*2.0*PI/400.0));
	      float y1=yc+(w/2.0+yr)*sin(-PI/2+edge_nr*2.0*PI/400.0);
	      float x2=xc+(miniwxr-w/2.0)*sin((edge_nr*2.0*PI/400.0));
	      float y2=yc+diagram_data["3Ddepth"]
		+(miniwyr-w/2.0)*sin(-PI/2+edge_nr*2.0*PI/400.0);
	      piediagram=piediagram->
		polygone(
			 make_polygon_from_line(diagram_data["linewidth"],
						({
						  x1,y1,
						  x2,y2
						})
						,
						0, 1)[0]
			 );
	      
	    }
	  edge_nr+=pnumbers[i];
	}
      
    }

  
  //write the text!
  int|float place;
  sum=0;
  if (names)
    for(int i=0; i<min(sizeof(pnumbers), sizeof(text)); i++)
      {
	int t;
	sum+=pnumbers[i];
	place=sum-pnumbers[i]/2;
	if (FI)
	  {
	    place=place+FI*400.0/(2.0*PI);
	    if (place>400)
	      place=place%400;
	  }
	piediagram=piediagram->setcolor(255,0, 0);

	
	t=(place<202)?0:-text[i]->xsize();
	int yt=0;
	if (((place>100)&&(place<300))&&
	    (!twoD))
	  yt=diagram_data["3Ddepth"];
	
	int ex=text[i]->ysize();
	int x=(int)(xc +t+ (xr+ex)*sin(place*PI*2.0/400.0+0.0001));
	int y=(int)(-text[i]->ysize()/2+yc +yt+ 
		    (yr+ex)*sin(-PI/2.0+place*PI*2.0/400.0+0.0001));
      
	piediagram=piediagram->paste_alpha_color(text[i], @fg, x, y);
      }

  diagram_data["ysize"]-=diagram_data["legend_size"];
  diagram_data["image"]=piediagram;
  return diagram_data;
}
