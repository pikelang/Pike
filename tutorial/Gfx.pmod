/* This module contains low-level graphics functions
 */


object image_cache=.Cache("illustration_cache");
object illustration_cache=.Cache("image_cache");



string cached_write(string data, string ext)
{
//  trace(1);
  string key=ext+"-"+Crypto.md5()->update(data)->digest();

//  werror("key=%O len=%d\n",key,strlen(data));

  foreach(image_cache[key] || ({}),string file)
    {
      if(Stdio.read_file(file)==data)
      {
	werror("Cache hit in cached_write: "+file+"\n");
	return file;
      }
    }
 
  int image_num=image_cache[0]++;
  mkdir("gfx");
  string filename="gfx/i"+image_num+"."+ext;
  rm(filename);
  werror("Writing "+filename+".\n");
  Stdio.write_file(filename,data);

  if(image_cache[key])
    image_cache[key]+=({filename});
  else
    image_cache[key]=({filename});

  return filename;
}


string mkgif(mixed o,void|mapping options)
{
  random_seed(0);
//  werror(master()->describe_backtrace(({"FOO\n",backtrace()})));
  if(!options) options=([]);
  string g=
     stringp(o)?o:
    options->alpha?Image.GIF.encode(o,options->alpha):
     Image.GIF.encode(o);

  return cached_write(g,"gif");
}

string mkjpg(mixed o,void|mapping options)
{
  if(!options) options=([]);
   string g=Image.JPEG.encode(o,options);
   return cached_write(g,"jpg");
}

#define PAPER_COLOUR 255,255,255

string low_make_eps(mixed o,void|mapping options)
{
}

string mkeps(mixed o,void|mapping options)
{
  if(stringp(o))
  {
    // Possible GIF animation (#*@&(*#&$
    return "error.eps";
  }
  if(!options) options=([]);
  if(options->alpha)
{
//    werror("\nFLALKAJF:LAKJF\n\n");
    object x=Image.Image( o->xsize(), o->ysize(), 255,255,255);
    x->paste_mask(o,options->alpha);
    o=x;
  }
  string g=Image.PS.encode(o, options);
  return cached_write(g,"eps");
}

string mkpdf(mixed o, void|mapping options)
{
  string eps=mkeps(o,options);
  Process.create_process( ({"epstopdf",eps }))->wait();
  array(string) tmp=eps/".";
  tmp[-1]="pdf";
  return tmp * ".";
}


string mkpng(mixed o,void|mapping options)
{
  random_seed(0);
  if(!options) options=([]);
  if(options->alpha)
  {
    // Make sure that transparent parts are white
    object x=Image.Image( o->xsize(), o->ysize(), 255,255,255);
    x->paste_mask(o,options->alpha);
    o=x;
  }
  string g=Image.PNG.encode(o,options);
  return cached_write(g,"png");
}

mapping(string:mapping) srccache=([]);
Image.Image errimg=Image.image(10,10)->test();

mapping read_image(string file, float|void wanted_dpi)
{
  if(!file) return 0;

//  werror("Reading %s ",file);
  if(file[0]!='_' && srccache[file])
  {
//    werror("(cached) ");
    return srccache[file];
  }

  if( (file/".")[-1]=="fig" )
  {
    Process.create_process(({"fig2dev","-L","ps",file,"___tmp.ps"}))->wait();
    Stdio.File("___tmp.ps","aw")->write("showpage\n");
    
    int res;
    int scale;
    switch(wanted_dpi)
    {
      case 0:
      case 0.0..: scale=3; wanted_dpi=75.0; break;
      case 10.0..:  scale=5; break;
      case 50.0..:  scale=3; break;
      case 150.0..: scale=2; break;
      case 300.0..: scale=1; break;
    }

    res=(int)(scale * wanted_dpi);
    int maxsize=11*res; // Max size = 11x11 inch

    rm("___tmp.ppm");
    werror("[rendering at %dx%d dpi] ",(int)wanted_dpi,scale);
#if 0
    Process.system("/bin/sh -c 'gs -q -sDEVICE=pbmraw -r"+res+" -g"+maxsize+"x"+maxsize+" -sOutputFile=___tmp.ppm ___tmp.ps </dev/null >/dev/null'");
#else
    int r=Process.create_process(({"gs","-q","-sDEVICE=pbmraw",
				   "-r"+res,sprintf("-g%dx%d",maxsize,maxsize),
				   "-sOutputFile=___tmp.ppm","___tmp.ps"}),
				 (["stdin":Stdio.File("/dev/null","r"),
				  "stdout":Stdio.File("/dev/null","w")]))->wait();

    if(r)
    {
      werror("Gs exited with return code %d.\n",r);
      exit(1);
    }
#endif
    object o=read_image("___tmp.ppm")->image;
    m_delete(srccache,"___tmp.ppm");
    o=o->autocrop()->scale(2.0/3/scale); //->rotate(-90);
    o=Image.image(o->xsize()+40, o->ysize()+40, PAPER_COLOUR)->paste(o,20,20);
    rm("___tmp.ps");
    rm("___tmp.ppm");
    // Not cached, too big..
    return (["image":o,"dpi":wanted_dpi]);
//    if(o!=errimg) srccache[file]=tmp;
//    return tmp;
  }

  string data=Stdio.read_file(file);
  if(!data || !strlen(data))
  {
    werror("\nFailed to read image %s\n\n",file);
    return srccache[file]=(["image":errimg]);
  }
    
  catch  {
    catch {
      Image.Image i,a;
      array chunks = Image.GIF._decode( data );
      
      // If there is more than one render chunk, the image is probably
      // an animation. Handling animations is left as an exercise for
      // the reader. :-)
      foreach(chunks, mixed chunk)
	if(arrayp(chunk) && chunk[0] == Image.GIF.RENDER )
	  [i,a] = chunk[3..4];
      if(i) return (["image":i,"alpha":a]);
    };
  };
  catch { return srccache[file]=(["image":Image.JPEG.decode(data)]); };
  catch  { return srccache[file]=(["image":Image.PNM.decode(data)]); };
  catch  { return srccache[file]=Image.PNG._decode(data); };
  werror("\nFailed to decode image %s\n\n",file);
  return srccache[file]=(["image":errimg]);
}

string write_image(string fmt, mapping opts)
{
}

string gettext(string s)
{
  string tmp=(s/".")[-1];
  if(tmp=="jpeg") tmp="jpg";
  return tmp;
}


array convret(string key,
	      string ret,
	      float dpi)
{
  array tmp=({ret, dpi});
  illustration_cache[key]=tmp;
  return tmp;
}

array convert(mapping params,
	     string wanted_formats,
	     void|float wanted_dpi,
	     void|string filter)
{
  if(!wanted_dpi) wanted_dpi=75.0;
  string input=params->src;
  array(string) tmp=input/"|";

// FIXME
//  if(params->scale) wanted_dpi*=(float)params->scale;

  array(float) dpi = (array(float)) ( (params->dpi || "75.0" )/"|" );
  if(sizeof(dpi) < sizeof(tmp))
    dpi+=({ dpi[-1] }) * ( sizeof(tmp) - sizeof(dpi) );

  mapping(string:string) ext_to_input=mkmapping(Array.map(tmp,gettext),tmp);
  mapping(string:float) ext_to_dpi=mkmapping(Array.map(tmp,gettext),dpi);

  if(!filter)
  {
    string best;
    float badness=100000.0;
    float best_dpi=0.0;
    
    foreach(wanted_formats/"|", string fmt)
      {
	if(ext_to_input[fmt])
	{
	  float tmp=ext_to_dpi[fmt]-wanted_dpi;
	  if(tmp<0) tmp*=-5.0;
	  if(tmp < badness)
	  {
	    best=ext_to_input[fmt];
	    best_dpi=ext_to_dpi[fmt];
	    badness=tmp;
	  }
	}
      }
    if(best)
    {
//      werror("convert not required: %s\n",best);
      return ({ best, best_dpi });
    }
  }

  werror("GFX: ");
  if(params->__from__) werror("[%s] ",params->__from__);
  array(int) mtimes=column(Array.map(tmp, file_stat)-({0}), 3);

  string key=encode_value( aggregate (
    input,
    dpi,
    mtimes,
    wanted_formats,
    wanted_dpi,
    filter));

  if(illustration_cache[key])
  {
    werror("(cached) %O\n",illustration_cache[key][0]||"Error");
    return illustration_cache[key];
  }


  foreach(wanted_formats/"|", string primary_format)
    {
      // FIXME: check dpi???
      switch(primary_format)
      {
	case "pdf":
	  if(ext_to_input->eps)
	  {
	    werror("eps->pdf ");
	    Process.create_process( ({"epstopdf","-o=___tmp.pdf",ext_to_input->eps}) )->wait();
	    return convret(key, 
			   cached_write(Stdio.read_file("___tmp.pdf"),"pdf"),
			   0.0);
	    break;
	  }

	case "eps":
	  if(ext_to_input->fig)
	  {
	    werror("fig->eps");
	    Process.create_process( ({"fig2dev","-L","ps","-m","0.6666666666",ext_to_input->fig,"___tmp.eps" }))->wait();


	    if(primary_format == "eps")
	    {
	      werror(" ");
	      return convret(key, 
			     cached_write(Stdio.read_file("___tmp.eps"),"eps"),
			     0.0);
	    }else{
	      werror("->pdf ");
	      Process.create_process( ({"epstopdf","___tmp.eps" }) )->wait();

	      return convret(key, 
			     cached_write(Stdio.read_file("___tmp.pdf"),"pdf"),
			     0.0);
	    }
	  }
	  break;

	  if(ext_to_input->fig)
	  {
	    werror("fig->eps ");
	    Process.create_process( ({"fig2dev","-L","ps","-m","0.6666666666",ext_to_input->fig,"___tmp.eps" }))->wait();
	    return convret(key, 
			   cached_write(Stdio.read_file("___tmp.eps"),"eps"),
			   0.0);
	  }
	  break;
	  
	case "tex":
	  if(ext_to_input->fig)
	  {
	    werror("fig->tex ");
	    Process.create_process( ({"fig2dev","-L","latex",ext_to_input->fig,"___tmp.tex" }))->wait();
	    return convret(key, 
			   cached_write(Stdio.read_file("___tmp.tex"),"tex"), 
			   0.0);
	  }
	  
	case "tpi":
	  if(ext_to_input->fig)
	  {
	    werror("fig->tpi ");
	    Process.create_process( ({"fig2dev","-L","eepic","-m","0.66666666666",ext_to_input->fig,"___tmp.tex" }))->wait();
	    
	    return convret(key, 
			   cached_write(Stdio.read_file("___tmp.tex"),"tpi"),
			   0.0);
	  }
      }
    }

  mapping o;
  for(int e=0;e<sizeof(tmp);e++)
  {
    if(dpi[e]<wanted_dpi) continue;
    if(o=read_image(tmp[e], wanted_dpi))
    {
      write("%s -> ",tmp[e]);
      if(!o->dpi) o->dpi=dpi[e];
      break;
    }
  }

  if(!o)
  {
    sort(dpi,tmp);
    tmp=reverse(tmp);
    dpi=reverse(dpi);

    for(int e=0;e<sizeof(tmp);e++)
    {
      if(o=read_image(tmp[e], wanted_dpi))
      {
	write("%s -> ",tmp[e]);
	if(!o->dpi) o->dpi=dpi[e];
	break;
      }
    }
  }

  if(!o || !o->image)
  {
    error("Failed to read image!\n");
  }


  random_seed(0);
  if(filter)
  {
    werror("running ");
    mixed err=catch {
      o=(["image":compile_string("import Image;\n"
		       "mixed `()(object src) { "+filter+" ;}")()(o->image)]);
    };
    if(err)
    {
      werror("This code caused an error: \n%s\n",filter);
      throw(err);
    }
  }

  string ret;
  foreach(wanted_formats/"|", string fmt)
    {
      mixed err=catch {
	switch(fmt)
	{
	  case "gif": ret=mkgif(o->image,o); break;
	  case "jpg": ret=mkjpg(o->image,o); break;
	  case "eps": ret=mkeps(o->image,o); break;
	  case "png": ret=mkpng(o->image,o); break;
	  case "pdf": ret=mkpdf(o->image,o); break;
	}
      };
      if(err) werror(master()->describe_backtrace(err));
      if(ret) break;
    }

  return convret(key, ret, o->dpi);
}


