/* This module contains low-level graphics functions
 */


int image_num;
mapping(string:array(string)) image_cache=([]);
mapping(string:string) illustration_cache=([]);


void save_image_cache()
{
  rm("illustration_cache");
  Stdio.write_file("illustration_cache",
		   encode_value( ({
		     image_num,
		     image_cache,
		     illustration_cache,
		     })));
}

void create()
{
  if(file_stat("illustration_cache"))
  {
    [image_num,image_cache,illustration_cache]=
      decode_value(Stdio.read_file("illustration_cache"));
  }
  werror("Illustration caches: %d %d\n",sizeof(image_cache), sizeof(illustration_cache));
}


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

  image_num++;
  string filename="illustration"+image_num+"."+ext;
  rm(filename);
  werror("Writing "+filename+".\n");
  Stdio.write_file(filename,data);

  if(image_cache[key])
    image_cache[key]+=({filename});
  else
    image_cache[key]=({filename});

  save_image_cache();

  return filename;
}


string mkgif(mixed o,void|mapping options)
{
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


string mkeps(mixed o,void|mapping options)
{
  if(!options) options=([]);
  string g=Image.PS.encode(o, options);
  return cached_write(g,"eps");
}

string mkpng(mixed o,void|mapping options)
{
  if(!options) options=([]);
  string g=Image.PNG.encode(o);
  return cached_write(g,"png");
}

mapping(string:Image.Image) srccache=([]);
Image.Image errimg=Image.image(10,10)->test();

Image.Image read_image(string file)
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
//    Process.system("/bin/sh -c 'gs -q -sDEVICE=pbmraw -r225 -g2500x2500 -sOutputFile=___tmp.ppm ___tmp.ps </dev/null >/dev/null'");

    Process.create_process(({"gs","-q","-sDEVICE=pbmraw","-r225","-g2500x2500","-sOutputFile=___tmp.ppm","___tmp.ps"}),(["stdin":Stdio.File("/dev/null","r"),"stdout":Stdio.File("/dev/null","w")]))->wait();
    object o=read_image("___tmp.ppm")->autocrop()->scale(1.0/3); //->rotate(-90);
    o=Image.image(o->xsize()+40, o->ysize()+40, 255,255,255)->paste(o,20,20);
    rm("___tmp.ps");
    rm("___tmp.ppm");
    if(o!=errimg) srccache[file]=o;
    return o;
  }

  string data=Stdio.read_file(file);
  if(!data || !strlen(data))
  {
    werror("\nFailed to read image %s\n\n",file);
    return srccache[file]=errimg;
  }
    
  catch  { return srccache[file]=Image.GIF.decode(data); };
  catch { return srccache[file]=Image.JPEG.decode(data); };
  catch  { return srccache[file]=Image.PNM.decode(data); };
  catch  { return srccache[file]=Image.PNG.decode(data); };
  werror("\nFailed to decode image %s\n\n",file);
  return srccache[file]=errimg;
}

string write_image(string fmt, Image.Image o, void|float pic_dpi)
{
  switch(fmt)
  {
    case "gif": return mkgif(o);
    case "jpg": return mkjpg(o);
    case "eps": return mkeps(o, (["dpi":pic_dpi ]));
    case "png": return mkpng(o);
    default:
      error("Unknown output format.\n");
  }
}

string gettext(string s)
{
  string tmp=(s/".")[-1];
  if(tmp=="jpeg") tmp="jpg";
  return tmp;
}


string convert(mapping params,
	       string wanted_formats,
	       void|float wanted_dpi,
	       void|string filter)
{
  if(params->__from__) werror("[%s]",params->__from__);
  if(!wanted_dpi) wanted_dpi=75.0;
  string input=params->src;
  array(string) tmp=input/"|";

// FIXME
//  if(params->scale) wanted_dpi*=(float)params->scale;

  array(float) dpi = (array(float)) ( (params->dpi || "75.0" )/"|" );
  if(sizeof(dpi) < sizeof(tmp))
    tmp+=({ tmp[-1] }) * ( sizeof(tmp) - sizeof(dpi) );

  mapping(string:string) ext_to_input=mkmapping(Array.map(tmp,gettext),tmp);
  mapping(string:float) ext_to_dpi=mkmapping(Array.map(tmp,gettext),dpi);

  if(!filter)
  {
    string best;
    float badness=100000.0;
    
    foreach(wanted_formats/"|", string fmt)
      {
	if(ext_to_input[fmt])
	{
	  float tmp=ext_to_dpi[fmt]-wanted_dpi;
	  if(tmp<0) tmp*=-5.0;
	  if(tmp < badness)
	  {
	    best=ext_to_input[fmt];
	    badness=tmp;
	  }
	}
      }
    if(best) return best;
  }

  array(int) mtimes=column(Array.map(tmp, file_stat)-({0}), 3);

  string key=encode_value( aggregate (
    input,
    dpi,
    mtimes,
    wanted_formats,
    wanted_dpi,
    filter));

  if(illustration_cache[key])
    return illustration_cache[key];


  string primary_format=(wanted_formats/"|")[0];


  // FIXME: check dpi!
  switch(primary_format)
  {
    case "eps":
      if(ext_to_input->fig)
      {
	Process.create_process( ({"fig2dev","-L","ps","-m","0.6666666666",ext_to_input->xfig,"___tmp.eps" }))->wait();
	return illustration_cache[key]=
	  cached_write(Stdio.read_file("___tmp.eps"),"eps");
      }
      break;

    case "tex":
      if(ext_to_input->fig)
      {
	Process.create_process( ({"fig2dev","-L","latex",ext_to_input->xfig,"___tmp.tex" }))->wait();
	return illustration_cache[key]=
	  cached_write(Stdio.read_file("___tmp.tex"),"tex");
      }
  }

  float pic_dpi=0.0;
  Image.Image o;
  for(int e=0;e<sizeof(tmp);e++)
  {
    if(dpi[e]<wanted_dpi) continue;
    if(o=read_image(tmp[e]))
    {
      pic_dpi=dpi[e];
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
      if(o=read_image(tmp[e]))
      {
	pic_dpi=dpi[e];
	break;
      }
    }
  }

  if(!o)
  {
    error("Failed to read image!\n");
  }



  if(filter)
  {
    mixed err=catch {
      o=compile_string("import Image;\n"
		       "mixed `()(object src) { "+filter+" ;}")()(o);
    };
    if(err)
    {
      werror("This code caused an error: \n%s\n",filter);
      throw(err);
    }
  }

  string ret;
  foreach(wanted_formats/"|", string tmp)
    {
      if(!catch {
	string ret=write_image(tmp, o, pic_dpi);
	}) break;
    }
  illustration_cache[key]=ret;
  save_image_cache();
  return ret;
}


