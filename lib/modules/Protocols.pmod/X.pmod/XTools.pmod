/* XTools.pmod
 *
 * Various tools that are higher level than raw X, but are lower level
 * than widgets.
 */


/* Steals and processes mousebutton events */
class Button
{
  object window;
  constant style = 1;
  int pressed; // button is pressed
  int inside;  // pointer is inside window
  int button;  // The number of the mouse button used
  
  function(object, int, mapping:void) redraw_callback;
  function(object:void) clicked_callback;

  void button_exposed(mapping event)
  {
    redraw_callback(this_object(), pressed && (!style || inside), event);
  }
  
  mapping button_pressed(mapping event)
  {
    werror(sprintf("Button %d pressed.\n", event->detail));
    if (event->detail == button)
      {
	pressed = 1;
	inside = 1;
	redraw_callback(this_object(), 1, 0);
	
	return 0;
      }
    else
      return event;
  }

  mapping button_released(mapping event)
  {
    if (event->detail == button)
      {
	pressed = 0;
	redraw_callback(this_object(), 0, 0);
	if (inside)
	  clicked_callback(this_object());
	return 0;
      }
    else 
      return event;
  }

  mapping window_entered(mapping event)
  {
    inside = 1;
    if (pressed && style)
      redraw_callback(this_object(), 1, 0);
    return 0;
  }

  mapping window_left(mapping event)
  {
    inside = 0;
    if (pressed && style)
      redraw_callback(this_object(), 0, 0);
    return 0;
  }
  
  void create(object w, int|void b)
  {
    window = w;
    button = b || 1;

    window->SelectInput("Exposure",
			"ButtonPress", "ButtonRelease",
			"EnterWindow", "LeaveWindow");
    // window->GrabButton(button, 0, "EnterWindow", "LeaveWindow");
    window->set_event_callback("Expose", button_exposed);
    window->set_event_callback("ButtonPress", button_pressed);
    window->set_event_callback("ButtonRelease", button_released);
    window->set_event_callback("EnterNotify", window_entered);
    window->set_event_callback("LeaveNotify", window_left);
  }
}
  
class Uglier_button
{
  inherit Button;
  constant style = 0;
}
