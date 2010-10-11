#!/usr/local/bin/pike
object win, button, frame, vbox, cs, edit, edit2;
int not_now=0;

void cs_changed(int foo, object w)
{
  [float r, float g, float b, float a] = w->get_color();
  edit->set_text(sprintf("#%02x%02x%02x",
                         (int)(r*255+0.5),(int)(g*255+0.5),(int)(b*255+0.5)));
  edit2->set_text(sprintf("({ %d, %d, %d })",
                          (int)(r*255+0.5), (int)(g*255+0.5),
                          (int)(b*255+0.5)));
}

void edit_changed(int foo, object w)
{
  if(not_now)
  {
    not_now = 0;
    return;
  }
  
  string t = w->get_text();
  array c;
  c = array_sscanf(t, "#%02x%02x%02x");
  if(sizeof(c)!=3&&w==edit2)
    catch {
      c = compile_string("array foo() { return "+t+";}")()->foo();
    };
  if(sizeof(c) != 3) return;

  array cv = cs->get_color()[..2];
   if(equal(Array.map(cv, lambda(float c){return (int)(c*255+0.5);}), c))
    return;

   not_now = 1;
   cs->set_color( Array.map(c, `/, 255.0) );

  if(w == edit)
    edit2->set_text(sprintf("({ %d, %d, %d })", @c));
  else
    edit->set_text(sprintf("#%02x%02x%02x", @c));
}


int main()
{
  GTK.setup_gtk( "Ultra Colorselector++" );
  if(file_stat( getenv("HOME")+"/.pgtkrc" ))
    GTK.parse_rc( cpp(Stdio.read_bytes(getenv("HOME")+"/.pgtkrc")) );
  win = GTK.Window( GTK.WindowToplevel );

  edit = GTK.Entry();
  edit2 = GTK.Entry();
  cs = GTK.ColorSelection();

  cs->signal_connect("color_changed",cs_changed,0);
  ({edit,edit2})->signal_connect("changed",edit_changed,0);

  frame = GTK.Frame(  );
  vbox = GTK.Vbox(0,0);

  vbox->set_homogeneous( 0 );
  object hbox=GTK.Hbox( 0, 0 );
  vbox->add(cs);
  vbox->set_child_packing( cs, 1, 1, 0, 0 );
  vbox->add(hbox);
  vbox->set_child_packing( hbox, 0, 0, 0, 0 );

  hbox->add(edit);
  hbox->add(edit2);
  frame->add(vbox);
  win->add(frame);

  win->border_width(10);

  ({cs,edit,edit2,hbox,vbox,frame})->show();
  cs_changed(0,cs);
  cs->signal_connect("destroy",_exit,0);
  win->show();
  return -1;
}
